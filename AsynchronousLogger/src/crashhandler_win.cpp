/** ==========================================================================
* Filename		: CrashHandler_win.cpp
* DESCRIPTION	: Flushes the logs as soon as a crash is signalled
*AUTHOR			: RAMESH KUMAR K
* ********************************************* */

#include "stdafx.h"
#include "CrashhandlerAsyncLoggerwin.h"
#include "Asynclog.h"

#include <csignal>
#include <cstring>
#include <cstdlib>
#if !(defined(WIN32) || defined(_WIN32) || defined(__WIN32__))
#error "crashhandler_win.cpp used but not on a windows system"
#endif

#include <process.h> // getpid
#define getpid _getpid

namespace AsyncLogger
{
namespace internal
{
tstring signalName(unsigned long signal_number)
{
  switch(signal_number)
  {
  case SIGABRT: return _T("SIGABRT");break;
  case SIGFPE:  return _T("SIGFPE"); break;
  case SIGSEGV: return _T("SIGSEGV"); break;
  case SIGILL:  return _T("SIGILL"); break;
  case SIGTERM: return _T("SIGTERM"); break;
default:
    tstringstream oss;
    oss << _T("UNKNOWN SIGNAL(") << signal_number << _T(")");
    return oss.str();
  }
}

// Triggered by Asynclog::LogWorker after receiving a FATAL trigger
// which is LOG(FATAL), CHECK(false) or a fatal signal our signalhandler caught.
// --- If LOG(FATAL) or CHECK(false) the signal_number will be SIGABRT
void exitWithDefaultSignalHandler(unsigned long signal_number)
{
   // Restore our signalhandling to default
	/*if(SIG_ERR == signal (SIGABRT, SIG_DFL))	
		perror("signal - SIGABRT");
	if(SIG_ERR == signal (SIGFPE, SIG_DFL))
				perror("signal - SIGABRT");
	if(SIG_ERR == signal (SIGSEGV, SIG_DFL))
				perror("signal - SIGABRT");
	if(SIG_ERR == signal (SIGILL, SIG_DFL))
				perror("signal - SIGABRT");
	if(SIG_ERR == signal (SIGTERM, SIG_DFL))
				perror("signal - SIGABRT");
	
   raise(signal_number);*/
}
} // end AsyncLogger::internal

void crashHandler(int signal_number)
{
    using namespace AsyncLogger::internal;
    tstringstream fatal_stream;
    fatal_stream << _T("\n\n***** FATAL TRIGGER RECEIVED ******* ") << std::endl;
    fatal_stream << _T("\n***** SIGNAL ") << signalName(signal_number) << _T("(") << signal_number << _T(")") << std::endl;

    FatalMessage fatal_message( LogEntry(fatal_stream.str(), AsyncLogger::internal::systemtime_now()),FatalMessage::kReasonOS_FATAL_SIGNAL, signal_number);
    FatalTrigger trigger(fatal_message);
    std::wcerr << fatal_message.message_.msg_ << std::endl << std::flush;
} // scope exit - message sent to LogWorker, wait to die...


void installSignalHandler()
{
    /*if(SIG_ERR == signal (SIGABRT, crashHandler))	
		perror("signal - SIGABRT");
	if(SIG_ERR == signal (SIGFPE, crashHandler))
				perror("signal - SIGFPE");
	if(SIG_ERR == signal (SIGSEGV, crashHandler))
				perror("signal - SIGSEGV");
	if(SIG_ERR == signal (SIGILL, crashHandler))
				perror("signal - SIGILL");
	if(SIG_ERR == signal (SIGTERM, crashHandler))
				perror("signal - SIGTERM");
*/

}
} // end namespace AsyncLogger
