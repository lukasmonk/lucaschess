
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
#include "moves.hpp"
#include "hash.hpp"

#ifdef	DEBUG
//static char const s_aszModule[] = __FILE__;
#endif

//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-
#if 0
static int const s_argcf[] = {
	~cfE1C1,~0,		~0,		~0,		~(cfE1G1 | cfE1C1), ~0,		~0,		~cfE1G1,0,0,0,0,0,0,0,0,
	~0,		~0,		~0,		~0,		~0,		~0,		~0,		~0,0,0,0,0,0,0,0,0,
	~0,		~0,		~0,		~0,		~0,		~0,		~0,		~0,0,0,0,0,0,0,0,0,
	~0,		~0,		~0,		~0,		~0,		~0,		~0,		~0,0,0,0,0,0,0,0,0,
	~0,		~0,		~0,		~0,		~0,		~0,		~0,		~0,0,0,0,0,0,0,0,0,
	~0,		~0,		~0,		~0,		~0,		~0,		~0,		~0,0,0,0,0,0,0,0,0,
	~0,		~0,		~0,		~0,		~0,		~0,		~0,		~0,0,0,0,0,0,0,0,0,
	~cfE8C8,~0,		~0,		~0,		~(cfE8G8 | cfE8C8), ~0,		~0,		~cfE8G8,0,0,0,0,0,0,0,0,
};

//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-

//	Making a null move does not mean doing nothing.  It means doing this.



void MakeNullMove(PCON pcon, PSTE pste)
{
	pste[1].isqEnP = isqNIL;	// No en-passant square.
	pste[1].fNull = true;		// Null move not allowed.
	pste[1].cf = pste->cf;		// Castling flags are passed.
	pste[1].pcmFirst =			// This is like a null move-generation.
		pste->pcmFirst;
	pste[1].plyFifty =			// Advance fifty-move counter.  This is a
		(pste + 1)->plyFifty + 1;	//  little strange but probably okay.
	pste[1].hashkPc =			// Switch colors, but other than that,
		HashkSwitch(pste->hashkPc);	//  hash key doesn't change.
	pste[1].hashkPn =			// Pawn hash key is unaffected.
		pste->hashkPn;
	//
	//	Material value counters aren't affected.
	//
	pste[1].valPcUs = pste->valPcThem;
	pste[1].valPcThem = pste->valPcUs;
	pste[1].valPnUs = pste->valPnThem;
	pste[1].valPnThem = pste->valPnUs;
}


//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-

//	The following makes a move ("pcm") on the internal board, and sets up the
//	next move stack element ("pste + 1") to reflect the changes.

