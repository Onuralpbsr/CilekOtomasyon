# Çilek Otomasyon

Topraksız tarım (coco coir + damlama) çilek yetiştiriciliği için ESP32-S3 tabanlı otomasyon sistemi.

## Yapı

- `firmware/` — ESP32-S3 N16R8 Arduino sketch'i. Sensör okuma, sulama/iklim kontrol mantığı, HTTP API, SIM800L SMS alarmı.
- `web/` — Mac üzerinde yerel çalışan Node.js/Express paneli. ESP32'nin `/api/status` uç noktasını periyodik çekip SQLite'a (node:sqlite) geçmiş kaydeder, canlı dashboard sunar.

## Kurulum

### Firmware
1. `firmware/` klasörünü Arduino IDE'de aç (File > Open).
2. Gerekli kütüphaneleri kur: Adafruit BME280, Adafruit SHT31, Adafruit TSL2561, Adafruit INA219, RTClib, OneWire, DallasTemperature, PZEM004Tv30, ArduinoJson.
3. `config.h` içinde WiFi bilgilerini ve SMS alarm numarasını doldur, pin numaralarını kendi board'unla doğrula.
4. ESP32-S3'e yükle.

### Web Paneli
```
cd web
npm install
npm run dev
```
`config.js` içinde `ESP32_BASE_URL`'i ESP32'nin yerel ağdaki IP adresiyle güncelle. Panel `http://localhost:3000` adresinde çalışır.
