
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
#include "engine.hpp"
#include "gproto.hpp"
#include "winboard.hpp"
#include "stats.hpp"
#include "board.hpp"
#include "uci.hpp"

static char const s_aszModule[] = __FILE__;

//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-

void VPrCallback(PCON pv)
{
	VProcessEngineCommand(pv, true);
}

//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-

//	Here's a move the engine wants to make.  The format for sending moves is
//	different depending upon whether I am using the old protocol or the new
//	one.

//	The "..." is a silly obsolete Winboard behavior.

void VPrSendMove(const char * szMove, const char *szPonder)
{
    if( szMove[0] == 0 )
        return;
    switch(s_ivars.iProtover) {
        case WB1:
            VSendToWinboard("%s %s", "1. ...", szMove);
            s_ivars.coOnMove ^= 1;					// Protover 1.
            break;
        case WB2:
            VSendToWinboard("move %s", szMove);
            break;
        case UCI:
            if( szPonder && szPonder[0] )
                VSendToWinboard("bestmove %s ponder %s", szMove, szPonder);
            else
                VSendToWinboard("bestmove %s", szMove);
            break;
    }
}

//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-

//	This is actually the move being pondered on, not a response to the
//	"hint" command that Winboard might send.

void VPrSendHint(const char * szMove)
{
	VSendToWinboard("Hint: %s", szMove);
}

//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-

void VPrSendAnalysis(int depth, int seldepth, int val, unsigned long tm,
	U64 nodes, int prsa, char * szLine) {

    if( s_ivars.iProtover == UCI ) {
        if( val <= -valMATEscore || val >= valMATEscore ) {
            int m;
            if(val > 0)
                m = (valMATE - val + 1) / 2;
            else
                m = -(valMATE + val) / 2;
	        VSendToWinboard("info depth %d seldepth %d score mate %d time %lu nodes " U64_FORMAT " pv %s", 
                depth, seldepth, m, tm / 10, nodes, szLine);
        } else
	        VSendToWinboard("info depth %d seldepth %d score cp %d %s time %lu nodes " U64_FORMAT " pv %s", 
            depth, seldepth, val, (prsa == prsaFAIL_HIGH) ? "lowerbound" : "", tm / 10, nodes, szLine);
    } else {
	    VSendToWinboard("%d %d %lu " U64_FORMAT " %s", depth, val, tm / 10, nodes, szLine);
	    switch (prsa) {
	    case prsaFAIL_HIGH:
		    VSendToWinboard("++");
		    break;
	    case prsaFAIL_LOW:
		    VSendToWinboard("--");
		    break;
    	}
    }
}

//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-

void VPrSendResult(const char * szResult, const char * szWhy)
{
	if (szWhy != NULL)
		VSendToWinboard("%s {%s}", szResult, szWhy);
	else
		VSendToWinboard("%s", szResult);
}

//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-

//	This sends a line to the interface, preceded by "# ".  This is just a
//	personal choice.  The interface may not demand this.
//
//	I'm going to send anything I think the interface won't really care about
//	with this function.

void VPrSendComment(const char * szFmt, ...)
{
	char	aszBuf[256];
	va_list	pvaArg;

	va_start(pvaArg, szFmt);
	vsprintf(aszBuf, szFmt, pvaArg);
	VSendToWinboard("# %s", aszBuf);
}

//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-

void VPrSendBookLine(const char * szLine, ...)
{
	if (szLine[0]) {
		char	aszBuf[256];
		va_list	pvaArg;

		va_start(pvaArg, szLine);
		vsprintf(aszBuf, szLine, pvaArg);
		VSendToWinboard(" %s", aszBuf);
	} else
		VSendToWinboard("");
}

//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-

void VPrOfferDraw(void)
{
	VSendToWinboard("offer draw");
}

//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-
void VPrSendUciCurrentLine(const CON *pcon, const STE *pste) {
    static TM nextTime = 0;
    TM	tmNow = TmNow();
    if( s_ivars.iProtover == UCI )
        if( tmNow > nextTime && uciShowCurrLine.get_check() ) {
            nextTime = tmNow + 1000;
            char asz[1024];
            char * sz = asz;
            sz[0] = 0;
            int cpt = 0;
            for (const STE *p = pcon->argste ; p < pste && cpt++ < 30; p++) {
                CM move(p->lastMove, 0);
                CmToSz( &move, sz );
                size_t l = strlen(sz);
                if( ((sz + l) - asz) >= sizeof(asz) - 1 )
                    break;
                sz += l;
                *sz++ = ' ';
                *sz = 0;
            }
            VSendToWinboard("info currline %s", asz);
        }
}
void VPrSendUciDepth(const SEARCH_STATUS &ss) {
    if( s_ivars.iProtover == UCI )
        VSendToWinboard("info depth %d", ss.plyDepth );
}

void VPrSendUciNodeCount(bool force, const SEARCH_STATUS &ss) {
    static TM nextTime = 0;
    TM	tmNow = TmNow();
    if( s_ivars.iProtover == UCI )
        if( tmNow > nextTime || force) {
            TM timeUsed = tmNow - ss.tmStart;
            int nps = (timeUsed > 100) ? int((1000.0f*ss.nodes) / timeUsed) : 1;
            nextTime = tmNow + 1000;
            VSendToWinboard("info time %d nodes " U64_FORMAT " nps %d hashfull %d tbhits %d", 
                timeUsed, ss.nodes, nps, int((1000.0f*stats.Hused) / stats.Hsize), ss.tbhits
            );
            // tbhits
            // cpuload 1000
        }
}

void VPrSendUciCurrMove(const SEARCH_STATUS &ss) {
    if( s_ivars.iProtover == UCI ) {
        if( (TmNow() - ss.tmStart) > 1500 )
            VSendToWinboard("info currmove %s currmovenumber %d", ss.aszSearchSan, ss.icmSearching );
    }
}
