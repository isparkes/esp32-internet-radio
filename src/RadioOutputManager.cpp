#include "RadioOutputManager.h"
#include "BluetoothManager.h"
#include "RadioMenuConfiguration.h"
#include "Globals.h"
#include <WiFi.h>
#include <driver/i2s.h>

// ************************************************************
// Custom AudioOutput that routes decoded PCM into the BT PCM ring buffer.
// Used when streaming radio to a Bluetooth speaker (A2DP source mode).
// ************************************************************
#ifdef FEATURE_BLUETOOTH
class AudioOutputBTBuffer : public AudioOutput {
public:
  bool begin() override { return true; }
  bool stop() override { return true; }
  bool ConsumeSample(int16_t sample[2]) override {
    // Apply gain (same fixed-point scheme as AudioOutputI2S)
    int16_t left  = (int32_t(sample[0]) * gainF2P6) >> 6;
    int16_t right = (int32_t(sample[1]) * gainF2P6) >> 6;
    // Return false when the ring buffer is full so the MP3 generator pauses.
    // This throttles the decoder to real-time speed and prevents the HTTP
    // download buffer from being drained faster than WiFi can refill it.
    return BluetoothManager_::writePcmFrame(left, right);
  }
};
#endif

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
  _songTitle[0] = '\0';
  StartPlaying();
}

// ************************************************************
// Start playing the stream
// ************************************************************
void RadioOutputManager_::StartPlaying() {
  debugMsgAud("Start play: mode=" + String(currentAudioMode) +
              " WiFi=" + String(WiFi.status()) +
              " url=" + _url);
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

  // Allocate streaming buffer from PSRAM if available, otherwise fall back to SRAM
  if (psramFound()) {
    audioBuffer = (uint8_t *)ps_malloc(bufferSize);
    debugMsgAud("Audio buffer: " + String(bufferSize / 1024) + "KB from PSRAM");
  }
  if (!audioBuffer) {
    audioBuffer = (uint8_t *)malloc(bufferSize);
    debugMsgAud("Audio buffer: " + String(bufferSize / 1024) + "KB from SRAM");
  }
  buff = new AudioFileSourceBuffer(file, audioBuffer, bufferSize);
  buff->RegisterStatusCB(StatusCallback, (void*)"buffer");

#ifdef FEATURE_BLUETOOTH
  if (currentAudioMode == AUDIO_MODE_RADIO_BLUETOOTH) {
    out = new AudioOutputBTBuffer();
  } else
#endif
  {
    AudioOutputI2S *i2sOut = new AudioOutputI2S();
    i2sOut->SetPinout(I2S_BCLK, I2S_LRC, I2S_DOUT);
    i2sOut->SetOutputModeMono(true);
    out = i2sOut;
  }
  out->SetGain(_fgain);
  mp3 = new AudioGeneratorMP3();
  mp3->RegisterStatusCB(StatusCallback, (void*)"mp3");
  mp3->begin(buff, out);

  playing = true;
  audioTaskRunning = true;

  debugMsgAud("Free heap before task create: " + String(ESP.getFreeHeap()) + " bytes");

  // Try to run the decoder as a pinned task. When BT A2DP source is active it
  // consumes most of the internal DRAM heap, leaving too little for a task stack.
  // In that case fall back to calling mp3->loop() inline from audioOncePerLoop().
  audioInlineMode = false;
  BaseType_t taskResult = xTaskCreatePinnedToCore(audioTask, "audio", 4096, this, 3, &audioTaskHandle, 1);
  if (taskResult != pdPASS) {
    debugMsgAud("Task creation failed (heap=" + String(ESP.getFreeHeap()) + ") - running inline");
    audioTaskHandle = nullptr;
    audioInlineMode = true;
  }

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
  // Free PSRAM task stack and TCB (only used when stack was PSRAM-allocated)
  if (audioTaskStack) { free(audioTaskStack); audioTaskStack = nullptr; }
  if (audioTaskTCB)   { free(audioTaskTCB);   audioTaskTCB   = nullptr; }
  audioInlineMode = false;

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
#ifdef FEATURE_BLUETOOTH
    if (currentAudioMode != AUDIO_MODE_RADIO_BLUETOOTH) {
#endif
      // mp3->stop() sets i2sOn=false so the destructor won't uninstall the driver.
      // Uninstall explicitly to free the I2S port.
      i2s_driver_uninstall(I2S_NUM_0);
#ifdef FEATURE_BLUETOOTH
    }
#endif
    delete out;
    out = NULL;
  }
  if (audioBuffer) {
    free(audioBuffer);
    audioBuffer = NULL;
  }

  btPlayPending = false;
  playing = false;
}

