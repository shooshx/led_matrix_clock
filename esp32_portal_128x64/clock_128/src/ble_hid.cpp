// BLE HID host: find a BLE keyboard / remote / air-mouse, pair (bond) with it,
// subscribe to its HID input reports and decode key presses, buttons, consumer keys
// and movement by parsing the device's HID report descriptor.
// Based on esp32_portal_tests/test_protomatter/src/ble.cpp
//
// The BLE connection is managed in a separate task, the decoded input is passed to loop()
// through a queue as HidEvent

#include <Arduino.h>
#include <NimBLEDevice.h>
#include <Preferences.h>
#include <map>
#include <set>
#include <vector>
#include "ble_hid.h"

// ---- configuration ----
// Leave both empty to auto-connect to the bonded devices, and while none is connected the first device advertising
// as HID. Otherwise set one of them to pick a specific device. New devices are paired from the admin view.
static const char *TARGET_ADDR = "";   // e.g. "aa:bb:cc:dd:ee:ff"
static const char *TARGET_NAME = "";   // substring of the advertised name
// If the device asks for a passkey (typical for keyboards) type this on it and press Enter.
static const uint32_t PASSKEY = 123456;
static const bool PRINT_RAW = false;   // hex dump every report
static const bool LOG_TIMING = true;   // times of presses, releases and movement starting after a pause

static const uint32_t REPEAT_DELAY_MS = 500;  // auto-repeat of a held key
static const uint32_t REPEAT_RATE_MS = 70;

// ---- HID report descriptor parsing ----

struct HidField {
    uint16_t bitOffset;
    uint8_t size, count;
    bool isArray, isConst, isRelative;
    int32_t logMin, logMax;
    uint32_t usageMin, usageMax;   // (page << 16) | usage
    std::vector<uint32_t> usages;
};

struct HidReport {
    std::vector<HidField> fields;
    std::set<uint32_t> prevActive;
};

typedef std::map<uint8_t, HidReport> HidReports;   // input reports by report ID (0 = no ID), of a device

static int32_t itemSigned(uint32_t v, uint8_t n) {
    if (n == 1) return (int8_t)v;
    if (n == 2) return (int16_t)v;
    return (int32_t)v;
}

// verbose: logs each input field, when pairing
static void parseReportMap(HidReports &reports, const uint8_t *d, size_t len, bool verbose)
{
    struct Globals { uint16_t page = 0; int32_t logMin = 0, logMax = 0; uint8_t size = 0, count = 0, id = 0; };
    Globals g;
    std::vector<Globals> stack;
    std::vector<uint32_t> usages;
    uint32_t umin = 0, umax = 0;
    bool haveRange = false;
    std::map<uint8_t, uint16_t> offsets;

    size_t i = 0;
    while (i < len) {
        uint8_t b = d[i++];
        if (b == 0xFE) {   // long item, skip
            if (i + 1 < len) i += 2 + d[i];
            continue;
        }
        uint8_t n = b & 3;
        if (n == 3) n = 4;
        uint32_t v = 0;
        for (uint8_t k = 0; k < n && i < len; ++k)
            v |= (uint32_t)d[i++] << (8 * k);
        auto fullUsage = [&](uint32_t u) { return n == 4 ? u : ((uint32_t)g.page << 16) | u; };

        switch (b & 0xFC) {
        // global
        case 0x04: g.page = v; break;
        case 0x14: g.logMin = itemSigned(v, n); break;
        case 0x24: g.logMax = itemSigned(v, n); if (g.logMax < g.logMin) g.logMax = v; break;
        case 0x74: g.size = v; break;
        case 0x84: g.id = v; break;
        case 0x94: g.count = v; break;
        case 0xA4: stack.push_back(g); break;
        case 0xB4: if (!stack.empty()) { g = stack.back(); stack.pop_back(); } break;
        // local
        case 0x08: usages.push_back(fullUsage(v)); break;
        case 0x18: umin = fullUsage(v); haveRange = true; break;
        case 0x28: umax = fullUsage(v); haveRange = true; break;
        // main
        case 0x80: {  // Input
            HidField f;
            f.bitOffset = offsets[g.id];
            f.size = g.size;
            f.count = g.count;
            f.isConst = v & 1;
            f.isArray = !(v & 2);
            f.isRelative = v & 4;
            f.logMin = g.logMin;
            f.logMax = g.logMax;
            f.usages = usages;
            if (haveRange) { f.usageMin = umin; f.usageMax = umax; }
            else if (!usages.empty()) { f.usageMin = usages.front(); f.usageMax = usages.back(); }
            else { f.usageMin = f.usageMax = 0; }
            offsets[g.id] += g.size * g.count;
            reports[g.id].fields.push_back(f);
        }   // fallthrough: main items clear locals
        case 0x90: case 0xA0: case 0xB0: case 0xC0:
            usages.clear();
            haveRange = false;
            umin = umax = 0;
            break;
        }
    }

    for (auto &r : reports) {
        Serial.printf("HID report id %d: %d fields\n", r.first, (int)r.second.fields.size());
        if (!verbose)
            continue;
        for (auto &f : r.second.fields) {
            if (f.isConst) continue;
            Serial.printf("    @%3d %2dx%-2d %s%s usage %04x:%04x..%04x log %d..%d\n",
                          f.bitOffset, f.count, f.size, f.isArray ? "array" : "var  ",
                          f.isRelative ? " rel" : "    ", (unsigned)(f.usageMin >> 16), (unsigned)(f.usageMin & 0xFFFF),
                          (unsigned)(f.usageMax & 0xFFFF), (int)f.logMin, (int)f.logMax);
        }
    }
}

