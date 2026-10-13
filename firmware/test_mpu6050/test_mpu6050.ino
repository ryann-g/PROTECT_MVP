// test_mpu6050.ino — MPU6050 ONLY (future addition).
// Wire: VCC->3V3 GND->GND SDA->21 SCL->22 AD0->3V3 (addr 0x69). Serial 115200.
// PASS: "found 0x69" + mag ~1.00 flat, spikes >2 on tap.
// If found 0x68 instead: AD0 not tied high — recheck that wire.
#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
static Adafruit_MPU6050 mpu;
static bool ok = false;
void setup() {
  Serial.begin(115200); delay(800);
  Serial.println(F("TEST MPU6050 boot"));
  Wire.begin(21, 22);
  for (uint8_t a = 1; a < 127; a++) {
    Wire.beginTransmission(a);
    if (Wire.endTransmission() == 0) Serial.printf("I2C found 0x%02X\n", a);
  }
  ok = mpu.begin(0x69, &Wire);
  if (ok) {
    mpu.setAccelerometerRange(MPU6050_RANGE_8_G);
    Serial.println(F("MPU:OK — flat=1.00g, tap it to spike"));
  } else Serial.println(F("MPU:MISSING — check VCC/GND/SDA21/SCL22/AD0->3V3"));
}
void loop() {
  if (!ok) { delay(1000); Serial.println(F("MPU:MISSING")); return; }
  static unsigned long t = 0;
  if (millis() - t < 500) return;
  t = millis();
  sensors_event_t a, g, temp;
  mpu.getEvent(&a, &g, &temp);
  float mag = sqrtf(a.acceleration.x * a.acceleration.x +
                    a.acceleration.y * a.acceleration.y +
                    a.acceleration.z * a.acceleration.z) / 9.81;
  Serial.printf("mag=%.2fg %s\n", mag, mag > 2.0 ? "SPIKE" : "rest");
}
