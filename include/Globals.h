#pragma once

#include <ESPAsyncWebServer.h>
#include "WiFi.h"
#include "Defs.h"
#include "esp_wps.h"

#include "SpiffsStorage.h"          // Access to config objects
#include "StorageTypes.h"           // Config and Stats objects

// ************************************************************
// Global shared components and objects
// ************************************************************
extern esp_wps_config_t wps_config;

extern unsigned long nowMillis;
extern unsigned long lastSecondStartMillis;
extern unsigned long previousMillisWiFi;
extern int lastSecond;
extern boolean triggeredThisSec;
extern int secsDeltaAbs;
extern int secsDelta;
extern bool upOrDown;  

extern bool blanked;  

// ************************************************************
// Shared config objects
// ************************************************************
extern spiffs_config_t* cc;
extern spiffs_stats_t* cs;

// ************************************************************
// Display values
// ************************************************************
extern portMUX_TYPE timerMux1;
extern portMUX_TYPE encoderMux;

  
extern float n;
extern int frame;

// ************************************************************
// General values
// ************************************************************
extern bool doAutoReconnect;

extern bool encoderToggle;

extern String uniqHostname;

extern AsyncWebServer server;

extern String lastWiFiScan;

extern unsigned long btn1ReadMillis;
extern unsigned long btn2ReadMillis;

extern bool lastExpired;

// ------------------ Global functions -----------------

void updateNowMillis();