// ---- usage translation ----

static const char *KEY_CHARS  = "abcdefghijklmnopqrstuvwxyz1234567890";
static const char *KEY_SHIFT  = "ABCDEFGHIJKLMNOPQRSTUVWXYZ!@#$%^&*()";
static const char *PUNCT      = "-=[]\\#;'`,./";
static const char *PUNCT_SHIFT = "_+{}|~:\"~<>?";

// ---- usage names, for the log ----

static String keyName(uint16_t u)
{
    static const char *mods[] = {"LCtrl", "LShift", "LAlt", "LGui", "RCtrl", "RShift", "RAlt", "RGui"};
    static const char *nav[] = {"PrintScr", "ScrollLock", "Pause", "Insert", "Home", "PgUp", "Delete",
                                "End", "PgDn", "Right", "Left", "Down", "Up"};
    if (u >= 0x04 && u <= 0x27) return String(KEY_CHARS[u - 0x04]);
    if (u >= 0x2D && u <= 0x38) return String(PUNCT[u - 0x2D]);
    if (u >= 0x3A && u <= 0x45) return "F" + String(u - 0x3A + 1);
    if (u >= 0x46 && u <= 0x52) return nav[u - 0x46];
    if (u >= 0xE0 && u <= 0xE7) return mods[u - 0xE0];
    switch (u) {
    case 0x28: return "Enter";
    case 0x29: return "Esc";
    case 0x2A: return "Backspace";
    case 0x2B: return "Tab";
    case 0x2C: return "Space";
    case 0x39: return "CapsLock";
    case 0x58: return "KpEnter";
    case 0x65: return "Menu";
    case 0x66: return "Power";
    case 0x7F: return "Mute";
    case 0x80: return "Volume+";
    case 0x81: return "Volume-";
    }
    return "Key";
}

static String consumerName(uint16_t u)
{
    switch (u) {
    case 0x30: return "Power";
    case 0x40: return "Menu";
    case 0x41: return "MenuPick";
    case 0x42: return "MenuUp";
    case 0x43: return "MenuDown";
    case 0x44: return "MenuLeft";
    case 0x45: return "MenuRight";
    case 0x46: return "MenuEscape";
    case 0x6F: return "Brightness+";
    case 0x70: return "Brightness-";
    case 0xB0: return "Play";
    case 0xB1: return "Pause";
    case 0xB3: return "FastForward";
    case 0xB4: return "Rewind";
    case 0xB5: return "NextTrack";
    case 0xB6: return "PrevTrack";
    case 0xB7: return "Stop";
    case 0xCD: return "Play/Pause";
    case 0xE2: return "Mute";
    case 0xE9: return "Volume+";
    case 0xEA: return "Volume-";
    case 0x221: return "Search";
    case 0x223: return "Home";
    case 0x224: return "Back";
    case 0x225: return "Forward";
    }
    return "Consumer";
}

static String usageName(uint32_t full)
{
    uint16_t page = full >> 16, u = full & 0xFFFF;
    switch (page) {
    case 0x01:
        switch (u) {
        case 0x81: return "SysPowerDown";
        case 0x82: return "SysSleep";
        case 0x83: return "SysWake";
        case 0x90: return "DpadUp";
        case 0x91: return "DpadDown";
        case 0x92: return "DpadRight";
        case 0x93: return "DpadLeft";
        }
        break;
    case 0x07: return keyName(u);
    case 0x09: return "Button" + String(u);
    case 0x0C: return consumerName(u);
    }
    return "Usage";
}

// usages reported as values (movement etc.) rather than on/off
static bool isAxis(uint32_t full)
{
    uint16_t page = full >> 16, u = full & 0xFFFF;
    return (page == 0x01 && u >= 0x30 && u <= 0x39) || (page == 0x0C && u == 0x238);
}

// keyboard usage equivalent of a key from any HID page, 0 if none
static uint16_t toKey(uint32_t full)
{
    uint16_t page = full >> 16, u = full & 0xFFFF;
    switch (page) {
    case 0x07:
        if (u == 0x58) return HID_KEY_ENTER;  // keypad enter
        if (u == 0x66) return HID_KEY_POWER;
        return u;
    case 0x01:   // generic desktop d-pad
        switch (u) {
        case 0x81: return HID_KEY_POWER;  // SysPowerDown
        case 0x90: return HID_KEY_UP;
        case 0x91: return HID_KEY_DOWN;
        case 0x92: return HID_KEY_RIGHT;
        case 0x93: return HID_KEY_LEFT;
        }
        break;
    case 0x09:   // buttons, the main button of an air-mouse is its OK key
        if (u == 1) return HID_KEY_ENTER;
        break;
    case 0x0C:   // consumer
        switch (u) {
        case 0x41: return HID_KEY_ENTER;   // MenuPick
        case 0x42: return HID_KEY_UP;      // MenuUp
        case 0x43: return HID_KEY_DOWN;    // MenuDown
        case 0x44: return HID_KEY_LEFT;    // MenuLeft
        case 0x45: return HID_KEY_RIGHT;   // MenuRight
        case 0x46: return HID_KEY_ESC;     // MenuEscape
        case 0x224: return HID_KEY_BACK;
        case 0x30: return HID_KEY_POWER;
        case 0x223: return HID_KEY_REMOTE_HOME;
        case 0xE9: return HID_KEY_VOL_UP;
        case 0xEA: return HID_KEY_VOL_DOWN;
        }
        break;
    }
    return 0;
}

