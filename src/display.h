#pragma once

#include <Arduino.h>
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>
#include <time.h>
#include "config.h"
#include "ui.h"
#include "events.h"
#include "weather.h"

extern Adafruit_ILI9341 tft;
extern uint32_t pendingDeleteId;
extern uint32_t pendingDeleteMs;

// =========================================================================
// Time formatting (24h)
// =========================================================================

inline void getClockString(char* buf, size_t n) {
  time_t now = time(nullptr);
  struct tm* t = localtime(&now);
  strftime(buf, n, "%H:%M", t);
}

inline void getEventTimeString(uint32_t ts, char* buf, size_t n) {
  time_t t = (time_t)ts;
  struct tm* tm = localtime(&t);
  strftime(buf, n, "%H:%M", tm);
}

inline void getDateString(uint8_t daysBack, char* buf, size_t n) {
  time_t t = time(nullptr) - (time_t)daysBack * 86400;
  struct tm* tm = localtime(&t);
  strftime(buf, n, "%a %b %d", tm);
}

// =========================================================================
// Backlight
// =========================================================================

inline void backlightInit() {
  ledcSetup(0, 5000, 8);
  ledcAttachPin(PIN_BACKLIGHT, 0);
  ledcWrite(0, BACKLIGHT_FULL);
}

inline void backlightSet(uint8_t v) { ledcWrite(0, v); }

// =========================================================================
// Display init
// =========================================================================

inline void displayInit() {
  tft.begin(TFT_SPI_HZ);
  tft.setRotation(TFT_ROTATION);
  tft.fillScreen(COL_BG);
}

// =========================================================================
// Primitive helpers
// =========================================================================

inline void drawTextAt(int16_t x, int16_t y, const char* s,
                       uint16_t color, uint8_t size = 1) {
  tft.setCursor(x, y);
  tft.setTextColor(color);
  tft.setTextSize(size);
  tft.print(s);
}

inline void drawTextCentered(int16_t cx, int16_t y, const char* s,
                             uint16_t color, uint8_t size = 1) {
  int16_t w = strlen(s) * 6 * size;
  drawTextAt(cx - w / 2, y, s, color, size);
}

// =========================================================================
// Header bar — clock + (optional) location
// =========================================================================

inline void drawHeader() {
  tft.fillRect(0, 0, SCREEN_W, 24, COL_BAR);

  char clock[8];
  getClockString(clock, sizeof(clock));
  tft.setTextSize(2);
  tft.setTextColor(COL_CLOCK);
  tft.setCursor(8, 5);
  tft.print(clock);

  if (hasWifi && location.valid) {
    tft.setTextSize(1);
    tft.setTextColor(COL_TEXT_DIM);
    int16_t w = strlen(location.city) * 6;
    tft.setCursor(SCREEN_W - w - 8, 9);
    tft.print(location.city);
  }
}

// =========================================================================
// Nav bar — arrows
// =========================================================================

inline void drawNavBar(Screen s) {
  tft.fillRect(0, 204, SCREEN_W, 36, COL_BAR);
  tft.drawLine(0, 204, SCREEN_W, 204, COL_DIVIDER);

  tft.setTextSize(3);
  if (hasLeftArrow(s)) {
    tft.setTextColor(COL_TEXT);
    tft.setCursor(14, 212);
    tft.print((char)0x11);  // '◄' approximation via char code
    // Fallback ASCII arrow
    tft.setCursor(14, 212);
    tft.print("<");
  }
  if (hasRightArrow(s)) {
    tft.setTextColor(COL_TEXT);
    tft.setCursor(SCREEN_W - 28, 212);
    tft.print(">");
  }

  tft.setTextSize(1);
  tft.setTextColor(COL_TEXT_DIM);
  const char* label = "Stats";
  switch (s) {
    case SCREEN_FORECAST:  label = "Forecast";   break;
    case SCREEN_STATS:     label = "Stats";      break;
    case SCREEN_TODAY:     label = "Today";      break;
    case SCREEN_YESTERDAY: label = "Yesterday";  break;
    case SCREEN_TWO_DAYS:  label = "2 Days Ago"; break;
  }
  drawTextCentered(SCREEN_W / 2, 220, label, COL_TEXT_DIM, 1);
}

// =========================================================================
// Stats screen — three cards
// =========================================================================

