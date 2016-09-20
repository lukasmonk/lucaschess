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



#include "engine.hpp"
#include "gproto.hpp"
#include "moves.hpp"
#include "magicmoves.h"
#include "genmagic.hpp"
#include "eval.hpp"
#include "egbbdll.h"
#include "uci.hpp"

#ifdef	DEBUG
//#include <windows.h>
static char const s_aszModule[] = __FILE__;
//#define BUILD_BB
#endif


static const int promotionSqrFileA[2] = {A1,A8};
static const int promotionSqrFileH[2] = {H1,H8};

static bool egbb_is_loaded = false;

#ifdef BUILD_BB

enum {_WHITE,_BLACK};
enum {_EMPTY,_WKING,_WQUEEN,_WROOK,_WBISHOP,_WKNIGHT,_WPAWN,
_BKING,_BQUEEN,_BROOK,_BBISHOP,_BKNIGHT,_BPAWN};
enum {LOAD_NONE,LOAD_4MEN,SMART_LOAD,LOAD_5MEN};

#define _NOTFOUND 99999

typedef int (*PPROBE_EGBB) (int player, int w_king, int b_king,
							int piece1, int square1,
							int piece2, int square2,
							int piece3, int square3);

typedef void (*PLOAD_EGBB) (char* path,int cache_size,int load_options);
static PPROBE_EGBB probe_egbb;

int egbb_load_type = LOAD_4MEN;

/*
Load the dll and get the address of the load and probe functions.
*/

#ifdef _MSC_VER
#    define EGBB_NAME "egbbdll.dll"
#else
#    define EGBB_NAME "egbbso.so"
#    define HMODULE void*
#    define LoadLibrary(x) dlopen(x,RTLD_LAZY)
#    define GetProcAddress dlsym
#endif

static int LoadEgbbLibrary(char* main_path,int egbb_cache_size) {
	HMODULE hmod;
	PLOAD_EGBB load_egbb;
	char path[256];

	strcpy(path,main_path);
	strcat(path,EGBB_NAME);
	if(hmod = LoadLibrary(path)) {
		load_egbb = (PLOAD_EGBB) GetProcAddress(hmod,"load_egbb_5men");
     	probe_egbb = (PPROBE_EGBB) GetProcAddress(hmod,"probe_egbb_5men");
        load_egbb(main_path,egbb_cache_size,egbb_load_type);
		return true;
	} else {
		printf("EgbbProbe not Loaded!\n");
		return false;
	}
}

#endif // BUILD_BB

typedef enum WDL {loss = -1, draw = 0 , win = 1} EGPROBE_result ;

typedef unsigned char byte;

class WDtable {
private:
    byte *table;
    int size;
public:
    WDtable(int _size) {
        size = _size;
        table = new byte[size/8];
    }
    ~WDtable() {
        delete table;
    }
    void set(int idx, enum WDL result) {
        if( result == loss )
            result = win;
        Assert(idx < size);
        Assert(result == draw || result == win);
        byte mask = 1 << (idx & 7);
        table[idx / 8] = table[idx / 8] & ~ mask | ((result & 1) << (idx & 7));
    }
    enum WDL get(int idx) {
        Assert(idx < size);
        return WDL((table[idx / 8] >> (idx & 7)) & 1);
    }
    void save(const char *filename) {
        FILE *f = fopen(filename, "wb");
        fwrite(table, 1, size/8, f);
        fclose(f);
    }
    void load(const char *filename) {
        FILE *f = fopen(filename, "rb");
        if( f ) {
            fread(table, 1, size/8, f);
            fclose(f);
        } else {
            VPrSendComment("***** could not load file %s", filename);
        }
    }
};

void WDL_set(byte *table, int idx, enum WDL result) {
    Assert(result == draw || result == win || result == loss);
    byte mask = 3 << (idx & 3);
    table[idx / 4] = table[idx / 4] & ~ mask | ((result & 3 ) << (idx & 3));
}
enum WDL WDL_get(byte *table, int idx) {
    return WDL((table[idx / 4] >> (idx & 3)) & 3);
}

WDtable KPK_bitbase(64*64*64*2);


/*static inline int sqDistance( int isqFrom, int isqTo ) {

    int dr = Rank( isqFrom ) - Rank( isqTo );
    int df = File( isqFrom ) - File( isqTo );
    if( dr < 0 ) dr = -dr;
    if( df < 0 ) df = -df;
    if( dr > df )
        return dr;
    return df;
}*/

static inline int idxFromSq(int sqWK, int sqBK, int sqWP, int side) {
    Assert(sqWP >= A2 && sqWP <= H7);
    return ((sqWK * 64 + sqBK) * 64 + sqWP) * 2 + side;
}

#ifdef BUILD_BB
static bool testKPKpos(PCON pcon, PSTE pste, int sqWK, int sqBK, int sqWP, int side) {
    int thisIdx = idxFromSq( sqWK, sqBK, sqWP, side);

    if( sqWK == sqBK || sqWK == sqWP || sqBK == sqWP || sqDistance(sqWK, sqBK) < 2 )
        return false;

    pste->checkf    = 0;
	pste->castle	= 0;
	pste->ep_t		= 0;
	memset(pcon->pos, 0, sizeof(pcon->pos));
	memset(pcon->pieces, 0, sizeof(pcon->pieces));
	pste->hashkPc = 0;		// Going to XOR the all pieces/pawns into this.
	pste->hashkPn = 0;		// Going to XOR the pawns only into this.
    pste->side = side;
    pste->xside = side ^ 1;
    pste->mlist_start = pcon->move_list;

    AddPiece(pcon, sqWK, KING|W);
    AddPiece(pcon, sqBK, KING|B);
    AddPiece(pcon, sqWP, PAWN|W);

    int xside = side ^ 1;
	// Initialize pin/check data
	pste->pinner = 0;
	pste->pinned = 0;

    int score;
    if( probe_bitbases(pcon, pste, side, score) ) {
        if( score == 0 )
            KPK_bitbase.set( thisIdx,  draw );
        else if( score < 0 )
            KPK_bitbase.set( thisIdx, loss );
        else
            KPK_bitbase.set( thisIdx, win );
        return true;
    } else {
        score = score;
    }

    return false;
}
#endif

