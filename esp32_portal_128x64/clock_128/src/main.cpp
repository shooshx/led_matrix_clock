#include <Arduino.h>
#include <WiFi.h>
#include <ESPmDNS.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <AsyncJson.h>

#include <TimeLib.h>
#include <LittleFS.h>
#include <Preferences.h>
#include <ArduinoJson.h>
#include <Adafruit_Protomatter.h>

#include "drawer.h"
#include "display.h"

#include "my_fonts/fonts_index.h"
#include "ClockState.h"
#include "TimerState.h"
#include "StopWState.h"
#include "base_utils.h"
#include "Buttons.h"
#include "wifi_secrets.h"
#include "ble_hid.h"
#include "audio.h"

#define DISPLAY_WIDTH 128
#define DISPLAY_HEIGHT 64

#include "image_handling.h"
#include "ModeIcons.h"
#include "DrawTextState.h"
#include "GamesState.h"
#include "DinoGame.h"
#include "TetrisGame.h"
#include "LifeGame.h"
#include "DustGame.h"
#include "Orientation.h"


AsyncWebServer server(80);
AsyncWebSocket ws("/ws"); // access at ws://[esp ip]/ws

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP);

// MatrixPortal ESP32-S3 HUB75 pins
// the standard order is {R1, G1, B1, R2, G2, B2} = {42, 41, 40, 38, 39, 37}
// but this panel has red and blue swapped
uint8_t rgbPins[]  = {40, 41, 42, 37, 39, 38};
uint8_t addrPins[] = {45, 36, 48, 35, 21};
uint8_t clockPin   = 2;
uint8_t latchPin   = 47;
uint8_t oePin      = 14;

#define NUM_ADDR_PINS 5    // for 64 rows
const int MIN_COLOR_BITS = 2;
const int SAFE_COLOR_BITS = 4; // the last fallback, known to work
const int MAX_COLOR_BITS = 6; // Protomatter's limit

// the matrix is made again when its bit depth (bits per color) changes, from the "color_bits" pref.
// more bits are smoother colors but a slower refresh, more flicker, and a bigger buffer in internal RAM
Adafruit_Protomatter* matrix = nullptr;
int g_color_bits = 0;  // of the current matrix

WrapGFX display(DISPLAY_WIDTH, DISPLAY_HEIGHT);
MatrixDrawer matrix_drawer(nullptr);
HaloDraw halo_drawer(&matrix_drawer, DISPLAY_WIDTH, DISPLAY_HEIGHT);

bool has_display = false;
bool g_admin = false;  // the hidden admin view, see AdminScreen
bool g_force_redraw = false; // draw the current mode on the next loop
volatile bool g_off_redraw = false; // blank the panel again in the off mode
// from the web-ui (its own task), done in the loop: a section change or drawing turns the display on.
// g_web_wake_section is the section to show if the display was off, -1 is the current one
volatile bool g_web_wake = false;
volatile int g_web_wake_section = -1;
// prefs changed from the web-ui are saved by the loop SAVE_QUIET_MS after the last change (millis, 0 is none):
// the clock faces are saved as blobs, a face copied from the web-ui is ~150 changes and one blob write
volatile unsigned long g_save_at = 0;
const unsigned long SAVE_QUIET_MS = 1000;

// the off mode: nothing shown
class OffScreen : public IScreen
{
public:
    void draw() override {
        display.setDrawer(&matrix_drawer);
        display.clearDisplay();
    }
} off_screen;
bool has_serial = false;

unsigned long g_epoch_time = 0;
Buttons buttons;
Orientation orientation;


enum Section {
    SECTION_OFF = 0,
    SECTION_CLOCK = 1,
    SECTION_TIMER = 2,
    SECTION_STOPW = 3,
    SECTION_DRAW = 4,  // drawing and text, from the web-ui draw tab and from the remote
    SECTION_IMAGE = 5,
    // 10 and 11 were the separate remote draw and type modes, now SECTION_DRAW
    SECTION_GAMES = 20, // only from the remote, not in the web-ui
};

// modes cycled with the keyboard/remote Up/Down keys
const int HID_MODES[] = { SECTION_CLOCK, SECTION_STOPW, SECTION_TIMER, SECTION_DRAW, SECTION_GAMES };

#define SECTION_BTN_FIRST SECTION_CLOCK
#define SECTION_BTN_LAST SECTION_STOPW

class TopState : public PropHolder<5>
{
public:
    static const int MOVE_SLOW_MIN = 10, MOVE_SLOW_MAX = 20, MOVE_SLOW_STEP = 2;

    Prop<int16_t> active_section;
    Prop<uint16_t> brightness;
    Prop<int16_t> gamma;  // in tenths, 10 is 1.0, no correction
    Prop<int16_t> color_bits; // bits per color of the matrix, 2-6
    // the remote's movement is divided by this, in tenths: 10 is normal, 20 moves half as much. from the admin view
    Prop<int16_t> move_slow;
    Preferences m_pref;

    TopState(NamesIndex* prop_map)
      : PropHolder(prop_map)
      , active_section(this, "active_section", 0)
      , brightness(this, "brightness", 255)
      , gamma(this, "gamma", 10)
      , color_bits(this, "color_bits", 4)
      , move_slow(this, "move_slow", MOVE_SLOW_MIN)
    {}

    void load() {
        m_pref.begin("top", false);
        PropHolder::load(m_pref);
    }
    void save() {
        PropHolder::save(m_pref);
    }

    void active_section_btn(int d) {
      auto cur = active_section.get();
      if (cur < SECTION_BTN_FIRST || cur > SECTION_BTN_LAST)
        cur = SECTION_BTN_FIRST;
      else {
        cur += d;
        if (cur > SECTION_BTN_LAST)
          cur = SECTION_BTN_FIRST;
        if (cur < SECTION_BTN_FIRST)
          cur = SECTION_BTN_LAST;
      }
      Serial.printf("top section %d\n", cur);
      active_section.set(cur);
      save();
    }
};

class State
{
public:
    NamesIndex m_prop_map;
    ClockState clock_state;
    TopState top;
    TimerState timer;
    StopWState stopw;
    DrawTextState drawtext;
    ImageState image;
    DinoGame dino;
    TetrisGame tetris;
    LifeGame life;
    DustGame dust;
    GamesState games;
    IScreen* m_cur_screen = nullptr;

    State()
        : clock_state(&m_prop_map)
        , top(&m_prop_map)
        , timer(&m_prop_map)
        , stopw(&m_prop_map)
        , games({&dino, &tetris, &life, &dust})
    {
    }

public:
    // return true if actually changed value
    bool update_prop(const String& name, int v)
    {
        auto prop = m_prop_map.get(name);
        if (prop == nullptr) {
            Serial.printf("prop not found %s\n", name.c_str());
            return false;
        }
        return prop->setInt(v);
    }

