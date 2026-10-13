# SESSION LOG — P.R.O.T.E.C.T. 48-Hour Build (Thorough Record)
Date: 2026-10-12 | Project folder: /home/ryan/PROTECT_MVP/ | Env: Arduino IDE, ESP32, Linux

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
