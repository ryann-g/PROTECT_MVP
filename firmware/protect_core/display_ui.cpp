// display_ui.cpp — requires TFT_eSPI configured per docs/TFT_SETUP.md.
// RPi 3.5" (ILI9486, 480x320 landscape): big vitals + status bar + sys strip.
#include "display_ui.h"
#include <TFT_eSPI.h>

static TFT_eSPI tft;

void displayBegin() {
  tft.init();
  tft.setRotation(1);  // landscape for bedside
  tft.fillScreen(TFT_BLACK);
}

static uint16_t statusColor(StatusLevel st) {
  if (st == ST_CRITICAL) return TFT_RED;
  if (st == ST_WARNING) return TFT_YELLOW;
  return TFT_GREEN;
}

void displayShow(const Vitals &v, StatusLevel st, const char *clockStr, bool sos) {
  SysInfo sys{true, false, false, 0.0f, NAN};
  displayShowFull(v, st, clockStr, sos, sys);
}

void displayShowFull(const Vitals &v, StatusLevel st, const char *clockStr,
                     bool sos, const SysInfo &sys) {
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setTextSize(2);
  tft.setCursor(10, 10);
  tft.print("PROTECT  ");
  tft.print(clockStr);

#if WORDS_ONLY
  // Words-only screen: one huge human word, no vital digits.
  // (Digits still measured + logged to SD every sample.)
  tft.setTextSize(5);
  tft.setCursor(20, 80);
  if (sos) tft.println("SOS-HELP!");
  else if (st == ST_CRITICAL) tft.println("ALERT");
  else if (st == ST_WARNING) tft.println("CHECK");
  else if (v.hrValid || v.tempValid) tft.println("WELL");
  else tft.println("WAIT...");
  tft.setTextSize(2);
  tft.setCursor(20, 150);
  if (sos) tft.println("SOS - HELP!");
  else if (st == ST_CRITICAL) tft.println("ALERT - check person");
  else if (st == ST_WARNING) tft.println("CHECK NEEDED");
  else if (v.hrValid || v.tempValid) tft.println("FEELING WELL");
  else tft.println("MEASURING...place finger");
#else
  tft.setTextSize(4);
  tft.setCursor(10, 42);
  if (v.hrValid) { tft.print("HR:"); tft.print(v.hr, 0); tft.println("bpm"); }
  else tft.println("HR:--");

  tft.setCursor(10, 96);
  if (v.hrValid) { tft.print("SpO2:"); tft.print(v.spo2, 0); tft.println("%"); }
  else tft.println("SpO2:--");

  tft.setCursor(10, 150);
  if (v.tempValid) { tft.print("T:"); tft.print(v.tempC, 1); tft.println("C"); }
  else tft.println("T:--.-C");
#endif

  // 5-sensor sys strip (small, researcher/caregiver): RTC/MPU/GSM + live values.
  tft.setTextSize(2);
  tft.setTextColor(TFT_CYAN, TFT_BLACK);
  tft.setCursor(10, 204);
  tft.print(sys.rtcOk ? "RTC:OK" : "RTC:--");
  if (!isnan(sys.rtcTempC)) { tft.print(" "); tft.print(sys.rtcTempC, 1); tft.print("C"); }
  tft.print(sys.mpuOk ? " MPU:OK" : " MPU:--");
  if (sys.mpuOk) { tft.print(" "); tft.print(sys.accelG, 2); tft.print("g"); }
  tft.print(sys.gsmOk ? " GSM:OK" : " GSM:--");

  // Status bar at bottom: friendly words, same colors.
  // Honesty rule: no valid data never says WELL — it says MEASURING.
  tft.fillRect(0, 230, 480, 90, statusColor(st));
  tft.setTextColor(TFT_BLACK, statusColor(st));
  tft.setTextSize(3);
  tft.setCursor(20, 250);
  if (sos) tft.println("SOS - HELP!");
#if TEST_CRITICAL
  else if (st == ST_CRITICAL) tft.println("DRILL");
#else
  else if (st == ST_CRITICAL) tft.println("ALERT");
#endif
  else if (st == ST_WARNING) tft.println("CHECK NEEDED");
  else if (v.hrValid || v.tempValid) tft.println("FEELING WELL");
  else tft.println("MEASURING...");
}
