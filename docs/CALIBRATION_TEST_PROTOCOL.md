# Calibration + Test Protocol (for Chapter 3, RQ2-RQ3)

## A. Pre-conditions
Still hand, seated, room 25-30C, forehead dry, finger warm, sensor windows clean, 5-10cm temp distance, 2-min warmup.

## B. Accuracy test (n=30 paired readings)
For each of 30 trials (rest + post-activity mix):
1. Record PROTECT HR/SpO2/Temp + RTC timestamp from TFT/SD.
2. Within 30s record reference oximeter HR/SpO2 + digital thermometer temp.
3. Enter row in `data/accuracy_n30.csv`: `trial,protect_hr,ref_hr,protect_spo2,ref_spo2,protect_temp,ref_temp,note`

Analyze: mean, SD, mean difference, paired t-test (H1/Ho), Bland-Altman note. Target: HR ±5 BPM, SpO2 ±2%, Temp ±0.3C.

## C. Consistency (repeatability)
Same subject, 10 consecutive reads 1 min apart. Report mean±SD, CV%. Pass if no alert flicker at rest.

## D. Response time
Stopwatch: (1) remove finger → SpO2 alert, (2) warm cloth near MLX → temp alert, (3) press SOS → buzzer. Each x3. Pass if mean <3s.

## E. Reliability run
2h continuous bedside run. Log freezes, false alarms, SD gaps. Pass if 0 freezes, <2 false alarms, CSV complete.

## F. Power
USB meter or multimeter: record idle mA + active mA. Runtime ≈ battery_mAh / active_mA. Report in paper 3.3.7 style table.