void MakeMove(PCON pcon, PSTE pste, const CM *pcm)
{
    PSTE npste = pste + 1;  // next stack element
    int side = pste->coUs;
    int isqTo = pcm->isqTo;
    int isqFrom = pcm->isqFrom;

	npste->isqEnP = isqNIL;	// By default, no en-passant square.
	npste->fNull = false;		// By default, null-move is allowed.
	npste->cf = pste->cf &		// Some castling flags might get turned
		s_argcf[isqFrom] &		//  off if I'm moving a king or rook.
		s_argcf[isqTo];
	//
	//	Initialize various values for the next ply.
	//
	npste->plyFifty = pste->plyFifty + 1;
	npste->hashkPc = HashkSwitch(pste->hashkPc);
	npste->hashkPn = pste->hashkPn;
	npste->valPcUs = pste->valPcThem;
	npste->valPcThem = pste->valPcUs;
	npste->valPnUs = pste->valPnThem;
	npste->valPnThem = pste->valPnUs;
	//
	//	This section handles cleaning up a captured piece.  The nastiest stuff
	//	is in the en-passant section.
	//
	PSQ psqTo = &pcon->argsq[isqTo];
	if (pcm->cmf & cmfCAPTURE) {
		int	isqTook;

		if (pcm->cmf & cmfMAKE_ENP) {			// This is gross.
			Assert(pste->isqEnP != isqNIL);
			isqTook = pste->isqEnP + ((pste->coUs == coWHITE) ? -filIBD : filIBD);
			pste->ppiTook = pcon->argsq[isqTook].ppi;
			Assert(pste->ppiTook != NULL);
			Assert(pste->ppiTook->pc == pcPAWN);
			Assert(pste->ppiTook->co != side);
			pcon->argsq[isqTook].ppi = NULL;
		} else {
			isqTook = isqTo;				    // This is normal.
			pste->ppiTook = psqTo->ppi;			// This field will be used
		}										//  when it's time to unmake.
		Assert(pste->ppiTook != NULL);
		pste->ppiTook->fDead = true;			// <-- Mark it dead.
        // XOR the piece out of the hash key
		rehash(npste->hashkPc, pste->ppiTook->pc, pste->ppiTook->co, isqTook);
        // Remove a captured pawn from the pawn hash key
		if (pste->ppiTook->pc == pcPAWN)		
			rehash( npste->hashkPn, pcPAWN, pste->ppiTook->co, isqTook );
		npste->plyFifty = 0;			    	// Capture resets this.
		npste->valPcUs -= s_argvalPcOnly[pste->ppiTook->pc];
		npste->valPnUs -= s_argvalPnOnly[pste->ppiTook->pc];
	}
 	//	The next few lines move the piece, and clear out the "from" square,
	//	modify the new hash key, and deal with the fifty-move counter if this
	//	is a pawn move.
 	//
	PSQ psqFrom = &pcon->argsq[isqFrom];
	if (psqFrom->ppi->pc == pcPAWN)
		npste->plyFifty = 0;
	psqTo->ppi = psqFrom->ppi;
	psqFrom->ppi = NULL;
	psqTo->ppi->isq = isqTo;
	rehash( npste->hashkPc, psqTo->ppi->pc, side, isqFrom );
	rehash( npste->hashkPc, psqTo->ppi->pc, side, isqTo );
	if (psqTo->ppi->pc == pcPAWN) {
		rehash( npste->hashkPn, pcPAWN, side, isqFrom );
		rehash( npste->hashkPn, pcPAWN, side, isqTo );
	}
	//	The rest of the function handles dumb special cases like castling
	//	and promotion and setting the en-passant square behind a two-square
	//	pawn move.
	//
	if (pcm->cmf & cmfCASTLE) {
		int	isqRookFrom;
		int	isqRookTo;

		switch (isqTo) {
		case isqC1:
			isqRookFrom = isqA1;
			isqRookTo = isqD1;
			break;
		case isqG1:
			isqRookFrom = isqH1;
			isqRookTo = isqF1;
			break;
		case isqC8:
			isqRookFrom = isqA8;
			isqRookTo = isqD8;
			break;
		default:
			Assert(false);
		case isqG8:
			isqRookFrom = isqH8;
			isqRookTo = isqF8;
			break;
		}
		PSQ psqRookFrom = &pcon->argsq[isqRookFrom];
	    PSQ psqRookTo = &pcon->argsq[isqRookTo];
		PPI ppiRook = psqRookFrom->ppi;
		Assert(ppiRook != NULL);
		ppiRook->isq = isqRookTo;
		Assert(psqRookTo->ppi == NULL);
		psqRookTo->ppi = ppiRook;
		psqRookFrom->ppi = NULL;
		rehash( npste->hashkPc, pcROOK, side, isqRookFrom );
		rehash( npste->hashkPc, pcROOK, side, isqRookTo );
	} else if (pcm->cmf & cmfPR_MASK) {
		int	pcTo = (int)(pcm->cmf & cmfPR_MASK);

		psqTo->ppi->pc = pcTo;
		npste->valPcThem += s_argvalPcOnly[pcTo];
		npste->valPnThem -= valPAWN;
		//
		//	Reflect the pawn's promotion in the hash key.
		//
		rehash( npste->hashkPc, pcPAWN, side, isqTo );
		rehash( npste->hashkPc, pcTo, side, isqTo );
		//
		//	In the pawn hash key, the pawn is just XOR'd out -- it's gone.
		//
		rehash( npste->hashkPn, pcTo, side, isqTo );
	} else if (pcm->cmf & cmfSET_ENP)
		npste->isqEnP = isqTo + ((side == coWHITE) ? -filIBD : filIBD);
}

//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-

//	This is just "VMakeMove" in reverse.  It's easier since a lot of the stuff
//	I put in "pste + 1" just gets popped off.

void UnmakeMove(PCON pcon, PSTE pste, const CM *pcm)
{
	PSQ psqTo = &pcon->argsq[pcm->isqTo];
	Assert(psqTo->ppi != NULL);
	PSQ psqFrom = &pcon->argsq[pcm->isqFrom];
	psqTo->ppi->isq = psqFrom->isq;
	psqFrom->ppi = psqTo->ppi;
	psqTo->ppi = NULL;
	if (pcm->cmf & cmfCAPTURE) {
		pste->ppiTook->fDead = false;
		pcon->argsq[pste->ppiTook->isq].ppi = pste->ppiTook;
	}
	if (pcm->cmf & cmfCASTLE) {
		PSQ	psqRookFrom;
		PSQ	psqRookTo;
		PPI	ppiRook;

		switch (pcm->isqTo) {
		case isqC1:
			psqRookFrom = &pcon->argsq[isqA1];
			psqRookTo = &pcon->argsq[isqD1];
			break;
		case isqG1:
			psqRookFrom = &pcon->argsq[isqH1];
			psqRookTo = &pcon->argsq[isqF1];
			break;
		case isqC8:
			psqRookFrom = &pcon->argsq[isqA8];
			psqRookTo = &pcon->argsq[isqD8];
			break;
		default:
			Assert(false);
		case isqG8:
			psqRookFrom = &pcon->argsq[isqH8];
			psqRookTo = &pcon->argsq[isqF8];
			break;
		}
		ppiRook = psqRookTo->ppi;
		ppiRook->isq = psqRookFrom->isq;
		psqRookFrom->ppi = ppiRook;
		psqRookTo->ppi = NULL;
	} else if (pcm->cmf & cmfPR_MASK) {
		pcon->argsq[pcm->isqFrom].ppi->pc = pcPAWN;
	}
}
#endif
//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-

