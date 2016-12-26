//
// Este fuente sirve para manejar los diferentes sistemas operativos
#ifdef MINGW
#include <windows.h>
#endif

#include "../Ajedrez.h"
#include <sched.h>  // Sleep(0) sched_yield
#include <time.h>
#  include <unistd.h>
#  include <sys/time.h>
#  include <signal.h>
#include <pthread.h>

long TiempoTranscurrido()
{
	return (double(clock()) / double(CLOCKS_PER_SEC)) * 1000; // OK if CLOCKS_PER_SEC is small enough
}

void Espera()
{
	sched_yield();  // Sleep(0) in windows
}

bool quit;
bool cancel;
bool MasterRun;
void *sleepCond;
void *handle;
void go(void);
void Quit(void);

pthread_t thread;
pthread_cond_t IdleEvent;
pthread_cond_t EndJobCond;
pthread_mutex_t WaitLock;

void *ThreadMain(LPVOID lpParam )
{
	CPartida * p;
	p = (CPartida *)lpParam;
	MasterRun = false;
	while(true)
	{
		if(quit) 
			break;
		// si no hay trabajo esperamos
		// no deberia ser un pthread_wait...
		pthread_mutex_lock(&WaitLock);
		pthread_cond_wait(&IdleEvent,&WaitLock); 
		pthread_mutex_unlock(&WaitLock);
		if(quit) break;
		// hacemos el trabajo
		MasterRun = true;
		p->IterativeDeepening();
		p->AnalisisFinalizado = 1;
		if(p->ponderMove[0])
			Print("bestmove %s ponder %s\n",p->JugadaActual,p->ponderMove);
		else
			Print("bestmove %s\n",p->JugadaActual);
		pthread_mutex_lock(&WaitLock);
		pthread_cond_broadcast(&EndJobCond);
		pthread_mutex_unlock(&WaitLock);
		MasterRun = false;
	}
	return 0;
}

void Quit(void)
{
	cancel = true;
	quit = true;
	go();
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
		pthread_mutex_lock(&WaitLock);
		pthread_cond_wait(&EndJobCond,&WaitLock); 
		pthread_mutex_unlock(&WaitLock);
	}
	Stop();
	pthread_mutex_lock(&WaitLock);
	pthread_cond_broadcast(&IdleEvent);
	pthread_mutex_unlock(&WaitLock);
}

void StartThreads(void *lpParam )
{
	pthread_cond_init(&IdleEvent, NULL);
	pthread_cond_init(&EndJobCond, NULL);
	pthread_mutex_init(&WaitLock, NULL);
	// Initialize main thread
	pthread_create(&thread, NULL, ThreadMain, (void *)(&Partida));
}
