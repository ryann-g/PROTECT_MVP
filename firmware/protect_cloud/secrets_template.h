// Copy this file to secrets.h in the same folder, then fill in the real
// values. secrets.h is gitignored - do not commit it.
//
// The sketch includes secrets.h when it exists and falls back to this file
// when it does not, so a fresh clone always compiles.

#pragma once

// ---- Wi-Fi (the network the unit uploads from) ----
#define WIFI_SSID_STR "YOUR_WIFI_SSID"
#define WIFI_PASS_STR "YOUR_WIFI_PASSWORD"

// ---- SMS recipients (empty = slot skipped) ----
static const char *CAREGIVER_NUMBERS[] = { "", "", "" };
