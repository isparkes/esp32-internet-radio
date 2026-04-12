#include "BluetoothManager.h"
#include "RadioMenuConfiguration.h"

#ifdef FEATURE_BLUETOOTH

// Static PCM ring buffer members
BluetoothManager_::PcmFrame* BluetoothManager_::pcmBuffer = nullptr;
volatile uint32_t BluetoothManager_::pcmWriteIdx = 0;
volatile uint32_t BluetoothManager_::pcmReadIdx = 0;
volatile bool BluetoothManager_::btSourceCallbackFired = false;

// Store strings in flash memory to save RAM
static const char BT_INIT[] PROGMEM = "BluetoothManager: Initializing Bluetooth A2DP Sink";
static const char BT_START[] PROGMEM = "BluetoothManager: Starting Bluetooth";
static const char BT_STARTED[] PROGMEM = "BluetoothManager: Bluetooth started - Device name: InternetRadio";
static const char BT_STOP[] PROGMEM = "BluetoothManager: Stopping Bluetooth";
static const char BT_STOPPED[] PROGMEM = "BluetoothManager: Bluetooth stopped";
static const char BT_CONNECTED[] PROGMEM = "BluetoothManager: Device connected";
static const char BT_DISCONNECTED[] PROGMEM = "BluetoothManager: Device disconnected";

// ************************************************************
// Initialize Bluetooth A2DP Sink
// ************************************************************
void BluetoothManager_::initializeBluetooth() {
  debugManagerLink(FPSTR(BT_INIT));
  // Don't allocate until actually started to save memory
}

// ************************************************************
// Start Bluetooth and make device discoverable
// ************************************************************
void BluetoothManager_::startBluetooth() {
  debugManagerLink(FPSTR(BT_START));

  // Allocate only when starting
  if (!a2dp_sink) {
    a2dp_sink = new BluetoothA2DPSink();
  }

  // Configure I2S pins for A2DP
  i2s_pin_config_t pin_config = {
    .bck_io_num = I2S_BCLK,
    .ws_io_num = I2S_LRC,
    .data_out_num = I2S_DOUT,
    .data_in_num = I2S_PIN_NO_CHANGE
  };

  // Set device name and start Bluetooth
  a2dp_sink->set_pin_config(pin_config);
  a2dp_sink->set_avrc_metadata_callback(avrc_metadata_callback);
  a2dp_sink->set_on_connection_state_changed(connection_state_callback, this);

  // Start the Bluetooth A2DP Sink with device name
  a2dp_sink->start("InternetRadio");

  bluetoothActive = true;
  debugManagerLink(FPSTR(BT_STARTED));
  menuSystem.showFlashMessage("Bluetooth started");
}

// ************************************************************
// Stop Bluetooth
// ************************************************************
void BluetoothManager_::stopBluetooth() {
  debugManagerLink(FPSTR(BT_STOP));

  if (a2dp_sink) {
    a2dp_sink->end(true);
    delete a2dp_sink;
    a2dp_sink = nullptr;
  }

  bluetoothActive = false;
  bluetoothConnected = false;
  connectedDevice = "";

  debugManagerLink(FPSTR(BT_STOPPED));
  menuSystem.showFlashMessage("Bluetooth stopped");
}

// ************************************************************
// Check if Bluetooth is active
// ************************************************************
bool BluetoothManager_::isBluetoothActive() {
  return bluetoothActive;
}

// ************************************************************
// Check if a device is connected
// ************************************************************
bool BluetoothManager_::isBluetoothConnected() {
  if (a2dp_sink && bluetoothActive) {
    return a2dp_sink->is_connected();
  }
  return false;
}

// ************************************************************
// Get connected device name
// ************************************************************
String BluetoothManager_::getConnectedDeviceName() {
  return connectedDevice;
}

// ************************************************************
// Set volume (0-255)
// ************************************************************
void BluetoothManager_::setVolume(uint8_t volume) {
  if (a2dp_sink && bluetoothActive) {
    a2dp_sink->set_volume(volume);
  }
}

// ************************************************************
// Metadata callback - called when track info is received
// ************************************************************
void BluetoothManager_::avrc_metadata_callback(uint8_t id, const uint8_t *text) {
  // Metadata callbacks disabled to save memory
  // Re-enable if you need track info display
}

// ************************************************************
// Connection state callback
// ************************************************************
void BluetoothManager_::connection_state_callback(esp_a2d_connection_state_t state, void *ptr) {
  BluetoothManager_ *manager = static_cast<BluetoothManager_*>(ptr);

  if (state == ESP_A2D_CONNECTION_STATE_CONNECTED) {
    debugManagerLink(FPSTR(BT_CONNECTED));
    menuSystem.showFlashMessage("BT device connected");
    if (manager) {
      manager->bluetoothConnected = true;
    }
  } else if (state == ESP_A2D_CONNECTION_STATE_DISCONNECTED) {
    debugManagerLink(FPSTR(BT_DISCONNECTED));
    menuSystem.showFlashMessage("BT device disconnected");
    if (manager) {
      manager->bluetoothConnected = false;
      manager->connectedDevice = "";
    }
  }
}

