#pragma once

#include "Configuration.h"
#include <Arduino.h>

// -------------------------------------------------------------------------------

#define WDT_TIMEOUT 5

#define SERIAL_BAUD_RATE 115200

// Onboard LED 
#define LED_PIN 2

// Menu system pin names (for clarity in menu code)
// GPIO26-37 are reserved on this board (SPI flash + octal PSRAM) - avoid them.
#define PIN_ENC_CLK     5
#define PIN_ENC_DT      14
#define PIN_ENC_SW      6
#define PIN_BTN_CONFIRM 13
#define PIN_BTN_BACK    4

// I2S Pins
#define I2S_BCLK   15   // "BCK"
#define I2S_LRC    16   // "LRCK" or "WS"
#define I2S_DOUT   17   // "DIN"

// Internally defined - so we don't reaassign them here
// GPIO22-25 do not physically exist on ESP32-S3 (SOC_GPIO_VALID_GPIO_MASK
// excludes them) - SCLint, the old I2S_LRC (23) and I2S_DOUT (25) all moved.
#define SDAint    21
#define SCLint    8
#define RX0Pin    3
#define TX0Pin    1

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels

// ----------------------------------------------------------------
