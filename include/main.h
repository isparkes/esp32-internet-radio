#pragma once

#include <Arduino.h>
#include <ArduinoJson.h>
#include <WiFi.h>

void initwifi();
void StartPlaying();
void StopPlaying();
void MDCallback(void *cbData, const char *type, bool isUnicode, const char *string);
void StatusCallback(void *cbData, int code, const char *string);
