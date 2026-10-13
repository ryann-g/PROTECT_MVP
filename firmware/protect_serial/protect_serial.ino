// protect_serial.ino — P.R.O.T.E.C.T. Final all-5 SERIAL-ONLY build.
// Same sensors + thresholds + alerts + SD + SMS as protect_core, but NO TFT.
// Output: Serial Monitor @115200 is the display (see display_ui.cpp text screen).
// Use when: TFT not wired yet, TFT driver unknown, or headless logging run.
// Wiring: identical to protect_core MINUS TFT pins (15/2/4 + SPI 23/19/18 free).
// Single Upload builds all tabs in this folder. See README.md + docs/.
#include "config.h"
#include "sensors.h"
#include "display_ui.h"
#include "logger.h"
#include "alerts.h"
#include "motion.h"
#include "gsm.h"

static unsigned long tSample = 0, tLog = 0;
static Vitals cur{0, 0, 0, false, false};
static StatusLevel curSt = ST_NORMAL;
static bool sosLatched = false;
static bool fallLatched = false;

void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.println(F("PROTECT Final all-5 SERIAL-ONLY boot (no TFT)"));
  Serial.println(F("Expect I2C: 0x57 MAX + 0x68 RTC + 0x69 MPU(AD0->3V3)"));

  alertsBegin();
  displayBegin();
  bool okS = sensorsBegin();
  bool okL = loggerBegin();
  bool okM = motionBegin();  // false when USE_MPU6050=0 or MPU missing (non-fatal)
  bool okG = gsmBegin();     // false when USE_GSM=0 or module missing (non-fatal)
  Serial.printf("sensors:%d logger(RTC):%d motion:%d gsm:%d testmode:%d sd:%d\n",
    okS, okL, okM, okG, TEST_MODE_NO_SENSORS, USE_SDCARD);

  char ts[20]; loggerGetClock(ts);
  SysInfo sys{loggerHasRtc(), motionHas(), gsmIsReady(), motionGetMag(), loggerGetRtcTemp()};
  displayShowFull(cur, ST_NORMAL, ts, false, sys);
}

void loop() {
  unsigned long now = millis();

  sensorsPump();  // fast MAX FIFO pump every loop (fills Maxim window in ~4s)

  // SOS latch from button ISR
  if (alertsSosFlag()) {
    sosLatched = true;
    alertsClearSos();
    loggerWriteFull(cur, ST_CRITICAL, "SOS_PRESS", motionGetMag(), gsmIsReady());
    gsmSendDistress(cur, ST_CRITICAL, "SOS");
    Serial.println(F("EVENT SOS_PRESS"));
  }

  // 2s sample + serial screen + alert (SAMPLE_MS=2000: DS18B20 conversion + Maxim window)
  if (now - tSample >= SAMPLE_MS) {
    tSample = now;
    cur = sensorsRead();
    if (motionCheckFall()) {  // no-op when USE_MPU6050=0; else latches critical
      fallLatched = true;
      sosLatched = true;  // fall pages like SOS until power-cycle/button review
      loggerWriteFull(cur, ST_CRITICAL, "FALL_EVENT", motionGetMag(), gsmIsReady());
      gsmSendDistress(cur, ST_CRITICAL, "FALL");
      Serial.println(F("EVENT FALL"));
    }
    curSt = alertsEvaluate(cur, sosLatched);
    alertsOutput(curSt, sosLatched);

    char ts[20]; loggerGetClock(ts);
    SysInfo sys{loggerHasRtc(), motionHas(), gsmIsReady(),
                motionGetMag(), loggerGetRtcTemp()};
    displayShowFull(cur, curSt, ts, sosLatched, sys);

    Serial.printf("[DATA] %s HR:%.0f SpO2:%.0f T:%.1f RTC_T:%.1f IR:%lu MAG:%.2f ST:%d SOS:%d FALL:%d GSM:%d\n",
      ts, cur.hr, cur.spo2, cur.tempC, loggerGetRtcTemp(),
      (unsigned long)sensorsGetLastIr(), motionGetMag(),
      (int)curSt, sosLatched, fallLatched, gsmIsReady());

    // Log + page immediately on new critical (response-time evidence).
    // SOS/FALL already paged above; this covers vitals-driven criticals
    // (previously silent). Rising edge only; 60s cooldown gates repeats.
    static StatusLevel prevSt = ST_NORMAL;
    if (curSt == ST_CRITICAL) {
      loggerWriteFull(cur, curSt, sosLatched ? (fallLatched ? "FALL" : "SOS") : "ALERT",
                      motionGetMag(), gsmIsReady());
      if (prevSt != ST_CRITICAL && !sosLatched)
        gsmSendDistress(cur, curSt, "ALERT");
    }
    prevSt = curSt;
  }

  // 5s periodic log for Chapter 3 tables
  if (now - tLog >= LOG_MS) {
    tLog = now;
    loggerWriteFull(cur, curSt, "PERIODIC", motionGetMag(), gsmIsReady());
  }
}