// caps lock inverts shift for letters only
static char toChar(uint32_t full, bool shift, bool caps)
{
    if ((full >> 16) != 0x07)
        return 0;
    uint16_t k = full & 0xFFFF;
    if (k >= 0x04 && k <= 0x1D) return ((shift != caps) ? KEY_SHIFT : KEY_CHARS)[k - 0x04];  // a-z
    if (k >= 0x1E && k <= 0x27) return (shift ? KEY_SHIFT : KEY_CHARS)[k - 0x04];            // 1-0
    if (k >= 0x2D && k <= 0x38) return (shift ? PUNCT_SHIFT : PUNCT)[k - 0x2D];
    if (k == 0x2C) return ' ';
    return 0;
}

// ---- report decoding ----

static QueueHandle_t g_queue = nullptr;
// the last press / release, like in the serial log, for the admin view
static char g_last_key[48] = "";
static volatile uint32_t g_last_key_count = 0;
static volatile bool g_last_key_pressed = false;
static volatile bool g_caps_lock = false;
static volatile bool g_disconnected = false;

static void pushEvent(const HidEvent &ev)
{
    if (xQueueSend(g_queue, &ev, 0) != pdTRUE)
        Serial.println("HID event queue full");
}

static uint32_t getBits(const uint8_t *d, size_t len, uint32_t off, uint8_t n)
{
    uint32_t v = 0;
    for (uint8_t i = 0; i < n && i < 32; ++i) {
        uint32_t bit = off + i;
        if (bit / 8 >= len) break;
        if ((d[bit / 8] >> (bit % 8)) & 1) v |= 1u << i;
    }
    return v;
}

static void decodeReport(HidReports &reports, uint8_t id, const uint8_t *d, size_t len)
{
    if (PRINT_RAW) {
        Serial.printf("[id %d]", id);
        for (size_t i = 0; i < len; ++i) Serial.printf(" %02x", d[i]);
        Serial.println();
    }

    auto it = reports.find(id);
    if (it == reports.end()) return;
    HidReport &rep = it->second;

    std::set<uint32_t> active;
    int32_t dx = 0, dy = 0;

    for (auto &f : rep.fields) {
        if (f.isConst) continue;
        for (uint8_t i = 0; i < f.count; ++i) {
            uint32_t raw = getBits(d, len, f.bitOffset + i * f.size, f.size);
            int32_t val = raw;
            if (f.logMin < 0 && f.size < 32 && (raw & (1u << (f.size - 1))))
                val = (int32_t)(raw | (~0u << f.size));

            if (f.isArray) {
                if (val == 0 || val < f.logMin || val > f.logMax) continue;   // 0 = no key
                uint32_t idx = val - f.logMin;
                uint32_t usage = idx < f.usages.size() ? f.usages[idx] : f.usageMin + idx;
                active.insert(usage);
            } else {
                uint32_t usage = i < f.usages.size() ? f.usages[i]
                               : (f.usageMin ? f.usageMin + i : (f.usages.empty() ? 0 : f.usages.back()));
                if (isAxis(usage)) {
                    if (f.isRelative && usage == 0x10030) dx = val;
                    if (f.isRelative && usage == 0x10031) dy = val;
                } else if (val != 0) {
                    active.insert(usage);
                }
            }
        }
    }

    bool shift = active.count(0x700E1) || active.count(0x700E5);
    // the keyboard only reports the key, the caps lock state is kept here. toggled before the chars
    // of the same report are made
    if (active.count(0x70039) && !rep.prevActive.count(0x70039))
        g_caps_lock = !g_caps_lock;
    for (uint32_t u : active) {
        if (rep.prevActive.count(u)) continue;
        HidEvent ev;
        ev.type = HidEvent::KEY_DOWN;
        ev.usage = u;
        ev.key = toKey(u);
        ev.ch = toChar(u, shift, g_caps_lock);
        pushEvent(ev); // before printing so that a slow serial doesn't delay it
        snprintf(g_last_key, sizeof(g_last_key), "%04x:%04x %s", (unsigned)(u >> 16), (unsigned)(u & 0xFFFF), usageName(u).c_str());
        g_last_key_pressed = true;
        ++g_last_key_count;
        String t = LOG_TIMING ? "  t=" + String(millis()) : String();
        if (ev.ch != 0)
            Serial.printf("HID press %04x:%04x %s  typed '%c'%s\n", u >> 16, u & 0xFFFF, usageName(u).c_str(), ev.ch, t.c_str());
        else
            Serial.printf("HID press %04x:%04x %s%s\n", u >> 16, u & 0xFFFF, usageName(u).c_str(), t.c_str());
    }
    for (uint32_t u : rep.prevActive) {
        if (active.count(u)) continue;
        HidEvent ev;
        ev.type = HidEvent::KEY_UP;
        ev.usage = u;
        ev.key = toKey(u);
        pushEvent(ev);
        snprintf(g_last_key, sizeof(g_last_key), "%04x:%04x %s", (unsigned)(u >> 16), (unsigned)(u & 0xFFFF), usageName(u).c_str());
        g_last_key_pressed = false;
        ++g_last_key_count;
        if (LOG_TIMING)
            Serial.printf("HID release %04x:%04x %s  t=%lu\n", u >> 16, u & 0xFFFF, usageName(u).c_str(), millis());
    }
    rep.prevActive = active;

    if (dx != 0 || dy != 0) {
        HidEvent ev;
        ev.type = HidEvent::MOVE;
        ev.dx = dx;
        ev.dy = dy;
        pushEvent(ev);
        static uint32_t last_move = 0;
        uint32_t now = millis();
        if (LOG_TIMING && now - last_move >= 100)
            Serial.printf("HID move resumed after %lu ms  t=%lu\n", now - last_move, now);
        last_move = now;
    }
}

