#ifndef Async_TIME_H_
#define Async_TIME_H_
/** ==========================================================================
* Filename:Asynctime.h cross-platform, thread-safe replacement for C++11 non-thread-safe
*                   localtime (and similar)
*
* ********************************************* */

#include <ctime>
#include <string>
#include <chrono>

// FYI: 
// namespace AsyncLogger::internal ONLY in Asynctime.cpp
//          tstring put_time(const struct tm* tmb, const char* c_time_format)

namespace AsyncLogger
{
  typedef std::chrono::steady_clock::time_point steady_time_point;
  typedef std::chrono::time_point<std::chrono::system_clock>  system_time_point;
  typedef std::chrono::milliseconds milliseconds;
  typedef std::chrono::microseconds microseconds;

  //  wrap for std::chrono::system_clock::now()
  std::time_t systemtime_now();

  /** return time representing POD struct (ref ctime + wchar) that is normally
  * retrieved with std::localtime. AsyncLogger::localtime is threadsafe which std::localtime is not.
  * AsyncLogger::localtime is probably used together with @ref AsyncLogger::systemtime_now */
  tm localtime(const std::time_t& time);

  /** format string must conform to std::put_time's demands.
  * WARNING: At time of writing there is only so-so compiler support for
  * std::put_time. A possible fix if your c++11 library is not updated is to
  * modify this to use std::strftime instead */
  tstring localtime_formatted(const std::time_t& time_snapshot, const tstring& time_format) ;
}

#endif
