# Wiring Map — Core MVP (ESP32 DevKit 30-pin)

## I2C shared bus (parallel, 3 wires + power)
- SDA → GPIO21, SCL → GPIO22, GND → GND, VCC → 3V3
- MAX30102 (0x57) + DS3231 (0x68) on same SDA/SCL. MLX90614 (0x5A) is OUT of
  the system (removed 2026-10-12); MCP9808 (0x18) is the planned temp sensor
  (§68 of the session log) and does not conflict with 0x57/0x68/0x69.
- Use 4.7k pull-ups if breakouts lack them. Keep wires <15cm, away from buck converter.

## Body temp: MCP9808 (I2C 0x18) — current sensor
- VCC → 3V3, GND → GND, SDA → GPIO21, SCL → GPIO22 (shares the bus with the
  MAX30102, the DS3231 and the MPU6050; no dedicated pin, no pull-up needed)
- A0/A1/A2 → GND gives the address 0x18.
- Typical accuracy 0.25 °C over the body-temperature range; 30 ms conversion.
- Offset-calibrate against a reference thermometer (`TEMP_OFFSET_C`).
- It replaced the DS18B20 probe, which occupied GPIO4 — the pin now used by
  the SOS button.

## Body temp: DS18B20 waterproof probe (legacy — UNPLUGGED, do not reconnect)
**STATUS 2026-10-20: the DS18B20 is DISCONNECTED. GPIO4 now belongs to the SOS
button. Leave the probe unplugged — see the warning below.**
- RED VCC → 3V3, BLACK GND → GND, YELLOW DATA → GPIO4 (was; do not reconnect)
- 4.7k pull-up DATA→3V3 (mandatory — isolation-proven pin, matches `PIN_DS18B20 = 4`).
- Terminal adapter: probe leads screwed firmly into block, adapter pins to ESP32.
- Codes: -127 = disconnected (DATA4/VCC/GND/pull-up), 85 = power-on reset (re-read).
- Use: hold steel probe in closed fist 1-3 min to equilibrium, offset-calibrate
  vs reference thermometer (`TEMP_OFFSET_C` in config.h).
- WHY IT MUST STAY UNPLUGGED: the OneWire library drives the bus for a presence
  pulse and a conversion. On GPIO4 that pulse is a falling edge, and the SOS
  button listens for falling edges on that same pin — the probe would trigger
  a false SOS (the same failure shape as the GPIO34 float storm in §17). The
  temperature is a placeholder value anyway, and the replacement (MCP9808,
  I2C 0x18) does not use GPIO4 at all.

## SPI (TFT + SD share MOSI/MISO/SCK, separate CS)
- MOSI → GPIO23, MISO → GPIO19 (NC on HUGA RPi V3.0: board-marked touch-only,
  display is write-only — leave unconnected while touch parked), SCK → GPIO18
- TFT_CS → GPIO15, TFT_DC → GPIO2, TFT_RST → GPIO33 (moved off GPIO4 when
  the DS18B20 took it; GPIO4 is now the SOS button)
- SD_CS → GPIO5
- HUGA 3.5" RPi V3.0: +5V → 5V ext (or VIN on USB), both 0V → GND star.
  No LED pin (backlight off 5V).
- XPT2046 touch parked: T_CS → 3V3 (mandatory, board says so),
  all other touch pins unconnected. Full table in docs/TFT_SETUP.md.

## Alerts + input (delivered rig)
- LED_YELLOW → GPIO26 (long leg), short leg → 220Ω → GND. ON = warning only.
  NOTE: on the delivered rig the LEDs are REMOVED — the screen and the buzzer
  carry the alert (deployed copy, `final_protect/PROTECT/PROTECT.ino`).
- GREEN (25) + RED UNWIRED — normal/critical show on screen; critical adds buzzer.
- BUZZER → GPIO27 (active buzzer to GND; passive needs tone()). CONFIRMED on
  the rig. The repo builds (`firmware/protect_core`, `protect_serial`,
  `protect_cloud`) still say GPIO13 — reconcile before the next upload (§66).
- BUTTON → GPIO4: button to GND, `INPUT_PULLUP` (GPIO4 has an internal
  pull-up, so NO external 10k resistor), interrupt FALLING, debounce 200 ms,
  `USE_SOS_BUTTON=1`. CONFIRMED on the rig.
  The repo builds still use GPIO34 (input-only → external 10k 34→3V3 required,
  `USE_SOS_BUTTON=0`) — reconcile before the next upload (§66).
  GPIO4 used to be the DS18B20 data pin; the probe must stay unplugged now
  (see the body-temp section above).

## Power
- LiPo → TP4056 B+/B- → OUT → buck IN → buck OUT 3.3V/5V → ESP32 VIN/3V3
- Common GND everywhere. Never charge + run unattended. Insulate solder joints.
- Unplug LiPo when flashing via USB if TFT flickers (USB current limit).

## Verify
1. Continuity beep every rail before power.
2. Power on: ESP32 LED on, TFT backlight on, no heat/smell.
3. I2C scan: must show 0x57 (MAX) + 0x68 (RTC) + 0x69 (MPU, AD0->3V3).
   If missing, check SDA/SCL swap and 3V3. Note: MAX30102 is currently dead,
   so 0x57 may be absent even with correct wiring.
4. RTC time set (see logger.cpp) before full run. SD init is optional —
   `USE_SDCARD=0` in the delivered system, the SD card is not part of it.
5. SOS button check: press → SOS-HELP! + buzzer (GPIO27) + SMS. If it fires
   without a press, check that the DS18B20 probe is unplugged from GPIO4.
