# TFT Setup — 3.5" RPi LCD V3.0, HUGA 480x320 (XPT2046 touch, touch unused)

## 0. XPT2046 is the TOUCH chip, not the display driver
Your module has two chips: a display driver (draws pixels — on this HUGA
V3.0 board usually ILI9486, else ILI9488) + XPT2046 (resistive touch).
PROTECT code uses display only; touch is parked (no touch lib, no touch
pins driven). You still must park ONE touch pin (below) or the touch chip
fights the display on the shared SPI lines.

## 1. Board pins (per HUGA V3.0 photo — verified 2026-10-14)
+5V, 0V (GND) x2, DC, RST, CS, MOSI, SCK, MISO (touch-only — LEAVE
UNCONNECTED when touch unused, board says so), T_CS (touch chip select).
No LED pin on this board (backlight runs off 5V).

## 2. Wire (frozen map, matches config.h) — 9 wires, MISO skipped
- +5V → 5V (ext 5V supply, or VIN while USB-powered)
- 0V → GND, both pins (common star with ESP32)
- DC → GPIO2, RST → GPIO33 (moved off GPIO4: DS18B20 owns 4), CS → GPIO15
- MOSI → GPIO23, SCK → GPIO18
- MISO → UNCONNECTED (board-marked touch-only; display is write-only)
- **T_CS → 3V3 (CRITICAL — board itself says "connect to 3.3V if not
  used").** Ties the XPT2046 deselected; floating T_CS = white screen.
- Later, if you want touch: T_CS → GPIO14 (currently free) + wire MISO → 19
  + XPT2046 lib. Not in MVP scope.

## 4. Configure TFT_eSPI
Open `Arduino/libraries/TFT_eSPI/User_Setup.h`, set:
```
#define ILI9486_DRIVER  // HUGA V3.0 first; if white/mirrored, try ILI9488_DRIVER then ST7796_DRIVER
#define TFT_CS 15
#define TFT_DC 2
#define TFT_RST 33
#define TFT_MOSI 23
#define TFT_MISO 19
#define TFT_SCLK 18
#define LOAD_GLCD / LOAD_FONT2 / LOAD_FONT4 / LOAD_FONT6
#define SPI_FREQUENCY 27000000
```
Save, reopen IDE. NOTE: the TFT library reads THESE values, not
`config.h` — both must say RST 33 or the reset pulse hits the temp probe.

## 5. Hello test
Examples → TFT_eSPI → `Test and diagnostics` → `Read_User_Setup` → Serial must echo your pins.
Then flash minimal:
```cpp
tft.init(); tft.setRotation(1); tft.fillScreen(TFT_BLACK);
tft.setTextSize(3); tft.setTextColor(TFT_WHITE);
tft.setCursor(20,100); tft.println("Hello PROTECT");
```
If backlight only: check RST/DC swap, 5V supply, CS pin, T_CS tied to 3V3.
Driver order matters on this board: wrong driver = white screen or mirrored text.

## Why TFT and not HDMI/phone?
Paper requires large, caregiver-readable bedside display with no technical knowledge and no internet. TFT meets FR3 directly; phone/cloud would violate Scope.
