#include "display.h"
#include "config.h"

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

static Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);
static bool displayOk = false;

static const char *STAGE_NAMES[4] = {"Fide", "Vegetatif", "Cicek", "Meyve"};

void displayInit() {
  displayOk = display.begin(SSD1306_SWITCHCAPVCC, SSD1306_I2C_ADDR);
  if (!displayOk) return;

  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.println("Cilek Otomasyon");
  display.println("Baslatiliyor...");
  display.display();
}

void displayUpdate(const SensorData &d, const ControlState &s) {
  if (!displayOk) return;

  float tempRef = !isnan(d.ambientTempBME) ? d.ambientTempBME : d.tempSHT1;
  float humRef = !isnan(d.ambientHumBME) ? d.ambientHumBME : d.humSHT1;

  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);

  display.print("Evre: ");
  display.println(STAGE_NAMES[s.stage]);

  display.print("Sicaklik: ");
  display.print(tempRef, 1);
  display.println("C");

  display.print("Nem: ");
  display.print(humRef, 1);
  display.println("%");

  display.print("Substrat: ");
  display.print(d.soilMoisturePct, 0);
  display.println("%");

  display.print("Su seviyesi: ");
  display.println(d.waterLevelOk ? "Normal" : "DUSUK");

  display.print("Pompa:");
  display.print(s.pumpOn ? "ON " : "OFF ");
  display.print("Fan:");
  display.println(s.fanOn ? "ON" : "OFF");

  if (s.pumpFault || s.waterLowFault) {
    display.println("!! ARIZA VAR !!");
  }

  display.display();
}