    void toJson(const JsonObject& root)
    {
        top.toJson(root);
        // not a pref, so that the web-ui knows the current orientation
        root["rotation"] = display.getRotation();
        root["color_bits_actual"] = g_color_bits; // what color_bits could be applied
        clock_state.toJson(root["clock"].to<JsonObject>());
        timer.toJson(root["timer"].to<JsonObject>());
        stopw.toJson(root["stopw"].to<JsonObject>());
    }

    void load()
    {
        Serial.printf("props: %d\n", m_prop_map.size());
        clock_state.load();
        top.load();
        timer.load();
        stopw.load();
        drawtext.load();
        games.load();
        // always start with the clock, the last mode isn't kept
        top.active_section.set(SECTION_CLOCK);
    }
    void save()
    {
        clock_state.save();
        top.save();
        timer.save();
        stopw.save();
    }
};

State* state = nullptr;

void setupPins()
{
  pinMode(LED_BUILTIN, OUTPUT);
  audio_begin();
}

void setupSerial()
{
  Serial.begin(115200);         // Start the Serial communication to send messages to the computer
  //for(int i = 0; i < 10; ++i) { Serial.printf("start %d\n", i); delay(1000); }
  Serial.println('\n');
  has_serial = true;
}

void listAllFilesInDir(String dir_path)
{
  File dir = LittleFS.open(dir_path);
  File f = dir.openNextFile();
  while(f) {
    if (!f.isDirectory()) {
      Serial.print("File: ");
      Serial.println(dir_path + f.name() + "  : " + f.size());
    }
    else {
      Serial.print("Dir: ");
      Serial.println(dir_path + f.name() + "/");
      listAllFilesInDir(dir_path + f.name() + "/");
    }
    f = dir.openNextFile();
  }
  dir.close();
}


void setupFs()
{
    if(!LittleFS.begin()){
      Serial.println("An Error has occurred while mounting FS");
      return;
    }
    Serial.println("FS mounted successfully");
    listAllFilesInDir("/");
}


const char* MDNS_NAME = "myesp32clock";
volatile int g_wifi_reason = 0;  // of the last disconnect, 0 if none yet
volatile bool g_wifi_got_ip = false;

// starts connecting, doesn't wait for it so that the clock (and the admin view) work without WiFi.
// the core keeps reconnecting by itself. mDNS is started in the loop once there's an IP (startMdns)
void connectWifi()
{
  WiFi.onEvent([](WiFiEvent_t event, WiFiEventInfo_t info) {
    g_wifi_reason = info.wifi_sta_disconnected.reason;
    Serial.printf("wifi: disconnected, reason %d\n", info.wifi_sta_disconnected.reason);
  }, ARDUINO_EVENT_WIFI_STA_DISCONNECTED);
  WiFi.onEvent([](WiFiEvent_t event, WiFiEventInfo_t info) {
    g_wifi_got_ip = true;
    Serial.printf("wifi: got ip %s\n", WiFi.localIP().toString().c_str());
  }, ARDUINO_EVENT_WIFI_STA_GOT_IP);
  WiFi.setAutoReconnect(true);

  Serial.printf("Connecting to %s\n", WIFI_SSID);
  WiFi.begin(WIFI_SSID, WIFI_PASSW);
}

// once, the first time there's an IP
void startMdns()
{
  static bool started = false;
  if (started || !g_wifi_got_ip)
    return;
  started = true;
  if (!MDNS.begin(MDNS_NAME))
    Serial.println("Error starting mDNS");
  else
    Serial.println("mDNS responder started");
  if (!MDNS.addService("http", "tcp", 80))
    Serial.println("MDNS addService failed");
}

// for the admin view
String wifiStatusText()
{
  if (WiFi.status() == WL_CONNECTED)
    return "IP " + WiFi.localIP().toString();
  switch (g_wifi_reason) {
    case 0:   return "WiFi connecting";
    case 201: return "WiFi: no network";      // WIFI_REASON_NO_AP_FOUND
    case 15: case 202: case 204:
              return "WiFi: bad password";    // 4WAY_HANDSHAKE_TIMEOUT, AUTH_FAIL, HANDSHAKE_TIMEOUT
    default:  return String("WiFi failed (") + g_wifi_reason + ")";
  }
}

void setupNtp()
{
  timeClient.begin();
  timeClient.setUpdateInterval(30 * 60000);
}


void set_builtin_led(bool v)
{
  digitalWrite(LED_BUILTIN, v ? 1 : 0);
}


