#include "MenuManager.h"

// -------------------------------------------------------------------------------------------------
//                                         menus defintion
// -------------------------------------------------------------------------------------------------

// ************************************************************
// Build the root menu / status display
// ************************************************************
void MenuManager_::rootMenu() {
  debugMsgMnm("Root Menu");
  resetMenu();
  menuMode = root;
}

// ************************************************************
// Main Menu
// ************************************************************
void MenuManager_::mainMenu() {
  debugMsgMnm("Main Menu");
  resetMenu();
  byte menuCount = 1;
  menuMode = menu;

  oledMenu.menuTitle = "Main Menu";
  oledMenu.menuItems[menuCount] = "Radio";           oledMenu.menuActions[menuCount++] = gotoRadioMenu;
  oledMenu.menuItems[menuCount] = "Wifi";            oledMenu.menuActions[menuCount++] = gotoWifiMenu;
  oledMenu.menuItems[menuCount] = "System";          oledMenu.menuActions[menuCount++] = gotoOptionsMenu;
  oledMenu.menuItems[menuCount] = "Menu Off";        oledMenu.menuActions[menuCount++] = menuOff;
  oledMenu.noOfmenuItems = --menuCount;
}

// ************************************************************
// Burn Menu
// ************************************************************
void MenuManager_::radioMenu() {
  resetMenu();
  byte menuCount = 1;
  menuMode = menu;

  oledMenu.menuTitle = "Radio Menu";

  oledMenu.menuItems[menuCount] = "Back";                                                   oledMenu.menuActions[menuCount++] = backToMain;
  oledMenu.noOfmenuItems = --menuCount;
}

// ************************************************************
// Wifi Menu
// ************************************************************
void MenuManager_::wifiMenu() {
  resetMenu();
  menuMode = menu;
  byte menuCount = 1;

  String wcTag = "("+String(wifiManager.getLastScanResultCount())+")";
  if (WiFi.isConnected()) {
    oledMenu.menuTitle = "WiFi Menu";           
    oledMenu.menuItems[menuCount] = "Disconnect WiFi";         oledMenu.menuActions[menuCount++] = disconnectWifi;
    oledMenu.menuItems[menuCount] = "Back";                    oledMenu.menuActions[menuCount++] = backToMain;
  } else {
    oledMenu.menuTitle = "WiFi Menu";
    if (wifiManager.wifiCredentialsReceived()) {
      oledMenu.menuItems[menuCount] = "Reconnect previous";    oledMenu.menuActions[menuCount++] = reconnectPrevious;
    }
    oledMenu.menuItems[menuCount] = "Connect with WPS";        oledMenu.menuActions[menuCount++] = connectWPS;
    oledMenu.menuItems[menuCount] = "Start SmartConfig";       oledMenu.menuActions[menuCount++] = smartConfig;
    oledMenu.menuItems[menuCount] = "Open Access Point";       oledMenu.menuActions[menuCount++] = openAccessPoint;
    oledMenu.menuItems[menuCount] = "Scan Wifi";               oledMenu.menuActions[menuCount++] = scanWiFi;
    oledMenu.menuItems[menuCount] = "Select WiFi " + wcTag;    oledMenu.menuActions[menuCount++] = showWifiSelection;
    oledMenu.menuItems[menuCount] = "Enter SSID";              oledMenu.menuActions[menuCount++] = enterWiFiSSID;
    oledMenu.menuItems[menuCount] = "Enter password";          oledMenu.menuActions[menuCount++] = enterWiFiPassword;
    oledMenu.menuItems[menuCount] = "Back";                    oledMenu.menuActions[menuCount++] = backToMain;
  }
  oledMenu.noOfmenuItems = --menuCount;
}

// ************************************************************
// System Menu
// ************************************************************
void MenuManager_::systemMenu() {
  resetMenu();
  menuMode = menu;
  byte menuCount = 1;

  oledMenu.menuTitle = "System";
  oledMenu.menuItems[menuCount] = "Restart Device"; oledMenu.menuActions[menuCount++] = restartClock;
  oledMenu.menuItems[menuCount] = "Save config";    oledMenu.menuActions[menuCount++] = saveConfig;
  String status = cc->WifiOnAtStart ? "off" : "on";
  oledMenu.menuItems[menuCount] = "WiFi at start: "+ status; oledMenu.menuActions[menuCount++] = toggleWiFiAtStart;
  #ifdef DEBUG
  oledMenu.menuItems[menuCount] = "Debug on 10m";   oledMenu.menuActions[menuCount++] = debugOn10mins;
  #endif
  oledMenu.menuItems[menuCount] = "Reset WiFi";     oledMenu.menuActions[menuCount++] = resetWiFiInfo;
  oledMenu.menuItems[menuCount] = "Back";           oledMenu.menuActions[menuCount++] = backToMain;
  oledMenu.noOfmenuItems = --menuCount;
}

