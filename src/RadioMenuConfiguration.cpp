#include "RadioMenuConfiguration.h"

// ************************************************************
// Menu system instance and state
// ************************************************************
MenuSystem menuSystem;
StatusData radioStatus = {false, "0.0.0.0"};
MenuItem* mainMenu = nullptr;
MenuItem* audioMenu = nullptr;
MenuItem* stationsMenu = nullptr;
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
    radioOutputManager.startRadioStream(url, name, gain, idx);
  }
  buildStationsMenuDynamic();
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
  // Snapshot the live playback settings so they are restored on next boot
  cc->volume = volume;
  cc->selectedStation = radioOutputManager.getStationIndex();
  spiffsStorage.saveConfigToSpiffs();
}

// Push the configured blanking timeout into the menu system.
// 0 minutes means the OLED is never blanked.
void applyOledBlanking() {
  if (cc->oledBlankMins > 0) {
    menuSystem.enableScreenSaver((unsigned long)cc->oledBlankMins * 60000UL);
  } else {
    menuSystem.disableScreenSaver();
  }
}

void saveOledBlankingCb() {
  applyOledBlanking();
  spiffsStorage.saveConfigToSpiffs();
}

void savePlayOnStartCb() {
  spiffsStorage.saveConfigToSpiffs();
}

void saveAudioBalanceCb() {
  radioOutputManager.setBalance(cc->audioBalance);
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

  menuSystem.addAction(audioMenu, "Play", startPlaying);
  menuSystem.addAction(audioMenu, "Stop", stopPlaying);
  menuSystem.addNumericValue(audioMenu, "Balance (0=mid)", &cc->audioBalance,
                             -MAX_AUDIO_BALANCE, MAX_AUDIO_BALANCE, AUDIO_BALANCE_STEP,
                             "L/R", saveAudioBalanceCb);
  menuSystem.addEitherOr(audioMenu, "Play at Start", &cc->playOnStart, "ON", "OFF", savePlayOnStartCb);

  menuSystem.navigateToMenu(audioMenu);
}

