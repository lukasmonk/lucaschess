//
// Este fuente sirve para manejar los diferentes sistemas operativos
// en este caso concreto WINDOWS
//#include <windows.h>
//#include <process.h>
#include "../Ajedrez.h"

#include <time.h>

long TiempoTranscurrido()
{
	return (double(clock()) / double(CLOCKS_PER_SEC)) * 1000; // OK if CLOCKS_PER_SEC is small enough
}

void LanzaAnalisis(void *pArg)
{
	CPartida * p;
	p = (CPartida *)pArg;
	p->IterativeDeepening();
	p->AnalisisFinalizado = 1;
}

void CierraThreads()
{
}

#ifdef OLD
unsigned __stdcall ThreadAnalisis( void* pArguments ) {
CPartida * p;
	p = (CPartida *)pArguments;
	p->ABReducido();
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
#endif
