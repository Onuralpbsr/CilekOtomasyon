# Çilek Otomasyon

Topraksız tarım (coco coir substrat + damlama sulama) yöntemiyle çilek yetiştiriciliği için ESP32-S3 tabanlı otomasyon sistemi. **Tek gerçek aktüatör pompa** — **sulama** tamamen otomatiktir (büyüme evresine göre zamanlama ve hacim). Işık, fan, ısıtıcı/nemlendirici/sisleme için fiziksel cihaz **yok**; bu yüzden sistem iklimi sürekli izler ve hedef dışına çıkınca SMS ile haber verir — ışık/havalandırma/nem müdahalesini sen yaparsın. Pompa arızaları (tıkanma, kuru çalışma, su seviyesi düşüklüğü) da SMS ile bildirilir ve pompa otomatik kilitlenir. Mac üzerinde çalışan web panelinden canlı/geçmiş veri takibi ve manuel röle kontrolü yapılır.

## Proje Hakkında

- **Yetiştirme yöntemi:** Coco coir substratlı, damlama sulamalı topraksız tarım.
- **Otomasyonun kapsamı — ne otomatik, ne değil:**
  - ✅ **Sulama** — substrat nemi ve büyüme evresine göre tamamen otomatik (zamanlama + hacim). Sistemin tek gerçek aktüatörü.
  - ✅ **Pompa güvenliği** — aşırı/düşük akım, su seviyesi, zaman aşımı: otomatik tespit + pompayı kapatma + SMS.
  - ⚠️ **Işık, iklim (sıcaklık/nem), havalandırma** — **hiçbir otomatik kontrol yok**, fiziksel ışık/fan/ısıtıcı/nemlendirici/sisleme cihazı bulunmuyor. Sıcaklık/nem/kök bölgesi sıcaklığı sadece izlenir, hedef dışına çıkınca SMS atılır. 4 kanallı röle kartının pompa dışındaki 3 kanalı şu an tamamen boşta — ileride bir cihaz bağlanırsa kullanılabilir, kod tarafında yer hazır.
- **Bitki için ne anlama geliyor:** Çilek köküne her zaman doğru zamanda doğru miktarda su/besin gider (susuz kalmaz, fazla sulanıp kök çürümez), pompa arızası anında durdurulup donanım korunur. Işık döngüsü, sıcaklık/nem konforu ise tamamen senin elinle/lambanla/havalandırmanla sağlanır — sistem sana ne zaman, neden müdahale etmen gerektiğini söyler.
- **Veri toplama:** ESP32 üzerindeki tüm sensörler ~2 saniyede bir okunur; Mac'teki web sunucusu ESP32'nin `/api/status` uç noktasını 5 saniyede bir çekip SQLite'a kaydeder, böylece geçmişe dönük grafiklerle (24 saatlik) trend takibi yapılabilir.
- **Uzaktan kontrol:** Web panelinden pompa ve 3 boş röle kanalı manuel olarak açılıp kapatılabilir, arızalar temizlenebilir.

## Sistem Mimarisi

```
[Sensörler] -> [ESP32-S3 firmware] -> WiFi/HTTP -> [Mac: Node.js web sunucusu] -> [SQLite] -> [Tarayıcı dashboard]
                      |
                      v
                 [Röleler: pompa (otomatik), 3x boş/manuel]
                      |
                      v
                 [SIM800L: pompa arızası + iklim hedef-dışı SMS'i]
```

- `firmware/CilekOtomasyon/` — ESP32-S3 N16R8 Arduino sketch'i:
  - `sensors.h/.cpp` — tüm sensörlerin okunması (I2C, OneWire, UART, analog, interrupt tabanlı debi sayacı)
  - `control.h/.cpp` — sulama, pompa güvenliği, iklim izleme (VPD + SMS uyarı) ve röle mantığı
  - `httpapi.h/.cpp` — `/api/status` ve `/api/control` HTTP uç noktaları (`network.h/.cpp` olarak başladı, ESP32 çekirdeğinin kendi `Network.h`'ıyla macOS'ta case-insensitive dosya sistemi yüzünden çakıştığı için `httpapi` olarak yeniden adlandırıldı)
  - `alerts.h/.cpp` — SIM800L üzerinden SMS gönderimi
  - `display.h/.cpp` — SSD1306 OLED üzerinde yerel özet ekranı (evre, sıcaklık/nem, substrat nemi, su seviyesi, pompa durumu, VPD, aktif arıza/iklim uyarısı)
- `web/` — Mac üzerinde yerel çalışan Node.js/Express paneli:
  - `server.js` — ESP32'yi periyodik polluyor, `/api/live`, `/api/history`, `/api/control` uç noktalarını sunuyor
  - `db.js` — `node:sqlite` ile geçmiş veri kaydı (`web/data/history.db`, repoya dahil değil)
  - `public/` — vanilla JS + Chart.js ile canlı kart ve grafik arayüzü