// ************************************************************
// Select WiFi network from those scanned
// ************************************************************
void MenuManager_::wifiSelectMenu() {
  resetMenu();
  menuMode = menu;
  byte menuCount = 1;

  oledMenu.menuTitle = "Select network";

  int numberOfEntries = wifiManager.getLastScanResultCount() < MENU_ITEMS ? wifiManager.getLastScanResultCount() : MENU_ITEMS;
  
  // Leave some room for the "Back" option
  numberOfEntries--;
  debugMsgMnm("Showing entries: " + String(numberOfEntries));

  if (wifiManager.getLastScanResultCount() < numberOfEntries) numberOfEntries = wifiManager.getLastScanResultCount();
  for (int i = 0; i < numberOfEntries ; i++) {
    oledMenu.menuItems[menuCount] = wifiManager.getLastScanResultSSID(i); oledMenu.menuActions[menuCount++] = selectWiFiSSID;
  }
  oledMenu.menuItems[menuCount] = "Back"; oledMenu.menuActions[menuCount++] = backToMain;
  oledMenu.noOfmenuItems = --menuCount;
}

// -------------------------------------------------------------------------------------------------
//                                         actiona defintion
// -------------------------------------------------------------------------------------------------

// ************************************************************
// actions for menu selections are put in here
// ************************************************************
void MenuManager_::menuActions(menuTargets selectedAction) {
  switch (selectedAction) {
    // --------------------------------------------------
    // Top Level Menu & Management
    case noTarget: {
      break;
    }
    case unmappedOption: {
      debugMsgMnm("Unmapped option");
      mainMenu();
      break;
    }
    case backToMain: {
      mainMenu();
      break;
    }
    case backToRadio: {
      radioMenu();
      break;
    }
    case gotoRadioMenu: {
      radioMenu();
      break;
    }
    case gotoWifiMenu: {
      wifiMenu();
      break;
    }
    case gotoOptionsMenu: {
      systemMenu();
      break;
    }
    case menuOff: {
      resetMenu();
      break;
    }

    // --------------------------------------------------
    // "Main Menu Items"

    // --------------------------------------------------
    // "Burn Menu Items"

    // --------------------------------------------------
    // "WiFi Menu Items"
    case reconnectPrevious: {
      wifiManager.connectToLastAP();
      wifiMenu();
      break;
    }
    case connectWPS: {
      wifiManager.connectWithWPS();
      wifiMenu();
      break;
    }
    case smartConfig: {
      wifiManager.startSmartConfig();
      wifiMenu();
      break;
    }
    case openAccessPoint: {
      wifiManager.openAccessPortal();
      wifiMenu();
      break;
    }
    case scanWiFi: {
      wifiManager.startScanWiFiNetworks();
      wifiMenu();
      break;
    }
    case selectWiFiSSID: {
      setWiFiSSIDFromSelection();
      wifiMenu();
      break;
    }
    case showWifiSelection: {
      wifiSelectMenu();
      break;
    }
    case disconnectWifi: {
      wifiManager.disconnectWiFi();
      wifiMenu();
      break;
    }
    case enterWiFiPassword: {
      setStringValue("Enter WiFi password", saveWiFiPassword, cc->WiFiPassword);
      break;
    }
    case saveWiFiPassword: {
      cc->WiFiPassword = getMenuValueEnteredText();
      debugMsgMnm("Set wiFi pw: " + cc->WiFiPassword);
      wifiMenu();
      break;
    }
    case enterWiFiSSID: {
      setStringValue("Enter WiFi name", saveWiFiSSID, cc->WiFiSSID);
      break;
    }
    case saveWiFiSSID: {
      cc->WiFiSSID = getMenuValueEnteredText();
      debugMsgMnm("Set SSID: " + cc->WiFiSSID);
      wifiMenu();
      break;
    }

    // --------------------------------------------------
    // "System Menu Items"
    case restartClock: {
      spiffsStorage.saveStatsToSpiffs();
      flashMenuMessage("Restart","Restarting\nburner\ndevice now");
      delay(1000);
      ESP.restart();
      break;
    }
    case saveConfig: {
      spiffsStorage.saveConfigToSpiffs();
      systemMenu();
      break;
    }
    case toggleWiFiAtStart: {
      cc->WifiOnAtStart = ! cc->WifiOnAtStart;
      systemMenu();
      break;
    }
    #ifdef DEBUG
    case debugOn10mins: {
      debugManager.setDebugAutoOff(600);
      systemMenu();
      break;
    }
    #endif
    case resetWiFiInfo: {
      resetWiFi();
      systemMenu();
      break;
    }
  }

  oledMenu.selectedMenuItem = noTarget;

}  // menuActions

