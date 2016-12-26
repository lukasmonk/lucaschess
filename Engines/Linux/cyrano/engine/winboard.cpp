
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
#include <ctype.h>

#include "engine.hpp"
#include "gproto.hpp"
#include "deprec.hpp"
#include "winboard.hpp"
#include "uci.hpp"
#include "hash.hpp"
#include "util.hpp"

static char const s_aszModule[] = __FILE__;
IVARS	s_ivars;

//-------------------------------------------------------------------------
static FILE *dbgout = 0;

//	This is just a normal printf with a "fflush" after it.

void VSendToWinboard(const char * szFmt, ...)
{
	char	aszBuf[1024];
	va_list	lpArgPtr;

	va_start(lpArgPtr, szFmt);
	vsprintf(aszBuf, szFmt, lpArgPtr);
	printf("%s\n", aszBuf);
	fflush(stdout);
//    if( dbgout == 0 )
//        dbgout = fopen("\\cyrano.debug","w");
    if( dbgout ) {
	    fprintf(dbgout, "%s\n", aszBuf);
	    fflush(dbgout);
    }
}

//-------------------------------------------------------------------------

static void VAssertFailedW(const char * szMod, int iLine)
{
	VSendToWinboard("Assert Failed: %s+%d\n", szMod, iLine);
	exit(1);
}

//-------------------------------------------------------------------------

void VStripWb(char * sz)
{
	int	i;

	for (i = 0; sz[i]; i++)
		if (sz[i] == '\n') {
			sz[i] = '\0';
			break;
		}
}

//-------------------------------------------------------------------------

int CszVectorizeWb(char * sz, char * rgsz[])
{
	int	i;
	int	csz;

	for (csz = 0, i = 0; sz[i]; i++)
		if (sz[i] != ' ') {
			rgsz[csz++] = sz + i;
			for (;; i++) {
				if (sz[i] == ' ')
					break;
				if (sz[i] == '\0')
					break;
			}
			if (sz[i] == '\0')
				break;
			sz[i] = '\0';
		}
	return csz;
}

//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-

void VErrorWb(const char * szMsg, const char * szCmd)
{
	VSendToWinboard("Error (%s): %s", szMsg, szCmd);
}

//-------------------------------------------------------------------------

//	I'm going to init the engine after sending "done=0".  I'm a little
//	worried about doing this at inital startup.

//	The engine I'm working on now isn't that likely to take a long time to
//	boot, but another engine might, and Winboard might decide that the engine
//	isn't going to send any feature commands, which Winboard would interpret
//	as meaning that it's protover 1.

//	If the interface really is using protover 1, we won't ever get to this
//	routine, and the engine will be initialized when we find a command that
//	requires engine initialization to run.

static bool FCmdProtover(char * szCmd, char * rgsz[], int csz)
{
	static char const * s_argszFeatures[] = {
		"done=0",		// <-- Must be first.
		"ping=1",
		"setboard=1",
		"sigint=0",
		"sigterm=0",
		"colors=0",
		"name=1",
		"draw=1",
		"time=1",
		"reuse=1",
		"analyze=1",
		"ics=1",
		NULL,
	};

	Assert(csz > 1);
    switch(atoi(rgsz[1])) {
        case 1:
            s_ivars.iProtover = WB1;
            break;
        case 2:
            s_ivars.iProtover = WB2;
            break;
        default:
            s_ivars.iProtover = WB2;
            break;
    }
    if (s_ivars.iProtover == WB2 ) {
		int	i;
		char	asz[256];

		for (i = 0; s_argszFeatures[i] != NULL; i++) {
			VSendToWinboard("feature %s", s_argszFeatures[i]);
		}
		VPrMyName(asz);
		VSendToWinboard("feature myname=\"%s\"", asz);
		VSendToWinboard("feature done=1");
	}
	return true;
}

//-------------------------------------------------------------------------

bool FCmdXboard(char * szCmd, char * rgsz[], int csz)
{
	Assert(csz == 1);
	return true;
}

//-------------------------------------------------------------------------

