#pragma once

#include <Arduino.h>
#include "ESP32MenuSystem.h"
#include "Defs.h"
#include "Globals.h"
#include "DebugManager.h"
#include "RadioOutputManager.h"
#include "BluetoothManager.h"
#include "WiFiManager.h"
#include "SpiffsStorage.h"
#include "utilities.h"

extern MenuSystem menuSystem;
extern StatusData radioStatus;
extern MenuItem* mainMenu;
extern MenuItem* audioMenu;
extern MenuItem* wifiMenu;
extern MenuItem* systemMenu;

// Menu building
void buildRadioMenus();
void buildAudioMenuDynamic();
void buildWifiMenuDynamic();
void buildSystemMenuDynamic();

// Menu loop functions
void menuOncePerLoop();
void menuOncePerSecond();

// Status screen
void renderRadioStatus(Adafruit_SH1106G* display, uint8_t width, uint8_t height);
bool handleStatusInput(ButtonEvent event);
void handleStatusEncoder(int delta);
