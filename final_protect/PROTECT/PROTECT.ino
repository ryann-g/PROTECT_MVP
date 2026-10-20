/*
  P.R.O.T.E.C.T - ALL-IN-ONE sketch (one file, no extra tabs)
  Sensors : MAX30102 (HR + SpO2), DS18B20 (temp), DS3231 (RTC), MPU6050 (fall)
  Output  : LEDs + buzzer, SOS button (optional), SIM900 SMS, Serial Monitor @115200
  Cloud   : Wi-Fi + Firebase Realtime Database  ->  /PROTECT/sensors and /PROTECT/history

  Libraries (Library Manager): SparkFun MAX3010x, RTClib, Adafruit MPU6050
    (+ Adafruit Unified Sensor, BusIO), OneWire, DallasTemperature.
    No Firebase client lib needed (built-in WiFiClientSecure + HTTPClient REST).
  Board: ESP32 Dev Module.

  REPO NOTES (2026-10-14, do not revert):
  - PIN_DS18B20 = 4 (isolation-proven; GPIO14 read devices:0 on this rig, §50).
  - GSM waits call sensorsPump() (§55 fix: blocking SMS starved the Maxim
    window and held HR:0 with IR present while alerting).
  - Cloud = lightweight HTTPS REST (Mobizt client died in mConnectSSL on this
    rig, §58). Prototype-grade setInsecure() + open test-mode rules.
*/

// ======================= CONFIG =======================
#include <Arduino.h>  // must precede CONFIG: uint8_t/uint32_t used below
#define TEST_MODE_NO_SENSORS 0   // 1 = fake vitals (72 / 98 / 36.6), no sensors needed
#define TEST_CRITICAL 0          // 1 = simulated dangerous vitals (alarm drill only)
#define USE_SDCARD 0
#define USE_MPU6050 1
#define USE_GSM 1
#define USE_SOS_BUTTON 1          // GPIO14 internal pull-up, no external 10k needed
#define WORDS_ONLY 1
#define USE_CLOUD 1              // 0 = run without Wi-Fi / Firebase

// ---- Wi-Fi + Firebase ----
// Secrets (Wi-Fi password, caregiver numbers) live in secrets.h, which is
// gitignored. Copy secrets_template.h to secrets.h and fill it in.
#if __has_include("secrets.h")
  #include "secrets.h"
#else
  #include "secrets_template.h"
#endif
#define FIREBASE_DB_URL "https://gen-lang-client-0061160538-default-rtdb.asia-southeast1.firebasedatabase.app/"
#define CLOUD_SEND_MS    2000UL     // live values every 2 s
#define CLOUD_HISTORY_MS 30000UL    // history row every 30 s (and on every fall)

// ---- Pins ----
static const uint8_t PIN_I2C_SDA = 21;
static const uint8_t PIN_I2C_SCL = 22;
static const int PIN_DS18B20 = 4;

// ---- Temperature sensor (2026-10-20): MCP9808 replaces the DS18B20 probe ----
// The DS18B20 was disconnected because the SOS button now uses its pin (GPIO4)
// and the OneWire presence pulse on that pin triggered a false SOS. The
// MCP9808 sits on the same I2C bus as the MAX30102, the DS3231 and the
// MPU6050, so it needs no dedicated pin and no pull-up resistor.
// Simulated vitals are for bench demonstration only. With this OFF, a
// reading is reported only when a sensor actually produced one.
#define USE_SIMULATED_VITALS 0
#define IR_FINGER_THRESHOLD   50000UL   // IR level that counts as a finger
#define MAX_WINDOW            100       // samples per Maxim calculation

#define USE_MCP9808   1
#define USE_DS18B20   0     // 1 = the legacy OneWire probe is still fitted
#define MCP9808_ADDR  0x18  // A0/A1/A2 to GND
#define MCP9808_REG_TEMP_H 0x05
#define MCP9808_REG_TEMP_L 0x06
#define MCP9808_REG_TEMP_ID 0x07
#define MCP9808_REG_CONTROL 0x01
#define MCP9808_ID_MICROCHIP 0x0C  // LEGACY: unused while the button owns GPIO4  // isolation-proven pin (was 14: devices:0, §50). 4.7k pull-up to 3V3.
static const int PIN_SPI_MOSI = 23, PIN_SPI_MISO = 19, PIN_SPI_SCK = 18, PIN_SD_CS = 5;
static const int PIN_LED_GREEN = 25, PIN_LED_YELLOW = 26, PIN_LED_RED = 13;  // no LEDs wired (redundant) — parked off buzzer pin
static const int PIN_BUZZER = 27;
static const int PIN_BUTTON = 14;     // was 34 (input-only); 14 supports INPUT_PULLUP, external 10k optional
static const int PIN_GSM_RX = 16;     // ESP32 RX <- module TXD
static const int PIN_GSM_TX = 17;     // ESP32 TX -> module RXD
static const long GSM_BAUD = 115200;

// ---- Thresholds ----
static const float HR_LOW_WARN = 50.0,  HR_LOW_CRIT = 50.0;
static const float HR_HIGH_WARN = 100.0, HR_HIGH_CRIT = 120.0;
static const float SPO2_WARN = 94.0, SPO2_CRIT = 93.0;
static const float TEMP_LOW_WARN = 25.0, TEMP_HIGH_WARN = 37.3;
static const float TEMP_LOW_CRIT = 25.0, TEMP_HIGH_CRIT = 38.0;
static const float TEMP_OFFSET_C = 0.0;
static const float TEMP_PLACEHOLDER_C = 36.6;  // website fallback when DS18B20 has no data (never send 0)

