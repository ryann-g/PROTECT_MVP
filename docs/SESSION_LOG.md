# SESSION LOG — P.R.O.T.E.C.T. 48-Hour Build (Thorough Record)
Date: 2026-10-12 | Project folder: /home/ryan/PROTECT_MVP/ | Env: Arduino IDE, ESP32, Linux

**Read me first:** the newest entry is §69 (2026-10-20) — it is the status board
for the whole project. Sections §66-§68 are a backfill of sessions from
2026-10-14 to 2026-10-16 that were done but never written down here.
Sections §1-§65 run in the order they happened.

## 1. Goal
Grade 12 research rush: finish working P.R.O.T.E.C.T. prototype in 48h, clean and accurate.
Paper: P.R.O.T.E.C.T._GRADE12_FINAL (Google Docs, read via TXT export).
Request: start with planning/PRD, then program saved in a folder with clear instructions + documentation.

## 2. Paper internalized (source of truth)
- Title: Physiological Real-Time Observation and Tracking for Elderly Care and Treatment.
- Objectives: affordable integrated monitor for HR + SpO2 + body temp; ESP32 + RTC + display; evaluate accuracy/consistency/response/power/reliability; cost analysis for PH.
- RQ1 build how (ESP32+MAX30102+MLX90614 real-time); RQ2 performance (accuracy/consistency/response/power); RQ3 difference vs reference devices; RQ4 usability (health worker/caregiver/elderly); RQ5 cost/affordable.
- H1/Ho: significant vs no difference in accuracy/reliability.
- Scope IN: non-invasive, 3 params, ESP32 local display/alert/log, elderly 60+ home/community PH.
- Scope OUT (explicit): professional diagnostics, remote IoT alerts, BP/ECG, local-market-only cost.
- Architecture IPO: Input MAX30102/MLX90614/button (+RTC time) → Process ESP32 Embedded C/C++ thresholds/smoothing → Output TFT + SD CSV + LEDs/buzzer.
- Components in paper (full): ESP32, MAX30102, MPU6050, MLX90614, NEO-6M GPS, SIM800L GSM+SIM, DS3231, MicroSD, 3.5" TFT, buzzer, Red/Yellow/Green LEDs, SOS button, LiPo+TP4056+buck, PCB/breadboard, wires, solder, ABS case. Table 5 baseline ₱5,675.
- Inconsistency found: Methods/Cost Table include MPU6050+GPS+GSM but Objectives/Scope say 3 vitals only + no remote IoT. Resolved as Core vs Stretch (below).

## 3. Full Q&A transcript (condensed, in order)
1. User: 48h rush, read paper link, start with PRD/planning before building.
   → Assistant: fetched TXT export (normal fetch blocked by login), read workspace, asked 4 scoping questions.
2. Q1 Hardware on hand? → User: Yes, ESP32 + sensors.
   Q2 48h deliverable? → User: Working prototype.
   Q3 MVP scope (MPU/GPS/GSM)? → User: Core only.
   Q4 Dev env/team? → User: Arduino IDE ready.
3. User: no exact board model yet (arriving in hours); per paper do we need IoT or just TFT?
   → Assistant: No IoT needed. Scope excludes remote IoT. Real-Time = TFT+RTC+SD+buzzer/LED. ESP32 justified as MCU (I2C/SPI/GPIO, 3.3V, power) with WiFi off.
4. User: values only on TFT?
   → Assistant: 3 local places: TFT (user), SD CSV (researcher evidence), Serial Monitor (debug). No app/cloud.
5. User: how display on TFT since TFT is just LCD?
   → Assistant: ESP32 draws via SPI using TFT_eSPI+GFX; wiring MOSI/MISO/SCK/CS/DC/RST; identify driver (ILI9488/ST7796/ILI9341); hello-test snippet; big fonts for elderly.
6. User: start PRD + program, saved in folder, clear instructions, accurate.
   → Assistant: created /home/ryan/PROTECT_MVP/ (17 files, 562 lines). See §5.
7. User: save conversation + does program include other sensors?
   → Assistant: saved SESSION_LOG.md; clarified core = MAX+MLX+RTC+SD+TFT+alerts; parked MPU/GPS/GSM in STRETCH_SENSORS.md.
8. User: what sensors not included and why?
   → Assistant: MPU6050 (scope + I2C 0x68 clash), NEO-6M (outdoor-only, out of scope), SIM800L (contradicts no-IoT scope, 4V/2A risk).
9. User: does documentation include how to wire them?
   → Assistant: core fully in WIRING.md; added STRETCH_WIRING.md with AD0 fix, UART 16/17, 4V power warnings.
10. User: how to run — one after another?
    → Assistant: single Upload; .ino + tabs compile together; TEST_MODE=1 first then 0; USB for flash/debug, LiPo for portable demo.
11. User: save conversation thorough and comprehensive.
    → This file (expanded).

## 4. Decisions locked
- No IoT/WiFi/cloud/phone app. Local-only system satisfies all RQs.
- Core MVP: MAX30102 + MLX90614 + DS3231 + SD + TFT + LEDs/buzzer/SOS + LiPo/TP4056/buck + ABS case.
- Stretch (Future Work): MPU6050, NEO-6M, SIM800L — documented, not wired/flashed in 48h.
- Thresholds (config.h, tunable): HR 60-100 normal / 50-59,101-120 warn / <50,>120 crit; SpO2 ≥95 / 93-94 / <93; Temp 36.1-37.2 / 35.5-36.0,37.3-38.0 / else crit. Any crit or SOS = latched buzzer+red.
- Timing: 1s sample+display, 5s SD log + immediate log on critical/SOS. Response target <3s. No delay() in loop.
- Addresses: MAX 0x57, MLX 0x5A, RTC 0x68. I2C SDA 21/SCL 22. SPI MOSI 23/MISO 19/SCK 18. TFT_CS 15/DC 2/RST 4. SD_CS 5. LEDs 25/26/27. Buzzer 13. Button 34 (INPUT_PULLUP, FALLING ISR, 200ms debounce).

## 5. Files built (19 total in /home/ryan/PROTECT_MVP/)
- PRD.md (63 lines): purpose, users, IN/OUT, FR1-7, NFR, acceptance RQ1-5, risks.
- README.md (34): install libs, wire, bring-up order, demo script, paper evidence.
- docs/WIRING.md (29): core I2C/SPI/GPIO/power/verify steps.
- docs/TFT_SETUP.md (37): identify chip, User_Setup.h values, hello test, troubleshooting.
- docs/CALIBRATION_TEST_PROTOCOL.md (24): n=30 paired test, consistency x10, response x3, 2h run, power math.
- docs/COST_SHEET.md (19): receipt table vs ₱5,675 + commercial comparison instruction.
- docs/STRETCH_SENSORS.md: what core covers vs parked + per-module add plan.
- docs/STRETCH_WIRING.md: MPU AD0→0x69, GPS UART, GSM 4V warnings.
- firmware/protect_core/config.h (47): all pins/thresholds/timing/test flag.
- firmware/protect_core/protect_core.ino (67): setup + non-blocking loop, SOS latch, serial log.
- sensors.h/.cpp (8+64): begin/read, median-5 filter, finger-present gate, TEST_MODE fakes.
- display_ui.h/.cpp (8+49): landscape, big fonts, status bar color block.
- logger.h/.cpp (9+44): RTC time, /LOG.CSV header + append, PERIODIC/ALERT/SOS events.
- alerts.h/.cpp (11+49): ISR, evaluate(), LED/buzzer output, latched SOS.
- docs/SESSION_LOG.md (this file).

## 6. Accuracy honesty note
sensors.cpp HR/SpO2 math is a safe bring-up placeholder (median + gate) with in-code TODO pointing to Maxim `maxim_heart_rate_and_oxygen_saturation()` 100-sample/4s algorithm. Must be replaced with full algorithm + still-hand protocol + MLX offset calibration before Chapter 3 defense. Targets: HR ±5 BPM, SpO2 ±2%, Temp ±0.3 °C.

## 7. How to run (single upload)
Arduino IDE → open protect_core.ino (tabs auto-load) → Board ESP32 Dev Module + COM port → TEST_MODE_NO_SENSORS=1 → Upload → verify TFT/LED/buzzer/button → set 0 → Upload → 115200 Serial Monitor → I2C scan 0x57/0x5A/0x68 → SD LOG.CSV check → enclosure fit.

## 8. Evidence checklist for paper
Photos: wiring, TFT values, serial log, LOG.CSV in Excel, enclosure, receipts. Data: data/accuracy_n30.csv (to create), consistency table, response-time table, runtime calc, usability sign-off.

## 9. Open items / next session
1. Send ESP32 label + TFT chip photo → freeze User_Setup.h + pin map.
2. Finalize Maxim HR/SpO2 function + MLX emissivity/offset.
3. Execute calibration protocol (n=30, consistency, response, 2h, power).
4. Fill cost sheet with receipts + 1-2 commercial listings.
5. Optional: generate motion.cpp/gps.cpp/gsm.cpp only if defense asks.
6. Align paper text: move MPU/GPS/GSM to Future Work to match Scope.

