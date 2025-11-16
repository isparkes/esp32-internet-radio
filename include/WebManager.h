#pragma once

#include <memory>
#include <ESPAsyncWebServer.h>
#include "Globals.h"
#include "utilities.h"
#include "DebugManager.h"
#include "DNSServer.h"

class WebManager_ {
  private:
    WebManager_() = default; // Make constructor private

  public:
    static WebManager_ &getInstance(); // Accessor for singleton instance

    WebManager_(const WebManager_ &) = delete; // no copying
    WebManager_ &operator=(const WebManager_ &) = delete;
    
    // Turn off or on logging
    void setDebugOutput(bool newDebug);
  public:
    void begin();
    void beginPortal();
    void startOTA();
  private:
};

extern AsyncWebServer server;

extern WebManager_ &webManager;