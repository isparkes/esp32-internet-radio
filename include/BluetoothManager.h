#pragma once

#include <Arduino.h>
#include "BluetoothA2DPSink.h"
#include "AudioOutputI2S.h"
#include "Defs.h"
#include "DebugManager.h"

class BluetoothManager_ {
  private:
    BluetoothManager_() = default;

  public:
    static BluetoothManager_ &getInstance();

    BluetoothManager_(const BluetoothManager_ &) = delete;
    BluetoothManager_ &operator=(const BluetoothManager_ &) = delete;

  public:
    void initializeBluetooth();
    void startBluetooth();
    void stopBluetooth();
    bool isBluetoothActive();
    bool isBluetoothConnected();
    String getConnectedDeviceName();
    void setVolume(uint8_t volume);

  private:
    BluetoothA2DPSink *a2dp_sink = nullptr;
    bool bluetoothActive = false;
    bool bluetoothConnected = false;
    String connectedDevice = "";

    static void avrc_metadata_callback(uint8_t id, const uint8_t *text);
    static void connection_state_callback(esp_a2d_connection_state_t state, void *ptr);
};

void debugManagerLink(String message);

extern BluetoothManager_ &bluetoothManager;
