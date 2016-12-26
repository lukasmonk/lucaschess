//    Copyright 2010-2012 Antonio Torrecillas Gonzalez
//
//    This file is part of Simplex and Rocinante
//
//    Simplex is free software: you can redistribute it and/or modify
//    it under the terms of the GNU General Public License as published by
//    the Free Software Foundation, either version 3 of the License, or
//    (at your option) any later version.
//
//    Simplex is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//    GNU General Public License for more details.
//
//    You should have received a copy of the GNU General Public License
//    along with Simplex.  If not, see <http://www.gnu.org/licenses/>
//

//
// Este fuente sirve para manejar los diferentes sistemas operativos
// en este caso concreto WINDOWS
#ifndef _MSC_VER

#  include <unistd.h>
#include "sched.h"
#  include <sys/time.h>
#  include <signal.h>
#  include <pthread.h>


pthread_t miThread;

#define HANDLE int
#else

#include <windows.h>
#include <process.h>
#include <time.h>

#endif

#include <stdlib.h>
#include <string.h>
#include "CDebug.h"
#include "MoveList.h"
#include "UndoData.h"
#include "Board.h"

#include "system.h"


double frequency;

long TimeElapsed()
{
#ifdef _MSC_VER
static	LARGE_INTEGER start;
	if(frequency == 0)
	{
		LARGE_INTEGER proc_freq;

		if (!::QueryPerformanceFrequency(&proc_freq)) frequency = -1;
		else
		{
			frequency = (double)proc_freq.QuadPart;
			::QueryPerformanceCounter(&start);
		}
	}
	if(frequency > 0)
	{
		LARGE_INTEGER stop;
		::QueryPerformanceCounter(&stop);
		return (long)((stop.QuadPart - start.QuadPart) * 1000 / frequency);
	}
	else
#endif
	return (long)((double(clock()) / double(CLOCKS_PER_SEC)) * 1000); // OK if CLOCKS_PER_SEC is small enough
}

#ifdef _MSC_VER
unsigned __stdcall ThreadAnalisis( void* pArguments ) {
#else
void *ThreadAnalisis( void* pArguments ) {
#endif
#ifdef _MSC_VER
        _endthread();
#else
        pthread_exit(0);
#endif
	return 0;
}

HANDLE hThread;
unsigned int threadID;
void LanzaAnalisis(void *pArg)
{
#ifndef _MSC_VER
	pthread_create(&miThread, NULL, ThreadAnalisis, (void *)(pArg));
#else
	hThread = (HANDLE)_beginthreadex( NULL, 0, &ThreadAnalisis, pArg, 0,
        &threadID );
#endif
}

void CierraThreads()
{
#ifndef _MSC_VER
	pthread_cancel(miThread);
#else
    WaitForSingleObject( hThread, INFINITE );
    // Destroy the thread object.
    CloseHandle( hThread );
	hThread = 0;
#endif
}

void Wait()
{
#ifndef _MSC_VER
		sched_yield();
#else
	Sleep(0);
#endif
}
