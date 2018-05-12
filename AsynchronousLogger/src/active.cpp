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
*AUTHOR		: RAMESH KUMAR K
*/

#include "stdafx.h"
#include "active.h"
#include <cassert>

using namespace AsyncLogger;

Active::Active(): done_(false){}

Active::~Active() {
  Callback quit_token = std::bind(&Active::doDone, this);
  send(quit_token); // tell thread to exit
  thd_.join();
}

// Add asynchronously a work-message to queue
void Active::send(Callback msg_){
	try
	{
		mq_.push(msg_);
	}
	catch (...)
	{

	}
}


// Will wait for msgs if queue is empty
// A great explanation of how this is done (using Qt's library):
// http://doc.qt.nokia.com/stable/qwaitcondition.html
void Active::run() {
  while (!done_) {
    // wait till job is available, then retrieve it and
    // executes the retrieved job in this thread (background)
	  try
	  {
		Callback func;
		if(mq_.wait_infitine_and_pop(func))
		{
			func();
		}
	  }
	  catch (...)
	  {

	  }
  }
}

// Factory: safe construction of object before thread start
std::unique_ptr<Active> Active::createActive(){
  std::unique_ptr<Active> aPtr(new Active());
  aPtr->thd_ = std::thread(&Active::run, aPtr.get());
  return aPtr;
}
