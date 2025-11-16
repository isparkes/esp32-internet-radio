#include "utilities.h"

// --------------------------------------------------------------------------------------------------------
// ----------------------------------------  Utility functions  -------------------------------------------
// --------------------------------------------------------------------------------------------------------

// ************************************************************
// Format a time into an output string
// ************************************************************
String timeToReadableStringFromTm(tm timeToFormat) {
  char buf1[20];
  sprintf(buf1, "%04d-%02d-%02d %02d:%02d:%02d",
    timeToFormat.tm_year + 1900,
    timeToFormat.tm_mon + 1,
    timeToFormat.tm_mday,
    timeToFormat.tm_hour,
    timeToFormat.tm_min,
    timeToFormat.tm_sec);
  return String(buf1);
}

// ************************************************************
// Format a duration into an output string
// ************************************************************
String secsToReadableString (long secsValue) {
  long upDays = secsValue / 86400;
  long upHours = (secsValue - (upDays * 86400)) / 3600;
  long upMins = (secsValue - (upDays * 86400) - (upHours * 3600)) / 60;
  secsValue = secsValue - (upDays * 86400) - (upHours * 3600) - (upMins * 60);
  String uptimeString = "";
  if (upDays > 0) {
    uptimeString += upDays; 
    uptimeString += " d ";
  }
  if (upHours > 0) {
    uptimeString += upHours;
    uptimeString += " h "; 
  }
  if (upMins > 0) {
    uptimeString += upMins; 
    uptimeString += " m ";
  }
  if (secsValue > 0) {
    uptimeString += secsValue; 
    uptimeString += " s";
  }
  if (uptimeString == "") {
    uptimeString = "0 s";
  }

  return uptimeString;
}

// ************************************************************
// Reset settings to factory defaults
// ************************************************************
void resetOptions() {
  cc->webAuthentication = false;
  cc->webUsername = "";
  cc->webPassword = "";
  cc->WifiOnAtStart = true;
  cc->WiFiSSID = "";
  cc->WiFiPassword = "";
  spiffsStorage.saveConfigToSpiffs();
}

// ************************************************************
// Get the field value at position Index
// ************************************************************
String getValueAtIndex(String data, char separator, int index)
{
  int found = 0;
  int strIndex[] = {0, -1};
  int maxIndex = data.length()-1;

  for(int i=0; i<=maxIndex && found<=index; i++){
    if(data.charAt(i)==separator || i==maxIndex){
        found++;
        strIndex[0] = strIndex[1]+1;
        strIndex[1] = (i == maxIndex) ? i+1 : i;
    }
  }

  return found>index ? data.substring(strIndex[0], strIndex[1]) : "";
}

// ************************************************************
// Get the number of fields
// ************************************************************
int getValueCount(String data, char separator)
{
  int found = 0;
  int strIndex[] = {0, -1};
  int maxIndex = data.length()-1;

  for(int i=0; i<=maxIndex ; i++){
    if(data.charAt(i)==separator || i==maxIndex){
        found++;
    }
  }

  return found;
}

//**********************************************************************************
//**********************************************************************************
//*                                  Web Handlers                                  *
//**********************************************************************************
//**********************************************************************************

// ************************************************************
// Debug server args
// ************************************************************
#ifdef DEBUG
void dumpArgs(AsyncWebServerRequest *request) {
  int headers = request->headers();
  int i;
  for(i=0;i<headers;i++){
    const AsyncWebHeader* h = request->getHeader(i);
    String message = "HEADER[" + h->name() + ":" + h->value();
    debugMsgUtl(message);
  }

  if (request->hasArg("body")) {
    debugMsgUtl("Body found arg");
  }
  if (request->hasParam("body")) {
    debugMsgUtl("Body found param");
  }
  int args = request->args();
  for(int i=0;i<args;i++){
    String message = "ARG[" + request->argName(i) + "]: " + request->arg(i); 
    debugMsgUtl(message);
  }  
}
#endif


// ************************************************************
// Main page handler
// ************************************************************
void mainHandler(AsyncWebServerRequest *request) {
	debugMsgUtl("Got Main Handler request");
	request->send(SPIFFS, "/web/index.html");
}

// ************************************************************
// Main CSS handler
// ************************************************************
void cssHandler(AsyncWebServerRequest *request) {
	debugMsgUtl("Got css request");
	request->send(SPIFFS, "/web/style.css");
}

// ************************************************************
// Command to save current stats
// ************************************************************
void saveStatsHandler(AsyncWebServerRequest *request) {
  debugMsgUtl("Got save stats request");

  spiffsStorage.saveStatsToSpiffs();
  
  request->send(200, "text/json", "{\"status\": \"Stats saved\"}");
}

