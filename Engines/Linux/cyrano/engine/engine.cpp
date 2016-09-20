//
// Cyrano Chess engine
//
// Copyright (C) 2007,2008  Harald JOHNSEN
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License as
// published by the Free Software Foundation; either version 2 of the
// License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA
//
//

#include "engine.hpp"
#include "gproto.hpp"
#include "hash.hpp"
#include "stats.hpp"
#include "board.hpp"
#include "moves.hpp"
#include "search.hpp"
#include "genmagic.hpp"
#include "moves.hpp"
#include "newbook.hpp"
#include "eval.hpp"
#include "uci.hpp"
#include "util.hpp"

#include <ctype.h>

#ifdef	_DEBUG
static char const s_aszModule[] = __FILE__;
#endif

// --------------------------------------------------------------------------

//  If this is called while a search is running, it won't change the time
//  control that's being used for this move.

void VSetTimeControl(PCON pcon, int cMoves, TM tmBase, TM tmIncr)
{
    switch (cMoves) {
    case 0:
        pcon->tc.tct = tctINCREMENT;
        break;
    case 1:
        pcon->tc.tct = tctFIXED_TIME;
        break;
    default:
        pcon->tc.tct = tctTOURNEY;
        break;
    }
    pcon->tc.cMoves = cMoves;
    pcon->tc.tmBase = tmBase;
    pcon->tc.tmIncr = tmIncr;
    VPrSendComment(" setting time control with cmodes=%d tmbase=%d tmincr=%d",
        cMoves, tmBase, tmIncr);
}

// --------------------------------------------------------------------------

//  See the caveat regarding "VSetTimeControl".

void VSetFixedDepthTimeControl(PCON pcon, int plyDepth)
{
    pcon->tc.plyDepth = plyDepth;
}

void VSetFixedNodeCount(PCON pcon, U64 nodeCount)
{
    pcon->tc.nodes = nodeCount;
}

// --------------------------------------------------------------------------

//  This is called at the start of a search that's expected to output a move
//  at some point.
//
//  "tmStart" is *not* set by this routine.
//
//  times are in milliseconds

