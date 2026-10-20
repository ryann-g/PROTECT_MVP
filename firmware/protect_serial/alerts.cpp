// alerts.cpp — single warning LED (Yellow) + buzzer for critical/SOS latched.
#include "alerts.h"

static volatile bool sosFlag = false;
static unsigned long lastIsr = 0;

static void IRAM_ATTR onSos() {
  unsigned long n = millis();
  if (n - lastIsr > DEBOUNCE_MS) { sosFlag = true; lastIsr = n; }
}

void alertsBegin() {
  // Single-LED build: only YELLOW (warning) wired. GREEN/RED unwired —
  // normal shows on TFT (green bar), critical shows on TFT (red bar) + buzzer.
  pinMode(PIN_LED_YELLOW, OUTPUT);
  pinMode(PIN_BUZZER, OUTPUT);
#if USE_SOS_BUTTON
  // GPIO34-39 are input-only: NO internal pull-up exists. Button MUST be
  // wired as: GPIO34 + 10k resistor to 3V3 + push-button to GND (FALLING = press).
  pinMode(PIN_BUTTON, INPUT);  // external 10k pull-up required, NOT INPUT_PULLUP
  attachInterrupt(digitalPinToInterrupt(PIN_BUTTON), onSos, FALLING);
#else
  // No button wired yet: leave pin alone so floating GPIO34 can't latch SOS.
  pinMode(PIN_BUTTON, INPUT);  // floating-safe, no ISR attached
#endif
  digitalWrite(PIN_BUZZER, LOW);
}

bool alertsSosFlag() { return sosFlag; }
void alertsClearSos() { sosFlag = false; }

StatusLevel alertsEvaluate(const Vitals &v, bool sosLatched) {
  if (sosLatched) return ST_CRITICAL;
  bool crit = false, warn = false;
  if (v.hrValid) {
    if (v.hr < HR_LOW_CRIT || v.hr > HR_HIGH_CRIT) crit = true;
    else if (v.hr < HR_LOW_WARN || v.hr > HR_HIGH_WARN) warn = true;
  }
  if (v.hrValid && v.spo2 < SPO2_CRIT) crit = true;
  else if (v.hrValid && v.spo2 < SPO2_WARN) warn = true;
  if (v.tempValid) {
    if (v.tempC < TEMP_LOW_CRIT || v.tempC > TEMP_HIGH_CRIT) crit = true;
    else if (v.tempC < TEMP_LOW_WARN || v.tempC > TEMP_HIGH_WARN) warn = true;
  }
  if (crit) return ST_CRITICAL;
  if (warn) return ST_WARNING;
  return ST_NORMAL;
}

void alertsOutput(StatusLevel st, bool sosLatched) {
  digitalWrite(PIN_LED_YELLOW, st == ST_WARNING);  // warning-only lamp
  // Active buzzer: steady ON when critical/SOS. Swap for tone() if passive.
  digitalWrite(PIN_BUZZER, (st == ST_CRITICAL || sosLatched) ? HIGH : LOW);
}