// ---- BLE ----

static const NimBLEUUID HID_SERVICE((uint16_t)0x1812);
static const NimBLEUUID HID_REPORT_MAP((uint16_t)0x2A4B);
static const NimBLEUUID HID_REPORT((uint16_t)0x2A4D);
static const NimBLEUUID HID_REPORT_REF((uint16_t)0x2908);
static const NimBLEUUID BOOT_KBD_IN((uint16_t)0x2A22);
static const NimBLEUUID BOOT_MOUSE_IN((uint16_t)0x2A33);
static const NimBLEUUID GAP_SERVICE((uint16_t)0x1800);
static const NimBLEUUID GAP_DEVICE_NAME((uint16_t)0x2A00);

static const int MAX_DEVICES = CONFIG_BT_NIMBLE_MAX_CONNECTIONS;  // connected at the same time
static const uint32_t SCAN_SECONDS = 5;
static const uint32_t PAIR_SCAN_SECONDS = 10;     // then restarted, so that what changed is reported again
static const uint32_t BG_SCAN_PERIOD_MS = 30000;  // for bonded devices that aren't there while others are connected
static const uint32_t FOUND_TIMEOUT_MS = 30000;   // a found device not seen for this long isn't listed
static const size_t MAX_FOUND = 80;               // phones change their address all the time

// a connected device
struct HidConn {
    NimBLEClient *client = nullptr;
    HidReports reports;
    std::map<uint16_t, uint8_t> handleToReportId;
    char name[32] = "";
    volatile bool ready = false;   // subscribed, its input is decoded
};
static HidConn g_conns[MAX_DEVICES];

// the device to connect to next, set by the scan or by ble_hid_pair()
static NimBLEAddress g_target_addr;
static std::string g_target_name;
static volatile bool g_doConnect = false;  // a bonded device (or the first HID one) seen by the scan
static volatile bool g_doPair = false;     // from ble_hid_pair(): verbose, and from a new bond

static volatile bool g_pairing = false;
static volatile bool g_restartScan = false;
static char g_pair_status[64] = "";

// what the scan found in pairing mode
struct Found {
    NimBLEAddress addr;
    std::string name;
    int rssi = 0;
    bool hid = false;
    uint32_t seen = 0;  // millis
};
static std::vector<Found> g_found;   // in the order found
static SemaphoreHandle_t g_found_mutex = nullptr;

static void setStatus(const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    vsnprintf(g_pair_status, sizeof(g_pair_status), fmt, args);
    va_end(args);
    Serial.printf("BLE pair: %s\n", g_pair_status);
}

// the names of the devices by their address, in NVS. the bonds NimBLE keeps don't have them,
// a device that is bonded but not around is known only by its address otherwise
static const char *NAMES_NVS = "ble_names";

static String nameKey(const NimBLEAddress &addr)  // NVS keys are up to 15 chars, the address without the ':'
{
    String k = addr.toString().c_str();
    k.replace(":", "");
    return k;
}

static void saveName(const NimBLEAddress &addr, const char *name)
{
    Preferences p;
    if (!p.begin(NAMES_NVS, false))
        return;
    String k = nameKey(addr);
    if (p.getString(k.c_str(), "") != name)  // not written again when it's the same
        p.putString(k.c_str(), name);
    p.end();
}

static String loadName(const NimBLEAddress &addr)
{
    Preferences p;
    if (!p.begin(NAMES_NVS, false))  // not read-only, that fails before the first name was saved
        return "";
    String name = p.getString(nameKey(addr).c_str(), "");
    p.end();
    return name;
}

static void forgetName(const NimBLEAddress &addr)
{
    Preferences p;
    if (!p.begin(NAMES_NVS, false))
        return;
    p.remove(nameKey(addr).c_str());
    p.end();
}

static bool isHidAppearance(uint16_t a)
{
    return (a >= 0x03C0 && a <= 0x03CF)    // HID category (keyboard, mouse, gamepad...)
        || (a >= 0x0180 && a <= 0x01BF);   // remote control category
}

static bool isConnectedTo(const NimBLEAddress &addr)
{
    for (auto &c : g_conns)
        if (c.client && c.client->isConnected() && c.client->getPeerAddress() == addr)
            return true;
    return false;
}

static int connectedCount()
{
    int n = 0;
    for (auto &c : g_conns)
        if (c.client && c.client->isConnected())
            ++n;
    return n;
}

// a random address that changes (resolvable / non-resolvable) can't be recognized as bonded unless it's resolved
static const char *addrTypeName(const NimBLEAddress &addr)
{
    switch (addr.getType()) {
    case BLE_ADDR_PUBLIC: return "public";
    case BLE_ADDR_PUBLIC_ID: return "public id";
    case BLE_ADDR_RANDOM_ID: return "random id";
    case BLE_ADDR_RANDOM:
        switch (addr.getNative()[5] >> 6) {  // the top 2 bits of the most significant byte
        case 3: return "random static";
        case 1: return "random resolvable";
        case 0: return "random non-resolvable";
        }
        return "random ?";
    }
    return "?";
}