void VSetTime(PCON pcon, bool wasPondering)
{
    Assert(sizeof(long) == sizeof(TM));		// If this fires, increment
											//  time control is broken.
//    TM	tmExtra;
#if 1
    int tmBase, tmLimit;
    int myTime = pcon->ss.tmUs - 500;   // lag margin
    if( myTime < 0 )
        myTime = 0;

    switch (pcon->tc.tct) {

    case tctINCREMENT:
        if (pcon->tc.tmIncr) {
            // is uci including the increment like wb ?
            tmBase = (myTime - pcon->tc.tmIncr) / 30 + pcon->tc.tmIncr;
            tmLimit = myTime / 3;
        } else {
            tmBase = myTime / 30;   // /35 is way too fast
            tmLimit = myTime / 3;
        }
lblSet:	;
        {
            int maxTime = myTime / 2;
            // allows more time on ponder hit
            if( wasPondering )
                tmBase += tmBase / 5;
            if (tmBase > maxTime)
                tmBase = maxTime;
            if (tmLimit > maxTime)
                tmLimit = maxTime;
            pcon->ss.tmEnd = pcon->ss.tmStart + tmBase;
            pcon->ss.tmEndLimit = pcon->ss.tmStart + tmLimit;

            VPrSendComment(" setting time tmBase=%d tmLimit=%d, tmUs=%d from=%s", 
                tmBase, tmLimit, pcon->ss.tmUs, wasPondering ? "ponder" : "nowhere");
        }
        break;
    case tctFIXED_TIME:
        // myTime ? tmBase ?
        // this is also the time for the last move in n/m
        tmBase  = pcon->tc.tmBase * 1 / 2;
        tmLimit = pcon->tc.tmBase * 9 / 10;
        // keep a margin for the lag
        if( tmLimit > 800 )
            tmLimit -= 200;
        else if( tmLimit > 200 )
            tmLimit -= 100;
        else
            tmLimit /= 2;

        pcon->ss.tmEnd = pcon->ss.tmStart + tmBase;
        pcon->ss.tmEndLimit = pcon->ss.tmStart + tmLimit;
        break;
    case tctTOURNEY:
    {
        int	movLeft;

        if(pcon->isUCI ) {
            movLeft = pcon->tc.cMoves;
        } else {
            int movMade = pcon->gc.ccm / 2 + pcon->gc.movStart - 1;
            while (movMade >= pcon->tc.cMoves)
                movMade -= pcon->tc.cMoves;
            movLeft = pcon->tc.cMoves - movMade;
            Assert(movLeft >= 1);
        }
//        tmBase = (4 * pcon->ss.tmUs) / (3 * (movLeft + 1));
        if( movLeft > 30 )
            movLeft = 30;
        tmBase = myTime / movLeft;
        tmLimit = myTime / 3;
        goto lblSet;
    }
    }
#else
    TM	tmBase, tmLimit;
    switch (pcon->tc.tct) {
    case tctINCREMENT:
        if (pcon->tc.tmIncr) {

            //	An increment time control tries not to go below the increment
            //	plus 30 seconds, or 6 times the increment, whichever is more.
            //
            //	If it finds itself below that, it will the increment minus
            //	1/10 of the time by which it is under the target.
            //
            //	If it is over that, it will use the increment plus 1/20 of
            //	the time by which it is over the target.
            //
            //	So the program will drift slowly down to the target, but if
            //	it gets a ways below it, it will tend to come up quickly.
            //
            //	There are a zillion different increment time controls, and
            //	sometimes I could end up with a stupid base time.  I try hard
            //	to make sure the base time doesn't go below zero, which would
            //	be very bad since my time quantity is an unsigned value.
            //
            //	I will try hard to use >= half of the increment.  I will also
            //	make sure to not use more than half the remaining time (which
            //	is taken care of by the code at "lblSet".
            //
            // base = tmus/30 + incr, sliding time for 30 moves
            tmBase = (pcon->ss.tmUs - pcon->tc.tmIncr) / 30 + pcon->tc.tmIncr;
        } else {
            //
            //	A zero-increment time control will use 1/30 of the remaining
            //	time down to 10 minutes, 1/40 down to one minute, and 1/60
            //	after that.
            //
            tmBase = pcon->ss.tmUs / 30;
        }
        //	This code dummy-checks "tmBase", assigns "tmExtra", then assigns
        //	an end-time based upon "pcon->ss.tmStart".
        //
        //	First I check to see if I'm scheduled to eat more than half of
        //	my remaining time.  I put a ceiling at that amount, right off.
        //
        //	Next, I'll assign some emergency time.  This is 3x the base time,
        //	with a ceiling on base + extra of 1/2 the remaining time.  I will
        //	also allocate no emergency time if I have < 30 seconds on the
        //	clock.
        //
        //	So there will be plenty of "extra" time if there is time left,
        //	otherwise there is little or none.
        //
lblSet:	;
        {
            TM maxTime = pcon->ss.tmUs / 2;
            // allows more time on ponder hit
            if( wasPondering )
                tmBase += tmBase / 3;
            if (tmBase > maxTime)
                tmBase = maxTime;
//            tmExtra = tmBase * 3;
            tmExtra = tmBase * 5;
            if (tmBase + tmExtra > maxTime)
                tmExtra = maxTime - tmBase;
            pcon->ss.tmEnd = pcon->ss.tmStart + tmBase;
            pcon->ss.tmExtra = tmExtra;
            VPrSendComment(" setting time tmBase=%d tmExtra=%d, tmUs=%d from=%s", 
                tmBase, pcon->ss.tmExtra, pcon->ss.tmUs, wasPondering ? "ponder" : "nowhere");
        }
        break;
    case tctFIXED_TIME:
        // this is also the time for the last move in n/m
        tmBase = pcon->tc.tmBase;		// Use *all* the time.
        // keep a margin for the lag
        if( tmBase > 800 )
            tmBase -= 200;
        else if( tmBase > 200 )
            tmBase -= 100;
        else
            tmBase /= 2;
        pcon->ss.tmEnd = pcon->ss.tmStart + tmBase;
        pcon->ss.tmExtra = 0;			// No emergency time.
        VPrSendComment(" setting time tmBase=%d tmExtra=%d, tmUs=%d", tmBase, pcon->ss.tmExtra, pcon->ss.tmUs);
        break;
    case tctTOURNEY:
    {
        int	movLeft;

        if(pcon->isUCI ) {
            movLeft = pcon->tc.cMoves;
        } else {
            int movMade = pcon->gc.ccm / 2 + pcon->gc.movStart - 1;
            while (movMade >= pcon->tc.cMoves)
            movMade -= pcon->tc.cMoves;
            movLeft = pcon->tc.cMoves - movMade;
            Assert(movLeft >= 1);
        }
        //
        //	This expression is kind of nasty.  It is:
        //
        //		Time / (3/4 x Moves + 3)
        //
        //	The point of the "3" is to make sure there's some extra time pad
        //	at the beginning, and the point of the 3/4 is to make sure that
        //	earlier moves take a little longer per move.
        //
        //	This expression has been reduced to ...
        //
        //		(4 * Time) / (3 * Moves + 12)
        //
        //	.. by multiplying the top and the bottom by 4.  This helps out
        //	with the integer math.
        //
        tmBase = (4 * pcon->ss.tmUs) / (3 * (movLeft + 1));
        goto lblSet;
    }
    }
#endif
}

// --------------------------------------------------------------------------

//	This is called a whole bunch of times via the engine/interface protocol.
//	Given what is known about the time control, the current time, and the
//	search depth, this may set "ptcon->fTimeout", which will eventually finish
//	the search.