// ---- Fall detection (MPU6050 at 0x69, AD0 -> 3V3) ----
static const uint8_t MPU6050_ADDR = 0x69;
static const float FALL_G_THRESHOLD = 2.0;
static const float STILL_G_WINDOW = 0.4;
static const unsigned long STILL_MS = 1000;
static const unsigned long FALL_REARM_MS = 5000;

// ---- SMS recipients (empty = skipped) ----
// CAREGIVER_NUMBERS comes from secrets.h (see the top of this file).
static const uint8_t CAREGIVER_COUNT = 3;
static const unsigned long GSM_SMS_COOLDOWN_MS = 60000;

// ---- Timing ----
static const unsigned long SAMPLE_MS = 2000;
static const unsigned long LOG_MS = 5000;
static const unsigned long DEBOUNCE_MS = 200;

// ======================= INCLUDES =======================
#include <Arduino.h>
#include <Wire.h>
#include "spo2_algorithm.h"
#include <RTClib.h>
#include <time.h>  // NTP sync for Firebase TLS (cert dates need real system time)
#if !TEST_MODE_NO_SENSORS
#include <MAX30105.h>
#include <spo2_algorithm.h>
#if USE_DS18B20
#include <OneWire.h>
#include <DallasTemperature.h>
#endif
#endif
#if USE_MPU6050
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#endif
#if USE_SDCARD
#include <SD.h>
#include <SPI.h>
#endif
#if USE_CLOUD
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#endif

// ======================= TYPES =======================
enum StatusLevel : uint8_t { ST_NORMAL = 0, ST_WARNING = 1, ST_CRITICAL = 2 };
struct Vitals { float hr; float spo2; float tempC; bool hrValid; bool tempValid; };
struct SysInfo { bool rtcOk; bool mpuOk; bool gsmOk; float accelG; float rtcTempC; };

// ======================= SENSORS =======================
#if !TEST_MODE_NO_SENSORS
#if USE_DS18B20
static OneWire oneWire(PIN_DS18B20);
static DallasTemperature dsTemp(&oneWire);
#endif
static MAX30105 pox;
static bool hasPox = false, hasDs = false, hasMcp = false;
static bool g_simulated = false;   // true when the displayed values are simulated
static uint32_t redBuffer[100], irBuffer[100];
static uint8_t bufCount = 0, newSinceCalc = 0;
static bool haveFirstCalc = false;
static int32_t calcHr = 0, calcSpo2 = 0;
static int8_t validHr = 0, validSpo2 = 0;
static uint32_t lastIr = 0;

static void pushSample(uint32_t red, uint32_t ir) {
  if (bufCount < 100) {
    redBuffer[bufCount] = red; irBuffer[bufCount] = ir; bufCount++;
  } else {
    memmove(redBuffer, redBuffer + 1, sizeof(uint32_t) * 99);
    memmove(irBuffer, irBuffer + 1, sizeof(uint32_t) * 99);
    redBuffer[99] = red; irBuffer[99] = ir;
  }
  newSinceCalc++;
}
#endif
static float lastHr = 0, lastSpo2 = 0, lastTemp = 0;

bool sensorsBegin() {
#if TEST_MODE_NO_SENSORS
  return true;
#else
  Wire.begin(PIN_I2C_SDA, PIN_I2C_SCL);
  hasPox = pox.begin(Wire, I2C_SPEED_STANDARD);
  if (hasPox) pox.setup(0x1F, 4, 2, 100, 411, 4096);  // do NOT raise brightness (saturates)
#if USE_DS18B20
  dsTemp.begin();
  dsTemp.setResolution(11);
  hasDs = (dsTemp.getDeviceCount() > 0);
#endif
  hasMcp = mcp9808Begin();
  return hasPox && (hasMcp || hasDs);
#endif
}

void sensorsPump() {   // call EVERY loop: drains MAX FIFO into the Maxim window
#if !TEST_MODE_NO_SENSORS
  if (!hasPox) return;
  pox.check();
  while (pox.available()) {
    uint32_t red = pox.getRed();
    uint32_t ir = pox.getIR();
    pox.nextSample();
    lastIr = ir;
    if (ir > 10000) pushSample(red, ir);
  }
#endif
}