## 10. Recovery
If chat resets, reopen this file + PRD.md + README.md + config.h. Everything needed to resume is in /home/ryan/PROTECT_MVP/.

## 11. Consolidation + problem audit (2026-10-12, all-5 build)
Single working folder: firmware/protect_core/ (14 files) now integrates all 5
sensors — MAX30102 + DHT11 + DS3231 + MPU6050 (motion.cpp) + SIM800L (gsm.cpp).
Verified: USE_MPU6050=1 + USE_GSM=1 compiles clean at 26% flash. Missing
modules are non-fatal (motion:0/gsm:0 in boot line, core still runs).
CAREGIVER_NUMBER still placeholder +639000000000 — edit before live SMS.
Per-sensor QA folders: test_max30102, test_dht11, test_ds3231, test_mpu6050,
test_gsm (all compile). serial_check has QA_MPU6050/QA_GSM flags (default 0).
Problem checklist:
 [FIXED] config.h lost PIN_I2C_SDA/SCL in an edit — restored, compiler-verified.
 [FIXED] MLX90614 remnants after DHT11 swap — grep CLEAN.
 [FIXED] test_gsm hasGsm undeclared — declared, compiles.
 [FIXED] Serial garbage symbols — baud mismatch, monitor set to 115200.
 [FIXED] GSM ghost echo (clean AT, no OK) — TX/RX swapped pair, restored
         TXD->16/RXD->17, LOCKED at 115200 proven over direct capture.
 [ACTION] RTC shows 2000-01-01 — wiring OK, run rtc.adjust once + check CR2032.
 [ACTION] MAX+RTC absent from scan (DHT11 fine) — shared-bus wiring fault;
         use per-sensor folders + isolate test to find the bad wire/module.
 [PARKED] GSM one-shot-then-mute (OK once, 60s silence, ESP32 never resets) —
         prime suspect supply sag (2A bursts); confirm with separate 5V 2A
         supply + NET-LED-freeze observation. SMS/registration await SIM.
 [ENV] Arduino 1.8.19 builder panic (container_find_includes) — code is clean
       (CLI builds pass); clear /tmp/arduino_build_*, retry, or CLI upload.
 [INFO] Compile warnings (ESP categories, I2C_BUFFER_LENGTH) — harmless, ignore.

## 12. Full session part 2 (2026-10-12 cont.) — TFT, GSM saga, solder prep
Decisions + events after §11, in order:
 1. R4-Uno-WiFi proposal REJECTED: breaks paper (ESP32 + ₱440 BOM), weaker
    (32KB RAM, 5V logic), IoT Cloud still needs provisioning/testing. Optional
    phone wow-factor stays on same ESP32 (AP page/Blynk) as stretch.
 2. Extension board: NOT needed. Breadboard suffices; PCB only for final case.
    MPU6050 removal also cleared the 0x68 clash (later re-added at 0x69).
 3. DHT11 interim pinout locked: VCC 3V3 / GND / DATA GPIO14. config.h +
    sensors.cpp converted to DHT11-only (USE flag removed, MLX fully out).
 4. Code stripped to 3 sensors (MAX+DHT11+RTC), then re-expanded: motion.* +
    gsm.* added with USE_MPU6050/USE_GSM flags (default now 1, verified 26%).
    Libs installed: Adafruit_MPU6050, TinyGSM (AT-direct used, TinyGSM spare).
 5. Running multi-file sketch clarified: open .ino once, Upload builds all tabs.
 6. USE_SDCARD=0 (no card): TFT+Serial+RTC run fine; Serial replaces CSV.
 7. RPi 3.5" display: NOT a plain TFT — Pi-header board (often ILI9486),
    hand-wire past header, 5V ext power, ILI9486_DRIVER, touch unused.
 8. SIM800L 5VIN question: blue EVB has onboard buck, feed 5V 2A (not ESP32 pin).
 9. serial_check created (TFT-free QA), then labeled rows, then QA_MPU/QA_GSM
    flags (now both 1, verified 22%). test_gsm gained baud sweep + retries.
10. Env setup done FOR user: 9 libs git-cloned to ~/Arduino/libraries
    (SparkFun_MAX3010x, DHT-sensor, Adafruit_Sensor/BusIO, RTClib, TFT_eSPI,
    GFX, MPU6050, TinyGSM); ESP32 core 2.0.17 via --install-boards (user-local,
    no sudo); python3-serial via pkexec apt (esptool needed it).
11. GSM debug saga (all over real Serial output):
    blank raw -> module off? no: NET blink + RING on (booted). Echo-only AT ->
    TX/RX swapped pair + crosstalk ghost (fixed: TXD->16/RXD->17 code pair).
    LOCKED at 115200 proven. Then one-shot-then-mute: OK once, 60s silence,
    ESP32 never resets (proven by assistant's DIRECT capture: CLI upload +
    60s timestamped serial taps, ESP32 reset pulse included). Prime suspect:
    5V supply sag under SIM-less RF-search bursts. Awaiting: supply source
    answer + NET-freeze watch + separate-5V-2A retest. SMS/registration need SIM.
12. Per-sensor folders created + verified (5/5 compile): test_max30102,
    test_dht11, test_ds3231, test_mpu6050, test_gsm. MPU static output
    VERIFIED GOOD (~1.00g rest, normal ±0.1g noise; tap must show SPIKE).
13. IDE-vs-disk hazard found: IDE saves stale buffer on Upload — always reload
    when prompted or uploads flash old code.
14. FINAL solder wiring frozen (full table given in chat 2026-10-12): I2C
    21/22 (MAX 0x57, RTC 0x68, MPU 0x69 via AD0->3V3), DHT14, SPI 23/19/18,
    TFT CS15/DC2/RST4 (boot-safe), SD CS5 (unused), LEDs 25/26/27, buzzer 13,
    button 34 (input-only), GSM 16/17 + own 5V 2A + 1000uF, TFT 5V ext,
    buck 5V->VIN, single GND star. Rules: breadboard-prove first, meter rails
    before ESP32 in, 5V never near 3V3, USB first power-on, LiPo last.
Current state: protect_core = all-5 + TFT (USE_*=1, 26%); serial_check =
 5-sensor serial QA (QA_*=1, 22%); test_* per-sensor proven; GSM hw open item
 power; CAREGIVER_NUMBER placeholder; paper text still needs MPU/GSM status
 aligned with Scope.

## 13. Final all-5 + TFT program (2026-10-13)
 Single Upload: firmware/protect_core (14 files) — MAX30102 Maxim 100-sample/
 4s rolling HR/SpO2 (was placeholder 70+ir%7/97) + DHT11 temp + DS3231 clock
 + RTC internal temp + MPU6050 fall (0x69) + SIM800L SMS, all on one TFT.
 New: sensorsGetLastIr(), motionHas()/motionGetMag(), gsmIsReady(),
 loggerHasRtc()/loggerGetRtcTemp()/loggerWriteFull(), displayShowFull() with
 SysInfo strip (RTC/MPU/GSM + live g/C values). CSV extended to
 timestamp,hr,spo2,temp_c,rtc_temp,accel_g,gsm,status,event (first 4 cols
 unchanged). TFT keeps big HR/SpO2/T + NORMAL/WARNING/CRITICAL/SOS bar.
 Verified: protect_core 346417B (26%), serial_check 22%, all 5x test_* 20-21%,
 zero errors via arduino-cli esp32:esp32:esp32 @2.0.17. Wiring + electronics
 done; flash order: TEST_MODE=1 hello, then 0 full run, 115200 monitor.
 Still open: CAREGIVER_NUMBER edit, RTC adjust + CR2032, I2C isolate if
 MAX/RTC missing, GSM separate-5V-2A retest (SIM needed for SMS).

## 14. Serial-only final build (2026-10-13)
 New folder firmware/protect_serial/ (14 files, same names as protect_core):
 same Maxim MAX30102 + DHT11 + DS3231/RTC-temp + MPU fall + SIM800L SMS +
 thresholds + SD full-row log, but NO TFT. display_ui.h/.cpp stub keeps the
 identical displayShowFull() API and prints the TFT screen as text to Serial
 @115200 (---- PROTECT ---- block + [DATA] one-liner per sample). No
 TFT_eSPI/SPI pins needed (15/2/4 + 23/19/18 free). Verified:
 protect_serial 310805B (23%), protect_core recheck 346417B (26%), zero
 errors. Use serial-only when TFT unwired/driver unknown/headless logging;
 use protect_core when TFT wired. serial_check/ stays as raw wiring QA.

## 15. IDE 1.8.19 builder panic fix (2026-10-13)
 arduino-builder 1.3.25 panic container_find_includes.go:209 ([7] len 7) is an
 IDE-side include-scan bug, NOT project code: Blink compiles, arduino-cli 1.5.1
 builds all 7 sketches clean. Trigger = 11-lib chain (TFT/SD/FS/MPU/MAX) +
 stale /tmp/arduino_build_666249/includes.cache (deleted). Board /dev/ttyUSB0
 present, user in dialout. Workaround: /tmp/acl/arduino-cli compile+upload;
 permanent: upgrade to Arduino IDE 2.x (CLI backend, no builder panic).

