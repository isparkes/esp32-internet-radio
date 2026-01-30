#include "TimerManager.h"

hw_timer_t * timer0 = NULL;
portMUX_TYPE timerMux0 = portMUX_INITIALIZER_UNLOCKED;

hw_timer_t * timer1 = NULL;
extern portMUX_TYPE timerMux1;

hw_timer_t * timer2 = NULL;
portMUX_TYPE timerMux2 = portMUX_INITIALIZER_UNLOCKED;

volatile int count0;
volatile int count0Max = COUNT0_MAX;
volatile int count0Off = COUNT0_OFF;

volatile uint32_t _dispVal = 0;
volatile uint8_t _dispBoardCount = 3;

// ************************************************************
// ISR for LED flash update
// ************************************************************
void IRAM_ATTR onTimer0() {
   portENTER_CRITICAL_ISR(&timerMux0);
   count0++;
   if (count0 > count0Max) {
     count0 = 0;
     digitalWrite(LED_PIN, HIGH);
   } else if (count0 == count0Off) {
     digitalWrite(LED_PIN, LOW);
   }
   portEXIT_CRITICAL_ISR(&timerMux0);
}

// ************************************************************
// Start the timers
// ************************************************************
void startTimers() {
  pinMode(LED_PIN, OUTPUT);

  // LED flash timer
  timer0 = timerBegin(0, 80, true);
  timerAttachInterrupt(timer0, &onTimer0, true);
  timerAlarmWrite(timer0, 10000, true);
  // https://community.platformio.org/t/hardware-timer-issue-with-esp32/22047/10
  delayMicroseconds(0);
  timerAlarmEnable(timer0);

  // Set default LED flash type
  setLedFlashType(1);
}

// ************************************************************
// Set the LED flash type
// 0: Connected - short flash 1/s
// 1: Connecting - long flash 2/3s
// ************************************************************
void setLedFlashType(byte flashType) {
  switch(flashType) {
    case 0: {
      count0Max = 100;
      count0Off = 1;
      break;
    }
    case 1: {
      count0Max = 100;
      count0Off = 50;
      break;
    }
    case 2: {
      count0Max = 50;
      count0Off = 25;
      break;
    }
    default: {
      count0Max = 200;
      count0Off = 150;
      break;
    }
  }
}
