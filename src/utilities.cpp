#include "utilities.h"
#include "RadioOutputManager.h"

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

//**********************************************************************************
//**********************************************************************************
//*                          Radio Web Interface                                   *
//**********************************************************************************
//**********************************************************************************

static const char RADIO_PAGE[] PROGMEM = R"rawliteral(
<!DOCTYPE html><html><head>
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>Internet Radio</title>
<style>
*{box-sizing:border-box;margin:0;padding:0}
body{font-family:system-ui,sans-serif;background:#1a1a2e;color:#e0e0e0;padding:16px;max-width:600px;margin:0 auto}
h1{color:#0f3460;background:#e0e0e0;padding:12px;border-radius:8px;text-align:center;margin-bottom:16px;font-size:1.3em}
.card{background:#16213e;border-radius:8px;padding:16px;margin-bottom:12px}
.card h2{font-size:1em;color:#a0a0a0;margin-bottom:8px}
.now{font-size:1.1em;color:#00d4ff;min-height:1.4em}
.station{display:flex;align-items:center;padding:8px 0;border-bottom:1px solid #1a1a2e}
.station:last-child{border:none}
.station .name{flex:1;font-weight:bold}
.station .url{flex:2;font-size:0.8em;color:#888;overflow:hidden;text-overflow:ellipsis;white-space:nowrap;padding:0 8px}
button{background:#0f3460;color:#fff;border:none;border-radius:4px;padding:6px 12px;cursor:pointer;font-size:0.85em}
button:hover{background:#1a5276}
button.stop{background:#922}
button.stop:hover{background:#b33}
button.del{background:#555;font-size:0.75em}
button.del:hover{background:#922}
input[type=text]{width:100%;padding:8px;border-radius:4px;border:1px solid #333;background:#0f1a30;color:#e0e0e0;margin-bottom:8px;font-size:0.9em}
input[type=range]{width:100%;margin:8px 0;accent-color:#00d4ff}
.row{display:flex;gap:8px;align-items:center}
.vol-val{min-width:32px;text-align:right;color:#00d4ff}
#msg{color:#0a4;font-size:0.85em;min-height:1.2em;margin-top:4px}
.notif{position:fixed;top:0;left:0;right:0;padding:12px;text-align:center;font-size:0.95em;z-index:99;transition:opacity 0.5s}
.notif.err{background:#922;color:#fff}
.notif.ok{background:#0a4;color:#fff}
</style></head><body>
<div id="notif" class="notif" style="display:none"></div>
<h1>Internet Radio</h1>
<div class="card"><h2>Now Playing</h2>
<div class="now" id="np">--</div>
<div style="margin-top:8px" id="ctrl">
<button onclick="doPlay(-1)">Play</button>
<button class="stop" onclick="doStop()">Stop</button>
</div></div>
<div class="card"><h2>Volume</h2>
<div class="row"><input type="range" id="vol" min="0" max="100" value="10"
oninput="document.getElementById('vv').textContent=this.value"
onchange="setVol(this.value)"><span class="vol-val" id="vv">10</span></div></div>
<div class="card"><h2>Stations</h2>
<div id="sl">Loading...</div></div>
<div class="card"><h2>Add Station</h2>
<input type="text" id="sn" placeholder="Station name">
<input type="text" id="su" placeholder="Stream URL (http://...)">
<button onclick="addStation()">Add</button>
<div id="msg"></div></div>
<script>
function notify(msg,type){var e=document.getElementById('notif');e.textContent=msg;e.className='notif '+(type||'err');e.style.display='block';e.style.opacity='1';setTimeout(function(){e.style.opacity='0';setTimeout(function(){e.style.display='none'},500)},4000)}
function api(u,m,b){return fetch(u,{method:m||'GET',headers:b?{'Content-Type':'application/x-www-form-urlencoded'}:{},body:b}).then(r=>r.json())}
function refresh(){
api('/api/status').then(d=>{
document.getElementById('np').textContent=d.playing?(d.station+' - '+d.url):'Stopped';
document.getElementById('vol').value=d.volume;
document.getElementById('vv').textContent=d.volume;
});
api('/api/stations').then(d=>{
let h='';
d.forEach((s,i)=>{
h+='<div class="station"><span class="name">'+s.name+'</span><span class="url">'+s.url+'</span><button onclick="doPlay('+i+')">Play</button> <button class="del" onclick="delStation('+i+')">Del</button></div>';
});
document.getElementById('sl').innerHTML=h||'No stations';
});
}
function doPlay(i){api('/api/play','POST','index='+i).then(refresh)}
function doStop(){api('/api/stop','POST','x=1').then(refresh)}
function setVol(v){api('/api/volume','POST','volume='+v)}
function addStation(){
let n=document.getElementById('sn').value,u=document.getElementById('su').value;
if(!n||!u){document.getElementById('msg').textContent='Name and URL required';return}
if(!u.startsWith('http://')){notify('Only http:// streams are supported. https:// will not work.','err');return}
api('/api/stations','POST','name='+encodeURIComponent(n)+'&url='+encodeURIComponent(u)).then(d=>{
document.getElementById('msg').textContent=d.status||'Added';
document.getElementById('sn').value='';document.getElementById('su').value='';refresh();
});
}
function delStation(i){api('/api/stations/delete','POST','index='+i).then(d=>{
document.getElementById('msg').textContent=d.status||'Deleted';refresh();
})}
refresh();
</script></body></html>
)rawliteral";

// ************************************************************
// Serve the radio web page
// ************************************************************
void radioPageHandler(AsyncWebServerRequest *request) {
  request->send(200, "text/html", RADIO_PAGE);
}

// ************************************************************
// GET /api/stations - return station list as JSON array
// ************************************************************
void getStationsHandler(AsyncWebServerRequest *request) {
  AsyncResponseStream *response = request->beginResponseStream("application/json");
  DynamicJsonBuffer jsonBuffer;
  JsonArray &arr = jsonBuffer.createArray();

  for (int i = 0; i < stationCount; i++) {
    JsonObject &s = arr.createNestedObject();
    s["name"] = stations[i].name;
    s["url"] = stations[i].url;
  }

  arr.printTo(*response);
  request->send(response);
}

// ************************************************************
// POST /api/stations - add a station
// ************************************************************
void postStationHandler(AsyncWebServerRequest *request) {
  if (stationCount >= MAX_STATIONS) {
    request->send(200, "application/json", "{\"status\":\"Station list full\"}");
    return;
  }

  String name = request->hasArg("name") ? request->arg("name") : "";
  String url = request->hasArg("url") ? request->arg("url") : "";

  if (name.length() == 0 || url.length() == 0) {
    request->send(200, "application/json", "{\"status\":\"Name and URL required\"}");
    return;
  }

  stations[stationCount].name = name;
  stations[stationCount].url = url;
  stationCount++;

  spiffsStorage.saveStationsToSpiffs();
  request->send(200, "application/json", "{\"status\":\"Station added\"}");
}

// ************************************************************
// POST /api/stations/delete - delete a station by index
// ************************************************************
void deleteStationHandler(AsyncWebServerRequest *request) {
  if (!request->hasArg("index")) {
    request->send(200, "application/json", "{\"status\":\"Index required\"}");
    return;
  }

  int idx = request->arg("index").toInt();
  if (idx < 0 || idx >= stationCount) {
    request->send(200, "application/json", "{\"status\":\"Invalid index\"}");
    return;
  }

  // Shift remaining stations down
  for (int i = idx; i < stationCount - 1; i++) {
    stations[i] = stations[i + 1];
  }
  stationCount--;
  stations[stationCount].name = "";
  stations[stationCount].url = "";

  spiffsStorage.saveStationsToSpiffs();
  request->send(200, "application/json", "{\"status\":\"Station deleted\"}");
}

// ************************************************************
// GET /api/status - return current playback status
// ************************************************************
void getStatusHandler(AsyncWebServerRequest *request) {
  AsyncResponseStream *response = request->beginResponseStream("application/json");
  DynamicJsonBuffer jsonBuffer;
  JsonObject &root = jsonBuffer.createObject();

  root["playing"] = radioOutputManager.isPlaying();
  root["volume"] = volume;
  root["mode"] = radioOutputManager.isBluetoothMode() ? "bluetooth" : "radio";

  root["station"] = radioOutputManager.getStationName();
  root["url"] = radioOutputManager.getUrl();

  root.printTo(*response);
  request->send(response);
}

// ************************************************************
// POST /api/play - play a station by index
// ************************************************************
void postPlayHandler(AsyncWebServerRequest *request) {
  int idx = request->hasArg("index") ? request->arg("index").toInt() : -1;

  if (idx >= 0 && idx < stationCount) {
    float gain = (volume / 100.0f) * MAX_GAIN;
    radioOutputManager.startRadioStream(stations[idx].url, stations[idx].name, gain);
    request->send(200, "application/json", "{\"status\":\"Playing\"}");
  } else if (stationCount > 0) {
    // Play first station if no valid index
    float gain = (volume / 100.0f) * MAX_GAIN;
    radioOutputManager.startRadioStream(stations[0].url, stations[0].name, gain);
    request->send(200, "application/json", "{\"status\":\"Playing\"}");
  } else {
    request->send(200, "application/json", "{\"status\":\"No stations\"}");
  }
}

// ************************************************************
// POST /api/stop - stop playback
// ************************************************************
void postStopHandler(AsyncWebServerRequest *request) {
  radioOutputManager.StopPlaying();
  request->send(200, "application/json", "{\"status\":\"Stopped\"}");
}

// ************************************************************
// POST /api/volume - set volume 0-100
// ************************************************************
void postVolumeHandler(AsyncWebServerRequest *request) {
  if (request->hasArg("volume")) {
    int vol = request->arg("volume").toInt();
    if (vol < 0) vol = 0;
    if (vol > 100) vol = 100;
    volume = vol;
    radioOutputManager.setVolume(volume);
    request->send(200, "application/json", "{\"status\":\"OK\"}");
  } else {
    request->send(200, "application/json", "{\"status\":\"Volume required\"}");
  }
}
