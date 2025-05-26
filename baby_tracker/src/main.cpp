#include <Arduino.h>
#include <WiFi.h>
#include <time.h>
#include "SPIFFS.h"
#include <ESPAsyncWebServer.h>
#include <PubSubClient.h>
#include <ESPmDNS.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>

// ==== Display setup ====
#define TFT_CS     15
#define TFT_RST    4
#define TFT_DC     2
Adafruit_ST7789 tft = Adafruit_ST7789(TFT_CS, TFT_DC, TFT_RST);

// ==== Network & MQTT ====
AsyncWebServer server(80);
WiFiClient espClient;
PubSubClient mqttClient(espClient);

const char* ssid = "Sun";
const char* password = "peaceandlove";

const char* mqtt_server = "rpi.local";
const int mqtt_port = 1883;
const char* mqtt_topic = "esp32/babytracker/logs";

const char* ntpServer = "pool.ntp.org";
const long gmtOffset_sec = 8 * 3600;
const int daylightOffset_sec = 0;

// ==== Button mapping ====
struct Button {
  const char* name;
  int pin;
  int state;
};

Button buttons[] = {
  {"Blue", 12, LOW},
  {"Red", 19, LOW},
  {"Green", 27, LOW},
  {"Yellow", 21, LOW},
  {"Black", 33, LOW}
};

unsigned long lastMqttReconnect = 0;

void connectToWiFi() {
  WiFi.setHostname("esp32");
  Serial.print("Connecting to WiFi");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }
  Serial.println("\n✅ Connected to WiFi");
  Serial.print("📡 IP Address: ");
  Serial.println(WiFi.localIP());

  if (MDNS.begin("esp32")) {
    Serial.println("🌐 mDNS responder started as esp32.local");
  } else {
    Serial.println("⚠️ Error starting mDNS");
  }
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

void showOnDisplay(const String& text) {
  tft.fillRect(0, 0, 240, 40, ST77XX_BLACK); // Clear top part
  tft.setCursor(0, 0);
  tft.setTextColor(ST77XX_WHITE);
  tft.setTextSize(2);
  tft.println(text);
}

void logEvent(const String& rawTextLog) {
  String filename = getLogFilename();
  File logFile = SPIFFS.open(filename, FILE_APPEND);
  if (logFile) {
    logFile.print(rawTextLog);
    logFile.close();
  } else {
    Serial.println("Failed to open log file for writing");
  }

  int firstSpace = rawTextLog.indexOf(' ');
  int secondSpace = rawTextLog.indexOf(' ', firstSpace + 1);
  int newlinePos = rawTextLog.indexOf('\n');

  if (firstSpace > 0 && secondSpace > firstSpace && newlinePos > secondSpace) {
    String date = rawTextLog.substring(0, firstSpace);
    String time = rawTextLog.substring(firstSpace + 1, secondSpace);
    String color = rawTextLog.substring(secondSpace + 1, newlinePos);
    String timestamp = date + " " + time;

    String jsonPayload = "{\"timestamp\":\"" + timestamp + "\",\"color\":\"" + color + "\"}";

    if (mqttClient.connected()) {
      mqttClient.publish(mqtt_topic, jsonPayload.c_str());
    }

  } else {
    Serial.println("Failed to parse log entry for JSON MQTT");
  }
}

void setup() {
  Serial.begin(115200);

  // === TFT display welcome ===
  tft.init(240, 240);
  tft.setRotation(1);
  tft.fillScreen(ST77XX_BLACK);
  tft.setCursor(10, 10);
  tft.setTextSize(2);
  tft.setTextColor(ST77XX_WHITE);
  tft.println("Welcome!");

  // Optional: wait briefly to stabilize pins before reading
  delay(500);  // allow GPIOs to settle

  connectToWiFi();
  setupTime();
  setupMQTT();

  if (!SPIFFS.begin(true)) {
    Serial.println("Failed to mount file system");
    return;
  }

  // === Web server setup ===
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

  // === Boot reason log ===
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

  // === Pin configuration ===
  for (auto &bn : buttons) {
    pinMode(bn.pin, INPUT_PULLDOWN);  // ✅ Prevent floating HIGH at boot
  }

  Serial.println("Setup complete. Waiting for button presses...");
}