//	This routine has a built-in defense against cases where someone might try
//	to force a move before the first ply has been fully considered.  It will
//	simply ignore such cases.

void VCheckTime(PCON pcon)
{
    TM	tmNow;

    if (pcon->ss.plyDepth == 1)	// Can't time out before a one-ply
        return;					//  search is finished.
    tmNow = TmNow();
    if( pcon->smode == smodeTHINK || pcon->smode == smodeANALYZE ) 
        VPrSendUciNodeCount(false, pcon->ss );
    // no time check in ponder or analyze mode
    if (pcon->smode != smodeTHINK )
        return;
    // absolute time limit
    if (tmNow >= pcon->ss.tmEndLimit) {
        VPrSendComment("time limit reached ...\n");
        pcon->fTimeout = true;
    }
    // 'normal' time limit
    else if (tmNow >= pcon->ss.tmEnd ) {
        if( pcon->ss.canStop )
            pcon->fTimeout = true;
//        else
//            VPrSendComment("time reached but can not stop : %d %d\n", pcon->ss.dontStop, pcon->ss.dontStopPrev);
    }
#if 0
    else if (tmNow >= pcon->ss.tmEnd) {
        //
        //	Time has expired.  Check for a fail-high or fail low and try to
        //	add some time. 
        //
        if ( pcon->ss.tmExtend || pcon->ss.failed ) {
            pcon->ss.tmExtend = false;
/*            if (pcon->ss.tmExtra < 1000) {
                VPrSendComment("adding all extra time : %d %d\n", pcon->ss.tmExtra, pcon->ss.failed);
                pcon->ss.tmEnd += pcon->ss.tmExtra;
                pcon->ss.tmExtra = 0;
            } else*/ {
                unsigned int moreTime = pcon->ss.tmExtra / 4;
                VPrSendComment("adding some more time : %d %d\n", moreTime, pcon->ss.failed);
                pcon->ss.tmEnd += moreTime;
                pcon->ss.tmExtra -= moreTime;
            }
        }
        if (tmNow >= pcon->ss.tmEnd)
            pcon->fTimeout = true;
    }
#endif
    else if( pcon->tc.nodes && pcon->ss.nodes > pcon->tc.nodes ) {
        pcon->fTimeout = true;
    }
}

// --------------------------------------------------------------------------

//	This spits a PV out to the interface.
//
//	WARNING: This function must have the root moves already generated.

