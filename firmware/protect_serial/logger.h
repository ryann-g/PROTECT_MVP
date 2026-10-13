// logger.h — DS3231 RTC + MicroSD CSV logging for paper evidence.
#pragma once
#include <Arduino.h>
#include "sensors.h"
#include "config.h"

bool loggerBegin();
void loggerGetClock(char *out20);  // "YYYY-MM-DD HH:MM:SS"
bool loggerHasRtc();               // true if DS3231 found on I2C
float loggerGetRtcTemp();          // DS3231 internal temp C (case-temp proxy)
void loggerWrite(const Vitals &v, StatusLevel st, const char *event);
void loggerWriteFull(const Vitals &v, StatusLevel st, const char *event,
                     float accelG, bool gsmOk);  // 5-sensor evidence row
