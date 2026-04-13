#include "RadioMenuConfiguration.h"

// ************************************************************
// Menu system instance and state
// ************************************************************
MenuSystem menuSystem;
StatusData radioStatus = {false, "0.0.0.0"};
MenuItem* mainMenu = nullptr;
MenuItem* audioMenu = nullptr;
MenuItem* wifiMenu = nullptr;
MenuItem* systemMenu = nullptr;

// WiFi string buffers for the menu system
static char wifiSSIDBuffer[32];
static char wifiPasswordBuffer[64];

// ************************************************************
// Audio menu callbacks
// ************************************************************
// Station selection callbacks (one per MAX_STATIONS slot)
static void playStation(int idx) {
  if (idx >= 0 && idx < stationCount) {
    float gain = (volume / 100.0f) * MAX_GAIN;
    radioOutputManager.startRadioStream(stations[idx].url, stations[idx].name, gain);
  }
  buildAudioMenuDynamic();
}
static void playStation0() { playStation(0); }
static void playStation1() { playStation(1); }
static void playStation2() { playStation(2); }
static void playStation3() { playStation(3); }
static void playStation4() { playStation(4); }
static void playStation5() { playStation(5); }
static void playStation6() { playStation(6); }
static void playStation7() { playStation(7); }
static void playStation8() { playStation(8); }

typedef void (*StationCallback)();
static StationCallback stationCallbacks[] = {
  playStation0, playStation1, playStation2, playStation3, playStation4,
  playStation5, playStation6, playStation7, playStation8
};

void startPlaying() {
  radioOutputManager.StartPlaying();
  buildAudioMenuDynamic();
}

void stopPlaying() {
  radioOutputManager.StopPlaying();
  buildAudioMenuDynamic();
}

#ifdef FEATURE_BLUETOOTH
void switchToBluetoothMode() {
  radioOutputManager.setAudioMode(AUDIO_MODE_BLUETOOTH);
  buildAudioMenuDynamic();
}

void switchToRadioBtMode() {
  radioOutputManager.setAudioMode(AUDIO_MODE_RADIO_BLUETOOTH);
  buildAudioMenuDynamic();
}

void switchToRadioMode() {
  radioOutputManager.setAudioMode(AUDIO_MODE_RADIO);
  buildAudioMenuDynamic();
}
#endif

// ************************************************************
// WiFi scan network selection callbacks
// ************************************************************
#define MAX_SCAN_RESULTS 10

static void selectScannedNetwork(int idx) {
  String ssid = wifiManager.getLastScanResultSSID(idx);
  strncpy(wifiSSIDBuffer, ssid.c_str(), sizeof(wifiSSIDBuffer) - 1);
  wifiSSIDBuffer[sizeof(wifiSSIDBuffer) - 1] = '\0';
  cc->WiFiSSID = ssid;
  debugMsgMnm("Selected scanned network: " + ssid);
  buildWifiMenuDynamic();
}
static void selectScannedNetwork0() { selectScannedNetwork(0); }
static void selectScannedNetwork1() { selectScannedNetwork(1); }
static void selectScannedNetwork2() { selectScannedNetwork(2); }
static void selectScannedNetwork3() { selectScannedNetwork(3); }
static void selectScannedNetwork4() { selectScannedNetwork(4); }
static void selectScannedNetwork5() { selectScannedNetwork(5); }
static void selectScannedNetwork6() { selectScannedNetwork(6); }
static void selectScannedNetwork7() { selectScannedNetwork(7); }
static void selectScannedNetwork8() { selectScannedNetwork(8); }
static void selectScannedNetwork9() { selectScannedNetwork(9); }

typedef void (*ScanCallback)();
static ScanCallback scanCallbacks[] = {
  selectScannedNetwork0, selectScannedNetwork1, selectScannedNetwork2,
  selectScannedNetwork3, selectScannedNetwork4, selectScannedNetwork5,
  selectScannedNetwork6, selectScannedNetwork7, selectScannedNetwork8,
  selectScannedNetwork9
};

// ************************************************************
// WiFi menu callbacks
// ************************************************************
void disconnectWifiCb() {
  wifiManager.disconnectWiFi();
  buildWifiMenuDynamic();
}

void reconnectPreviousCb() {
  wifiManager.connectToLastAP();
  buildWifiMenuDynamic();
}

void connectWPSCb() {
  wifiManager.connectWithWPS();
  buildWifiMenuDynamic();
}

void smartConfigCb() {
  wifiManager.startSmartConfig();
  buildWifiMenuDynamic();
}

void openAccessPointCb() {
  wifiManager.openAccessPortal();
  buildWifiMenuDynamic();
}

