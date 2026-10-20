// test_gsm.ino — SIM800L / SIM900 ONLY (2G GSM, AT commands identical).
// Wire: VCC->own 5V 2A supply (NOT ESP32 pin) GND->common GND with ESP32,
//   module TXD->ESP32 pin 16, module RXD->ESP32 pin 17. SIM + antenna first.
//   SIM900 boards: HOLD the PWRKEY button ~2s until NET LED blinks
//   (slow blink = registered; fast blink = searching). If module TXD meters
//   above 3.6V, put a 10k/20k divider before ESP32 pin 16.
// Serial 115200. PASS: AT:OK + CSQ 10-31 + CPIN READY. CSQ 0/99 = no signal.
#include <Arduino.h>
#define GSM_RX 16  // ESP32 RX <- module TXD (proven mapping, do not swap)
#define GSM_TX 17  // ESP32 TX -> module RXD
// PRIVATE verification recipients — LOCATE HERE (Ctrl+F "TEST_NUMBERS").
// Mirrors CAREGIVER_NUMBERS in the main builds: one upload texts every listed
// phone so you can prove each caregiver receives it. Empty "" = slot skipped.
// Numbers live only in this file on your PC. If ALL slots are empty, setup()
// asks you to type one number in Serial Monitor instead.
// TEST_NUMBERS comes from secrets.h (local only, gitignored).
#if __has_include("secrets.h")
  #include "secrets.h"
#else
  #include "secrets_template.h"
