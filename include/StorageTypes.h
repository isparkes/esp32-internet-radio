// ************************************************************
// Global types used to hold the config and statistics
// ************************************************************

#pragma once

#include <Arduino.h>

// ------------------------ Types ------------------------

// Used for holding the config set
typedef struct {
  bool webAuthentication;
  String webUsername;
  String webPassword;
  String WiFiSSID;
  String WiFiPassword;
  bool WifiOnAtStart;

  String counterValuesZIN70;
  String counterValuesZIN18;
  int tubeType;
  int tubeBoardCount;

} spiffs_config_t;

// Station entry for the station list
typedef struct {
  String name;
  String url;
} station_t;

typedef struct {
  unsigned long uptimeMins = 0;
  unsigned long tubeOnTimeMins = 0;
} spiffs_stats_t;

