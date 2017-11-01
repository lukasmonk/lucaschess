
//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-
//
//	Gerbil
//
//	Copyright (c) 2001, Bruce Moreland.  All rights reserved.
//
//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-
//
//	This file is part of the Gerbil chess program project.
//
//	Gerbil is free software; you can redistribute it and/or modify it under
//	the terms of the GNU General Public License as published by the Free
//	Software Foundation; either version 2 of the License, or (at your option)
//	any later version.
//
//	Gerbil is distributed in the hope that it will be useful, but WITHOUT ANY
//	WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
//	FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
//	details.
//
//	You should have received a copy of the GNU General Public License along
//	with Gerbil; if not, write to the Free Software Foundation, Inc.,
//	59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//
//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include "engine.hpp"
#ifdef USE_PTHREADS
    #include <pthread.h>
    #include <errno.h>
    #ifdef TARGET_WIN
    // use pthreads on windoze, for testing only
    #pragma comment(lib,"pthreadVC2.lib")
    #endif
#else
    #ifdef TARGET_WIN
    #include <windows.h>
    #endif
#endif
#include "gproto.hpp"
#include "winboard.hpp"

//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-

static char const s_aszModule[] = __FILE__;

//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-

//	This is the engine thread.  The following documentation is very important
//	and unless you understand it, you won't understand how any of this works.

//	Commands are sent to the engine thread by the input thread by use of
//	"VSendEngineCommand".  This function sits (maybe for a while) on an event
//	called "s_evars.heventCmdFinished".  If this is not set, it assumes
//	that the engine is working on a command, and waits.

//	The engine thread sits in a loop ("DwEngine"), processing engine commands
//	from the input thread, via use of "VProcessEngineCommand".  This function
//	waits for "s_evars.heventCmdPresent" to be set, and if it finds it,
//	it reads it and processes it, then it sets "s_evars.heventCmdFinished",
//	so the input thread can send another command.

//  If it gets a command that tells it to go think, it sets the command
//	finished event and calls "VPrThink" or "VPrAnalyze".

//	If it calls either of these, it's gone until these functions end, and that
//	could be a very, very long time.  But it is still possible to issue engine
//	commands and have them responded to.

//	How it works is that the engine is required to call "VPrCallback" every
//	once in a while, while it is thinking.

//	And what "VPrCallback" does is call into here ("VProcessEngineCommand")
//	and polls to see if there is an engine command ready, and if so it
//	executes it.

//	Many of the engine commands can be handled while the engine is thinking.
//	For those that can be handled, they are just handled normally.

//	For those that can't be handled, something special needs to be done.  The
//	current state is that we're sitting in "VProcessEngineCommand", with a
//	whole bunch of chess engine thinking context in the backtrace, and then
//	the original called "VProcessEngineComand" further back in the trace.  The
//	input thread is either off doing its own thing or it's blocked, waiting
//	for us to finish this command so it can send another one.

//	What is done is that the engine is told to abort its thinking, and then
//	the "VProcessEngineCommand" function returns.  The current command is
//	still executing, so the input thread can't talk to us.

//	The chess engine will then unwind its stack and return, and we end up
//	sitting in the original "VProcessEngine" function, which then loops back
//	up to the top and grabs another engine command, which is, not by
//	coincidence, guaranteed to be the same one it just tried to process.

//	It then processes it and sets the command finished event.

//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-

class cy_event {
public:
#ifdef USE_PTHREADS
    pthread_cond_t  eventCond;
    pthread_mutex_t eventMutex;
    volatile bool    signaled;
#else
    HANDLE          eventHandle;
#endif

