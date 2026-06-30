const express = require("express");
const fetch = require("node-fetch");
const { ESP32_BASE_URL, POLL_INTERVAL_MS, WEB_PORT } = require("./config");
const { insertReading, getHistory } = require("./db");

const app = express();
app.use(express.json());
app.use(express.static("public"));

let latestStatus = null;
let lastPollError = null;

async function pollEsp32() {
  try {
    const res = await fetch(`${ESP32_BASE_URL}/api/status`, { timeout: 4000 });
    if (!res.ok) throw new Error(`HTTP ${res.status}`);
    const status = await res.json();
    latestStatus = status;
    lastPollError = null;
    if (status.valid) insertReading(status);
  } catch (err) {
    lastPollError = err.message;
  }
}

setInterval(pollEsp32, POLL_INTERVAL_MS);
pollEsp32();

app.get("/api/live", (req, res) => {
  res.json({ status: latestStatus, error: lastPollError });
});

app.get("/api/history", (req, res) => {
  const hours = Number(req.query.hours) || 24;
  res.json(getHistory(hours));
});

app.post("/api/control", async (req, res) => {
  try {
    const r = await fetch(`${ESP32_BASE_URL}/api/control`, {
      method: "POST",
      headers: { "Content-Type": "application/json" },
      body: JSON.stringify(req.body),
      timeout: 4000,
    });
    const data = await r.json();
    res.status(r.status).json(data);
  } catch (err) {
    res.status(502).json({ error: err.message });
  }
});

app.listen(WEB_PORT, () => {
  console.log(`Çilek Otomasyon paneli http://localhost:${WEB_PORT} adresinde çalışıyor`);
});