// === State tracking ===
static bool preStates[5] = {false};
static unsigned long lastBluePress = ULONG_MAX;
static unsigned long lastBlackPress = ULONG_MAX;
static bool logsCleared = false;

static bool pendingRed = false;
static bool pendingYellow = false;
static unsigned long redPressTime = 0;
static unsigned long yellowPressTime = 0;
static bool comboFired = false;

void logAndDisplay(const char* label) {
  struct tm timeinfo;
  if (getLocalTime(&timeinfo, 1000)) {
    char timeStr[64];
    strftime(timeStr, sizeof(timeStr), "%Y-%m-%d %H:%M:%S", &timeinfo);
    String logEntry = String(timeStr) + " " + String(label) + "\n";
    Serial.print(logEntry);
    logEvent(logEntry);
    showOnDisplay(label);
  }
}

void handleBlueBlackCombo(bool blue, bool black, unsigned long now) {
  if (blue && !preStates[0]) lastBluePress = now;
  if (black && !preStates[4]) lastBlackPress = now;

  if (blue && black &&
      abs((long)(lastBluePress - lastBlackPress)) <= 500 &&
      !logsCleared) {
    Serial.println("🧹 Combo triggered: clearing logs...");
    showOnDisplay("Logs cleared");

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

    logsCleared = true;
  }

  if (!blue && !black) logsCleared = false;

  if (blue && !black && !logsCleared && !preStates[0]) {
    logAndDisplay("Blue");
  }

  if (black && !blue && !logsCleared && !preStates[4]) {
    logAndDisplay("Black");
  }
}

void handleRedYellowCombo(bool red, bool yellow, unsigned long now) {
  if (red && !preStates[1]) {
    redPressTime = now;
    pendingRed = true;
    comboFired = false;
  }

  if (yellow && !preStates[3]) {
    yellowPressTime = now;
    pendingYellow = true;
    comboFired = false;
  }

  if (pendingRed && pendingYellow &&
      abs((long)(redPressTime - yellowPressTime)) <= 200 &&
      !comboFired) {
    logAndDisplay("Pee-Poo");
    pendingRed = pendingYellow = false;
    comboFired = true;
  }

  if (pendingRed && now - redPressTime > 200) {
    logAndDisplay("Red");
    pendingRed = false;
  }

  if (pendingYellow && now - yellowPressTime > 200) {
    logAndDisplay("Yellow");
    pendingYellow = false;
  }
}

void handleSingleButtonPresses(bool states[]) {
  for (int i = 0; i < 5; ++i) {
    if (i == 0 || i == 4) continue; // skip blue/black here (handled by combo)
    if (i == 1 || i == 3) continue; // skip red/yellow (handled with delay)

    if (states[i] != preStates[i]) {
      if (states[i]) {
        logAndDisplay(buttons[i].name);
      }
    }
  }
}

void loop() {
  mqttClient.loop();
  if (!mqttClient.connected() && millis() - lastMqttReconnect > 5000) {
    reconnectMQTT();
    lastMqttReconnect = millis();
  }

  unsigned long now = millis();

  // Read button states
  bool states[5];
  for (int i = 0; i < 5; ++i) {
    states[i] = digitalRead(buttons[i].pin) == HIGH;
  }

  // Handle all button behavior
  handleBlueBlackCombo(states[0], states[4], now);
  handleRedYellowCombo(states[1], states[3], now);
  handleSingleButtonPresses(states);

  // Update all preStates at the end
  for (int i = 0; i < 5; ++i) {
    preStates[i] = states[i];
  }

  delay(100);
}
