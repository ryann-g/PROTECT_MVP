// config.h — P.R.O.T.E.C.T. Final all-5 + TFT: single place to tune pins + thresholds.
// Frozen solder map (2026-10-12): I2C 21/22 (MAX 0x57, RTC 0x68, MPU 0x69 AD0->3V3),
// DS18B20 DATA 14, SPI 23/19/18, TFT CS15/DC2/RST4, SD CS5, LEDs 25/26/27, buzzer 13,
// button 34 (input-only), GSM 16/17 + own 5V 2A + 1000uF, TFT 5V ext, buck 5V->VIN.
// Edit ONLY this file to adapt to your exact ESP32 / TFT boards.

#pragma once
#include <Arduino.h>

// 0 = normal (needs sensors). 1 = TFT/LED/buzzer/button test without sensors.
#define TEST_MODE_NO_SENSORS 0

// 1 = SIMULATE dangerous vitals (HR 130, SpO2 90, Temp 39.0) to drill the
// ALERT screen + buzzer + caregiver SMS end to end. TEST ONLY: alarm drill,
// set back to 0 right after. Never present drill readings as real data.
#define TEST_CRITICAL 0

// 1 = SD card present, 0 = no SD card (TFT + Serial only, no CSV file).
#define USE_SDCARD 0

// ---- All 5 sensors, one build (0 = compiled out, 1 = enabled) ----
#define USE_MPU6050 1
#define USE_GSM 1

// ---- Sensors: MAX30102 + DS18B20 + DS3231-mini (+ MPU6050 + SIM800L when enabled) ----
static const uint8_t PIN_I2C_SDA = 21;
static const uint8_t PIN_I2C_SCL = 22;
static const int PIN_DS18B20 = 14;  // OneWire DATA (was DHT11 pin — no rewiring), 4.7k pull-up to 3V3
// Expected I2C addresses: MAX30102 0x57, DS3231-mini 0x68, MPU6050 0x69 (AD0->3V3)

// ---- SPI (TFT + SD share MOSI/MISO/SCK) ----
static const int PIN_SPI_MOSI = 23;
static const int PIN_SPI_MISO = 19;
static const int PIN_SPI_SCK  = 18;
static const int PIN_TFT_CS   = 15;
static const int PIN_TFT_DC   = 2;
static const int PIN_TFT_RST  = 4;
static const int PIN_SD_CS    = 5;

// ---- Alerts + input ----
static const int PIN_LED_GREEN  = 25;
static const int PIN_LED_YELLOW = 26;
static const int PIN_LED_RED    = 27;
static const int PIN_BUZZER     = 13;
static const int PIN_BUTTON     = 34;  // input-only, NO internal pull-up on GPIO34-39!
#define USE_SOS_BUTTON 0  // 1 = button wired (34 + 10k to 3V3 + btn to GND), 0 = no button yet

// 1 = screen shows WORDS only (FEELING WELL / CHECK NEEDED / ALERT / MEASURING).
// Digits are still measured every sample and stay in the SD/[DATA] log —
// words summarize real data, never replace it. 0 = digits + words (researcher).
#define WORDS_ONLY 1

// ---- Thresholds (elderly early-warning, tunable for defense) ----
static const float HR_LOW_WARN = 50.0,  HR_LOW_CRIT = 50.0;   // <50 critical
static const float HR_HIGH_WARN = 100.0, HR_HIGH_CRIT = 120.0;
static const float SPO2_WARN = 94.0;   // 93-94 warning
static const float SPO2_CRIT = 93.0;   // <93 critical
static const float TEMP_LOW_WARN = 35.5, TEMP_HIGH_WARN = 37.3;
static const float TEMP_LOW_CRIT = 35.5, TEMP_HIGH_CRIT = 38.0;
static const float TEMP_OFFSET_C = 0.0;  // calibrate vs reference thermometer

// ---- MPU6050 fall detection (I2C shared, AD0->3V3 = 0x69) ----
static const uint8_t MPU6050_ADDR = 0x69;  // NOT 0x68 (DS3231 lives there)
static const float FALL_G_THRESHOLD = 2.0;  // spike above 2g arms detector
static const float STILL_G_WINDOW = 0.4;    // then stillness within 1g +/-0.4
static const unsigned long STILL_MS = 1000;  // held 1s = FALL_EVENT
static const unsigned long FALL_REARM_MS = 5000;  // drop arming after 5s

// ---- SIM800L/SIM900 distress SMS (Serial2, module on own 5V 2A supply) ----
static const int PIN_GSM_RX = 16;  // ESP32 RX <- module TXD
static const int PIN_GSM_TX = 17;  // ESP32 TX -> module RXD
static const long GSM_BAUD = 115200;
// Recipients, in order: slot 1 = your number (installed), slots 2-3 empty —
// paste more +63 numbers (own PC only, never chats). Empty = slot skipped.
// Each SMS costs load + ~3-5s airtime; keep the list short (max 3).
static const char *CAREGIVER_NUMBERS[] = {
  "+639935330191",
  "+639055679117",
  "",
};
static const uint8_t CAREGIVER_COUNT = 3;
static const unsigned long GSM_SMS_COOLDOWN_MS = 60000;  // max 1 broadcast/min

// ---- Timing ----
static const unsigned long SAMPLE_MS = 2000;  // 2s: DS18B20 750ms conversion + Maxim window
static const unsigned long LOG_MS    = 5000;  // SD log interval
static const unsigned long DEBOUNCE_MS = 200;

// Status levels: 0 normal, 1 warning, 2 critical/sos
enum StatusLevel : uint8_t { ST_NORMAL = 0, ST_WARNING = 1, ST_CRITICAL = 2 };
