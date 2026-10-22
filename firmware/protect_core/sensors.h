// sensors.h — MAX30102 (HR+SpO2 via Maxim algorithm) + MCP9808 (temp) + DS3231-mini (RTC in logger).
#pragma once
#include <Arduino.h>

struct Vitals { float hr; float spo2; float tempC; bool hrValid; bool tempValid; };

bool sensorsBegin();
void sensorsPump();  // call EVERY loop: drains MAX FIFO to Maxim window (fast, non-blocking)
Vitals sensorsRead();  // call every SAMPLE_MS; Maxim calc + MCP9808 inside
uint32_t sensorsGetLastIr();  // raw IR for debug / finger gate evidence
