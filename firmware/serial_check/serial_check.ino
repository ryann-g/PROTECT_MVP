// serial_check.ino — Sensor QA WITHOUT TFT (Serial Monitor only).
// Purpose: verify MAX30102 + DS18B20 + DS3231-mini (+ optional MPU6050 + SIM800L)
// one at a time before running the full protect_serial / protect_core build.
// Board: ESP32 Dev Module. Serial Monitor @115200 after upload.
//
// Wiring (frozen map): MAX VIN->3V3 GND->GND SDA->21 SCL->22.
//   DS18B20 RED->3V3 BLACK->GND YELLOW->14 (+4.7k DATA->3V3).
//   DS3231-mini VCC->3V3 GND->GND SDA->21 SCL->22.
//   MPU6050 (if on) AD0->3V3, same SDA/SCL. GSM (if on) RX<-TXD TX->RXD + own 5V 2A.
//
// What good looks like (every 2s):
//   ---- PROTECT QA ----
//   Clock: HH:MM:SS (or seconds if RTC missing)
//   HR-sensor: IR=123456 FINGER:OK / FINGER:none
//   Temp: 36.5C body (hold probe in fist 30s) / ERR codes below
//   RTC:OK / RTC:MISSING | MPU:OK 1.00g / MPU:-- | GSM:OK / GSM:silent
//
// Decode:
//   IR>50000 = finger present. IR<20000 = no finger. IR=0 = MAX missing.
//   Temp -127 = probe disconnected (DATA14/VCC/GND/pull-up). 85 = reset, retry.
//   RTC:MISSING = SDA21/SCL22/3V3 or dead battery (clock shows seconds).
//   MPU:-- = AD0 not to 3V3 or SDA/SCL swap. GSM:silent = normal without SIM/5V2A.

// Set to 1 to QA future additions (wire them first).
#define QA_MPU6050 1
#define QA_GSM 1

#include <Arduino.h>
#include <Wire.h>
#include <MAX30105.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <RTClib.h>
#if QA_MPU6050
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#endif

#define DS_PIN 14
static OneWire oneWire(DS_PIN);
static DallasTemperature dsTemp(&oneWire);
static MAX30105 pox;
static RTC_DS3231 rtc;
static bool hasPox = false, hasRtc = false;
#if QA_MPU6050
static Adafruit_MPU6050 mpu;
static bool hasMpu = false;
#endif
#if QA_GSM
#define GSM_RX 16
#define GSM_TX 17
static bool hasGsm = false;
#endif

static void i2cScan() {
  Serial.println(F("-- I2C scan (want 0x57 MAX + 0x68 RTC + 0x69 MPU) --"));
  uint8_t found = 0;
  for (uint8_t a = 1; a < 127; a++) {
    Wire.beginTransmission(a);
    if (Wire.endTransmission() == 0) { Serial.printf("found 0x%02X\n", a); found++; }
  }
  if (!found) Serial.println(F("none — check SDA=21 SCL=22 + 3V3/GND"));
}

void setup() {
  Serial.begin(115200);
  delay(800);
  Serial.println(F("PROTECT serial_check boot (QA, no TFT)"));
  Wire.begin(21, 22);
  i2cScan();
  hasRtc = rtc.begin();
  Serial.printf("RTC:%s (MISSING = check SDA21/SCL22/3V3/battery)\n", hasRtc ? "OK" : "MISSING");
  // If clock wrong: uncomment next line, upload once, re-comment, re-upload:
  // rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  dsTemp.begin();
  dsTemp.setResolution(11);
  Serial.printf("DS18B20 probes:%d (0 = check VCC/GND/DATA14/4.7k pull-up)\n", (int)dsTemp.getDeviceCount());
  hasPox = pox.begin(Wire, I2C_SPEED_STANDARD);
  if (hasPox) pox.setup(0x1F, 4, 2, 100, 411, 4096);
  Serial.printf("MAX30102:%s\n", hasPox ? "OK (place finger steady)" : "MISSING (check VIN3V3/GND/SDA21/SCL22)");
#if QA_MPU6050
  hasMpu = mpu.begin(0x69, &Wire);  // AD0->3V3
  if (hasMpu) { mpu.setAccelerometerRange(MPU6050_RANGE_8_G); }
  Serial.printf("MPU6050:%s\n", hasMpu ? "OK (~1.00g at rest)" : "MISSING (check AD0->3V3)");
#endif
#if QA_GSM
  Serial2.begin(115200, SERIAL_8N1, GSM_RX, GSM_TX);
  delay(4000);  // module boot + autobaud settle
  for (uint8_t i = 1; i <= 2 && !hasGsm; i++) {  // 1st AT often only syncs baud
    Serial2.println("AT");
    String r; unsigned long t0 = millis();
    while (millis() - t0 < 3000) {
      while (Serial2.available()) r += (char)Serial2.read();
      if (r.indexOf("OK") >= 0) break;
    }
    hasGsm = r.indexOf("OK") >= 0;
    Serial.printf("GSM AT try%d:%s\n", i, hasGsm ? "OK" : "silent");
  }
  if (!hasGsm) Serial.println(F("GSM:silent — normal without SIM/5V2A, see test_gsm"));
#endif
  Serial.println(F("--- legend: IR = finger raw | Temp = DS18B20 body | ---"));
}

void loop() {
  static unsigned long t = 0;
  if (millis() - t < 2000) return;
  t = millis();

  char ts[20] = "millis";
  if (hasRtc) {
    DateTime n = rtc.now();
    snprintf(ts, 20, "%02d:%02d:%02d", n.hour(), n.minute(), n.second());
  } else snprintf(ts, 20, "%lus", millis() / 1000);

  dsTemp.requestTemperatures();
  float temp = dsTemp.getTempCByIndex(0);
  uint32_t ir = hasPox ? pox.getIR() : 0;

  // Clean QA screen — same order every sample, no column counting.
  Serial.println(F("---- PROTECT QA ----"));
  Serial.printf("Clock: %s\n", ts);
  Serial.printf("HR-sensor: IR=%lu %s\n", (unsigned long)ir,
    !hasPox ? "(MAX:MISSING)" : (ir > 50000 ? "FINGER:OK" : "FINGER:none (cover window, hold still)"));
  Serial.print("Temp: ");
  if (temp == -127.0) Serial.println("ERR(-127 probe disconnected)");
  else if (temp == 85.0) Serial.println("ERR(85 reset, retrying)");
  else Serial.printf("%.1fC body\n", temp);
  Serial.printf("RTC:%s MPU:", hasRtc ? "OK" : "MISSING");
#if QA_MPU6050
  if (hasMpu) {
    sensors_event_t a, g, t2;
    mpu.getEvent(&a, &g, &t2);
    float mag = sqrtf(a.acceleration.x * a.acceleration.x +
                      a.acceleration.y * a.acceleration.y +
                      a.acceleration.z * a.acceleration.z) / 9.81;
    Serial.printf("OK %.2fg", mag);
  } else Serial.print("--");
#else
  Serial.print("off");
#endif
#if QA_GSM
  Serial.printf(" GSM:%s\n", hasGsm ? "OK" : "silent");
#else
  Serial.println(" GSM:off");
#endif
}