// ************************************************************
// Build the Stations menu (the station list)
// ************************************************************
void buildStationsMenuDynamic() {
  if (!stationsMenu) return;

  // Clear existing items and repopulate
  menuSystem.clearMenuItems(stationsMenu);

  int count = spiffsStorage.getStationCount();
  for (int i = 0; i < count && i < MAX_MENU_STATIONS; i++) {
    String name, url;
    spiffsStorage.getStation(i, name, url);
    strncpy(stationNameBufs[i], name.c_str(), sizeof(stationNameBufs[0]) - 1);
    stationNameBufs[i][sizeof(stationNameBufs[0]) - 1] = '\0';
    menuSystem.addAction(stationsMenu, stationNameBufs[i], stationCallbacks[i]);
  }

  menuSystem.navigateToMenu(stationsMenu);
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
  menuSystem.addNumericValue(systemMenu, "Blank (0=off)", &cc->oledBlankMins,
                             0, MAX_OLED_BLANK_MINS, 1, "min", saveOledBlankingCb);
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
  stationsMenu = menuSystem.createMenu("Stations");
  {
    int count = spiffsStorage.getStationCount();
    for (int i = 0; i < count && i < MAX_MENU_STATIONS; i++) {
      String name, url;
      spiffsStorage.getStation(i, name, url);
      strncpy(stationNameBufs[i], name.c_str(), sizeof(stationNameBufs[0]) - 1);
      stationNameBufs[i][sizeof(stationNameBufs[0]) - 1] = '\0';
      menuSystem.addAction(stationsMenu, stationNameBufs[i], stationCallbacks[i]);
    }
  }

  audioMenu = menuSystem.createMenu("Audio");
  menuSystem.addAction(audioMenu, "Play", startPlaying);
  menuSystem.addAction(audioMenu, "Stop", stopPlaying);
  menuSystem.addNumericValue(audioMenu, "Balance (0=mid)", &cc->audioBalance,
                             -MAX_AUDIO_BALANCE, MAX_AUDIO_BALANCE, AUDIO_BALANCE_STEP,
                             "L/R", saveAudioBalanceCb);
  menuSystem.addEitherOr(audioMenu, "Play at Start", &cc->playOnStart, "ON", "OFF", savePlayOnStartCb);

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
  menuSystem.addNumericValue(systemMenu, "Blank (0=off)", &cc->oledBlankMins,
                             0, MAX_OLED_BLANK_MINS, 1, "min", saveOledBlankingCb);
  #ifdef DEBUG
  menuSystem.addAction(systemMenu, "Debug 10m", debugOn10minsCb);
  #endif
  menuSystem.addAction(systemMenu, "Reset WiFi", resetWiFiInfoCb);

  menuSystem.addSubmenu(mainMenu, "Stations", stationsMenu);
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

// ************************************************************
// Geek-info overlay — cycled by repeated back-button presses
// ************************************************************
enum GeekInfoPage {
  GEEK_PAGE_SYSTEM = 0,
  GEEK_PAGE_WIFI,
  GEEK_PAGE_STATION,
  GEEK_PAGE_COUNT
};

// How long an info page stays up after the last back-button press
static const unsigned long GEEK_INFO_TIMEOUT_MS = 2000;

// Timestamp (millis) until which the geek-info overlay should be shown.
static unsigned long geekInfoUntil = 0;
// Which page the overlay is currently showing
static uint8_t geekInfoPage = GEEK_PAGE_SYSTEM;

// Characters that fit across the display at text size 1 (6 px per character)
static const uint8_t GEEK_LINE_CHARS = SCREEN_WIDTH / 6;

// Print at most GEEK_LINE_CHARS characters of s, so a long value cannot wrap
// onto the line below it.
static void printClipped(Adafruit_SH1106G* display, const char* s) {
  char buf[GEEK_LINE_CHARS + 1];
  strncpy(buf, s, GEEK_LINE_CHARS);
  buf[GEEK_LINE_CHARS] = '\0';
  display->print(buf);
}

// Common page header: centred title with a rule under it
static void drawGeekHeader(Adafruit_SH1106G* display, uint8_t width, const char* title) {
  display->setTextSize(1);
  display->setTextColor(SH110X_WHITE);
  display->setCursor((width - strlen(title) * 6) / 2, 2);
  display->print(title);
  display->drawLine(0, 12, width, 12, SH110X_WHITE);
}

// ------------------------------------------------------------
// Page 1 — System Info
// ------------------------------------------------------------
static void renderSystemInfoPage(Adafruit_SH1106G* display, uint8_t width) {
  drawGeekHeader(display, width, "-- System Info --");

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

  // Total play time over the lifetime of the unit
  display->setCursor(0, 56);
  snprintf(buf, sizeof(buf), "Play: %luh %02lum", cs->playtimeMins / 60, cs->playtimeMins % 60);
  display->print(buf);
}

// ------------------------------------------------------------
// Page 2 — WiFi Info
// ------------------------------------------------------------
static void renderWifiInfoPage(Adafruit_SH1106G* display, uint8_t width) {
  drawGeekHeader(display, width, "--- WiFi Info ---");

  char buf[32];

  if (WiFi.isConnected()) {
    // SSID of the AP we are joined to
    display->setCursor(0, 16);
    snprintf(buf, sizeof(buf), "SSID: %s", WiFi.SSID().c_str());
    printClipped(display, buf);

    // Our address and the gateway we route through
    display->setCursor(0, 26);
    snprintf(buf, sizeof(buf), "IP: %s", WiFi.localIP().toString().c_str());
    printClipped(display, buf);

    display->setCursor(0, 36);
    snprintf(buf, sizeof(buf), "GW: %s", WiFi.gatewayIP().toString().c_str());
    printClipped(display, buf);

    // Signal strength, with a rough percentage (-90 dBm = 0%, -30 dBm = 100%)
    int rssi = WiFi.RSSI();
    int quality = (rssi + 90) * 100 / 60;
    if (quality < 0) quality = 0;
    if (quality > 100) quality = 100;
    display->setCursor(0, 46);
    snprintf(buf, sizeof(buf), "Sig: %ddBm (%d%%)", rssi, quality);
    printClipped(display, buf);

    // Our own MAC - exactly fills the line
    display->setCursor(0, 56);
    snprintf(buf, sizeof(buf), "MAC:%s", WiFi.macAddress().c_str());
    printClipped(display, buf);
  } else {
    display->setCursor(0, 16);
    display->print("Not connected");

    // The credentials we would use on the next connection attempt
    display->setCursor(0, 26);
    snprintf(buf, sizeof(buf), "SSID: %s", cc->WiFiSSID.c_str());
    printClipped(display, buf);

    display->setCursor(0, 36);
    snprintf(buf, sizeof(buf), "Saved: %s", wifiManager.wifiCredentialsReceived() ? "Yes" : "No");
    printClipped(display, buf);

    display->setCursor(0, 46);
    snprintf(buf, sizeof(buf), "At start: %s", cc->WifiOnAtStart ? "ON" : "OFF");
    printClipped(display, buf);

    display->setCursor(0, 56);
    snprintf(buf, sizeof(buf), "MAC:%s", WiFi.macAddress().c_str());
    printClipped(display, buf);
  }
}

// ------------------------------------------------------------
// Page 3 — Station Info
// ------------------------------------------------------------
static void renderStationInfoPage(Adafruit_SH1106G* display, uint8_t width) {
  drawGeekHeader(display, width, "-- Station Info --");

  char buf[32];

  // Station name, and its position in the station list
  String name = radioOutputManager.getStationName();
  display->setCursor(0, 16);
  if (name.length() > 0) {
    printClipped(display, name.c_str());
  } else {
    display->print("No station");
  }

  display->setCursor(0, 26);
  snprintf(buf, sizeof(buf), "Station: %d of %d",
           radioOutputManager.getStationIndex() + 1, spiffsStorage.getStationCount());
  printClipped(display, buf);

  // URL over two lines - long URLs are simply clipped
  String url = radioOutputManager.getUrl();
  display->setCursor(0, 36);
  printClipped(display, url.c_str());
  if (url.length() > GEEK_LINE_CHARS) {
    display->setCursor(0, 46);
    printClipped(display, url.c_str() + GEEK_LINE_CHARS);
  }

  // Playback state and how full the stream buffer is
  display->setCursor(0, 56);
  const char* state = radioOutputManager.isPlaying()      ? "Playing"
                    : radioOutputManager.isReconnecting() ? "Resync"
                                                          : "Stopped";
  snprintf(buf, sizeof(buf), "%s  Buf:%d%%", state, radioOutputManager.getBufferFillPercent());
  printClipped(display, buf);
}

// ************************************************************
// Status screen renderer
// ************************************************************
void renderRadioStatus(Adafruit_SH1106G* display, uint8_t width, uint8_t height) {

  // Geek-info overlay - shown for a short while after the back button is pressed
  if (millis() < geekInfoUntil) {
    switch (geekInfoPage) {
      case GEEK_PAGE_WIFI:    renderWifiInfoPage(display, width);    break;
      case GEEK_PAGE_STATION: renderStationInfoPage(display, width); break;
      default:                renderSystemInfoPage(display, width);  break;
    }
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
    // While an info page is up, each press moves on to the next one; once it
    // has timed out the next press starts again at the system page.
    if (millis() < geekInfoUntil) {
      geekInfoPage = (geekInfoPage + 1) % GEEK_PAGE_COUNT;
    } else {
      geekInfoPage = GEEK_PAGE_SYSTEM;
    }
    geekInfoUntil = millis() + GEEK_INFO_TIMEOUT_MS;
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