void prepareEGBB(void) {
    if(egbb_is_loaded) {
        // how to clean bb ?
        egbb_is_loaded = false;
    }
    if( uciUseEGBB.get_check() && strlen( uciEGBBDir.get_string() ) && !egbb_is_loaded) {
        char tmp[512];
        strncpy(tmp, uciEGBBDir.get_string() , sizeof(tmp));
        strcat(tmp, "/");
        VPrSendComment("# before load_egbb_5men (%s)", tmp);
        load_egbb_5men( tmp, 4*1024*1024, /*LOAD_4MEN*/ LOAD_NONE);
        VPrSendComment("# after load_egbb_5men");
        egbb_is_loaded = true;
    }
}

void initEG(PCON pcon, PSTE pste) {
    VPrSendComment("# start of initEG");
    int t = TmNow();
    init_recognizers();
    prepareEGBB();

#ifdef BUILD_BB
    LoadEgbbLibrary("F:/Chess/TB/egbbs_3_4/", 4*1024*1024);

    for(int sqWP = A2; sqWP <= H7; sqWP++)
        for(int sqWK = A1; sqWK <= H8; sqWK++)
            for(int sqBK = A1; sqBK <= H8; sqBK++)
                for( int side = B; side <= W ; side ++) {
                    testKPKpos(pcon, pste, sqWK, sqBK, sqWP, side);
                }

    KPK_bitbase.save("kpk.bit");
#else
    KPK_bitbase.load("kpk.bit");
#endif
    t = TmNow() - t;
//    testKPKpos(pcon, pste, A8, C8, A7, B);
//    t = probeKPKpos(pcon, pste, A8, C8, A7, B);
//    VPrSendComment("# val %d", KPK_bitbase.get( idxFromSq( B6, B8, A6, B) ));
//    VPrSendComment("# val %d", KPK_bitbase.get( idxFromSq( B6, B8, A6, W) ));
//    VPrSendComment("# val %d", KPK_bitbase.get( idxFromSq( A6, C8, A5, W) ));
    VPrSendComment("# end of initEG t=%d", t);
}

static enum WDL probeKPKpos(const CON *pcon, const STE *pste, int sqWK, int sqBK, int sqWP, int side) {
    int thisIdx = idxFromSq( sqWK, sqBK, sqWP, side);
    return KPK_bitbase.get( thisIdx );
}


//
// King & Pawn vs King
// http://en.wikipedia.org/wiki/King_and_pawn_versus_king_endgame
//
// co is the winning side
//
bool KPK(const CON *pcon, const STE *pste, int co, int & val) {
    bitboard pawns = pcon->pieces[PAWN|co];
    int isqPawn = PopLSB(pawns) ;
    int nc = co ^ 1 ;
#if 1
    isqPawn = remapSq[nc][ isqPawn ];

    int isqBKing = remapSq[nc][pcon->king[co ^ 1]];
    int isqWKing = remapSq[nc][pcon->king[co]];
    int rankPawn = Rank( isqPawn );
    int thisIdx = idxFromSq( isqWKing, isqBKing, isqPawn, co == W ? pste->side : pste->side ^ 1);
    int res = KPK_bitbase.get( thisIdx );
//    if( co == pste->side )
//        res = -res;
    int delta = - sqDistance( isqBKing, isqPawn ) * 2 + sqDistance( isqWKing, isqPawn ) * 1 + rankPawn * 8;
    switch( res ) {
        case 0:
//            val = (25 + delta); // 0 gives less nodes but why not try to draw a lost game :p
            val = 0;
            break;
        case 1:
            val = 700 + delta;
            break;
        case -1:
            val = -( 700  + delta);
            break;
    }
    return true;
#else
    // R1: square rule
    // if dist to promote < dist from king then win
    if( SquareRuleForPawn(pcon, pste, co, isqPawn, val) )
        return true;
    isqPawn = remapSq[nc][ isqPawn ];

    int isqBKing = remapSq[nc][pcon->king[co ^ 1]];
    int isqWKing = remapSq[nc][pcon->king[co]];
    int pawnFile = File(isqPawn);
    int rankPawn = Rank( isqPawn );
    int filePawn = File( isqPawn );
    if( pawnFile == filH || pawnFile == filA ) {
        if( pawnFile == File(isqBKing) ) {
            val = 50 + Rank( isqWKing );
            return true;
        }
        int dot1, dot2;
        if( pawnFile == filH ) {
            dot1 = G8; dot2 = G7;
            if( isqBKing == F8 || isqBKing == F7 || isqBKing == G8 || isqBKing == G7 ) {
                val = 50 - sqDistance( isqWKing, dot2 );
                return true;
            }
        } else {
            dot1 = B8; dot2 = B7;
            if( isqBKing == C8 || isqBKing == C7 || isqBKing == B8 || isqBKing == B7 ) {
                val = 50 - sqDistance( isqWKing, dot2 );
                return true;
            }
        }
        val = 700 + sqDistance( isqBKing, dot2 ) * 32 - sqDistance( isqWKing, dot2 ) * 4 + rankPawn;
        return true;
    } else {
        bitboard keys = 0;
        keys |= BitIdx( rankPawn + 2, filePawn - 1);
        keys |= BitIdx( rankPawn + 2, filePawn);
        keys |= BitIdx( rankPawn + 2, filePawn + 1);
        if( rankPawn >= 4 ) {
            keys |= BitIdx( rankPawn + 1, filePawn - 1);
            keys |= BitIdx( rankPawn + 1, filePawn);
            keys |= BitIdx( rankPawn + 1, filePawn + 1);
        }
        if( rankPawn == 6 ) {
            keys |= BitIdx( rankPawn, filePawn - 1);
            keys |= BitIdx( rankPawn, filePawn);
            keys |= BitIdx( rankPawn, filePawn + 1);
        }
        bool white_on_square = (IdxToU64( isqWKing ) & keys) != 0;
        bool black_on_square = (IdxToU64( isqBKing ) & keys) != 0;
        if( pawnFile != filB && pawnFile != filG ) {
            if( white_on_square ) {
                val = 700 + rankPawn*4;
                return true;
            }
            if(black_on_square ) {
                val = 50;
                return true;
            }
        }
    }
#endif
    return false;
}