void VDumpPv(PCON pcon, int ply, int val, int prsa)
{
	char asz[1024];
	char * sz;
	int	i;
    bool inQS = false;

    // check if the root move has changed
    // this is also set in the root search in case of fail high but only under some condition
    if( (pcon->ss.plyDepth >= 2) && (pcon->ss.currBestMove.m !=  pcon->argste[0].argcmPv[0].m) ) {
        pcon->ss.rootHasChanged = true;
        pcon->ss.easyMove = false;
    }
    // old code : add some more time when the score drops
    // new code : set the 'can not stop' flag

// 35,20 => 30,20 => 25,15 => 30,15

    // the score dropped a lot, don't stop now
//    if( (pcon->ss.plyDepth >= 2) && (pcon->ss.lastVal - val >= 35) ) {
    if( (pcon->ss.plyDepth >= 2) && (pcon->ss.lastVal - val >= 30) ) {
        pcon->ss.scoreDropped = true;
        pcon->ss.easyMove = false;
        //VPrSendComment(" big score drop = %d\n", -(pcon->ss.lastVal - val));
        pcon->ss.canStop = false;
    } else {
        // reset because we can find a better move in the same iteration
        pcon->ss.scoreDropped = false;

        // the score dropped a bit, don't stop now
        if( (pcon->ss.plyDepth >= 2) && (pcon->ss.lastVal - val >= 15) ) {
            // but only if only used half the time ; without that time condition and
            // a small delta score the search can last too long (usually for nothing)
            int usedTime = TmNow() - pcon->ss.tmStart;
            int remaingingTime = pcon->ss.tmEnd  - TmNow();
            if( usedTime < remaingingTime ) {
                pcon->ss.scoreDropped = true;
                pcon->ss.easyMove = false;
                //VPrSendComment(" small score drop = %d\n", -(pcon->ss.lastVal - val));
                pcon->ss.canStop = false;
            }
        }
    }

    //pcon->ss.lastVal = val;

/*
    // if we get a worse score, set the fail_low flag, this will allocate
    // more time if the normal time expires
    if( pcon->ss.lastVal - val >= 20 ) {
		pcon->ss.tmExtend = true;
        if( (pcon->ss.plyDepth > 4) && (pcon->ss.lastVal - val >= 30) ) {
            pcon->ss.failed = true;
            // add time now
            unsigned int moreTime = (1*pcon->ss.tmExtra) / 4;
            if( pcon->ss.lastVal - val >= 50 )
                moreTime = (2*pcon->ss.tmExtra) / 4;
            else if( pcon->ss.lastVal - val >= 99 )
                moreTime = (4*pcon->ss.tmExtra) / 4;
            VPrSendComment("adding some time(bad score) : %d delta=%d\n", moreTime, pcon->ss.lastVal - val);
            pcon->ss.tmEnd += moreTime;
            pcon->ss.tmExtra -= moreTime;
        }
    }
    else
        pcon->ss.tmExtend = false;
    if( (pcon->ss.plyDepth > 6) && (pcon->ss.currBestMove.m !=  pcon->argste[0].argcmPv[0].m) ) {
        unsigned int moreTime = (1*pcon->ss.tmExtra) / 11;
        pcon->ss.tmEnd += moreTime;
        pcon->ss.tmExtra -= moreTime;
    }
    if( (!pcon->ss.failed) && (val >= pcon->ss.lastVal) || (pcon->ss.plyDepth <= 3))
        pcon->ss.lastVal = val;
    else
        pcon->ss.lastVal = (val + pcon->ss.lastVal) / 2;
*/
    pcon->ss.currBestMove = pcon->argste[0].argcmPv[0];
	pcon->ss.val = val;	// Remember the value of this search so far.
	pcon->ss.prsa = prsa;
	if (!pcon->fPost && !pcon->isUCI)	// The interface doesn't want PV's.
		return;
	sz = asz;
//    printf("# ");
	//
	//	In order to do the SAN thing, I need to actually execute the moves
	//	on the board, so I call "VMakeMove" at the end of this.
	//
	//	I assume the moves in the PV are legal (it can't be otherwise).
	//
    int oldPlyrem = pcon->argste[0].plyRem;

	for (i = 0; i < pcon->argste[0].ccmPv; i++) {
		SAN	san;
		PCM	pcmPv;
		PSTE	pste;

		pste = pcon->argste + i;
		pcmPv = &pcon->argste[0].argcmPv[i];
        if( pcmPv->cmk & cmkQS ) {
            inQS = true;
//			*sz++ = ' ';
//			*sz++ = '/';
//			*sz++ = ' ';
        }

		if (i)
			bGenMoves(pcon, pste);
        if( pcon->isUCI )
            CmToSz( pcmPv, sz );
        else {
		    CmToSan(pcon, pste, pcmPv, &san);
		    SanToSz(&san, sz);
        }
        //printf("%s", sz);
		sz += strlen(sz);
		if (i + 1 < pcon->argste[0].ccmPv)
			*sz++ = ' ';

        {
            bMove(pcon, pste, pcmPv->m);
            PHASH nextEntry = ProbeHashValue(pste+1);
/*            if( nextEntry ) {
                printf("%c %d  ", (nextEntry->getExt() ? '!' : ' '), nextEntry->count );
            }
            else
                printf(" . ");*/
            bUndoMove(pcon, pste, pcmPv->m);
            {
/*                foreachmove(pste,pcm) {
                    bMove(pcon, pste, pcm->m);
                    PHASH nextEntry = ProbeHashValue(pste+1);
                    if( (pcm->m != pcmPv->m) && nextEntry && nextEntry->getForced()) {
                        char sz2[16];
                        CmToSz( pcm, sz2 );
                        printf(" (%s%c %d) ", sz2, (nextEntry->getExt() ? '!' : ' '), nextEntry->count);
                    }
                    bUndoMove(pcon, pste, pcm->m); 
                }*/
            }
        }
        // refresh hash table entries
        PHASH thisEntry = ProbeHashValue(pste);
        int plyrem = pcon->argste[0].ccmPv - i - 1;
        //(pste)->plyRem = plyrem;
        (pste)->plyRem = HASH::pvDepth;     // plyrem can not be altered to include qs depth
        if( !inQS ) 
        {
            if( thisEntry ) {
                //thisEntry->setMove( pcmPv->m );
                //thisEntry->setPlyRem( HASH::pvDepth );
                //thisEntry->setBound( hashfEXACT );
                thisEntry->touch();
            } else {
                int newVal = val;
                if( i & 1 )
                    newVal = -newVal;
                RecordHashValue(pcon, pste, *pcmPv, 0/*newVal*/, valMIN, valMAX, i+1, false);
            }
        }

		bMove(pcon, pste, pcmPv->m);
	}
	//	Undo all of the make-move's.
	//
    printf("\n");
	for (; --i >= 0;)
		bUndoMove(pcon, pcon->argste + i, pcon->argste[0].argcmPv[i].m);
    pcon->argste[0].plyRem = oldPlyrem;
	*sz++ = '\0';
	VPrSendAnalysis(ply, pcon->ss.plyMaxDepth, val, TmNow() - pcon->ss.tmStart,
		pcon->ss.nodes, prsa, asz);
}


