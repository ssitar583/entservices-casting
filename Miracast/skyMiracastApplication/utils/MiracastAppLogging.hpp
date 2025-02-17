/*
    Copyright (C) 2021 Apple Inc. All Rights Reserved. Not to be used or disclosed without permission from Apple.
*/
#ifndef MiracastAppLogging_hpp
#define MiracastAppLogging_hpp
#define kLoggingFacilityAppEngine "MiracastApp.Engine"
#define MIRACASTAPP_APP_LOG "MiracastApp"
#define MIRACASTAPP_MARKER "MIRACASTAPP_MARKER"
extern int Level_Limit;
#include <string>
#include <syslog.h>
#include "MiracastLogger.h"

//TODO: Flush out logging
namespace MiracastApp {
namespace Logging {
void setLogLevel();
}
}
#endif /* MiracastAppLogging_hpp */