static String hexStr(const uint8_t *d, size_t len)
{
    String r;
    char b[4];
    for (size_t i = 0; i < len; ++i) {
        snprintf(b, sizeof(b), "%02x", d[i]);
        r += b;
    }
    return r;
}

static void logAdvert(NimBLEAdvertisedDevice *dev, bool hid, bool bonded)
{
    NimBLEAddress addr = dev->getAddress();
    String line = String("BLE pair scan: ") + addr.toString().c_str() + " (" + addrTypeName(addr) + ")";
    line += " rssi " + String(dev->getRSSI());
    line += dev->isConnectable() ? " connectable" : " NOT-connectable";
    line += " advType " + String(dev->getAdvType());
    line += String(" name '") + dev->getName().c_str() + "'";
    if (dev->haveAppearance()) {
        char b[24];
        snprintf(b, sizeof(b), " appearance 0x%04x", dev->getAppearance());
        line += b;
    }
    for (int i = 0; i < dev->getServiceUUIDCount(); ++i)
        line += String(" svc ") + dev->getServiceUUID(i).toString().c_str();
    for (int i = 0; i < dev->getManufacturerDataCount(); ++i) {
        std::string m = dev->getManufacturerData((uint8_t)i);
        line += " mfr " + hexStr((const uint8_t *)m.data(), m.length());
    }
    if (hid) line += " [HID]";
    if (bonded) line += " [bonded]";
    Serial.println(line);
}

// adds or updates it in g_found. returns true if it's new or its name changed, then it's logged
static bool addFound(NimBLEAdvertisedDevice *dev, bool hid)
{
    NimBLEAddress addr = dev->getAddress();
    std::string name = dev->getName();
    bool changed = false;
    xSemaphoreTake(g_found_mutex, portMAX_DELAY);
    Found *f = nullptr;
    for (auto &e : g_found)
        if (e.addr == addr)
            f = &e;
    if (f == nullptr) {
        if (g_found.size() >= MAX_FOUND) {  // the one not seen for the longest makes room
            size_t oldest = 0;
            for (size_t i = 1; i < g_found.size(); ++i)
                if (g_found[i].seen < g_found[oldest].seen)
                    oldest = i;
            g_found.erase(g_found.begin() + oldest);
        }
        g_found.push_back(Found());
        f = &g_found.back();
        f->addr = addr;
        changed = true;
    }
    else if (!name.empty() && f->name != name)
        changed = true;
    if (!name.empty())
        f->name = name;  // only some of the advertisements (the scan responses) have it
    f->rssi = dev->getRSSI();
    f->hid = f->hid || hid;
    f->seen = millis();
    xSemaphoreGive(g_found_mutex);
    return changed;
}

class ScanCallbacks : public NimBLEAdvertisedDeviceCallbacks {
    void onResult(NimBLEAdvertisedDevice *dev) override {
        bool hid = dev->isAdvertisingService(HID_SERVICE) ||
                   (dev->haveAppearance() && isHidAppearance(dev->getAppearance()));
        NimBLEAddress addr = dev->getAddress();
        bool bonded = NimBLEDevice::isBonded(addr);
        if (g_pairing && addFound(dev, hid))
            logAdvert(dev, hid, bonded);

        if (g_doConnect || g_doPair || isConnectedTo(addr))
            return;
        std::string name = dev->getName();
        int connected = connectedCount();
        bool match;
        if (*TARGET_ADDR) match = strcasecmp(addr.toString().c_str(), TARGET_ADDR) == 0;
        else if (*TARGET_NAME) match = name.find(TARGET_NAME) != std::string::npos;
        // a new device only when nothing is connected, and in pairing mode only what's picked
        else match = bonded || (hid && connected == 0 && !g_pairing);

        if (match && connected < MAX_DEVICES) {
            Serial.printf("BLE HID: found %s '%s'%s, connecting\n", addr.toString().c_str(), name.c_str(),
                          bonded ? " (bonded)" : "");
            NimBLEDevice::getScan()->stop();
            g_target_addr = addr;
            g_target_name = name;
            g_doConnect = true;
        }
    }
};

class ClientCallbacks : public NimBLEClientCallbacks {
public:
    explicit ClientCallbacks(int slot) : m_slot(slot) {}

    void onConnect(NimBLEClient *c) override {
        Serial.printf("BLE HID: connected to %s\n", c->getPeerAddress().toString().c_str());
    }
    void onDisconnect(NimBLEClient *c) override {
        HidConn &conn = g_conns[m_slot];
        Serial.printf("BLE HID: disconnected from %s '%s'\n", c->getPeerAddress().toString().c_str(), conn.name);
        conn.ready = false;
        conn.name[0] = 0;
        g_disconnected = true;
    }
    uint32_t onPassKeyRequest() override {
        setStatus("passkey asked, answering %06u", (unsigned)PASSKEY);
        return PASSKEY;
    }
    bool onConfirmPIN(uint32_t pin) override {
        setStatus("confirm PIN %06u: yes", (unsigned)pin);
        return true;
    }
    void onAuthenticationComplete(ble_gap_conn_desc *desc) override {
        Serial.printf("BLE HID: auth complete: encrypted=%d authenticated=%d bonded=%d key_size=%d\n",
                      desc->sec_state.encrypted, desc->sec_state.authenticated, desc->sec_state.bonded,
                      desc->sec_state.key_size);
    }
private:
    int m_slot;
};

