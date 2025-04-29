#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <signal.h>
#include <sys/resource.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fstream>
#include <cstring>

#include "MiracastAppMgr.h"
#include "MiracastAppLogging.hpp"
#include "CrashLogger.h"

#include <QGuiApplication>
#include <QImage>
#include <QPainter>
#include <QFontDatabase>
#include <QFont>

#ifdef ENABLE_BREAKPAD
#include "client/linux/handler/exception_handler.h"
#endif

using namespace IReport;
using namespace CrashLogger;

enum Reason {
    Reason_USER_Kill                    = 1,    // User requests MiracastApp application to self-terminate
    Reason_USER_Nav                     = 2,    // User navigated away from application (ex: press Home)
    Reason_AM_LowResource               = 4,    // AM terminated application due to low resources
    Reason_Unknown                      = 98,   // Used if AM cannot relay the above enums
    Reason_Testing                      = 99,   // Can be used during testing/development phase
    Reason_LAST_ENTRY_POSITION
};

static int exitSignal = 0;
static Reason exitReason = Reason_USER_Kill;
static bool didCrash = false;
static std::string miracastappWriteDir = "";

#ifdef ENABLE_BREAKPAD
static bool breakpadDumpCallback(const google_breakpad::MinidumpDescriptor& descriptor,
								void* context,
								bool succeeded)
{
	MIRACASTLOG_INFO("\nbreakpadDumpCallback: MiracastAppPluginApp minidump ---- Dump path: %s\n",descriptor.path());
    
  didCrash = true;
	return false; // we want our signal handler to continue processing to write crash file for MiracastApp
}

const int kExceptionSignals[] = {
  SIGSEGV, SIGABRT, SIGFPE, SIGILL, SIGBUS, SIGTERM, SIGINT, SIGALRM
};
const int kNumHandledSignals =
    sizeof(kExceptionSignals) / sizeof(kExceptionSignals[0]);
struct sigaction old_handlers[kNumHandledSignals];

#endif

void writeCrashFile(int signal)
{
  std::string reason, dump;
  MIRACASTLOG_VERBOSE("breakpad sky-MiracastAppPlugin: entry\n");

  switch(signal)
  {
     
    case SIGABRT:
        reason = CrashLogger::toString(IReport::REASON_OTHER);
        dump = "SIGABRT received.";
        break;

    case SIGILL:
        reason = CrashLogger::toString(IReport::REASON_INVALID_INSTRUCTION);
        dump = "SIGILL received.";
        break;

    case SIGFPE:
        reason = CrashLogger::toString(IReport::REASON_INVALID_ARITHMETIC_OPERATION);
        dump = "SIGFPE received.";
        break;

    case SIGSEGV:
        reason = CrashLogger::toString(IReport::REASON_UNALIGNED_MEMORY_ACCESS);
        dump = "SIGSEGV received.";

    case SIGBUS:
        reason = CrashLogger::toString(IReport::REASON_INVALID_MEMORY_REFERENCE);
        dump = "SIGBUS received.";
        break;

    case SIGINT:
        dump = "SIGINT received.";
        reason = CrashLogger::toString(IReport::REASON_USER_KILL);
        break;

    case SIGKILL:
        reason = CrashLogger::toString(IReport::REASON_USER_KILL);
        dump = "SIGKILL received.";
        break;

    case SIGALRM:
        reason = CrashLogger::toString(IReport::REASON_LOW_RESOURCES);
        dump = "SIGALRM received.";
        break;
      
  }
  
  std::string reportFile = miracastappWriteDir + "/miracastapp_report";
  std::ofstream os(reportFile.c_str(), std::ios::trunc);
  if (os && reason.size() > 0) 
  {
    os << reason;
    os << dump;
    os.close();
  }
  MIRACASTLOG_VERBOSE("breakpad sky-MiracastAppPlugin: exit\n");
}

// handle SIGINT and SIGTERM and custom signals
void SignalHandler(int sig)
{
    MIRACASTLOG_ERROR("Connection_Error:breakpad sky-MiracastAppPlugin: entry \n");
    MIRACASTLOG_ERROR("Connection_Error :\n[SIG_HANDLER] Signal=%d caught by SignalHandler from callback!\n", sig);

#ifdef ENABLE_BREAKPAD

    //allow signal to be processed normally for correct core dump
    MIRACASTLOG_VERBOSE("breakpad sky-MiracastAppPlugin: Restoring breakpad signal handlers\n");
    for (int i = 0; i < kNumHandledSignals; ++i) {
      if (sigaction(kExceptionSignals[i], &old_handlers[i], NULL) == -1) {
    	  MIRACASTLOG_ERROR("breakpad sky-MiracastAppPlugin: failed to set signal: %s \n",strsignal(kExceptionSignals[i]));
          signal(kExceptionSignals[i], SIG_DFL);
      }
    }

    if(1)
    {
      MIRACASTLOG_VERBOSE("writing MiracastApp crash file\n");
      writeCrashFile(sig);
      //signal(sig, SIG_DFL); avoid reset the signal to default before raising the signal
      raise(sig);
      return;
    }
#endif
    // Do graceful shutdown based on signals
    // SIGTERM and SIGINT will be considerd normal shutdowns
    // SIGUSR1 will be used to signify that suspend or resume got stuck
    // SIGALRM will be used to signify miracastapp should be shutdown becuase of low system resources
    if(sig == SIGTERM || sig == SIGINT || sig == SIGUSR1 || sig == SIGALRM)
    {
        exitSignal = sig;
        //if(nrdApp() == nullptr)
        {
            MIRACASTLOG_ERROR("MiracastApplication::instance() is NULL while trying to quit, shutting down immediately\n");
            fflush(stdout);
            _exit(0);
        }
    }else
    {
        exit(sig);
    }
    MIRACASTLOG_VERBOSE("breakpad sky-MiracastAppPlugin: exit \n");
}

