// Sadece fiziksel olarak bağlı/kullanılan röleler gösterilir. relay1/relay2/relay3
// hiçbir cihaza bağlı değil; firmware'de hâlâ /api/control ile erişilebilirler
// ama kullanılmadıkları için panelde gösterilmiyorlar.
const RELAYS = [
  { key: "pump", label: "Pompa" },
];

const STAGE_NAMES = ["Fide", "Vegetatif", "Çiçek", "Meyve"];

// Her grafik tek bir veri serisi gösterir - kategori başlığı altında gruplanır.
const CHART_DEFS = [
  { id: "chartAmbientTemp", group: "İklim", title: "Ortam Sıcaklık (BME280)", unit: "°C", field: "ambient_temp", color: "#e0418f" },
  { id: "chartAmbientHum", group: "İklim", title: "Ortam Nem (BME280)", unit: "%", field: "ambient_hum", color: "#4caf6d" },
  { id: "chartSht1Temp", group: "İklim", title: "SHT30 #1 Sıcaklık", unit: "°C", field: "sht1_temp", color: "#9b59b6" },
  { id: "chartSht1Hum", group: "İklim", title: "SHT30 #1 Nem", unit: "%", field: "sht1_hum", color: "#9b59b6" },
  { id: "chartSht2Temp", group: "İklim", title: "SHT30 #2 Sıcaklık", unit: "°C", field: "sht2_temp", color: "#3498db" },
  { id: "chartSht2Hum", group: "İklim", title: "SHT30 #2 Nem", unit: "%", field: "sht2_hum", color: "#3498db" },
  { id: "chartLux", group: "İklim", title: "Işık", unit: "lux", field: "lux", color: "#f1c40f" },
  { id: "chartVpd", group: "İklim", title: "VPD", unit: "kPa", field: "vpd_kpa", color: "#e2574c" },
  { id: "chartRootTemp", group: "Kök & Substrat", title: "Kök Bölgesi Sıcaklığı", unit: "°C", field: "rootzone_temp", color: "#f1c40f" },
  { id: "chartNutrientTemp", group: "Kök & Substrat", title: "Besin Çözeltisi Sıcaklığı", unit: "°C", field: "nutrient_temp", color: "#e67e22" },
  { id: "chartSoilMoisture", group: "Kök & Substrat", title: "Substrat Nemi", unit: "%", field: "soil_moisture_pct", color: "#3498db" },
  { id: "chartFlowIn", group: "Sulama", title: "Sulama", unit: "L", field: "flow_in_l", color: "#9b59b6" },
  { id: "chartFlowDrain", group: "Sulama", title: "Drenaj", unit: "L", field: "flow_drain_l", color: "#e67e22" },
  { id: "chartRunoff", group: "Sulama", title: "Drenaj Oranı", unit: "%", field: "runoff_pct", color: "#e2574c" },
  { id: "chartPumpCurrent", group: "Güç", title: "Pompa Akımı", unit: "A", field: "pump_current_a", color: "#e0418f" },
  { id: "chartAcPower", group: "Güç", title: "AC Güç", unit: "W", field: "ac_power_w", color: "#4caf6d" },
  { id: "chartAcEnergy", group: "Güç", title: "AC Enerji", unit: "kWh", field: "ac_energy_kwh", color: "#3498db" },
];

const charts = {};

function card(label, value, unit) {
  return `<div class="card"><div class="label">${label}</div><div class="value">${value}<span class="unit"> ${unit || ""}</span></div></div>`;
}

function stat(label, value, alert = false) {
  return `<div class="stat ${alert ? "alert" : ""}"><div class="label">${label}</div><div class="value">${value}</div></div>`;
}

function fmt(n, digits = 1) {
  return (n === null || n === undefined || Number.isNaN(n)) ? "—" : Number(n).toFixed(digits);
}

function setBadge(el, text, cls) {
  el.innerHTML = `<span class="dot"></span>${text}`;
  el.className = `badge ${cls}`;
}

