#pragma once

#include <Arduino.h>

#include <AudioFileSource.h>
#include <AudioFileSourceBuffer.h>
#include <AudioFileSourceICYStream.h>
#include <AudioGeneratorTalkie.h>
#include <AudioGeneratorMP3.h>
#include <AudioOutputI2S.h>

#include "Defs.h"
#include "DebugManager.h"

const int bufferSize = 256 * 1024; // 64KB buffer in PSRAM (was 16KB in SRAM)

static void StatusCallback(void *cbData, int code, const char *string);
static void MDCallback(void *cbData, const char *type, bool isUnicode, const char *string);

class RadioOutputManager_ {
    private:
      RadioOutputManager_() = default; // Make constructor private
  
    public:
      static RadioOutputManager_ &getInstance(); // Accessor for singleton instance
  
      RadioOutputManager_(const RadioOutputManager_ &) = delete; // no copying
      RadioOutputManager_ &operator=(const RadioOutputManager_ &) = delete;

    public:
      void initializeAudioOutput();
      void startRadioStream(String url, String stationName, float gain);
      void stopRadioStream();
      void StartPlaying();
      void StopPlaying();
      void audioOncePerSecond();
      void audioOncePerHour();
      void audioOncePerLoop();
      bool isPlaying() { return playing; }
      bool isMuted() { return (_fgain == 0.0f); }
      bool togglePlay() {
        if (playing) {
          StopPlaying();
          return false;
        } else {
          StartPlaying();
          return true;
        }
      }

      // Volume control (0-100)
      void setVolume(int vol);

      // Audio mode management
      void setAudioMode(AudioMode mode);
      AudioMode getAudioMode();
      bool isRadioMode();
      bool isBluetoothMode();
      bool isRadioBtMode();
      String getStationName() { return _stationName; }
      String getUrl() { return _url; }

    private:
      AudioFileSourceICYStream *file = nullptr;
      AudioFileSourceBuffer *buff = nullptr;
      AudioGeneratorMP3 *mp3 = nullptr;
      AudioOutput *out = nullptr;
      uint8_t *audioBuffer = nullptr;  // PSRAM-allocated streaming buffer

      float _fgain = DEFAULT_GAIN;
      String _url = "";
      String _stationName = "";
      volatile bool playing = false;
      volatile bool audioTaskRunning = false;
      TaskHandle_t audioTaskHandle = nullptr;
      StackType_t* audioTaskStack = nullptr;    // PSRAM-allocated task stack
      StaticTask_t* audioTaskTCB = nullptr;     // Task control block (DRAM)
      bool audioInlineMode = false;             // true when running decoder in main loop (no task)
      AudioMode currentAudioMode = AUDIO_MODE_RADIO;
      bool btPlayPending = false;  // true while waiting for BT source to connect

      static void audioTask(void *param);
  };
  
  // free function link to the class function
  extern void debugManagerLink(String message);
  
  extern RadioOutputManager_ &radioOutputManager;