// -------------------------------------------------------------------------------------------------
// -------------------------------------------------------------------------------------------------
//                     menu operations code - not usually changed per application
// -------------------------------------------------------------------------------------------------
// -------------------------------------------------------------------------------------------------

// ************************************************************
// Flash a message to the display
// ************************************************************
void MenuManager_::flashMenuMessage(String heading, String message) {
  // Only flash the message if the display is already on
  if (!oled.getBlanked()) {
    resetTimeouts();
    flashTimeout = FLASH_TIME;
    displayMessage(heading, message);
  }
} 

// ************************************************************
// Cancel a Flash message
// ************************************************************
void MenuManager_::clearFlashMenuMessage() {
  countdownMenuTimeouts(clearFlashTimeout);
} 

// ************************************************************
// Add a scrolling message to the display
// ************************************************************
void MenuManager_::scrollMenuMessage(String message) {
  // Only show the message if the display is already on
  if (!oled.getBlanked()) {
    resetTimeouts();
    oled.showScrollingMessage(message);
  }
} 

// ************************************************************
// Set up a string on the display for entry
// ************************************************************
void MenuManager_::setStringValue(String title, menuTargets target, String initialValue) {
  resetMenu();                           // clear any previous menu
  menuMode = stringValue;                // enable value entry
  
  oledMenu.menuTitle = title;            // title (used to identify which number was entered)
  oledMenu.mValueLow = 0;                // minimum value allowed - this refers to the character set!
  oledMenu.mValueHigh = 82;              // maximum value allowed - this refers to the character set!
  oledMenu.mValueStep = 1;               // step size
  oledMenu.mValueEntered = 0;            // starting value
  oledMenu.nextTarget = target;          // action to call when button pressed
  oledMenu.enteredString = initialValue;
}

// ************************************************************
// Set up an integer on the display for entry
// ************************************************************
void MenuManager_::setIntegerValue(String title, int startValue, int min, int max, menuTargets target) {
  resetMenu();                           // clear any previous menu
  menuMode = value;                      // enable value entry
  oledMenu.menuTitle = title;            // title (used to identify which number was entered)
  oledMenu.mValueLow = min;              // minimum value allowed
  oledMenu.mValueHigh = max;             // maximum value allowed
  oledMenu.mValueStep = 1;               // step size
  oledMenu.mValueEntered = startValue;   // starting value
  oledMenu.nextTarget = target;          // action to call when button pressed
}

// ************************************************************
// When we are in the root, do the actions needed to service it
// ************************************************************
void MenuManager_::serviceRootMenu() {
  bool doToggle = false;

  if (getEncoderCCW()) {
    doToggle = true;
  }

  if (getEncoderCW()) {
    doToggle = true;
  }
  
    if (readUnhandledEncoderPress()) {
      debugMsgMnm("button press!");
    }

  oled.setTextSize(1);
  oled.showStatusLine();

  oled.clearScrollingMessage();
  oled.setCursor(0, 0);
  if (WiFi.isConnected()) {
    oled.showScrollingMessage("IP: " + WiFi.localIP().toString());
    oled.showScrollingMessage(String(WiFi.getHostname()) + ".local");
    oled.showScrollingMessage(String(WiFi.SSID()));
  } else {
    oled.showScrollingMessage("WiFi not connected");
  }
}