async function refreshLive() {
  const connBadge = document.getElementById("connStatus");
  try {
    const res = await fetch("/api/live");
    const { status, error } = await res.json();

    if (error || !status) {
      setBadge(connBadge, "ESP32 bağlantısı yok", "err");
      return;
    }
    setBadge(connBadge, "Bağlı", "ok");

    const hasFault = status.faults.pumpFault || status.faults.waterLowFault || status.climate.alertActive;

    document.getElementById("statusStrip").innerHTML = [
      stat("Evre", STAGE_NAMES[status.stage] ?? "—"),
      stat("Su Seviyesi", status.irrigation.waterLevelOk ? "Normal" : "DÜŞÜK", !status.irrigation.waterLevelOk),
      stat("Sulama", status.irrigation.inProgress ? "Sürüyor" : "Bekliyor"),
      stat("Bugün Sulama", `${status.irrigation.irrigationsToday} kez`),
      stat("İklim", status.climate.alertActive ? "Hedef dışı" : "Normal", status.climate.alertActive),
      stat("Uyarı", hasFault ? (status.faults.lastAlarm || "Aktif arıza") : "Yok", hasFault),
    ].join("");

    document.getElementById("cardsClimate").innerHTML = [
      card("Ortam Sıcaklık (BME280)", fmt(status.climate.ambientTempBME), "°C"),
      card("Ortam Nem (BME280)", fmt(status.climate.ambientHumBME), "%"),
      card("Basınç", fmt(status.climate.pressureHPa, 0), "hPa"),
      card("SHT30 #1", `${fmt(status.climate.tempSHT1)}°C / ${fmt(status.climate.humSHT1)}`, "%"),
      card("SHT30 #2", `${fmt(status.climate.tempSHT2)}°C / ${fmt(status.climate.humSHT2)}`, "%"),
      card("Işık", fmt(status.climate.lux, 0), "lux"),
      card("VPD", fmt(status.climate.vpdKPa, 2), "kPa"),
    ].join("");

    document.getElementById("cardsRoot").innerHTML = [
      card("Besin Sıcaklığı", fmt(status.root.nutrientTempC), "°C"),
      card("Kök Bölgesi Sıcaklığı", fmt(status.root.rootZoneTempC), "°C"),
      card("Substrat Nemi", fmt(status.root.soilMoisturePct), "%"),
    ].join("");

    document.getElementById("cardsIrrigation").innerHTML = [
      card("Sulama (Bugün)", fmt(status.irrigation.flowInLitresToday, 2), "L"),
      card("Drenaj (Bugün)", fmt(status.irrigation.flowDrainLitresToday, 2), "L"),
      card("Sulama Sayısı", status.irrigation.irrigationsToday, "kez"),
      card("Drenaj Oranı", `${fmt(status.irrigation.runoffPctToday, 0)} / ${fmt(status.irrigation.runoffTargetPct, 0)}`, "% (gerçek/hedef)"),
    ].join("");

    document.getElementById("cardsPower").innerHTML = [
      card("Pompa Voltajı", fmt(status.power.pumpVoltage, 2), "V"),
      card("Pompa Akımı", fmt(status.power.pumpCurrentA, 2), "A"),
      card("Pompa Güç", fmt(status.power.pumpPowerW, 1), "W"),
      card("AC Voltaj", fmt(status.power.acVoltage, 0), "V"),
      card("AC Akım", fmt(status.power.acCurrentA, 2), "A"),
      card("AC Güç", fmt(status.power.acPowerW, 0), "W"),
      card("AC Enerji (toplam)", fmt(status.power.acEnergyKWh, 3), "kWh"),
      card("AC Frekans", fmt(status.power.acFrequency, 0), "Hz"),
      card("AC Güç Faktörü", fmt(status.power.acPowerFactor, 2), ""),
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
    alarms.className = hasFault ? "has-alarm" : "";
    alarms.textContent = hasFault ? (status.faults.lastAlarm || "Aktif arıza var") : "Aktif uyarı yok";
    if (status.climate.alertActive && !status.faults.pumpFault && !status.faults.waterLowFault) {
      alarms.textContent += " (fan/ısıtıcı/nemlendirici yok, manuel müdahale gerekebilir)";
    }

  } catch (e) {
    setBadge(connBadge, "Panel sunucusuna ulaşılamıyor", "err");
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

function buildHistoryChartLayout() {
  const container = document.getElementById("historyCharts");
  const groups = [...new Set(CHART_DEFS.map(c => c.group))];
  container.innerHTML = groups.map(group => `
    <div class="chart-group">
      <h3>${group}</h3>
      <div class="chart-mini-grid">
        ${CHART_DEFS.filter(c => c.group === group).map(c => `
          <div class="chart-mini">
            <div class="chart-mini-title">${c.title} <span class="unit">${c.unit}</span></div>
            <div class="chart-mini-canvas"><canvas id="${c.id}"></canvas></div>
          </div>
        `).join("")}
      </div>
    </div>
  `).join("");
}

function miniChartOptions() {
  return {
    responsive: true,
    maintainAspectRatio: false,
    animation: false,
    plugins: { legend: { display: false }, tooltip: { mode: "index", intersect: false } },
    elements: { point: { radius: 0 } },
    scales: {
      x: { ticks: { maxTicksLimit: 4, font: { size: 10 } }, grid: { display: false } },
      y: { ticks: { maxTicksLimit: 4, font: { size: 10 } } },
    },
  };
}

async function refreshHistory() {
  if (!document.getElementById(CHART_DEFS[0].id)) buildHistoryChartLayout();

  const res = await fetch("/api/history?hours=24");
  const rows = await res.json();
  const labels = rows.map(r => new Date(r.ts).toLocaleTimeString("tr-TR", { hour: "2-digit", minute: "2-digit" }));

  CHART_DEFS.forEach(c => {
    const data = {
      labels,
      datasets: [{ data: rows.map(r => r[c.field]), borderColor: c.color, tension: 0.3, borderWidth: 1.5 }],
    };
    if (charts[c.id]) {
      charts[c.id].data = data;
      charts[c.id].update();
    } else {
      charts[c.id] = new Chart(document.getElementById(c.id), { type: "line", data, options: miniChartOptions() });
    }
  });
}

refreshLive();
refreshHistory();
setInterval(refreshLive, 5000);
setInterval(refreshHistory, 60000);
