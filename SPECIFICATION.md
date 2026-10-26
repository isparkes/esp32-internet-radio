# Internet Radio — Technical Specification

## Overview

ESP32-based internet radio that streams MP3 audio over WiFi to an I2S DAC. User interaction via a 128x64 OLED display with rotary encoder and buttons. Remote control via a web interface.

Software version: `INR-ESP32 1.0.0.0`

## System Architecture

### Processor and Memory

- **MCU**: ESP32 (dual-core Xtensa LX6, 240 MHz)
- **Flash**: 4MB QSPI
- **RAM**: 520KB SRAM
- **Framework**: Arduino (ESP-IDF 4.4.x underlying)
- **Build system**: PlatformIO (espressif32 platform)

### Flash Partition Layout

| Partition | Type | Offset | Size |
|-----------|------|--------|------|
| nvs | data | 0x9000 | 20 KB |
| otadata | data | 0xE000 | 8 KB |
| app0 | app (ota_0) | 0x10000 | 1536 KB |
| app1 | app (ota_1) | 0x190000 | 1536 KB |
| spiffs | data | 0x310000 | 896 KB |
| coredump | data | 0x3F0000 | 64 KB |

Two app partitions so firmware can be updated over the air; the running image is
limited to 1536 KB as a result.

### Memory Budget (typical)

- RAM usage: ~16.5% of 320 KB
- Flash usage: ~83% of 1.5 MB app partition

## Audio Subsystem

### Radio Mode

1. `AudioFileSourceICYStream` opens an HTTP stream URL with ICY metadata support
2. `AudioFileSourceBuffer` provides a ring buffer to absorb network jitter — 256 KB from PSRAM where present, otherwise up to 32 KB from SRAM (`PSRAM_BUFFER_SIZE` / `SRAM_BUFFER_MAX`)
3. `AudioGeneratorMP3` decodes the MP3 stream
4. `AudioOutputI2SBalance` (an `AudioOutputI2S` subclass) applies the stereo balance and sends PCM samples to the I2S peripheral

The MP3 decode loop runs on a dedicated FreeRTOS task pinned to **core 0** at priority 3 with a 4096-byte stack. This isolates audio from display/WiFi processing on core 1.

### Volume Control

- Integer range: 0–100 (stored in global `volume`)
- Mapped to float gain 0.0–1.2 (`MAX_GAIN`) via `AudioOutputI2S::SetGain()`
- Adjustable via rotary encoder on status screen or web interface
- Not saved as it changes; System → Save Config snapshots it to `config.json`, and it is restored on the next boot

### Balance Control

- Integer range: -100 (full left) to +100 (full right), 0 = centre, stored as `AudioBalance`
- Applied in `AudioOutputI2SBalance::ConsumeSample()` (`include/AudioOutputI2SBalance.h`) as a Q8 fixed-point scale per channel — two multiplies and two shifts per frame
- Attenuates the opposite channel only; it never boosts, so it cannot add clipping on top of the gain stage
- Mono streams are duplicated to stereo in the subclass, which reports stereo to the base class so that `MakeSampleStereo16()` cannot overwrite the balanced right channel
- A new output object is created per stream, so the balance is re-applied in `StartPlaying()` alongside the gain
- Adjustable via the Audio menu; persisted in `config.json`

### Play at Start

With `PlayOnStart` set, the saved station starts playing by itself after a boot.
WiFi association takes a few seconds, so `setup()` only registers the request
(`requestAutoPlay()`) and `audioOncePerLoop()` starts playback once WiFi is up,
giving up after 60 seconds. Any stop — menu, web or status screen — cancels a
request that has not fired yet.

### I2S Configuration

| Signal | GPIO |
|--------|------|
| BCLK | 33 |
| LRC (WSEL) | 23 |
| DOUT | 25 |

Connected to a PCM5102 DAC module.

## Display and Menu System

### Hardware

- SH1106 128x64 monochrome OLED, I2C (SDA=21, SCL=22)
- Adafruit SH110X + GFX libraries

