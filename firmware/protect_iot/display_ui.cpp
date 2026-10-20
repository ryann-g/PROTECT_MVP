// display_ui.cpp — serial-only: renders the same TFT screen as text lines.
// No TFT_eSPI, no SPI pins, no backlight wiring needed.
#include "display_ui.h"

void displayBegin() {
  Serial.println(F("PROTECT serial-only build: TFT disabled, Serial Monitor is the display."));
  Serial.println(F("--- STATUS LEGEND (config.h thresholds, shown only from REAL data) ---"));
  Serial.println(F("FEELING WELL: HR 50-100 bpm, Oxy 94-100%, Temp 35.5-37.3C -> You are OK. Rest."));
  Serial.println(F("CHECK NEEDED: HR 101-120, Oxy 93%, Temp 37.3-38.0C -> Please rest, recheck soon."));
  Serial.println(F("ALERT: HR <50 or >120, Oxy <93%, Temp <35.5 or >38.0, fall, SOS -> Sit down, call caregiver!"));
  Serial.println(F("MEASURING: no finger / warming up ~4-6s — never a status without data -> Keep finger still, wait."));
  Serial.println(F("SMS is sent only on SOS / FALL / new ALERT (60s cooldown). Warning/normal = no SMS."));
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
  // Words-only screen: the human word big + one action line, no vital digits.
  // ([DATA] log line in the .ino still carries digits for the record.)
  // Unified with TFT: FEELING WELL / CHECK NEEDED / ALERT / MEASURING... / SOS-HELP!
  Serial.println();
  Serial.print(F(">>> "));
  if (sos) Serial.print(F("SOS-HELP!"));
#if TEST_CRITICAL
  else if (st == ST_CRITICAL) Serial.print(F("ALERT-DRILL (simulated vitals)"));
#else
  else if (st == ST_CRITICAL) Serial.print(F("ALERT"));
#endif
  else if (st == ST_WARNING) Serial.print(F("CHECK NEEDED"));
  else if (v.hrValid || v.tempValid) Serial.print(F("FEELING WELL"));
  else Serial.print(F("MEASURING..."));
  Serial.println(F(" <<<"));
  if (sos) Serial.println(F("HELP! Caregiver called!"));
  else if (st == ST_CRITICAL) Serial.println(F("Sit down, call caregiver now!"));
  else if (st == ST_WARNING) Serial.println(F("Please rest, recheck soon."));
  else if (v.hrValid || v.tempValid) Serial.println(F("You are OK. Rest."));
  else Serial.println(F("Keep finger still, wait 4s."));
  Serial.println();
#else
  if (v.hrValid) { Serial.printf("HR: %.0f bpm | SpO2: %.0f %%\n", v.hr, v.spo2); }
  else { Serial.println(F("HR: -- | SpO2: -- (place finger, wait 4s)")); }
  if (v.tempValid) { Serial.printf("T: %.1f C", v.tempC); }
  else { Serial.print(F("T: --.-C")); }
  if (!isnan(sys.rtcTempC)) { Serial.printf(" | RTC_T: %.1f C", sys.rtcTempC); }
  Serial.println();
#endif
  Serial.printf("Time:%s Fall:%s", sys.rtcOk ? "OK" : "--", sys.mpuOk ? "OK" : "--");
  if (sys.mpuOk) { Serial.printf(" %.2fg", sys.accelG); }
  Serial.printf(" SMS:%s\n", sys.gsmOk ? "OK" : "--");
#if !WORDS_ONLY
  Serial.print(F("Status: "));
  if (sos) Serial.println(F("SOS-HELP! - HELP! Caregiver called!"));
  else if (st == ST_CRITICAL) Serial.println(F("ALERT - Sit down, call caregiver now!"));
  else if (st == ST_WARNING) Serial.println(F("CHECK NEEDED - Please rest, recheck soon."));
  else if (v.hrValid || v.tempValid) Serial.println(F("FEELING WELL - You are OK. Rest."));
  else Serial.println(F("MEASURING... - Keep finger still, wait 4s."));
#endif
}
