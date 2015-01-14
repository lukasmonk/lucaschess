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


#ifndef _EVAL_HXX
#define _EVAL_HXX

const bitboard fileAMask = 0x0101010101010101ULL;
const bitboard fileBMask = 0x0202020202020202ULL;
const bitboard fileCMask = 0x0404040404040404ULL;
const bitboard fileDMask = 0x0808080808080808ULL;
const bitboard fileEMask = 0x1010101010101010ULL;
const bitboard fileFMask = 0x2020202020202020ULL;
const bitboard fileGMask = 0x4040404040404040ULL;
const bitboard fileHMask = 0x8080808080808080ULL;

const bitboard QSideMask = fileAMask | fileBMask | fileCMask;
const bitboard CSideMask = fileDMask | fileEMask;
const bitboard KSideMask = fileFMask | fileGMask | fileHMask;

const bitboard Rank1Mask = 0xFFULL;
const bitboard Rank2Mask = 0xFF00ULL;
const bitboard Rank3Mask = 0xFF0000ULL;
const bitboard Rank4Mask = 0xFF000000ULL;
const bitboard Rank5Mask = 0xFF00000000ULL;
const bitboard Rank6Mask = 0xFF0000000000ULL;
const bitboard Rank7Mask = 0xFF000000000000ULL;
const bitboard Rank8Mask = 0xFF00000000000000ULL;

// the 16 center pawns
const bitboard centerMask = 0x00003C3C3C3C0000ULL;

// Pawn structure hash element
// The pawn hash contains informations and *static* eval of the pawn structure
// The real value of the pawn structure depends of the complete position (and side
// to move), this value is not integrated in the val tag
// The eval() function will use the pawn structure information to recompute the
// real value depending on the phase of the game

//#define KEYSQUARE

#define KINGSIDE_QS 0
#define KINGSIDE_C  1
#define KINGSIDE_KS 2

typedef struct {
    int distance[2][64];
    int square;
    int winningSide;
} squareDist;

struct KingShelterInfo {
    short   val;
//    short   acount;
};

typedef	struct tagHASHP {
    HASHK	hashk;				// Pawn key (other pieces not in here).
    struct tagStructure {
        bitboard candidate;     // candidate pawn
        bitboard passed;        // passed pawns
        bitboard pawnatk;       // squares attack by pawns
        KingShelterInfo KSI[8];
        short   val;
        short   endgval;

        U8 c_pawn[8];           // pawn exists on file (bool 0/1)
        U8 c_center;            // count of attacked center squares
//        U8 c_Qspace;            // count of 'space' on each flank
//        U8 c_Cspace;
//        U8 c_Kspace;
        U8 c_space;             // Q+C+K space
    } side[2];                  // this is for each side
#ifdef KEYSQUARE
    squareDist  keySquare[3];
#endif
    bool    pawnon6or7;
    U8      c_center_pawns;     // number of pawns in the central square
} HASHPawn, * PHASHPawn;

extern int pathLength(U64 sq1, U64 sq2, U64 path);
extern void EvalPawns(const CON *pcon, const STE *pste, PHASHPawn & phashp);
extern int const remapSq[2][64];
extern const int exchangePieceValue[16];
//extern int	const staticPosVal[6+2][64];
extern unsigned int pawn_bonus_to_7[coMAX][8];
extern int pstVal[12][64];

extern U8 squareDistance[64][64];
#define sqDistance( isqFrom, isqTo ) squareDistance[isqFrom][isqTo]

extern int see(PCON pcon, const STE *pste, int from, int to);
// for checks in QS, move is allready played
int see(const CON *pcon, const STE *pste, int to);
extern void initEG(PCON pcon, PSTE pste);
bool KPK(PCON pcon, PSTE pste, int co, int & val);
bool KBNK(PCON pcon, PSTE pste, int co, int & val);
#ifdef _DEBUG
void VDumpPawnMaps(char * szFmt, ...);
#endif

#define WIN_CHANCE(n)       ((n)*256/100)
#define WIN_CHANCE_LOW      WIN_CHANCE(20)
#define WIN_CHANCE_DRAWISH  WIN_CHANCE(10)
#define WIN_CHANCE_NONE     WIN_CHANCE(0)


// -- recognizers
//
// non bitbase/tablebase evaluation can only be done on quiescent position
// if the position is not quiescent the recognizer will only return a score, not a bound
typedef enum {fail_bound, lower_bound, upper_bound, exact_bound, score_bound} recog_bound;
typedef enum {recog_need_validation, recog_allways_valid} recog_validation_type;
typedef struct {
    recog_bound             bound;
    int                     score;
    recog_validation_type   recog_validation;
    int                     win_chance[2];
} recog_result;
typedef bool (*recog)(PCON pcon, PSTE pste, recog_result &ret);

extern U32 recog_avail[32];    // this is 32*32 boolean
//extern recog recog_table[64];
extern recog recog_table[1024];
extern bitboard pawnFileMask[8];
extern const int sideForFile[8];

#define recog_available(w_mstate, b_mstate) ((recog_avail[(w_mstate)]>>(b_mstate))&1)
//#define recog_index(b_mstate, w_mstate) (((b_mstate)|(w_mstate))+32*((b_mstate) && (w_mstate)))
#define recog_index(w_mstate, b_mstate) ((w_mstate)*32+(b_mstate))

#define msig(B,N,Q,R,P) ((B)|(N)<<1|(Q)<<2|(R)<<3|(P)<<4)
unsigned inline msigToIdx( msigT & sig ) {
    return msig(sig.msB, sig.msN, sig.msQ, sig.msR , sig.msP );
}

extern void init_recognizers(void);
extern bool positionOkForRecog(const CON *pcon, const STE *pste);
extern bool probe_bitbases(CON *pcon, const STE *pste, int player, int& score);
extern void prepareEGBB(void);

#define ValIsOk(val)    ((val) >= -32767 && (val) <= 32767)
#endif // _EVAL_HXX
