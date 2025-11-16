#include "RadioOutputManager.h"

void RadioOutputManager_::initializeAudioOutput() {
  debugManagerLink("RadioOutputManager: Initializing audio output");

  out = new AudioOutputI2S(); // Output to externalDAC
  out->SetPinout(I2S_BCLK, I2S_LRC, I2S_DOUT); // (int bclk, int lrc, int dout)
  out->SetOutputModeMono(true);
}

void RadioOutputManager_::startRadioStream(const char* url, float gain) {
  debugManagerLink("RadioOutputManager: Starting radio stream: " + String(url) + " with gain: " + String(gain));

  file = new AudioFileSourceICYStream(url);
  buff = new AudioFileSourceBuffer(file, 2048);
  out->SetGain(gain);
  mp3 = new AudioGeneratorMP3();
  mp3->begin(buff, out);
}

void RadioOutputManager_::stopRadioStream() {
  debugManagerLink("RadioOutputManager: Stopping radio stream");

  if (mp3) {
    mp3->stop();
    delete mp3;
    mp3 = nullptr;
  }
  if (buff) {
    buff->close();
    delete buff;
    buff = nullptr;
  }
  if (file) {
    file->close();
    delete file;
    file = nullptr;
  }
}

// ************************************************************
// Library internal singleton wiring
// ************************************************************
RadioOutputManager_ &RadioOutputManager_::getInstance() {
  static RadioOutputManager_ instance;
  return instance;
}

RadioOutputManager_ &radioOutputManager = radioOutputManager.getInstance();