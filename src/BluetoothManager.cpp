#include "BluetoothManager.h"

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
    if (manager) {
      manager->bluetoothConnected = true;
    }
  } else if (state == ESP_A2D_CONNECTION_STATE_DISCONNECTED) {
    debugManagerLink(FPSTR(BT_DISCONNECTED));
    if (manager) {
      manager->bluetoothConnected = false;
      manager->connectedDevice = "";
    }
  }
}

// ************************************************************
// Library internal singleton wiring
// ************************************************************
BluetoothManager_ &BluetoothManager_::getInstance() {
  static BluetoothManager_ instance;
  return instance;
}

BluetoothManager_ &bluetoothManager = bluetoothManager.getInstance();