    cy_event() {}
    void create() {
#ifdef USE_PTHREADS
        pthread_cond_init(&eventCond, NULL);
        pthread_mutex_init(&eventMutex, NULL);
        signaled = false;
#else
        eventHandle = CreateEvent(NULL, FALSE, FALSE, NULL);
#endif
    }
    void destroy() {
#ifdef USE_PTHREADS
#else
#endif
    }
    void signal() {
#ifdef USE_PTHREADS
        pthread_mutex_lock(&eventMutex);
        //pthread_cond_broadcast, pthread_cond_signal
//        pthread_cond_signal(&eventCond);
        pthread_cond_broadcast(&eventCond);
        signaled = true;
        pthread_mutex_unlock(&eventMutex);
#else
    SetEvent(eventHandle);
#endif
    }
    /// infinite wait for a signal
    bool wait_signal() {
#ifdef USE_PTHREADS
        pthread_mutex_lock(&eventMutex);
        if( !signaled )
            int res = pthread_cond_wait(&eventCond, &eventMutex);
        signaled = false;
        pthread_mutex_unlock(&eventMutex);
#else
        WaitForSingleObject(eventHandle, INFINITE);
#endif
        return true;
    }
    /// return false if not signaled (timeout)
    bool check_signal() {
#ifdef USE_PTHREADS
        pthread_mutex_lock(&eventMutex);
        int res = 0;
        if( !signaled ) {
            struct timespec abstime;
            abstime.tv_sec = 0;
            abstime.tv_nsec = 0;
            res = pthread_cond_timedwait(&eventCond, &eventMutex, &abstime);
        }
        signaled = false;
        pthread_mutex_unlock(&eventMutex);
        if( res == ETIMEDOUT )
            return false;
        Assert( res == 0 );

#else
	    if (WaitForSingleObject(eventHandle, 0) == WAIT_TIMEOUT)
		    return false;
#endif
        return true;
    }
};

#ifdef USE_PTHREADS
#define thread_fun_ret_type void *
typedef void * thread_fun_param_type;
#else
#define thread_fun_ret_type DWORD WINAPI
typedef LPVOID thread_fun_param_type;
#endif

typedef	struct tagEVARS {
    cy_event    heventCmdPresent;
    cy_event    heventCmdFinished;

    char	aszEngineInBuf[1024];	// Command input buffer.
	bool    fLegal;					// If we get an illegal "setboard"
									//  command, until we get another one
									//  we respond to every attempt to move
									//  by emitting an error message.
									// It should respond to "undo" commands
									//  and so on by emitting and error
									//  message, but I didn't architect this
									//  properly.
}	EVARS;

#ifdef USE_PTHREADS
static EVARS s_evars;
#else
static EVARS s_evars;
#endif


//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-


static void SetPresent(void) {
    s_evars.heventCmdPresent.signal();
}
static void SetFinished(void) {
    s_evars.heventCmdFinished.signal();
}


//	Wait until the engine is ready, and then say what you want to say.

void VSendToEngine(const char * szFmt, ...)
{
	char	aszBuf[2048];
	va_list	pvaArg;

    s_evars.heventCmdFinished.wait_signal();
	va_start(pvaArg, szFmt);
	vsprintf(aszBuf, szFmt, pvaArg);
	VStripWb(aszBuf);
	strcpy(s_evars.aszEngineInBuf, aszBuf);
	SetPresent();
}

//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-

static bool FEcmdSetboard(PCON pv, char * szCmd,
	char * rgsz[], int csz, bool fThinking)
{
	char	asz[256];

	Assert(csz == 7);
	if (fThinking)
		return true;
	sprintf(asz, "%s %s %s %s %s %s",
		rgsz[1], rgsz[2], rgsz[3], rgsz[4], rgsz[5], rgsz[6]);
	if (!(s_evars.fLegal = FPrSetboard(pv, asz)))
		VSendToWinboard("tellusererror Illegal position");
	SetFinished();
	return false;
}

static bool FEcmdHint(PCON pv, char * szCmd,
	char * rgsz[], int csz, bool fThinking)
{
	if (fThinking)
		VPrHint(pv);
	SetFinished();
	return false;
}

static bool FEcmdBk(PCON pv, char * szCmd,
	char * rgsz[], int csz, bool fThinking)
{
	if (!fThinking)
		VPrBk(pv);
	VPrSendBookLine("");
	SetFinished();
	return false;
}

