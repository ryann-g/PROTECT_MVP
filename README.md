# P.R.O.T.E.C.T. Core MVP — Build Guide

## What you have
- `PRD.md` — what to build and why (matches your paper, no IoT).
- `firmware/protect_core/` — Arduino sketch, open in Arduino IDE.
- `docs/` — wiring, TFT setup, test protocol, cost sheet.

## 0. Install (do now, no hardware needed)
1. Arduino IDE 2.x → Boards Manager → install `esp32 by Espressif`.
2. Tools → Board → `ESP32 Dev Module`. Leave defaults.
3. Library Manager → install:
   - `SparkFun MAX3010x Pulse and Proximity Sensor`
   - `Adafruit MLX90614`
   - `TFT_eSPI` (by Bodmer) + `Adafruit GFX`
   - `RTClib` (by Adafruit), `SD` (built-in)
4. For TFT_eSPI: open `libraries/TFT_eSPI/User_Setup.h` → follow `docs/TFT_SETUP.md` once your TFT chip is known.

## 1. Wire (after parts arrive)
Follow `docs/WIRING.md`. Then Tools → Serial Monitor 115200 → upload `I2C_Scan` example → expect `0x57, 0x5A, 0x68`.

## 2. Bring-up order (do not skip)
1. Flash `protect_core` with `TEST_MODE_NO_SENSORS 1` → TFT hello + LEDs + buzzer + button test.
2. Set back to `0` → test MAX30102 alone (Serial Plotter, steady finger).
3. Test MLX90614 alone (5-10cm forehead, still hand).
4. Enable SD + RTC → check `LOG.CSV` on PC.
5. Full integration → threshold demo → enclosure fit.

## 3. Demo script (defense-ready)
Finger on sensor → TFT updates ~1s → remove finger (SpO2 drop) or press SOS → Red LED + buzzer + TFT `ALERT/SOS` → show SD log line with timestamp.

## Status legend (shown only from REAL measured data)
- `FEELING WELL` (green): HR 50-100 bpm, SpO2 94-100%, Temp 35.5-37.3C
- `CHECK NEEDED` (yellow): HR 101-120, SpO2 93%, Temp 37.3-38.0C
- `ALERT` (red + buzzer): HR <50 or >120, SpO2 <93%, Temp <35.5 or >38.0, fall, SOS
- `MEASURING...`: no finger / warming up ~4-6s — never a status without data

## 4. Evidence for paper
- `docs/CALIBRATION_TEST_PROTOCOL.md` → fill `data/accuracy_n30.csv`
- `docs/COST_SHEET.md` → paste receipts
- Photos: wiring, TFT values, SD CSV open in Excel, enclosure
