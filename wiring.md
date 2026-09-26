# Wiring

## Mega

| Component | Mega pin |
|---|---|
| HC-SR04 Trig | D9 |
| HC-SR04 Echo | D8 |
| HC-SR04 VCC/GND | 5V / GND |
| IR sensor OUT | D7 |
| IR sensor VCC/GND | 5V / GND |
| Dropout switch | D2 (pullup, other leg to GND) |
| Corrupt button | D3 (pullup, other leg to GND) |
| Green LED | D5 (220ohm resistor to GND) |
| Red LED | D6 (220ohm resistor to GND) |
| Blue LED | D4 (220ohm resistor to GND) |
| TX1 to ESP32 RX | pin 18 |
| RX1 from ESP32 TX | pin 19 |
| GND | shared with ESP32 |

## Uno

| Component | Uno pin |
|---|---|
| TX to ESP32 RX2 | pin 1 |
| RX from ESP32 TX2 (optional) | pin 0 |
| GND | shared with ESP32 |

Disconnect the Uno's TX/RX from the ESP32 before uploading new code to it, since pins 0/1 are also used by USB.

## ESP32

| Connection | ESP32 pin |
|---|---|
| RX1 from Mega TX1 | GPIO16 |
| TX1 to Mega RX1 | GPIO17 |
| RX2 from Uno TX | GPIO25 |
| TX2 to Uno RX (optional) | GPIO26 |
| GND | shared with Mega and Uno |

GPIO numbers can vary a bit by ESP32 board. If 16/17/25/26 aren't broken out on yours, pick different free GPIOs and update the `#define`s at the top of `esp32_flight_computer.ino`.

## Power

Power the Mega, Uno, and ESP32 from separate USB ports for the demo. Don't try to run all three off one board's 5V pin.