// ************************************************************
// When we are in a menu, do the actions needed to service it
// ************************************************************
void MenuManager_::serviceMenu() {
  if (getEncoderCCW()) {
    oledMenu.highlightedMenuItem++;
    oledMenu.needUpdate = true;
  }

  if (getEncoderCW()) {
    oledMenu.highlightedMenuItem--;
    oledMenu.needUpdate = true;
  }

  if (readUnhandledEncoderPress()) {
    oledMenu.selectedMenuItem = oledMenu.highlightedMenuItem;
    oledMenu.lastMenuActivity = nowMillis;
    oledMenu.needUpdate = true;
    debugMsgMnm("menu '" + oledMenu.menuTitle + "' item '" + oledMenu.menuItems[oledMenu.highlightedMenuItem] + "' selected");
  }

  if (oledMenu.needUpdate) {
    const int _centreLine = DISPLAY_MAX_LINES / 2 + 1;    // mid list point
    oled.clearDisplay();
    oled.setTextColor(WHITE);

    // verify valid highlighted item
    if (oledMenu.highlightedMenuItem > oledMenu.noOfmenuItems) oledMenu.highlightedMenuItem = oledMenu.noOfmenuItems;
    if (oledMenu.highlightedMenuItem < 1) oledMenu.highlightedMenuItem = 1;

    // title
    oled.setCursor(0, 0);
    if (MENU_LARGE_TEXT) {
      oled.setTextSize(2);
      oled.println(oledMenu.menuItems[oledMenu.highlightedMenuItem].substring(0, MAX_TITLE_LENGTH));
    } else {
      if (oledMenu.menuTitle.length() > MAX_TITLE_LENGTH) oled.setTextSize(1);
      else oled.setTextSize(2);
      oled.println(oledMenu.menuTitle);
    }
    oled.drawLine(0, TOP_LINE - 1, oled.width(), TOP_LINE - 1, WHITE);       // draw horizontal line under title

    // menu
    oled.setTextSize(1);
    oled.setCursor(0, TOP_LINE);
    for (int i=1; i <= DISPLAY_MAX_LINES; i++) {
      int item = oledMenu.highlightedMenuItem - _centreLine + i;
      if (item == oledMenu.highlightedMenuItem) oled.setTextColor(BLACK, WHITE);
      else oled.setTextColor(WHITE);
      if (item > 0 && item <= oledMenu.noOfmenuItems) oled.println(oledMenu.menuItems[item]);
      else oled.println(" ");
    }

    oled.outputDisplay();
    oledMenu.needUpdate = false;
  }
}

// ************************************************************
// Service value entry
// ************************************************************
void MenuManager_::serviceValue() {
  if (getEncoderCCW()) {
    oledMenu.mValueEntered -= oledMenu.mValueStep;
    oledMenu.needUpdate = true;
  }

  if (getEncoderCW()) {
    oledMenu.mValueEntered+= oledMenu.mValueStep;
    oledMenu.needUpdate = true;
  }

  // ---------------------------------------------------------------------- 
  // Wrap entered value if out of bounds
  if (oledMenu.mValueEntered < oledMenu.mValueLow) {
    oledMenu.mValueEntered = oledMenu.mValueLow;
    oledMenu.lastMenuActivity = nowMillis;
    oledMenu.needUpdate = true;
  }

  if (oledMenu.mValueEntered > oledMenu.mValueHigh) {
    oledMenu.mValueEntered = oledMenu.mValueHigh;
    oledMenu.lastMenuActivity = nowMillis;
    oledMenu.needUpdate = true;
  }

  // ---------------------------------------------------------------------- 
  // redraw the display if needed
  if (oledMenu.needUpdate) {
    if (menuMode == value) {
      const int _valueSpacingX = 30;      // spacing for the displayed value y position
      const int _valueSpacingY = 5;       // spacing for the displayed value y position
      oled.clearDisplay();
      oled.setTextColor(WHITE);

      // title
      oled.setCursor(0, 0);
      if (oledMenu.menuTitle.length() > MAX_TITLE_LENGTH) oled.setTextSize(1);
      else oled.setTextSize(2);
      oled.println(oledMenu.menuTitle);
      oled.drawLine(0, TOP_LINE - 1, oled.width(), TOP_LINE - 1, WHITE);       // draw horizontal line under title

      // value selected
      oled.setCursor(_valueSpacingX, TOP_LINE + _valueSpacingY);
      oled.setTextSize(3);
      oled.println(String(oledMenu.mValueEntered));

      // range
      oled.setCursor(0, oled.height() - LINE_SPACE_1 - 1 );   // bottom of display
      oled.setTextSize(1);
      oled.println(String(oledMenu.mValueLow) + " to " + String(oledMenu.mValueHigh));

      // bar
      int Tlinelength = map(oledMenu.mValueEntered, oledMenu.mValueLow, oledMenu.mValueHigh, 0 , oled.width());
      oled.drawLine(0, oled.height()-1, Tlinelength, oled.height()-1, WHITE);

      oled.outputDisplay();
      oledMenu.needUpdate = false;
    } else

    if (menuMode == stringValue) {
      const int _valueSpacingY = 5;       // spacing for the displayed value y position
      oled.clearDisplay();
      oled.setTextColor(WHITE);

      // title
      oled.setCursor(0, 0);
      if (oledMenu.menuTitle.length() > MAX_TITLE_LENGTH) oled.setTextSize(1);
      else oled.setTextSize(2);
      oled.println(oledMenu.menuTitle);
      oled.drawLine(0, TOP_LINE - 1, oled.width(), TOP_LINE - 1, WHITE);       // draw horizontal line under title

      // value selected
      switch (oledMenu.mValueEntered) {
        case BACKSPACE: {
          oled.setCursor(30, TOP_LINE + _valueSpacingY);
          oled.setTextSize(2);
          oled.println("DELETE");
          break;
        }
        case DONE: {
          oled.setCursor(40, TOP_LINE + _valueSpacingY);
          oled.setTextSize(2);
          oled.println("DONE");
          break;
        }
        case RESTART: {
          oled.setCursor(26, TOP_LINE + _valueSpacingY);
          oled.setTextSize(2);
          oled.println("RESTART");
          break;
        }
        default: {
          oled.setCursor(52, TOP_LINE + _valueSpacingY);
          oled.setTextSize(3);
          oled.println(CHARSET.substring(oledMenu.mValueEntered, oledMenu.mValueEntered+1));
        }
      }

      // range
      oled.setCursor(0, oled.height() - LINE_SPACE_1 - 1 );   // bottom of display
      oled.setTextSize(1);
      if (oledMenu.enteredString.length() == 0) {
        oled.println("Enter value then DONE");
      } else {
        String displayString = oledMenu.enteredString;
        int displayStrLen = oledMenu.enteredString.length();
        if (displayStrLen > 16) {
          displayString = "..." + oledMenu.enteredString.substring(displayStrLen-14, displayStrLen);
        }
        
        oled.println(String("<" + displayString + ">"));
      }
      oled.outputDisplay();
      oledMenu.needUpdate = false;

      // bar
      int Tlinelength = map(oledMenu.mValueEntered, oledMenu.mValueLow, oledMenu.mValueHigh, 0 , oled.width());
      oled.drawLine(0, oled.height()-1, Tlinelength, oled.height()-1, WHITE);

    }
  }

  reUpdateButton();        // check status of button
}

