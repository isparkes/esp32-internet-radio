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
void setStation() {
  radioOutputManager.startRadioStream("http://mp3.ffh.de/radioffh/hqlivestream.mp3", "FFH", 1.0f);
  buildAudioMenuDynamic();
}

void startPlaying() {
  radioOutputManager.StartPlaying();
  buildAudioMenuDynamic();
}

void stopPlaying() {
  radioOutputManager.StopPlaying();
  buildAudioMenuDynamic();
}

void switchToBluetoothMode() {
  radioOutputManager.setAudioMode(AUDIO_MODE_BLUETOOTH);
  buildAudioMenuDynamic();
}

void switchToRadioMode() {
  radioOutputManager.setAudioMode(AUDIO_MODE_RADIO);
  buildAudioMenuDynamic();
}

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
    menuSystem.addAction(audioMenu, "Set Station", setStation);
    menuSystem.addAction(audioMenu, "Start", startPlaying);
    menuSystem.addAction(audioMenu, "Stop", stopPlaying);
    menuSystem.addAction(audioMenu, "Switch to BT", switchToBluetoothMode);
  } else {
    menuSystem.addInfo(audioMenu, "Mode: Bluetooth");
    if (bluetoothManager.isBluetoothConnected()) {
      menuSystem.addInfo(audioMenu, "Status: Connected");
    } else {
      menuSystem.addInfo(audioMenu, "Status: Waiting...");
    }
    menuSystem.addAction(audioMenu, "Switch to Radio", switchToRadioMode);
  }

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
    menuSystem.addAction(audioMenu, "Set Station", setStation);
    menuSystem.addAction(audioMenu, "Start", startPlaying);
    menuSystem.addAction(audioMenu, "Stop", stopPlaying);
    menuSystem.addAction(audioMenu, "Switch to BT", switchToBluetoothMode);
  } else {
    menuSystem.addInfo(audioMenu, "Mode: Bluetooth");
    menuSystem.addInfo(audioMenu, "Status: Waiting...");
    menuSystem.addAction(audioMenu, "Switch to Radio", switchToRadioMode);
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
  } else {
    display->print("BT: ");
    display->print(bluetoothManager.isBluetoothConnected() ? "Connected" : "Waiting...");
  }
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

  // Hint at bottom
  display->setCursor(0, height - 10);
  display->print("[Press Enc for Menu]");
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
