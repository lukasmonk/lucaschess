//
// Cyrano Chess engine
//
// Copyright (C) 2007  Harald JOHNSEN
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

#ifndef _MOVES_HPP_
#define _MOVES_HPP_

//#define inCheck(pcon, side) (FAttacked(pcon, pcon->argpi[side][ipiKING].isq, side ^ 1))
//#define inCheck(pcon, side) (Attacked(pcon, pcon->king[side], (side)^1))

#define foreachmove(pste,pcm) \
    int iMove = 0;\
    int moveCount = int( (pste+1)->mlist_start - pste->mlist_start);\
    for(PCM pcm = pste->mlist_start; iMove < moveCount ; pcm++,iMove++)


//	Candidate move key.  In the search, it is very useful to search the best
//	move first, so in the candidate move record, a sort key is kept.  The
//	sort key may consist of some of these flags.

#if 0   // bad captures before quiet moves
#define	cmkNONE		0x00100000
#define	cmkBADCAPT  0x00200000
#else   // bad captures at the end
#define	cmkBADCAPT  0x00100000
#define	cmkNONE		0x00200000
#endif
#define	cmkQUEEN	0x00400000
#define	cmkKILLER	0x01000000
#define	cmkEQUCAPT	0x02000000
#define	cmkGOODCAPT	0x04000000
#define	cmkHASH		0x10000000
#define	cmkPV		0x20000000
#define	cmkQS		0x80000000
#define cmkMASK     0xfff00000

// Move and board conversion macros
//
#define From(m)		((m) & 63)
#define To(m)		((m >> 6) & 63)
#define Pc(m)		((m >> 12) & 15)
#define Capt(m)		((m >> 16) & 15)
#define Prom(m)		((m >> 20) & 3)

#define Rank(sqr)			((sqr) >> 3)
#define File(sqr)			((sqr) & 7)

// Piece/engine constants
//
const int W	= 1;
const int B	= 0;

// Base piece/move types
const int KING		= 0;
const int QUEEN		= 2;
const int ROOK		= 4;
const int BISHOP	= 6;
const int KNIGHT	= 8;
const int PAWN		= 10;
const int EP_CAPT	= 12;
const int CASTLE	= 12;
const int PROM		= 14;
const int PIECES    = 14;

extern const char pc_char_map[12];
typedef struct _attack {
    int attacks[128];
    int mobility[PIECES];
    int c_mobility;
} Attack;


// Indexes into the piece array
enum PIECE_ARRAY {
	BK, WK, BQ, WQ, BR, WR, BB, WB, BN, WN, BP, WP,
	OCCU, X_OCCU, B_OCCU, W_OCCU, TEMP
};

enum SPECIAL_MOVES {
	B_CASTLE = 12, W_CASTLE, B_EP = 12, W_EP,
	B_PROM, W_PROM,
};

enum PROM_PIECE {
	PROM_Q,		PROM_R,		PROM_B,		PROM_N
};

enum SQUARES {  
	A1, B1, C1, D1, E1, F1, G1, H1,		A2, B2, C2, D2, E2, F2, G2, H2,
	A3, B3, C3, D3, E3, F3, G3, H3,		A4, B4, C4, D4, E4, F4, G4, H4,
	A5, B5, C5, D5, E5, F5, G5, H5,		A6, B6, C6, D6, E6, F6, G6, H6,
	A7, B7, C7, D7, E7, F7, G7, H7,		A8, B8, C8, D8, E8, F8, G8, H8
};

extern void genAttacks(const CON *pcon, const STE *pste, int side, Attack *attack);
extern void initMVVLVA(const CON * pcon);
extern void perft(PCON pcon, int depth);

int Attacked(const CON *pcon, int sqr, int col);
U64 AttacksTo(const CON *pcon, int sqr);
CM* bGenCaptures(PCON pcon, PSTE pste, CM * list);
CM* bGenNonCaptures(PCON pcon, PSTE pste, CM * list);
CM* bGenEvasions(PCON pcon, PSTE pste, CM * list = NULL);
CM* bGenPasserPush(PCON pcon, PSTE pste, CM * list);
CM* bGenPawnPush(PCON pcon, PSTE pste, CM * list);
void bGenMoves(PCON pcon, PSTE pste);
CM* bGenPseudoCheck(PCON pcon, PSTE pste, CM * list);

void bMove(PCON pcon, PSTE pste, U32 move);
void bUndoMove(PCON pcon, PSTE pste, U32 move);
void bNullMove(PCON pcon, PSTE pste);
void bUndoNullMove(PCON pcon, PSTE pste);

static inline int legalMoveCount(const STE *pste) {
    return int( (pste+1)->mlist_start - pste->mlist_start) ;
}
extern void Print(const CON *pcon, const STE *pste);


extern unsigned int MVV_LVA[16][16];

#endif // _MOVES_HPP_
