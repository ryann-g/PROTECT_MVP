// display_ui.h — serial-only final build: same API as TFT version, no display.
// All output goes to Serial Monitor @115200. No TFT_eSPI dependency.
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

void displayBegin();  // prints banner to Serial
void displayShow(const Vitals &v, StatusLevel st, const char *clockStr, bool sos);
void displayShowFull(const Vitals &v, StatusLevel st, const char *clockStr,
                     bool sos, const SysInfo &sys);
