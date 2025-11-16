#pragma once

#include "Defs.h"
#include <memory>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_SH110X.h>
#include "TimeLib.h"
#include "Globals.h"
#include "DebugManager.h"

// ----------------------- Defines -----------------------

#define STATUS_LINE_Y 54
#define WIFI_IND_X    5
#define RUN_IND_X     12
#define END_IND_X     19
#define TUBE_IND_X    29
#define VAL_IND_X     57
#define REP_IND_X     100

#define STATUS_BOX_X   0
#define STATUS_BOX_Y  52
#define STATUS_BOX_W 127
#define STATUS_BOX_H  12

// ----------------------------------------------------------------------------------------------------
// ------------------------------------------ OLED Component ------------------------------------------
// ----------------------------------------------------------------------------------------------------

class OLED_
{
  private:
    OLED_() = default; // Make constructor private

  public:
    static OLED_ &getInstance(); // Accessor for singleton instance

    OLED_(const OLED_ &) = delete; // no copying
    OLED_ &operator=(const OLED_ &) = delete;

  public:
    void setUp();
    void showMenuHeading(String menuText);
    void showMenuEntry(byte level, String menuText);
    void blankMenuEntry(byte level);
    void showStatusLine();
    void clearDisplay();
    void blankDisplay();
    bool getBlanked();
    void outputDisplay();
    void clearScrollingMessage();
    void showScrollingMessage(String messageText);
    void setTimeString(String timeText);

    // Hacks for menu manager
    void setTextColor(uint16_t c);
    void setTextColor(uint16_t c, uint16_t bg);
    void setCursor(int16_t x, int16_t y);
    void setTextSize(uint8_t s);
    void println(const String &s);
    void drawLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t color);
    int16_t width();
    int16_t height();

  private:
  bool _blanked;
    String _bufferLines[6] = {"","","","","",""};
    byte _bufferIdx = 0;
    #ifdef OLED_SSD1306
    std::unique_ptr<Adafruit_SSD1306> _display;
    #endif

    #ifdef OLED_SH1106
    std::unique_ptr<Adafruit_SH1106G> _display;
    #endif

    void drawWiFiInd();
    void drawRunInd();
    void drawEndInd();
    void drawTubeTypeInd();
    void drawValueInd();
    void drawRepetitionsInd();
};

extern OLED_ &oled;
