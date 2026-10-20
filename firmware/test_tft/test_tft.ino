// test_tft.ino — TFT-ONLY isolation (no sensors, no GSM, no SD, no buzzer).
// Purpose: prove display + power + wiring before running the full build.
// Board: HUGA 3.5" RPi LCD V3.0, 480x320 (XPT2046 touch parked).
//
// WIRING (8 wires, display-only):
//   +5V -> 5V rail | 0V -> GND (Pin 6 enough, ground plane) | DC -> GPIO2
//   RST -> GPIO33 | CS -> GPIO15 | MOSI -> GPIO23 | SCK -> GPIO18
//   T_CS -> 3V3 (mandatory, parks touch) | MISO NC | header Pin 1 NC.
//
// User_Setup.h (Arduino/libraries/TFT_eSPI/User_Setup.h) MUST say:
//   #define ILI9486_DRIVER  (this board's usual chip; white screen -> try ILI9488)
//   TFT_CS 15 / TFT_DC 2 / TFT_RST 33 / TFT_MOSI 23 / TFT_MISO 19 / TFT_SCLK 18
//
// POWER: TFT +5V from solid 5V (bench supply/buck, NOT weak PC USB alone).
//   ESP32 may stay on USB for Serial Monitor; GNDs MUST be common.
//
// PASS: color bands + big words draw, counter rises forever, Serial ticks.
//   Counter restarting at #1 repeatedly = power reboot loop (fix supply first).
//   White screen with counter ticking = wrong driver OR RST/T_CS wiring.
// Serial 115200. Flash Mode DIO.
#include <Arduino.h>
#include <TFT_eSPI.h>

static TFT_eSPI tft;
static uint32_t n = 0;

void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.println(F("TEST TFT boot (display-only, HUGA RPi V3.0)"));
  Serial.println(F("Want User_Setup: ILI9486 first, CS15/DC2/RST33/MOSI23/MISO19/SCK18"));
  tft.init();
  tft.setRotation(1);  // landscape 480x320 bedside
  tft.fillScreen(TFT_BLACK);
  Serial.println(F("tft.init OK. White screen now = wrong driver or RST33/T_CS/power."));
}

void loop() {
  static unsigned long t = 0;
  if (millis() - t < 2000) return;
  t = millis();
  n++;
  uint8_t phase = n % 5;

  if (phase == 0) {
    // Pixel proof: full-screen primary bands, 1s apart feel via 2s tick.
    tft.fillScreen(TFT_RED); delay(600);
    tft.fillScreen(TFT_GREEN); delay(600);
    tft.fillScreen(TFT_BLUE);
    Serial.printf("screen #%lu RGB bands (pixel proof)\n", (unsigned long)n);
    return;
  }

  const char *big, *sub;
  uint16_t bar;
  if (phase == 1)      { big = "WELL";       sub = "You are OK. Rest.";              bar = TFT_GREEN;  }
  else if (phase == 2) { big = "CHECK";      sub = "Please rest, recheck soon.";    bar = TFT_YELLOW; }
  else if (phase == 3) { big = "ALERT";      sub = "Sit down, call caregiver now!"; bar = TFT_RED;    }
  else                 { big = "WAIT...";    sub = "Keep finger still, wait.";      bar = TFT_WHITE;  }

  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setCursor(10, 10);
  tft.setTextSize(2);
  tft.print("PROTECT TFT TEST #");
  tft.print(n);
  tft.setTextSize(5);
  tft.setCursor(20, 80);
  tft.println(big);
  tft.setTextSize(2);
  tft.setCursor(20, 150);
  tft.println(sub);
  tft.fillRect(0, 230, 480, 90, bar);
  tft.setTextColor(TFT_BLACK, bar);
  tft.setTextSize(3);
  tft.setCursor(20, 250);
  tft.println(big);

  Serial.printf("screen #%lu %s drawn (counter rising = no reboot loop)\n",
                (unsigned long)n, big);
}
