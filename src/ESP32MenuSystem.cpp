/*
 * ESP32MenuSystem.cpp
 * Implementation of the ESP32 Menu System Library
 */

#include "ESP32MenuSystem.h"

// Static instance for ISR access
MenuSystem *MenuSystem::instance = NULL;

// Default character set for string composition
const char DEFAULT_CHARSET[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+@#*%&/()=?!$-_ ";

// Constructor
MenuSystem::MenuSystem(uint8_t width, uint8_t height, int8_t resetPin)
    : screenWidth(width), screenHeight(height), rootMenu(NULL), currentMenu(NULL),
      selectedItem(NULL), selectedIndex(0), scrollOffset(0), visibleItems(4),
      currentMode(MODE_STATUS_SCREEN), encoderPosition(0), lastEncoderTime(0),
      lastEncoderState(0), encoderPulseCount(0), confirmPressed(false), backPressed(false),
      encoderSwPressed(false), screenSaverTimeout(0), lastActivityTime(0),
      screenSaverActive(false), lastMenuActivity(0), statusData(NULL)
{

  display = new Adafruit_SH1106G(width, height, &Wire, resetPin);
  instance = this;
}

// Destructor
MenuSystem::~MenuSystem()
{
  if (display)
  {
    delete display;
  }
  if (tempEditItem)
  {
    delete tempEditItem;
  }
}

// ISR Handlers
void IRAM_ATTR MenuSystem::encoderISR()
{
  if (!instance)
    return;

  uint8_t clkState = digitalRead(instance->encoderClkPin);
  uint8_t dtState = digitalRead(instance->encoderDtPin);

  // Simple quadrature decoding - count raw pulses
  if (clkState != instance->lastEncoderState)
  {
    if (dtState != clkState)
    {
      instance->encoderPulseCount++;
    }
    else
    {
      instance->encoderPulseCount--;
    }

    // Only increment position every N pulses
    if (instance->encoderPulseCount >= instance->encoderPulsesPerStep)
    {
      instance->encoderPosition++;
      instance->encoderPulseCount = 0;
    }
    else if (instance->encoderPulseCount <= -instance->encoderPulsesPerStep)
    {
      instance->encoderPosition--;
      instance->encoderPulseCount = 0;
    }

    instance->lastEncoderTime = millis();
  }
  instance->lastEncoderState = clkState;
}

void IRAM_ATTR MenuSystem::confirmISR()
{
  if (!instance)
    return;
  instance->confirmPressed = true;
  instance->confirmPressTime = millis();
}

void IRAM_ATTR MenuSystem::backISR()
{
  if (!instance)
    return;
  instance->backPressed = true;
  instance->backPressTime = millis();
}

void IRAM_ATTR MenuSystem::encoderSwISR()
{
  if (!instance)
    return;
  instance->encoderSwPressed = true;
  instance->encoderSwPressTime = millis();
}

// Initialize the menu system
bool MenuSystem::begin(uint8_t sdaPin, uint8_t sclPin,
                       uint8_t encClk, uint8_t encDt,
                       uint8_t btnConfirm, uint8_t btnBack,
                       uint8_t encSw)
{

  // Store pin assignments
  encoderClkPin = encClk;
  encoderDtPin = encDt;
  encoderSwPin = encSw;
  confirmPin = btnConfirm;
  backPin = btnBack;

  // Initialize I2C
  Wire.begin(sdaPin, sclPin);

  // Initialize display
  display->begin(0x3C, true); // Address 0x3C, reset=true

  display->clearDisplay();
  display->setTextSize(1);
  display->setTextColor(SH110X_WHITE);
  display->display();

  // Setup encoder pins
  pinMode(encoderClkPin, INPUT_PULLUP);
  pinMode(encoderDtPin, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(encoderClkPin), encoderISR, CHANGE);
  attachInterrupt(digitalPinToInterrupt(encoderDtPin), encoderISR, CHANGE);

  lastEncoderState = digitalRead(encoderClkPin);

  // Setup button pins
  // GPIO 34+ are input-only with no internal pull-up - use INPUT for those
  if (confirmPin >= 34)
  {
    pinMode(confirmPin, INPUT); // Needs external pull-up resistor
  }
  else
  {
    pinMode(confirmPin, INPUT_PULLUP);
  }
  if (backPin >= 34)
  {
    pinMode(backPin, INPUT); // Needs external pull-up resistor
  }
  else
  {
    pinMode(backPin, INPUT_PULLUP);
  }
  attachInterrupt(digitalPinToInterrupt(confirmPin), confirmISR, FALLING);
  attachInterrupt(digitalPinToInterrupt(backPin), backISR, FALLING);

  if (encSw != 255)
  {
    pinMode(encoderSwPin, INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt(encoderSwPin), encoderSwISR, FALLING);
  }

  // Initialize activity timer
  lastActivityTime = millis();

  return true;
}

// Create a new menu
MenuItem *MenuSystem::createMenu(const char *title)
{
  MenuItem *menu = new MenuItem(title, MENU_ITEM_SUBMENU);
  return menu;
}

// Recursively free all children of a menu (but not the menu itself)
void MenuSystem::clearMenuItems(MenuItem *menu)
{
  if (!menu)
    return;
  MenuItem *item = menu->child;
  while (item)
  {
    MenuItem *next = item->next;
    // Recursively free submenu children
    if (item->child)
    {
      clearMenuItems(item);
    }
    delete item;
    item = next;
  }
  menu->child = nullptr;
}

// Set root menu
void MenuSystem::setRootMenu(MenuItem *menu)
{
  rootMenu = menu;
  currentMenu = menu;
  selectedItem = menu->child;
  selectedIndex = 0;
  scrollOffset = 0;
}

// Add submenu item
MenuItem *MenuSystem::addSubmenu(MenuItem *parent, const char *label, MenuItem *submenu)
{
  MenuItem *item = new MenuItem(label, MENU_ITEM_SUBMENU);
  item->parent = parent;
  item->child = submenu;
  submenu->parent = item;

  // Add to parent's child list
  if (parent->child == NULL)
  {
    parent->child = item;
  }
  else
  {
    MenuItem *last = parent->child;
    while (last->next)
      last = last->next;
    last->next = item;
  }

  return item;
}

// Add action item
MenuItem *MenuSystem::addAction(MenuItem *parent, const char *label, ActionCallback callback)
{
  MenuItem *item = new MenuItem(label, MENU_ITEM_ACTION);
  item->parent = parent;
  item->data.actionCallback = callback;

  if (parent->child == NULL)
  {
    parent->child = item;
  }
  else
  {
    MenuItem *last = parent->child;
    while (last->next)
      last = last->next;
    last->next = item;
  }

  return item;
}

// Add numeric value item
MenuItem *MenuSystem::addNumericValue(MenuItem *parent, const char *label, int *valuePtr,
                                      int minVal, int maxVal, int step, const char *unit,
                                      ActionCallback onSave)
{
  MenuItem *item = new MenuItem(label, MENU_ITEM_NUMERIC_VALUE);
  item->parent = parent;
  item->data.numericData.valuePtr = valuePtr;
  item->data.numericData.minValue = minVal;
  item->data.numericData.maxValue = maxVal;
  item->data.numericData.step = step;
  item->data.numericData.unit = unit;
  item->data.numericData.onSave = onSave;

  if (parent->child == NULL)
  {
    parent->child = item;
  }
  else
  {
    MenuItem *last = parent->child;
    while (last->next)
      last = last->next;
    last->next = item;
  }

  return item;
}

// Add string value item
MenuItem *MenuSystem::addStringValue(MenuItem *parent, const char *label, char *buffer,
                                     uint8_t maxLen, const char *charset,
                                     ActionCallback onSave)
{
  MenuItem *item = new MenuItem(label, MENU_ITEM_STRING_VALUE);
  item->parent = parent;
  item->data.stringData.buffer = buffer;
  item->data.stringData.maxLength = maxLen;
  item->data.stringData.charset = charset ? charset : DEFAULT_CHARSET;
  item->data.stringData.onSave = onSave;

  if (parent->child == NULL)
  {
    parent->child = item;
  }
  else
  {
    MenuItem *last = parent->child;
    while (last->next)
      last = last->next;
    last->next = item;
  }

  return item;
}

// Add either-or item
MenuItem *MenuSystem::addEitherOr(MenuItem *parent, const char *label, bool *valuePtr,
                                  const char *opt1, const char *opt2,
                                  ActionCallback onSave)
{
  MenuItem *item = new MenuItem(label, MENU_ITEM_EITHER_OR);
  item->parent = parent;
  item->data.eitherOrData.valuePtr = valuePtr;
  item->data.eitherOrData.option1 = opt1;
  item->data.eitherOrData.option2 = opt2;
  item->data.eitherOrData.onSave = onSave;

  if (parent->child == NULL)
  {
    parent->child = item;
  }
  else
  {
    MenuItem *last = parent->child;
    while (last->next)
      last = last->next;
    last->next = item;
  }

  return item;
}

// Add info item
MenuItem *MenuSystem::addInfo(MenuItem *parent, const char *label)
{
  MenuItem *item = new MenuItem(label, MENU_ITEM_INFO);
  item->parent = parent;

  if (parent->child == NULL)
  {
    parent->child = item;
  }
  else
  {
    MenuItem *last = parent->child;
    while (last->next)
      last = last->next;
    last->next = item;
  }

  return item;
}

// Get menu item count
uint8_t MenuSystem::getMenuItemCount(MenuItem *menu)
{
  if (!menu)
    return 0;

  uint8_t count = 0;
  MenuItem *item = menu->child;
  while (item)
  {
    count++;
    item = item->next;
  }

  return count;
}

// Get item at index
MenuItem *MenuSystem::getItemAtIndex(uint8_t index)
{
  if (!currentMenu)
    return NULL;

  MenuItem *item = currentMenu->child;
  for (uint8_t i = 0; i < index && item; i++)
  {
    item = item->next;
  }

  return item;
}

// Check button events
ButtonEvent MenuSystem::checkButtons()
{
  unsigned long now = millis();

  if (confirmPressed && (now - confirmPressTime) > debounceDelay)
  {
    confirmPressed = false;
    if (digitalRead(confirmPin) == LOW)
    {
      debugMsgMnm("[BTN] Confirm button pressed");
      if ((now - confirmPressTime) > longPressDelay)
      {
        debugMsgMnm("[BTN] Confirm LONG press");
        return BTN_CONFIRM_LONG;
      }
      debugMsgMnm("[BTN] Confirm CLICK");
      return BTN_CONFIRM_CLICK;
    }
  }

  if (backPressed && (now - backPressTime) > debounceDelay)
  {
    backPressed = false;
    if (digitalRead(backPin) == LOW)
    {
      debugMsgMnm("[BTN] Back button pressed");
      if ((now - backPressTime) > longPressDelay)
      {
        debugMsgMnm("[BTN] Back LONG press");
        return BTN_BACK_LONG;
      }
      debugMsgMnm("[BTN] Back CLICK");
      return BTN_BACK_CLICK;
    }
  }

  if (encoderSwPressed && (now - encoderSwPressTime) > debounceDelay)
  {
    encoderSwPressed = false;
    if (encoderSwPin != 255 && digitalRead(encoderSwPin) == LOW)
    {
      debugMsgMnm("[BTN] Encoder button pressed");
      debugMsgMnm("[BTN] Encoder CLICK");
      return BTN_ENCODER_CLICK;
    }
  }

  return BTN_NONE;
}

// Render status screen
void MenuSystem::renderStatusScreen()
{
  // If custom render callback is set, use it
  if (statusRenderCallback)
  {
    statusRenderCallback(display, screenWidth, screenHeight);
    return;
  }

  // Default rendering if no callback
  if (!statusData)
  {
    display->setTextSize(1);
    display->setCursor(20, 28);
    display->print("Press encoder");
    display->setCursor(30, 38);
    display->print("for menu");
    return;
  }

  display->setTextSize(1);
  uint8_t yPos = 2;

  // WiFi Status
  display->setCursor(0, yPos);
  display->print("WiFi: ");
  display->print(statusData->wifiConnected ? "Connected" : "Disconnected");
  yPos += 10;

  // IP Address (only if connected)
  if (statusData->wifiConnected && strlen(statusData->ipAddress) > 0)
  {
    display->setCursor(0, yPos);
    display->print("IP: ");
    display->print(statusData->ipAddress);
    yPos += 10;
  }

  // Separator
  display->drawLine(0, yPos + 2, screenWidth, yPos + 2, SH110X_WHITE);
  yPos += 6;

  // Show hint at bottom
  display->setCursor(0, screenHeight - 10);
  display->setTextSize(1);
  display->print("[Press Enc for Menu]");
}

// Enter menu item
void MenuSystem::enterMenuItem(MenuItem *item)
{
  if (!item)
    return;

  switch (item->type)
  {
  case MENU_ITEM_SUBMENU:
    if (item->child)
    {
      debugMsgMnm("[MENU] Entering submenu: " + String(item->label));
      // If child is a submenu container (created with createMenu), skip to its children
      if (item->child->type == MENU_ITEM_SUBMENU && item->child->child)
      {
        currentMenu = item->child;
        selectedItem = item->child->child;
      }
      else
      {
        currentMenu = item;
        selectedItem = item->child;
      }
      selectedIndex = 0;
      scrollOffset = 0;
    }
    break;

  case MENU_ITEM_ACTION:
    if (item->data.actionCallback)
    {
      debugMsgMnm("[ACTION] Executing callback for: " + String(item->label));
      item->data.actionCallback();
    }
    break;

  case MENU_ITEM_NUMERIC_VALUE:
    debugMsgMnm("[NUMERIC] Editing: " + String(item->label));
    currentMode = MODE_NUMERIC_VALUE;
    editingValue = *(item->data.numericData.valuePtr);
    editingMin = item->data.numericData.minValue;
    editingMax = item->data.numericData.maxValue;
    editingStep = item->data.numericData.step;
    encoderPosition = 0;
    lastEncoderPos = 0;
    break;

  case MENU_ITEM_STRING_VALUE:
    debugMsgMnm("[STRING] Editing: " + String(item->label));
    currentMode = MODE_STRING_COMPOSITION;
    editingBuffer = item->data.stringData.buffer;
    editingMaxLen = item->data.stringData.maxLength;
    editingCharset = item->data.stringData.charset;
    editingCursorPos = strlen(editingBuffer);
    editingCharIndex = 0;
    encoderPosition = 0;
    lastEncoderPos = 0;
    break;

  case MENU_ITEM_EITHER_OR:
    debugMsgMnm("[EITHER-OR] Editing: " + String(item->label));
    currentMode = MODE_EITHER_OR;
    editingEitherOr = *(item->data.eitherOrData.valuePtr);
    encoderPosition = 0;
    lastEncoderPos = 0;
    break;

  case MENU_ITEM_INFO:
    debugMsgMnm("[INFO] Displayed: " + String(item->label));
    break;
  }
}

// Exit current editing mode
void MenuSystem::exitCurrentMode()
{
  currentMode = MODE_MENU_NAVIGATION;
  encoderPosition = 0;
  lastEncoderPos = 0;
}

// Main update loop
void MenuSystem::update()
{
  // lastEncoderPos is now a class member to allow syncing when entering edit modes

  // Check for screen saver timeout
  if (screenSaverTimeout > 0)
  {
    unsigned long now = millis();
    if (!screenSaverActive && (now - lastActivityTime) > screenSaverTimeout)
    {
      screenSaverActive = true;
      display->clearDisplay();
      display->display();
      debugMsgMnm("[SCREEN] Screen saver activated");
      return; // Don't process anything while screen saver is active
    }
  }

  // Check for flash message timeout
  if (currentMode == MODE_FLASH_MESSAGE)
  {
    if ((millis() - flashMessageStart) >= flashMessageDuration)
    {
      currentMode = flashPreviousMode;
    }
    // Render flash message but skip input processing
    display->clearDisplay();
    renderFlashMessage();
    display->display();
    return;
  }

  // Check for menu timeout (return to status screen)
  if (currentMode != MODE_STATUS_SCREEN && statusData != NULL)
  {
    unsigned long now = millis();
    if ((now - lastMenuActivity) > menuTimeout)
    {
      debugMsgMnm("[MENU] Menu timeout - returning to status screen");
      showStatusScreen();
      return;
    }
  }

  // Handle encoder rotation
  if (encoderPosition != lastEncoderPos)
  {
    resetActivity(); // Reset screen saver on any activity
    if (currentMode != MODE_STATUS_SCREEN)
    {
      resetMenuActivity(); // Reset menu timeout
    }

    int delta = encoderPosition - lastEncoderPos;
    lastEncoderPos = encoderPosition;

    switch (currentMode)
    {
    case MODE_MENU_NAVIGATION:
    {
      int itemCount = getMenuItemCount(currentMenu);
      int newIndex = selectedIndex + delta;

      // Clamp to valid range - no wraparound
      if (newIndex < 0)
        newIndex = 0;
      if (newIndex >= itemCount)
        newIndex = itemCount - 1;

      // Only update if index actually changed
      if (newIndex != selectedIndex)
      {
        selectedIndex = newIndex;
        selectedItem = getItemAtIndex(selectedIndex);

        // Adjust scroll offset with proper bounds checking
        if (itemCount <= visibleItems)
        {
          // All items fit on screen, no scrolling needed
          scrollOffset = 0;
        }
        else
        {
          // Items don't all fit, need to scroll
          if (selectedIndex < scrollOffset)
          {
            // Selected item is above visible area
            scrollOffset = selectedIndex;
          }
          else if (selectedIndex > scrollOffset + visibleItems - 1)
          {
            // Selected item is below visible area
            scrollOffset = selectedIndex - visibleItems + 1;
          }

          // Ensure scroll offset doesn't go beyond valid range
          int maxScrollOffset = itemCount - visibleItems;
          if (scrollOffset > maxScrollOffset)
          {
            scrollOffset = maxScrollOffset;
          }
          if (scrollOffset < 0)
          {
            scrollOffset = 0;
          }
        }
      }

      break;
    }

    case MODE_NUMERIC_VALUE:
      editingValue += delta * editingStep;
      if (editingValue < editingMin)
        editingValue = editingMin;
      if (editingValue > editingMax)
        editingValue = editingMax;
      break;

    case MODE_STRING_COMPOSITION:
    {
      int charsetLen = strlen(editingCharset);
      editingCharIndex += delta;
      if (editingCharIndex < 0)
        editingCharIndex = charsetLen - 1;
      if (editingCharIndex >= charsetLen)
        editingCharIndex = 0;
      break;
    }

    case MODE_EITHER_OR:
      editingEitherOr = !editingEitherOr;
      break;

    case MODE_STATUS_SCREEN:
      // Call encoder callback if configured
      if (statusEncoderCallback)
      {
        statusEncoderCallback(delta);
      }
      break;
    }
  }

  // Handle button events
  ButtonEvent event = checkButtons();

  if (event != BTN_NONE)
  {
    resetActivity(); // Reset screen saver on any button press
    if (currentMode != MODE_STATUS_SCREEN)
    {
      resetMenuActivity(); // Reset menu timeout
    }
  }

  // Handle button events on status screen
  if (currentMode == MODE_STATUS_SCREEN && event != BTN_NONE)
  {
    // Call custom input callback if set - if it returns true, event was consumed
    if (statusInputCallback && statusInputCallback(event))
    {
      return;
    }
    // Default: encoder click enters menu, ignore all other events
    if (event == BTN_ENCODER_CLICK)
    {
      debugMsgMnm("[STATUS] Entering menu from status screen");
      showMenu();
    }
    return;
  }

  switch (event)
  {
  case BTN_CONFIRM_CLICK:
    if (currentMode == MODE_MENU_NAVIGATION)
    {
      debugMsgMnm("[MENU] Entering item: " + String(selectedItem ? selectedItem->label : "NULL"));
      enterMenuItem(selectedItem);
    }
    else if (currentMode == MODE_NUMERIC_VALUE)
    {
      debugMsgMnm("[VALUE] Saving numeric value: " + String(editingValue));
      *(selectedItem->data.numericData.valuePtr) = editingValue;
      // Call onSave callback if provided
      if (selectedItem->data.numericData.onSave)
      {
        selectedItem->data.numericData.onSave();
      }
      exitCurrentMode();
    }
    else if (currentMode == MODE_STRING_COMPOSITION)
    {
      // Confirm button finishes string entry
      debugMsgMnm("[STRING] Saving string: " + String(editingBuffer));
      // Call onSave callback if provided
      if (selectedItem->data.stringData.onSave)
      {
        selectedItem->data.stringData.onSave();
      }
      exitCurrentMode();
    }
    else if (currentMode == MODE_EITHER_OR)
    {
      debugMsgMnm("[EITHER-OR] Selected: " + String(editingEitherOr ? selectedItem->data.eitherOrData.option1 : selectedItem->data.eitherOrData.option2));
      *(selectedItem->data.eitherOrData.valuePtr) = editingEitherOr;
      // Call onSave callback if provided
      if (selectedItem->data.eitherOrData.onSave)
      {
        selectedItem->data.eitherOrData.onSave();
      }
      exitCurrentMode();
    }
    break;

  case BTN_ENCODER_CLICK: // Encoder button adds character or selects in other modes
    if (currentMode == MODE_MENU_NAVIGATION)
    {
      debugMsgMnm("[MENU] Entering item: " + String(selectedItem ? selectedItem->label : "NULL"));
      enterMenuItem(selectedItem);
    }
    else if (currentMode == MODE_NUMERIC_VALUE)
    {
      debugMsgMnm("[VALUE] Saving numeric value: " + String(editingValue));
      *(selectedItem->data.numericData.valuePtr) = editingValue;
      // Call onSave callback if provided
      if (selectedItem->data.numericData.onSave)
      {
        selectedItem->data.numericData.onSave();
      }
      exitCurrentMode();
    }
    else if (currentMode == MODE_STRING_COMPOSITION)
    {
      // Encoder button adds the current character
      if (editingCursorPos < editingMaxLen - 1)
      {
        editingBuffer[editingCursorPos] = editingCharset[editingCharIndex];
        debugMsgMnm("[STRING] Added character: " + String(editingCharset[editingCharIndex]));
        editingCursorPos++;
        editingBuffer[editingCursorPos] = '\0';
        editingCharIndex = 0;
      }
      else
      {
        debugMsgMnm("[STRING] Max length reached, saving: " + String(editingBuffer));
        // Call onSave callback if provided
        if (selectedItem->data.stringData.onSave)
        {
          selectedItem->data.stringData.onSave();
        }
        exitCurrentMode();
      }
    }
    else if (currentMode == MODE_EITHER_OR)
    {
      debugMsgMnm("[EITHER-OR] Selected: " + String(editingEitherOr ? selectedItem->data.eitherOrData.option1 : selectedItem->data.eitherOrData.option2));
      *(selectedItem->data.eitherOrData.valuePtr) = editingEitherOr;
      // Call onSave callback if provided
      if (selectedItem->data.eitherOrData.onSave)
      {
        selectedItem->data.eitherOrData.onSave();
      }
      exitCurrentMode();
    }
    break;

  case BTN_BACK_CLICK:
    if (currentMode == MODE_MENU_NAVIGATION)
    {
      if (currentMenu->parent)
      {
        MenuItem *parent = currentMenu->parent;
        debugMsgMnm("[MENU] Going back from: " + String(currentMenu->label) + " to parent: " + String(parent->parent ? parent->parent->label : rootMenu->label));
        currentMenu = parent->parent ? parent->parent : rootMenu;
        selectedItem = parent;

        // Find index of parent item
        selectedIndex = 0;
        MenuItem *item = currentMenu->child;
        while (item && item != parent)
        {
          selectedIndex++;
          item = item->next;
        }
        scrollOffset = 0;
      }
      else
      {
        // At root menu - return to status screen
        debugMsgMnm("[MENU] At root, returning to status screen");
        showStatusScreen();
      }
    }
    else if (currentMode == MODE_STRING_COMPOSITION)
    {
      if (editingCursorPos > 0)
      {
        editingCursorPos--;
        editingBuffer[editingCursorPos] = '\0';
        debugMsgMnm("[STRING] Deleted character, now: " + String(editingBuffer));
      }
      else
      {
        debugMsgMnm("[STRING] Cancelled string editing");
        exitCurrentMode();
      }
    }
    else
    {
      debugMsgMnm("[MENU] Cancelled editing mode");
      exitCurrentMode();
    }
    break;

  default:
    break;
  }

  // Render current mode
  display->clearDisplay();

  switch (currentMode)
  {
  case MODE_STATUS_SCREEN:
    renderStatusScreen();
    break;
  case MODE_MENU_NAVIGATION:
    renderMenuNavigation();
    break;
  case MODE_NUMERIC_VALUE:
    renderNumericValue();
    break;
  case MODE_STRING_COMPOSITION:
    renderStringComposition();
    break;
  case MODE_EITHER_OR:
    renderEitherOr();
    break;
  }

  display->display();
}

// Render menu navigation
void MenuSystem::renderMenuNavigation()
{
  const uint8_t lineHeight = 12;
  const uint8_t titleHeight = 12;
  const uint8_t startY = titleHeight + 2;

  // Draw title bar (always visible)
  display->setTextSize(1);
  display->setCursor(0, 0);
  display->print(currentMenu->label);
  display->drawLine(0, 10, screenWidth, 10, SH110X_WHITE);

  // Calculate available space for menu items
  uint8_t availableHeight = screenHeight - startY - 2;
  uint8_t maxVisibleItems = availableHeight / lineHeight;

  // Draw menu items
  MenuItem *item = currentMenu->child;
  uint8_t index = 0;

  // Skip to scroll offset
  for (uint8_t i = 0; i < scrollOffset && item; i++)
  {
    item = item->next;
    index++;
  }

  // Draw visible items
  uint8_t yPos = startY;
  uint8_t itemsDrawn = 0;
  while (item && itemsDrawn < maxVisibleItems && yPos + lineHeight <= screenHeight)
  {
    if (index == selectedIndex)
    {
      display->fillRect(0, yPos, screenWidth, lineHeight, SH110X_WHITE);
      display->setTextColor(SH110X_BLACK);
    }
    else
    {
      display->setTextColor(SH110X_WHITE);
    }

    display->setCursor(4, yPos + 2);
    display->print(item->label);

    // Draw indicators
    if (item->type == MENU_ITEM_SUBMENU)
    {
      display->setCursor(screenWidth - 10, yPos + 2);
      display->print(">");
    }

    display->setTextColor(SH110X_WHITE); // Reset color
    yPos += lineHeight;
    item = item->next;
    index++;
    itemsDrawn++;
  }
}

// Render numeric value editing
void MenuSystem::renderNumericValue()
{
  display->setTextSize(1);
  display->setCursor(0, 0);
  display->print(selectedItem->label);
  display->drawLine(0, 10, screenWidth, 10, SH110X_WHITE);

  // Draw large value
  display->setTextSize(3);
  char valueStr[16];
  snprintf(valueStr, 16, "%d", editingValue);

  int16_t x1, y1;
  uint16_t w, h;
  display->getTextBounds(valueStr, 0, 0, &x1, &y1, &w, &h);
  display->setCursor((screenWidth - w) / 2, 28);
  display->print(valueStr);

  // Draw unit
  if (selectedItem->data.numericData.unit && strlen(selectedItem->data.numericData.unit) > 0)
  {
    display->setTextSize(1);
    display->setCursor((screenWidth - w) / 2 + w + 4, 40);
    display->print(selectedItem->data.numericData.unit);
  }

  // Draw range
  display->setTextSize(1);
  display->setCursor(0, screenHeight - 10);
  display->print(editingMin);
  snprintf(valueStr, 16, "%d", editingMax);
  display->getTextBounds(valueStr, 0, 0, &x1, &y1, &w, &h);
  display->setCursor(screenWidth - w, screenHeight - 10);
  display->print(editingMax);
}

// Render string composition
void MenuSystem::renderStringComposition()
{
  display->setTextSize(1);
  display->setCursor(0, 0);
  display->print(selectedItem->label);
  display->drawLine(0, 10, screenWidth, 10, SH110X_WHITE);

  // Draw current string
  display->setCursor(2, 16);
  display->print(editingBuffer);
  display->print("_");

  // Draw character selector
  display->setTextSize(2);
  char currentChar[2] = {editingCharset[editingCharIndex], '\0'};

  int16_t x1, y1;
  uint16_t w, h;
  display->getTextBounds(currentChar, 0, 0, &x1, &y1, &w, &h);
  display->setCursor((screenWidth - w) / 2, 36);
  display->print(currentChar);

  // Draw prev/next indicators
  display->setTextSize(1);
  int charsetLen = strlen(editingCharset);
  int prevIdx = (editingCharIndex - 1 + charsetLen) % charsetLen;
  int nextIdx = (editingCharIndex + 1) % charsetLen;

  char prevChar[2] = {editingCharset[prevIdx], '\0'};
  char nextChar[2] = {editingCharset[nextIdx], '\0'};

  display->setCursor(4, 42);
  display->print(prevChar);

  display->setCursor(screenWidth - 10, 42);
  display->print(nextChar);

  // Instructions
  display->setCursor(0, screenHeight - 10);
  display->print("Enc:Add OK:Done");
}

// Render either-or selection
void MenuSystem::renderEitherOr()
{
  display->setTextSize(1);
  display->setTextColor(SH110X_WHITE);
  display->setCursor(0, 0);
  display->print(selectedItem->label);
  display->drawLine(0, 10, screenWidth, 10, SH110X_WHITE);

  const char *opt1 = selectedItem->data.eitherOrData.option1;
  const char *opt2 = selectedItem->data.eitherOrData.option2;

  // Draw options side by side
  uint8_t midX = screenWidth / 2;

  // Option 1
  if (editingEitherOr)
  {
    display->fillRect(0, 20, midX - 2, 30, SH110X_WHITE);
    display->setTextColor(SH110X_BLACK);
  }
  else
  {
    display->drawRect(0, 20, midX - 2, 30, SH110X_WHITE);
    display->setTextColor(SH110X_WHITE);
  }

  int16_t x1, y1;
  uint16_t w, h;
  display->setTextSize(2);
  display->getTextBounds(opt1, 0, 0, &x1, &y1, &w, &h);
  display->setCursor((midX - 2 - w) / 2, 28);
  display->print(opt1);

  // Reset text color after Option 1
  display->setTextColor(SH110X_WHITE);

  // Option 2
  if (!editingEitherOr)
  {
    display->fillRect(midX + 2, 20, midX - 2, 30, SH110X_WHITE);
    display->setTextColor(SH110X_BLACK);
  }
  else
  {
    display->drawRect(midX + 2, 20, midX - 2, 30, SH110X_WHITE);
    display->setTextColor(SH110X_WHITE);
  }

  display->getTextBounds(opt2, 0, 0, &x1, &y1, &w, &h);
  display->setCursor(midX + 2 + (midX - 2 - w) / 2, 28);
  display->print(opt2);

  // Reset text color at end
  display->setTextColor(SH110X_WHITE);
}

// Render flash message - centered text with border
void MenuSystem::renderFlashMessage()
{
  display->setTextSize(1);
  display->setTextColor(SH110X_WHITE);

  // Measure the message text
  int16_t x1, y1;
  uint16_t w, h;
  display->getTextBounds(flashMessage, 0, 0, &x1, &y1, &w, &h);

  // Draw a bordered box centered on screen
  uint8_t boxW = w + 16;
  uint8_t boxH = h + 16;
  if (boxW > screenWidth)
    boxW = screenWidth;
  uint8_t boxX = (screenWidth - boxW) / 2;
  uint8_t boxY = (screenHeight - boxH) / 2;

  display->fillRect(boxX, boxY, boxW, boxH, SH110X_BLACK);
  display->drawRect(boxX, boxY, boxW, boxH, SH110X_WHITE);
  display->drawRect(boxX + 1, boxY + 1, boxW - 2, boxH - 2, SH110X_WHITE);

  // Center the text inside the box
  uint8_t textX = (screenWidth - w) / 2;
  uint8_t textY = (screenHeight - h) / 2;
  display->setCursor(textX, textY);
  display->print(flashMessage);
}

// Show a temporary flash message overlay
void MenuSystem::showFlashMessage(const char *message, unsigned long durationMs)
{
  flashPreviousMode = currentMode;
  strncpy(flashMessage, message, sizeof(flashMessage) - 1);
  flashMessage[sizeof(flashMessage) - 1] = '\0';
  flashMessageStart = millis();
  flashMessageDuration = durationMs;
  currentMode = MODE_FLASH_MESSAGE;
}

// Set display contrast
void MenuSystem::setContrast(uint8_t contrast)
{
  display->setContrast(contrast);
}

// Enable screen saver with timeout in milliseconds
void MenuSystem::enableScreenSaver(unsigned long timeoutMs)
{
  screenSaverTimeout = timeoutMs;
  lastActivityTime = millis();
  screenSaverActive = false;
  debugMsgMnm("[SCREEN] Screen saver enabled with timeout: " + String(timeoutMs / 1000));
}

// Disable screen saver
void MenuSystem::disableScreenSaver()
{
  screenSaverTimeout = 0;
  screenSaverActive = false;
  debugMsgMnm("[SCREEN] Screen saver disabled");
}

// Reset activity timer (called on any user interaction)
void MenuSystem::resetActivity()
{
  lastActivityTime = millis();
  if (screenSaverActive)
  {
    screenSaverActive = false;
    debugMsgMnm("[SCREEN] Screen saver deactivated");
  }
}

// Reset menu activity timer
void MenuSystem::resetMenuActivity()
{
  lastMenuActivity = millis();
}

// Set status data pointer
void MenuSystem::setStatusData(StatusData *data)
{
  statusData = data;
  debugMsgMnmX("[STATUS] Status data configured");
}

// Set custom status render callback
void MenuSystem::setStatusRenderCallback(StatusRenderCallback callback)
{
  statusRenderCallback = callback;
  debugMsgMnmX("[STATUS] Custom render callback configured");
}

// Set custom status input callback
void MenuSystem::setStatusInputCallback(StatusInputCallback callback)
{
  statusInputCallback = callback;
  debugMsgMnm("[STATUS] Custom input callback configured");
}

// Set custom status encoder callback
void MenuSystem::setStatusEncoderCallback(StatusEncoderCallback callback)
{
  statusEncoderCallback = callback;
  debugMsgMnm("[STATUS] Custom encoder callback configured");
}

// Show status screen
void MenuSystem::showStatusScreen()
{
  currentMode = MODE_STATUS_SCREEN;
  debugMsgMnm("[STATUS] Showing status screen");
}

// Show menu
void MenuSystem::showMenu()
{
  if (rootMenu)
  {
    currentMode = MODE_MENU_NAVIGATION;
    currentMenu = rootMenu;
    selectedItem = rootMenu->child;
    selectedIndex = 0;
    scrollOffset = 0;
    resetMenuActivity();
    debugMsgMnm("[MENU] Showing menu");
  }
}

void MenuSystem::enterEitherOrEdit(bool *valuePtr, const char *opt1, const char *opt2,
                                   ActionCallback onConfirm, const char *label)
{
  // Create a persistent MenuItem for editing (delete old one if exists)
  if (tempEditItem)
  {
    delete tempEditItem;
  }
  tempEditItem = new MenuItem(label, MENU_ITEM_EITHER_OR);
  tempEditItem->data.eitherOrData.valuePtr = valuePtr;
  tempEditItem->data.eitherOrData.option1 = opt1;
  tempEditItem->data.eitherOrData.option2 = opt2;
  tempEditItem->data.eitherOrData.onSave = onConfirm;

  selectedItem = tempEditItem;
  debugMsgMnm("[EITHER-OR] Editing: " + String(label));
  currentMode = MODE_EITHER_OR;
  editingEitherOr = *valuePtr;
  encoderPosition = 0;
  lastEncoderPos = 0;
  resetMenuActivity();
  resetActivity();
}

// Set menu timeout - this is the timeout to return to the status menu
void MenuSystem::setMenuTimeout(unsigned long timeoutMs)
{
  menuTimeout = timeoutMs;
  debugMsgMnm("[MENU] Menu timeout set to: " + String(timeoutMs / 1000));
}

// Navigate to specific menu
void MenuSystem::navigateToMenu(MenuItem *menu)
{
  if (menu && menu->type == MENU_ITEM_SUBMENU)
  {
    currentMenu = menu;
    selectedItem = menu->child;
    selectedIndex = 0;
    scrollOffset = 0;
    resetActivity();
  }
}