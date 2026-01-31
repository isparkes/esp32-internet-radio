#include "Globals.h"

// ************************************************************
// Global shared components and objects
// ************************************************************
esp_wps_config_t wps_config;

// ************************************************************
// Shared config objects
// ************************************************************
spiffs_config_t current_config;
spiffs_stats_t current_stats;

spiffs_config_t* cc = &current_config;
spiffs_stats_t* cs = &current_stats;

// ************************************************************
// Display values
// ************************************************************

// Makes sure we are not writing to the display buffer
// when the interrupt is reading it
portMUX_TYPE timerMux1 = portMUX_INITIALIZER_UNLOCKED;

// Used for clean handling of the encoder 
portMUX_TYPE encoderMux = portMUX_INITIALIZER_UNLOCKED;


float n=0;
int frame;
int volume = 10;
unsigned long lastEncoderMillis = 0;

// ************************************************************
// General values
// ************************************************************
unsigned long nowMillis = 0;
unsigned long previousMillisWiFi = 0;
unsigned long lastSecondStartMillis = 0;
int lastSecond = 0;
boolean triggeredThisSec = false;
int secsDeltaAbs;                   // Sawtooth 0..1000 every sec
int secsDelta;                      // Triangle 0..1000..0 every 2 secs
bool upOrDown;  

bool blanked = false;

// Station list
station_t stations[MAX_STATIONS];
int stationCount = 0;

bool doAutoReconnect = false;

bool encoderToggle = false;

// Our network name
String uniqHostname;

// singleton object
AsyncWebServer server(80);

String lastWiFiScan = "";

unsigned long confBtnReadMillis = 0;
unsigned long backBtnReadMillis = 0;

// ------------------ Global functions -----------------

void updateNowMillis() {
    nowMillis = millis();
}
