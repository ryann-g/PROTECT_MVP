// test_max30102.ino — MAX30102 ONLY. Nothing else wired matters.
// Wire: VIN->3V3 GND->GND SDA->21 SCL->22. Serial 115200.
// PASS: "found 0x57" + IR>50000 with finger, IR small without.
#include <Arduino.h>
#include <Wire.h>
#include <MAX30105.h>
static MAX30105 pox;
void setup() {
  Serial.begin(115200); delay(800);
  Serial.println(F("TEST MAX30102 boot"));
  Wire.begin(21, 22);
  for (uint8_t a = 1; a < 127; a++) {
    Wire.beginTransmission(a);
    if (Wire.endTransmission() == 0) Serial.printf("I2C found 0x%02X\n", a);
  }
  if (pox.begin(Wire, I2C_SPEED_STANDARD)) {
    pox.setup(0x1F, 4, 2, 100, 411, 4096);
    Serial.println(F("MAX:OK — place finger steady"));
  } else Serial.println(F("MAX:MISSING — check VIN3V3/GND/SDA21/SCL22"));
}
void loop() {
  static unsigned long t = 0;
  if (millis() - t < 1000) return;
  t = millis();
  uint32_t ir = pox.getIR();
  Serial.printf("IR=%lu %s\n", (unsigned long)ir, ir > 50000 ? "FINGER:OK" : "FINGER:none");
}
