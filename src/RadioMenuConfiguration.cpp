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
static const int MAX_MENU_STATIONS = 9;
// Persistent name buffers — kept alive between menu rebuilds so the menu
// system can hold const char* pointers without copying them internally.
static char stationNameBufs[MAX_MENU_STATIONS][48];

static void playStation(int idx) {
  String name, url;
  if (spiffsStorage.getStation(idx, name, url)) {
    float gain = (volume / 100.0f) * MAX_GAIN;
    radioOutputManager.startRadioStream(url, name, gain);
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
// Build the Audio menu (station list + stop)
// ************************************************************
void buildAudioMenuDynamic() {
  if (!audioMenu) return;

  // Clear existing items and repopulate
  menuSystem.clearMenuItems(audioMenu);

  int count = spiffsStorage.getStationCount();
  for (int i = 0; i < count && i < MAX_MENU_STATIONS; i++) {
    String name, url;
    spiffsStorage.getStation(i, name, url);
    strncpy(stationNameBufs[i], name.c_str(), sizeof(stationNameBufs[0]) - 1);
    stationNameBufs[i][sizeof(stationNameBufs[0]) - 1] = '\0';
    menuSystem.addAction(audioMenu, stationNameBufs[i], stationCallbacks[i]);
  }
  menuSystem.addAction(audioMenu, "Stop", stopPlaying);

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
    char ipInfoBuf[20];
    snprintf(ipInfoBuf, sizeof(ipInfoBuf), "IP: %s", WiFi.localIP().toString().c_str());
    menuSystem.addInfo(wifiMenu, ipInfoBuf);
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
  {
    int count = spiffsStorage.getStationCount();
    for (int i = 0; i < count && i < MAX_MENU_STATIONS; i++) {
      String name, url;
      spiffsStorage.getStation(i, name, url);
      strncpy(stationNameBufs[i], name.c_str(), sizeof(stationNameBufs[0]) - 1);
      stationNameBufs[i][sizeof(stationNameBufs[0]) - 1] = '\0';
      menuSystem.addAction(audioMenu, stationNameBufs[i], stationCallbacks[i]);
    }
    menuSystem.addAction(audioMenu, "Stop", stopPlaying);
  }

  wifiMenu = menuSystem.createMenu("WiFi");
  if (WiFi.isConnected()) {
    char ipInfoBuf[20];
    snprintf(ipInfoBuf, sizeof(ipInfoBuf), "IP: %s", WiFi.localIP().toString().c_str());
    menuSystem.addInfo(wifiMenu, ipInfoBuf);
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
// Status icon drawing helpers (w x h scalable)
// ************************************************************
static void drawPlayIcon(Adafruit_SH1106G* display, uint8_t x, uint8_t y, uint8_t w, uint8_t h) {
  display->fillTriangle(x, y, x, y + h - 1, x + w - 1, y + h / 2, SH110X_WHITE);
}

static void drawStopIcon(Adafruit_SH1106G* display, uint8_t x, uint8_t y, uint8_t w, uint8_t h) {
  display->fillRect(x + 2, y + 2, w - 4, h - 4, SH110X_WHITE);
}

static void drawResyncIcon(Adafruit_SH1106G* display, uint8_t x, uint8_t y, uint8_t w, uint8_t h) {
  uint8_t cx = x + w / 2, cy = y + h / 2;
  uint8_t r  = (w < h ? w : h) / 2 - 2;
  display->drawCircle(cx, cy, r, SH110X_WHITE);
  display->fillTriangle(cx + r - 2, cy - r, cx + r + 3, cy - r, cx + r, cy - r + 4, SH110X_WHITE);
}

// Timestamp (millis) until which the geek-info overlay should be shown.
static unsigned long geekInfoUntil = 0;

// ************************************************************
// Status screen renderer
// ************************************************************
void renderRadioStatus(Adafruit_SH1106G* display, uint8_t width, uint8_t height) {

  // Geek-info overlay — shown for 2 s after the back button is pressed
  if (millis() < geekInfoUntil) {
    display->setTextSize(1);
    display->setTextColor(SH110X_WHITE);

    display->setCursor(20, 2);
    display->print("-- System Info --");
    display->drawLine(0, 12, width, 12, SH110X_WHITE);

    char buf[24];

    // IP address
    display->setCursor(0, 16);
    display->print("IP: ");
    display->print(WiFi.isConnected() ? WiFi.localIP().toString() : "Not connected");

    // Free heap
    display->setCursor(0, 26);
    snprintf(buf, sizeof(buf), "Heap: %lu KB", ESP.getFreeHeap() / 1024);
    display->print(buf);

    // Streams played
    display->setCursor(0, 36);
    snprintf(buf, sizeof(buf), "Streams: %lu", (unsigned long)radioOutputManager.getStreamsPlayed());
    display->print(buf);

    // Frames decoded (show in thousands)
    display->setCursor(0, 46);
    snprintf(buf, sizeof(buf), "Frames: %luk", (unsigned long)(radioOutputManager.getFramesDecoded() / 1000));
    display->print(buf);

    return;
  }
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

  // Layout: 16x16 icon centred vertically across all 3 rows; text/bars indented past it
  const uint8_t contentY = yPos;                              // = 26
  const uint8_t iconX    = 3;                                 // 3 px left margin
  const uint8_t iconW    = 16;
  const uint8_t iconH    = 16;
  const uint8_t rowH     = 10;
  const uint8_t indent   = iconX + iconW + 7;                 // 3 left + 16 icon + 7 right gap = 26
  const uint8_t barH     = 5;
  // content area height = rowH*2 + barH = 25; centre icon within it
  const uint8_t iconY    = contentY + (rowH * 2 + barH - iconH) / 2;

  // Row 1: status text (indented); icon centred vertically beside all 3 rows
  if (radioOutputManager.isPlaying()) {
    drawPlayIcon(display, iconX, iconY, iconW, iconH);
    display->setCursor(indent, contentY + 1);
    display->print("Playing");
  } else if (radioOutputManager.isReconnecting()) {
    drawResyncIcon(display, iconX, iconY, iconW, iconH);
    display->setCursor(indent, contentY + 1);
    display->print("Resyncing");
  } else {
    drawStopIcon(display, iconX, iconY, iconW, iconH);
    display->setCursor(indent, contentY + 1);
    display->print("Stopped");
  }

  // Shared bar geometry — 90% of available width, right-aligned
  const uint8_t labelW = 6 * 6;                              // "Volume"/"Buffer" = 36 px
  const uint8_t barW   = (width - indent - labelW - 1) * 9 / 10;
  const uint8_t barX   = width - barW - 1;                   // flush to right edge

  // Row 2: Volume label + bar
  const uint8_t row2Y = contentY + rowH;
  display->setCursor(indent, row2Y);
  display->print("Volume");
  display->drawRect(barX, row2Y, barW, barH, SH110X_WHITE);
  if (volume > 0) {
    display->fillRect(barX + 1, row2Y + 1, (barW - 2) * volume / 100, barH - 2, SH110X_WHITE);
  }

  // Row 3: Buffer label + bar
  const uint8_t row3Y = contentY + rowH * 2;
  display->setCursor(indent, row3Y);
  display->print("Buffer");
  display->drawRect(barX, row3Y, barW, barH, SH110X_WHITE);
  if (radioOutputManager.isPlaying()) {
    int fillPct = radioOutputManager.getBufferFillPercent();
    if (fillPct > 0) {
      display->fillRect(barX + 1, row3Y + 1, (barW - 2) * fillPct / 100, barH - 2, SH110X_WHITE);
    }
  }

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
    return true;
  }
  if (event == BTN_BACK_CLICK) {
    geekInfoUntil = millis() + 2000;
    return true;
  }
  return false;
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