void handleWebSocketMessage(const uint8_t *data, size_t len)
{
    std::vector<uint8_t> v;
    v.resize(len + 1);
    memcpy(&v[0], data, len);
    v[len] = 0;

    if (v.size() < 2) {
        Serial.printf("data too short %d\n", (int)v.size());
        return;
    }

    char cmd = (char)v[0];
    char subcmd = (char)v[1];
    if (cmd == 'U' && subcmd == 'T')
    {
        // UT <name> <text>: a text pref, the text is to the end of the line
        String line((char*)&v[0]);
        int sp1 = line.indexOf(' ');
        int sp2 = sp1 < 0 ? -1 : line.indexOf(' ', sp1 + 1);
        if (sp1 < 0) {
            Serial.printf("Unexpected UT %s\n", line.c_str());
            return;
        }
        String name = sp2 < 0 ? line.substring(sp1 + 1) : line.substring(sp1 + 1, sp2);
        String text = sp2 < 0 ? String("") : line.substring(sp2 + 1);
        Serial.printf("prop update %s '%s'\n", name.c_str(), text.c_str());
        IProp* prop = state->m_prop_map.get(name);
        if (prop == nullptr)
            Serial.printf("prop not found %s\n", name.c_str());
        else if (prop->setText(text))
            g_save_at = millis() | 1;
    }
    else if (cmd == 'U')
    {
        String sp[3];
        String line((char*)&v[0]);

        int count = strSplit(line, sp, 3);
        if (count != 3) {
            Serial.printf("Unexpected number of args of U %d\n", count);
            return;
        }
        int v = sp[2].toInt();
        Serial.printf("prop update %s %d\n", sp[1].c_str(), v);
        if (sp[1] == "active_section") { // opening a tab, also the same one again
            g_web_wake_section = -1;
            g_web_wake = true;
        }
        if (state->update_prop(sp[1], v))
            g_save_at = millis() | 1;  // saved by the loop when the changes stop
    }
    else if (cmd == 'D')
    {
        if (subcmd == 'P' || subcmd == 'X' || subcmd == 'I') { // drawing shows it if the display was off
            g_web_wake_section = (subcmd == 'I') ? SECTION_IMAGE : SECTION_DRAW;
            g_web_wake = true;
        }
        if (subcmd == 'P')
            parse_pixel_cmd(v, [](int x, int y, uint16_t c) { state->drawtext.set_pixel(x, y, c); });
        else if (subcmd == 'X')  // clear of the draw tab, the drawing and the text
            state->drawtext.clear_all();
        else if (subcmd == 'C')  // the off tab, the loop blanks the panel
            g_off_redraw = true;
        else if (subcmd == 'I') {
            parse_img_cmd(v, [](int x, int y, uint16_t c) { state->image.set_pixel(x, y, c); });
            state->image.changed();
        }
        else
            Serial.printf("Unknown D subcmd %c\n", subcmd);
    }
    else if (cmd == 'S' && subcmd == 'M')
    {
        // SM <minute>: the clock shows that minute of the day for a while ("Show on panel"), -1 stops
        String sp[2];
        String line((char*)&v[0]);
        if (strSplit(line, sp, 2) == 2)
            state->clock_state.m_panel.simulate(sp[1].toInt());
    }
    else if (cmd == 'T')
    {
        if (subcmd == 'R') {
            Serial.printf("stop watch reset\n");
            state->stopw.m_panel.reset();
        }
        else if (subcmd == 'P') {
            Serial.printf("play sound %d,%d\n", state->timer.m_panel.m_snd_vol.get(), state->timer.m_panel.m_snd_file_num.get());
            // TODO sound is not implemented yet on this board
        }
        else {
            String sp[2];
            String line((char*)&v[0]);
            int count = strSplit(line, sp, 2);
            if (count != 2) {
              Serial.printf("Unexpeted number of argument to T %d\n", count);
              return;
            }
            int v = sp[1].toInt();
            Serial.printf("timer toggle %c %d\n", subcmd, v);
            if (subcmd == 'T')
                state->timer.m_panel.toggle_run(v);
            else if (subcmd == 'S')
                state->stopw.m_panel.toggle_run(v);
            else
                Serial.printf("Unknown T subcmd %c\n", subcmd);
        }
    }
    else if (cmd == 'A')
    {
        // the audio tab's test beep. AB <0/1>: on/off, AF <hz>: frequency, AV <percent>: volume
        String sp[2];
        String line((char*)&v[0]);
        int count = strSplit(line, sp, 2);
        if (count != 2) {
            Serial.printf("Unexpected number of arguments to A %d\n", count);
            return;
        }
        int val = sp[1].toInt();
        if (subcmd == 'B')
            audio_beep(val != 0);
        else if (subcmd == 'F')
            audio_set_freq(val);
        else if (subcmd == 'V')
            audio_set_volume(val);
        else
            Serial.printf("Unknown A subcmd %c\n", subcmd);
    }
    else
    {
        Serial.printf("Unknown command %c\n", cmd);
    }
}

struct FragmentCombine
{
  std::vector<uint8_t> m_d;

  int add(uint8_t* data, size_t len, int index) {
    if (index == 0)
      m_d.clear();
    if (index != m_d.size()) {
      Serial.printf("FRAG: Unexpected index %d,%d\n", index, m_d.size());
      return 0;
    }
    int at_index = m_d.size();
    m_d.resize(m_d.size() + len);
    memcpy(&m_d[at_index], data, len);
    return (int)m_d.size();
  }
} g_frag;


// the web-ui sends the pref updates ("U ...", "UT ...") together, one per line, in messages of up to about 1K:
// hundreds of small messages (a clock face copied, prefs uploaded) were sometimes lost.
// only text messages that start with U are split, the binary ones (DP, DI) may have any byte
void handleWebSocketData(const uint8_t* data, size_t len)
{
    if (len == 0 || data[0] != 'U' || memchr(data, '\n', len) == nullptr) {
        handleWebSocketMessage(data, len);
        return;
    }
    size_t start = 0;
    while (start < len) {
        const uint8_t* nl = (const uint8_t*)memchr(data + start, '\n', len - start);
        size_t end = nl ? (size_t)(nl - data) : len;
        if (end > start)
            handleWebSocketMessage(data + start, end - start);
        start = end + 1;
    }
}

void onEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len) {
  switch (type) {
    case WS_EVT_CONNECT:
      Serial.printf("WebSocket client #%u connected from %s\n", client->id(), client->remoteIP().toString().c_str());
      break;
    case WS_EVT_DISCONNECT:
      Serial.printf("WebSocket client #%u disconnected\n", client->id());
      break;
    case WS_EVT_DATA: {
      AwsFrameInfo *info = (AwsFrameInfo*)arg;
      if (len != info->len) {
          auto total_len = g_frag.add(data, len, info->index);
          if (total_len == info->len)
            handleWebSocketData(g_frag.m_d.data(), g_frag.m_d.size());
      }
      else if (info->final && info->index == 0 && info->len == len && (info->opcode == WS_TEXT || info->opcode == WS_BINARY)) {
          handleWebSocketData(data, len);
      }
      else {
          Serial.printf("Unhandled WS event\n");
      }
      break;
    }
    case WS_EVT_PONG:
    case WS_EVT_ERROR:
      Serial.printf("WebSocket error len=%d", len);
      break;
    default:
      Serial.printf("WebSocket unknown event %d\n", type);
      break;
  }
}

void setupWebSocket() {
  ws.onEvent(onEvent);
  server.addHandler(&ws);
}


// ArduinoJson allocations in PSRAM
struct SpiRamAllocator : ArduinoJson::Allocator {
    void* allocate(size_t size) override { return heap_caps_malloc(size, MALLOC_CAP_SPIRAM); }
    void deallocate(void* p) override { heap_caps_free(p); }
    void* reallocate(void* p, size_t size) override { return heap_caps_realloc(p, size, MALLOC_CAP_SPIRAM); }
};

// the ETag of a file of the web-ui: for a .gz (made by gzip_web.py) the one the web server sends with it,
// made from the CRC at its end, with the digits in its order (AsyncWebServerRequest::_getEtag, private).
// else its size. gz says which. empty if there's no such file
String fileEtag(const char* path, bool& gz)
{
    // exists() first, open() of a missing file logs an error
    String gz_path = String(path) + ".gz";
    gz = LittleFS.exists(gz_path);
    if (gz) {
        File f = LittleFS.open(gz_path, "r");
        if (f && f.size() > 8 && f.seek(f.size() - 8)) {
            uint32_t crc = 0;
            f.read((uint8_t*)&crc, sizeof(crc));
            static const char hex[] = "0123456789ABCDEF";
            const int shifts[8] = { 4, 0, 12, 8, 20, 16, 28, 24 };
            char buf[11];
            buf[0] = '"';
            for (int i = 0; i < 8; ++i)
                buf[1 + i] = hex[(crc >> shifts[i]) & 0x0F];
            buf[9] = '"';
            buf[10] = 0;
            return buf;
        }
        return "";
    }
    if (LittleFS.exists(path)) {
        File f = LittleFS.open(path, "r");
        if (f)
            return "\"s" + String((unsigned)f.size()) + "\"";
    }
    return "";
}

