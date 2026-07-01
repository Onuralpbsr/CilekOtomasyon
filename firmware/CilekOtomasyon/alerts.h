#pragma once
#include <Arduino.h>

void alertsInit();
void alertsSendSMS(const String &message);
void alertsLoop(); // SIM800L AT yanıtlarını boşaltmak için periyodik çağrılmalı
bool alertsGsmOk(); // açılışta "AT" komutuna modem "OK" ile cevap verdi mi
