#include "SpiffsStorage.h"
#include <esp32-hal-psram.h>

//**********************************************************************************
//**********************************************************************************
//*                               SPIFFS functions                                 *
//**********************************************************************************
//**********************************************************************************
// ************************************************************
// Test SPIFFS
// ************************************************************
bool SpiffsStorage_::testMountSpiffs()
{
  bool mounted = false;
  if (SPIFFS.begin())
  {
    mounted = true;
  }
  return mounted;
}

// ************************************************************
// Retrieve the config from the SPIFFS
// ************************************************************
bool SpiffsStorage_::getConfigFromSpiffs()
{
  bool loaded = false;
  debugMsgSpfX("mounted file system config read");
  if (SPIFFS.exists("/config/config.json"))
  {
    // file exists, reading and loading
    debugMsgSpf("Reading config file");
    File configFile = SPIFFS.open("/config/config.json", "r");
    if (configFile)
    {
      debugMsgSpfX("opened config file");
      size_t size = configFile.size();
      // Allocate a buffer from PSRAM if available
      char *rawBuf = psramFound() ? (char *)ps_malloc(size) : nullptr;
      if (!rawBuf) rawBuf = (char *)malloc(size);
      std::unique_ptr<char[], decltype(&free)> buf(rawBuf, free);
      configFile.readBytes(buf.get(), size);
      DynamicJsonBuffer jsonBuffer;
      JsonObject &json = jsonBuffer.parseObject(buf.get());
      #ifdef SPF_EXTENDED_DEBUG
      // Dump the raw JSON
      json.printTo(Serial);
      debugMsgSpfX("\n");
      #endif
      if (json.success())
      {
        debugMsgSpfX("parsed config json");

        cc->WiFiSSID = json["WiFiSSID"].as<String>();
        debugMsgSpfX("Loaded WiFiSSID: " + String(cc->WiFiSSID));

        cc->WiFiPassword = json["WiFiPassword"].as<String>();
        debugMsgSpfX("Loaded WiFiPassword: " + String(cc->WiFiPassword));

        cc->WifiOnAtStart = json["WifiOnAtStart"].as<bool>();
        debugMsgSpfX("Loaded WifiOnAtStart: " + String(cc->WifiOnAtStart));

        loaded = true;
      }
      else
      {
        debugMsgSpf("failed to load json config");
      }
      debugMsgSpfX("Closing config file");

      configFile.close();
    }
  }
  return loaded;
}

// ************************************************************
// Save config back to the SPIFFS
// ************************************************************
void SpiffsStorage_::saveConfigToSpiffs()
{
  debugMsgSpf("Saving config");

  DynamicJsonBuffer jsonBuffer;
  JsonObject &json = jsonBuffer.createObject();
  json["WiFiSSID"] = cc->WiFiSSID;
  json["WiFiPassword"] = cc->WiFiPassword;
  json["WifiOnAtStart"] = cc->WifiOnAtStart;
  
  File configFile = SPIFFS.open("/config/config.json", "w");
  if (!configFile)
  {
    debugMsgSpf("Failed to open config file for writing");

    configFile.close();
    return;
  }
  json.printTo(configFile);
  configFile.close();
  debugMsgSpf("Saved config");
}

// ************************************************************
// Get the statistics from the SPIFFS
// ************************************************************
bool SpiffsStorage_::getStatsFromSpiffs()
{
  bool loaded = false;
  if (SPIFFS.exists("/config/stats.json"))
  {
    // file exists, reading and loading
    debugMsgSpf("Reading stats file");

    File statsFile = SPIFFS.open("/config/stats.json", "r");
    if (statsFile)
    {
      debugMsgSpfX("opened stats file");

      size_t size = statsFile.size();
      // Allocate a buffer from PSRAM if available
      char *rawBuf = psramFound() ? (char *)ps_malloc(size) : nullptr;
      if (!rawBuf) rawBuf = (char *)malloc(size);
      std::unique_ptr<char[], decltype(&free)> buf(rawBuf, free);
      statsFile.readBytes(buf.get(), size);
      DynamicJsonBuffer jsonBuffer;
      JsonObject &json = jsonBuffer.parseObject(buf.get());
      if (json.success())
      {
        debugMsgSpfX("parsed stats json");

        cs->uptimeMins = json.get<unsigned long>("uptime");
        debugMsgSpfX("Loaded uptime: " + String(cs->uptimeMins));

        cs->playtimeMins = json.get<unsigned long>("playtime");
        debugMsgSpfX("Loaded playtime: " + String(cs->playtimeMins));

        loaded = true;
      }
      else
      {
        debugMsgSpf("Failed to load json config");
      }
      debugMsgSpfX("Closing stats file");

      statsFile.close();
    }
  }
  return loaded;
}

