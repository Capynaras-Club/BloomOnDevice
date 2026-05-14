#pragma once

#include <Arduino.h>

typedef enum {
  SCREEN_FORECAST  = 0,
  SCREEN_STATS     = 1,
  SCREEN_TODAY     = 2,
  SCREEN_YESTERDAY = 3,
  SCREEN_TWO_DAYS  = 4,
} Screen;

#define SCREEN_COUNT 5

extern Screen   currentScreen;
extern bool     hasWifi;
extern uint8_t  forecastView;      // 0 = hourly, 1 = 3-day
extern uint8_t  logPage;           // page index for log screens

inline Screen nextScreen(Screen s) {
  for (int i = (int)s + 1; i < SCREEN_COUNT; i++) {
    if (i == SCREEN_FORECAST && !hasWifi) continue;
    return (Screen)i;
  }
  return s;
}

inline Screen prevScreen(Screen s) {
  for (int i = (int)s - 1; i >= 0; i--) {
    if (i == SCREEN_FORECAST && !hasWifi) continue;
    return (Screen)i;
  }
  return s;
}

inline bool hasLeftArrow(Screen s) {
  if (s == SCREEN_STATS && !hasWifi) return false;
  if (s == SCREEN_FORECAST) return false;
  return true;
}

inline bool hasRightArrow(Screen s) {
  return s != SCREEN_TWO_DAYS;
}
