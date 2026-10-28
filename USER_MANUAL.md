# Internet Radio — User Manual

**Firmware version:** INR-ESP32 1.0.1.0

---

## Contents

1. [Overview](#1-overview)
2. [First-time setup](#2-first-time-setup)
3. [Physical controls](#3-physical-controls)
4. [The status screen](#4-the-status-screen)
5. [Info pages](#5-info-pages)
6. [The menu](#6-the-menu)
7. [Playing stations](#7-playing-stations)
8. [Volume and balance](#8-volume-and-balance)
9. [Web interface](#9-web-interface)
10. [Managing stations](#10-managing-stations)
11. [Saving settings](#11-saving-settings)
12. [Display blanking](#12-display-blanking)
13. [Status LED](#13-status-led)
14. [Firmware updates](#14-firmware-updates)
15. [Factory reset](#15-factory-reset)
16. [Troubleshooting](#16-troubleshooting)

---

## 1. Overview

The radio streams internet radio stations over WiFi and plays them through a
PCM5102 I2S DAC. A 128x64 OLED display with a rotary encoder and two buttons
gives local control; a web interface served by the radio itself does the same
job from a phone or computer, and is the easier place to manage the station
list.

Everything — stations, WiFi credentials, volume, balance and the rest — is held
on the radio itself. No account, app or cloud service is involved.

---

## 2. First-time setup

### 2.1 Powering on

Connect power. The status LED flashes rapidly while the radio boots, a splash
screen appears on the display, and a short startup jingle plays through the
speakers — if you hear the jingle, the audio path is working end to end.

The display then settles on the status screen (§4).

### 2.2 Connecting to WiFi

Out of the box the radio has no WiFi credentials, so it stays offline until you
give it some. There are four ways to do that, all under **WiFi** in the menu
(§6). Pick whichever suits your router.

**Captive portal (works with any router)**

1. Menu → **WiFi** → **Access Point**.
2. The display flashes the portal address, normally `192.168.4.1`.
3. On your phone or computer, join the open WiFi network named **ESP32-xxxxxx**
   (the last six characters are taken from the radio's MAC address).
4. A setup page should open by itself. If it does not, browse to
   `http://192.168.4.1`.
5. Choose your network from the list, enter the password and save.

**WPS**

1. Menu → **WiFi** → **Connect WPS**.
2. The display asks you to press the WPS button on your router; do so within
   two minutes.

**SmartConfig**

1. Menu → **WiFi** → **SmartConfig**.
2. Use the Espressif ESP-Touch app on a phone that is already on the target
   network, and send the credentials from there.

**Typing them in**

Menu → **WiFi** → **Enter SSID** and **Enter Password** use the on-screen
string editor (§6.3). Slow, but it needs nothing else to hand. **Scan WiFi**
saves half the typing: it lists the networks in range under *-- Scanned
Networks --*, and picking one fills the SSID in for you, leaving only the
password to type. When both are entered, choose **Reconnect Prev** to connect.

Once connected, the radio registers itself on the network as
`ESP32-xxxxxx.local`, and remembers the credentials for next time.

### 2.3 Check it worked

Press the **Back** button on the status screen: the first info page shows the
radio's IP address (§5). If it shows one, you are online.

---

## 3. Physical controls

| Control | On the status screen | In the menu |
|---------|----------------------|-------------|
| **Rotary encoder** (turn) | Volume up/down | Move through the list, change the value being edited |
| **Encoder button** (click) | Open the menu | Select the highlighted item |
| **Confirm button** | Start/stop playing | Select item, save the value being edited |
| **Back button** | Show the info pages (§5) | Go up one level; at the top level, return to the status screen |

The menu returns to the status screen by itself after 10 seconds without input,
so the radio never sits in a half-finished menu.

---

## 4. The status screen

This is the default display. From top to bottom:

- **INet Radio** title bar
- A **play, stop or resync icon**, with the matching word beside it:
  - *Playing* — audio is streaming
  - *Stopped* — nothing is playing
  - *Resyncing* — the stream dropped and the radio is reconnecting by itself
- **Volume** bar — the current volume, 0 to 100
- **Buffer** bar — how full the stream buffer is. A healthy stream sits near
  full; a bar that keeps dipping towards empty means the network is struggling
  (§16)
- **Song title** — scrolls along the bottom when the station sends track
  information. When nothing is playing it reads `[Press Enc for Menu]`

---

## 5. Info pages

Pressing **Back** on the status screen brings up a set of detail pages over the
top of it. Each press moves on to the next page, and the overlay disappears
three seconds after the last press. Once it has gone, the next press starts
again at System Info.

**Holding the Back button down keeps the page up.** The three-second countdown
is suspended for as long as the button is held — and the display will not blank
either — so you can read a page at your own pace, and it only starts counting
down once you let go. Note that the press that starts the hold still counts as
a press, so holding the button parks you on the page *after* the one that was
showing.

The four pages are:

| Page | Shows |
|------|-------|
| **System Info** | IP address, free heap, streams played, frames decoded, total play time |
| **WiFi Info** | SSID, IP, gateway, signal strength in dBm and percent, MAC address. When offline it shows the configured SSID instead, whether credentials are stored, and the WiFi-at-start setting |
| **Station Info** | Station name, its position in the list, the stream URL over two lines, playback state and buffer fill |
| **Build Info** | Firmware version, build date and time, sketch size against the available partition, flash size and CPU clock, ESP-IDF version |

---

## 6. The menu

Click the **encoder button** on the status screen to open the menu. Turn the
encoder to move, click the encoder or press **Confirm** to select, press
**Back** to go up a level.

```
Main Menu
  ├─ Stations        — the station list; selecting one plays it
  ├─ Audio
  │   ├─ Play            — start the selected station
  │   ├─ Stop            — stop playing
  │   ├─ Balance (0=mid) — stereo balance, -100 (left) to +100 (right)
  │   └─ Play at Start   — start playing by itself after a boot
  ├─ WiFi
  │   ├─ Disconnect WiFi — shown when connected
  │   ├─ Reconnect Prev  — shown when credentials are stored
  │   ├─ Connect WPS     — WPS push-button pairing
  │   ├─ SmartConfig     — ESP-Touch pairing
  │   ├─ Access Point    — start the captive portal
  │   ├─ Scan WiFi       — list the networks in range, pick one to set the SSID
  │   ├─ Enter SSID      — type a network name
  │   └─ Enter Password  — type a password
  └─ System
      ├─ Restart Device  — reboot
      ├─ Save Config     — write settings to flash (§11)
      ├─ WiFi at Start   — connect to the stored network on boot
      ├─ Blank (0=off)   — display blanking timeout in minutes (§12)
      └─ Reset WiFi      — forget the stored credentials
```

The WiFi entries change with the situation — *Disconnect WiFi* only appears
while connected, for instance — so the menu will not always look identical.
Debug builds carry one extra item, **Debug 10m**, which turns on serial logging
for ten minutes.

### 6.1 Toggle settings

*Play at Start* and *WiFi at Start* are ON/OFF settings. Turn the encoder to
pick the option, press **Confirm** to keep it, **Back** to leave it alone.

### 6.2 Number settings

*Balance* and *Blank* are numbers. Turn the encoder to change the value,
**Confirm** to save it, **Back** to cancel.

### 6.3 Text settings

*Enter SSID* and *Enter Password* use a character picker: turn the encoder to
choose a character, click the **encoder button** to add it, press **Back** to
delete the character before the cursor, and **Confirm** to finish. Pressing
**Back** on an empty entry cancels.

---

## 7. Playing stations

- **From the menu** — Menu → **Stations**, then pick one. It starts straight
  away and becomes the selected station.
- **From the status screen** — press **Confirm** to start or stop the station
  that is already selected.
- **From the web interface** — §9.

The display shows up to nine stations; if the list is longer, the rest are
reachable from the web interface.

If a stream drops out, the radio shows *Resyncing* and reconnects by itself —
no action needed.

With **Play at Start** switched on, the radio starts the saved station on its
own once WiFi comes up after a boot. It gives up if WiFi has not connected
within 60 seconds, and any stop — from the menu, the web page or the Confirm
button — cancels a start that has not happened yet.

---

## 8. Volume and balance

**Volume** is set by turning the encoder on the status screen, or from the web
interface. The range is 0 to 100, two points per click of the encoder.

**Balance** shifts the sound left or right: Menu → **Audio** → **Balance**,
from -100 (full left) through 0 (centre) to +100 (full right), in steps of 5.
It only ever turns the opposite channel down, never up, so it cannot introduce
distortion. Balance is saved as soon as you confirm it.

Volume is *not* saved as you change it — see §11.

---

## 9. Web interface

With the radio on your network, browse to `http://ESP32-xxxxxx.local/` or to
the IP address from the System Info page (§5).

| Page | Address | What it does |
|------|---------|--------------|
| **Home** | `/` | Links to everything else |
| **Radio Control** | `/radio` | Play and stop stations, add and delete them, volume slider |
| **Diagnostics** | `/web/diags.html` | Uptime, total play time, memory, firmware details, partition table |
| **Firmware Update** | `/update` | Upload new firmware (§14) |

The pages fetch their data once when they load rather than polling, because
constant web traffic competes with the audio stream and makes it break up. To
see fresh numbers, reload the page.

---

## 10. Managing stations

Stations live in a list on the radio, each one a name and a stream URL. The
`/radio` page is the place to edit it:

- **Add** — type a name and the stream URL, then add. MP3 streams are
  supported; the usual `http://…/stream.mp3` style of address from a station's
  website is what you want.
- **Delete** — remove a station from the list.
- **Play** — start any station in the list.

Changes are written to flash immediately. A radio that has never had a station
added starts with one: *Radio FFH*.

---

## 11. Saving settings

Some settings are written to flash the moment you change them, others are only
written when you ask:

| Setting | Saved |
|---------|-------|
| Stations | Immediately |
| WiFi credentials | As soon as a connection to them succeeds |
| Balance, Blank, Play at Start | On confirming the setting |
| **Volume** | Only by **System → Save Config** |
| **Selected station** | Only by **System → Save Config** |
| **WiFi at Start** | Only by **System → Save Config** |

So the way to make the radio come up at your preferred volume on your preferred
station is: set them, then Menu → **System** → **Save Config**. The same applies
to *WiFi at Start*: it takes effect at once, but a reboot without a Save Config
brings back the old setting.

Usage statistics — total uptime and total play time — are written once an hour
and at midnight.

---

## 12. Display blanking

Menu → **System** → **Blank (0=off)** sets how many minutes of no input the
radio waits before switching the display off; 0 leaves it on permanently and 60
is the longest setting.

While blanked, the radio stops talking to the display altogether. This is
deliberate: the display's I2C traffic can otherwise be heard faintly in the
audio. Any turn of the encoder or press of a button lights it again, and that
first input only wakes the screen — it will not also press whatever happened to
be under it. Playback carries on regardless.

---

## 13. Status LED

The small LED on the board reports what the radio is doing:

| Pattern | Meaning |
|---------|---------|
| Rapid flash, about 8 per second | Booting |
| Dark | Stopped |
| A brief flash every 2 seconds | Playing |
| Steady flash, twice a second | Resyncing after a dropped stream |

---

## 14. Firmware updates

Browse to `/update` and upload a new firmware image (`firmware.bin`). The radio
restarts into the new firmware when the upload finishes. There are two
partitions, so an interrupted upload leaves the previous firmware intact.

The web pages and default configuration live in a separate filesystem image
(`spiffs.bin`), uploaded the same way from `/updatefs`.

Do not cut the power during an upload.

---

## 15. Factory reset

| What | How |
|------|-----|
| Forget WiFi credentials | Menu → **System** → **Reset WiFi**, or browse to `/utils/resetwifi` |
| Reset settings to defaults | Browse to `/utils/resetoptions` |
| Erase everything, including stations | Browse to `/utils/resetall` |

After a full reset the radio comes back with no WiFi credentials and the
default station, exactly as it left the bench — start again from §2.

---

## 16. Troubleshooting

**Nothing on the display**
Check that the blanking timeout has not switched it off (§12) — turn the
encoder and see whether it lights. Otherwise check the I2C wiring; the radio
can scan the bus for you at `/utils/scanI2C`.

**No sound, but the display says *Playing***
Check the volume (§8) and the balance — a balance of ±100 silences one channel
entirely. If the startup jingle plays at boot but streams do not, the problem
is the stream rather than the audio hardware.

**Audio breaks up, buffer bar keeps dipping**
The stream is arriving more slowly than it is being played. Try a lower-bitrate
stream or a different station, move the radio closer to the access point, and
check the signal strength on the WiFi info page (§5). Reloading web pages
repeatedly while listening will also do it.

**Stuck on *Resyncing***
The station's server is unreachable or has dropped the connection. The radio
keeps trying; check the stream URL on the Station Info page (§5) and try
another station to tell a dead stream apart from a dead network.

**Cannot find the radio on the network**
Read the IP address from the System Info page (§5) and use it directly —
`.local` addressing does not work on every network. If there is no IP address,
the radio is not connected: redo §2.2.

**The captive portal page does not open**
Browse to `http://192.168.4.1` by hand. Some phones need mobile data turned off
before they will talk to a network that has no internet connection.

**An info page disappears before you can read it**
Hold the Back button down instead of pressing it (§5).