void setupWeb()
{
    setupWebSocket();

    server.on("/heap", HTTP_GET, [](AsyncWebServerRequest *request){
        request->send(200, "text/plain", String(ESP.getFreeHeap()));
    });
    server.on("/hello", HTTP_GET, [](AsyncWebServerRequest *request){
        Serial.println("got hello");
        request->send(200, "text/plain", "hello world");
    });
    server.on("/gettime", HTTP_GET, [](AsyncWebServerRequest *request){
        request->send(200, "text/plain", String(g_epoch_time));
    });
    server.on("/led", HTTP_GET, [](AsyncWebServerRequest *request){
        if (request->args() == 0) {
            request->send(200, "text/plain", "no-arg");
            return;
        }
        bool v = request->arg((size_t)0) == "1";
        set_builtin_led(v);
        request->send(200, "text/plain", String("ok ") + v);
    });

    // the json of all the prefs is big, made in PSRAM so that the internal RAM is left to WiFi
    server.on("/pref", HTTP_GET, [](AsyncWebServerRequest *request){
        Serial.println("got /pref req");
        static SpiRamAllocator allocator;
        JsonDocument doc(&allocator);
        state->toJson(doc.to<JsonObject>());
        String* out = new String();  // long, so its buffer is in PSRAM
        serializeJson(doc, *out);
        AsyncWebServerResponse* response = request->beginResponse(200, "application/json", *out);
        delete out;
        response->addHeader("Server","ESP Clock");
        request->send(response);
    });

    // many prefs at once, a json like the one of GET /pref (nested or not, any part of it): the prefs tab's
    // upload, a clock face copied. one request instead of hundreds of websocket updates, that were sometimes
    // lost. the body is collected in PSRAM. the answer says what was done:
    // {"applied": n, "unknown": [names], "bad": [names with a value that isn't a number, true/false or a text]}
    server.on("/pref", HTTP_POST, [](AsyncWebServerRequest *request){
        char* body = (char*)request->_tempObject;
        if (body == nullptr) {
            request->send(400, "text/plain", "no body");
            return;
        }
        static SpiRamAllocator allocator;
        JsonDocument doc(&allocator);
        DeserializationError err = deserializeJson(doc, body);
        if (err) {
            request->send(400, "text/plain", String("bad json: ") + err.c_str());
            return;
        }
        JsonDocument answer(&allocator);
        JsonArray unknown = answer["unknown"].to<JsonArray>();
        JsonArray bad = answer["bad"].to<JsonArray>();
        int applied = 0;
        bool changed = false;
        // the json of GET /pref has these too, they aren't prefs
        const char* not_prefs[] = { "rotation", "color_bits_actual" };
        std::function<void(JsonObjectConst)> apply = [&](JsonObjectConst obj) {
            for (JsonPairConst kv : obj) {
                const char* name = kv.key().c_str();
                JsonVariantConst v = kv.value();
                if (v.is<JsonObjectConst>()) {
                    apply(v.as<JsonObjectConst>());
                    continue;
                }
                bool skip = false;
                for (const char* np : not_prefs)
                    skip |= strcmp(name, np) == 0;
                if (skip)
                    continue;
                IProp* prop = state->m_prop_map.get(name);
                if (prop == nullptr) {
                    unknown.add(name);
                    continue;
                }
                if (v.is<bool>())
                    changed |= prop->setInt(v.as<bool>() ? 1 : 0);
                else if (v.is<int>())
                    changed |= prop->setInt(v.as<int>());
                else if (v.is<const char*>())
                    changed |= prop->setText(v.as<const char*>());
                else {
                    bad.add(name);
                    continue;
                }
                ++applied;
            }
        };
        apply(doc.as<JsonObjectConst>());
        answer["applied"] = applied;
        Serial.printf("POST /pref: %d applied, %d unknown, %d bad\n", applied, (int)unknown.size(), (int)bad.size());
        if (changed)
            g_save_at = millis() | 1;  // saved by the loop
        String* out = new String();
        serializeJson(answer, *out);
        request->send(200, "application/json", *out);
        delete out;
    }, nullptr, [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total){
        const size_t MAX_BODY = 128 * 1024;
        if (total > MAX_BODY)
            return;  // no body, the request answers 400
        if (index == 0) {
            // freed with the request (free() works for heap_caps_malloc too)
            request->_tempObject = heap_caps_malloc(total + 1, MALLOC_CAP_SPIRAM);
            if (request->_tempObject == nullptr)
                return;
            ((char*)request->_tempObject)[total] = 0;
        }
        if (request->_tempObject != nullptr && index + len <= total)
            memcpy((char*)request->_tempObject + index, data, len);
    });

    // the drawing and the text as they are shown, in the current orientation, for the web-ui draw tab:
    // width and height as 16 bit little endian and then the pixels as RGB
    server.on("/drawing", HTTP_GET, [](AsyncWebServerRequest *request){
        AsyncResponseStream *response = request->beginResponseStream("application/octet-stream");
        int w = display.width(), h = display.height();
        uint8_t hdr[4] = { (uint8_t)(w & 0xFF), (uint8_t)(w >> 8), (uint8_t)(h & 0xFF), (uint8_t)(h >> 8) };
        response->write(hdr, sizeof(hdr));
        const uint16_t* img = state->drawtext.rendered();
        uint8_t row[DISPLAY_WIDTH * 3];
        for (int y = 0; y < h; ++y) {
            for (int x = 0; x < w; ++x) {
                int16_t px = x, py = y;
                display.to_physical(px, py);
                color888(img[py * DISPLAY_WIDTH + px], row[x * 3], row[x * 3 + 1], row[x * 3 + 2]);
            }
            response->write(row, w * 3);
        }
        request->send(response);
    });
    // changes when the drawing or the text change, so the web-ui knows when to get /drawing again
    server.on("/drawing_ver", HTTP_GET, [](AsyncWebServerRequest *request){
        request->send(200, "text/plain", String(state->drawtext.version()));
    });

    // browsers want the correct mime type for streaming wasm compilation, so not by serveStatic.
    // with an ETag like serveStatic, so that a reload doesn't get the big file again
    server.on("/asm_gfx.wasm", HTTP_GET, [](AsyncWebServerRequest *request){
        // the files change only with uploadfs, that restarts
        static bool gz = false;
        static String etag = fileEtag("/asm_gfx.wasm", gz);
        if (etag.length() > 0 && request->header("If-None-Match") == etag) {
            request->send(304);
            return;
        }
        // sends /asm_gfx.wasm.gz compressed if there's no /asm_gfx.wasm, then with the ETag and no-cache
        AsyncWebServerResponse* response = request->beginResponse(LittleFS, "/asm_gfx.wasm", "application/wasm");
        if (!gz && etag.length() > 0) { // the same for the uncompressed one
            response->addHeader("ETag", etag);
            response->addHeader("Cache-Control", "no-cache"); // may be kept, asking each time if it changed
        }
        request->send(response);
    });

    server.serveStatic("/", LittleFS, "/").setDefaultFile("main.html");

    server.onNotFound([](AsyncWebServerRequest *request){
        request->send(404);
    });

    server.begin();
}


