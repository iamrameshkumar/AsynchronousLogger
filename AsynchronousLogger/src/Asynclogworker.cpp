/************************************************************************************************************************************************************************************************************
*FILE:  AsyncLogWorker.cpp
*
*DESCRIPTION - Framework for Logging and Design By Contract
*
*AUTHOR		: RAMESH KUMAR K
*
*Date: NOV 2016
**************************************************************************************************************************************************************************************************************/

#include "stdafx.h"

#include "Asynclogworker.h"

#include <fstream>
#include <sstream>
#include <cassert>
#include <algorithm>
#include <string>
#include <chrono>
#include <functional>
#include <future>

#include "active.h"
#include "Asynclog.h"
#include "CrashhandlerAsyncLoggerwin.h"
#include "Asynctime.h"
#include "Asyncfuture.h"
#include <locale>
#include <codecvt>
#include "SmartMutex.h"

using namespace std;

#include "ICriticalSection.h"

#define MAX_LOG_FILE_ROTATE_RETRIES 5

using namespace std;
using namespace AsyncLogger;
using namespace AsyncLogger::internal;

namespace {
static const tstring date_formatted =  _T("%Y/%m/%d");
static const tstring time_formatted = _T("%H:%M:%S");
static const tstring file_name_time_formatted =  _T("%Y%m%d-%H%M%S");
static const DWORD	 currentProcessPid		= ::GetCurrentProcessId ();

// check for filename validity -  filename should not be part of PATH
bool isValidFilename(const tstring prefix_filename) {

   tstring illegal_characters(_T("/,|<>:#$%{}()[]\'\"^!?+* "));
   size_t pos = prefix_filename.find_first_of(illegal_characters, 0);
   if (pos != tstring::npos) {
      std::wcerr << _T("Illegal character [") << prefix_filename.at(pos) << _T("] in logname prefix: ") << _T("[") << prefix_filename << _T("]") << std::endl;
      return false;
   } else if (prefix_filename.empty()) {
      std::wcerr << _T("Empty filename prefix is not allowed") << std::endl;
      return false;
   }

   return true;
}


// Clean up the path if put in by mistake in the prefix
tstring prefixSanityFix(tstring prefix) {
   prefix.erase(std::remove_if(prefix.begin(), prefix.end(), ::isspace), prefix.end());
   prefix.erase(std::remove( prefix.begin(), prefix.end(), _T('/')), prefix.end()); // '/'
   prefix.erase(std::remove( prefix.begin(), prefix.end(), _T('\\')), prefix.end()); // '\\'
   prefix.erase(std::remove( prefix.begin(), prefix.end(), _T('.')), prefix.end());  // '.'
   prefix.erase(std::remove(prefix.begin(), prefix.end(), _T(':')), prefix.end()); // ':'

   if (!isValidFilename(prefix)) {
      return _T("");
   }
   return prefix;
}


tstring pathSanityFix(tstring path, tstring file_name) {
   // Unify the delimeters,. maybe sketchy solution but it seems to work
   // on at least win7 + ubuntu. All bets are off for older windows
   std::replace(path.begin(), path.end(), _T('\\'), _T('/'));

   // clean up in case of multiples
   auto contains_end = [&](tstring & in) -> bool {
      size_t size = in.size();
      if (!size) return false;
      char end = in[size - 1];
      return (end == _T('/') || end == _T(' '));
   };

   while (contains_end(path)) { path.erase(path.size() - 1); }
   if (!path.empty()) {
      path.insert(path.end(), _T('/'));
   }
   path.insert(path.size(), file_name);
   return path;
}


int createLogFileName(const tstring& verified_prefix, tstring &outFile, unsigned int max_files_to_rotate = 10,bool rotate_existing_log = true, bool time_based_names = false) {
	
	int result = 0;

	if (time_based_names)
	{
		tstringstream oss_name;
		oss_name.fill(_T('0'));
		oss_name << verified_prefix;
		oss_name << AsyncLogger::localtime_formatted(AsyncLogger::systemtime_now(), file_name_time_formatted);

		outFile = oss_name.str();
	}
	else
	{
		tstring myfile, curfile;

		myfile = curfile = verified_prefix;

		if (rotate_existing_log)
		{
			curfile = curfile + _T(".log");

			int i;

			for (i = 1; i <= max_files_to_rotate; i++)
			{
				wifstream cur_file(curfile);

				if (!cur_file.good())
				{
					cur_file.close();
					break;
				}
				else
				{
					cur_file.close();
					curfile = myfile;
					curfile = curfile + std::to_wstring(i);
					curfile = curfile + _T(".log");
				}
			}

			while ((i - 2) > 0)
			{
				tstring file1, file2;
				file1 = file2 = myfile;

				file1 = file1 + std::to_wstring(i - 2) + _T(".log");

				file2 = file2 + std::to_wstring(i - 1) + _T(".log");


				wifstream cur_file(file1);

				if (cur_file.good())
				{
					cur_file.close();
					wifstream next_file(file2);
					if (next_file.good())
					{
						next_file.close();
						_tremove ( file2.c_str() ) ;
					}
					else
					{
						next_file.close();
					}

					_trename ( file1.c_str() , file2.c_str() ) ;
				}
				else
				{
					cur_file.close();
				}

				i--;
			}

			tstring file1, file2;
			file1 = file2 = myfile;

			file1 = file1 + _T(".log");

			file2 = file2 + std::to_wstring(1) + _T(".log");

			result = _trename ( file1.c_str() , file2.c_str() ) ;
		}

		outFile = tstring(myfile + _T(".log"));
	}
	
	return result;
}


bool openLogFile(const tstring& complete_file_with_path, std::wofstream& outstream) {
   std::ios_base::openmode mode = std::ios_base::out | std::ios_base::ate | std::ios::in | std::ios_base::binary; // for clarity: it's really overkill since it's an tfstream

   //mode |= std::ios_base::trunc;
   outstream.open(complete_file_with_path, mode);
	if (!outstream.is_open()) {

		outstream.close();

		outstream.open(complete_file_with_path,std::fstream::binary | std::fstream::trunc | std::fstream::out);    
		outstream.close();
		// re-open with original flags
		outstream.open(complete_file_with_path, mode);

		if (!outstream.is_open())
		{
			tstringstream ss_error;
			ss_error << "FILE ERROR:  could not open log file:[" << complete_file_with_path << "]";
			ss_error << "\n\t\t std::ios_base state = " << outstream.rdstate();
			std::cerr << ss_error.str().c_str() << std::endl << std::flush;
			return false;
		}
   }  

   return true;
}


std::unique_ptr<std::wofstream> createLogFile(const tstring& file_with_full_path) {
   std::unique_ptr<std::wofstream> out(new std::wofstream);
   std::wofstream& stream(*(out.get()));
   bool success_with_open_file = openLogFile(file_with_full_path, stream);
   if (false == success_with_open_file) {
      out.release(); // nullptr contained ptr<file> signals error in creating the log file
   }
   return out;
}
}  // end anonymous namespace


