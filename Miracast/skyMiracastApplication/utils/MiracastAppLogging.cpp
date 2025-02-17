/*
    Copyright (C) 2021 Apple Inc. All Rights Reserved. Not to be used or disclosed without permission from Apple.
*/
#include "MiracastAppLogging.hpp"
#include <iostream>
#include <cstdlib>
#include <syslog.h>
#include <stdarg.h>
#include <libgen.h>
#include "MiracastLogger.h"

int Level_Limit = MIRACAST::TRACE_LEVEL;

namespace MiracastApp {
namespace Logging {
void setLogLevel()
{
    char * log_level_limit = getenv("MIRACAST_LOG_LEVEL");
    if(log_level_limit!=NULL)
    {
        Level_Limit=std::stoi(log_level_limit);
    }
}
}
}

namespace MIRACAST
{
    void log(LogLevel level,
             const char *func,
             const char *file,
             int line,
             int threadID,
             const char *format, ...)
    {
        const char *levelMap[] = {"FATAL", "ERROR", "WARN", "INFO", "VERBOSE", "TRACE"};
        const short kFormatMessageSize = 4096;
        char formatted[kFormatMessageSize];
        int logLevel = -1;

        switch (level)
        {
            case FATAL_LEVEL:
            {
                logLevel = LOG_CRIT;
            }
            break;
            case ERROR_LEVEL:
            {
                logLevel = LOG_ERR;
            }
            break;
            case WARNING_LEVEL:
            {
                logLevel = LOG_WARNING;
            }
            break;
            case INFO_LEVEL:
            {
                logLevel = LOG_INFO;
            }
            break;
            case VERBOSE_LEVEL:
            case TRACE_LEVEL:
            {
                logLevel = LOG_DEBUG;
            }
            break;
            default:
                break;
        }

        if ((-1==logLevel)||(level > Level_Limit))
        {
            return;
        }

        va_list argptr;
        va_start(argptr, format);
        vsnprintf(formatted, kFormatMessageSize, format, argptr);
        va_end(argptr);

        syslog(logLevel,"[MiracastApp][%d] %s [%s:%d] %s: %s \n",
                    (int)syscall(SYS_gettid),
                    levelMap[static_cast<int>(level)],
                    basename(file),
                    line,
                    func,
                    formatted);
    }
}
