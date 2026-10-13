# Stretch Sensors — NOT in Core MVP (parked per your Core-only decision)

## Current firmware covers
- MAX30102 (HR + SpO2, I2C 0x57) → sensors.cpp
- MLX90614 (temp, I2C 0x5A) → sensors.cpp
- DS3231 (timestamps, I2C 0x68) → logger.cpp
- MicroSD (CSV log, SPI CS 5) → logger.cpp
- 3.5" TFT (SPI) → display_ui.cpp
- LEDs + buzzer + SOS button → alerts.cpp

## Parked (in paper Methods/Cost Table, OUT of 48h MVP)
1. MPU6050 (motion/fall, I2C 0x68 — address conflicts with DS3231, needs AD0 pin change to 0x69)
   - Add: `motion.h` fall threshold (±2g spike + 1s stillness), new WARNING→CRITICAL path, log `FALL_EVENT`.
2. NEO-6M GPS (location, UART TX→GPIO16 RX→GPIO17, 9600 baud, outdoor only)
   - Add: `gps.h` TinyGPS++ parse, append lat/lon to CSV, show on TFT SOS screen.
3. SIM800L GSM (SMS, UART2 + 4V/2A supply, SIM, AT commands)
   - Add: `gsm.h` send `ALERT HR:xx SpO2:xx T:xx + Google Maps link`, needs separate power (buck 4V, NOT ESP32 3V3).

## Why parked
Scope limitation excludes remote IoT alerts; each adds wiring/power/library risk that breaks the 48h timeline. Structure in `config.h` + `protect_core.ino` already isolates them — say the word and I generate `motion.cpp/gps.cpp/gsm.cpp` as drop-in tabs without touching core.
