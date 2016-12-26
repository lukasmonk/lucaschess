//    Copyright 2009-2012 Antonio Torrecillas Gonzalez
//
//    This file is part of Rocinante.
//
//    Rocinante is free software: you can redistribute it and/or modify
//    it under the terms of the GNU General Public License as published by
//    the Free Software Foundation, either version 3 of the License, or
//    (at your option) any later version.
//
//    Rocinante is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//    GNU General Public License for more details.
//
//    You should have received a copy of the GNU General Public License
//    along with Rocinante.  If not, see <http://www.gnu.org/licenses/>
//

#ifndef _MSC_VER
#  include <unistd.h>
#  include <sys/time.h>
#  include <signal.h>
#  include <pthread.h>
#else
#  include <windows.h>
#  define inline __inline
#endif
#include <assert.h>


#include <stdlib.h>
#include <stdio.h>

#include <string.h>
#include "CDebug.h"
#include "MoveList.h"
#include "UndoData.h"
#include "Board.h"

#include "TreeNode.h"
#include "JobWorker.h"
#include "SmpManager.h"
#include "system.h"

struct _ThreadData {
	JobWorker work;
	bool Running;
	bool Idle;
	bool Start;
	int index;
	bool Stop;
#ifndef _MSC_VER
	pthread_cond_t IdleEvent;
	pthread_t thread;
	pthread_mutex_t WaitLock;
#else
	HANDLE IdleEvent;
#endif
} ThreadData[MaxNumOfThreads];

SmpManager::SmpManager(void)
{

  NumThreads = 1;
  InitDone = false;
}

SmpManager::~SmpManager(void)
{
}

void SmpManager::Sleep()
{
	Wait();
}

void SmpManager::InitWorkers(int cpus)
{
  volatile int i;

  NumThreads = cpus;

  
  // All threads except the main thread should be initialized to idle state:
  for(i = 0; i < NumThreads; i++) {
	  ThreadData[i].Running = false;
	  ThreadData[i].Idle = true;
	  ThreadData[i].work.Node = NULL;
	  ThreadData[i].work.ThreadId = i+1;
	  ThreadData[i].Stop = false;
	  ThreadData[i].Start = false;
	  ThreadData[i].index = i;
#ifndef _MSC_VER
  pthread_cond_init(&ThreadData[i].IdleEvent, NULL);
  pthread_mutex_init(&ThreadData[i].WaitLock, NULL); 
#else
    ThreadData[i].IdleEvent = CreateEvent(0, FALSE, FALSE, 0);
#endif
  }
  // Launch the helper threads:
  for(i = 0; i < NumThreads; i++) {
#ifndef _MSC_VER
    pthread_create(&ThreadData[i].thread, NULL, SmpManager::RunThread, (void *)(&ThreadData[i]));
#else
    {
      DWORD iID[1];
      CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)SmpManager::RunThread, (LPVOID)(&ThreadData[i]), 0, iID);
    }
#endif
    // Wait until the thread has finished launching:
	while(!ThreadData[i].Running) Sleep();
	InitDone = true;
  }
}

void SmpManager::StopWorkers()
{
	int i;
	for(i = 0; i < NumThreads; i++)
	{
		ThreadData[i].Stop = true;
#ifndef _MSC_VER
		pthread_mutex_lock(&ThreadData[i].WaitLock);
		pthread_cond_broadcast(&ThreadData[i].IdleEvent);
		pthread_mutex_unlock(&ThreadData[i].WaitLock);
#else
		SetEvent(ThreadData[i].IdleEvent);
#endif
	}
}