static void onNotify(NimBLERemoteCharacteristic *ch, uint8_t *data, size_t len, bool isNotify)
{
    NimBLEClient *client = ch->getRemoteService()->getClient();
    for (auto &c : g_conns) {
        if (c.client != client || !c.ready)
            continue;
        auto it = c.handleToReportId.find(ch->getHandle());
        decodeReport(c.reports, it != c.handleToReportId.end() ? it->second : 0, data, len);
        return;
    }
}

static String propsStr(NimBLERemoteCharacteristic *ch)
{
    String p;
    if (ch->canRead()) p += "read ";
    if (ch->canWrite()) p += "write ";
    if (ch->canWriteNoResponse()) p += "write-no-resp ";
    if (ch->canNotify()) p += "notify ";
    if (ch->canIndicate()) p += "indicate ";
    return p;
}

// connects to the device in a free slot, bonds with it and subscribes to its input reports.
// verbose (pairing): logs every step, and shows it in the pairing status
static bool connectToHid(const NimBLEAddress &addr, const std::string &advName, bool verbose)
{
    int slot = -1;
    for (int i = 0; i < MAX_DEVICES && slot < 0; ++i)
        if (!g_conns[i].client || !g_conns[i].client->isConnected())
            slot = i;
    if (slot < 0) {
        setStatus("no room, %d devices connected", MAX_DEVICES);
        return false;
    }
    HidConn &c = g_conns[slot];
    c.ready = false;
    if (!c.client) {
        c.client = NimBLEDevice::createClient();
        if (!c.client) {
            setStatus("can't make a BLE client");
            return false;
        }
        c.client->setClientCallbacks(new ClientCallbacks(slot), true);
        c.client->setConnectTimeout(10);
    }

    String label = advName.empty() ? String(addr.toString().c_str()) : String(advName.c_str());
    auto step = [&](const char *what) {
        if (verbose)
            setStatus("%s: %s", label.c_str(), what);
    };
    auto fail = [&](const char *what, bool have_rc) {
        int rc = have_rc ? c.client->getLastError() : 0;
        if (verbose)
            setStatus(have_rc ? "%s failed (%d)" : "%s", what, rc);
        if (have_rc)
            Serial.printf("BLE HID: %s %s failed, rc=%d %s\n", label.c_str(), what, rc, NimBLEUtils::returnCodeToString(rc));
        else
            Serial.printf("BLE HID: %s %s\n", label.c_str(), what);
        if (c.client->isConnected())
            c.client->disconnect();
        return false;
    };

    Serial.printf("BLE HID: connecting to %s (%s) '%s' in slot %d, %d connected. "
                  "if the device asks for a code, type %06u and Enter on it\n",
                  addr.toString().c_str(), addrTypeName(addr), advName.c_str(), slot, connectedCount(), (unsigned)PASSKEY);
    step("connecting");
    if (!c.client->connect(addr))
        return fail("connect", true);
    step("pairing");
    if (!c.client->secureConnection())
        return fail("pairing / encryption", true);

    if (verbose) {
        auto *svcs = c.client->getServices(true);
        Serial.printf("BLE HID: %d services\n", (int)svcs->size());
        for (auto *s : *svcs)
            Serial.printf("  service %s\n", s->getUUID().toString().c_str());
    }
    step("reading the HID service");
    NimBLERemoteService *svc = c.client->getService(HID_SERVICE);
    if (!svc)
        return fail("has no HID service", false);

    std::vector<NimBLERemoteCharacteristic *> *chars = svc->getCharacteristics(true);
    if (verbose)
        for (auto *ch : *chars)
            Serial.printf("  characteristic %s handle %d: %s\n", ch->getUUID().toString().c_str(), ch->getHandle(),
                          propsStr(ch).c_str());

    c.reports.clear();
    c.handleToReportId.clear();
    for (auto *ch : *chars) {
        if (ch->getUUID() == HID_REPORT_MAP) {
            auto v = ch->readValue();
            if (verbose) {
                Serial.printf("BLE HID: report map, %d bytes:\n", (int)v.length());
                for (size_t i = 0; i < v.length(); i += 32)
                    Serial.printf("  %s\n", hexStr((const uint8_t *)v.data() + i, min((size_t)32, v.length() - i)).c_str());
            }
            parseReportMap(c.reports, (const uint8_t *)v.data(), v.length(), verbose);
        }
    }

    int subscribed = 0;
    for (auto *ch : *chars) {
        if (!(ch->getUUID() == HID_REPORT) || !(ch->canNotify() || ch->canIndicate()))
            continue;
        uint8_t id = 0, type = 1;
        NimBLERemoteDescriptor *ref = ch->getDescriptor(HID_REPORT_REF);
        if (ref) {
            auto v = ref->readValue();
            if (v.length() >= 2) { id = v.data()[0]; type = v.data()[1]; }
        }
        if (!c.client->isConnected()) break;
        if (verbose)
            Serial.printf("  report handle %d: id %d type %d (%s)\n", ch->getHandle(), id, type,
                          type == 1 ? "input" : type == 2 ? "output" : type == 3 ? "feature" : "?");
        if (type != 1) continue;   // only input reports
        c.handleToReportId[ch->getHandle()] = id;
        // response=true: CCCD must be written with a write request, not a write command
        bool ok = ch->subscribe(ch->canNotify(), onNotify, true);
        if (verbose)
            Serial.printf("  subscribe to report id %d: %s\n", id, ok ? "ok" : "FAILED");
        if (ok)
            ++subscribed;
    }

    // fallback for devices that only expose boot protocol reports
    if (subscribed == 0) {
        for (auto *ch : *chars) {
            if ((ch->getUUID() == BOOT_KBD_IN || ch->getUUID() == BOOT_MOUSE_IN) && ch->canNotify()) {
                bool ok = ch->subscribe(true, onNotify, true);
                if (verbose)
                    Serial.printf("  subscribe to boot report %s: %s\n", ch->getUUID().toString().c_str(), ok ? "ok" : "FAILED");
                if (ok)
                    ++subscribed;
            }
        }
    }

    if (!c.client->isConnected())
        return fail("lost the connection while setting up", false);
    if (subscribed == 0)
        return fail("has no input reports", false);

    // the advertised name, else the one in its GAP service, else the address
    std::string name = advName;
    if (name.empty()) {
        auto v = c.client->getValue(GAP_SERVICE, GAP_DEVICE_NAME);
        name.assign((const char *)v.data(), v.length());
    }
    if (name.empty())
        name = loadName(addr).c_str();  // a name it had before
    else
        saveName(addr, name.c_str());
    snprintf(c.name, sizeof(c.name), "%s", name.empty() ? addr.toString().c_str() : name.c_str());
    c.ready = true;
    Serial.printf("BLE HID: ready, '%s' subscribed to %d reports, bonded=%d\n", c.name, subscribed,
                  NimBLEDevice::isBonded(addr));
    if (verbose)
        setStatus("paired with %s", c.name);
    return true;
}

