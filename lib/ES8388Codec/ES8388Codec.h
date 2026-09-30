// Minimal ES8388 codec bring-up for the ESP32-A1S (AI-Thinker Audio Kit).
//
// Register sequence is the standard one from the ES8388 User Guide, as used
// by e.g. https://github.com/maditnerd/es8388. Verified working on real
// hardware (confirmed via I2C scan + oscilloscope-free listening test).
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
    // I2C device wired to different pins).
    bool begin(TwoWire &wire, int sda, int scl, uint8_t volume = 25);

  private:
    TwoWire *_wire = nullptr;
    bool writeReg(uint8_t reg, uint8_t val);

    static const uint8_t ADDR = 0x10;
};
