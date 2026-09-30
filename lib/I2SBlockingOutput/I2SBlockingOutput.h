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
