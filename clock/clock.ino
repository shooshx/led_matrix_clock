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
//#include "SPIFFS.h"
#include <Preferences.h>

#include <ArduinoJson.h>
#include <PxMatrix.h>

#include "my_fonts/fonts_index.h"
#include "ClockState.h"
#include "base_utils.h"


// Replace with your network credentials
const char* SSID = "TheCatsMew";
const char* PASSW = "abcdeg123458";

//int LED_BUILTIN = 2;

//WebServer server(80);

AsyncWebServer server(80);
AsyncWebSocket ws("/ws"); // access at ws://[esp ip]/ws

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP);


unsigned long epochTime;
int tzOffset = +3;


#define P_LAT 22
#define P_A 19
#define P_B 23
#define P_C 18
#define P_D 5
#define P_E 15
#define P_OE 2

PxMATRIX display(64,32,P_LAT, P_OE,P_A,P_B,P_C,P_D);
bool has_display = false;
bool has_serial = false;

enum Section {
    SECTION_CLOCK = 0,
    SECTION_DRAW = 1
};

class TopState : public PropHolder<1>
{
public:
    Prop<int16_t> active_section;
    Preferences m_pref;

    TopState(NamesIndex* prop_map)
      : PropHolder(prop_map)
      , active_section(this, "active_section", 0)
    {}

    void load() {
        m_pref.begin("top", false);
        PropHolder::load(m_pref);
    }
    void save() {
        PropHolder::save(m_pref);
    }
};

class State
{
public:
    NamesIndex m_prop_map;
    ClockState clock_state;
    TopState top;


    State()
        : clock_state(&m_prop_map)
        , top(&m_prop_map)
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
        //zSerial.printf("found %s, going to call setInt %d\n", name.c_str(), v);
        return prop->setInt(v);
    }
    
    void toJson(const JsonObject& root)
    {
        top.toJson(root);
        clock_state.toJson(root.createNestedObject("clock"));
    }

    void load()
    {
        Serial.printf("props: %d\n", m_prop_map.size());
        clock_state.load();
        top.load();
    }
    void save()
    {
        clock_state.save();
	top.save();
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

  if(!MDNS.begin("myesp32"))
    Serial.println("Error starting mDNS");
  else
    Serial.println("mDNS responder started");
  
}

void setupNtp()
{
  timeClient.begin();
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

struct PixParse
{
    int i = 0;
    int state = 0;
    int expect_count = 0;
    int count = 0;
    int x = 0, y = 0, col = 0;

    void operator()(const char* s) {
        //Serial.printf("PP `%s` - %d, %d, %d\n", s, i, state, count);
        if (i == 0) {
            ++i;  // command
            return;
        }
        if (i == 1) {
            expect_count = atoi(s);
            ++i;  // pixel count
            return;
        }
        if (state == 0)
            x = atoi(s);
        else if (state == 1)
            y = atoi(s);
        else {
            col = atoi(s);
            display.drawPixel(x, y, col);
            Serial.printf("PP-draw %d,%d, %d\n", x, y, col);
            ++count;
        }
        state = (state + 1) % 3;
        ++i;
    }
};

void parse_pixel_cmd(String& line)
{
    PixParse pp;
    strSplitStream(line, pp);
    if (pp.count != pp.expect_count)
        Serial.printf("draw command parsed unexpected number of pixels %d, %d\n", pp.count, pp.expect_count);
}


void handleWebSocketMessage(uint8_t *data, size_t len) {
    std::vector<char> v;
    v.resize(len + 1);
    memcpy(&v[0], data, len);
    v[len] = 0;
    String sp[3];
    String line(&v[0]);
    Serial.printf("WebSocket data `%s`\n", line.c_str());

    if (line.length() < 2)
        return;
    char cmd = line[0];
    char subcmd = line[1];
    if (cmd == 'U')
    {
        int count = strSplit(line, sp, 3);
        if (count != 3) {
            Serial.printf("Unexpected number of args of UC %d\n", count);
            return;
        }
        //Serial.printf("prop update %s %d\n", sp[1].c_str(), sp[2].toInt());
        if (state->update_prop(sp[1], sp[2].toInt())) {
            state->save();
        }
    }
    else if (cmd == 'D')
    {
        if (subcmd == 'P')
            parse_pixel_cmd(line);        
        else
            Serial.printf("Unknown subcmd %s\n", sp[0].c_str());
    }
    else
    {
        Serial.printf("Unknown command %s\n", sp[0].c_str());
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
      if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT) {
          handleWebSocketMessage(data, len);
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
        request->send(200, "text/plain", String(epochTime));
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
#if 0
    server.onFileUpload([](AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final){
       //Handle upload
    });
    server.onRequestBody([](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total){
       //Handle body
    });
#endif
    
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
        display.display(70);
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

  timer = timerBegin(0, 80, true);
  timerAttachInterrupt(timer, &display_updater, true);
  timerAlarmWrite(timer, 4000, true);
  timerAlarmEnable(timer);
}


void IRAM_ATTR display_updater_old(){
  // Increment the counter and set the time of ISR
  portENTER_CRITICAL_ISR(&timerMux);
  display.display(70);
  //display.displayTestPattern(70);
  portEXIT_CRITICAL_ISR(&timerMux);
}

void setupDisplayISR_old()
{
  timer = timerBegin(0, 80, true);
  timerAttachInterrupt(timer, &display_updater_old, true);
  timerAlarmWrite(timer, 4000, true);
  timerAlarmEnable(timer);
}



uint16_t myCYAN = display.color565(0, 255, 255);
uint16_t myRED = display.color565(255, 0, 0);

void setupDisplay() 
{
  display.begin(8);
  display.setPanelsWidth(2);
  display.clearDisplay();
  display.flushDisplay();

  display.setTextColor(myCYAN);
  display.setCursor(2,0);
  display.print("Pixel");
  display.setTextWrap(false);

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
}

bool updateTime()
{
  if (WiFi.status() != WL_CONNECTED)
    return false;
  timeClient.update();
  unsigned long prevEpochTime = epochTime;
  epochTime =  timeClient.getEpochTime();
  return (epochTime != prevEpochTime);
}



int g_loop_count = 0;
int g_last_count = 0;

void loop(void){
  ++g_loop_count;
  // MDNS.update(); not needed?
  
  bool timeChanged = updateTime(); // TODO: needed this often?

  if (timeChanged && state->top.active_section.get() == SECTION_CLOCK)
  {
    Timer t;
    
    time_t utc = epochTime;
    time_t local = utc + tzOffset * 3600;
    if (has_display)
    {
        state->clock_state.draw(local);
        display.drawPixel(0, epochTime % 32, 0xffff);
    }

    //Serial.printf("draw took %d loops:%d\n", t.elapsed(), g_loop_count - g_last_count);
    g_last_count = g_loop_count;

  }

  ws.cleanupClients();
  /*
  digitalWrite(LED_BUILTIN, LOW);  
  delay(500);
  digitalWrite(LED_BUILTIN, HIGH); 
  delay(500);
  */
}
