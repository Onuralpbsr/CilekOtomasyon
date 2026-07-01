// Sadece fiziksel olarak bağlı/kullanılan röleler gösterilir. relay1/relay2/relay3
// hiçbir cihaza bağlı değil; firmware'de hâlâ /api/control ile erişilebilirler
// ama kullanılmadıkları için panelde gösterilmiyorlar.
const RELAYS = [
  { key: "pump", label: "Pompa" },
];

const STAGE_NAMES = ["Fide", "Vegetatif", "Çiçek", "Meyve"];

// Renkler metrik türüne göre sabit (tutarlı okuma için): sıcaklık, nem,
// ışık, VPD, hacim, oran ve güç her yerde aynı renk.
const COLOR = {
  temp: "#e0418f",
  hum: "#3498db",
  light: "#f1c40f",
  vpd: "#e2574c",
  volume: "#9b59b6",
  ratio: "#e67e22",
  power: "#4caf6d",
};

// Her grafik tek bir veri serisi gösterir - kategori başlığı altında gruplanır.
const CHART_DEFS = [
  { id: "chartAmbientTemp", group: "İklim", title: "Ortam Sıcaklık (BME280)", unit: "°C", field: "ambient_temp", color: COLOR.temp },
  { id: "chartAmbientHum", group: "İklim", title: "Ortam Nem (BME280)", unit: "%", field: "ambient_hum", color: COLOR.hum },
  { id: "chartSht1Temp", group: "İklim", title: "SHT30 #1 Sıcaklık", unit: "°C", field: "sht1_temp", color: COLOR.temp },
  { id: "chartSht1Hum", group: "İklim", title: "SHT30 #1 Nem", unit: "%", field: "sht1_hum", color: COLOR.hum },
  { id: "chartSht2Temp", group: "İklim", title: "SHT30 #2 Sıcaklık", unit: "°C", field: "sht2_temp", color: COLOR.temp },
  { id: "chartSht2Hum", group: "İklim", title: "SHT30 #2 Nem", unit: "%", field: "sht2_hum", color: COLOR.hum },
  { id: "chartLux", group: "İklim", title: "Işık", unit: "lux", field: "lux", color: COLOR.light },
  { id: "chartVpd", group: "İklim", title: "VPD", unit: "kPa", field: "vpd_kpa", color: COLOR.vpd },
  { id: "chartRootTemp", group: "Kök & Substrat", title: "Kök Bölgesi Sıcaklığı", unit: "°C", field: "rootzone_temp", color: COLOR.temp },
  { id: "chartNutrientTemp", group: "Kök & Substrat", title: "Besin Çözeltisi Sıcaklığı", unit: "°C", field: "nutrient_temp", color: COLOR.temp },
  { id: "chartSoilMoisture", group: "Kök & Substrat", title: "Substrat Nemi", unit: "%", field: "soil_moisture_pct", color: COLOR.hum },
  { id: "chartFlowIn", group: "Sulama", title: "Sulama", unit: "L", field: "flow_in_l", color: COLOR.volume },
  { id: "chartFlowDrain", group: "Sulama", title: "Drenaj", unit: "L", field: "flow_drain_l", color: COLOR.volume },
  { id: "chartRunoff", group: "Sulama", title: "Drenaj Oranı", unit: "%", field: "runoff_pct", color: COLOR.ratio },
  { id: "chartPumpCurrent", group: "Güç", title: "Pompa Akımı", unit: "A", field: "pump_current_a", color: COLOR.power },
  { id: "chartAcPower", group: "Güç", title: "AC Güç", unit: "W", field: "ac_power_w", color: COLOR.power },
  { id: "chartAcEnergy", group: "Güç", title: "AC Enerji", unit: "kWh", field: "ac_energy_kwh", color: COLOR.power },
];

const charts = {};

function card(label, value, unit) {
  return `<div class="card"><div class="label">${label}</div><div class="value">${value}<span class="unit"> ${unit || ""}</span></div></div>`;
}

const STAT_ICONS = {
  stage: '<path d="M12 21V11M12 11C6.5 11 3 7 3 3c5.5 0 9 3.5 9 8zm0 0c0-4.5 3.5-8 9-8 0 4-3.5 8-9 8z"/>',
  water: '<path d="M12 2s7 7.5 7 12a7 7 0 1 1-14 0c0-4.5 7-12 7-12z"/>',
  clock: '<circle cx="12" cy="12" r="9"/><path d="M12 7v5l3 3"/>',
  calendar: '<rect x="3" y="5" width="18" height="16" rx="2"/><path d="M8 3v4M16 3v4M3 10h18"/>',
  climate: '<path d="M10 13.5V4a2 2 0 1 1 4 0v9.5a4 4 0 1 1-4 0z"/>',
  bell: '<path d="M12 3a6 6 0 0 0-6 6v4l-2 4h16l-2-4V9a6 6 0 0 0-6-6z"/><path d="M9.5 20.5a2.5 2.5 0 0 0 5 0"/>',
};

