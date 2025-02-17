#include "CrashLogger.h"

#include <fstream>
#include <unordered_map>
#include <unistd.h>
#include "MiracastAppLogging.hpp"

using namespace IReport;

namespace CrashLogger
{
    static const std::string crashReportDir = "/home/private/miracastapp/";

    std::string toString(IReport::TerminationReason reason)
    {
        std::unordered_map<IReport::TerminationReason, std::string> map =
        {
        {IReport::REASON_LOW_RESOURCES, "REASON_LOW_RESOURCES"},
        {IReport::REASON_USER_KILL, "REASON_USER_KILL"},
        {IReport::REASON_USER_NAVIGATION, "REASON_USER_NAVIGATION"},
        {IReport::REASON_MAINTENANCE, "REASON_MAINTENANCE"},
        {IReport::REASON_SYSTEM_ERROR, "REASON_SYSTEM_ERROR"},
        {IReport::REASON_UNRESPONSIVE, "REASON_UNRESPONSIVE"},
        {IReport::REASON_TIMEOUT_EXIT, "REASON_TIMEOUT_EXIT"},
        {IReport::REASON_TIMEOUT_SUSPEND, "REASON_TIMEOUT_SUSPEND"},
        {IReport::REASON_INACTIVITY, "REASON_INACTIVITY"},
        {IReport::REASON_OTHER, "REASON_OTHER"},
        {IReport::REASON_INVALID_INSTRUCTION, "REASON_INVALID_INSTRUCTION"},
        {IReport::REASON_INVALID_ARITHMETIC_OPERATION, "REASON_INVALID_ARITHMETIC_OPERATION"},
        {IReport::REASON_INVALID_MEMORY_REFERENCE, "REASON_INVALID_MEMORY_REFERENCE"},
        {IReport::REASON_UNALIGNED_MEMORY_ACCESS, "REASON_UNALIGNED_MEMORY_ACCESS"}
        };

        auto it = map.find(reason);
        if (it != map.end())
        {
            return it->second;
        }

        return "REASON_OTHER";
    }
    void logCrash(const IReport::TerminationReason reason, const unsigned address, const std::string& comment)
    {
        std::string reportFile = crashReportDir + "/miracastapp_report";
        std::ofstream os(reportFile.c_str(), std::ios::trunc);

        std::string crashReason = toString(reason);

        if (os)
        {
            MIRACASTLOG_INFO("Writing crash reason '%s', address 0x%X and comment '%s' to report file (%s)",
                crashReason.c_str(), address, comment.c_str(), reportFile.c_str());

            os << crashReason << std::endl;
            os << address << std::endl;
            os << comment << std::endl;
            os.close();
        }
        else
        {
            MIRACASTLOG_WARNING("Failed to write crash '%s' to file %s", crashReason.c_str(), reportFile.c_str());
        }
    }

    bool reportExists()
    {
        std::string reportFile = crashReportDir + "/miracastapp_report";

        return access(reportFile.c_str(), F_OK) == 0;
    }

}