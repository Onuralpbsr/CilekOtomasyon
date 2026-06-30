#pragma once
#include "sensors.h"
#include "control.h"

void networkInit();
void networkHandle(SensorData &data, ControlState &state);
