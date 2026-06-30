#pragma once

// ---------- WiFi ----------
#define WIFI_SSID        "EVIN_WIFI_ADI"
#define WIFI_PASSWORD    "EVIN_WIFI_SIFRESI"

// Mac'teki web sunucusu bu adresten /api/status okur, /api/control'e komut atar.
// Burada sadece ESP32'nin kendi HTTP sunucusunun port'u tanımlı.
#define HTTP_SERVER_PORT 80

// ---------- I2C (BME280, SHT30 x2, TSL2561, INA219, DS3231, SSD1306) ----------
#define PIN_I2C_SDA      8
#define PIN_I2C_SCL      9

// SHT30 iki adet aynı bus'ta farklı adreste: 0x44 (ADDR pin GND) ve 0x45 (ADDR pin VCC)
#define SHT30_ADDR_1     0x44
#define SHT30_ADDR_2     0x45

// OLED SSD1306, aynı I2C bus'ta. Çoğu 128x64 modülde adres 0x3C'dir (bazı 128x32 varyantları 0x3D olabilir).
#define SCREEN_WIDTH     128
#define SCREEN_HEIGHT    64
#define SSD1306_I2C_ADDR 0x3C

// ---------- OneWire (DS18B20 x2, aynı hat) ----------
#define PIN_ONEWIRE      4

// ---------- Akış sensörleri (interrupt) ----------
#define PIN_FLOW_IN      5   // giriş (sulama hacmi)
#define PIN_FLOW_DRAIN   6   // drenaj (runoff)
#define FLOW_PULSES_PER_LITRE 450.0f  // YF-S201 tipik kalibrasyon, gerçek değerle kalibre et

// ---------- Toprak nem sensörü (analog) ----------
#define PIN_SOIL_MOISTURE   1   // ADC1_CH0
#define SOIL_RAW_DRY        2800  // havada kuru okuma (kalibre edilmeli)
#define SOIL_RAW_WET        1200  // suda ıslak okuma (kalibre edilmeli)

// ---------- Şamandıra (su seviyesi) ----------
#define PIN_FLOAT_SWITCH    2   // LOW = su yok (NC şamandıra varsayımı, kablolamaya göre değiştir)

// ---------- Röle kartı (4 kanal, aktif LOW çoğu modülde) ----------
#define PIN_RELAY_PUMP      15
#define PIN_RELAY_FAN       16
#define PIN_RELAY_LIGHT     17
#define PIN_RELAY_CLIMATE   18  // ısıtıcı/nemlendirici
#define RELAY_ACTIVE_LOW    true

// ---------- SIM800L (UART1) ----------
#define PIN_SIM800_RX       11  // ESP32 RX <- SIM800L TX
#define PIN_SIM800_TX       12  // ESP32 TX -> SIM800L RX
#define SIM800_BAUD         9600
#define ALERT_PHONE_NUMBER  "+90XXXXXXXXXX"

// ---------- PZEM-004T (UART2, Modbus RTU) ----------
#define PIN_PZEM_RX         13
#define PIN_PZEM_TX         14

// ---------- DS18B20 indeksleri (aynı bus'ta sırayla bulunur) ----------
#define DS18B20_NUTRIENT_INDEX  0  // besin çözeltisi sıcaklığı
#define DS18B20_ROOTZONE_INDEX  1  // kök bölgesi sıcaklığı

// ================= AGRONOMİK EŞİKLER =================
// Evre: 0=Fide, 1=Vegetatif, 2=Çiçek, 3=Meyve
struct StageThresholds {
  float soilMoistureMin;     // % - bunun altına düşünce sula
  float soilMoistureMax;     // % - hedef üst sınır
  float targetRunoffPct;     // drenaj/giriş oranı hedefi
  uint16_t maxIrrigationsPerDay;
  float pulseVolumeLitres;   // tek sulama darbesinde verilecek hacim (bitki/kök hacmiyle birlikte büyür)
};

static const StageThresholds STAGE_PARAMS[4] = {
  // soilMin, soilMax, runoff%, maxSulamaSayisi, pulseLitre
  {70.0f, 75.0f, 12.5f, 3,  0.15f},   // Fide
  {65.0f, 70.0f, 17.5f, 6,  0.25f},   // Vegetatif
  {60.0f, 65.0f, 22.5f, 8,  0.35f},   // Çiçek
  {55.0f, 65.0f, 27.5f, 12, 0.45f},   // Meyve
};

// İklim hedefleri (gündüz/gece ortak; basit tutuldu, evreye göre ince ayar koddan yapılabilir)
#define TEMP_DAY_MAX        24.0f
#define TEMP_DAY_MIN        20.0f
#define TEMP_NIGHT_MAX      18.0f
#define TEMP_NIGHT_MIN      13.0f
#define RH_MAX               75.0f
#define RH_MIN               60.0f
#define ROOTZONE_TEMP_MAX    22.0f
#define ROOTZONE_TEMP_MIN    18.0f

// Fotoperiyot (gün-nötr çeşit varsayımı)
#define LIGHT_ON_HOUR        6
#define LIGHT_ON_MINUTE      0
#define LIGHT_OFF_HOUR       22
#define LIGHT_OFF_MINUTE     0

// Sulama gece yasak penceresi (kök çürümesi riskini azaltmak için)
#define IRRIGATION_NIGHT_BLOCK_START_HOUR  22
#define IRRIGATION_NIGHT_BLOCK_END_HOUR    7

// Pompa güvenliği (INA219 ile)
#define PUMP_NOMINAL_CURRENT_A   0.8f
#define PUMP_OVERCURRENT_RATIO   1.5f   // bu kat üstü = tıkanma/blokaj
#define PUMP_UNDERCURRENT_RATIO  0.3f   // bu kat altı = kuru çalışma/hava

// Tek sulama darbesinin zaman aşımı (hacim STAGE_PARAMS.pulseVolumeLitres'ten gelir)
#define IRRIGATION_TIMEOUT_MS     60000UL
