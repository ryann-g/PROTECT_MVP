// Copy this file to secrets.h in the same folder, then fill in the real
// values. secrets.h is gitignored - do not commit it.
//
// config.h falls back to this file automatically when secrets.h is missing,
// so a fresh clone always compiles.

#pragma once

// Recipients, in order. Empty = slot skipped.
static const char *CAREGIVER_NUMBERS[] = {
  "",
  "",
  "",
};

// ---- IoT display AP (phone/laptop joins this to open 192.168.4.1) ----
static const char *WIFI_AP_SSID = "PROTECT-AP";
static const char *WIFI_AP_PASS = "YOUR_AP_PASSWORD";  // min 8 chars for WPA2
