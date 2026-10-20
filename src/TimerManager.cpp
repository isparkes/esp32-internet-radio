#include "TimerManager.h"

// LED pattern state. Written from the main loop and read by the LED task, so
// a pattern change can cost at most one ragged blink - cosmetic only.
static volatile byte ledMode = LED_MODE_OFF;
static volatile unsigned long ledOnMs = 0;
static volatile unsigned long ledOffMs = 0;
static volatile bool ledLit = false;
static volatile unsigned long ledLastChange = 0;

static TaskHandle_t ledTaskHandle = nullptr;

// ************************************************************
// LED service task
//
// setup() blocks in several places (SPIFFS, WiFi, the startup jingle), so the
// flashing can't be driven from the main loop alone - a task keeps the pattern
// running from the moment the board boots.
// ************************************************************
static void ledTask(void *param) {
  (void)param;
  for (;;) {
    ledOncePerLoop();
    vTaskDelay(pdMS_TO_TICKS(10));
  }
}

// ************************************************************
// Drive the LED pin, honouring the board's LED polarity
// ************************************************************
static void writeLed(bool lit) {
  ledLit = lit;
  digitalWrite(LED_PIN, lit ? LED_ON_LEVEL : !LED_ON_LEVEL);
}

// ************************************************************
// Start the status LED. Called first thing in setup() so the rapid
// startup flash is the earliest sign that the board is alive.
// ************************************************************
void startLed() {
  pinMode(LED_PIN, OUTPUT);
  setLedFlashType(LED_MODE_STARTUP);

  // Lowest priority - the LED must never get in the way of audio. If the task
  // can't be created the main loop still services the pattern, it just won't
  // flash during the blocking parts of setup().
  if (xTaskCreate(ledTask, "led", 2048, NULL, 1, &ledTaskHandle) != pdPASS) {
    ledTaskHandle = nullptr;
  }
}

// ************************************************************
// Select the LED flash pattern
// LED_MODE_OFF:     radio stopped - dark
// LED_MODE_PLAYING: radio playing - slow flash
// LED_MODE_STARTUP: booting - rapid flash
// LED_MODE_RESYNC:  stream dropped, reconnecting - medium flash
// ************************************************************
void setLedFlashType(byte flashType) {
  if (flashType == ledMode) return;   // Don't restart a pattern already running

  ledMode = flashType;

  switch (ledMode) {
    case LED_MODE_PLAYING: {
      ledOnMs  = LED_PLAYING_ON_MS;
      ledOffMs = LED_PLAYING_OFF_MS;
      break;
    }
    case LED_MODE_STARTUP: {
      ledOnMs  = LED_STARTUP_ON_MS;
      ledOffMs = LED_STARTUP_OFF_MS;
      break;
    }
    case LED_MODE_RESYNC: {
      ledOnMs  = LED_RESYNC_ON_MS;
      ledOffMs = LED_RESYNC_OFF_MS;
      break;
    }
    default: {
      ledOnMs  = 0;
      ledOffMs = 0;
      break;
    }
  }

  // Take effect now rather than at the end of the pattern we were part way
  // through: dark immediately when stopping, lit immediately when starting.
  ledLastChange = millis();
  writeLed(ledMode != LED_MODE_OFF);
}

// ************************************************************
// Service the LED flash pattern - called by the LED task, and by the main
// loop as a fallback should the task not have been created
// ************************************************************
void ledOncePerLoop() {
  if (ledMode == LED_MODE_OFF) {
    if (ledLit) {
      writeLed(false);
    }
    return;
  }

  unsigned long interval = ledLit ? ledOnMs : ledOffMs;
  if ((millis() - ledLastChange) >= interval) {
    ledLastChange = millis();
    writeLed(!ledLit);
  }
}