// contain the distance of a square to the corner
// used to eval simple mat positions to speedup the finding of the mat

const static int c_argdistcorner[64] = {
//  a   b   c   d   e   f   g   h
	0,	5,	10,	20,	30,	40,	50,	60,
	5,	5,	10,	20,	30,	40,	50,	50,
	10,	10,	10,	20,	30,	40,	40,	40,
	20,	20,	20,	20,	30,	30,	30,	30,
	30,	30,	30,	30,	20,	20,	20,	20,
	40,	40,	40,	30,	20,	10,	10,	10,
	50,	50,	50,	30,	20,	10,	5,	5,
	60,	50,	40,	30,	20,	10,	5,	0,
};

// from Gerd
static unsigned int colorOfSquare (unsigned int sq) {
   return  (0xAA55AA55 >> sq) & 1;
}
// from h.g.m
static bool sameSquareColor(unsigned int sq1, unsigned int sq2) {
    sq1 ^= sq2;
    return  ( (0xAA55AA55 >> sq1) & 1);
}


static int INLINE promotionSqr(int pawnSqr, int co) {
    if( co == W )
        return Idx( 7, File(pawnSqr) );
    else
        return Idx( 0, File(pawnSqr) );
}

static int distanceToPromotionSqr(int pawnSqr, int co) {
    if( co == W )
        return 7 - Rank(pawnSqr);
    else
        return Rank(pawnSqr);
}

// contain the distance of a square to the border
// used to eval simple mat positions to speedup the finding of the mat
// higher value when opposite king is near the border

const static int c_argdistborder[64] = {
//  a   b   c   d   e   f   g   h
	0,	0,	0,	0,	0,	0,	0,	0,
	0,	10,	10,	10,	10,	10,	10,	0,
	0,	10,	20,	20,	20,	20,	10,	0,
	0,	10,	20,	30,	30,	20,	10,	0,
	0,	10,	20,	30,	30,	20,	10,	0,
	0,	10,	20,	20,	20,	20,	10,	0,
	0,	10,	10,	10,	10,	10,	10,	0,
	0,	0,	0,	0,	0,	0,	0,	0,
};

// co is defender
static bool hangingStuff(const CON *pcon, bitboard stuff, int co) {
    return (king_attacks[pcon->king[co]] & stuff) && !(king_attacks[pcon->king[co^1]] & stuff);
}

// 
static bool BishopAttacksSqr(const CON *pcon, int sqr, int col)
{
	if (Bmagic(sqr, pcon->pieces[OCCU]) & pcon->pieces[BISHOP|col] )
		return true;
	return false;
}
static bool BishopAttacksStuff(const CON *pcon, bitboard stuff, int col)
{
    int sqr = LSB(pcon->pieces[BISHOP|col]);
	if (Bmagic(sqr, pcon->pieces[OCCU]) & stuff )
		return true;
	return false;
}

static bool KnightAttacksSqr(const CON *pcon, int sqr, int col)
{
	if ( knight_attacks[sqr] & pcon->pieces[KNIGHT|col] )
		return true;
	return false;
}

// look at captures, pins are not handled
static bool checkCaptures(const CON *pcon, const STE *pste, int side)
{
	U64 pc, moves;
	int sqr[2];
    const U8 *pos = pcon->pos;
    int xside = side ^ 1;
    const unsigned char *king = pcon->king;

	// - Knights -
	for (pc = pcon->pieces[KNIGHT|side]; pc; ) {
		sqr[0] = PopLSB(pc);
		moves = knight_attacks[sqr[0]] & pcon->pieces[B_OCCU|xside];
		if (moves)
            return true;
	}
	// - Bishop -
	for (pc = pcon->pieces[BISHOP|side]; pc; ) {
		sqr[0] = PopLSB(pc);
		moves = Bmagic(sqr[0], pcon->pieces[OCCU]) & pcon->pieces[B_OCCU|xside];
		if (moves)
            return true;
	}
	// - Rook -
	for (pc = pcon->pieces[ROOK|side]; pc; ) {
		sqr[0] = PopLSB(pc);
		moves = Rmagic(sqr[0], pcon->pieces[OCCU]) & pcon->pieces[B_OCCU|xside];
		if (moves)
            return true;
	}
	// - Queen -
	for (pc = pcon->pieces[QUEEN|side]; pc; ) {
		sqr[0] = PopLSB(pc);
		moves = Qmagic(sqr[0], pcon->pieces[OCCU]) & pcon->pieces[B_OCCU|xside];
		if (moves)
            return true;
	}
	// - King -
	moves = king_attacks[king[side]] & pcon->pieces[B_OCCU|xside];
	while (moves) {
		sqr[1] = PopLSB(moves);
		if (Attacked(pcon, sqr[1], xside))
			continue;
		return true;
	}
	// - Pawns forward one -
	U64 p_TEMP = (pcon->pieces[xoccu_wp[side]] << 8) 
		& pcon->pieces[xoccu_bp[xside]];
	moves = p_TEMP & prom_mask[side];
	if (moves)
        return true;

	// - Pawns seven-shift -
	pc = ((pcon->pieces[woccu_wp[side]] & CLEAR_LEFT) << 7)
		& pcon->pieces[boccu_bp[xside]];
	moves = pc & xprom_mask[side];
	if (moves)
        return true;
	moves = pc & prom_mask[side];
	if (moves)
        return true;
	// - Pawns nine-shift -
	pc = ((pcon->pieces[woccu_wp[side]] & CLEAR_RIGHT) << 9)
		& pcon->pieces[boccu_bp[xside]];
	moves = pc & xprom_mask[side];
	if (moves)
        return true;
	moves = pc & prom_mask[side];
	if (moves)
        return true;
	return false;
}

// we can not return a bound for some positions (mate threat, hanging material, ...)
bool positionOkForRecog(const CON *pcon, const STE *pste) {
    // we should handle, mate, pat, captures
    if( pste->checkf )
        return false;
    if( checkCaptures(pcon, pste, B) )
        return false;
    if( checkCaptures(pcon, pste, W) )
        return false;
    return true;
}

