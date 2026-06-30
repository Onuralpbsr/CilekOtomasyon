#pragma once
#include "sensors.h"
#include "control.h"

void displayInit();
void displayUpdate(const SensorData &data, const ControlState &state);