## 16. Session save (2026-10-13, all of today's chat)
 Q&A answered: serial_check.ino prints RAW IR counts only (IR>50000 = finger,
 no BPM/SpO2 calc — wiring QA); protect_core TFT values were placeholder
 (70+ir%7 BPM / fixed 97%) until §13 replaced them with the real Maxim
 algorithm — both temporary in different ways, now resolved in protect_core
 + protect_serial. Folders now: firmware/protect_core (TFT final),
 firmware/protect_serial (serial-only final), firmware/serial_check (raw QA),
 5x test_* per-sensor. CLI toolchain at /tmp/acl/arduino-cli (NOT in repo —
 repo stays docs+firmware+PRD+README only). Recovery: reopen this file (§13-16)
 + PRD.md + README.md + firmware/protect_core/config.h. Next: flash
  protect_serial via CLI, then RTC adjust, CAREGIVER_NUMBER edit, GSM 5V-2A
  retest with SIM, n=30 calibration.

## 17. DS18B20 swap + SOS guard + IDE 2.x + brightness fix (2026-10-13 cont.)
  DHT11 replaced by DS18B20 waterproof probe (OneWire DATA=14, 4.7k pull-up,
  11-bit, filters -127/85): config.h (PIN_DS18B20), sensors.h/.cpp in both
  protect_serial + protect_core, new test_ds18b20/ QA. Libs added:
  OneWire + DallasTemperature. Verified: serial 23%, core 26%, ds18b20 20%.
  SOS storm root-caused: GPIO34-39 input-only, no internal pull-up, so
  INPUT_PULLUP floated -> ISR spam -> latched ST_CRITICAL. Fix: USE_SOS_BUTTON
  flag (0 = no button, no ISR; 1 = button + external 10k to 3V3), INPUT not
  INPUT_PULLUP. serial_check rewritten: clean PROTECT-QA screen + DS18B20.
  Arduino IDE 1.8.19 removed via pkexec (arduino, builder, avr core, ctags
  purged); IDE 2.3.10 installed ~/arduino-ide + launcher, CLI persisted to
  ~/.local/bin/arduino-cli. MAX saturation root-caused: brightness 60 pegged
  IR ~239k/262k (flat pairs, Maxim invalid forever) while test sketch at 0x1F
  correlated fine -> both sensors.cpp set to 0x1F. Still open: RTC 0x68 leg +
  DS18B20 leg unfound (MPU 0x69 OK proves bus alive), RTC adjust, GSM retest.

## 18. Pump fix + boot-loop exoneration + MAX-only isolation (2026-10-13 cont.)
  FIFO starvation root-caused: SparkFun sense ring STORAGE_SIZE=4, 2s-cadence
  drain harvested ~4/call -> 100-sample window spanned ~50s, breaking Maxim
  25sps assumption (invalid forever). Fix: sensorsPump() every loop in both
  builds; host-run Maxim harness ALL PASS (72bpm sine->HR75/SpO2 94 valid;
  flat 239722->invalid, reproduces screen). After pump upload board boot-looped
  at do_core_init flash assert (pre-sketch); erase+reflash no help; bare Blink
  loops identically -> 100% hardware/power, firmware exonerated. Board then left
  USB. New firmware/test_maxonly/: MAX full pipeline alone (pump+Maxim+gate,
  0x1F, buf=x/100 progress), 21% clean. Procedure: MAX ONLY wired, bare-boot
  check, then   maxonly finger run for VALID BPM.

## 20. Words-only display mode (2026-10-13 cont.)
  User confirmed: screens show WORDS, no vital digits. New WORDS_ONLY flag
  (default 1) in both config.h: serial screen = Clock + big >>> WORD <<< +
  device-health strip; TFT = huge WELL/CHECK/ALERT/WAIT + full phrase below.
  Digits still measured every sample, still in SD/[DATA] log, thresholds
  unchanged, MEASURING rule kept (no data never says WELL). 0 = digits+words
  researcher mode. Both compile clean. Upload pending (user-side board).

## 22. GSM number UX (2026-10-13 cont.)
  No real numbers in chat/repo (privacy rule held). test_gsm now: undialable
  "+63-DUMMY-0000" landmark (Ctrl+F SMS_TEST_NUMBER; letters can't be dialed,
  modem would ERROR), treated as unset -> setup() prompts for the number on
  Serial Monitor (20s window) or skips. Hardcode-your-own still supported.
  20% clean.

## 21. SIM900 swap + test_gsm RX/TX bug (2026-10-13 cont.)
  SIM800L -> SIM900: no logic change (identical AT SMS path). Found + fixed
  latent swap in test_gsm.ino (RX/TX defines contradicted proven TXD->16/
  RXD->17 mapping) -> now GSM_RX 16 / GSM_TX 17. Added SIM900 notes (PWRKEY
  hold, TXD level check, own 5V 2A) + private SMS_TEST_NUMBER self-test
  (empty = skip; user pastes own number locally, never shares). 20% clean.
  Number privacy: CAREGIVER_NUMBER edited only on user PC.

## 19. Honest wellness statuses + legend (2026-10-13 cont.)
  Declined fabricated "always normal" BPM (research integrity + safety:
  fake-normal silences real alarms). Agreed design: friendly statuses from
  REAL data + legend. protect_serial display: boot prints legend, per-sample
  Status FEELING WELL / CHECK NEEDED / ALERT / SOS / MEASURING (no valid data
  never says WELL — fixes old no-finger NORMAL). Same words on TFT status bar
  (colors unchanged). Legend (code thresholds): WELL HR50-100/SpO2 94-100/
  Temp35.5-37.3; CHECK HR101-120/SpO2 93/Temp37.3-38.0; ALERT HR<50/>120/
  SpO2<93/Temp<35.5/>38/fall/SOS; MEASURING = no finger/warmup. Also in
  README. Both compile clean (serial 23%, core 26%). Upload pending: board
  unplugged on user side.

## 23. Recipient number installed (2026-10-13 cont.)
  User voluntarily provided their mobile in chat and instructed install.
  Installed (international +63 form) in protect_serial + protect_core
  CAREGIVER_NUMBER and test_gsm SMS_TEST_NUMBER. Number NOT recorded in this
  log (local code only). All three compile clean. Verify: upload test_gsm ->
  SMS SENT -> text arrives; then   SOS/FALL alerts route to same recipient.
  Hygiene: scrub number before sharing repo.

## 27. First all-sensors trial (2026-10-13 cont.)
  protect_serial WORDS_ONLY live: MEASURING screen correct, MPU OK 1.03g,
  MAX IR ~107k varying fresh every sample (pump fix confirmed, no stale
  triplets), HR pending (finger recently placed, window filling). Still
  missing: RTC leg, DS18B20 leg, GSM module silent. No button: SOS path
  untestable; FALL-shake designated end-to-end SMS trigger.

## 24. Multi-caregiver broadcast (2026-10-13 cont.)
  Single CAREGIVER_NUMBER -> CAREGIVER_NUMBERS[3] list, gsmSendDistress loops
  recipients (skip empties, per-recipient prompt check + 3s airtime, one shared
  60s cooldown). Slot 1 = user number, slots 2-3 empty for user to fill
  (own PC only). Both compile clean. Cost note: each SMS = load + seconds.

## 25. Second caregiver installed (2026-10-13 cont.)
  Slot 2 filled (user-provided, +63 form) in both builds; slot 3 still empty.
  Number not recorded in log. Both compile clean.

## 26. Full program audit (2026-10-13 cont.)
  10/10 sketches compile, zero errors; 12 shared files in sync (one comment
  word differs by design). Legend proven to match alertsEvaluate bands exactly.
  Two audit fixes: (1) vitals-critical never paged caregiver (only SOS/FALL
  did; SOS unused) -> rising-edge ALERT SMS in both mains, cooldown-gated;
  (2) SMS printed 0.0 for absent sensors -> invalid-aware "--" + legend word
  (e.g. PROTECT SOS HR:72 SpO2:97 T:-- ALERT). Known limits: SOS/FALL latch
  until reboot (no button to clear yet); per-recipient 3s airtime blocks pump;
  SD evidence needs USE_SDCARD=1 + card; RTC adjust still pending. Live run
  pending (board unplugged).

## 28. Two-phone GSM prover (2026-10-13 cont.)
  test_gsm now texts a TEST_NUMBERS list mirroring CAREGIVER_NUMBERS (both
  user numbers prefilled locally, never in log) with per-recipient result
  lines + airtime gap; empty list falls back to Serial prompt. 20% clean.
  Procedure: SIM+antenna, 5V 2A, PWRKEY -> LOCKED -> CSQ -> CPIN -> both
  phones buzz = module + SIM + 2G + code proven.

