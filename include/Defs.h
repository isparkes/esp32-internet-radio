#pragma once

#include "Configuration.h"
#include <Arduino.h>

// -------------------------------------------------------------------------------

#define WDT_TIMEOUT 5

#define SERIAL_BAUD_RATE 115200

// Onboard LED 
#define LED_PIN 2

// Encoder
#define ENC_APin  14
#define ENC_BPin  5
#define ENC_BTN   16

#define BTNCONF   4   // Play / Pause
#define BTNBACK   2   // Station Select / Volume

// I2S Pins
#define I2S_BCLK   33
#define I2S_LRC    14
#define I2S_DOUT   25

// Internally defined - so we don't reaassign them here
#define SDAint    21
#define SCLint    22
#define RX0Pin    3
#define TX0Pin    1

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels

// ----------------------------------------------------------------
