// test_dht11.ino — DHT11 ONLY.
// Wire: VCC->3V3 GND->GND DATA->14. Serial 115200.
// PASS: Temp/Hum update every 2s, no ERR storms.
#include <Arduino.h>
#include <DHT.h>
#define DHTPIN 14
#define DHTTYPE DHT11
static DHT dht(DHTPIN, DHTTYPE);
void setup() {
  Serial.begin(115200); delay(800);
  Serial.println(F("TEST DHT11 boot"));
  dht.begin();
}
void loop() {
  static unsigned long t = 0;
  if (millis() - t < 2000) return;
  t = millis();
  float temp = dht.readTemperature(false), hum = dht.readHumidity();
  Serial.print("Temp=");
  if (isnan(temp)) Serial.print("ERR(check VCC/GND/DATA14)");
  else Serial.printf("%.1fC", temp);
  Serial.print(" Hum=");
  if (isnan(hum)) Serial.println("ERR");
  else Serial.printf("%.0f%%\n", hum);
}