inline void drawStatCard(int16_t x, const char* title,
                         uint16_t fg, uint16_t bg, uint8_t type) {
  const int16_t w = 100;
  const int16_t h = 172;
  const int16_t y = 28;

  tft.fillRoundRect(x, y, w, h, 6, bg);
  tft.drawRoundRect(x, y, w, h, 6, fg);

  drawTextCentered(x + w / 2, y + 8, title, fg, 2);

  const Event* last = getLastOfType(type);
  if (last) {
    char tbuf[8];
    getEventTimeString(last->timestamp, tbuf, sizeof(tbuf));
    drawTextCentered(x + w / 2, y + 56, "Last", COL_TEXT_DIM, 1);
    drawTextCentered(x + w / 2, y + 72, tbuf, COL_TEXT, 3);

    uint32_t now = (uint32_t)time(nullptr);
    uint32_t mins = (now > last->timestamp) ? (now - last->timestamp) / 60 : 0;
    char buf[16];
    if (mins < 60) snprintf(buf, sizeof(buf), "%lu min ago", (unsigned long)mins);
    else           snprintf(buf, sizeof(buf), "%luh%lum",
                            (unsigned long)(mins / 60),
                            (unsigned long)(mins % 60));
    drawTextCentered(x + w / 2, y + 108, buf, COL_TEXT_DIM, 1);
  } else {
    drawTextCentered(x + w / 2, y + 72, "—", COL_TEXT_DIM, 3);
  }

  uint16_t count = countEventsForDay(0, type);
  char dbuf[16];
  snprintf(dbuf, sizeof(dbuf), "%u today", (unsigned)count);
  drawTextCentered(x + w / 2, y + 140, dbuf, fg, 1);
}

inline void drawStatsScreen() {
  tft.fillRect(0, 24, SCREEN_W, 180, COL_BG);
  drawStatCard(  0, "Feed",   COL_FEED,   COL_FEED_BG,   EV_FEED);
  drawStatCard(106, "Diaper", COL_DIAPER, COL_DIAPER_BG, EV_DIAPER);
  drawStatCard(218, "Sleep",  COL_SLEEP,  COL_SLEEP_BG,  EV_SLEEP);
}

inline void refreshTimerDisplay() {
  drawStatsScreen();
}

// =========================================================================
// Log screen — paginated event list
// =========================================================================

inline const char* typeLabel(uint8_t t) {
  switch (t) {
    case EV_FEED:   return "Feed";
    case EV_DIAPER: return "Diaper";
    case EV_SLEEP:  return "Sleep";
  }
  return "?";
}

inline uint16_t typeColor(uint8_t t) {
  switch (t) {
    case EV_FEED:   return COL_FEED;
    case EV_DIAPER: return COL_DIAPER;
    case EV_SLEEP:  return COL_SLEEP;
  }
  return COL_TEXT;
}

inline void drawLogScreen(uint8_t daysBack) {
  tft.fillRect(0, 24, SCREEN_W, 180, COL_BG);

  char date[16];
  getDateString(daysBack, date, sizeof(date));
  drawTextCentered(SCREEN_W / 2, 6, date, COL_TEXT_DIM, 1);

  const Event* day[64];
  uint16_t n = getEventsForDay(daysBack, day, 64);

  uint16_t totalPages = (n + EVENTS_PER_PAGE - 1) / EVENTS_PER_PAGE;
  if (totalPages == 0) totalPages = 1;
  if (logPage >= totalPages) logPage = totalPages - 1;

  // Pagination buttons
  if (logPage > 0) {
    tft.fillTriangle(SCREEN_W / 2 - 8, 46,
                     SCREEN_W / 2 + 8, 46,
                     SCREEN_W / 2,     30, COL_TEXT_DIM);
  }
  if (logPage + 1 < totalPages) {
    tft.fillTriangle(SCREEN_W / 2 - 8, 180,
                     SCREEN_W / 2 + 8, 180,
                     SCREEN_W / 2,     196, COL_TEXT_DIM);
  }

  uint16_t start = logPage * EVENTS_PER_PAGE;
  for (uint8_t i = 0; i < EVENTS_PER_PAGE; i++) {
    uint16_t idx = start + i;
    if (idx >= n) break;
    const Event* e = day[idx];
    int16_t y = 32 + i * 30;

    bool armed = (pendingDeleteId == e->id);
    uint16_t rowBg = armed ? COL_DEL_ARM : COL_BAR;
    tft.fillRoundRect(8, y, SCREEN_W - 16, 26, 4, rowBg);

    char tbuf[8];
    getEventTimeString(e->timestamp, tbuf, sizeof(tbuf));
    drawTextAt(16, y + 8, tbuf, COL_TEXT, 1);

    drawTextAt(72, y + 8, typeLabel(e->type), typeColor(e->type), 1);

    if (e->duration > 0) {
      char dbuf[16];
      snprintf(dbuf, sizeof(dbuf), "%u min", e->duration);
      drawTextAt(160, y + 8, dbuf, COL_TEXT_DIM, 1);
    }

    // Delete X (armed = red box)
    uint16_t delColor = armed ? COL_DEL_CONFIRM : COL_TEXT_DIM;
    if (armed) {
      tft.fillRoundRect(SCREEN_W - 30, y + 2, 22, 22, 3, COL_DEL_CONFIRM);
      drawTextAt(SCREEN_W - 23, y + 8, "X", COL_TEXT, 1);
    } else {
      drawTextAt(SCREEN_W - 23, y + 8, "x", delColor, 1);
    }
  }

  if (n == 0) {
    drawTextCentered(SCREEN_W / 2, 110, "No events", COL_TEXT_DIM, 2);
  }
}