// a new matrix with this many bits per color, replacing the current one. its buffer needs one contiguous block
// of internal DMA memory, if there isn't one big enough it goes back to the previous depth. returns true if done
bool setColorBits(int bits)
{
  bits = constrain(bits, MIN_COLOR_BITS, MAX_COLOR_BITS);
  if (matrix != nullptr && bits == g_color_bits)
    return true;
  int prev = g_color_bits;
  bool live = matrix != nullptr; // replacing a running one, not at boot
  if (live) { // stop and free first, so that its memory can be used by the new one
    matrix_drawer.setMatrix(nullptr);
    delete matrix;
    matrix = nullptr;
    // Protomatter leaves its timer (#0) set up with the interrupt attached.
    // released so that the new matrix sets it up from scratch, like at boot
    hw_timer_t* t = timerBegin(0, 2, true);
    timerDetachInterrupt(t);
    timerEnd(t);
  }
  for (int tryb : {bits, prev, SAFE_COLOR_BITS}) {
    if (tryb < MIN_COLOR_BITS)
      continue;
    Adafruit_Protomatter* m = new Adafruit_Protomatter(DISPLAY_WIDTH, tryb, 1, rgbPins, NUM_ADDR_PINS, addrPins,
                                                       clockPin, latchPin, oePin, true);
    ProtomatterStatus status = m->begin();
    Serial.printf("matrix: %d bits per color, begin() status %d, largest internal block %u\n", tryb, status,
                  heap_caps_get_largest_free_block(MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL));
    if (status == PROTOMATTER_OK) {
      matrix = m;
      g_color_bits = tryb;
      matrix_drawer.setMatrix(m);
      if (live) {
        // is it really refreshing? if not, restart: at boot it's set up with the saved color_bits and that works
        // getFrameCount() is the frames since the previous call
        m->getFrameCount();
        uint32_t frames = 0;
        unsigned long start = millis();
        while (frames == 0 && millis() - start < 500) {
          delay(5);
          frames += m->getFrameCount();
        }
        if (frames == 0) {
          Serial.printf("matrix: %d bits doesn't refresh after the change, restarting to apply it\n", tryb);
          state->save();
          delay(100);
          ESP.restart();
        }
        Serial.printf("matrix: refreshing, first frame after %lu ms\n", millis() - start);
      }
      return tryb == bits;
    }
    delete m;
  }
  matrix_drawer.setMatrix(nullptr);
  g_color_bits = 0;
  return false;
}

void setupDisplay()
{
  setColorBits(state->top.color_bits.get());
  if (matrix == nullptr) {
    Serial.println("no display");
    return;
  }

  display.setDrawer(&matrix_drawer);
  display.clearDisplay();
  display.setTextColor(matrix_drawer.color565(0, 255, 255));
  display.setCursor(2,0);
  display.print("Pixel");
  display.setTextWrap(false);
  display.finish();

  Serial.println("initialized display");
  has_display = true;
}

void setupState()
{
    // the hundreds of props are small allocations (their names, the names index) that would all go to the
    // internal RAM, which WiFi and the web server need. while they are made, small allocations go to PSRAM too.
    // only while constructing: loading uses NVS (flash), keep that the default
    Serial.printf("heap before state: %u\n", ESP.getFreeHeap());
    heap_caps_malloc_extmem_enable(16);
    state = new State();
    heap_caps_malloc_extmem_enable(4096);  // the default threshold
    state->load();
    Serial.printf("heap after state: %u, psram free %u\n", ESP.getFreeHeap(), ESP.getFreePsram());
}

// MatrixPortal S3 on-board buttons:
//   UP click: next section, UP long-press: change(+1) (timer: +1 minute, repeats while held)
//   DOWN click: start/stop, DOWN long-press: change(-1) (timer: -1 minute, stop watch: reset)
#define BTN_UP PIN_BUTTON_UP
#define BTN_DOWN PIN_BUTTON_DOWN

void setupButtons()
{
  auto click_handler = [](int pin) {
    Serial.printf("click %d %p\n", pin, state->m_cur_screen);
    switch(pin) {
      case BTN_UP:
        state->top.active_section_btn(1);
        break;
      case BTN_DOWN:
        if (state->m_cur_screen)
          state->m_cur_screen->toggle_run(2);
        break;
    }
  };
  auto long_press_handler = [](int pin) {
    Serial.printf("long-press %d\n", pin);
    if (state->m_cur_screen == nullptr)
      return;
    switch(pin) {
      case BTN_UP:
        state->m_cur_screen->change(1);
        break;
      case BTN_DOWN:
        state->m_cur_screen->change(-1);
        break;
    }
  };

  buttons.setup( {
    Button(BTN_UP, click_handler, long_press_handler),
    Button(BTN_DOWN, click_handler, long_press_handler),
  });
}

void setup(void)
{
  // holding the board's Up or Down button while it starts opens the admin view
  pinMode(BTN_UP, INPUT_PULLUP);
  pinMode(BTN_DOWN, INPUT_PULLUP);
  delay(5);
  g_admin = digitalRead(BTN_UP) == LOW || digitalRead(BTN_DOWN) == LOW;

  setupSerial();
  if (g_admin)
    Serial.println("admin view");
  setupFs(); // before the state, the draw mode loads its drawing from it
  setupState();
  setupPins();

  // the display before WiFi and BLE take memory, its buffer needs a big contiguous block of internal RAM
  setupDisplay();

  connectWifi();
  setupNtp();
  setupWeb();

  setupButtons();
  orientation.begin();
  ble_hid_begin();
}


