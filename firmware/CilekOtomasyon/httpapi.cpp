#include "httpapi.h"
#include "config.h"

#include <WiFi.h>
#include <WebServer.h>
#include <ArduinoJson.h>

static WebServer server(HTTP_SERVER_PORT);
static SensorData *gData = nullptr;
static ControlState *gState = nullptr;

static void handleStatus() {
  if (!gData || !gState) { server.send(503, "application/json", "{\"error\":\"not_ready\"}"); return; }

  JsonDocument doc;
  doc["valid"] = gData->valid;
  doc["time"]["hour"] = gData->hour;
  doc["time"]["minute"] = gData->minute;
  doc["time"]["day"] = gData->day;
  doc["time"]["month"] = gData->month;
  doc["time"]["year"] = gData->year;

  doc["climate"]["ambientTempBME"] = gData->ambientTempBME;
  doc["climate"]["ambientHumBME"] = gData->ambientHumBME;
  doc["climate"]["pressureHPa"] = gData->ambientPressure;
  doc["climate"]["tempSHT1"] = gData->tempSHT1;
  doc["climate"]["humSHT1"] = gData->humSHT1;
  doc["climate"]["tempSHT2"] = gData->tempSHT2;
  doc["climate"]["humSHT2"] = gData->humSHT2;
  doc["climate"]["lux"] = gData->lux;

  doc["root"]["nutrientTempC"] = gData->nutrientTemp;
  doc["root"]["rootZoneTempC"] = gData->rootZoneTemp;
  doc["root"]["soilMoisturePct"] = gData->soilMoisturePct;

  doc["irrigation"]["flowInLitresToday"] = gData->flowInLitresToday;
  doc["irrigation"]["flowDrainLitresToday"] = gData->flowDrainLitresToday;
  doc["irrigation"]["irrigationsToday"] = gState->irrigationsToday;
  doc["irrigation"]["inProgress"] = gState->irrigationInProgress;
  doc["irrigation"]["waterLevelOk"] = gData->waterLevelOk;
  doc["irrigation"]["runoffPctToday"] = gState->runoffPctToday;
  doc["irrigation"]["runoffTargetPct"] = STAGE_PARAMS[gState->stage].targetRunoffPct;

  doc["power"]["pumpVoltage"] = gData->pumpVoltage;
  doc["power"]["pumpCurrentA"] = gData->pumpCurrentA;
  doc["power"]["pumpPowerW"] = gData->pumpPowerW;
  doc["power"]["acVoltage"] = gData->acVoltage;
  doc["power"]["acCurrentA"] = gData->acCurrentA;
  doc["power"]["acPowerW"] = gData->acPowerW;
  doc["power"]["acEnergyKWh"] = gData->acEnergyKWh;
  doc["power"]["acFrequency"] = gData->acFrequency;
  doc["power"]["acPowerFactor"] = gData->acPowerFactor;

  doc["relays"]["pump"] = gState->pumpOn;
  doc["relays"]["fan"] = gState->fanOn;
  doc["relays"]["light"] = gState->lightOn;
  doc["relays"]["climate"] = gState->climateOn;

  doc["faults"]["pumpFault"] = gState->pumpFault;
  doc["faults"]["waterLowFault"] = gState->waterLowFault;
  doc["faults"]["lastAlarm"] = gState->lastAlarm;

  doc["stage"] = (int)gState->stage;

  String out;
  serializeJson(doc, out);
  server.send(200, "application/json", out);
}

static void handleControl() {
  if (!gState) { server.send(503, "application/json", "{\"error\":\"not_ready\"}"); return; }
  if (!server.hasArg("plain")) { server.send(400, "application/json", "{\"error\":\"missing_body\"}"); return; }

  JsonDocument doc;
  DeserializationError err = deserializeJson(doc, server.arg("plain"));
  if (err) { server.send(400, "application/json", "{\"error\":\"bad_json\"}"); return; }

  if (doc["clearFaults"].is<bool>() && doc["clearFaults"].as<bool>()) {
    controlClearFaults(*gState);
  }
  if (doc["stage"].is<int>()) {
    controlSetStage(*gState, (GrowthStage)doc["stage"].as<int>());
  }
  if (doc["relay"].is<const char*>() && doc["on"].is<bool>()) {
    controlManualOverride(*gState, String(doc["relay"].as<const char*>()), doc["on"].as<bool>());
  }

  server.send(200, "application/json", "{\"ok\":true}");
}

void networkInit() {
  WiFi.mode(WIFI_STA);
  WiFi.setAutoReconnect(true);
  WiFi.persistent(true);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - start < 15000) {
    delay(300);
  }

  server.on("/api/status", HTTP_GET, handleStatus);
  server.on("/api/control", HTTP_POST, handleControl);
  server.begin();
}

static unsigned long lastWifiCheckMs = 0;
static const unsigned long WIFI_CHECK_INTERVAL_MS = 30000;

static void ensureWifiConnected() {
  unsigned long now = millis();
  if (now - lastWifiCheckMs < WIFI_CHECK_INTERVAL_MS) return;
  lastWifiCheckMs = now;

  if (WiFi.status() != WL_CONNECTED) {
    WiFi.disconnect();
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  }
}

void networkHandle(SensorData &data, ControlState &state) {
  gData = &data;
  gState = &state;
  ensureWifiConnected();
  server.handleClient();
}