// =========================================================================
// Forecast screen
// =========================================================================

inline void drawForecastHourly() {
  tft.fillRect(0, 24, SCREEN_W, 180, COL_BG);

  // Toggle arrows
  tft.fillTriangle(SCREEN_W / 2 - 8, 46,
                   SCREEN_W / 2 + 8, 46,
                   SCREEN_W / 2,     30, COL_TEXT);
  drawTextCentered(SCREEN_W / 2, 50, "Hourly", COL_TEXT_DIM, 1);

  if (!wxValid) {
    drawTextCentered(SCREEN_W / 2, 110, "No data", COL_TEXT_DIM, 2);
    return;
  }

  char now[16];
  snprintf(now, sizeof(now), "%.1f C  %u%%",
           wxNow.temp, (unsigned)wxNow.humidity);
  drawTextCentered(SCREEN_W / 2, 64, now, COL_TEXT, 2);
  drawTextCentered(SCREEN_W / 2, 84, weatherCodeShort(wxNow.code),
                   typeColor(EV_DIAPER), 1);

  // 6 next hours as mini-cards
  int16_t x0 = 8;
  int16_t y  = 100;
  int16_t cw = (SCREEN_W - 16) / 6;
  for (uint8_t i = 0; i < 6; i++) {
    if (wxHourly[i].ts == 0) break;
    int16_t cx = x0 + i * cw;

    time_t t = wxHourly[i].ts;
    struct tm* tm = localtime(&t);
    char hbuf[6];
    strftime(hbuf, sizeof(hbuf), "%H", tm);

    drawTextCentered(cx + cw / 2, y, hbuf, COL_TEXT_DIM, 1);

    char tbuf[8];
    snprintf(tbuf, sizeof(tbuf), "%d", (int)wxHourly[i].temp);
    drawTextCentered(cx + cw / 2, y + 14, tbuf, COL_TEXT, 2);

    char rbuf[8];
    snprintf(rbuf, sizeof(rbuf), "%u%%", (unsigned)wxHourly[i].rain);
    uint16_t rc = wxHourly[i].rain >= 30 ? COL_RAIN : COL_TEXT_DIM;
    drawTextCentered(cx + cw / 2, y + 34, rbuf, rc, 1);
  }

  drawTextCentered(SCREEN_W / 2, 180, "v 3-Day", COL_TEXT_DIM, 1);
}

inline void drawForecastDaily() {
  tft.fillRect(0, 24, SCREEN_W, 180, COL_BG);
  drawTextCentered(SCREEN_W / 2, 30, "^ Hourly", COL_TEXT_DIM, 1);
  drawTextCentered(SCREEN_W / 2, 50, "3-Day", COL_TEXT_DIM, 1);

  if (!wxValid) {
    drawTextCentered(SCREEN_W / 2, 110, "No data", COL_TEXT_DIM, 2);
    return;
  }

  for (uint8_t i = 0; i < DAILY_SLOTS; i++) {
    if (wxDaily[i].ts == 0) break;
    int16_t y = 68 + i * 38;
    tft.fillRoundRect(8, y, SCREEN_W - 16, 32, 4, COL_BAR);

    time_t t = wxDaily[i].ts;
    struct tm* tm = localtime(&t);
    char dbuf[8];
    strftime(dbuf, sizeof(dbuf), "%a", tm);
    drawTextAt(16, y + 10, dbuf, COL_TEXT, 2);

    char hilo[20];
    snprintf(hilo, sizeof(hilo), "%d / %d C",
             (int)wxDaily[i].hi, (int)wxDaily[i].lo);
    drawTextAt(80, y + 10, hilo, COL_TEXT, 2);

    char rbuf[10];
    snprintf(rbuf, sizeof(rbuf), "%u%%", (unsigned)wxDaily[i].rain);
    uint16_t rc = wxDaily[i].rain >= 30 ? COL_RAIN : COL_TEXT_DIM;
    drawTextAt(220, y + 10, rbuf, rc, 2);

    drawTextAt(270, y + 10, weatherCodeShort(wxDaily[i].code),
               typeColor(EV_DIAPER), 1);
  }
}

inline void drawForecastScreen() {
  if (forecastView == 0) drawForecastHourly();
  else                   drawForecastDaily();
}

// =========================================================================
// Top-level renderer
// =========================================================================

inline void renderScreen(Screen s) {
  drawHeader();
  switch (s) {
    case SCREEN_FORECAST:  drawForecastScreen();    break;
    case SCREEN_STATS:     drawStatsScreen();       break;
    case SCREEN_TODAY:     drawLogScreen(0);        break;
    case SCREEN_YESTERDAY: drawLogScreen(1);        break;
    case SCREEN_TWO_DAYS:  drawLogScreen(2);        break;
  }
  drawNavBar(s);
}