bool updateTime()
{
  if (WiFi.status() == WL_CONNECTED) {
    timeClient.update();
  }
  unsigned long prevEpochTime = g_epoch_time;
  g_epoch_time =  timeClient.getEpochTime();
  state->clock_state.m_epoch_time = g_epoch_time;
  return (g_epoch_time != prevEpochTime);
}


// The hidden admin view: when the board's Up or Down button is held while it starts,
// or when the remote's Power turns the display on while Home is held.
// a list of lines, the remote's Up/Down select one, Left/Right change the editable ones, Back goes to the clock.
// a reset without holding a button goes back to the clock

class AdminScreen : public IScreen
{
public:
    void on_hid(const HidEvent& ev) {
        if (ev.type != HidEvent::KEY_DOWN)
            return; // a release is shown at the top by update()
        switch (ev.key) {
            case HID_KEY_UP:    m_sel = (m_sel + LINES - 1) % LINES; break;
            case HID_KEY_DOWN:  m_sel = (m_sel + 1) % LINES; break;
            case HID_KEY_LEFT:  change(-1); break;
            case HID_KEY_RIGHT: change(1); break;
            default: return;
        }
        m_need_draw = true;
    }

    // the WiFi and the remote change by themselves, redrawn twice a second, and when a key comes
    bool update() {
        uint32_t keys;
        ble_hid_last_key(&keys);
        if (millis() - m_last_draw >= 500 || keys != m_drawn_keys)
            m_need_draw = true;
        bool r = m_need_draw;
        m_need_draw = false;
        return r;
    }

    // all in the small 4x6 font
    void draw() override {
        m_last_draw = millis();
        ble_hid_last_key(&m_drawn_keys);
        display.setDrawer(&matrix_drawer);
        display.clearDisplay();
        int w = display.width();
        int cols = w / CHAR_W;
        // the title and the last key from the remote, like the serial log. the key goes under it if it doesn't fit
        // a full circle for a press, an empty one for a release, then the code
        bool pressed = false;
        String key = ble_hid_last_key(nullptr, &pressed);
        small_text(0, 0, "ADMIN", rgb565(255, 210, 0));
        int y = 0;
        if (key.length() > 0) {
            const int mark = 2;  // the circle takes the room of 2 chars
            int col = 6;         // after "ADMIN "
            if (col + mark + (int)key.length() > cols) { // under the title if it doesn't fit
                y += LINE_H;
                col = 0;
            }
            uint16_t c = rgb565(120, 200, 255);
            int cx = col * CHAR_W + 2;
            if (pressed)
                display.fillCircle(cx, y + 2, 2, c);
            else
                display.drawCircle(cx, y + 2, 2, c);
            // the rest of this line after the circle, then whole lines (portrait)
            int pos = 0, room = cols - col - mark;
            small_text((col + mark) * CHAR_W, y, key.substring(0, room), c);
            for (pos = room; pos < (int)key.length(); pos += cols) {
                y += LINE_H;
                small_text(0, y, key.substring(pos, pos + cols), c);
            }
        }
        y += LINE_H + 2;
        for (int i = 0; i < LINES; ++i) {
            String t = text(i);
            int rows = max(1, ((int)t.length() + cols - 1) / cols); // long lines wrap (portrait)
            if (i == m_sel)
                display.fillRect(0, y - 1, w, rows * LINE_H + 1, rgb565(30, 40, 120));
            for (int r = 0; r < rows; ++r) {
                small_text(0, y, t.substring(r * cols, min((int)t.length(), (r + 1) * cols)),
                           editable(i) ? rgb565(255, 255, 255) : rgb565(170, 170, 190));
                y += LINE_H;
            }
        }
    }

private:
    enum { L_IP, L_SIGNAL, L_DNS, L_TZ, L_BRIGHTNESS, L_MOVE, L_REMOTE, L_IMU, LINES };
    int m_sel = 0;
    bool m_need_draw = true;
    unsigned long m_last_draw = 0;

    static bool editable(int i) { return i == L_TZ || i == L_BRIGHTNESS || i == L_MOVE; }

    static const int CHAR_W = 4; // the 4x6 font
    static const int LINE_H = 7;
    uint32_t m_drawn_keys = 0;

    static void small_text(int x, int y, const String& t, uint16_t color) {
        display.setFont(&MiscFixedMediumRNormal6607575C40iso88598); // 4x6, drawn from the top
        display.setTextSize(1);
        display.setTextColor(color);
        display.setCursor(x, y);
        display.print(t);
        display.setFont(nullptr);
    }

    void change(int d) {
        if (m_sel == L_TZ) {
            auto& tz = state->clock_state.m_panel.m_tz_offset;
            tz.set(constrain(tz.get() + d, -12, 14));
        }
        else if (m_sel == L_BRIGHTNESS) {
            auto& b = state->top.brightness;
            b.set(constrain((int)b.get() + d * 16, 0, 255));
        }
        else if (m_sel == L_MOVE) {
            auto& m = state->top.move_slow;
            m.set(constrain(m.get() + d * TopState::MOVE_SLOW_STEP, TopState::MOVE_SLOW_MIN, TopState::MOVE_SLOW_MAX));
        }
        else
            return;
        state->save();
    }

    // the selected editable one shows < > around its value
    String value(int i, const String& v) {
        return (i == m_sel) ? "<" + v + ">" : v;
    }

    String text(int i) {
        switch (i) {
            case L_IP:  return wifiStatusText();
            case L_SIGNAL: {
                if (WiFi.status() != WL_CONNECTED)
                    return "Signal -";
                int rssi = WiFi.RSSI();
                const char* q = rssi >= -55 ? "great" : rssi >= -67 ? "good" : rssi >= -75 ? "fair" : "weak";
                return String("Signal ") + rssi + " dBm " + q;
            }
            case L_DNS: return String(MDNS_NAME) + ".local";
            case L_TZ: {
                int tz = state->clock_state.m_panel.m_tz_offset.get();
                return "Time zone " + value(i, String(tz >= 0 ? "+" : "") + tz);
            }
            case L_BRIGHTNESS:
                return "Brightness " + value(i, String(state->top.brightness.get()));
            case L_MOVE: // 1.0 is normal, 2.0 moves the cursor half as much
                return "Move sensitivity " + value(i, String(state->top.move_slow.get() / 10.0f, 1));
            case L_REMOTE: {
                const char* d = ble_hid_device();
                return String("Remote ") + (*d ? d : "searching");
            }
            case L_IMU: {
                if (!orientation.found())
                    return "IMU not found";
                char buf[40];
                snprintf(buf, sizeof(buf), "IMU x=%.1f y=%.1f z=%.1f", orientation.ax(), orientation.ay(), orientation.az());
                return buf;
            }
        }
        return "";
    }
} admin_screen;

