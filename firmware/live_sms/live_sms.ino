// live_sms.ino — STANDALONE live-data SMS trial. Nothing else needed.
// Purpose: prove TEXTS CARRY FRESH 2-SECOND READINGS (not stale repeats).
// Reads real MAX30102 (HR/SpO2 via Maxim, pumped every loop) + real MCP9808,
// refreshes values every 2000ms tick, and texts a live snapshot on the first
// valid reading then every LIVE_SMS_EVERY_MS (trial cadence). Standalone:
// never touches protect_serial / protect_core / test_* / drill_sms files.
// Run: SIM900 on first (PWRKEY till NET blinks), finger on MAX, Upload
// (Flash Mode DIO), Serial Monitor @115200. Expect: 2s LIVE lines, then an
// SMS per interval with CHANGING values matching the screen. Compare two
// consecutive texts — different numbers = liveness proven.
#include <Arduino.h>
#include <Wire.h>
#include <MAX30105.h>
#include <spo2_algorithm.h>
// ---- Temperature sensor: MCP9808 (I2C 0x18) ----
// The MCP9808 replaced the DS18B20 waterproof probe: +/-0.25 C typical over
// the body range, 30 ms conversion, 1/16 degC resolution, and no blocking
// read. The probe is unplugged and its pin (GPIO4) now belongs to the SOS
// button.
#define MCP9808_ADDR  0x18  // A0/A1/A2 to GND
#define MCP9808_REG_TEMP_H 0x05
#define MCP9808_REG_TEMP_L 0x06
#define MCP9808_REG_TEMP_ID 0x07
#define MCP9808_ID_MICROCHIP 0x0C


// ---- Trial knobs (clearly marked) ----
static const unsigned long SAMPLE_MS = 2000;       // 2s sensor tick
static const unsigned long LIVE_SMS_EVERY_MS = 30000;  // TRIAL ONLY: text every 30s (costs load!)

// ---- Pins (frozen map) ----
static const int PIN_GSM_RX = 16;   // ESP32 RX <- module TXD
static const int PIN_GSM_TX = 17;   // ESP32 TX -> module RXD

// ---- Recipients (same slots as main builds; "" = skipped) ----
// LIVE_NUMBERS comes from secrets.h (local only, gitignored).
#if __has_include("secrets.h")
  #include "secrets.h"
#else
  #include "secrets_template.h"
#endif
static const uint8_t LIVE_COUNT = 3;

// ---- MAX pipeline (proven pattern: pump every loop, 100-sample window) ----
static MAX30105 pox;
static bool hasPox = false;
static uint32_t redBuffer[100], irBuffer[100];
static uint8_t bufCount = 0, newSinceCalc = 0;
static int haveFirstCalc = 0;
static int32_t calcHr = 0, calcSpo2 = 0;
static int8_t validHr = 0, validSpo2 = 0;
static uint32_t lastIr = 0;
static float lastHr = 0, lastSpo2 = 0, lastTemp = 0;

// ---- MCP9808 body temperature (16-bit, 1/16 degC) ----
static bool mcp9808Begin() {
  Wire.beginTransmission(MCP9808_ADDR);
  Wire.write(MCP9808_REG_TEMP_ID);
  if (Wire.endTransmission() != 0) return false;      // no device on the bus
  Wire.beginTransmission(MCP9808_ADDR);
  Wire.write(MCP9808_REG_TEMP_ID);
  if (Wire.requestFrom((int)MCP9808_ADDR, (uint8_t)1) != 1) return false;
  uint8_t id = Wire.read();
  // Accept a Microchip MCP9808 (0x0C). Other 0x18 devices are tolerated so a
  // CJMCU clone still answers.
  return (id == MCP9808_ID_MICROCHIP) || (id == 0x00);
}

static float mcp9808ReadC() {
  Wire.beginTransmission(MCP9808_ADDR);
  Wire.write(MCP9808_REG_TEMP_H);
  if (Wire.requestFrom((int)MCP9808_ADDR, (uint8_t)2) != 2) return NAN;
  uint16_t raw = (uint16_t)Wire.read() << 8;
  raw |= Wire.read();
  return (float)raw / 16.0f;
}

