/** ==========================================================================
*
* Example of a Active Object, using C++11 std::thread mechanisms to make it
* safe for thread communication.
*
* This was originally published at http://sites.google.com/site/kjellhedstrom2/active-object-with-cpp0x
* and inspired from Herb Sutter's C++11 Active Object
* http://herbsutter.com/2010/07/12/effective-concurrency-prefer-using-active-objects-instead-of-naked-threads
*
*
*/

#ifndef ACTIVE_H_
#define ACTIVE_H_

#include <thread>
#include <functional>
#include <condition_variable>
#include <mutex>
#include <memory>

#include "shared_queue.h"

namespace AsyncLogger {
typedef std::function<void()> Callback;

class Active {
private:
  Active(const Active&); // c++11 feature not yet in vs2010 = delete;
  Active& operator=(const Active&); // c++11 feature not yet in vs2010 = delete;
  Active();                         // Construction ONLY through factory createActive();
  void doDone(){done_ = true;}
  void run();

  shared_queue<Callback> mq_;
  std::thread thd_;
  bool done_;  // finished flag to be set through msg queue by ~Active


public:
  virtual ~Active();
  void send(Callback msg_);
  static std::unique_ptr<Active> createActive(); // Factory: safe construction & thread start
};
} // end namespace AsyncLogger



#endif
