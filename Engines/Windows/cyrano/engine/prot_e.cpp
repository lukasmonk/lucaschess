
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

#include "engine.hpp"
#include "gproto.hpp"
#include "board.hpp"
#include "moves.hpp"
#include "hash.hpp"
#include "newbook.hpp"

#ifdef	DEBUG
static char const s_aszModule[] = __FILE__;
#endif

//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-

//	This is how my program implements my engine/Winboard protocol.  If you
//	want to use my code to hook your engine up to Winboard, all you should
//	have to do is write these functions.

//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-

//	Tells the engine to think forever on this position.

void VPrAnalyze(PCON pcon)
{
	Assert(pcon->smode == smodeIDLE);
	VThink(pcon, tmANALYZE, tmANALYZE);
}

//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-

//	Tells the engine to think for "tmUs", then emit a move, and start
//	pondering if that flag is on.

void VPrThink(PCON pcon, unsigned long tmUs, unsigned long tmThem)
{
	Assert(pcon->smode == smodeIDLE);
	VThink(pcon, tmUs, tmThem);
}

//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-

//	Clears the current game and starts a new one based upon "szFen".

bool FPrSetboard(PCON pcon, char * szFen)
{
	Assert(pcon->smode == smodeIDLE);
    // not in SetBoard because it is called often when reading the book
    // commented out because setboard is called for every positions in uci mode
//	VClearHashe();
//	VClearHashp();
	pcon->fDrawOffered = false;
	return SetBoardFromFEN(pcon, szFen);
}

//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-

//	Returns TRUE or FALSE depending upon whether the engine is currently
//	pondering.

bool FPrPondering(PCON pcon)
{
	return (pcon->smode == smodePONDER);
}

//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-

//	This will eventually respond by sending book moves.  For now it does
//	nothing.

void VPrBk(PCON pcon)
{
	PSTE pste = &pcon->argste[0];
/*
	bGenMoves(pcon, pste);
    foreachmove(pste,pcm) {
		HASHK	hashkFull;

		bMove(pcon, pste, pcm->m);
		hashkFull = (pste+1)->hashkPc;
		bUndoMove(pcon, pste, pcm->m);
		if (FBnHashSet(hashkFull)) {
			char	asz[32];
			SAN	san;

			CmToSan(pcon, pste, pcm, &san);
			SanToSz(&san, asz);
			VPrSendBookLine(asz);
		}
	}
*/
}

//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-

//	A move is passed in, and if the engine is pondering, the engine goes
//	into normal "think" mode.

//	At the time the engine started pondering, "tmStart" was set.  This
//	routine will call "VSetTime", which will set the end time based upon
//	the time pondering started.

bool FPrPonderHit(PCON pcon, char * szMove, unsigned long tmUs,
	unsigned long tmThem)
{
    if( pcon->isUCI ) {
	    pcon->smode = smodeTHINK;
	    pcon->ss.tmUs = tmUs;
	    pcon->ss.tmThem = tmThem;
	    VSetTime(pcon);
    } else {
	    if (pcon->smode != smodePONDER)
		    return false;
	    if (strcmp(pcon->aszPonder, szMove))
		    return false;
	    pcon->smode = smodeTHINK;
	    pcon->ss.tmUs = tmUs;
	    pcon->ss.tmThem = tmThem;
	    VSetTime(pcon, true);
    }
	return true;
}

//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-

//	Adds this move to the current game.

bool FPrAdvance(PCON pcon, char * szMove)
{
	Assert(pcon->smode == smodeIDLE);
	return FAdvance(pcon, szMove);
}

//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-

//	Tells the engine whether or not to post analysis while thinking.

void VPrSetPost(PCON pcon, bool fPost)
{
	pcon->fPost = fPost;
}

//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-

void VPrSetTimeControl(PCON pcon, int cMoves,
	unsigned long tmBase, unsigned long tmIncr)
{
	VSetTimeControl(pcon, cMoves, tmBase, tmIncr);
}

//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-

void VPrSetFixedDepthTimeControl(PCON pcon, int plyDepth)
{
	VSetFixedDepthTimeControl(pcon, plyDepth);
}

void VPrSetFixedNodeCount(PCON pcon, U64 nodeCount) {
    VSetFixedNodeCount(pcon, nodeCount);
}
//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-

//	This initiates a request to move now.  The engine is thinking but may
//	be pondering.

void VPrMoveNow(PCON pcon)
{
	pcon->ss.tmEnd = pcon->ss.tmStart;
	pcon->ss.tmEndLimit = pcon->ss.tmStart;
    pcon->ss.canStop = true;
//	pcon->ss.tmExtra = 0;
}

//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-

//	Back up in the game list.

