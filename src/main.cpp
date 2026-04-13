#include <WiFi.h>
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h>

#include "main.h"
#include "Globals.h"
#include "utilities.h"
#include "BluetoothManager.h"
#include "RadioMenuConfiguration.h"

// ************************************************************
// Set up the unit
// ************************************************************
void setup() {
  Serial.begin(SERIAL_BAUD_RATE);
  
  #ifdef DEBUG
  // Debug for 10 minutes
  debugManager.setDebugAutoOff(600);
  #endif

  // -------------------------------------------------------------------------

  nowMillis = millis();

  // -------------------------------------------------------------------------

  // Check for PSRAM
  if (psramFound()) {
    debugMsgInr("PSRAM found: " + String(ESP.getPsramSize() / 1024) + " KB total, " + String(ESP.getFreePsram() / 1024) + " KB free");
  } else {
    debugMsgInr("WARNING: No PSRAM detected");
  }

  // -------------------------------------------------------------------------

  debugMsgInr("Start up SPIFFS");

  // Initialize SPIFFS
  if(!SPIFFS.begin(true)){
    debugMsgInr("An Error has occurred while mounting SPIFFS");
    return;
  }

  bool statsLoaded = spiffsStorage.getStatsFromSpiffs();

  if (!statsLoaded) {
    debugMsgInr("SPIFFS storage: read stats failed");
    spiffsStorage.saveStatsToSpiffs();
  }

  bool configloaded = spiffsStorage.getConfigFromSpiffs();

  if (configloaded) {
    debugMsgInr("SPIFFS storage: Loaded");
  } else {
    debugMsgInr("SPIFFS storage: read config failed - do factory reset");
    resetOptions();
    spiffsStorage.saveConfigToSpiffs();
  }

  // Load station list
  spiffsStorage.getStationsFromSpiffs();
  debugMsgInr("Loaded " + String(stationCount) + " stations");

  // -------------------------------------------------------------------------

  debugMsgInr("Start up Timers");

  // Starts the display and the status LED flashing
  startTimers();

  // -------------------------------------------------------------------------
  
  // -------------------------------------------------------------------------

  #ifdef FEATURE_MENU
  debugMsgInr("Starting Menu System");
  if (!menuSystem.begin(SDAint, SCLint,
                       PIN_ENC_CLK, PIN_ENC_DT,
                       PIN_BTN_CONFIRM, PIN_BTN_BACK,
                       PIN_ENC_SW)) {
    debugMsgInr("Failed to initialize menu system!");
  } else {
    debugMsgInr("Menu system initialized successfully");
    buildRadioMenus();
    menuSystem.setRootMenu(mainMenu);
    menuSystem.setStatusData(&radioStatus);
    menuSystem.setStatusRenderCallback(renderRadioStatus);
    menuSystem.setStatusInputCallback(handleStatusInput);
    menuSystem.setStatusEncoderCallback(handleStatusEncoder);
    menuSystem.setMenuTimeout(10000);
    menuSystem.showStatusScreen();
  }
  #endif

  // -------------------------------------------------------------------------
  
  debugMsgInr("Initialising WiFi");
  wifiManager.setUpWiFi();

  if (cc->WifiOnAtStart && wifiManager.wifiCredentialsReceived()) {
    debugMsgInr("Connecting to previous AP");    
    wifiManager.connectToLastAP();
  } else {
    if (!cc->WifiOnAtStart) {
      debugMsgInr("Skipping connect to previous AP - told not to");
    } else if (!wifiManager.wifiCredentialsReceived()) {
      debugMsgInr("Skipping connect to previous AP - no AP defined");
    }
  }

  // -------------------------------------------------------------------------
  
  debugMsgInr("Start timers");
  startTimers();

  // -------------------------------------------------------------------------

  debugMsgInr("Initialising Audio");

  // The audio task runs on core 1. mp3->loop() can block on network I/O,
  // which would starve the core 1 idle task and trigger its WDT.
  // Core 0 is left entirely to the BT stack and WiFi.
  disableCore1WDT();

  radioOutputManager.initializeAudioOutput();

  // -------------------------------------------------------------------------

#ifdef FEATURE_BLUETOOTH
  debugMsgInr("Initialising Bluetooth");
  bluetoothManager.initializeBluetooth();
#endif

  // -------------------------------------------------------------------------

  debugMsgInr("Startup done");
}


