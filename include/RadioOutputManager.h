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

const int bufferSize = 16 * 1024; // buffer in byte, reduced from 16kb to 8kb to save RAM

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
      String getStationName() { return _stationName; }
      String getUrl() { return _url; }

    private:
      AudioFileSourceICYStream *file = nullptr;
      AudioFileSourceBuffer *buff = nullptr;
      AudioGeneratorMP3 *mp3 = nullptr;
      AudioOutputI2S *out = nullptr;

      float _fgain = DEFAULT_GAIN;
      String _url = "";
      String _stationName = "";
      volatile bool playing = false;
      volatile bool audioTaskRunning = false;
      TaskHandle_t audioTaskHandle = nullptr;
      AudioMode currentAudioMode = AUDIO_MODE_RADIO;

      static void audioTask(void *param);
  };
  
  // free function link to the class function
  extern void debugManagerLink(String message);
  
  extern RadioOutputManager_ &radioOutputManager;