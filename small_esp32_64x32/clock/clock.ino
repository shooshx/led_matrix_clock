#include <WiFi.h>
//#include <WebServer.h>
#include <ESPmDNS.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <AsyncTCP.h>
#include "ESPAsyncWebServer.h"
#include "AsyncJson.h"

#include <Time.h>
#include <TimeLib.h>
#include <LittleFS.h>
#include <Preferences.h>

#include "drawer.h"
#include <ArduinoJson.h>
#define double_buffer
#include "myPxMatrix.h"
#include "SerialMp3Player.h"

#include "my_fonts/fonts_index.h"
#include "ClockState.h"
#include "TimerState.h"
#include "StopWState.h"
#include "base_utils.h"
#include "Buttons.h"

#define DISPLAY_WIDTH 64
#define DISPLAY_HEIGHT 32

#include "image_handling.h"


// Replace with your network credentials
//const char* SSID = "TheCatsMew";
//const char* PASSW = "abcdeg123458";


//int LED_BUILTIN = 2;

//WebServer server(80);

AsyncWebServer server(80);
AsyncWebSocket ws("/ws"); // access at ws://[esp ip]/ws

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP);

#define P_LAT 22
#define P_A 19
#define P_B 23
#define P_C 18
#define P_D 5
#define P_E 15
#define P_OE 2

WrapGFX display(DISPLAY_WIDTH,DISPLAY_HEIGHT);
PxMATRIX matrix_drawer(DISPLAY_WIDTH,DISPLAY_HEIGHT,P_LAT, P_OE,P_A,P_B,P_C,P_D);

SerialMp3Player sound_player;

bool has_display = false;
bool has_serial = false;

unsigned long g_epoch_time = 0;
Buttons buttons;


enum Section {
    SECTION_OFF = 0,
    SECTION_CLOCK = 1,
    SECTION_TIMER = 2,
    SECTION_STOPW = 3,
    SECTION_DRAW = 4,
    SECTION_IMAGE = 5
};

#define SECTION_BTN_FIRST SECTION_CLOCK
#define SECTION_BTN_LAST SECTION_STOPW

class TopState : public PropHolder<2>
{
public:
    Prop<int16_t> active_section;
    Prop<uint16_t> brightness;
    Preferences m_pref;

    TopState(NamesIndex* prop_map)
      : PropHolder(prop_map)
      , active_section(this, "active_section", 0)
      , brightness(this, "brightness", 255)
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
    IScreen* m_cur_screen = nullptr;

    State()
        : clock_state(&m_prop_map)
        , top(&m_prop_map)
        , timer(&m_prop_map)
        , stopw(&m_prop_map)
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
        //Serial.printf("found %s, going to call setInt %d\n", name.c_str(), v);
        return prop->setInt(v);
    }
    
    void toJson(const JsonObject& root)
    {
        top.toJson(root);
        clock_state.toJson(root.createNestedObject("clock"));
        timer.toJson(root.createNestedObject("timer"));
        stopw.toJson(root.createNestedObject("stopw"));
    }

    void load()
    {
        Serial.printf("props: %d\n", m_prop_map.size());
        clock_state.load();
        top.load();
        timer.load();
        stopw.load();
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
}

void setupSerial()
{
  Serial.begin(115200);         // Start the Serial communication to send messages to the computer
  delay(10);
  Serial.println('\n');
  has_serial = true;
}

