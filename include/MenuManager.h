#pragma once

#include <Arduino.h>
#include "Defs.h"
#include "OLED.h"
#include "DebugManager.h"
#include "utilities.h"
#include "WiFiManager.h"
#include "Globals.h"

#define BUTTONPRESSEDSTATE   0            // rotary encoder gpio pin logic level when the button is pressed (usually 0)
#define DEBOUNCEDELAY        100          // debounce delay for button inputs
#define MENU_ITEMS           18           // max number of items used in any of the menus (keep as low as possible to save memory)

#define OLED_ON_ALWAYS       0            // 
#define OLED_ON_SHORT        1            // 
#define OLED_ON_LONG         2            // 
#define OLED_ON_DEF          1            // Default value

#define OLED_ON_TIME         60           // 4 Hours

#define CONFIG_TIME         10            // Time in seconds we stay in config mode
#define FLASH_TIME           2            // Time in seconds we show an OLED flash message for

#define TICKS_PER_MOVE       2            // number of encoder ticks before we move one position in a menu

#define MENU_LARGE_TEXT      0            // show larger text when possible (if struggling to read the small text)
#define TOP_LINE            18            // y position of lower area of the display (18 with two colour displays)
#define LINE_SPACE_1         9            // line spacing for textsize 1 (small text)
#define LINE_SPACE_2        17            // line spacing for textsize 2 (large text)
#define DISPLAY_MAX_LINES    5            // max lines that can be displayed in lower section of display in textsize1 (5 on larger oLeds)
#define MAX_TITLE_LENGTH    10            // max characters per line when using text size 2 (usually 10)

const String CHARSET = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+@#*%&/()=?!$-_ ";
#define BACKSPACE           80
#define DONE                81
#define RESTART             82

enum clearTimeoutsType {
  clearFlashTimeout,
  clearConfigTimeout,
  clearOledTimeout,
  normalTimeouts
};

enum menuTargets {
  noTarget,
  unmappedOption,

  // Move around in menus
  backToMain,
  gotoRadioMenu,
  backToRadio,
  gotoWifiMenu,
  gotoOptionsMenu,
  menuOff,

  // WiFi management
  toggleWiFiAtStart,
  disconnectWifi,
  resetWiFiInfo,
  connectWPS,
  reconnectPrevious,
  openAccessPoint,
  smartConfig,
  scanWiFi,
  showWifiSelection,
  selectWiFiSSID,
  enterWiFiPassword,
  saveWiFiPassword,
  enterWiFiSSID,
  saveWiFiSSID,

  // System options
  restartClock,
  saveConfig,
  debugOn10mins
};

// modes that the menu system can be in
enum menuModes {
  off,                                  // display is off
  root,                                 // main menu is active
  menu,                                 // a menu is active
  value,                                // 'enter a numeric value' non blocking is active
  stringValue,                          // 'enter a string value' non blocking is active
  message                               // displaying a message
};

struct oledMenus {
  // menu
  String menuTitle = "";                    // the title of active mode
  byte noOfmenuItems = 0;                   // number if menu items in the active menu
  byte selectedMenuItem = 0;                // when a menu item is selected it is flagged here until actioned and cleared
  byte highlightedMenuItem = 0;             // which item is curently highlighted in the menu
  String menuItems[MENU_ITEMS+1];           // store for the menu item titles
  menuTargets menuActions[MENU_ITEMS+1];    // The action to carry out
  uint32_t lastMenuActivity = 0;            // time the menu last saw any activity (used for timeout)

  // 'enter a value'
  int mValueEntered = 0;                    // store for number entered by value entry menu
  int mValueLow = 0;                        // lowest allowed value
  int mValueHigh = 0;                       // highest allowed value
  int mValueStep = 0;                       // step size when encoder is turned
  menuTargets nextTarget = noTarget;        // the target to continue when a value is received
  bool needUpdate;                          // If the menu changed and needs to be output again
  String enteredString = "";
};

struct rotaryEncoders {
  volatile int encoder0Pos = 0;                          // current value selected with rotary encoder (updated by interrupt routine)
  volatile bool encoderPrevA;                            // used to debounced rotary encoder
  volatile bool encoderPrevB;                            // used to debounced rotary encoder
  uint32_t reLastButtonChange = 0;                       // last time state of button changed (for debouncing)
  bool encoderPrevButton = 0;                            // used to debounce button
  int reButtonDebounced = 0;                             // debounced current button state (1 when pressed)
  const bool reButtonPressedState = BUTTONPRESSEDSTATE;  // the logic level when the button is pressed
  const uint32_t reDebounceDelay = DEBOUNCEDELAY;        // button debounce delay setting
  bool reButtonPressed = 0;                              // flag set when the button is pressed (it has to be manually reset)
};

class MenuManager_ {
  private:
    MenuManager_() = default; // Make constructor private

  public:
    static MenuManager_ &getInstance(); // Accessor for singleton instance

    MenuManager_(const MenuManager_ &) = delete; // no copying
    MenuManager_ &operator=(const MenuManager_ &) = delete;

  public:
    void setupMenuManager();
    void ICACHE_RAM_ATTR doEncoder();
    void flashMenuMessage(String heading, String message);
    void clearFlashMenuMessage();
    void scrollMenuMessage(String message);
    void menuOncePerSecond();
    void menuOncePerHour();
    void menuOncePerLoop();
    bool getOledIsBlanked();
    void confirmButtonPress();
    void backButtonPress();
  private:
    menuModes menuMode = off;                 // default mode at startup is off
    oledMenus oledMenu;
    rotaryEncoders rotaryEncoder;
    String _chosenArea;

    // Menu  management - OLED timeouts
    int oledTimeout = OLED_ON_TIME;
    int configTimeout = 0;
    int flashTimeout = 0;

    // trigger for Oled reset
    bool resetDisplay;

    // Menus
    void rootMenu();
    void mainMenu();
    void radioMenu();
    void wifiMenu();
    void systemMenu();
    void nixieClockMenu();
    void wifiSelectMenu();
    void setTimeMenu();
    void locationAreaMenu();
    void locationMenu();

    void menuActions(menuTargets selectedAction);
    void setIntegerValue(String title, int startValue, int minValue, int maxValue, menuTargets target);
    void setStringValue(String title, menuTargets target, String intialValue);

    void serviceRootMenu();
    void serviceMenu();
    void serviceValue();
    void menuValues();
    void reUpdateButton();
    void createList(String _title, int _noOfElements, String *_list);
    void displayMessage(String _title, String _message);
    void resetMenu();
    void resetTimeouts();
    void countdownMenuTimeouts(clearTimeoutsType clearType);
    void manageMenu();
    byte getMenuOptionSelected();
    String getMenuOptionSelectedText();
    String getMenuValueEnteredText();
    void calculateAndSaveHourValue();
    void calculateAndSaveMinuteValue();
    void setWiFiSSIDFromSelection();

    bool getEncoderCW();
    bool getEncoderCCW();
    bool readUnhandledEncoderPress();
};

extern MenuManager_ &menuManager;
