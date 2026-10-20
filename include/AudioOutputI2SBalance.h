// ************************************************************
// AudioOutputI2S with a stereo balance (pan) control
//
// The PCM5102 has no control registers on this wiring, so balance has to be
// applied to the sample stream. AudioGeneratorMP3 pushes every frame through
// the virtual ConsumeSample(), and the base ConsumeSamples() just loops over
// it, so overriding ConsumeSample() catches all of the audio.
// ************************************************************

#pragma once

#include <AudioOutputI2S.h>

class AudioOutputI2SBalance : public AudioOutputI2S {
  public:
    AudioOutputI2SBalance(int port = 0, int output_mode = EXTERNAL_I2S,
                          int dma_buf_count = 8, int use_apll = APLL_DISABLE)
      : AudioOutputI2S(port, output_mode, dma_buf_count, use_apll) {}

    // -100 = full left, 0 = centre, +100 = full right.
    // Only ever attenuates the opposite channel - boosting would clip against
    // the gain ceiling in the base class Amplify().
    void SetBalance(int balance) {
      if (balance < -100) balance = -100;
      if (balance >  100) balance =  100;
      _balance = balance;
      _leftQ8  = (balance > 0) ? ((100 - balance) * 256) / 100 : 256;
      _rightQ8 = (balance < 0) ? ((100 + balance) * 256) / 100 : 256;
    }

    int GetBalance() const { return _balance; }

    // The decoder reports the stream's channel count here. Remember it and
    // always tell the base class "stereo": its MakeSampleStereo16() copies
    // left over right whenever channels == 1, which runs after our scaling
    // and would undo the balance on mono streams. We do the mono duplication
    // ourselves instead. The base only stores the count - the I2S peripheral
    // is configured for stereo frames either way.
    virtual bool SetChannels(int channels) override {
      if ((channels < 1) || (channels > 2)) return false;
      _srcChannels = channels;
      return AudioOutputI2S::SetChannels(2);
    }

    // MP3 always decodes to 16 bits, so the 8-bit upsample in the base class
    // is never in play here and scaling before the call is safe.
    virtual bool ConsumeSample(int16_t sample[2]) override {
      int16_t s[2];
      s[LEFTCHANNEL]  = sample[LEFTCHANNEL];
      s[RIGHTCHANNEL] = (_srcChannels == 1) ? sample[LEFTCHANNEL] : sample[RIGHTCHANNEL];

      if (_balance != 0) {
        s[LEFTCHANNEL]  = (int16_t)(((int32_t)s[LEFTCHANNEL]  * _leftQ8)  >> 8);
        s[RIGHTCHANNEL] = (int16_t)(((int32_t)s[RIGHTCHANNEL] * _rightQ8) >> 8);
      }

      return AudioOutputI2S::ConsumeSample(s);
    }

  private:
    int _balance     = 0;
    int32_t _leftQ8  = 256;   // Q8 channel scalers, 256 = unity
    int32_t _rightQ8 = 256;
    int _srcChannels = 2;
};