/** The Real McCoy Background worker, while AsyncLogWorker gives the
* asynchronous API to put job in the background the AsyncLogWorkerImpl
* does the actual background thread work */
struct AsyncLogWorkerImpl {
   AsyncLogWorkerImpl(const tstring& log_prefix, const tstring& log_directory, bool rotate_logs = true, unsigned int max_files_to_rotate = 10, bool time_based_file_names = false);
   ~AsyncLogWorkerImpl();

   void backgroundFileWrite(AsyncLogger::internal::LogEntry message);
   void backgroundExitFatal(AsyncLogger::internal::FatalMessage fatal_message);
   tstring  backgroundChangeLogFile(const tstring& directory, const tstring& file_name, bool rotate = false);
   tstring  backgroundFileName();

   int openFile(tstring file_path, tstring file_name, std::unique_ptr<std::wofstream> &out, bool rotate = false);

   tstring log_file_path_;
   tstring log_file_name_; // needed in case of future log file changes of directory
   std::unique_ptr<AsyncLogger::Active> bg_;
   std::unique_ptr<std::wofstream> outptr_;
   steady_time_point steady_start_time_;

   unsigned long long file_size_kb;

   int change_log_file_retry;

   bool _mRotate_log_files;
   bool	_mTime_based_file_names;

