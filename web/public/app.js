const RELAYS = [
  { key: "pump", label: "Pompa" },
  { key: "fan", label: "Fan" },
  { key: "light", label: "Işık" },
  { key: "climate", label: "Isıtıcı/Nem" },
];

let climateChart, irrigationChart;

function card(label, value, unit) {
  return `<div class="card"><div class="label">${label}</div><div class="value">${value}<span class="unit"> ${unit || ""}</span></div></div>`;
}

function fmt(n, digits = 1) {
  return (n === null || n === undefined || Number.isNaN(n)) ? "—" : Number(n).toFixed(digits);
}

async function refreshLive() {
  const connBadge = document.getElementById("connStatus");
  try {
    const res = await fetch("/api/live");
    const { status, error } = await res.json();

    if (error || !status) {
      connBadge.textContent = "ESP32 bağlantısı yok";
      connBadge.className = "badge err";
      return;
    }
    connBadge.textContent = "Bağlı";
    connBadge.className = "badge ok";

    const cards = document.getElementById("cards");
    cards.innerHTML = [
      card("Ortam Sıcaklık", fmt(status.climate.ambientTempBME), "°C"),
      card("Ortam Nem", fmt(status.climate.ambientHumBME), "%"),
      card("Işık", fmt(status.climate.lux, 0), "lux"),
      card("Besin Sıcaklığı", fmt(status.root.nutrientTempC), "°C"),
      card("Kök Bölgesi Sıcaklığı", fmt(status.root.rootZoneTempC), "°C"),
      card("Substrate Nem", fmt(status.root.soilMoisturePct), "%"),
      card("Sulama (Bugün)", fmt(status.irrigation.flowInLitresToday, 2), "L"),
      card("Drenaj (Bugün)", fmt(status.irrigation.flowDrainLitresToday, 2), "L"),
      card("Sulama Sayısı", status.irrigation.irrigationsToday, "kez"),
      card("Pompa Akımı", fmt(status.power.pumpCurrentA, 2), "A"),
      card("AC Güç", fmt(status.power.acPowerW, 0), "W"),
      card("Su Seviyesi", status.irrigation.waterLevelOk ? "Normal" : "DÜŞÜK", ""),
    ].join("");

    const relayDiv = document.getElementById("relayControls");
    relayDiv.innerHTML = RELAYS.map(r => {
      const on = status.relays[r.key];
      return `<button class="relay-btn ${on ? "on" : ""}" data-relay="${r.key}" data-on="${!on}">${r.label}: ${on ? "AÇIK" : "KAPALI"}</button>`;
    }).join("");
    relayDiv.querySelectorAll(".relay-btn").forEach(btn => {
      btn.onclick = () => sendControl({ relay: btn.dataset.relay, on: btn.dataset.on === "true" });
    });

    const alarms = document.getElementById("alarms");
    const hasFault = status.faults.pumpFault || status.faults.waterLowFault;
    alarms.className = hasFault ? "has-alarm" : "";
    alarms.textContent = hasFault ? (status.faults.lastAlarm || "Aktif arıza var") : "Aktif uyarı yok";

  } catch (e) {
    connBadge.textContent = "Panel sunucusuna ulaşılamıyor";
    connBadge.className = "badge err";
  }
}

async function sendControl(body) {
  await fetch("/api/control", {
    method: "POST",
    headers: { "Content-Type": "application/json" },
    body: JSON.stringify(body),
  });
  refreshLive();
}

document.getElementById("clearFaultsBtn").onclick = () => sendControl({ clearFaults: true });

async function refreshHistory() {
  const res = await fetch("/api/history?hours=24");
  const rows = await res.json();
  const labels = rows.map(r => new Date(r.ts).toLocaleTimeString("tr-TR", { hour: "2-digit", minute: "2-digit" }));

  const climateData = {
    labels,
    datasets: [
      { label: "Ortam Sıcaklık (°C)", data: rows.map(r => r.ambient_temp), borderColor: "#e0418f", tension: 0.3 },
      { label: "Ortam Nem (%)", data: rows.map(r => r.ambient_hum), borderColor: "#4caf6d", tension: 0.3 },
      { label: "Kök Bölgesi (°C)", data: rows.map(r => r.rootzone_temp), borderColor: "#f1c40f", tension: 0.3 },
    ],
  };

  const irrigationData = {
    labels,
    datasets: [
      { label: "Substrate Nem (%)", data: rows.map(r => r.soil_moisture_pct), borderColor: "#3498db", tension: 0.3 },
      { label: "Sulama (L, kümülatif)", data: rows.map(r => r.flow_in_l), borderColor: "#9b59b6", tension: 0.3 },
      { label: "Drenaj (L, kümülatif)", data: rows.map(r => r.flow_drain_l), borderColor: "#e67e22", tension: 0.3 },
    ],
  };

  const opts = { responsive: true, interaction: { mode: "index", intersect: false } };

  if (climateChart) { climateChart.data = climateData; climateChart.update(); }
  else climateChart = new Chart(document.getElementById("climateChart"), { type: "line", data: climateData, options: opts });

  if (irrigationChart) { irrigationChart.data = irrigationData; irrigationChart.update(); }
  else irrigationChart = new Chart(document.getElementById("irrigationChart"), { type: "line", data: irrigationData, options: opts });
}

refreshLive();
refreshHistory();
setInterval(refreshLive, 5000);
setInterval(refreshHistory, 60000);
