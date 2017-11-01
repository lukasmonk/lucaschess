//    Copyright 2010 Antonio Torrecillas Gonzalez
//
//    This file is part of Simplex.
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
#include <windows.h>
#include <process.h>
#include "../ajedrez.h"

#include <time.h>

LARGE_INTEGER start;
double frequency;

long TiempoTranscurrido()
{
	if(frequency == 0)
	{
		LARGE_INTEGER proc_freq;

		if (!::QueryPerformanceFrequency(&proc_freq)) frequency = -1;
		else
		{
			frequency = proc_freq.QuadPart;
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
	return (long)((double(clock()) / double(CLOCKS_PER_SEC)) * 1000); // OK if CLOCKS_PER_SEC is small enough
}

unsigned __stdcall ThreadAnalisis( void* pArguments ) {
CPartida * p;
	p = (CPartida *)pArguments;
	p->IterativeDeepening();
	p->AnalisisFinalizado = 1;
	_endthread();
	return 0;
}

HANDLE hThread;
unsigned int threadID;

void LanzaAnalisis(void *pArg)
{
	hThread = (HANDLE)_beginthreadex( NULL, 0, &ThreadAnalisis, pArg, 0,
        &threadID );

//	ThreadAnalisis(pArg);
}

void CierraThreads()
{
    WaitForSingleObject( hThread, INFINITE );
    // Destroy the thread object.
    CloseHandle( hThread );
	hThread = 0;
}