//
// King & Bishop & Knight vs King
//
// co is the winning side
//
bool KBNK(PCON pcon, PSTE pste, int co, int & val) {
    bitboard bishops = pcon->pieces[BISHOP|co];
    int isqBishop = LSB(bishops);
    bitboard knights = pcon->pieces[KNIGHT|co];
    int isqKnight = LSB(knights);
    int idxKnight = isqKnight;
    int nc = co ^ 1 ;
    int isqBKing = pcon->king[co ^ 1];
    int idxBKing = isqBKing;
    int isqWKing = pcon->king[co];
    int idxWKing = isqWKing;

    if( colorOfSquare( isqBishop) == 0 ) {
        idxBKing = Idx(Rank(isqBKing), 7-File(isqBKing));
        idxWKing = Idx(Rank(isqWKing), 7-File(isqWKing));
        idxKnight= Idx(Rank(isqKnight), 7-File(isqKnight));
    }
    //sqDistance
    // must lead the black king to the corner of the bishop
//    val = 800 - c_argdistcorner[idxBKing] - c_argdistcorner[idxWKing] / 2 - c_argdistcorner[idxKnight] / 4;
    val = 800 - c_argdistcorner[idxBKing] - sqDistance(isqWKing, isqBKing) * 2 - sqDistance(isqKnight, isqBKing);
    return true;
}

/*
Probe:
Change internal scorpio board representaion to [A1 = 0 ... H8 = 63]
board representation and then probe bitbase.
*/

bool probe_bitbases(CON *pcon, const STE *pste, int player, int& score) {

    if( !egbb_is_loaded )
        return false;

    int piece[32],square[32];
    int count = 0;
    piece[0] = _EMPTY;
    piece[1] = _EMPTY;
    piece[2] = _EMPTY;
    square[0] = 0;
    square[1] = 0;
    square[2] = 0;

#define ADD_PIECE(list,type) \
    for (bitboard pc = list; pc; ) {\
        int sqr = PopLSB(pc);\
        piece[count] = type;\
        square[count] = sqr;\
        count++;\
    }

    ADD_PIECE(pcon->pieces[PAWN|W],_WPAWN);
    ADD_PIECE(pcon->pieces[KNIGHT|W],_WKNIGHT);
    ADD_PIECE(pcon->pieces[BISHOP|W],_WBISHOP);
    ADD_PIECE(pcon->pieces[ROOK|W],_WROOK);
    ADD_PIECE(pcon->pieces[QUEEN|W],_WQUEEN);
    ADD_PIECE(pcon->pieces[PAWN|B],_BPAWN);
    ADD_PIECE(pcon->pieces[KNIGHT|B],_BKNIGHT);
    ADD_PIECE(pcon->pieces[BISHOP|B],_BBISHOP);
    ADD_PIECE(pcon->pieces[ROOK|B],_BROOK);
    ADD_PIECE(pcon->pieces[QUEEN|B],_BQUEEN);
    if( count > 3 )
        return false;
    score = probe_egbb_5men(player ^ 1, pcon->king[W], pcon->king[B],
        piece[0],square[0],piece[1],square[1],piece[2],square[2]);

    if(score != _NOTFOUND) {
        pcon->ss.tbhits++;
        return true;
    }

    return false;
}

U32 recog_avail[32];    // this is 32*32 boolean
//recog recog_table[64];
recog recog_table[1024];

// not exactly the DT indexing because it's simpler like that
static void new_recog(recog fun, unsigned mstate_1, unsigned mstate_2) {
    recog_avail[mstate_1] |= 1 << mstate_2;
//    recog_avail[mstate_2] |= 1 << mstate_1;
    Assert( recog_table[recog_index(mstate_1, mstate_2)] == 0);
    recog_table[recog_index(mstate_1, mstate_2)] = fun;
}

bool invertScore(bool result, recog_result &ret) {
    ret.score = - ret.score;
    if( ret.bound == lower_bound )
        ret.bound = upper_bound;
    else if( ret.bound == upper_bound )
        ret.bound = lower_bound;
    return result;
}

// --------------------------------------------------------------------------

static bool evalKBK(const CON *pcon, const STE *pste, recog_result &ret, int co) {
    if( pcon->msigCount[BISHOP|co] != 1 )
        return false;
    // the bishop can not win
    ret.recog_validation = recog_allways_valid;
    ret.win_chance[co] = WIN_CHANCE_NONE;
    ret.bound = exact_bound;
    ret.score = 0;
    return true;
}

static bool recogKBK(PCON pcon, PSTE pste, recog_result &ret) {
    return evalKBK(pcon, pste, ret, W);
}
static bool recogKKB(PCON pcon, PSTE pste, recog_result &ret) {
    return evalKBK(pcon, pste, ret, B);
}

// --------------------------------------------------------------------------

static bool evalKNK(const CON *pcon, const STE *pste, recog_result &ret, int co) {
    // the knight can not win
    ret.bound = exact_bound;
    ret.score = 0;
    if( pcon->msigCount[KNIGHT|co] == 1) {
        ret.recog_validation = recog_allways_valid;
        ret.win_chance[co] = WIN_CHANCE_NONE;
        return true;
    }
    // need validation
    if( pcon->msigCount[KNIGHT|co] == 2 ) {
        ret.win_chance[co] = WIN_CHANCE_NONE;
        return true;
    }
    return false;
}

static bool recogKNK(PCON pcon, PSTE pste, recog_result &ret) {
    return evalKNK(pcon, pste, ret, W);
}
static bool recogKKN(PCON pcon, PSTE pste, recog_result &ret) {
    return evalKNK(pcon, pste, ret, B);
}

