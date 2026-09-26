// Mega side - reads the ultrasonic + IR, handles the fault switches, drives the LEDs
// sends everything to the ESP32 over Serial1 as JSON

#include <ArduinoJson.h>

const int TRIG_PIN = 9;
const int ECHO_PIN = 8;
const int IR_PIN = 7;

const int DROPOUT_SWITCH_PIN = 2;   // LOW when pressed (pullup)
const int CORRUPT_BUTTON_PIN = 3;

const int LED_GREEN = 5;
const int LED_RED = 6;
const int LED_BLUE = 4;

bool corruptModeUltrasonic = false;
bool lastCorruptButtonState = HIGH;
unsigned long lastSendTime = 0;
const unsigned long SEND_INTERVAL_MS = 200; // 5 Hz telemetry

void setup() {
  Serial.begin(115200);   // USB debug
  Serial1.begin(115200);  // link to ESP32

  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  pinMode(IR_PIN, INPUT);
  pinMode(DROPOUT_SWITCH_PIN, INPUT_PULLUP);
  pinMode(CORRUPT_BUTTON_PIN, INPUT_PULLUP);
  pinMode(LED_GREEN, OUTPUT);
  pinMode(LED_RED, OUTPUT);
  pinMode(LED_BLUE, OUTPUT);

  digitalWrite(LED_GREEN, HIGH);
}

long readUltrasonicCm() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  long duration = pulseIn(ECHO_PIN, HIGH, 30000);
  if (duration == 0) return -1; // timed out, no echo
  return duration / 58;
}

void checkCorruptButton() {
  bool state = digitalRead(CORRUPT_BUTTON_PIN);
  if (state == LOW && lastCorruptButtonState == HIGH) {
    corruptModeUltrasonic = !corruptModeUltrasonic; // toggle on press
  }
  lastCorruptButtonState = state;
}

void checkIncomingCommands() {
  while (Serial1.available()) {
    String line = Serial1.readStringUntil('\n');
    line.trim();
    if (line == "CORRUPT:ON") corruptModeUltrasonic = true;
    else if (line == "CORRUPT:OFF") corruptModeUltrasonic = false;
  }
}

void sendTelemetry() {
  bool dropoutActive = (digitalRead(DROPOUT_SWITCH_PIN) == LOW);

  long ultrasonicCm = dropoutActive ? -1 : readUltrasonicCm();
  if (corruptModeUltrasonic && !dropoutActive) {
    ultrasonicCm = random(500, 900); // out of the sensor's real range on purpose
  }

  int irState = digitalRead(IR_PIN);

  StaticJsonDocument<256> doc;
  doc["src"] = "mega";
  doc["ts"] = millis();

  JsonObject u = doc.createNestedObject("ultrasonic");
  u["value_cm"] = ultrasonicCm;
  u["fault_sim"] = dropoutActive ? "dropout" : (corruptModeUltrasonic ? "corrupt" : "none");

  JsonObject ir = doc.createNestedObject("ir");
  ir["value"] = irState;
  ir["fault_sim"] = "none";

  serializeJson(doc, Serial1);
  Serial1.println();

  digitalWrite(LED_RED, (dropoutActive || corruptModeUltrasonic) ? HIGH : LOW);
  digitalWrite(LED_GREEN, (dropoutActive || corruptModeUltrasonic) ? LOW : HIGH);
}

void loop() {
  checkCorruptButton();
  checkIncomingCommands();

  if (millis() - lastSendTime >= SEND_INTERVAL_MS) {
    lastSendTime = millis();
    sendTelemetry();
  }
}
