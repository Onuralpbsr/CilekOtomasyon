#!/usr/bin/env bash
# Çilek Otomasyon - tek komutla kurulum (macOS)
# Kullanım: git clone sonrası repo kökünde -> bash setup.sh

set -euo pipefail

REPO_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
SKETCHBOOK_DIR="$HOME/Documents/Arduino"
SKETCH_LINK="$SKETCHBOOK_DIR/CilekOtomasyon"
SKETCH_SRC="$REPO_DIR/firmware/CilekOtomasyon"

ESP32_BOARD_URL="https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json"

ARDUINO_LIBS=(
  "Adafruit BME280 Library"
  "Adafruit SHT31 Library"
  "Adafruit TSL2561"
  "Adafruit INA219"
  "RTClib"
  "OneWire"
  "DallasTemperature"
  "PZEM004Tv30"
  "ArduinoJson"
  "Adafruit GFX Library"
  "Adafruit SSD1306"
  "Adafruit BusIO"
  "Adafruit Unified Sensor"
)

step() { echo; echo "==> $1"; }

step "Homebrew kontrolü"
if ! command -v brew >/dev/null 2>&1; then
  /bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"
  eval "$(/opt/homebrew/bin/brew shellenv)"
fi

step "Temel araçlar (node, gh, arduino-cli)"
brew install node gh arduino-cli >/dev/null

step "Arduino IDE (GUI, isteğe bağlı düzenleme için)"
brew install --cask arduino-ide >/dev/null 2>&1 || echo "Arduino IDE zaten kurulu veya cask bulunamadı, atlanıyor."

step "arduino-cli yapılandırması"
arduino-cli config init --overwrite >/dev/null 2>&1 || true
arduino-cli config set board_manager.additional_urls "$ESP32_BOARD_URL" >/dev/null
arduino-cli config set directories.user "$SKETCHBOOK_DIR" >/dev/null

step "ESP32 board paketi kuruluyor (ilk kurulumda birkaç dakika sürebilir)"
arduino-cli core update-index >/dev/null
arduino-cli core install esp32:esp32 >/dev/null

step "Arduino kütüphaneleri varsayılan klasöre kuruluyor ($SKETCHBOOK_DIR/libraries)"
arduino-cli lib update-index >/dev/null
for lib in "${ARDUINO_LIBS[@]}"; do
  echo "  - $lib"
  arduino-cli lib install "$lib" >/dev/null 2>&1 || echo "    ! '$lib' kurulamadı, manuel kontrol gerekebilir."
done

step "Sketch'i sketchbook'a bağlama (symlink)"
mkdir -p "$SKETCHBOOK_DIR"
ln -sfn "$SKETCH_SRC" "$SKETCH_LINK"
echo "  $SKETCH_LINK -> $SKETCH_SRC"

step "Web paneli bağımlılıkları kuruluyor"
( cd "$REPO_DIR/web" && npm install )

step "Derleme testi (ESP32-S3, sadece sözdizimi/kütüphane kontrolü)"
arduino-cli compile --fqbn esp32:esp32:esp32s3 "$SKETCH_SRC" || \
  echo "  ! Derleme şu an başarısız olabilir, genelde firmware/CilekOtomasyon/config.h içindeki WiFi/telefon bilgilerini doldurman gerekir. Aşağıdaki adıma bak."

cat <<'EOF'

==================== KURULUM TAMAMLANDI ====================

Otomatik olarak hazır olan her şey:
  - Homebrew, Node.js, GitHub CLI, Arduino CLI, Arduino IDE
  - ESP32 board paketi
  - Tüm gerekli Arduino kütüphaneleri (~/Documents/Arduino/libraries)
  - Sketch, Arduino IDE'nin sketchbook menüsünde "CilekOtomasyon" olarak görünür
  - Web paneli bağımlılıkları (web/node_modules)

Senin doldurman gereken (güvenlik nedeniyle repoda tutulmuyor):
  1. firmware/CilekOtomasyon/config.h
       -> WIFI_SSID, WIFI_PASSWORD, ALERT_PHONE_NUMBER
  2. web/config.js
       -> ESP32_BASE_URL (ESP32'nin yerel ağdaki IP'si)

Web panelini başlatmak için:
  cd web && npm run dev

==============================================================
EOF
