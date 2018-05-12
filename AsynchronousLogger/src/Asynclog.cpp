/************************************************************************************************************************************************************************************************************
*FILE:  Asynclog.cpp
*
*DESCRIPTION -  * influenced from the following sources

 * 1. AsyncLogger.cc ;)
 * 2. Dr.Dobbs, Petru Marginean:  http://drdobbs.com/article/printableArticle.jhtml?articleId=201804215&dept_url=/cpp/
 * 3. Dr.Dobbs, Michael Schulze: http://drdobbs.com/article/printableArticle.jhtml?articleId=225700666&dept_url=/cpp/
 * 4. Google 'glog': http://google-glog.googlecode.com/svn/trunk/doc/glog.html
 * 5. Various Q&A at StackOverflow
*
*AUTHOR		: RAMESH KUMAR K
*
**************************************************************************************************************************************************************************************************************/

#include "stdafx.h"

#include "Asynclog.h"

#include <iostream>
#include <sstream>
#include <string>
#include <stdexcept> // exceptions
#include <cstdio>    // vsnprintf
#include <cassert>
#include <mutex>
#include <chrono>
#include <signal.h>
#include <thread>

#include "Asynclogworker.h"
#include "CrashhandlerAsyncLoggerwin.h"

unsigned int log_level = INFO;

tstring _product_name = _T("AsyncLogger");
tstring _product_version = _T("0.0.1");

namespace {
std::once_flag g_initialize_flag;
AsyncLogWorker* g_logger_instance = nullptr; // instantiated and OWNED somewhere else (main)
std::mutex g_logging_init_mutex;

AsyncLogger::internal::LogEntry g_first_unintialized_msg = AsyncLogger::internal::LogEntry(_T(""), 0);
std::once_flag g_set_first_uninitialized_flag;
std::once_flag g_save_first_unintialized_flag;

const int kMaxMessageSize = 4096;
const tstring kTruncatedWarningText = _T("[...truncated...]");




void saveToLogger(const AsyncLogger::internal::LogEntry& log_entry) {
   // Uninitialized messages are ignored but does not CHECK/crash the logger
   if (!AsyncLogger::internal::isLoggingInitialized()) {
      tstring err(_T("LOGGER NOT INITIALIZED: ") + log_entry.msg_);
      std::call_once(g_set_first_uninitialized_flag,
                     [&] { g_first_unintialized_msg.msg_ += err;
                           g_first_unintialized_msg.timestamp_ = AsyncLogger::internal::systemtime_now();
                         });
      // dump to std::err all the non-initialized logs
	  std::wcerr << err << std::endl;
      return;
   }
   // Save the first uninitialized message, if any
   std::call_once(g_save_first_unintialized_flag, [] {
      if (!g_first_unintialized_msg.msg_.empty()) {
         g_logger_instance->save(g_first_unintialized_msg);
      }
   });

   g_logger_instance->save(log_entry);
}
} // anonymous