static bool FEcmdThink(PCON pv, char * szCmd,
	char * rgsz[], int csz, bool fThinking)
{
	Assert(csz >= 3);
	Assert(csz <= 4);
    char *move = (csz < 4) ? 0 : rgsz[3];

    VPrSendComment("in FEcmdThink fthinking=%d csz=%d", fThinking, csz);
	if (fThinking) {
		if (csz < 4)
			return true;
        if ( !FPrPonderHit(pv, rgsz[3], atol(rgsz[1]), atol(rgsz[2])) ) {
//            VPrSendComment("cmdThink: pondering NOT ok for %s\n", move);
			return true;
        }
//        VPrSendComment("# cmdThink: pondering ok for %s\n", move);
		SetFinished();
		return false;
	}
	if (csz > 3)
		if (!s_evars.fLegal)
			VSendToWinboard("Illegal move: %s", rgsz[3]);
		else if (!FPrAdvance(pv, rgsz[3])) {
			VSendToWinboard("Illegal move: %s", rgsz[3]);
			SetFinished();
			return false;
		}
	SetFinished();
    if (s_evars.fLegal) {
//        VPrSendComment("# cmdThink: move was %s\n", move);
		VPrThink(pv, atol(rgsz[1]), atol(rgsz[2]));
    }
	return false;
}

static bool FEcmdAnalyze(PCON pv, char * szCmd,
	char * rgsz[], int csz, bool fThinking)
{
	Assert(csz >= 0);
	if (fThinking)
		return true;
	if (csz > 1)
		if (!s_evars.fLegal)
			VSendToWinboard("Illegal move: %s", rgsz[1]);
		else if (!FPrAdvance(pv, rgsz[1])) {
			VSendToWinboard("Illegal move: %s", rgsz[1]);
			SetFinished();
			return false;
		}
	SetFinished();
	if (s_evars.fLegal)
		VPrAnalyze(pv);
	return false;
}

static bool FEcmdAbort(PCON pv, char * szCmd,
	char * rgsz[], int csz, bool fThinking)
{
	if (fThinking)
		return true;
	SetFinished();
	return false;
}

static bool FEcmdPost(PCON pv, char * szCmd,
	char * rgsz[], int csz, bool fThinking)
{
	VPrSetPost(pv, true);
	SetFinished();
	return false;
}

static bool FEcmdNoPost(PCON pv, char * szCmd,
	char * rgsz[], int csz, bool fThinking)
{
	VPrSetPost(pv, false);
	SetFinished();
	return false;
}

static bool FEcmdMove(PCON pv, char * szCmd,
	char * rgsz[], int csz, bool fThinking)
{
	Assert(csz == 2);
//    VSendToWinboard("in move, thinking = %d\n", fThinking);
	if (fThinking)
		return true;
	if (!s_evars.fLegal)
		VSendToWinboard("Illegal move: %s", rgsz[1]);
	else if (!FPrAdvance(pv, rgsz[1])) {
		VSendToWinboard("Illegal move: %s", rgsz[1]);
		SetFinished();
		return false;
	}
	SetFinished();
	return false;
}

static bool FEcmdLevel(PCON pv, char * szCmd,
	char * rgsz[], int csz, bool fThinking)
{
	Assert(csz == 4);
	if (fThinking)
		return true;
	VPrSetTimeControl(pv, atoi(rgsz[1]), atol(rgsz[2]), atol(rgsz[3]));
	SetFinished();
	return false;
}

static bool FEcmdSt(PCON pv, char * szCmd,
	char * rgsz[], int csz, bool fThinking)
{
	Assert(csz >= 1);
	if (fThinking)
		return true;
	VPrSetTimeControl(pv, 1, atol(rgsz[1]), 0);
	SetFinished();
	return false;
}

static bool FEcmdSd(PCON pv, char * szCmd,
	char * rgsz[], int csz, bool fThinking)
{
	Assert(csz >= 1);
	if (fThinking)
		return true;
	VPrSetFixedDepthTimeControl(pv, atoi(rgsz[1]));
	SetFinished();
	return false;
}

