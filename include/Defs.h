#pragma once

#include "Configuration.h"
#include <Arduino.h>

// -------------------------------------------------------------------------------

#define WDT_TIMEOUT 5

#define SERIAL_BAUD_RATE 115200

// ESP32-A1S (AI-Thinker Audio Kit) pin map.
//
// Every pin on this board's expansion header already has something else
// wired to it (see below) - there are no genuinely free pins. Encoder/button
// pins below are wired in parallel with the board's onboard buttons; this
// works but means pressing the physical board button and an externally
// wired control on the same net can both trigger the same input.
//
// No onboard status LED: GPIO22 (the board's own LED) is repurposed below
// as PIN_BTN_BACK, so LED_PIN is intentionally not defined - the OLED shows
// connection/playing status instead. See TimerManager.cpp.

// Menu system pins (each shares a net with an onboard tactile button):
#define PIN_ENC_CLK     5    // onboard KEY6
#define PIN_ENC_DT      18   // onboard KEY5
#define PIN_ENC_SW      23   // onboard KEY4
#define PIN_BTN_CONFIRM 19   // onboard KEY3
#define PIN_BTN_BACK    22   // onboard LED pad (LED function given up)

// I2S pins to the ES8388 codec (fixed by the board's own wiring - see
// board_pins_config.c in the ESP-ADF board support package for this board)
#define I2S_MCLK   0
#define I2S_BCLK   27
#define I2S_LRC    25   // "LRCK" or "WS"
#define I2S_DOUT   26   // "DIN"

// ES8388 codec's I2C control bus - physically separate bus from the OLED's,
// requires the second hardware I2C peripheral (Wire1). See RadioOutputManager.
#define CODEC_SDA  33
#define CODEC_SCL  32

// Speaker amplifier enable - must be driven HIGH or there is no audio output
// at all, even with the codec correctly configured.
#define PA_ENABLE_PIN 21

// OLED I2C bus. Repurposes the USB-serial TX0/RX0 pins (GPIO1/3) since no
// other pins are free - this permanently loses the Serial/debug console
// during normal operation (re-flashing over USB is unaffected).
#define SDAint    1
#define SCLint    3

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels

// ----------------------------------------------------------------
