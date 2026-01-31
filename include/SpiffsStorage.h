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

    // These load/store the global objects, defined in globals.h
    bool getConfigFromSpiffs();
    void saveConfigToSpiffs();
    bool getStatsFromSpiffs();
    void saveStatsToSpiffs();
    bool getStationsFromSpiffs();
    void saveStationsToSpiffs();

    JsonObject& getConfigAsJsonObject();
  private:
    bool _spiffsMounted = false;
    DynamicJsonBuffer _jsonBuffer;
};

extern SpiffsStorage_ &spiffsStorage;