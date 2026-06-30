#include "control.h"
#include "config.h"
#include "alerts.h"

static void setRelay(uint8_t pin, bool on) {
  digitalWrite(pin, (on != RELAY_ACTIVE_LOW) ? HIGH : LOW);
}

void controlInit() {
  pinMode(PIN_RELAY_PUMP, OUTPUT);
  pinMode(PIN_RELAY_FAN, OUTPUT);
  pinMode(PIN_RELAY_LIGHT, OUTPUT);
  pinMode(PIN_RELAY_CLIMATE, OUTPUT);
  setRelay(PIN_RELAY_PUMP, false);
  setRelay(PIN_RELAY_FAN, false);
  setRelay(PIN_RELAY_LIGHT, false);
  setRelay(PIN_RELAY_CLIMATE, false);
}

void controlSetStage(ControlState &state, GrowthStage stage) {
  state.stage = stage;
}

void controlClearFaults(ControlState &state) {
  state.pumpFault = false;
  state.waterLowFault = false;
}

static bool isNightIrrigationBlocked(uint8_t hour) {
  if (IRRIGATION_NIGHT_BLOCK_START_HOUR < IRRIGATION_NIGHT_BLOCK_END_HOUR) {
    return hour >= IRRIGATION_NIGHT_BLOCK_START_HOUR && hour < IRRIGATION_NIGHT_BLOCK_END_HOUR;
  }
  return hour >= IRRIGATION_NIGHT_BLOCK_START_HOUR || hour < IRRIGATION_NIGHT_BLOCK_END_HOUR;
}

static bool isLightHour(uint8_t hour, uint8_t minute) {
  uint16_t nowMin = hour * 60 + minute;
  uint16_t onMin = LIGHT_ON_HOUR * 60 + LIGHT_ON_MINUTE;
  uint16_t offMin = LIGHT_OFF_HOUR * 60 + LIGHT_OFF_MINUTE;
  if (onMin < offMin) return nowMin >= onMin && nowMin < offMin;
  return nowMin >= onMin || nowMin < offMin;
}

static void startIrrigation(const SensorData &d, ControlState &s) {
  s.irrigationInProgress = true;
  s.irrigationStartMs = millis();
  s.irrigationStartLitres = d.flowInLitresToday;
  setRelay(PIN_RELAY_PUMP, true);
  s.pumpOn = true;
}

static void stopIrrigation(ControlState &s) {
  setRelay(PIN_RELAY_PUMP, false);
  s.pumpOn = false;
  s.irrigationInProgress = false;
  s.irrigationsToday++;
}

static void updateIrrigation(const SensorData &d, ControlState &s) {
  if (s.waterLowFault || s.pumpFault) {
    if (s.irrigationInProgress) stopIrrigation(s);
    return;
  }

  if (s.irrigationInProgress) {
    float delivered = d.flowInLitresToday - s.irrigationStartLitres;
    bool volumeReached = delivered >= IRRIGATION_PULSE_LITRES;
    bool timedOut = (millis() - s.irrigationStartMs) > IRRIGATION_TIMEOUT_MS;
    if (timedOut && !volumeReached) {
      s.pumpFault = true;
      s.lastAlarm = "Pompa zaman asimi: hedef hacme ulasilamadi";
      alertsSendSMS(s.lastAlarm);
    }
    if (volumeReached || timedOut) stopIrrigation(s);
    return;
  }

  const StageThresholds &t = STAGE_PARAMS[s.stage];
  if (s.irrigationsToday >= t.maxIrrigationsPerDay) return;
  if (isNightIrrigationBlocked(d.hour)) return;
  if (isnan(d.soilMoisturePct) || d.soilMoisturePct >= t.soilMoistureMin) return;

  startIrrigation(d, s);
}

static void updatePumpSafety(const SensorData &d, ControlState &s) {
  if (!s.pumpOn) return;
  if (isnan(d.pumpCurrentA)) return;

  if (d.pumpCurrentA > PUMP_NOMINAL_CURRENT_A * PUMP_OVERCURRENT_RATIO) {
    s.pumpFault = true;
    s.lastAlarm = "Pompa asiri akim: tikanma/blokaj suphesi";
    alertsSendSMS(s.lastAlarm);
  } else if (d.pumpCurrentA < PUMP_NOMINAL_CURRENT_A * PUMP_UNDERCURRENT_RATIO) {
    s.pumpFault = true;
    s.lastAlarm = "Pompa dusuk akim: kuru calisma/hava suphesi";
    alertsSendSMS(s.lastAlarm);
  }
}

static void updateWaterLevel(const SensorData &d, ControlState &s) {
  if (!d.waterLevelOk && !s.waterLowFault) {
    s.waterLowFault = true;
    s.lastAlarm = "Rezervuar su seviyesi dusuk";
    alertsSendSMS(s.lastAlarm);
  } else if (d.waterLevelOk) {
    s.waterLowFault = false;
  }
}

static void updateClimate(const SensorData &d, ControlState &s) {
  bool daytime = isLightHour(d.hour, d.minute);
  float tempRef = !isnan(d.ambientTempBME) ? d.ambientTempBME : (d.tempSHT1 + d.tempSHT2) / 2.0f;
  float rhRef = !isnan(d.ambientHumBME) ? d.ambientHumBME : (d.humSHT1 + d.humSHT2) / 2.0f;

  float tempMax = daytime ? TEMP_DAY_MAX : TEMP_NIGHT_MAX;
  float tempMin = daytime ? TEMP_DAY_MIN : TEMP_NIGHT_MIN;

  if (!isnan(tempRef)) {
    if (tempRef > tempMax) s.fanOn = true;
    else if (tempRef < tempMin) s.fanOn = false;
  }
  setRelay(PIN_RELAY_FAN, s.fanOn);

  if (!isnan(rhRef)) {
    if (rhRef > RH_MAX) s.climateOn = false;       // nem fazlaysa nemlendirme kapat
    else if (rhRef < RH_MIN) s.climateOn = true;   // nem azsa aç
  }
  if (!isnan(d.rootZoneTemp)) {
    if (d.rootZoneTemp < ROOTZONE_TEMP_MIN) s.climateOn = true;
    if (d.rootZoneTemp > ROOTZONE_TEMP_MAX) s.climateOn = false;
  }
  setRelay(PIN_RELAY_CLIMATE, s.climateOn);

  s.lightOn = daytime;
  setRelay(PIN_RELAY_LIGHT, s.lightOn);
}

void controlManualOverride(ControlState &s, const String &relay, bool on) {
  if (relay == "pump") { setRelay(PIN_RELAY_PUMP, on); s.pumpOn = on; }
  else if (relay == "fan") { setRelay(PIN_RELAY_FAN, on); s.fanOn = on; }
  else if (relay == "light") { setRelay(PIN_RELAY_LIGHT, on); s.lightOn = on; }
  else if (relay == "climate") { setRelay(PIN_RELAY_CLIMATE, on); s.climateOn = on; }
}

void controlUpdate(const SensorData &d, ControlState &s) {
  if (!d.valid) return;
  updateWaterLevel(d, s);
  updatePumpSafety(d, s);
  updateIrrigation(d, s);
  updateClimate(d, s);
}
