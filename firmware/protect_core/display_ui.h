// display_ui.h — big-font TFT layout for elderly + caregivers.
// Final all-5 build: vitals big, 5-sensor status strip small.
#pragma once
#include <Arduino.h>
#include "sensors.h"
#include "config.h"

struct SysInfo {
  bool rtcOk;
  bool mpuOk;
  bool gsmOk;
  float accelG;    // 0 when MPU absent
  float rtcTempC;  // NAN when RTC absent
};

void displayBegin();
void displayShow(const Vitals &v, StatusLevel st, const char *clockStr, bool sos);
void displayShowFull(const Vitals &v, StatusLevel st, const char *clockStr,
                     bool sos, const SysInfo &sys);
