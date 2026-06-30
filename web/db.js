const { DatabaseSync } = require("node:sqlite");
const fs = require("fs");
const path = require("path");
const { DB_PATH } = require("./config");

fs.mkdirSync(path.dirname(DB_PATH), { recursive: true });
const db = new DatabaseSync(DB_PATH);

db.exec(`
  CREATE TABLE IF NOT EXISTS readings (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    ts INTEGER NOT NULL,
    ambient_temp REAL,
    ambient_hum REAL,
    pressure_hpa REAL,
    sht1_temp REAL,
    sht1_hum REAL,
    sht2_temp REAL,
    sht2_hum REAL,
    lux INTEGER,
    vpd_kpa REAL,
    climate_alert INTEGER,
    nutrient_temp REAL,
    rootzone_temp REAL,
    soil_moisture_pct REAL,
    flow_in_l REAL,
    flow_drain_l REAL,
    runoff_pct REAL,
    pump_voltage REAL,
    pump_current_a REAL,
    pump_power_w REAL,
    ac_voltage REAL,
    ac_current_a REAL,
    ac_power_w REAL,
    ac_energy_kwh REAL,
    ac_frequency REAL,
    ac_power_factor REAL,
    water_level_ok INTEGER,
    pump_fault INTEGER,
    water_low_fault INTEGER,
    stage INTEGER
  );
  CREATE INDEX IF NOT EXISTS idx_readings_ts ON readings(ts);
`);

const insertStmt = db.prepare(`
  INSERT INTO readings (
    ts, ambient_temp, ambient_hum, pressure_hpa, sht1_temp, sht1_hum, sht2_temp, sht2_hum, lux,
    vpd_kpa, climate_alert,
    nutrient_temp, rootzone_temp, soil_moisture_pct, flow_in_l, flow_drain_l, runoff_pct,
    pump_voltage, pump_current_a, pump_power_w,
    ac_voltage, ac_current_a, ac_power_w, ac_energy_kwh, ac_frequency, ac_power_factor,
    water_level_ok, pump_fault, water_low_fault, stage
  ) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)
`);

function n(v) {
  return v === undefined || v === null || Number.isNaN(v) ? null : v;
}

function insertReading(status) {
  insertStmt.run(
    Date.now(),
    n(status.climate?.ambientTempBME),
    n(status.climate?.ambientHumBME),
    n(status.climate?.pressureHPa),
    n(status.climate?.tempSHT1),
    n(status.climate?.humSHT1),
    n(status.climate?.tempSHT2),
    n(status.climate?.humSHT2),
    n(status.climate?.lux),
    n(status.climate?.vpdKPa),
    status.climate?.alertActive ? 1 : 0,
    n(status.root?.nutrientTempC),
    n(status.root?.rootZoneTempC),
    n(status.root?.soilMoisturePct),
    n(status.irrigation?.flowInLitresToday),
    n(status.irrigation?.flowDrainLitresToday),
    n(status.irrigation?.runoffPctToday),
    n(status.power?.pumpVoltage),
    n(status.power?.pumpCurrentA),
    n(status.power?.pumpPowerW),
    n(status.power?.acVoltage),
    n(status.power?.acCurrentA),
    n(status.power?.acPowerW),
    n(status.power?.acEnergyKWh),
    n(status.power?.acFrequency),
    n(status.power?.acPowerFactor),
    status.irrigation?.waterLevelOk ? 1 : 0,
    status.faults?.pumpFault ? 1 : 0,
    status.faults?.waterLowFault ? 1 : 0,
    n(status.stage)
  );
}

const historyStmt = db.prepare("SELECT * FROM readings WHERE ts >= ? ORDER BY ts ASC");

function getHistory(hoursBack = 24) {
  const since = Date.now() - hoursBack * 3600 * 1000;
  return historyStmt.all(since);
}

module.exports = { insertReading, getHistory };
