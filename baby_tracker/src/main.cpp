#include <Arduino.h>

struct Button {
  const char* name; // Name of the button
  int pin; // Pin number for the button
  int state; // Current state of the button
};

Button buttons[] = {
  {"Blue Button", 4, LOW},
  {"Red Button", 19, LOW},
  {"Green Button", 5, LOW},
  {"Yellow Button", 21, LOW},
  {"Black Button", 18, LOW}
};

void setup() {
  Serial.begin(115200); // Initialize serial communication at 115200 bps
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
      if (currentState == HIGH) { // If the button is pressed
        unsigned long now = millis();
        Serial.printf("%s button pressed at %lus\n", bn.name, now / 1000); // Print the button name and time
      }
    }
  }
  delay(100); // Wait for a short period to avoid bouncing
}