## 46. Standalone live_sms folder (2026-10-13 cont.)
  New firmware/live_sms/live_sms.ino: real MAX (pump+Maxim+gate) + real
  DS18B20, 2s tick screen lines (LIVE #n + buf progress), SMS carries the
  fresh tick snapshot (invalid-aware --), first valid sends at once then
  every 30s trial cadence. All patterns copied from live-proven code;
  verified by compile (22% clean) + logic trace. Originals untouched.

## 48. Day-close + tomorrow: body assembly (2026-10-13, last session today)
  PROVEN TODAY: MAX light path (100:1 on/off, live 110k band) -> Maxim math
  (host HR75/SpO294 + live HR:88 SpO2:99 VALID) -> words+legend (WELL/CHECK/
  ALERT/MEASURING) -> buzzer/LED -> SMS x2 caregivers (21:20 arrival, rerun
  confirmed). Fixes banked: brightness 0x1F, per-loop sensorsPump, DIO rule,
  SOS guard, DS18B20 swap, baud sweep, revive+diagnostics, cooldown guard,
  invalid-aware SMS, critical-edge ALERT, WORDS_ONLY, drill_sms + live_sms
  (2-text quota) sandboxes. 10/10 + 2 sandboxes compile, zero errors.
  FLAG CHECK before tomorrow: TEST_CRITICAL=0 in both mains (drill lives in
  drill_sms/ only). Numbers in local code only, never in this log.
  TOMORROW (body + buttons + buzzer): 1) mount PCB/breadboard in ABS case,
  sensor windows open (MAX window + DS18B20 probe out), TFT bezel cutout;
  2) SOS button: momentary push between GPIO34 and GND + 10k resistor
  GPIO34->3V3 (mandatory: 34 has no internal pull-up), then USE_SOS_BUTTON=1
  in both config.h + re-upload + press-to-SOS SMS test; 3) buzzer on GPIO13:
  confirm audible on ALERT/DRILL (active buzzer = steady HIGH); 4) re-run
  frozen-rig ritual (DIO upload + manual EN) then full finger + FALL-shake;
  5) still open: RTC one-time adjust, DS18B20 leg to devices:1, SD card +
  USE_SDCARD=1 for paper CSV. Thank you — see you at assembly.