// ************************************************************
// create a menu from a list
// e.g.       String tList[]={"main menu", "2", "3", "4", "5", "6"};
//            createList("demo_list", 6, &tList[0]);
// ************************************************************
void MenuManager_::createList(String _title, int _noOfElements, String *_list) {
  resetMenu();                                // clear any previous menu
  menuMode = menu;                            // enable menu mode
  oledMenu.noOfmenuItems = _noOfElements;     // set the number of items in this menu
  oledMenu.menuTitle = _title;                // menus title (used to identify it)

  for (int i=1; i <= _noOfElements; i++) {
    oledMenu.menuItems[i] = _list[i-1];       // set the menu items
  }
}

// ************************************************************
// Display a multi line message
// 21 characters per line, use "\n" for next line
// assistant:  <     line 1        ><     line 2        ><     line 3        ><     line 4         >
// ************************************************************
void MenuManager_::displayMessage(String _title, String _message) {
  resetMenu();
  menuMode = message;

  oled.clearDisplay();
  oled.setTextColor(WHITE);

  // title
  oled.setCursor(0, 0);
  if (MENU_LARGE_TEXT) {
    oled.setTextSize(2);
    oled.println(_title.substring(0, MAX_TITLE_LENGTH));
  } else {
    if (_title.length() > MAX_TITLE_LENGTH) oled.setTextSize(1);
    else oled.setTextSize(2);
    oled.println(_title);
  }

  // message
  oled.setCursor(0, TOP_LINE);
  oled.setTextSize(1);
  oled.println(_message);

  oled.outputDisplay();
 }

// ************************************************************
// reset all menu variables / flags
// ************************************************************
void MenuManager_::resetMenu() {
  oledMenu.selectedMenuItem = noTarget;
  oledMenu.noOfmenuItems = 0;
  oledMenu.menuTitle = "";
  oledMenu.highlightedMenuItem = 0;
  oledMenu.mValueEntered = 0;

  oledMenu.lastMenuActivity = nowMillis;
}

// ************************************************************
// Reset the display timeout
// ************************************************************
void MenuManager_::resetTimeouts() {
  // first press: wake up
  if (oledTimeout == 0) {
    debugMsgMnm("OLED: ON");
    resetDisplay = true;
  } else if (menuMode > off) {
    configTimeout = CONFIG_TIME;
    oledMenu.needUpdate = true;
  }
  oledTimeout = OLED_ON_TIME;
}

