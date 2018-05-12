/** ==========================================================================
* a normal std::queue protected by a mutex for operations,
* making it safe for thread communication, using std::mutex from C++0x
* ref: http://www.stdthread.co.uk/doc/headers/mutex.html
*
* This is totally inspired by Anthony Williams lock-based data structures in
* Ref: "C++ Concurrency In Action" http://www.manning.com/williams
*
* Author : Ramesh Kumar K
*
*/

#ifndef SHARED_QUEUE
#define SHARED_QUEUE

#include <queue>
#include <mutex>
#include <exception>
#include <condition_variable>

# include <ppl.h>
# include "ConcRTExtras/ppl_extras.h"
# include "concurrent_queue.h"


#define MAX_TIME_TO_WAIT_ON_QUEUE_S 1 /*sec*/

using namespace Concurrency;
using namespace ppl_extras;


namespace shared_queue_globals
{
	extern std::chrono::seconds sec;
	extern std::chrono::seconds default_wait;
}


/** Multiple producer, multiple consumer thread safe queue
* Since 'return by reference' is used this queue won't throw */
template<typename T>
class shared_queue
{
	std::queue<T> queue_;
	critical_section m_;
	std::condition_variable_any data_cond_;

public:

	shared_queue& operator=(const shared_queue& other)
	{
		queue_ = other.queue_;

		return *this;
	}

	shared_queue(const shared_queue& other)
	{
		queue_ = other.queue_;
	}

public:

	shared_queue()
	{

	}

	void push(T item)
	{
		try
		{
			{
				std::unique_lock<critical_section> lock(m_);

				queue_.push(item);

				lock.unlock(); //Unlock the mutex

				data_cond_.notify_one();
			}
		}
		catch(...)
		{
			// TODO
		}
	}

	/// \return immediately, with true if successful retrieval
	bool try_and_pop(T& popped_item)
	{
		try
		{
			std::unique_lock<critical_section> lock(m_);

			if(queue_.empty())
			{
				return false;
			}

			popped_item = std::move(queue_.front());
			queue_.pop();

			lock.unlock();

			return true;
		}
		catch(...)
		{
			// TODO
		}

		return false;
	}

	/// Try to retrieve, if no items, wait till an item is available and try again
	bool wait_infitine_and_pop(T& popped_item)
	{
		try
		{
			std::unique_lock<critical_section> lock(m_); // note: unique_lock is needed for std::condition_variable::wait

			while(queue_.empty())
			{ //                       The 'while' loop below is equal to
				data_cond_.wait(lock);  //data_cond_.wait(lock, [](bool result){return !queue_.empty();});
			}

			popped_item = std::move(queue_.front());
			queue_.pop();

			return true;
		}
		catch(...)
		{
			// TODO
		}

		return false;
	}

	/// Try to retrieve, if no items, wait till an item is available and try again or exit after timeout
	bool wait_and_pop(T& popped_item)
	{
		try
		{
			std::unique_lock<critical_section> lock(m_); // note: unique_lock is needed for std::condition_variable::wait

			if(queue_.empty())
			{
				if(std::cv_status::no_timeout != data_cond_.wait_for(lock, shared_queue_globals::default_wait))
				{
					if (!queue_.empty())
					{
						popped_item = std::move(queue_.front());
						queue_.pop();

						return true;
					}
				}
			}
			else
			{
				popped_item = std::move(queue_.front());
				queue_.pop();

				return true;
			}

		}
		catch(...)
		{
			// TODO
		}

		return false;
	}

	/// Try to retrieve, if no items, wait till an item is available and try again or exit after timeout
	bool wait_for_and_pop(T& popped_item, unsigned long long wait_duration = MAX_TIME_TO_WAIT_ON_QUEUE_S)
	{
		try
		{
			std::unique_lock<critical_section> lock(m_); // note: unique_lock is needed for std::condition_variable::wait

			if(queue_.empty())
			{
				if(std::cv_status::no_timeout != data_cond_.wait_for(lock, wait_duration * shared_queue_globals::sec))
				{
					if (!queue_.empty())
					{
						popped_item = std::move(queue_.front());
						queue_.pop();

						return true;
					}
				}
			}
			else
			{
				popped_item = std::move(queue_.front());
				queue_.pop();

				return true;
			}

		}
		catch(...)
		{
			// TODO
		}

		return false;
	}

	bool empty() const
	{
		try
		{
			std::lock_guard<critical_section> lock(m_);

			return queue_.empty();
		}
		catch(...)
		{
			// TODO
		}
	}

	unsigned size() const
	{
		try
		{
			std::lock_guard<critical_section> lock(m_);

			return queue_.size();
		}
		catch(...)
		{
			// TODO
		}

		return 0;
	}

	void clear( )
	{
		try
		{
			std::lock_guard<critical_section> lock(m_);

			std::queue<T> empty;
			std::swap( queue_, empty );
		}
		catch(...)
		{
			// TODO
		}
	}

	void ReleaseAllLocks()
	{
		try
		{
			data_cond_.notify_all();
		}
		catch(...)
		{
			// TODO
		}
	}
};

#endif
