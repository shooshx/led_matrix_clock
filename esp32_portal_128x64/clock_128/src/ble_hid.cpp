// BLE HID host: find a BLE keyboard / remote / air-mouse, pair (bond) with it,
// subscribe to its HID input reports and decode key presses, buttons, consumer keys
// and movement by parsing the device's HID report descriptor.
// Based on esp32_portal_tests/test_protomatter/src/ble.cpp
//
// The BLE connection is managed in a separate task, the decoded input is passed to loop()
// through a queue as HidEvent

#include <Arduino.h>
#include <NimBLEDevice.h>
#include <map>
#include <set>
#include <vector>
#include "ble_hid.h"

// ---- configuration ----
// Leave both empty to auto-connect to the first device advertising as HID.
// Otherwise set one of them to pick a specific device.
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

static std::map<uint8_t, HidReport> g_reports;   // input reports by report ID (0 = no ID)

static int32_t itemSigned(uint32_t v, uint8_t n) {
    if (n == 1) return (int8_t)v;
    if (n == 2) return (int16_t)v;
    return (int32_t)v;
}

static void parseReportMap(const uint8_t *d, size_t len)
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
            g_reports[g.id].fields.push_back(f);
        }   // fallthrough: main items clear locals
        case 0x90: case 0xA0: case 0xB0: case 0xC0:
            usages.clear();
            haveRange = false;
            umin = umax = 0;
            break;
        }
    }

    for (auto &r : g_reports)
        Serial.printf("HID report id %d: %d fields\n", r.first, (int)r.second.fields.size());
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