void set_section(int section)
{
  Serial.printf("section %d\n", section);
  state->top.active_section.set(section);
  state->top.save();
  if (section == SECTION_DRAW)
    state->drawtext.reset_pen();
  if (section == SECTION_GAMES)
    state->games.enter();
}

// d=1 next, d=-1 previous
void cycle_hid_mode(int d)
{
  const int n = sizeof(HID_MODES) / sizeof(HID_MODES[0]);
  int cur = state->top.active_section.get();
  int idx = -1;
  for (int i = 0; i < n; ++i)
    if (HID_MODES[i] == cur)
      idx = i;
  if (idx == -1)
    idx = 0;  // from a mode that is not in the cycle
  else
    idx = (idx + d + n) % n;
  set_section(HID_MODES[idx]);
}

// Enter: start/stop, Right/Left: +-1 minute, Volume+/-: +-10 minutes, Back: stop and back to the set time
// returns true if handled
bool handleTimerKey(const HidEvent& ev)
{
  if (ev.type != HidEvent::KEY_DOWN)
    return false;
  TimerState& timer = state->timer;
  switch (ev.key) {
    case HID_KEY_ENTER:
      if (!ev.repeat)
        timer.toggle_run(2);
      return true;
    case HID_KEY_BACK:
      if (!ev.repeat)
        timer.m_panel.reset();
      return true;
    case HID_KEY_RIGHT:    timer.change(1); return true;   // change() doesn't go below 0
    case HID_KEY_LEFT:     timer.change(-1); return true;
    case HID_KEY_VOL_UP:   timer.change(10); return true;
    case HID_KEY_VOL_DOWN: timer.change(-10); return true;
  }
  return false;
}

// Enter: start/stop, Back: stop and reset to 0
bool handleStopwatchKey(const HidEvent& ev)
{
  if (ev.type != HidEvent::KEY_DOWN || ev.repeat)
    return false;
  StopWState& stopw = state->stopw;
  switch (ev.key) {
    case HID_KEY_ENTER:
      stopw.toggle_run(2);
      return true;
    case HID_KEY_BACK:
      stopw.toggle_run(0);
      stopw.m_panel.reset();
      return true;
  }
  return false;
}

// the Power key of the remote turns the display off, Power again or Up/Down turn it back on.
// Power while Home is held turns it on in the admin view
bool g_display_off = false;
bool g_home_held = false;

// the remote's movement divided by the admin view's move sensitivity. the fractions are kept for the next
// movement, so slow movements still move
HidEvent scaleMove(const HidEvent& ev)
{
  static float rest_x = 0, rest_y = 0;
  int slow = state->top.move_slow.get();
  if (ev.type != HidEvent::MOVE || slow <= TopState::MOVE_SLOW_MIN)
    return ev;
  HidEvent r = ev;
  rest_x += ev.dx * 10.0f / slow;
  rest_y += ev.dy * 10.0f / slow;
  r.dx = (int16_t)rest_x;  // towards 0, the rest has the same sign
  r.dy = (int16_t)rest_y;
  rest_x -= r.dx;
  rest_y -= r.dy;
  return r;
}

void handleHidEvent(const HidEvent& ev)
{
  bool press = ev.type == HidEvent::KEY_DOWN && !ev.repeat;
  // a remote that sends one key at a time sends the Power press before the release of Home
  if (ev.key == HID_KEY_REMOTE_HOME && ev.type != HidEvent::MOVE)
    g_home_held = ev.type == HidEvent::KEY_DOWN;
  if (press && ev.key == HID_KEY_POWER) {
    if (g_display_off && g_home_held && !g_admin) {
      Serial.println("display on, admin view");
      g_admin = true;
      g_force_redraw = true;
    }
    g_display_off = !g_display_off;
    Serial.printf("display %s\n", g_display_off ? "off" : "on");
    return;
  }
  if (g_display_off) {
    if (press && (ev.key == HID_KEY_UP || ev.key == HID_KEY_DOWN)) {
      g_display_off = false;
      Serial.println("display on");
    }
    return;  // nothing else while it's off
  }
  if (g_admin) {
    if (press && ev.key == HID_KEY_BACK) { // out of the admin view, to the clock
      g_admin = false;
      set_section(SECTION_CLOCK);
      g_force_redraw = true;
      return;
    }
    admin_screen.on_hid(ev);
    return;
  }
  int section = state->top.active_section.get();
  // while typing, all the keys go to the editor and the movement is ignored, Esc stops typing
  // in a game all the input goes to it, in the games menu Up/Down still change the mode
  if (section == SECTION_GAMES && state->games.on_hid(ev))
    return;
  TypeState& text = state->drawtext.m_text;
  if (section == SECTION_DRAW) {
    // asking whether to clear takes all the input
    if (state->drawtext.on_confirm_hid(ev))
      return;
    if (ev.type == HidEvent::KEY_DOWN && !ev.repeat && ev.key == HID_KEY_BACK) {
      state->drawtext.ask_clear();
      return;
    }
  }
  if (section == SECTION_DRAW && text.m_editing) {
    if (ev.type == HidEvent::KEY_DOWN && ev.key == HID_KEY_CAPS_LOCK)
      state->drawtext.request_draw();  // the icon shows the caps lock state
    text.on_hid(ev);
    return;
  }
  if (ev.type == HidEvent::KEY_DOWN && !ev.repeat && (ev.key == HID_KEY_UP || ev.key == HID_KEY_DOWN)) {
    cycle_hid_mode(ev.key == HID_KEY_DOWN ? 1 : -1);
    return;
  }
  if (section == SECTION_TIMER && handleTimerKey(ev))
    return;
  if (section == SECTION_STOPW && handleStopwatchKey(ev))
    return;
  if (ev.type == HidEvent::KEY_DOWN && !ev.repeat) {
    if (ev.key == HID_KEY_ESC) {
      if (section != SECTION_DRAW)
        set_section(SECTION_DRAW);
      text.set_editing(true);
      return;
    }
  }
  if (section == SECTION_DRAW)
    state->drawtext.on_hid(ev);
}

