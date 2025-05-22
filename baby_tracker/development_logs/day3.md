# Day 3: Per-Day Persistent Logging, Web Access, and Remote Syncing

## Goals:
* Persistently save button events with timestamps on the ESP32
* Serve all daily logs over WiFi for remote access
* Allow a Raspberry Pi to automatically sync and manage logs
* Enable physical button combination to delete logs locally
## Hardware:
* ESP32-WROOM-32 (DOIT DevKit V1)
* 5 push-button modules (active-high)
* Raspberry Pi 4 (always on, LAN-connected)
## Features Implemented:
### 1. Per-Day Persistent Logging to SPIFFS
* Logs are saved into **per-day files**:
  `/log-YYYY-MM-DD.txt`
* On each button press, ESP32 logs:
  ```
  2025-05-20 15:54:34 Red
  ```
* On each reboot, ESP32 logs boot cause via `esp_reset_reason()`:
  ```
  2025-05-20 16:12:00 [BOOT] Power-on reset
  ```
* Uses SPIFFS (`SPIFFS.open(...)`) and survives reboots
### 2. Web Server for Log Access
* Web interface built using `ESPAsyncWebServer`
* Features:
  * List all log files on the root page (`/`)
  * View individual logs:
    `http://<ESP32-IP>/log?file=log-2025-05-20.txt`
  * Clean UI: links generated dynamically from stored files
  * Proper UTF-8 support via HTML headers
### 3. Remote Sync Service on Raspberry Pi
* Custom script: `/usr/local/bin/sync-esp32-log.sh`
  * Pulls yesterday’s log using `curl`
  * Saves to `/home/pi/baby-logs/log-YYYY-MM-DD.txt`
  * Optionally clears log via:
    `http://<ESP32-IP>/clear-log?file=...`
* Managed via `systemd`:
  * Runs every 10 minutes
  * Ensures no duplication or log loss
### 4. Safe Local Log Wipe via Dual-Button Combo
* Holding **Blue + Red** buttons together triggers log wipe:
  * Detects simultaneous GPIO 4 and 19 HIGH
  * Deletes all `/log-*.txt` files on SPIFFS
  * Includes cooldown timer to avoid repeated wipes
### 5. Log File Listing on Web Root
* Visiting `http://<ESP32-IP>/` lists all logs:
  ```html
  📝 Available log files:
  - log-2025-05-20.txt
  - log-2025-05-21.txt
  ```
* Clickable links open individual logs via `/log?file=...`
## Example Log Entries
```
2025-05-20 15:54:34 Red
2025-05-20 15:54:35 Black
2025-05-20 16:12:00 [BOOT] Power-on reset
```
## Challenges & Fixes
| Issue                          | Resolution                                                           |
| ------------------------------ | -------------------------------------------------------------------- |
| 🧩 `AsyncTCP_RP2040W` error    | Switched to ESP32-compatible `ESPAsyncWebServer` via GitHub          |
| ❌ `log.txt` hardcoded          | Moved to per-day log files using `getLocalTime()`                    |
| 🛑 No file list at root page   | Added dynamic SPIFFS directory scanning and fixed filename filtering |
| 🤔 Long-press not reliable     | Replaced with dual-button short press (Blue + Red)                   |
| 🌀 Unicode render glitch (ðŸ…) | Fixed with proper UTF-8 headers and meta tag in HTML                 |
## Next Steps (Planned for Day 4)
* Improve UI: add file sizes, delete buttons next to logs
* Implement debounce logic for noisy button inputs
* Enable cloud push from Raspberry Pi (e.g., upload to Google Drive)
