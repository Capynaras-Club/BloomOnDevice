#pragma once

#include <Arduino.h>
#include <SPI.h>
#include <XPT2046_Touchscreen.h>
#include "config.h"
#include "ui.h"
#include "display.h"   // for layout constants

extern XPT2046_Touchscreen touch;

inline void touchInit() {
  touch.begin();
  touch.setRotation(TFT_ROTATION);
  // GPIO36 is input-only and has no internal pull-up; rely on the
  // 10K external pull-up on the Sunton CYD T_IRQ trace.
  pinMode(PIN_TOUCH_IRQ, INPUT);
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
// Hit-zone tests — portrait 240x320
//   nav bar:     y >= NAV_Y (284)
//   stats cards: stacked vertically at CARD_FEED_Y/CARD_DIAPER_Y/CARD_SLEEP_Y
//   log rows:    y = LOG_ROW_Y0 + i * LOG_ROW_H, delete X at right edge
// =========================================================================

inline bool hitLeftArrow(uint16_t x, uint16_t y) {
  return y >= NAV_Y && x < 50;
}

inline bool hitRightArrow(uint16_t x, uint16_t y) {
  return y >= NAV_Y && x >= (SCREEN_W - 50);
}

inline bool hitPageUp(uint16_t x, uint16_t y) {
  (void)x;
  return y >= HDR_H && y < LOG_ROW_Y0;
}

inline bool hitPageDown(uint16_t x, uint16_t y) {
  (void)x;
  uint16_t y0 = LOG_ROW_Y0 + EVENTS_PER_PAGE * LOG_ROW_H;
  return y >= y0 && y < NAV_Y;
}

inline int8_t hitStatsCard(uint16_t x, uint16_t y) {
  (void)x;
  if (y < CARD_FEED_Y || y >= NAV_Y) return -1;
  if (y < CARD_DIAPER_Y) return EV_FEED;
  if (y < CARD_SLEEP_Y)  return EV_DIAPER;
  return EV_SLEEP;
}

inline int8_t hitDeleteRow(uint16_t x, uint16_t y) {
  if (x < (SCREEN_W - 40)) return -1;
  for (uint8_t i = 0; i < EVENTS_PER_PAGE; i++) {
    uint16_t y0 = LOG_ROW_Y0 + i * LOG_ROW_H;
    if (y >= y0 && y < y0 + (LOG_ROW_H - 6)) return i;
  }
  return -1;
}

inline int8_t hitForecastToggle(uint16_t x, uint16_t y) {
  (void)x;
  if (y >= HDR_H && y < LOG_ROW_Y0)        return 0; // up: hourly
  if (y >= (NAV_Y - 30) && y < NAV_Y)      return 1; // down: 3-day
  return -1;
}
