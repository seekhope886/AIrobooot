#define oled_dsp
#ifdef oled_dsp
#include <U8x8lib.h>          //使用U8g2插件並選用文字專用U8x8lib.h,減低記憶體用量
#endif
#include <WiFi.h>
#include <WiFiClientSecure.h> //加密型客戶端連線
#include <WebServer.h>
#include <PubSubClient.h>
#include <Wire.h>
//#include <Adafruit_AHTX0.h>  //個人感覺用這個常會出亂子
#include <AHT20.h>             //所以使用這個簡單穩定
#include <ArduinoJson.h>       //要他的編解碼功能
#include "SPIFFS.h"
#ifdef oled_dsp
U8X8_SSD1306_128X64_NONAME_HW_I2C u8x8(/* reset=*/ U8X8_PIN_NONE);         
#endif
// ===== WiFi =====
const char* ssid = "要使用的WiFi網路SSID";
const char* password = "WiFi網路密碼";

// ===== HiveMQ =====
const char* mqtt_server = "從HiveMQ註冊後取得的連結.s1.eu.hivemq.cloud";
const int mqtt_port = 8883;
const char* mqtt_user = "從HiveMQ註冊後並登錄一個項目名稱";
const char* mqtt_pass = "項目登入密碼";

WebServer server(80);
WiFiClientSecure espClient;
PubSubClient client(espClient);
//Adafruit_AHTX0 aht;
AHT20 aht;

float temperature = 0;
float humidity = 0;
unsigned long lastRead = 0;

char st[20];                      //原本想在訊息收到或發送, 訊息字串無法直接覆蓋,所以改設定到st後外部更新顯示

// ===== GPIO 控制 callback =====
void mqttCallback(char* topic, byte* payload, unsigned int length) {
  String msg;
  Serial.print("mqttCallBack:");
  for(int i=0;i<length;i++) msg += (char)payload[i];
#ifdef oled_dsp
//  u8x8.setCursor(0,4);
//  u8x8.print(random(100));
#endif    
  Serial.println(msg);

  if(String(topic) == "esp32/gpio/control") {
    StaticJsonDocument<100> doc;
    DeserializationError error = deserializeJson(doc, msg);
    if(!error){
      int pin = doc["pin"];
      int state = doc["state"];
      pinMode(pin, OUTPUT);
      digitalWrite(pin, state);
      Serial.printf("GPIO %d -> %d\n", pin, state);
    }
  }
}

// ===== 讀取 SPIFFS 檔案 =====
void handleFile(String path, String contentType) {
  File file = SPIFFS.open(path, "r");
  if (!file) {
    server.send(404, "text/plain", "File Not Found");
    return;
  }
  server.streamFile(file, contentType);
  file.close();
}

// ===== JSON API =====
void handleData() {
  StaticJsonDocument<200> doc;
//  doc["temp"] = temperature;
  doc["temp"] = static_cast<int>(temperature + 0.5f);
//  doc["hum"] = humidity;
  doc["hum"] = static_cast<int>(humidity*100 + 0.5f);
  String json;
  serializeJson(doc, json);
  server.send(200, "application/json", json);
}

// ===== WiFi 連線 =====
void setup_wifi() {
  WiFi.begin(ssid, password);
  WiFi.setTxPower(WIFI_POWER_8_5dBm);     
  Serial.print("Connecting WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println(" connected");
}

// ===== MQTT 重連 =====
void reconnect() {
  while (!client.connected()) {
    Serial.print("Connecting MQTT...");
    if (client.connect("ESP32Client", mqtt_user, mqtt_pass)) {
      
      Serial.println("connected");
      client.subscribe("esp32/gpio/control");//連線後訂閱
#ifdef oled_dsp
  u8x8.clear();
#endif    
      
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5s");
      delay(5000);
    }
  }
}

void setup() {
  Serial.begin(115200);
  delay(2000);
  Wire.begin(8, 9);
#ifdef oled_dsp  
  u8x8.begin();
  u8x8.setPowerSave(0);
  u8x8.setFont(u8x8_font_amstrad_cpc_extended_r);
  u8x8.clear();
  u8x8.drawString(5,3,"Start...");
#endif
  // 初始化 SPIFFS
  if (!SPIFFS.begin(true)) {
    Serial.println("SPIFFS 初始化失敗");
    return;
  }

//  if (!aht.begin()) {
//    Serial.println("AHT20 初始化失敗");
//    while (1);
//  }
  aht.begin();

  setup_wifi();

  espClient.setInsecure(); // 使用 TLS，不驗證憑證
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(mqttCallback);

  // ===== Web 路由 =====
server.onNotFound([]() {
  String path = server.uri();
  String contentType = "text/plain";

  if (path.endsWith(".html")) contentType = "text/html";
  else if (path.endsWith(".css")) contentType = "text/css";
  else if (path.endsWith(".js")) contentType = "application/javascript";
  else if (path.endsWith(".jpg")) contentType = "image/jpeg";
  else if (path.endsWith(".png")) contentType = "image/png";

  handleFile(path, contentType);
});

  
  server.on("/", HTTP_GET, []() { handleFile("/index.html", "text/html"); });
//  server.on("/sensor", HTTP_GET, []() { handleFile("/sensor.html", "text/html"); });
  server.on("/data", HTTP_GET, handleData);
  server.on("/gpio", HTTP_GET, []() {
//    Serial.println("recv gpio req..");
      if(server.hasArg("pin") && server.hasArg("state")){
      int pin = server.arg("pin").toInt();
      int state = server.arg("state").toInt();
      pinMode(pin, OUTPUT);
      digitalWrite(pin, state);
      server.send(200, "text/plain", "OK");
    } else {
      server.send(400, "text/plain", "Missing arguments");
    }
    });
  server.begin();
}

void loop() {
  
  if (!client.connected()) {
    reconnect();
  }
  client.loop();
  server.handleClient();

  if (millis() - lastRead > 10000) {
    lastRead = millis();
//    sensors_event_t hum, temp;
//    aht.getEvent(&hum, &temp);
//    temperature = temp.temperature;
//    humidity = hum.relative_humidity;
      aht.getSensor(&humidity, &temperature);// populate temp and humidity objects with fresh data
      

    // 發布溫濕度到 HiveMQ
    StaticJsonDocument<200> doc;
    doc["temp"] = static_cast<int>(temperature + 0.5f);
    doc["hum"] = static_cast<int>(humidity*100 + 0.5f);
    String payload;
    serializeJson(doc, payload);
    client.publish("esp32/aht20", payload.c_str());
#ifdef oled_dsp 
    sprintf(st, "temp:%3.3d hum:%3.3d", static_cast<int>(temperature), static_cast<int>(humidity*100) );
    u8x8.drawString(0,0,st);
#endif    
    Serial.println(payload);
  }
}
