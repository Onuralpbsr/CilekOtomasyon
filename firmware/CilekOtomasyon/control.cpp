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

// Pompayı nasıl başlatılmış olursa olsun (otomatik sulama veya manuel röle
// kontrolü) anında durdurur. irrigationsToday'i artırmaz; tamamlanmış bir
// sulamayı değil, güvenlik kesintisini temsil eder.
static void forceStopPump(ControlState &s) {
  setRelay(PIN_RELAY_PUMP, false);
  s.pumpOn = false;
  s.irrigationInProgress = false;
}

static void stopIrrigation(ControlState &s) {
  forceStopPump(s);
  s.irrigationsToday++;
}

static void updateIrrigation(const SensorData &d, ControlState &s) {
  if (s.waterLowFault || s.pumpFault) {
    // Pompa manuel olarak (irrigationInProgress=false) açılmış olsa bile
    // arıza varken çalışmaya devam etmemeli.
    if (s.pumpOn) forceStopPump(s);
    return;
  }

  const StageThresholds &activeStage = STAGE_PARAMS[s.stage];

  if (s.irrigationInProgress) {
    float delivered = d.flowInLitresToday - s.irrigationStartLitres;
    bool volumeReached = delivered >= activeStage.pulseVolumeLitres;
    bool timedOut = (millis() - s.irrigationStartMs) > IRRIGATION_TIMEOUT_MS;
    if (timedOut && !volumeReached) {
      s.pumpFault = true;
      s.lastAlarm = "Pompa zaman asimi: hedef hacme ulasilamadi";
      alertsSendSMS(s.lastAlarm);
    }
    if (volumeReached || timedOut) stopIrrigation(s);
    return;
  }

  if (s.irrigationsToday >= activeStage.maxIrrigationsPerDay) return;
  if (isNightIrrigationBlocked(d.hour)) return;
  if (isnan(d.soilMoisturePct) || d.soilMoisturePct >= activeStage.soilMoistureMin) return;

  startIrrigation(d, s);
}

// Bugünkü gerçek drenaj/giriş oranını hesaplar, evrenin hedef drenaj oranıyla
// karşılaştırmak için (su/besin israfı veya yetersiz drenaj erken görülsün diye).
static void updateRunoff(const SensorData &d, ControlState &s) {
  const float MIN_INPUT_FOR_RATIO_LITRES = 0.5f; // gün başında gürültülü oranları önlemek için
  if (d.flowInLitresToday < MIN_INPUT_FOR_RATIO_LITRES) {
    s.runoffPctToday = NAN;
    return;
  }
  s.runoffPctToday = 100.0f * d.flowDrainLitresToday / d.flowInLitresToday;
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

// Buhar basıncı açığı (VPD, kPa) - sıcaklık ve bağıl nemden hesaplanan,
// bitkinin terlemesini tek sayıda özetleyen bilgilendirici metrik.
// Sadece izleme amaçlı; otomatik bir aksiyona bağlı değil.
static float calcVpdKPa(float tempC, float rhPct) {
  if (isnan(tempC) || isnan(rhPct)) return NAN;
  float svpKPa = 0.6108f * expf((17.27f * tempC) / (tempC + 237.3f));
  return svpKPa * (1.0f - rhPct / 100.0f);
}

// Fiziksel fan/ısıtıcı/nemlendirici YOK, dolayısıyla iklim için otomatik
// düzeltme yapılamaz. Bu fonksiyon sadece izler: sıcaklık/nem/kök bölgesi
// sıcaklığı hedef aralığın dışına çıkarsa bir kere SMS gönderir, aralığa
// dönünce otomatik temizlenir (pompa arızaları gibi manuel temizlik
// gerektirmez, çünkü kilitlenen bir donanım yok).
static void updateClimateMonitoring(const SensorData &d, ControlState &s) {
  bool daytime = isLightHour(d.hour, d.minute);
  float tempRef = bestAmbientTemp(d);
  float rhRef = bestAmbientHum(d);

  float tempMax = daytime ? TEMP_DAY_MAX : TEMP_NIGHT_MAX;
  float tempMin = daytime ? TEMP_DAY_MIN : TEMP_NIGHT_MIN;

  s.vpdKPa = calcVpdKPa(tempRef, rhRef);

  bool tempBad = !isnan(tempRef) && (tempRef > tempMax || tempRef < tempMin);
  bool rhBad = !isnan(rhRef) && (rhRef > RH_MAX || rhRef < RH_MIN);
  bool rootBad = !isnan(d.rootZoneTemp) && (d.rootZoneTemp > ROOTZONE_TEMP_MAX || d.rootZoneTemp < ROOTZONE_TEMP_MIN);
  bool alertNow = tempBad || rhBad || rootBad;

  if (alertNow && !s.climateAlertActive) {
    s.climateAlertActive = true;
    String msg = "Iklim uyarisi (fan/isitici yok, manuel mudahale gerekebilir):";
    if (tempBad) msg += " sicaklik " + String(tempRef, 1) + "C;";
    if (rhBad) msg += " nem " + String(rhRef, 1) + "%;";
    if (rootBad) msg += " kok bolgesi " + String(d.rootZoneTemp, 1) + "C;";
    s.lastAlarm = msg;
    alertsSendSMS(msg);
  } else if (!alertNow) {
    s.climateAlertActive = false;
  }
}

void controlManualOverride(ControlState &s, const String &relay, bool on) {
  if (relay == "pump") {
    if (on && (s.pumpFault || s.waterLowFault)) return; // aktif arızada manuel açmaya izin verme
    setRelay(PIN_RELAY_PUMP, on);
    s.pumpOn = on;
  }
  // relay1/relay2/relay3: şu an fiziksel cihaz bağlı değil (ışık, fan,
  // sisleme/nemlendirme - hiçbiri satın alınmadı). Röleyi yine de
  // anahtarlıyoruz ki ileride bir cihaz takıldığında ya da test amaçlı
  // kullanmak istendiğinde panelden erişilebilsin.
  else if (relay == "relay1") { setRelay(PIN_RELAY_LIGHT, on); s.spareRelay1On = on; }
  else if (relay == "relay2") { setRelay(PIN_RELAY_FAN, on); s.spareRelay2On = on; }
  else if (relay == "relay3") { setRelay(PIN_RELAY_CLIMATE, on); s.spareRelay3On = on; }
}

void controlUpdate(const SensorData &d, ControlState &s) {
  if (!d.valid) return;
  updateWaterLevel(d, s);
  updatePumpSafety(d, s);
  updateIrrigation(d, s);
  updateClimateMonitoring(d, s);
  updateRunoff(d, s);
}