// --------------------------------------------------------------------------
//
// co is the winning side
//
static bool evalKPK(const CON *pcon, const STE *pste, recog_result &ret, int co) {
    if( pcon->msigCount[PAWN|co] == 1 ) {
        bitboard pawns = pcon->pieces[PAWN|co];
        int isqPawn = PopLSB(pawns) ;
        int nc = co ^ 1 ;
        isqPawn = remapSq[nc][ isqPawn ];

        int isqBKing = remapSq[nc][pcon->king[co ^ 1]];
        int isqWKing = remapSq[nc][pcon->king[co]];
        int rankPawn = Rank( isqPawn );
        int thisIdx = idxFromSq( isqWKing, isqBKing, isqPawn, co == W ? pste->side : pste->side ^ 1);
        int res = KPK_bitbase.get( thisIdx );
        int delta = - sqDistance( isqBKing, isqPawn ) * 2 + sqDistance( isqWKing, isqPawn ) * 1 + rankPawn * 8;
        switch( res ) {
            case 0:
                ret.bound = exact_bound;
                ret.score = 0;
    //            val = (25 + delta); // 0 gives less nodes but why not try to draw a lost game :p
                break;
            case 1:
                ret.bound = lower_bound;
                ret.score = 700 + delta;
                break;
            case -1:
                ret.bound = upper_bound;
                ret.score = -(700 + delta);
                break;
        }
        // allways valid when using a complete egt
        ret.recog_validation = recog_allways_valid;
        if( co != pste->side )
            invertScore( true, ret);
        return true;
    }
    if( (pcon->pieces[PAWN|co] & ~fileAMask) == 0 && sqDistance( pcon->king[co^1], promotionSqrFileA[co] ) <= 1 ) {
        ret.bound = exact_bound;
        ret.score = 0;
        return true;
    }
    if( (pcon->pieces[PAWN|co] & ~fileHMask) == 0 && sqDistance( pcon->king[co^1], promotionSqrFileH[co] ) <= 1 ) {
        ret.bound = exact_bound;
        ret.score = 0;
        return true;
    }

    return false;
}

static bool recogKPK(PCON pcon, PSTE pste, recog_result &ret) {
    return evalKPK(pcon, pste, ret, W);
}
static bool recogKKP(PCON pcon, PSTE pste, recog_result &ret) {
    return evalKPK(pcon, pste, ret, B);
}

// --------------------------------------------------------------------------

static bool evalKPKB(const CON *pcon, const STE *pste, recog_result &ret, int co) {
    ret.score = 0;
    ret.bound = exact_bound;
    // king in corner can be mated
    if( pcon->msigCount[BISHOP|(co^1)] != 1 )
        return false;
    // remove that
    // (many) pawns on file A
    if( (pcon->pieces[PAWN|co] & ~fileAMask) == 0 )
        // if opposite king is in the corner then we can not win
        if( sqDistance( pcon->king[co^1], promotionSqrFileA[co] ) <= 1 ) {
            ret.recog_validation = recog_allways_valid;
            return true;
        }
    // (many) pawns on file H
    if( (pcon->pieces[PAWN|co] & ~fileHMask) == 0 )
        // if opposite king is in the corner then we can not win
        if( sqDistance( pcon->king[co^1], promotionSqrFileH[co] ) <= 1 ) {
            ret.recog_validation = recog_allways_valid;
            return true;
        }

    bitboard pawns = pcon->pieces[PAWN|co];
    if( pcon->msigCount[PAWN|co] != 1 )
        return false;
    // the bishop will not win
    int isqPawn = LSB( pawns ) ;
    bitboard pFrontSpan = 0;
    if( co == W )
        pFrontSpan = wFrontSpans( pawns );
    else
        pFrontSpan = bFrontSpans( pawns );

    // bishop attacks pawn path
    if( BishopAttacksStuff(pcon, pFrontSpan, FlipColor(co) ) ) {
        ret.bound = exact_bound;
        ret.score = 0;
        return true;
    }

    // defending king in front of pawn
    if( pFrontSpan & IdxToU64( pcon->king[co^1] ) ) {
        ret.recog_validation = recog_allways_valid;
        return true;
    }
    ret.bound = score_bound;
    ret.score = 50 - 6*distanceToPromotionSqr( isqPawn, co )  - 3*sqDistance( pcon->king[co], isqPawn);

    ret.recog_validation = recog_need_validation;
    if( co != pste->side )
        invertScore( true, ret);
    return true;
}
static bool recogKPKB(PCON pcon, PSTE pste, recog_result &ret) {
    return evalKPKB(pcon, pste, ret, W);
}
static bool recogKBKP(PCON pcon, PSTE pste, recog_result &ret) {
    return evalKPKB(pcon, pste, ret, B);
}
// --------------------------------------------------------------------------
// B + pawn on a/h file vs nothing
static bool evalKBPK(const CON *pcon, const STE *pste, recog_result &ret, int co) {
    ret.score = 0;
    ret.bound = exact_bound;
    // king in corner can be mated
    if( pcon->msigCount[BISHOP|co] != 1 )
        return false;

    int bSqr = LSB(pcon->pieces[BISHOP|co]) ;
    // (many) pawns on file A
    if( ((pcon->pieces[PAWN|co] & ~fileAMask) == 0) && !sameSquareColor(promotionSqrFileA[co], bSqr) )
        // if opposite king is in the corner then we can not win
        if( sqDistance( pcon->king[co^1], promotionSqrFileA[co] ) <= 1 ) {
            ret.recog_validation = recog_allways_valid;
            return true;
        }
    // (many) pawns on file H
    if( ((pcon->pieces[PAWN|co] & ~fileHMask) == 0)  && !sameSquareColor(promotionSqrFileH[co], bSqr) )
        // if opposite king is in the corner then we can not win
        if( sqDistance( pcon->king[co^1], promotionSqrFileH[co] ) <= 1 ) {
            ret.recog_validation = recog_allways_valid;
            return true;
        }

    return false;
}
static bool recogKBPK(PCON pcon, PSTE pste, recog_result &ret) {
    return evalKBPK(pcon, pste, ret, W);
}
static bool recogKKBP(PCON pcon, PSTE pste, recog_result &ret) {
    return evalKBPK(pcon, pste, ret, B);
}
// --------------------------------------------------------------------------