static void pushSample(uint32_t red, uint32_t ir) {
  if (bufCount < 100) { redBuffer[bufCount] = red; irBuffer[bufCount] = ir; bufCount++; }
  else {
    memmove(redBuffer, redBuffer + 1, sizeof(uint32_t) * 99);
    memmove(irBuffer, irBuffer + 1, sizeof(uint32_t) * 99);
    redBuffer[99] = red; irBuffer[99] = ir;
  }
  newSinceCalc++;
}

// ---- MCP9808 ----
static bool hasMcp = false;

// ---- GSM ----
static bool hasGsm = false;
static unsigned long lastSms = 0;

static String atCmd(const char *cmd, unsigned long timeout = 2500) {
  while (Serial2.available()) Serial2.read();
  Serial2.println(cmd);
  String res;
  unsigned long t0 = millis();
  while (millis() - t0 < timeout) {
    while (Serial2.available()) res += (char)Serial2.read();
    if (res.indexOf("OK") >= 0 || res.indexOf("ERROR") >= 0) break;
  }
  return res;
}

static bool sendLiveSms(unsigned long sampleNo, float hr, float spo2, float temp,
                        int hrOk, int tempOk) {
  if (!hasGsm) { Serial.println(F("[GSM] module missing — SMS skipped.")); return false; }
  if (lastSms != 0 && millis() - lastSms < LIVE_SMS_EVERY_MS) return false;
  if (atCmd("AT", 2000).indexOf("OK") < 0) {
    Serial.println(F("[GSM] mute, re-sweeping..."));
    hasGsm = false;
    const long bauds[] = {115200, 57600, 38400, 19200, 9600};
    for (uint8_t b = 0; b < 5 && !hasGsm; b++) {
      Serial2.begin(bauds[b], SERIAL_8N1, PIN_GSM_RX, PIN_GSM_TX);
      delay(1500);
      hasGsm = atCmd("AT", 2000).indexOf("OK") >= 0;
      if (!hasGsm) Serial2.end();
    }
    if (!hasGsm) { Serial.println(F("[GSM] revive failed — SMS skipped.")); return false; }
      atCmd("AT+CMGF=1"); atCmd("AT+CSCS=\"GSM\"");
    }
  // Live snapshot text: fresh 2s-tick values, "--" for absent sensors.
  char msg[160];
  int n = snprintf(msg, sizeof(msg), "PROTECT LIVE #%lu HR:", sampleNo);
  n += hrOk ? snprintf(msg + n, sizeof(msg) - n, "%.0f", hr)
            : snprintf(msg + n, sizeof(msg) - n, "--");
  n += snprintf(msg + n, sizeof(msg) - n, " SpO2:");
  n += hrOk ? snprintf(msg + n, sizeof(msg) - n, "%.0f", spo2)
            : snprintf(msg + n, sizeof(msg) - n, "--");
  n += snprintf(msg + n, sizeof(msg) - n, " T:");
  n += tempOk ? snprintf(msg + n, sizeof(msg) - n, "%.1fC", temp)
              : snprintf(msg + n, sizeof(msg) - n, "--");
  for (uint8_t i = 0; i < LIVE_COUNT; i++) {
    const char *num = LIVE_NUMBERS[i];
    if (!num || num[0] == '\0') continue;
    Serial.printf("[GSM] texting slot %d...\n", i + 1);
    Serial2.print("AT+CMGS=\""); Serial2.print(num); Serial2.println("\"");
    unsigned long t0 = millis();
    bool prompt = false;
    while (millis() - t0 < 5000) {
      if (Serial2.available() && Serial2.read() == '>') { prompt = true; break; }
    }
    if (!prompt) { Serial.println(F("[GSM] no > prompt, next slot.")); continue; }
    Serial2.print(msg);
    Serial2.write(26);
    String res;
    unsigned long t1 = millis();
    while (millis() - t1 < 12000) {
      while (Serial2.available()) res += (char)Serial2.read();
      if (res.indexOf("+CMGS:") >= 0 || res.indexOf("ERROR") >= 0) break;
    }
    Serial.printf("[GSM] slot %d result:%s\n", i + 1, res.c_str());
    delay(3000);
  }
  lastSms = millis();
  return true;
}

