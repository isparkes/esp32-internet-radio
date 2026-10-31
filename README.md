# Internet Radio

An ESP32-based internet radio with an OLED menu system and web interface. The radio is limited to 2.4GHz WiFi, determined by the ability of the ESP32 WiFi subsystem. Later versions of the ESP32 might relax this restriction.

## Documentation

- [USER_MANUAL.md](USER_MANUAL.md) — operating the radio: controls, menu, info pages, web interface, troubleshooting
- [SPECIFICATION.md](SPECIFICATION.md) — technical specification: architecture, pin assignment, API, storage layout

## Hardware

- **ESP32-WROVER** (4MB flash + 4MB PSRAM) — PSRAM used for the audio streaming buffer
- **ESP32-WROOM** (4MB flash) — works without PSRAM too
- **PCM5102 I2S DAC** — stereo audio output
- **SH1106 1.3" OLED** — 128x64 I2C display with integrated rotary encoder
- **Rotary encoder** — volume control and menu navigation
- **Confirm/Back buttons** — menu interaction

### Pin Assignment

| Function | GPIO |
|----------|------|
| I2S BCLK/BCK | 33 |
| I2S LRC/LCK | 23 |
| I2S DOUT/DIN | 25 |
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
- **Stereo Balance** — software L/R balance applied to the decoded samples, set from the Audio menu
- **OLED Menu System** — hierarchical menu with rotary encoder navigation
- **Info Pages** — back button cycles system, WiFi, station and build info over the status screen; hold it to keep a page up
- **Display Blanking** — optional idle timeout that blanks the OLED; any encoder or button input wakes it
- **Status LED** — rapid flash while booting, dark when stopped, slow flash while playing, medium flash while resyncing
- **Play at Start** — optionally resume the saved station by itself once WiFi connects after a boot
- **Web Interface** — station management, playback control, volume, and diagnostics
- **WiFi Management** — WPS, SmartConfig, captive portal, and manual credential entry
- **Persistent Storage** — stations, configuration, and statistics saved to SPIFFS; System → Save Config also stores the current volume and station for the next boot
- **OTA Updates** — firmware and SPIFFS images uploadable over WiFi, plus ArduinoOTA

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
- **Diagnostics** (`/web/diags.html`) — system info, uptime and total play time, memory, firmware, partition table
- **Firmware Update** (`/update`) — upload a new firmware image; `/updatefs` does the same for the SPIFFS image

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
- `MAX_GAIN` — maximum audio gain (default 1.2)
- `DEFAULT_OLED_BLANK_MINS` — factory default display blanking timeout in minutes (default 0 = never blank)
- `MAX_OLED_BLANK_MINS` — longest blanking timeout selectable in the menu (default 60)
- `LED_ON_LEVEL` (in `Defs.h`) — logic level that lights the onboard LED; set to `LOW` if the LED is inverted
- `DEFAULT_AUDIO_BALANCE` — factory default stereo balance (default 0 = centre)
- `MAX_AUDIO_BALANCE` / `AUDIO_BALANCE_STEP` — balance range either side of centre and menu step size
- `DEBUG` — enable serial debug output

Runtime settings — display blanking timeout and stereo balance — are changed from the
OLED menu and persisted to `/config/config.json`; the defines above only supply the
values used on a factory reset or for a config file written before the setting existed.

## Dependencies

- [ESP8266Audio](https://github.com/earlephilhower/ESP8266Audio) — MP3 decoding
- [ESPAsyncWebServer](https://github.com/esp32async/ESPAsyncWebServer) — async HTTP server
- [ArduinoJson 5](https://github.com/bblanchon/ArduinoJson) — JSON serialization
- [Adafruit SH110X](https://github.com/adafruit/Adafruit_SH110X) — OLED driver
- [Adafruit GFX](https://github.com/adafruit/Adafruit-GFX-Library) — graphics primitives

## License

This project is licensed under a strict Attribution-NonCommercial license — see
[`LICENSE`](LICENSE) for the full text. In short: attribution to Ian Sparkes is required, and
commercial use is not permitted without a separate written license. The third-party libraries
listed above remain under their own licenses.