static bool evalKPKN(const CON *pcon, const STE *pste, recog_result &ret, int co) {
    ret.score = 0;
    ret.bound = exact_bound;
    // king in corner can be mated
    if( pcon->msigCount[KNIGHT|(co^1)] != 1)
        return false;
    if( (pcon->pieces[PAWN|co] & ~fileAMask) == 0 )
        // if opposite king is in the corner then we can not win
        if( sqDistance( pcon->king[co^1], promotionSqrFileA[co] ) <= 1 ) {
            ret.recog_validation = recog_allways_valid;
            return true;
        }
    // pawns on file H
    if( (pcon->pieces[PAWN|co] & ~fileHMask) == 0 )
        // if opposite king is in the corner then we can not win
        if( sqDistance( pcon->king[co^1], promotionSqrFileH[co] ) <= 1 ) {
            ret.recog_validation = recog_allways_valid;
            return true;
        }
    if( pcon->msigCount[PAWN|co] != 1 )
        return false;
    // the knight will not win
    bitboard pawns = pcon->pieces[PAWN|co];
    int isqPawn = LSB(pawns) ;
    bitboard pFrontSpan = 0;
    if( co == W )
        pFrontSpan = wFrontSpans( pawns );
    else
        pFrontSpan = bFrontSpans( pawns );

    // defending king in front of pawn
    if( pFrontSpan & IdxToU64( pcon->king[co^1] ) ) {
        ret.recog_validation = recog_allways_valid;
        return true;
    }

    ret.score = 75 - 6*distanceToPromotionSqr( isqPawn, co ) - 3*sqDistance( pcon->king[co], isqPawn) 
        + 8*sqDistance( pcon->king[co^1], isqPawn);

//    ret.bound = lower_bound;
    ret.bound = score_bound;
    ret.recog_validation = recog_need_validation;
    if( co != pste->side )
        invertScore( true, ret);
    return true;
}
static bool recogKPKN(PCON pcon, PSTE pste, recog_result &ret) {
    return evalKPKN(pcon, pste, ret, W);
}
static bool recogKNKP(PCON pcon, PSTE pste, recog_result &ret) {
    return evalKPKN(pcon, pste, ret, B);
}
// --------------------------------------------------------------------------

static bool evalKQKP(const CON *pcon, const STE *pste, recog_result &ret, int co) {
    // 
    if( pcon->msigCount[PAWN|(co^1)] != 1 || pcon->msigCount[QUEEN|(co)] != 1)
        return false;

    ret.recog_validation = recog_need_validation;
    // 
    int isqPawn = LSB(pcon->pieces[PAWN|(co^1)]) ;
    if( distanceToPromotionSqr(isqPawn, co^1 ) == 1 
        && distanceToPromotionSqr(pcon->king[co^1], co^1 ) <= 2 ) {
        ret.score = 200 - 10*sqDistance( pcon->king[co], isqPawn )
            + 5*sqDistance( pcon->king[co^1], isqPawn );
//        ret.bound = lower_bound;
        // no idea, dont return a bound
        ret.bound = score_bound;
        if( co != pste->side )
            invertScore( true, ret);
        return true;
    }
    return false;
}
static bool recogKQKP(PCON pcon, PSTE pste, recog_result &ret) {
    return evalKQKP(pcon, pste, ret, W);
}
static bool recogKPKQ(PCON pcon, PSTE pste, recog_result &ret) {
    return evalKQKP(pcon, pste, ret, B);
}
// --------------------------------------------------------------------------

static bool evalKBKB(const CON *pcon, const STE *pste, recog_result &ret, int co) {
    // king in corner can be mated
    if( pcon->msigCount[BISHOP|co] > 2 || pcon->msigCount[BISHOP|(co^1)] > 2)
        return false;

    ret.recog_validation = recog_need_validation;
    if( pcon->msigCount[BISHOP|co] == 1 && pcon->msigCount[BISHOP|(co^1)] == 1) {
        ret.bound = exact_bound;
        ret.score = 0;
        return true;
    }
    ret.bound = score_bound;
    ret.score = 0;
    if( co != pste->side )
        invertScore( true, ret);
    return true;
}
static bool recogKBKB(PCON pcon, PSTE pste, recog_result &ret) {
    return evalKBKB(pcon, pste, ret, pcon->msigCount[WB] >= pcon->msigCount[BB] ? W : B);
}

// --------------------------------------------------------------------------

static bool evalKRKR(const CON *pcon, const STE *pste, recog_result &ret, int co) {

    ret.recog_validation = recog_need_validation;
    if( pcon->msigCount[ROOK|co] == 1 && pcon->msigCount[ROOK|(co^1)] == 1) {
        // 8/8/3R3k/5K2/7r/8/7P/8 b - - 16 64 
//        ret.bound = exact_bound;
        ret.bound = score_bound;
        ret.score = 0;
        return true;
    }
    return false;
}
static bool recogKRKR(PCON pcon, PSTE pste, recog_result &ret) {
    return evalKRKR(pcon, pste, ret, pcon->msigCount[WR] >= pcon->msigCount[BR] ? W : B);
}

// --------------------------------------------------------------------------

static bool evalKQKQ(const CON *pcon, const STE *pste, recog_result &ret, int co) {

    if( pcon->msigCount[QUEEN|co] != 1 || pcon->msigCount[QUEEN|(co^1)] != 1)
        return false;

    // special positions
    ret.recog_validation = recog_need_validation;
    ret.bound = score_bound;
    ret.score = 0;
    return true;
}
static bool recogKQKQ(PCON pcon, PSTE pste, recog_result &ret) {
    return evalKQKQ(pcon, pste, ret, pcon->msigCount[WQ] >= pcon->msigCount[BQ] ? W : B);
}

// --------------------------------------------------------------------------

static bool evalKQKR(const CON *pcon, const STE *pste, recog_result &ret, int co) {
    // 
    if( pcon->msigCount[ROOK|(co^1)] != 1 || pcon->msigCount[QUEEN|(co)] != 1)
        return false;

    ret.recog_validation = recog_need_validation;
    int bSqr = LSB(pcon->pieces[ROOK|(co^1)]);

    ret.score = 500 - c_argdistborder[ pcon->king[co^1] ]
        - 8*sqDistance( pcon->king[co^1], pcon->king[co] ) 
        + 2*sqDistance( pcon->king[co^1], bSqr );
    if( pste->checkf && pste->side == co )
        ret.score -= 10;
    ret.bound = score_bound;
//    if( co == pste->side )
//        ret.bound = lower_bound;
    if( co != pste->side )
        invertScore( true, ret);
    return true;
}
static bool recogKQKR(PCON pcon, PSTE pste, recog_result &ret) {
    return evalKQKR(pcon, pste, ret, W);
}
static bool recogKRKQ(PCON pcon, PSTE pste, recog_result &ret) {
    return evalKQKR(pcon, pste, ret, B);
}
// --------------------------------------------------------------------------

