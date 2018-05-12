/** ==========================================================================
 *
 * Filename:Asynclog.h  Framework for Logging and Design By Contract
 *
 * influenced at least in "spirit" from the following sources
 * 1. AsyncLogger.cc ;)
 * 2. Dr.Dobbs, Petru Marginean:  http://drdobbs.com/article/printableArticle.jhtml?articleId=201804215&dept_url=/cpp/
 * 3. Dr.Dobbs, Michael Schulze: http://drdobbs.com/article/printableArticle.jhtml?articleId=225700666&dept_url=/cpp/
 * 4. Google 'glog': http://google-glog.googlecode.com/svn/trunk/doc/glog.html
 * 5. Various Q&A at StackOverflow
 * ********************************************* */


#ifndef AsyncLOG_H
#define AsyncLOG_H

#include "stdafx.h"
#include <string>
#include <sstream>
#include <iostream>
#include <cstdarg>
#include <chrono>
#include <functional>
#include <ctime>

class AsyncLogWorker;


// Levels for logging, made so that it would be easy to change, remove, add levels
enum SEVERITY_TYPE
{
	FATAL = 1,
	SILENT,
	CRITICAL,
	WARNING,
	INFO , 
	NORMAL,
	DBUG, 
	LOG_ALL
	
};

static const tstring k_fatal_log_expression = _T(""); // using LogContractMessage but no boolean expression
static    bool	is_logging_started = false;

static tstring log_level_strings [] = {_T("UNKNOWN"), _T("FATAL"), _T("SILENT") , _T("CRITICAL"), _T("WARNING"), _T("INFO"), _T("NORMAL"), _T("DEBUG"), _T("ALL")};

extern tstring _product_name;
extern tstring _product_version;

extern unsigned int log_level;

// GCC Predefined macros: http://gcc.gnu.org/onlinedocs/cpp/Standard-Predefined-Macros.html
//     and http://gcc.gnu.org/onlinedocs/gcc/Function-Names.html
//
// The ## macro is helpful as it gives compile time error in case there's a typo
// Example: MYLEVEL doesn't exist so LOG(MYLEVEL) << "bla bla bla"; would
//         generate a compile error when it is rolled out by the
//         macro as Async_LOG_MYLEVEL, since "#define Async_LOG_MYLEVEL" doesn't exist


// BELOW -- LOG stream syntax

#define LOGDEBUG LOG(DBUG)
#define LOGCRITICAL LOG(CRITICAL)
#define LOGINFO LOG(INFO)
#define LOGWARNING LOG(WARNING)
#define LOGFATAL LOG(FATAL)

#define Async_LOG_DBUG  AsyncLogger::internal::LogMessage($File,__LINE__,$Function,DBUG)
#define Async_LOG_INFO  AsyncLogger::internal::LogMessage($File,__LINE__,$Function,INFO)
#define Async_LOG_WARNING  AsyncLogger::internal::LogMessage($File,__LINE__,$Function,WARNING)
#define Async_LOG_CRITICAL  AsyncLogger::internal::LogMessage($File,__LINE__,$Function,CRITICAL)
#define Async_LOG_FATAL  AsyncLogger::internal::LogContractMessage($File,__LINE__,$Function,k_fatal_log_expression)


#ifdef STATIC_LOG_LEVEL
#define LOG(level)\
	if(level <= log_level)          \
		Async_LOG_##level.messageStream()
#else
#define LOG(level)\
	Async_LOG_##level
#endif
// LOG(level) is the API for the stream log

// conditional stream log

#ifdef STATIC_LOG_LEVEL
#define LOG_IF(level, boolean_expression)  \
	if(level <= log_level)          \
	  if(true == boolean_expression)          \
		 Async_LOG_##level.messageStream()
#else
#define LOG_IF(level, boolean_expression)  \
	  if(true == boolean_expression)          \
		 Async_LOG_##level
#endif

