// motion.h — MPU6050 fall detection (USE_MPU6050 flag).
// Wiring: SDA 21 / SCL 22 shared, AD0 -> 3V3 so addr = 0x69 (0x68 is DS3231).
// Logic: accel spike > FALL_G_THRESHOLD arms, then stillness ~1g for STILL_MS = fall.
#pragma once
#include <Arduino.h>

bool motionBegin();      // false if MPU missing (non-fatal, core still runs)
bool motionCheckFall();  // call every SAMPLE_MS; true once per fall event
bool motionHas();        // true if MPU6050 present on bus
float motionGetMag();    // last accel magnitude in g (1.00 at rest)
