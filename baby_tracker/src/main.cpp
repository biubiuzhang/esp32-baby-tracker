#include <Arduino.h>
#include <WiFi.h>
#include <time.h>


const char* ssid = "###"; // Replace with your WiFi SSID
const char* password = "###"; // Replace with your WiFi password

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
  Serial.begin(115200); // Initialize serial communication at 115200 bps
  connectToWiFi(); // Connect to WiFi
  setupTime(); // Setup time using NTP
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
          Serial.printf("📌 %s button pressed at %s\n", bn.name, timeString);
        } else {
          Serial.printf("📌 %s button pressed, but failed to get time\n", bn.name);
        }
      }
    }
  }

  delay(100); // debounce
}