static bool evalKRKB(const CON *pcon, const STE *pste, recog_result &ret, int co) {

    if( pcon->msigCount[ROOK|(co)] != 1 || pcon->msigCount[BISHOP|FlipColor(co)] != 1)
        return false;

    ret.recog_validation = recog_need_validation;

    bitboard bishops = pcon->pieces[BISHOP|FlipColor(co)];
    int isqBishop = LSB(bishops);
    int nc = co ^ 1 ;
    int isqBKing = pcon->king[co ^ 1];
    int idxBKing = isqBKing;
    int isqWKing = pcon->king[co];
    int idxWKing = isqWKing;

    if( colorOfSquare( isqBishop) == 0 ) {
        idxBKing = Idx(Rank(isqBKing), 7-File(isqBKing));
        idxWKing = Idx(Rank(isqWKing), 7-File(isqWKing));
    }

    ret.score = ( 70 - c_argdistcorner[idxBKing]  - sqDistance(isqWKing, isqBKing) * 4 );
    ret.score /= 6;

    ret.bound = score_bound;
    if( co != pste->side )
        invertScore( true, ret);
    return true;
}
static bool recogKRKB(PCON pcon, PSTE pste, recog_result &ret) {
    return evalKRKB(pcon, pste, ret, W);
}
static bool recogKBKR(PCON pcon, PSTE pste, recog_result &ret) {
    return evalKRKB(pcon, pste, ret, B);
}
// --------------------------------------------------------------------------

static bool evalKRKN(const CON *pcon, const STE *pste, recog_result &ret, int co) {

    if( pcon->msigCount[ROOK|(co)] != 1 || pcon->msigCount[KNIGHT|FlipColor(co)] != 1)
        return false;

    ret.recog_validation = recog_need_validation;

    bitboard knights = pcon->pieces[KNIGHT|FlipColor(co)];
    int isqKnight = LSB(knights);
    int nc = co ^ 1 ;
    int isqBKing = pcon->king[co ^ 1];
    int isqWKing = pcon->king[co];

    ret.score = ( 40 - c_argdistborder[isqBKing] - sqDistance(isqWKing, isqBKing) + sqDistance(isqBKing, isqKnight) * 4);
    ret.score /= 8;

    ret.bound = score_bound;
    if( co != pste->side )
        invertScore( true, ret);
    return true;
}
static bool recogKRKN(PCON pcon, PSTE pste, recog_result &ret) {
    return evalKRKN(pcon, pste, ret, W);
}
static bool recogKNKR(PCON pcon, PSTE pste, recog_result &ret) {
    return evalKRKN(pcon, pste, ret, B);
}

// --------------------------------------------------------------------------
static bool evalKBPKB(const CON *pcon, const STE *pste, recog_result &ret, int co) {
    // king in corner can be mated
    if( (pcon->msigCount[BISHOP|co] != 1) || (pcon->msigCount[BISHOP|(co^1)] != 1))
        return false;
    // can be more than one pawn if they are on the same file
    PHASHPawn phashp;
    EvalPawns(pcon, pste, phashp);
    const U8 *c_pawn= phashp->side[co].c_pawn;
    if( (c_pawn[0] + c_pawn[1] + c_pawn[2] + c_pawn[3] + c_pawn[4] + c_pawn[5] + c_pawn[6] + c_pawn[7]) != 1 )
//    if( (pcon->msigCount[PAWN|co] != 1) )
        return false;

    ret.recog_validation = recog_need_validation;
    ret.bound = score_bound;
    ret.score = 0;
    const int wSqr = LSB(pcon->pieces[BISHOP|(co)]);
    const int bSqr = LSB(pcon->pieces[BISHOP|(co^1)]);
    int isqPawn = LSB(pcon->pieces[PAWN|co]) ;
    bitboard kingBackPawn;
    bitboard defKing = IdxToU64(pcon->king[co^1]);
    if(co == B )    // looking for pawns more advanced than the defending king
        kingBackPawn = bFrontSpans(defKing);
    else
        kingBackPawn = wFrontSpans(defKing);
    int fileDist = abs( File(pcon->king[co^1]) - File(isqPawn) );
    // defending king in front of pawn
    if( (fileDist == 0) && !sameSquareColor(pcon->king[co^1], wSqr) &&
//        (((co == W) && Rank(pcon->king[co^1]) > Rank(isqPawn)) || ((co == B) && Rank(pcon->king[co^1]) < Rank(isqPawn)))) {
        (kingBackPawn & pcon->pieces[PAWN|co]) == 0) {
        ret.bound = exact_bound;
        ret.score = 0;
        return true;
    }
    int promSqr = promotionSqr(isqPawn, co);
    if( (sqDistance( promSqr, pcon->king[co^1]) <= 2 ) && (sqDistance( promSqr, pcon->king[co]) > 3 ) ) {
        ret.win_chance[co] = WIN_CHANCE(50);
        return false;
    }
    if( (pcon->msigCount[PAWN|co] != 1) )
        return false;
    ret.score = 50 - 6*distanceToPromotionSqr( isqPawn, co ) - 3*sqDistance( pcon->king[co], isqPawn);

    // bishop attacks pawn path
    bitboard pFrontSpan = 0;
    if( co == W )
        pFrontSpan = wFrontSpans( pcon->pieces[PAWN|co] );
    else
        pFrontSpan = bFrontSpans( pcon->pieces[PAWN|co] );
    if( BishopAttacksStuff(pcon, pFrontSpan, FlipColor(co) ) ) {
        ret.score /= 2;
        if( !sameSquareColor( wSqr, bSqr) ) {
            ret.bound = exact_bound;
            ret.score = 0;
            return true;
        }
    }

    ret.bound = score_bound;
    if( co != pste->side )
        invertScore( true, ret);
    return true;
}
static bool recogKBPKB(PCON pcon, PSTE pste, recog_result &ret) {
    return evalKBPKB(pcon, pste, ret, W);
}
static bool recogKBKBP(PCON pcon, PSTE pste, recog_result &ret) {
    return evalKBPKB(pcon, pste, ret, B);
}

// --------------------------------------------------------------------------

