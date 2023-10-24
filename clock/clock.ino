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

ClockState clock_state;

void setupPins()
{
  pinMode(LED_BUILTIN, OUTPUT);
}

void setupSerial()
{
  Serial.begin(115200);         // Start the Serial communication to send messages to the computer
  delay(10);
  Serial.println('\n');
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


void onEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len) {
  switch (type) {
    case WS_EVT_CONNECT:
      Serial.printf("WebSocket client #%u connected from %s\n", client->id(), client->remoteIP().toString().c_str());
      break;
    case WS_EVT_DISCONNECT:
      Serial.printf("WebSocket client #%u disconnected\n", client->id());
      break;
    case WS_EVT_DATA:
      //handleWebSocketMessage(arg, data, len);
      Serial.printf("WebSocket data len=%d\n", len);
      break;
    case WS_EVT_PONG:
    case WS_EVT_ERROR:
      Serial.printf("WebSocket error len=%d", len);
      break;
    default:
      Serial.printf("WebSocket unknown event %d\n", type);
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
        clock_state.toJson(root.createNestedObject("clock"));
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

void setup(void)
{
  setupPins();
  setupSerial();
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



void loop(void){
  // MDNS.update(); not needed?
  
  bool timeChanged = updateTime(); // TODO: needed this often?

  if (timeChanged)
  {
    //unsigned long start_time=micros();
    
    time_t utc = epochTime;
    time_t local = utc + tzOffset * 3600;
    if (has_display)
    {
        clock_state.draw(local);
    }
    //unsigned long elapsed = micros() - start_time;
    //Serial.print("draw took ");
    //Serial.println(elapsed);

  }

  ws.cleanupClients();
  /*
  digitalWrite(LED_BUILTIN, LOW);  
  delay(500);
  digitalWrite(LED_BUILTIN, HIGH); 
  delay(500);
  */
}