#endif
static const uint8_t TEST_COUNT = 3;
static bool hasGsm = false;
static String at(const char *cmd, unsigned long timeout = 3000) {
  while (Serial2.available()) Serial2.read();
  Serial2.println(cmd);
  String r; unsigned long t0 = millis();
  while (millis() - t0 < timeout) {
    while (Serial2.available()) r += (char)Serial2.read();
    if (r.indexOf("OK") >= 0 || r.indexOf("ERROR") >= 0) break;
  }
  return r;
}
// One baud knock: begin, settle, up to 2x AT. True = OK seen (Serial2 left
// open at that baud). False = Serial2 ended.
static bool tryBaud(long b) {
  Serial2.begin(b, SERIAL_8N1, GSM_RX, GSM_TX);
  delay(2500);  // module boot + autobaud settle
  for (uint8_t i = 1; i <= 2; i++) {
    String r = at("AT");
    Serial.printf("[%ld] try%d raw:%s\n", b, i, r.c_str());
    if (r.indexOf("OK") >= 0) {
      Serial.printf("LOCKED at %ld baud\n", b);
      return true;
    }
    delay(800);
  }
  Serial2.end();
  return false;
}
void setup() {
  Serial.begin(115200); delay(800);
  Serial.println(F("TEST SIM800L/SIM900 boot — sweeping 115200/57600/38400/19200/9600/4800..."));
  const long bauds[6] = {115200, 57600, 38400, 19200, 9600, 4800};
  for (uint8_t b = 0; b < 6 && !hasGsm; b++) hasGsm = tryBaud(bauds[b]);
  if (!hasGsm) {
    Serial.println(F("No OK. Listening 10s for spontaneous module output..."));
    unsigned long t0 = millis();
    while (millis() - t0 < 10000) {
      while (Serial2.available()) Serial.write(Serial2.read());
    }
    Serial.println(F("\nListen done. Silence = module not executing (power sag/PWRKEY/fault)."));
    return;
  }
  // Passive listen BEFORE transmitting again: module boot chatter here
  // (RDY/CFUN/Call Ready gibberish) means it rebooted on its own.
  Serial.println(F("Listening 8s for spontaneous module chatter (reboot signs)..."));
  unsigned long tL = millis();
  while (millis() - tL < 8000) {
    while (Serial2.available()) Serial.write(Serial2.read());
  }
  Serial.println(F("\nListen done."));
  delay(500);
  // Re-sync: the first OK sometimes costs the module its breath (power sag
  // on reply bursts). Knock until it answers again before trusting the link.
  bool synced = false;
  for (uint8_t i = 1; i <= 4 && !synced; i++) {
    String chk = at("AT");
    Serial.printf("AT re-sync try%d:%s\n", i, chk.c_str());
    synced = chk.indexOf("OK") >= 0;
    if (!synced) delay(1500);
  }
  if (!synced) {
    // Controlled second sweep: if the module re-locks now, it rebooted itself
    // between (power delivery), not a baud/logic problem.
    Serial.println(F("One more full sweep (did the module reboot itself?)..."));
    bool relocked = false;
    for (uint8_t b = 0; b < 6 && !relocked; b++) relocked = tryBaud(bauds[b]);
    if (relocked) {
      Serial.println(F("RE-LOCKED after silence: module is rebooting itself."));
      Serial.println(F("Verdict: POWER delivery — short thick cable, 1000uF cap,"));
      Serial.println(F("snug barrel, common GND. Fix supply, not code. Stopping here."));
    } else {
      Serial.println(F("Module answered once then went mute: prime suspect POWER."));
      Serial.println(F("Check: own 5V 2A supply (not ESP32 pin), common GND, PWRKEY held,"));
      Serial.println(F("1000uF cap across module VCC/GND, antenna on, SIM seated."));
    }
    return;
  }
  for (uint8_t i = 1; i <= 3; i++) {
    String c = at("AT+CSQ");
    Serial.printf("CSQ try%d:%s (want +CSQ: 10-31)\n", i, c.c_str());
    if (c.indexOf("+CSQ:") >= 0) break;
    delay(1000);
  }
  for (uint8_t i = 1; i <= 2; i++) {
    String r = at("AT+CREG?");
    Serial.printf("CREG try%d:%s (0,1 = home registered, 0,5 = roaming)\n", i, r.c_str());
    if (r.indexOf("+CREG:") >= 0) break;
    delay(1000);
  }
  for (uint8_t i = 1; i <= 3; i++) {
    String s = at("AT+CPIN?");
    Serial.printf("SIM try%d:%s\n", i, s.c_str());
    if (s.indexOf("CPIN") >= 0 || s.indexOf("ERROR") >= 0) break;
    delay(1000);
  }
  // Collect recipients (skip empties; "DUMMY" counts as empty).
  String targets[3];
  uint8_t nT = 0;
  for (uint8_t i = 0; i < TEST_COUNT && nT < 3; i++) {
    String s = String(TEST_NUMBERS[i]);
    if (s.length() >= 10 && s.indexOf("DUMMY") < 0) targets[nT++] = s;
  }
  if (nT == 0) {
    Serial.println(F("Type a test mobile number (+63...) + ENTER within 20s, or wait to skip:"));
    String typed = "";
    unsigned long t0 = millis();
    bool done = false;
    while (!done && millis() - t0 < 20000) {
      while (Serial.available()) {
        char c = (char)Serial.read();
        if (c == '\n' || c == '\r') { done = true; break; }
        typed += c;
      }
    }
    typed.trim();
    if (typed.length() >= 10) targets[nT++] = typed;
  }
  if (nT == 0) {
    Serial.println(F("SMS self-test skipped (no destination number)."));
    return;
  }
  Serial.println(F("SMS self-test: text mode..."));
  at("AT+CMGF=1");
  at("AT+CSCS=\"GSM\"");
  for (uint8_t i = 0; i < nT; i++) {
    Serial.printf("Texting %d/%d...\n", i + 1, nT);
    while (Serial2.available()) Serial2.read();
    Serial2.print("AT+CMGS=\"");
    Serial2.print(targets[i]);
    Serial2.println("\"");
    unsigned long t0 = millis();
    bool prompt = false;
    while (millis() - t0 < 8000) {
      if (Serial2.available() && Serial2.read() == '>') { prompt = true; break; }
    }
    if (!prompt) { Serial.println(F("SMS: no > prompt (not registered?)")); continue; }
    Serial2.print(F("PROTECT test - module alive"));
    Serial2.write(26);  // Ctrl+Z sends
    String r = "";
    t0 = millis();
    while (millis() - t0 < 15000) {
      while (Serial2.available()) r += (char)Serial2.read();
      if (r.indexOf("OK") >= 0 || r.indexOf("ERROR") >= 0) break;
    }
    Serial.printf("SMS result:%s\n", r.c_str());
    if (r.indexOf("+CMGS") >= 0 || r.indexOf("OK") >= 0)
      Serial.println(F("SMS SENT — check that phone within a minute."));
    else Serial.println(F("SMS FAILED — check signal (CSQ), SIM load, 2G coverage."));
    delay(3000);  // airtime between recipients
  }
}
void loop() {
  if (!hasGsm) { delay(2000); return; }
  static unsigned long t = 0;
  if (millis() - t < 5000) return;
  t = millis();
  Serial.printf("heartbeat CSQ:%s\n", at("AT+CSQ").c_str());
}
