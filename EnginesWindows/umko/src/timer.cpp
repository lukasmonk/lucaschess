/***************************************************************************
 *   Copyright (C) 2009 by Borko Bošković                                  *
 *   borko.boskovic@gmail.com                                              *
 *                                                                         *
 *   This program is free software: you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation, either version 3 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>. *
 ***************************************************************************/

#include <sys/time.h>
#include <iostream>

#include "timer.h"
#include "engine.h"


#if defined(__MINGW32__)
HANDLE Timer::timer_lock;
int Timer::time;
#else
pthread_t Timer::timer;
timespec Timer::time;
pthread_cond_t Timer::timer_cond = PTHREAD_COND_INITIALIZER;
pthread_mutex_t Timer::timer_mutex = PTHREAD_MUTEX_INITIALIZER;
#endif

#if defined(__MINGW32__)
DWORD WINAPI run_timer(LPVOID ){
	DWORD retcode = WaitForSingleObject(Timer::timer_lock, Timer::time);
	if(retcode == WAIT_TIMEOUT) Engine::stop_search();
    return NULL;
}
#else
void* run_timer(void *){
	pthread_mutex_lock(&Timer::timer_mutex);
	int retcode = pthread_cond_timedwait(&Timer::timer_cond, &Timer::timer_mutex, &Timer::time);
	if(retcode != 0) Engine::stop_search();
	pthread_mutex_unlock(&Timer::timer_mutex);
    return NULL;
}
#endif

void Timer::stop(){
#if defined(__MINGW32__)
	SetEvent(timer_lock);
#else
	pthread_mutex_lock(&timer_mutex);
	pthread_cond_signal(&timer_cond);
	pthread_mutex_unlock(&timer_mutex);
#endif
}


void Timer::start(const int time){
	if(time <= 0) return;

#ifndef __MINGW32__
	timeval current;
	int sec = time / 1000;
	int msec = (time - sec*1000);
	
	gettimeofday(&current, NULL);
	sec += current.tv_sec;
	msec += current.tv_usec/1000;
	if(msec >= 1000){
		sec++;
		msec -= 1000;
	}
	Timer::time.tv_sec = sec;
	Timer::time.tv_nsec = msec * 1000000;
	pthread_create(&timer,NULL,run_timer,NULL);
#else
	Timer::time = time;
	DWORD iID[1];
	timer_lock = CreateEvent(0, FALSE, FALSE, 0);
	CreateThread(NULL, 0, run_timer, NULL, 0, iID);
#endif
}
