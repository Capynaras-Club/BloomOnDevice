#pragma once

// WiFi connect + first-boot captive portal + NTP/timezone sync.
//
// We delegate the SoftAP / DNS / web-form work to tzapu's WiFiManager. It:
//   - reads saved credentials from NVS (its own namespace),
//   - if none or stale, opens a "BabyStatsSetup" AP with a DNS captive portal
//     so any URL on the connected client redirects to the config form,
//     and blocks here until the user submits or the portal times out,
//   - persists credentials on success.

#include <Arduino.h>
#include <WiFi.h>
#include <WiFiManager.h>
#include <time.h>
#include "config.h"

#define AP_SSID                 "BabyStatsSetup"
#define WIFI_CONNECT_TIMEOUT_S  15
#define PORTAL_TIMEOUT_S        180

extern bool hasWifi;

namespace WifiMgr {

// Returns true if connected to a saved network, or if the user completed
// provisioning in the captive portal. Returns false on offline boot
// (no creds AND portal timed out).
inline bool ensureConnected() {
  WiFi.mode(WIFI_STA);

  WiFiManager wm;
  wm.setConfigPortalTimeout(PORTAL_TIMEOUT_S);
  wm.setConnectTimeout(WIFI_CONNECT_TIMEOUT_S);
  wm.setDarkMode(true);
  wm.setTitle("Baby Stats Setup");

  bool ok = wm.autoConnect(AP_SSID);
  Serial.printf("[wifi] %s\n", ok ? "connected" : "offline");
  return ok;
}

inline void forgetCredentials() {
  WiFiManager wm;
  wm.resetSettings();
}

inline void setTimezone(const char* tz) {
  configTzTime(tz, "pool.ntp.org", "time.nist.gov");
  Serial.printf("[ntp] sync with tz=%s\n", tz);
  uint32_t start = millis();
  while (time(nullptr) < 1700000000UL && millis() - start < 5000) {
    delay(100);
  }
  Serial.printf("[ntp] time=%ld\n", (long)time(nullptr));
}

}  // namespace WifiMgr
