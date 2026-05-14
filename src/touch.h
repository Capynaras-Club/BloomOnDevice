#pragma once

#include <Arduino.h>
#include <SPI.h>
#include <XPT2046_Touchscreen.h>
#include "config.h"
#include "ui.h"

extern XPT2046_Touchscreen touch;

inline void touchInit() {
  touch.begin();
  touch.setRotation(TFT_ROTATION);
}

inline uint16_t mapRaw(uint16_t raw, uint16_t mn, uint16_t mx, uint16_t out) {
  if (raw < mn) raw = mn;
  if (raw > mx) raw = mx;
  return (uint32_t)(raw - mn) * out / (mx - mn);
}

inline uint16_t toScreenX(uint16_t raw) {
  return mapRaw(raw, TOUCH_MIN_X, TOUCH_MAX_X, SCREEN_W);
}

inline uint16_t toScreenY(uint16_t raw) {
  return mapRaw(raw, TOUCH_MIN_Y, TOUCH_MAX_Y, SCREEN_H);
}

// =========================================================================
// Hit-zone tests
// =========================================================================

inline bool hitLeftArrow(uint16_t x, uint16_t y) {
  return y >= 204 && x < 50;
}

inline bool hitRightArrow(uint16_t x, uint16_t y) {
  return y >= 204 && x >= 270;
}

inline bool hitPageUp(uint16_t x, uint16_t y) {
  (void)x;
  return y >= 28 && y < 52;
}

inline bool hitPageDown(uint16_t x, uint16_t y) {
  (void)x;
  return y >= 176 && y < 200;
}

inline int8_t hitStatsCard(uint16_t x, uint16_t y) {
  if (y < 28 || y > 200) return -1;
  if (x < 100)  return EV_FEED;
  if (x < 212)  return EV_DIAPER;
  return EV_SLEEP;
}

// Log rows: 5 rows, y starting at 32, each ~30px tall, delete in x 290-320
inline int8_t hitDeleteRow(uint16_t x, uint16_t y) {
  if (x < 290) return -1;
  uint16_t base = 32;
  for (uint8_t i = 0; i < EVENTS_PER_PAGE; i++) {
    uint16_t y0 = base + i * 30;
    if (y >= y0 && y < y0 + 28) return i;
  }
  return -1;
}

inline int8_t hitForecastToggle(uint16_t x, uint16_t y) {
  (void)x;
  if (y >= 28 && y < 52)   return 0; // up: hourly
  if (y >= 176 && y < 200) return 1; // down: 3-day
  return -1;
}