void listAllFilesInDir(String dir_path)
{
  File dir = LittleFS.open(dir_path);
  File f = dir.openNextFile();
  while(f) {
    if (!f.isDirectory()) {
      // print file names
      Serial.print("File: ");
      Serial.println(dir_path + f.name() + "  : " + f.size());
    }
    else {
      // print directory names
      Serial.print("Dir: ");
      Serial.println(dir_path + f.name() + "/");
      // recursive file listing inside new directory
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


void connectWifi()
{
  Serial.print("Connecting to ");
  Serial.println(SSID);
  WiFi.begin(SSID, PASSW);
  while (WiFi.status() != WL_CONNECTED) {
    delay(250);
    Serial.print(".");
  }
  // Print local IP address and start web server
  Serial.println("");
  Serial.println("connected.");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  if(!MDNS.begin("myesp32clock"))
    Serial.println("Error starting mDNS");
  else
    Serial.println("mDNS responder started");
  
  if (!MDNS.addService("http", "tcp", 80))
    Serial.println("MDNS addService failed");
  
}

void setupNtp()
{
  timeClient.begin();
  timeClient.setUpdateInterval(30 * 60000);
  // Set offset time in seconds to adjust for your timezone, for example:
  // GMT +1 = 3600
  //timeClient.setTimeOffset(3600);
}



void set_builtin_led(bool v) 
{
  if (v)
    digitalWrite(LED_BUILTIN, 1);  
  else
    digitalWrite(LED_BUILTIN, 0);  
}




void handleWebSocketMessage(const uint8_t *data, size_t len) 
{
    //Serial.printf("WebSocket data %d\n", (int)len);
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
    if (cmd == 'U')
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
        if (state->update_prop(sp[1], v)) {
            state->save();
        }
    }
    else if (cmd == 'D')
    {
        if (subcmd == 'P')
            parse_pixel_cmd(v);        
        else if (subcmd == 'C') {
            display.clearDisplay();
            display.finish();
        }
        else if (subcmd == 'I')
            parse_img_cmd(v);
        else
            Serial.printf("Unknown D subcmd %c\n", subcmd);
    }
    else if (cmd == 'T')
    {
        if (subcmd == 'R') {
            Serial.printf("stop watch reset\n");
            state->stopw.m_panel.reset();
        }
        else if (subcmd == 'P') {
            Serial.printf("play sound %d,%d\n", state->timer.m_panel.m_snd_vol.get(), state->timer.m_panel.m_snd_file_num.get());
            sound_player.set_volume(state->timer.m_panel.m_snd_vol.get());
            delay(200);
            sound_player.play_file(1, state->timer.m_panel.m_snd_file_num.get());
            delay(200);
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
      //Serial.printf("WS event final=%d, num=%d, index=%d, len=%d(%d), opcode=%d,%d\n", (int)(info->final ? 1 : 0), (int)info->num, (int)info->index, (int)info->len, (int)len, (int)info->opcode, (int)info->message_opcode);
      if (len != info->len) {
          auto total_len = g_frag.add(data, len, info->index);
          if (total_len == info->len)
            handleWebSocketMessage(g_frag.m_d.data(), g_frag.m_d.size());
      }
      else if (info->final && info->index == 0 && info->len == len && (info->opcode == WS_TEXT || info->opcode == WS_BINARY)) {
          handleWebSocketMessage(data, len);
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

  //server.addHandler(&events);
}


void setupWeb()
{
    setupWebSocket();

    // respond to GET requests on URL /heap
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

    server.on("/pref", HTTP_GET, [](AsyncWebServerRequest *request){
        Serial.println("got /pref req");
        AsyncJsonResponse * response = new AsyncJsonResponse(false, 1024);
        response->addHeader("Server","ESP Clock");
        JsonObject root = response->getRoot();
        state->toJson(root);
        response->setLength();
        request->send(response);
    });

    server.serveStatic("/", LittleFS, "/");

    // Catch-All Handlers Any request that can not find a Handler that canHandle it ends in the callbacks below.
    server.onNotFound([](AsyncWebServerRequest *request){
        //Handle Unknown Request
        request->send(404);
    });

    
    server.begin();
}

// from https://github.com/2dom/PxMatrix/issues/225
#define CORE_1 1
TaskHandle_t displayUpdateTaskHandle = NULL;

hw_timer_t * timer = NULL;
portMUX_TYPE timerMux = portMUX_INITIALIZER_UNLOCKED;

void IRAM_ATTR display_updater()
{
  portENTER_CRITICAL_ISR(&timerMux);
  BaseType_t xHigherPriorityTaskWoken = pdFALSE;
  //notify task to unblock it 
  vTaskNotifyGiveFromISR(displayUpdateTaskHandle, &xHigherPriorityTaskWoken );

  //display task will be unblocked
  if(xHigherPriorityTaskWoken){
    //force context switch
    portYIELD_FROM_ISR( );
  }
  portEXIT_CRITICAL_ISR(&timerMux);
}

 
void displayUpdateTask(void *)
{
  for(;;){
    //block here untill timer ISR unblocks task
    if (ulTaskNotifyTake( pdTRUE, portMAX_DELAY)){
        matrix_drawer.display(70);
        //vTaskDelay( 1 );
    }
  }
}

void setupDisplayISR()
{
  /* we create a new task here */
  xTaskCreatePinnedToCore(
    displayUpdateTask, /* where display() actually runs. */
    "displayUpdateTask", /* name of task. */
    2048, /* Stack size of task */
    NULL, /* parameter of the task */
    3, /* Highest priority so it is immediately launched on context switch after the ISR */
    &displayUpdateTaskHandle, /* Task handle to use for task notification */
    CORE_1);
#if 1
  timer = timerBegin(0, 80, true);
  timerAttachInterrupt(timer, &display_updater, true);
  timerAlarmWrite(timer, 4000, true);
  timerAlarmEnable(timer);
#endif
}


void setupDisplay() 
{
  matrix_drawer.begin(8);
  matrix_drawer.setFastUpdate(true);
  matrix_drawer.setPanelsWidth(2);
  matrix_drawer.clearDisplay();
  matrix_drawer.flushDisplay();

  display.setTextColor(matrix_drawer.color565(0, 255, 255));
  display.setCursor(2,0);
  display.print("Pixel");
  display.setTextWrap(false);
  display.finish();

  setupDisplayISR();
  //setupDisplayISR_old();
  
  Serial.println("initialized display");
  has_display = true;


  //delay(1000);
}

void setupState()
{
    state = new State();
    state->load();
}

void setupSound()
{
    sound_player.begin();
    Serial.println("initialized sound");
}

#define BTN_LEFT 26
#define BTN_RIGHT 17
#define BTN_MID 4
#define BTN_UP 12
#define BTN_DOWN 0

void setupButtons()
{
  auto click_handler = [](int pin) {
    Serial.printf("click %d %p\n", pin, state->m_cur_screen);
    switch(pin) {
      case BTN_LEFT:  
        state->top.active_section_btn(1); 
        break;
      case BTN_RIGHT: 
        state->top.active_section_btn(-1); 
        break;
      case BTN_MID: 
        if (state->m_cur_screen)
          state->m_cur_screen->toggle_run(2);
        break;
      case BTN_UP:
        if (state->m_cur_screen)
          state->m_cur_screen->change(1);
        break;
      case BTN_DOWN:
        if (state->m_cur_screen)
          state->m_cur_screen->change(-1);
        break;  
    }
  };
  auto long_press_handler = [](int pin) {
    Serial.printf("long-press %d\n", pin);
  };

  buttons.setup( {
    Button(BTN_LEFT, click_handler, long_press_handler),
    Button(BTN_RIGHT, click_handler, long_press_handler),
    Button(BTN_MID, click_handler, long_press_handler),
    Button(BTN_UP, click_handler, long_press_handler),
    Button(BTN_DOWN, click_handler, long_press_handler),
  });
}

void setup(void)
{
  setupSerial();
  setupState();
  setupPins();
  setupFs();
  
  connectWifi();
  setupNtp();
  setupWeb();

  setupDisplay();
  setupSound();
  setupButtons();
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



int g_loop_count = 0;
int g_last_count = 0;
int g_prev_section = -1;

void loop(void)
{
  ++g_loop_count;
  //Serial.printf("Loop %d\n", g_loop_count);
  int section = state->top.active_section.get();
  bool section_changed = (section != g_prev_section);
  g_prev_section = section;

  display.setDrawer(&matrix_drawer);
  matrix_drawer.setBrightness(state->top.brightness.get());
  
  bool time_changed = updateTime();
  bool timer_need_draw = state->timer.m_panel.update_time();
  bool stopw_need_draw = state->stopw.m_panel.update_time();

  IScreen* screen = nullptr;

  if ((time_changed || section_changed) && section == SECTION_CLOCK)
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

  if (screen != nullptr) {
    state->m_cur_screen = screen;
    //Serial.printf("draw %d %p\n", section, screen);
    screen->draw();
    //Serial.printf("finish\n");
    display.finish();
  }

  //Serial.printf("r\n");
  buttons.scan();
  ws.cleanupClients();
  /*
  digitalWrite(LED_BUILTIN, LOW);  
  delay(500);
  digitalWrite(LED_BUILTIN, HIGH); 
  delay(500);
  */
}


/*
    //Timer t;
    screen = &state->clock_state;

    // time passing indicator pixel
    //display.drawPixel(0, epochTime % 32, 0xffff);

    //Serial.printf("draw took %d loops:%d\n", t.elapsed(), g_loop_count - g_last_count);
    //g_last_count = g_loop_count;
*/
