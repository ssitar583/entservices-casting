#ifndef CRASHLOGGER_H
#define CRASHLOGGER_H

#include <string>

namespace IReport{
    enum TerminationReason{
        REASON_LOW_RESOURCES,
        REASON_USER_KILL,
        REASON_USER_NAVIGATION,
        REASON_MAINTENANCE,
        REASON_SYSTEM_ERROR,
        REASON_UNRESPONSIVE,
        REASON_TIMEOUT_EXIT,
        REASON_TIMEOUT_SUSPEND,
        REASON_INACTIVITY,
        REASON_OTHER,
        REASON_INVALID_INSTRUCTION,
        REASON_INVALID_ARITHMETIC_OPERATION,
        REASON_INVALID_MEMORY_REFERENCE,
        REASON_UNALIGNED_MEMORY_ACCESS
    };
}
namespace CrashLogger
{
    void logCrash(const IReport::TerminationReason reason, const unsigned address, const std::string& comment);
    bool reportExists();
    std::string toString(IReport::TerminationReason reason);	
}

#endif // CRASHLOGGER_H