# ASTRA-IMMUNE

### Autonomous Telemetry Fault Detection, Isolation & Recovery Testbed

ASTRA-IMMUNE is a cyber-physical spacecraft/rover telemetry testbed inspired by an **autonomous immune system**.

The system monitors telemetry from multiple sources, detects suspicious or faulty sensor data, assigns a trust score, isolates unreliable readings, and provides the mission-control dashboard with a safer fallback value.

Built for **Stardance — Hack Club**.

---

## 🚀 What is ASTRA-IMMUNE?

Spacecraft and autonomous vehicles depend on sensor telemetry to make decisions. A faulty, corrupted, missing, or spoofed sensor reading can cause incorrect decisions.

ASTRA-IMMUNE demonstrates a simplified **Fault Detection, Isolation and Recovery (FDIR)** pipeline:

```text
             ┌─────────────────────┐
             │     Mega 2560       │
             │ Real Sensor Data    │
             │ Ultrasonic + IR     │
             └──────────┬──────────┘
                        │
                        │ Telemetry
                        ▼
             ┌─────────────────────┐
             │       ESP32         │
             │ Flight Computer     │
             │                     │
             │ • Validation        │
             │ • Trust Scoring     │
             │ • Fault Isolation   │
             │ • Recovery          │
             └───────┬─────┬───────┘
                     │     │
          ┌──────────┘     └──────────┐
          │                           │
          ▼                           ▼
┌───────────────────┐       ┌───────────────────┐
│   Uno Spoof       │       │ Mission Control   │
│ Fake Telemetry    │       │ Web Dashboard     │
└───────────────────┘       └───────────────────┘
```

The important idea is that the system does not simply display sensor data. It attempts to determine **how much the system should trust that data**.

---

# ✨ Features

- Real ultrasonic sensor telemetry
- Real IR sensor telemetry
- Simulated sensor dropout
- Simulated corrupted sensor values
- Separate telemetry source for spoofing experiments
- Sensor trust scoring
- Fault isolation
- Last-known-good-value fallback
- Autonomous recovery after valid data returns
- ESP32 WebSocket telemetry server
- Browser-based mission-control dashboard
- Remote fault injection from the dashboard
- Event logging
- Hardware status LEDs

---

# 🧠 Fault Detection

ASTRA-IMMUNE currently demonstrates three main fault scenarios.

### 1. Sensor Dropout

The ultrasonic sensor can intentionally be forced into a missing-data state.

The Mega reports:

```text
value_cm = -1
```

The ESP32 treats this as an invalid reading and reduces its trust score.

---

### 2. Sensor Corruption

The ultrasonic reading can be intentionally changed to an unrealistic value.

During corruption simulation, the Mega generates values approximately in:

```text
500–900 cm
```

The ESP32 checks the reading against the configured ultrasonic operating range.

---

### 3. Telemetry Spoofing

The Arduino Uno independently generates fake ultrasonic telemetry.

It produces:

- Plausible-looking values
- Obvious outliers
- Irregular transmission intervals

This provides a separate source of telemetry that can be used to test whether the telemetry validation system can distinguish trustworthy data from suspicious data.

> **Current limitation:** ASTRA-IMMUNE's spoofing protection is a prototype. A plausible spoofed value can currently pass the value-based checks because the system does not yet cryptographically authenticate the sensor source.

---

# 📊 Trust Scoring

The ESP32 currently evaluates ultrasonic readings using three basic checks.

| Condition | Effect |
|---|---:|
| Valid reading | Base trust = `1.0` |
| Invalid/negative value | Trust → `0.0` |
| Outside `2–400 cm` | `-0.7` |
| Sudden jump > `150 cm` | `-0.5` |
| Minimum trust threshold | `0.5` |

The final score is clamped between:

```text
0.0 → 1.0
```

A reading with:

```text
trust >= 0.5
```

is considered trusted.

A reading below that threshold is isolated.

---

# 🔄 Recovery

When an invalid reading is detected, the ESP32 can continue reporting the most recent trusted ultrasonic value instead of immediately passing the bad value to the dashboard.

Conceptually:

```text
Sensor Reading
      │
      ▼
  Validation
      │
 ┌────┴────┐
 │         │
Valid    Invalid
 │         │
 ▼         ▼
Trust    Isolate
 │         │
 ▼         ▼
Current   Last Known
Value     Good Value
 │         │
 └────┬────┘
      ▼
Mission Control
```

When valid telemetry returns, the sensor can recover from isolation automatically.