void scanWiFiCb() {
  wifiManager.startScanWiFiNetworks();
  buildWifiMenuDynamic();
}

void saveWiFiSSIDCb() {
  cc->WiFiSSID = String(wifiSSIDBuffer);
  debugMsgMnm("Set SSID: " + cc->WiFiSSID);
}

void saveWiFiPasswordCb() {
  cc->WiFiPassword = String(wifiPasswordBuffer);
  debugMsgMnm("Set WiFi pw: " + cc->WiFiPassword);
}

// ************************************************************
// System menu callbacks
// ************************************************************
void restartDeviceCb() {
  spiffsStorage.saveStatsToSpiffs();
  delay(1000);
  ESP.restart();
}

void saveConfigCb() {
  spiffsStorage.saveConfigToSpiffs();
}

void toggleWiFiAtStartCb() {
  cc->WifiOnAtStart = !cc->WifiOnAtStart;
  buildSystemMenuDynamic();
}

#ifdef DEBUG
void debugOn10minsCb() {
  debugManager.setDebugAutoOff(600);
}
#endif

void resetWiFiInfoCb() {
  resetWiFi();
  buildSystemMenuDynamic();
}

// ************************************************************
// Build the Audio menu (dynamic based on mode)
// ************************************************************
void buildAudioMenuDynamic() {
  if (!audioMenu) return;

  // Clear existing items and repopulate
  menuSystem.clearMenuItems(audioMenu);

  if (radioOutputManager.isRadioMode()) {
    menuSystem.addInfo(audioMenu, "Mode: Radio");
    for (int i = 0; i < stationCount && i < MAX_STATIONS; i++) {
      menuSystem.addAction(audioMenu, stations[i].name.c_str(), stationCallbacks[i]);
    }
    menuSystem.addAction(audioMenu, "Stop", stopPlaying);
#ifdef FEATURE_BLUETOOTH
    menuSystem.addAction(audioMenu, "Switch to BT Sink", switchToBluetoothMode);
    menuSystem.addAction(audioMenu, "Radio to BT Spkr", switchToRadioBtMode);
#endif
  }
#ifdef FEATURE_BLUETOOTH
  else if (radioOutputManager.isRadioBtMode()) {
    menuSystem.addInfo(audioMenu, "Mode: Radio->BT");
    if (radioOutputManager.isPlaying()) {
      menuSystem.addInfo(audioMenu, "Status: Streaming");
    } else {
      menuSystem.addInfo(audioMenu, "Status: Connecting");
    }
    for (int i = 0; i < stationCount && i < MAX_STATIONS; i++) {
      menuSystem.addAction(audioMenu, stations[i].name.c_str(), stationCallbacks[i]);
    }
    menuSystem.addAction(audioMenu, "Switch to Radio", switchToRadioMode);
  } else {
    menuSystem.addInfo(audioMenu, "Mode: BT Sink");
    if (bluetoothManager.isBluetoothConnected()) {
      menuSystem.addInfo(audioMenu, "Status: Connected");
    } else {
      menuSystem.addInfo(audioMenu, "Status: Waiting...");
    }
    menuSystem.addAction(audioMenu, "Switch to Radio", switchToRadioMode);
  }
#endif

  menuSystem.navigateToMenu(audioMenu);
}

// ************************************************************
// Build the WiFi menu (dynamic based on connection state)
// ************************************************************
void buildWifiMenuDynamic() {
  if (!wifiMenu) return;

  // Clear existing items and repopulate
  menuSystem.clearMenuItems(wifiMenu);

  if (WiFi.isConnected()) {
    menuSystem.addAction(wifiMenu, "Disconnect WiFi", disconnectWifiCb);
  } else {
    if (wifiManager.wifiCredentialsReceived()) {
      menuSystem.addAction(wifiMenu, "Reconnect Prev", reconnectPreviousCb);
    }
    menuSystem.addAction(wifiMenu, "Connect WPS", connectWPSCb);
    menuSystem.addAction(wifiMenu, "SmartConfig", smartConfigCb);
    menuSystem.addAction(wifiMenu, "Access Point", openAccessPointCb);
    menuSystem.addAction(wifiMenu, "Scan WiFi", scanWiFiCb);

    int scanCount = wifiManager.getLastScanResultCount();
    if (scanCount > 0) {
      menuSystem.addInfo(wifiMenu, "-- Scanned Networks --");
      for (int i = 0; i < scanCount && i < MAX_SCAN_RESULTS; i++) {
        menuSystem.addAction(wifiMenu, wifiManager.getLastScanResultSSID(i).c_str(), scanCallbacks[i]);
      }
    }

    // Copy current values into char buffers for string editing
    strncpy(wifiSSIDBuffer, cc->WiFiSSID.c_str(), sizeof(wifiSSIDBuffer) - 1);
    wifiSSIDBuffer[sizeof(wifiSSIDBuffer) - 1] = '\0';
    strncpy(wifiPasswordBuffer, cc->WiFiPassword.c_str(), sizeof(wifiPasswordBuffer) - 1);
    wifiPasswordBuffer[sizeof(wifiPasswordBuffer) - 1] = '\0';

    menuSystem.addStringValue(wifiMenu, "Enter SSID", wifiSSIDBuffer, sizeof(wifiSSIDBuffer), NULL, saveWiFiSSIDCb);
    menuSystem.addStringValue(wifiMenu, "Enter Password", wifiPasswordBuffer, sizeof(wifiPasswordBuffer), NULL, saveWiFiPasswordCb);
  }

  menuSystem.navigateToMenu(wifiMenu);
}