static bool FEcmdSnodes(PCON pv, char * szCmd,
	char * rgsz[], int csz, bool fThinking)
{
	Assert(csz >= 1);
	if (fThinking)
		return true;
	VPrSetFixedNodeCount(pv, atol(rgsz[1]));
	SetFinished();
	return false;
}

static bool FEcmdNps(PCON pv, char * szCmd,
	char * rgsz[], int csz, bool fThinking)
{
	Assert(csz >= 1);
	if (fThinking)
		return true;
	VPrSetFixedNodeCount(pv, atol(rgsz[1]));
	SetFinished();
	return false;
}

static bool FEcmdStatus(PCON pv, char * szCmd,
	char * rgsz[], int csz, bool fThinking)
{
	Assert(csz >= 1);
	if (fThinking) {
		STAT_REC	sr;

		if (FPrStatus(pv, &sr))
			VSendToWinboard("stat01: %lu " U64_FORMAT " %d %d %d %s",
				sr.tmElapsed / 10, sr.nodes, sr.plyDepth,
				sr.cLegalMoves - sr.iMoveSearching,
				sr.cLegalMoves, sr.aszSearching);
	}
	SetFinished();
	return false;
}

static bool FEcmdComputer(PCON pv, char * szCmd,
	char * rgsz[], int csz, bool fThinking)
{
	//	This is not supposed to be called while the engine is thinking, so
	//	just ignore it if the engine is thinking.
	//
	if (!fThinking)
		VPrOpponentComputer(pv);
	SetFinished();
	return false;
}

static bool FEcmdOppName(PCON pv, char * szCmd,
	char * rgsz[], int csz, bool fThinking)
{
	//	This is not supposed to be called while the engine is thinking, so
	//	just ignore it if the engine is thinking.
	//
	if (!fThinking) {
		while (*szCmd == ' ')
			szCmd++;
		while ((*szCmd != ' ') && (*szCmd != '\0'))
			szCmd++;
		while (*szCmd == ' ')
			szCmd++;
		VPrOpponentName(pv, szCmd);
	}
	SetFinished();
	return false;
}

static bool FEcmdRating(PCON pv, char * szCmd,
	char * rgsz[], int csz, bool fThinking)
{
	Assert(csz == 3);
	//
	//	This is not supposed to be called while the engine is thinking, so
	//	just ignore it if the engine is thinking.
	//
	if (!fThinking)
		VPrRating(pv, atol(rgsz[1]), atol(rgsz[2]));
	SetFinished();
	return false;
}

static bool FEcmdIcs(PCON pv, char * szCmd,
	char * rgsz[], int csz, bool fThinking)
{
	Assert(csz == 2);
	//
	//	This is not supposed to be called while the engine is thinking, so
	//	just ignore it if the engine is thinking.
	//
	if ((!fThinking) && (strcmp(rgsz[1], "-")))
		VPrIcs(pv, rgsz[1]);
	SetFinished();
	return false;
}

static bool FEcmdMovenow(PCON pv, char * szCmd,
	char * rgsz[], int csz, bool fThinking)
{
	if (fThinking)
		VPrMoveNow(pv);
	SetFinished();
	return false;
}

static bool FEcmdDraw(PCON pv, char * szCmd,
	char * rgsz[], int csz, bool fThinking)
{
	VPrDrawOffered(pv);
	SetFinished();
	return false;
}

static bool FEcmdUndo(PCON pv, char * szCmd,
	char * rgsz[], int csz, bool fThinking)
{
	if (fThinking)
		return true;
	if (s_evars.fLegal)
		VPrUndoMove(pv);
	SetFinished();
	return false;
}

static bool FEcmdRemove(PCON pv, char * szCmd,
	char * rgsz[], int csz, bool fThinking)
{
	if (fThinking)
		return true;
	if (s_evars.fLegal) {
		VPrUndoMove(pv);
		VPrUndoMove(pv);
	}
	SetFinished();
	return false;
}

static bool FEcmdPonder(PCON pv, char * szCmd,
	char * rgsz[], int csz, bool fThinking)
{
	if (fThinking)
		return true;
	VPrSetPonder(pv, true);
	SetFinished();
	return false;
}

