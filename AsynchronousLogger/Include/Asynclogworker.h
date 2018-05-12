#ifndef Async_LOG_WORKER_H_
#define Async_LOG_WORKER_H_
/** ==========================================================================
* Filename:Asynclogworker.h  Framework for Logging and Design By Contract
*
* ********************************************* */


#include <memory>
#include <future>
#include <string>
#include <functional>
#include "Asynclog.h"

struct AsyncLogWorkerImpl;

/**
* \param log_prefix is the 'name' of the binary, this give the log name 'LOG-'name'-...
* \param log_directory gives the directory to put the log files */
class AsyncLogWorker {
 public:
	 AsyncLogWorker(const tstring& log_prefix, const tstring& log_directory, UINT log_level = CRITICAL, const tstring& product_name = _T("AsyncLogger"), const tstring& version = _T("0.0.1"));
   virtual ~AsyncLogWorker();

   /// pushes in background thread (asynchronously) input messages to log file
   void save(const AsyncLogger::internal::LogEntry& entry);

   /// Will push a fatal message on the queue, this is the last message to be processed
   /// this way it's ensured that all existing entries were flushed before 'fatal'
   /// Will abort the application!
   void fatal(AsyncLogger::internal::FatalMessage fatal_message);

   /// Attempt to change the current log file to another name/location.
   /// returns filename with full path if successful, else empty string
   std::future<tstring> changeLogFile(const tstring& log_directory, const tstring& file, bool rotate = false);

   void setlogLevel(unsigned int level);

   /// Does an independent action in FIFO order, compared to the normal LOG statements
   /// Example: auto threadID = [] { std::cout << "thread id: " << std::this_thread::get_id() << std::endl; };
   ///          auto call = logger.genericAsyncCall(threadID); 
   ///          // this will print out the thread id of the background worker
   std::future<int> genericAsyncCall(std::function<int(void)> f);

   /// Probably only needed for unit-testing or specific log management post logging
   /// request to get log name is processed in FIFO order just like any other background job.
   std::future<tstring> logFileName();

 private:
   std::unique_ptr<AsyncLogWorkerImpl> pimpl_;

   AsyncLogWorker(const AsyncLogWorker&); // c++11 feature not yet in vs2010 = delete;
   AsyncLogWorker& operator=(const AsyncLogWorker&); // c++11 feature not yet in vs2010 = delete;
};





#endif // LOG_WORKER_H_