// ************************************************************
// Build the System menu
// ************************************************************
void buildSystemMenuDynamic() {
  if (!systemMenu) return;

  // Clear existing items and repopulate
  menuSystem.clearMenuItems(systemMenu);

  menuSystem.addAction(systemMenu, "Restart Device", restartDeviceCb);
  menuSystem.addAction(systemMenu, "Save Config", saveConfigCb);
  menuSystem.addEitherOr(systemMenu, "WiFi at Start", &cc->WifiOnAtStart, "ON", "OFF", NULL);
  #ifdef DEBUG
  menuSystem.addAction(systemMenu, "Debug 10m", debugOn10minsCb);
  #endif
  menuSystem.addAction(systemMenu, "Reset WiFi", resetWiFiInfoCb);

  menuSystem.navigateToMenu(systemMenu);
}

// ************************************************************
// Build all menus (called once at startup)
// ************************************************************
void buildRadioMenus() {
  mainMenu = menuSystem.createMenu("Main Menu");

  // Build submenus
  audioMenu = menuSystem.createMenu("Audio");
  if (radioOutputManager.isRadioMode()) {
    menuSystem.addInfo(audioMenu, "Mode: Radio");
    for (int i = 0; i < stationCount && i < MAX_STATIONS; i++) {
      menuSystem.addAction(audioMenu, stations[i].name.c_str(), stationCallbacks[i]);
    }
    menuSystem.addAction(audioMenu, "Stop", stopPlaying);
#ifdef FEATURE_BLUETOOTH
    menuSystem.addAction(audioMenu, "Switch to BT", switchToBluetoothMode);
  } else {
    menuSystem.addInfo(audioMenu, "Mode: Bluetooth");
    menuSystem.addInfo(audioMenu, "Status: Waiting...");
    menuSystem.addAction(audioMenu, "Switch to Radio", switchToRadioMode);
#endif
  }

  wifiMenu = menuSystem.createMenu("WiFi");
  if (WiFi.isConnected()) {
    menuSystem.addAction(wifiMenu, "Disconnect WiFi", disconnectWifiCb);
  } else {
    if (wifiManager.wifiCredentialsReceived()) {
      menuSystem.addAction(wifiMenu, "Reconnect Prev", reconnectPreviousCb);
    }
    menuSystem.addAction(wifiMenu, "Connect WPS", connectWPSCb);
    menuSystem.addAction(wifiMenu, "SmartConfig", smartConfigCb);
    menuSystem.addAction(wifiMenu, "Access Point", openAccessPointCb);
    menuSystem.addAction(wifiMenu, "Scan WiFi", scanWiFiCb);

    strncpy(wifiSSIDBuffer, cc->WiFiSSID.c_str(), sizeof(wifiSSIDBuffer) - 1);
    wifiSSIDBuffer[sizeof(wifiSSIDBuffer) - 1] = '\0';
    strncpy(wifiPasswordBuffer, cc->WiFiPassword.c_str(), sizeof(wifiPasswordBuffer) - 1);
    wifiPasswordBuffer[sizeof(wifiPasswordBuffer) - 1] = '\0';

    menuSystem.addStringValue(wifiMenu, "Enter SSID", wifiSSIDBuffer, sizeof(wifiSSIDBuffer), NULL, saveWiFiSSIDCb);
    menuSystem.addStringValue(wifiMenu, "Enter Password", wifiPasswordBuffer, sizeof(wifiPasswordBuffer), NULL, saveWiFiPasswordCb);
  }

  systemMenu = menuSystem.createMenu("System");
  menuSystem.addAction(systemMenu, "Restart Device", restartDeviceCb);
  menuSystem.addAction(systemMenu, "Save Config", saveConfigCb);
  menuSystem.addEitherOr(systemMenu, "WiFi at Start", &cc->WifiOnAtStart, "ON", "OFF", NULL);
  #ifdef DEBUG
  menuSystem.addAction(systemMenu, "Debug 10m", debugOn10minsCb);
  #endif
  menuSystem.addAction(systemMenu, "Reset WiFi", resetWiFiInfoCb);

  menuSystem.addSubmenu(mainMenu, "Audio", audioMenu);
  menuSystem.addSubmenu(mainMenu, "WiFi", wifiMenu);
  menuSystem.addSubmenu(mainMenu, "System", systemMenu);
}