static bool FEcmdNoponder(PCON pv, char * szCmd,
	char * rgsz[], int csz, bool fThinking)
{
	if (fThinking)
		return true;
	VPrSetPonder(pv, false);
	SetFinished();
	return false;
}

static bool FEcmdResult(PCON pv, char * szCmd,
	char * rgsz[], int csz, bool fThinking)
{
	if (fThinking)
		return true;
	//
	//	Skip "result".
	//
	while (*szCmd == ' ')
		szCmd++;
	while ((*szCmd != ' ') && (*szCmd != '\0'))
		szCmd++;
	//
	//	Skip 1-0 or whatever.
	//
	while (*szCmd == ' ')
		szCmd++;
	while ((*szCmd != ' ') && (*szCmd != '\0'))
		szCmd++;
	//
	//	Skip leading blanks to get to the real result
	//
	while (*szCmd == ' ')
		szCmd++;
	VPrResult(rgsz[1], szCmd);
	SetFinished();
	return false;
}

static bool FEcmdPerft(PCON pv, char * szCmd,
	char * rgsz[], int csz, bool fThinking)
{
	Assert(csz >= 0);
	if (fThinking)
		return true;
	SetFinished();
    int depth = 4;
    if (csz > 1) {
        depth = atoi(rgsz[1]);
	}
    VPrPerft(pv, depth);
	return false;
}

static bool FEcmdShowBook(PCON pv, char * szCmd,
	char * rgsz[], int csz, bool fThinking)
{
	if (fThinking)
		return true;
	SetFinished();
    VShowBook(pv);
	return false;
}

static bool FEcmdUci(PCON pv, char * szCmd,
	char * rgsz[], int csz, bool fThinking)
{
	if (fThinking)
		return true;
	VPrSetUciMode(pv, true);
	SetFinished();
	return false;
}


static bool FEcmdUciIsReady(PCON pv, char * szCmd,
	char * rgsz[], int csz, bool fThinking)
{
	if (fThinking)
		return true;
	SetFinished();
    VSendToWinboard("readyok");
	return false;
}

static bool FEcmdEval(PCON pv, char * szCmd,
	char * rgsz[], int csz, bool fThinking)
{
//	if (fThinking)
//		return true;
	SetFinished();
    pv->argste->evaluated = false;
    int staticVal = ValEval(pv, pv->argste, -20000, 20000);
	return false;
}

typedef	struct	tagECMD {
	const char * sz;
	int	cszMin;
	int	cszMax;
	bool (* pfn)(PCON pv, char * szCmd,
		char * rgsz[], int csz, bool fThinking);
}	ECMD, * PECMD;

static ECMD const c_argecmdEngine[] = {
	"setboard",		7,	7,		FEcmdSetboard,
	"think",		3,	4,		FEcmdThink,
	"move",			2,	2,		FEcmdMove,
	"remove",		1,	1,		FEcmdRemove,
	"abort",		1,	1,		FEcmdAbort,
	"hint",			1,	1,		FEcmdHint,
	"bk",			1,	1,		FEcmdBk,
	"analyze",		1,	2,		FEcmdAnalyze,
	"post",			1,	1,		FEcmdPost,
	"nopost",		1,	1,		FEcmdNoPost,
	"level",		4,	4,		FEcmdLevel,
	"movenow",		1,	1,		FEcmdMovenow,
	"draw",			1,	1,		FEcmdDraw,
	"undo",			1,	1,		FEcmdUndo,
	"ponder",		1,	1,		FEcmdPonder,
	"noponder",		1,	1,		FEcmdNoponder,
	"result",		1,	256,	FEcmdResult,
	"st",			2,	2,		FEcmdSt,
	"sd",			2,	2,		FEcmdSd,
	"status",		1,	1,		FEcmdStatus,
	"computer",		1,	1,		FEcmdComputer,
	"name",			2,	256,	FEcmdOppName,
	"rating",		3,	3,		FEcmdRating,
	"ics",			2,	2,		FEcmdIcs,
    "perft",        1,  2,      FEcmdPerft,
    "showbook",     1,  1,      FEcmdShowBook,

    "uci",          1,  1,      FEcmdUci,
    "isready",      1,  1,      FEcmdUciIsReady,

    "sn",           2,  2,      FEcmdSnodes,
    "nps",          2,  2,      FEcmdNps,

    "eval",         1,  1,      FEcmdEval,

	NULL,0,0,0
};

