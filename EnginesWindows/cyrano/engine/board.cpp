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
#include "board.hpp"
#include "hash.hpp"
#include "moves.hpp"
#include "genmagic.hpp"

#ifdef	_DEBUG
static char const s_aszModule[] = __FILE__;
#endif

//	Candidate move flags.  The move generator marks each move it generates
//	with zero or more of these.

#define	cmfNONE		0x00000000	// A normal move.
#define	cmfPR_MASK	0x0000000F	// If a move promotes, the "pc" it promotes
								//  to will be stuck in these bits.
#define	cmfSET_ENP	0x00010000	// e2-e4 will set this flag.
#define	cmfMAKE_ENP	0x00020000	// An en-passant capture will set this flag.
#define	cmfCASTLE	0x00040000	// O-O will set this flag.
#define	cmfCAPTURE	0x00080000	// Any capture (including en-passant) will set
								//  this flag.

//	PROM_Q,		PROM_R,		PROM_B,		PROM_N
static char promPc[] = {'Q','R','B','N'};


//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-

//	This routine takes a "SAN" record and produces a SAN move output string
//	(such as "e4", or "dxe6+", or "Nbd7", or "Qxg7#").

//	The SAN element already has all the information needed to do this, so it's
//	a very simple process.

void SanToSz(const SAN *psan, char * sz)
{
    if( psan->isqFrom == 0 && psan->isqTo == 0 ) {
        sz += sprintf(sz, "0000");
	    *sz = '\0';
        return;
    }
	if (psan->cmf & cmfCASTLE) {
		if ((psan->isqTo == G1) || (psan->isqTo == G8))
			sz += sprintf(sz, "O-O");
		else
			sz += sprintf(sz, "O-O-O");
	} else {
		if (psan->pc == WP || psan->pc == BP )
			*sz++ = char(File(psan->isqFrom) + 'a');
		else {
			*sz++ = pc_char_map[psan->pc|1];
			if (psan->sanf & sanfRNK_AMBIG)
				*sz++ = char(File(psan->isqFrom) + 'a');
			if (psan->sanf & sanfFIL_AMBIG)
				*sz++ = char(Rank(psan->isqFrom) + '1');
		}
		if (psan->cmf & cmfCAPTURE) {
			*sz++ = 'x';
			*sz++ = char(File(psan->isqTo) + 'a');
			*sz++ = char(Rank(psan->isqTo) + '1');
		} else if ((psan->pc|1) == WP)
			*sz++ = char(Rank(psan->isqTo) + '1');
		else {
			*sz++ = char(File(psan->isqTo) + 'a');
			*sz++ = char(Rank(psan->isqTo) + '1');
		}
		if (psan->cmf & cmfPR_MASK) {
			*sz++ = '=';
			*sz++ = promPc[(psan->cmf & cmfPR_MASK)-1];
		}
	}
	if (psan->sanf & sanfMATE)
		*sz++ = '#';
	else if (psan->sanf & sanfCHECK)
		*sz++ = '+';
	*sz = '\0';
}

//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-

//	This takes a CM and converts it to a SAN record, which can be used by
//	"VSanToSz" to make something pretty to display.

//	This is a terrible process, in large part because of disambiguation.
//	Disambiguation would be extremely easy, if all I had to do is disambiguate
//	between moves produced by the move generator.  But some of these moves are
//	not actually legal, so I have to delete those.

//	Also, part of the process includes appending "+", or "#" if necessary,
//	which especially in the latter case is annoying.

//	So this routine figures out if a given move is ambiguous, and if so
//	records exactly how, and it also figures out if the move is check or mate.

//	WARNING! This function won't work if the moves aren't generated for this
//	position, and these moves are pseudo-legal, not fully legal.

void CmToSan(PCON pcon, PSTE pste, const CM *pcmSrc, PSAN psan)
{
	//	Easy stuff first.
	//
    U32 move = pcmSrc->m;
	// Decompress the move
	int from	= From(move);
	int to		= To(move);
    int pc      = Pc(move);

	psan->pc = pc;
	psan->isqFrom = from;
	psan->isqTo = to;
    psan->cmf = 0;
    if( from == 0 && to == 0 )
        return; // null move
    if( Prom(move) )
        to=to;
    psan->m   = move;
	psan->sanf = sanfNONE;
    if( Capt(move) ) {
        psan->cmf |= cmfCAPTURE;
    }
    if( (pc|1) == (PROM|1)) {
        psan->pc = PAWN | (pc & 1);
        psan->cmf |= Prom(move) + 1;
    }
    if( pc == B_CASTLE) {
    	psan->pc = BK;
        psan->cmf |= cmfCASTLE;
    }
    if( pc == W_CASTLE) {
    	psan->pc = WK;
        psan->cmf |= cmfCASTLE;
    }

	//
	//	Disambiguate.
	//
	bool fAmbiguous = false;
    foreachmove(pste,pcm) {
		if ( (To(pcm->m) == To(move)) && From(pcm->m) != From(move) &&
            Pc(pcm->m) == Pc(move)) {
			bMove(pcon, pste, pcm->m);
			if (Rank(From(pcm->m)) == Rank(From(move)))
				psan->sanf |= sanfRNK_AMBIG;
			if (File(From(pcm->m)) == File(From(move)))
				psan->sanf |= sanfFIL_AMBIG;
			fAmbiguous = true;
			bUndoMove(pcon, pste, pcm->m);
		}
    }
	if ((fAmbiguous) && (!(psan->sanf & (sanfRNK_AMBIG | sanfFIL_AMBIG))))
		psan->sanf |= sanfRNK_AMBIG;	// "Nbd2" rather than "N1d2".
	//
	//	The gross disambiguation has already been done.  Now I'm going to
	//	execute the move and see if it is check or mate.
	//
	bMove(pcon, pste, pcmSrc->m);
	if ( pste[1].checkf ) {
		psan->sanf |= sanfCHECK;
		bGenEvasions(pcon, pste + 1);
        if( legalMoveCount( pste + 1 ) == 0 )
			psan->sanf |= sanfMATE;
	}
	bUndoMove(pcon, pste, pcmSrc->m);
	//
	//	That was painful.
}

//	This converts a CM to "e2e4" (or "e7e8q") format.  I'm writing this with
//	Winboard in mind, and that is a format that Winboard will accept.  It is
//	also very easy to generate.
void CmToSz(U32 m, char * sz)
{
    if( m == 0 )
        // null move
        strcpy(sz, "0000");
    else
	    sz += sprintf(sz, "%c%c%c%c",
		    File(From(m)) + 'a',
		    Rank(From(m)) + '1',
		    File(To(m)) + 'a',
		    Rank(To(m)) + '1');
	if (Pc(m) == W_PROM || Pc(m) == B_PROM )
		sz += sprintf(sz, "%c", promPc[Prom(m)]);
}

void CmToSz(const CM *pcm, char * sz)
{
    CmToSz(pcm->m, sz);
}

bool move_is_legal(PCON pcon, PSTE pste, U32 m) {
    bGenMoves(pcon, pste);
    foreachmove(pste,pcm)
        if(pcm->m == m)
            return true;
    return false;
}