// ************************************************************
// Status screen renderer
// ************************************************************
void renderRadioStatus(Adafruit_SH1106G* display, uint8_t width, uint8_t height) {
  display->setTextSize(1);
  display->setTextColor(SH110X_WHITE);

  uint8_t yPos = 2;

  // Title
  display->setCursor(0, yPos);
  display->setTextSize(2);
  display->print("INet Radio");
  yPos += 20;

  display->setTextSize(1);
  display->drawLine(0, yPos, width, yPos, SH110X_WHITE);
  yPos += 4;

  // Audio mode and status
  display->setCursor(0, yPos);
  if (radioOutputManager.isRadioMode()) {
    display->print("Radio: ");
    display->print(radioOutputManager.isPlaying() ? "Playing" : "Stopped");
  }
#ifdef FEATURE_BLUETOOTH
  else if (radioOutputManager.isRadioBtMode()) {
    display->print("Radio>BT: ");
    if (radioOutputManager.isPlaying()) {
      display->print("Streaming");
    } else if (bluetoothManager.isBluetoothSourceConnected()) {
      display->print("BT ready");
    } else {
      display->print("Connecting...");
    }
  } else {
    display->print("BT: ");
    display->print(bluetoothManager.isBluetoothConnected() ? "Connected" : "Waiting...");
  }
#endif
  yPos += 10;

  // WiFi status
  display->setCursor(0, yPos);
  display->print("WiFi: ");
  if (WiFi.isConnected()) {
    display->print(WiFi.localIP().toString());
  } else {
    display->print("Not connected");
  }
  yPos += 10;

  // Volume
  display->setCursor(0, yPos);
  display->print("Volume: ");
  display->print(volume);

  // Song title scroll (or fallback hint) at bottom
  const uint8_t scrollY = height - 8;
  display->fillRect(0, scrollY, width, height - scrollY, SH110X_BLACK);
  const char* songTitle = radioOutputManager.getSongTitle();
  if (songTitle[0] != '\0' && radioOutputManager.isPlaying()) {
    static char lastTitle[64] = "";
    static int scrollPos = 0;
    static unsigned long lastScrollTick = 0;

    if (strncmp(songTitle, lastTitle, sizeof(lastTitle)) != 0) {
      strncpy(lastTitle, songTitle, sizeof(lastTitle) - 1);
      lastTitle[sizeof(lastTitle) - 1] = '\0';
      scrollPos = 0;
      lastScrollTick = millis();
    }

    // Carousel: unit = title + 4-space gap, scroll wraps at one unit width
    int unitPx = (strlen(songTitle) + 4) * 6;
    unsigned long now = millis();
    if (now - lastScrollTick >= 50) {
      scrollPos += 2;
      if (scrollPos >= unitPx) scrollPos = 0;
      lastScrollTick = now;
    }
    display->setTextWrap(false);
    display->setCursor(-scrollPos, scrollY);
    display->print(songTitle);
    display->print("    ");
    display->print(songTitle);
    display->setTextWrap(true);
  } else {
    display->setCursor(0, scrollY);
    display->print("[Press Enc for Menu]");
  }
}

// ************************************************************
// Status screen input handler
// ************************************************************
bool handleStatusInput(ButtonEvent event) {
  if (event == BTN_CONFIRM_CLICK) {
    radioOutputManager.togglePlay();
    return true;  // Consume event
  }
  return false;  // Let default handling (encoder click = menu)
}

// ************************************************************
// Status screen encoder handler (volume control)
// ************************************************************
void handleStatusEncoder(int delta) {
  volume += delta * 2;
  if (volume > 100) volume = 100;
  if (volume < 0) volume = 0;
  radioOutputManager.setVolume(volume);
}

// ************************************************************
// Menu loop functions
// ************************************************************
void menuOncePerLoop() {
  menuSystem.update();
}

void menuOncePerSecond() {
  // Update status data
  radioStatus.wifiConnected = WiFi.isConnected();
  if (WiFi.isConnected()) {
    strncpy(radioStatus.ipAddress, WiFi.localIP().toString().c_str(), 15);
    radioStatus.ipAddress[15] = '\0';
  }
}