// ************************************************************
// Summary page
// ************************************************************
void getSummaryDataHandler(AsyncWebServerRequest *request) {
  debugMsgUtl("Got api summary GET request");
  
  AsyncResponseStream *response = request->beginResponseStream("application/json");
  DynamicJsonBuffer jsonBuffer;
  JsonObject &root = jsonBuffer.createObject();  

  root["ip"] = WiFi.localIP().toString();
  root["mac"] = WiFi.macAddress();
  root["ssid"] = WiFi.SSID();
  String clockUrl = "http://" + String(WiFi.getHostname()) + ".local";
  clockUrl.toLowerCase();
  root["clockurl"] = clockUrl;
  root["version"] = SOFTWARE_VERSION;

  root.printTo(*response);

  request->send(response);
}

// ************************************************************
// Config page
// ************************************************************
void getConfigDataHandler(AsyncWebServerRequest *request) {
  debugMsgUtl("Got api config GET request");
  
  AsyncResponseStream *response = request->beginResponseStream("application/json");
  DynamicJsonBuffer jsonBuffer;
  JsonObject &root = jsonBuffer.createObject();

  root["WifiOnAtStart"] = cc->WifiOnAtStart;

  root.printTo(*response);
  request->send(response);
}

// ************************************************************
// Check if a key is present in the JSON
// ************************************************************
bool elementPresent(JsonObject& json, const char* key) {
  if (json.containsKey(key)) {
    debugMsgUtl(String(key) + " is present");
    return true;
  } else {
    return false;
  }
}

// ************************************************************
// Check if a key is present in the JSON, update the value
// ************************************************************
void compareAndUpdateByte(JsonObject& json, const char* key, byte* variable) {
  if (json.containsKey(key)) {
    byte newVal = json[key];
    if (*variable != newVal) {
      debugMsgUtl(String(key) + " old: " + String(*variable));
      *variable = newVal;
      debugMsgUtl(String(key) + " new: " + String(*variable));
    }
  }
}

// ------------------------------------------------------------

void compareAndUpdateInt(JsonObject& json, const char* key, int* variable) {
  if (json.containsKey(key)) {
    int newVal = json[key];
    if (*variable != newVal) {
      debugMsgUtl(String(key) + " old: " + String(*variable));
      *variable = newVal;
      debugMsgUtl(String(key) + " new: " + String(*variable));
    }
  }
}

// ------------------------------------------------------------

void compareAndUpdateBool(JsonObject& json, const char* key, bool* variable) {
  if (json.containsKey(key)) {
    bool newVal = json[key].as<bool>();
    if (*variable != newVal) {
      debugMsgUtl(String(key) + " old: " + String(*variable));
      *variable = newVal;
      debugMsgUtl(String(key) + " new: " + String(*variable));
    }
  }
}

// ------------------------------------------------------------

void compareAndUpdateString(JsonObject& json, const char* key, String* variable) {
  if (json.containsKey(key)) {
    String newVal = json[key];
    if (*variable != newVal) {
      debugMsgUtl(String(key) + " old: " + String(*variable));
      *variable = newVal;
      debugMsgUtl(String(key) + " new: " + String(*variable));
    }
  }
}

// ************************************************************
// See if a key exists
// ************************************************************
bool checkPresence(JsonObject& json, const char* key) {
  return  json.containsKey(key);
}

// ************************************************************
// Process the post for changing the config
// ************************************************************
void postConfigDataHandler(AsyncWebServerRequest *request) {
  debugMsgUtl("Got api config POST request");

  // dumpArgs(request);

  DynamicJsonBuffer jsonBuffer;
  JsonObject& json = jsonBuffer.parse(String(request->arg("body")));

  if (json.success()) {

    // ------------------------------------------------------------

    compareAndUpdateBool  (json, "WifiOnAtStart",&cc->WifiOnAtStart);

    // ------------------------------------------------------------

    spiffsStorage.saveConfigToSpiffs();
    debugMsgUtl("Saved new config");
  } else {
    debugMsgUtl("Json parse failure: " + String(request->arg("body")));
  }

  // Return the updated values
  getConfigDataHandler(request);
}

