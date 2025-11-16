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

const uint32_t DECODE_DIGIT_ZIN70[] = { 0x0200, 0x0001, 0x0002, 0x0004, 0x0008, 0x0010, 0x0020, 0x0040, 0x0080, 0x0100, 0x0400};

const uint32_t DECODE_DIGIT_ZIN18[] = { 0x0200, 0x0001, 0x0002, 0x0004, 0x0008, 0x0010, 0x0020, 0x0040, 0x0080, 0x0100, 0x0000};

// Formatting routines
String timeToReadableStringFromTm(tm timeToFormat);
uint32_t decodeBCD(byte valueToDecode);

String getValueAtIndex(String data, char separator, int index);
int getValueCount(String data, char separator);

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

void startCounterHandler(AsyncWebServerRequest *request);
void stopCounterHandler(AsyncWebServerRequest *request);
void resetCounterHandler(AsyncWebServerRequest *request);
void pauseCounterHandler(AsyncWebServerRequest *request);
void counterStatusHandler(AsyncWebServerRequest *request);

void restartHandler(AsyncWebServerRequest *request);

void postValueHandler(AsyncWebServerRequest *request);

void wifiBeginWithCredentials();
bool gotCredentials();

void resetWiFi();
void resetOptions();
void resetAll();

void enableWatchdog();
void disableWatchdog();
void feedWatchdog();

void playTune();