### Input

| Control | GPIO | Function |
|---------|------|----------|
| Encoder CLK | 5 | Scroll/adjust values |
| Encoder DT | 14 | Direction sense |
| Encoder SW | 27 | Click to select |
| Confirm | 13 | Play/confirm action |
| Back | 4 | Navigate back |

### Menu Hierarchy

```
Status Screen (default)
  └─ [Encoder click] → Main Menu
      ├─ Stations
      │   └─ Station 1..N (play)
      ├─ Audio
      │   ├─ Play
      │   ├─ Stop
      │   ├─ Balance (0=mid) (stereo balance, -100..+100)
      │   └─ Play at Start (ON/OFF toggle)
      ├─ WiFi
      │   ├─ Disconnect WiFi (when connected)
      │   ├─ Reconnect Prev (when credentials stored)
      │   ├─ Connect WPS
      │   ├─ SmartConfig
      │   ├─ Access Point
      │   ├─ Scan WiFi
      │   ├─ Enter SSID (string editor)
      │   └─ Enter Password (string editor)
      └─ System
          ├─ Restart Device
          ├─ Save Config (also snapshots current volume and station)
          ├─ WiFi at Start (ON/OFF toggle)
          ├─ Blank (0=off) (OLED blanking timeout, minutes)
          ├─ Debug 10m (debug builds only)
          └─ Reset WiFi
```

The status screen shows: title bar, play/stop/resync icon, playback state, volume bar, buffer fill bar, and a scrolling song title. The encoder adjusts volume, confirm button toggles play/stop, encoder click enters the menu, back button shows a 2-second info overlay.

The back button cycles through three info pages, each shown for 2 seconds after the last press. Pressing back while a page is up moves to the next page; once the overlay has timed out, the next press starts again at System Info:

1. **System Info** - IP, free heap, streams played, frames decoded, total play time
2. **WiFi Info** - SSID, IP, gateway, signal strength (dBm and %), MAC address. When not connected: the configured SSID, whether credentials are saved, the WiFi-at-start setting and the MAC address
3. **Station Info** - station name, position in the station list, stream URL (over two lines), playback state and buffer fill

Menus are rebuilt dynamically when state changes (e.g., WiFi connects/disconnects).

### Display Blanking

The panel is blanked after `OledBlankMins` minutes without encoder or button
activity (0 disables blanking). While blanked nothing is rendered and no I2C
traffic is generated, so the display cannot couple interference into the audio.
Any encoder movement or button press wakes the panel; that first input is
swallowed so it only lights the screen rather than also acting on whatever was
under it. Audio playback is unaffected by blanking.

### Status LED

The onboard LED (GPIO 2) shows what the radio is doing:

| State | Pattern |
|-------|---------|
| Booting | Rapid flash, ~8/s — the first sign of life, started before anything else in `setup()` |
| Radio stopped | Dark |
| Radio playing | Slow flash, 100 ms every 2 s |
| Resyncing after a dropped stream | Medium flash, 2/s |

The pattern is produced by a low-priority FreeRTOS task (`TimerManager.cpp`) rather
than the main loop, because `setup()` blocks in several places — SPIFFS, WiFi and
the startup jingle — and the boot flash has to keep running through all of them.
The main loop sets the pattern from the playback state on every pass and also
services it, so the LED still works if the task could not be created.

`LED_ON_LEVEL` in `Defs.h` sets which logic level lights the LED; boards that wire
the LED between 3V3 and the pin need it set to `LOW`.

## WiFi Management

### Connection Methods

1. **Auto-reconnect** — connects to last stored AP on boot (if `WifiOnAtStart` enabled)
2. **WPS** — push-button WPS pairing
3. **SmartConfig** — ESP-Touch from Espressif app
4. **Captive Portal** — opens AP mode with DNS spoofing, serves credential entry page
5. **Manual** — SSID/password entry via OLED menu string editor

