// display_ui.cpp — serial-only: renders the same TFT screen as text lines.
// No TFT_eSPI, no SPI pins, no backlight wiring needed.
#include "display_ui.h"

void displayBegin() {
  Serial.println(F("PROTECT serial-only build: TFT disabled, Serial Monitor is the display."));
  Serial.println(F("--- STATUS LEGEND (config.h thresholds, shown only from REAL data) ---"));
  Serial.println(F("FEELING WELL: HR 50-100 bpm, SpO2 94-100%, Temp 35.5-37.3C"));
  Serial.println(F("CHECK NEEDED: HR 101-120, SpO2 93%, Temp 37.3-38.0C"));
  Serial.println(F("ALERT: HR <50 or >120, SpO2 <93%, Temp <35.5 or >38.0, fall, SOS"));
  Serial.println(F("MEASURING: no finger / warming up ~4-6s — never a status without data"));
#if TEST_CRITICAL
  Serial.println(F("!!! TEST_CRITICAL DRILL ACTIVE — vitals simulated, set back to 0 !!!"));
#endif
}

void displayShow(const Vitals &v, StatusLevel st, const char *clockStr, bool sos) {
  SysInfo sys{true, false, false, 0.0f, NAN};
  displayShowFull(v, st, clockStr, sos, sys);
}

void displayShowFull(const Vitals &v, StatusLevel st, const char *clockStr,
                     bool sos, const SysInfo &sys) {
  // One compact screen block per sample — mirrors the TFT layout line-for-line.
  Serial.println(F("---- PROTECT ----"));
  Serial.print(F("Clock: ")); Serial.println(clockStr);
#if WORDS_ONLY
  // Words-only screen: the human word big, no vital digits.
  // ([DATA] log line in the .ino still carries digits for the record.)
  Serial.println();
  Serial.print(F(">>> "));
  if (sos) Serial.print(F("SOS - HELP!"));
#if TEST_CRITICAL
  else if (st == ST_CRITICAL) Serial.print(F("ALERT - DRILL (simulated vitals)"));
#else
  else if (st == ST_CRITICAL) Serial.print(F("ALERT - check person"));
#endif
  else if (st == ST_WARNING) Serial.print(F("CHECK NEEDED"));
  else if (v.hrValid || v.tempValid) Serial.print(F("FEELING WELL"));
  else Serial.print(F("MEASURING... (place finger, wait 4s)"));
  Serial.println(F(" <<<"));
  Serial.println();
#else
  if (v.hrValid) { Serial.printf("HR: %.0f bpm | SpO2: %.0f %%\n", v.hr, v.spo2); }
  else { Serial.println(F("HR: -- | SpO2: -- (place finger, wait 4s)")); }
  if (v.tempValid) { Serial.printf("T: %.1f C", v.tempC); }
  else { Serial.print(F("T: --.-C")); }
  if (!isnan(sys.rtcTempC)) { Serial.printf(" | RTC_T: %.1f C", sys.rtcTempC); }
  Serial.println();
#endif
  Serial.printf("RTC:%s MPU:%s", sys.rtcOk ? "OK" : "--", sys.mpuOk ? "OK" : "--");
  if (sys.mpuOk) { Serial.printf(" %.2fg", sys.accelG); }
  Serial.printf(" GSM:%s\n", sys.gsmOk ? "OK" : "--");
#if !WORDS_ONLY
  Serial.print(F("Status: "));
  if (sos) Serial.println(F("SOS - HELP!"));
  else if (st == ST_CRITICAL) Serial.println(F("ALERT - check person"));
  else if (st == ST_WARNING) Serial.println(F("CHECK NEEDED"));
  else if (v.hrValid || v.tempValid) Serial.println(F("FEELING WELL"));
  else Serial.println(F("MEASURING... (place finger, wait 4s)"));
#endif
}
