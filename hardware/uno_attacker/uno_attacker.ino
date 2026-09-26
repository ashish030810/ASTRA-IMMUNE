// Uno pretends to be the ultrasonic sensor and sends fake readings to the ESP32
// disconnect TX/RX from the ESP32 before uploading, pins 0/1 are also used by USB

#include <ArduinoJson.h>

unsigned long nextSendTime = 0;

void setup() {
  Serial.begin(115200); // this IS the link to the ESP32 (pins 0/1)
  randomSeed(analogRead(A0));
}

void loop() {
  if (millis() >= nextSendTime) {
    StaticJsonDocument<256> doc;
    doc["src"] = "uno_spoof";
    doc["ts"] = millis();

    JsonObject u = doc.createNestedObject("ultrasonic");
    int roll = random(0, 10);
    int fakeValue;
    if (roll < 7) {
      fakeValue = random(20, 150); // looks plausible
    } else {
      fakeValue = random(600, 999); // obvious outlier sometimes
    }
    u["value_cm"] = fakeValue;
    u["fault_sim"] = "spoof";

    serializeJson(doc, Serial);
    Serial.println();

    nextSendTime = millis() + random(150, 600); // irregular on purpose
  }
}