// ************************************************************
// Start Bluetooth as A2DP Source (stream radio to BT speaker)
// ************************************************************
void BluetoothManager_::startBluetoothSource(const char* speakerName) {
  debugManagerLink("BluetoothManager: Starting A2DP Source → " + String(speakerName));

  // Allocate PCM ring buffer in PSRAM if available
  if (!pcmBuffer) {
    if (psramFound()) {
      pcmBuffer = (PcmFrame*)ps_malloc(BT_PCM_BUFFER_FRAMES * sizeof(PcmFrame));
    }
    if (!pcmBuffer) {
      pcmBuffer = (PcmFrame*)malloc(BT_PCM_BUFFER_FRAMES * sizeof(PcmFrame));
    }
  }
  pcmWriteIdx = 0;
  pcmReadIdx = 0;
  btSourceCallbackFired = false;

  if (!a2dp_source) {
    a2dp_source = new BluetoothA2DPSource();
  }
  a2dp_source->start(speakerName, sourceDataCallback);
  bluetoothSourceActive = true;
  menuSystem.showFlashMessage("BT Source started");
}

// ************************************************************
// Stop Bluetooth A2DP Source
// ************************************************************
void BluetoothManager_::stopBluetoothSource() {
  debugManagerLink("BluetoothManager: Stopping A2DP Source");

  if (a2dp_source) {
    a2dp_source->end(true);
    delete a2dp_source;
    a2dp_source = nullptr;
  }
  if (pcmBuffer) {
    free(pcmBuffer);
    pcmBuffer = nullptr;
  }
  btSourceCallbackFired = false;
  bluetoothSourceActive = false;
  menuSystem.showFlashMessage("BT Source stopped");
}

// ************************************************************
// Write a decoded PCM frame into the ring buffer (called from audio task)
// ************************************************************
bool BluetoothManager_::writePcmFrame(int16_t left, int16_t right) {
  if (!pcmBuffer) return false;
  uint32_t nextWrite = (pcmWriteIdx + 1) & (BT_PCM_BUFFER_FRAMES - 1);
  if (nextWrite == pcmReadIdx) return false;  // buffer full, drop frame
  pcmBuffer[pcmWriteIdx] = {left, right};
  pcmWriteIdx = nextWrite;
  return true;
}

// ************************************************************
// A2DP source callback — feeds PCM to the BT stack
// ************************************************************
int32_t BluetoothManager_::sourceDataCallback(Frame *frame, int32_t frame_count) {
  // First callback call means the BT audio channel is fully established.
  btSourceCallbackFired = true;

  if (!pcmBuffer) {
    memset(frame, 0, frame_count * sizeof(Frame));
    return frame_count;
  }
  for (int32_t i = 0; i < frame_count; i++) {
    if (pcmReadIdx == pcmWriteIdx) {
      // Buffer underrun — fill remainder with silence
      memset(&frame[i], 0, (frame_count - i) * sizeof(Frame));
      break;
    }
    frame[i].channel1 = pcmBuffer[pcmReadIdx].left;
    frame[i].channel2 = pcmBuffer[pcmReadIdx].right;
    pcmReadIdx = (pcmReadIdx + 1) & (BT_PCM_BUFFER_FRAMES - 1);
  }
  return frame_count;
}

bool BluetoothManager_::isBluetoothSourceActive() {
  return bluetoothSourceActive;
}

bool BluetoothManager_::isBluetoothSourceConnected() {
  if (a2dp_source && bluetoothSourceActive) {
    return a2dp_source->is_connected();
  }
  return false;
}

bool BluetoothManager_::isBluetoothSourceAudioStarted() {
  return btSourceCallbackFired;
}

#else
// ************************************************************
// Stub implementations for platforms without Classic Bluetooth
// (ESP32-S3, ESP32-C3, etc.)
// ************************************************************
void BluetoothManager_::initializeBluetooth() {}
void BluetoothManager_::startBluetooth() {}
void BluetoothManager_::stopBluetooth() {}
bool BluetoothManager_::isBluetoothActive() { return false; }
bool BluetoothManager_::isBluetoothConnected() { return false; }
String BluetoothManager_::getConnectedDeviceName() { return ""; }
void BluetoothManager_::setVolume(uint8_t volume) {}
void BluetoothManager_::startBluetoothSource(const char* speakerName) {}
void BluetoothManager_::stopBluetoothSource() {}
bool BluetoothManager_::isBluetoothSourceActive() { return false; }
bool BluetoothManager_::isBluetoothSourceConnected() { return false; }
bool BluetoothManager_::isBluetoothSourceAudioStarted() { return false; }
bool BluetoothManager_::writePcmFrame(int16_t left, int16_t right) { return false; }

#endif

// ************************************************************
// Library internal singleton wiring
// ************************************************************
BluetoothManager_ &BluetoothManager_::getInstance() {
  static BluetoothManager_ instance;
  return instance;
}

BluetoothManager_ &bluetoothManager = bluetoothManager.getInstance();
