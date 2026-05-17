#pragma once

// =========================================================================
// Pin definitions — Sunton ESP32-2432S024R (CYD 2.4" resistive)
//   - TFT (ILI9341)  on HSPI: SCK=14, MOSI=13, MISO=12, CS=15, DC=2, BL=27
//   - Touch (XPT2046) on VSPI: SCK=25, MOSI=32, MISO=39, CS=33, IRQ=36
//   - TFT RST is tied to the ESP_EN line, so the MCU pin is -1.
//   - GPIO36 is RTC_GPIO0 → ext0 light-sleep wake works on Touch IRQ.
// =========================================================================

// TFT bus (HSPI)
#define PIN_TFT_SCK     14
#define PIN_TFT_MOSI    13
#define PIN_TFT_MISO    12
#define PIN_TFT_CS      15
#define PIN_TFT_DC       2
#define PIN_TFT_RST     -1
#define PIN_BACKLIGHT   27

// Touch bus (VSPI / default SPI)
#define PIN_TOUCH_SCK   25
#define PIN_TOUCH_MOSI  32
#define PIN_TOUCH_MISO  39
#define PIN_TOUCH_CS    33
#define PIN_TOUCH_IRQ   36

// BOOT pushbutton on the CYD: connects GPIO0 to GND when pressed, sits
// HIGH otherwise thanks to the strapping-pin pull-up. Read at runtime to
// offer a "skip WiFi setup" escape hatch on the splash screen.
#define PIN_BOOT_BUTTON  0
#define SKIP_WIFI_WINDOW_MS  3000

// =========================================================================
// SPI speeds
// =========================================================================

#define TFT_SPI_HZ      40000000
#define TOUCH_SPI_HZ     2500000

// =========================================================================
// Display geometry — portrait UI (240×320 logical).
//
// Adafruit_ILI9341::setRotation() updates _width/_height to match the
// requested rotation. The MV-transpose path (rotation 1/3) leaves the
// library convinced the canvas is 320×240, so every draw at y > 239 in
// our portrait 240×320 coord space was being silently clipped — that's
// the 80-px noise band at the bottom of the panel we kept seeing.
//
// Rotation 0 lines the library up with the physical panel without any
// transpose: _width=240, _height=320 matches SCREEN_W/SCREEN_H one-to-one
// and the full panel gets cleared on fillScreen(). If the image ends up
// upside-down for your case orientation, change to 2 (also MX/MY only,
// no MV). 1 / 3 are landscape and will reintroduce the clipping.
// =========================================================================

#define SCREEN_W        240
#define SCREEN_H        320
#define TFT_ROTATION    0

// =========================================================================
// Touch calibration (raw XPT2046 min/max)
// =========================================================================

#define TOUCH_MIN_X    200
#define TOUCH_MAX_X   3900
#define TOUCH_MIN_Y    240
#define TOUCH_MAX_Y   3800

// =========================================================================
// UI colors (RGB565)
//
// The 2.4" CYD panel renders narrow gamut and is viewed from arm's length
// in a dim room. Subtle palettes wash out — every card looked like the
// same olive blob in the field photo. Boost saturation, push text to pure
// white, and lean on hue contrast between cards instead of brightness.
// =========================================================================

#define COL_BG            0x0000  // #000000 — pure black, max contrast against white text
#define COL_BAR           0x10A2  // #131515 — header/nav strip, slightly lifted from BG
#define COL_DIVIDER       0x4208  // #424242
#define COL_TEXT          0xFFFF  // #FFFFFF — pure white
#define COL_TEXT_DIM      0xC618  // #C0C0C0 — light grey, still readable
#define COL_CLOCK         0xFFFF  // pure white, the clock is the headline

// Per-type accent hues. Used as the card background AND the type-name
// color on log rows, so a glance at the log instantly shows which row
// is a feed/diaper/sleep. drawStatCard always renders card text in
// COL_TEXT (white) on top of these for guaranteed contrast.
#define COL_FEED          0xC2A0  // #C45000 — warm saturated orange
#define COL_FEED_BG       0xC2A0
#define COL_DIAPER        0x033F  // #0066FF — saturated blue
#define COL_DIAPER_BG     0x033F
#define COL_SLEEP         0x6817  // #6802C0 — saturated purple
#define COL_SLEEP_BG      0x6817

#define COL_DEL_ARM       0x8000  // #800000 — dark red row
#define COL_DEL_CONFIRM   0xF800  // #FF0000 — full red on confirm
#define COL_RAIN          0x07FF  // #00FFFF — cyan rain indicator

// =========================================================================
// Power management
// =========================================================================

#define BACKLIGHT_FULL      255
#define BACKLIGHT_DIM        64
#define DIM_TIMEOUT_MS    30000
#define SLEEP_TIMEOUT_MS  40000

// Backup wake interval during light sleep, in seconds. Touch-IRQ wake is
// the primary path, but if T_IRQ is stuck the chip would never wake on
// its own. Timer wake re-renders clock/weather every period and falls
// back to sleep immediately if nothing actually changed.
#define SLEEP_WAKE_PERIOD_S  300

// =========================================================================
// Behavior
// =========================================================================

#define TOUCH_DEBOUNCE_MS   200
#define TIMER_REFRESH_MS  60000
#define WEATHER_REFRESH_MS 1800000UL    // 30 min
#define DELETE_CONFIRM_MS  2000

// Real XPT2046 taps fall in roughly 100..3500. Below = noise, at/above 4000 =
// the chip isn't responding (MISO floating, garbage bytes). Both filtered.
#define TOUCH_Z_MIN          100
#define TOUCH_Z_MAX         3500

#define EVENTS_PER_PAGE      5
#define HISTORY_DAYS         3

#define MAX_EVENTS         500

// =========================================================================
// Event types
// =========================================================================

#define EV_FEED     0
#define EV_DIAPER   1
#define EV_SLEEP    2
