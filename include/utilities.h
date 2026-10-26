#pragma once

#include <Arduino.h>
#include <ESPAsyncWebServer.h>

#include <TimeLib.h>
#include <rom/rtc.h>

#include "Defs.h"
#include "Globals.h"

#include "AsyncJson.h"
#include "ArduinoJson.h"

#include <Wire.h>
#include "SpiffsStorage.h"
#include "TimerManager.h"
#include "Globals.h"
#include "DebugManager.h"
#include <esp_task_wdt.h>
#include "WiFiManager.h"

// -------------------------------------------------------------------------------

// Web handlers
void getCredentialsHandler(AsyncWebServerRequest *request);
void getWiFiNetworksHandler(AsyncWebServerRequest *request);
void saveWiFiCredentials(String newWiFiSSID, String newWiFiPassword);

void getSummaryDataHandler(AsyncWebServerRequest *request);

void getConfigDataHandler(AsyncWebServerRequest *request);
void postConfigDataHandler(AsyncWebServerRequest *request);

void getDiagsDataHandler(AsyncWebServerRequest *request);

void postWiFiCredentialsHandler(AsyncWebServerRequest *request);
void resetWifiHandler(AsyncWebServerRequest *request);

void getI2CScanHandler(AsyncWebServerRequest *request);
void getSPIFFSScanHandler(AsyncWebServerRequest *request);
void saveStatsHandler(AsyncWebServerRequest *request);

void restartHandler(AsyncWebServerRequest *request);

void wifiBeginWithCredentials();

void resetWiFi();
void resetOptions();
void resetAll();

// Firmware image sizes - measured once, see utilities.cpp
uint32_t getSketchSizeCached();
uint32_t getAppPartitionSize();

void enableWatchdog();
void disableWatchdog();
void feedWatchdog();

// Radio web interface handlers
void radioPageHandler(AsyncWebServerRequest *request);
void getStationsHandler(AsyncWebServerRequest *request);
void postStationHandler(AsyncWebServerRequest *request);
void deleteStationHandler(AsyncWebServerRequest *request);
void getStatusHandler(AsyncWebServerRequest *request);
void postPlayHandler(AsyncWebServerRequest *request);
void postStopHandler(AsyncWebServerRequest *request);
void postVolumeHandler(AsyncWebServerRequest *request);
