//    Copyright 2010-2011 Antonio Torrecillas Gonzalez
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
	return (long)((double(clock()) / double(CLOCKS_PER_SEC)) * 1000); // OK if CLOCKS_PER_SEC is small enough
}

void Espera()
{
	Sleep(0);
}

bool quit;
bool cancel;
bool MasterRun;
void *sleepCond;
void *EndJobCond;
void *handle;
void go(void);
void Quit(void);

DWORD WINAPI ThreadMain(void * lpParam )
{
	CPartida * p;
	p = (CPartida *)lpParam;
	MasterRun = false;

	while(true)
	{
		if(quit) 
			break;
		// si no hay trabajo esperamos
		WaitForSingleObject(sleepCond, INFINITE);
		if(quit) break;
		// hacemos el trabajo
		MasterRun = true;
		p->IterativeDeepening();
		p->AnalisisFinalizado = 1;
		if(p->ponderMove[0])
			Print("bestmove %s ponder %s\n",p->JugadaActual,p->ponderMove);
		else
			Print("bestmove %s\n",p->JugadaActual);
		SetEvent(EndJobCond);
		MasterRun = false;
	}
	return 0;
}


void Quit(void)
{
	cancel = true;
	quit = true;
	go();
	CloseHandle(EndJobCond);
	CloseHandle(sleepCond);
}

void Stop(void)
{
	cancel = false;
}


void go(void)
{
	if(MasterRun) // espera activa a que termine
	{
		cancel = true;
		WaitForSingleObject(EndJobCond,10);// INFINITE);
	}
	Stop();
	SetEvent(sleepCond);
}

void StartThreads(LPVOID lpParam )
{
	sleepCond = CreateEvent(0, FALSE, FALSE, 0);
	EndJobCond= CreateEvent(0, FALSE, FALSE, 0);
	// Initialize main thread
	handle = CreateThread(NULL, 0, ThreadMain, (LPVOID)lpParam ,0,NULL); 
	if (handle == NULL)
	{
		exit(EXIT_FAILURE);
	}
}
