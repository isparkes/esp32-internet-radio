# Internet Radio

An ESP32-based internet radio with optional Bluetooth audio, OLED menu system, and web interface.

## Hardware

- **ESP32-WROVER** (4MB flash + 4MB PSRAM) — classic ESP32 required for Bluetooth; PSRAM required for audio buffering
- **ESP32-WROOM** (4MB flash) — practically any ESP32 required for no BT version
- **PCM5102 I2S DAC** — stereo audio output
- **SH1106 1.3" OLED** — 128x64 I2C display with integrated rotary encoder
- **Rotary encoder** — volume control and menu navigation
- **Confirm/Back buttons** — menu interaction

### Pin Assignment

| Function | GPIO |
|----------|------|
| I2S BCLK | 33 |
| I2S LRC | 23 |
| I2S DOUT | 25 |
| I2C SDA | 21 |
| I2C SCL | 22 |
| Encoder CLK | 5 |
| Encoder DT | 14 |
| Encoder SW | 27 |
| Confirm Button | 13 |
| Back Button | 4 |
| Status LED | 2 |

## Features

- **Internet Radio** — MP3 stream decoding with ICY metadata support, up to 9 configurable stations
- **Bluetooth A2DP** *(optional, see below)* — sink mode (stream from phone) or source mode (stream radio to BT speaker)
- **OLED Menu System** — hierarchical menu with rotary encoder navigation
- **Web Interface** — station management, playback control, volume, and diagnostics
- **WiFi Management** — WPS, SmartConfig, captive portal, and manual credential entry
- **Persistent Storage** — stations, configuration, and statistics saved to SPIFFS

## Building

Requires [PlatformIO](https://platformio.org/).

```bash
# Build firmware
pio run

# Upload firmware
pio run --target upload

# Upload SPIFFS filesystem (web pages, default config)
pio run --target uploadfs

# Serial monitor
pio device monitor
```

## Web Interface

Once connected to WiFi, access the device at `http://<device-ip>/` or `http://<hostname>.local/`.

- **Home** (`/`) — landing page with links to controls, diagnostics, and restart
- **Radio Control** (`/radio`) — play/stop stations, manage station list, volume slider
- **Diagnostics** (`/web/diags.html`) — system info, memory, firmware, partition table

### REST API

| Endpoint | Method | Description |
|----------|--------|-------------|
| `/api/getSummary` | GET | IP, SSID, version |
| `/api/getDiags` | GET | System diagnostics |
| `/api/stations` | GET | List stations |
| `/api/stations` | POST | Add station |
| `/api/stations/delete` | POST | Delete station |
| `/api/status` | GET | Playback status |
| `/api/play` | POST | Play station by index |
| `/api/stop` | POST | Stop playback |
| `/api/volume` | POST | Set volume (0-100) |

## Configuration

Edit `include/Configuration.h` to change:

- `OLED_SH1106` / `OLED_SSD1306` — display type
- `MAX_STATIONS` — maximum station count (default 9)
- `MAX_GAIN` — maximum audio gain (default 1.2)
- `DEBUG` — enable serial debug output

### Enabling Bluetooth

Bluetooth support is off by default. To enable it, uncomment `-DFEATURE_BLUETOOTH` in `platformio.ini`:

```ini
build_flags =
    ...
    -DFEATURE_BLUETOOTH  ; Classic BT A2DP (ESP32 only, uses ~100KB DRAM)
```

**Requirements:**
- **Classic ESP32 only** — the ESP32-S3, ESP32-C3, and other variants do not support Classic Bluetooth (A2DP). This will not compile on those targets.
- **PSRAM required** — the audio streaming buffer (256KB) and Bluetooth PCM ring buffer are allocated from PSRAM. An ESP32-WROVER or equivalent module with at least 4MB PSRAM is needed.
- **`-DBOARD_HAS_PSRAM` and `-mfix-esp32-psram-cache-issue`** must remain in `build_flags` (already set for the `esp-wrover-kit` board).

**Audio modes available with Bluetooth enabled:**
- *Radio → I2S DAC* — default mode, works without Bluetooth
- *BT Sink* — device appears as a Bluetooth speaker; audio from a phone plays through the DAC
- *Radio → BT Speaker* — streams an internet radio station to a Bluetooth speaker (A2DP source)

## Dependencies

- [ESP8266Audio](https://github.com/earlephilhower/ESP8266Audio) — MP3 decoding
- [ESP32-A2DP](https://github.com/pschatzmann/ESP32-A2DP) — Bluetooth A2DP sink/source *(only used when `FEATURE_BLUETOOTH` is enabled)*
- [ESPAsyncWebServer](https://github.com/esp32async/ESPAsyncWebServer) — async HTTP server
- [ArduinoJson 5](https://github.com/bblanchon/ArduinoJson) — JSON serialization
- [Adafruit SH110X](https://github.com/adafruit/Adafruit_SH110X) — OLED driver
- [Adafruit GFX](https://github.com/adafruit/Adafruit-GFX-Library) — graphics primitives