// ************************************************************
// Count down the various timeouts
// ************************************************************
void MenuManager_::countdownMenuTimeouts(clearTimeoutsType clearType) {
  if (clearType == clearFlashTimeout) {
    flashTimeout = 0;
    oled.clearDisplay();
  }

  if (flashTimeout > 0) {
    flashTimeout--;
    if(flashTimeout == 0) {
      oled.clearDisplay();
    }
  }

  if (clearType == clearConfigTimeout) {
    configTimeout = 0;
    oled.clearDisplay();
  }

  if (configTimeout > 0) {
    configTimeout--;
    if (configTimeout == 0) {
      oled.clearDisplay();
    }
  }

  if (clearType == clearOledTimeout) {
    oledTimeout = 1;
  }

  if (oledTimeout > 0) {
    oledTimeout--;
    if (oledTimeout == 0) {
      oled.blankDisplay();
      debugMsgMnm("OLED: OFF");
      menuMode = off;
    }
  }

  if (menuMode != off) {
    if (configTimeout == 0 && flashTimeout == 0) {
      rootMenu();
    }
  }
}

// ************************************************************
// Set the WiFi SSID from the selected menu item
// ************************************************************
void MenuManager_::setWiFiSSIDFromSelection() {
  debugMsgMnm("Selected option = " + String(oledMenu.selectedMenuItem));
  String selectedWiFi = wifiManager.getLastScanResultSSID(oledMenu.selectedMenuItem - 1);
  debugMsgMnm("Selected WiFi = " + selectedWiFi);
  cc->WiFiSSID = selectedWiFi;
}

// ************************************************************
// After a menu item is selected, return the value entered
// ************************************************************
byte MenuManager_::getMenuOptionSelected() {
  return oledMenu.selectedMenuItem;
}

// ************************************************************
// After a menu item is selected, return the text selected
// ************************************************************
String MenuManager_::getMenuOptionSelectedText() {
  return oledMenu.menuItems[oledMenu.highlightedMenuItem];
}

// ************************************************************
// After a menu item is selected, return the text selected
// ************************************************************
String MenuManager_::getMenuValueEnteredText() {
  return oledMenu.enteredString;
}

// ************************************************************
// Main entry point for the menu mangement:
//  - Build and update the status display
//  - Service the menu system and value entry system
// ************************************************************
void MenuManager_::menuOncePerLoop() {
  reUpdateButton();               // update rotary encoder button status (if pressed activate default menu)

  debugMsgMnmX("Menu mode: " + String(menuMode) + " sel: " + String(oledMenu.selectedMenuItem) + " high: " + String(oledMenu.highlightedMenuItem) + " val: " + String(oledMenu.mValueEntered) + " conf: " + String(configTimeout) + " flash: " + String(flashTimeout) + " oled: " + String(oledTimeout));

  // Read and debounce buttons 
  if (btn1ReadMillis > nowMillis) {
    if (digitalRead(BTNCONF) == LOW) {
      btn1ReadMillis = 0;
      debugMsgInr("Confirm Button pressed");
      menuManager.confirmButtonPress();
    }
  }
  if (btn2ReadMillis > nowMillis) {
    if (digitalRead(BTNBACK) == LOW) {
      btn2ReadMillis = 0;
      debugMsgInr("Back Button pressed");
      menuManager.backButtonPress();
    }
  }
  
  // Re-initialise the display if it was off
  if (resetDisplay) {
    oled.setUp();
    resetDisplay = false;
  }

  switch (menuMode) {
    case root:
      serviceRootMenu();
      break;

    case menu:
      serviceMenu();
      menuActions(oledMenu.menuActions[oledMenu.selectedMenuItem]);
      break;

    case value:
      serviceValue();
      if (readUnhandledEncoderPress()) {
        debugMsgMnm("Button pressed: value: "+ String(oledMenu.mValueEntered));
        menuActions(oledMenu.nextTarget);
      }
      break;

    case stringValue:
      serviceValue();
      if (readUnhandledEncoderPress()) {
        debugMsgMnm("Button pressed: value: "+ String(oledMenu.mValueEntered));

        // Spedial handling for string entry to allow a string to be built up and edited
        if (oledMenu.mValueEntered == BACKSPACE) {
          oledMenu.enteredString = oledMenu.enteredString.substring(0, oledMenu.enteredString.length() - 1);
        } else if (oledMenu.mValueEntered == RESTART) {
          oledMenu.enteredString = "";
        } else if (oledMenu.mValueEntered == DONE) {
          menuActions(oledMenu.nextTarget);
        } else {
          oledMenu.enteredString = oledMenu.enteredString + CHARSET.substring(oledMenu.mValueEntered, oledMenu.mValueEntered+1);
        }
      }
      break;

    case message:
      if (readUnhandledEncoderPress()) clearFlashMenuMessage();
      break;

    case off:
      if (readUnhandledEncoderPress()) rootMenu();
      break;

    default:
      break;
  }
}

