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

#include "thread.h"

// FIXME: gettimeofday is unreliable!!! change it! (really?)

#ifdef _WIN32
#define USE_TIMEGETTIME			// should be much more accurate, but... there's timeBeginPeriod which is potentially bad
#include <windows.h>
#undef min
#undef max
#include <time.h>
#include <process.h>

#if defined(USE_TIMEGETTIME) && !defined(__GNUC__)
#pragma comment(lib, "winmm.lib")
#endif

#else
#include <time.h>
#include <sys/time.h>
#include <pthread.h>
#include <unistd.h>
#include <sched.h>
#endif

namespace cheng4
{

// Mutex

Mutex::Mutex( int enabled ) : handle(0)
{
	if ( !enabled )
		return;
#ifdef _WIN32
	handle = new CRITICAL_SECTION;
	InitializeCriticalSection( (LPCRITICAL_SECTION)handle );
#else
	handle = new pthread_mutex_t;
	pthread_mutex_init( (pthread_mutex_t *)handle, 0 );
#endif
}

Mutex::~Mutex()
{
	if ( handle )
	{
#ifdef _WIN32
		DeleteCriticalSection( (LPCRITICAL_SECTION)handle );
		delete (CRITICAL_SECTION *)handle;
#else
		pthread_mutex_destroy( (pthread_mutex_t *)handle );
		delete (pthread_mutex_t *)handle;
#endif
	}
}

void Mutex::lock()
{
	if ( !handle )
		return;
#ifdef _WIN32
	EnterCriticalSection( (LPCRITICAL_SECTION)handle );
#else
	pthread_mutex_lock( (pthread_mutex_t *)handle );
#endif
}

void Mutex::unlock()
{
	if ( !handle )
		return;
#ifdef _WIN32
	LeaveCriticalSection( (LPCRITICAL_SECTION)handle );
#else
	pthread_mutex_unlock( (pthread_mutex_t *)handle );
#endif
}

// MutexLock

MutexLock::MutexLock( Mutex &m, bool nolock ) : mref( &m )
{
	if ( nolock )
		mref = 0;
	else
		mref->lock();
}

MutexLock::~MutexLock()
{
	if ( mref )
		mref->unlock();
}

// Event

Event::Event(bool autoReset_) : handle(0), handle2(0), autoReset( autoReset_ ), flag(0)
{
#ifdef _WIN32
	handle = CreateEvent( 0, autoReset ? FALSE : TRUE, FALSE, 0);
#else
	handle = new pthread_cond_t;
	pthread_cond_init((pthread_cond_t *)handle, 0);
	handle2 = new pthread_mutex_t;
	pthread_mutex_init( (pthread_mutex_t *)handle2, 0 );
#endif
}

Event::~Event()
{
#ifdef _WIN32
	if ( handle != 0 )
		CloseHandle( (HANDLE)handle );

#else
	pthread_cond_destroy((pthread_cond_t *)handle);
	delete (pthread_cond_t *)handle;
	pthread_mutex_destroy( (pthread_mutex_t *)handle2 );
	delete (pthread_mutex_t *)handle2;
#endif
}

bool Event::wait( int ms )
{
#ifdef _WIN32
	if ( handle != 0 )
	{
		return WaitForSingleObject( (HANDLE)handle,
			ms == -1 ? INFINITE : ms ) == WAIT_OBJECT_0;
	}
	return 0;
#else
	int res = 0;
	if ( ms == -1 )
	{
		pthread_mutex_lock( (pthread_mutex_t *)handle2 );
		while ( !flag && (res = pthread_cond_wait((pthread_cond_t *)handle, (pthread_mutex_t *)handle2)) == 0 )
			;
		if ( autoReset )
			flag = 0 ;
		pthread_mutex_unlock( (pthread_mutex_t *)handle2 );
	}
	else
	{
		struct timeval tv;
		struct timespec ts;
		gettimeofday(&tv, 0);
		tv.tv_sec += ms / 1000;
		tv.tv_usec += (ms % 1000) * 1000;
		// handle overflow
		while ( tv.tv_usec >= 1000000 )
		{
			tv.tv_usec -= 1000000;
			tv.tv_sec++;
		}
		ts.tv_sec = tv.tv_sec;
		ts.tv_nsec = tv.tv_usec * 1000;
		pthread_mutex_lock( (pthread_mutex_t *)handle2 );
		while ( !flag && (res = pthread_cond_timedwait((pthread_cond_t *)handle, (pthread_mutex_t *)handle2, &ts)) == 0 )
			;
		if ( res == 0 && autoReset )
			flag = 0 ;
		pthread_mutex_unlock( (pthread_mutex_t *)handle2 );
	}
	return res == 0;
#endif
}

void Event::signal()
{
#ifdef _WIN32
	if ( handle != 0 )
		SetEvent( (HANDLE)handle );
#else
	pthread_mutex_lock( (pthread_mutex_t *)handle2 );
	flag = 1;
	pthread_cond_signal((pthread_cond_t *)handle);
	pthread_mutex_unlock( (pthread_mutex_t *)handle2 );
#endif
}

void Event::reset()
{
#ifdef _WIN32
	if ( handle != 0 )
		ResetEvent( (HANDLE)handle );
#else
	pthread_mutex_lock( (pthread_mutex_t *)handle2 );
	flag = 0;
	pthread_mutex_unlock( (pthread_mutex_t *)handle2 );
#endif
}

// Thread

#ifdef _WIN32
static unsigned int __stdcall threadProc( void *param )
{
	static_cast<Thread *>(param)->work();
	return 0;
}
#else
static void *threadProc( void *param )
{
	static_cast<Thread *>(param)->work();
	return 0;
}
#endif

Thread::Thread( void *param ) : thrParam( param ), handle(0), shouldTerminate(0), killFlag(0)
{
}

// use this instead of dtor!
void Thread::destroy()
{
}

void Thread::wait()
{
	if ( !handle )
		return;			// not running
	destroy();
	shouldTerminate = 1;
#ifdef _WIN32
	WaitForSingleObject( (HANDLE)handle, INFINITE );
	CloseHandle( (HANDLE)handle );
#else
	void *vp = 0;
	if ( pthread_join( *(pthread_t *)handle, &vp ) !=  0 )
	{
#ifndef __ANDROID__
		pthread_setcanceltype( PTHREAD_CANCEL_ASYNCHRONOUS, 0 );
		pthread_cancel( *(pthread_t *)handle );
#endif
	}
	delete (pthread_t *)handle;
#endif
	handle = 0;
	shouldTerminate = 0;
}

Thread::~Thread()
{
	assert( killFlag && "don't call delete directly!" );
}

void Thread::kill()
{
	killFlag = 1;
	wait();
	delete this;
}

// returns current thread id
void *Thread::current()
{
#ifdef _WIN32
	return (void *)(uintptr_t)GetCurrentThreadId();
#else
	return (void *)pthread_self();
#endif
}

// create and run thread
bool Thread::run()
{
	if ( handle )
		return 0;			// already running
#ifdef _WIN32
	handle = (HANDLE)_beginthreadex( 0, 0, threadProc, (LPVOID)this, 0/*CREATE_SUSPENDED*/, 0 );
	if ( !handle || handle == INVALID_HANDLE_VALUE )
		return 0;
#else
	handle = new pthread_t;
	pthread_attr_t attr;
	pthread_attr_init(&attr);
	pthread_create( (pthread_t *)handle, 0 /* attr */, threadProc, this );
	pthread_attr_destroy(&attr);
#endif
	return 1;
}

bool Thread::setPriority(int prior)
{
#ifdef _WIN32
	if ( !handle )
		return 0;
	if ( prior > 3 )
		prior = 3;
	else if ( prior < - 4 )
		prior = -4;
	int tp;
	switch( prior )
	{
		case +3:
			tp = THREAD_PRIORITY_TIME_CRITICAL;
			break;
		case +2:
			tp = THREAD_PRIORITY_HIGHEST;
			break;
		case +1:
			tp = THREAD_PRIORITY_ABOVE_NORMAL;
			break;
		case 0:
			tp = THREAD_PRIORITY_NORMAL;
			break;
		case -1:
			tp = THREAD_PRIORITY_BELOW_NORMAL;
			break;
		case -2:
			tp = THREAD_PRIORITY_LOWEST;
			break;
		case -3:
			tp = THREAD_PRIORITY_IDLE;
			break;
		default:
			tp = THREAD_PRIORITY_NORMAL;
	}
	return SetThreadPriority( (HANDLE)handle, tp ) != FALSE;
#else
	struct sched_param param;
	param.sched_priority = -prior;
	/*int ret =*/ pthread_setschedparam (*(pthread_t *)handle, SCHED_OTHER, &param);
	return 1;
#endif
}

// sleep in ms
void Thread::sleep( int ms )
{
#ifdef _WIN32
	Sleep( ms );
#else
	::usleep( ms * 1000 );
#endif
}

// Timer

void Timer::init()
{
#if defined(_WIN32) && defined(USE_TIMEGETTIME)
	timeBeginPeriod(1);
#endif
}

void Timer::done()
{
#if defined(_WIN32) && defined(USE_TIMEGETTIME)
	timeEndPeriod(1);
#endif
}

i32 Timer::getMillisec()
{
#ifndef _WIN32
	struct timeval tp;
	struct timezone tzp;

	gettimeofday( &tp, &tzp );

	return (i32)((tp.tv_sec) * 1000 + tp.tv_usec / 1000);
#else

#ifdef USE_TIMEGETTIME
	return (i32)(timeGetTime() & 0xffffffffU);
#else
	return (i32)(GetTickCount() & 0xffffffffU);
#endif

#endif
}

}
