#pragma once

#include <Arduino.h>

#include <AudioFileSource.h>
#include <AudioFileSourceBuffer.h>
#include <AudioFileSourceICYStream.h>
#include <AudioGeneratorTalkie.h>
#include <AudioGeneratorMP3.h>
#include <AudioOutputI2S.h>

#include "Defs.h"


class RadioOutputManager_ {
    private:
      RadioOutputManager_() = default; // Make constructor private
  
    public:
      static RadioOutputManager_ &getInstance(); // Accessor for singleton instance
  
      RadioOutputManager_(const RadioOutputManager_ &) = delete; // no copying
      RadioOutputManager_ &operator=(const RadioOutputManager_ &) = delete;

    public:
      void initializeAudioOutput();
      void startRadioStream(const char* url, float gain);
      void stopRadioStream();

    private:
      AudioFileSourceICYStream *file = nullptr;
      AudioFileSourceBuffer *buff = nullptr;
      AudioGeneratorMP3 *mp3 = nullptr;
      AudioOutputI2S *out = nullptr;
      
  };
  
  // free function link to the class function
  extern void debugManagerLink(String message);
  
  extern RadioOutputManager_ &radioOutputManager;