function stat(label, value, alert = false, icon = "") {
  const iconSvg = icon
    ? `<svg class="stat-icon" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round" aria-hidden="true">${icon}</svg>`
    : "";
  return `<div class="stat ${alert ? "alert" : ""}"><div class="stat-head">${iconSvg}<span class="label">${label}</span></div><div class="value">${value}</div></div>`;
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
    document.getElementById("lastUpdate").textContent =
      `Son güncelleme: ${new Date().toLocaleTimeString("tr-TR", { hour: "2-digit", minute: "2-digit", second: "2-digit" })}`;

    const hasFault = status.faults.pumpFault || status.faults.waterLowFault || status.climate.alertActive;

    document.getElementById("statusStrip").innerHTML = [
      stat("Evre", STAGE_NAMES[status.stage] ?? "—", false, STAT_ICONS.stage),
      stat("Su Seviyesi", status.irrigation.waterLevelOk ? "Normal" : "DÜŞÜK", !status.irrigation.waterLevelOk, STAT_ICONS.water),
      stat("Sulama", status.irrigation.inProgress ? "Sürüyor" : "Bekliyor", false, STAT_ICONS.clock),
      stat("Bugün Sulama", `${status.irrigation.irrigationsToday} kez`, false, STAT_ICONS.calendar),
      stat("İklim", status.climate.alertActive ? "Hedef dışı" : "Normal", status.climate.alertActive, STAT_ICONS.climate),
      stat("Uyarı", hasFault ? (status.faults.lastAlarm || "Aktif arıza") : "Yok", hasFault, STAT_ICONS.bell),
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
      return `<button class="pump-card ${on ? "on" : ""}" data-relay="${r.key}" data-on="${!on}">
        <span class="pump-icon-wrap">
          <svg class="pump-icon" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round" aria-hidden="true"><path d="M12 2s7 7.5 7 12a7 7 0 1 1-14 0c0-4.5 7-12 7-12z"/></svg>
        </span>
        <span class="pump-card-text">
          <span class="pump-card-label">${r.label}</span>
          <span class="pump-card-state">${on ? "AÇIK" : "KAPALI"}</span>
        </span>
        <span class="pump-card-indicator"></span>
      </button>`;
    }).join("");
    relayDiv.querySelectorAll(".pump-card").forEach(btn => {
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

const COLOR_LEGEND = [
  { label: "Sıcaklık", color: COLOR.temp },
  { label: "Nem", color: COLOR.hum },
  { label: "Işık", color: COLOR.light },
  { label: "VPD", color: COLOR.vpd },
  { label: "Hacim", color: COLOR.volume },
  { label: "Oran", color: COLOR.ratio },
  { label: "Güç", color: COLOR.power },
];

function buildHistoryChartLayout() {
  const container = document.getElementById("historyCharts");
  const groups = [...new Set(CHART_DEFS.map(c => c.group))];

  const legend = `<div class="chart-legend">${COLOR_LEGEND.map(l =>
    `<span class="chart-legend-item"><span class="dot" style="background:${l.color}"></span>${l.label}</span>`
  ).join("")}</div>`;

  const groupsHtml = groups.map(group => `
    <div class="chart-group">
      <h3>${group}</h3>
      <div class="chart-mini-grid">
        ${CHART_DEFS.filter(c => c.group === group).map(c => `
          <div class="chart-mini" style="--chart-color:${c.color}">
            <div class="chart-mini-title">
              <span>${c.title} <span class="unit">${c.unit}</span></span>
              <span class="latest" id="${c.id}-latest"></span>
            </div>
            <div class="chart-mini-canvas"><canvas id="${c.id}"></canvas></div>
          </div>
        `).join("")}
      </div>
    </div>
  `).join("");

  container.innerHTML = legend + groupsHtml;
}

function hexToRgba(hex, alpha) {
  const n = parseInt(hex.slice(1), 16);
  const r = (n >> 16) & 255, g = (n >> 8) & 255, b = n & 255;
  return `rgba(${r}, ${g}, ${b}, ${alpha})`;
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

const UNIT_DIGITS = { "°C": 1, "%": 0, lux: 0, kPa: 2, L: 2, A: 2, W: 0, kWh: 3 };

function latestValue(rows, field) {
  for (let i = rows.length - 1; i >= 0; i--) {
    const v = rows[i][field];
    if (v !== null && v !== undefined) return v;
  }
  return null;
}

async function refreshHistory() {
  if (!document.getElementById(CHART_DEFS[0].id)) buildHistoryChartLayout();

  const res = await fetch("/api/history?hours=24");
  const rows = await res.json();
  const labels = rows.map(r => new Date(r.ts).toLocaleTimeString("tr-TR", { hour: "2-digit", minute: "2-digit" }));

  CHART_DEFS.forEach(c => {
    const data = {
      labels,
      datasets: [{
        data: rows.map(r => r[c.field]),
        borderColor: c.color,
        backgroundColor: hexToRgba(c.color, 0.12),
        fill: true,
        tension: 0.3,
        borderWidth: 1.5,
      }],
    };
    if (charts[c.id]) {
      charts[c.id].data = data;
      charts[c.id].update();
    } else {
      charts[c.id] = new Chart(document.getElementById(c.id), { type: "line", data, options: miniChartOptions() });
    }

    const latestEl = document.getElementById(`${c.id}-latest`);
    if (latestEl) latestEl.textContent = fmt(latestValue(rows, c.field), UNIT_DIGITS[c.unit] ?? 1);
  });
}

refreshLive();
refreshHistory();
setInterval(refreshLive, 5000);
setInterval(refreshHistory, 60000);

function setupScrollSpy() {
  const navLinks = document.querySelectorAll(".quicknav a");
  const sections = [...navLinks].map(a => document.getElementById(a.getAttribute("href").slice(1))).filter(Boolean);

  const observer = new IntersectionObserver((entries) => {
    entries.forEach(entry => {
      if (!entry.isIntersecting) return;
      navLinks.forEach(a => a.classList.toggle("active", a.getAttribute("href") === `#${entry.target.id}`));
    });
  }, { rootMargin: "-72px 0px -70% 0px" });

  sections.forEach(s => observer.observe(s));
}

setupScrollSpy();
