#pragma once
#include <Arduino.h>
#include "sensors.h"

enum GrowthStage { STAGE_SEEDLING = 0, STAGE_VEGETATIVE = 1, STAGE_FLOWER = 2, STAGE_FRUIT = 3 };

struct ControlState {
  GrowthStage stage = STAGE_VEGETATIVE;

  bool pumpOn = false;
  bool lightOn = false;

  // Fiziksel fan/ısıtıcı/nemlendirici yok. Bu iki röle kanalı şu an
  // boşta/manuel; ileride bir cihaz takılırsa controlManualOverride ile
  // kullanılabilir ama otomatik mantık tarafından sürülmüyor.
  bool spareRelay2On = false;
  bool spareRelay3On = false;

  uint16_t irrigationsToday = 0;
  bool irrigationInProgress = false;
  unsigned long irrigationStartMs = 0;
  float irrigationStartLitres = 0;

  bool pumpFault = false;       // aşırı/düşük akım kaynaklı kilit
  bool waterLowFault = false;   // şamandıra düşük seviye kilidi

  float runoffPctToday = NAN;   // bugünkü gerçek drenaj/giriş oranı (%), STAGE_PARAMS.targetRunoffPct ile karşılaştırılır

  // İklim için aktüatör olmadığından otomatik düzeltme yapılmaz; sadece
  // hedef aralığın dışına çıkınca SMS ile bildirilir.
  bool climateAlertActive = false;
  float vpdKPa = NAN;            // buhar basıncı açığı (kPa) - sıcaklık+nemden hesaplanan bilgilendirici metrik

  String lastAlarm = "";
};

void controlInit();
void controlUpdate(const SensorData &data, ControlState &state);
void controlSetStage(ControlState &state, GrowthStage stage);
void controlManualOverride(ControlState &state, const String &relay, bool on);
void controlClearFaults(ControlState &state);
