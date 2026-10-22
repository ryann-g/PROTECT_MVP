// serial_check.ino — Sensor QA WITHOUT TFT (Serial Monitor only).
// Purpose: verify MAX30102 + MCP9808 + DS3231-mini (+ optional MPU6050 + SIM800L)
// one at a time before running the full protect_serial / protect_core build.
// Board: ESP32 Dev Module. Serial Monitor @115200 after upload.
//
// Wiring (frozen map): MAX VIN->3V3 GND->GND SDA->21 SCL->22.
//   MCP9808 VCC->3V3 GND->GND SDA->21 SCL->22, A0/A1/A2->GND (0x18).
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
//   Temp -- = no MCP9808 on the bus. The DS18B20 probe is gone: GPIO4 is the
//   SOS button now and the OneWire presence pulse there caused false SOS.
//   RTC:MISSING = SDA21/SCL22/3V3 or dead battery (clock shows seconds).
//   MPU:-- = AD0 not to 3V3 or SDA/SCL swap. GSM:silent = normal without SIM/5V2A.

// Set to 1 to QA future additions (wire them first).
#define QA_MPU6050 1
#define QA_GSM 1

#include <Arduino.h>
#include <Wire.h>
#include <MAX30105.h>
// ---- Temperature sensor: MCP9808 (I2C 0x18) ----
#define MCP9808_ADDR  0x18  // A0/A1/A2 to GND
#define MCP9808_REG_TEMP_H 0x05
#define MCP9808_REG_TEMP_L 0x06
#define MCP9808_REG_TEMP_ID 0x07
#define MCP9808_ID_MICROCHIP 0x0C
#include <RTClib.h>
#if QA_MPU6050
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#endif

static MAX30105 pox;
static RTC_DS3231 rtc;
static bool hasPox = false, hasRtc = false, hasMcp = false;
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
  hasMcp = mcp9808Begin();
  Serial.printf("MCP9808:%s (check VCC/GND/SDA21/SCL22/A0-GND)\n", hasMcp ? "OK" : "MISSING");
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
  Serial.println(F("--- legend: IR = finger raw | Temp = MCP9808 body | ---"));
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

  float temp = hasMcp ? mcp9808ReadC() : NAN;
  uint32_t ir = hasPox ? pox.getIR() : 0;

  // Clean QA screen — same order every sample, no column counting.
  Serial.println(F("---- PROTECT QA ----"));
  Serial.printf("Clock: %s\n", ts);
  Serial.printf("HR-sensor: IR=%lu %s\n", (unsigned long)ir,
    !hasPox ? "(MAX:MISSING)" : (ir > 50000 ? "FINGER:OK" : "FINGER:none (cover window, hold still)"));
  Serial.print("Temp: ");
  if (isnan(temp)) Serial.println("ERR(no MCP9808 on the bus)");
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
