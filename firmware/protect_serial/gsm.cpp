// gsm.cpp — needs no extra lib (raw AT over Serial2). Compiles to stubs when off.
#include "gsm.h"

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
  // Invalid-aware text: absent sensors read 0 — never send 0.0 as a
  // measurement. "--" instead, plus the legend word for the status.
  char msg[160];
  int n = snprintf(msg, sizeof(msg), "PROTECT %s HR:", why);
  n += v.hrValid ? snprintf(msg + n, sizeof(msg) - n, "%.0f", v.hr)
                 : snprintf(msg + n, sizeof(msg) - n, "--");
  n += snprintf(msg + n, sizeof(msg) - n, " SpO2:");
  n += v.hrValid ? snprintf(msg + n, sizeof(msg) - n, "%.0f", v.spo2)
                 : snprintf(msg + n, sizeof(msg) - n, "--");
  n += snprintf(msg + n, sizeof(msg) - n, " T:");
  n += v.tempValid ? snprintf(msg + n, sizeof(msg) - n, "%.1fC", v.tempC)
                   : snprintf(msg + n, sizeof(msg) - n, "--");
  snprintf(msg + n, sizeof(msg) - n, " %s",
           st == ST_CRITICAL ? "ALERT" : st == ST_WARNING ? "CHECK" : "OK");
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
    }
    if (!prompt) { Serial.println(F("[GSM] no > prompt, next slot.")); continue; }
    Serial2.print(msg);
    Serial2.write(26);  // Ctrl+Z sends
    String res;
    unsigned long t1 = millis();  // wait for network accept (+CMGS:) or error
    while (millis() - t1 < 12000) {
      while (Serial2.available()) res += (char)Serial2.read();
      if (res.indexOf("+CMGS:") >= 0 || res.indexOf("ERROR") >= 0) break;
    }
    Serial.printf("[GSM] slot %d result:%s\n", i + 1, res.c_str());
    delay(3000);  // airtime per SMS before next recipient
  }
  lastSms = millis();
}

#else
bool gsmBegin() { return false; }
bool gsmIsReady() { return false; }
void gsmSendDistress(const Vitals &, StatusLevel, const char *) {}
#endif
