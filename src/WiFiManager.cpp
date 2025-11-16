#include "WiFiManager.h"

// ************************************************************
// Utility: Set up WPS
// ************************************************************
void wpsInitConfig()
{
  wps_config.wps_type = ESP_WPS_MODE;
  strcpy(wps_config.factory_info.manufacturer, ESP_MANUFACTURER);
  strcpy(wps_config.factory_info.model_number, ESP_MODEL_NUMBER);
  strcpy(wps_config.factory_info.model_name, ESP_MODEL_NAME);
  strcpy(wps_config.factory_info.device_name, ESP_DEVICE_NAME);
}

// ************************************************************
// Conditionally send a message to the OLED
// ************************************************************
void flashMenuEvent(String titleTest, String flashText) {
    #ifdef FEATURE_MENU
    menuManager.flashMenuMessage(titleTest, flashText);
    #endif
}

// ************************************************************
// WiFi event handler
// ************************************************************
void WiFiEvent(WiFiEvent_t event, arduino_event_info_t info)
{
  switch (event)
  {
  case WiFiEvent_t::ARDUINO_EVENT_WIFI_STA_START:
    debugMsgWfm("Station Mode Started");
    break;
  case ARDUINO_EVENT_WIFI_AP_START:
    debugMsgWfm("AP Mode Started");
    wifiManager.startWiFiServicesPortal();
    break;
  case ARDUINO_EVENT_WIFI_STA_GOT_IP:
    debugMsgWfm("Connected to:" + WiFi.SSID() + ", password: " + WiFi.psk());
    debugMsgWfm("IP Address: " + WiFi.localIP().toString());
    debugMsgWfm("MAC Address: " + WiFi.macAddress());
    debugMsgWfm("Host name: " + String(WiFi.getHostname()));
    wifiManager.saveWiFiCredentials(WiFi.SSID(), WiFi.psk());
    wifiManager.startWiFiServices();
    flashMenuEvent("WiFi Status", "WiFi connected to\n"+String(WiFi.SSID()));
    break;
  case ARDUINO_EVENT_WIFI_STA_DISCONNECTED:
    debugMsgWfm("Disconnected from station");
    if (doAutoReconnect) {
      debugMsgWfm("autoreconnect on, trying reconnect");
      WiFi.reconnect();
    }
    break;
  case ARDUINO_EVENT_WPS_ER_SUCCESS:
    debugMsgWfm("WPS Successfull, saving credentials. SSID: |" + WiFi.SSID() + "| password: |" + WiFi.psk() + "|");
    wifiManager.saveWiFiCredentials(WiFi.SSID(), WiFi.psk());
    esp_wifi_wps_disable();
    flashMenuEvent("WPS Status", "WPS was successful\nPassword:\n"+WiFi.psk());
    break;
  case ARDUINO_EVENT_WPS_ER_FAILED:
    debugMsgWfm("WPS Failed, retrying");
    esp_wifi_wps_disable();
    esp_wifi_wps_enable(&wps_config);
    esp_wifi_wps_start(0);
    break;
  case ARDUINO_EVENT_WPS_ER_TIMEOUT:
    debugMsgWfm("WPS Timedout, retrying");
    esp_wifi_wps_disable();
    esp_wifi_wps_enable(&wps_config);
    esp_wifi_wps_start(0);
    break;
  case ARDUINO_EVENT_WIFI_SCAN_DONE:
    debugMsgWfm("Scan complete");
    wifiManager.processScanResults();
    break;
  case ARDUINO_EVENT_WIFI_READY:
    debugMsgWfm("WiFi ready");
    break;
//  case ARDUINO_EVENT_WIFI_AP_STACONNECTED:
//    debugMsgWfm("AP_STA Connected to station");
//    break;
//  case ARDUINO_EVENT_WIFI_AP_STAIPASSIGNED:
//    debugMsgWfm("AP_STA Connected to station");
//    debugMsgWfm("IP Address: " + WiFi.localIP().toString());
//    break;
//  case ARDUINO_EVENT_WIFI_AP_STOP:
//    debugMsgWfm("AP stop");
//    break;
  default:
    debugMsgWfm("Wifi event (not mapped): " + String(event));
    break;
  }
}

// ************************************************************
// Set up the WiFi for normal use
// ************************************************************
void WiFiManager_::setUpWiFi() {
  WiFi.onEvent(WiFiEvent);

  String mac = String(WiFi.macAddress());
  mac.replace(":","");
  uniqHostname = "ESP32-"+mac.substring(6);

  debugMsgWfm("Unique hostname: " + uniqHostname);
  WiFi.setHostname(uniqHostname.c_str());
}

