#include <Arduino.h>
#include <WiFi.h>
#include <time.h>
#include "SPIFFS.h"
#include <ESPAsyncWebServer.h>
#include "esp_system.h"

AsyncWebServer server(80);

const char* ssid = "Sun"; // Replace with your WiFi SSID
const char* password = "peaceandlove"; // Replace with your WiFi password

const char* ntpServer = "pool.ntp.org"; // NTP server
const long gmtOffset_sec = 8 * 3600; // GMT offset in seconds, GMT+8 for China
const int daylightOffset_sec = 0; // No daylight saving time


void connectToWiFi() {
  Serial.print("Connecting to WiFi");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }
  Serial.println("Connected to WiFi");
  Serial.println(WiFi.localIP());
}

void setupTime() {
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  Serial.println("Waiting for NTP time sync...");
  time_t now = time(nullptr);
  while (now < 8 * 3600 * 2) { // Wait for time to be set
    delay(1000);
    Serial.print(".");
    now = time(nullptr);
  }
  Serial.println("NTP time sync complete");
}
struct Button {
  const char* name; // Name of the button
  int pin; // Pin number for the button
  int state; // Current state of the button
};

Button buttons[] = {
  {"Blue", 4, LOW},
  {"Red", 19, LOW},
  {"Green", 5, LOW},
  {"Yellow", 21, LOW},
  {"Black", 18, LOW}
};

void setup() {
  // Initialize serial communication at 115200 bps
  Serial.begin(115200);

  // Setup WiFi and NTP
  connectToWiFi(); // Connect to WiFi
  setupTime(); // Setup time using NTP

  // Initialize SPIFFS
  if (!SPIFFS.begin(true)) {
    Serial.println("Failed to mount file system");
    return;
  }
  Serial.println("SPIFFS mounted successfully");

  // Initialize web server
  server.on("/log.txt", HTTP_GET, [](AsyncWebServerRequest *request) {
    if (!SPIFFS.exists("/log.txt")) {
      request->send(404, "text/plain", "Log File Not Found");
    } else {
      request->send(SPIFFS, "/log.txt", "text/plain");
    }
  });

  server.begin(); // Start the server
  Serial.println("Web server started: http://" + WiFi.localIP().toString() + "/log.txt");

  // Record each boot
  esp_reset_reason_t resetReason = esp_reset_reason();
  String resetReasonString;

  switch (resetReason) {
    case ESP_RST_POWERON:
      resetReasonString = "Power on";
      break;
    case ESP_RST_EXT:
      resetReasonString = "External reset";
      break;
    case ESP_RST_SW:
      resetReasonString = "Software reset";
      break;
    case ESP_RST_DEEPSLEEP:
      resetReasonString = "Deep sleep wakeup";
      break;
    case ESP_RST_BROWNOUT:
      resetReasonString = "Brownout reset";
      break;
    case ESP_RST_SDIO:
      resetReasonString = "SDIO reset";
      break;
    default:
      resetReasonString = "Unknown reason";
  }

  struct tm timeinfo;
  if (getLocalTime(&timeinfo, 1000)) {
    char timeString[64];
    strftime(timeString, sizeof(timeString), "%Y-%m-%d %H:%M:%S", &timeinfo);
    String logEntry = String(timeString) + " [Boot] " + resetReasonString + "\n";
    Serial.print(logEntry);
    File logFile = SPIFFS.open("/log.txt", "a");
    if (!logFile) {
      Serial.println("Failed to open log file for writing");
    } else {
      logFile.print(logEntry);
      logFile.flush();
      logFile.close();
    }
  } else {
    Serial.println("Failed to get time");
  }

  // Initialize button pins
  for (auto &bn : buttons) {
    pinMode(bn.pin, INPUT);
  }
  Serial.println("Setup complete. Waiting for button presses...");
}

void loop() {
  for (auto &bn : buttons) {
    int currentState = digitalRead(bn.pin); // Read the current state of the button
    if (currentState != bn.state) { // Check if the state has changed
      bn.state = currentState; // Update the state
      if (currentState == HIGH) { // Button pressed
        struct tm timeinfo;
        if (getLocalTime(&timeinfo, 1000)) {
          char timeString[64];
          strftime(timeString, sizeof(timeString), "%Y-%m-%d %H:%M:%S", &timeinfo);
          String logEntry = String(timeString) + " " + String(bn.name) + "\n";
          Serial.print(logEntry);
          File logFile = SPIFFS.open("/log.txt", "a");
          if (!logFile) {
            Serial.println("Failed to open log file for writing");
          } else {
            logFile.print(logEntry);
            logFile.flush();
            logFile.close();
          }
        } else {
          Serial.printf("📌 %s button pressed, but failed to get time\n", bn.name);
        }
      }
    }
  }
  delay(100); // debounce
}