void VDumpLine(const CON *pcon, const STE *pste, const char *comment)
{
	char asz[1024];
	char * sz;

	sz = asz;
    sz[0] = 0;
	//
	//	In order to do the SAN thing, I need to actually execute the moves
	//	on the board, so I call "VMakeMove" at the end of this.
	//
	//	I assume the moves in the PV are legal (it can't be otherwise).
	//
	for (const STE *p = pcon->argste ; p <= pste; p++) {
        CM move;
        move.m = p->lastMove;
        move.cmk = 0;
        if( move.m  == 0 ) {
            strcat(sz, " - ");
/*		    sz += strlen(sz);
            *sz++ = ' ';
            if( p == pste ) {
                move.m = (p+1)->lastMove;
                move.cmk = 0;
                CmToSz( &move, sz );
            }*/
        }
        else
            CmToSz( &move, sz );
		sz += strlen(sz);
        *sz++ = ' ';
//        unsigned int fh = (move.cmk >> 16) & 0xffff;
//        unsigned int fl = (move.cmk) & 0xffff;
//        sz += sprintf(sz, "(%d/%d) ", fh, fl);
        *sz = 0;
	}
    VPrSendComment("V %s %s", comment, asz);
    printf("info pv %s\n", asz);
}

static FILE *dumpFile = 0;
static bool dumpFileOpened = false;

void OutDump(const char *txt) {
    if( dumpFile == 0 && !dumpFileOpened ) {
        dumpFileOpened = true;
        dumpFile = fopen("/cyranodump.txt", "w");
    }
    if( dumpFile != 0 ) {
        fputs(txt, dumpFile);
    }
}

void VDumpSearch(PCON pcon, PSTE psteBase, int val, int Alpha, int Beta)
{
    char asz[1024];
    char * sz;
    int	i;

    sz = asz;
    //
    //	In order to do the SAN thing, I need to actually execute the moves
    //	on the board, so I call "VMakeMove" at the end of this.
    //
    //	I assume the moves in the PV are legal (it can't be otherwise).
    //

    int depth = int(psteBase - &pcon->argste[0]);
    sprintf(asz, "%2d  %4d [%5d,%5d] ", depth, val, Alpha, Beta);
    sz += strlen(asz);

    for( i = 0 ; i < depth ; i++ ) {
        *sz++ = ' ';
        *sz++ = ' ';
        *sz++ = ' ';
        *sz++ = ' ';
    }
	//*sz++ = '\0';

    for (i = 0; i < psteBase->ccmPv; i++) {
		SAN     san;
		PCM     pcmPv;
		PSTE    pste = psteBase + i;
		pcmPv = &psteBase->argcmPv[i];
		if (i)
			bGenMoves(pcon, pste);
	    CmToSan(pcon, pste, pcmPv, &san);
	    SanToSz(&san, sz);
		sz += strlen(sz);
		if (i + 1 < psteBase->ccmPv)
			*sz++ = ' ';

		bMove(pcon, pste, pcmPv->m);
	}
	//	Undo all of the make-move's.
	//
	for (; --i >= 0;)
		bUndoMove(pcon, psteBase + i, psteBase->argcmPv[i].m);
	*sz++ = '\n';
	*sz++ = '\0';
    printf( asz );
    OutDump( asz );
}



// --------------------------------------------------------------------------

//	This is called before a search in order to tell the search which moves
//	to try first when iterating through them.

void VSetPv(PCON pcon)
{
	int	i;
	
	for (i = 0; i < pcon->argste[0].ccmPv; i++)
		lastPV[i+1] = pcon->argste[0].argcmPv[i];
	for (; i < csteMAX - 1; i++) {
		lastPV[i+1].m = 0;
	}
}


// --------------------------------------------------------------------------

//	Once the first STE record is set up properly, this cleans up a few
//	details in the record, and sets up the rest of the array properly.
//	

void VFixSte(PCON pcon)
{
    PSTE	pste = &pcon->argste[0];

    pste->fNull = false;
    for (int i = 1; i < csteMAX; i++)
        pcon->argste[i].side = pste->side ^ (i & 1);
    pste->mlist_start = pcon->move_list;
}


// --------------------------------------------------------------------------

//	Execute "szMove" on the board, update game context ("pcon->gc"), then set
//	the other "pcon" fields up so it's ready to search the new position.

//	If it doesn't find the move, it returns FALSE.

//	Advancing a move is mostly moving "pcon->argste[1]" on top of
//	"pcon->argste[0]", after a call to "VMakeMove" has set up the new context.
//	This is a little nasty and dangerous, so it's a potential bug source.

//	If you modify "VMakeMove", take this potential problem into account.

//	Be especially careful if you mess with this routine, since bugs in it
//	won't show up in test suites -- they'll happen in games.

