# Çilek Otomasyon

Topraksız tarım (coco coir + damlama) çilek yetiştiriciliği için ESP32-S3 tabanlı otomasyon sistemi.

## Yapı

- `firmware/CilekOtomasyon/` — ESP32-S3 N16R8 Arduino sketch'i. Sensör okuma, sulama/iklim kontrol mantığı, HTTP API, SIM800L SMS alarmı.
- `web/` — Mac üzerinde yerel çalışan Node.js/Express paneli. ESP32'nin `/api/status` uç noktasını periyodik çekip SQLite'a (node:sqlite) geçmiş kaydeder, canlı dashboard sunar.
- `setup.sh` — clone sonrası tek komutla tüm kurulumu yapan betik (macOS).

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
