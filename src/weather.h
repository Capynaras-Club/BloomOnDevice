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
  char  timezone[40];        // IANA name from ip-api, e.g. "America/Sao_Paulo"
  char  posix_tz[16];        // POSIX TZ string for configTzTime, e.g. "<-03>3"
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

// ip-api returns UTF-8 city names with diacritics (e.g. "Florianópolis").
// Adafruit_GFX's classic font is a 256-entry CP437 glyph table; multi-byte
// UTF-8 sequences come through as one garbage glyph per byte (so "ó" =>
// 0xC3 0xB3 => two random Greek/math chars on screen). Strip to ASCII by
// dropping high-bit bytes and mapping common Latin-1 / Latin Extended-A
// accents to their base letter as we copy.
inline void copyAsAscii(char* dst, const char* src, size_t cap) {
  if (cap == 0) return;
  size_t w = 0;
  for (size_t i = 0; src[i] && w + 1 < cap; ) {
    uint8_t b = (uint8_t)src[i];
    if (b < 0x80) {                     // plain ASCII
      dst[w++] = (char)b;
      i++;
    } else if ((b & 0xE0) == 0xC0 && (uint8_t)src[i+1]) {  // 2-byte UTF-8
      uint16_t cp = ((b & 0x1F) << 6) | ((uint8_t)src[i+1] & 0x3F);
      char r = '?';
      // Lower-case
      if      (cp == 0x00E0 || cp == 0x00E1 || cp == 0x00E2 || cp == 0x00E3 || cp == 0x00E4 || cp == 0x00E5) r = 'a';
      else if (cp == 0x00E7) r = 'c';
      else if (cp == 0x00E8 || cp == 0x00E9 || cp == 0x00EA || cp == 0x00EB) r = 'e';
      else if (cp == 0x00EC || cp == 0x00ED || cp == 0x00EE || cp == 0x00EF) r = 'i';
      else if (cp == 0x00F1) r = 'n';
      else if (cp == 0x00F2 || cp == 0x00F3 || cp == 0x00F4 || cp == 0x00F5 || cp == 0x00F6 || cp == 0x00F8) r = 'o';
      else if (cp == 0x00F9 || cp == 0x00FA || cp == 0x00FB || cp == 0x00FC) r = 'u';
      else if (cp == 0x00FD || cp == 0x00FF) r = 'y';
      // Upper-case
      else if (cp == 0x00C0 || cp == 0x00C1 || cp == 0x00C2 || cp == 0x00C3 || cp == 0x00C4 || cp == 0x00C5) r = 'A';
      else if (cp == 0x00C7) r = 'C';
      else if (cp == 0x00C8 || cp == 0x00C9 || cp == 0x00CA || cp == 0x00CB) r = 'E';
      else if (cp == 0x00CC || cp == 0x00CD || cp == 0x00CE || cp == 0x00CF) r = 'I';
      else if (cp == 0x00D1) r = 'N';
      else if (cp == 0x00D2 || cp == 0x00D3 || cp == 0x00D4 || cp == 0x00D5 || cp == 0x00D6 || cp == 0x00D8) r = 'O';
      else if (cp == 0x00D9 || cp == 0x00DA || cp == 0x00DB || cp == 0x00DC) r = 'U';
      else if (cp == 0x00DD) r = 'Y';
      else if (cp == 0x00DF) r = 's';   // ß
      dst[w++] = r;
      i += 2;
    } else if ((b & 0xF0) == 0xE0) {    // 3-byte UTF-8 (CJK etc.) — placeholder
      dst[w++] = '?';
      i += 3;
    } else if ((b & 0xF8) == 0xF0) {    // 4-byte UTF-8 — placeholder
      dst[w++] = '?';
      i += 4;
    } else {                            // stray continuation byte
      i++;
    }
  }
  dst[w] = '\0';
}

inline bool fetchLocation() {
  HTTPClient http;
  http.begin("http://ip-api.com/json");
  http.setTimeout(8000);
  int code = http.GET();
  if (code != 200) { http.end(); return false; }

  JsonDocument doc;
  DeserializationError err = deserializeJson(doc, http.getStream());
  http.end();
  if (err) return false;

  copyAsAscii(location.city, doc["city"] | "Unknown", sizeof(location.city));
  strlcpy(location.cc,       doc["countryCode"] | "",    sizeof(location.cc));
  strlcpy(location.timezone, doc["timezone"]    | "UTC", sizeof(location.timezone));
  location.lat   = doc["lat"] | 0.0f;
  location.lon   = doc["lon"] | 0.0f;

  // ip-api returns the UTC offset in seconds (positive = east of UTC).
  // configTzTime() expects POSIX TZ, *not* an IANA name like
  // "America/Sao_Paulo" — passing the IANA string silently fails and
  // localtime() stays on UTC. Build a POSIX string from the offset
  // instead: the sign convention is flipped (POSIX offset is the value
  // to add to local time to get UTC), so UTC-3 becomes "<-03>3", UTC+9
  // becomes "<+09>-9", etc.
  int32_t off  = doc["offset"] | 0;
  int     hrs  = off / 3600;
  int     mins = (off >= 0 ? off : -off) % 3600 / 60;
  if (mins == 0) {
    snprintf(location.posix_tz, sizeof(location.posix_tz),
             "<%+03d>%d", hrs, -hrs);
  } else {
    snprintf(location.posix_tz, sizeof(location.posix_tz),
             "<%+03d:%02d>%d:%02d", hrs, mins, -hrs, mins);
  }

  location.valid = true;

  Serial.printf("[loc] %s, %s (%.4f, %.4f) tz=%s posix=%s\n",
                location.city, location.cc,
                location.lat, location.lon,
                location.timezone, location.posix_tz);
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

  JsonDocument doc;
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
