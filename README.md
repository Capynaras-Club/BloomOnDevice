# Baby Stats Display

![build](https://github.com/Capynaras-Club/BloomOnDevice/actions/workflows/build.yml/badge.svg)

> **Just want to build one?** Head to **[GETTING_STARTED.md](GETTING_STARTED.md)**
> — a zero-knowledge walkthrough with the bill of materials, wiring, and a
> one-click in-browser installer. No toolchain required.
>
> Or open the installer directly:
> **[capynaras-club.github.io/BloomOnDevice/install/](https://capynaras-club.github.io/BloomOnDevice/install/)**

Bedside baby tracker on ESP32-C3 with a 2.4" ILI9341 TFT touchscreen. Tap to
log a feed, diaper, or sleep. See the last event of each type, today's count,
the next few hours of weather, and three days of history. All times shown in
24-hour format.

The rest of this README covers building from source. New users should start
with [GETTING_STARTED.md](GETTING_STARTED.md) instead.

---

## Table of contents

1. [Screens & gestures](#screens--gestures)
2. [Bill of materials](#bill-of-materials)
3. [Wiring](#wiring)
4. [Toolchain setup](#toolchain-setup)
5. [Flashing the device](#flashing-the-device)
6. [First boot — WiFi provisioning](#first-boot--wifi-provisioning)
7. [Daily use](#daily-use)
8. [Power management](#power-management)
9. [Troubleshooting](#troubleshooting)
10. [Configuration knobs](#configuration-knobs)
11. [File layout](#file-layout)

---

## Screens & gestures

```
[Forecast] <-> [Stats] <-> [Today] <-> [Yesterday] <-> [2 Days Ago]
```

- Boot lands on **Stats**.
- `<` and `>` on the bottom bar move between screens.
- **Stats**: tap the **Feed**, **Diaper**, or **Sleep** card to log an event
  with the current timestamp.
- **Today / Yesterday / 2 Days Ago**: ▲ / ▼ in the centre of the top and
  bottom pages through 5 events at a time.
- **Forecast**: ▲ for hourly, ▼ for 3-day. Only visible when WiFi is up.
- **Delete an event**: tap the `x` on a log row — the row turns red. Tap it
  again within 2 seconds to confirm. Anywhere else cancels the arm.

---

## Bill of materials

| Part | Notes |
|---|---|
| ESP32-C3-DevKitM-1 | Any ESP32-C3 module works; pin numbers in `src/config.h` are for the DevKitM-1 |
| 2.4" ILI9341 TFT, 320×240, with XPT2046 touch | The common "MSP2402"-style boards work fine |
| USB-C cable (data, not just power) | For flashing and serial monitor |
| Dupont jumper wires, 10× | M-F if breadboarding the dev kit |
| (optional) 3.7V LiPo + TP4056 charger | For battery operation |
| (optional) 3D-printed enclosure | Not provided here |

The display module includes its own 3.3V regulator and level shifters on
most variants, so it can be driven directly from the dev kit's 3V3 pin.

---

## Wiring

Single SPI bus shared between the display and the touch controller.

| Signal      | ESP32-C3 GPIO | Display pin |
|-------------|--------------:|------------:|
| SPI Clock   | GPIO6         | `CLK` / `SCK` |
| MOSI        | GPIO7         | `DIN` / `MOSI` |
| MISO        | GPIO2         | `DO` / `MISO`  |
| Display CS  | GPIO10        | `CS`           |
| Display DC  | GPIO3         | `DC`           |
| Display RST | GPIO4         | `RST`          |
| Touch CS    | GPIO8         | `T_CS`         |
| Touch IRQ   | GPIO9         | `T_IRQ`        |
| Backlight   | GPIO5 (PWM)   | `LED`          |
| Power       | 3V3           | `VCC`          |
| Ground      | GND           | `GND`          |

Notes:

- The touch panel's `T_DIN`, `T_DO`, `T_CLK` lines share the bus with the
  display, so they go to the same `MOSI`/`MISO`/`SCK` pins above.
- `T_IRQ` **must** be on a pin that supports `ext0` light-sleep wakeup —
  GPIO0–GPIO5 on the C3. GPIO9 is used here; if you change it, update
  `PIN_TOUCH_IRQ` in `src/config.h`.
- Some cheap modules want 5 V on `VCC` despite shipping with 3.3 V logic.
  Read the silkscreen on yours. The `LED` pin tolerates 3.3 V PWM.

If you change any pin, edit `src/config.h` and re-flash. Nothing else needs
to change.

---

## Toolchain setup

Pick **one** of these. PlatformIO is recommended because the CI build uses
it and it pins library versions for you.

### Option A — PlatformIO (recommended)

```bash
# macOS / Linux
python3 -m pip install --user platformio

# Windows (PowerShell)
py -m pip install platformio
```

Or install the [PlatformIO IDE extension for VS Code][pio-vscode], which
bundles the CLI.

Verify:

```bash
pio --version
```

### Option B — Arduino IDE

1. Install Arduino IDE 2.x.
2. Add the ESP32 board package: **File → Preferences → Additional Board
   Manager URLs**, paste
   `https://espressif.github.io/arduino-esp32/package_esp32_index.json`,
   then **Tools → Board → Boards Manager**, search "esp32", install.
3. Install libraries from **Sketch → Include Library → Manage Libraries**:
   - `Adafruit GFX Library`
   - `Adafruit ILI9341`
   - `XPT2046_Touchscreen` (by Paul Stoffregen)
   - `ArduinoJson` (v7.x)
4. Board settings:
   - Board: **ESP32C3 Dev Module**
   - Flash Size: **4MB (32Mb)**
   - Partition Scheme: **Default 4MB with spiffs** (formatted as LittleFS at first boot)
   - Upload Speed: **921600**
   - USB CDC On Boot: **Enabled**

---

## Flashing the device

### From PlatformIO

```bash
pio run                          # compile only
pio run --target upload          # compile + flash
pio device monitor               # serial console at 115200
```

The first build downloads the ESP32 toolchain (~200 MB) and is slow.
Subsequent builds take a few seconds.

If `pio device monitor` can't find the port, list devices with `pio device list`
and pass it explicitly:

```bash
pio device monitor --port /dev/ttyACM0
```

### From Arduino IDE

1. Open `src/main.cpp` — Arduino IDE will offer to put it in a sketch
   folder; accept.
2. Connect the board over USB. The C3 enumerates as a CDC serial device.
3. **Sketch → Upload** (`Ctrl+U`).
4. Open the Serial Monitor at **115200 baud** to see boot logs.

### From a release

The fastest path if you don't want to set up a toolchain:

- **Nightly** (latest `main` commit) →
  `https://github.com/Capynaras-Club/BloomOnDevice/releases/tag/nightly`
- **Stable** (when one is cut) → the
  [releases page](https://github.com/Capynaras-Club/BloomOnDevice/releases)

Download `firmware.bin`, `bootloader.bin`, and `partitions.bin` and flash
with `esptool.py`:

```bash
pip install esptool
esptool.py --chip esp32c3 --port /dev/ttyACM0 --baud 921600 write_flash \
  0x0000  bootloader.bin \
  0x8000  partitions.bin \
  0x10000 firmware.bin
```

---

## First boot — WiFi provisioning

The very first time the device boots (or after credentials are cleared), it
launches a SoftAP captive portal:

1. Power on the device. The serial console prints:
   ```
   [wifi] starting SoftAP provisioning
   ```
2. On your phone or laptop, connect to the WiFi network named
   **`BabyStatsSetup`** (no password).
3. Browse to **`http://192.168.4.1`**. A small form appears.
4. Enter your home WiFi SSID and password, tap **Save**.
5. The device stores the credentials in NVS and reboots. It will then:
   - Reconnect to your network.
   - Hit `http://ip-api.com/json` to look up city, latitude, longitude, and
     timezone.
   - Sync the clock against `pool.ntp.org` using that timezone.
   - Fetch a weather forecast from Open-Meteo.

The whole sequence takes about 10 seconds on a healthy network.

Subsequent boots reuse the stored credentials and skip provisioning. To
force re-provisioning, hold the BOOT button while powering on — or, if
you'd rather, erase NVS with `esptool.py --chip esp32c3 erase_flash` and
re-flash.

**Offline mode:** if no WiFi is reachable, the device continues to boot
into Stats. The Forecast screen is hidden and the left-arrow on Stats is
greyed out. Logging, history, and delete all keep working — only the
clock is wrong until WiFi comes back.

---

## Daily use

- **Log a feed:** tap the orange **Feed** card. The "last" timestamp on the
  card updates immediately and "today" count increments.
- **Log a diaper change:** tap the blue **Diaper** card. Same flow.
- **Log a sleep:** tap the purple **Sleep** card. (Durations aren't tracked
  in the UI yet; the field exists in the `Event` struct for future use.)
- **Review history:** swipe right with `>` to Today, Yesterday, 2 Days Ago.
  Page within a day with the centre triangles.
- **Fix a mistake:** find the row, tap `x` (row turns red), tap `x` again
  within 2 seconds.
- **Check the weather:** swipe left from Stats to the Forecast screen.
  ▲ = next 6 hours, ▼ = next 3 days.

Events older than 3 days are pruned automatically on every boot — there's
no manual maintenance.

---

## Power management

| State        | Trigger                  | Current draw |
|--------------|--------------------------|--------------|
| Active       | Touch within 30 s        | ~80 mA       |
| Dim (25 %)   | 30 s idle                | ~40 mA       |
| Light sleep  | 40 s idle, backlight off | ~2 mA        |

Light sleep is woken by `T_IRQ` on the XPT2046 — touching the screen
anywhere resumes immediately and re-renders the previous screen. WiFi is
kept up so the clock and weather stay current.

The thresholds are in `src/config.h`:

```c
#define DIM_TIMEOUT_MS    30000
#define SLEEP_TIMEOUT_MS  40000
```

For battery use, a 1000 mAh LiPo gives roughly:

- Always-on: ~12 hours
- Mostly idle (typical bedside use): several days

---

## Troubleshooting

| Symptom | Likely cause | Fix |
|---|---|---|
| Blank screen | Backlight pin wrong, or 5 V/3.3 V mix-up | Re-check `LED` and `VCC` wiring |
| White screen, no graphics | SPI miswired or CS/DC swapped | Verify `CS`, `DC`, `RST` go to GPIO10/3/4 |
| Garbled colours / artefacts | SPI clock too fast for your wiring | Lower `TFT_SPI_HZ` in `src/config.h` to 16 MHz |
| Touch reports wrong coordinates | Calibration off | Edit `TOUCH_MIN_X/Y` and `TOUCH_MAX_X/Y` in `src/config.h` |
| Device never wakes from light sleep | `T_IRQ` not on a wake-capable pin | Move `T_IRQ` to GPIO0–GPIO5 and update `PIN_TOUCH_IRQ` |
| Clock stuck at 1970 | NTP didn't sync | Check serial log for `[ntp] time=...`; ensure UDP outbound is open |
| Wrong city / wrong timezone | `ip-api.com` geolocation off (VPN, mobile hotspot) | Connect through a non-VPN network or hard-code `location` in `weather.h` |
| Events lost on reboot | SPIFFS not mounted | Re-flash with the correct partition scheme (must include SPIFFS) |
| Build fails on `ledcSetup` | arduino-esp32 v3.x upgrade | Pin to `platform = espressif32@^6.5.0` in `platformio.ini` |

Enable verbose logs by changing `CORE_DEBUG_LEVEL=1` to `CORE_DEBUG_LEVEL=4`
in `platformio.ini`.

---

## Configuration knobs

Most behavior lives in `src/config.h`:

| Define | Default | Meaning |
|---|---|---|
| `TFT_SPI_HZ` | 27 MHz | Display SPI clock — drop if you see artefacts |
| `TOUCH_SPI_HZ` | 2.5 MHz | Touch SPI clock — XPT2046 is slow, don't raise |
| `TOUCH_DEBOUNCE_MS` | 200 | Minimum gap between touch events |
| `TIMER_REFRESH_MS` | 5000 | Stats screen "X min ago" refresh interval |
| `WEATHER_REFRESH_MS` | 1 800 000 (30 min) | How often to re-fetch the forecast |
| `DIM_TIMEOUT_MS` | 30000 | Idle time before dimming backlight |
| `SLEEP_TIMEOUT_MS` | 40000 | Idle time before light sleep |
| `EVENTS_PER_PAGE` | 5 | Rows per log page |
| `HISTORY_DAYS` | 3 | Days of history kept; older pruned on boot |
| `MAX_EVENTS` | 500 | Hard cap on event log (~5.5 KB RAM, ~5.5 KB flash) |

Re-flash after editing.

---

## License

[CC0 1.0 Universal](LICENSE) — public-domain dedication. Do whatever you
want with this code.

## Contributing

See [CONTRIBUTING.md](CONTRIBUTING.md).

## File layout

```
src/
  main.cpp     — setup(), loop(), touch dispatch, power management
  config.h     — pins, colors, timeouts, behavior constants
  display.h    — ILI9341 init, screen drawing, 24h time formatting
  touch.h      — XPT2046 init, calibration, hit zones
  events.h     — Event struct, log/delete/query, getLastOfType()
  storage.h    — SPIFFS save/load, 3-day pruning, globals
  weather.h    — ip-api + Open-Meteo fetch and parse
  wifi_mgr.h   — saved-credential connect, SoftAP portal, NTP sync
  ui.h         — screen enum, WiFi-aware navigation
.github/workflows/build.yml    — PlatformIO compile on every push/PR
.github/workflows/release.yml  — nightly + tagged firmware releases
                                  + GitHub Pages deploy of docs/install/

docs/install/
  index.html      — ESP Web Tools browser-based installer
  manifest.json   — chip family + flash offsets for esp-web-tools

GETTING_STARTED.md  — zero-knowledge walkthrough for end users
platformio.ini               — board, libs, partition scheme
```

[pio-vscode]: https://platformio.org/install/ide?install=vscode