// ************************************************************
// Save the statistics back to the SPIFFS
// ************************************************************
void SpiffsStorage_::saveStatsToSpiffs()
{
  debugMsgSpf("Saving stats");
  DynamicJsonBuffer jsonBuffer;
  JsonObject &json = jsonBuffer.createObject();
  json.set("uptime", cs->uptimeMins);

  File statsFile = SPIFFS.open("/config/stats.json", "w");
  if (!statsFile)
  {
    debugMsgSpf("Failed to open stats file for writing");
    statsFile.close();
    return;
  }
  json.printTo(statsFile);
  statsFile.close();
  debugMsgSpf("Saved stats");
}


// ************************************************************
// Station helpers — read/write stations.json on demand (no in-memory array)
// ************************************************************

static const char* STATIONS_FILE = "/config/stations.json";

// Allocate and return a null-terminated copy of stations.json.
// Returns nullptr if the file doesn't exist or can't be read.
// Caller must free() the returned pointer.
static char* readStationsRaw() {
  if (!SPIFFS.exists(STATIONS_FILE)) return nullptr;
  File f = SPIFFS.open(STATIONS_FILE, "r");
  if (!f) return nullptr;
  size_t sz = f.size();
  char* raw = psramFound() ? (char*)ps_malloc(sz + 1) : nullptr;
  if (!raw) raw = (char*)malloc(sz + 1);
  if (!raw) { f.close(); return nullptr; }
  f.readBytes(raw, sz);
  raw[sz] = '\0';
  f.close();
  return raw;
}

int SpiffsStorage_::getStationCount() {
  char* raw = readStationsRaw();
  if (!raw) {
    // No file yet — seed a default and return 1
    appendStation("Radio FFH", "http://mp3.ffh.de/radioffh/hqlivestream.mp3");
    return 1;
  }
  DynamicJsonBuffer jsonBuffer;
  JsonArray& arr = jsonBuffer.parseArray(raw);
  int count = arr.success() ? (int)arr.size() : 0;
  free(raw);
  return count;
}

bool SpiffsStorage_::getStation(int idx, String& name, String& url) {
  char* raw = readStationsRaw();
  if (!raw) return false;
  DynamicJsonBuffer jsonBuffer;
  JsonArray& arr = jsonBuffer.parseArray(raw);
  if (!arr.success() || idx < 0 || idx >= (int)arr.size()) {
    free(raw);
    return false;
  }
  JsonObject& s = arr[idx];
  name = s["name"].as<String>();
  url  = s["url"].as<String>();
  free(raw);
  return true;
}

bool SpiffsStorage_::appendStation(const String& name, const String& url) {
  DynamicJsonBuffer jsonBuffer;
  JsonArray* arrPtr = nullptr;
  char* raw = readStationsRaw();
  if (raw) {
    JsonArray& parsed = jsonBuffer.parseArray(raw);
    if (parsed.success()) arrPtr = &parsed;
  }
  JsonArray& arr = arrPtr ? *arrPtr : jsonBuffer.createArray();
  JsonObject& s = arr.createNestedObject();
  s["name"] = name;
  s["url"]  = url;
  File f = SPIFFS.open(STATIONS_FILE, "w");
  if (!f) { if (raw) free(raw); return false; }
  arr.printTo(f);
  f.close();
  if (raw) free(raw);
  debugMsgSpf("Station appended: " + name);
  return true;
}

bool SpiffsStorage_::deleteStation(int idx) {
  char* raw = readStationsRaw();
  if (!raw) return false;
  DynamicJsonBuffer inBuf;
  JsonArray& arr = inBuf.parseArray(raw);
  if (!arr.success() || idx < 0 || idx >= (int)arr.size()) {
    free(raw);
    return false;
  }
  // Rebuild the array without the deleted element using copies of the strings
  DynamicJsonBuffer outBuf;
  JsonArray& newArr = outBuf.createArray();
  for (int i = 0; i < (int)arr.size(); i++) {
    if (i == idx) continue;
    JsonObject& src = arr[i];
    JsonObject& dst = newArr.createNestedObject();
    dst["name"] = src["name"].as<String>();
    dst["url"]  = src["url"].as<String>();
  }
  free(raw);
  File f = SPIFFS.open(STATIONS_FILE, "w");
  if (!f) return false;
  newArr.printTo(f);
  f.close();
  debugMsgSpf("Station deleted at index " + String(idx));
  return true;
}

// ************************************************************
// Internal plumbing
// ************************************************************

SpiffsStorage_ &SpiffsStorage_::getInstance() {
  static SpiffsStorage_ instance;
  return instance;
}

SpiffsStorage_ &spiffsStorage = spiffsStorage.getInstance();