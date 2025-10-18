#pragma once

// Uses PlatformIO package ESP32 v6.9.0

// -------------------------------------------------------------------------------
// This files hold high level hardware configurations
// -------------------------------------------------------------------------------


// -------------------------------------------------------------------------------
#define SOFTWARE_VERSION "INR-ESP32 0.0.1.0"

// Add debug statments to code - needs extra space
#define DEBUG                       // DEBUG | DEBUG_OFF

// -------------------------------------------------------------------------------

// Define the type of OLED
#define OLED_SH1106                 // OLED_SH1106 (1.3") |  OLED_SSD1306 (0.96" and 2.4")

// -------------------------------------------------------------------------------

#define FEATURE_MENU

// -------------------------------------------------------------------------------

#define CLOCK_MENU_TITLE "INet Radio"

#define MAX_STATIONS 9                              // Max number of stations in station list

#define MAX_GAIN 1.20                               // Max gain value before we clip
#define VOLUME_STEPS 10                             // Number of volume steps
#define VOLUME_STEP_SIZE (MAX_GAIN / VOLUME_STEPS)  // Volume step size
#define DEFAULT_GAIN VOLUME_STEP_SIZE*3             // Default gain on start up