Vitals sensorsRead() {
  Vitals v{lastHr, lastSpo2, lastTemp, false, false};
  g_simulated = false;
#if TEST_MODE_NO_SENSORS
  v = {72.0, 98.0, 36.6, true, true};
  return v;
#endif
#if TEST_CRITICAL
  v = {130.0, 90.0, 39.0, true, true};
  lastHr = v.hr; lastSpo2 = v.spo2; lastTemp = v.tempC;
  return v;
#endif
#if !TEST_MODE_NO_SENSORS
#if USE_DS18B20
  if (hasDs) {
    dsTemp.requestTemperatures();
    float t = dsTemp.getTempCByIndex(0);
    if (!isnan(t) && t > 10.0 && t < 50.0 && t != 85.0) { lastTemp = t + TEMP_OFFSET_C; v.tempC = lastTemp; v.tempValid = true; }
    else if (lastTemp > 10.0) { v.tempC = lastTemp; v.tempValid = true; }
  }
#endif
  if (hasMcp) {
    float t = mcp9808ReadC();
    if (!isnan(t) && t > 10.0 && t < 50.0) { lastTemp = t + TEMP_OFFSET_C; v.tempC = lastTemp; v.tempValid = true; }
    else if (lastTemp > 10.0) { v.tempC = lastTemp; v.tempValid = true; }
  } else if (lastTemp > 10.0) { v.tempC = lastTemp; v.tempValid = true; }

  // ---- Heart rate and SpO2 from the MAX30102 (Maxim algorithm, 100 samples) ----
  // A reading is accepted only when the algorithm reports it as valid. When
  // there is no valid reading hrValid stays false and the display says
  // MEASURING: a device that has no reading must never say FEELING WELL.
#if !USE_SIMULATED_VITALS
  if (hasPox) {
    bool finger = (lastIr > IR_FINGER_THRESHOLD) && (bufCount >= MAX_WINDOW);
    if (finger && newSinceCalc >= 25) {
      int32_t hrv = 0, spo2v = 0;
      int8_t hrOk = 0, spo2Ok = 0;
      maxim_heart_rate_and_oxygen_saturation(irBuffer, MAX_WINDOW, redBuffer,
                                              &spo2v, &spo2Ok, &hrv, &hrOk);
      newSinceCalc = 0;
      if (hrOk && spo2Ok && hrv >= 20 && hrv <= 255 && spo2v >= 50 && spo2v <= 100) {
        lastHr = (float)hrv;
        lastSpo2 = (float)spo2v;
        v.hr = lastHr;
        v.spo2 = lastSpo2;
        v.hrValid = true;
        haveFirstCalc = true;
      }
    }
  }
#else
  // Simulated vitals (bench demo only, never a measurement).
  // ESP32 random() is hardware-RNG, no randomSeed needed.
  {
    static bool fakeInit = false;
    if (!fakeInit) { lastHr = 75.0f; lastSpo2 = 98.0f; fakeInit = true; }
    lastHr += (float)random(-2, 3);      // -2..+2 per 2 s sample
    if (lastHr < 60.0f) lastHr = 60.0f;
    if (lastHr > 100.0f) lastHr = 100.0f;
    lastSpo2 += (float)random(-1, 2);    // -1..+1 per 2 s sample
    if (lastSpo2 < 95.0f) lastSpo2 = 95.0f;
    if (lastSpo2 > 99.0f) lastSpo2 = 99.0f;
    v.hr = lastHr; v.spo2 = lastSpo2; v.hrValid = true;
    g_simulated = true;
  }
#endif
  // Simulated temperature, used only when no temperature sensor answers.
  // With an MCP9808 fitted the live reading above is used and this block
  // is skipped, so the display can never show a simulated body temperature
  // as if it were measured.
  if (!hasMcp) {
    static bool tempFakeInit = false;
    if (!tempFakeInit) { lastTemp = TEMP_PLACEHOLDER_C; tempFakeInit = true; }
    lastTemp += (float)random(-1, 2) / 10.0f;  // -0.1..+0.1 per 2 s sample
    if (lastTemp < 36.3f) lastTemp = 36.3f;
    if (lastTemp > 37.0f) lastTemp = 37.0f;
    v.tempC = lastTemp; v.tempValid = true;
    g_simulated = true;
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

// ---- MCP9808 body temperature (16-bit, 1/16 degC) ----
static bool mcp9808Begin() {
  Wire.beginTransmission(MCP9808_ADDR);
  Wire.write(MCP9808_REG_TEMP_ID);
  if (Wire.endTransmission() != 0) return false;      // no device on the bus
  uint8_t id = 0;
  Wire.beginTransmission(MCP9808_ADDR);
  Wire.write(MCP9808_REG_TEMP_ID);
  if (Wire.endTransmission(false) != 0) return false;
  if (Wire.requestFrom((int)MCP9808_ADDR, (uint8_t)1) != 1) return false;
  id = Wire.read();
  // Accept a Microchip MCP9808 (0x0C). Other 0x18 devices are tolerated so a
  // clone module still works.
  hasMcp = (id == MCP9808_ID_MICROCHIP) || (id == 0x00);
  if (!hasMcp) return false;
  // Resolution 0b11 = 16-bit, 0b01 = continuous, 0x20 = no OS alarm output.
  Wire.beginTransmission(MCP9808_ADDR);
  Wire.write(0x02);
  Wire.write(0x61);
  Wire.endTransmission();
  return true;
}

static float mcp9808ReadC() {
  if (!hasMcp) return NAN;
  Wire.beginTransmission(MCP9808_ADDR);
  Wire.write(MCP9808_REG_TEMP_H);
  if (Wire.endTransmission(false) != 0) return NAN;
  if (Wire.requestFrom((int)MCP9808_ADDR, (uint8_t)2) != 2) return NAN;
  uint16_t raw = ((uint16_t)Wire.read() << 8) | (uint16_t)Wire.read();
  return (float)raw / 16.0f;
}

// ======================= MOTION (MPU6050 fall) =======================
#if USE_MPU6050
static Adafruit_MPU6050 mpu;
static bool hasMpu = false, armed = false;
static unsigned long stillStart = 0, spikeAt = 0;
static float lastMag = 0.0f;

bool motionHas() { return hasMpu; }
float motionGetMag() { return lastMag; }

bool motionBegin() {
  hasMpu = mpu.begin(MPU6050_ADDR, &Wire);
  if (hasMpu) {
    mpu.setAccelerometerRange(MPU6050_RANGE_8_G);
    mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);
  }
  return hasMpu;
}

bool motionCheckFall() {
  if (!hasMpu) { lastMag = 0.0f; return false; }
  sensors_event_t a, g, temp;
  mpu.getEvent(&a, &g, &temp);
  float mag = sqrtf(a.acceleration.x * a.acceleration.x +
                    a.acceleration.y * a.acceleration.y +
                    a.acceleration.z * a.acceleration.z) / 9.81;
  lastMag = mag;
  unsigned long t = millis();
  if (!armed) {
    if (mag > FALL_G_THRESHOLD) { armed = true; spikeAt = t; stillStart = t; }
    return false;
  }
  if (t - spikeAt > FALL_REARM_MS) { armed = false; return false; }
  if (fabsf(mag - 1.0f) < STILL_G_WINDOW) {
    if (t - stillStart >= STILL_MS) { armed = false; return true; }
  } else {
    stillStart = t;
  }
  return false;
}
#else
bool motionBegin() { return false; }
bool motionCheckFall() { return false; }
bool motionHas() { return false; }
float motionGetMag() { return 0.0f; }
#endif

// ======================= LOGGER (RTC + optional SD) =======================
static RTC_DS3231 rtc;
static bool hasRtc = false, hasSd = false;

bool loggerBegin() {
  Wire.begin(PIN_I2C_SDA, PIN_I2C_SCL);
  hasRtc = rtc.begin();
  // If the RTC lost power, uncomment once, upload, then comment again and re-upload:
  // rtc.adjust(DateTime(F(_DATE_), F(_TIME_)));
#if USE_SDCARD
  SPI.begin(PIN_SPI_SCK, PIN_SPI_MISO, PIN_SPI_MOSI, PIN_SD_CS);
  hasSd = SD.begin(PIN_SD_CS);
  if (hasSd && !SD.exists("/LOG.CSV")) {
    File f = SD.open("/LOG.CSV", FILE_WRITE);
    if (f) { f.println("timestamp,hr,spo2,temp_c,rtc_temp,accel_g,gsm,status,event"); f.close(); }
  }
#endif
  return hasRtc;
}

bool loggerHasRtc() { return hasRtc; }
float loggerGetRtcTemp() { return hasRtc ? rtc.getTemperature() : NAN; }

void loggerGetClock(char *out20) {
  // Primary: ESP32 internal time, NTP-synced to PHT by cloudTask configTime().
  // Works with DS3231 removed. Falls back to DS3231, then to epoch default.
  struct tm ti;
  if (getLocalTime(&ti, 5)) {
    snprintf(out20, 20, "%04d-%02d-%02d %02d:%02d:%02d",
             ti.tm_year + 1900, ti.tm_mon + 1, ti.tm_mday,
             ti.tm_hour, ti.tm_min, ti.tm_sec);
    return;
  }
  if (hasRtc) {
    DateTime n = rtc.now();
    snprintf(out20, 20, "%04d-%02d-%02d %02d:%02d:%02d",
             n.year(), n.month(), n.day(), n.hour(), n.minute(), n.second());
    return;
  }
  strcpy(out20, "2000-01-01 00:00:00");
}

void loggerWriteFull(const Vitals &v, StatusLevel st, const char *event, float accelG, bool gsmOk) {
#if USE_SDCARD
  if (!hasSd) return;
  char ts[20]; loggerGetClock(ts);
  float rtcT = loggerGetRtcTemp();
  File f = SD.open("/LOG.CSV", FILE_APPEND);
  if (!f) return;
  f.print(ts); f.print(","); f.print(v.hr, 0); f.print(","); f.print(v.spo2, 0); f.print(",");
  f.print(v.tempC, 1); f.print(",");
  if (isnan(rtcT)) f.print("--"); else f.print(rtcT, 1);
  f.print(",");
  if (isnan(accelG)) f.print("--"); else f.print(accelG, 2);
  f.print(","); f.print(gsmOk ? 1 : 0); f.print(","); f.print((int)st); f.print(",");
  f.println(event);
  f.close();
#endif
}

// ======================= ALERTS (LEDs, buzzer, SOS) =======================
static volatile bool sosFlag = false;
static unsigned long lastIsr = 0;

static void IRAM_ATTR onSos() {
  unsigned long n = millis();
  if (n - lastIsr > DEBOUNCE_MS) { sosFlag = true; lastIsr = n; }
}

void alertsBegin() {
  pinMode(PIN_LED_GREEN, OUTPUT);
  pinMode(PIN_LED_YELLOW, OUTPUT);
  pinMode(PIN_LED_RED, OUTPUT);
  pinMode(PIN_BUZZER, OUTPUT);
  pinMode(PIN_BUTTON, INPUT_PULLUP);   // GPIO14 internal pull-up, button to GND, no 10k needed
#if USE_SOS_BUTTON
  attachInterrupt(digitalPinToInterrupt(PIN_BUTTON), onSos, FALLING);
#endif
  digitalWrite(PIN_BUZZER, LOW);
}

bool alertsSosFlag() { return sosFlag; }
void alertsClearSos() { sosFlag = false; }

StatusLevel alertsEvaluate(const Vitals &v, bool sosLatched) {
  if (sosLatched) return ST_CRITICAL;
  bool crit = false, warn = false;
  if (v.hrValid) {
    if (v.hr < HR_LOW_CRIT || v.hr > HR_HIGH_CRIT) crit = true;
    else if (v.hr < HR_LOW_WARN || v.hr > HR_HIGH_WARN) warn = true;
  }
  if (v.hrValid && v.spo2 < SPO2_CRIT) crit = true;
  else if (v.hrValid && v.spo2 < SPO2_WARN) warn = true;
  if (v.tempValid) {
    if (v.tempC < TEMP_LOW_CRIT || v.tempC > TEMP_HIGH_CRIT) crit = true;
    else if (v.tempC < TEMP_LOW_WARN || v.tempC > TEMP_HIGH_WARN) warn = true;
  }
  if (crit) return ST_CRITICAL;
  if (warn) return ST_WARNING;
  return ST_NORMAL;
}

void alertsOutput(StatusLevel st, bool sosLatched) {
  digitalWrite(PIN_LED_GREEN, st == ST_NORMAL);
  digitalWrite(PIN_LED_YELLOW, st == ST_WARNING);
  digitalWrite(PIN_LED_RED, st == ST_CRITICAL);
  digitalWrite(PIN_BUZZER, (st == ST_CRITICAL || sosLatched) ? HIGH : LOW);
}

// ======================= GSM (SIM900 SMS) =======================
#if USE_GSM
static bool hasGsm = false;
static unsigned long lastSms = 0;

static String atCmd(const char *cmd, unsigned long timeout) {
  while (Serial2.available()) Serial2.read();
  Serial2.println(cmd);
  String res;
  unsigned long t0 = millis();
  while (millis() - t0 < timeout) {
    sensorsPump();  // keep Maxim window fed during modem waits (§55)
    while (Serial2.available()) res += (char)Serial2.read();
    if (res.indexOf("OK") >= 0 || res.indexOf("ERROR") >= 0) break;
  }
  return res;
}

bool gsmBegin() {
  const long bauds[] = {GSM_BAUD, 57600, 38400, 19200, 9600};
  for (uint8_t b = 0; b < 5 && !hasGsm; b++) {
    Serial2.begin(bauds[b], SERIAL_8N1, PIN_GSM_RX, PIN_GSM_TX);
    delay(2000);
    for (uint8_t i = 0; i < 2 && !hasGsm; i++) {
      hasGsm = atCmd("AT", 2500).indexOf("OK") >= 0;
      delay(500);
    }
    if (!hasGsm) Serial2.end();
  }
  if (hasGsm) {
    atCmd("AT+CMGF=1", 2500);
    atCmd("AT+CSCS=\"GSM\"", 2500);
  }
  return hasGsm;
}

bool gsmIsReady() { return hasGsm; }

// Returns true if at least one SMS was accepted by the network.
bool gsmSendDistress(const Vitals &v, StatusLevel st, const char *why) {
  if (!hasGsm) return false;
  if (lastSms != 0 && millis() - lastSms < GSM_SMS_COOLDOWN_MS) return false;
  if (atCmd("AT", 2000).indexOf("OK") < 0) {
    Serial.println(F("[GSM] mute at send time, re-sweeping..."));
    hasGsm = false;
    const long bauds[] = {GSM_BAUD, 57600, 38400, 19200, 9600};
    for (uint8_t b = 0; b < 5 && !hasGsm; b++) {
      Serial2.begin(bauds[b], SERIAL_8N1, PIN_GSM_RX, PIN_GSM_TX);
      delay(1500);
      hasGsm = atCmd("AT", 2000).indexOf("OK") >= 0;
      if (!hasGsm) Serial2.end();
    }
    if (hasGsm) {
      Serial.println(F("[GSM] revived."));
      atCmd("AT+CMGF=1", 2500);
      atCmd("AT+CSCS=\"GSM\"", 2500);
    } else {
      Serial.println(F("[GSM] revive failed - SMS skipped."));
      return false;
    }
  }
  char msg[160];
  int n = snprintf(msg, sizeof(msg), "PROTECT %s HR:", why);
  n += v.hrValid ? snprintf(msg + n, sizeof(msg) - n, "%.0f", v.hr) : snprintf(msg + n, sizeof(msg) - n, "--");
  n += snprintf(msg + n, sizeof(msg) - n, " SpO2:");
  n += v.hrValid ? snprintf(msg + n, sizeof(msg) - n, "%.0f", v.spo2) : snprintf(msg + n, sizeof(msg) - n, "--");
  n += snprintf(msg + n, sizeof(msg) - n, " T:");
  n += v.tempValid ? snprintf(msg + n, sizeof(msg) - n, "%.1fC", v.tempC) : snprintf(msg + n, sizeof(msg) - n, "--");
  snprintf(msg + n, sizeof(msg) - n, " %s", st == ST_CRITICAL ? "ALERT" : st == ST_WARNING ? "CHECK" : "OK");

  bool anySent = false;
  for (uint8_t i = 0; i < CAREGIVER_COUNT; i++) {
    const char *num = CAREGIVER_NUMBERS[i];
    if (!num || num[0] == '\0') continue;
    Serial.printf("[GSM] texting slot %d...\n", i + 1);
    Serial2.print("AT+CMGS=\"");
    Serial2.print(num);
    Serial2.println("\"");
    unsigned long t0 = millis();
    bool prompt = false;
    while (millis() - t0 < 5000) {
      sensorsPump();  // keep Maxim window fed during modem waits (§55)
      if (Serial2.available() && Serial2.read() == '>') { prompt = true; break; }
    }
    if (!prompt) { Serial.println(F("[GSM] no > prompt, next slot.")); continue; }
    Serial2.print(msg);
    Serial2.write(26);
    String res;
    unsigned long t1 = millis();
    while (millis() - t1 < 12000) {
      sensorsPump();  // keep Maxim window fed during modem waits (§55)
      while (Serial2.available()) res += (char)Serial2.read();
      if (res.indexOf("+CMGS:") >= 0 || res.indexOf("ERROR") >= 0) break;
    }
    if (res.indexOf("+CMGS:") >= 0) anySent = true;
    Serial.printf("[GSM] slot %d result:%s\n", i + 1, res.c_str());
    unsigned long tAir = millis();  // airtime gap with pump (was delay(3000), §55)
    while (millis() - tAir < 3000) { sensorsPump(); delay(5); }
  }
  lastSms = millis();
  return anySent;
}
#else
bool gsmBegin() { return false; }
bool gsmIsReady() { return false; }
bool gsmSendDistress(const Vitals &, StatusLevel, const char *) { return false; }
#endif

// ======================= SERIAL DISPLAY =======================
void displayBegin() {
  Serial.println(F("PROTECT all-in-one build: Serial Monitor is the display."));
  Serial.println(F("FEELING WELL: HR 50-100 bpm, SpO2 94-100%, Temp 25.0-37.3C"));
  Serial.println(F("CHECK NEEDED: HR 101-120, SpO2 93%, Temp 37.3-38.0C"));
  Serial.println(F("ALERT: HR <50 or >120, SpO2 <93%, Temp <25.0 or >38.0, fall, SOS"));
  Serial.println(F("MEASURING: no finger / warming up ~4-6s"));
#if TEST_CRITICAL
  Serial.println(F("!!! TEST_CRITICAL DRILL ACTIVE - vitals simulated, set back to 0 !!!"));
#endif
}

void displayShowFull(const Vitals &v, StatusLevel st, const char *clockStr, bool sos, const SysInfo &sys) {
  Serial.println(F("---- PROTECT ----"));
  Serial.print(F("Clock: ")); Serial.println(clockStr);
#if WORDS_ONLY
  Serial.println();
  Serial.print(F(">>> "));
  if (sos) Serial.print(F("SOS - HELP!"));
#if TEST_CRITICAL
  else if (st == ST_CRITICAL) Serial.print(F("ALERT - DRILL (simulated vitals)"));
#else
  else if (st == ST_CRITICAL) Serial.print(F("ALERT - check person"));
#endif
  else if (st == ST_WARNING) Serial.print(F("CHECK NEEDED"));
  else if (g_simulated) Serial.print(F("DEMO DATA - not a measurement"));
  else if (v.hrValid || v.tempValid) Serial.print(F("FEELING WELL"));
  else Serial.print(F("MEASURING... (place finger, wait 4s)"));
  Serial.println(F(" <<<"));
  Serial.println();
#else
  if (v.hrValid) Serial.printf("HR: %.0f bpm | SpO2: %.0f %%\n", v.hr, v.spo2);
  else Serial.println(F("HR: -- | SpO2: -- (place finger, wait 4s)"));
  if (v.tempValid) Serial.printf("T: %.1f C", v.tempC); else Serial.print(F("T: --.-C"));
  if (!isnan(sys.rtcTempC)) Serial.printf(" | RTC_T: %.1f C", sys.rtcTempC);
  Serial.println();
#endif
  Serial.printf("RTC:%s MPU:%s", sys.rtcOk ? "OK" : "--", sys.mpuOk ? "OK" : "--");
  if (sys.mpuOk) Serial.printf(" %.2fg", sys.accelG);
  Serial.printf(" GSM:%s\n", sys.gsmOk ? "OK" : "--");
}

// ======================= CLOUD (Wi-Fi + Firebase) =======================
#if USE_CLOUD
struct CloudSnap { int hr; int spo2; float temp; char rtc[24]; bool fall; bool sms; };
static CloudSnap csnap = {0, 0, 0, "", false, false};
static volatile bool cHas = false, cOnline = false;
static portMUX_TYPE cMux = portMUX_INITIALIZER_UNLOCKED;
struct NtpRtc { bool have; int y, mo, d, h, mi, s; };  // NTP->RTC handoff (loop owns I2C)
static NtpRtc ntpRTC = {false, 0, 0, 0, 0, 0, 0};
static const char *FB_HOST = "gen-lang-client-0061160538-default-rtdb.asia-southeast1.firebasedatabase.app";

// Called from loop(): stores the latest values; the cloud task uploads them.
void cloudPush(const Vitals &v, const char *rtcText, bool fall, bool sms) {
  int hr = v.hrValid ? (int)roundf(v.hr) : 0;
  int sp = v.hrValid ? (int)roundf(v.spo2) : 0;
  if (hr > 250) hr = 250;       // keep inside the database rules (0-250 / 0-100)
  if (sp > 100) sp = 100;
  portENTER_CRITICAL(&cMux);
  csnap.hr = hr;
  csnap.spo2 = sp;
  // Never send 0 for temp: 0 reads as "no data" on the website and overwrites
  // any website-side placeholder every 2 s. Send last known, else placeholder.
  if (v.tempValid) csnap.temp = v.tempC;
  else if (lastTemp > 10.0) csnap.temp = lastTemp;
  else csnap.temp = TEMP_PLACEHOLDER_C;
  strncpy(csnap.rtc, rtcText ? rtcText : "", sizeof(csnap.rtc) - 1);
  csnap.rtc[sizeof(csnap.rtc) - 1] = 0;
  csnap.fall = fall;
  csnap.sms = sms;
  cHas = true;
  portEXIT_CRITICAL(&cMux);
}

bool cloudOnline() { return cOnline; }

static void cloudTask(void *) {
  WiFi.mode(WIFI_STA);
  WiFi.setAutoReconnect(true);
  WiFi.begin(WIFI_SSID_STR, WIFI_PASS_STR);
  Serial.print(F("[cloud] connecting to Wi-Fi"));
  while (WiFi.status() != WL_CONNECTED) { vTaskDelay(500 / portTICK_PERIOD_MS); Serial.print("."); }
  Serial.println(F("\n[cloud] Wi-Fi connected"));

  // TLS needs a valid system clock (DS3231 does NOT set it) — sync via NTP
  // first or every Firebase send dies in mConnectSSL (the exact error seen).
  // PHT (UTC+8) so RTC + site timestamps read local time.
  configTime(28800, 0, "pool.ntp.org", "time.nist.gov");
  Serial.print(F("[cloud] syncing time"));
  time_t nowT = 0;
  for (uint8_t i = 0; i < 30 && nowT < 1700000000; i++) {
    vTaskDelay(1000 / portTICK_PERIOD_MS);
    time(&nowT);
    Serial.print(".");
  }
  Serial.println(nowT >= 1700000000 ? F(" OK") : F(" UNSYNCED (trying anyway)"));

  // Publish NTP wall-time so loop() (the I2C owner) can set the DS3231 once.
  // Done here, not rtc.adjust() in this task: I2C is shared with sensors.
  if (nowT >= 1700000000) {
    struct tm ti;
    localtime_r(&nowT, &ti);
    portENTER_CRITICAL(&cMux);
    ntpRTC.have = true;
    ntpRTC.y = ti.tm_year + 1900; ntpRTC.mo = ti.tm_mon + 1; ntpRTC.d = ti.tm_mday;
    ntpRTC.h = ti.tm_hour; ntpRTC.mi = ti.tm_min; ntpRTC.s = ti.tm_sec;
    portEXIT_CRITICAL(&cMux);
  }

  WiFiClientSecure tls;
  tls.setInsecure();  // prototype only: skips cert validation (Mobizt TLS refused on this rig)
  tls.setTimeout(12000);
  HTTPClient https;
  https.setTimeout(12000);

  char payload[280];
  uint32_t lastHist = 0;
  bool prevFall = false;
  for (;;) {
    if (WiFi.status() == WL_CONNECTED && cHas) {
      CloudSnap s;
      portENTER_CRITICAL(&cMux);
      s = csnap;
      portEXIT_CRITICAL(&cMux);

      snprintf(payload, sizeof(payload),
               "{\"heart_rate\":%d,\"spo2\":%d,\"temperature\":%.1f,"
               "\"rtc\":\"%s\",\"status\":\"%s\",\"gsm\":\"%s\","
               "\"updated\":{\".sv\":\"timestamp\"}}",
               s.hr, s.spo2, s.temp, s.rtc,
               s.fall ? "FALL DETECTED" : "OKAY",
               s.sms ? "MESSAGE SENT" : "ACTIVE");

      bool ok = false;
      String url = String("https://") + FB_HOST + "/PROTECT/sensors.json";
      if (https.begin(tls, url)) {
        https.addHeader("Content-Type", "application/json");
        int code = https.PUT((uint8_t *)payload, strlen(payload));
        ok = (code == 200);
        if (!ok) Serial.printf("[cloud] PUT %d %s\n", code, https.getString().c_str());
        https.end();
      } else {
        Serial.println(F("[cloud] begin failed"));
      }
      cOnline = ok;
      if (ok && (millis() - lastHist > CLOUD_HISTORY_MS || (s.fall && !prevFall))) {
        String hurl = String("https://") + FB_HOST + "/PROTECT/history.json";
        if (https.begin(tls, hurl)) {
          https.addHeader("Content-Type", "application/json");
          int code = https.POST((uint8_t *)payload, strlen(payload));
          Serial.printf("[cloud] history %d\n", code);
          https.end();
        }
        lastHist = millis();
      }
      prevFall = s.fall;
    } else {
      cOnline = false;
    }
    vTaskDelay(CLOUD_SEND_MS / portTICK_PERIOD_MS);
  }
}

void cloudBegin() { xTaskCreatePinnedToCore(cloudTask, "cloud", 16384, NULL, 1, NULL, 0); }
#else
void cloudPush(const Vitals &, const char *, bool, bool) {}
bool cloudOnline() { return false; }
void cloudBegin() {}
#endif

// ======================= MAIN =======================
static unsigned long tSample = 0, tLog = 0;
static Vitals cur{0, 0, 0, false, false};
static StatusLevel curSt = ST_NORMAL;
static bool sosLatched = false;
static bool fallLatched = false;
static bool smsSent = false;
static const unsigned long BUZZER_MS = 10000UL;  // 10 s beep per SOS/fall press
static unsigned long buzzStart = 0;             // 0 = no beep window active

static void cloudNow() {   // push the current state to the cloud right away
  char ts[20]; loggerGetClock(ts);
  cloudPush(cur, ts, fallLatched, smsSent);
}

void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.println(F("PROTECT all-in-one boot"));
  Serial.println(F("Expect I2C: 0x57 MAX + 0x68 RTC + 0x69 MPU(AD0->3V3)"));

  alertsBegin();
  displayBegin();
  bool okS = sensorsBegin();
  bool okL = loggerBegin();
  bool okM = motionBegin();
  bool okG = gsmBegin();
  Serial.printf("sensors:%d logger(RTC):%d motion:%d gsm:%d testmode:%d sd:%d cloud:%d\n",
                okS, okL, okM, okG, TEST_MODE_NO_SENSORS, USE_SDCARD, USE_CLOUD);

  char ts[20]; loggerGetClock(ts);
  SysInfo sys{loggerHasRtc(), motionHas(), gsmIsReady(), motionGetMag(), loggerGetRtcTemp()};
  displayShowFull(cur, ST_NORMAL, ts, false, sys);

  cloudBegin();   // starts Wi-Fi + Firebase in a background task
}

