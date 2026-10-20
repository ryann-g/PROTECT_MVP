// gsm.cpp — needs no extra lib (raw AT over Serial2). Compiles to stubs when off.
#include "gsm.h"
#include "logger.h"

#if USE_GSM
static bool hasGsm = false;
static unsigned long lastSms = 0;

static String atCmd(const char *cmd, unsigned long timeout = 2500) {
  while (Serial2.available()) Serial2.read();  // flush
  Serial2.println(cmd);
  String res;
  unsigned long t0 = millis();
  while (millis() - t0 < timeout) {
    while (Serial2.available()) res += (char)Serial2.read();
    if (res.indexOf("OK") >= 0 || res.indexOf("ERROR") >= 0) break;
    sensorsPump();  // keep MAX FIFO drained during AT waits (see below)
  }
  return res;
}

bool gsmBegin() {
  // SIM900 autobaud lands where it wants (observed 57600, not 115200):
  // sweep instead of assuming. Worst case adds ~20s to boot, then SMS works.
  const long bauds[] = {GSM_BAUD, 57600, 38400, 19200, 9600};
  for (uint8_t b = 0; b < 5 && !hasGsm; b++) {
    Serial2.begin(bauds[b], SERIAL_8N1, PIN_GSM_RX, PIN_GSM_TX);
    delay(2000);  // module boot + autobaud settle
    for (uint8_t i = 0; i < 2 && !hasGsm; i++) {
      hasGsm = atCmd("AT", 2500).indexOf("OK") >= 0;
      delay(500);
    }
    if (!hasGsm) Serial2.end();
  }
  if (hasGsm) {
    atCmd("AT+CMGF=1");           // text mode
    atCmd("AT+CSCS=\"GSM\"");     // charset
  }
  return hasGsm;
}

bool gsmIsReady() { return hasGsm; }

