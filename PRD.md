# P.R.O.T.E.C.T. Core MVP — Product Requirements Document (PRD)
Version 1.0 | 48-Hour Build | Grade 12 Research

## 1. Purpose
Build a working, demonstrable prototype that satisfies the paper
`P.R.O.T.E.C.T._GRADE12_FINAL` Objectives 1-3 and RQ1-3 without IoT/cloud.

> Real-Time in this study = live TFT display + RTC timestamp + SD log + local buzzer/LED.
> Remote IoT alerts are explicitly OUT OF SCOPE per paper Scope and Limitations.

## 2. Users
- **Elderly (60+):** wears/places finger, reads big numbers, presses SOS button.
- **Caregiver/family:** reads TFT status colors, hears buzzer, reviews SD logs.
- **Researcher (you):** flashes firmware, calibrates, collects n=30 accuracy data, fills cost sheet.

## 3. Scope: IN vs OUT

### IN (48h MVP)
- ESP32 DevKit reads MAX30102 (HR + SpO2) and MLX90614 (temp) every 1s
- 3.5" TFT shows HR / SpO2 / Temp + clock + NORMAL/WARNING/CRITICAL/SOS
- DS3231 RTC timestamps every sample and event
- MicroSD CSV log for paper evidence
- Green/Yellow/Red LEDs + piezo buzzer + emergency push button (interrupt)
- LiPo + TP4056 + buck converter power, ABS enclosure
- Response time <3s, 4h+ continuous run, no `delay()` blocking

### OUT (parked as Future Work — fixes paper inconsistency)
- MPU6050 fall detection, NEO-6M GPS, SIM800L GSM/SMS
- Phone app, WiFi, cloud dashboard, blood pressure, ECG
- Clinical diagnosis claims (early-warning only)

## 4. Functional Requirements
- FR1: HR (BPM) + SpO2 (%) from MAX30102 via I2C 0x57, smoothed, 1s output.
- FR2: Body temp (°C) from MLX90614 via I2C 0x5A, emissivity 0.98 + offset calibrate.
- FR3: TFT layout: 3 big values, RTC time, status bar. Readable at arm's length.
- FR4: Threshold engine in `config.h`:
  - HR: 60-100 Normal, 50-59/101-120 Warning, <50/>120 Critical
  - SpO2: >=95 Normal, 93-94 Warning, <93 Critical
  - Temp: 36.1-37.2 Normal, 35.5-36.0/37.3-38.0 Warning, else Critical
  - Any Critical OR button press = latched buzzer + Red LED until acknowledged.
- FR5: SOS button on interrupt, debounced 200ms, logs `SOS_PRESS` event.
- FR6: SD log every 5s + on every alert: `timestamp,hr,spo2,temp_c,status,event`
- FR7: RTC survives reboot via coin cell. No WiFi required.

## 5. Non-Functional
- Accuracy: paired test vs reference oximeter/thermometer, n=30, report mean±SD.
- Consistency: 10 repeat reads on same subject, SD reported.
- Response: stopwatch from finger-remove / temp-spike / button to buzzer+display.
- Power: measure ESP32+peripherals current, estimate runtime from LiPo mAh.
- Usability: caregiver completes read + SOS + SD-eject unaided in 5 min.

## 6. Acceptance Criteria (maps to paper)
- RQ1: board flashes clean, I2C scan shows 0x57/0x5A/0x68, TFT hello passes.
- RQ2: 1s serial + SD updates stable 10 min, alert fires on demand.
- RQ3: `data/accuracy_n30.csv` filled, ready for t-test in Chapter 3.
- RQ4: usability checklist signed.
- RQ5: receipts summed vs Table 5 (₱5,675 baseline).

## 7. Risks for 48h
- Unknown TFT driver (ILI9488 vs ST7796) → mitigated by TFT_eSPI config doc.
- MAX30102 motion noise → finger holder + smoothing + still-hand protocol.
- MLX90614 reads ambient not core → fixed 5-10cm distance + offset calibration.
- SD/SPI pin conflict with TFT → separate CS pins, verified in WIRING.md.