// small icon at the bottom left that identifies the mode
void drawModeIcon(int section)
{
  const uint8_t* icon = nullptr;
  uint16_t color = MODE_ICON_COLOR;
  switch (section) {
    case SECTION_CLOCK: icon = icon_clock; break;
    case SECTION_STOPW: icon = icon_stopwatch; break;
    case SECTION_TIMER: icon = icon_timer; break;
    case SECTION_DRAW:
      // only while a cursor is shown: T while typing, the pencil while the remote draw cursor is visible
      if (state->drawtext.m_text.m_editing)
        icon = ble_hid_caps_lock() ? icon_type : icon_type_lower;
      else if (state->drawtext.cursor_visible())
        icon = icon_movedraw;
      else
        return;
      color = hid_palette[state->drawtext.color()];
      break;
    default: return;
  }
  display.setDrawer(&matrix_drawer);
  // in the current orientation, so it's at the bottom left of whichever side the panel stands on
  int y = display.height() - MODE_ICON_SIZE;
  display.fillRect(0, y, MODE_ICON_SIZE, MODE_ICON_SIZE, 0);
  display.drawBitmap(0, y, icon, MODE_ICON_SIZE, MODE_ICON_SIZE, color);
}

int g_loop_count = 0;
int g_prev_section = -1;
int g_prev_brightness = -1;
int g_prev_gamma = -1;
int g_prev_rotation = -1;
bool g_prev_display_off = false;

void loop(void)
{
  ++g_loop_count;

  HidEvent hid_ev;
  while (ble_hid_poll(hid_ev))
    handleHidEvent(scaleMove(hid_ev));

  // the web-ui opened a tab or draws: the display turns on, drawing shows the draw (or image) mode
  if (g_web_wake) {
    g_web_wake = false;
    if (g_display_off) {
      g_display_off = false;
      int s = g_web_wake_section;
      Serial.printf("display on from the web-ui, section %d\n", s);
      if (s >= 0 && s != state->top.active_section.get())
        set_section(s);
    }
  }

  // the prefs from the web-ui, when they stopped changing
  unsigned long save_at = g_save_at;
  if (save_at != 0 && millis() - save_at >= SAVE_QUIET_MS) {
    g_save_at = 0;
    state->save();
  }

  // a period whose face has "show when it starts" starts: the clock is shown, whatever was shown, also when the
  // display was off or in a game. only when the time is known (from NTP), not when it's already on at boot
  static int prev_wake = -2;
  if (g_epoch_time > 1600000000UL) {
    int wp = state->clock_state.m_panel.wake_period(g_epoch_time);
    if (wp >= 0 && wp != prev_wake && prev_wake != -2) {
      Serial.printf("period %d starts, to the clock\n", wp);
      g_display_off = false;
      if (state->top.active_section.get() != SECTION_CLOCK)
        set_section(SECTION_CLOCK);
    }
    prev_wake = wp;
  }

  int section = state->top.active_section.get();
  int brightness = state->top.brightness.get();
  if (orientation.update())
    display.setRotation(orientation.rotation());
  int rotation = display.getRotation();
  // brightness is applied when drawing, so a change needs a redraw, same for a new orientation
  int gamma = state->top.gamma.get();
  bool section_changed = (section != g_prev_section) || (brightness != g_prev_brightness) || (rotation != g_prev_rotation)
                         || (gamma != g_prev_gamma) || g_force_redraw;
  g_force_redraw = false;
  g_prev_gamma = gamma;
  if (g_display_off != g_prev_display_off) {
    g_prev_display_off = g_display_off;
    if (g_display_off && has_display) { // blank once
      display.setDrawer(&matrix_drawer);
      display.clearDisplay();
      display.finish();
    }
    section_changed = true; // when it's back on, draw the mode again
  }
  g_prev_section = section;
  g_prev_brightness = brightness;
  g_prev_rotation = rotation;

  // a new bit depth from the web-ui. tried once per new value, if it can't be done color_bits_actual says so
  static int tried_bits = -1;
  int bits = state->top.color_bits.get();
  if (has_display && bits != tried_bits) {
    tried_bits = bits;
    if (bits != g_color_bits) {
      setColorBits(bits);
      section_changed = true; // the new matrix is empty
    }
  }

  display.setDrawer(&matrix_drawer);
  matrix_drawer.setBrightness(brightness);
  matrix_drawer.setGamma(gamma);

  bool time_changed = updateTime();
  bool timer_need_draw = state->timer.m_panel.update_time();
  bool stopw_need_draw = state->stopw.m_panel.update_time();
  bool drawtext_need_draw = state->drawtext.update();
  bool image_need_draw = state->image.update();
  bool clock_need_draw = (section == SECTION_CLOCK) && state->clock_state.update(); // special times
  bool games_need_draw = (section == SECTION_GAMES) && !g_display_off && state->games.update(); // no playing in the dark

  IScreen* screen = nullptr;

  if ((time_changed || section_changed || clock_need_draw) && section == SECTION_CLOCK)
  {
    screen = &state->clock_state;
  }

  if ((timer_need_draw || section_changed) && section == SECTION_TIMER)
  {
    screen = &state->timer;
  }

  if ((stopw_need_draw || section_changed) && section == SECTION_STOPW)
  {
    screen = &state->stopw;
  }

  if ((drawtext_need_draw || section_changed) && section == SECTION_DRAW)
  {
    screen = &state->drawtext;
  }

  // redrawn when the brightness changes too, like the other modes
  if ((image_need_draw || section_changed) && section == SECTION_IMAGE)
  {
    screen = &state->image;
  }

  if ((games_need_draw || section_changed) && section == SECTION_GAMES)
  {
    screen = &state->games;
  }

  if ((section_changed || g_off_redraw) && section == SECTION_OFF)
  {
    g_off_redraw = false;
    screen = &off_screen;
  }

  if (g_admin) // instead of the mode
    screen = (admin_screen.update() || section_changed) ? &admin_screen : nullptr;

  if (screen != nullptr && has_display && !g_display_off) {
    state->m_cur_screen = screen;
    screen->draw();
    // not in the admin view, and not when the clock's face is off: then nothing is lit
    if (!g_admin && !(section == SECTION_CLOCK && state->clock_state.dark()))
      drawModeIcon(section);
    // finish() flushes the halo drawer, it has the kept outlined text and then flushes the matrix
    display.setDrawer(&halo_drawer);
    display.finish();
  }

  startMdns(); // once WiFi has an IP
  buttons.scan();
  ws.cleanupClients();

  // a status line every 30 seconds, for when the device stops answering on the network
  static unsigned long last_status = 0;
  if (millis() - last_status >= 30000) {
    last_status = millis();
    Serial.printf("status: wifi %s rssi %d, heap free %u min %u largest block %u, psram free %u\n",
                  WiFi.status() == WL_CONNECTED ? "connected" : "NOT connected", WiFi.RSSI(),
                  ESP.getFreeHeap(), ESP.getMinFreeHeap(),
                  heap_caps_get_largest_free_block(MALLOC_CAP_8BIT | MALLOC_CAP_INTERNAL), ESP.getFreePsram());
  }
}
