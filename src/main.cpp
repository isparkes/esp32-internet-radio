#include <WiFi.h>
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h>

#include "main.h"
#include "Globals.h"
#include "utilities.h"

const int bufferSize = 16 * 1024; // buffer in byte, default 16 * 1024 = 16kb

String arrayURL[MAX_STATIONS] = {
  "http://mp3.ffh.de/radioffh/hqlivestream.mp3",
  "http://stream.laut.fm/numanme",
  "http://stream.governamerica.com:4030/index.html/128k.mp3%3Ftype=http&nocache=46079",
  "http://kvbstreams.dyndns.org:8000/wkvi-am",
  "http://hoth.alonhosting.com:1100/stream/1/",
  "http://radio.punjabrocks.com:9998/stream/1/",
  "http://server.mixify.in:8000/stream/1/",
  "http://142.4.219.8:8255/stream/1/",
  "http://mehefil.no-ip.com/stream/1/",
};

String arrayStation[MAX_STATIONS] = {
  "Radio FFH",
  "Numanme Radio",
  "Govern America",
  "WKVI Radio",
  "BollyHits Radio",
  "Taal Radio",
  "Mixify",
  "Bollywood Gold",
  "Mehefil Radio"
};

uint32_t LastTime = 0;
int playflag = 0;
int ledflag = 0;
float fgain = DEFAULT_GAIN;
int sflag = 0;
String URL = arrayURL[sflag];
String station = arrayStation[sflag];

byte b=2;
bool inv=0;



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

  debugMsgInr("Start up GPIOs");
//  pinMode(BTNCONF, INPUT_PULLUP);
//  pinMode(BTNBACK, INPUT_PULLUP);

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
  
  // Default pins SDA 21, SCL 22 Frequency 400kHz 
  debugMsgInr("Start up I2C...");
  Wire.begin(SDAint, SCLint, 400000L);

  // -------------------------------------------------------------------------
  
  #ifdef FEATURE_MENU
  debugMsgInr("Starting OLED");
  oled.setUp();
  oled.clearDisplay();
  menuManager.flashMenuMessage(CLOCK_MENU_TITLE, "Starting");
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

  // display.clearDisplay();
  // display.drawBitmap(0,0,startScreen,128,64,1);
  // display.display();
  // delay(1000);
  // debugMsgInr("STATUS(System) Ready");

  // -------------------------------------------------------------------------
  
  debugMsgInr("Initialising Audio");

  radioOutputManager.initializeAudioOutput();

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
  //   if (digitalRead(BTNCONF) ==  LOW) {
  //     StartPlaying();
  //     playflag = 1;
  //   }

  //   if (digitalRead(BTNBACK) ==  LOW) {
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

  // if (playflag == 1) {
  //   if (mp3->isRunning()) {
  //     if (millis() - lastms > 1000) {
  //       lastms = millis();
  //       debugMsgInrX("STATUS(Streaming) " + String(lastms) + " ms...");
  //       debugMsgInrX("Buffer " + String(buff->getFillLevel()) + "/" + String(bufferSize));
  //     }
  //     if (!mp3->loop()) mp3->stop();
  //   } else {
  //     debugMsgInr("MP3 done");
  //     playflag = 0; 
  //   }
  //   if (digitalRead(BTNCONF) ==  LOW) {
  //     StopPlaying();
  //     display.clearDisplay();
  //     display.drawBitmap(0,0,startScreen,128,64,1);
  //     display.display();
  //     playflag = 0;
  //     delay(200);
  //   }
  //   if (digitalRead(BTNBACK) ==  LOW) {
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

// void StartPlaying() {
//   debugMsgInr("Start play");
//   frame = 0;
//   while(frame < 27){
//     display.clearDisplay();
//     display.drawBitmap(48, 16, play[frame], FRAME_WIDTH, FRAME_HEIGHT, 1);
//     display.display();
//     frame = (frame + 1) % FRAME_COUNT;
//     debugMsgInrX(frame);
//     delay(FRAME_DELAY);
//   }
//   file = new AudioFileSourceICYStream(URL.c_str());
//   file->RegisterMetadataCB(MDCallback, (void*)"ICY");
//   buff = new AudioFileSourceBuffer(file, bufferSize);
//   buff->RegisterStatusCB(StatusCallback, (void*)"buffer");
//   out = new AudioOutputI2S(); //
//   out->SetPinout(33, 14, 25); // (int bclk, int lrc, int dout)
//   out->SetOutputModeMono(true);
//   out->SetGain(fgain);
//   mp3 = new AudioGeneratorMP3();
//   mp3->RegisterStatusCB(StatusCallback, (void*)"mp3");
//   mp3->begin(buff, out);

//   debugMsgInr("STATUS(URL) " + String(URL));
// }

// void StopPlaying() {
//   frame = 0;
//   while(frame < 27){
//     display.clearDisplay();
//     display.drawBitmap(48, 16, stop[frame], FRAME_WIDTH, FRAME_HEIGHT, 1);
//     display.display();
//     frame = (frame + 1) % FRAME_COUNT;
//     debugMsgInrX(frame);
//     delay(FRAME_DELAY);
//   }
//   if (mp3) {
//     mp3->stop();
//     delete mp3;
//     mp3 = NULL;
//   }
//   if (buff) {
//     buff->close();
//     delete buff;
//     buff = NULL;
//   }
//   if (file) {
//     file->close();
//     delete file;
//     file = NULL;
//   }
//   debugMsgInrX("STATUS(Stopped)");
// }


// void MDCallback(void *cbData, const char *type, bool isUnicode, const char *string) {
//   const char *ptr = reinterpret_cast<const char *>(cbData);
//   (void) isUnicode; // Punt this ball for now
//   // Note that the type and string may be in PROGMEM, so copy them to RAM for printf
//   char s1[32], s2[64];
//   strncpy_P(s1, type, sizeof(s1));
//   s1[sizeof(s1) - 1] = 0;
//   strncpy_P(s2, string, sizeof(s2));
//   s2[sizeof(s2) - 1] = 0;
//   display.clearDisplay();
//   display.setTextSize(1);      // Normal 1:1 pixel scale
//   display.setTextColor(1); // Draw white text
//   display.setCursor(0, 0);
//   display.println(station);
//   display.drawFastHLine(0, 10, 128, 1);
//   display.setCursor(0, 20);
//   display.println(s2);
//   display.display();
//   debugMsgInr("METADATA(" + String(ptr) + ") '" + String(s1) + "' = '" + String(s2));                                                                      
// }

// void StatusCallback(void *cbData, int code, const char *string) {
//   const char *ptr = reinterpret_cast<const char *>(cbData);
//   // Note that the string may be in PROGMEM, so copy it to RAM for printf
//   char s1[64];
//   strncpy_P(s1, string, sizeof(s1));
//   s1[sizeof(s1) - 1] = 0;
//   debugMsgInr("STATUS(" + String(ptr) + ") '" + String(code) + "' = '" + String(s1));
// }

// ************************************************************
// Called every 10mS or so
// ************************************************************
void performOncePerLoopProcessing() {
  
  // -------------------------------------------------------------------------------

  #ifdef FEATURE_MENU
  menuManager.menuOncePerLoop();
  #endif

  // -------------------------------------------------------------------------------

  wifiManager.manageDNSInOpenAP();

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
  } else {
    setLedFlashType(1);
  }

  // -------------------------------------------------------------------------------

  // Service the menu
  #ifdef FEATURE_MENU
  menuManager.menuOncePerSecond();
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