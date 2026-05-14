// Baby Stats Display — ESP32-C3 + ILI9341 + XPT2046
//
// Five-screen bedside display: Forecast | Stats | Today | Yesterday | 2 Days Ago.
// Stats screen logs feeds, diapers and sleeps; log screens page through history
// with tap-to-arm + tap-to-confirm delete. Weather via Open-Meteo, location
// auto-detected via ip-api.com. Light sleep between interactions.

#include <Arduino.h>
#include <SPI.h>
#include <Adafruit_ILI9341.h>
#include <XPT2046_Touchscreen.h>
#include <esp_sleep.h>
#include <time.h>

#include "config.h"
#include "ui.h"
#include "events.h"
#include "storage.h"
#include "weather.h"
#include "wifi_mgr.h"
#include "touch.h"
#include "display.h"

// =========================================================================
// Globals
// =========================================================================

Adafruit_ILI9341     tft(PIN_TFT_CS, PIN_TFT_DC, PIN_TFT_RST);
XPT2046_Touchscreen  touch(PIN_TOUCH_CS, PIN_TOUCH_IRQ);

Screen        currentScreen   = SCREEN_STATS;
bool          hasWifi         = false;
uint8_t       forecastView    = 0;
uint8_t       logPage         = 0;

uint32_t      pendingDeleteId = 0;
uint32_t      pendingDeleteMs = 0;

Location      location        = {};
WeatherCurrent wxNow          = {};
WeatherHourly  wxHourly[HOURLY_SLOTS] = {};
WeatherDaily   wxDaily[DAILY_SLOTS]   = {};
bool           wxValid        = false;

static uint32_t lastRefresh      = 0;
static uint32_t lastWeatherFetch = 0;
static uint32_t lastTouch        = 0;
static uint32_t lastActivity     = 0;
static bool     isDimmed         = false;
static bool     isAsleep         = false;

// =========================================================================
// Activity / power
// =========================================================================

void onActivity() {
  lastActivity = millis();
  if (isDimmed) {
    backlightSet(BACKLIGHT_FULL);
    isDimmed = false;
  }
}

void updatePowerState() {
  uint32_t idle = millis() - lastActivity;

  if (idle > SLEEP_TIMEOUT_MS && !isAsleep) {
    isAsleep = true;
    backlightSet(0);
    esp_sleep_enable_ext0_wakeup((gpio_num_t)PIN_TOUCH_IRQ, 0);
    esp_light_sleep_start();
    // Resumes here on touch
    lastActivity = millis();
    isDimmed = isAsleep = false;
    backlightSet(BACKLIGHT_FULL);
    renderScreen(currentScreen);
  } else if (idle > DIM_TIMEOUT_MS && !isDimmed && !isAsleep) {
    backlightSet(BACKLIGHT_DIM);
    isDimmed = true;
  }
}

// =========================================================================
// Touch dispatch
// =========================================================================

static void handleDeleteTap(uint32_t id) {
  uint32_t now = millis();
  if (pendingDeleteId == id && now - pendingDeleteMs < DELETE_CONFIRM_MS) {
    deleteEvent(id);
    pendingDeleteId = 0;
  } else {
    pendingDeleteId = id;
    pendingDeleteMs = now;
  }
}

static void handleTouchOnStats(uint16_t x, uint16_t y) {
  int8_t card = hitStatsCard(x, y);
  if (card >= 0) {
    logEvent((uint8_t)card);
  }
}

static void handleTouchOnLog(uint16_t x, uint16_t y, uint8_t daysBack) {
  // Pagination
  if (hitPageUp(x, y) && logPage > 0) {
    logPage--;
    return;
  }
  if (hitPageDown(x, y)) {
    const Event* day[64];
    uint16_t n = getEventsForDay(daysBack, day, 64);
    uint16_t totalPages = (n + EVENTS_PER_PAGE - 1) / EVENTS_PER_PAGE;
    if (logPage + 1 < totalPages) {
      logPage++;
      return;
    }
  }

  // Per-row delete
  int8_t row = hitDeleteRow(x, y);
  if (row >= 0) {
    const Event* day[64];
    uint16_t n = getEventsForDay(daysBack, day, 64);
    uint16_t idx = logPage * EVENTS_PER_PAGE + row;
    if (idx < n) handleDeleteTap(day[idx]->id);
  }
}

