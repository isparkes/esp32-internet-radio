#include "ES8388Codec.h"

bool ES8388Codec::writeReg(uint8_t reg, uint8_t val) {
  _wire->beginTransmission(ADDR);
  _wire->write(reg);
  _wire->write(val);
  return _wire->endTransmission() == 0;
}

bool ES8388Codec::begin(TwoWire &wire, int sda, int scl, uint8_t volume) {
  _wire = &wire;
  _wire->begin(sda, scl);
  _wire->setClock(100000);

  _wire->beginTransmission(ADDR);
  if (_wire->endTransmission() != 0) {
    return false; // codec not responding
  }

  // Start from a known state. Without the reset, a warm restart (which does
  // not power-cycle the codec) re-runs this sequence on top of whatever the
  // previous run left behind.
  bool ok = true;
  ok &= writeReg(0x00, 0x80);  delay(10);   // ESCONTROL0: reset
  ok &= writeReg(0x00, 0x00);  delay(10);

  // --- power / reference ---
  ok &= writeReg(0x02, 0xF3);  // CHIPPOWER:  hold DAC+ADC down while configuring
  ok &= writeReg(0x01, 0x50);  // CONTROL2:   VMID 50k, analog reference on
  ok &= writeReg(0x08, 0x00);  // MASTERMODE: codec is I2S *slave*, ESP32 drives BCLK/LRCK
  ok &= writeReg(0x03, 0xFF);  // ADCPOWER:   ADC fully down - this build is playback-only
  ok &= writeReg(0x04, 0x3C);  // DACPOWER:   LOUT1/ROUT1 (speaker amp) + LOUT2/ROUT2 (headphone)

  // --- DAC serial format ---
  ok &= writeReg(0x17, 0x18);  // DACCONTROL1: I2S format, 16-bit word length
  ok &= writeReg(0x18, 0x02);  // DACCONTROL2: MCLK = 256 * fs (see I2SBlockingOutput)
  ok &= writeReg(0x1A, 0x00);  // DACCONTROL4: left  DAC digital volume = 0 dB
  ok &= writeReg(0x1B, 0x00);  // DACCONTROL5: right DAC digital volume = 0 dB

  // --- output mixer routing ---
  // Without 0x27/0x2A the DAC is not connected to the output mixers and the
  // chip is silent no matter what the volumes or the amp enable do. Bit 7
  // (LD2LO/RD2RO) selects the DAC; bit 6 (LI2LO/RI2LO) would mix in the
  // analog line/mic input instead and stays clear.
  ok &= writeReg(0x26, 0x00);  // DACCONTROL16: no line-in feed into the mixer
  ok &= writeReg(0x27, 0xB8);  // DACCONTROL17: left  DAC -> left  output mixer (LD2LO)
  ok &= writeReg(0x2A, 0xB8);  // DACCONTROL20: right DAC -> right output mixer (RD2RO)
  ok &= writeReg(0x2B, 0x80);  // DACCONTROL21: run DACLRC/ADCLRC from one clock, mixers on
  ok &= writeReg(0x2D, 0x00);  // DACCONTROL23: VROI normal

  // --- output level (0x00..0x21, 0x1E = 0 dB, 1.5 dB per step) ---
  if (volume > 0x21) volume = 0x21;
  ok &= writeReg(0x2E, volume); // LOUT1 volume (speaker amp path)
  ok &= writeReg(0x2F, volume); // ROUT1 volume
  ok &= writeReg(0x30, volume); // LOUT2 volume (headphone path)
  ok &= writeReg(0x31, volume); // ROUT2 volume

  ok &= writeReg(0x02, 0x00);  // CHIPPOWER: everything back up
  ok &= writeReg(0x19, 0x00);  // DACCONTROL3: unmute (0x04 would mute)
  return ok;
}