   unsigned int _mMax_files_to_rotate;

 private:
   AsyncLogWorkerImpl& operator=(const AsyncLogWorkerImpl&); // c++11 feature not yet in vs2010 = delete;
   AsyncLogWorkerImpl(const AsyncLogWorkerImpl& other); // c++11 feature not yet in vs2010 = delete;
   inline std::wofstream& filestream() {return *(outptr_.get());}

   ICriticalSection change_log_path;
};

tstringstream getLoggerInittext()
{
	tstringstream ss_entry;
	//  Day Month Date Time Year: is written as "%a %b %d %H:%M:%S %Y" and formatted output as : Wed Sep 19 08:28:16 2012
	ss_entry << "\n***************************************************************************************************************************************************\n\n";
	ss_entry << _T("\t\t\t\tLogging Started from: ") << AsyncLogger::localtime_formatted(AsyncLogger::systemtime_now(), _T("%a %b %d %H:%M:%S %Y")) << _T("\n");
	ss_entry << _T("\t\t\t\tLOG format: [YYYY/MM/DD hh:mm:ss uuu* [Process ID] [ loglevel ] [FILE:LINE] ] message\t\t (uuu*: microsecond counter since app start)\n");

	ss_entry << _T("\t\t\t\tLOG levels(Lower number means high priority):\t\t FATAL = 0\t CRITICAL = 1\t WARNING = 2 \t INFO = 3 \t DEBUG = 4\t ALL = 5\n");

	ss_entry << _T("\t\t\t\tCurrent LOG level: ") << log_level_strings[log_level] << _T("\n\n");

	ss_entry << _T("\t\t\t\tProduct Name: ") << _product_name << _T("\n\t\t\t\tVersion: ") << _product_version << _T("\n");

	ss_entry << _T("\t\t\t\tBuilDate: ") << __DATE__ << _T(" ") << __TIME__ << _T("\n");

	TCHAR   ImageName[MAX_PATH] = _T("");
	GetModuleFileName(NULL, ImageName, MAX_PATH);

	ss_entry << _T("\t\t\t\tImage path: ") << ImageName << _T("\n\n");

	ss_entry << _T("***************************************************************************************************************************************************\n\n");

	return ss_entry;
}

//
// Private API implementation : AsyncLogWorkerImpl
AsyncLogWorkerImpl::AsyncLogWorkerImpl(const tstring& log_prefix, const tstring& log_directory, bool rotate_logs, unsigned int max_files_to_rotate, bool time_based_file_names)
   : log_file_path_(log_directory)
   , log_file_name_(log_prefix)
   , _mRotate_log_files(rotate_logs)
   , _mMax_files_to_rotate(max_files_to_rotate)
   , _mTime_based_file_names(time_based_file_names)
   , bg_(AsyncLogger::Active::createActive())
   , outptr_(new std::wofstream)
   , steady_start_time_(std::chrono::steady_clock::now()) 

{ // TODO: ha en timer function steadyTimer som har koll på start
   
	is_logging_started = false;

	log_file_name_ = prefixSanityFix(log_prefix);

	change_log_file_retry = 0;

   TRY
   {
	   if (!isValidFilename(log_file_name_))
	   {
		   // illegal prefix, refuse to start
		   std::wcerr << _T("Asynclog: forced abort due to illegal log prefix [") << log_prefix << _T("]") << std::endl << std::flush;
		   abort();
	   }

		openFile(log_file_path_, log_file_name_, outptr_);

		if (outptr_)
		{
			const std::locale utf8_locale = std::locale(std::locale(), new std::codecvt_utf8<wchar_t>());
			outptr_->imbue(utf8_locale);
		}

		if (!outptr_) 
		{
			std::wcerr << _T("Cannot write logfile to location, attempting current directory") << std::endl;
		}
		else
		{
			is_logging_started = true;

			tstringstream ss_entry = getLoggerInittext();

			backgroundFileWrite(LogEntry(ss_entry.str(), AsyncLogger::internal::systemtime_now()));

			outptr_->fill(_T('0'));
		}
   }
   CATCH_ALL(e)
   {
   
   }
   END_CATCH_ALL

}

