#pragma once

#include <Arduino.h>
#include "Defs.h"
#include "Globals.h"
#include "utilities.h"
#include "wps.h"
#include <ESPmDNS.h>
#include "WebManager.h"
#include <DNSServer.h>
#ifdef FEATURE_MENU
#include "MenuManager.h"
#endif


const byte    DNS_PORT                = 53;

class WiFiManager_ {
  private:
    WiFiManager_() = default; // Make constructor private

  public:
    static WiFiManager_ &getInstance(); // Accessor for singleton instance

    WiFiManager_(const WiFiManager_ &) = delete; // no copying
    WiFiManager_ &operator=(const WiFiManager_ &) = delete;

  public:
    void setUpWiFi();

    void connectToLastAP();
    bool connectWithWPS();
    void openAccessPortal();
    void startSmartConfig();

    void startWiFiServices();
    void startWiFiServicesPortal();
    void resetWiFiCredentials();
    bool wifiCredentialsReceived();
    void disconnectWiFi();

    void startScanWiFiNetworks();
    int getLastScanResultCount();
    String getLastScanResultSSID(int index);
    void wifiBeginWithCredentials();
    void saveWiFiCredentials(String newWiFiSSID, String newWiFiPassword);
    void processScanResults();

    // For captive portal
    void startDNSD();
    void stopDNSD();
    void manageDNSInOpenAP();

  private:
    bool _isOpenAP = false;
    std::unique_ptr<DNSServer>        dnsServer;    

    // For resolving names to esp32xxxxx.local
    void startMDNS();
};

extern WiFiManager_ &wifiManager;