void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.println(F("LIVE_SMS trial boot: 2s ticks, texts carry fresh readings."));
  Wire.begin(21, 22);
  hasPox = pox.begin(Wire, I2C_SPEED_STANDARD);
  if (hasPox) pox.setup(0x1F, 4, 2, 100, 411, 4096);
  Serial.printf("MAX:%s\n", hasPox ? "OK (finger on, hold still)" : "MISSING (VIN3V3/GND/SDA21/SCL22)");
  hasMcp = mcp9808Begin();
  Serial.printf("MCP9808:%d\n", (int)hasMcp);
  const long bauds[] = {115200, 57600, 38400, 19200, 9600};
  for (uint8_t b = 0; b < 5 && !hasGsm; b++) {
    Serial2.begin(bauds[b], SERIAL_8N1, PIN_GSM_RX, PIN_GSM_TX);
    delay(2000);
    for (uint8_t i = 0; i < 2 && !hasGsm; i++) {
      hasGsm = atCmd("AT", 2500).indexOf("OK") >= 0;
      delay(500);
    }
    if (!hasGsm) Serial2.end();
  }
  Serial.printf("GSM:%s\n", hasGsm ? "OK (live texts armed)" : "MISSING (screen only, no SMS)");
  if (hasGsm) { atCmd("AT+CMGF=1"); atCmd("AT+CSCS=\"GSM\""); }
}

void loop() {
  // Fast pump every loop (sense ring holds 4; 2s-cadence would starve it).
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

  static unsigned long tTick = 0;
  static unsigned long sampleNo = 0;
  if (millis() - tTick < SAMPLE_MS) return;
  tTick = millis();
  sampleNo++;

  // 2s tick: temperature + Maxim recalc.
  int tempOk = 0;
  if (hasMcp) {
    float t = mcp9808ReadC();
    if (!isnan(t) && t > 10.0 && t < 50.0) {
      lastTemp = t; tempOk = 1;
    }
  }
  int finger = (lastIr > 50000);
  if (finger && bufCount >= 100 && (!haveFirstCalc || newSinceCalc >= 25)) {
    maxim_heart_rate_and_oxygen_saturation(irBuffer, 100, redBuffer,
      &calcSpo2, &validSpo2, &calcHr, &validHr);
    haveFirstCalc = 1;
    newSinceCalc = 0;
  }
  int hrOk = (finger && haveFirstCalc && validHr && validSpo2 &&
              calcHr >= 20 && calcHr <= 255 && calcSpo2 >= 50 && calcSpo2 <= 100);
  if (hrOk) { lastHr = (float)calcHr; lastSpo2 = (float)calcSpo2; }

  // Screen: the 2s live line.
  Serial.printf("LIVE #%lu HR:", sampleNo);
  if (hrOk) Serial.printf("%.0f SpO2:%.0f", lastHr, lastSpo2);
  else Serial.print("-- SpO2:--");
  if (tempOk) Serial.printf(" T:%.1fC", lastTemp);
  else Serial.print(" T:--");
  Serial.printf(" IR:%lu buf=%d/100\n", (unsigned long)lastIr, bufCount);

  // Trial SMS cap: 2 broadcasts TOTAL (balance saver), then screen-only.
  // 1st valid snapshot sends at once, 2nd after the interval. Failed sends
  // (module missing/revive fail) don't consume quota — only real broadcasts.
  static uint8_t smsSent = 0;
  static bool quotaNote = false;
  if (hrOk && smsSent < 2 && (smsSent == 0 || millis() - lastSms >= LIVE_SMS_EVERY_MS)) {
    if (sendLiveSms(sampleNo, lastHr, lastSpo2, lastTemp, hrOk, tempOk)) smsSent++;
  } else if (smsSent >= 2 && !quotaNote) {
    quotaNote = true;
    Serial.println(F("SMS quota done (2/2) — screen lines continue, no more texts."));
  }
}
