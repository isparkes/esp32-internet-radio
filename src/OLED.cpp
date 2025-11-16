#include "OLED.h"

void OLED_::setUp()
{
  // Set up based on the OLED display we are using
  // SSD_1306 is the 0.96" and 2.42"
  // SH_1106 is the 1.30"
  bool oledInitialized = false;
  #ifdef OLED_SSD1306
  _display.reset(new Adafruit_SSD1306(128,64, &Wire, -1));
  oledInitialized = _display->begin(SSD1306_SWITCHCAPVCC, 0x3C, true, true);
  #endif
  #ifdef OLED_SH1106
  _display.reset(new Adafruit_SH1106G(128,64, &Wire, -1));
  oledInitialized = _display->begin(0x3C, true);
  #endif

  if(!oledInitialized) {
    debugMsgMnm("OLED allocation failed");
  }else {
    debugMsgMnmX("OLED allocation OK!");
  }

  _display->setTextSize(1);
  _display->setTextColor(WHITE, BLACK);
  _display->clearDisplay();
}

void OLED_::clearDisplay()
{
  _blanked = false;
  _display->clearDisplay();
  _bufferIdx = 0;
  for (int i = 5 ; i > 0 ; i--) {
    _bufferLines[i] = "";
  }
}

void OLED_::blankDisplay()
{
  _display->clearDisplay();
  _display->display();
  _blanked = true;
}

bool OLED_::getBlanked()
{
  return _blanked;
}

void OLED_::outputDisplay()
{
  _display->display();
}

// ************************************************************
// Clear the scrolling message buffer
// ************************************************************
void OLED_::clearScrollingMessage()
{
  for (int tmpBuffer =  0 ; tmpBuffer < 6; tmpBuffer++) {
    _bufferLines[tmpBuffer] = "";
  }
  _bufferIdx = 0;  
}

// ************************************************************
// Add a message to the scrolling message buffer
// ************************************************************
void OLED_::showScrollingMessage(String messageText)
{
  String formattedString = messageText + "                   ";
  formattedString = formattedString.substring(0,20);
  if (_bufferIdx < 6) {
    _bufferLines[_bufferIdx] = formattedString;
    _bufferIdx++;
  } else {
    for (int i = 1 ; i < 6 ; i++) {
      _bufferLines[i-1] = _bufferLines[i];
    }
    _bufferLines[5] = formattedString;
  }

  _display->setCursor(0,0);
  for (int i = 0 ; i < 6 ; i++) {
    _display->println(_bufferLines[i]);
  }
  _display->display();
}

// ************************************************************
// Build the status line
// ************************************************************
void OLED_::showStatusLine()
{
  _display->fillRect(STATUS_BOX_X, STATUS_BOX_Y, STATUS_BOX_W, STATUS_BOX_H, BLACK);
  _display->drawRect(STATUS_BOX_X, STATUS_BOX_Y, STATUS_BOX_W, STATUS_BOX_H, WHITE);
  drawWiFiInd();
  drawRunInd();
  drawEndInd();
  drawTubeTypeInd();
  drawValueInd();
  drawRepetitionsInd();
  _display->display();
}

// --------------------------------------------------

void OLED_::drawWiFiInd() {
  _display->setCursor(WIFI_IND_X,STATUS_LINE_Y);
  if (WiFi.isConnected()) {
    _display->print("W");
  } else {
    _display->print("w");
  }
}

// --------------------------------------------------

void OLED_::drawRunInd() {
  _display->setCursor(RUN_IND_X,STATUS_LINE_Y);
  _display->print("-");
}

// --------------------------------------------------

void OLED_::drawEndInd() {
  _display->setCursor(END_IND_X,STATUS_LINE_Y);
  _display->print("-");
}

// --------------------------------------------------

void OLED_::drawTubeTypeInd() {
  _display->setCursor(TUBE_IND_X,STATUS_LINE_Y);
  _display->print("T:70");
}

// --------------------------------------------------

void OLED_::drawValueInd() {
  _display->setCursor(VAL_IND_X,STATUS_LINE_Y);
  _display->print("V:");
}

// --------------------------------------------------

void OLED_::drawRepetitionsInd() {
  _display->setCursor(REP_IND_X,STATUS_LINE_Y);
  _display->print("R:");
}

// ************************************************************
// Show a menu heading
// ************************************************************
void OLED_::showMenuHeading(String menuText) {
  _display->setCursor(0,0);
  _display->setTextColor(BLACK, WHITE);
  _display->print(menuText);
  _display->setTextColor(WHITE, BLACK);
}

// ************************************************************
// Show a menu entry
// ************************************************************
void OLED_::showMenuEntry(byte level, String menuText) {
  int16_t posX = level*4;
  int16_t posY = 4 + level*8;
  _display->setCursor(posX,posY);
  _display->print(menuText);
}

// ************************************************************
// Blank a menu entry
// ************************************************************
void OLED_::blankMenuEntry(byte level) {
  int16_t posX = level*4;
  int16_t posY = 4 + level*8;
  _display->setCursor(posX,posY);
  _display->print("              ");
}

void OLED_::setTextColor(uint16_t c) {
  _display->setTextColor(c);
}

void OLED_::setTextColor(uint16_t c, uint16_t bg) {
  _display->setTextColor(c, bg);
}

void OLED_::setCursor(int16_t x, int16_t y) {
  _display->setCursor(x, y);
}

void OLED_::setTextSize(uint8_t s) {
  _display->setTextSize(s);
}

void OLED_::println(const String &s) {
  _display->println(s);
}

void OLED_::drawLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t color) {
  _display->drawLine(x0, y0, x1, y1, color);
}

int16_t OLED_::width() {
  return _display->width();
}

int16_t OLED_::height() {
  return _display->height();
}

OLED_ &OLED_::getInstance() {
  static OLED_ instance;
  return instance;
}

OLED_ &oled = oled.getInstance();