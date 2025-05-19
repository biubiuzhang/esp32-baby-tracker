#include <Arduino.h>

// put function declarations here:
// int myFunction(int, int);

void setup() {
  // put your setup code here, to run once:
  // int result = myFunction(2, 3);

  pinMode(LED_BUILTIN, OUTPUT); // Initialize the LED_BUILTIN pin as an output
  Serial.begin(115200); // Initialize serial communication at 115200 bps
  Serial.println("Hello, world!"); // Print a message to the serial monitor
}

void loop() {
  // put your main code here, to run repeatedly:
  digitalWrite(LED_BUILTIN, HIGH); // Turn the LED on (HIGH is the voltage level)
  Serial.println("LED is ON"); // Print a message to the serial monitor
  delay(1000); // Wait for a second
  digitalWrite(LED_BUILTIN, LOW); // Turn the LED off by making the voltage LOW
  Serial.println("LED is OFF"); // Print a message to the serial monitor
  delay(1000); // Wait for a second
}

// // put function definitions here:
// int myFunction(int x, int y) {
//   return x + y;
// }