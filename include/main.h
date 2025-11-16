#pragma once

#include <Arduino.h>
#include <ArduinoJson.h>
#include <WiFi.h>
#include "OLEDAnimations.h"
#include "Defs.h"
#include "DebugManager.h"
#include "RadioOutputManager.h"


// void initwifi();
// void StartPlaying();
// void StopPlaying();
// void MDCallback(void *cbData, const char *type, bool isUnicode, const char *string);
// void StatusCallback(void *cbData, int code, const char *string);

void performOncePerLoopProcessing();
void performOncePerSecondProcessing();
void performOncePerMinuteProcessing();
void performOncePerHourProcessing();
void performOncePerDayProcessing();

