// Drop-in replacement for ESP8266Audio 2.2.0's AudioOutputI2S that fixes a
// real hang on this hardware: AudioOutputI2S::ConsumeSample() uses a
// non-blocking i2s_write() (0-tick timeout). On the ESP32-A1S, that call
// reliably returns "0 bytes written" forever, even though the DMA buffer
// genuinely has room the entire time (confirmed: a blocking i2s_write() at
// the exact same point succeeds on every single call, instantly). The
// result without this fix is playback that never starts and an MP3 decode
// loop that spins at 100%, never returning - which is audible as continuous
// noise since the codec keeps clocking out I2S garbage the whole time.
//
// Upstream fixed the underlying non-blocking write bug in ESP8266Audio
// 2.4.1 ("Properly timeout for ESP32 I2S writes"), but 2.3.0+ requires the
// new i2s_std.h driver (ESP-IDF 5.x / Arduino-ESP32 3.x), which this board's
// pinned platform (classic Arduino-ESP32 2.x) can't provide without a much
// larger separate migration. This subclass reuses AudioOutputI2S's proven
// pin/clock/rate handling unchanged and only replaces the actual sample
// write with a bounded blocking one.
#pragma once

#include <AudioOutputI2S.h>
#include <driver/i2s.h>

class I2SBlockingOutput : public AudioOutputI2S {
  public:
    using AudioOutputI2S::AudioOutputI2S;

    // Second fix, for white noise on this board: ESP8266Audio 2.2.0's
    // begin() builds its i2s_config_t with
    //     .fixed_mclk = use_mclk, // Unused
    // but fixed_mclk is NOT unused and is not a flag - the IDF reads it as
    // an MCLK frequency in Hz ("If use_apll = true and fixed_mclk > 0, then
    // the clock output for i2s is fixed and equal to the fixed_mclk value.
    // If fixed_mclk set, mclk_multiple won't take effect" - driver/i2s.h).
    // SetPinout(bclk, wclk, dout, mclk) sets use_mclk = true, so simply
    // asking for an MCLK pin writes fixed_mclk = 1, i.e. "MCLK = 1 Hz", and
    // the ES8388 is fed a clock it cannot possibly lock to. The ES8388 needs
    // a real MCLK here (DACCONTROL2 = 0x02 selects MCLK = 256 * fs), so the
    // pin cannot just be dropped.
    //
    // Install the driver ourselves instead, with the values proven on the
    // AudioKit V2.2 tone test: no APLL, fixed_mclk 0, MCLK = 256 * fs.
    // Everything else (pin routing, rate changes, teardown) stays with the
    // base class. begin() is the virtual entry point AudioGenerator calls.
    bool begin() override {
      if (!i2sOn) {
        i2s_config_t cfg = {
          .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX),
          .sample_rate = 44100,                            // replaced by SetRate() below
          .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
          .channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT,
          .communication_format = I2S_COMM_FORMAT_STAND_I2S,
          .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
          .dma_buf_count = dma_buf_count,
          .dma_buf_len = 64,
          .use_apll = false,
          .tx_desc_auto_clear = true,                      // silence, not noise, on underflow
          .fixed_mclk = 0,                                 // 0 = derive MCLK from mclk_multiple
          .mclk_multiple = I2S_MCLK_MULTIPLE_256,
          .bits_per_chan = I2S_BITS_PER_CHAN_DEFAULT,
        };
        if (i2s_driver_install((i2s_port_t)portNo, &cfg, 0, NULL) != ESP_OK) return false;
        SetPinout();   // routes MCLK to mclkPin, as set by the 4-arg SetPinout()
        i2s_zero_dma_buffer((i2s_port_t)portNo);
        i2sOn = true;
      }
      SetRate(hertz ? hertz : 44100);
      return true;
    }

    bool ConsumeSample(int16_t sample[2]) override {
      if (!i2sOn) return false;

      int16_t ms[2] = {sample[0], sample[1]};
      MakeSampleStereo16(ms);
      if (mono) {
        int32_t ttl = ms[LEFTCHANNEL] + ms[RIGHTCHANNEL];
        ms[LEFTCHANNEL] = ms[RIGHTCHANNEL] = (int16_t)((ttl >> 1) & 0xffff);
      }

      uint32_t s32 = ((uint32_t)(uint16_t)Amplify(ms[RIGHTCHANNEL]) << 16) |
                     ((uint32_t)(uint16_t)Amplify(ms[LEFTCHANNEL]) & 0xffff);
      size_t written = 0;
      // Bounded (not infinite) so a genuinely stalled peripheral fails
      // loudly instead of hanging the audio task forever.
      i2s_write((i2s_port_t)portNo, &s32, sizeof(s32), &written, pdMS_TO_TICKS(100));
      return written == sizeof(s32);
    }
};
