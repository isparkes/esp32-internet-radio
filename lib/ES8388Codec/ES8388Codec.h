// Minimal ES8388 codec bring-up for the ESP32-A1S (AI-Thinker Audio Kit).
//
// Register sequence is the one verified on AudioKit V2.2 hardware by the
// standalone tone test in esp32_drum_computer/src/main.cpp - keep the two
// in step, that sketch is the reference for "the codec is definitely set up
// right" when chasing an audio fault here.
//
// Uses a dedicated TwoWire instance because the codec's I2C control bus
// (GPIO33/32) is physically separate from the board's other I2C bus (used
// here for the OLED) - they cannot share the default Wire peripheral.
#pragma once

#include <Arduino.h>
#include <Wire.h>

class ES8388Codec {
  public:
    // sda/scl: codec's I2C control bus pins. wire: which hardware I2C
    // peripheral to use (must not be the same instance/bus as any other
    // I2C device wired to different pins). volume: LOUT/ROUT analog output
    // level, 0x00..0x21 in 1.5 dB steps, 0x1E = 0 dB.
    bool begin(TwoWire &wire, int sda, int scl, uint8_t volume = 0x1E);

  private:
    TwoWire *_wire = nullptr;
    bool writeReg(uint8_t reg, uint8_t val);

    static const uint8_t ADDR = 0x10;
};
