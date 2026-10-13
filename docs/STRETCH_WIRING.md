# Stretch Wiring — MPU6050 / NEO-6M / SIM800L (NOT wired in 48h MVP)

Wire these ONLY if you later activate stretch modules. Core MVP wiring stays in docs/WIRING.md.

## 1. MPU6050 (I2C motion/fall) — ⚠️ address conflict
- Default addr 0x68 = SAME as DS3231. Must change: tie AD0 pin → 3V3 → MPU becomes 0x69.
- VCC → 3V3, GND → GND, SDA → GPIO21, SCL → GPIO22 (same bus as MAX/MLX/RTC).
- Place flat on PCB, axes marked. Mount rigid — loose wires = false falls.
- Code: new `motion.cpp`, threshold ~2g spike + 1s stillness → FALL_EVENT.

## 2. NEO-6M GPS (UART location) — outdoor only
- VCC → 3V3 (some boards 5V — check yours), GND → GND
- TX → GPIO16 (ESP32 RX2), RX → GPIO17 (ESP32 TX2), 9600 baud via TinyGPS++
- Antenna face-up, near window/outside. Cold fix 1-5 min. Indoors = no fix (expected).
- Code: new `gps.cpp`, append lat/lon to CSV, show on SOS screen.

## 3. SIM800L GSM (SMS alerts) — ⚠️ separate 4V power, NOT ESP32 3V3
- VCC → buck set to 4.0V / 2A peak (NOT 3V3, NOT 5V — module burns).
- GND → common GND with ESP32. TX → GPIO26? Use UART1: TX→GPIO12/RX→GPIO14 or SoftwareSerial. Keep core UART0 free for USB debug.
- SIM holder → active SIM with load. Antenna connected before power.
- AT test: `AT` → OK, `AT+CSQ` → signal, `AT+CMGS` → SMS. No signal indoors = normal.
- Code: new `gsm.cpp`, sends `ALERT HR/SpO2/T + maps link` on CRITICAL/SOS only (rate-limited).

## Rule
Do NOT wire stretch parts during 48h core build — each adds a failure point (I2C clash / no-fix / brownout) that breaks your defense demo.
