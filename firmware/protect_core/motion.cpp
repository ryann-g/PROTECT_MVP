// motion.cpp — needs Adafruit_MPU6050 lib (installed). Compiles to stubs when off.
#include "motion.h"
#include "config.h"

#if USE_MPU6050
#include <Wire.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>

static Adafruit_MPU6050 mpu;
static bool hasMpu = false;
static bool armed = false;
static unsigned long stillStart = 0, spikeAt = 0;
static float lastMag = 0.0f;

bool motionHas() { return hasMpu; }
float motionGetMag() { return lastMag; }

bool motionBegin() {
  hasMpu = mpu.begin(MPU6050_ADDR, &Wire);
  if (hasMpu) {
    mpu.setAccelerometerRange(MPU6050_RANGE_8_G);
    mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);
  }
  return hasMpu;
}

bool motionCheckFall() {
  if (!hasMpu) { lastMag = 0.0f; return false; }
  sensors_event_t a, g, temp;
  mpu.getEvent(&a, &g, &temp);
  float mag = sqrtf(a.acceleration.x * a.acceleration.x +
                    a.acceleration.y * a.acceleration.y +
                    a.acceleration.z * a.acceleration.z) / 9.81;
  lastMag = mag;
  unsigned long now = millis();
  if (!armed) {
    if (mag > FALL_G_THRESHOLD) { armed = true; spikeAt = now; stillStart = now; }
    return false;
  }
  // Armed: timeout or wild motion re-arms
  if (now - spikeAt > FALL_REARM_MS) { armed = false; return false; }
  if (fabsf(mag - 1.0f) < STILL_G_WINDOW) {
    if (now - stillStart >= STILL_MS) { armed = false; return true; }
  } else {
    stillStart = now;  // still moving — restart stillness clock
  }
  return false;
}

#else
bool motionBegin() { return false; }
bool motionCheckFall() { return false; }
bool motionHas() { return false; }
float motionGetMag() { return 0.0f; }
#endif
