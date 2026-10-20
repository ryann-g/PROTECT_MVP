# Calibration + Test Protocol (for Chapter 3, RQ2-RQ3)

## A. Pre-conditions
Still hand, seated, room 25-30C, forehead dry, finger warm, sensor windows clean, 5-10cm temp distance, 2-min warmup.

## B. Accuracy test (n=30 paired readings)
For each of 30 trials (rest + post-activity mix):
1. Record PROTECT HR/SpO2/Temp + RTC timestamp from TFT/SD.
2. Within 30s record reference oximeter HR/SpO2 + digital thermometer temp.
3. Enter the row in `data/accuracy_n30.csv`, using the header of
   `data/accuracy_n30_template.csv`:
   `participant,reading,hr_prototype,hr_reference,spo2_prototype,spo2_reference,temp_prototype,temp_reference`
   Use participant codes, not names.
4. When the file is complete, build the agreement figure (scatter +
   Bland-Altman, with the bias, the limits of agreement and the paired
   t-test computed from the file):
   `python3 docs/paper/make_agreement_figure.py data/accuracy_n30.csv`
   Nothing in that figure is typed by hand.

Analyze: mean, SD, mean difference, paired t-test (H1/Ho), Bland-Altman note. Target: HR ±5 BPM, SpO2 ±2%, Temp ±0.3C.

## C. Consistency (repeatability)
Same subject, 10 consecutive reads 1 min apart. Report mean±SD, CV%. Pass if no alert flicker at rest.

## D. Response time
Stopwatch: (1) remove finger → SpO2 alert, (2) warm cloth near MLX → temp alert, (3) press SOS → buzzer. Each x3. Pass if mean <3s.

## E. Reliability run
2h continuous bedside run. Log freezes, false alarms, SD gaps. Pass if 0 freezes, <2 false alarms, CSV complete.

## F. Power
USB meter or multimeter: record idle mA + active mA. Runtime ≈ battery_mAh / active_mA. Report in paper 3.3.7 style table.
