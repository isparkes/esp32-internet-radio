// ************************************************************
// Global types used to hold the config and statistics
// ************************************************************

#pragma once

#include <Arduino.h>

// ------------------------ Types ------------------------

// Used for holding the config set
typedef struct {
  String WiFiSSID;
  String WiFiPassword;
  bool WifiOnAtStart;
  int oledBlankMins;            // OLED blanking timeout in minutes, 0 = never blank
  int audioBalance;             // Stereo balance, -100 = full left, 0 = centre, +100 = full right
  int volume;                   // Playback volume 0-100, restored at startup
  int selectedStation;          // Index into stations.json, restored at startup
  bool playOnStart;             // Start playing the saved station once WiFi is up
} spiffs_config_t;

typedef struct {
  unsigned long uptimeMins = 0;
  unsigned long playtimeMins = 0;
} spiffs_stats_t;