---

# 🖥️ Mission Control Dashboard

The dashboard provides:

- ESP32 connection control
- Ultrasonic telemetry
- IR telemetry
- Trust score visualization
- Isolation status
- Event log
- Fault injection controls

The dashboard communicates with the ESP32 through a WebSocket connection.

```text
Browser
   │
   │ WebSocket
   ▼
ESP32 :80
   │
   ├── Telemetry
   ├── Events
   └── Fault Commands
```

---

# 🔌 Hardware

## Main Components

- Arduino Mega 2560
- ESP32
- Arduino Uno
- HC-SR04 ultrasonic sensor
- IR sensor
- Push buttons / switches
- Green LED
- Red LED
- Blue LED
- Resistors
- USB power connections
- Jumper wires

---

# 📁 Repository Structure

The repository currently uses the following structure:

```text
ASTRA-IMMUNE/
│
├── README.md
├── app.js
├── index.html
├── style.css
│
├── esp32_flight_computer.ino
├── mega_sensor_hub.ino
├── uno_attacker.ino
│
├── fault_types.md
├── wiring.md
│
├── hardware/
├── software/
├── docs/
│
└── download
```

### Core firmware

| File | Purpose |
|---|---|
| `mega_sensor_hub.ino` | Reads real sensors and generates fault conditions |
| `esp32_flight_computer.ino` | Performs telemetry processing, trust scoring, isolation and dashboard communication |
| `uno_attacker.ino` | Generates simulated spoofed telemetry |

### Dashboard

| File | Purpose |
|---|---|
| `index.html` | Mission-control interface |
| `style.css` | Dashboard styling |
| `app.js` | WebSocket communication and dashboard logic |

### Documentation

| File | Purpose |
|---|---|
| `wiring.md` | Hardware wiring reference |
| `fault_types.md` | Fault scenarios and detection logic |

---

# 🔧 Hardware Architecture

## Arduino Mega 2560

The Mega acts as the **real sensor hub**.

It handles:

- HC-SR04 ultrasonic sensor
- IR sensor
- Dropout fault
- Corruption fault
- Status LEDs

Telemetry is sent to the ESP32 through serial communication.

---

## Arduino Uno

The Uno acts as a **telemetry spoofing source**.

It generates packets that imitate the real sensor source.

Example telemetry structure:

```json
{
  "src": "uno_spoof",
  "ts": 12345,
  "ultrasonic": {
    "value_cm": 85.4
  },
  "fault_sim": "spoof"
}
```

---

## ESP32

The ESP32 acts as the system's **flight computer / telemetry security layer**.

It:

1. Receives Mega telemetry.
2. Receives Uno telemetry.
3. Parses telemetry.
4. Calculates trust.
5. Detects suspicious readings.
6. Isolates untrusted data.
7. Maintains a last-known-good value.
8. Sends telemetry to the dashboard.
9. Receives fault-injection commands.

---

# 🔌 Communication

The system uses serial communication between the boards.

```text
Mega 2560
   │
   │ Serial
   ▼
 ESP32
   ▲
   │ Serial
   │
 Arduino Uno
```

The ESP32 then exposes a WebSocket endpoint:

```text
/ws
```

The browser dashboard connects to:

```text
ws://<ESP32-IP>/ws
```

---

# 🧪 Fault Injection

Faults can be triggered directly from the hardware or through the dashboard.

The dashboard supports commands such as:

```text
CMD:CORRUPT_ON
CMD:CORRUPT_OFF
```

The ESP32 forwards the command to the Mega.

This allows the system to demonstrate a complete loop:

```text
Mission Control
       │
       ▼
     ESP32
       │
       ▼
      Mega
       │
       ▼
  Fault injected
       │
       ▼
 Telemetry changes
       │
       ▼
     ESP32
       │
       ▼
 Fault detected
       │
       ▼
 Dashboard updated
```

---

# 🛠️ Setup

## 1. Install Arduino Libraries

Install the required libraries in the Arduino IDE.

### ESP32

```text
ArduinoJson
AsyncTCP
ESPAsyncWebServer
```

### Mega / Uno

```text
ArduinoJson
```

---

## 2. Wire the Hardware

Follow:

```text
wiring.md
```

for the current pin assignments.

Make sure all boards share an appropriate common ground and use suitable power sources.

> Check the voltage compatibility between the Arduino Mega's serial signals and the ESP32 before connecting them directly. The Mega uses 5 V logic while ESP32 GPIO is 3.3 V logic.

---

