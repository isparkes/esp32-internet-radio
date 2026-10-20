#include "RadioOutputManager.h"
#include <SPIFFS.h>
#include <AudioFileSourceFS.h>
#include "RadioMenuConfiguration.h"
#include "Globals.h"
#include <WiFi.h>

// ************************************************************
// Play a short startup jingle via I2S
// ************************************************************
void RadioOutputManager_::playStartupJingle() {
  AudioFileSourceFS *src = new AudioFileSourceFS(SPIFFS, "/startup.mp3");
  if (!src->isOpen()) {
    debugMsgAud("startup.mp3 not found in SPIFFS - skipping jingle");
    delete src;
    return;
  }

  AudioOutputI2SBalance *jingleOut = new AudioOutputI2SBalance(0, AudioOutputI2S::EXTERNAL_I2S, 8, AudioOutputI2S::APLL_AUTO);
  jingleOut->SetPinout(I2S_BCLK, I2S_LRC, I2S_DOUT);
  jingleOut->SetGain(DEFAULT_GAIN);
  jingleOut->SetBalance(cc->audioBalance);

  AudioGeneratorMP3 *mp3Jingle = new AudioGeneratorMP3();
  mp3Jingle->begin(src, jingleOut);
  while (mp3Jingle->isRunning()) {
    if (!mp3Jingle->loop()) mp3Jingle->stop();
  }

  delete mp3Jingle;
  delete src;
  delete jingleOut;
}

// ************************************************************
// Set up the audio output
// ************************************************************
void RadioOutputManager_::initializeAudioOutput() {
  debugManagerLink("RadioOutputManager: Initializing audio output");

  // Restore the saved station, falling back to the first entry if the saved
  // index no longer exists (the list may have been edited since the save)
  String name, url;
  int idx = cc->selectedStation;
  if (!spiffsStorage.getStation(idx, name, url)) {
    idx = 0;
    spiffsStorage.getStation(idx, name, url);
  }
  if (name.length() > 0) {
    _url = url;
    _stationName = name;
    _stationIdx = idx;
  } else {
    _url = "http://mp3.ffh.de/radioffh/hqlivestream.mp3";
    _stationName = "Radio FFH";
    _stationIdx = 0;
  }

  volume = cc->volume;
  _fgain = (volume / 100.0f) * MAX_GAIN;
  _balance = cc->audioBalance;
}

// ************************************************************
// Initiate a radio stream
// ************************************************************
void RadioOutputManager_::startRadioStream(String url, String stationName, float gain, int stationIdx) {
  debugManagerLink("RadioOutputManager: Starting radio stream: " + url + " with gain: " + String(gain));

  // Stop any existing playback first
  if (playing) {
    StopPlaying();
  }

  _url = url;
  _stationName = stationName;
  _stationIdx = stationIdx;
  _fgain = gain;
  _songTitle[0] = '\0';
  StartPlaying();
}

