#pragma once

#include "Configuration.h"
#include <Arduino.h>

// -------------------------------------------------------------------------------

#define WDT_TIMEOUT 5

#define SERIAL_BAUD_RATE 115200

// Onboard LED 
#define LED_PIN 2

// Menu system pin names (for clarity in menu code)
#define PIN_ENC_CLK     5
#define PIN_ENC_DT      14
#define PIN_ENC_SW      27
#define PIN_BTN_CONFIRM 13
#define PIN_BTN_BACK    4

// I2S Pins
#define I2S_BCLK   33
#define I2S_LRC    23
#define I2S_DOUT   25

// Internally defined - so we don't reaassign them here
#define SDAint    21
#define SCLint    22
#define RX0Pin    3
#define TX0Pin    1

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels

// Audio Modes
enum AudioMode {
  AUDIO_MODE_RADIO,
  AUDIO_MODE_BLUETOOTH
};

// ----------------------------------------------------------------
