#include "sensors.h"
#include "config.h"

#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#include <Adafruit_SHT31.h>
#include <Adafruit_TSL2561_U.h>
#include <Adafruit_INA219.h>
#include <RTClib.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <PZEM004Tv30.h>

static Adafruit_BME280 bme;
static Adafruit_SHT31 sht1 = Adafruit_SHT31();
static Adafruit_SHT31 sht2 = Adafruit_SHT31();
static Adafruit_TSL2561_Unified tsl(TSL2561_ADDR_FLOAT, 12345);
static Adafruit_INA219 ina219;
static RTC_DS3231 rtc;
static OneWire oneWire(PIN_ONEWIRE);
static DallasTemperature ds18b20(&oneWire);
static PZEM004Tv30 pzem(Serial2, PIN_PZEM_RX, PIN_PZEM_TX);

static volatile uint32_t flowInPulses = 0;
static volatile uint32_t flowDrainPulses = 0;

static void IRAM_ATTR onFlowInPulse() { flowInPulses++; }
static void IRAM_ATTR onFlowDrainPulse() { flowDrainPulses++; }

static bool bmeOk = false, sht1Ok = false, sht2Ok = false, tslOk = false, inaOk = false, rtcOk = false;

void sensorsInit() {
  Wire.begin(PIN_I2C_SDA, PIN_I2C_SCL);

  bmeOk = bme.begin(0x76) || bme.begin(0x77);
  sht1Ok = sht1.begin(SHT30_ADDR_1);
  sht2Ok = sht2.begin(SHT30_ADDR_2);
  tslOk = tsl.begin();
  if (tslOk) {
    tsl.enableAutoRange(true);
    tsl.setIntegrationTime(TSL2561_INTEGRATIONTIME_402MS);
  }
  inaOk = ina219.begin();
  rtcOk = rtc.begin();

  ds18b20.begin();

  pinMode(PIN_SOIL_MOISTURE, INPUT);
  pinMode(PIN_FLOAT_SWITCH, INPUT_PULLUP);

  pinMode(PIN_FLOW_IN, INPUT_PULLUP);
  pinMode(PIN_FLOW_DRAIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(PIN_FLOW_IN), onFlowInPulse, FALLING);
  attachInterrupt(digitalPinToInterrupt(PIN_FLOW_DRAIN), onFlowDrainPulse, FALLING);
}

void sensorsResetDailyCounters() {
  noInterrupts();
  flowInPulses = 0;
  flowDrainPulses = 0;
  interrupts();
}

void sensorsRead(SensorData &d) {
  if (bmeOk) {
    d.ambientTempBME = bme.readTemperature();
    d.ambientHumBME = bme.readHumidity();
    d.ambientPressure = bme.readPressure() / 100.0f;
  }

  if (sht1Ok) {
    d.tempSHT1 = sht1.readTemperature();
    d.humSHT1 = sht1.readHumidity();
  }
  if (sht2Ok) {
    d.tempSHT2 = sht2.readTemperature();
    d.humSHT2 = sht2.readHumidity();
  }

  if (tslOk) {
    sensors_event_t event;
    tsl.getEvent(&event);
    d.lux = event.light ? (uint32_t)event.light : 0;
  }

  if (inaOk) {
    d.pumpVoltage = ina219.getBusVoltage_V();
    d.pumpCurrentA = ina219.getCurrent_mA() / 1000.0f;
    d.pumpPowerW = ina219.getPower_mW() / 1000.0f;
  }

  d.acVoltage = pzem.voltage();
  d.acCurrentA = pzem.current();
  d.acPowerW = pzem.power();
  d.acEnergyKWh = pzem.energy();
  d.acFrequency = pzem.frequency();
  d.acPowerFactor = pzem.pf();

  if (rtcOk) {
    DateTime now = rtc.now();
    d.hour = now.hour();
    d.minute = now.minute();
    d.second = now.second();
    d.day = now.day();
    d.month = now.month();
    d.year = now.year();
  }

  ds18b20.requestTemperatures();
  d.nutrientTemp = ds18b20.getTempCByIndex(DS18B20_NUTRIENT_INDEX);
  d.rootZoneTemp = ds18b20.getTempCByIndex(DS18B20_ROOTZONE_INDEX);

  int soilRaw = analogRead(PIN_SOIL_MOISTURE);
  float pct = 100.0f * (float)(SOIL_RAW_DRY - soilRaw) / (float)(SOIL_RAW_DRY - SOIL_RAW_WET);
  d.soilMoisturePct = constrain(pct, 0.0f, 100.0f);

  noInterrupts();
  uint32_t inPulses = flowInPulses;
  uint32_t drainPulses = flowDrainPulses;
  interrupts();
  d.flowInLitresToday = inPulses / FLOW_PULSES_PER_LITRE;
  d.flowDrainLitresToday = drainPulses / FLOW_PULSES_PER_LITRE;

  // Şamandıra NC varsayımı: su varken kontak kapalı (LOW), su bitince açık (HIGH)
  d.waterLevelOk = (digitalRead(PIN_FLOAT_SWITCH) == LOW);

  d.valid = true;
}
