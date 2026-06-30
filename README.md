# Çilek Otomasyon

Topraksız tarım (coco coir + damlama) çilek yetiştiriciliği için ESP32-S3 tabanlı otomasyon sistemi.

## Yapı

- `firmware/CilekOtomasyon/` — ESP32-S3 N16R8 Arduino sketch'i. Sensör okuma, sulama/iklim kontrol mantığı, HTTP API, SIM800L SMS alarmı.
- `web/` — Mac üzerinde yerel çalışan Node.js/Express paneli. ESP32'nin `/api/status` uç noktasını periyodik çekip SQLite'a (node:sqlite) geçmiş kaydeder, canlı dashboard sunar.
- `setup.sh` — clone sonrası tek komutla tüm kurulumu yapan betik (macOS).

## Malzemeler

- ESP32-S3 N16R8
- OLED SSD1306 (I2C) — *donanım listede, firmware'de henüz sürülmüyor*
- BME280
- SHT30 (1. adet)
- SHT30 (2. adet)
- TSL2561
- INA219
- PZEM-004T-100A V3.0
- DS3231 RTC
- DS18B20 (1. adet, besin çözeltisi sıcaklığı)
- DS18B20 (2. adet, kök bölgesi sıcaklığı)
- Kapasitif toprak nem sensörü
- YF-S201 debi sensörü (giriş)
- YF-S201 debi sensörü (drenaj)
- SIM800L
- 4 kanal röle kartı
- 12V 17W DC su pompası
- XPC-Y25-T12V V1.0 şamandıra (su seviye sensörü)
- LM2596 DC-DC dönüştürücü (5V hattı için)
- LM2596 DC-DC dönüştürücü (SIM800L için 4.0V)
- 12V 20.8A güç kaynağı
- 1000µF 16V kondansatör (SIM800L beslemesi için)
- 4.7kΩ direnç (DS18B20 pull-up)

## Kurulum (tek komut)

```
git clone https://github.com/Onuralpbsr/CilekOtomasyon.git
cd CilekOtomasyon
bash setup.sh
```

Bu betik otomatik olarak:
- Homebrew, Node.js, GitHub CLI, Arduino CLI, Arduino IDE'yi kurar
- ESP32 board paketini kurar
- Gerekli tüm Arduino kütüphanelerini varsayılan `~/Documents/Arduino/libraries` klasörüne kurar
- Sketch'i `~/Documents/Arduino/CilekOtomasyon` olarak sketchbook'a bağlar (symlink), Arduino IDE'de doğrudan görünür
- Web paneli bağımlılıklarını (`web/node_modules`) kurar
- Bir derleme testi çalıştırır

### Senin elle doldurman gerekenler

Güvenlik nedeniyle WiFi şifresi ve telefon numarası repoda tutulmuyor, `setup.sh` bunları dolduramaz:

1. `firmware/CilekOtomasyon/config.h` → `WIFI_SSID`, `WIFI_PASSWORD`, `ALERT_PHONE_NUMBER`
2. `web/config.js` → `ESP32_BASE_URL` (ESP32'nin yerel ağdaki IP'si)

Pin numaralarını da kendi ESP32-S3 N16R8 board'unun gerçek GPIO çıkışlarıyla karşılaştır.

## Web Panelini Çalıştırma

```
cd web
npm run dev
```

Panel `http://localhost:3000` adresinde çalışır.
