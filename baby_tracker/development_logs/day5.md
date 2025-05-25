# Day 5 – Button Combo Logic, Display, and MQTT Integration

## Summary

Significant improvements were made to the ESP32 firmware, enabling **reliable button detection**, **combo handling**, **OLED display messages**, and **real-time MQTT updates**. The logic now supports **meaningful activity tracking** for feeding, sleep, pee, poo, stop, and combo events like "pee+poo" or log clearing. The firmware now forms a robust, production-ready core for the baby tracker system.

## Core Features Implemented

### Button Mapping & Behavior

* Implemented **edge detection** for all button presses using `prevState` comparison.
* Added **combo handling** with time-based detection: two buttons must be pressed within **500ms** of each other to trigger combo.
* Used `INPUT_PULLDOWN` to prevent floating `HIGH` states during boot.

| Function             | Description               | ESP32 GPIO | Notes                           |
|---------------------|---------------------------|------------|---------------------------------|
| Feeding             | Green button               | GPIO27     | Start feeding session           |
| Sleeping            | Blue button                | GPIO12     | Start sleep session             |
| Pee                 | Yellow button              | GPIO21     | +1 diaper                       |
| Poo                 | Red button                 | GPIO19     | +1 diaper                       |
| Stop                | Black button               | GPIO33     | Ends feeding/sleep sessions     |
| Pee + Poo Combo     | Red + Yellow               | –          | Triggered within 500ms          |
| Clear Logs Combo    | Blue + Black               | –          | Triggered within 500ms          |

### TFT Display (ST7789) Wiring

| Signal         | Description             | ESP32 GPIO | Notes              |
|----------------|-------------------------|------------|--------------------|
| TFT_CS         | Chip Select             | GPIO15     | `#define TFT_CS 15`|
| TFT_DC         | Data/Command            | GPIO2      | `#define TFT_DC 2` |
| TFT_RST        | Reset                   | GPIO4      | `#define TFT_RST 4`|
| TFT_SCL        | SPI Clock (SCK)         | GPIO18     | Hardware SPI       |
| TFT_SDA        | SPI Data (MOSI)         | GPIO23     | Hardware SPI       |
| TFT_BL         | Backlight               | 3.3V       | Or controlled via GPIO if needed |

### Display Output

* Added `showOnDisplay(text)` function to display last activity on ST7789.

### MQTT Publishing

* All events (including combos) are published to topic `esp32/babytracker/logs` in this format:

  ```json
  {
    "timestamp": "2025-05-26 00:15:52",
    "color": "Blue"
  }
  ```
* `logEvent()` function handles:

  * Saving to SPIFFS daily file (`/log-YYYY-MM-DD.txt`)
  * Publishing the MQTT payload if connected

### Stability Fixes

* 🧹 **Log-clearing logic** was previously unstable (triggered without pressing).

  * Fixed by ensuring:

    * Two specific buttons must be pressed within 500ms
    * `logsCleared` flag resets only after release
* 🐛 Prevented button bouncing by adding `delay(100)` debounce and `INPUT_PULLDOWN` resistor config.

## Status

Your ESP32 firmware is now:

* 🧠 Smart enough to handle combinations and session logic
* 💾 Persisting data reliably via SPIFFS
* 📢 Syncing events in real-time over MQTT
* 📺 Displaying useful info on a crisp TFT screen
