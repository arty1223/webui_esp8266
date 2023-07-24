#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include "LittleFS.h"
#include <Arduino_JSON.h>
#include <EEPROM.h>
#include <TLC5615.h>

#define AP_SSID "ИМЯ СЕТИ"
#define AP_PASS "ПАРОЛЬ"

// Create AsyncWebServer object on port 80
AsyncWebServer server(80);
AsyncWebSocket ws("/ws");
TLC5615 dac(D8);

String message = "";
String sliderValue = "0";

String ButtValue1 = "false";
String ButtValue2 = "false";
String ButtValue3 = "false";
String ButtValue4 = "false";

#define PowerON D4
#define EmergencyShutdown D3
#define PositiveInput D0
#define NegativeInput D6

int dutyCycle;

bool Button1,Button2,Button3,Button4;

//Json Variable to Hold Values
JSONVar Values;

void handleMain();
void handleNotFound();

//Get Values
String getSliderValues(){  
  Values["sliderValue"] = sliderValue;
  String jsonString = JSON.stringify(Values);
  return jsonString;
}

String getButtonValues(){  
  Values["buttonValue1"] = ButtValue1;
  Values["buttonValue2"] = ButtValue2;
  Values["buttonValue3"] = ButtValue3;
  Values["buttonValue4"] = ButtValue4;
  String jsonString = JSON.stringify(Values);
  return jsonString;
}

float measure();
int DAC(int val);
// Initialize LittleFS
void initFS() {
  if (!LittleFS.begin()) {
    Serial.println("An error has occurred while mounting LittleFS");
  }
  else{
   Serial.println("LittleFS mounted successfully");
  }
}

// WiFi
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
      // Serial.println(dutyCycle);
      // Serial.print(getSliderValues());
      notifyClients(getSliderValues());
      EEPROM.put(0,dutyCycle);
      EEPROM.commit();      
    }
    
    bool val;
    if (message.substring(2) == "true"){
      val = true;
    }else{
      val = false;
    }

    if (message.indexOf("1b") >= 0) {
      // ButtValue1 = message.substring(2);      
      // dutyCycle = map(sliderValue.toInt(), 0, 100, 0, 1023);
      Button1 = val;
      // Serial.println(Button1);
      // Serial.println(getButtonValues());
    }
    if (message.indexOf("2b") >= 0) {
      // ButtValue2 = message.substring(2);      
      // Button1 = map(sliderValue.toInt(), 0, 100, 0, 1023);
      Button2 = val;
      // Serial.println(Button2);
      // Serial.println(getButtonValues());
    }
    if (message.indexOf("3b") >= 0) {
      // ButtValue3 = message.substring(2);      
      // Button1 = map(sliderValue.toInt(), 0, 100, 0, 1023);
      Button3 = val;
      // Serial.println(Button3);
      // Serial.println(getButtonValues());
    }
    if (message.indexOf("4b") >= 0) {
      // ButtValue4 = message.substring(2);      
      // dutyCycle = map(sliderValue.toInt(), 0, 100, 0, 1023);
      Button4 = val;
      // Serial.println(Button4);
      // Serial.println(getButtonValues());
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

float measure(){ // Измерение потребляемого тока;
  float cur = analogRead(A0);
  cur = (cur * 3.08) / 10.23 - 169.81;
  return cur;
}

int DAC(int val){// Преобразование значения val (0 - 100) в напряжение 0 - 3V в 10битном формате
  int voltage;
  voltage = (169.81 + val) * 10.23 / 3.08;
  return voltage;
}

void setup() {
  Serial.begin(115200);
  EEPROM.begin(4);
  EEPROM.get(0, dutyCycle);

  pinMode(D0,OUTPUT);
  pinMode(D6,OUTPUT);
  pinMode(D3,OUTPUT);
  pinMode(D4,OUTPUT);
  
  digitalWrite(D6,1);
  digitalWrite(D0,0); // При нажатии кнопки сброса D6 0, D0 1

  digitalWrite(D3,0);
  digitalWrite(D4,1); // Кнопка включения

  // Serial.println("");
  // Serial.println(dutyCycle);
  dac.begin();
  dac.analogWrite(DAC(dutyCycle));

  initFS();
  initWiFi();
  initWebSocket();
  
  //Коренной каталог
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(LittleFS, "/index.html", "text/html");
  });
  
  server.serveStatic("/", LittleFS, "/");
  
  server.begin();

  sliderValue = String(dutyCycle);
  Values["sliderValue"] = sliderValue;
  // Serial.println(JSON.stringify(Values));
  notifyClients(JSON.stringify(Values));
}

void loop() {
  static uint64_t tmr, tmr2, tmrb1, tmrb2, tmrb3;
  static float current; // текущее значение тока
  static JSONVar CurVal;  
  static int dacval;

  dacval = DAC(dutyCycle);
  dac.analogWrite(dacval);

  if(millis() - tmr > 50){    
    tmr = millis();
    current = measure();
  }
  if(millis() - tmr2 > 250){
    tmr2 = millis();
    CurVal["CurrentValue"] = String(current);
    notifyClients(JSON.stringify(CurVal));
  }

  if(Button1){
    tmrb1 = millis();
    digitalWrite(PositiveInput,Button1);
    digitalWrite(NegativeInput,!Button1);
  }
  if(millis() - tmrb1 > 500){
    tmrb1 = millis();
    Button1 = false;
    digitalWrite(PositiveInput,Button1);
    digitalWrite(NegativeInput,!Button1);
  }

  if(Button2){
    tmrb2 = millis();
    digitalWrite(PowerON,!Button2);    
  }
  if(millis() - tmrb2 > 500){
    tmrb2 = millis();
    Button2 = false;
    digitalWrite(PowerON,!Button2);  
  }
  
  if(Button3){
    tmrb3 = millis();
    digitalWrite(PowerON,!Button3);    
  }
  if(millis() - tmrb3 > 5000){
    tmrb3 = millis();
    Button3 = false;
    digitalWrite(PowerON,!Button3);  
  }

  // digitalWrite(EmergencyShutdown,Button4);

  ws.cleanupClients();
}