- `setup.sh` — clone sonrası tek komutla tüm kurulumu yapan betik (macOS).

## Malzemeler

- ESP32-S3 N16R8
- OLED SSD1306 (I2C, 128x64, adres 0x3C varsayılan)
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
| I2C SDA / SCL | 8 / 9 | BME280, SHT30 x2, TSL2561, INA219, DS3231, SSD1306 ortak hat |
| OneWire (DS18B20 x2) | 4 | indeks 0 = besin çözeltisi, indeks 1 = kök bölgesi |
| Debi - giriş | 5 | interrupt, FALLING |
| Debi - drenaj | 6 | interrupt, FALLING |
| Toprak nemi (analog) | 1 (ADC1_CH0) | kalibrasyon: kuru=2800, ıslak=1200 ham okuma |
| Şamandıra | 2 | INPUT_PULLUP, NC varsayımı (LOW = su var) |
| Röle - pompa | 15 | aktif LOW, otomatik sulama (tek gerçek aktüatör) |
| Röle 1 (boşta) | 17 | aktif LOW, fiziksel ışık/lamba yok, sadece manuel/ileride kullanım |
| Röle 2 (boşta) | 16 | aktif LOW, fiziksel fan yok, sadece manuel/ileride kullanım |
| Röle 3 (boşta) | 18 | aktif LOW, fiziksel ısıtıcı/nemlendirici/sisleme yok, sadece manuel/ileride kullanım |
| SIM800L RX / TX | 11 / 12 | UART1, 9600 baud |
| PZEM-004T RX / TX | 13 / 14 | UART2, Modbus RTU |

SHT30 I2C adresleri: `0x44` (ADDR pin GND) ve `0x45` (ADDR pin VCC). Debi sensörü kalibrasyonu YF-S201 için 450 pals/litre varsayılan, gerçek değerle kalibre edilmeli.

## Büyüme Evreleri ve Sulama Mantığı

Evreye göre substrat nem eşiği, hedef drenaj oranı, tek sulama darbesinin hacmi ve günlük maksimum sulama sayısı (`STAGE_PARAMS`, web panelinden `/api/control` ile `stage` 0-3 olarak değiştirilebilir):

| Evre | Nem alt sınırı (sulama tetik) | Nem üst hedef | Hedef drenaj oranı | Darbe hacmi | Gün içi max sulama |
|---|---|---|---|---|---|
| 0 — Fide | %70 | %75 | %12.5 | 0.15 L | 3 |
| 1 — Vegetatif | %65 | %70 | %17.5 | 0.25 L | 6 |
| 2 — Çiçek | %60 | %65 | %22.5 | 0.35 L | 8 |
| 3 — Meyve | %55 | %65 | %27.5 | 0.45 L | 12 |

- Substrat nemi eşiğin altına düşünce ve gece yasak penceresi (22:00–07:00, kök çürümesi riskini azaltmak için) dışında ve günlük sulama limiti dolmamışsa pompa devreye girer.
- Tek sulama, evrenin darbe hacmine ulaşılınca veya 60 saniyelik zaman aşımına uğrayınca durur; zaman aşımına uğrarsa pompa arızası (`pumpFault`) işaretlenir ve SMS atılır. Darbe hacmi bitki/kök hacmiyle birlikte büyür: fide en az, meyve evresi en çok su alır.
- **Drenaj oranı takibi:** giriş ve drenaj debi sensörlerinden bugünkü gerçek drenaj/giriş oranı hesaplanır (`runoffPctToday`) ve evrenin hedefiyle (`targetRunoffPct`) birlikte API'den ve web panelinden izlenebilir. Gün içi giriş hacmi 0.5 L'nin altındayken gürültülü oran hesaplanmaz. Bu sadece bilgilendirme amaçlıdır, sulamayı otomatik değiştirmez — hedeften sapma sürekliyse darbe hacmini veya frekansını elle ayarlamak gerekebilir.

## İklim İzleme ve SMS Uyarısı (otomatik düzeltme YOK)

Fiziksel ışık/fan/ısıtıcı/nemlendirici bulunmadığı için sistem sıcaklık/nem/kök bölgesi sıcaklığını **sadece izler**; hedef aralığın dışına çıkınca bir kere SMS gönderir, aralığa dönünce otomatik temizlenir (manuel "arıza temizle" gerekmez — kilitlenen bir donanım yok, sadece bilgi amaçlı). Gündüz/gece sınırı (06:00–22:00) sadece sıcaklık hedefini seçmek için kullanılır, fiziksel bir ışık rölesi sürmez.