// ************************************************************
// Build the status display
// ************************************************************
void MenuManager_::menuOncePerSecond() {
  // Manage timeouts
  countdownMenuTimeouts(normalTimeouts);

  #ifdef MNM_EXTENDED_DEBUG
  String menuText = "";
  switch (menuMode) {
    case off:
    menuText = "OFF";
      break;
    case root:
    menuText = "ROOT";
      break;
    case menu:
    menuText = "MENU";
      break;
    case value:
    menuText = "VALUE";
      break;
    case stringValue:
    menuText = "STRING";
      break;
    case message:
    menuText = "MESSAGE";
      break;
    default:
      break;
  }
  debugMsgMnm("Menu mode: " + menuText);
  #endif
}

// ************************************************************
// Things that need updating once per hour
// ************************************************************
void MenuManager_::menuOncePerHour() {
  // nothing at present
}

// ************************************************************
// Get the current blanking status of the OLED
// ************************************************************
bool MenuManager_::getOledIsBlanked() {
  return oledTimeout == 0;
}

// ************************************************************
// Capture the confirm button press
// ************************************************************
void MenuManager_::confirmButtonPress() {
}

// ************************************************************
// Capture the back button press - get out of menus
// ************************************************************
void MenuManager_::backButtonPress() {
  if (menuMode == menu || menuMode == value || menuMode == stringValue) {
    oled.clearDisplay();
    rootMenu();
  }
}

// ************************************************************
//                        -internal plumbing
// ************************************************************
void IRAM_ATTR doEncoderWrapper() {
  portENTER_CRITICAL_ISR(&encoderMux);
  menuManager.doEncoder();
  portEXIT_CRITICAL_ISR(&encoderMux);
}

// ************************************************************
// Switch changed - mark that there is an event waiting
// ************************************************************
void IRAM_ATTR btn1ISR() {
  btn1ReadMillis = millis() + 50;
}

// ************************************************************
// Switch changed - mark that there is an event waiting
// ************************************************************
void IRAM_ATTR btn2ISR() {
  btn2ReadMillis = millis() + 50;
}

// ************************************************************
// Operation setup for the menu system
// ************************************************************
void MenuManager_::setupMenuManager() {
  pinMode(ENC_BTN, INPUT_PULLUP);
  pinMode(ENC_APin, INPUT);
  pinMode(ENC_BPin, INPUT);

  // Interrupt for reading the rotary encoder position
  rotaryEncoder.encoder0Pos = 0;
  attachInterrupt(digitalPinToInterrupt(ENC_APin), doEncoderWrapper, CHANGE);
  attachInterrupt(digitalPinToInterrupt(ENC_BPin), doEncoderWrapper, CHANGE);

  // Back and confirm buttons
  pinMode(BTNCONF, INPUT_PULLUP);
  pinMode(BTNBACK, INPUT_PULLUP);

  // Hook up the switches to the trigger handler
  attachInterrupt(BTNCONF, btn1ISR, CHANGE);
  attachInterrupt(BTNBACK, btn2ISR, CHANGE);
}

// -------------------------------------------------------------------------------------------------
// -------------------------------------------------------------------------------------------------
//                                  rotary encoder routines
// -------------------------------------------------------------------------------------------------
// -------------------------------------------------------------------------------------------------