// ************************************************************
// Diags page
// ************************************************************
void getDiagsDataHandler(AsyncWebServerRequest *request) {
  debugMsgUtl("Got api diagnostics GET request");
  
  AsyncResponseStream *response = request->beginResponseStream("application/json");
  DynamicJsonBuffer jsonBuffer;
  JsonObject &root = jsonBuffer.createObject();  

  const char compile_date[] = __DATE__ " " __TIME__;
  // Total ontime for the life of the clock
  root["uptime"] = secsToReadableString(cs->uptimeMins * 60);

  // Total time the tubes have been on for
  root["ontime"] = secsToReadableString(cs->tubeOnTimeMins * 60);

  root["heap"] = ESP.getFreeHeap();
  root["freesketch"] = ESP.getFreeSketchSpace();
  root["sketchsize"] = ESP.getSketchSize();
  root["flashsize"] = ESP.getFlashChipSize();
  root["compiledate"] = String(compile_date);
  root["cpufreq"] = ESP.getCpuFreqMHz();
  root["sdkversion"] = ESP.getSdkVersion();
  root["sketchmd5"] = ESP.getSketchMD5();

  // Time since last reboot
  root["runtime"] = secsToReadableString(nowMillis/1000);
  root["cyclecount"] = ESP.getCycleCount();
  root["minfreepsram"] = ESP.getMinFreePsram();
  root["minfreeheap"] = ESP.getMinFreeHeap();
  root["resetreason"] = String(rtc_get_reset_reason(0)) + "/" + String(rtc_get_reset_reason(1));

  debugMsgUtl("Start partition recovery");
  String partitionStr = "Name,type,subtype,offset,length;";
  esp_partition_iterator_t iter = esp_partition_find(ESP_PARTITION_TYPE_APP, ESP_PARTITION_SUBTYPE_ANY, NULL);
  while (iter != nullptr)
  {
    const esp_partition_t *partition = esp_partition_get(iter);
    char buffer[60];
    sprintf(buffer, "%s,app,%d,0x%x,0x%x,(%d);", partition->label, partition->subtype, partition->address, partition->size, partition->size);
    partitionStr = partitionStr + String(buffer);
    iter = esp_partition_next(iter);
  }
  
  esp_partition_iterator_release(iter);
  iter = esp_partition_find(ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_ANY, NULL);
  while (iter != nullptr)
  {
    const esp_partition_t *partition = esp_partition_get(iter);
    char buffer[60];
    sprintf(buffer, "%s,data,%d,0x%x,0x%x,(%d);", partition->label, partition->subtype, partition->address, partition->size, partition->size);
    partitionStr = partitionStr + String(buffer);
    iter = esp_partition_next(iter);
  }
  
  esp_partition_iterator_release(iter);
  debugMsgUtl("End partition recovery");
  
  #ifdef DIGIT_DIAGNOSTICS
  root["diagsMode"] = cc->diagsMode;
  #endif

  String featureString = "";
  #ifdef DEBUG
  featureString += "DEB ";
  #endif

  #ifdef OLED_SSD1306
  featureString += "SSD1306 ";
  #endif

  #ifdef OLED_SH1106
  featureString += "SH1106 ";
  #endif

  root["features"] = featureString;

  root["partitions"] = partitionStr;

  root.printTo(*response);
  request->send(response);
}

// ************************************************************
// WiFi
// ************************************************************
void getCredentialsHandler(AsyncWebServerRequest *request) {
  debugMsgUtl("Got api wifi credentials request");
  
//  #ifdef DEBUG
//  dumpArgs(request);
//  #endif

  if (WiFi.isConnected()) {
    AsyncWebServerResponse* response = request->beginResponse(200, "text/json", "{\"connected\": \"true\", \"SSID\": \"" + WiFi.SSID() + "\"}");
    request->send(response);        
  } else {
    AsyncWebServerResponse* response = request->beginResponse(200, "text/json", "{\"connected\": \"false\"}");
    request->send(response);        
  }
}

// ************************************************************
// WiFi Credentials
// ************************************************************
void postWiFiCredentialsHandler(AsyncWebServerRequest *request) {
  debugMsgUtl("Got api wifi credentials POST request");
  
//  #ifdef DEBUG
//  dumpArgs(request);
//  #endif

  String newSSID = "";
  String newPassword = "";

  if (request->hasArg("SSID")) {
    newSSID = request->arg("SSID");
  }
  if (request->hasArg("password")) {
    newPassword = request->arg("password");
  }

  if (newSSID.length() > 0 && newPassword.length() > 0) {
    debugMsgUtl("Setting new WiFi credentials - " + newSSID + ":" + newPassword);

    wifiManager.saveWiFiCredentials(newSSID, newPassword);

    AsyncWebServerResponse* response = request->beginResponse(200, "text/json", "{\"status\": \"Saved " + newSSID + "\"}");
    request->send(response);
  } else {
    AsyncWebServerResponse* response = request->beginResponse(200, "text/json", "{\"status\": \"No changes saved\"}");
    request->send(response);
  }

  // Autorestart
  delay(1000);

  ESP.restart();
}

