#pragma once

#include <FS.h>
#include <ArduinoJson.h>
#include "SPIFFS.h"
#include "DebugManager.h"
#include "Globals.h"

// ----------------------------------------------------------------------------------------------------
// ------------------------------------- SPIFFS Clock Component ---------------------------------------
// ----------------------------------------------------------------------------------------------------

class SpiffsStorage_
{
  private:
    SpiffsStorage_() = default; // Make constructor private

  public:
    static SpiffsStorage_ &getInstance(); // Accessor for singleton instance

    SpiffsStorage_(const SpiffsStorage_ &) = delete; // no copying
    SpiffsStorage_ &operator=(const SpiffsStorage_ &) = delete;

  public:
    bool testMountSpiffs();
    bool getSpiffsMounted();

    bool getConfigFromSpiffs();
    void saveConfigToSpiffs();
    bool getStatsFromSpiffs();
    void saveStatsToSpiffs();

    // Direct per-operation station access (reads/writes stations.json each call)
    int  getStationCount();
    bool getStation(int idx, String& name, String& url);
    bool appendStation(const String& name, const String& url);
    bool deleteStation(int idx);

    JsonObject& getConfigAsJsonObject();
  private:
    bool _spiffsMounted = false;
    DynamicJsonBuffer _jsonBuffer;
};

extern SpiffsStorage_ &spiffsStorage;