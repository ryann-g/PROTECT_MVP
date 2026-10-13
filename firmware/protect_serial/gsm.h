// gsm.h — SIM800L/SIM900 distress SMS (USE_GSM flag).
// Wiring: GSM TXD -> GPIO16, RXD -> GPIO17, 5VIN from ext 5V 2A, common GND.
// Direct AT commands (no wrapper API) so the paper can quote each step.
// TinyGSM lib also installed as an alternative transport if preferred.
#pragma once
#include <Arduino.h>
#include "sensors.h"
#include "config.h"

bool gsmBegin();  // AT handshake; false if module missing (non-fatal)
bool gsmIsReady();  // true after successful AT OK in gsmBegin()
void gsmSendDistress(const Vitals &v, StatusLevel st, const char *why);  // rate-limited