bool FCmdNew(char * szCmd, char * rgsz[], int csz)
{
	Assert(csz == 1);
	s_ivars.fForce = false;
	VSendToEngine("setboard "
		"rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
	VSendToEngine("sd 0");
	s_ivars.coOnMove = coWHITE;					// Protover 1.
	if (s_ivars.wmode == wmodeANALYZE)
		VSendToEngine("analyze");
	return true;
}

//-------------------------------------------------------------------------

bool FCmdQuit(char * szCmd, char * rgsz[], int csz)
{
	Assert(csz == 1);
	return false;
}

//-------------------------------------------------------------------------

bool FCmdForce(char * szCmd, char * rgsz[], int csz)
{
	Assert(csz == 1);
	s_ivars.fForce = true;
	VSendToEngine("abort");
	return true;
}

//-------------------------------------------------------------------------

bool FCmdLevel(char * szCmd, char * rgsz[], int csz)
{
	int	iMoves;
	unsigned	tmMinutes;
	unsigned	tmSeconds;
	unsigned	tmIncr;
	char * sz;

	Assert(csz == 4);
	iMoves = atoi(rgsz[1]);
	sz = rgsz[2];
	tmMinutes = tmSeconds = 0;
	for (; *sz; sz++) {
		if (*sz == ':') {
			for (sz++; *sz; sz++) {
				if (!isdigit(*sz)) {
					VErrorWb("bad 'seconds' value", szCmd);
					return true;
				}
				tmSeconds = tmSeconds * 10 + *sz - '0';
			}
			break;
		}
		if (!isdigit(*sz)) {
			VErrorWb("bad 'minutes' value", szCmd);
			return true;
		}
		tmMinutes = tmMinutes * 10 + *sz - '0';
	}
	tmIncr = atol(rgsz[3]) * 1000;
	VSendToEngine("level %d %ld %ld",
		iMoves, (tmMinutes * 60 + tmSeconds) * 1000, tmIncr);
    s_ivars.tmIncr = tmIncr;    // HACK
	return true;
}

//-------------------------------------------------------------------------

bool FCmdSt(char * szCmd, char * rgsz[], int csz)
{
	Assert(csz == 2);
	VSendToEngine("st %lu", atol(rgsz[1]) * 1000);
	return true;
}

//-------------------------------------------------------------------------

bool FCmdSd(char * szCmd, char * rgsz[], int csz)
{
	int	plyDepth;

	Assert(csz == 2);
	plyDepth = atoi(rgsz[1]);
	if (plyDepth < 1)
		plyDepth = 1;
	VSendToEngine("sd %d", plyDepth);
	return true;
}

//-------------------------------------------------------------------------

bool FCmdSnodes(char * szCmd, char * rgsz[], int csz)
{
	int	nodes;

	Assert(csz == 2);
	nodes = atoi(rgsz[1]);
	if (nodes < 0)
		nodes = 0;
	VSendToEngine("sn %d", nodes);
	return true;
}

bool FCmdNps(char * szCmd, char * rgsz[], int csz)
{
	int	nodes;

	Assert(csz == 2);
	nodes = atoi(rgsz[1]);
	if (nodes < 0)
		nodes = 0;
	VSendToEngine("nps %d", nodes);
	return true;
}

//-------------------------------------------------------------------------

bool FCmdTime(char * szCmd, char * rgsz[], int csz)
{
	Assert(csz == 2);
	s_ivars.tmMyTime = atol(rgsz[1]) * 10;
	return true;
}

//-------------------------------------------------------------------------

bool FCmdOtim(char * szCmd, char * rgsz[], int csz)
{
	Assert(csz == 2);
	s_ivars.tmTheirTime = atol(rgsz[1]) * 10;
	return true;
}

//-------------------------------------------------------------------------

bool FCmdHook(char * szCmd, char * rgsz[], int csz)
{
	Assert(csz == 1);
	VSendToEngine("movenow");
	return true;
}

//-------------------------------------------------------------------------

bool FCmdPing(char * szCmd, char * rgsz[], int csz)
{
	Assert(csz == 2);
	VSendToWinboard("pong %s", rgsz[1]);
	return true;
}

//-------------------------------------------------------------------------

bool FCmdDraw(char * szCmd, char * rgsz[], int csz)
{
	Assert(csz == 1);
	VSendToEngine("draw");
	return true;
}

//-------------------------------------------------------------------------

bool FCmdResult(char * szCmd, char * rgsz[], int csz)
{
	//	This can have any number of parameters, because the comment field is
	//	not handled properly.
	//
	VSendToEngine("%s", szCmd);
	return true;
}

//-------------------------------------------------------------------------

bool FCmdSetboard(char * szCmd, char * rgsz[], int csz)
{
	Assert(csz == 7);
	VSendToEngine("setboard %s %s %s %s %s %s",
		rgsz[1], rgsz[2], rgsz[3], rgsz[4], rgsz[5], rgsz[6]);
	s_ivars.coOnMove = (rgsz[2][0] == 'w') ?	// Protover 1.
		coWHITE : coBLACK;
	if (s_ivars.wmode == wmodeANALYZE)
		VSendToEngine("analyze");
	return true;
}

//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-

bool FCmdHint(char * szCmd, char * rgsz[], int csz)
{
	Assert(csz == 1);
	VSendToEngine("hint");
	return true;
}

//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-

bool FCmdBk(char * szCmd, char * rgsz[], int csz)
{
	Assert(csz == 1);
	VSendToEngine("bk");
	return true;
}

//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-

bool FCmdUndo(char * szCmd, char * rgsz[], int csz)
{
	Assert(csz == 1);
	VSendToEngine("undo");
	s_ivars.coOnMove ^= 1;					// Protover 1.
	if (s_ivars.wmode == wmodeANALYZE)
		VSendToEngine("analyze");
	return true;
}

//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-

bool FCmdRemove(char * szCmd, char * rgsz[], int csz)
{
	Assert(csz == 1);
	VSendToEngine("remove");
	return true;
}

//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-

bool FCmdHard(char * szCmd, char * rgsz[], int csz)
{
	Assert(csz == 1);
	VSendToEngine("ponder");
	return true;
}

//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-

bool FCmdEasy(char * szCmd, char * rgsz[], int csz)
{
	Assert(csz == 1);
	VSendToEngine("noponder");	// This will cause a current search to abort.
	return true;
}

//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-

bool FCmdPost(char * szCmd, char * rgsz[], int csz)
{
	Assert(csz == 1);
	VSendToEngine("post");
	return true;
}

//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-

bool FCmdNopost(char * szCmd, char * rgsz[], int csz)
{
	Assert(csz == 1);
	VSendToEngine("nopost");
	return true;
}

//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-

bool FCmdAnalyze(char * szCmd, char * rgsz[], int csz)
{
	Assert(csz == 1);
	s_ivars.fForce = false;
	s_ivars.wmode = wmodeANALYZE;
	VSendToEngine("analyze");
	return true;
}

//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-

//	Exit analysis, not exit the whole thing.  All this does is turn the
//	engine off.

bool FCmdExit(char * szCmd, char * rgsz[], int csz)
{
	Assert(csz == 1);
	s_ivars.wmode = wmodeTHINK;
	VSendToEngine("abort");
	return true;
}

//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-

bool FCmdComputer(char * szCmd, char * rgsz[], int csz)
{
	Assert(csz == 1);
	VSendToEngine("computer");
	return true;
}

//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-

bool FCmdIcs(char * szCmd, char * rgsz[], int csz)
{
	Assert(csz == 2);
	VSendToEngine(szCmd);
	return true;
}

//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-

bool FCmdRating(char * szCmd, char * rgsz[], int csz)
{
	Assert(csz == 3);
	VSendToEngine("rating %ld %ld", atol(rgsz[1]), atol(rgsz[2]));
	return true;
}

//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-

bool FCmdName(char * szCmd, char * rgsz[], int csz)
{
	VSendToEngine("%s", szCmd);
	return true;
}

//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-

//	I spit out an error message if a few of the weirder functions are
//	encountered, including a few that I explicitly turn off with "feature"
//	commands.

bool FCmdUnimplemented(char * szCmd, char * rgsz[], int csz)
{
	Assert(csz >= 0);
	VErrorWb("Unimplemented", rgsz[0]);
	return true;
}

//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-

//	This section is for arcane protover 1 garbage.

//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-

//	This one just turns edit mode on, and gets us ready to accept pieces.

bool FCmdEdit(char * szCmd, char * rgsz[], int csz)
{
	int	i;

	Assert(csz == 1);
	s_ivars.fEditing = true;
	s_ivars.coEditing = coWHITE;
	for (i = 0; i < 64; i++) {
		s_ivars.argpcco[i].co = coMAX;
		s_ivars.argpcco[i].pc = pcMAX;
	}
	return true;
}

//	I need this to avoid a lot of ugly code.

char const s_argbPcWb[coMAX][pcMAX] = {
	'P',	'N',	'B',	'R',	'Q',	'K',
	'p',	'n',	'b',	'r',	'q',	'k',
};

//	I need a few defines for critical squares (64 square board, not 128).

#undef  isqA8
#undef  isqE8
#undef  isqH8

#define	isqA1	0
#define	isqE1	4
#define	isqH1	7
#define	isqA8	56
#define	isqE8	60
#define	isqH8	63

//	This ends edit mode.  I convert the internal piece array into a FEN and
//	spit it at the engine.  The whole reason this is so nasty and awful is
//	that I have to deal with the color to move.  Winboard protover 1 expected
//	you to keep track of that yourself, although it sometimes sends "white"
//	and "black" commands whose purpose seems to be completely without point.

bool FCmdDot(char * szCmd, char * rgsz[], int csz)
{
	Assert(csz == 1);
	if (s_ivars.fEditing) {
		char	aszFen[256];
		char *	sz = aszFen;
		bool	fAllowWhiteOO;
		bool	fAllowWhiteOOO;
		bool	fAllowBlackOO;
		bool	fAllowBlackOOO;
		int	rnk;
		int	fil;
		int	isq;

		for (rnk = 7; rnk >= 0; rnk--) {
			int	csqEmpty;

			if (rnk != 7)
				*sz++ = '/';
			csqEmpty = 0;
			for (fil = 0; fil < 8; fil++) {
				isq = rnk * 8 + fil;

				if (s_ivars.argpcco[isq].pc != pcMAX) {
					if (csqEmpty) {
						*sz++ = char(csqEmpty + '0');
						csqEmpty = 0;
					}
					*sz++ = s_argbPcWb[s_ivars.argpcco[isq].co]
						[s_ivars.argpcco[isq].pc];
				} else
					csqEmpty++;
			}
			if (csqEmpty)
				*sz++ = char(csqEmpty + '0');
		}
		*sz++ = ' ';
		*sz++ = (s_ivars.coOnMove == coWHITE) ? 'w' : 'b';
		*sz++ = ' ';
		fAllowWhiteOO = fAllowWhiteOOO = false;
		fAllowBlackOO = fAllowBlackOOO = false;
		if ((s_ivars.argpcco[isqE1].pc == pcKING) &&
			(s_ivars.argpcco[isqE1].co == coWHITE)) {
			if ((s_ivars.argpcco[isqH1].pc == pcROOK) &&
				(s_ivars.argpcco[isqH1].co == coWHITE))
				fAllowWhiteOO = true;
			if ((s_ivars.argpcco[isqA1].pc == pcROOK) &&
				(s_ivars.argpcco[isqA1].co == coWHITE))
				fAllowWhiteOOO = true;
		}
		if ((s_ivars.argpcco[isqE8].pc == pcKING) &&
			(s_ivars.argpcco[isqE8].co == coBLACK)) {
			if ((s_ivars.argpcco[isqH8].pc == pcROOK) &&
				(s_ivars.argpcco[isqH8].co == coBLACK))
				fAllowBlackOO = true;
			if ((s_ivars.argpcco[isqA8].pc == pcROOK) &&
				(s_ivars.argpcco[isqA8].co == coBLACK))
				fAllowBlackOOO = true;
		}
		if (fAllowWhiteOO)
			*sz++ = 'K';
		if (fAllowWhiteOOO)
			*sz++ = 'Q';
		if (fAllowBlackOO)
			*sz++ = 'k';
		if (fAllowBlackOOO)
			*sz++ = 'q';
		if (*(sz - 1) == ' ')
			*sz++ = '-';
		*sz++ = ' ';
		*sz++ = '-';
		*sz++ = ' ';
		*sz++ = '0';
		*sz++ = ' ';
		*sz++ = '1';
		*sz = '\0';
		s_ivars.fEditing = false;
		VSendToEngine("setboard %s", aszFen);
		if (s_ivars.wmode == wmodeANALYZE)
			VSendToEngine("analyze");
	} else
		VSendToEngine("status");
	return true;
}

//	Change colors.

bool FCmdC(char * szCmd, char * rgsz[], int csz)
{
	Assert(csz == 1);
	if (s_ivars.fEditing)
		s_ivars.coEditing ^= 1;
	return true;
}

//	In editing mode, we will get a "white" right after entering edit mode.  I
//	can only surmise that this means that we're placing white pieces.  I could
//	ignore this entirely, I think, but I might as well implement it.

bool FCmdWhite(char * szCmd, char * rgsz[], int csz)
{
	Assert(csz == 1);
	if (s_ivars.fEditing)
		s_ivars.coEditing = coWHITE;
	return true;
}

//	See the comment for "white".  I don't expect to get this, but who knows.

bool FCmdBlack(char * szCmd, char * rgsz[], int csz)
{
	Assert(csz == 1);
	if (s_ivars.fEditing)
		s_ivars.coEditing = coBLACK;
	return true;
}

//	This is called when I'm in edit mode and get something that I don't
//	recognize as a valid command.  It eats syntax such as "Pa4".  "xa4" is
//	defined but I won't get it.  I implement it anyway.

bool FEditCmd(char * rgsz[], int csz)
{
	int	pc;

	Assert(csz == 1);
	for (pc = pcPAWN; pc < pcMAX; pc++)
		if (rgsz[0][0] == s_argbPcWb[coWHITE][pc]) {
			int	isq;

lblSet:		if ((rgsz[0][1] < 'a') || (rgsz[0][1] > 'h'))
				return false;
			if ((rgsz[0][2] < '1') || (rgsz[0][2] > '8'))
				return false;
			if (rgsz[0][3] != '\0')
				return false;
			isq = (rgsz[0][1] - 'a') + (rgsz[0][2] - '1') * 8;
			s_ivars.argpcco[isq].pc = pc;
			s_ivars.argpcco[isq].co = s_ivars.coEditing;
			return true;
		}
	if (rgsz[0][0] == 'x') {	// We won't get this, but it's defined.
		pc = pcMAX;
		goto lblSet;
	}
	return false;
}

bool FCmdPerft(char * szCmd, char * rgsz[], int csz)
{
	int	plyDepth = 4;

	if(csz == 2)
	    plyDepth = atoi(rgsz[1]);
	if (plyDepth < 1)
		plyDepth = 1;
	VSendToEngine("perft %d", plyDepth);
	return true;
}

bool FCmdShowBook(char * szCmd, char * rgsz[], int csz)
{
	VSendToEngine("showbook");
	return true;
}

//-------------------------------------------------------------------------

static bool FCmdUci(char * szCmd, char * rgsz[], int csz) {
    // in uci we are in force mode
	s_ivars.fForce = true;
    s_ivars.iProtover = UCI;
    VSendToEngine("abort");
    VSendToEngine("uci");
    // send id
	char	asz[256];
	VPrMyName(asz);
    VSendToWinboard("id name Cyrano " VERSION);
    VSendToWinboard("id author Harald Johnsen");

    // send options
    cyranoOptions.print();

    // we are ok now
    VSendToWinboard("uciok");
    return true;
}

static bool FCmdUciDebug(char * szCmd, char * rgsz[], int csz) {
    // nop
    return true;
}

static bool FCmdUciIsReady(char * szCmd, char * rgsz[], int csz) {
	VSendToEngine("isready");
    return true;
}

static bool FCmdUciSetOption(char * szCmd, char * rgsz[], int csz) {
//    printf("# setoption count=%d\n", csz);
//    fflush(stdout);
    if( csz < 4 )
        return true;
//    printf("# setoption %s %s\n", rgsz[1], rgsz[3]);
    if( my_stricmp(rgsz[1], "name") )
        return true;
    if(!my_stricmp(rgsz[2],"clear")) {
        if(!my_stricmp(rgsz[3],"hash")) {
            // clear hash
            // init hash if not allready done
//            if( uciHash.has_changed() )
//                FInitHashe(pcon);
            printf("# clearing hash\n");
            VClearHashe();
            VClearHashp();
            return true;
        }
    }
    char optionName[512];
    strncpy(optionName, rgsz[2], sizeof(optionName));
    int x = 3;
    while(x < csz && my_stricmp(rgsz[x], "value") ) {
        strncat(optionName, " ", sizeof(optionName));
        strncat(optionName, rgsz[x], sizeof(optionName));
        x++;
    }
    if( 1+x < csz )
        cyranoOptions.set( optionName, rgsz[x + 1]);
    else
        cyranoOptions.set( optionName, "");
    return true;
}

static bool FCmdUciRegister(char * szCmd, char * rgsz[], int csz) {
    // nop
    return true;
}

static bool FCmdUciNewGame(char * szCmd, char * rgsz[], int csz) {
	Assert(csz == 1);
	s_ivars.fForce = true;
	VSendToEngine("setboard rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
	VSendToEngine("sd 0");
	s_ivars.coOnMove = coWHITE;
//	if (s_ivars.wmode == wmodeANALYZE)
//		VSendToEngine("analyze");
    return true;
}

static bool FCmdUciPosition(char * szCmd, char * rgsz[], int csz) {
    int nextArg = 1;
	s_ivars.fForce = true;
    if( csz <= 1 )
        return true;
    if( !my_stricmp(rgsz[1], "startpos") ) {
    	VSendToEngine("setboard rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
        s_ivars.coOnMove = coWHITE;
        nextArg = 2;
    } else if( !my_stricmp(rgsz[1], "fen") ) {
        if( csz < 8 ) {
            VErrorWb("not enought data after 'fen'", "position");
            return true;
        }
	    VSendToEngine("setboard %s %s %s %s %s %s",
		    rgsz[2], rgsz[3], rgsz[4], rgsz[5], rgsz[6], rgsz[7]);
        s_ivars.coOnMove = (rgsz[3][0] == 'w') ? coWHITE : coBLACK;
        nextArg = 8;
    }
    if( (nextArg < csz) && !my_stricmp(rgsz[nextArg], "moves") ) {
        nextArg++;
        while( nextArg < csz ) {
		    VSendToEngine("move %s", rgsz[nextArg]);
            s_ivars.coOnMove ^= 1;
            nextArg++;
        }
    }
    return true;
}

bool FCmdUciGo(char * szCmd, char * rgsz[], int csz)
{
    bool ponder = false;
    int nextArg = 1;
    int cmoves = 0;
    int tmIncr = 0;
    int plyDepth = 0;
    int nodes = 0;

    if( s_ivars.iProtover == UCI )
	    s_ivars.fForce = true;
    else {
        s_ivars.fForce = false;
        // don't 'forget' increment in WB mode !! 'level 0 1 2'
        tmIncr = s_ivars.tmIncr;
    }

    while( nextArg < csz ) {
        if( !my_stricmp(rgsz[nextArg], "infinite") ) {
//	        s_ivars.fForce = false;
	        s_ivars.wmode = wmodeANALYZE;
	        VSendToEngine("analyze");
            return true;
        }
        // go ponder wtime 108172 btime 118025 winc 6000 binc 6000
        if( !my_stricmp(rgsz[nextArg], "ponder") ) {
            ponder = true;
        } else if( !my_stricmp(rgsz[nextArg], "wtime") ) {
            if( nextArg >= csz )
                VErrorWb("not enought parameter", "wtime");
            else {
                if(s_ivars.coOnMove == coWHITE )
                    s_ivars.tmMyTime = atol(rgsz[nextArg+1]);
                else
                    s_ivars.tmTheirTime =  atol(rgsz[nextArg+1]);
                nextArg++;
            }
        } else if( !my_stricmp(rgsz[nextArg], "btime") ) {
            if( nextArg >= csz )
                VErrorWb("not enought parameter", "btime");
            else {
                if(s_ivars.coOnMove == coBLACK )
                    s_ivars.tmMyTime = atol(rgsz[nextArg+1]);
                else
                    s_ivars.tmTheirTime =  atol(rgsz[nextArg+1]);
                nextArg++;
            }
        } else if( !my_stricmp(rgsz[nextArg], "winc") ) {
            if( nextArg >= csz )
                VErrorWb("not enought parameter", "winc");
            else {
                // note that i don't have a time control type per side
                if(s_ivars.coOnMove == coWHITE )
                    tmIncr = atol(rgsz[nextArg+1]);
                //VSendToEngine("level 0 0 %ld", tmIncr);
                nextArg++;
            }
        } else if( !my_stricmp(rgsz[nextArg], "binc") ) {
            if( nextArg >= csz )
                VErrorWb("not enought parameter", "binc");
            else {
                // note that i don't have a time control type per side
                if(s_ivars.coOnMove == coBLACK )
                    tmIncr = atol(rgsz[nextArg+1]);
                //VSendToEngine("level 0 0 %ld", tmIncr);
                nextArg++;
            }
        } else if( !my_stricmp(rgsz[nextArg], "depth") ) {
            if( nextArg >= csz )
                VErrorWb("not enought parameter", "depth");
            else {
                plyDepth = atoi(rgsz[nextArg+1]);
                nextArg++;
            }
        } else if( !my_stricmp(rgsz[nextArg], "nodes") ) {
            if( nextArg >= csz )
                VErrorWb("not enought parameter", "nodes");
            else {
                nodes = atoi(rgsz[nextArg+1]);
                nextArg++;
            }
        } else if( !my_stricmp(rgsz[nextArg], "movestogo") ) {
            if( nextArg >= csz )
                VErrorWb("not enought parameter", "movestogo");
            else {
                // note that i don't have a time control type per side
                cmoves = atol(rgsz[nextArg+1]);
                nextArg++;
            }
        } else {
            VErrorWb("'go' parameter not recognized", rgsz[nextArg]);
            return true;
        }
        nextArg++;
    }
    if( plyDepth || nodes ) {
	    s_ivars.tmMyTime =  tmIncr = 999*1000;
    }
    VSendToEngine("level %d %d %ld", cmoves, s_ivars.tmMyTime, tmIncr);
//    if( plyDepth ) {
	    VSendToEngine("sd %d", plyDepth);
//    }
//    if( nodes ) {
	    VSendToEngine("sn %d", nodes);
//    }
    if( ponder ) {
            // set analyze mode for ponder !!
	        s_ivars.wmode = wmodeANALYZE;
	        VSendToEngine("analyze");
    } else {
	    s_ivars.wmode = wmodeTHINK;
	    VSendToEngine("think %ld %ld", s_ivars.tmMyTime, s_ivars.tmTheirTime);
    }
	return true;
}

static bool FCmdUciStop(char * szCmd, char * rgsz[], int csz) {
	VSendToEngine("abort");
    return true;
}

static bool FCmdUciPonderHit(char * szCmd, char * rgsz[], int csz) {
//    s_ivars.wmode = wmodeTHINK;
	VSendToEngine("think %ld %ld XXXX",
		s_ivars.tmMyTime, s_ivars.tmTheirTime);
    return true;
}

static bool FCmdEval(char * szCmd, char * rgsz[], int csz) {
	VSendToEngine("eval");
    return true;
}

//-------------------------------------------------------------------------

typedef	struct	tagCMD {
	const char * sz;
	bool	fInitFirst;
	int	cszMin;				// I need at least this many params.
	int	cszMax;				// I need at most this many params (-1 for any).
	bool (* pfn)(char * szCmd, char * rgsz[], int csz);
}	CMD, * PCMD;

static CMD const c_argcmd[] = {
	"protover",		false,	2,	2,		FCmdProtover,
	"xboard",		false,	1,	1,		FCmdXboard,
	"accepted",		true,	0,	256,	NULL,
	"rejected",		true,	0,	256,	NULL,
	"new",			true,	1,	1,		FCmdNew,
	"variant",		true,	0,	256,	NULL,
	"quit",			true,	1,	1,		FCmdQuit,
	"random",		true,	0,	256,	NULL,
	"force",		true,	1,	1,		FCmdForce,
	"playother",	true,	0,	256,	FCmdUnimplemented,
	"white",		true,	1,	1,		FCmdWhite,				// Protover 1.
	"black",		true,	1,	1,		FCmdBlack,				// Protover 1.
	"level",		true,	4,	4,		FCmdLevel,
	"st",			true,	2,	2,		FCmdSt,
	"sd",			true,	2,	2,		FCmdSd,
	"time",			true,	2,	2,		FCmdTime,
	"otim",			true,	0,	256,	FCmdOtim,
	"usermove",		true,	0,	256,	FCmdUnimplemented,
	"?",			true,	1,	1,		FCmdHook,
	".",			true,	1,	1,		FCmdDot,				// Protover 1.
	"c",			true,	1,	1,		FCmdC,					// Protover 1.
	"#",			true,	0,	256,	NULL,					// Protover 1.
	"ping",			true,	2,	2,		FCmdPing,
	"draw",			true,	1,	1,		FCmdDraw,
	"result",		true,	0,	256,	FCmdResult,
	"setboard",		true,	7,	7,		FCmdSetboard,
	"edit",			true,	1,	1,		FCmdEdit,				// Protover 1.
	"hint",			true,	1,	1,		FCmdHint,
	"bk",			true,	1,	1,		FCmdBk,
	"undo",			true,	1,	1,		FCmdUndo,
	"remove",		true,	1,	1,		FCmdRemove,
	"hard",			true,	1,	1,		FCmdHard,
	"easy",			true,	1,	1,		FCmdEasy,
	"post",			true,	1,	1,		FCmdPost,
	"nopost",		true,	1,	1,		FCmdNopost,
	"analyze",		true,	1,	1,		FCmdAnalyze,
	"exit",			true,	1,	1,		FCmdExit,
	"name",			true,	2,	256,	FCmdName,
	"rating",		true,	3,	3,		FCmdRating,
	"computer",		true,	1,	1,		FCmdComputer,
	"pause",		true,	0,	256,	FCmdUnimplemented,
	"resume",		true,	0,	256,	FCmdUnimplemented,
	"ics",			true,	2,	2,		FCmdIcs,
    "perft",        true,   1,  2,      FCmdPerft,
    "showbook",     true,   1,  1,      FCmdShowBook,

    "uci",          true,   1,  1,      FCmdUci,
    "debug",        true,   1,  2,      FCmdUciDebug,
    "isready",      true,   1,  1,      FCmdUciIsReady,
    "setoption",    true,   4,  99,     FCmdUciSetOption,
    "register",     true,   1,  1,      FCmdUciRegister,
    "ucinewgame",   true,   1,  1,      FCmdUciNewGame,
    "position",     true,   1,  9999,   FCmdUciPosition,
    "go",           true,   1,  256,    FCmdUciGo,
    "stop",         true,   1,  1,      FCmdUciStop,
    "ponderhit",    true,   1,  1,      FCmdUciPonderHit,

    "sn",           true,   1,  1,      FCmdSnodes,
    "nps",          true,   1,  1,      FCmdNps,

    "eval",         true,   1,  1,      FCmdEval,

	NULL,0,0,0,0
};

//-------------------------------------------------------------------------

void VExecuteMove(char * rgsz[], int csz)
{
	Assert(csz >= 1);
	s_ivars.coOnMove ^= 1;						// Protover 1.
	if (s_ivars.fForce)
		VSendToEngine("move %s", rgsz[0]);
	else
		switch (s_ivars.wmode) {
		case wmodeANALYZE:
			VSendToEngine("analyze %s", rgsz[0]);
			break;
		case wmodeTHINK:
			VSendToEngine("think %ld %ld %s",
				s_ivars.tmMyTime, s_ivars.tmTheirTime, rgsz[0]);
			break;
		}
}

//-------------------------------------------------------------------------

//	This is the input thread.  The program will spin in here until it gets a
//	command that tells it to exit.

//	The engine is supposed to be initialized in response to the "protover"
//	command from Winboard.  If the user starts Gerbil from the command line,
//	or if an old version of Winboard is used, I am not going to get the
//	"protover" command.  So what I do is check for a line from the user that
//	is going to make me do something (a move, or a command other than the
//	"protover" command), and initialize at that point if I haven't already.

//	Note that the "xboard" command is not hooked up, or that would cause the
//	engine to initialize.  If you hook this command up, you have some changes
//	to make.

void VReadFromWinboard(void)
{
	for (;;) {
		char	aszBuf[4096];
		char	aszVec[4096];
		char *	argsz[512];
		int	csz;

		if (fgets(aszBuf, sizeof(aszBuf), stdin) != aszBuf)
			break;
        //VSendToWinboard("# got(%s)", aszBuf);
		VStripWb(aszBuf);
		strcpy(aszVec, aszBuf);
		csz = CszVectorizeWb(aszVec, argsz);
		if (csz != 0)
			if ((isdigit(argsz[0][1])) && (isdigit(argsz[0][3]))) {
				VExecuteMove(argsz, csz);
			} else {
				CMD const * pcmd;

				for (pcmd = c_argcmd; pcmd->sz != NULL; pcmd++) {
					if (strcmp(pcmd->sz, argsz[0]))
						continue;
					if (pcmd->pfn == NULL)
						break;
					//
					//	Check for some dummy cases.  These might really happen
					//	if the user starts the program from the command-line.
					//
					if (csz < pcmd->cszMin) {
						VErrorWb("too few parameters", aszBuf);
						break;
					}
					if (csz > pcmd->cszMax) {
						VErrorWb("too many parameters", aszBuf);
						break;
					}
//                    if( pcmd->pfn == FCmdUciGo )
//                        VSendToWinboard("# process (%s)\n", aszBuf);
					if (!(*pcmd->pfn)(aszBuf, argsz, csz))
						return;
					break;
				}
				if ((pcmd->sz == NULL) &&
					((!s_ivars.fEditing) || (!FEditCmd(argsz, csz))))
					VErrorWb("Unknown command", argsz[0]);
			}
	}
}

//-------------------------------------------------------------------------

int
#ifdef _MSC_VER
__cdecl
#endif
main(int argc, char * argv[])
{
 /*   extern int goGame();
    goGame();
    return 1;*/
	s_ivars.wmode = wmodeTHINK;
	s_ivars.fEditing = false;
	s_ivars.fForce = false;
	s_ivars.tmMyTime = 2000;	// If nobody tells me differently, I am going
								//  to think for 2 seconds per move.
	s_ivars.tmTheirTime = 2000;
    s_ivars.tmIncr = 0;
	s_ivars.iProtover = WB1;
    setbuf(stdout, NULL);
    setbuf(stdin, NULL);
	VPrBanner();
	if (!FInitEngineThread(argc, argv))
		return 1;
	VReadFromWinboard();
    if( dbgout )
        fclose( dbgout );
	return 1;
}

//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-
