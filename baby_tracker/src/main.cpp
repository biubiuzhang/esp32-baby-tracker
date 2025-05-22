#include <Arduino.h>
#include <WiFi.h>
#include <time.h>
#include "SPIFFS.h"
#include <ESPAsyncWebServer.h>
#include <PubSubClient.h>

#include "esp_system.h"

AsyncWebServer server(80);
WiFiClient espClient;
PubSubClient mqttClient(espClient);

const char* ssid = "Sun";
const char* password = "peaceandlove";

const char* mqtt_server = "broker.hivemq.com";
const int mqtt_port = 1883;
const char* mqtt_topic = "esp32/babytracker/logs";

const char* ntpServer = "pool.ntp.org";
const long gmtOffset_sec = 8 * 3600;
const int daylightOffset_sec = 0;

struct Button {
  const char* name;
  int pin;
  int state;
};

Button buttons[] = {
  {"Blue", 4, LOW},
  {"Red", 19, LOW},
  {"Green", 23, LOW},
  {"Yellow", 21, LOW},
  {"Black", 18, LOW}
};

unsigned long bluePressStart = 0;
unsigned long lastMqttReconnect = 0;

void connectToWiFi() {
  Serial.print("Connecting to WiFi");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }
  Serial.println("\nConnected to WiFi");
  Serial.println(WiFi.localIP());
}

void setupTime() {
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  Serial.println("Waiting for NTP time sync...");
  time_t now = time(nullptr);
  while (now < 8 * 3600 * 2) {
    delay(1000);
    Serial.print(".");
    now = time(nullptr);
  }
  Serial.println("NTP time sync complete");
}

void setupMQTT() {
  mqttClient.setServer(mqtt_server, mqtt_port);
}

void reconnectMQTT() {
  if (!mqttClient.connected()) {
    Serial.print("Connecting to MQTT...");

    String clientId = "ESP32Client-" + String((uint32_t)ESP.getEfuseMac(), HEX);

    if (mqttClient.connect(clientId.c_str())) {
      Serial.println("connected ✅");
      mqttClient.publish(mqtt_topic, "MQTT connected from ESP32");
    } else {
      Serial.print("❌ failed, rc=");
      Serial.print(mqttClient.state());
      Serial.println(" → see: https://pubsubclient.knolleary.net/api#state");
    }
  }
}

String getLogFilename() {
  struct tm timeinfo;
  if (getLocalTime(&timeinfo, 1000)) {
    char date[16];
    strftime(date, sizeof(date), "%Y-%m-%d", &timeinfo);
    return "/log-" + String(date) + ".txt";
  }
  return "/log-unknown.txt";
}

void logEvent(const String& event) {
  String filename = getLogFilename();
  File logFile = SPIFFS.open(filename, FILE_APPEND);
  if (logFile) {
    logFile.print(event);
    logFile.close();
  } else {
    Serial.println("Failed to open log file for writing");
  }

  if (mqttClient.connected()) {
    mqttClient.publish(mqtt_topic, event.c_str());
  }
}

void setup() {
  Serial.begin(115200);
  connectToWiFi();
  setupTime();
  setupMQTT();

  if (!SPIFFS.begin(true)) {
    Serial.println("Failed to mount file system");
    return;
  }
  Serial.println("SPIFFS mounted successfully");

  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    String html = "<!DOCTYPE html><html><head><meta charset=\"UTF-8\"></head><body>";
    html += "<h3>📝 Available log files:</h3><ul>";
  
    File root = SPIFFS.open("/");
    File file = root.openNextFile();
    while (file) {
      String name = String(file.name());
      if (name.startsWith("log-") && name.endsWith(".txt")) {
        html += "<li><a href=\"/log?file=" + name + "\">" + name + "</a></li>";
      }
      file = root.openNextFile();
    }
  
    html += "</ul></body></html>";
    request->send(200, "text/html; charset=utf-8", html);
  });

  server.on("/log", HTTP_GET, [](AsyncWebServerRequest *request) {
    if (!request->hasParam("file")) {
      request->send(400, "text/plain", "Missing file param");
      return;
    }
    String filename = "/" + request->getParam("file")->value();
    if (!SPIFFS.exists(filename)) {
      request->send(404, "text/plain", "File not found");
      return;
    }
    request->send(SPIFFS, filename, "text/plain");
  });

  server.on("/clear-log", HTTP_GET, [](AsyncWebServerRequest *request) {
    if (!request->hasParam("file")) {
      request->send(400, "text/plain", "Missing file param");
      return;
    }
    String filename = "/" + request->getParam("file")->value();
    if (SPIFFS.exists(filename)) {
      SPIFFS.remove(filename);
      request->send(200, "text/plain", "Deleted: " + filename);
    } else {
      request->send(404, "text/plain", "File not found");
    }
  });

  server.begin();
  Serial.println("Web server started: http://" + WiFi.localIP().toString());

  esp_reset_reason_t reason = esp_reset_reason();
  String reasonStr;
  switch (reason) {
    case ESP_RST_POWERON:   reasonStr = "Power-on"; break;
    case ESP_RST_EXT:       reasonStr = "External reset"; break;
    case ESP_RST_SW:        reasonStr = "Software reset"; break;
    case ESP_RST_DEEPSLEEP: reasonStr = "Deep sleep wakeup"; break;
    default:                reasonStr = "Other reset"; break;
  }

  struct tm timeinfo;
  if (getLocalTime(&timeinfo, 1000)) {
    char timeStr[64];
    strftime(timeStr, sizeof(timeStr), "%Y-%m-%d %H:%M:%S", &timeinfo);
    String logEntry = String(timeStr) + " [Boot] " + reasonStr + "\n";
    Serial.print(logEntry);
    logEvent(logEntry);
  }

  for (auto &bn : buttons) {
    pinMode(bn.pin, INPUT);
  }

  Serial.println("Setup complete. Waiting for button presses...");
}

void loop() {
  mqttClient.loop();

  if (!mqttClient.connected() && millis() - lastMqttReconnect > 5000) {
    reconnectMQTT();
    lastMqttReconnect = millis();
  }

  int blue = digitalRead(4);
  int yellow = digitalRead(21);

  static unsigned long lastTrigger = 0;
  if (blue == HIGH && yellow == HIGH && millis() - lastTrigger > 2000) {
    Serial.println("🧹 Combo triggered: clearing logs...");

    File root = SPIFFS.open("/");
    File file = root.openNextFile();
    while (file) {
      String name = file.name();
      if (name.startsWith("log")) {
        SPIFFS.remove("/" + name);
        Serial.println("🧹 Deleted /" + name);
      }
      file = root.openNextFile();
    }

    lastTrigger = millis(); // prevent retriggering
  }

  for (int i = 0; i < sizeof(buttons)/sizeof(Button); ++i) {
    Button &bn = buttons[i];
    int currentState = digitalRead(bn.pin);
    if (currentState != bn.state) {
      bn.state = currentState;
      if (currentState == HIGH) {
        struct tm timeinfo;
        if (getLocalTime(&timeinfo, 1000)) {
          char timeStr[64];
          strftime(timeStr, sizeof(timeStr), "%Y-%m-%d %H:%M:%S", &timeinfo);
          String logEntry = String(timeStr) + " " + bn.name + "\n";
          Serial.print(logEntry);
          logEvent(logEntry);
        } else {
          Serial.printf("📌 %s button pressed, but failed to get time\n", bn.name);
        }
      }
    }
  }

  delay(100); // debounce
}
