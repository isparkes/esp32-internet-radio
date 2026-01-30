#pragma once

#include <Arduino.h>
#include "Configuration.h"

typedef void (*DebugCallback) (String);

// Extended debug settings - these allow trace level debugging
#define INR_EXTENDED_DEBUG_OFF
#define OTM_EXTENDED_DEBUG_OFF
#define WFM_EXTENDED_DEBUG_OFF
#define SPF_EXTENDED_DEBUG_OFF
#define CDM_EXTENDED_DEBUG_OFF
#define CMG_EXTENDED_DEBUG_OFF
#define UTL_EXTENDED_DEBUG_OFF
#define MNM_EXTENDED_DEBUG_OFF
#define AUD_EXTENDED_DEBUG

// Basic debug settings
#ifdef DEBUG
#define debugMsgInr(message) debugManager.debugMsg("[INR]", message);
#define debugMsgOtm(message) debugManager.debugMsg("[OTM]", message);
#define debugMsgSpf(message) debugManager.debugMsg("[SPF]", message);
#define debugMsgWbm(message) debugManager.debugMsg("[WEB]", message);
#define debugMsgUtl(message) debugManager.debugMsg("[UTL]", message);
#define debugMsgWfm(message) debugManager.debugMsg("[WFM]", message);
#define debugMsgCdm(message) debugManager.debugMsg("[CDM]", message);
#define debugMsgCmg(message) debugManager.debugMsg("[GMC]", message);
#define debugMsgMnm(message) debugManager.debugMsg("[MNM]", message);
#define debugMsgAud(message) debugManager.debugMsg("[AUD]", message);
#else
#define debugMsgMain(message)
#define debugMsgOtm(message)
#define debugMsgSpf(message)
#define debugMsgWbm(message)
#define debugMsgUtl(message)
#define debugMsgWfm(message)
#define debugMsgCdm(message)
#define debugMsgCmg(message)
#define debugMsgMnm(message)
#define debugMsgAud(message)
#endif

// Extended debug settings
#ifdef INR_EXTENDED_DEBUG
#define debugMsgInrX(message) debugManager.debugMsg("[INR]", message);
#else
#define debugMsgInrX(message)
#endif

#ifdef SPF_EXTENDED_DEBUG
#define debugMsgSpfX(message) debugManager.debugMsg("[UTL]", message);
#else
#define debugMsgSpfX(message)
#endif

#ifdef MNM_EXTENDED_DEBUG
#define debugMsgMnmX(message) debugManager.debugMsg("[MNM]", message);
#else
#define debugMsgMnmX(message)
#endif

#ifdef AUD_EXTENDED_DEBUG
#define debugMsgAudX(message) debugManager.debugMsg("[AUD]", message);
#else
#define debugMsgAudX(message)
#endif

class DebugManager_ {
  private:
    DebugManager_() = default; // Make constructor private

  public:
    static DebugManager_ &getInstance(); // Accessor for singleton instance

    DebugManager_(const DebugManager_ &) = delete; // no copying
    DebugManager_ &operator=(const DebugManager_ &) = delete;

  public:
    void setDebuggingOutput(bool newState);
    void debugMsg(String prefix, String message);
    void debugMsgCont(String message);
    void setDebugAutoOff(unsigned int seconds);
    void debugAutoOffCheck();
    bool isDebugOn();

    // Some components need to use a callback
    DebugCallback getDebugCallBack();

  private:
    bool _state = true;
    unsigned int _debugForSecs = 0;
};

// free function link to the class function
extern void debugManagerLink(String message);

extern DebugManager_ &debugManager;