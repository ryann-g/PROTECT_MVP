// web_dashboard.h — AP-local IoT display for P.R.O.T.E.C.T. (TFT fallback).
// Transport-only swap: same Vitals + thresholds + words as TFT/serial,
// presented at http://192.168.4.1 on phones/laptops. No internet needed.
#pragma once
#include <Arduino.h>
#include "sensors.h"
#include "config.h"

struct WebSnapshot {
  Vitals v;
  StatusLevel st;
  char clockStr[20];
  bool sos;
  bool fall;
  bool rtcOk;
  bool mpuOk;
  bool gsmOk;
  float accelG;
  float rtcTempC;
  unsigned long ir;
};

void webBegin();                    // starts PROTECT-AP + HTTP routes, non-fatal if WiFi fails
void webHandle();                   // call EVERY loop (non-blocking)
void webUpdate(const WebSnapshot &s);  // publish latest sample to / + /data
const char *webWordFor(StatusLevel st, const Vitals &v, bool sos);
