# P.R.O.T.E.C.T — Physiological Real-Time Observation and Tracking for Elderly Care

ESP32 monitoring unit for elderly care: vitals → threshold bands → a plain word
on screen + buzzer + SMS to caregivers + a cloud snapshot on the operator
website.

Full history of how it got here: **[docs/SESSION_LOG.md](docs/SESSION_LOG.md)**
(newest entry §69 = status board). Fleet architecture: **[docs/FLEET_MULTI_SITE.md](docs/FLEET_MULTI_SITE.md)**.
Diagrams of the whole flow (unit → cloud → relay → website, and who can see
what): **[docs/SYSTEM_FLOW.md](docs/SYSTEM_FLOW.md)**. Requirements:
**[PRD.md](PRD.md)**.

## Live

| What | Where |
|---|---|
| Fleet site + relay (current) | https://protect-fleet.vercel.app |
| Single-unit site (legacy, rollback) | https://protect-monitor.vercel.app |
| ESP32 → relay → Firebase RTDB | `/PROTECT/devices/{id}/sensors` every 2 s, `.../history` every 30 s |

The browser never talks to Firebase. The public view shows places, unit counts
and status only — no vitals, no unit ids. Vitals need an operator session.

## What is in the box

```
firmware/protect_cloud/     deployed cloud build (WiFi + Firebase REST + SMS)
firmware/protect_core/      multi-file build, TFT variant (TFT is parked)
firmware/protect_serial/    multi-file build, Serial-only variant
firmware/protect_iot/       offline SoftAP fallback (no internet needed)
firmware/drill_sms/         alarm drill sandbox (simulated critical vitals)
firmware/live_sms/          live SMS sandbox (real MAX + DS18B20)
firmware/serial_check/      raw wiring QA
firmware/test_*/            one folder per sensor (MAX, DHT11, DS18B20, DS3231, MPU6050, GSM, TFT, maxonly)
final_protect/PROTECT/      single-file all-in-one build (the one flashed on the rig)
api/ + relay/               Vercel relay (9 endpoints + _lib) and its tests
web/index.html              the fleet website (no secrets in the file)
docs/                       wiring, TFT, calibration, cost, fleet, flow, session log
```

**Local secrets:** every sketch reads its Wi-Fi password and the caregiver
numbers from a `secrets.h` in its own folder. That file is **gitignored** and
stays on this machine; `secrets_template.h` is the committed placeholder, so a
fresh clone compiles as-is (no Wi-Fi, no SMS) and the real values never enter
git. Copy the template to `secrets.h` and fill it in.

## Sensors and outputs (as built)

| Part | Role | State |
|---|---|---|
| MAX30102 (0x57) | HR + SpO2, Maxim 100-sample/4 s | **sensor is dead** → HR/SpO2 are placeholder drift values |
| DS18B20 | body temp probe | **malfunctioning, now unplugged** (GPIO4 was taken by the SOS button) → static temp placeholder; MCP9808 chosen as the replacement (§68) |
| DS3231 (0x68) | RTC | present; the clock is NTP-synced (PHT) and the RTC is set from it |
| MPU6050 (0x69) | fall detection (>2 g spike + stillness) | working |
| SIM900A | SMS to up to 3 caregivers | working, proven end-to-end ×2 |
| SOS button (GPIO4) | manual panic | wired, `USE_SOS_BUTTON=1` — **the sketch still listens on GPIO14, see §66** |
| Buzzer (GPIO27) | local audible alarm | working, 10 s per alarm |
| LEDs | — | **removed from the rig**; the screen and the buzzer carry the alert |
| SD card | CSV log | **not part of the delivered system** (`USE_SDCARD=0`) |

Status words, from real measured data only: `FEELING WELL` / `CHECK NEEDED` /
`ALERT` / `MEASURING...` / `SOS-HELP!`. No valid reading never means "well".

## Build and flash

```bash
arduino-cli compile --fqbn esp32:esp32:esp32:FlashMode=dio <sketch-folder>
arduino-cli upload  --fqbn esp32:esp32:esp32:FlashMode=dio <sketch-folder> -p /dev/ttyUSB0
```

- **DIO flash mode is mandatory** on this board (QIO fails at `do_core_init`).
- Boot ritual: upload, then press **EN** after the power settles (DTR toggle
  causes a brownout boot loop).
- Serial Monitor at **115200**; the boot line prints which parts answered.
- Never `delay()` in the loop; every wait calls `sensorsPump()` so the Maxim
  window stays gapless.

Libraries (Arduino Library Manager): SparkFun MAX3010x, RTClib, Adafruit
MPU6050 + Unified Sensor + BusIO, OneWire + DallasTemperature, TinyGSM (spare),
TFT_eSPI + Adafruit GFX (TFT variant only), Mobizt Firebase ESP Client (legacy).

## Tests

```bash
npm test        # 143 assertions: relay 47 / accounts 49 / website DOM 47
```

The database is mocked in the tests — no test touches the live database.

## Before any real deployment (must be closed)

1. **The database is world-readable and world-writable.** Create a service
   account, publish the per-device rules from the fleet doc, then set
   `FIREBASE_SERVICE_ACCOUNT` and lock the rules to the relay.
2. **The caregiver numbers are still in git history** (the 2026-10-13 commit
   `c4d6e15` published them). The working tree is clean now — they live in the
   gitignored `secrets.h` — but history is public, so the numbers must still be
   ROTATED. Editing files is not enough; only the SIM provider can retire them.
3. **Only 3 commits exist.** The whole fleet phase (`api/`, `relay/`, the new
   `web/index.html`, `docs/FLEET_MULTI_SITE.md`) is uncommitted.
4. **The firmware pins do not match the rig** — the rig is buzzer 27 + button
   4 (DS18B20 unplugged), the repo builds say buzzer 13 + button 34 and still
   read the DS18B20 on GPIO4. The physical SOS button does not work until the
   sketch is re-pinned (§66).
5. HR/SpO2 and temperature are placeholder values until the sensors are fixed.

## Evidence for the paper

- `docs/CALIBRATION_TEST_PROTOCOL.md` → fill `data/accuracy_n30.csv`
- `docs/COST_SHEET.md` → paste receipts
- Photos: wiring, screen with values, Serial log, enclosure, receipts
- Data: consistency (×10), response time (×3), 2 h run, power calculation
