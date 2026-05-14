# Baby Stats Display

Bedside baby tracker on ESP32-C3 with a 2.4" ILI9341 TFT touchscreen.

## Screens

```
[Forecast] <-> [Stats] <-> [Today] <-> [Yesterday] <-> [2 Days Ago]
```

Boot lands on **Stats**. Tap a card to log a feed, diaper, or sleep. Swipe
through log days with the side arrows; page within a day with the up/down
triangles. Tap the `x` on a row once to arm deletion, again within 2s to
confirm. If WiFi isn't available, the forecast screen is hidden.

All times are displayed in 24-hour format.

## Hardware

- ESP32-C3-DevKitM-1
- 2.4" ILI9341 TFT + XPT2046 touch (320x240)

### Pinout

| Signal      | ESP32-C3 GPIO | Display Pin |
|-------------|--------------:|------------:|
| SPI Clock   | GPIO6         | CLK         |
| MOSI        | GPIO7         | DIN         |
| MISO        | GPIO2         | DO          |
| Display CS  | GPIO10        | CS          |
| Display DC  | GPIO3         | DC          |
| Display RST | GPIO4         | RST         |
| Touch CS    | GPIO8         | T_CS        |
| Touch IRQ   | GPIO9         | T_IRQ       |
| Backlight   | GPIO5 (PWM)   | LED         |
| Power       | 3V3           | VCC         |
| Ground      | GND           | GND         |

## Building

### PlatformIO

```bash
pio run --target upload
pio device monitor
```

### Arduino IDE

- Board: `ESP32C3 Dev Module`
- Flash Size: `4MB`
- Partition Scheme: `Default 4MB with spiffs`
- Upload Speed: `921600`

Required libraries:

- Adafruit GFX
- Adafruit ILI9341
- XPT2046_Touchscreen (Paul Stoffregen)
- ArduinoJson

## First boot

If no WiFi credentials are stored, the device starts a SoftAP named
`BabyStatsSetup`. Connect from a phone, browse to `192.168.4.1`, and enter
your SSID and password. The device reboots and connects.

Once online, location is auto-detected via ip-api.com and weather pulled from
Open-Meteo. NTP syncs the clock to your local timezone.

## Power

| State        | Current  |
|--------------|----------|
| Active       | ~80 mA   |
| Dim (30s)    | ~40 mA   |
| Light sleep  | ~2 mA    |

Touch wakes the device from light sleep via the XPT2046 IRQ line.

## Files

```
src/
  main.cpp     — setup(), loop(), power management
  config.h     — pins, colors, timeouts
  display.h    — ILI9341 rendering, 24h time formatting
  touch.h      — XPT2046 calibration and hit zones
  events.h     — Event struct, log/delete/query
  storage.h    — SPIFFS persistence with 3-day pruning
  weather.h    — ip-api + Open-Meteo fetch and parse
  wifi_mgr.h   — saved-credential connect + SoftAP provisioning + NTP
  ui.h         — screen enum + WiFi-aware navigation
```
