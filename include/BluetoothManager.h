#pragma once

#include <Arduino.h>
#include "Defs.h"
#include "DebugManager.h"

#ifdef FEATURE_BLUETOOTH
#include "BluetoothA2DPSink.h"
#include "BluetoothA2DPSource.h"
#include "AudioOutputI2S.h"

static const uint32_t BT_PCM_BUFFER_FRAMES = 16384;  // ~370ms at 44100Hz stereo
#endif

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

    // A2DP Source (radio → BT speaker)
    void startBluetoothSource(const char* speakerName);
    void stopBluetoothSource();
    bool isBluetoothSourceActive();
    bool isBluetoothSourceConnected();
    bool isBluetoothSourceAudioStarted();
    static bool writePcmFrame(int16_t left, int16_t right);

  private:
#ifdef FEATURE_BLUETOOTH
    BluetoothA2DPSink *a2dp_sink = nullptr;
    bool bluetoothActive = false;
    bool bluetoothConnected = false;
    String connectedDevice = "";

    BluetoothA2DPSource *a2dp_source = nullptr;
    bool bluetoothSourceActive = false;

    struct PcmFrame { int16_t left; int16_t right; };
    static PcmFrame *pcmBuffer;
    static volatile uint32_t pcmWriteIdx;
    static volatile uint32_t pcmReadIdx;
    static volatile bool btSourceCallbackFired;  // true once BT stack starts requesting audio

    static void avrc_metadata_callback(uint8_t id, const uint8_t *text);
    static void connection_state_callback(esp_a2d_connection_state_t state, void *ptr);
    static int32_t sourceDataCallback(Frame *frame, int32_t frame_count);
#endif
};

void debugManagerLink(String message);

extern BluetoothManager_ &bluetoothManager;