### mDNS

Device registers as `<hostname>.local` for local network discovery.

## Web Interface

### Pages

| URL | Source | Description |
|-----|--------|-------------|
| `/` | SPIFFS `/web/index.html` | Landing page with navigation |
| `/radio` | PROGMEM | Radio control panel (station list, playback, volume) |
| `/web/diags.html` | SPIFFS | System diagnostics display |
| `/web/portal.html` | SPIFFS | Captive portal WiFi setup |

All pages use inline CSS/JS with no external dependencies. Dark theme, mobile-responsive, max-width 480px.

### REST API

#### System

| Endpoint | Method | Request | Response |
|----------|--------|---------|----------|
| `/api/getSummary` | GET | — | `{ ip, mac, ssid, clockurl, version }` |
| `/api/getDiags` | GET | — | `{ uptime, playtime, heap, minfreeheap, cpufreq, sdkversion, sketchsize, flashsize, compiledate, sketchmd5, resetreason, partitions, features, ... }` |
| `/api/getConfig` | GET | — | `{ WifiOnAtStart }` |
| `/api/postConfig` | POST | JSON config fields | — |
| `/utils/restart` | GET | — | Reboots device |

#### Radio

| Endpoint | Method | Request | Response |
|----------|--------|---------|----------|
| `/api/stations` | GET | — | `[ { name, url }, ... ]` |
| `/api/stations` | POST | `{ name, url }` | — (saves to SPIFFS) |
| `/api/stations/delete` | POST | `{ index }` | — (saves to SPIFFS) |
| `/api/status` | GET | — | `{ playing, station, url, volume }` |
| `/api/play` | POST | `{ index }` | — |
| `/api/stop` | POST | — | — |
| `/api/volume` | POST | `{ volume: 0-100 }` | — |

#### WiFi

| Endpoint | Method | Request | Response |
|----------|--------|---------|----------|
| `/api/postWiFiCredentials` | POST | `{ ssid, password }` | — |
| `/api/credentials` | GET | — | `{ ssid, password }` |
| `/api/getWiFiNetworks` | GET | — | Scanned network list (AP mode) |

#### Utilities

| Endpoint | Method | Description |
|----------|--------|-------------|
| `/utils/resetwifi` | GET | Clear stored WiFi credentials |
| `/utils/scanI2C` | GET | Scan I2C bus, return found addresses |
| `/utils/scanSPIFFS` | GET | List all files in SPIFFS |
| `/utils/saveStats` | GET | Persist statistics to SPIFFS |
| `/utils/resetoptions` | GET | Reset config to defaults |
| `/utils/resetall` | GET | Factory reset all data |

## Persistent Storage (SPIFFS)

### File Layout

```
/config/
  config.json      — WiFi credentials, WifiOnAtStart flag, OledBlankMins, AudioBalance,
                     Volume, SelectedStation, PlayOnStart
  stats.json       — Usage counters (uptimeMins, playtimeMins)
  stations.json    — Radio station list [{ name, url }, ...]
/web/
  index.html       — Landing page
  diags.html       — Diagnostics page
  portal.html      — Captive portal page
```

### Station Storage

- Stored as a JSON array in `/config/stations.json`; no fixed limit on count
- Read directly from SPIFFS on demand — no in-memory array; each call to `getStationCount()`, `getStation()`, `appendStation()`, or `deleteStation()` opens, operates on, and closes the file
- Default station seeded automatically on first access if the file is absent: "Radio FFH" (`http://mp3.ffh.de/radioffh/hqlivestream.mp3`)
- The OLED Stations menu shows up to 9 stations (one callback slot per entry); the JSON file may contain more
- Managed via web interface (`/radio` page)

### Configuration (`config.json`)