static void decodeReport(uint8_t id, const uint8_t *d, size_t len)
{
    if (PRINT_RAW) {
        Serial.printf("[id %d]", id);
        for (size_t i = 0; i < len; ++i) Serial.printf(" %02x", d[i]);
        Serial.println();
    }

    auto it = g_reports.find(id);
    if (it == g_reports.end()) return;
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

static NimBLEAdvertisedDevice *g_target = nullptr;
static volatile bool g_doConnect = false;
static NimBLEClient *g_client = nullptr;
static std::map<uint16_t, uint8_t> g_handleToReportId;

static bool isHidAppearance(uint16_t a)
{
    return (a >= 0x03C0 && a <= 0x03CF)    // HID category (keyboard, mouse, gamepad...)
        || (a >= 0x0180 && a <= 0x01BF);   // remote control category
}

class ScanCallbacks : public NimBLEAdvertisedDeviceCallbacks {
    void onResult(NimBLEAdvertisedDevice *dev) override {
        bool hid = dev->isAdvertisingService(HID_SERVICE) ||
                   (dev->haveAppearance() && isHidAppearance(dev->getAppearance()));
        std::string addr = dev->getAddress().toString();
        std::string name = dev->getName();

        bool match;
        if (*TARGET_ADDR) match = strcasecmp(addr.c_str(), TARGET_ADDR) == 0;
        else if (*TARGET_NAME) match = name.find(TARGET_NAME) != std::string::npos;
        else match = hid;

        if (match && !g_doConnect) {
            Serial.printf("BLE HID: found %s '%s', connecting\n", addr.c_str(), name.c_str());
            NimBLEDevice::getScan()->stop();
            g_target = dev;
            g_doConnect = true;
        }
    }
};

class ClientCallbacks : public NimBLEClientCallbacks {
    void onConnect(NimBLEClient *c) override {
        Serial.println("BLE HID: connected");
    }
    void onDisconnect(NimBLEClient *c) override {
        Serial.println("BLE HID: disconnected");
        g_disconnected = true;
    }
    uint32_t onPassKeyRequest() override {
        Serial.printf("BLE HID: passkey requested, answering %06u\n", PASSKEY);
        return PASSKEY;
    }
    bool onConfirmPIN(uint32_t pin) override {
        Serial.printf("BLE HID: confirm PIN %06u -> yes\n", pin);
        return true;
    }
    void onAuthenticationComplete(ble_gap_conn_desc *desc) override {
        Serial.printf("BLE HID: auth complete: encrypted=%d bonded=%d authenticated=%d\n",
                      desc->sec_state.encrypted, desc->sec_state.bonded, desc->sec_state.authenticated);
    }
};

static void onNotify(NimBLERemoteCharacteristic *ch, uint8_t *data, size_t len, bool isNotify)
{
    auto it = g_handleToReportId.find(ch->getHandle());
    uint8_t id = it != g_handleToReportId.end() ? it->second : 0;
    decodeReport(id, data, len);
}

static bool connectToHid(NimBLEAdvertisedDevice *dev)
{
    if (!g_client) {
        g_client = NimBLEDevice::createClient();
        g_client->setClientCallbacks(new ClientCallbacks(), false);
        g_client->setConnectTimeout(10);
    }

    Serial.printf("BLE HID: connecting to %s (if the device asks for a code, type %06u and Enter on it)\n",
                  dev->getAddress().toString().c_str(), PASSKEY);
    if (!g_client->connect(dev)) {
        Serial.println("BLE HID: connect failed");
        return false;
    }
    if (!g_client->secureConnection()) {
        Serial.println("BLE HID: pairing / encryption failed");
        g_client->disconnect();
        return false;
    }

    NimBLERemoteService *svc = g_client->getService(HID_SERVICE);
    if (!svc) {
        Serial.println("BLE HID: no HID service on this device");
        g_client->disconnect();
        return false;
    }

    std::vector<NimBLERemoteCharacteristic *> *chars = svc->getCharacteristics(true);

    g_reports.clear();
    g_handleToReportId.clear();
    for (auto *ch : *chars) {
        if (ch->getUUID() == HID_REPORT_MAP) {
            auto v = ch->readValue();
            parseReportMap((const uint8_t *)v.data(), v.length());
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
        if (!g_client->isConnected()) break;
        if (type != 1) continue;   // only input reports
        g_handleToReportId[ch->getHandle()] = id;
        // response=true: CCCD must be written with a write request, not a write command
        if (ch->subscribe(ch->canNotify(), onNotify, true))
            ++subscribed;
    }

    // fallback for devices that only expose boot protocol reports
    if (subscribed == 0) {
        for (auto *ch : *chars) {
            if ((ch->getUUID() == BOOT_KBD_IN || ch->getUUID() == BOOT_MOUSE_IN) && ch->canNotify()) {
                if (ch->subscribe(true, onNotify, true))
                    ++subscribed;
            }
        }
    }

    if (!g_client->isConnected()) {
        Serial.println("BLE HID: lost connection during setup");
        return false;
    }
    if (subscribed == 0) {
        Serial.println("BLE HID: no input reports to subscribe to");
        g_client->disconnect();
        return false;
    }
    Serial.printf("BLE HID: ready, subscribed to %d reports\n", subscribed);
    return true;
}

static const uint32_t SCAN_SECONDS = 5;

static void startScan()
{
    NimBLEDevice::getScan()->clearResults();
    NimBLEDevice::getScan()->start(SCAN_SECONDS, nullptr, false);
}

// connecting blocks for a few seconds so it's done here and not in loop()
static void bleTask(void *)
{
    for (;;) {
        if (g_doConnect) {
            if (!connectToHid(g_target))
                vTaskDelay(pdMS_TO_TICKS(1000));
            g_doConnect = false;
        }

        bool connected = g_client && g_client->isConnected();
        if (!connected && !g_doConnect && !NimBLEDevice::getScan()->isScanning())
            startScan();
        vTaskDelay(pdMS_TO_TICKS(50));
    }
}

void ble_hid_begin()
{
    g_queue = xQueueCreate(64, sizeof(HidEvent));

    NimBLEDevice::init("ESP32 Clock");
    NimBLEDevice::setPower(ESP_PWR_LVL_P9);
    // bond, no MITM requirement from our side, secure connections.
    // DISPLAY_ONLY lets keyboards that want passkey entry pair with PASSKEY;
    // devices without I/O fall back to "just works".
    NimBLEDevice::setSecurityAuth(true, false, true);
    NimBLEDevice::setSecurityIOCap(BLE_HS_IO_DISPLAY_ONLY);
    NimBLEDevice::setSecurityPasskey(PASSKEY);
    Serial.printf("BLE HID: %d bonded device(s) stored\n", NimBLEDevice::getNumBonds());

    NimBLEScan *scan = NimBLEDevice::getScan();
    scan->setAdvertisedDeviceCallbacks(new ScanCallbacks());
    scan->setActiveScan(true);
    // lower duty cycle than the tester so that scanning doesn't starve WiFi
    scan->setInterval(100);
    scan->setWindow(40);

    xTaskCreate(bleTask, "ble_hid", 8192, nullptr, 1, nullptr);
}

// ---- events for loop() ----

static bool isModifier(uint16_t key)
{
    return key >= 0xE0 && key <= 0xE7;
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
