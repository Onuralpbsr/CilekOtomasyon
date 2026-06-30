#pragma once
#include <Arduino.h>
#include "sensors.h"

enum GrowthStage { STAGE_SEEDLING = 0, STAGE_VEGETATIVE = 1, STAGE_FLOWER = 2, STAGE_FRUIT = 3 };

struct ControlState {
  GrowthStage stage = STAGE_VEGETATIVE;

  bool pumpOn = false;
  bool fanOn = false;
  bool lightOn = false;
  bool climateOn = false;

  uint16_t irrigationsToday = 0;
  bool irrigationInProgress = false;
  unsigned long irrigationStartMs = 0;
  float irrigationStartLitres = 0;

  bool pumpFault = false;       // aşırı/düşük akım kaynaklı kilit
  bool waterLowFault = false;   // şamandıra düşük seviye kilidi

  String lastAlarm = "";
};

void controlInit();
void controlUpdate(const SensorData &data, ControlState &state);
void controlSetStage(ControlState &state, GrowthStage stage);
void controlManualOverride(ControlState &state, const String &relay, bool on);
void controlClearFaults(ControlState &state);
