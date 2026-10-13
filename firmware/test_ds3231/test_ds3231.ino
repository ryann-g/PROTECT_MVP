// test_ds3231.ino — DS3231 RTC ONLY.
// Wire: VCC->3V3 GND->GND SDA->21 SCL->22. Serial 115200.
// PASS: "found 0x68" + ticking clock. If time wrong: uncomment adjust, upload
// once, re-comment, upload again.
#include <Arduino.h>
#include <Wire.h>
#include <RTClib.h>
static RTC_DS3231 rtc;
void setup() {
  Serial.begin(115200); delay(800);
  Serial.println(F("TEST DS3231 boot"));
  Wire.begin(21, 22);
  for (uint8_t a = 1; a < 127; a++) {
    Wire.beginTransmission(a);
    if (Wire.endTransmission() == 0) Serial.printf("I2C found 0x%02X\n", a);
  }
  if (rtc.begin()) {
    Serial.println(F("RTC:OK"));
    // rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));  // set once if needed
  } else Serial.println(F("RTC:MISSING — check VCC3V3/GND/SDA21/SCL22"));
}
void loop() {
  static unsigned long t = 0;
  if (millis() - t < 1000) return;
  t = millis();
  DateTime n = rtc.now();
  Serial.printf("%04d-%02d-%02d %02d:%02d:%02d\n",
    n.year(), n.month(), n.day(), n.hour(), n.minute(), n.second());
}
