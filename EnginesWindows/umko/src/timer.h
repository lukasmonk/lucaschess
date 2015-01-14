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

#ifndef TIMER_H
#define TIMER_H

#ifndef __MINGW32__
#include <pthread.h>
#else
#include <windows.h>
#endif

class Timer{
public:
    static void stop();
	static void start(const int tim);
private:
#if defined(__MINGW32__)
	static HANDLE timer_lock;
	friend DWORD WINAPI run_timer(LPVOID arg);
    static int time;
#else
    static pthread_t timer;
    static timespec time;
    static pthread_cond_t timer_cond;
    static pthread_mutex_t timer_mutex;
    friend void* run_timer(void *);
#endif
};

#endif // TIMER_H
