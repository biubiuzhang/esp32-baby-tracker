📅 Day 1 – Initial Setup and First Blink Test (May 19, 2025)

✅ Goals: 
Set up ESP32 development environment
Test board with a simple blink + serial output
Push code to GitHub

🧰 Tools:
ESP32-WROOM-32
PlatformIO in VS Code
Host OS: Linux (based on /dev/ttyUSB0 and udev warning)
Git + GitHub for version control

🪜 Steps:
📦 Installed PlatformIO extension in VS Code
🆕 Created a new PlatformIO project
    Board: DOIT DEVKIT V1 variant
    Framework: arduino
    Project name: baby_tracker

✅ Verified project structure created by PlatformIO:
bash
Copy
Edit
baby_tracker/
├── src/main.cpp
├── platformio.ini
├── lib/ include/ test/ .pio/ (auto-generated)
✍️ Edited main.cpp with blink + serial print test:

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  Serial.begin(115200);
  Serial.println("ESP32 Blink Test Starting...");
}

void loop() {
  digitalWrite(LED_BUILTIN, HIGH);
  Serial.println("LED ON");
  delay(500);
  digitalWrite(LED_BUILTIN, LOW);
  Serial.println("LED OFF");
  delay(500);
}
⚠️ Faced flash error during first upload:

pgsql
Copy
Edit
A fatal error occurred: Unable to verify flash chip connection (No serial data received.)
🛠️ Resolved it by:

Holding the BOOT button during upload

Adding upload_speed = 115200 in platformio.ini (optional)

Planning to fix udev warning later

🚀 Successfully uploaded firmware again by just click Upload button again:

[SUCCESS] Took 6.58 seconds

🖥️ Opened serial monitor and confirmed output:

ESP32 Blink Test Starting...
LED ON
LED OFF
🔘 Confirmed onboard LED is blinking

🔁 Restarted board using EN button to re-trigger serial output

✅ Results:
ESP32 verified working
Upload, serial output, and onboard LED confirmed
Ready to move to next module (e.g. DHT sensor, buttons)

📌 Notes:
Selected board type esp32doit-devkit-v1 seems correct for ESP-WROOM-32 from Jingdong