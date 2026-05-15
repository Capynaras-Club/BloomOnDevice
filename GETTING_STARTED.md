# Getting started

A bedside display that tracks your baby's feeds, diapers, and sleeps with a
single tap. No phone, no app — just a screen by the crib.

This page assumes **zero prior experience** with electronics or programming.
If you can plug in a USB cable and connect to WiFi, you can build this.

> **For developers:** if you want to compile from source, contribute, or
> change behavior, head to the [main README](README.md) instead.

---

## 1. What you need

| Part                                                         | Cost  | Where to look                          |
|--------------------------------------------------------------|-------|----------------------------------------|
| ESP32-C3-DevKitM-1 board                                     | ~$6   | AliExpress / Amazon — search the name  |
| 2.4" ILI9341 TFT touchscreen, 320×240, with XPT2046 touch    | ~$8   | "2.4 inch SPI TFT XPT2046"             |
| USB-C cable (must be a **data** cable, not just power)       | ~$3   | Anything you have lying around         |
| Female-to-female jumper wires (10 wires)                     | ~$2   | "dupont jumper wires"                  |

Total: **~$15-20**.

Optional, for cordless use:

| Part                                              | Cost  |
|---------------------------------------------------|-------|
| 1000 mAh LiPo battery + TP4056 charger module     | ~$5   |
| 3D-printed enclosure (no STL provided here yet)   | —     |

You'll also need:

- A laptop or desktop running **Chrome, Edge, or Opera** (Firefox and Safari
  don't yet support the browser API used by the installer)
- A home WiFi network with the password handy

---

## 2. Wire it up

You're connecting two boards together with 10 jumper wires. Each wire goes
from one labeled pin on the ESP32 to one labeled pin on the display.

| Display pin (silkscreen) | ESP32 pin    |
|--------------------------|--------------|
| `VCC` or `3.3V`          | `3V3`        |
| `GND`                    | `GND`        |
| `CS`                     | `GPIO10`     |
| `RST` or `RESET`         | `GPIO4`      |
| `DC` or `RS`             | `GPIO3`      |
| `MOSI` or `SDI` or `DIN` | `GPIO7`      |
| `SCK` or `CLK`           | `GPIO6`      |
| `LED` or `BL`            | `GPIO5`      |
| `MISO` or `SDO` or `DO`  | `GPIO2`      |
| `T_CS`                   | `GPIO8`      |
| `T_IRQ`                  | `GPIO9`      |

(`T_CLK`, `T_DIN`, `T_DO` on the touch side share wires with the display —
you don't need separate jumpers for them.)

**Sanity checks:**

- Wrong `VCC` is the most common mistake. Read the display silkscreen — if
  it says `5V` only, that board needs a different power source. Most
  modern boards accept 3.3V.
- Wires can be loose on cheap jumpers. Tug each one gently after seating.

---

## 3. Install the firmware

> **You don't need to install anything.** No PlatformIO, no Arduino IDE,
> no Python — just Chrome.

1. Plug the ESP32 into your computer with the USB-C cable.
2. Open **[the installer page][installer]** in Chrome, Edge, or Opera on a
   desktop. (Phone browsers don't have the right API.)
3. Click **Install Baby Stats Display**.
4. A pop-up will ask which serial port to use — pick the one that just
   appeared (usually labeled "USB JTAG/serial debug unit" or similar).
5. Confirm and wait. The installer downloads ~1 MB and flashes the board.
   This takes about 30 seconds.
6. When it says "Installation complete," unplug and reconnect, or hit the
   board's RESET button.

[installer]: https://capynaras-club.github.io/BloomOnDevice/install/

---

## 4. First-time setup

The display should power on within a couple of seconds. The first boot
needs WiFi credentials:

1. On your phone, open **Settings → WiFi**. You'll see a new network named
   **`BabyStatsSetup`**. Connect to it (no password).
2. Your phone should automatically open a setup page. If it doesn't, open
   any browser and go to `http://192.168.4.1`.
3. Pick your home WiFi from the list, type your password, and tap **Save**.
4. The display reboots and connects. After about 10 seconds it'll show the
   correct time, your city, and the current weather.

That's it. You're done.

---

## 5. Using it

The display has **five screens**, swipe between them with the `<` and `>`
arrows at the bottom:

```
[Forecast] <-> [Stats] <-> [Today] <-> [Yesterday] <-> [2 Days Ago]
```

It boots into **Stats**.

### Logging an event

On the Stats screen, **tap a card** to log:

- **Feed** card — record a feeding (current time is captured)
- **Diaper** card — record a diaper change
- **Sleep** card — record the baby going to sleep

The "since last" time on the card updates immediately. Today's count
appears below.

### Looking at history

- `>` arrow → **Today** screen, listing every event you logged today
- `>` again → **Yesterday**
- `>` again → **2 Days Ago**

If there are more than 5 events in a day, use the small ▲ ▼ in the centre
of the screen to page.

### Deleting a mistake

In any log screen, tap the **`x`** at the end of a row. The row turns red.
Tap the `x` a second time within 2 seconds to confirm. Tap anywhere else
to cancel.

### Weather

`<` from Stats opens the **Forecast** screen. ▲ shows the next 6 hours,
▼ shows the next 3 days. The forecast refreshes every 30 minutes.

---

## 6. Day-to-day tips

- The screen **dims** after 30 seconds of no touching, and goes **dark**
  after 40 seconds. Touch anywhere to wake it.
- Events older than 3 days **disappear** automatically — no manual cleanup.
- If you unplug it and plug it back in, **everything is remembered** —
  WiFi creds, recent events, and the clock re-syncs in seconds.
- If you move house or want to change WiFi: hold the BOOT button on the
  ESP32 while powering on. The `BabyStatsSetup` portal will reopen.

---

## 7. Something's wrong?

| What you see                       | What to try                                                    |
|------------------------------------|----------------------------------------------------------------|
| Black screen, board feels warm     | Power wires are swapped. Unplug. Recheck `VCC` ↔ `3V3`.        |
| White screen, no graphics          | `CS` or `DC` is on the wrong pin. Recheck the wiring table.    |
| Display works but touch doesn't    | Touch wires (`T_CS`, `T_IRQ`) on wrong pins.                   |
| Garbled colours, flicker           | A jumper is loose, or wires are too long. Reseat them.         |
| Stuck at "connecting wifi..."      | Wrong password. Hold BOOT during power-on to reopen setup.     |
| Wrong city / weather               | VPN or mobile hotspot fooling the geolocation. Reconnect to home WiFi. |
| Clock shows 1970                   | Time hasn't synced yet. Wait a minute, or check WiFi.          |
| Browser installer can't find port  | Cable is power-only (no data). Try a different USB-C cable.    |

Still stuck? [Open an issue](https://github.com/Capynaras-Club/BloomOnDevice/issues/new/choose)
with what you saw and a photo of your wiring.

---

## What now?

That's the whole device. If you want to peek under the hood —

- The [main README](README.md) covers building from source and the file layout.
- [CONTRIBUTING](CONTRIBUTING.md) is for sending fixes back.
- The code is [public domain (CC0)](LICENSE) — fork it, change it, sell it.
