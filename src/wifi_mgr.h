#pragma once

// WiFi connect + first-boot captive portal + NTP/timezone sync.
//
// We delegate the SoftAP / DNS / web-form work to tzapu's WiFiManager. It:
//   - reads saved credentials from NVS (its own namespace),
//   - if none or stale, opens a "BloomSetup" AP with a DNS captive portal
//     so any URL on the connected client redirects to the config form,
//     and blocks here until the user submits or the portal times out,
//   - persists credentials on success.

#include <Arduino.h>
#include <WiFi.h>
#include <WiFiManager.h>
#include <esp_wifi.h>
#include <time.h>
#include "config.h"

#define AP_SSID                 "BloomSetup"
#define WIFI_CONNECT_TIMEOUT_S  15
#define PORTAL_TIMEOUT_S        180

extern bool hasWifi;

namespace WifiMgr {

// True if the WiFi NVS namespace has a non-empty SSID stored. Used by the
// splash screen to swap "connecting wifi…" for the captive-portal hint
// when this is a first boot or NVS was wiped.
inline bool hasSavedCredentials() {
  WiFi.mode(WIFI_STA);
  wifi_config_t conf = {};
  if (esp_wifi_get_config(WIFI_IF_STA, &conf) != ESP_OK) return false;
  return conf.sta.ssid[0] != '\0';
}

// Returns true if connected to a saved network, or if the user completed
// provisioning in the captive portal. Returns false on offline boot
// (no creds AND portal timed out).
inline bool ensureConnected() {
  WiFi.mode(WIFI_STA);

  WiFiManager wm;
  wm.setConfigPortalTimeout(PORTAL_TIMEOUT_S);
  wm.setConnectTimeout(WIFI_CONNECT_TIMEOUT_S);
  wm.setDarkMode(true);
  wm.setTitle("Bloom on Device Setup");

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
