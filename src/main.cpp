// Bloom on Device — Sunton ESP32-2432S024R + ILI9341 + XPT2046
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
#include <driver/gpio.h>
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

// TFT on HSPI; touch is wired to a separate bus and uses the default SPI.
SPIClass             tftSPI(HSPI);
Adafruit_ILI9341     tft(&tftSPI, PIN_TFT_DC, PIN_TFT_CS, PIN_TFT_RST);
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
    // T_IRQ (GPIO36 = RTC_GPIO0) pulls low on touch; ext0 wakes on the low
    // level. ext0 is the classic-ESP32-friendly path for an RTC GPIO.
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
  Serial.println("\n[boot] bloom on device");

  // Split SPI: TFT on HSPI, touch on the default SPI (VSPI).
  tftSPI.begin(PIN_TFT_SCK, PIN_TFT_MISO, PIN_TFT_MOSI, PIN_TFT_CS);
  SPI.begin(PIN_TOUCH_SCK, PIN_TOUCH_MISO, PIN_TOUCH_MOSI, PIN_TOUCH_CS);

  backlightInit();
  displayInit();
  touchInit();
  storageInit();
  loadEvents();
  // Pruning is deferred until after NTP sync — pruneOldEvents() now
  // refuses to run without a valid clock (otherwise it would silently
  // drop everything as "older than HISTORY_DAYS before epoch").

  // Splash
  drawTextCentered(SCREEN_W / 2, 100, "Bloom on Device", COL_TEXT, 2);
  // Build stamp: __DATE__/__TIME__ are set by the compiler at this TU's
  // build, so every flash shows a unique line — confirms what's running.
  drawTextCentered(SCREEN_W / 2, 124, __DATE__ " " __TIME__, COL_TEXT_DIM, 1);
  drawTextCentered(SCREEN_W / 2, 144, "starting...", COL_TEXT_DIM, 1);

  // WiFi — connects to saved creds, or runs the WiFiManager captive
  // portal at "BloomSetup" until a network is configured or the
  // portal times out. Returns false → offline mode.
  //
  // Tell the user what's happening: if there are no saved creds we'll be
  // sitting on the captive portal for up to 3 minutes, so show the SSID
  // and IP they need to join. Otherwise just say we're connecting.
  if (WifiMgr::hasSavedCredentials()) {
    drawTextCentered(SCREEN_W / 2, 174, "connecting wifi...", COL_TEXT_DIM, 1);
  } else {
    drawTextCentered(SCREEN_W / 2, 174, "first-time setup:", COL_TEXT, 1);
    drawTextCentered(SCREEN_W / 2, 192, "join wifi 'BloomSetup'", COL_TEXT, 1);
    drawTextCentered(SCREEN_W / 2, 208, "then open 192.168.4.1", COL_TEXT_DIM, 1);
  }
  if (WifiMgr::ensureConnected()) {
    hasWifi = true;
    if (fetchLocation()) {
      WifiMgr::setTimezone(location.posix_tz);
      // Clock is set now — drop events older than HISTORY_DAYS and
      // flush the trimmed log to flash. No-op if nothing aged out.
      if (pruneOldEvents()) saveEvents();
      fetchWeather();
      lastWeatherFetch = millis();
    }
  } else {
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

  // Touch sanity gate. Two conditions must both hold:
  //   1. tirqTouched() — the XPT2046 lib's FALLING-edge ISR has fired
  //      since the last getPoint(). A statically-low T_IRQ (which we saw
  //      on this board) trips digitalRead() forever but only fires the
  //      ISR once, so this filters that case.
  //   2. z is in [TOUCH_Z_MIN, TOUCH_Z_MAX]. SPI reads from a misbehaving
  //      XPT2046 come back as z=0 or z=4095; real taps live in the middle.
  if (touch.tirqTouched()) {
    TS_Point p = touch.getPoint();
    if (p.z >= TOUCH_Z_MIN && p.z <= TOUCH_Z_MAX) {
      uint32_t now = millis();
      if (now - lastTouch > TOUCH_DEBOUNCE_MS) {
        lastTouch = now;
        onActivity();

        uint16_t x = toScreenX(p.x);
        uint16_t y = toScreenY(p.y);
        Serial.printf("[touch] raw=(%4u,%4u) z=%4u  screen=(%3u,%3u)\n",
                      p.x, p.y, p.z, x, y);
        handleTouch(x, y);
        needsRender = true;
      }
    } else {
      // Logging rejects too so we can see what the panel is sending and
      // tune TOUCH_Z_MIN/MAX if real taps fall outside the current band.
      Serial.printf("[touch] REJECTED raw=(%4u,%4u) z=%4u (band %u..%u)\n",
                    p.x, p.y, p.z,
                    (unsigned)TOUCH_Z_MIN, (unsigned)TOUCH_Z_MAX);
    }
  }

  // Auto-cancel pending delete after timeout
  if (pendingDeleteId &&
      millis() - pendingDeleteMs > DELETE_CONFIRM_MS) {
    pendingDeleteId = 0;
    needsRender = true;
  }

  // Live "X min ago" tick on the stats screen. Partial repaint — wiping
  // only the age line of each card, not the whole screen — so the minute
  // tick is invisible rather than a full-screen flash every 60 s.
  if (currentScreen == SCREEN_STATS &&
      millis() - lastRefresh > TIMER_REFRESH_MS) {
    lastRefresh = millis();
    refreshStatsAges();
  }

  // Periodic weather refresh
  if (hasWifi && millis() - lastWeatherFetch > WEATHER_REFRESH_MS) {
    lastWeatherFetch = millis();
    fetchWeather();
    if (currentScreen == SCREEN_FORECAST) needsRender = true;
  }

  if (needsRender) renderScreen(currentScreen);

  updatePowerState();

  // Yield 10ms each iteration: feeds the watchdog, lets IDLE/WiFi/lwIP tasks
  // run, and allows the core to drop into automatic light sleep between
  // ticks. 10ms is well below the touch debounce window so there is no
  // perceptible latency cost.
  vTaskDelay(pdMS_TO_TICKS(10));
}