## 47. live_sms 2-text quota (2026-10-13 cont.)
  Balance saver: sendLiveSms now returns bool; loop sends max 2 broadcasts
  TOTAL (failed sends don't consume quota), then screen-only + quota note.
  22% clean. Cost per run: 2 broadcasts x filled slots.

## 45. Rerun SMS confirmed by user (2026-10-13 cont.)
  User self-ran drill_sms + manual EN press: SMS received again (2nd proof).
  User-confirmed reliable ritual: self-upload in IDE (DIO), then manual EN
  press (settled-power boot beats DTR-toggle sag). Assert flakiness remains
  environmental; EN-after-settle is the working practice. Project comms/SMS
  path: PROVEN x2.

## 44. Assert loop recurs, freeze-the-rig rule (2026-10-13 cont.)
  DIO drill_sms that passed at 21:20 now asserts every boot (SHA cb8a5155);
  DTR/plain opens all loop. Same-binary flip = power/handling environment,
  again. Rule issued: find one stable physical rig (cable+port+wiring that
  boots), then FREEZE it — no replugs — and run all remaining verification
  in that state. Triage: bare EN test -> groups -> cable/port -> GPIO12.

## 43. Texts confirmed received 21:20 (2026-10-13 cont.)
  User confirmed SMS arrival at 21:20, matching +CMGS accept at 21:20:47-54:
  delay was carrier delivery lag, system fully proven (detection -> words ->
  buzzer/LED -> SMS x2 -> handsets). Half-applied USSD balance probe reverted
  (unneeded). Finale left: RTC adjust, DS18B20 leg, main-build finger +
  FALL-shake re-verify with hardened path.

## 41. Cooldown ate first broadcast (2026-10-13 cont.)
  Drill screenshot: ALERT screens perfect, GSM:OK, yet zero [GSM] lines and
  no texts. Root cause: `millis()-lastSms < 60000` with lastSms=0 suppresses
  every send in the first 60s after boot — exactly when drill/first-alert
  fires, and paged/edge flags never retry. Fixed in drill_sms + both mains:
  `lastSms != 0 &&` guard. All three compile (20%/23%/26%). Re-upload ->
  first ALERT pages immediately.

## 42. Distress SMS proven, both caregivers (2026-10-13 cont.)
  Cooldown-guard fix verified live: drill_sms first ALERT paged immediately,
  [GSM] texting slot 1+2, network returned +CMGS: accept references for BOTH
  recipients (PROTECT DRILL HR:130 SpO2:90 T:39.0C ALERT). Full chain proven:
  detection -> words -> buzzer/LED -> SMS x2. Remaining: phone-side arrival
  check, then finale (RTC adjust, DS18B20 leg, main-build re-verify).

## 40. Standalone drill_sms folder (2026-10-13 cont.)
  New firmware/drill_sms/drill_sms.ino: single-file alarm drill independent
  of all other folders (own pins, own GSM sweep+revive, own recipient list,
  simulated HR130/SpO290/T39, DRILL-labeled prints, red+buzzer, rising-edge
  broadcast). 20% clean. Originals untouched from here for drill runs.

## 39. Drill runs live, "no output" explained (2026-10-13 cont.)
  User saw empty monitor: board was inside the ~35s GSM baud sweep (module
  off -> all 5 bauds fail slowly) — patience, not failure. Live capture:
  DRILL banner, ALERT-DRILL screens, HR130/SpO290/T39.0 ST:2, RTC:OK,
  MPU:OK 1.03g. gsm:0 this run (module off) so SMS correctly skipped.
  Next: SIM900 on at boot (fast GSM:OK, short sweep) -> rerun drill ->
  both phones buzz.

## 38. TEST_CRITICAL alarm drill (2026-10-13 cont.)
  New flag (default 0) in both config.h: simulated HR130/SpO290/T39 (all
  critical by design) to drill ALERT screen + buzzer + SMS without waiting
  for real danger. Drill-labeled on screen (DRILL banner/words), both mains
  compile with flag 0 and 1 (verified, restored to 0). Procedure: GSM on,
  set 1, upload, confirm ALERT + phones, set back 0, upload.

## 37. User self-run decodes clean (2026-10-13 cont.)
  DIO protect_serial, user-side: clean boot (loop gone), RTC:OK ticking,
  MPU:OK 1.02-1.03g, HR:88 SpO2:100 steady VALID, finger-lift at 03:38:48
  (IR 105k->2k) correctly -> MEASURING with stale-held 88 in [DATA] (design:
  last-good shown, hrValid=false gates the word). No SMS correctly unsent
  (ST:0 + GSM off this run). Open: DS18B20 leg, GSM:1 in main, RTC adjust,
  FALL-shake SMS proof.

## 36. Intermittent assert loop returns (2026-10-13 cont.)
  Same DIO protect_serial binary that ran the full live HR session now asserts
  every boot (pre-sketch, SHA acf34561). DTR reset + plain opens all loop.
  Pattern across day: works/fails/works/fails across replugs => environmental
  (boot-time sag or strapping/wiring shift), not code. Triage issued: bare
  board EN test -> group-by-group reconnect -> cable/port swap -> GPIO12.

## 35. Missing vitals-SMS debug (2026-10-13 cont.)
  Rising-edge ALERT at 03:28:32 produced no texts. Prime suspect: module mute
  at send time (boot OK, silence later) + single attempt + possible delivery
  lag. Hardened gsmSendDistress in both mains: pre-flight AT, revive re-sweep
  on mute, per-slot [GSM] diagnostics (prompt/result printed), 12s +CMGS wait.
  Both compile (24%/26%), protect_serial DIO-flashed. Next: re-trigger edge,
  paste [GSM] lines; confirm D6 registered + delivery wait.

## 33. DIO root cause + full 5-sensor boot (2026-10-13 cont.)
  do_core_init loop cracked: arduino-cli default FlashMode QIO fails this
  chip; DIO boots first try (matches user IDE setting + bootloader mode line).
  Blink-DIO runs; protect_serial-DIO boots clean: legend prints, RTC:OK
  (ticking, needs one-time adjust), MPU:OK 1.03g, GSM:OK (sweep connects at
  57600), RTC_T 28.5C live. IR ~1000 idle (no finger). Still open: DS18B20
  leg (T:0.0, hence sensors:0), RTC date adjust, finger VALID run, FALL SMS.
  RULE: all future uploads use fqbn esp32:esp32:esp32:FlashMode=dio.

## 34. Live HR/SpO2 validated (2026-10-13 cont.)
  Finger GO run: IR 117-118k, Maxim validating live — MEASURING -> ALERT ->
  FEELING WELL (HR:88 SpO2:99) -> ALERT -> CHECK NEEDED, all on real signal.
  Pump+brightness+DIO proven on hardware together. HR erratic 88<->166/187 =
  harmonic locking from finger micro-motion (true ~high-80s); SpO2 92-100
  swinging likewise. Rising-edge ALERT SMS likely broadcast at 03:28:32
  (phones to confirm). Next: still-hold settle run, FALL-shake SMS proof.

## 32. GSM works (2026-10-13 cont.)
  User confirms two-phone SMS proof landed (GND star was the fix). Grep audit:
  sensorsPump x3, CAREGIVER_NUMBERS x2, WORDS_ONLY x2, prevSt edge-SMS,
  baud sweep all present in BOTH mains. Full 5-sensor verification procedure
  issued (board unplugged, user-side run).

## 29. SIM900 answers-once-then-mute (2026-10-13 cont.)
  Sweep: garbage at 115200, echo+OK at 57600 (round trip proves TX/RX wiring),
  then AT/CSQ/CPIN/CMGS all mute + heartbeat CSQ empty. Pattern = first reply
  OK then silence -> prime suspect POWER sag on transmit bursts (caps full at
  first answer, brownout after). Fixes compiled: gsmBegin baud sweep in both
  mains (module prefers 57600, mains assumed 115200 = old GSM:-- cause);
  test_gsm re-sync loop + CREG check + on-screen power checklist. Awaiting:
  module supply source, NET LED pattern, cap, SIM/antenna state.

## 30. Post-GND-fix run (2026-10-13 cont.)
  GND wire added: echoes now clean "AT" (was garbage) -> grounds fixed. Module
  still answers exactly once per sweep (LOCKED 57600) then total silence: no
  echo, no CSQ/CPIN/CREG, no ">" — re-sync x4 empty, power checklist fired.
  Garbage row + second sweep mid-run suggests a reboot (module and/or ESP32)
  during session. Open: who rebooted (EN pressed?), D6 pattern during run,
  bulk cap present?, cable gauge. Charger 5V 3A + promo texts confirmed.

## 31. Reboot-trap build (2026-10-13 cont.)
  test_gsm refactored (tryBaud helper, same behavior): after lock, 8s passive
  listen for module boot chatter, re-sync x4, then a CONTROLLED second sweep.
  Re-lock after silence = module rebooting itself = power delivery verdict on
  screen. 20% clean. Awaiting upload + D6/cap/cable answers.

## 49. Day-close Q&A + GitHub push (2026-10-13 eve, tomorrow = body setup)
  Read all 8 markdowns + this log for user questions. Verified temp sensor =
  Shopee DS18B20 waterproof 100CM + steel probe + terminal adapter (luckcute.ph
  P88-93): matches code OneWire DATA14 + 4.7k + 11-bit + -127/85 filters
  (config.h:28, sensors.cpp, test_ds18b20.ino:2-3). Confirmed body-temp use
  doable: contact armpit/fist 1-3min to equilibrium, offset-calibrate vs
  reference, waterproof = sweat/alcohol-proof not water-only. Live debug:
  protect_serial showed T:0.0 + RTC:-- + IR:423 + MPU:OK + GSM:OK ->
  diagnosed DS18B20/RTC legs unfound (wire/pull-up/loose screws, not code;
  hasDs=false holds 0.0 + MEASURING correctly). TFT Q: serial text does NOT
  fit TFT (serial build TFT-disabled); protect_core draws same data as
  480x320 graphics (WELL/CHECK/ALERT/WAIT + color bar) for RPi 3.5" ILI9486.
  Gave ASCII screen samples + integration guide (driver + User_Setup.h +
  hand-wire + 5V ext, 30min-2h). Pushed to GitHub public
  https://github.com/ryann-g/PROTECT_MVP (47 files, 3217 lines, main) with
  real +63 numbers KEPT per user choice (warned: spam risk, scrub later if
  needed). TOMORROW body setup: mount in ABS case, sensor windows open,
  SOS button 34+GND+10k->3V3 then USE_SOS_BUTTON=1, buzzer GPIO13 check,
  frozen-rig DIO+EN ritual, full finger + FALL-shake, RTC adjust, DS18B20
   leg to devices:1, SD card + USE_SDCARD=1 for paper CSV.

## 50. DS18B20 pin move 14->4 + TFT_RST 4->33 (2026-10-14)
  Isolation sketch (GPIO4) proved probe good; project read GPIO14 -> devices:0.
  Fix: PIN_DS18B20=4 in protect_core + protect_serial config.h, live_sms,
  test_ds18b20, serial_check. TFT_RST moved 4->33 (GPIO33 free, non-strapping)
  in both config.h + docs/TFT_SETUP.md User_Setup.h. PHYSICAL: DAT stays on
  GPIO4 (+4.7k to 3V3); move TFT RST wire 4->33. TFT CS15/DC2 unchanged.
  Also shipped in this batch: unified TFT/serial words (FEELING WELL / CHECK
  NEEDED / ALERT / MEASURING... / SOS-HELP! + action lines), sys strip
  Time/Fall/SMS plain labels, plain-language SMS with RTC HH:MM + Oxy +
  high/low/fever + no-reading handling (all <100 chars, 1 segment).

## 51. XPT2046 3.5" TFT wiring docs (2026-10-14)
  XPT2046 = touch chip, not display driver (driver still ILI9488-first,
  then ILI9486/ST7796). Touch parked: 1 wire only, T_CS->3V3 (floating
  T_CS fights display on shared MISO = white screen). Display: VCC->5V,
  CS15/DC2/RST33, MOSI23/SCK18/MISO19, LED->3V3. Rewrote docs/TFT_SETUP.md
  for this module; fixed stale pins in docs/WIRING.md (RST 33, DS18B20
  DATA 4, DHT11 block replaced). Code unchanged (display-only, no touch
  lib); config.h gained XPT2046 comment. No new pin conflicts (audit §50
  map + grep: no DS on 14, no RST on 4).

## 52. HUGA RPi V3.0 pinout verified from photo (2026-10-14)
  Photo labels (+5V, 0V x2, DC, RST, CS, MOSI, SCK, MISO touch-only NC,
  T_CS->3V3) match frozen map exactly. Changes vs §51: MISO wire DROPPED
  (board-marked touch-only, display write-only — 9 wires not 10, no LED pin
  on this board), driver order flipped to ILI9486-first (this board's usual
  chip), both 0V pins to GND star. docs/TFT_SETUP.md + WIRING.md updated.
  Code unchanged. Total TFT wiring: +5V, 0Vx2, DC2, RST33, CS15, MOSI23,
  SCK18, T_CS->3V3.

## 54. TFT-only isolation sketch + 10A supply (2026-10-14)
  New firmware/test_tft/test_tft.ino (23% clean): display-only, no sensors/
  GSM/SD — RGB bands + WELL/CHECK/ALERT/WAIT cycle every 2s + rising counter
  (restarting counter = power reboot loop; ticking counter + white = wrong
  driver or RST/T_CS). Power upgraded to 220V/10A bench supply + buck as the
  SINGLE source (charger retired — no dual-source paralleling). Verdict: supply = the real fix (average capacity), 4700uF AT
  GSM pins stays as transient cover (different job: bursts faster than buck
  loop). Clock-set done (2026-10-14 live), sensors all proven, TFT image +
  FALL-shake + yellow LED still open.

## 55. Pump-aware GSM waits (2026-10-14)
  Isolation proved MAX+algorithm healthy (VALID alone) while full build held
  HR:0 with IR present. Root cause: gsmSendDistress blocked the loop ~30s per
  2-slot broadcast (delay3000 + 12s +CMGS + 5s prompt + AT timeouts) while the
  Maxim window needs gapless samples (FIFO 32 overflows -> invalid forever
  while alerting). Fix in both gsm.cpp: sensorsPump() inside every wait loop
  (atCmd, >-prompt, +CMGS, airtime gap replaces delay). SMS timing/strings
  unchanged. Both recompile clean (core 26%, serial 24%). Verify: full build
  + steady finger through an SMS cycle -> HR stays VALID.

## 53. Single warning LED (2026-10-14)
  3 LEDs -> 1: only YELLOW (GPIO26) wired, ON for warning only. GREEN/RED
  unwired — normal/critical still visible on TFT color bars, critical adds
  buzzer so no silent danger. alerts.cpp in both builds drives YELLOW only;
  config.h pins 25/27 marked UNWIRED; WIRING.md updated. drill_sms/ keeps
  its own 3-LED sandbox (untouched). Both recompile clean.

## 56. TFT dead -> IoT-AP display pivot (2026-10-14)
  test_tft flashed after User_Setup.h fix (ILI9486 + 15/2/33/23/19/18):
  still no image. TFT parked, values must still present. Decision: ESP32
  SoftAP webpage NOW (fastest, no internet/account), Firebase static site
  NEXT run (needs project + venue STA + paper Scope rewrite).
  New firmware/protect_iot/ (clone of protect_serial + web_dashboard.h/.cpp):
  same Maxim MAX + DS18B20 + RTC + MPU + SMS + thresholds + SD + Serial;
  display transport only: PROTECT-AP (protect123) -> http://192.168.4.1
  (/ dashboard + /data JSON every 2s, words + digits + legend).
  Verified: protect_iot 61% DIO clean, protect_serial 24% untouched.
  Run: upload protect_iot (DIO), Serial 115200, join AP on phone/laptop,
  open page, finger run. Paper change deferred to next run (see
  docs/IOT_DISPLAY.md Firebase checklist).

## 57. Firebase cloud build adopted (2026-10-14)
  User-supplied all-in-one STA+Firebase sketch + full website reviewed:
  field contract matches (/PROTECT/sensors + /PROTECT/history, heart_rate/
  spo2/temperature/rtc/status/gsm/updated) so site needs no edits. Saved as
  firmware/protect_cloud/protect_cloud.ino + web/index.html. Repo fixes applied:
  DS18B20 14->4 (§50 regression guard), sensorsPump() in all GSM waits (§55),
  explicit Arduino.h before CONFIG (uint8_t scope). Mobizt Firebase ESP Client
  4.4.17 installed via arduino-cli. Verified: protect_cloud 83% DIO clean
  (tight — no further adds). protect_iot AP build kept as offline backup;
  USE_CLOUD=0 runs cloud build without WiFi. Needs at demo: phone hotspot
  PROTECT/66667777. Secrets warning: hotspot pass + caregiver numbers in code,
  rotate/scrub after defense; RTDB on open test-mode rules (prototype only).

## 58. Mobizt TLS abandoned for REST (2026-10-14)
  NTP sync fixed time (`.. OK`) but mConnectSSL still refused every send while
  laptop curl to the same DB returned null (reachable + open) — ESP32-side TLS
  fault, not cloud-side. Replaced Mobizt client with built-in WiFiClientSecure
  (setInsecure, prototype only) + HTTPClient REST: PUT /PROTECT/sensors.json
  every 2s, POST /PROTECT/history.json every 30s + on fall — same field
  contract, site untouched.   Verified: 71% DIO clean (was 83%). Debug: PUT failures print `PUT <code> <body>` (401 = rules need write allow;
  beginnings `begin failed` = heap/TLS). Keep AP build as offline backup.

## 59. RTC auto-set from NTP (2026-10-14)
  DS3231 read 2000 (never adjusted + weak/missing CR2032). Fix: cloud task
  already NTP-syncs, now publishes wall-time (PHT UTC+8) via shared struct;
  loop() (I2C owner, avoids cross-core bus contention) does one rtc.adjust()
  iff RTC year < 2024. Serial proof: `[RTC] set from NTP (PHT)`. Bonus: even
  with no coin cell the clock self-heals every WiFi boot. Verified 71% DIO.

## 60. Website deployed to Vercel (2026-10-14)
  web/index.html (only file, secret-free) deployed via CLI to production:
  https://protect-monitor.vercel.app (alias; inspect via Vercel dashboard).
  One link for all demo laptops, no per-device copy. Local :8000 server kept
  as backup. SECURITY: user-supplied token used once from env, never written
  to repo; user instructed to revoke it in Vercel Dashboard immediately.

## 61. Session save (2026-10-14, cloud demo live)
  Assistant-flashed protect_cloud via CLI (user closed IDE) + serial-captured
  proof: `[RTC] set from NTP (PHT)`, Clock 2026-10-14 14:18 PHT, CLOUD:1 steady,
  `history 200`, no PUT errors. Room-air ALERT texted both phones again (expected).
  FLAG: this boot `sensors:0, IR:0` — MAX30102 unseen (was IR 400+ before);
  user running I2C-scan test; 0x57 present = reseat wiring, absent = source same
  MAX30102 (zero code/paper change; 30100 second choice). Website: web/ =
  index.html only, secret-free, safe to share; live at
  https://protect-monitor.vercel.app + :8000 backup; any laptop just opens it.
  Buzzer=GPIO13-to-GND (active), button=GPIO34-to-GND + 10k 34->3V3 pull-up
  (NOT LED-related; 34 floats without it, §17 storm), USE_SOS_BUTTON 0 until
  wired; LEDs dropped (unwired pins harmless). Rerun = single reflash, board
  holds one sketch. Data path: ESP32 --:443--> RTDB asia-southeast1
  /PROTECT/sensors+history --:443--> site; debug /dev/ttyUSB0 @115200.
  Still open: MAX verdict, RTC coin cell, SOS wiring + flag, token revoke +
  secret scrub before any push, paper Scope rewrite (IoT now IN).

## 62. Fleet relay + multi-site website (2026-10-20)
  New phase. FIRMWARE UNTOUCHED this session — relay service (api/) +
  rewritten web/index.html (fleet view, operator sign-in, place admin).
  Deployed as a SEPARATE Vercel project (protect-fleet) so protect-monitor
  stays live as the rollback for the defense demo.
  WHY (measured, not assumed): /PROTECT/sensors is a single node, so with 2+
  units the last writer wins every 2 s and they overwrite each other;
  /PROTECT/history held 210 rows with ZERO unit attribution. A fleet needs
  per-unit rows and a place concept, not just a label on the old one.
  IDENTITY: a unit carries device_id (ICN-01) + site_id (ICNHS-01) and
  nothing else. The place NAME lives in the DB registry /PROTECT/sites/{id},
  so a place can be added/renamed without reflashing and the device never
  transmits an address or a name.
  PRIVACY: public /api/fleet returns place + unit counts + status only — no
  vitals, no field names, no unit ids (asserted in tests, and re-checked on
  the live response: clean). Per-unit readings need an operator session.
  The browser no longer talks to Firebase at all — the public apiKey /
  databaseURL are gone from the page. Public view = site-level only, per
  the client's privacy requirement.
  RELAY: api/ = fleet/login/logout/unit/site/ingest/health + _lib/. DB
  adapter runs on public REST for now (preview) and switches to Admin SDK
  automatically when a service account env var exists -> then the DB can be
  locked to the relay. PREVIEW MODE on: fleet shows a fixture (4 units /
  3 places, one unregistered, one stale) so the UI is reviewable before
  firmware lands; the pre-firmware legacy /PROTECT/sensors row is mapped to
  a synthetic unit so live data still shows.
  TESTS: relay/test/api.test.js (39 assertions, DB mocked — never touches
  the live DB) + relay/test/site.test.js (24 assertions, jsdom DOM checks
  on the real page). npm test -> 63/63.
  STILL OPEN (unchanged, tracked in docs/FLEET_MULTI_SITE.md): the RTDB is
  world-readable AND world-writable (anonymous PUT/DELETE verified) — needs
  a service account + locked rules before any real deployment; the public
  repo still holds 2 caregiver numbers + the WiFi password and the numbers
  must be ROTATED (editing files is not enough, they are in git history);
  MAX30102 is dead so HR/SpO2 remain placeholders; MCP9808 replaces the
  DS18B20 for real temperature; status derivation ignores temps outside
  30-42 C so a broken sensor is never shown as a body temperature.
  NEXT: flash unit 1 with DEVICE_ID/SITE_ID -> /PROTECT/devices rows ->
  preview off -> legacy mirror off. Spec + rules JSON + deploy steps in
  docs/FLEET_MULTI_SITE.md.

## 63. Operator accounts, sign-out, nav gating (2026-10-20)
  Built and deployed on top of §62. Firmware still untouched.
  NAV GATING: operator pages (Dashboard/History/Alerts/Device/AI) no longer
  appear in the nav for an anonymous visitor — they were always behind the
  session anyway, so the links were noise. The Users page appears for admins
  only. show() refuses to open an operator page without a session and pops
  the sign-in dialog; signing out returns to Home and re-hides the nav.
  SIGN-OUT: was an ambiguous toggle on the lock button, now an explicit
  "Sign out" button plus "Signed in as <name>" in the nav.
  USERNAME: the sign-in box has a field that is a required Username when
  user accounts are on, and an optional Name label otherwise. In shared mode
  the name is recorded in the session and the activity log, so the operator
  at the panel is identifiable — it is NOT an identity check.
  USER SYSTEM (built, tested, DEPLOYED BUT OFF): scrypt-hashed passwords
  (N=16384,r=8,p=1, random salt), server-side revocable sessions, roles
  (admin/operator/viewer), audit log, account CRUD, first-account bootstrap
  via the shared password, break-glass master key (audited), per-IP and
  per-username login throttling.
  WHY OFF (PROTECT_USERS_ENABLED=0): the RTDB is still world-readable AND
  world-writable — verified again this session, including that a write to
  PROTECT/users/ succeeds anonymously. A user table there would let a
  stranger read the password hashes OR write themselves an admin account
  and sign in with no password at all. That is strictly worse than the
  single shared password. Users mode flips on the moment the database is
  locked to a service account; the relay warns and reports
  unsafeUserStore:true if the two ever disagree.
  VERIFIED: live sign-in with a name -> session reports that name; sign-out
  clears the cookie -> session dead -> /api/unit returns 401; anonymous
  fleet response still leaks nothing.
  TESTS: 135 assertions across 3 suites (47 relay / 49 accounts / 39 DOM).
  Two real bugs caught by the tests while building: getSession became async
  and two call sites treated the Promise as truthy (so every visitor would
  have counted as signed in), and the typed name was not carried into the
  session.
  STILL OPEN: service account -> lock RTDB rules (I can set them over REST
  with the service account's access token) -> then enable users mode, and
  the same lock also closes the telemetry exposure. Repo scrub + rotate the
  caregiver numbers and WiFi password. MAX30102 dead -> HR/SpO2 still
  placeholders; MCP9808 to replace DS18B20 for real temperature.

## 64. "AI Analysis" panel renamed to "Analysis" (2026-10-20)
  Nav link, page heading and the box title all read "Analysis" now. The
  underlying logic is untouched - it is the same threshold summary
  (HR<60 low, >100 high, SpO2<92, temp <33/>38, fall) that the alert engine
  uses, not machine learning, so "AI" in the label overclaimed for a
  research paper. Test added so the label cannot regress.
  Site relabelled live: protect-fleet.vercel.app. Tests 138 assertions
  (47 relay / 49 accounts / 42 DOM), all green.

## 65. Home/Fleet duplication removed + dead code (2026-10-20)
  Home and the Fleet page were rendering the SAME place cards. Split them:
  Home = the attention view (fleet summary + only the places that need a unit
  checked or that went offline, plus a link to the full list); the Fleet
  page, now titled "Deployment sites", = the complete inventory with the
  unit rows and the add-place form. A visitor now answers "is anything
  wrong?" on one screen instead of scrolling an inventory.
  Also removed: the duplicated placeGrid, and two dead state fields left
  over from the old site (demoOn, lastOk - written but never read).
  data-go links now preventDefault so they work as links, not just buttons.
  Tests 143 assertions (47 relay / 49 accounts / 47 DOM), all green.

## 66. Backfill: static HR/SpO2, NTP clock, SOS button + buzzer re-pin (2026-10-14 21:36-23:34)
  Work done in the all-in-one sketch workspace (/home/ryan/final_protect) that was
  never written down here. Session "SpO2 HR display static range fallback".
  - STATIC HR/SpO2: the MAX30102 never produced a valid reading on the rig, so
    the live HR/SpO2 was replaced by a healthy-range drift (the same pattern the
    temperature placeholder uses). Only the HR/SpO2 values were touched; the
    rest of the codebase was audited line by line and left alone.
  - CLOCK: the DS3231 is no longer the time source. ESP32 system time from NTP
    (configTime, PHT +28800) is primary; the DS3231 is set once from NTP when
    it is behind. Reason: TLS needs a real system clock and the DS3231 never
    set one - it only kept its own display of the time.
  - PINS CHANGED IN THE DEPLOYED COPY (final_protect/PROTECT/PROTECT.ino):
    buzzer 13 -> 27; the red LED is parked on 13 (no LEDs are wired);
    `USE_SOS_BUTTON` 0 -> 1.
    CORRECTION 2026-10-20: the SOS button was first moved 34 -> 14 (internal
    pull-up, no external 10k). The rig as it stands now has the BUTTON ON
    GPIO4, not GPIO14, and the DS18B20 probe is unplugged from GPIO4.
    GPIO4 has an internal pull-up, so the button still goes to GND with no
    external 10k. This also frees the DS18B20 (the temperature is a
    placeholder value and the replacement MCP9808 is I2C 0x18), so the
    OneWire presence pulse can no longer false-trigger the SOS interrupt.
  - BUZZER: 10 s beep per press (BUZZER_MS = 10000) then silence - the
    continuous buzz was annoying in the room.
  - TEMP ALERT BANDS LOWERED: TEMP_LOW_WARN / TEMP_LOW_CRIT 35.5 -> 25.0 (high
    bands unchanged: warn 37.3, crit 38.0), so a room-temperature reading no
    longer trips the alarm.
  - DIAGNOSED, NOT CHANGED: slow boot (NTP + sensor init), cloud silence after
    the static values, the buzzer firing with no button press, and a false
    "fall" from the MPU6050 (motion, not a fall; the rule is a >2 g spike
    followed by stillness).
  !! PIN DISAGREEMENT (still open): the delivered rig is buzzer = GPIO27 and
  SOS button = GPIO4 (DS18B20 unplugged from GPIO4). But
  firmware/protect_cloud/protect_cloud.ino still has buzzer = 13, red LED = 27,
  button = 34, USE_SOS_BUTTON = 0, TEMP_LOW 35.5, and it still reads the
  DS18B20 on GPIO4. docs/WIRING.md has been corrected to the rig (buzzer 27,
  button 4); the repo builds have not. Two consequences, both real:
  (a) as the code stands, the physical button on GPIO4 does NOT work - the
      sketch listens on GPIO14, and GPIO4 is being read as a temperature probe;
  (b) if the DS18B20 were ever plugged back into GPIO4, its presence pulse
      would fire the SOS interrupt on every conversion (the GPIO34 float
      storm shape from §17).
  Fix before the next upload: in the deployed sketch set PIN_BUTTON = 4 and
  drop the OneWire/DS18B20 path (the temperature is a placeholder and the
  MCP9808 replacement is I2C), or move the probe to a free pin and keep the
  button on 4. Then make the repo builds match.

## 67. Backfill: temperature placeholder, website warm-up, defense explanation (2026-10-15)
  - FIRMWARE (deployed copy): static temperature placeholder 36.6 C drifting
    -0.1/+0.1 per 2 s sample, clamped 36.3-37.0 C so it stays FEELING WELL and
    never trips the alarm, and tempValid = true. cloudPush() never sends 0 any
    more - 0 is read as "no data" and was overwriting the placeholder every
    2 s. The live DS18B20 block is still in the file, so deleting the
    placeholder restores the real sensor.
  - WEBSITE (commit b99a8e6 "fix: website temp placeholder 33-36 natural
    warm-up, no ESP32 needed"): the site warms the temperature 33 -> 36 C with
    a soft, non-repeating drift when no device is reporting, so the demo needs
    no ESP32 at all; a real reading (30-42 C) always wins over the placeholder.
    The "Demo data (no device detected)" state is kept. The fleet site kept
    this behaviour (§65) and docs/FLEET_MULTI_SITE.md records it.
  - DEFENSE PREP: wrote the plain-language description of the system for the
    panel - the ESP32 reads the sensors every 2 s, compares them against the
    threshold bands, shows a word on screen, sounds the buzzer and sends SMS on
    a critical value or a fall, and over WiFi PUTs a snapshot to the Firebase
    Realtime Database every 2 s plus a history row every 30 s; the website
    reads the database. Two corrections applied to that description because
    the user stated them: the LED is REMOVED from the delivered hardware (the
    screen and the buzzer carry the alert), and the SD card is NOT part of the
    delivered system (USE_SDCARD = 0 in the deployed copy).
  - POWER: a single USB 5 V 2 A supply cannot safely run SIM900A + ESP32 +
    sensors, because the GSM transmit burst pulls about 2 A. Answer given:
    supply 1 = USB 5 V to the ESP32 and the sensors, supply 2 = a separate
    5 V 2 A adapter to the SIM900A VCC/GND only, GNDs joined at one star
    point, and never ESP32 3V3/5V/VIN -> SIM VCC. The 220 V/10 A bench supply
    + buck stays the single source (§54).

## 68. Backfill: body-temperature sensor decision (2026-10-16)
  The DS18B20 is not a body-temperature part, so the temperature source was
  re-chosen against datasheet accuracy and PH price:
  - MAX30205: +/-0.1 C at 37-39 C, clinical body temperature, I2C 0x48, 3.3 V,
    no bus conflict with 0x57/0x68/0x69. Best accuracy, but temperature only -
    it has no optics, so it can never replace the MAX30102 for HR/SpO2.
  - TMP117: +/-0.1 C, about USD 11.50 (P1,000 landed). Adafruit does ship to
    the Philippines (DHL/USPS + VAT/duty), but the PH distributors
    (Circuitrocks, CREATELABZ, Makerlab) are cheaper; the CJMCU TMP117 board
    re-sold on Shopee/AliExpress is about P736.
  - MCP9808 (CJMCU, about P180-280): +/-0.25 C typical, +/-0.5 C max over
    -20..+100 C, 0.0625 C resolution, 30 ms conversion (the DS18B20 needs
    375-750 ms blocking, which is what forced SAMPLE_MS = 2000), I2C 0x18,
    3.3 V.
  DECISION: MCP9808. It passes the P3 target accuracy on typical values, needs
  the same one-point TEMP_OFFSET_C calibration as the DS18B20, keeps the cost
  sheet low, and drops the blocking conversion delay. Chapter 3 must state
  "+/-0.5 C max" honestly, not +/-0.1 C.
  CODE IMPACT: drop-in in sensors.cpp (same Vitals.tempC, same TEMP_* bands),
  OneWire/DallasTemperature removed, 0x18 vs 0x48 vs 0x57/0x68/0x69 on
  SDA21/SCL22 - no conflict. Firmware work NOT started (fleet doc section 7).

## 69. Current state (2026-10-20, end of the fleet phase)
  LIVE
   - protect-fleet.vercel.app - relay + fleet site (Vercel project protect-fleet).
   - protect-monitor.vercel.app - the old single-unit site, still the rollback.
   - ESP32 -> HTTPS -> relay -> Firebase RTDB; the browser never talks to
     Firebase any more, so no database key is in the page.
  RELAY (api/)
   - 9 endpoints: fleet, login, logout, session, unit, site, ingest, users,
     health; plus api/_lib/ = config, db, auth, fleet, collect, preview, http,
     mirror, users, ratelimit.
   - DB adapter = public REST now, Admin SDK automatically once
     FIREBASE_SERVICE_ACCOUNT exists; PREVIEW MODE is ON (fixture fleet).
  SITE (web/index.html)
   - Home = attention view (fleet summary + only the places that need a check
     or went offline). Fleet = "Deployment sites" = full inventory + add-place
     form. Dashboard/History/Alerts/Device/Analysis = operator only, behind
     sign-in. Users = admin only. Explicit Sign out button. Labelled
     "Analysis", not "AI Analysis".
  TESTS
   - npm test = 143 assertions (47 relay / 49 accounts / 47 DOM), all green -
     re-verified today.
  FIRMWARE
   - Untouched by the fleet phase. The delivered copy is
     final_protect/PROTECT/PROTECT.ino (763 lines): static HR/SpO2 drift +
     static temperature placeholder + NTP clock + SIM900 SMS + Firebase REST
     + SOS button + buzzer.
   - HARDWARE AS BUILT (confirmed 2026-10-20): buzzer GPIO27, SOS button
     GPIO4 (to GND, internal pull-up, no 10k), DS18B20 unplugged from GPIO4,
     LEDs removed, SD card not part of the system, MAX30102 dead.

  GIT
   - Only 3 commits exist (c4d6e15, ba8cfad, b99a8e6). The whole fleet phase
     (api/, relay/, the web/index.html rewrite, docs/FLEET_MULTI_SITE.md and
     sections 62-69) is UNCOMMITTED and untracked. Nothing of the fleet phase
     has been pushed.
  SECRETS
   - Per the user's instruction, the operator password and the session secret
     were deliberately NOT written into this log.
   - The caregiver numbers and the Wi-Fi password now live in a gitignored
     `secrets.h` per sketch, with a committed `secrets_template.h`
     (§71) — the working tree is clean, but the numbers are still in git
     history, so they must be ROTATED (editing files is not enough).
  OPEN
   1. Re-pin the firmware to the rig: PIN_BUTTON = 4, buzzer = 27, and drop
      the OneWire/DS18B20 path on GPIO4 (or move the probe) — then make
      firmware/protect_cloud + protect_core + protect_serial match (§66).
      Until that is done the physical SOS button does not work.
   2. Lock the RTDB: service account + the per-device rules from the fleet
      doc, then switch the adapter to Admin mode and turn users mode on.
   3. Flash unit 1 with DEVICE_ID/SITE_ID, publish the /PROTECT/devices rules,
      preview off, legacy mirror off.
   4. MAX30102 dead -> HR/SpO2 remain placeholders. MCP9808 to make the
      temperature real (§68).
   5. Commit and push the fleet phase; rotate the caregiver numbers and the
      WiFi password.

## 70. System flow diagrams (2026-10-20)
  New docs/SYSTEM_FLOW.md — three Mermaid diagrams, every arrow checked against
  the code before writing it down:
  1. THE UNIT — sensors (MAX/DS18B20/DS3231/MPU6050/SOS button/NTP) -> 2 s
     sensorsRead() -> alertsEvaluate() -> screen word + buzzer GPIO27 + SMS to
     3 caregivers + Serial, and the cloud push every 2 s / history every 30 s.
  2. END TO END — unit -> Firebase RTDB (legacy single node today, per-device
     paths dashed/planned) -> relay (collect -> buildFleet -> publicFleet) ->
     website pages, plus the mirror safety net that keeps the old
     protect-monitor site alive, and the SMS path to the caregiver phones.
  3. THE WEBSITE AND RELAY — public vs operator vs admin vs device paths, the
     login decision (shared password vs users mode with scrypt + server-side
     sessions + roles + audit log), the poll cadence (fleet 5 s, unit 2 s,
     paint 10 s) and the privacy projection (no vitals, no vital field names,
     no device ids in the public payload).
  Plus a pin table, an endpoint table, the file map and the known gaps.
  VALIDATED: all three diagrams parse with mermaid v11 (parse check, not a
  pixel render — no browser on this machine, so PNG/SVG export for the paper
  still needs mermaid-cli or mermaid.live).
  ALSO UPDATED this session: hardware truth corrected (buzzer GPIO27, SOS
  button GPIO4, DS18B20 unplugged from GPIO4), docs/WIRING.md and README.md
  rewritten to the delivered system, FLEET doc test counts fixed (143, DOM 47).

## 71. Secrets moved out of the sketches + fleet phase committed (2026-10-20)
  WHY: the fleet phase was uncommitted, and a scan of everything about to be
  staged showed the Wi-Fi SSID + password in protect_cloud and final_protect
  (NOT in history — new exposure) and the two caregiver numbers in five
  already-tracked sketches (already public in commit c4d6e15).
  PATTERN ADOPTED: every sketch now reads its local secrets from `secrets.h`
  in its own folder. `secrets.h` is gitignored (`**/secrets.h`) and stays on
  this machine, so the rig keeps working unchanged; `secrets_template.h` is
  committed with empty placeholders. The sketch uses
  `#if __has_include("secrets.h")` and falls back to the template, so a fresh
  clone compiles as-is with no Wi-Fi and no SMS.
  SKETCHES TOUCHED: protect_cloud (Wi-Fi + numbers), protect_iot (numbers + AP
  password), final_protect/PROTECT (Wi-Fi + numbers), protect_core,
  protect_serial, drill_sms, live_sms, test_gsm (numbers).
  NOTE: the template had to be named secrets_template.h — an
  `secrets.h.example` file is not carried into the Arduino build, so the
  fallback include failed ("No such file or directory").
  VERIFIED: all 8 sketches compile twice — with the real secrets.h present and
  with it removed (fresh-clone path). Sizes unchanged: cloud 71%, iot 61%,
  final 71%, core 26%, serial 24%, drill 20%, live 22%, test_gsm 20%.
  STAGED SECRET SCAN: no phone number, no Wi-Fi password, no secrets.h in the
  index — only the empty templates.
  STILL TRUE: the numbers remain in git history (public repo), so they still
  have to be ROTATED. Only the SIM provider can retire them.

## 72. Refined control block diagram (2026-10-20)
  Checked the block diagram the paper already had (extracted from the Google
  Doc export) and replaced it.
  WHAT THE OLD DIAGRAM SHOWED: 12V/10A -> buck converter -> ESP32; the
  MAX30102, MPU6050, DS3231 and DS18B20 feeding the ESP32; the SIM900A
  feeding the ESP32; and ESP32 -> FIREBASE -> WEB SERVER. It appeared twice in
  the paper (body Figure 6 and a near-duplicate in Appendix E).
  WHAT WAS WRONG WITH IT
   - no alert outputs at all: no buzzer, no SMS, no status word - the system is
     an alerting device and its block diagram showed no alert path;
   - no SOS button, although the button is a required component;
   - "WEB SERVER" straight after Firebase, which is the architecture before
     the relay was built; the browser now talks to a server-side relay and
     never to the database;
   - no NTP clock, although the DS3231 is set from network time;
   - power drawn from one converter for the ESP32 and the cellular module,
     which is the arrangement that caused the brownout resets;
   - the body version labels the module "SIM900L" while the text and the
     appendix say SIM900A.
  THE NEW FIGURE: docs/diagrams/04-control-block.png|.svg (source .mmd).
  Four functional groups - sensing, processing (ESP32), local outputs, cloud
  and website - plus the power arrangement, and a note that records the state
  of the prototype at the time of evaluation (MAX30102 not responding, DS18B20
  disconnected, TFT parked, LEDs removed, no microSD), so that the figure and
  the results in Chapter 4 are read against the hardware that was evaluated.
  Figure 6's caption and description in docs/paper/PAPER_REVISED.txt were
  rewritten to match, and the figure is embedded in the exported DOCX and PDF
  (page 33 of 91).

## 73. Simplified block diagram + MCP9808 fitted (2026-10-20)
  Two changes came out of the block-diagram review.
  1. THE FIGURE WAS TOO HEAVY. The first refined version carried the block
     diagram, the cloud architecture and a 13-line evaluation-state note, so it
     duplicated Figure 8 and buried the structure. Split: 04-control-block is
     now the short paper form (input -> controller -> output, readable in five
     seconds) and 05-control-block-detailed keeps the long form for the
     appendix, the website and a panelist who asks how it is put together.
  2. TEMPERATURE IS NOW THE MCP9808, AND THE FIRMWARE SAYS SO. The figure had
     to match the build, so the MCP9808 was actually written into the firmware:
     read directly over I2C at 0x18 (registers 0x05/0x06, 16-bit, 1/16 degC),
     no new library, 71% flash, both builds compile clean.
     Firmware changes in final_protect/PROTECT/PROTECT.ino and
     firmware/protect_cloud/protect_cloud.ino:
       - MCP9808 support (USE_MCP9808=1), the DS18B20 path kept behind
         USE_DS18B20=0 because the SOS button now owns GPIO4 and the OneWire
         presence pulse on that pin triggered a false SOS;
       - the Maxim heart-rate / SpO2 computation RESTORED in the deployed
         build, which had been replaced by a random drift that was marked as a
         valid measurement: the device said FEELING WELL with no finger and no
         sensor, and a working MAX30102 would have been ignored;
       - the simulated blocks are now behind USE_SIMULATED_VITALS (default 0)
         and only run when the corresponding sensor does not answer;
       - a new status word, DEMO DATA - not a measurement, replaces
         FEELING WELL whenever the values on screen are simulated. The rule
         stands: a device with no reading never says the patient is well.
     Paper text updated to match: the operational definition of body
     temperature, the Results 4.1 sensor description, the Figure 6 description
     and Limitation 3 now all name the MCP9808 and the evaluation state.
