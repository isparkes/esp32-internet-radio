#include "DebugManager.h"

// ************************************************************
// Set if we are to send messages to the serial port or not
// ************************************************************
void DebugManager_::setDebuggingOutput(bool newState) {
  _state = newState;
}

// ************************************************************
// Output a logging message to the debug output, if set, 
// adding prefix
// ************************************************************
void DebugManager_::debugMsg(String prefix, String message) {
  if (_state) {
    Serial.println(prefix + ": " + message);
    Serial.flush();
  }
}

// ************************************************************
// Output a logging message to the debug output, if set, 
// Continued messages don't need a prefix
// ************************************************************
void DebugManager_::debugMsgCont(String message) {
  if (_state) {
    Serial.print(message);
    Serial.flush();
  }
}

// ************************************************************
// Called once per second, we can turn debugging off after a
// defined number of seconds
// ************************************************************
void DebugManager_::debugAutoOffCheck() {
  if (_state) {
    if (_debugForSecs > 0) {
      _debugForSecs--;
    }
    if (_debugForSecs == 0) {
      debugMsg("[DBG]", "Auto off");
      _state = false;
    }
  }
}

// ************************************************************
// Is debug on right now
// ************************************************************
bool DebugManager_::isDebugOn() {
  return _state;
}

// ************************************************************
// Set the number of seconds we should produce debug messages
// for
// ************************************************************
void DebugManager_::setDebugAutoOff(unsigned int seconds) {
  _debugForSecs = seconds;
  _state = true;
  debugMsg("[DBG]", "Debug on for " + String(seconds) + " seconds");
}

// ************************************************************
// Library internal singleton wiring
// ************************************************************
DebugManager_ &DebugManager_::getInstance() {
  static DebugManager_ instance;
  return instance;
}

// ************************************************************
// This is a free function helper to allow the member function to
// be called from a callback.
// ************************************************************
void debugManagerLink(String message) {
  debugManager.debugMsg("[CBK]: ", message);
}

DebugManager_ &debugManager = debugManager.getInstance();