- `WiFiSSID` / `WiFiPassword` — stored WiFi credentials
- `WifiOnAtStart` — boolean, auto-connect on boot
- `OledBlankMins` — OLED blanking timeout in minutes (0 = never blank, max 60, default 0)
- `AudioBalance` — stereo balance, -100 (full left) to +100 (full right), default 0 (centre)
- `Volume` — playback volume 0-100 as of the last Save Config, restored at startup (default 10)
- `SelectedStation` — index into `stations.json` as of the last Save Config, restored at startup; falls back to the first station if that index no longer exists (default 0)
- `PlayOnStart` — boolean, start playing the saved station once WiFi connects after a boot (default false)

### Statistics (`stats.json`)

- `uptimeMins` — total lifetime uptime in minutes
- `playtimeMins` — total lifetime play time in minutes, accumulated a second at a time while the stream is playing
- Saved once an hour, at midnight, and on demand via `/utils/saveStats`

## FreeRTOS Task Layout

| Task | Core | Priority | Stack | Purpose |
|------|------|----------|-------|---------|
| Main loop | 1 | 1 | default | WiFi, web server, menu, display |
| Audio decode | 1 | 3 | 4096 | MP3 stream decoding |
| Status LED | any | 1 | 2048 | Flashing the onboard LED |

The audio task yields with `vTaskDelay(1)` each iteration so the lower-priority
main loop still runs on core 1. If its stack cannot be allocated the decoder
falls back to running inline from `audioOncePerLoop()`.

### Watchdog

- The IDF task watchdog is already running at boot (5 s, panic on timeout) with the core 0 idle task subscribed; core 1 idle is not subscribed in this build, so `disableCore1WDT()` in `setup()` is defensive only
- `enableWatchdog()` at the end of `setup()` widens the timeout to `WDT_TIMEOUT` (15 s) and subscribes the main loop task; `feedWatchdog()` resets it once per second
- 15 s is chosen to clear the longest blocking call made from the loop task — opening a stream sits in the HTTP connect for up to ~5 s, longer over https
- An OTA upload blocks inside `ArduinoOTA.handle()` for the whole transfer, so the OTA `onStart` callback unsubscribes the loop task and `onEnd` / `onError` resubscribe it

## Build Configuration

### Key Build Flags

- `-Os` — optimize for size
- `-DCORE_DEBUG_LEVEL=0` — disable ESP-IDF debug output

### Compile-Time Options (`Configuration.h`)

| Define | Values | Description |
|--------|--------|-------------|
| `OLED_SH1106` | `OLED_SH1106` / `OLED_SSD1306` | OLED controller type |
| `DEBUG` | `DEBUG` / `DEBUG_OFF` | Enable serial debug logging |
| `FEATURE_MENU` | defined or not | Enable OLED menu system |
| `MAX_GAIN` | float (default 1.2) | Audio gain ceiling |
| `DEFAULT_OLED_BLANK_MINS` | int (default 0) | Factory default OLED blanking timeout, 0 = never |
| `MAX_OLED_BLANK_MINS` | int (default 60) | Longest blanking timeout selectable in the menu |
| `LED_ON_LEVEL` | `HIGH` / `LOW` (`Defs.h`) | Logic level that lights the onboard LED |
| `DEFAULT_AUDIO_BALANCE` | int (default 0) | Factory default stereo balance, 0 = centre |
| `MAX_AUDIO_BALANCE` | int (default 100) | Balance range limit, applied either side of centre |
| `AUDIO_BALANCE_STEP` | int (default 5) | Balance adjustment step in the menu |
| `DEFAULT_VOLUME` | int (default 10) | Volume used before anything has been saved |
| `DEFAULT_STATION` | int (default 0) | Station index used before anything has been saved |
| `DEFAULT_PLAY_ON_START` | bool (default false) | Whether a fresh config starts playing at boot |

## Known Constraints

- Web page polling (auto-refresh) causes audio breakup due to WiFi bandwidth contention; web pages use one-time data fetch only
- The dual OTA partition layout caps the app image at 1536 KB, which is most of the reason flash sits around 83% full
- ArduinoJson v5 (not v6/v7) due to existing codebase patterns
