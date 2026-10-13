// alerts.h — LEDs + buzzer + SOS button threshold engine.
#pragma once
#include <Arduino.h>
#include "sensors.h"
#include "config.h"

void alertsBegin();
StatusLevel alertsEvaluate(const Vitals &v, bool sosLatched);
void alertsOutput(StatusLevel st, bool sosLatched);
bool alertsSosFlag();       // true if button ISR fired
void alertsClearSos();
