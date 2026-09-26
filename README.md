# ASTRA-IMMUNE

A spacecraft "immune system" - a rover/satellite testbed that detects faulty, spoofed, or dangerous telemetry and recovers on its own instead of waiting for a command from Earth.

Made for Stardance (Hack Club). Split into two projects:
- **Hardware** (`/hardware`) - a Mega + ESP32 + Uno testbed that produces real telemetry and real faults
- **Software** (`/software`) - a mission control dashboard with sensor trust scoring, fault isolation, and a fault-injection panel

## How it fits together

The Mega reads a real ultrasonic sensor and IR sensor, handles the fault-trigger switches and 3 status LEDs, and sends JSON telemetry to the ESP32 over serial. The Uno runs independently and sends fake telemetry pretending to be the same sensor, to test whether the system catches it. The ESP32 takes both streams, scores every reading for trust, isolates anything bad, and pushes everything to the dashboard over WebSocket. The dashboard can also send fault commands back down through the ESP32 to the Mega.

## Repo layout

```
astra-immune/
  hardware/
    mega_sensor_hub/mega_sensor_hub.ino
    esp32_flight_computer/esp32_flight_computer.ino
    uno_attacker/uno_attacker.ino
    wiring.md
  software/
    dashboard/
      index.html
      style.css
      app.js
  docs/
    fault_types.md
```

## Setup

1. Install the ArduinoJson library (all 3 boards need it), plus AsyncTCP and ESPAsyncWebServer (ESP32 only). Arduino IDE > Tools > Manage Libraries.
2. Wire everything up following `hardware/wiring.md`.
3. Flash `mega_sensor_hub.ino` to the Mega, `uno_attacker.ino` to the Uno, and `esp32_flight_computer.ino` to the ESP32 (edit the WiFi ssid/password at the top first). Check the serial monitor for the ESP32's IP address after it connects.
4. Open `software/dashboard/index.html` in a browser, type in the ESP32's IP, hit connect.
5. Flip the dropout switch, press the corrupt button, or just watch the log - the Uno is spoofing data in the background the whole time.

## Notes for changes

The trust scoring thresholds live in `esp32_flight_computer.ino` (`ULTRASONIC_MIN_CM`, `ULTRASONIC_MAX_CM`, `MAX_JUMP_CM`). `docs/fault_types.md` explains how each fault works and what to tune if the scoring feels too strict or too loose.
