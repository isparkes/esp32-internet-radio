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

// String arrayURL[MAX_STATIONS] = {
//   "http://mp3.ffh.de/radioffh/hqlivestream.mp3",
//   "http://stream.laut.fm/numanme",
//   "http://stream.governamerica.com:4030/index.html/128k.mp3%3Ftype=http&nocache=46079",
//   "http://kvbstreams.dyndns.org:8000/wkvi-am",
//   "http://hoth.alonhosting.com:1100/stream/1/",
//   "http://radio.punjabrocks.com:9998/stream/1/",
//   "http://server.mixify.in:8000/stream/1/",
//   "http://142.4.219.8:8255/stream/1/",
//   "http://mehefil.no-ip.com/stream/1/",
// };

// String arrayStation[MAX_STATIONS] = {
//   "Radio FFH",
//   "Numanme Radio",
//   "Govern America",
//   "WKVI Radio",
//   "BollyHits Radio",
//   "Taal Radio",
//   "Mixify",
//   "Bollywood Gold",
//   "Mehefil Radio"
// };


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

  radioOutputManager.initializeAudioOutput();

  // -------------------------------------------------------------------------

  debugMsgInr("Initialising Bluetooth");

  bluetoothManager.initializeBluetooth();

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

  // if (playflag == 0) {
  //   if (digitalRead(BTN_CONFIRM) ==  LOW) {
  //     StartPlaying();
  //     playflag = 1;
  //   }

  //   if (digitalRead(BTN_BACK) ==  LOW) {
  //     sflag = (sflag + 1) % MAX_STATIONS;
  //     URL = arrayURL[sflag];
  //     station = arrayStation[sflag];  
  //     debugMsgInr(station);
  //     display.clearDisplay();
  //     display.setTextSize(1);      // Normal 1:1 pixel scale
  //     display.setTextColor(1); // Draw white text
  //     display.setCursor(0, 20);
  //     display.println(station);
  //     display.display();             
  //     delay(300);
  //   }
  // }

  //   if (digitalRead(BTN_CONFIRM) ==  LOW) {
  //     StopPlaying();
  //     display.clearDisplay();
  //     display.drawBitmap(0,0,startScreen,128,64,1);
  //     display.display();
  //     playflag = 0;
  //     delay(200);
  //   }
  //   if (digitalRead(BTN_BACK) ==  LOW) {
  //     fgain = fgain + VOLUME_STEP_SIZE;
  //     if (fgain > MAX_GAIN) {
  //       fgain = VOLUME_STEP_SIZE;
  //     }
  //     out->SetGain(fgain);
  //     debugMsgInr("STATUS(Gain) " + String(fgain));
  //     delay(200);
  //   }
  // }
}




// ************************************************************
// Called every 10mS or so
// ************************************************************
void performOncePerLoopProcessing() {
  
  // -------------------------------------------------------------------------------
  // Audio loop must run as frequently as possible to avoid choppy playback
  radioOutputManager.audioOncePerLoop();

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