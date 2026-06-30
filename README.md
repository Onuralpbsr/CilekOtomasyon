# Çilek Otomasyon

Topraksız tarım (coco coir substrat + damlama sulama) yöntemiyle çilek yetiştiriciliği için ESP32-S3 tabanlı, tam otomatik sera/yetiştirme otomasyon sistemi. Sistem sıcaklık/nem/ışık/toprak nemi/elektrik tüketimi gibi tüm büyüme parametrelerini sürekli izler, sulamayı ve iklimlendirmeyi büyüme evresine göre otomatik yönetir, arıza durumunda SMS ile uyarır ve Mac üzerinde çalışan bir web panelinden canlı/geçmiş veriyi takip etmeyi ve manuel müdahaleyi mümkün kılar.

## Proje Hakkında

- **Yetiştirme yöntemi:** Coco coir substratlı, damlama sulamalı topraksız tarım. Gün-nötr çeşit varsayımıyla sabit fotoperiyot uygulanır.
- **Otomasyonun amacı:** Sulama zamanlamasını ve hacmini substrat nemine ve büyüme evresine göre otomatik ayarlamak, sıcaklık/nemi hedef aralıkta tutmak, pompa arızalarını (tıkanma, kuru çalışma) ve su seviyesi düşüklüğünü erken tespit edip telefon başında olmasan da SMS ile haber vermek.
- **Veri toplama:** ESP32 üzerindeki tüm sensörler ~2 saniyede bir okunur; Mac'teki web sunucusu ESP32'nin `/api/status` uç noktasını 5 saniyede bir çekip SQLite'a kaydeder, böylece geçmişe dönük grafiklerle (24 saatlik) trend takibi yapılabilir.
- **Uzaktan kontrol:** Web panelinden röleler (pompa/fan/ışık/ısıtıcı-nemlendirici) manuel olarak açılıp kapatılabilir, arızalar temizlenebilir.

## Sistem Mimarisi

```
[Sensörler] -> [ESP32-S3 firmware] -> WiFi/HTTP -> [Mac: Node.js web sunucusu] -> [SQLite] -> [Tarayıcı dashboard]
                      |
                      v
                 [Röleler: pompa/fan/ışık/iklim]
                      |
                      v
                 [SIM800L: arıza SMS'i]
```

- `firmware/CilekOtomasyon/` — ESP32-S3 N16R8 Arduino sketch'i:
  - `sensors.h/.cpp` — tüm sensörlerin okunması (I2C, OneWire, UART, analog, interrupt tabanlı debi sayacı)
  - `control.h/.cpp` — sulama, iklim, pompa güvenliği ve röle mantığı
  - `httpapi.h/.cpp` — `/api/status` ve `/api/control` HTTP uç noktaları (`network.h/.cpp` olarak başladı, ESP32 çekirdeğinin kendi `Network.h`'ıyla macOS'ta case-insensitive dosya sistemi yüzünden çakıştığı için `httpapi` olarak yeniden adlandırıldı)
  - `alerts.h/.cpp` — SIM800L üzerinden SMS gönderimi
- `web/` — Mac üzerinde yerel çalışan Node.js/Express paneli:
  - `server.js` — ESP32'yi periyodik polluyor, `/api/live`, `/api/history`, `/api/control` uç noktalarını sunuyor
  - `db.js` — `node:sqlite` ile geçmiş veri kaydı (`web/data/history.db`, repoya dahil değil)
  - `public/` — vanilla JS + Chart.js ile canlı kart ve grafik arayüzü
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

## Pin Haritası (`config.h`)

| Sinyal | Pin | Not |
|---|---|---|
| I2C SDA / SCL | 8 / 9 | BME280, SHT30 x2, TSL2561, INA219, DS3231, (SSD1306) ortak hat |
| OneWire (DS18B20 x2) | 4 | indeks 0 = besin çözeltisi, indeks 1 = kök bölgesi |
| Debi - giriş | 5 | interrupt, FALLING |
| Debi - drenaj | 6 | interrupt, FALLING |
| Toprak nemi (analog) | 1 (ADC1_CH0) | kalibrasyon: kuru=2800, ıslak=1200 ham okuma |
| Şamandıra | 2 | INPUT_PULLUP, NC varsayımı (LOW = su var) |
| Röle - pompa | 15 | aktif LOW |
| Röle - fan | 16 | aktif LOW |
| Röle - ışık | 17 | aktif LOW |
| Röle - iklim (ısıtıcı/nemlendirici) | 18 | aktif LOW |
| SIM800L RX / TX | 11 / 12 | UART1, 9600 baud |
| PZEM-004T RX / TX | 13 / 14 | UART2, Modbus RTU |