// ************************************************************
//                     -interrupt for rotary encoder
// rotary encoder interrupt routine to update position counter when turned
// interrupt info: https://www.gammon.com.au/forum/bbshowpost.php?id=11488
// ************************************************************
void ICACHE_RAM_ATTR MenuManager_::doEncoder() {
  bool pinA = digitalRead(ENC_APin);
  bool pinB = digitalRead(ENC_BPin);

  encoderToggle = !encoderToggle;

  if ( (rotaryEncoder.encoderPrevA == pinA && rotaryEncoder.encoderPrevB == pinB) ) return;  // no change since last time (i.e. reject bounce)

  // same direction (alternating between 0,1 and 1,0 in one direction or 1,1 and 0,0 in the other direction)
       if (rotaryEncoder.encoderPrevA == 1 && rotaryEncoder.encoderPrevB == 0 && pinA == 0 && pinB == 1) {rotaryEncoder.encoder0Pos -= 1;}
  else if (rotaryEncoder.encoderPrevA == 0 && rotaryEncoder.encoderPrevB == 1 && pinA == 1 && pinB == 0) {rotaryEncoder.encoder0Pos -= 1;}
  else if (rotaryEncoder.encoderPrevA == 0 && rotaryEncoder.encoderPrevB == 0 && pinA == 1 && pinB == 1) {rotaryEncoder.encoder0Pos += 1;}
  else if (rotaryEncoder.encoderPrevA == 1 && rotaryEncoder.encoderPrevB == 1 && pinA == 0 && pinB == 0) {rotaryEncoder.encoder0Pos += 1;}

  // change of direction
  else if (rotaryEncoder.encoderPrevA == 1 && rotaryEncoder.encoderPrevB == 0 && pinA == 0 && pinB == 0) {rotaryEncoder.encoder0Pos += 1;}
  else if (rotaryEncoder.encoderPrevA == 0 && rotaryEncoder.encoderPrevB == 1 && pinA == 1 && pinB == 1) {rotaryEncoder.encoder0Pos += 1;}
  else if (rotaryEncoder.encoderPrevA == 0 && rotaryEncoder.encoderPrevB == 0 && pinA == 1 && pinB == 0) {rotaryEncoder.encoder0Pos -= 1;}
  else if (rotaryEncoder.encoderPrevA == 1 && rotaryEncoder.encoderPrevB == 1 && pinA == 0 && pinB == 1) {rotaryEncoder.encoder0Pos -= 1;}

  // update previous readings
  rotaryEncoder.encoderPrevA = pinA;
  rotaryEncoder.encoderPrevB = pinB;

  // Reset the display timeouts if we have movement
  resetTimeouts();
}

  // ************************************************************
  // Process encoder movement CCW
  // ************************************************************
  bool MenuManager_::getEncoderCCW() {
  if (rotaryEncoder.encoder0Pos >= TICKS_PER_MOVE) {
    rotaryEncoder.encoder0Pos -= TICKS_PER_MOVE;
    oledMenu.lastMenuActivity = nowMillis;
    return true;
  }
  return false;
}

  // ************************************************************
  // Process encoder movement CW
  // ************************************************************
  bool MenuManager_::getEncoderCW() {
  if (rotaryEncoder.encoder0Pos <= -TICKS_PER_MOVE) {
    rotaryEncoder.encoder0Pos += TICKS_PER_MOVE;
    oledMenu.lastMenuActivity = nowMillis;
    return true;
  }
  return false;
}

// ************************************************************
// Button debounce (rotary encoder)
// ************************************************************
void MenuManager_::reUpdateButton() {
  bool tReading = digitalRead(ENC_BTN);               // read current button state
  if (tReading != rotaryEncoder.encoderPrevButton) rotaryEncoder.reLastButtonChange = nowMillis;     // if it has changed reset timer
  if ( (unsigned long)(nowMillis - rotaryEncoder.reLastButtonChange) > rotaryEncoder.reDebounceDelay ) {  // if button state is stable
    if (rotaryEncoder.encoderPrevButton == rotaryEncoder.reButtonPressedState) {
      if (rotaryEncoder.reButtonDebounced == 0) {     // if the button has been pressed
        rotaryEncoder.reButtonPressed = 1;            // flag set when the button has been pressed
      }
      rotaryEncoder.reButtonDebounced = 1;            // debounced button status  (1 when pressed)
    } else {
      rotaryEncoder.reButtonDebounced = 0;
    }
  }

  if (rotaryEncoder.reButtonDebounced == 1) {
    resetTimeouts();
  }

  rotaryEncoder.encoderPrevButton = tReading;         // update last state read
}

// ************************************************************
// Button debounce (rotary encoder)
// ************************************************************
bool MenuManager_::readUnhandledEncoderPress() {
  if (rotaryEncoder.reButtonPressed == 1) {
    rotaryEncoder.reButtonPressed = 0;
    return true;
  }
  return false;
}


// -------------------------------------------------------------------------------------------------
// -------------------------------------------------------------------------------------------------
//                                         class wiring
// -------------------------------------------------------------------------------------------------
// -------------------------------------------------------------------------------------------------

// ************************************************************
// Library internal singleton wiring
// ************************************************************
MenuManager_ &MenuManager_::getInstance() {
  static MenuManager_ instance;
  return instance;
}

MenuManager_ &menuManager = menuManager.getInstance();