void VPrUndoMove(PCON pcon)
{
	Assert(pcon->smode == smodeIDLE);
	VUndoMove(pcon);
}

//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-

//	Set ponder flag TRUE or FALSE.

void VPrSetPonder(PCON pcon, bool fPonder)
{
	pcon->fPonder = fPonder;
}

//	Set UCI flag TRUE or FALSE.

void VPrSetUciMode(PCON pcon, bool isUCI)
{
	pcon->isUCI = isUCI;
}

//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-

//	Abort the search immediately.

void VPrAbort(PCON pcon)
{
	pcon->fAbort = true;
}

//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-

//	This does nothing in Gerbil.

void VPrResult(const char * szResult, const char * szWhy)
{
}

//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-

//	Name of engine must be second line in here.
#define STRINGIFY(x)    #x
#define TOSTRING(x) STRINGIFY(x)
#if defined(__INTEL_COMPILER)
    #define COMPILER_NAME "INTEL " TOSTRING(__INTEL_COMPILER)
#elif defined(_MSC_VER)
    #define COMPILER_NAME "MSVC " TOSTRING(_MSC_VER)
#elif defined(__MINGW32__)
    #define COMPILER_NAME "MINGW32" TOSTRING(__MINGW32_MAJOR_VERSION) TOSTRING(__MINGW32_MINOR_VERSION)
#elif defined(__GNUC__)
    #define COMPILER_NAME "GCC " TOSTRING(__GNUC__) TOSTRING(__GNUC_MINOR__) TOSTRING(__GNUC_PATCHLEVEL__)
#elif defined(__CYGWIN__)
    #define COMPILER_NAME "CYGWIN"
#endif


#ifndef COMPILER_NAME
#define COMPILER_NAME "unknown"
#endif

static const char * const c_argszHelp[] = {
	"",
	"Cyrano"
#ifdef _DEBUG
    " (debug build)"
#endif
    " " VERSION,
    "compiler: " COMPILER_NAME,
	"Copyright (c) 2007, 2008, Harald Johnsen.  All rights reserved.",
	"Cyrano is derived from Gerbil (or not)",
	"",
	"Please use a Winboard or UCI interface.  Please see the file",
	"\"readme.txt\" for details.",
	"Before you do anything, type \"protover 2\" and press the enter key.",
	">perft n",
	">showbook",
	"",
	NULL,
};

void VPrBanner(void)
{
	for (int i = 0; c_argszHelp[i] != NULL; i++)
		VPrSendComment(c_argszHelp[i]);
}

//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-

//	Returns the engine's name.  The engine's name must be the second string in
//	the "c_argszHelp" array above.

void VPrMyName(char * szName)
{
	strcpy(szName, c_argszHelp[1]);
}

//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-

//	This passes back the move I'm pondering on.

void VPrHint(PCON pcon)
{
	if (pcon->smode != smodePONDER)
		return;
	if (pcon->aszPonder[0] != '\0')
		VPrSendHint(pcon->aszPonder);
}

//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-

bool FPrStatus(PCON pcon, PSTAT_REC psr)
{
	Assert(pcon->smode != smodeIDLE);
	psr->tmElapsed = TmNow() - pcon->ss.tmStart;
	psr->nodes = pcon->ss.nodes;
	psr->plyDepth = pcon->ss.plyDepth;
	psr->iMoveSearching = pcon->ss.ccmLegalMoves - pcon->ss.icmSearching;
	psr->cLegalMoves = pcon->ss.ccmLegalMoves;
	strcpy(psr->aszSearching, pcon->ss.aszSearching);
	return true;
}

//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-

//	This engine ignores draw offers.

void VPrDrawOffered(PCON pcon)
{
	pcon->fDrawOffered = true;
}

//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-

void VPrOpponentComputer(PCON pcon)
{
	VPrSendComment("Opponent is a computer.");
}

//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-

void VPrOpponentName(PCON pcon, char * szName)
{
	VPrSendComment("Opponent is: %s", szName);
}

//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-

void VPrRating(PCON pcon, int eloUs, int eloThem)
{
	VPrSendComment("My rating is %ld; opponent's rating is %ld.",
		eloUs, eloThem);
}

//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-

void VPrIcs(PCON pcon, char * szIcs)
{
	VPrSendComment("I am playing on: %s", szIcs);
}

//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-

//	Inits engine and returns pointer to context.

void * PvPrEngineInit(int argc, char * argv[])
{
	return (void *)PconInitEngine(argc, argv);
}

//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-

void VPrPerft(PCON pcon, int depth) {
    perft(pcon, depth);
}

void VShowBook(PCON pcon) {
    book_disp(pcon, pcon->argste);
}