// ************************************************************
// Process the results of the SSID scan
// ************************************************************
void WiFiManager_::startScanWiFiNetworks() {
  debugMsgWfm("Start wifi scan");
  WiFi.onEvent(WiFiEvent, ARDUINO_EVENT_SC_SCAN_DONE);

  WiFi.mode(WIFI_AP_STA);
  WiFi.disconnect();
  delay(500);

  WiFi.scanNetworks(true);
}

// ************************************************************
// Process the results of the SSID scan
// ************************************************************
void WiFiManager_::processScanResults() {
  int n = WiFi.scanComplete();
  if (n == 0) {
    debugMsgWfm("no networks found");
    flashMenuEvent("Scan Done", "No WiFi\nnetworks\nfound.");
  } else {
    debugMsgWfm("");
    debugMsgWfm(String(n) + " networks found");
    flashMenuEvent("Scan Done", "Found " + String(n) + " networks.");
  }
}

//     String result = "";
//     for (int i = 0; i < n; ++i) {
//       if (_ssidList.indexOf(WiFi.SSID(i)) > 0) {
//         debugMsgWfm("Already have: " + WiFi.SSID(i));
//       } else {
//         debugMsgWfm("Add: " + WiFi.SSID(i));
//         _ssidList = _ssidList + "," + String(WiFi.SSID(i));
//       }
//       #ifdef WFM_EXTENDED_DEBUG
//       // Print SSID and RSSI for each network found
//       bool encrypted = WiFi.encryptionType(i) == WIFI_AUTH_OPEN;
//       String msg = String(i) + " : " + WiFi.SSID(i) + " (" + WiFi.RSSI(i) + ")";
//       if (encrypted) {
//         msg = msg + " *";
//       }
//       debugMsgWfm(msg);
//       #endif
//       if (result.length() > 0) {
//         result = result + ",";
//       }
//       result = result + WiFi.SSID(i);
//     }
//     #ifdef WFM_EXTENDED_DEBUG
//     debugMsgWfm("Returning network list: " + result);
//     #endif
//     lastWiFiScan = result;
//   }
// }

// ************************************************************
// Get the number of SSIDs we found so far
// ************************************************************
int WiFiManager_::getLastScanResultCount() {
  return WiFi.scanComplete();
}

// ************************************************************
// Get the nth entry in the SSID list
// ************************************************************
String WiFiManager_::getLastScanResultSSID(int index) {
  return String(WiFi.SSID(index));
}

// ************************************************************
// Start up Smart Config - fire and forget
// http://www.iotsharing.com/2017/05/how-to-use-smartconfig-on-esp32.html
// ************************************************************
void WiFiManager_::startSmartConfig() {
  flashMenuEvent("Smartconfig", "Open Smartconfig\napp and enter\nyour information.");
  WiFi.disconnect();
  delay(500);

  WiFi.mode(WIFI_AP_STA);
  WiFi.beginSmartConfig();
}

// ************************************************************
// Reconnect to the last registered SSID
// ************************************************************
void WiFiManager_::connectToLastAP() {
  if(wifiCredentialsReceived()) {
    debugMsgWfm("Trying to reconnect to last known AP: " + String(cc->WiFiSSID) + ":" + String(cc->WiFiPassword));
    flashMenuEvent("Reconnect","Reconnecting to:\n" + cc->WiFiSSID + "\n");
    wifiBeginWithCredentials();
  }
}

// ************************************************************
// Kick off WPS connect - fire and forget
// ************************************************************
bool WiFiManager_::connectWithWPS() {
  // Autoreconnect is needed for WPS!
  doAutoReconnect = true;

  if (WiFi.status() != WL_CONNECTED) {
    debugMsgWfm("Connect using WPS");
    // ToDo show this status better
    flashMenuEvent("WPS", "Press the WPS\nbutton on your\nrouter now.");

    WiFi.mode(WIFI_STA);
    delay(1000);
      
    wpsInitConfig();

    esp_err_t retCodeEnable = esp_wifi_wps_enable(&wps_config);
    debugMsgWfm("WPS Enable Result: " + String(retCodeEnable));

    esp_err_t retCodeStart = esp_wifi_wps_start(0);
    debugMsgWfm("WPS Start Result: " + String(retCodeStart));

    return (retCodeEnable == 0 && retCodeStart == 0);
  } else {
    debugMsgWfm("Already connected, won't do WPS");
    return false;
  }
}

// ************************************************************
// Start up AP mode
// ************************************************************
void WiFiManager_::openAccessPortal() {
  // Captive portal
  if (WiFi.status() != WL_CONNECTED) {
    startScanWiFiNetworks();

    debugMsgWfm("");
    debugMsgWfm("Portal mode");
    WiFi.disconnect();
    delay(100);
    WiFi.mode(WIFI_AP_STA);
    delay(100);
    debugMsgWfm("Setting soft-AP configuration ... ");
    WiFi.softAP(uniqHostname.c_str());
    delay(100);
    debugMsgWfm("Soft-AP IP address: " + WiFi.softAPIP().toString());
    flashMenuEvent("Portal", "Opened access\nportal at IP: " + WiFi.softAPIP().toString());
    _isOpenAP = true;
  } else {
    flashMenuEvent("Portal", "WiFi is already\nconnected to:\n" + WiFi.SSID());
  }
}