int perftSearch(PCON pcon, PSTE pste)
{
    pste->ccmPv = 0;

    if (pste->plyRem <= 0)
        return 1;

	if (++pcon->ss.nodes >= pcon->ss.nodesNext) {
		//
		//	2000 nodes is maybe 1/50to 1/100 of a second.
		//
		pcon->ss.nodesNext = pcon->ss.nodes + 20000;
        extern void VPrCallback(PCON pv);
		VPrCallback(pcon);
		VCheckTime(pcon);
		if ((pcon->fAbort) || (pcon->fTimeout))
			return 0;
	}

	//	Generate moves.
	//
	bGenMoves(pcon, pste);
	//	Mark the PV and hash moves so they sort first.
	//

    bool fFound = false;
    int  c_move = 0;

	//
	//	Iterate through the move list, trying everything out.
	//
    foreachmove(pste, pcm) {
//	for (PCM pcm = pste->pcmFirst; pcm < (pste + 1)->pcmFirst; pcm++) {
		//
		//	Make the move, check legality, then see if the 50-move counter 
        //  indicates that I need to return a draw score
		//
		bMove(pcon, pste, pcm->m);
		//
		//
		//	Set up for recursion.
		//
		(pste + 1)->plyRem = pste->plyRem - 1;
		//
		//	Recurse into normal search or quiescent search, as appropriate.
		//
        c_move += perftSearch(pcon, pste + 1);

		bUndoMove(pcon, pste, pcm->m);
		if ((pcon->fAbort) || (pcon->fTimeout)) {
			return 0;
		}
		//
		fFound = true;
	}
	return c_move;
}

void perft(PCON pcon, int depth) {
//    extern int testMAGIC(PCON pcon, PSTE pste, int depth);
    extern int genmagic2();

//    genmagic2();
//    return;

    initMVVLVA( pcon );
    PSTE pste = &pcon->argste[0];
//    testMAGIC(pcon, pste, depth);
//    return;

    for(int i = 1; i <= depth; i++) {
	    pcon->fAbort = false;
	    pcon->smode = smodeANALYZE;
        pcon->fTimeout = false;	    	// I'm not timed out yet.
		pcon->ss.tmStart = TmNow();		// Record when I started.
        pcon->ss.nodes = 0;
		pcon->ss.nodesNext = 20000;
        pste->plyRem = i;
        printf("%d\t", i);
        int nodes = perftSearch(pcon, pste );
        int t = TmNow() - pcon->ss.tmStart;
        int nps = 1;
        if( t > 0 && nodes > 100000 )
            nps = int(nodes*1.0 / t);
        printf("%3.1f \t %10d %10dk\n", t / 1000.0, nodes, nps);
        fflush(stdout);
    }
	pcon->smode = smodeIDLE;
}
/*
perft 6 (debug)
1       0.0              20          1
2       0.0             400          1
3       0.0            8902          1
4       0.2          197281    1146982
5       4.2         4865609    1149175
6       103.6     119060324    1148776

bb perft 6 (debug)
 1.         20    0.00
 2.        400    0.00
 3.       8902    0.02
 4.     197281    0.13
 5.    4865609    3.16
 6.  119060324   77.08

perft 6 (release)
1       0.0              20          1
2       0.0             400          1
3       0.0            8902          1
4       0.0          197281    4197468
5       1.2         4865609    3939764
6       29.9      119060324    3981151

 bb perft 6 (release)
 1.         20    0.00
 2.        400    0.00
 3.       8902    0.00
 4.     197281    0.03
 5.    4865609    0.58
 6.  119060324   13.86

 perft 6
1       0.0              20          1k
2       0.0             400          1k
3       0.0            8902          1k
4       0.0          197281      12330k
5       0.7         4865609       7072k
6       16.4      119060324       7250k

*/
