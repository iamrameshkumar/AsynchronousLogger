#ifndef CRASH_HANDLER_H_
#define CRASH_HANDLER_H_

#include <string>
#include <csignal>

namespace AsyncLogger
{

// PRIVATE-INTERNAL  API
namespace internal
{
/** \return signal_name. Ref: signum.h and \ref installSignalHandler */
tstring signalName(unsigned long signal_number);

/** Re-"throw" a fatal signal, previously caught. This will exit the application
  * This is an internal only function. Do not use it elsewhere. It is triggered
  * from Asynclog, AsyncLogWorker after flushing messages to file */
void exitWithDefaultSignalHandler(unsigned long signal_number);
} // end AsyncLogger::interal


// PUBLIC API:
/** Install signal handler that catches FATAL C-runtime or OS signals
    SIGABRT  ABORT (ANSI), abnormal termination
    SIGFPE   Floating point exception (ANSI): http://en.wikipedia.org/wiki/SIGFPE
    SIGILL   ILlegal instruction (ANSI)
    SIGSEGV  Segmentation violation i.e. illegal memory reference
    SIGTERM  TERMINATION (ANSI) */
void installSignalHandler();

void crashHandler(int);

}

#endif // CRASH_HANDLER_H_
