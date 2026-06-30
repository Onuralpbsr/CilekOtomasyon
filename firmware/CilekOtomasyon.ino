#include "config.h"
#include "sensors.h"
#include "control.h"
#include "network.h"
#include "alerts.h"

static SensorData sensorData;
static ControlState controlState;

static unsigned long lastSensorReadMs = 0;
static const unsigned long SENSOR_READ_INTERVAL_MS = 2000;

static int lastResetDay = -1;

void setup() {
  Serial.begin(115200);

  sensorsInit();
  controlInit();
  alertsInit();
  networkInit();

  controlSetStage(controlState, STAGE_VEGETATIVE); // başlangıç evresi; /api/control ile değiştirilebilir
}

void loop() {
  unsigned long now = millis();

  if (now - lastSensorReadMs >= SENSOR_READ_INTERVAL_MS) {
    lastSensorReadMs = now;
    sensorsRead(sensorData);

    if (sensorData.valid && sensorData.day != lastResetDay) {
      if (lastResetDay != -1) {
        sensorsResetDailyCounters();
        controlState.irrigationsToday = 0;
      }
      lastResetDay = sensorData.day;
    }

    controlUpdate(sensorData, controlState);
  }

  alertsLoop();
  networkHandle(sensorData, controlState);
}