static void startScan(bool pairing)
{
    NimBLEScan *scan = NimBLEDevice::getScan();
    // pairing: listening more of the time, to find the device quickly. else less, so that WiFi isn't starved
    scan->setInterval(100);
    scan->setWindow(pairing ? 90 : 40);
    scan->clearResults();
    scan->start(pairing ? PAIR_SCAN_SECONDS : SCAN_SECONDS, nullptr, false);
}

static void logBonds()
{
    int n = NimBLEDevice::getNumBonds();
    Serial.printf("BLE HID: %d bonded device(s) stored, room for %d\n", n, CONFIG_BT_NIMBLE_MAX_BONDS);
    for (int i = 0; i < n; ++i) {
        NimBLEAddress addr = NimBLEDevice::getBondedAddress(i);
        String name = loadName(addr);
        Serial.printf("  bond %s '%s'%s\n", addr.toString().c_str(), name.length() ? name.c_str() : "(name not known)",
                      isConnectedTo(addr) ? " connected" : "");
    }
}

// connecting blocks for a few seconds so it's done here and not in loop()
static void bleTask(void *)
{
    uint32_t last_bg_scan = 0;
    for (;;) {
        NimBLEScan *scan = NimBLEDevice::getScan();
        if (g_restartScan) {
            g_restartScan = false;
            if (scan->isScanning())
                scan->stop();
        }

        if (g_doPair || g_doConnect) {
            bool pair = g_doPair;
            if (scan->isScanning())
                scan->stop();
            if (pair && isConnectedTo(g_target_addr)) {
                setStatus("already connected");
            }
            else {
                if (pair && NimBLEDevice::isBonded(g_target_addr)) {
                    Serial.printf("BLE pair: deleting the old bond with %s, pairing from scratch\n",
                                  g_target_addr.toString().c_str());
                    NimBLEDevice::deleteBond(g_target_addr);
                    forgetName(g_target_addr);
                }
                bool ok = connectToHid(g_target_addr, g_target_name, pair || g_pairing);
                if (pair)
                    logBonds();
                else if (!ok)
                    vTaskDelay(pdMS_TO_TICKS(1000));
            }
            g_doConnect = false;
            g_doPair = false;
        }

        if (!scan->isScanning() && !g_doConnect && !g_doPair) {
            int n = connectedCount();
            uint32_t now = millis();
            if (g_pairing)
                startScan(true);
            else if (n == 0)
                startScan(false);
            else if (n < MAX_DEVICES && NimBLEDevice::getNumBonds() > n && now - last_bg_scan >= BG_SCAN_PERIOD_MS) {
                last_bg_scan = now;  // some bonded device isn't connected, look for it once in a while
                startScan(false);
            }
        }
        vTaskDelay(pdMS_TO_TICKS(50));
    }
}

void ble_hid_begin()
{
    g_queue = xQueueCreate(64, sizeof(HidEvent));
    g_found_mutex = xSemaphoreCreateMutex();

    NimBLEDevice::init("ESP32 Clock");
    NimBLEDevice::setPower(ESP_PWR_LVL_P9);
    // bond, no MITM requirement from our side, secure connections.
    // DISPLAY_ONLY lets keyboards that want passkey entry pair with PASSKEY;
    // devices without I/O fall back to "just works".
    NimBLEDevice::setSecurityAuth(true, false, true);
    NimBLEDevice::setSecurityIOCap(BLE_HS_IO_DISPLAY_ONLY);
    NimBLEDevice::setSecurityPasskey(PASSKEY);
    logBonds();

    NimBLEScan *scan = NimBLEDevice::getScan();
    scan->setAdvertisedDeviceCallbacks(new ScanCallbacks());
    scan->setActiveScan(true);

    xTaskCreate(bleTask, "ble_hid", 8192, nullptr, 1, nullptr);
}

