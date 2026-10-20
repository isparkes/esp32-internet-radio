#pragma once

#include <Arduino.h>
#include "Globals.h"
#include "Defs.h"

// ************************************************************
// Status LED patterns
//
// Driven from the main loop rather than a hardware timer, so "off" is a plain
// digitalWrite that nothing can leave asserted behind our back.
// ************************************************************
#define LED_MODE_OFF     0   // Radio stopped - LED dark
#define LED_MODE_PLAYING 1   // Radio playing - slow flash
#define LED_MODE_STARTUP 2   // Booting - rapid flash, the first sign of life
#define LED_MODE_RESYNC  3   // Stream dropped, waiting to reconnect - medium flash

#define LED_STARTUP_ON_MS    60    // Rapid: ~8 flashes/s
#define LED_STARTUP_OFF_MS   60
#define LED_PLAYING_ON_MS   100    // Slow: a short blink every 2s
#define LED_PLAYING_OFF_MS 1900
#define LED_RESYNC_ON_MS    100    // Medium: 2 flashes/s
#define LED_RESYNC_OFF_MS   400

// ************************************************************
// Public prototypes
// ************************************************************
void startLed();
void setLedFlashType(byte flashType);
void ledOncePerLoop();