## 3. Flash the Mega

Upload:

```text
mega_sensor_hub.ino
```

to the Arduino Mega 2560.

---

## 4. Flash the Uno

Upload:

```text
uno_attacker.ino
```

to the Arduino Uno.

The Uno's serial connections may need to be disconnected during uploading because pins 0 and 1 are used for serial communication.

---

## 5. Configure the ESP32

Open:

```text
esp32_flight_computer.ino
```

Enter your Wi-Fi credentials in the configuration section.

Then upload the firmware to the ESP32.

After connecting to Wi-Fi, the ESP32 prints its IP address.

---

# 🌐 Opening Mission Control

Open:

```text
index.html
```

in a browser.

Enter the IP address displayed by the ESP32.

Then click:

```text
Connect
```

If the connection succeeds, the dashboard should begin receiving telemetry.

---

# 🎮 Testing

### Test 1 — Normal Telemetry

Start the system with no fault enabled.

Expected:

```text
Ultrasonic → Trusted
IR         → Normal
Isolation  → No
```

---

### Test 2 — Dropout

Activate the dropout control.

The Mega should report an invalid ultrasonic value.

The ESP32 should detect the fault and isolate the reading.

---

### Test 3 — Corruption

Enable ultrasonic corruption.

The Mega generates an unrealistic distance.

The ESP32 checks the value against its configured limits and jump detection.

---

### Test 4 — Spoofing

Run the Uno alongside the Mega.

The Uno sends telemetry marked:

```text
src = uno_spoof
```

This allows the telemetry validation system to be tested against a second, potentially untrusted source.

---

# ⚠️ Current Limitations

ASTRA-IMMUNE is a **prototype/testbed**, not a flight-certified spacecraft safety system.

Current limitations include:

- No cryptographic telemetry authentication
- No replay protection
- No message sequence numbers
- Spoof detection primarily relies on telemetry plausibility
- IR telemetry currently has a fixed trust value
- No cross-sensor consistency model
- Packet timing anomalies are not yet fully scored
- Trust decisions are currently based largely on individual readings
- Dashboard communication uses WebSocket over the local network
- No authentication is implemented for dashboard commands

These limitations are intentional areas for future development.

---

# 🚀 Future Development

## Phase 1 — Better Telemetry Validation

- Packet sequence numbers
- Packet timing analysis
- Source identity validation
- Schema validation
- Better anomaly detection

## Phase 2 — Sensor Fusion

Cross-check multiple sensors instead of evaluating them independently.

For example:

```text
Ultrasonic ──┐
             ├──► Sensor Fusion ──► Trust
IR ──────────┘
```

## Phase 3 — Advanced Trust Model

Move from a single-reading decision to a rolling trust model.

```text
Reading History
      │
      ▼
 ┌───────────────┐
 │ Time analysis │
 │ Value change  │
 │ Sensor health │
 │ Cross-check   │
 └───────┬───────┘
         ▼
   Trust Score
```

## Phase 4 — Secure Telemetry

Potential future additions:

- Message authentication
- Cryptographic signatures/MACs
- Replay protection
- Secure command authentication
- Tamper-evident event logs

## Phase 5 — Autonomous FDIR

Expand the prototype toward a more complete:

```text
Fault Detection
       ↓
Fault Isolation
       ↓
Fault Recovery
       ↓
System Verification
       ↓
Return to Normal Operation
```

---

# 🛰️ Why ASTRA-IMMUNE?

Traditional telemetry systems generally assume that incoming data is trustworthy.

ASTRA-IMMUNE explores a different approach:

> **Telemetry should earn trust.**

The project combines:

- Embedded systems
- Sensor telemetry
- Cybersecurity
- Fault injection
- Autonomous recovery
- Web dashboards
- Space-system concepts

into a single experimental platform.

---

# 📜 Project Status

**Status:** Active prototype

**Purpose:** Educational / experimental cyber-physical systems research

**Platform:** Arduino Mega + Arduino Uno + ESP32

**Interface:** Web-based Mission Control

**Focus:** Telemetry integrity, fault detection, isolation and recovery

---

# 👨‍💻 Author

**Ashish**

GitHub:

`ashish030810`

Repository:

ASTRA-IMMUNE

---

## ⚠️ Disclaimer

ASTRA-IMMUNE is an experimental prototype designed for education, demonstrations and research.

It is **not intended for deployment on real spacecraft, aircraft, safety-critical systems, or other mission-critical infrastructure without extensive additional engineering, testing, validation and certification.**
