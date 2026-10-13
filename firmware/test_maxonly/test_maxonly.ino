// test_maxonly.ino — MAX30102 FULL PIPELINE, nothing else.
// Purpose: prove the MAX program path (FIFO pump -> 100-sample window ->
// Maxim HR/SpO2 -> validity gate) with zero interference from other sensors.
// Wire ONLY: MAX VIN->3V3 GND->GND SDA->21 SCL->22. Unplug everything else.
// Serial 115200. PASS: finger on 10s -> "HR:72bpm SpO2:97% VALID".
// FAIL guide:
//   "MAX:MISSING"            -> VIN/GND/SDA21/SCL22 wiring or dead module.
//   IR<20000 always          -> no finger / window covered / bright ambient.
//   IR>200000 frozen        -> saturated: lighten pressure, shade sensor.
//   IR varies but never VALID -> motion: hold finger dead still on table.
#include <Arduino.h>
#include <Wire.h>
#include <MAX30105.h>
#include <spo2_algorithm.h>

static MAX30105 pox;
static bool hasPox = false;

// Maxim window: 100 samples = 4s @25sps (100sps / avg4).
static uint32_t redBuffer[100];
static uint32_t irBuffer[100];
static uint8_t bufCount = 0;
static uint8_t newSinceCalc = 0;
static int haveFirstCalc = 0;
static int32_t calcHr = 0, calcSpo2 = 0;
static int8_t validHr = 0, validSpo2 = 0;
static uint32_t lastIr = 0;

static void pushSample(uint32_t red, uint32_t ir) {
  if (bufCount < 100) {
    redBuffer[bufCount] = red;
    irBuffer[bufCount] = ir;
    bufCount++;
  } else {
    memmove(redBuffer, redBuffer + 1, sizeof(uint32_t) * 99);
    memmove(irBuffer, irBuffer + 1, sizeof(uint32_t) * 99);
    redBuffer[99] = red;
    irBuffer[99] = ir;
  }
  newSinceCalc++;
}

void setup() {
  Serial.begin(115200);
  delay(800);
  Serial.println(F("TEST MAXONLY boot (MAX full pipeline, nothing else)"));
  Wire.begin(21, 22);
  for (uint8_t a = 1; a < 127; a++) {
    Wire.beginTransmission(a);
    if (Wire.endTransmission() == 0) Serial.printf("I2C found 0x%02X\n", a);
  }
  hasPox = pox.begin(Wire, I2C_SPEED_STANDARD);
  if (hasPox) {
    pox.setup(0x1F, 4, 2, 100, 411, 4096);  // brightness 31: proven vs saturation
    Serial.println(F("MAX:OK brightness=0x1F — place finger steady, hold still"));
  } else {
    Serial.println(F("MAX:MISSING — check VIN3V3/GND/SDA21/SCL22"));
  }
}

void loop() {
  // Fast pump EVERY loop: sense ring holds 4; 2s-cadence starves the window.
  if (hasPox) {
    pox.check();
    while (pox.available()) {
      uint32_t red = pox.getRed();
      uint32_t ir = pox.getIR();
      pox.nextSample();
      lastIr = ir;
      if (ir > 10000) pushSample(red, ir);
    }
  }

  static unsigned long t = 0;
  if (millis() - t < 1000) return;
  t = millis();

  int finger = (lastIr > 50000);
  if (finger && bufCount >= 100 && (!haveFirstCalc || newSinceCalc >= 25)) {
    maxim_heart_rate_and_oxygen_saturation(irBuffer, 100, redBuffer,
      &calcSpo2, &validSpo2, &calcHr, &validHr);
    haveFirstCalc = 1;
    newSinceCalc = 0;
  }

  Serial.printf("IR=%lu %s buf=%d/100 ", (unsigned long)lastIr,
    finger ? "FINGER:OK" : "FINGER:none", bufCount);
  if (finger && haveFirstCalc && validHr && validSpo2 &&
      calcHr >= 20 && calcHr <= 255 && calcSpo2 >= 50 && calcSpo2 <= 100) {
    Serial.printf("HR:%d bpm SpO2:%d%% VALID\n", (int)calcHr, (int)calcSpo2);
  } else {
    Serial.println(finger ? "warming up (need 4s still)..." : "(cover window, hold still)");
  }
}
