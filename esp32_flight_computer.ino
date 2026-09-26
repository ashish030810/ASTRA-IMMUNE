// ESP32 side - gets data from Mega + Uno over serial, runs trust scoring,
// pushes everything to the dashboard over websocket

#include <WiFi.h>
#include <ArduinoJson.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>

const char* WIFI_SSID = "YOUR_WIFI_SSID";
const char* WIFI_PASSWORD = "YOUR_WIFI_PASSWORD";

#define MEGA_RX_PIN 16
#define MEGA_TX_PIN 17
#define UNO_RX_PIN  25
#define UNO_TX_PIN  26

HardwareSerial MegaSerial(1);
HardwareSerial UnoSerial(2);

AsyncWebServer server(80);
AsyncWebSocket ws("/ws");

struct SensorHistory {
  float lastGoodValue = -1;
  unsigned long lastGoodTs = 0;
  unsigned long lastTs = 0;
  bool isolated = false;
};

SensorHistory ultrasonicMega;
SensorHistory ultrasonicSpoofCheck; // tracks the uno's claimed channel

const float ULTRASONIC_MIN_CM = 2.0;
const float ULTRASONIC_MAX_CM = 400.0;
const float MAX_JUMP_CM = 150.0;

// 0 = totally untrusted, 1 = fully trusted
float scoreUltrasonicReading(float value, unsigned long ts, SensorHistory &hist) {
  float score = 1.0;

  if (value < 0) return 0.0; // no echo at all

  if (value < ULTRASONIC_MIN_CM || value > ULTRASONIC_MAX_CM) {
    score -= 0.7;
  }

  if (hist.lastGoodTs != 0) {
    float jump = abs(value - hist.lastGoodValue);
    if (jump > MAX_JUMP_CM) {
      score -= 0.5;
    }
  }

  if (score < 0) score = 0;
  return score;
}

void broadcastJson(JsonDocument &doc) {
  String out;
  serializeJson(doc, out);
  ws.textAll(out);
}

void sendEvent(const char* level, const String &message) {
  StaticJsonDocument<256> doc;
  doc["type"] = "event";
  doc["level"] = level; // "info" | "warning" | "critical"
  doc["message"] = message;
  doc["ts"] = millis();
  broadcastJson(doc);
}

void handleMegaLine(const String &line) {
  StaticJsonDocument<384> doc;
  DeserializationError err = deserializeJson(doc, line);
  if (err) return;

  float value = doc["ultrasonic"]["value_cm"] | -1.0;
  unsigned long ts = doc["ts"] | 0;
  const char* faultSim = doc["ultrasonic"]["fault_sim"] | "none";

  float score = scoreUltrasonicReading(value, ts, ultrasonicMega);
  bool trusted = score >= 0.5;

  if (trusted) {
    if (ultrasonicMega.isolated) {
      sendEvent("info", "Ultrasonic (Mega) RECOVERED — trust restored");
    }
    ultrasonicMega.isolated = false;
    ultrasonicMega.lastGoodValue = value;
    ultrasonicMega.lastGoodTs = ts;
  } else {
    if (!ultrasonicMega.isolated) {
      sendEvent("critical",
        String("Ultrasonic (Mega) ISOLATED — fault_sim=") + faultSim +
        " raw_value=" + String(value));
    }
    ultrasonicMega.isolated = true;
  }

  StaticJsonDocument<384> out;
  out["type"] = "telemetry";
  out["source"] = "mega";
  out["sensor"] = "ultrasonic";
  out["raw_value"] = value;
  out["reported_value"] = trusted ? value : ultrasonicMega.lastGoodValue;
  out["trust_score"] = score;
  out["isolated"] = ultrasonicMega.isolated;
  out["fault_sim"] = faultSim;
  broadcastJson(out);

  if (doc.containsKey("ir")) {
    StaticJsonDocument<256> irOut;
    irOut["type"] = "telemetry";
    irOut["source"] = "mega";
    irOut["sensor"] = "ir";
    irOut["reported_value"] = doc["ir"]["value"] | 0;
    irOut["trust_score"] = 1.0;
    irOut["isolated"] = false;
    broadcastJson(irOut);
  }
}

void handleUnoLine(const String &line) {
  StaticJsonDocument<384> doc;
  DeserializationError err = deserializeJson(doc, line);
  if (err) return;

  float value = doc["ultrasonic"]["value_cm"] | -1.0;
  unsigned long ts = doc["ts"] | 0;

  float score = scoreUltrasonicReading(value, ts, ultrasonicSpoofCheck);
  bool trusted = score >= 0.5;

  if (!trusted) {
    sendEvent("warning",
      String("Spoofed packet rejected on ultrasonic channel, value=") +
      String(value) + " score=" + String(score));
  } else {
    sendEvent("warning",
      String("Spoofed packet got through scoring, value=") + String(value));
  }

  ultrasonicSpoofCheck.lastGoodValue = value;
  ultrasonicSpoofCheck.lastGoodTs = ts;
}

void onWsEvent(AsyncWebSocket *server, AsyncWebSocketClient *client,
               AwsEventType type, void *arg, uint8_t *data, size_t len) {
  if (type == WS_EVT_DATA) {
    String msg;
    for (size_t i = 0; i < len; i++) msg += (char)data[i];

    if (msg == "CMD:CORRUPT_ON") {
      MegaSerial.println("CORRUPT:ON");
    } else if (msg == "CMD:CORRUPT_OFF") {
      MegaSerial.println("CORRUPT:OFF");
    }
  }
}

void setup() {
  Serial.begin(115200);
  MegaSerial.begin(115200, SERIAL_8N1, MEGA_RX_PIN, MEGA_TX_PIN);
  UnoSerial.begin(115200, SERIAL_8N1, UNO_RX_PIN, UNO_TX_PIN);

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(400);
    Serial.print(".");
  }
  Serial.println();
  Serial.print("Connected. IP address: ");
  Serial.println(WiFi.localIP());

  ws.onEvent(onWsEvent);
  server.addHandler(&ws);
  server.begin();
}

String megaBuffer;
String unoBuffer;

void loop() {
  ws.cleanupClients();

  while (MegaSerial.available()) {
    char c = MegaSerial.read();
    if (c == '\n') {
      handleMegaLine(megaBuffer);
      megaBuffer = "";
    } else {
      megaBuffer += c;
    }
  }

  while (UnoSerial.available()) {
    char c = UnoSerial.read();
    if (c == '\n') {
      handleUnoLine(unoBuffer);
      unoBuffer = "";
    } else {
      unoBuffer += c;
    }
  }
}
