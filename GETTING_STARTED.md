# Getting started

A bedside display that tracks your baby's feeds, diapers, and sleeps with a
single tap. No phone, no app — just a screen by the crib.

This page assumes **zero prior experience** with electronics or programming.
If you can plug in a USB cable and connect to WiFi, you can build this.

> **For developers:** if you want to compile from source, contribute, or
> change behavior, head to the [main README](README.md) instead.

---

## 1. What you need

| Part                                                                | Cost  | Where to look                                  |
|---------------------------------------------------------------------|-------|------------------------------------------------|
| Sunton **ESP32-2432S024R** board (CYD 2.4" resistive)               | ~$10  | AliExpress / Amazon / Mercado Livre — "CYD 2.4 resistive" or the model number |
| USB-C cable (must be a **data** cable, not just power)              | ~$3   | Anything you have lying around                 |

Total: **~$15**. No wiring, no soldering — the ESP32, 2.4" display, touch
panel, microSD slot, speaker amp and USB-to-serial are all on a single PCB.

> **Important:** make sure the listing says **`R`** (resistive, with the
> XPT2046 touch chip), not the **`C`** (capacitive) variant. The two boards
> use different touch hardware and need different firmware.

Optional:

| Part                                              | Cost  |
|---------------------------------------------------|-------|
| 3D-printed enclosure (no STL provided here yet)   | —     |

You'll also need:

- A laptop or desktop running **Chrome, Edge, or Opera** (Firefox and Safari
  don't yet support the browser API used by the installer)
- A home WiFi network with the password handy

---

## 2. Wire it up

Nothing to wire — everything is on the same PCB. Just plug the USB-C cable
in. (If macOS or Windows can't see the board, install the CH340 driver from
[WCH][ch340] — the board's USB-to-serial chip needs it.)

[ch340]: https://www.wch-ic.com/downloads/CH341SER_ZIP.html

---

## 3. Install the firmware

> **You don't need to install anything.** No PlatformIO, no Arduino IDE,
> no Python — just Chrome.

1. Plug the ESP32 into your computer with the USB-C cable.
2. Open **[the installer page][installer]** in Chrome, Edge, or Opera on a
   desktop. (Phone browsers don't have the right API.)
3. Click **Install Bloom on Device**.
4. A pop-up will ask which serial port to use — pick the one that just
   appeared (usually labeled "USB-SERIAL CH340" or similar).
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
   **`BloomSetup`**. Connect to it (no password).
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
  ESP32 while powering on. The `BloomSetup` portal will reopen.

---

## 7. Something's wrong?

| What you see                       | What to try                                                    |
|------------------------------------|----------------------------------------------------------------|
| Black screen, board feels warm     | Bad USB cable or USB-C orientation. Try a different cable / port. |
| White screen, no graphics          | You may have the `C` (capacitive) variant — this firmware is for the `R` (resistive, XPT2046) board. |
| Display works but touch doesn't    | Same as above — confirm you have the **R** variant.            |
| Garbled colours, flicker           | Try a shorter / better-quality USB cable.                      |
| Stuck at "connecting wifi..."      | Wrong password. Hold BOOT during power-on to reopen setup.     |
| Wrong city / weather               | VPN or mobile hotspot fooling the geolocation. Reconnect to home WiFi. |
| Clock shows 1970                   | Time hasn't synced yet. Wait a minute, or check WiFi.          |
| Browser installer can't find port  | Install the CH340 driver (link above), or try a different USB-C **data** cable. |
| Installer can't connect to board   | Hold the **BOOT** button, tap **RST**, release **BOOT**, retry. |

Still stuck? [Open an issue](https://github.com/Capynaras-Club/BloomOnDevice/issues/new/choose)
with what you saw and a photo of your wiring.

---

## What now?

That's the whole device. If you want to peek under the hood —

- The [main README](README.md) covers building from source and the file layout.
- [CONTRIBUTING](CONTRIBUTING.md) is for sending fixes back.
- The code is [public domain (CC0)](LICENSE) — fork it, change it, sell it.