bool FAdvance(PCON pcon, char * szMove)
{
    const char *szCastle = "";
    PSTE    pste = pcon->argste;
//    Print(pcon, pste );
    // handle castle (coming from frc notation)
    if( !my_stricmp(szMove,"o-o") )
        if(pste->side == W )
            szCastle = "e1g1";
        else
            szCastle = "e8g8";
    if( !my_stricmp(szMove,"o-o-o") )
        if(pste->side == W )
            szCastle = "e1c1";
        else
            szCastle = "e8c8";
    bGenMoves(pcon, pste);
    foreachmove(pste,pcm) {
        char	asz[16];

        CmToSz(pcm, asz);
        if (!my_stricmp(asz, szMove) || !my_stricmp(asz, szCastle)) {
            bMove(pcon, pste, pcm->m);
            pste[0] = pste[1];
            VFixSte(pcon);
            --pcon->gc.ccm;     // HACK alert, is incremented in bMove now
            // add the move to the list of played move
            pcon->gc.argcm[pcon->gc.ccm] = *pcm;
            ++pcon->gc.ccm;
            pcon->gc.arghashk[pcon->gc.ccm] = pste->hashkPc;
            return true;
        }
    }
    VPrSendComment("we have a problem FAdvance failed for %s\n", szMove);
#if 0
    {
        foreachmove(pste,pcm) {
		    char	asz[16];
		    CmToSz(pcm, asz);
            VPrSendComment(" a move =%s\n", asz);
	    }
    }
#endif
	return false;
}

// --------------------------------------------------------------------------

//	This backs up one move in the current game, and deletes all trace of the
//	backed up move.

//	It works by going back to the beginning and going forward.

//	This routine is a little evil because it assumes that parts of the current
//	game context aren't blown out by "FInitCon".

void VUndoMove(PCON pcon)
{
    int	i;
    GAME_CONTEXT	gc;

    gc = pcon->gc;
    if (!gc.ccm)
        return;
    bool f = SetBoardFromFEN(pcon, gc.aszFen);
    Assert(f);
    for (i = 0, gc.ccm--; i < gc.ccm; i++) {
        char	asz[16];

        CmToSz(&gc.argcm[i], asz);
        f = FAdvance(pcon, asz);
        Assert(f);
    }
}

// --------------------------------------------------------------------------

//	This checks to see if the side to move at the root of "pcon" is
//	stalemated.

bool FStalemated(PCON pcon)
{
    PSTE    pste = pcon->argste;
    if (pste->checkf == 0 )
        return false;
    bGenMoves(pcon, pste);
    return legalMoveCount(pste) == 0;
}


// --------------------------------------------------------------------------

//	A simple routine that checks for draws based upon the game history, and by
//	that I mean repetition draws and 50-move draws.  It does nothing about
//	stalemate, insuffient material, etc.
//
//	This uses "full" hash keys to check for 3x repetition, in order to avoid
//	declaring a draw when a position has appeard three times, but with
//	different castling and/or en-passant possibilities.

void VDrawCheck(PCON pcon)
{
	PSTE pste = &pcon->argste[0];
	int	cReps;

	Assert(pcon->smode == smodeTHINK);
	if ((pste->valPnUs == 0) && (pste->valPnThem == 0)) {
		//
		//	If there are no pawns, claim a draw due to insufficient material
		//	if there are no pieces, one side has a bare king and the other
		//	has a minor, or if both sides have bishops (same color).
		//
		if ((pste->valPcUs == 0) && (pste->valPcThem == 0)) {
lblIns:		VPrSendResult("1/2-1/2", "Insufficient material");
			return;
		}
		if ((pste->valPcUs == valBISHOP || pste->valPcUs == valKNIGHT) && (pste->valPcThem == 0))
			goto lblIns;
		if ((pste->valPcUs == 0) && (pste->valPcThem == valBISHOP || pste->valPcThem == valKNIGHT))
			goto lblIns;
		if ((pste->valPcUs == valBISHOP) && (pste->valPcThem == valBISHOP)) {
#if 0
			int	argco[coMAX];
            int co;
			for (co = coBLACK; co <= coWHITE; co++) {
				int	ipi;
				int	rnk;
				int	fil;
				
				for (ipi = 0; ipi < pcon->argcpi[co]; ipi++)
					if ((!pcon->argpi[co][ipi].fDead) &&
						(pcon->argpi[co][ipi].pc == pcBISHOP))
						break;
				if (ipi == pcon->argcpi[co])
					break;
				rnk = RnkFromIsq(pcon->argpi[co][ipi].isq);
				fil = FilFromIsq(pcon->argpi[co][ipi].isq);
				argco[co] = !((rnk ^ fil) & 1);
			}
			if ((co > coWHITE) && (argco[coWHITE] == argco[coBLACK]))
				goto lblIns;
#endif
		}
	}
	if (pcon->argste[0].plyFifty >= 50 * 2) {
		VPrSendResult("1/2-1/2", "Draw by 50-move rule");
		return;
	}
	cReps = 1;
	for (int i = 0; i < pcon->gc.ccm; i++)
		if (pcon->gc.arghashk[i] == pcon->gc.arghashk[pcon->gc.ccm])
			if (++cReps >= 3) {
				VPrSendResult("1/2-1/2", "Draw by repetition");
				return;
			}
}

