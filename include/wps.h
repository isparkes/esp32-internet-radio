#pragma once

#include "Defs.h"
#include "Globals.h"

/*
  Change the definition of the WPS mode
  from WPS_TYPE_PBC to WPS_TYPE_PIN in
  the case that you are using pin type
  WPS
*/
#define ESP_WPS_MODE WPS_TYPE_PBC
#define ESP_MANUFACTURER "ESPRESSIF"
#define ESP_MODEL_NUMBER "ESP32"
#define ESP_MODEL_NAME "ESPRESSIF IOT"
#define ESP_DEVICE_NAME "ESP STATION"

// ************************************************************
// Public prototypes
// ************************************************************

void wpsInitConfig();
String wpspin2string(uint8_t a[]);