// ---- pairing mode ----

void ble_hid_pairing(bool on)
{
    if (on == g_pairing)
        return;
    if (on) {
        xSemaphoreTake(g_found_mutex, portMAX_DELAY);
        g_found.clear();
        xSemaphoreGive(g_found_mutex);
        Serial.println("BLE pair: pairing mode on, every advertisement found is logged");
        logBonds();
        setStatus("put the device in pairing mode");
    }
    else {
        Serial.println("BLE pair: pairing mode off");
    }
    g_pairing = on;
    g_restartScan = true;  // with the pairing settings, and so that everything is reported again
}

bool ble_hid_pairing_active()
{
    return g_pairing;
}

int ble_hid_found(BleFoundDevice *out, int max)
{
    int n = 0;
    // the connected ones first: they don't advertise while connected, so the scan doesn't find them
    for (auto &c : g_conns) {
        if (n >= max || !c.ready)
            continue;
        BleFoundDevice &d = out[n++];
        NimBLEAddress addr = c.client->getPeerAddress();
        snprintf(d.name, sizeof(d.name), "%s", c.name);
        snprintf(d.addr, sizeof(d.addr), "%s", addr.toString().c_str());
        d.rssi = c.client->getRssi();
        d.hid = true;
        d.bonded = NimBLEDevice::isBonded(addr);
        d.connected = true;
    }

    uint32_t now = millis();
    xSemaphoreTake(g_found_mutex, portMAX_DELAY);
    for (int pass = 0; pass < 2; ++pass) {  // the HID ones first
        for (auto &f : g_found) {
            if (n >= max)
                break;
            if (f.hid != (pass == 0) || (f.name.empty() && !f.hid) || now - f.seen > FOUND_TIMEOUT_MS)
                continue;
            if (isConnectedTo(f.addr))  // already listed, from before it connected
                continue;
            BleFoundDevice &d = out[n++];
            snprintf(d.name, sizeof(d.name), "%s", f.name.c_str());
            snprintf(d.addr, sizeof(d.addr), "%s", f.addr.toString().c_str());
            d.rssi = f.rssi;
            d.hid = f.hid;
            d.bonded = NimBLEDevice::isBonded(f.addr);
            d.connected = isConnectedTo(f.addr);
        }
    }
    xSemaphoreGive(g_found_mutex);
    return n;
}

void ble_hid_pair(const char *addr)
{
    if (g_doPair || g_doConnect) {
        setStatus("busy, try again");
        return;
    }
    bool found = false;
    xSemaphoreTake(g_found_mutex, portMAX_DELAY);
    for (auto &f : g_found) {
        if (f.addr.toString() == addr) {
            g_target_addr = f.addr;
            g_target_name = f.name;
            found = true;
        }
    }
    xSemaphoreGive(g_found_mutex);
    if (!found) {
        setStatus("%s isn't there anymore", addr);
        return;
    }
    setStatus("pairing with %s", g_target_name.empty() ? addr : g_target_name.c_str());
    g_doPair = true;
}

const char *ble_hid_pair_status()
{
    return g_pair_status;
}

uint32_t ble_hid_passkey()
{
    return PASSKEY;
}

// ---- events for loop() ----

static bool isModifier(uint16_t key)
{
    return key >= 0xE0 && key <= 0xE7;
}

const char* ble_hid_last_key(uint32_t* count, bool* pressed)
{
    if (count)
        *count = g_last_key_count;
    if (pressed)
        *pressed = g_last_key_pressed;
    return g_last_key;
}

const char* ble_hid_device()
{
    static char buf[100];
    buf[0] = 0;
    for (auto &c : g_conns) {
        if (!c.ready)
            continue;
        if (buf[0])
            strlcat(buf, ", ", sizeof(buf));
        strlcat(buf, c.name, sizeof(buf));
    }
    return buf;
}

bool ble_hid_caps_lock()
{
    return g_caps_lock;
}

bool ble_hid_poll(HidEvent &ev)
{
    // key that is currently held down, for auto-repeat
    static uint16_t held_key = 0;
    static uint32_t held_usage = 0;
    static char held_ch = 0;
    static uint32_t next_repeat = 0;

    if (g_queue == nullptr)
        return false;
    if (g_disconnected) {
        g_disconnected = false;
        held_key = 0;
    }

    if (xQueueReceive(g_queue, &ev, 0) == pdTRUE) {
        if (ev.type == HidEvent::KEY_DOWN && ev.key != 0 && !isModifier(ev.key)) {
            held_key = ev.key;
            held_usage = ev.usage;
            held_ch = ev.ch;
            next_repeat = millis() + REPEAT_DELAY_MS;
        }
        else if (ev.type == HidEvent::KEY_UP && ev.usage == held_usage) {
            held_key = 0;
        }
        return true;
    }

    if (held_key != 0 && (int32_t)(millis() - next_repeat) >= 0) {
        ev = HidEvent();
        ev.type = HidEvent::KEY_DOWN;
        ev.repeat = true;
        ev.key = held_key;
        ev.usage = held_usage;
        ev.ch = held_ch;
        next_repeat += REPEAT_RATE_MS;
        return true;
    }
    return false;
}
