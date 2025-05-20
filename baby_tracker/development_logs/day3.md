# Day 3: Persistent Logging and Remote Syncing

## Goals:
* Persistently save button events with timestamps on the ESP32
* Serve the log over WiFi for remote access
* Automate syncing to a Raspberry Pi for archival and further analysis
## Hardware:
* ESP32-WROOM-32 (DOIT DevKit V1)
* 5 push-button modules (active-high)
* Raspberry Pi 4 (always on, LAN-connected)
## Features Implemented:
### 1. Persistent Logging to SPIFFS
* On each button press, ESP32 logs a line like:
  ```
  2025-05-20 15:54:34 Red
  ```
* Log is written to `/log.txt` in **SPIFFS**, so it survives reboots
* Each reboot is also logged with a timestamp and reset reason using `esp_reset_reason()`:
  ```
  2025-05-20 16:12:00 [BOOT] Power-on reset
  ```
### 2. HTTP File Server on ESP32
* Added `ESPAsyncWebServer` + `AsyncTCP` via GitHub in PlatformIO
* ESP32 now serves its log at:
  ```
  http://<ESP32-IP>/log.txt
  ```
* Allows access from any browser, device, or command-line tool like `curl`
### 3. Raspberry Pi Auto-Sync Service
* Created `/usr/local/bin/sync-esp32-log.sh` to pull daily logs using `curl`
* Stores log files in:
  ```
  /home/pi/baby-logs/log-YYYY-MM-DD.txt
  ```
* Created a **systemd service** to run the script as a daemon, retrying every 10 minutes
* Logs are accessible and organized daily on the Pi
### Example Log Entries from `/log.txt`
```
2025-05-20 15:54:34 Red
2025-05-20 15:54:35 Black
2025-05-20 15:54:37 Green
2025-05-20 16:12:00 [BOOT] Power-on reset
```
## Challenges & Fixes
| Issue                           | Resolution                                             |
| ------------------------------- | ------------------------------------------------------ |
| AsyncTCP\_RP2040W wrong library | Replaced with ESP32-compatible GitHub source           |
| Multiple false Green triggers   | Planning to add pull-down resistors and debounce logic |
## Next Steps (Planned for Day 4)
* ⏰ Add log rotation (one file per day or auto-clear after sync)
* 🧪 Implement physical debounce or software debounce per pin
* 📱 Build mobile-friendly web UI for logs (or live dashboard)
* 📤 Optionally push logs to a cloud service from the Pi