// ************************************************************
// Return a list of WiFi Networks
// ************************************************************
void getWiFiNetworksHandler(AsyncWebServerRequest *request) {
  debugMsgUtl("Got api wifi networks request");
  
  if (WiFi.isConnected()) {
    AsyncWebServerResponse* response = request->beginResponse(200, "text/json", "{\"connected\": \"true\", \"SSID\": \"" + WiFi.SSID() + "\"}");
    request->send(response);        
    debugMsgUtl("Scan aborted because we are already connected");
  } else {
    AsyncWebServerResponse* response = request->beginResponse(200, "text/json", "{\"connected\": \"false\", \"SSIDs\": \"" + lastWiFiScan + "\"}");
    request->send(response);        
    debugMsgUtl("Scan done");

    // trigger a new scan
    wifiManager.startScanWiFiNetworks();
  }
}

// ************************************************************
// Reset / restart
// ************************************************************
void restartHandler(AsyncWebServerRequest *request) {
  debugMsgUtl("Got api restart request");
  
  AsyncWebServerResponse* response = request->beginResponse(200, "text/json", "{\"status\": \"Restart in 1s\"}");
  request->send(response);

  // preserve the uptime over restarts, especially after OTA
  spiffsStorage.saveStatsToSpiffs();

  delay(1000);
  ESP.restart();
}

// ************************************************************
// Web Handler for reset WiFi
// ************************************************************
void resetWifiHandler(AsyncWebServerRequest *request) {
  resetWiFi();
  request->send(200, "text/json", "{\"status\": \"WiFi was reset\"}");
}

// ************************************************************
// Reset the WiFi credentials we have stored
// ************************************************************
void resetWiFi() {
  debugMsgUtl("Got utils RESET request");
  WiFi.disconnect();

  wifiManager.resetWiFiCredentials();
}

// ************************************************************
// Reset everything
// ************************************************************
void resetAll() {
  resetOptions();
  resetWiFi();
}

// ************************************************************
// Utilities
// ************************************************************
void getI2CScanHandler(AsyncWebServerRequest *request) {
  debugMsgUtl("Got I2C scan request");
  
  AsyncJsonResponse * response = new AsyncJsonResponse();
  response->addHeader("Server", "ESP Async Web Server");
  JsonObject& root = response->getRoot();

  byte error, address;
  int nDevices;
  nDevices = 0;
  for(address = 1; address < 127; address++ ) {
    Wire.beginTransmission(address);
    error = Wire.endTransmission();
    if (error == 0) {
      debugMsgUtl("I2C device found at address 0x" + String(address, HEX));
      root["I2C"+String(address)] = "found";
      nDevices++;
    }
    else if (error==4) {
      debugMsgUtl("Unknown error at address 0x" + String(address, HEX));
    }    
  }
  if (nDevices == 0) {
    debugMsgUtl("No I2C devices found");
  }
  else {
    debugMsgUtl("done");
  }

  response->setLength();
  request->send(response);
}

// ************************************************************
// Utilities
// ************************************************************
void getSPIFFSScanHandler(AsyncWebServerRequest *request) {
  debugMsgUtl("Got SPIFFS scan request");
  
  AsyncJsonResponse * response = new AsyncJsonResponse();
  response->addHeader("Server", "ESP Async Web Server");
  JsonObject& responseRoot = response->getRoot();

  responseRoot["FILE Listing"] = "";
  File root = SPIFFS.open("/");
  File file = root.openNextFile();
 
  int i = 1;
  while(file){ 
      responseRoot["FILE " + String(i) + ": "] = String(file.name());
      file = root.openNextFile();
      i++;
  }

  debugMsgUtl("done");

  response->setLength();
  request->send(response);
}

// ************************************************************
// Turn on Watchdog
// ************************************************************
void enableWatchdog() {
  esp_task_wdt_init(WDT_TIMEOUT, true);
  esp_task_wdt_add(NULL);
}

// ************************************************************
// Turn off Watchdog
// ************************************************************
void disableWatchdog() {
  esp_task_wdt_delete(NULL);
  esp_task_wdt_deinit();
}

// ************************************************************
// Reset the Watchdog timeout
// ************************************************************
void feedWatchdog() {
  esp_task_wdt_reset();
}
