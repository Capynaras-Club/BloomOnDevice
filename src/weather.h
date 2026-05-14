#pragma once

#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <time.h>
#include "config.h"

struct WeatherCurrent { float temp; float humidity; uint8_t code; };
struct WeatherHourly  { uint32_t ts; float temp; uint8_t rain; uint8_t code; };
struct WeatherDaily   { uint32_t ts; float hi; float lo; uint8_t rain; uint8_t code; };

struct Location {
  char  city[32];
  char  cc[4];
  char  timezone[40];
  float lat;
  float lon;
  bool  valid;
};

#define HOURLY_SLOTS  12
#define DAILY_SLOTS    3

extern Location       location;
extern WeatherCurrent wxNow;
extern WeatherHourly  wxHourly[HOURLY_SLOTS];
extern WeatherDaily   wxDaily[DAILY_SLOTS];
extern bool           wxValid;

inline bool fetchLocation() {
  HTTPClient http;
  http.begin("http://ip-api.com/json");
  http.setTimeout(8000);
  int code = http.GET();
  if (code != 200) { http.end(); return false; }

  StaticJsonDocument<768> doc;
  DeserializationError err = deserializeJson(doc, http.getStream());
  http.end();
  if (err) return false;

  strlcpy(location.city,     doc["city"]        | "Unknown", sizeof(location.city));
  strlcpy(location.cc,       doc["countryCode"] | "",        sizeof(location.cc));
  strlcpy(location.timezone, doc["timezone"]    | "UTC",     sizeof(location.timezone));
  location.lat   = doc["lat"] | 0.0f;
  location.lon   = doc["lon"] | 0.0f;
  location.valid = true;

  Serial.printf("[loc] %s, %s (%.4f, %.4f) tz=%s\n",
                location.city, location.cc,
                location.lat, location.lon, location.timezone);
  return true;
}

inline bool fetchWeather() {
  if (!location.valid) return false;

  char url[256];
  snprintf(url, sizeof(url),
    "https://api.open-meteo.com/v1/forecast"
    "?latitude=%.4f&longitude=%.4f"
    "&current=temperature_2m,relative_humidity_2m,weathercode"
    "&hourly=temperature_2m,precipitation_probability,weathercode"
    "&daily=temperature_2m_max,temperature_2m_min,precipitation_probability_max,weathercode"
    "&forecast_days=3&timezone=auto",
    location.lat, location.lon);

  HTTPClient http;
  http.begin(url);
  http.setTimeout(10000);
  int code = http.GET();
  if (code != 200) { http.end(); return false; }

  DynamicJsonDocument doc(16384);
  DeserializationError err = deserializeJson(doc, http.getStream());
  http.end();
  if (err) {
    Serial.printf("[wx] parse failed: %s\n", err.c_str());
    return false;
  }

  JsonObject cur = doc["current"];
  wxNow.temp     = cur["temperature_2m"]      | 0.0f;
  wxNow.humidity = cur["relative_humidity_2m"]| 0.0f;
  wxNow.code     = cur["weathercode"]        | 0;

  JsonArray hTimes = doc["hourly"]["time"];
  JsonArray hTemps = doc["hourly"]["temperature_2m"];
  JsonArray hRain  = doc["hourly"]["precipitation_probability"];
  JsonArray hCode  = doc["hourly"]["weathercode"];

  time_t now = time(nullptr);
  uint16_t startIdx = 0;
  for (uint16_t i = 0; i < hTimes.size(); i++) {
    const char* iso = hTimes[i];
    struct tm tm = {};
    strptime(iso, "%Y-%m-%dT%H:%M", &tm);
    time_t t = mktime(&tm);
    if (t >= now) { startIdx = i; break; }
  }

  for (uint16_t i = 0; i < HOURLY_SLOTS; i++) {
    uint16_t idx = startIdx + i;
    if (idx >= hTimes.size()) { wxHourly[i].ts = 0; continue; }
    const char* iso = hTimes[idx];
    struct tm tm = {};
    strptime(iso, "%Y-%m-%dT%H:%M", &tm);
    wxHourly[i].ts   = (uint32_t)mktime(&tm);
    wxHourly[i].temp = hTemps[idx] | 0.0f;
    wxHourly[i].rain = hRain[idx]  | 0;
    wxHourly[i].code = hCode[idx]  | 0;
  }

  JsonArray dTimes = doc["daily"]["time"];
  JsonArray dHi    = doc["daily"]["temperature_2m_max"];
  JsonArray dLo    = doc["daily"]["temperature_2m_min"];
  JsonArray dRain  = doc["daily"]["precipitation_probability_max"];
  JsonArray dCode  = doc["daily"]["weathercode"];

  for (uint8_t i = 0; i < DAILY_SLOTS; i++) {
    if (i >= dTimes.size()) { wxDaily[i].ts = 0; continue; }
    const char* iso = dTimes[i];
    struct tm tm = {};
    strptime(iso, "%Y-%m-%d", &tm);
    wxDaily[i].ts   = (uint32_t)mktime(&tm);
    wxDaily[i].hi   = dHi[i]   | 0.0f;
    wxDaily[i].lo   = dLo[i]   | 0.0f;
    wxDaily[i].rain = dRain[i] | 0;
    wxDaily[i].code = dCode[i] | 0;
  }

  wxValid = true;
  Serial.printf("[wx] now %.1fC %u%% code=%u\n",
                wxNow.temp, (unsigned)wxNow.humidity, wxNow.code);
  return true;
}

inline const char* weatherCodeShort(uint8_t code) {
  if (code == 0)                    return "Clear";
  if (code <= 3)                    return "Cloud";
  if (code == 45 || code == 48)     return "Fog";
  if (code >= 51 && code <= 57)     return "Drzl";
  if (code >= 61 && code <= 67)     return "Rain";
  if (code >= 71 && code <= 77)     return "Snow";
  if (code >= 80 && code <= 82)     return "Shwr";
  if (code >= 95)                   return "Storm";
  return "?";
}