// ************************************************************
// Start up mDNS
// ************************************************************
void WiFiManager_::startMDNS() {
  // The MDNS host name does not seem to work at the moment - it is being set by OTA
  if(!MDNS.begin(uniqHostname.c_str())) {
      debugMsgWfm("Error starting mDNS");
      return;
  }

  MDNS.addService("http", "tcp", 80);
}

// ************************************************************
// Start up DNS for captive portal capture
// ************************************************************
void WiFiManager_::startDNSD() {
  dnsServer.reset(new DNSServer());

  /* Setup the DNS server redirecting all the domains to the apIP */
  dnsServer->setErrorReplyCode(DNSReplyCode::NoError);
  debugMsgWfm("dns server started with ip: " + WiFi.softAPIP().toString());
  dnsServer->start(DNS_PORT, F("*"), WiFi.softAPIP());
}

// ************************************************************
// Stop DNS for captive portal capture
// ************************************************************
void WiFiManager_::stopDNSD() {
  dnsServer->stop();
  dnsServer.reset();
}

// ************************************************************
// Reular update to allow DNS process to be managed while in
// Captive Portal mode
// ************************************************************
void WiFiManager_::manageDNSInOpenAP() {
  if (_isOpenAP) {
    dnsServer->processNextRequest();
  }
}

// ************************************************************
// Start up the web server for normal use
// ************************************************************
void WiFiManager_::startWiFiServices() {
  if (WiFi.isConnected()) {
    // -------------------------------------------------------------------------

    debugMsgWfm("Start up WebServer" );
    webManager.begin();

    // -------------------------------------------------------------------------
    
    debugMsgWfm("Start up OTA");
    webManager.startOTA();

    // -------------------------------------------------------------------------
    
    debugMsgWfm("Start up mDNS on http://" + String(WiFi.getHostname()) + ".local");
    startMDNS();
  } else {
    debugMsgWfm("No WiFi, skipping web services startup");
  }
}

// ************************************************************
// Start the web server with Access Portal set up
// ************************************************************
void WiFiManager_::startWiFiServicesPortal() {
  debugMsgWfm("Start up WebServer for Portal services" );

  webManager.beginPortal();
}

// ************************************************************
// Startup th WiFi using the credentials we have
// ************************************************************
void WiFiManager_::wifiBeginWithCredentials() {
  WiFi.disconnect();
  delay(1000);
  WiFi.mode(WIFI_MODE_STA);
  delay(1000);
  WiFi.begin(cc->WiFiSSID.c_str(), cc->WiFiPassword.c_str());
}

// ************************************************************
// Save the credentials to SPIFFS if they are valid
// ************************************************************
void WiFiManager_::saveWiFiCredentials(String newWiFiSSID, String newWiFiPassword) {
  if ((cc->WiFiSSID != newWiFiSSID || 
      cc->WiFiPassword != newWiFiPassword) && 
      newWiFiSSID.length() > 0 &&
      newWiFiPassword.length() > 0) {
    debugMsgWfm("Updating stored WiFi credentials");
    cc->WiFiSSID = newWiFiSSID;
    cc->WiFiPassword = newWiFiPassword;
    cc->WifiOnAtStart = true;
    spiffsStorage.saveConfigToSpiffs();
    debugMsgWfm("Saved WiFi credentials");
  } else {
    debugMsgWfm("No changes to WiFi credentials saved");
  }
}

// ************************************************************
// Undock from the WiFi mothership
// ************************************************************
void WiFiManager_::disconnectWiFi() {
  WiFi.disconnect();
}

// ************************************************************
// Clear down credentials
// ************************************************************
void WiFiManager_::resetWiFiCredentials() {
  cc->WiFiSSID = "";
  cc->WiFiPassword = "";
  cc->WifiOnAtStart = false;
  spiffsStorage.saveConfigToSpiffs();
}

// ************************************************************
// Return true if we have received and stored some credentials
// ************************************************************
bool WiFiManager_::wifiCredentialsReceived() {
  return (cc->WiFiSSID.length() > 0 && cc->WiFiPassword.length() > 0);
}

// ************************************************************
// Library internal singleton wiring
// ************************************************************
WiFiManager_ &WiFiManager_::getInstance() {
  static WiFiManager_ instance;
  return instance;
}

WiFiManager_ &wifiManager = wifiManager.getInstance();