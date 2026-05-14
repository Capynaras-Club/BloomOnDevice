#pragma once

#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <Preferences.h>
#include <time.h>
#include "config.h"

#define WIFI_CONNECT_TIMEOUT_MS  15000
#define AP_SSID                  "BabyStatsSetup"

extern bool hasWifi;

namespace WifiMgr {

inline Preferences& prefs() {
  static Preferences p;
  return p;
}

inline bool connectSaved() {
  prefs().begin("wifi", true);
  String ssid = prefs().getString("ssid", "");
  String pass = prefs().getString("pass", "");
  prefs().end();
  if (ssid.length() == 0) return false;

  Serial.printf("[wifi] connecting to %s\n", ssid.c_str());
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid.c_str(), pass.c_str());

  uint32_t start = millis();
  while (WiFi.status() != WL_CONNECTED &&
         millis() - start < WIFI_CONNECT_TIMEOUT_MS) {
    delay(250);
  }
  bool ok = WiFi.status() == WL_CONNECTED;
  Serial.printf("[wifi] %s\n", ok ? "OK" : "FAIL");
  return ok;
}

inline void saveCreds(const String& ssid, const String& pass) {
  prefs().begin("wifi", false);
  prefs().putString("ssid", ssid);
  prefs().putString("pass", pass);
  prefs().end();
}

inline void startProvisioning() {
  Serial.println("[wifi] starting SoftAP provisioning");
  WiFi.mode(WIFI_AP);
  WiFi.softAP(AP_SSID);

  static WebServer server(80);
  server.on("/", []() {
    server.send(200, "text/html",
      "<!doctype html><html><head><meta name=viewport "
      "content='width=device-width,initial-scale=1'>"
      "<title>Baby Stats Setup</title></head><body>"
      "<h2>WiFi Setup</h2>"
      "<form method=POST action=/save>"
      "SSID:<br><input name=ssid><br>"
      "Password:<br><input name=pass type=password><br><br>"
      "<input type=submit value=Save></form></body></html>");
  });
  server.on("/save", []() {
    String s = server.arg("ssid");
    String p = server.arg("pass");
    saveCreds(s, p);
    server.send(200, "text/html",
      "<h3>Saved. Device will reboot.</h3>");
    delay(1000);
    ESP.restart();
  });
  server.begin();

  while (true) {
    server.handleClient();
    delay(10);
  }
}

inline void setTimezone(const char* tz) {
  configTzTime(tz, "pool.ntp.org", "time.nist.gov");
  Serial.printf("[ntp] sync with tz=%s\n", tz);
  uint32_t start = millis();
  while (time(nullptr) < 1700000000UL && millis() - start < 5000) {
    delay(100);
  }
  time_t now = time(nullptr);
  Serial.printf("[ntp] time=%ld\n", (long)now);
}

}  // namespace WifiMgr
