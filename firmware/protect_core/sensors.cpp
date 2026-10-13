// sensors.cpp — Final: MAX30102 + DS18B20 waterproof probe (+ DS3231-mini in logger.cpp).
// - MAX30102 via SparkFun MAX30105 lib (I2C 0x57), Maxim
//   maxim_heart_rate_and_oxygen_saturation() on 100-sample (4s @25sps) window,
//   rolling recalc every ~25 new samples (~1s). Non-blocking: each sensorsRead()
//   drains FIFO, appends, recalculates only when enough new data arrived.
// - Finger gate IR>50000; no finger => hrValid=false so TFT shows "--".
// - DS18B20 on PIN_DS18B20 via OneWire (waterproof probe, 4.7k pull-up to 3V3,
//   11-bit = 375ms conversion, + TEMP_OFFSET_C). -127 = disconnected, 85 = power-on reset.
#include "sensors.h"
#include "config.h"
#include <Wire.h>

#if !TEST_MODE_NO_SENSORS
#include <MAX30105.h>
#include <spo2_algorithm.h>
#include <OneWire.h>
#include <DallasTemperature.h>
static OneWire oneWire(PIN_DS18B20);
static DallasTemperature dsTemp(&oneWire);
static MAX30105 pox;
static bool hasPox = false, hasDs = false;

// Maxim window: 100 samples = 4s @25sps effective (100sps / avg4).
static uint32_t redBuffer[100];
static uint32_t irBuffer[100];
static uint8_t bufCount = 0;
static uint8_t newSinceCalc = 0;
static bool haveFirstCalc = false;
static int32_t calcHr = 0, calcSpo2 = 0;
static int8_t validHr = 0, validSpo2 = 0;
static uint32_t lastIr = 0;
#endif

static float lastHr = 0, lastSpo2 = 0, lastTemp = 0;

bool sensorsBegin() {
#if TEST_MODE_NO_SENSORS
  return true;
#else
  Wire.begin(PIN_I2C_SDA, PIN_I2C_SCL);
  hasPox = pox.begin(Wire, I2C_SPEED_STANDARD);
  if (hasPox) {
    // Brightness 0x1F (31) matches proven test_max30102 config. Do NOT raise:
    // 60 saturated the photodiode (~239k/262k, flat line) so Maxim never
    // validated HR/SpO2. Avg 4, mode 2 (Red+IR), rate 100, width 411,
    // ADC 4096 -> ~25sps usable for Maxim algorithm.
    pox.setup(0x1F, 4, 2, 100, 411, 4096);
  }
  dsTemp.begin();
  dsTemp.setResolution(11);  // 375ms conversion (vs 750ms at 12-bit), +-0.25C fine for body
  hasDs = (dsTemp.getDeviceCount() > 0);
  return hasPox && hasDs;
#endif
}

#if !TEST_MODE_NO_SENSORS
static void pushSample(uint32_t red, uint32_t ir) {
  if (bufCount < 100) {
    redBuffer[bufCount] = red;
    irBuffer[bufCount] = ir;
    bufCount++;
  } else {
    // Shift left by 1, append at end (cheap rolling; recalc gated by count).
    memmove(redBuffer, redBuffer + 1, sizeof(uint32_t) * 99);
    memmove(irBuffer, irBuffer + 1, sizeof(uint32_t) * 99);
    redBuffer[99] = red;
    irBuffer[99] = ir;
  }
  newSinceCalc++;
}
#endif

void sensorsPump() {
#if TEST_MODE_NO_SENSORS
  return;
#else
  // CRITICAL: SparkFun sense ring holds only STORAGE_SIZE=4 samples, so the
  // FIFO must be pumped every loop (~ms). Pumping once per 2s sample harvests
  // ~4 samples/call -> 100-sample window spans ~50s wall time, breaking the
  // Maxim 25sps assumption (invalid forever). Pumped per-loop, the window
  // fills in ~4s of finger contact as designed. Non-blocking: check() with no
  // new samples is a 2-byte I2C read.
  if (!hasPox) return;
  pox.check();  // pull HW FIFO (32 deep) into sense ring first...
  while (pox.available()) {  // ...then move all of them to the Maxim window
    uint32_t red = pox.getRed();
    uint32_t ir = pox.getIR();
    pox.nextSample();
    lastIr = ir;
    // Only feed plausible samples; very low IR = no finger, skip buffer
    // pollution but still update gate below.
    if (ir > 10000) pushSample(red, ir);
  }
#endif
}

Vitals sensorsRead() {
  Vitals v{lastHr, lastSpo2, lastTemp, false, false};
#if TEST_MODE_NO_SENSORS
  v = {72.0, 98.0, 36.6, true, true};  // hello-test values
  return v;
#endif
#if TEST_CRITICAL
  // SIMULATED DANGER — alarm-path drill only (see config.h). HR 130 (>120),
  // SpO2 90 (<93), Temp 39.0 (>38): all critical by design. TEST ONLY.
  v = {130.0, 90.0, 39.0, true, true};
  lastHr = v.hr; lastSpo2 = v.spo2; lastTemp = v.tempC;
  return v;
#endif
#if !TEST_MODE_NO_SENSORS
  if (hasDs) {
    dsTemp.requestTemperatures();
    float t = dsTemp.getTempCByIndex(0);  // -127 = disconnected, 85 = power-on reset
    if (!isnan(t) && t > 10.0 && t < 50.0 && t != 85.0) { lastTemp = t + TEMP_OFFSET_C; v.tempC = lastTemp; v.tempValid = true; }
    else if (lastTemp > 10.0) { v.tempC = lastTemp; v.tempValid = true; }
  } else if (lastTemp > 10.0) { v.tempC = lastTemp; v.tempValid = true; }

  if (hasPox) {
    // FIFO already pumped continuously by sensorsPump() (every loop) — here
    // we only evaluate the latest 100-sample window.
    bool finger = (lastIr > 50000);
    if (finger && bufCount >= 100 && (!haveFirstCalc || newSinceCalc >= 25)) {
      maxim_heart_rate_and_oxygen_saturation(irBuffer, 100, redBuffer,
        &calcSpo2, &validSpo2, &calcHr, &validHr);
      haveFirstCalc = true;
      newSinceCalc = 0;
    }

    if (finger && haveFirstCalc && validHr && validSpo2 &&
        calcHr >= 20 && calcHr <= 255 && calcSpo2 >= 50 && calcSpo2 <= 100) {
      lastHr = (float)calcHr;
      lastSpo2 = (float)calcSpo2;
      v.hr = lastHr; v.spo2 = lastSpo2; v.hrValid = true;
    } else {
      // Keep last-good numbers but flag invalid -> TFT shows "--" until
      // a valid 4s window + finger pressure is present.
      v.hr = lastHr; v.spo2 = lastSpo2; v.hrValid = false;
    }
  }
  return v;
#endif
}

uint32_t sensorsGetLastIr() {
#if TEST_MODE_NO_SENSORS
  return 60000;
#else
  return lastIr;
#endif
}
