# Wiring Map — Core MVP (ESP32 DevKit 30-pin)

## I2C shared bus (parallel, 3 wires + power)
- SDA → GPIO21, SCL → GPIO22, GND → GND, VCC → 3V3
- MAX30102 (0x57) + DS3231 (0x68) on same SDA/SCL. MLX90614 (0x5A) joins later.
- Use 4.7k pull-ups if breakouts lack them. Keep wires <15cm, away from buck converter.

## Interim temp: DHT11 (1 digital pin, NOT I2C)
- DHT11 VCC → 3V3, GND → GND, DATA → GPIO14
- 10k pull-up DATA→3V3 if your module lacks it (most blue modules include it).
- Library: `DHT sensor library by Adafruit` + `Adafruit Unified Sensor`.
- Limits: air temp only ±2C, max 1 reading/2s (SAMPLE_MS=2000). Final MLX90614 later.
- To switch later: set USE_DHT11_TEMP 0 in config.h, wire MLX SDA/SCL to 21/22.

## SPI (TFT + SD share MOSI/MISO/SCK, separate CS)
- MOSI → GPIO23, MISO → GPIO19, SCK → GPIO18
- TFT_CS → GPIO15, TFT_DC → GPIO2, TFT_RST → GPIO4
- SD_CS → GPIO5
- TFT VCC → 3V3 (or 5V if board marked 5V-tolerant), LED/BL → 3V3 via 100Ω if dim

## Alerts + input (direct GPIO)
- LED_GREEN → GPIO25, LED_YELLOW → GPIO26, LED_RED → GPIO27 (220Ω to GND)
- BUZZER → GPIO13 (active buzzer to GND; passive needs tone())
- BUTTON → GPIO34 to GND (INPUT_PULLUP, interrupt FALLING, debounce 200ms)
  - GPIO34 is input-only — correct for button, never set as OUTPUT.

## Power
- LiPo → TP4056 B+/B- → OUT → buck IN → buck OUT 3.3V/5V → ESP32 VIN/3V3
- Common GND everywhere. Never charge + run unattended. Insulate solder joints.
- Unplug LiPo when flashing via USB if TFT flickers (USB current limit).

## Verify
1. Continuity beep every rail before power.
2. Power on: ESP32 LED on, TFT backlight on, no heat/smell.
3. I2C scan: must show 0x57 (MAX) + 0x68 (RTC). 0x5A appears only after MLX swap.
   If missing, check SDA/SCL swap and 3V3.
4. SD init + RTC time set (see logger.cpp) before full run.
