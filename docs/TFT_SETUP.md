# TFT Setup — Identify + Configure in 5 Minutes

## 1. Identify your 3.5" driver (do when part arrives)
Flip TFT over, read chip or sticker. Common:
- ILI9488 (320x480, most 3.5" cheap) → TFT_eSPI `ILI9488_DRIVER`
- ST7796 → `ST7796_DRIVER`
- ILI9341 (usually 2.8", some 3.5") → `ILI9341_DRIVER`

If unreadable: try ILI9488 first, then ST7796. Wrong driver = white screen or mirrored text.

## 2. Configure TFT_eSPI
Open `Arduino/libraries/TFT_eSPI/User_Setup.h`, set:
```
#define ILI9488_DRIVER  // or your chip
#define TFT_CS 15
#define TFT_DC 2
#define TFT_RST 4
#define TFT_MOSI 23
#define TFT_MISO 19
#define TFT_SCLK 18
#define LOAD_GLCD / LOAD_FONT2 / LOAD_FONT4 / LOAD_FONT6
#define SPI_FREQUENCY 27000000
```
Save, reopen IDE.

## 3. Hello test
Examples → TFT_eSPI → `Test and diagnostics` → `Read_User_Setup` → Serial must echo your pins.
Then flash minimal:
```cpp
tft.init(); tft.setRotation(1); tft.fillScreen(TFT_BLACK);
tft.setTextSize(3); tft.setTextColor(TFT_WHITE);
tft.setCursor(20,100); tft.println("Hello PROTECT");
```
If backlight only: check RST/DC swap, 3V3 vs 5V VCC, CS pin.

## Why TFT and not HDMI/phone?
Paper requires large, caregiver-readable bedside display with no technical knowledge and no internet. TFT meets FR3 directly; phone/cloud would violate Scope.