// Design By Contract, stream API. Throws std::runtime_eror if contract breaks
#define CHECK(boolean_expression)                                                    \
if (false == (boolean_expression))                                                     \
  AsyncLogger::internal::LogContractMessage($File,__LINE__,$Function, _T(#boolean_expression)).messageStream()


// BELOW -- LOG "printf" syntax
/**
  * For details please see this
  * REFERENCE: http://www.cppreference.com/wiki/io/c/printf_format
  * \verbatim
  *
  There are different %-codes for different variable types, as well as options to
    limit the length of the variables and whatnot.
    Code Format
    %[flags][width][.precision][length]specifier
 SPECIFIERS
 ----------
 %c character
 %d signed integers
 %i signed integers
 %e scientific notation, with a lowercase “e”
 %E scientific notation, with a uppercase “E”
 %f floating point
 %g use %e or %f, whichever is shorter
 %G use %E or %f, whichever is shorter
 %o octal
 %s a string of characters
 %u unsigned integer
 %x unsigned hexadecimal, with lowercase letters
 %X unsigned hexadecimal, with uppercase letters
 %p a pointer
 %n the argument shall be a pointer to an integer into which is placed the number of characters written so far

For flags, width, precision etc please see the above references.
EXAMPLES:
{
   LOGF(INFO, "Characters: %c %c \n", 'a', 65);
   LOGF(INFO, "Decimals: %d %ld\n", 1977, 650000L);      // printing long
   LOGF(INFO, "Preceding with blanks: %10d \n", 1977);
   LOGF(INFO, "Preceding with zeros: %010d \n", 1977);
   LOGF(INFO, "Some different radixes: %d %x %o %#x %#o \n", 100, 100, 100, 100, 100);
   LOGF(INFO, "floats: %4.2f %+.0e %E \n", 3.1416, 3.1416, 3.1416);
   LOGF(INFO, "Width trick: %*d \n", 5, 10);
   LOGF(INFO, "%s \n", "A string");
   return 0;
}
And here is possible output
:      Characters: a A
:      Decimals: 1977 650000
:      Preceding with blanks:       1977
:      Preceding with zeros: 0000001977
:      Some different radixes: 100 64 144 0x64 0144
:      floats: 3.14 +3e+000 3.141600E+000
:      Width trick:    10
:      A string  \endverbatim */
#define Async_LOGF_INFO     AsyncLogger::internal::LogMessage($File,__LINE__,$Function,INFO)
#define Async_LOGF_DBUG    AsyncLogger::internal::LogMessage($File,__LINE__,$Function,DBUG)
#define Async_LOGF_WARNING  AsyncLogger::internal::LogMessage($File,__LINE__,$Function,WARNING)
#define Async_LOGF_CRITICAL  AsyncLogger::internal::LogMessage($File,__LINE__,$Function,CRITICAL)
#define Async_LOGF_FATAL    AsyncLogger::internal::LogContractMessage($File,__LINE__,$Function,k_fatal_log_expression)

#define LogNormal LogInfo

#define LogDebug(printf_like_message, ...) LOGF(DBUG, printf_like_message, ##__VA_ARGS__)

#define LogCritical(printf_like_message, ...) LOGF(CRITICAL, printf_like_message, ##__VA_ARGS__)

#define LogInfo(printf_like_message, ...) LOGF(INFO, printf_like_message, ##__VA_ARGS__)

#define LogWarning(printf_like_message, ...) LOGF(WARNING, printf_like_message, ##__VA_ARGS__)

#define LogFatal(printf_like_message, ...) LOGF(Fatal, printf_like_message, ##__VA_ARGS__)

#ifdef STATIC_LOG_LEVEL
// LOGF(level,msg,...) is the API for the "printf" like log
#define LOGF(level, printf_like_message, ...)                 \
	if(level <= log_level)          \
		Async_LOGF_##level.messageSave(printf_like_message, ##__VA_ARGS__)
#else
// LOGF(level,msg,...) is the API for the "printf" like log
#define LOGF(level, printf_like_message, ...)                 \
		Async_LOGF_##level.messageSave(printf_like_message, ##__VA_ARGS__)
#endif


#ifdef STATIC_LOG_LEVEL
// conditional log printf syntax
#define LOGF_IF(level,boolean_expression, printf_like_message, ...) \
	if(level <= log_level)          \
	  if(true == boolean_expression)                                     \
		 Async_LOG_##level.messageSave(printf_like_message, ##__VA_ARGS__)
#else
// conditional log printf syntax
#define LOGF_IF(level,boolean_expression, printf_like_message, ...) \
	  if(true == boolean_expression)                                     \
		 Async_LOG_##level.messageSave(printf_like_message, ##__VA_ARGS__)
#endif


// Design By Contract, printf-like API syntax with variadic input parameters. Throws std::runtime_eror if contract breaks */
#define CHECK_F(boolean_expression, printf_like_message, ...)                                     \
   if (false == (boolean_expression))                                                             \
  AsyncLogger::internal::LogContractMessage($File,__LINE__,$Function,#boolean_expression).messageSave(printf_like_message, ##__VA_ARGS__)


/** namespace for LOG() and CHECK() frameworks
  */
namespace AsyncLogger {
/** Should be called at very first startup of the software with \ref AsyncLogWorker pointer. Ownership of the \ref AsyncLogWorker is
* the responsibilkity of the caller */
void initializeLogging(AsyncLogWorker* logger);

void HandleFatalError(int signal);

/** Shutdown the logging by making the pointer to the background logger to nullptr
 * The \ref pointer to the AsyncLogWorker is owned by the instantniater \ref initializeLogging
 * and is not deleted.
 */
void shutDownLogging();

/** Same as the Shutdown above but called by the destructor of the LogWorker, thus ensuring that no further
 *  LOG(...) calls can happen to  a non-existing LogWorker.
 *  @param active MUST BE the LogWorker initialized for logging. If it is not then this call is just ignored
 *         and the logging continues to be active.
 * @return true if the correct worker was given,. and shutDownLogging was called
*/
bool shutDownLoggingForActiveOnly(AsyncLogWorker* active);

// defined here but should't not have to be used outside the Asynclog
namespace internal {

/// returns timepoint as std::time_t
std::time_t systemtime_now();


struct LogEntry {
   LogEntry(tstring msg, std::time_t timestamp) : msg_(msg), timestamp_(timestamp) {}
   LogEntry(const LogEntry& other): msg_(other.msg_), timestamp_(other.timestamp_) {}
   LogEntry& operator=(const LogEntry& other) {
      msg_ = other.msg_;
      timestamp_ = other.timestamp_;
      return *this;
   }


   tstring msg_;
   std::time_t timestamp_;
};

bool isLoggingInitialized();

/** Trigger for flushing the message queue and exiting the application
    A thread that causes a FatalMessage will sleep forever until the
    application has exited (after message flush) */
struct FatalMessage {
   enum FatalType {kReasonFatal, kReasonOS_FATAL_SIGNAL};
   FatalMessage(LogEntry message, FatalType type, unsigned long signal_id);
   ~FatalMessage() {};
   FatalMessage& operator=(const FatalMessage& fatal_message);


   LogEntry message_;
   FatalType type_;
   unsigned long signal_id_;
};
// Will trigger a FatalMessage sending
struct FatalTrigger {
   FatalTrigger(const FatalMessage& message);
   ~FatalTrigger();
   FatalMessage message_;
};


// Log message for 'printf-like' or stream logging, it's a temporary message constructions
class LogMessage {
 public:
   LogMessage(const tstring& file, const int line, const tstring& function, const unsigned int level);
   virtual ~LogMessage(); // at destruction will flush the message

   tstringstream& messageStream() {return stream_;}

   void WriteLogToStream();

   // The __attribute__ generates compiler warnings if illegal "printf" format
   // IMPORTANT: You muse enable the compiler flag '-Wall' for this to work!
   // ref: http://www.unixwiz.net/techtips/gnu-c-attributes.html
   //
   //If the compiler does not support attributes, disable them
#ifndef __GNUC__
#define  __attribute__(x)
#endif
   // Coder note: Since it's C++ and not C EVERY CLASS FUNCTION always get a first
   // compiler given argument 'this' this must be supplied as well, hence '2,3'
   // ref: http://www.codemaestro.com/reviews/18 
    void messageSave(const TCHAR* printf_like_message, ...)
		__attribute__((format(printf, 2, 3) ));

	void messageSave(const char* printf_like_message, ...)
   __attribute__((format(printf, 2, 3) ));

 protected:
   const tstring file_;
   const int line_;
   const tstring function_;
   const unsigned int level_;
   tstringstream stream_;
   tstring log_entry_;
   std::time_t timestamp_;

public:
    template<class T>
	LogMessage& operator<<(T const &x) {


		if (level_ <= log_level && is_logging_started)
		{
			stream_ << x;
		}
		else
		{
			
		}

		return *this;
	}

	LogMessage& GetLogger() // TODO:: handle this using null streams
	{
		if (level_ <= log_level)
		{
			return *this;
		}
		else
		{
			NULL;
		}
	}

};



// 'Design-by-Contract' temporary messsage construction
class LogContractMessage : public LogMessage {
 public:
   LogContractMessage(const tstring& file, const int line,
                      const tstring& function, const tstring& boolean_expression);
   virtual ~LogContractMessage(); // at destruction will flush the message

 protected:
   const tstring expression_;
};


/** By default the Asynclog will call AsyncLogWorker::fatal(...) which will
  * abort() the system after flushing the logs to file. This makes unit
  * test of FATAL level cumbersome. A work around is to change the
  * 'fatal call'  which can be done here
  *
  *  The bool return values in the fatal_call is whether or not the fatal_call should */
void changeFatalInitHandlerForUnitTesting(std::function<void(FatalMessage) > fatal_call);
} // end namespace internal
} // end namespace AsyncLogger

#endif // AsyncLOG_H