SHT30 I2C adresleri: `0x44` (ADDR pin GND) ve `0x45` (ADDR pin VCC). Debi sensörü kalibrasyonu YF-S201 için 450 pals/litre varsayılan, gerçek değerle kalibre edilmeli.

## Büyüme Evreleri ve Sulama Mantığı

Evreye göre substrat nem eşiği, hedef drenaj oranı ve günlük maksimum sulama sayısı (`STAGE_PARAMS`, web panelinden `/api/control` ile `stage` 0-3 olarak değiştirilebilir):

| Evre | Nem alt sınırı (sulama tetik) | Nem üst hedef | Hedef drenaj oranı | Gün içi max sulama |
|---|---|---|---|---|
| 0 — Fide | %70 | %75 | %12.5 | 3 |
| 1 — Vegetatif | %65 | %70 | %17.5 | 6 |
| 2 — Çiçek | %60 | %65 | %22.5 | 8 |
| 3 — Meyve | %55 | %65 | %27.5 | 12 |

- Substrat nemi eşiğin altına düşünce ve gece yasak penceresi (22:00–07:00, kök çürümesi riskini azaltmak için) dışında ve günlük sulama limiti dolmamışsa pompa devreye girer.
- Tek sulama, 0.25 L hacme ulaşılınca veya 60 saniyelik zaman aşımına uğrayınca durur; zaman aşımına uğrarsa pompa arızası (`pumpFault`) işaretlenir ve SMS atılır.

## İklim Kontrolü

- **Sıcaklık:** Gündüz 20–24°C, gece 13–18°C hedef aralık; aralık dışına çıkınca fan açılır/kapanır.
- **Bağıl nem:** %60–75 hedef aralık; düşükse nemlendirme (iklim rölesi) açılır, yüksekse kapanır.
- **Kök bölgesi sıcaklığı:** 18–22°C hedef; bu aralığın dışına çıkarsa iklim rölesi buna göre devreye girer/çıkar.
- **Fotoperiyot:** Işık rölesi 06:00–22:00 arası (16 saat) açık tutulur, gün-nötr çeşit varsayımıyla.

## Güvenlik / Alarm Mantığı (SIM800L SMS)

- **Pompa aşırı akım** (nominal 0.8A'nın 1.5 katı üstü, INA219 ile ölçülür) → tıkanma/blokaj şüphesi, pompa kilitlenir.
- **Pompa düşük akım** (nominal 0.8A'nın 0.3 katı altı) → kuru çalışma/hava şüphesi, pompa kilitlenir.
- **Pompa zaman aşımı** (60 sn'de hedef hacme ulaşılamazsa) → arıza işaretlenir.
- **Su seviyesi düşük** (şamandıra) → sulama tamamen durdurulur.
- Arızalar web panelindeki "Arızaları Temizle" butonuyla (`clearFaults`) sıfırlanabilir.

## HTTP API

**ESP32 firmware (`httpapi.cpp`):**
- `GET /api/status` — tüm sensör verisi, röle durumları, arızalar ve aktif evre (JSON)
- `POST /api/control` — gövde: `{ "clearFaults": true }`, `{ "stage": 0-3 }`, veya `{ "relay": "pump"|"fan"|"light"|"climate", "on": true|false }`

**Web sunucusu (`server.js`, Mac üzerinde):**
- `GET /api/live` — `{ status, error }`, en son ESP32 poll sonucu
- `GET /api/history?hours=24` — SQLite'tan geçmiş okuma listesi
- `POST /api/control` — gövdeyi olduğu gibi ESP32'nin `/api/control`'üne ileten proxy

## Web Paneli

`http://localhost:3000` adresinde, koyu temalı tek sayfa dashboard:
- **Canlı kartlar:** ortam sıcaklık/nem/basınç (BME280), iki ayrı SHT30 sıcaklık/nem, ışık (lux), besin çözeltisi ve kök bölgesi sıcaklığı, substrat nemi, günlük sulama/drenaj hacmi ve sayısı, pompa voltaj/akım/güç, AC voltaj/akım/güç/enerji/frekans/güç faktörü, su seviyesi durumu.
- **Röle kontrolü:** pompa/fan/ışık/iklim rölelerini tek tıkla aç/kapat.
- **Uyarılar paneli:** aktif arıza varsa kırmızı, yoksa "Aktif uyarı yok"; "Arızaları Temizle" butonu.
- **24 saatlik geçmiş grafikler:** Sıcaklık/Nem, Sulama, Işık, Güç/Enerji — Chart.js ile, dakikada bir yenilenir.

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
