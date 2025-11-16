#pragma once

#include <Arduino.h>
#include "Globals.h"
#include "Defs.h"

// count0 is used to flah the status led 
#define COUNT0_MAX 1000
#define COUNT0_OFF 100

// ************************************************************
// Public prototypes
// ************************************************************
void startTimers();
void setLedFlashType(byte flashType);