namespace AsyncLogger {

// signalhandler and internal clock is only needed to install once
// for unit testing purposes the initializeLogging might be called
// several times... for all other practical use, it shouldn't!


void HandleFatalError(int signal)
{
	crashHandler(signal);
}

void initializeLogging(AsyncLogWorker* bgworker) {
   std::call_once(g_initialize_flag, []() {
      installSignalHandler();
   });

   std::lock_guard<std::mutex> lock(g_logging_init_mutex);
   CHECK(!internal::isLoggingInitialized());
   CHECK(bgworker != nullptr);
   g_logger_instance = bgworker;
}



void  shutDownLogging() {
   std::lock_guard<std::mutex> lock(g_logging_init_mutex);
   g_logger_instance = nullptr;
}



bool shutDownLoggingForActiveOnly(AsyncLogWorker* active) {

	TRY
	{
		 if (internal::isLoggingInitialized() && nullptr != active  &&
			(dynamic_cast<void*>(active) != dynamic_cast<void*>(g_logger_instance))) 
		{
			 LOG(WARNING) << _T("\n\t\tShutting down logging, but the ID of the Logger is not the one that is active.")
					   << _T("\n\t\tHaving multiple instances of the AsyncLogger::LogWorker is likely a BUG")
					   << _T("\n\t\tEither way, this call to shutDownLogging was ignored");
			return false;
		}

	   shutDownLogging();
	   return true;
	}
	CATCH_ALL(e)
	{
		//
	}
	END_CATCH_ALL

	return false;
}



namespace internal {

/// returns timepoint as std::time_t
std::time_t systemtime_now() {
   const auto now = std::chrono::system_clock::now();
   return std::chrono::system_clock::to_time_t(now);
}


bool isLoggingInitialized() {
   return g_logger_instance != nullptr;
}


/** Fatal call saved to logger. This will trigger SIGABRT or other fatal signal
  * to exit the program. After saving the fatal message the calling thread
  * will sleep forever (i.e. until the background thread catches up, saves the fatal
  * message and kills the software with the fatal signal.
*/
void fatalCallToLogger(FatalMessage message) {
   if (!isLoggingInitialized()) {
      tstringstream error;
      error << _T("FATAL CALL but logger is NOT initialized\n")
            << _T("SIGNAL: " << AsyncLogger::internal::signalName(message.signal_id_))
            << _T("\nMessage: \n" << message.message_.msg_ << std::flush);
      std::wcerr << error.str();

      internal::exitWithDefaultSignalHandler(message.signal_id_);
   }
   g_logger_instance->fatal(message);
   while (internal::isLoggingInitialized()) {
      std::this_thread::sleep_for(std::chrono::seconds(1));
   }
}

// By default this function pointer goes to \ref callFatalInitial;
std::function<void(FatalMessage) > g_fatal_to_Asynclogworker_function_ptr = fatalCallToLogger;

// In case of unit-testing - a replacement 'fatal function' can be called
void changeFatalInitHandlerForUnitTesting(std::function<void(FatalMessage) > fatal_call) {
   g_fatal_to_Asynclogworker_function_ptr = fatal_call;
}





LogContractMessage::LogContractMessage(const tstring& file, const int line,
                                       const tstring& function, const tstring& boolean_expression)
   : LogMessage(file, line, function, FATAL)
   , expression_(boolean_expression)
{}

LogContractMessage::~LogContractMessage() {
   tstringstream oss;
   if (0 == expression_.compare(k_fatal_log_expression)) {
      oss << _T("\n[  *******\tEXIT trigger caused by File :: ")<< file_ << _T(" Line: ") << line_ << _T(" Function:") << function_ << _T(" LOG(FATAL): \n\t");
   } else {
      oss << _T("\n[  *******\tEXIT trigger caused by broken Contract: CHECK(" << expression_ << ")\n\t");
   }
   log_entry_ = oss.str();
}

LogMessage::LogMessage(const tstring& file, const int line, const tstring& function, const unsigned int level)
	: file_(file)
   , line_(line)
   , function_(function)
   , level_(level)
   , timestamp_(systemtime_now())
{}


void LogMessage::WriteLogToStream()
{
	const bool fatal = (FATAL == level_);
	
	try
	{
		if (level_ <= log_level)
		{
			const tstring str(stream_.str());

			if (!str.empty())
			{
				tstringstream oss;

				if (fatal)
				{
					oss << _T("Fatal error at: ") << function_;
				}

				oss << _T(" [") << log_level_strings[level_] << _T("] ") << _T("[") << splitFileName(file_);
				oss << _T(" L: ") << line_ << _T("]\t");

				log_entry_ += oss.str();

				log_entry_ += str;

				saveToLogger(LogEntry(log_entry_, timestamp_)); // message saved
			}
		}

		if (fatal)
		{     // os_fatal is handled by crashhandlers
			  // local scope - to trigger FatalMessage sending
			FatalMessage::FatalType fatal_type(FatalMessage::kReasonFatal);
			FatalMessage fatal_message(LogEntry(log_entry_, timestamp_), fatal_type, SIGABRT);
			FatalTrigger trigger(fatal_message);
			std::wcerr << log_entry_ << _T("\t*******  ]") << std::endl << std::flush;
			// will send to worker
		}
	}
	catch (...)
	{

	}
}

LogMessage::~LogMessage() {

	using namespace internal;

	WriteLogToStream();
}


// represents the actual fatal message
FatalMessage::FatalMessage(LogEntry message, FatalType type, unsigned long signal_id)
   : message_(message)
   , type_(type)
   , signal_id_(signal_id) {}


FatalMessage& FatalMessage::operator=(const FatalMessage& other) {
   message_ = other.message_;
   type_ = other.type_;
   signal_id_ = other.signal_id_;
   return *this;
}


// used to RAII trigger fatal message sending to AsyncLogWorker
FatalTrigger::FatalTrigger(const FatalMessage& message)
   :  message_(message) {}

// at destruction, flushes fatal message to AsyncLogWorker
FatalTrigger::~FatalTrigger() {
   // either we will stay here eternally, or it's in unit-test mode
   g_fatal_to_Asynclogworker_function_ptr(message_);

}

void LogMessage::messageSave( const TCHAR* printf_like_message, ...) 
{

	if (isLoggingInitialized)
	{
		

#ifndef STATIC_LOG_LEVEL
		if (level_ <= log_level)
		{
#endif
			__try
			{
				if (stream_ && printf_like_message)
				{
					TCHAR finished_message[kMaxMessageSize];
					va_list arglist;
					va_start(arglist, printf_like_message);

					if (arglist)
					{
						const int nbrcharacters = _vsnwprintf (finished_message, sizeof(finished_message), printf_like_message, arglist);
						va_end(arglist);

						if (nbrcharacters <= 0) 
						{
							stream_ << _T("\n\tERROR LOG MSG NOTIFICATION: Failure to parse the message successfully");
							stream_ << _T('"') << printf_like_message << _T('"') << std::endl;
						}	
						else if (nbrcharacters > kMaxMessageSize) 
						{
							stream_  << finished_message << kTruncatedWarningText;
						}	
						else 
						{
							stream_ << finished_message;
						}
					}
				}
			}
			__except(EXCEPTION_EXECUTE_HANDLER)
			{

			}

#ifndef STATIC_LOG_LEVEL
		}
#endif

	}
}

void LogMessage::messageSave( const char* printf_like_message, ...) {

	if (isLoggingInitialized)
	{

#ifndef STATIC_LOG_LEVEL
		if (level_ <= log_level)
		{
#endif
			__try
			{

				char finished_message[kMaxMessageSize];
				va_list arglist;

				va_start(arglist, printf_like_message);

				if (arglist)
				{
					const int nbrcharacters = _vsnprintf(finished_message, sizeof(finished_message), printf_like_message, arglist);

					va_end(arglist);

					if (nbrcharacters <= 0) {
						stream_ << _T("\n\tERROR LOG MSG NOTIFICATION: Failure to parse successfully the message");
						stream_ << _T('"') << printf_like_message << _T('"') << std::endl;
					}
					else if (nbrcharacters > kMaxMessageSize) {
						stream_ << finished_message << kTruncatedWarningText;
					}
					else {
						stream_ << finished_message;
					}
				}
			}
			__except (EXCEPTION_EXECUTE_HANDLER)
			{

			}

#ifndef STATIC_LOG_LEVEL
		}
#endif
	}
}

} // end of namespace AsyncLogger::internal
} // end of namespace AsyncLogger