// ************************************************************
// Start playing the stream
// ************************************************************
void RadioOutputManager_::StartPlaying() {
  debugMsgAud("Start play: WiFi=" + String(WiFi.status()) +
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

  if (_url.startsWith("https://")) {
    file = new AudioFileSourceICYStreamSecure(_url.c_str());
  } else {
    file = new AudioFileSourceICYStream(_url.c_str());
  }
  file->RegisterMetadataCB(MDCallback, (void*)"ICY");

  // Allocate streaming buffer — prefer PSRAM, otherwise use up to half of largest
  // contiguous free SRAM block (capped at SRAM_BUFFER_MAX).
  if (psramFound()) {
    audioBuffer = (uint8_t *)ps_malloc(PSRAM_BUFFER_SIZE);
    if (audioBuffer) {
      audioBufferSize = PSRAM_BUFFER_SIZE;
      debugMsgAud("Audio buffer: " + String(PSRAM_BUFFER_SIZE / 1024) + "KB from PSRAM");
    }
  }
  if (!audioBuffer) {
    size_t available = ESP.getMaxAllocHeap();
    audioBufferSize  = min(SRAM_BUFFER_MAX, available / 2);
    audioBuffer      = (uint8_t *)malloc(audioBufferSize);
    if (!audioBuffer) {
      debugMsgAud("Buffer alloc failed (free=" + String(available) + ") - cannot play");
      return;
    }
    debugMsgAud("Audio buffer: " + String(audioBufferSize / 1024) + "KB from SRAM (free was " + String(available / 1024) + "KB)");
  }
  buff = new AudioFileSourceBuffer(file, audioBuffer, audioBufferSize);
  buff->RegisterStatusCB(StatusCallback, (void*)"buffer");

  AudioOutputI2SBalance *i2sOut = new AudioOutputI2SBalance(0, AudioOutputI2S::EXTERNAL_I2S, 8, AudioOutputI2S::APLL_AUTO);
  i2sOut->SetPinout(I2S_BCLK, I2S_LRC, I2S_DOUT);
  i2sOut->SetBalance(_balance);
  out = i2sOut;
  out->SetGain(_fgain);
  mp3 = new AudioGeneratorMP3();
  mp3->RegisterStatusCB(StatusCallback, (void*)"mp3");
  mp3->begin(buff, out);

  _streamsPlayed++;
  playing = true;
  audioTaskRunning = true;

  debugMsgAud("Free heap before task create: " + String(ESP.getFreeHeap()) + " bytes");

  // Try to run the decoder as a pinned task. If the task stack can't be
  // allocated (e.g. under heap pressure), fall back to calling mp3->loop()
  // inline from audioOncePerLoop().
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
// Ask for playback to start once WiFi is up. WiFi association takes a few
// seconds, so a boot-time StartPlaying() would simply fail - audioOncePerLoop()
// retries until WiFi arrives or the wait is given up on.
// ************************************************************
void RadioOutputManager_::requestAutoPlay() {
  autoPlayPending = true;
  autoPlayGiveUpAt = millis() + AUTOPLAY_WIFI_WAIT_MS;
  debugMsgAud("Auto-play requested - waiting for WiFi");
}

// ************************************************************
// Stop playing the stream
// ************************************************************
void RadioOutputManager_::StopPlaying() {
  debugMsgAud("Stop play");

  // Any stop cancels a boot auto-play that hasn't fired yet
  autoPlayPending = false;

  // Cancel any pending reconnect - this is an intentional stop
  streamFailed = false;
  reconnecting = false;
  reconnectAt = 0;

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
    // mp3->stop() above already called out->stop(), which uninstalls the I2S driver.
    delete out;
    out = NULL;
  }
  if (audioBuffer) {
    free(audioBuffer);
    audioBuffer = NULL;
  }

  playing = false;
}

// ************************************************************
// Set volume (0-100)
// ************************************************************
void RadioOutputManager_::setVolume(int vol) {
  _fgain = (vol / 100.0f) * MAX_GAIN;
  if (out) {
    out->SetGain(_fgain);
  }
}

// ************************************************************
// Set stereo balance (-100 = full left, 0 = centre, +100 = full right)
// ************************************************************
void RadioOutputManager_::setBalance(int balance) {
  if (balance < -MAX_AUDIO_BALANCE) balance = -MAX_AUDIO_BALANCE;
  if (balance >  MAX_AUDIO_BALANCE) balance =  MAX_AUDIO_BALANCE;
  _balance = balance;
  if (out) {
    static_cast<AudioOutputI2SBalance *>(out)->SetBalance(_balance);
  }
}

// ************************************************************
// Report the streaming buffer fill level once per second
// ************************************************************
void RadioOutputManager_::audioOncePerSecond() {
  // buff only exists while a stream is set up.
  if (!buff) return;
  debugMsgAud("Buffer " + String(buff->getFillLevel()) + "/" + String(audioBufferSize));
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
  // Boot auto-play: wait for WiFi, then start the restored station
  if (autoPlayPending && !playing) {
    if (WiFi.status() == WL_CONNECTED) {
      autoPlayPending = false;
      debugMsgAud("Auto-play: WiFi up, starting " + _stationName);
      StartPlaying();
    } else if (millis() >= autoPlayGiveUpAt) {
      autoPlayPending = false;
      debugMsgAud("Auto-play: gave up waiting for WiFi");
    }
  }

  // Run the decoder inline when no task could be created.
  // Core 1 WDT is disabled so brief blocking on network I/O is safe.
  if (audioInlineMode && playing && mp3) {
    if (mp3->loop()) {
      _framesDecoded++;
    } else {
      debugMsgAud("Stream ended (inline)");
      streamFailed = true;
      playing = false;
      audioTaskRunning = false;
      audioInlineMode = false;
    }
  }

  // Check if the audio task flagged stream end - clean up from main loop context
  if (!audioTaskRunning && !playing && (mp3 || buff || file || out)) {
    debugMsgAud("Cleaning up after stream end");
    bool wasStreamFailed = streamFailed;  // save before StopPlaying() clears it
    StopPlaying();
    if (wasStreamFailed) {
      reconnecting = true;
      reconnectAt = millis() + RECONNECT_DELAY_MS;
      debugMsgAud("Stream failed - reconnect in " + String(RECONNECT_DELAY_MS / 1000) + "s");
      menuSystem.showFlashMessage("Resyncing...");
    }
  }

  // Handle scheduled reconnect after a stream failure
  if (reconnecting && !playing) {
    if (WiFi.status() != WL_CONNECTED) {
      reconnectAt = millis() + RECONNECT_WIFI_WAIT_MS;
    } else if (millis() >= reconnectAt) {
      debugMsgAud("Attempting stream reconnect");
      reconnecting = false;
      StartPlaying();
    }
  }
}

// ************************************************************
// Dedicated audio task - runs on core 0
// ************************************************************
void RadioOutputManager_::audioTask(void *param) {
  RadioOutputManager_ *self = static_cast<RadioOutputManager_ *>(param);

  debugMsgAud("Audio task started on core " + String(xPortGetCoreID()));

  while (self->audioTaskRunning) {
    if (self->playing && self->mp3) {
      if (self->mp3->loop()) {
        self->_framesDecoded++;
      } else {
        debugMsgAud("Stream ended - stopping playback");
        self->streamFailed = true;
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
// Library internal singleton wiring
// ************************************************************
RadioOutputManager_ &RadioOutputManager_::getInstance() {
  static RadioOutputManager_ instance;
  return instance;
}

RadioOutputManager_ &radioOutputManager = radioOutputManager.getInstance();