static void handleTouchOnForecast(uint16_t x, uint16_t y) {
  int8_t t = hitForecastToggle(x, y);
  if (t == 0) forecastView = 0;
  if (t == 1) forecastView = 1;
}

void handleTouch(uint16_t x, uint16_t y) {
  // Nav arrows take priority
  if (hitLeftArrow(x, y) && hasLeftArrow(currentScreen)) {
    currentScreen   = prevScreen(currentScreen);
    logPage         = 0;
    pendingDeleteId = 0;
    return;
  }
  if (hitRightArrow(x, y) && hasRightArrow(currentScreen)) {
    currentScreen   = nextScreen(currentScreen);
    logPage         = 0;
    pendingDeleteId = 0;
    return;
  }

  switch (currentScreen) {
    case SCREEN_FORECAST:  handleTouchOnForecast(x, y);       break;
    case SCREEN_STATS:     handleTouchOnStats(x, y);          break;
    case SCREEN_TODAY:     handleTouchOnLog(x, y, 0);         break;
    case SCREEN_YESTERDAY: handleTouchOnLog(x, y, 1);         break;
    case SCREEN_TWO_DAYS:  handleTouchOnLog(x, y, 2);         break;
  }
}

// =========================================================================
// Setup
// =========================================================================

void setup() {
  Serial.begin(115200);
  delay(100);
  Serial.println("\n[boot] baby stats display");

  // Shared SPI bus for display and touch
  SPI.begin(PIN_SCK, PIN_MISO, PIN_MOSI);

  backlightInit();
  displayInit();
  touchInit();
  storageInit();
  loadEvents();
  pruneOldEvents();

  // Splash
  drawTextCentered(SCREEN_W / 2, 100, "Baby Stats",  COL_TEXT, 3);
  drawTextCentered(SCREEN_W / 2, 140, "starting...", COL_TEXT_DIM, 1);

  // WiFi
  if (WifiMgr::connectSaved()) {
    hasWifi = true;
    drawTextCentered(SCREEN_W / 2, 160, "wifi ok", COL_TEXT_DIM, 1);
    if (fetchLocation()) {
      WifiMgr::setTimezone(location.timezone);
      fetchWeather();
      lastWeatherFetch = millis();
    }
  } else {
    drawTextCentered(SCREEN_W / 2, 160, "no wifi (offline mode)", COL_TEXT_DIM, 1);
    hasWifi = false;
  }

  currentScreen = SCREEN_STATS;
  lastActivity  = millis();
  delay(800);
  renderScreen(currentScreen);
}

// =========================================================================
// Loop
// =========================================================================

void loop() {
  bool needsRender = false;

  if (touch.touched()) {
    uint32_t now = millis();
    if (now - lastTouch > TOUCH_DEBOUNCE_MS) {
      lastTouch = now;
      onActivity();

      TS_Point p = touch.getPoint();
      uint16_t x = toScreenX(p.x);
      uint16_t y = toScreenY(p.y);
      handleTouch(x, y);
      needsRender = true;
    }
  }

  // Auto-cancel pending delete after timeout
  if (pendingDeleteId &&
      millis() - pendingDeleteMs > DELETE_CONFIRM_MS) {
    pendingDeleteId = 0;
    needsRender = true;
  }

  // Live timer refresh on stats screen
  if (currentScreen == SCREEN_STATS &&
      millis() - lastRefresh > TIMER_REFRESH_MS) {
    lastRefresh = millis();
    needsRender = true;
  }

  // Periodic weather refresh
  if (hasWifi && millis() - lastWeatherFetch > WEATHER_REFRESH_MS) {
    lastWeatherFetch = millis();
    fetchWeather();
    if (currentScreen == SCREEN_FORECAST) needsRender = true;
  }

  if (needsRender) renderScreen(currentScreen);

  updatePowerState();
}
