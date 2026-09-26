let ws = null;
let corruptOn = false;

const connStatus = document.getElementById("connStatus");
const eventLog = document.getElementById("eventLog");

function setConnected(connected) {
  connStatus.textContent = connected ? "Connected" : "Disconnected";
  connStatus.className = "status " + (connected ? "connected" : "disconnected");
}

function logEvent(level, message, ts) {
  const line = document.createElement("div");
  line.className = "event-line " + level;
  const time = new Date().toLocaleTimeString();
  line.textContent = `[${time}] ${message}`;
  eventLog.prepend(line);
}

function updateSensorCard(sensor, reportedValue, trustScore, isolated) {
  const valEl = document.getElementById(`val-${sensor}`);
  const trustEl = document.getElementById(`trust-${sensor}`);
  const statusEl = document.getElementById(`status-${sensor}`);
  if (!valEl) return;

  valEl.textContent = reportedValue;
  trustEl.style.width = `${Math.round(trustScore * 100)}%`;

  if (isolated) {
    trustEl.style.background = "var(--bad)";
    statusEl.textContent = "ISOLATED — using fallback value";
    statusEl.style.color = "var(--bad)";
  } else if (trustScore < 0.8) {
    trustEl.style.background = "var(--warn)";
    statusEl.textContent = "degraded trust";
    statusEl.style.color = "var(--warn)";
  } else {
    trustEl.style.background = "var(--good)";
    statusEl.textContent = "nominal";
    statusEl.style.color = "var(--good)";
  }
}

function connect() {
  const ip = document.getElementById("espIp").value.trim();
  if (!ip) return;

  ws = new WebSocket(`ws://${ip}/ws`);

  ws.onopen = () => {
    setConnected(true);
    logEvent("info", "Connected to ESP32");
  };

  ws.onclose = () => {
    setConnected(false);
    logEvent("critical", "Disconnected from ESP32");
  };

  ws.onerror = () => {
    logEvent("critical", "connection error, check the IP and that you're on the same wifi");
  };

  ws.onmessage = (event) => {
    let msg;
    try {
      msg = JSON.parse(event.data);
    } catch (e) {
      return;
    }

    if (msg.type === "telemetry") {
      updateSensorCard(msg.sensor, msg.reported_value, msg.trust_score, msg.isolated);
    } else if (msg.type === "event") {
      logEvent(msg.level, msg.message, msg.ts);
    }
  };
}

document.getElementById("connectBtn").addEventListener("click", connect);

document.getElementById("corruptBtn").addEventListener("click", () => {
  if (!ws || ws.readyState !== WebSocket.OPEN) {
    logEvent("warning", "Not connected — can't send fault command");
    return;
  }
  corruptOn = !corruptOn;
  ws.send(corruptOn ? "CMD:CORRUPT_ON" : "CMD:CORRUPT_OFF");
  logEvent("info", `Sent corruption fault: ${corruptOn ? "ON" : "OFF"}`);
});
