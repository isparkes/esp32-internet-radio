# Internet Radio — Technical Specification

## Overview

ESP32-based internet radio that streams MP3 audio over WiFi to an I2S DAC. Supports Bluetooth A2DP sink mode as an alternative audio source. User interaction via a 128x64 OLED display with rotary encoder and buttons. Remote control via a web interface.

Software version: `INR-ESP32 0.0.1.0`

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
| phy_init | data | 0xE000 | 4 KB |
| factory | app | 0x10000 | 2944 KB |
| spiffs | data | 0x2F0000 | 1088 KB |

Single factory app partition (no OTA). OTA was removed to maximize app space.

### Memory Budget (typical)

- RAM usage: ~19.5% of 320 KB
- Flash usage: ~64.4% of ~3 MB app partition

## Audio Subsystem

### Radio Mode

1. `AudioFileSourceICYStream` opens an HTTP stream URL with ICY metadata support
2. `AudioFileSourceBuffer` provides a 16 KB ring buffer to absorb network jitter
3. `AudioGeneratorMP3` decodes the MP3 stream
4. `AudioOutputI2S` sends PCM samples to the I2S peripheral

The MP3 decode loop runs on a dedicated FreeRTOS task pinned to **core 0** at priority 3 with a 4096-byte stack. This isolates audio from display/WiFi processing on core 1.

When stopping radio playback, `i2s_driver_uninstall(I2S_NUM_0)` is called explicitly because the ESP8266Audio library's `AudioOutputI2S::stop()` does not release the I2S driver.

### Bluetooth Mode

Uses the ESP32-A2DP library to act as a Bluetooth A2DP sink. The device advertises as "InternetRadio" and accepts connections from phones/tablets.

WiFi and Bluetooth Classic share the 2.4 GHz radio on ESP32. When switching to Bluetooth mode, WiFi is disconnected (`WiFi.disconnect(true)`). When switching back to radio mode, WiFi reconnects automatically.

### Volume Control

- Integer range: 0–100 (stored in global `volume`)
- Radio mode: mapped to float gain 0.0–1.2 (`MAX_GAIN`) via `AudioOutputI2S::SetGain()`
- Bluetooth mode: mapped to 0–127 via A2DP volume control
- Adjustable via rotary encoder on status screen or web interface

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
      ├─ Audio
      │   ├─ Mode: Radio/Bluetooth (info)
      │   ├─ Station 1..N (play)
      │   ├─ Stop
      │   └─ Switch to BT / Switch to Radio
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
          ├─ Save Config
          ├─ WiFi at Start (ON/OFF toggle)
          ├─ Debug 10m (debug builds only)
          └─ Reset WiFi
```

The status screen shows: title, audio mode/status, WiFi IP, and volume. The encoder adjusts volume, confirm button toggles play/stop, encoder click enters the menu.

Menus are rebuilt dynamically when state changes (e.g., WiFi connects/disconnects, mode switches).

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
| `/api/getDiags` | GET | — | `{ uptime, heap, minfreeheap, cpufreq, sdkversion, sketchsize, flashsize, compiledate, sketchmd5, resetreason, partitions, features, ... }` |
| `/api/getConfig` | GET | — | `{ WifiOnAtStart }` |
| `/api/postConfig` | POST | JSON config fields | — |
| `/utils/restart` | GET | — | Reboots device |

#### Radio

| Endpoint | Method | Request | Response |
|----------|--------|---------|----------|
| `/api/stations` | GET | — | `[ { name, url }, ... ]` |
| `/api/stations` | POST | `{ name, url }` | — (saves to SPIFFS) |
| `/api/stations/delete` | POST | `{ index }` | — (saves to SPIFFS) |
| `/api/status` | GET | — | `{ playing, station, url, volume, mode }` |
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
  config.json      — WiFi credentials, WifiOnAtStart flag
  stats.json       — Uptime counters (uptimeMins, tubeOnTimeMins)
  stations.json    — Radio station list [{ name, url }, ...]
/web/
  index.html       — Landing page
  diags.html       — Diagnostics page
  portal.html      — Captive portal page
```

### Station Storage

- Up to `MAX_STATIONS` (9) stations
- Stored as a JSON array in `/config/stations.json`
- Default station seeded on first boot: "Radio FFH" (`http://mp3.ffh.de/radioffh/hqlivestream.mp3`)
- Managed via web interface or future menu additions

### Configuration (`config.json`)

- `WiFiSSID` / `WiFiPassword` — stored WiFi credentials
- `WifiOnAtStart` — boolean, auto-connect on boot

### Statistics (`stats.json`)

- `uptimeMins` — total lifetime uptime in minutes
- `tubeOnTimeMins` — total display-on time in minutes
- Saved periodically and on restart

## FreeRTOS Task Layout

| Task | Core | Priority | Stack | Purpose |
|------|------|----------|-------|---------|
| Main loop | 1 | 1 | default | WiFi, web server, menu, display |
| Audio decode | 0 | 3 | 4096 | MP3 stream decoding |

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
| `MAX_STATIONS` | integer (default 9) | Max stored stations |
| `MAX_GAIN` | float (default 1.2) | Audio gain ceiling |

## Known Constraints

- WiFi and Bluetooth Classic cannot operate simultaneously on ESP32; switching modes disconnects the other
- Web page polling (auto-refresh) causes audio breakup due to WiFi bandwidth contention; web pages use one-time data fetch only
- No OTA update support (removed to fit in flash); firmware updates require USB
- ArduinoJson v5 (not v6/v7) due to existing codebase patterns