// ************************************************************
// Main loop
// ************************************************************
void loop() {


  nowMillis = millis();

  if (lastSecondStartMillis > nowMillis) {
    // rollover
    lastSecondStartMillis = 0;
  }

  // -------------------------------------------------------------------------------

  performOncePerLoopProcessing();

  if (lastSecond != second()) {
    lastSecond = second();
    performOncePerSecondProcessing();

    if ((second() == 0) && (!triggeredThisSec)) {
      if ((minute() == 0)) {
        if (hour() == 0) {
          performOncePerDayProcessing();
        }
        performOncePerHourProcessing();
      }
      performOncePerMinuteProcessing();
    }

    // Make sure we don't call multiple times
    triggeredThisSec = true;
    if ((second() > 0) && triggeredThisSec) {
      triggeredThisSec = false;
    }
  }
}




// ************************************************************
// Called every 10mS or so
// ************************************************************
void performOncePerLoopProcessing() {
  
  // -------------------------------------------------------------------------------
  // Audio loop must run as frequently as possible to avoid choppy playback
  radioOutputManager.audioOncePerLoop();

  // -------------------------------------------------------------------------------

  // OTA polling - every 500ms is more than responsive enough
  static unsigned long lastOTACheck = 0;
  if (nowMillis - lastOTACheck >= 500) {
    lastOTACheck = nowMillis;
    webManager.handleOTA();
  }

  // -------------------------------------------------------------------------------

  wifiManager.manageDNSInOpenAP();

  // -------------------------------------------------------------------------------

  // Throttle menu/display updates - OLED I2C is slow and starves the audio decoder
  #ifdef FEATURE_MENU
  static unsigned long lastMenuUpdate = 0;
  if (nowMillis - lastMenuUpdate >= 50) {  // ~20fps is plenty for UI
    lastMenuUpdate = nowMillis;
    menuOncePerLoop();
  }
  #endif

  // -------------------------------------------------------------------------------

  // Calculate the intra second millis
  secsDeltaAbs = (nowMillis - lastSecondStartMillis);
  if (secsDeltaAbs > 1000) {secsDeltaAbs = 1000;}
  if (secsDeltaAbs < 0) {secsDeltaAbs = 0;}
  upOrDown = (second() % 2) == 0;
  
  if (upOrDown) {
    secsDelta = secsDeltaAbs;
  } else {
    secsDelta = 1000 - secsDeltaAbs;
  }
}

// ************************************************************
// Called once per second. Trigger all the things that do
// Not need processing continuously multiple times per second
// ************************************************************
void performOncePerSecondProcessing() {
  lastSecondStartMillis = nowMillis;

  // -------------------------------------------------------------------------------
  
  // Maintain the LED next to the controller
  if (WiFi.status() == WL_CONNECTED) {
    setLedFlashType(0);

    if (radioOutputManager.isPlaying()) {
      setLedFlashType(2);
    }
  } else {
    setLedFlashType(1);
  }

  // -------------------------------------------------------------------------------

  // Service the menu
  #ifdef FEATURE_MENU
  menuOncePerSecond();
  #endif

  // -------------------------------------------------------------------------------
  
  debugManager.debugAutoOffCheck();

  // -------------------------------------------------------------------------------

  feedWatchdog();
}

// ************************************************************
// Called once per minute
// ************************************************************
void performOncePerMinuteProcessing() {
  debugMsgInr("---> OncePerMinuteProcessing");
  // Usage stats
  cs->uptimeMins++;
}

// ************************************************************
// Called once per hour
// ************************************************************
void performOncePerHourProcessing() {
  debugMsgInr("---> OncePerHourProcessing");
}

// ************************************************************
// Called once per day
// ************************************************************
void performOncePerDayProcessing() {
  debugMsgInr("---> OncePerDayProcessing");

  spiffsStorage.saveStatsToSpiffs();
}