void gsmSendDistress(const Vitals &v, StatusLevel st, const char *why) {
  // PUMP-AWARE: every wait below drains the MAX FIFO via sensorsPump().
  // The Maxim window needs gapless samples; the old blocking delays froze the
  // pump for ~30s per 2-slot broadcast (FIFO overflow -> HR invalid forever
  // while alerting). SMS timing/behavior unchanged — only the idle side-effect.
  if (!hasGsm) return;
  // lastSms==0 (fresh boot) must NOT trigger cooldown — else the first
  // post-boot alert is silently eaten.
  if (lastSms != 0 && millis() - lastSms < GSM_SMS_COOLDOWN_MS) return;  // rate limit
  // Pre-flight: the module often goes mute after boot (supply sag on reply
  // bursts answers AT once, then silence). A mute module fails every CMGS
  // silently — so revive it first, else skip loudly.
  if (atCmd("AT", 2000).indexOf("OK") < 0) {
    Serial.println(F("[GSM] mute at send time, re-sweeping..."));
    hasGsm = false;
    const long bauds[] = {GSM_BAUD, 57600, 38400, 19200, 9600};
    for (uint8_t b = 0; b < 5 && !hasGsm; b++) {
      Serial2.begin(bauds[b], SERIAL_8N1, PIN_GSM_RX, PIN_GSM_TX);
      delay(1500);
      hasGsm = atCmd("AT", 2000).indexOf("OK") >= 0;
      if (!hasGsm) Serial2.end();
    }
    if (hasGsm) {
      Serial.println(F("[GSM] revived."));
      atCmd("AT+CMGF=1");
      atCmd("AT+CSCS=\"GSM\"");
    } else {
      Serial.println(F("[GSM] revive failed — SMS skipped."));
      return;
    }
  }
  // Plain-language text for lay caregivers (no "SpO2", no "--", no double
  // label): time + what happened + numbers with high/low/fever + one action.
  // All ASCII (GSM charset), always under 140 chars = 1 SMS segment.
  // Invalid sensors say "no reading", never 0.0. Drill build says DRILL.
  char clockFull[20] = {0};
  loggerGetClock(clockFull);  // "YYYY-MM-DD HH:MM:SS"
  char hhmm[6] = "--:--";
  if (clockFull[10] == ' ' && clockFull[13] == ':') {
    hhmm[0] = clockFull[11]; hhmm[1] = clockFull[12]; hhmm[2] = ':';
    hhmm[3] = clockFull[14]; hhmm[4] = clockFull[15]; hhmm[5] = '\0';
  }

  // Short vitals: "HR88, Oxy99%, Temp36.6C" (FALL/SOS/DRILL style).
  char vitalsShort[80];
  {
    char hrPart[32], tPart[24];
    if (v.hrValid) snprintf(hrPart, sizeof(hrPart), "HR%.0f, Oxy%.0f%%", v.hr, v.spo2);
    else snprintf(hrPart, sizeof(hrPart), "HR/Oxy no reading");
    if (v.tempValid) snprintf(tPart, sizeof(tPart), "Temp%.1fC", v.tempC);
    else snprintf(tPart, sizeof(tPart), "Temp no reading");
    snprintf(vitalsShort, sizeof(vitalsShort), "%s, %s", hrPart, tPart);
  }

  // Long vitals for ALERT: adds high/low/fever so caregiver knows which is bad.
  char vitalsLong[96];
  {
    char hrPart[32], oxPart[24], tPart[28];
    if (v.hrValid) {
      const char *q = (v.hr > HR_HIGH_CRIT || v.hr < HR_LOW_CRIT) ? " high" : "";
      if (v.hr < HR_LOW_CRIT) q = " low";
      snprintf(hrPart, sizeof(hrPart), "HR%.0f%s", v.hr, q);
    } else snprintf(hrPart, sizeof(hrPart), "HR no reading");
    if (v.hrValid) {
      const char *q = (v.spo2 < SPO2_CRIT) ? " low" : "";
      snprintf(oxPart, sizeof(oxPart), "Oxy%.0f%%%s", v.spo2, q);
    } else snprintf(oxPart, sizeof(oxPart), "Oxy no reading");
    if (v.tempValid) {
      const char *q = "";
      if (v.tempC > TEMP_HIGH_CRIT) q = " fever";
      else if (v.tempC < TEMP_LOW_CRIT) q = " low";
      snprintf(tPart, sizeof(tPart), "Temp%.1fC%s", v.tempC, q);
    } else snprintf(tPart, sizeof(tPart), "Temp no reading");
    snprintf(vitalsLong, sizeof(vitalsLong), "%s, %s, %s", hrPart, oxPart, tPart);
  }

  char msg[160];
#if TEST_CRITICAL
  // Drill build: never send a real-looking ALERT.
  snprintf(msg, sizeof(msg), "PROTECT DRILL %s Drill only, no danger. %s. Ignore.",
           hhmm, vitalsShort);
#else
  if (strcmp(why, "SOS") == 0) {
    snprintf(msg, sizeof(msg), "PROTECT SOS %s Help button pressed! Please go check now.", hhmm);
  } else if (strcmp(why, "FALL") == 0) {
    snprintf(msg, sizeof(msg), "PROTECT FALL %s Fall detected! %s. Please go check now.",
             hhmm, vitalsShort);
  } else {
    snprintf(msg, sizeof(msg), "PROTECT ALERT %s Check person now! %s. Please go check.",
             hhmm, vitalsLong);
  }
#endif
  for (uint8_t i = 0; i < CAREGIVER_COUNT; i++) {
    const char *num = CAREGIVER_NUMBERS[i];
    if (!num || num[0] == '\0') continue;  // empty slot = skipped
    Serial.printf("[GSM] texting slot %d...\n", i + 1);
    Serial2.print("AT+CMGS=\"");
    Serial2.print(num);
    Serial2.println("\"");
    unsigned long t0 = millis();  // wait for '>' prompt
    bool prompt = false;
    while (millis() - t0 < 5000) {
      if (Serial2.available() && Serial2.read() == '>') { prompt = true; break; }
      sensorsPump();
    }
    if (!prompt) { Serial.println(F("[GSM] no > prompt, next slot.")); continue; }
    Serial2.print(msg);
    Serial2.write(26);  // Ctrl+Z sends
    String res;
    unsigned long t1 = millis();  // wait for network accept (+CMGS:) or error
    while (millis() - t1 < 12000) {
      while (Serial2.available()) res += (char)Serial2.read();
      if (res.indexOf("+CMGS:") >= 0 || res.indexOf("ERROR") >= 0) break;
      sensorsPump();
    }
    Serial.printf("[GSM] slot %d result:%s\n", i + 1, res.c_str());
    { unsigned long air = millis();  // airtime per SMS (pump-aware, was delay)
      while (millis() - air < 3000) { sensorsPump(); delay(5); } }
  }
  lastSms = millis();
}

#else
bool gsmBegin() { return false; }
bool gsmIsReady() { return false; }
void gsmSendDistress(const Vitals &, StatusLevel, const char *) {}
#endif
