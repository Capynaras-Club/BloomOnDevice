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

// =========================================================================
// SPI speeds
// =========================================================================

#define TFT_SPI_HZ      40000000
#define TOUCH_SPI_HZ     2500000

// =========================================================================
// Display geometry — landscape 320x240
// =========================================================================

#define SCREEN_W        240
#define SCREEN_H        320
#define TFT_ROTATION    2

// =========================================================================
// Touch calibration (raw XPT2046 min/max)
// =========================================================================

#define TOUCH_MIN_X    200
#define TOUCH_MAX_X   3900
#define TOUCH_MIN_Y    240
#define TOUCH_MAX_Y   3800

// =========================================================================
// UI colors (RGB565)
// =========================================================================

#define COL_BG            0x0863  // #0C0C18
#define COL_BAR           0x0843  // #10101E
#define COL_DIVIDER       0x2947  // #2A2A3E
#define COL_TEXT          0xEF3F  // #E8E8FF
#define COL_TEXT_DIM      0x7394  // #7070A0
#define COL_CLOCK         0xD69F  // #D0D0FF

#define COL_FEED          0xF4EC  // #F4A261
#define COL_FEED_BG       0x28C1  // #2A1A0E
#define COL_DIAPER        0x7E5C  // #7EC8E3
#define COL_DIAPER_BG     0x08E4  // #0A1E26
#define COL_SLEEP         0xC57E  // #C0AEF5
#define COL_SLEEP_BG      0x187D  // #160F2E

#define COL_DEL_ARM       0x2061  // #2A0A0A
#define COL_DEL_CONFIRM   0xC185  // #C0302A
#define COL_RAIN          0x7E5C  // #7EC8E3

// =========================================================================
// Power management
// =========================================================================

#define BACKLIGHT_FULL      255
#define BACKLIGHT_DIM        64
#define DIM_TIMEOUT_MS    30000
#define SLEEP_TIMEOUT_MS  40000

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