//	If this is called from "DwEngine", "fThinking" will be FALSE.  If it's
//	called from "VPrCallback", it will be TRUE.

void VProcessEngineCommand(PCON pv, bool fThinking)
{
	char	aszVec[1024];
	char	aszBuf[1024];
	char *	argsz[256];
	ECMD const * pcmd;
	int	csz;

    if( fThinking ) {
        if( !s_evars.heventCmdPresent.check_signal() )
            return;
    } else {
        if( !s_evars.heventCmdPresent.wait_signal() )
            return;
    }
//	if (WaitForSingleObject(s_evars.heventCmdPresent,
//		(fThinking) ? 0 : INFINITE) == WAIT_TIMEOUT)
//		return;
	//
	//	The buffer "s_evars.aszEngineInBuf" is mine, but if I send an ACK by
	//	setting "s_evars.heventCmdFinished", others are free to write all over
	//	that buffer.  There are some cases where I send an ack, then fiddle
	//	around with things some more.  So I'm going to copy the string out of
	//	the input buffer and then never deal with it again.
	//
	//	I don't know if I had bugs previously due to this, but I caught a
	//	potential new bug, and that's good enough.
	//
	strcpy(aszBuf, s_evars.aszEngineInBuf);
	strcpy(aszVec, s_evars.aszEngineInBuf);
	//
	//	Break the command into pieces.  It should not be possible to get a
	//	null command (csz == 0), but if I do, ignore it.
	//
	if (!(csz = CszVectorizeWb(aszVec, argsz)))
		return;
	//
	//	The command is broken out.  Groot through the command table and figure
	//	out which one to call.
	//
	for (pcmd = c_argecmdEngine; pcmd->sz != NULL; pcmd++) {
		if (strcmp(pcmd->sz, argsz[0]))
			continue;
		if (csz < pcmd->cszMin) {
			VErrorWb("internal error #1", aszBuf);
			break;
		}
		if (csz > pcmd->cszMax) {
			VErrorWb("internal error #2", aszBuf);
			break;
		}
//        VSendToWinboard("#     processing (%s)\n", argsz[0]);
		if ((*pcmd->pfn)(pv, aszBuf, argsz, csz, fThinking)) {
			VPrAbort(pv);
			SetPresent();
			return;
		}
		break;
	}
	if (pcmd->sz == NULL)
		VErrorWb("internal error #3", aszBuf);
}

// note for pthreads on windows : can not use fastcall here, but the prototype
// will use the same calling convention as the program being compiled 
// (pthread.h should explicitly say stdcall but don't)
thread_fun_ret_type DwEngine(thread_fun_param_type arg)
{
    CON *pv = (CON *) arg;
//	SetFinished();
	for (;;)
		VProcessEngineCommand( pv, false);
	return 0;
}

bool FInitEngineThread(int argc, char * argv[])
{
    CON * g_pv;

	s_evars.fLegal = true;
    s_evars.heventCmdFinished.create();
	s_evars.heventCmdPresent.create();
	SetFinished();
    if ((g_pv = (CON *) PvPrEngineInit(argc, argv)) == NULL)
		return false;
#ifdef USE_PTHREADS
    pthread_t pthread[1];
    pthread_attr_t attrib;
    pthread_attr_init( &attrib );
    pthread_attr_setstacksize( &attrib, 1024*64 );
    pthread_create(pthread, &attrib, DwEngine, (thread_fun_param_type) g_pv);
#else
	DWORD	dw;
	HANDLE	hthread;
	hthread = CreateThread(NULL, 0, DwEngine, g_pv, 0, &dw);
	if (hthread == NULL)
		return false;
#endif
	return true;
}