bool addSignalHandling()
{
    struct sigaction act;

    sigset_t mask;
    sigset_t orig_mask;

    MIRACASTLOG_VERBOSE("breakpad sky-MiracastAppPlugin: entry \n");

#ifdef ENABLE_BREAKPAD
    MIRACASTLOG_INFO("breakpad sky-MiracastAppPlugin: backing up handlers\n");
            //Backup breakpad signal handlers in old_handlers
            for (int i = 0; i < kNumHandledSignals; ++i) {
              if (sigaction(kExceptionSignals[i], NULL, &old_handlers[i]) == -1) {
            	  MIRACASTLOG_ERROR("Connection_Error: breakpad sky-MiracastAppPlugin: failed to backup signal: %s \n",strsignal(kExceptionSignals[i]));
                   break;
              }
            }

            memset (&act, 0, sizeof(act));
            act.sa_handler = SignalHandler;
    MIRACASTLOG_INFO("breakpad sky-MiracastAppPlugin: setting up handlers\n");
            //Override signals with the desired signal handler
            for (int i = 0; i < kNumHandledSignals; ++i) {
              if (sigaction(kExceptionSignals[i], &act, NULL)) {
            	  MIRACASTLOG_ERROR("Connection_Error:breakpad sky-MiracastAppPlugin: failed to set signal: %s \n",strsignal(kExceptionSignals[i]));
              }
            }
#endif

    memset (&act, 0, sizeof(act));
    act.sa_handler = SignalHandler;

    // catch SIGINT and SIGTERM for proper shutdown
    if (sigaction(SIGTERM, &act, 0)) {
        MIRACASTLOG_ERROR("sigaction() failed to add SIGTERM\n");
        return false;
    }
    if (sigaction(SIGINT, &act, 0)) {
        MIRACASTLOG_ERROR("sigaction() failed to add SIGINT\n");
       return false;
    }
    // handle custom signals so we can report better shutdown reasons to Netlix
    // SIGUSR1 will be used to notify that we got stuck in suspend or resume
    // SIGALRM will be used to notify that the app shutdown because of low resources
    // SIGUSR2 doesn't seem to work for some reason...
    if (sigaction(SIGUSR1, &act, 0)) {
        MIRACASTLOG_ERROR("sigaction() failed to add SIGUSR1\n");
        return false;
    }
    if (sigaction(SIGALRM, &act, 0)) {
        MIRACASTLOG_ERROR("sigaction() failed to add SIGALRM\n");
       return false;
    }
    // follow miracastapp binary and an executable and ignore SIGPIPE
    // we don't want the app to terminate if a write or send call fails
    signal(SIGPIPE, SIG_IGN);
    sigemptyset (&mask);
    sigaddset (&mask, SIGTERM);

#ifdef ENABLE_BREAKPAD
            for (int i = 0; i < kNumHandledSignals; ++i) {
                 sigaddset(&mask, kExceptionSignals[i]);
            }
#endif

    if (sigprocmask(SIG_UNBLOCK, &mask, &orig_mask) < 0) {
        MIRACASTLOG_ERROR("sigprocmask() failed to unblock SIGTERM\n");
        return false;
    }

    MIRACASTLOG_VERBOSE("SIGTERM, SIGINT, SIGUSR1, and SIGALRM handlers installed...\n");
    MIRACASTLOG_VERBOSE("breakpad sky-MiracastAppPlugin: exit \n");
    return true;
}

int main(int argc, char *argv[])
{
    int returnStatus = -1;
    MiracastAppMgr* miracastAppMgr = nullptr;
    MIRACASTLOG_VERBOSE("sky-MiracastAppPlugin: Entered \n");
    qputenv("QT_QPA_FONTDIR", "/usr/share/fonts/ttf");
    QGuiApplication app(argc, argv);
    addSignalHandling();
    miracastAppMgr = MiracastAppMgr::getInstance();
    if ( nullptr != miracastAppMgr )
    {
        returnStatus = miracastAppMgr->startAppAndWaitForFinish();
    }
    else
    {
        MIRACASTLOG_ERROR("MiracastAppMgr::getInstance() failed\n");
    }
    return returnStatus;
}