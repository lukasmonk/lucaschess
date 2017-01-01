/*
You can use this program under the terms of either the following zlib-compatible license
or as public domain (where applicable)

  Copyright (C) 2012-2015 Martin Sedlak

  This software is provided 'as-is', without any express or implied
  warranty.  In no event will the authors be held liable for any damages
  arising from the use of this software.

  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely, subject to the following restrictions:

  1. The origin of this software must not be misrepresented; you must not
     claim that you wrote the original software. If you use this software
     in a product, an acknowledgement in the product documentation would be
     appreciated but is not required.
  2. Altered source versions must be plainly marked as such, and must not be
     misrepresented as being the original software.
  3. This notice may not be removed or altered from any source distribution.
*/

#pragma once

#include "types.h"

namespace cheng4
{

// micro platform-dependent sys core

class Mutex
{
	Mutex( const Mutex & )
	{
		assert( 0 && "Mutex cannot be copied!" );
	}
	Mutex &operator =( const Mutex & )
	{
		assert( 0 && "Mutex cannot be copied!" );
		return *this;
	}
protected:
	void *handle;		// system-specific mutex handle
public:
	Mutex( int enabled = 1 );
	~Mutex();

	void lock();
	void unlock();
};

class MutexLock
{
protected:
	Mutex *mref;		// mutex refptr
public:
	MutexLock( Mutex &m, bool nolock = 0 );
	~MutexLock();
};

class Event
{
	Event( const Event & )
	{
		assert( 0 && "Event cannot be copied!" );
	}
	Event &operator =( const Event & )
	{
		assert( 0 && "Event cannot be copied!" );
		return *this;
	}
protected:
	void *handle, *handle2;
	bool autoReset;
	volatile bool flag;
public:
	Event( bool autoReset_ = 1 );
	~Event();

	bool wait( int ms = -1 );
	void reset();
	void signal();
};

// thread is not run/created until the first call to resume()
class Thread
{
protected:
	void *thrParam;
	void *handle;					// sys-specific handle
	volatile bool shouldTerminate;	// thread should terminate flag
	volatile bool killFlag;			// killing flag
public:
	Thread( void *param = 0 );
	// never implement dtor, implement destroy() instead
	virtual ~Thread();

	// use this instead of dtor!
	virtual void destroy();

	// run thread (first time)
	virtual bool run();

	// main worker proc
	// MUST be overridden
	virtual void work() = 0;

	// wait for thread to terminate
	void wait();

	// kill thread (DON'T USE DELETE!!!)
	// also waits for thread to terminate
	void kill();

	// sets thread priority: 0 = normal, -1 = below normal, +1 = above normal
	bool setPriority( int prio );

	// sleep in ms
	static void sleep( int ms );

	// returns current thread id
	static void *current();
};

struct Timer
{
	static void init();
	static void done();
	// get millisecond counter
	static i32 getMillisec();
};

}