// ************************************************************
// Set volume (0-100) - applies to current audio mode
// ************************************************************
void RadioOutputManager_::setVolume(int vol) {
  _fgain = (vol / 100.0f) * MAX_GAIN;
  if ((currentAudioMode == AUDIO_MODE_RADIO || currentAudioMode == AUDIO_MODE_RADIO_BLUETOOTH) && out) {
    out->SetGain(_fgain);
  }
#ifdef FEATURE_BLUETOOTH
  else if (currentAudioMode == AUDIO_MODE_BLUETOOTH) {
    // A2DP volume is 0-127
    bluetoothManager.setVolume((uint8_t)(vol * 127 / 100));
  }
#endif
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
  // Run the decoder inline when no task could be created (e.g. DRAM exhausted by BT).
  // Core 1 WDT is disabled so brief blocking on network I/O is safe.
  if (audioInlineMode && playing && mp3) {
    if (!mp3->loop()) {
      debugMsgAud("Stream ended (inline)");
      playing = false;
      audioTaskRunning = false;
      audioInlineMode = false;
    }
  }

  // Check if the audio task flagged stream end - clean up from main loop context
  if (!audioTaskRunning && !playing && (mp3 || buff || file || out)) {
    debugMsgAud("Cleaning up after stream end");
    StopPlaying();
  }

#ifdef FEATURE_BLUETOOTH
  if (btPlayPending) {
    static unsigned long lastBtLog = 0;
    if (millis() - lastBtLog > 3000) {
      lastBtLog = millis();
      debugMsgAud("BT wait: connected=" + String(bluetoothManager.isBluetoothSourceConnected()) +
                  " cbFired=" + String(bluetoothManager.isBluetoothSourceAudioStarted()) +
                  " WiFi=" + String(WiFi.status()));
    }

    if (bluetoothManager.isBluetoothSourceAudioStarted()) {
      btPlayPending = false;
      menuSystem.showFlashMessage("BT ready - streaming");
      StartPlaying();
    }
  }
#endif
}

// ************************************************************
// Dedicated audio task - runs on core 0
// ************************************************************
void RadioOutputManager_::audioTask(void *param) {
  RadioOutputManager_ *self = static_cast<RadioOutputManager_ *>(param);

  debugMsgAud("Audio task started on core " + String(xPortGetCoreID()));

  while (self->audioTaskRunning) {
    if (self->playing && self->mp3) {
      if (!self->mp3->loop()) {
        debugMsgAud("Stream ended - stopping playback");
        self->audioTaskRunning = false;
        self->playing = false;
      }
    }
    vTaskDelay(1);  // Yield to allow other tasks to run
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
  if (strcmp(s1, "StreamTitle") == 0) {
    radioOutputManager.setSongTitle(s2);
  }
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
  debugManagerLink("RadioOutputManager: Setting audio mode to " + String(mode));

#ifndef FEATURE_BLUETOOTH
  if (mode == AUDIO_MODE_BLUETOOTH || mode == AUDIO_MODE_RADIO_BLUETOOTH) {
    debugManagerLink("Bluetooth not supported on this platform");
    return;
  }
#endif

  if (currentAudioMode == mode) return;

  // Stop whatever is currently running
  if (currentAudioMode == AUDIO_MODE_RADIO) {
    StopPlaying();
  }
#ifdef FEATURE_BLUETOOTH
  else if (currentAudioMode == AUDIO_MODE_BLUETOOTH) {
    bluetoothManager.stopBluetooth();
  } else if (currentAudioMode == AUDIO_MODE_RADIO_BLUETOOTH) {
    StopPlaying();
    bluetoothManager.stopBluetoothSource();
  }
#endif

  currentAudioMode = mode;

  // Start the new mode
#ifdef FEATURE_BLUETOOTH
  if (mode == AUDIO_MODE_BLUETOOTH) {
    bluetoothManager.startBluetooth();
  } else if (mode == AUDIO_MODE_RADIO_BLUETOOTH) {
    // Start BT scanning first. WiFi streaming starts only once BT has connected,
    // because ESP32 shares one radio and active WiFi blocks BT inquiry scans.
    bluetoothManager.startBluetoothSource("XTREME");
    btPlayPending = true;
    menuSystem.showFlashMessage("Scanning for XTREME...");
  }
#endif
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
// Check if in Bluetooth sink mode
// ************************************************************
bool RadioOutputManager_::isBluetoothMode() {
  return currentAudioMode == AUDIO_MODE_BLUETOOTH;
}

// ************************************************************
// Check if in Radio→BT source mode
// ************************************************************
bool RadioOutputManager_::isRadioBtMode() {
  return currentAudioMode == AUDIO_MODE_RADIO_BLUETOOTH;
}



// ************************************************************
// Library internal singleton wiring
// ************************************************************
RadioOutputManager_ &RadioOutputManager_::getInstance() {
  static RadioOutputManager_ instance;
  return instance;
}

RadioOutputManager_ &radioOutputManager = radioOutputManager.getInstance();