static bool evalKRPKR(const CON *pcon, const STE *pste, recog_result &ret, int co) {

    if( (pcon->msigCount[PAWN|co] != 1) || (pcon->msigCount[ROOK|co] != 1) ||
        (pcon->msigCount[ROOK|(co^1)] != 1))
        return false;

    ret.recog_validation = recog_need_validation;
    ret.bound = score_bound;
    ret.score = 0;
    int wSqr = LSB(pcon->pieces[ROOK|(co)]);
    int bSqr = LSB(pcon->pieces[ROOK|(co^1)]);
    int isqPawn = LSB(pcon->pieces[PAWN|co]) ;
    int wpF = File( isqPawn );
    int bkF = File( pcon->king[co^1] );
    int bkR = Rank( pcon->king[co^1] );
    ret.score = 50 - 6*distanceToPromotionSqr( isqPawn, co ) - 5*sqDistance( pcon->king[co], isqPawn) 
        + 6*sqDistance( pcon->king[co^1], isqPawn);
    ret.bound = score_bound;
    if( (co == W && bkR < Rank( wSqr ) && Rank(isqPawn) > Rank( wSqr )) 
        || (co == B && bkR > Rank( wSqr ) && Rank(isqPawn) < Rank( wSqr )) ) {
        // it's nearly won, king cut by rank
        ret.score += 100 - 5*distanceToPromotionSqr( isqPawn, co );
        ret.bound = score_bound;
    }
    int cutF = 0;
    if( wpF < bkF && File( wSqr ) > wpF && File( wSqr ) < bkF)
        cutF = File( wSqr ) - wpF;
    if( wpF > bkF && File( wSqr ) < wpF && File( wSqr ) > bkF)
        cutF = wpF - File( wSqr );
    int d = distanceToPromotionSqr( isqPawn, co );
    int pR = 7 - d; // pawn rank
    if( cutF != 0 && wpF != filA && wpF != filH ) {
        if( sqDistance( pcon->king[co], isqPawn) <= 1 && (pR >= 5 ) ) {
            int wrR = Rank( wSqr );
            if( co == B )
                wrR = 7 - wrR;
            ret.score += 150 - 5 * d + (cutF >= 2) * 64 + (wrR >= 3 && wrR <= 5) * 64;
        }
        else if( (pR == 3 || pR == 4) && (wpF >= filC && wpF <= filF) ) {
            if( (pR == 4 && cutF >= 2) || (pR == 3 && cutF >= 3) )
                ret.score += 150 - 5 * d;
        } else if( (pR == 3 || pR == 4) && (wpF == filB || wpF == filG) ) {
            if( cutF >= 3 )
                ret.score += 150 - 5 * d;
        }
    }
    // TODO : backrank defense
    if( co != pste->side )
        invertScore( true, ret);
    return true;
}
static bool recogKRPKR(PCON pcon, PSTE pste, recog_result &ret) {
    return evalKRPKR(pcon, pste, ret, W);
}
static bool recogKRKRP(PCON pcon, PSTE pste, recog_result &ret) {
    return evalKRPKR(pcon, pste, ret, B);
}

// --------------------------------------------------------------------------

bool recogKK(PCON pcon, PSTE pste, recog_result &ret) {
    ret.recog_validation = recog_allways_valid;
    ret.win_chance[W] = WIN_CHANCE_NONE;
    ret.win_chance[B] = WIN_CHANCE_NONE;
    ret.score = 0;
    ret.bound = exact_bound;
    return true;
}

// --------------------------------------------------------------------------

//msig(B,N,Q,R,P) ((B)|(N)<<1|(Q)<<2|(R)<<3|(P)<<4)
#define msigK   msig(0,0,0,0,0)
#define msigB   msig(1,0,0,0,0)
#define msigN   msig(0,1,0,0,0)
#define msigQ   msig(0,0,1,0,0)
#define msigR   msig(0,0,0,1,0)
#define msigP   msig(0,0,0,0,1)

//  K   K   *
//  KB  K   *
//  KN  K   *
//  KBB K
//  KNN K   *
//  KBN K
//  KP  K   *
//  KP  K   a/h file
//  KQ  KQ  x
//  KP  KB  x
//  KP  KN  x
//  KQ  KP  (a/c pawn)
//  KB  KB  x
//  KBP KB  x
//  KRP KR  x
//  KQ  KR

void init_recognizers(void) {
    for(int i = 0; i <= 31; i++) {
        recog_avail[i] = 0;
    }
    for(int i = 0; i <= 1023; i++) {
        recog_table[i] = 0;
    }

    new_recog(recogKK,  0, 0);
    new_recog(recogKBK,  msigB, 0);
    new_recog(recogKKB,  0, msigB);
    new_recog(recogKNK,  msigN, 0);
    new_recog(recogKKN,  0, msigN);
    new_recog(recogKPK,  msigP, 0);
    new_recog(recogKKP,  0, msigP);

    new_recog(recogKBPK,  msigP| msigB, 0);
    new_recog(recogKKBP,  0, msigP| msigB);

    new_recog(recogKPKB,  msigP, msigB);
    new_recog(recogKBKP,  msigB, msigP);
    new_recog(recogKPKN,  msigP, msigN);
    new_recog(recogKNKP,  msigN, msigP);
    new_recog(recogKBKB,  msigB, msigB);
    new_recog(recogKRKR,  msigR, msigR);

    new_recog(recogKRKB,  msigR, msigB);
    new_recog(recogKBKR,  msigB, msigR);
    new_recog(recogKRKN,  msigR, msigN);
    new_recog(recogKNKR,  msigN, msigR);

    new_recog(recogKQKQ,  msigQ, msigQ);
    new_recog(recogKBPKB,  msigB|msigP, msigB);
    new_recog(recogKBKBP,  msigB, msigB|msigP);
    new_recog(recogKQKP,  msigQ, msigP);
    new_recog(recogKPKQ,  msigP, msigQ);
    new_recog(recogKQKR,  msigQ, msigR);
    new_recog(recogKRKQ,  msigR, msigQ);
    new_recog(recogKRPKR,  msigR|msigP, msigR);
    new_recog(recogKRKRP,  msigR, msigR|msigP);

//    new_recog(evalKQKR, msig(0,0,1,0,0), msig(0,0,0,1,0));
}