int AsyncLogWorkerImpl::openFile(tstring file_path, tstring file_name, std::unique_ptr<std::wofstream> &out, bool rotate)
{
	int result = -1;

	TRY
	{
		tstring log_file_with_path = pathSanityFix(file_path, file_name);

		tstring log_file; 
		result = createLogFileName(log_file_with_path, log_file, _mMax_files_to_rotate, rotate /*append logs to the existing log file*/, _mTime_based_file_names /*are we using unique time based file names?*/);

		out = createLogFile(log_file);
		if (!out) {
			std::wcerr << _T("Cannot write logfile to location, attempting current directory") << std::endl;
		}
	
	}
	CATCH_ALL(e)
	{
   
	}
	END_CATCH_ALL

	return result;
}


AsyncLogWorkerImpl::~AsyncLogWorkerImpl() {
   tstringstream ss_exit;
   bg_.reset(); // flush the log queue
   ss_exit << _T("\n\t\tLogger file shutdown at: ") << AsyncLogger::localtime_formatted(AsyncLogger::systemtime_now(), time_formatted);
   filestream() << ss_exit.str() << std::flush;
}


void AsyncLogWorkerImpl::backgroundFileWrite(LogEntry message) {

   TRY
   {
	   if (!(is_logging_started && outptr_))
	   {
		   return;
	   }

	   if (filestream())
	   {
		   std::wofstream& out(filestream());

		   if (out)
		   {
			   auto log_time = message.timestamp_;
			   auto steady_time = std::chrono::steady_clock::now();

			   out.seekp(0, ios_base::end);

			   out << _T("\n");
			   out << AsyncLogger::localtime_formatted(log_time, date_formatted);
			   out << _T(" ") << AsyncLogger::localtime_formatted(log_time, time_formatted);
			   out << _T(" ") << chrono::duration_cast<std::chrono::microseconds>(steady_time - steady_start_time_).count();

			   out << _T(" ") << currentProcessPid;

			   out << _T("  ") << message.msg_ << std::flush;

			   file_size_kb =  (out.tellp()) / 1024;

			   if (file_size_kb > 1024 && change_log_file_retry < MAX_LOG_FILE_ROTATE_RETRIES)
			   {
				   file_size_kb = 0;

				   if (change_log_path.TryCaptureMutexLock())
				   {
						change_log_file_retry ++;
						change_log_path.ReleaseLock();
						backgroundChangeLogFile(log_file_path_, log_file_name_, true);
				   }
			   }
		   }
	   }
   }
   CATCH_ALL(e)
   {
   
   }
   END_CATCH_ALL
}


void AsyncLogWorkerImpl::backgroundExitFatal(FatalMessage fatal_message) 
{
	backgroundFileWrite(fatal_message.message_);
	LogEntry flushEntry(_T("Log flushed successfully to disk \nExiting...\n\n"), AsyncLogger::internal::systemtime_now());
	backgroundFileWrite(flushEntry);

	std::wcerr << _T("Asynclog exiting after receiving fatal event") << std::endl;
	std::wcerr << _T("Log file at: [") << log_file_path_ << _T("]\n") << std::endl << std::flush;
	filestream().close();

	AsyncLogger::shutDownLogging(); // only an initialized logger can recieve a fatal message. So shutting down logging now is fine.
	exitWithDefaultSignalHandler(fatal_message.signal_id_);
	perror("Asynclog exited after receiving FATAL trigger. Flush message status: "); // should never reach this point
}