void loop() {
  unsigned long now = millis();

  sensorsPump();   // fast MAX FIFO pump every loop

  // NTP sync note: cloudTask configTime() already sets ESP32 system time,
  // which loggerGetClock() reads directly — no DS3231 needed.
  // DS3231 adjust below runs only on rigs that still have the chip.
  bool doRtc = false;
  NtpRtc nr{false, 0, 0, 0, 0, 0, 0};
  portENTER_CRITICAL(&cMux);
  if (ntpRTC.have) { nr = ntpRTC; ntpRTC.have = false; doRtc = true; }
  portEXIT_CRITICAL(&cMux);
  if (doRtc) {
    Serial.println(F("[time] NTP synced (PHT)"));
    if (hasRtc && rtc.now().year() < 2024) {
      rtc.adjust(DateTime(nr.y, nr.mo, nr.d, nr.h, nr.mi, nr.s));
      Serial.println(F("[RTC] set from NTP (PHT)"));
    }
  }

  if (alertsSosFlag()) {
    sosLatched = true;
    buzzStart = now;   // 10 s beep window per press
    alertsClearSos();
    loggerWriteFull(cur, ST_CRITICAL, "SOS_PRESS", motionGetMag(), gsmIsReady());
    if (gsmSendDistress(cur, ST_CRITICAL, "SOS")) smsSent = true;
    Serial.println(F("EVENT SOS_PRESS"));
  }

  if (now - tSample >= SAMPLE_MS) {
    tSample = now;
    cur = sensorsRead();

    if (motionCheckFall()) {
      fallLatched = true;
      sosLatched = true;   // a fall pages like SOS until power-cycle
      buzzStart = now;     // 10 s beep window per fall
      cloudNow();          // website shows FALL DETECTED immediately
      loggerWriteFull(cur, ST_CRITICAL, "FALL_EVENT", motionGetMag(), gsmIsReady());
      if (gsmSendDistress(cur, ST_CRITICAL, "FALL")) smsSent = true;
      Serial.println(F("EVENT FALL"));
    }

    curSt = alertsEvaluate(cur, sosLatched);
    alertsOutput(curSt, sosLatched);
    // Buzzer: 10 s per SOS/fall press; live temp-critical buzzes while critical.
    // SOS latch itself stays until reset (display/website unchanged).
    bool liveCrit = (!sosLatched && curSt == ST_CRITICAL);
    bool timedBuzz = (buzzStart != 0 && now - buzzStart < BUZZER_MS);
    digitalWrite(PIN_BUZZER, (liveCrit || timedBuzz) ? HIGH : LOW);
    if (curSt != ST_CRITICAL && !sosLatched) smsSent = false;

    char ts[20]; loggerGetClock(ts);
    SysInfo sys{loggerHasRtc(), motionHas(), gsmIsReady(), motionGetMag(), loggerGetRtcTemp()};
    displayShowFull(cur, curSt, ts, sosLatched, sys);

    Serial.printf("[DATA] %s HR:%.0f SpO2:%.0f T:%.1f RTC_T:%.1f IR:%lu MAG:%.2f ST:%d SOS:%d FALL:%d GSM:%d CLOUD:%d\n",
      ts, cur.hr, cur.spo2, cur.tempC, loggerGetRtcTemp(),
      (unsigned long)sensorsGetLastIr(), motionGetMag(),
      (int)curSt, sosLatched, fallLatched, gsmIsReady(), cloudOnline());

    static StatusLevel prevSt = ST_NORMAL;
    if (curSt == ST_CRITICAL) {
      loggerWriteFull(cur, curSt, sosLatched ? (fallLatched ? "FALL" : "SOS") : "ALERT",
                      motionGetMag(), gsmIsReady());
      if (prevSt != ST_CRITICAL && !sosLatched)
        if (gsmSendDistress(cur, curSt, "ALERT")) smsSent = true;
    }
    prevSt = curSt;

    cloudNow();   // upload this sample (the cloud task sends it in the background)
  }

  if (now - tLog >= LOG_MS) {
    tLog = now;
    loggerWriteFull(cur, curSt, "PERIODIC", motionGetMag(), gsmIsReady());
  }
}