- **Sıcaklık hedefi:** Gündüz (06:00–22:00) 20–24°C, gece 13–18°C.
- **Bağıl nem hedefi:** %60–75.
- **Kök bölgesi sıcaklığı hedefi:** 18–22°C.
- **VPD (buhar basıncı açığı, kPa):** Sıcaklık ve nemden hesaplanan, çilek gibi meyveli bitkilerde sıcaklık/nemi ayrı ayrı izlemekten daha anlamlı kabul edilen tek bir "bitki konforu" göstergesi (düşük VPD = nem fazla/terleme yetersiz/küf riski, yüksek VPD = hava çok kuru/bitki strese girer). Sadece bilgilendirme amaçlı gösterilir, eşik bazlı otomatik alarm üretmez — API'den ve web panelinden (kart + grafik) takip edilebilir.
- **Sensör yedekliliği:** Ortam sıcaklık/nem referansı önce BME280'den alınır; o sensör arızalanırsa iki SHT30'dan geçerli olan(lar)ı kullanılır (her ikisi de çalışıyorsa ortalaması, sadece biri çalışıyorsa o tek değer). Üçü de okunamazsa ilgili kontrol o döngüde atlanır.
- **Spare röle kanalları:** Işık, fan, ısıtıcı/nemlendirici röle çıkışları (pin 17, 16, 18) donanım bağlanmadığı için otomatik sürülmüyor; web panelinden manuel açılıp kapatılabilir (test veya ileride bağlanacak bir cihaz için).

## Güvenlik / Alarm Mantığı (SIM800L SMS)

- **Pompa aşırı akım** (nominal 0.8A'nın 1.5 katı üstü, INA219 ile ölçülür) → tıkanma/blokaj şüphesi, pompa kilitlenir.
- **Pompa düşük akım** (nominal 0.8A'nın 0.3 katı altı) → kuru çalışma/hava şüphesi, pompa kilitlenir.
- **Pompa zaman aşımı** (60 sn'de hedef hacme ulaşılamazsa) → arıza işaretlenir.
- **Su seviyesi düşük** (şamandıra) → sulama tamamen durdurulur.
- Güvenlik kesintisi otomatik sulamayla sınırlı değildir: pompa web panelinden **manuel** açılmışken arıza oluşursa da pompa anında kapatılır (manuel açma da aynı korumadan geçer). Aktif arıza varken pompa manuel olarak tekrar açılamaz.
- Arızalar web panelindeki "Arızaları Temizle" butonuyla (`clearFaults`) sıfırlanabilir.
- **WiFi bağlantısı koparsa** ESP32 30 saniyede bir otomatik yeniden bağlanmayı dener (`WiFi.setAutoReconnect` + periyodik kontrol); sulama/iklim mantığı WiFi'den bağımsız çalışmaya devam eder, sadece web panelinden erişim o süre kesilir.

## HTTP API

**ESP32 firmware (`httpapi.cpp`):**
- `GET /api/status` — tüm sensör verisi, röle durumları, arızalar, aktif evre, `irrigation.runoffPctToday`/`runoffTargetPct`, `climate.vpdKPa`/`alertActive` (JSON)
- `POST /api/control` — gövde: `{ "clearFaults": true }`, `{ "stage": 0-3 }`, veya `{ "relay": "pump"|"relay1"|"relay2"|"relay3", "on": true|false }` (`relay1`/`relay2`/`relay3` = boşta kanallar, fiziksel cihaz yok)

**Web sunucusu (`server.js`, Mac üzerinde):**
- `GET /api/live` — `{ status, error }`, en son ESP32 poll sonucu
- `GET /api/history?hours=24` — SQLite'tan geçmiş okuma listesi
- `POST /api/control` — gövdeyi olduğu gibi ESP32'nin `/api/control`'üne ileten proxy

## Web Paneli

`http://localhost:3000` adresinde, koyu temalı tek sayfa dashboard:
- **Canlı kartlar:** ortam sıcaklık/nem/basınç (BME280), iki ayrı SHT30 sıcaklık/nem, ışık (lux), VPD, besin çözeltisi ve kök bölgesi sıcaklığı, substrat nemi, günlük sulama/drenaj hacmi ve sayısı, gerçek/hedef drenaj oranı, pompa voltaj/akım/güç, AC voltaj/akım/güç/enerji/frekans/güç faktörü, su seviyesi durumu.
- **Durum şeridi:** evre, su seviyesi, sulama durumu, iklim durumu (Normal/Hedef dışı), aktif uyarı — tek bakışta.
- **Röle kontrolü:** pompa ve 3 boş röle kanalını (ışık/fan/ısıtıcı bağlanırsa kullanılacak) tek tıkla aç/kapat.
- **Uyarılar paneli:** aktif arıza/iklim uyarısı varsa kırmızı, yoksa "Aktif uyarı yok"; "Arızaları Temizle" butonu.
- **24 saatlik geçmiş grafikler:** her metrik kendi tek-seri mini grafiğinde (toplam 17 grafik), İklim/Kök & Substrat/Sulama/Güç başlıkları altında gruplanmış — Chart.js ile, dakikada bir yenilenir.

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
