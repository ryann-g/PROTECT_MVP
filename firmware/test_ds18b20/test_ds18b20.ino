// test_ds18b20.ino — DS18B20 waterproof probe ONLY.
// Wire: RED VCC->3V3, BLACK GND->GND, YELLOW DATA->GPIO14 + 4.7k pull-up DATA->3V3.
// Terminal adapter: probe leads into terminal block, adapter pins to ESP32.
// Serial 115200. PASS: Temp updates every 2s, ~36-37C body (hold probe in hand).
// FAIL codes: -127 = disconnected (check DATA14/VCC/GND/pull-up), 85 = power-on reset (re-read).
#include <Arduino.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#define DS_PIN 14
static OneWire oneWire(DS_PIN);
static DallasTemperature ds(&oneWire);
void setup() {
  Serial.begin(115200); delay(800);
  Serial.println(F("TEST DS18B20 boot (DATA=14, 4.7k to 3V3)"));
  ds.begin();
  ds.setResolution(11);
  Serial.printf("devices:%d\n", (int)ds.getDeviceCount());
  if (ds.getDeviceCount() == 0) Serial.println(F("ERR: no probe (check VCC/GND/DATA14/pull-up)"));
}
void loop() {
  static unsigned long t = 0;
  if (millis() - t < 2000) return;
  t = millis();
  ds.requestTemperatures();
  float temp = ds.getTempCByIndex(0);
  Serial.print("Temp=");
  if (temp == -127.0) Serial.println("ERR(-127 disconnected)");
  else if (temp == 85.0) Serial.println("ERR(85 reset, retry)");
  else Serial.printf("%.1fC\n", temp);
}
