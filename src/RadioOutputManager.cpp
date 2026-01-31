#include "RadioOutputManager.h"
#include "BluetoothManager.h"
#include "RadioMenuConfiguration.h"
#include "Globals.h"
#include <WiFi.h>
#include <driver/i2s.h>
#include "WiFiManager.h"

// ************************************************************
// Set up the audio output
// ************************************************************
void RadioOutputManager_::initializeAudioOutput() {
  debugManagerLink("RadioOutputManager: Initializing audio output");

  // Set default station from stored station list
  if (stationCount > 0) {
    _url = stations[0].url;
    _stationName = stations[0].name;
  } else {
    _url = "http://mp3.ffh.de/radioffh/hqlivestream.mp3";
    _stationName = "Radio FFH";
  }
  _fgain = (volume / 100.0f) * MAX_GAIN;
}

// ************************************************************
// Initiate a radio stream
// ************************************************************
void RadioOutputManager_::startRadioStream(String url, String stationName, float gain) {
  debugManagerLink("RadioOutputManager: Starting radio stream: " + url + " with gain: " + String(gain));

  // Stop any existing playback first
  if (playing) {
    StopPlaying();
  }

  _url = url;
  _stationName = stationName;
  _fgain = gain;
  StartPlaying();
}

// ************************************************************
// Start playing the stream
// ************************************************************
void RadioOutputManager_::StartPlaying() {
  debugMsgAud("Start play");
  if (_url.length() == 0) {
    debugMsgAud("No URL set - cannot play");
    menuSystem.showFlashMessage("No URL set");
    return;
  }
  if (WiFi.status() != WL_CONNECTED) {
    debugMsgAud("No WiFi - cannot play");
    menuSystem.showFlashMessage("No WiFi connection");
    return;
  }

  // Clean up any leftover objects
  StopPlaying();

  file = new AudioFileSourceICYStream(_url.c_str());
  file->RegisterMetadataCB(MDCallback, (void*)"ICY");
  buff = new AudioFileSourceBuffer(file, bufferSize);
  buff->RegisterStatusCB(StatusCallback, (void*)"buffer");
  out = new AudioOutputI2S();
  out->SetPinout(I2S_BCLK, I2S_LRC, I2S_DOUT);
  out->SetOutputModeMono(true);
  out->SetGain(_fgain);
  mp3 = new AudioGeneratorMP3();
  mp3->RegisterStatusCB(StatusCallback, (void*)"mp3");
  mp3->begin(buff, out);

  playing = true;
  audioTaskRunning = true;

  // Run MP3 decoder on dedicated task on core 0 (WiFi core has idle time)
  xTaskCreatePinnedToCore(audioTask, "audio", 4096, this, 3, &audioTaskHandle, 0);

  debugMsgAud("STATUS(URL) " + _url);
}

// ************************************************************
// Stop playing the stream
// ************************************************************
void RadioOutputManager_::StopPlaying() {
  debugMsgAud("Stop play");

  // Stop the audio task first
  audioTaskRunning = false;
  if (audioTaskHandle) {
    // Wait for the task to finish
    vTaskDelay(pdMS_TO_TICKS(50));
    audioTaskHandle = nullptr;
  }

  if (mp3) {
    mp3->stop();
    delete mp3;
    mp3 = NULL;
  }
  if (buff) {
    buff->close();
    delete buff;
    buff = NULL;
  }
  if (file) {
    file->close();
    delete file;
    file = NULL;
  }
  if (out) {
    // mp3->stop() calls out->stop() which sets i2sOn=false,
    // so the destructor won't uninstall the I2S driver.
    // We must uninstall it explicitly to free the I2S port
    // for Bluetooth A2DP.
    i2s_driver_uninstall(I2S_NUM_0);
    delete out;
    out = NULL;
  }

  playing = false;
}

// ************************************************************
// Set volume (0-100) - applies to current audio mode
// ************************************************************
void RadioOutputManager_::setVolume(int vol) {
  _fgain = (vol / 100.0f) * MAX_GAIN;
  if (currentAudioMode == AUDIO_MODE_RADIO && out) {
    out->SetGain(_fgain);
  } else if (currentAudioMode == AUDIO_MODE_BLUETOOTH) {
    // A2DP volume is 0-127
    bluetoothManager.setVolume((uint8_t)(vol * 127 / 100));
  }
}

// ************************************************************
//
// ************************************************************
void RadioOutputManager_::audioOncePerSecond() {
  debugMsgAud("Buffer " + String(buff->getFillLevel()) + "/" + String(bufferSize));
}

