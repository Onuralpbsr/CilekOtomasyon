#pragma once
#include <Arduino.h>

struct SensorData {
  // BME280
  float ambientTempBME = NAN;
  float ambientHumBME = NAN;
  float ambientPressure = NAN;

  // SHT30 x2
  float tempSHT1 = NAN, humSHT1 = NAN;
  float tempSHT2 = NAN, humSHT2 = NAN;

  // TSL2561
  uint32_t lux = 0;

  // INA219 (pompa hattı)
  float pumpVoltage = NAN;
  float pumpCurrentA = NAN;
  float pumpPowerW = NAN;

  // PZEM-004T (ana AC besleme)
  float acVoltage = NAN;
  float acCurrentA = NAN;
  float acPowerW = NAN;
  float acEnergyKWh = NAN;
  float acFrequency = NAN;
  float acPowerFactor = NAN;

  // DS3231
  uint8_t hour = 0, minute = 0, second = 0;
  uint8_t day = 0, month = 0;
  uint16_t year = 0;

  // DS18B20 x2
  float nutrientTemp = NAN;
  float rootZoneTemp = NAN;

  // Toprak nemi (%)
  float soilMoisturePct = NAN;

  // Akış sensörleri (litre, kümülatif gün içi)
  float flowInLitresToday = 0;
  float flowDrainLitresToday = 0;

  // Şamandıra
  bool waterLevelOk = true;

  // Donanım sağlığı. I2C/RTC/DS18B20 açılışta bir kez tespit edilir;
  // PZEM her okumada dinamik kontrol edilir (iletişim o an kesilmiş olabilir).
  bool bmeOk = false;
  bool sht1Ok = false;
  bool sht2Ok = false;
  bool tslOk = false;
  bool inaOk = false;
  bool rtcOk = false;
  bool ds18b20Ok = false;
  bool pzemOk = false;

  bool valid = false;
};

void sensorsInit();
void sensorsRead(SensorData &data);
void sensorsResetDailyCounters();

// BME280 önceliklidir; BME280 yoksa geçerli olan SHT30'ların ortalaması,
// sadece biri geçerliyse o değer döner. İkisi de yoksa NAN.
float bestAmbientTemp(const SensorData &data);
float bestAmbientHum(const SensorData &data);