tstring AsyncLogWorkerImpl::backgroundChangeLogFile(const tstring& directory, const tstring& file, bool _rotate) {

	TRY
	{
		{

			LockGuard<ICriticalSection> lock_holder(change_log_path);

			// setting the new log as active
			tstring old_log = log_file_path_ + _T("\\") + log_file_name_ + _T(".log");

			if (outptr_)
			{
				if (outptr_->is_open())
				{
					is_logging_started = false;
					outptr_->close();
				}
			}

			

			std::unique_ptr<std::wofstream> log_stream = nullptr;

			int result = openFile(directory, file, log_stream, _rotate);

			if (nullptr == log_stream) 
			{

				openFile(log_file_path_, log_file_name_, log_stream, _rotate);

				if (!log_stream) 
				{
					std::wcerr << _T("Cannot write logfile to location, attempting current directory") << std::endl;
				}
				else
				{
					outptr_ = std::move(log_stream);
					is_logging_started = true;
				}
			}
			else
			{
				log_file_name_ = file;
				log_file_path_ = directory;
				outptr_ = std::move(log_stream);

				is_logging_started = true;

				tstringstream ss_entry;

				if (ERROR_SUCCESS == result)
				{
					ss_entry = getLoggerInittext();

					ss_entry << _T("\n\tNew log file. The previous log file was at: ");
					ss_entry << old_log;
				}
				else
				{
					ss_entry << _T("\n\tChanging logfile failed");
				}

				backgroundFileWrite(LogEntry(ss_entry.str(), AsyncLogger::internal::systemtime_now()));
			}
		}
	}
	CATCH_ALL(e)
	{

	}
	END_CATCH_ALL


	return tstring(log_file_path_ + log_file_name_ + _T(".log"));
}

tstring  AsyncLogWorkerImpl::backgroundFileName() {
   return log_file_path_;
}


void AsyncLogWorker::setlogLevel(unsigned int level)
{
	log_level = level;
}

//
// *****   BELOW AsyncLogWorker    *****
// Public API implementation
//
AsyncLogWorker::AsyncLogWorker(const tstring& log_prefix, const tstring& log_directory, UINT level, const tstring& product_name , const tstring& version )
   :  pimpl_(new AsyncLogWorkerImpl(log_prefix, log_directory))
{

	log_level = level;

	_product_name = product_name;

	_product_version = version;

	assert((pimpl_ != nullptr) && "should never happen");
}

AsyncLogWorker::~AsyncLogWorker() {
	if (pimpl_)
		pimpl_.reset();

   AsyncLogger::shutDownLoggingForActiveOnly(this);
   std::wcerr << _T("\nExiting...") << _T("\n") << std::flush;
}

void AsyncLogWorker::save(const AsyncLogger::internal::LogEntry& msg) {
	if ( pimpl_ && pimpl_->bg_)
		pimpl_->bg_->send(std::bind(&AsyncLogWorkerImpl::backgroundFileWrite, pimpl_.get(), msg));
}

void AsyncLogWorker::fatal(AsyncLogger::internal::FatalMessage fatal_message) {
	if ( pimpl_ && pimpl_->bg_)
		pimpl_->bg_->send(std::bind(&AsyncLogWorkerImpl::backgroundExitFatal, pimpl_.get(), fatal_message));
}


std::future<tstring> AsyncLogWorker::changeLogFile(const tstring& log_directory, const tstring& file, bool rotate) {
   AsyncLogger::Active* bgWorker = pimpl_->bg_.get();
   //auto future_result = AsyncLogger::spawn_task(std::bind(&AsyncLogWorkerImpl::backgroundChangeLogFile, pimpl_.get(), log_directory), bgWorker);

   if (bgWorker && pimpl_)
   {
	   tstringstream ss_change;
		ss_change << _T("\n\tChanging log file to new location: ") << log_directory << file << _T(".log") << _T("\n");

		pimpl_->backgroundFileWrite(LogEntry(ss_change.str().c_str(), AsyncLogger::internal::systemtime_now()));

		ss_change.str(_T(""));

	   auto bg_call =     [this, log_directory, file, rotate]() {return pimpl_->backgroundChangeLogFile(log_directory, file, rotate);};
	   auto future_result = AsyncLogger::spawn_task(bg_call, bgWorker);
	   return std::move(future_result);
   }
}

std::future<tstring> AsyncLogWorker::logFileName() {
	if (pimpl_)
	{
		AsyncLogger::Active* bgWorker = pimpl_->bg_.get();
	   auto bg_call = [&]() {return pimpl_->backgroundFileName();};
	   auto future_result = AsyncLogger::spawn_task(bg_call , bgWorker);
	   return std::move(future_result);
	}
}

std::future<int> AsyncLogWorker::genericAsyncCall(std::function<int(void)> f) {

	if (pimpl_)
	{
		auto bgWorker = pimpl_->bg_.get();
		auto future_result = AsyncLogger::spawn_task(f, bgWorker);
		return std::move(future_result);
	}
}