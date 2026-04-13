#include "WebManager.h"
#include <ArduinoOTA.h>
#include <Update.h>

// ************************************************************
// Open up the normal page handlers
// ************************************************************
void WebManager_::begin() {
  debugMsgWbm("Setting up server endpoints");
  server.reset();
  server.serveStatic("/", SPIFFS, "/web/").setDefaultFile("index.html");

  // Summary and diagnostics
  server.on("/api/getSummary", HTTP_GET, getSummaryDataHandler);
  server.on("/api/getDiags", HTTP_GET, getDiagsDataHandler);

 // Configure options
  server.on("/api/getConfig", HTTP_GET, getConfigDataHandler);
  server.on("/api/postConfig", HTTP_POST, postConfigDataHandler);

  // wifi credentials
  server.on("/api/postWiFiCredentials", HTTP_POST, postWiFiCredentialsHandler);
  server.on("/api/credentials", HTTP_GET, getCredentialsHandler);

  // Utilities
  server.on("/utils/resetwifi", HTTP_GET, resetWifiHandler);
  server.on("/utils/scanI2C", HTTP_GET, getI2CScanHandler);
  server.on("/utils/scanSPIFFS", HTTP_GET, getSPIFFSScanHandler);
  server.on("/utils/saveStats", HTTP_GET, saveStatsHandler);
  server.on("/utils/resetoptions", HTTP_GET, [] (AsyncWebServerRequest *request) {
    resetOptions();
        request->redirect("/utility.html");;
    });
  server.on("/utils/resetall", HTTP_GET, [] (AsyncWebServerRequest *request) {
    resetAll();
        request->redirect("/utility.html");;
    });
  server.on("/utils/restart", HTTP_GET, restartHandler);

  // OTA web update
  server.on("/update", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(SPIFFS, "/web/update.html", "text/html");
  });
  server.on("/update", HTTP_POST,
    [](AsyncWebServerRequest *request) {
      bool ok = !Update.hasError();
      AsyncWebServerResponse *resp = request->beginResponse(200, "text/plain", ok ? "OK" : "FAIL");
      resp->addHeader("Connection", "close");
      request->send(resp);
      if (ok) {
        delay(100);
        ESP.restart();
      }
    },
    [](AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final) {
      if (!index) {
        debugMsgWbm("OTA upload start: " + filename);
        if (!Update.begin(UPDATE_SIZE_UNKNOWN)) {
          debugMsgWbm("OTA begin failed");
        }
      }
      if (Update.write(data, len) != len) {
        debugMsgWbm("OTA write error");
      }
      if (final) {
        if (Update.end(true)) {
          debugMsgWbm("OTA upload complete: " + String(index + len) + " bytes");
        } else {
          debugMsgWbm("OTA end failed");
        }
      }
    }
  );

  // OTA filesystem update
  server.on("/updatefs", HTTP_POST,
    [](AsyncWebServerRequest *request) {
      bool ok = !Update.hasError();
      AsyncWebServerResponse *resp = request->beginResponse(200, "text/plain", ok ? "OK" : "FAIL");
      resp->addHeader("Connection", "close");
      request->send(resp);
      if (ok) {
        delay(100);
        ESP.restart();
      }
    },
    [](AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final) {
      if (!index) {
        debugMsgWbm("OTA FS upload start: " + filename);
        if (!Update.begin(UPDATE_SIZE_UNKNOWN, U_SPIFFS)) {
          debugMsgWbm("OTA FS begin failed");
        }
      }
      if (Update.write(data, len) != len) {
        debugMsgWbm("OTA FS write error");
      }
      if (final) {
        if (Update.end(true)) {
          debugMsgWbm("OTA FS upload complete: " + String(index + len) + " bytes");
        } else {
          debugMsgWbm("OTA FS end failed");
        }
      }
    }
  );

  // Radio web interface
  server.on("/radio", HTTP_GET, radioPageHandler);
  server.on("/api/stations/delete", HTTP_POST, deleteStationHandler);
  server.on("/api/stations", HTTP_GET, getStationsHandler);
  server.on("/api/stations", HTTP_POST, postStationHandler);
  server.on("/api/status", HTTP_GET, getStatusHandler);
  server.on("/api/play", HTTP_POST, postPlayHandler);
  server.on("/api/stop", HTTP_POST, postStopHandler);
  server.on("/api/volume", HTTP_POST, postVolumeHandler);

  server.onNotFound([](AsyncWebServerRequest *request){
      request->send(404, "text/plain", "The content you are looking for was not found.");
  });

  debugMsgWbm("Start up web server");

  server.begin();
}

// ************************************************************
// Handler for the captive page
// ************************************************************
class CaptiveRequestHandler : public AsyncWebHandler {
public:
  CaptiveRequestHandler() {}
  virtual ~CaptiveRequestHandler() {}

  bool canHandle(AsyncWebServerRequest *request){
//    debugMsgWbm("Handling URL: " + request->url());
    if (request->url().startsWith("/api/")) return false;
    if (request->url().startsWith("/utils/")) return false;
    return true;
  }

  void handleRequest(AsyncWebServerRequest *request) {
    debugMsgWbm("Sending captive page");
    request->send(SPIFFS, "/web/portal.html", String(), false);
  }
};

// ************************************************************
// Open up the Portal Page
// ************************************************************
void WebManager_::beginPortal() {
  debugMsgWbm("Setting up server endpoints for Portal");
  server.reset();

  // serve the captive page
  server.addHandler(new CaptiveRequestHandler()).setFilter(ON_AP_FILTER);

  // wifi credentials
  server.on("/api/postWiFiCredentials", HTTP_POST, postWiFiCredentialsHandler);
  server.on("/api/credentials", HTTP_GET, getCredentialsHandler);
  server.on("/api/getWiFiNetworks", HTTP_GET, getWiFiNetworksHandler);

  // Utilities
  server.on("/utils/resetwifi", HTTP_GET, resetWifiHandler);
  server.on("/utils/scanI2C", HTTP_GET, getI2CScanHandler);
  server.on("/utils/scanSPIFFS", HTTP_GET, getSPIFFSScanHandler);
  server.on("/utils/saveStats", HTTP_GET, saveStatsHandler);

  // All your DNS requests are belong to us
  wifiManager.startDNSD();

  debugMsgWbm("Start up web server");

  server.begin();
}

// ************************************************************
// Start the OTA service
// ************************************************************
void WebManager_::startOTA() {
  ArduinoOTA.setHostname(uniqHostname.c_str());

  ArduinoOTA.onStart([]() {
    debugMsgWbm("OTA update starting");
  });
  ArduinoOTA.onEnd([]() {
    debugMsgWbm("OTA update complete");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    debugMsgWbm("OTA progress: " + String(progress * 100 / total) + "%");
  });
  ArduinoOTA.onError([](ota_error_t error) {
    debugMsgWbm("OTA error: " + String(error));
  });

  ArduinoOTA.begin();
  debugMsgWbm("OTA ready on " + uniqHostname);
}

// ************************************************************
// Service OTA requests - call from main loop
// ************************************************************
void WebManager_::handleOTA() {
  ArduinoOTA.handle();
}

// ************************************************************
// Get singleton instance
// ************************************************************
WebManager_ &WebManager_::getInstance() {
  static WebManager_ instance;
  return instance;
}

WebManager_ &webManager = webManager.getInstance();