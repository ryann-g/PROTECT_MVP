// sensors.h — MAX30102 (HR+SpO2 via Maxim algorithm) + DS18B20 (temp) + DS3231-mini (RTC in logger).
#pragma once
#include <Arduino.h>

struct Vitals { float hr; float spo2; float tempC; bool hrValid; bool tempValid; };

bool sensorsBegin();
void sensorsPump();  // call EVERY loop: drains MAX FIFO to Maxim window (fast, non-blocking)
Vitals sensorsRead();  // call every SAMPLE_MS; Maxim calc + DS18B20 inside
uint32_t sensorsGetLastIr();  // raw IR for debug / finger gate evidence
