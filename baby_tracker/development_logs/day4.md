# Day 4 – Local MQTT Integration

## **What Was Achieved**

### 1. **Local MQTT Broker Setup on Raspberry Pi**

* Installed Mosquitto broker on Raspberry Pi using `apt`.
* Created config file `/etc/mosquitto/conf.d/default.conf` to allow external connections:

  ```ini
  listener 1883
  allow_anonymous true
  ```
* Verified broker runs as a systemd service.
* Enabled `avahi-daemon` to allow `.local` hostname resolution (`rpi.local`).

### 2. **ESP32 MQTT Integration Updated**

* Switched MQTT server from public (`broker.hivemq.com`) to local (`rpi.local`).
* Replaced static `clientId` with a unique one based on MAC address to avoid disconnects.
* Modified `mqttClient.connect(...)` logic to auto-reconnect.
* Confirmed messages are published to topic:
  `esp32/babytracker/logs`

### 3. **Tested Real-Time Messaging**

* Subscribed to the topic on Raspberry Pi:

  ```bash
  mosquitto_sub -h rpi.local -t "esp32/babytracker/logs" -v
  ```
* Successfully received multiple button press events from ESP32:

  ```
  esp32/babytracker/logs 2025-05-22 19:15:00 Yellow
  esp32/babytracker/logs 2025-05-22 19:15:04 Red
  ```

### 4. **Refined ESP32 Codebase**

* Reorganized MQTT logic for clarity.
* Improved logging of connection state and failure reason.
* Ensured the system remains modular, stable, and testable.

## **Reflection**

Today's progress marks the beginning of a **scalable, offline-capable, and secure local IoT communication stack** — a major step toward production-ready architecture for the baby activity tracker.

## **To-Do for Tomorrow (Day 5)**

### ESP32 / Backend Enhancements:

* [ ] **Add MQTT “last will” message** to notify if device disconnects unexpectedly.
* [ ] **Send MQTT message when logs are cleared** via button combo.
* [ ] **Update logEvent() to also include button GPIO pin or unique ID (optional)**.

### Raspberry Pi / Flask Dashboard:

* [ ] Add a **Flask MQTT subscriber** to receive button events and:

  * Save them to PostgreSQL.
  * Display them on the web frontend in real time.
* [ ] Log MQTT errors or disconnects to `/var/log/mosquitto/mosquitto.log` and monitor them.

### UI Enhancements:

* [ ] Add MQTT status indicator on the frontend (e.g., green dot = connected).

### Testing / Maintenance:

* [ ] Stress test MQTT with rapid presses (hold buttons) — ensure no message loss.
* [ ] Test ESP32 reconnect behavior if Raspberry Pi is temporarily offline.
* [ ] Backup `/etc/mosquitto/` and Avahi config files.