// --------------------------------------------------------------------------

static char const * c_argszUsage[] = {
	"Usage: %s [flags]",
	"  -?        | Usage",
    "  -b<B>     | {0 | 1} disable or enable the use of an opening book",
	"  -bf<F>    | name of the opening book to use",
	"  -hp<N>    | size of the pawn hash table in bytes",
	"  -ht<N>    | size of the transposition tables in bytes",
	"  -p        | Tell the engine to reduce its system priority.",
    "  -r<B>     | {0 | 1} disable of enable the auto resign feature",
	"  -t<F> <N> | Profile a position for N seconds, F is the FEN string",
	NULL,
};

static void VUsage(int argc, char * argv[])
{
	for (int i = 0; c_argszUsage[i] != NULL; i++)
		VPrSendComment(c_argszUsage[i], argv[0]);
}

// --------------------------------------------------------------------------

PCON PconInitEngine(int argc, char * argv[])
{
	static	CON s_con;
	PCON pcon = &s_con;
	char	aszBook[512];	// Opening book currently loaded.
	char	newBook[512];	// Opening book currently loaded.
	char	egbbdir[512];	// end game bitbases directory
	int	cbMaxHash;			// Number of transposition hash element bytes
							//  at most (actual number will be <= this).
	int	cbMaxPawnHash;		// Number of pawn hash bytes at most (actual
							//  number will be <= this).
	bool	f;
	char	aszProfileFen[256];
	TM	tmProfile = 1;

    cyranoOptions.init();

	VGetProfileSz("OldBookFile", "gerbil.opn", aszBook, sizeof(aszBook));
	VGetProfileSz("BookFile", "test.pbk", newBook, sizeof(newBook));
	VGetProfileSz("egbbdir", ".", egbbdir, sizeof(egbbdir));
    bool fUseBook = IGetProfileInt("UseBook", 1) ? true : false;
	cbMaxHash = IGetProfileInt("MaxHash", 32);
	cbMaxPawnHash = IGetProfileInt("MaxPawnHash", 1000000);

    pcon->fResign = IGetProfileInt("Resign", 1) ? true : false;
	pcon->fLowPriority = false;
	aszProfileFen[0] = '\0';
	//
	//	Process command line switches after setting appropriate defaults.
	//
	for (int i = 1; i < argc; i++) {
		char * sz = argv[i];

		switch (*sz++) {
		case '/':
		case '-':
			switch (*sz++) {
			case '?':
lblError:		VUsage(argc, argv);
				return NULL;
			case 'b':
				switch (*sz++) {
				case 'f':		// -bf[F] "BookFile"
					if (*sz != '\0')
						strcpy(newBook, sz);
					else if (i + 1 == argc)
						goto lblError;
					else if ((argv[++i][0] != '-') &&
						(argv[i][0] != '/'))
						strcpy(newBook, argv[i]);
					else
						goto lblError;
					break;
				case 's':		// -bs[F] "Bitbase"
					if (*sz != '\0')
						strcpy(egbbdir, sz);
					else if (i + 1 == argc)
						goto lblError;
					else if ((argv[++i][0] != '-') &&
						(argv[i][0] != '/'))
						strcpy(egbbdir, argv[i]);
					else
						goto lblError;
					break;
				default:		// -b[D] "UseBook"
					if (*--sz != '\0') {
						if (!isdigit(*sz))
							goto lblError;
                        fUseBook = atoi(sz) ? true : false;
					} else if (i + 1 == argc)
						goto lblError;
					else if (isdigit(argv[++i][0]))
                        fUseBook = atoi(argv[i]) ? true : false;
					else
						goto lblError;
					break;
				}
				break;
			case 'd':
				switch (*sz++) {
				case 'f':
					break;
				case 'c':
					break;
				default:
					goto lblError;
				}
				break;
			case 'h':
				switch (*sz++) {
				case 'p':		// -hp[D] "MaxPawnHash"
					if (*sz != '\0') {
						if (!isdigit(*sz))
							goto lblError;
						cbMaxPawnHash = atoi(sz);
					} else if (i + 1 == argc)
						goto lblError;
					else if (isdigit(argv[++i][0]))
						cbMaxPawnHash = atoi(argv[i]);
					else
						goto lblError;
					break;
				case 't':		// -hp[D] "MaxHash"
					if (*sz != '\0') {
						if (!isdigit(*sz))
							goto lblError;
						cbMaxHash = atoi(sz);
					} else if (i + 1 == argc)
						goto lblError;
					else if (isdigit(argv[++i][0]))
						cbMaxHash = atoi(argv[i]);
					else
						goto lblError;
					break;
				default:
					goto lblError;
				}
				break;
			case 'p':			// -p "Priority"
				pcon->fLowPriority = true;
				break;
			case 'r':			// -r<B> "Resign"
				if (*sz != '\0') {
					if (!isdigit(*sz))
						goto lblError;
                    pcon->fResign = atoi(sz) ? true : false;
				} else if (i + 1 == argc)
					goto lblError;
				else if (isdigit(argv[++i][0]))
                    pcon->fResign = atoi(argv[i]) ? true : false;
				else
					goto lblError;
				break;
			case 't':			// -t<F> <N> "Profile Fen"
				if (*sz != '\0')
					strcpy(aszProfileFen, sz);
				else if (i + 1 == argc)
					goto lblError;
				else if ((argv[++i][0] != '-') && (argv[i][0] != '/'))
					strcpy(aszProfileFen, argv[i]);
				else
					goto lblError;
				if (i + 1 == argc)
					goto lblError;
				else if (!isdigit(argv[++i][0]))
					goto lblError;
				tmProfile = atol(argv[i]) * 1000;
				break;
			default:
				goto lblError;
			}
			break;
		default:
			goto lblError;
		}
	}
	VPrSendComment("UseBook: %s", c_argszNoYes[fUseBook]);
    InitBitboards();
	VInitHashk();
	VInitMiddlegamePawnTable();
    uciHash.set_spin( cbMaxHash );
	if (!FInitHashp(pcon, cbMaxPawnHash))
		return NULL;
    book_clear();
    uciOwnBook.set_check( fUseBook );
    uciBookName.set_string( newBook );
    uciEGBBDir.set_string( egbbdir );
    if( fUseBook )
        book_open( newBook );
	VReseed();
	pcon->smode = smodeIDLE;
	f = SetBoardFromFEN(pcon, s_aszFenDefault);
	Assert(f);

    	PSTE	pste = pcon->argste;
        initEG(pcon, pste);
    	f = SetBoardFromFEN(pcon, s_aszFenDefault);

//    book_find_move(pcon, &pcon->argste[0], false);
//    book_disp(pcon, &pcon->argste[0]);
	VPrSetPost(pcon, false);
	VPrSetPonder(pcon, false);
    VPrSetUciMode(pcon, false);
    VPrSetFixedNodeCount(pcon, 0);
    VPrSetFixedDepthTimeControl(pcon,0);
	VSetTimeControl(pcon, 1, 2000, 0);	// Set to play 2 seconds per move if
										//  the user is foolish enough to try
										//  to run the program from the
										//  command line.
//	VSetTimeControl(pcon, 0, 9999, 9999);	// Set to play 2 seconds per move if
	if (aszProfileFen[0]) {
		if (!FPrSetboard(pcon, aszProfileFen)) {
			VPrSendComment("Can't initialize profile FEN.");
			exit(1);
		}
		VPrSetTimeControl(pcon, 1, tmProfile, 0);
		VPrSetPost(pcon, false);
		VPrSetPonder(pcon, false);
		VPrSendComment("Profile for %ld sec", tmProfile / 1000);
		VPrThink(pcon, tmProfile, 0);
		exit(1);
	}
    verify_move_gen(pcon, pste);
    SetBoardFromFEN(pcon, s_aszFenDefault);
/*    int t = 60*40;
    int t2 = 60*40;
    for(int m = 32; m >= 5; m--) {
        int tm = t * 4 / (3*(m+1));
        int m2 = m >= 25 ? 25 : m;
        int tm2 = t2 / m2;
        printf("%d\t%d\t%d\t%d\t%d\t%d\n", t, m, tm, t2, m2, tm2);
        t -= int(tm*1.0f);
        t2 -= int(tm2*1.0f);
    }*/
	return pcon;
}

// --------------------------------------------------------------------------


void VStalemate(int coStalemated)
{
    VPrSendResult("1/2-1/2", "Stalemate");
}

//	"coWin" just checkmated.  Tell the interface.

void VMates(int coWin)
{
    char	aszResult[64];
    char	aszReason[64];

    sprintf(aszResult, "%d-%d", coWin, coWin ^ 1);
    sprintf(aszReason, "%s mates", s_argszCo[coWin]);
    VPrSendResult(aszResult, aszReason);
}

// --------------------------------------------------------------------------

//	These two functions look for specific moves in the move list, in order to
//	increase their sort key so they will sort first.

void SortPV(PSTE pste, int depth)
{
    foreachmove(pste, pcm)
		if ( *pcm == lastPV[depth] ) {
			pcm->cmk |= cmkPV;
			return;
		}
}

void SortHash(PSTE pste, PHASH phash)
{
    U32 hm = phash ? phash->getMove() : 0;
    if( !hm )
        return;
    foreachmove(pste, pcm)
        if (pcm->m == hm) {
			pcm->cmk |= cmkHASH;
			return;
		}
}


// --------------------------------------------------------------------------