void SmpManager::DoWork(TreeNode *w,bool EvalOpt,int credit)
{
	int i;
	if(w->fen[0] == '\0')
	{
		w = w;
		return;
	}
	if(NumThreads == 1)
	{
		// don't schedule.
		ThreadData[0].work.Node = w;
		ThreadData[0].work.EvalOptimism = EvalOpt;
		ThreadData[0].work.CreditNps = credit;
		ThreadData[0].work.DoJob();
		return;
	}
	if(InitDone == false)
	{
		InitWorkers(NumThreads);
		// start smp threads
		while(!SmpWorkers.AllStarted())SmpWorkers.Sleep();
	}
#ifndef _MSC_VER
	while(true)
	{
		for(i = 0; i < NumThreads; i++)
		{
		// buscamos alguien libre
			if(ThreadData[i].Idle && !ThreadData[i].Start )
			{
				ThreadData[i].work.Node = w;
				ThreadData[i].work.EvalOptimism = EvalOpt;
				ThreadData[i].work.CreditNps = credit;
				ThreadData[i].Start = true;
				// liberamos el thread
				pthread_mutex_lock(&ThreadData[i].WaitLock);
				pthread_cond_signal(&ThreadData[i].IdleEvent);
				pthread_mutex_unlock(&ThreadData[i].WaitLock);
				// esperamos a que se ponga en marcha
				while(ThreadData[i].Idle && ThreadData[i].Start){
					Sleep();
				}
				ThreadData[i].Start = false;
				return;
			}
		}
		// no hemos podido lanzarlo cedemos el paso a otro a ver si termina
		Sleep();
	}
#else
	while(true)
	{
		for(i = 0; i < NumThreads; i++)
		{
		// buscamos alguien libre
			if(ThreadData[i].Idle)// && ThreadData[i].Start == false)
			{
				ThreadData[i].work.Node = w;
				ThreadData[i].work.EvalOptimism = EvalOpt;
				ThreadData[i].work.CreditNps = credit;
				ThreadData[i].Start = true;
				// liberamos el thread
				SetEvent(ThreadData[i].IdleEvent);
				// esperamos a que se ponga en marcha
				while(ThreadData[i].Idle && ThreadData[i].Start) Sleep();
				ThreadData[i].Start = false;
				return;
			}
		}
		// no hemos podido lanzarlo cedemos el paso a otro a ver si termina
		Sleep();
	}
#endif
}

int SmpManager::AllIdle()
{
	int i;
	int ret = 1;
	if(NumThreads == 1)
		return true;
	for(i = 0; i < NumThreads; i++)
	{
		// buscamos alguien libre
		if(!ThreadData[i].Idle) 
		{
			ret = 0;
			break;
		}
	}
	return ret;
}

int SmpManager::AllStopped()
{
	int i;
	for(i = 0; i < NumThreads; i++)
	{
		// buscamos alguien libre
		if(ThreadData[i].Running)
		{
			return false;
		}
	}
	return true;
}
int SmpManager::AllStarted()
{
	int i;
	for(i = 0; i < NumThreads; i++)
	{
		// buscamos alguien libre
		if(!ThreadData[i].Running)
		{
			return false;
		}
	}
	return true;
}

void *SmpManager::RunThread(void *data)
{
	struct _ThreadData * pData = (struct _ThreadData *)data;
	pData->Running = true; // estamos vivos

#ifndef _MSC_VER
	while(true)
	{
		// we'r idle
		pData->Idle = true;
		// esperamos faena // Wait for work to do
		pthread_mutex_lock(&pData->WaitLock);
		if(!pData->Start)	// we are signaled while waiting for the lock ?
		{
			pthread_cond_wait(&pData->IdleEvent,&pData->WaitLock);
		}
		pthread_mutex_unlock(&pData->WaitLock);
		if(pData->Start) // check false awakening
		{
			pData->Idle = false;
			// si nos avisan de salir // if we are signaled to stop
			if(pData->Stop) break;
			// hacemos la faena
			pData->work.DoJob();
		}
    } // vuelta a empezar
#else
	while(true)
	{
		pData->Idle = true;
	// esperamos faena // Wait for work to do
      WaitForSingleObject(pData->IdleEvent, INFINITE);
	  if(pData->Start) // check false awakening
	  {
		  pData->Idle = false;
			// si nos avisan de salir // if we are signaled to stop
			if(pData->Stop) break;
		  // hacemos la faena
			pData->work.DoJob();
		  // avisamos volvemos a estar libres
		  pData->Start = false;
	  }
    } // vuelta a empezar
#endif
	// salida
	pData->Running = false; // No estamos vivos
	pData->Idle = true;
	return 0;
}
