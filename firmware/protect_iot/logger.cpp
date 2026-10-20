// logger.cpp — CSV: timestamp,hr,spo2,temp_c,rtc_temp,accel_g,gsm,status,event
#include "logger.h"
#include <Wire.h>
#include <RTClib.h>
#include <SD.h>
#include <SPI.h>

static RTC_DS3231 rtc;
static bool hasRtc = false, hasSd = false;

bool loggerBegin() {
  Wire.begin(PIN_I2C_SDA, PIN_I2C_SCL);
  hasRtc = rtc.begin();
  // If RTC lost power, set once then re-flash with this line commented:
  rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));  // TEMP CLOCK-SET: re-comment after one boot!
#if USE_SDCARD
  SPI.begin(PIN_SPI_SCK, PIN_SPI_MISO, PIN_SPI_MOSI, PIN_SD_CS);
  hasSd = SD.begin(PIN_SD_CS);
  if (hasSd && !SD.exists("/LOG.CSV")) {
    File f = SD.open("/LOG.CSV", FILE_WRITE);
    if (f) { f.println("timestamp,hr,spo2,temp_c,rtc_temp,accel_g,gsm,status,event"); f.close(); }
  }
#else
  hasSd = false;  // no card in this build — TFT + Serial only
#endif
  return hasRtc;  // SD optional, RTC required for timestamps
}

bool loggerHasRtc() { return hasRtc; }

float loggerGetRtcTemp() {
  if (!hasRtc) return NAN;
  return rtc.getTemperature();
}

void loggerGetClock(char *out20) {
  if (!hasRtc) { strcpy(out20, "2000-01-01 00:00:00"); return; }
  DateTime n = rtc.now();
  snprintf(out20, 20, "%04d-%02d-%02d %02d:%02d:%02d",
    n.year(), n.month(), n.day(), n.hour(), n.minute(), n.second());
}

void loggerWrite(const Vitals &v, StatusLevel st, const char *event) {
  loggerWriteFull(v, st, event, NAN, false);
}

void loggerWriteFull(const Vitals &v, StatusLevel st, const char *event,
                     float accelG, bool gsmOk) {
#if !USE_SDCARD
  return;  // no card — Serial output in protect_core.ino is your record
#endif
  if (!hasSd) return;
  char ts[20]; loggerGetClock(ts);
  float rtcT = loggerGetRtcTemp();
  File f = SD.open("/LOG.CSV", FILE_APPEND);
  if (!f) return;
  f.print(ts); f.print(",");
  f.print(v.hr, 0); f.print(",");
  f.print(v.spo2, 0); f.print(",");
  f.print(v.tempC, 1); f.print(",");
  if (isnan(rtcT)) f.print("--"); else f.print(rtcT, 1);
  f.print(",");
  if (isnan(accelG)) f.print("--"); else f.print(accelG, 2);
  f.print(",");
  f.print(gsmOk ? 1 : 0); f.print(",");
  f.print((int)st); f.print(",");
  f.println(event);
  f.close();
}
