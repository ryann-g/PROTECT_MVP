# IoT Display — AP fallback + Firebase cloud

## A: protect_iot (AP-local, no internet) — offline backup
Firmware: `firmware/protect_iot/` — same sensors/thresholds/SMS/SD/Serial as
`protect_serial`, display = webpage instead of dead TFT.

Run:
1. Arduino IDE → open `protect_iot.ino` → FlashMode DIO → Upload.
2. Serial Monitor 115200 → expect `[WEB] AP up: PROTECT-AP | open http://192.168.4.1`.
3. Phone/laptop → join WiFi `PROTECT-AP` (pass `protect123`) → browser → `http://192.168.4.1`.
4. Page shows big word (FEELING WELL / CHECK NEEDED / ALERT / MEASURING... / SOS-HELP!)
   + HR / SpO2 / Temp digits + clock + Time/Fall/SMS strip + legend. Auto-refresh 2s.
5. Defense demo = same finger + FALL-shake + buzzer/SMS as before; point camera at page.

Notes: web shows digits always (presentable mirror); Serial/TFT WORDS_ONLY rule unchanged.
AP fails safe: Serial/SD/SMS continue, `[WEB] softAP start FAILED` on Serial.

## B: protect_cloud + web/index.html (Firebase, needs hotspot) — main demo
Firmware: `firmware/protect_cloud/protect_cloud.ino` (all-in-one: sensors + SMS +
WiFi STA + Firebase RTDB push). Site: `web/index.html` (deploy to Firebase Hosting
or open locally with internet — it reads RTDB directly).

Run:
1. Router `XCM` reachable (2.4 GHz).
2. Arduino IDE → open `protect_cloud.ino` → FlashMode DIO → Upload (no extra
   libs needed — cloud uses built-in WiFiClientSecure + HTTPClient REST).
3. Serial 115200 → `PROTECT all-in-one boot` → `[cloud] Wi-Fi connected` →
   time sync OK → `[DATA] ... CLOUD:1` (PUT 200s stay silent; failures print).
4. Open `web/index.html` (hosted URL or local file) → `Receiving from device` + live cards.
   No device within 20s → page shows labeled demo data (never present as real).
5. Finger + fist + FALL-shake as usual; history rows land in `/PROTECT/history` every 30s + on fall.

Repo fixes vs the pasted original (do not revert): DS18B20 pin 14→4 (§50),
sensorsPump() inside all GSM waits (§55), explicit `#include <Arduino.h>` before
CONFIG (uint8_t scope), Mobizt client replaced by REST PUT/POST (§58).
Build: 71% flash DIO clean. `USE_CLOUD=0` runs everything minus WiFi/Firebase.

## Paper (next run)
- Move IoT from Scope-OUT to IN (Objectives/RQ5 cost + RQ4 usability for remote viewer
  + power re-measure with WiFi ~+150mA).
- Scrub `CAREGIVER_NUMBERS` + WiFi pass + Firebase keys before any public repo push
  (rule: credentials live on your PC only; rotate hotspot pass after defense).