// ************************************************************
// 
// ************************************************************
void RadioOutputManager_::audioOncePerHour() {
  // Placeholder for actions to be performed once per hour
}

// ************************************************************
// 
// ************************************************************
void RadioOutputManager_::audioOncePerLoop() {
  // Check if the audio task flagged stream end - clean up from main loop context
  if (!audioTaskRunning && !playing && (mp3 || buff || file || out)) {
    debugMsgAud("Cleaning up after stream end");
    StopPlaying();
  }
}

// ************************************************************
// Dedicated audio task - runs on core 0
// ************************************************************
void RadioOutputManager_::audioTask(void *param) {
  RadioOutputManager_ *self = static_cast<RadioOutputManager_ *>(param);
  while (self->audioTaskRunning) {
    if (self->playing && self->mp3) {
      if (!self->mp3->loop()) {
        debugMsgAud("Stream ended - stopping playback");
        self->audioTaskRunning = false;
        self->playing = false;
        // Can't safely delete objects from this task context,
        // flag for main loop cleanup
      }
    }
    vTaskDelay(1);  // Yield briefly to avoid watchdog
  }
  vTaskDelete(NULL);
}

static void MDCallback(void *cbData, const char *type, bool isUnicode, const char *string) {
  const char *ptr = reinterpret_cast<const char *>(cbData);
  (void) isUnicode; // Punt this ball for now
  // Note that the type and string may be in PROGMEM, so copy them to RAM for printf
  char s1[32], s2[64];
  strncpy_P(s1, type, sizeof(s1));
  s1[sizeof(s1) - 1] = 0;
  strncpy_P(s2, string, sizeof(s2));
  s2[sizeof(s2) - 1] = 0;
  debugMsgInr("METADATA(" + String(ptr) + ") '" + String(s1) + "' = '" + String(s2));                                                                      
}

static void StatusCallback(void *cbData, int code, const char *string) {
  const char *ptr = reinterpret_cast<const char *>(cbData);
  // Note that the string may be in PROGMEM, so copy it to RAM for printf
  char s1[64];
  strncpy_P(s1, string, sizeof(s1));
  s1[sizeof(s1) - 1] = 0;
  debugMsgInr("STATUS(" + String(ptr) + ") '" + String(code) + "' = '" + String(s1));
}

void RadioOutputManager_::stopRadioStream() {
  debugManagerLink("RadioOutputManager: Stopping radio stream");
  StopPlaying();
}

// ************************************************************
// Set audio mode (Radio or Bluetooth)
// ************************************************************
void RadioOutputManager_::setAudioMode(AudioMode mode) {
  debugManagerLink("RadioOutputManager: Setting audio mode to " + String(mode == AUDIO_MODE_RADIO ? "Radio" : "Bluetooth"));

  // Stop current playback if switching modes
  if (currentAudioMode != mode) {
    if (currentAudioMode == AUDIO_MODE_RADIO) {
      StopPlaying();  // Always ensure I2S is released
    } else if (currentAudioMode == AUDIO_MODE_BLUETOOTH) {
      bluetoothManager.stopBluetooth();
    }

    currentAudioMode = mode;

    // Start the new mode
    if (mode == AUDIO_MODE_BLUETOOTH) {
      // WiFi and BT Classic share the 2.4GHz radio and compete for heap.
      // Disconnect WiFi to free resources for A2DP streaming.
      if (WiFi.isConnected()) {
        debugManagerLink("Disconnecting WiFi for Bluetooth mode");
        WiFi.disconnect(true);  // true = turn off WiFi radio
        delay(100);
      }
      bluetoothManager.startBluetooth();
    } else if (mode == AUDIO_MODE_RADIO) {
      // Reconnect WiFi when switching back to radio
      debugManagerLink("Reconnecting WiFi for Radio mode");
      wifiManager.connectToLastAP();
    }
  }
}

// ************************************************************
// Get current audio mode
// ************************************************************
AudioMode RadioOutputManager_::getAudioMode() {
  return currentAudioMode;
}

// ************************************************************
// Check if in Radio mode
// ************************************************************
bool RadioOutputManager_::isRadioMode() {
  return currentAudioMode == AUDIO_MODE_RADIO;
}

// ************************************************************
// Check if in Bluetooth mode
// ************************************************************
bool RadioOutputManager_::isBluetoothMode() {
  return currentAudioMode == AUDIO_MODE_BLUETOOTH;
}



// ************************************************************
// Library internal singleton wiring
// ************************************************************
RadioOutputManager_ &RadioOutputManager_::getInstance() {
  static RadioOutputManager_ instance;
  return instance;
}

RadioOutputManager_ &radioOutputManager = radioOutputManager.getInstance();