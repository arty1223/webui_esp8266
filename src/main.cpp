#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include "LittleFS.h"
#include <Arduino_JSON.h>
#include <EEPROM.h>

#define AP_SSID "СЕТЬ"
#define AP_PASS "ПАРОЛЬ"

// Create AsyncWebServer object on port 80
AsyncWebServer server(80);


AsyncWebSocket ws("/ws");

String message = "";

String sliderValue = "0";


int dutyCycle;

//Json Variable to Hold Values
JSONVar Values;

void handleMain();
void handleNotFound();

//Get Slider Values
String getSliderValues(){
  // Values["CurrentValue"] = String(CurrentValue);
  Values["sliderValue"] = String(sliderValue);
  String jsonString = JSON.stringify(Values);
  return jsonString;
}

float measure();
// Initialize LittleFS
void initFS() {
  if (!LittleFS.begin()) {
    Serial.println("An error has occurred while mounting LittleFS");
  }
  else{
   Serial.println("LittleFS mounted successfully");
  }
}

// Initialize WiFi
void initWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(AP_SSID, AP_PASS);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
 
  Serial.println(WiFi.localIP());
}

//Обновление сервера
void notifyClients(String Values) {
  ws.textAll(Values);
}

//обработка сообщение от js
void handleWebSocketMessage(void *arg, uint8_t *data, size_t len) {
  AwsFrameInfo *info = (AwsFrameInfo*)arg;
  if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT) {
    data[len] = 0;
    message = (char*)data;
    if (message.indexOf("1s") >= 0) {
      sliderValue = message.substring(2);      
      // dutyCycle = map(sliderValue.toInt(), 0, 100, 0, 1023);
      dutyCycle = sliderValue.toInt();
      Serial.println(dutyCycle);
      Serial.print(getSliderValues());
      notifyClients(getSliderValues());
      EEPROM.put(0,dutyCycle);
      EEPROM.commit();
    }
    if (strcmp((char*)data, "getValues") == 0) {
      notifyClients(getSliderValues());
    }
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
    case WS_EVT_DATA:
      handleWebSocketMessage(arg, data, len);
      break;
    case WS_EVT_PONG:
    case WS_EVT_ERROR:
      break;
  }
}

void initWebSocket() {
  ws.onEvent(onEvent);
  server.addHandler(&ws);
} 

void setup() {
  Serial.begin(115200);
  EEPROM.begin(4);
  EEPROM.get(0, dutyCycle);
  Serial.println("");
  Serial.println(dutyCycle);
  initFS();
  initWiFi();
  initWebSocket();
  
  // Web Server Root URL
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(LittleFS, "/index.html", "text/html");
  });
  
  server.serveStatic("/", LittleFS, "/");
  // Start server
  server.begin();
  sliderValue = String(dutyCycle);
  Values["sliderValue"] = sliderValue;
  // Serial.println(JSON.stringify(Values));
  notifyClients(JSON.stringify(Values));
}

void loop() {
  static uint64_t tmr, tmr2;
  static float current; // текущее значение тока
  static JSONVar CurVal; 
  if(millis() - tmr > 50){    
    tmr = millis();
    current = measure();
  }
  if(millis() - tmr2 > 250){
    tmr2 = millis();
    CurVal["CurrentValue"] = String(current);
    notifyClients(JSON.stringify(CurVal));    
  }  
  ws.cleanupClients();
}

float measure(){
  float cur = analogRead(A0);  
  // возможно нужна дополнительная калибровка, я добился точности в 1-6 сотых на всем диапозоне
  // R1 = 1.2k, R2 = 330, 0-15.3v 14.7 в формуле - подбирал чтобы было ± точно с моими резисторами
  // u = ((u - 0) * 14.7) / 4095; - для 32   
  // cur = ((cur - 0) * 14.76) / 1023; // u напряжение с выхода датчика  
  cur = (cur * 3.08) / 10.23 - 169.81;
  // float I = (cur - U / 2) / (U * 0.02);
  // Serial.print(cur); Serial.print(" current:");  // Serial.println(I);
  return cur;
}