#include "alerts.h"
#include "config.h"

static HardwareSerial sim800(1);
static bool gsmOk = false;

static bool waitForOk(unsigned long timeoutMs) {
  String resp;
  unsigned long start = millis();
  while (millis() - start < timeoutMs) {
    while (sim800.available()) resp += (char)sim800.read();
    if (resp.indexOf("OK") != -1) return true;
  }
  return false;
}

void alertsInit() {
  sim800.begin(SIM800_BAUD, SERIAL_8N1, PIN_SIM800_RX, PIN_SIM800_TX);
  delay(3000); // SIM800L açılış süresi
  sim800.println("AT");
  gsmOk = waitForOk(1000);
  sim800.println("AT+CMGF=1"); // SMS text modu
  delay(500);
  while (sim800.available()) sim800.read();
}

bool alertsGsmOk() { return gsmOk; }

void alertsSendSMS(const String &message) {
  sim800.print("AT+CMGS=\"");
  sim800.print(ALERT_PHONE_NUMBER);
  sim800.println("\"");
  delay(300);
  sim800.print(message);
  sim800.write(26); // Ctrl+Z gönderimi tamamlar
  delay(300);
}

void alertsLoop() {
  while (sim800.available()) sim800.read();
}
