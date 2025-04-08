/*
 * If not stated otherwise in this file or this component's Licenses.txt file the
 * following copyright and licenses apply:
 *
 * Copyright 2025 RDK Management
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
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