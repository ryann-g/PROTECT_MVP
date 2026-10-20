// drill_sms.ino — STANDALONE alarm drill. Nothing else needed.
// Purpose: prove ALERT screen + buzzer/LED + caregiver SMS end to end WITHOUT
// any sensor. Vitals are SIMULATED danger (HR 130 / SpO2 90 / Temp 39.0) and
// every print says DRILL. This folder is independent: it never touches
// protect_serial / protect_core / test_* files.
// Run: SIM900 on first (PWRKEY till NET blinks), Upload (Flash Mode DIO),
// Serial Monitor @115200. Expect: DRILL banner -> ALERT screens + red +
// buzzer -> [GSM] lines -> BOTH phones buzz. No sensors required.
#include <Arduino.h>

// ---- Pins (same frozen map as main builds) ----
static const int PIN_LED_GREEN = 25;
static const int PIN_LED_YELLOW = 26;
static const int PIN_LED_RED = 27;
static const int PIN_BUZZER = 13;
static const int PIN_GSM_RX = 16;  // ESP32 RX <- module TXD
static const int PIN_GSM_TX = 17;  // ESP32 TX -> module RXD

// ---- Drill recipients (same slots as main builds; "" = skipped) ----
// DRILL_NUMBERS comes from secrets.h (local only, gitignored).
#if __has_include("secrets.h")
  #include "secrets.h"
#else
  #include "secrets_template.h"
#endif
static const uint8_t DRILL_COUNT = 3;
static const unsigned long SMS_COOLDOWN_MS = 60000;

static bool hasGsm = false;
static unsigned long lastSms = 0;

static String atCmd(const char *cmd, unsigned long timeout = 2500) {
  while (Serial2.available()) Serial2.read();
  Serial2.println(cmd);
  String res;
  unsigned long t0 = millis();
  while (millis() - t0 < timeout) {
    while (Serial2.available()) res += (char)Serial2.read();
    if (res.indexOf("OK") >= 0 || res.indexOf("ERROR") >= 0) break;
  }
  return res;
}

void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.println(F("!!!! DRILL_SMS — simulated danger, no real vitals !!!!"));
  pinMode(PIN_LED_GREEN, OUTPUT);
  pinMode(PIN_LED_YELLOW, OUTPUT);
  pinMode(PIN_LED_RED, OUTPUT);
  pinMode(PIN_BUZZER, OUTPUT);
  digitalWrite(PIN_BUZZER, LOW);

  // Baud sweep: SIM900 autobaud lands where it wants (often 57600).
  const long bauds[] = {115200, 57600, 38400, 19200, 9600};
  for (uint8_t b = 0; b < 5 && !hasGsm; b++) {
    Serial2.begin(bauds[b], SERIAL_8N1, PIN_GSM_RX, PIN_GSM_TX);
    delay(2000);
    for (uint8_t i = 0; i < 2 && !hasGsm; i++) {
      hasGsm = atCmd("AT", 2500).indexOf("OK") >= 0;
      delay(500);
    }
    if (!hasGsm) Serial2.end();
  }
  Serial.printf("DRILL GSM:%s\n", hasGsm ? "OK (will page on ALERT)" : "MISSING (drill screens only, no SMS)");
  if (hasGsm) { atCmd("AT+CMGF=1"); atCmd("AT+CSCS=\"GSM\""); }
}

static void sendDrillSms() {
  if (!hasGsm) { Serial.println(F("[GSM] module missing — SMS skipped.")); return; }
  // lastSms==0 (fresh boot) must NOT trigger cooldown — else the first
  // broadcast is silently eaten and paged=true never retries.
  if (lastSms != 0 && millis() - lastSms < SMS_COOLDOWN_MS) return;
  // Pre-flight revive (module often mutes after boot).
  if (atCmd("AT", 2000).indexOf("OK") < 0) {
    Serial.println(F("[GSM] mute, re-sweeping..."));
    hasGsm = false;
    const long bauds[] = {115200, 57600, 38400, 19200, 9600};
    for (uint8_t b = 0; b < 5 && !hasGsm; b++) {
      Serial2.begin(bauds[b], SERIAL_8N1, PIN_GSM_RX, PIN_GSM_TX);
      delay(1500);
      hasGsm = atCmd("AT", 2000).indexOf("OK") >= 0;
      if (!hasGsm) Serial2.end();
    }
    if (!hasGsm) { Serial.println(F("[GSM] revive failed — SMS skipped.")); return; }
    atCmd("AT+CMGF=1"); atCmd("AT+CSCS=\"GSM\"");
  }
  const char *msg = "PROTECT DRILL HR:130 SpO2:90 T:39.0C ALERT (test, not real)";
  for (uint8_t i = 0; i < DRILL_COUNT; i++) {
    const char *num = DRILL_NUMBERS[i];
    if (!num || num[0] == '\0') continue;
    Serial.printf("[GSM] texting slot %d...\n", i + 1);
    Serial2.print("AT+CMGS=\""); Serial2.print(num); Serial2.println("\"");
    unsigned long t0 = millis();
    bool prompt = false;
    while (millis() - t0 < 5000) {
      if (Serial2.available() && Serial2.read() == '>') { prompt = true; break; }
    }
    if (!prompt) { Serial.println(F("[GSM] no > prompt, next slot.")); continue; }
    Serial2.print(msg);
    Serial2.write(26);
    String res;
    unsigned long t1 = millis();
    while (millis() - t1 < 12000) {
      while (Serial2.available()) res += (char)Serial2.read();
      if (res.indexOf("+CMGS:") >= 0 || res.indexOf("ERROR") >= 0) break;
    }
    Serial.printf("[GSM] slot %d result:%s\n", i + 1, res.c_str());
    delay(3000);
  }
  lastSms = millis();
}

void loop() {
  static unsigned long t = 0;
  static bool paged = false;
  if (millis() - t < 2000) return;
  t = millis();

  // Simulated danger every sample: red + buzzer + DRILL word.
  digitalWrite(PIN_LED_GREEN, LOW);
  digitalWrite(PIN_LED_YELLOW, LOW);
  digitalWrite(PIN_LED_RED, HIGH);
  digitalWrite(PIN_BUZZER, HIGH);
  Serial.println(F("---- DRILL ----"));
  Serial.println(F(">>> ALERT - DRILL (simulated HR130/SpO290/T39.0) <<<"));
  if (!paged) {
    paged = true;  // first ALERT edge pages once (cooldown gates repeats)
    sendDrillSms();
  }
}
