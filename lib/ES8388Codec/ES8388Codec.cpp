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

  _wire->beginTransmission(ADDR);
  if (_wire->endTransmission() != 0) {
    return false; // codec not responding
  }

  bool ok = true;
  ok &= writeReg(0x08, 0x00); // MASTERMODE: chip is I2S slave
  ok &= writeReg(0x02, 0xFF); // CHIPPOWER: power down DEM/STM while configuring
  ok &= writeReg(0x2b, 0x80); // DACCONTROL21: same LRCK for ADC/DAC
  ok &= writeReg(0x00, 0x05); // CONTROL1: play & record mode
  ok &= writeReg(0x01, 0x40); // CONTROL2: power up analog + Ibias

  ok &= writeReg(0x04, 0x3C); // DACPOWER: power up DAC, enable LOUT1/ROUT1/LOUT2/ROUT2
  ok &= writeReg(0x17, 0x18); // DACCONTROL1: I2S format, 16-bit
  ok &= writeReg(0x18, 0x02); // DACCONTROL2: DAC MCLK/LRCK ratio = 256
  ok &= writeReg(0x19, 0x00); // DACCONTROL3: unmute
  ok &= writeReg(0x1a, 0x00); // DACCONTROL4: digital volume L = 0dB
  ok &= writeReg(0x1b, 0x00); // DACCONTROL5: digital volume R = 0dB

  // Mixer: route DAC to LOUT/ROUT, and disconnect the analog line/mic-in
  // bypass path (bit7=LD2LO/RD2RO=1 selects DAC, bit6=LI2LO/RI2RO=0 disables
  // the analog passthrough - without this the codec is audible but only
  // plays back whatever is on its analog line/mic input, never the DAC).
  ok &= writeReg(0x26, 0x09); // DACCONTROL16
  ok &= writeReg(0x27, 0x90); // DACCONTROL17: LD2LO=1, LI2LO=0
  ok &= writeReg(0x28, 0x38); // DACCONTROL18
  ok &= writeReg(0x29, 0x38); // DACCONTROL19
  ok &= writeReg(0x2a, 0x90); // DACCONTROL20: RD2RO=1, RI2RO=0

  // Output level (LOUT1/ROUT1/LOUT2/ROUT2). 0 = -45dB, 33 = loudest.
  ok &= writeReg(0x2e, volume); // DACCONTROL24 LOUT1VOL
  ok &= writeReg(0x2f, volume); // DACCONTROL25 ROUT1VOL
  ok &= writeReg(0x30, volume); // DACCONTROL26 LOUT2VOL
  ok &= writeReg(0x31, volume); // DACCONTROL27 ROUT2VOL

  ok &= writeReg(0x02, 0x00); // CHIPPOWER: power DEM/STM back up
  return ok;
}
