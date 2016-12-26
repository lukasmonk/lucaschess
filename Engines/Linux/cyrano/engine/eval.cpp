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
#include "moves.hpp"
#include "magicmoves.h"
#include "genmagic.hpp"
#include "eval.hpp"
#include "stats.hpp"
#include "hash.hpp"
#include <stdlib.h>     // for dummy rand()
#include <math.h>

#ifdef _DEBUG
static char const s_aszModule[] = __FILE__;
#endif

//#define C4_EVAL
//#define OLD_MOBILITY
//#define XRAY_MOBILITY
#ifdef C4_EVAL
    // cyrano 0.4 emulation
    #define bRook7thMG      (0+8)
    #define bRook7thEG      25
    #define bRookSemiOpen   5
    #define bRookOpen       5   // added to bRookSemiOpen
#else
    // cyrano 0.5 test value
    // queen eg bonus on 7th ?
    #define bQueen7thMG     5       // was 20
    #define bQueen7thEG     20
    #define bRook7thMG      (10)    // was 20
    #define bRook7thEG      30      // check that, but a high value is needed
    #define bRookSemiOpen   10
    #define bRookOpen       10   // added to bRookSemiOpen
    #define MOB_INCLUDE_DEF
#endif

#ifdef _DEBUG
#define DODEBUG(i)  i
static struct mobDebug {
    int piece;
    int mobility;
} mobDebugTable[32];
static int mobDebugIdx;
#else
#define DODEBUG(i)
#endif

const int promotionSqrFileA[2] = {A1,A8};
const int promotionSqrFileH[2] = {H1,H8};

static bitboard attackMap[coMAX];
static bitboard pawnAttacks[coMAX];
static int posval[coMAX];       // position value
static int otherval[coMAX];     // other value ?
static int endgval[coMAX];      // end game values
static int notpawn[coMAX];
//static int attackval[coMAX];
DODEBUG(static int c_mobility[coMAX];)
DODEBUG(static int prev_posval[coMAX];)
DODEBUG(static int prev_endgval[coMAX];)
static int mobility[coMAX];
static int defSpace[coMAX];
//static int pawnDist[coMAX];
static bitboard knight_moves[coMAX];
static bitboard tmp_attack[coMAX];
static int bishopSquare[coMAX];
static PHASHPawn phashp;
static int mat[coMAX];  // material
static bool kingDanger[2];
//static int underdev[coMAX];
//
static bitboard kingRing[coMAX];
static bool kingAtkFSlider[coMAX];
static int c_kingAtk[coMAX];
static int kingAtkVal[coMAX];
//static int c_kingDef[coMAX];
static int safetyval[coMAX];
static bitboard queen_castle_attack[coMAX];
//
//static bitboard moveBlocker[coMAX];
//static bitboard targets[coMAX];
//static int c_attack[coMAX];
//
DODEBUG(static int valKingCastle[coMAX];);

// attacker type sorted by value
#define ATK_PAWN    1
#define ATK_MINOR   2
#define ATK_ROOK    2
#define ATK_QUEEN   3

//#define ATK_KING    0x01

/* piece code defines */
#define APC_no_piece 0
#define APC_pawn 1
#define APC_knight 2
#define APC_bishop 3
#define APC_rook 4
#define APC_queen 5
#define APC_king 6
#define APC_bpawn 7
#define APC_BLACKKING   12

DODEBUG(static char stringPc[] = "KkQqRrBbNnPp";)



//#define mobK(n)     (n)
#define mobK(n)     ((3*n)/4)   // reduce since no safety eval
#define queenK(n)   ((n)/2)
#define rookK(n)   ((3*mobK(n))/4)
#ifdef OLD_MOBILITY

// mobility adjusted with unsafe mobility
static int KnightMobility[1+8] = {mobK(-15),mobK(-10),mobK(-5),mobK(0),mobK(7),
    mobK(12),mobK(15),mobK(15),mobK(15)};

// 14 * ((64-32) / 64) => 7,        14 * ((64-20)/64) => 9.6
static int BishopMobility[1+14] = {mobK(-15),mobK(-12),mobK(-10),mobK(-7),mobK(-4),mobK(0),mobK(5),
    mobK(10),mobK(12),mobK(15),mobK(20),mobK(20),mobK(20),mobK(20),mobK(20)};

// first and last ranks don't really count
// 14 * ((48-32)/48)   => 4.6,      14 * ((48-20)/48) => 8.1
//static int RookMobility[1+14] = {mobK(-30),mobK(-20),mobK(-10),mobK(-5),mobK(0),mobK(5),mobK(10),
//    mobK(15),mobK(20),mobK(22),mobK(25),mobK(27),mobK(30),mobK(30),mobK(30)};
static int RookMobility[1+14] = {rookK(-20),rookK(-10),rookK(-5),rookK(0),rookK(5),rookK(10),
    rookK(15),rookK(20),rookK(22),rookK(25),rookK(25),rookK(25),rookK(25),rookK(25),rookK(25)};

// bishop+rook 32 => 11.6       20 => 17.7
static int QueenMobility[1+28] = {queenK(-25),queenK(-19),queenK(-13),queenK(-8),queenK(-3),queenK(1),queenK(4),queenK(7),
/*8*/    queenK(10),queenK(12),queenK(14),queenK(16),queenK(17),queenK(18),queenK(19),queenK(20),queenK(21),queenK(22),queenK(23),
/*19*/    queenK(23),queenK(24),queenK(25),queenK(26),queenK(27),queenK(28),queenK(30),queenK(30),queenK(30),queenK(30)};

#else
// the new mobility uses linear values 
// (non linear values for low mobility is too pessimistic and results in decreasing value of eval at deeper search)
// base value
static int KnightMobility[1+8];
static int BishopMobility[1+14];
static int RookMobility[1+14];
static int QueenMobility[1+28];
#endif

//==============================================================================

static const bitboard rankMask[8] = {0xff, 0xff00, 0xff0000, 0xff000000, C64(0xff00000000), C64(0xff0000000000), C64(0xff000000000000), C64(0xff00000000000000)};
//const unsigned char castle_mask[2] = { 12, 3 };

// I don't want to use one array per color for positional values
// this array precomputes rank swap
int const remapSq[2][64] = {
    {
//  a   b   c   d   e   f   g   h
	0,	1,	2,	3,	4,	5,	6,	7,
	8,	9,	10,	11,	12,	13,	14,	15,
	16,	17,	18,	19,	20,	21,	22,	23,
	24,	25,	26,	27,	28,	29,	30,	31,
	32,	33,	34,	35,	36,	37,	38,	39,
	40,	41,	42,	43,	44,	45,	46,	47,
	48,	49,	50,	51,	52,	53,	54,	55,
	56,	57,	58,	59,	60,	61,	62,	63
    },{
	56,	57,	58,	59,	60,	61,	62,	63,
	48,	49,	50,	51,	52,	53,	54,	55,
	40,	41,	42,	43,	44,	45,	46,	47,
	32,	33,	34,	35,	36,	37,	38,	39,
	24,	25,	26,	27,	28,	29,	30,	31,
	16,	17,	18,	19,	20,	21,	22,	23,
	8,	9,	10,	11,	12,	13,	14,	15,
	0,	1,	2,	3,	4,	5,	6,	7
    }
};

//
//	These are piece-square tables.  The award bonuses (or penalties) in
//	centipawns if a piece is sitting on a particular square
//

/*
const int KING		= 0;    => 0
const int QUEEN		= 2;    => 1
const int ROOK		= 4;    => 2
const int BISHOP	= 6;    => 3
const int KNIGHT	= 8;    => 4
const int PAWN		= 10;   => 5
*/
int pstVal[12][64];
static int initPstVal[6][64] = {
    //
    //	White king.
    //
//  a   b   c   d   e   f   g   h
    -20,-20,-20,-20,-20,-20,-20,-20,
    -20,-20,-20,-20,-20,-20,-20,-20,
    -20,-20,-20,-20,-20,-20,-20,-20,
    -20,-20,-20,-20,-20,-20,-20,-20,
    -20,-20,-20,-20,-20,-20,-20,-20,
    -7,	-15,-15,-15,-15,-15,-15,-7,
    -5,	-5,	-15,-15,-15,-15,-5, -5,
     8,	10,	  8,-15, 0, -15,10, 10,
    //
    //	White queen.
    //
//  a   b   c   d   e   f   g   h
    -9,	-4,	0,	0,	0,	0,	-4,	-9,
    -9,	-4,	0,	0,	0,	0,	-4,	-9,
    -9,	-4,	0,	0,	0,	0,	-4,	-9,
    -9,	-4,	0,	0,	0,	0,	-4,	-9,
    -9,	-4,	0,	0,	0,	0,	-4,	-9,
    -9,	-4,	0,	0,	0,	0,	-4,	-9,
    -9,	-4,	0,	0,	0,	0,	-4,	-9,
    -9,	-4,	0,	0,	0,	0,	-4,	-9,
	//
	//	White rook.
	//
//  a   b   c   d   e   f   g   h
    -4,	0,	1,	2,	2,	1,	0,	-4,
    -4,	0,	1,	2,	2,	1,	0,	-4,
    -4,	0,	1,	2,	2,	1,	0,	-4,
    -4,	0,	1,	2,	2,	1,	0,	-4,
    -4,	0,	1,	2,	2,	1,	0,	-4,
    -4,	0,	1,	2,	2,	1,	0,	-4,
    -4,	0,	1,	2,	2,	1,	0,	-4,
    -4,	0,	1,	2,	2,	1,	0,	-4,
    //
    //	White bishop.
    //
//  a   b   c   d   e   f   g   h
    0,	0,	0,	0,	0,	0,	0,	0,
    0,	4,	4,  4,  4,	4,	4,	0,
    0,	4,	6,  8,  8,	6,	4,	0,
    0,	4,	6,  8,  8,	6,	4,	0,
    0,	4,	6,  8,  8,	6,	4,	0,
    0,	4,	6,  8,  8,	6,	4,	0,
    0,  7,	5,  5,  5,	5,  7,	0,
//	-15,-15,-15,-15,-15,-15,-15,-15,
    -12,-12,-12,-12,-12,-12,-12,-12,

    //
    //	White knight.
    //
//  a   b   c   d   e   f   g   h
    -9,-5,-5, -5, -5, -5, -5, -9,
    -4,	0,  2,	4,	4,  2,	0, -4,
    -4,	4,	4,  6,  6,	4,	4, -4,
    -2,	6,	6,  8,  8,	6,	6, -2,
    -2,	4,	5,	8,	8,	5,	4, -2,
    -8,	5,  8,	5,	5,  8,	5, -8,
    -8,	0,  2,  7,  7,  2,	0, -8,
    -9-8,-5-8, -5-8, -5-8, -5-8, -5-8, -5-8, -9-8,
    //
    //	White pawn.
    //
//  a   b   c   d   e   f   g   h
    0,	0,	0,	0,	0,	0,	0,	0,  // a8..h8
    0,	0,	0,	0,	0,	0,	0,	0,
    0,	0,	0,  0,  0,	0,	0,	0,
    0,	0,	0,  7,  7,	0,	0,	0,
    0,	0,	5, 10, 10,	5,	0,	0,
    0,	0,	0,	0,	0,	0,	0,	0,
    2,	2,	2,-10,-10,	2,	2,	2,
    0,	0,	0,	0,	0,	0,	0,	0,  // a1..h1
};
/*
//  a   b   c   d   e   f   g   h
    0,	0,	0,	0,	0,	0,	0,	0,  // a8..h8
    0,	0,	0,	4,	4,	0,	0,	0,
    1,	2,	4,  6,  6,	0,	0,	0,
    1,	2,	4,	7,	7,	2,	0,	0,
    2,	2,	5,  7,  7,	4,	0,	0,
    2,	2,	1,	2,	2,	0,	2,	0,
    5,	5,	2, -5, -5,	5,	5,	5,
    0,	0,	0,	0,	0,	0,	0,	0,  // a1..h1
*/



static const int knightOutpostVal[64] = {
    //
    //	White knight.
    //
//  a   b   c   d   e   f   g   h
	 0, 0, 0,  0,  0,  0,  0,  0,
	 0,	0,  3,  5,  5,  3,	0, 0,
	 0,	0,  5,  7,  7,  5,	0, 0,
	 0,	0,	5,  7,  7,	5,	0, 0,
	 0,	0,	2,	5,	5,	2,	0, 0,
	 0, 0, 0,  0,  0,  0,  0,  0,
	 0, 0, 0,  0,  0,  0,  0,  0,
	 0, 0, 0,  0,  0,  0,  0,  0,
};

//	The king is a weird piece because in the engame its eval is approximately
//	opposite of its eval in the middlegame or opening.  At the start of the
//	game, you want to hide your king in the corner, and at the end, you want
//	to run to the middle.

//	This problem is taken care of by having two tables.  If you try to make
//	things work with the middlegame table, the program will play like crap in
//	endings.

static int KingEGPosVal[64] = {
    //
    //	White king.
    //
//  a   b   c   d   e   f   g   h
    -5,	0,	0,	0,	0,	0,	0, -5,
    3,	6,	7,	8,	8,	7,	6,  3,
    4,	7,	8,	9,	9,	8,	7, 4,
    3,	6,	7,	8,	8,	7,	6, 3,
    2,	4,	5,	6,	6,	5,	4, 2,
    0,	2,	3,	4,	4,	3,	2, 0,
    -3,	0,	1,	2,	2,	1,	0, -3,
    -5, -2,-1,	-1,	-1,	-1, -2, -5,
};

//#define	sideK	0
//#define	sideC	1
//#define	sideQ	2

//#define	sideMAX	3

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

// minor value less when there is fewer pawns
//static int minor_eg_malus[] = {75,15,0,0,0,0,0,0,0};

// rank bonus for advancing pawns
//static int pawn_bonus_to_7[coMAX][8] = {{0,25,17,10,6,5,3,0}};
//unsigned int pawn_bonus_to_7[coMAX][8] = {{0,36,23,13,8,6,4,0}};
unsigned int pawn_bonus_to_7[coMAX][8] = {{0,30,23,13,8,6,4,0}};
static int pawn_dist_scale[coMAX][8] = {{32,26,20,14,8,4,0,0}};
//unsigned int pawn_bonus_to_7[coMAX][8] = {{0,42,28,13,8,6,4,0}};
//unsigned int pawn_bonus_to_7[coMAX][8] = {{0,40,35,25,17,11,5,0}};
// * 8 1,1.5,1.75,1.875
//static const unsigned int pawn_bonus_count[] = {8,12,14,15};
//static const unsigned int pawn_bonus_count[] = {12,16,18,19};
//static const unsigned int pawn_bonus_count[] = {11,13,16,17};
static const unsigned int pawn_bonus_count[] = {11,12,13,14};
//static const unsigned int pawn_bonus_count[] = {8,10,12,15};
// 1,1.333,1.444,1.48 : 8,10,12,12
//static const unsigned int pawn_bonus_count[] = {0,8,10,12,12};
// indexed by square content
static const int kingCastleDelay[16]={0,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7};
const int sideForFile[8] = {KINGSIDE_QS,KINGSIDE_QS,KINGSIDE_QS,KINGSIDE_C,KINGSIDE_C,KINGSIDE_C,KINGSIDE_KS,KINGSIDE_KS};
const U8 anyCastleForSide[2] = {cfE8G8|cfE8C8, cfE1G1|cfE1C1};
const U8 ksCastleForSide[2] = {cfE8G8, cfE1G1};
const U8 qsCastleForSide[2] = {cfE8C8, cfE1C1};
const int pawnDirection[2] = {-8,8};

// piece:Q+R+R+B+B+N+N + K
// full score when 8 pieces (start of game)
// 0 or near 0 when only the king left
// mid game position tables are worthless in end game
//#define MidGameValue(n,pop)     (((n)*(pop))/8)
//#define EndGameValue(n,pop)     (((n)*(8-pop))/8)
// ------------------------------? K+1+2+3+4+5+6+7 --- space for more pieces...
static int MidGameValueCoef[] = {0,0,0,2,4,6,7,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8};
static int EndGameValueCoef[] = {8,8,8,6,4,2,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
#define MidGameValue(n,pop)     (((n)*MidGameValueCoef[pop])/8)
#define EndGameValue(n,pop)     (((n)*EndGameValueCoef[pop])/8)

// indexed by color and piece square
static bitboard bishopTraps[2][64];
static bitboard bishopBlocked[2][64];
static bitboard outpostMask[2];
// kingSafetyWeight : 128 = 100%, 102=80%, 90=70%
static const int kingSafetyWeight = 128;
//static const int kingSafetyWeight = 102;
// kingSafetyDynamicWeight : piece activity and weak squares
//static const int kingSafetyDynamicWeight = (12 * kingSafetyWeight) / 128;
static const int kingSafetyDynamicWeight = (10 * kingSafetyWeight) / 128;
//static const int kingSafetyDynamicWeight = (8 * kingSafetyWeight) / 128;
static int kingSafetyCurve[16];
static int kingRingPressure[2];

U8 squareDistance[64][64];

//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-

static int OldSqDistance( int isqFrom, int isqTo ) {

    int dr = Rank( isqFrom ) - Rank( isqTo );
    int df = File( isqFrom ) - File( isqTo );
    if( dr < 0 ) dr = -dr;
    if( df < 0 ) df = -df;
    if( dr > df )
        return dr;
    return df;
}

static void initCenterDist(const char *tableName, int *table, float scale, float bias, float baseRank = 3.5f) {
    for(int sq = 0; sq < 64 ; sq++) {
        int f = File(sq);
        int r = 7-Rank(sq);
        float df = -fabsf(f - 3.5f) + 0.5f + bias;
        float dr = -fabsf(r - baseRank) + 0.5f + bias;
        float d = (df + dr) * scale;
        table[sq] = int(d);
    }
#ifdef DEBUG
    printf("PST %s\n", tableName);
    for(int r = 0; r <= 7 ; r++) {
        for(int f = 0; f <= 7 ; f++) {
            int sq = Idx(r,f);
            printf("%3d ", table[sq]);
        }
        printf("\n");
    }
#endif
}

// Build a mobility bonus table
// the entry with index zeroBonusMobility will have a zero value
// factor 16 so that zeroBonusMobility can be a non integer value

static void initMobility(int *table, float zeroBonusMobility, float bonusPerSquare, int arraySize) {
    for(int i = 0; i < arraySize; i++) {
        table[i] = int(16 * ( i - zeroBonusMobility ) * (bonusPerSquare+0.08f));
    }
}

void VInitMiddlegamePawnTable(void)
{
    // center value = scale * bias * 2
    initCenterDist( "King EG", KingEGPosVal, 6.0f, 1.0f );
//    initCenterDist( "Knight MG", initPstVal[BN/2], 6.0f, 1.0f );
//    initCenterDist( "Knight MG", initPstVal[BN/2], 6.0f, 1.0f, 4.0f );
    initCenterDist( "Knight MG", initPstVal[BN/2], 6.0f, 1.0f, 4.5f );

#ifndef OLD_MOBILITY

    // values to approximate old non linear value
    // bonus should be * 2/3
    initMobility( KnightMobility, 3.0f, 5*3/4.0f, 8+1 );
    initMobility( BishopMobility, 5.0f, 3.5f*3/4.0f, 14+1 );
    initMobility( RookMobility, 4.0f, 3.5f*3/4.0f*3/4.0f, 14+1 );
    initMobility( QueenMobility, 5.5f, 2.5f*3/4.0f/2.0f, 28+1 );

    // zero point will need to be adjusted when mobility will include attacking self pieces

    initMobility( KnightMobility, 4.0f, 4.0f, 8+1 );
    initMobility( BishopMobility, 6.0f, 5.0f, 14+1 );
    initMobility( RookMobility, 7.0f, 2.0f, 14+1 );
    initMobility( QueenMobility, 13.0f, 1.0f, 28+1 );

#ifdef MOB_INCLUDE_DEF
    initMobility( KnightMobility, 4.0f, 4.0f, 8+1 );
    initMobility( BishopMobility, 1+6.0f, 5.0f-0.5f-0.5f, 14+1 );
    initMobility( RookMobility, 1+7.0f, 2.0f-0.25f, 14+1 );
    initMobility( QueenMobility, 2+13.0f, 1.0f, 28+1 );

    initMobility( KnightMobility, 4.0f, 3.0f, 8+1 );
    initMobility( BishopMobility, 1+6.0f, 3.5f, 14+1 );
    initMobility( RookMobility, 1+7.0f, 1.5f, 14+1 );
    initMobility( QueenMobility, 13.0f, 1.0f, 28+1 );
/*//TEST
    initMobility( KnightMobility, 4.0f, 4.0f, 8+1 );
    initMobility( BishopMobility, 1+6.0f, 4.0f, 14+1 );
    initMobility( RookMobility, 1+7.0f, 2.0f, 14+1 );
    initMobility( QueenMobility, 13.0f, 1.0f, 28+1 );
*/
#endif

#endif

    // TODO:check that ==========================================================
    for(int i = 0; i < 8 ; i++) {
        pawn_bonus_to_7[coBLACK][i] = int(1.5f * pawn_bonus_to_7[coBLACK][i]);
        pawn_bonus_to_7[coWHITE][7-i] = pawn_bonus_to_7[coBLACK][i];
        pawn_dist_scale[coWHITE][7-i] = pawn_dist_scale[coBLACK][i];
    }
    for(int p = 0; p < 6;p++) {
        int cp = (p<<1);
        for(int sq = 0; sq < 64; sq++) {
            pstVal[cp+W][sq] = initPstVal[p][remapSq[W][sq]];
            pstVal[cp+B][sq] = initPstVal[p][remapSq[B][sq]];
        }
    }
#if 0
    for(int p = 0; p < 6;p++) {
        int cp = (p<<1);
        printf("%c\n", stringPc[cp]);
        for(int r = 7; r >= 0 ; r--) {
            for(int f = 0; f <= 7 ; f++) {
                int sq = Idx(r,f);
                printf("%3d ", pstVal[cp|B][sq]);
            }
            printf("\n");
        }
    }

#endif
    // bishop & knight malus on first rank (not just first square because of frc)
    for(int from = 0; from < 64; from++)
        for(int to = 0; to < 64; to++)
            squareDistance[from][to] = OldSqDistance(from,to);
    for(int sq = 0; sq < 64 ; sq++) {
        bishopTraps[W][sq] = 0;
        bishopBlocked[W][sq] = 0;
    }
    Set(B6,bishopTraps[W][A7]);
    Set(C7,bishopTraps[W][A7]);
    Set(G6,bishopTraps[W][H7]);
    Set(F7,bishopTraps[W][H7]);

    Set(E2,bishopBlocked[W][F1]);
    Set(G2,bishopBlocked[W][F1]);
    Set(D2,bishopBlocked[W][C1]);
    Set(B2,bishopBlocked[W][C1]);
    Set(B2,bishopBlocked[W][A1]); // frc
    Set(G2,bishopBlocked[W][H1]); // frc

    outpostMask[W] = 0;
    for(int r = 3; r <= 6; r++)
        for(int f = filC; f <= filF; f++)
            Set(Idx(r,f), outpostMask[W]);
    outpostMask[B] = flipVertical(outpostMask[W]);

//            if( (sqr == G1 && pos[F2] == WP) || (sqr == H2 && pos[G3] == WP) )
//                otherval[co] -= 125;

    for(int sq = 0; sq < 64 ; sq++) {
        bishopTraps[B][sq] = flipVertical(bishopTraps[W][sq^56]);
        bishopBlocked[B][sq] = flipVertical(bishopBlocked[W][sq^56]);
    }
    // 0;8;32;48;56;64
    for(int i = 0; i < 16 ; i ++ )
        kingSafetyCurve[i] = kingSafetyDynamicWeight*64;
    kingSafetyCurve[0] = 0 * kingSafetyDynamicWeight;
    kingSafetyCurve[1] = 8 * kingSafetyDynamicWeight;
    kingSafetyCurve[2] = 24* kingSafetyDynamicWeight; // 32=half value
    kingSafetyCurve[3] = 40* kingSafetyDynamicWeight;
    kingSafetyCurve[4] = 56* kingSafetyDynamicWeight;
    kingSafetyCurve[5] = 64* kingSafetyDynamicWeight;

}

//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-

// from Gerd
//static unsigned int colorOfSquare (unsigned int sq) {
//   return  (0xAA55AA55 >> sq) & 1;
//}
// from h.g.m
static bool inline sameSquareColor(unsigned int sq1, unsigned int sq2) {
    sq1 ^= sq2;
    return  ( (0xAA55AA55 >> sq1) & 1);
}

// does not check if there is two bishops
static bool bishopcanreach(const CON *pcon, int side, int sqr ) {
    bitboard bishops = pcon->pieces[BISHOP|side];
    if( bishops == 0 )
        return false;
    int bishop_sqr = LSB( bishops );
    return sameSquareColor(bishop_sqr, sqr);
}

//
// Square rule
// used to see a winning position with KP vs K
// and to give a bonus for passed pawn based on distance to king
//
static bool SquareRuleForPawn(const CON *pcon, const STE *pste, int co, int isqPawn, int & val) {
    // compute ranks from pawn to promotion
    // compute file dist from opposite king to pawn
    // if dist to promote < dist from king then win
    int myturn = pste->side != co;
    int isqKing = pcon->king[co ^ 1];
    int isqWKing = pcon->king[co];
    int distPawn;
    int rankKing = 7 - Rank( isqKing );
    int rankPawn = 7 - Rank( isqPawn );
    val = 0;
    // can not count if own king on pawn file
    if( File(isqWKing) == File(isqPawn) )
        return false;
    if( co != coWHITE ) {
        distPawn = 7 - rankPawn;
        rankKing = 7 - rankKing;
    } else
        distPawn = rankPawn;
    if( rankKing > (distPawn + myturn) ) {
        val = 750 + (7-distPawn)*4;
        return true;
    }
    int distFil = File( isqKing ) - File( isqPawn );
    if( distFil < 0 )
        distFil = - distFil;
    if( distFil >  (distPawn + myturn) ) {
        val = 750 + (7-distPawn)*4;
        return true;
    }
    return false;
}


static void inline mobDebugADD(int pc, int m) {
#ifdef _DEBUG
    mobDebugTable[mobDebugIdx].piece = pc;
    mobDebugTable[mobDebugIdx].mobility = m;
    mobDebugIdx++;
#endif
}

static void INLINE computeKingRing(const CON *pcon, const STE *pste, int co, bitboard & kingRing) {
    int sqr = pcon->king[co];
//    kingRing = king_attacks[sqr];
//    kingRing |= soutOne(kingRing);
//    kingRing |= nortOne(kingRing);
    kingRing = king_attacks_ring[sqr];
    if(pcon->msigCount[QUEEN|FlipColor(co)]) {
        if( co == B )
            kingRing |= soutOne(kingRing) & pcon->pieces[PAWN|B];
        else
            kingRing |= nortOne(kingRing) & pcon->pieces[PAWN|W];
        // add (advanced) pawns to the king ring
/*        int f = File( sqr );
        kingRing |= pcon->pieces[PAWN|co] & pawnFileMask[f];
        if( f > 0 )
            kingRing |= pcon->pieces[PAWN|co] & pawnFileMask[f-1];
        if( f < 7 )
            kingRing |= pcon->pieces[PAWN|co] & pawnFileMask[f+1];*/
    }
}

// friendly pawns in front of the king
// ennemy pawns in front of the king
// open files on the king
// new version uses cached values
/*
static int XXX_pawnOnKing(const CON *pcon, const STE *pste, int co, int sqr) {

    int atk = c_kingAtk[co^1];
    int k_side = sideForFile[ File(sqr) ];

    return phashp->side[co].KSI[k_side].val - atk * phashp->side[co].KSI[k_side].acount;
}
*/
// this has mostly to do with king safety
static int evalKingCastle(const CON *pcon, const STE *pste, int co) {
    int sqr = pcon->king[co];
    const U8 *pos = pcon->pos;
    // friendly pawns in front of the king
    // ennemy pawns in front of the king
    // compute safety value of the king present position
    int k_side = sideForFile[ File(sqr) ];
    Assert(k_side >= KINGSIDE_QS && k_side <= KINGSIDE_KS );

    int val = phashp->side[co].KSI[File(sqr)].val;
    if( pcon->msigCount[QUEEN|FlipColor(co)] ) {
        // after bishop sac on h7 king will be on g6
        if( (co == B && Rank(sqr) < 6) || (co == W && Rank(sqr) > 1) )
            val -= 50;
    }

    //    int val = pawnOnKing(pcon, pste, co, sqr);
    if( k_side == KINGSIDE_C ) {

//        if( pcon->msigCount[QUEEN|FlipColor(co)] )
//            val -= 35;

        // compute safety value of the king future position if the castle can still be done
        // if the king has not castled yet (perhaps because of some tactical moves)
        // and can still castle then blend the 2 scores

        int CastleSqrVal = -999;
        if( pste->castle & ksCastleForSide[co] ) {
            // can't castle
            if( attackMap[FlipColor(co)] & IdxToU64(sqr + 1) )
                val -= 25;
            else {
                // king side castle
//                CastleSqrVal = phashp->side[co].KSI[KINGSIDE_KS].val - atk * phashp->side[co].KSI[KINGSIDE_KS].acount;
                CastleSqrVal = phashp->side[co].KSI[filG].val;
            }
            // pieces between king and rook are delaying the castle (-7 if square is not empty)
            CastleSqrVal -= 2*kingCastleDelay[ pos[sqr+1] ];
            CastleSqrVal -= 2*kingCastleDelay[ pos[sqr+2] ];
//            val -= 4*(kingCastleDelay[ pos[sqr+1] ] + kingCastleDelay[ pos[sqr+2] ]) / 2;
//            val -= 5;
        }
        if( pste->castle & qsCastleForSide[co] ) {
            // can't castle
            if( attackMap[FlipColor(co)] & IdxToU64(sqr - 1) )
                val -= 10;
            else {
                // queen side castle
//                int QueenCastleSqrVal = phashp->side[co].KSI[KINGSIDE_QS].val - atk * phashp->side[co].KSI[KINGSIDE_QS].acount;
                int QueenCastleSqrVal = phashp->side[co].KSI[filB].val;
                // pieces between king and rook are delaying the castle
                QueenCastleSqrVal -= kingCastleDelay[ pos[sqr-1] ];
                QueenCastleSqrVal -= kingCastleDelay[ pos[sqr-2] ];
                QueenCastleSqrVal -= kingCastleDelay[ pos[sqr-3] ];
//                val -= kingCastleDelay[ pos[sqr-1] ];
//                val -= kingCastleDelay[ pos[sqr-2] ];
//                val -= kingCastleDelay[ pos[sqr-3] ];
                if( QueenCastleSqrVal > CastleSqrVal )
                    CastleSqrVal = QueenCastleSqrVal;
            }
//            val -= 5;
        }
        if( CastleSqrVal > val ) {
            // We can still castle and it's better for us, blend the scores.
            // Scores are averaged because the current score is not permanent.
            // how much of each ???
//            val = (CastleSqrVal + val*3) / 4;
            val = (CastleSqrVal + val) / 2;
        }
    } else if( k_side == KINGSIDE_KS ) {
        if( co == B && (pos[G7] == BB) && (pos[G6] == BP) )
            val += 7;
        if( co == W && (pos[G2] == WB) && (pos[G3] == WP) )
            val += 7;
    }
#if 0
    DODEBUG(printf("king shelter=%d \n", val);)
#endif
    val *= kingSafetyWeight;
    if( pcon->msigCount[QUEEN|FlipColor(co)] == 0 )
        val /= 2;
//    if(mat[FlipColor(co)] < 1500)
//        val /= 2;
    val /= 128;

    DODEBUG(valKingCastle[co] = val;)
    return val;
}

    // for each piece of the board...
#define pieceheader(kind) \
		int sqr = PopLSB(pieces);  \
		int pc = kind|co;          \
        /* basic positional value */ \
        posval[co] += pstVal[pc][sqr];


static INLINE void evalPAWN(const CON *pcon, const STE *pste, int co) {
//    if( attackMap[co] & kingRing[co^1] ) {
//        c_kingAtk[co] ++;
//        kingAtkVal[co] += ATK_PAWN;
//    }
    int dir = 8;
    if( co == B )
        dir = -8;
	for (bitboard pieces = phashp->side[co].candidate; pieces; ) {
		int sqr = dir + PopLSB(pieces);

        // give a little bonus for candidate pawns
//        endgval[co] += 2*sqDistance(sqr, pcon->king[co^1]);
        // this will incite the king to move to own candidate/passed pawn
//        endgval[co] -= 1*sqDistance(sqr, pcon->king[co]);
        int distance_bonus = 10*sqDistance(sqr, pcon->king[co^1]) 
                        - 5*sqDistance(sqr, pcon->king[co]);
        // note, only half compared to passer
        endgval[co] += (distance_bonus * pawn_dist_scale[co][Rank(sqr)]) / 32;
    }
}

static INLINE void evalOther(const CON *pcon, const STE *pste, int co) {
    const U8 *pos = pcon->pos;
        //	Computers are by default very stupid about putting
        //	bishops and knights on d3 and e3 while there is still
        //	a pawn on d2 or e2.  I will detect this case and
        //	penalize heavily.
    if(co == W ) {
        if ((pos[E3] && ColorOf(pos[E3]) == W) && (pos[E2] == WP) )
            posval[co] -= 25;
        if ((pos[D3] && ColorOf(pos[D3]) == W) && (pos[D2] == WP) )
            posval[co] -= 25;
    }
    if(co == B ) {
        if ((pos[E6] && ColorOf(pos[E6]) == W) && (pos[E7] == WP) )
            posval[co] -= 25;
        if ((pos[D6] && ColorOf(pos[D6]) == W) && (pos[D7] == WP) )
            posval[co] -= 25;
    }
}

static INLINE void evalPAWNpasser(const CON *pcon, const STE *pste, int co) {
    // we need the attack map for this evaluation
    const U8 *pos = pcon->pos;
    int dir = -8;
    if( co == W )
        dir = 8;
	for (bitboard pieces = phashp->side[co].passed; pieces; ) {
		int sqr = PopLSB(pieces);

        bitboard thispawn = IdxToU64(sqr);
        unsigned int rankBonus = pawn_bonus_to_7[co][Rank(sqr)];
        // give a little bonus for passed pawns
        // protected passed pawn
        if( pos[sqr-1] == (PAWN|co) || pos[sqr+1] == (PAWN|co)
            || pos[sqr-dir-1] == (PAWN|co) || pos[sqr-dir+1] == (PAWN|co)) {
            int support_bonus = rankBonus; // / 2;
//            endgval[co]  += support_bonus;
//            posval[co] += support_bonus / 2;
            endgval[co]  += support_bonus / 2;
            posval[co] += support_bonus;
            }
        // From Ed
        int count = 0;
        int s = sqr + dir;
        for(int i = 0; i < 3; i ++) {
            bitboard thisSqr = IdxToU64(s);
            if( (pos[s] != 0)) {
                if((pos[s] & 1) != co) {
                    int r = Rank(s);
                    if( co == B )
                        r = 7 - r;
                    int blockingBonus = 22 - r * 3;
                    posval[co^1] += blockingBonus;
                    endgval[co^1]  += blockingBonus;
                    if( (attackMap[co] & thisSqr) && (attackMap[co^1] & thisSqr) == 0)
                        count++;
                }
                break;
            }
            // opponent attack this square
            if( (attackMap[co^1] & thisSqr) ) {
                // we should check with see
                if ((attackMap[co] & thisSqr) != 0)
                    count++;
                break;
            }
            if( s < A2 || s > H7 ) {
                count += (3-i);
                break;
            }
            count++;
            s += dir;
        }
        Assert(count >=0 && count <= 3);
        unsigned int count_scale = pawn_bonus_count[count];
        int bonus = (count_scale * rankBonus) / 8;
        posval[co] += bonus;
        endgval[co]  += bonus*2;
        // the next line has a bad effect : pawns won't advance
//        if( thispawn & king_attacks[ pcon->king[co] ] )
//            endgval[co]  += rankBonus + rankBonus;
        // this will incite the king to move to candidate/passed pawn of other side
        // this will incite the king to move to own candidate/passed pawn
        // sqr + dir : next square is more interesting
        int nextSquare = sqr + dir;
        int distance_bonus = 10*sqDistance(nextSquare, pcon->king[co^1]) 
                        - 5*sqDistance(nextSquare, pcon->king[co]);
        endgval[co] += (distance_bonus * pawn_dist_scale[co][Rank(nextSquare)]) / 16;
        if( Rank(nextSquare) == 0 || Rank(nextSquare) == 7 ) {
            if( pcon->pos[nextSquare] == 0 && sqDistance(nextSquare, pcon->king[co]) == 1 ) {
                // pawn near promotion square, king helping the pawn, promotion square is empty
                endgval[co] += valPAWN;
            }
        }
//        printf("%d value of pawn# %d = %d/%d (rankbonus=%d) %d %d\n", co, sqr, bonus, bonus*2, rankBonus, distance_bonus, (distance_bonus * pawn_dist_scale[co][Rank(sqr+dir)]) / 16);
//        printf("distance def=%d att=%d\n", 10*sqDistance(sqr + dir, pcon->king[co^1]) ,- 5*sqDistance(sqr + dir, pcon->king[co]));
        if( mat[co ^ 1] == 0 ) {
            int specialVal;
            // compute square rule
            SquareRuleForPawn(pcon, pste, co, sqr, specialVal);
            if( specialVal > 0 ) {
                endgval[co] += (specialVal);
            }
        }
    }
}

static INLINE void evalKNIGHTmob(const CON *pcon, const STE *pste, int co, int sqr, bitboard moves) {
    bitboard thisPiece = IdxToU64(sqr);
//#ifndef MOB_INCLUDE_DEF
//    moves &= ~pcon->pieces[B_OCCU|co];
    moves &= ~(pcon->pieces[X_OCCU] & pawnAttacks[FlipColor(co)]);
//#endif
//    int mob = (1+popCount(moves) * 2 - popCount(moves & attackMap[co^1]))>>1;
    int mob = popCount(moves);
    mobility[co] += KnightMobility[mob];

    DODEBUG(mobDebugADD(KNIGHT|co,mob);)
    DODEBUG(c_mobility[co] += mob;)
    Assert(mob>=0 && mob <= 8);
    // knight outpost
    int nSqr = remapSq[co][sqr];
    int bonus = knightOutpostVal[nSqr];
    if( bonus ) {
        bonus = 1 + (bonus >> 1);
        // not attacked by an ennemy pawn
        if( (thisPiece & pawnAttacks[FlipColor(co)]) == 0 )
            posval[co] += bonus;
            // defended by my pawns
            if( thisPiece & pawnAttacks[co] ) {
                posval[co] += bonus;
                endgval[co] += bonus;
            }
            // not attacked at all
            // can't do that anymore, attackmap is not complete
/*            if( (thisPiece & attackMap[co^1]) == 0 )
                posval[co] += bonus;*/
    }
    if( knight_future_attacks[sqr] & ~pawnAttacks[FlipColor(co)] & pawnAttacks[co] & outpostMask[co]) {
        posval[co] += 4;
    }

}

static INLINE void evalKNIGHT(const CON *pcon, const STE *pste, int co) {
    const U8 *pos = pcon->pos;
    for (bitboard pieces = pcon->pieces[KNIGHT|co]; pieces; ) {
        //xx__pieceheader(KNIGHT);
		int sqr = PopLSB(pieces); 
		int pc = KNIGHT|co;         
        /* basic positional value */ 
        posval[co] += pstVal[pc][sqr];

        int pawnBonus = (pcon->msigCount[PAWN|co] - 5)*3;
        posval[co] += pawnBonus;
        endgval[co] += pawnBonus;
        bitboard moves = knight_attacks[sqr];
        tmp_attack[co] |= moves;
        evalKNIGHTmob(pcon, pste, co, sqr, moves);
        bitboard king_attack_mask = moves & kingRing[co^1];
        if( king_attack_mask ) {
            kingAtkVal[co] += ATK_MINOR;
            c_kingAtk[co]++;
        } else if( knight_future_attacks[sqr] & kingRing[co^1] ) {
//        } else if( knight_future_attacks[sqr] & king_attacks_ring[ pcon->king[co^1] ] ) {
            kingAtkVal[co] += ATK_MINOR;
        }
//        if( moves & kingRing[co] )
//            c_kingDef[co] += ATK_MINOR;

    }
/*    if( pcon->msigCount[KNIGHT|co] >= 2 ) {
        posval[co] -= 10;
        endgval[co] -= 10;
    }*/
}

static INLINE void evalBISHOPmob(const CON *pcon, const STE *pste, int co, int sqr, bitboard moves) {
    bitboard thisPiece = IdxToU64(sqr);
#ifndef MOB_INCLUDE_DEF
    moves &= ~pcon->pieces[B_OCCU|co];
#endif
//    int mob = (1+popCount(moves) * 2 - popCount(moves & attackMap[co^1]))>>1;
    int mob = popCount(moves);
    mobility[co] += BishopMobility[mob];

    DODEBUG(mobDebugADD(BISHOP|co,mob);)
    DODEBUG(c_mobility[co] += mob;)
    Assert(mob>=0 && mob <= 14);

    // bishop outpost
    int nSqr = remapSq[co][sqr];
    int bonus = knightOutpostVal[nSqr];
    if( bonus ) {
        bonus = 1 + (bonus >> 1);
        // not attacked by an ennemy pawn
        if( (thisPiece & pawnAttacks[ FlipColor(co) ]) == 0 )
            posval[co] += bonus;
            // defended by my pawns
            if( thisPiece & pawnAttacks[ co ] )
                posval[co] += bonus;
            // not attacked at all
            // attackmap is not complete
/*            if( (thisPiece & attackMap[ FlipColor(co) ]) == 0 )
                posval[co] += bonus;*/
    }
}


static INLINE void evalBISHOP(const CON *pcon, const STE *pste, int co) {

    // don't reduce the mobility of linked bishop/queen
#ifdef XRAY_MOBILITY
    bitboard bishopBlocker = pcon->pieces[OCCU] ^ pcon->pieces[QUEEN|co];
#else
    bitboard bishopBlocker = pcon->pieces[OCCU];
#endif
	for (bitboard pieces = pcon->pieces[BISHOP|co]; pieces; ) {
        pieceheader(BISHOP);
        bishopSquare[co] = sqr;
        //
        // trapped bishop, etc
        if( bishopTraps[co][sqr] && (bishopTraps[co][sqr] == (bishopTraps[co][sqr] & pcon->pieces[PAWN|(co^1)])) )
            otherval[co] -= 125;
        if( bishopBlocked[co][sqr] && (bishopBlocked[co][sqr] == (bishopBlocked[co][sqr] & pcon->pieces[PAWN|co])) )
            otherval[co] -= 15;

        bitboard moves = Bmagic(sqr, bishopBlocker);
        tmp_attack[co] |= moves;
        evalBISHOPmob(pcon, pste, co, sqr, moves);
//        c_attack[co] += lowpopCount(moves & targets[co]);

        bitboard king_attack_mask = moves & kingRing[co^1];
        if( king_attack_mask ) {
            kingAtkVal[co] += ATK_MINOR;
            c_kingAtk[co]++;
        }
//        if( moves & kingRing[co] )
//            c_kingDef[co] += ATK_MINOR;

        // the bishop is a little better than the knight in eg
        //endgval[co] += 20;
    }
    if( pcon->msigCount[BISHOP|co] >= 2 ) {
        // bishop bonus if open position
        posval[co] += (4 - phashp->c_center_pawns);
        // bishop pair
        posval[co] += 50;
        endgval[co] += 50;
    }
}

static INLINE void evalROOKmob(const CON *pcon, const STE *pste, int co, int sqr, bitboard moves) {

#ifndef MOB_INCLUDE_DEF
    moves &= ~pcon->pieces[B_OCCU|co];
#endif
    int mob = popCount(moves);

    mobility[co] += RookMobility[mob];
    DODEBUG(mobDebugADD(ROOK|co,mob);)
    DODEBUG(c_mobility[co] += mob;)
    Assert(mob>=0 && mob <= 14);
}

static bool INLINE somePawns(bitboard pawns) {
    return (pawns & (pawns - 1)) != 0;
}

static INLINE void evalROOK(const CON *pcon, const STE *pste, int co) {

    // don't reduce the mobility of linked rooks
#ifdef XRAY_MOBILITY
    bitboard rookBlocker = pcon->pieces[OCCU] ^ pcon->pieces[ROOK|co] ^ pcon->pieces[QUEEN|co];
#else
    bitboard rookBlocker = pcon->pieces[OCCU];
#endif
	for (bitboard pieces = pcon->pieces[ROOK|co]; pieces; ) {
        pieceheader(ROOK);

        int pawnMalus = (pcon->msigCount[PAWN|co] - 5)*3;  // 1/8
        posval[co] -= pawnMalus;
        endgval[co] -= pawnMalus;

        // Xray attack
        bitboard moves = Rmagic(sqr, rookBlocker);
        tmp_attack[co] |= moves;
        evalROOKmob(pcon, pste, co, sqr, moves);

//        if( moves & kingRing[co] )
//            c_kingDef[co] += ATK_ROOK;

        if( moves & kingRing[co^1] ) {
            kingAtkVal[co] += ATK_ROOK;
            c_kingAtk[co]++;
            kingAtkFSlider[co] = true;
//            if( moves & (moves - 1) & kingRing[co^1] )
//                kingAtkVal[co] += 2;
            if( abs(File(sqr) - File(pcon->king[co^1])) <= 1 )
                posval[co] += 10;
        }
        // rook on the 7th (king blocked on 8th or pawns to grab
        if( co == W && (Rank(sqr) == 6) && (Rank(pcon->king[B]) == 7
            || somePawns(pcon->pieces[PAWN|B] & Rank7Mask)) ) {
            posval[co] += bRook7thMG;
            endgval[co] += bRook7thEG;
        }
        if( co == B && (Rank(sqr) == 1) && (Rank(pcon->king[W]) == 0
            || somePawns(pcon->pieces[PAWN|W] & Rank2Mask)) ) {
            posval[co] += bRook7thMG;
            endgval[co] += bRook7thEG;
        }
        // rook on semi-open file
        int rookF = File(sqr);
        if(phashp->side[co].c_pawn[rookF] == 0) {
            posval[co] += bRookSemiOpen;
            endgval[co] += bRookSemiOpen;
            // rook on open file
            if(phashp->side[co^1].c_pawn[rookF] == 0) {
                posval[co] += bRookOpen;
                endgval[co] += bRookOpen;
            }
        } else {
            if(phashp->side[co^1].c_pawn[rookF] == 0) {
                posval[co] += 4;
                endgval[co] += 4;
            }
            if(pawnFileMask[rookF] & phashp->side[co].passed) {
                posval[co] += 10;
                endgval[co] += 10;
            }
        }

        //
//        endgval[co] += 30;
//        if( mat[co ^ 1] < valROOK )
//            endgval[co] += 40;
        int knSqr = remapSq[co][pcon->king[co]];
        // trapped rook
        if( phashp->side[co].c_pawn[rookF] ) {
            int nSqr = remapSq[co][sqr];
            if( (nSqr == A8 || nSqr == B8) && (knSqr == B8 || knSqr == C8) ) {
                posval[co] -= 15;
                endgval[co] -= 15;
            }
            if( (nSqr == H8 || nSqr == G8) && (knSqr == F8 || knSqr == G8) ) {
                posval[co] -= 15;
                endgval[co] -= 15;
            }
        }
    }
    // rook redundancy
    if( pcon->msigCount[WQ] == pcon->msigCount[BQ] )
        if( pcon->msigCount[ROOK|co] >= 2 ) {
            posval[co] -= 25;
            endgval[co] -= 25;
        }

}


static INLINE void evalQUEENmob(const CON *pcon, const STE *pste, int co, int sqr, bitboard moves) {

#ifndef MOB_INCLUDE_DEF
    moves &= ~pcon->pieces[B_OCCU|co];
#endif
//    int mob = (1+popCount(moves) * 2 - popCount(moves & attackMap[co^1]))>>1;
    int mob = popCount(moves);
    mobility[co] += QueenMobility[mob];
    DODEBUG(mobDebugADD(QUEEN|co,mob);)
    DODEBUG(c_mobility[co] += mob;)
    Assert(mob>=0 && mob <= 28);
}

static INLINE void evalQUEEN(const CON *pcon, const STE *pste, int co) {
    queen_castle_attack[co] = 0;
    const U8 *pos = pcon->pos;
	for (bitboard pieces = pcon->pieces[QUEEN|co]; pieces; ) {
        pieceheader(QUEEN);

        // rook on the 7th (king blocked on 8th or pawns to grab
        if( co == W && (Rank(sqr) == 6) && (Rank(pcon->king[B]) == 7
            || somePawns(pcon->pieces[PAWN|B] & Rank7Mask)) ) {
            posval[co] += bQueen7thMG;
            endgval[co] += bQueen7thEG;
        }
        if( co == B && (Rank(sqr) == 1) && (Rank(pcon->king[W]) == 0
            || somePawns(pcon->pieces[PAWN|W] & Rank2Mask)) ) {
            posval[co] += bQueen7thMG;
            endgval[co] += bQueen7thEG;
        }

#ifdef XRAY_MOBILITY
        bitboard moves = Rmagic(sqr, pcon->pieces[OCCU]^pcon->pieces[ROOK|co]) |
            Bmagic(sqr, pcon->pieces[OCCU]^pcon->pieces[BISHOP|co]);
#else
        bitboard moves = Rmagic(sqr, pcon->pieces[OCCU]) |
            Bmagic(sqr, pcon->pieces[OCCU]);
#endif
        tmp_attack[co] |= moves;
        evalQUEENmob(pcon, pste, co, sqr, moves);
//        c_attack[co] += (1+lowpopCount(moves & targets[co])) / 2;

        bitboard king_attack_mask = moves & kingRing[co^1];
        if( king_attack_mask ) {
            kingAtkVal[co] += ATK_QUEEN;
            c_kingAtk[co]++;
            kingAtkFSlider[co] = true;
//            if( (moves - 1) & king_attack_mask )
//                kingAtkVal[co] += 2;
            // save bitmap attack of knight/bishop/pawn
            // if Q atk & minor atk & ~atk[co^1] => high threat
            queen_castle_attack[co] = king_attack_mask & attackMap[co];
        }
//        if( moves & kingRing[co] )
//            c_kingDef[co] += ATK_QUEEN;

        //endgval[co] += 75;
        //int minorMalus = 25 * (4- (pcon->msigCount[KNIGHT|co] + pcon->msigCount[BISHOP|co]) );
        //posval[co] -= minorMalus / 2;
        //endgval[co] -= minorMalus / 2;
        // queen counts for two
        notpawn[co]++;
    }
}

static INLINE void evalKING(const CON *pcon, const STE *pste, int co) {

    int sqr = pcon->king[co];
    int pc = KING|co;
    /* neutral color square for positional values */
    int nSqr = remapSq[co][sqr];
    /* basic positional value */
    posval[co] += pstVal[pc][sqr];

    if( kingDanger[co] )
        safetyval[co] += evalKingCastle(pcon, pste, co);

    endgval[co] += KingEGPosVal[nSqr];
    // add king mobility for endgame
    bitboard moves = king_attacks[sqr];
    moves &= ~attackMap[co^1];
    tmp_attack[co] |= moves;
#ifndef MOB_INCLUDE_DEF
    moves &= ~pcon->pieces[B_OCCU|co];
#endif
    int mob = popCount(moves);
    mobDebugADD(pc,mob);
    endgval[co] += mob*2;

    if( false && notpawn[co] == 1 && notpawn[co^1] == 1) {   // pawn endgame only
        // accessible squares
        bitboard kingSqr = IdxToU64(sqr);
        bitboard hisDefense = pawnAttacks[co ^ 1] | king_attacks[pcon->king[co^1]];
        // acessible squares : not occupied by my pawns, not attacked by their pawns
        bitboard path = 0xffffffffffffffffULL & (~pcon->pieces[PAWN|co]) & ~hisDefense;
        // look for pawns not defended
        bitboard targets = pcon->pieces[PAWN|co^1] & ~hisDefense;
	    if ( targets ) {
            int dist = pathLength(kingSqr, targets, path);
            if( dist < 50 ) {
                bitboard myDefense = pawnAttacks[co] | king_attacks[pcon->king[co]];
                // oppPath is not allways correct 
                bitboard oppPath = 0xffffffffffffffffULL & (~(pcon->pieces[PAWN|co^1]^targets)) & ~myDefense;
                int hisDist = pathLength(IdxToU64(pcon->king[co^1]), targets, oppPath);
                if( dist + 1 < hisDist )
                    endgval[co] += 50;
            }
        }
    }

}

//	This eval function evaluates:
//
//	1.	Pawn structure, which is hashed and handled in another module.
//	2.	Piece location, which is usually related to central occupation or
//		in the case of pawns, simple advancement.
//	3.	Minors blocking d2/e2 pawns.
//	4.	King on f1/g1 with rook on g1/h1.
//	5.	Pawn shelter.
//	6.	Endgame king position values versus middlegame values.
//
//	Cases 3 and 4 are surprisingly important.
//
//	The piece-square tables are designed to induce the program to castle
//	behind a safe pawn structure, to move its central pawns, and to develop
//	its pieces.
//
//	There is also a chance that the program would feel an interest in breaking
//	its opponent's king position, but I don't have high hopes for this.



#ifdef DEBUG
static void dbgPos(char *txt) {
    printf("#W:%3d(%3d)/%3d + %3d m=%3d\tB:%3d(%3d)/%3d + %3d m=%3d\t(%s)\n",
        posval[W], endgval[W], posval[W] - prev_posval[W], endgval[W] - prev_endgval[W], mobility[W]
#ifndef OLD_MOBILITY
        / 16
#endif
        , 
        posval[B], endgval[B], posval[B] - prev_posval[B], endgval[B] - prev_endgval[B], mobility[B]
#ifndef OLD_MOBILITY
        / 16
#endif
        , txt);
    prev_posval[W] = posval[W];
    prev_posval[B] = posval[B];
    prev_endgval[W] = endgval[W];
    prev_endgval[B] = endgval[B];
}

#define DBGPOS(txt) if( Alpha == -20000 ) dbgPos(txt)
//#define EVALINFO(...) if( Alpha == -20000 ) {printf(__VA_ARGS__);}
#else
#define DBGPOS(txt)
//#define EVALINFO(...)
#endif

static inline void updateWinChance(int &chance, int new_chance) {
    if( new_chance < chance )
        chance = new_chance;
}

// call the endgames recognizers with material for one side only
// if that side can not win against zero material he *usually* will not win against more material

static void INLINE checkWinChance(CON *pcon, STE *pste, int co, int &chance) {
    // -- call recognizers --
    unsigned b_mstate = 0;
    unsigned w_mstate = 0;
    if( co == B )
        b_mstate = msigToIdx( pcon->msigBit[B] );
    else
        w_mstate = msigToIdx( pcon->msigBit[W] );

    // calls the recognizer, only zero score is interesting here
    if( true && recog_available(b_mstate,w_mstate)) {
        recog thisRecog = recog_table[recog_index(w_mstate,b_mstate)];
        recog_result ret = {fail_bound,0,recog_need_validation, WIN_CHANCE(100), WIN_CHANCE(100)};
        if( thisRecog(pcon, pste, ret) ) {
            updateWinChance( chance, ret.win_chance[co] );
            if( ret.bound == exact_bound && ret.score == 0)
                updateWinChance( chance, WIN_CHANCE_DRAWISH );
        }
    }
}

int ValEval(PCON pcon, PSTE pste, int Alpha, int Beta)
{
    if( pste->evaluated && !pste->evalislazy )
        return pste->val;

    int specialVal;
    int side = pste->side;
    int xside = side ^ 1;
    int isqKingUs = pcon->king[side];
    int isqKingThem = pcon->king[xside];
#ifdef _DEBUG
    mobDebugIdx = 0;
#endif
    stats.cEval++;
    pste->bound = fail_bound;
    pste->threat[W] = 0;
    pste->threat[B] = 0;

    const EvalCacheEntry *thisEval = evalCache->find( pste );
    if( true && thisEval ) {
        stats.evc_hit++;
        stats.cFullEval++;
        pste->evaluated = true;
        pste->evalislazy = false;
        pste->val = thisEval->val;
        return thisEval->val;
    }
    int majors[2] = {pcon->msigCount[BR] + pcon->msigCount[BQ], pcon->msigCount[WR] + pcon->msigCount[WQ]};
    int minors[2] = {pcon->msigCount[BN] + pcon->msigCount[BB], pcon->msigCount[WN] + pcon->msigCount[WB]};
    notpawn[B] = 1 + minors[B] + majors[B];
    notpawn[W] = 1 + minors[W] + majors[W];

    // -- probe bitbases --
    // handle only 4 men
    if( true && (pcon->msigCount[WP] + pcon->msigCount[BP]) <= 2 )
        if((pcon->msigCount[WP] + pcon->msigCount[BP] +
            notpawn[B] + notpawn[W]) <= 4 ) {
            // do probe...
            if( probe_bitbases(pcon, pste, side, specialVal) ) {
                stats.c_recog++;
                pste->evaluated = true;
                pste->evalislazy = false;
                pste->val = specialVal;
                pste->bound = exact_bound;
                return specialVal;
            }
        }

    // -- call recognizers --
    recog_result recogResult = {fail_bound,0,recog_need_validation, WIN_CHANCE(100), WIN_CHANCE(100)};
    unsigned b_mstate = msigToIdx( pcon->msigBit[B] );
    unsigned w_mstate = msigToIdx( pcon->msigBit[W] );
    if( true && recog_available(b_mstate,w_mstate)) {
        recog thisRecog = recog_table[recog_index(w_mstate,b_mstate)];
        if( thisRecog(pcon, pste, recogResult) ) {
            // we can not return a bound for some positions, convert the bound into a simple score
            if( (recogResult.recog_validation == recog_need_validation) && 
                (recogResult.bound != score_bound)
                && !positionOkForRecog(pcon, pste) )
                recogResult.bound = score_bound;
            stats.c_recog++;
            pste->evaluated = true;
            pste->evalislazy = false;
            pste->val = recogResult.score;
            pste->bound = int(recogResult.bound);
#ifdef DEBUG
            if( Alpha == -20000 ) 
                {printf("recog found val=%d\n", recogResult.score );}
            else
#endif
            return recogResult.score;
        }
    }
    pste->evalislazy = true;

	//	Get value for pawn structure.
	//
    EvalPawns(pcon, pste, phashp);
	//
	//	Get material value.
	//
    int val[coMAX];
    // pawn structure score not included in eg, structure is added in posval only
    val[side] = pste->valPcUs + pste->valPnUs;// + phashp->side[side].val ;
    val[xside] = pste->valPcThem + pste->valPnThem;// + phashp->side[xside].val ;
//    val[side] = pste->valPcUs + pste->valPnUs;
//    val[xside] = pste->valPcThem + pste->valPnThem;
    mat[side] = pste->valPcUs;
    mat[xside] = pste->valPcThem;
    if( abs( pste->valPcThem - pste->valPcUs ) <= 4*valPAWN && (pste->valPnThem + pste->valPnUs == 0) ) {
        updateWinChance(recogResult.win_chance[0], WIN_CHANCE_DRAWISH);
        updateWinChance(recogResult.win_chance[1], WIN_CHANCE_DRAWISH);
    }
    int win_chance[2] = {recogResult.win_chance[0], recogResult.win_chance[1]};

    //
    //  Check for insuffisant material
    //
    // TODO:cleanup since we have some recognizers now
    //
    // No pawn
	if ( (pste->valPnUs + pste->valPnThem) == 0 ) {
        // K + Q vs K
		if ((pste->valPcUs == valQUEEN||pste->valPcUs == valROOK) && (pste->valPcThem == 0))
			return pste->valPcUs + 150 - c_argdistborder[ isqKingThem ] -
                sqDistance( isqKingUs, isqKingThem );
        // K vs K + Q | R
		if ((pste->valPcUs == 0) && (pste->valPcThem == valQUEEN||pste->valPcThem == valROOK))
			return -(pste->valPcThem + 150 - c_argdistborder[ isqKingUs ] -
                sqDistance( isqKingUs, isqKingThem ));
        if (pste->valPcUs == (valBISHOP + valKNIGHT)) {
            if( (pste->valPcThem == valBISHOP||pste->valPcThem == valKNIGHT))
                return 0;
            // KBNK
            else if( (pste->valPcThem == 0))
                if( KBNK( pcon, pste, side, specialVal) )
                    return specialVal;
        }
        if (pste->valPcThem == (valBISHOP + valKNIGHT)) {
            if( (pste->valPcUs == valBISHOP||pste->valPcUs == valKNIGHT))
                return 0;
            else if( (pste->valPcUs == 0))
                if( KBNK( pcon, pste, xside, specialVal) )
                    return -specialVal;
        }
    }
    // bishop + pawns vs nothing
    if ((pste->valPcUs == valBISHOP || pste->valPcUs == 0) && (pste->valPcThem == 0) &&
        (pste->valPnUs > 0) && (pste->valPnThem == 0)) {
        // pawns on file A
        if( (pcon->pieces[PAWN|side] & ~fileAMask) == 0 )
            // if opposite king is in the corner then we can not win
            if( !bishopcanreach(pcon, side, promotionSqrFileA[side]) && sqDistance( pcon->king[xside], promotionSqrFileA[side] ) <= 1 )
                return 50;
        // pawns on file H
        if( (pcon->pieces[PAWN|side] & ~fileHMask) == 0 )
            // if opposite king is in the corner then we can not win
            if( !bishopcanreach(pcon, side, promotionSqrFileH[side]) && sqDistance( pcon->king[xside], promotionSqrFileH[side] ) <= 1 )
                return 50;
    }
    // bishop + pawns vs nothing
    if ((pste->valPcThem == valBISHOP || pste->valPcThem == 0) && (pste->valPcUs == 0) &&
        (pste->valPnThem > 0) && (pste->valPnUs == 0)) {
        // pawns on file A
        if( (pcon->pieces[PAWN|xside] & ~fileAMask) == 0 )
            // if opposite king is in the corner then we can not win
            if( !bishopcanreach(pcon, xside, promotionSqrFileA[xside]) && sqDistance( pcon->king[side], promotionSqrFileA[xside] ) <= 1 )
                return -50;
        // pawns on file H
        if( (pcon->pieces[PAWN|xside] & ~fileHMask) == 0 )
            // if opposite king is in the corner then we can not win
            if( !bishopcanreach(pcon, xside, promotionSqrFileH[xside]) && sqDistance( pcon->king[side], promotionSqrFileH[xside] ) <= 1 )
                return -50;
    }

    //
    //	Cut off if it appears that this is futile.
    //  Don't do this cut off in endgames !!
    //
#if 0
    // caution with score and pruning
    // handle endgame values
    int tmpval = val[side] - val[xside] + phashp->side[side].val - + phashp->side[xside].val;
    int margin = 500;
    const int posMargin = 50;
    if( (pste->valPcUs && pste->valPcThem) && ((Alpha+1) == Beta) ) {
	    if (tmpval - margin >= Beta)		// We're already great?
            return (tmpval - margin);
//	if (tmpval + margin <= Alpha) 	// We're already toast?
//        return (tmpval + posMargin);
    }
#endif

    DODEBUG(prev_posval[W] = prev_posval[B] = prev_endgval[W] = prev_endgval[B] = 0);
    kingDanger[W] = (notpawn[B] >= 3) && (pcon->msigCount[BQ] >= 1);    // 3 because of king
    kingDanger[B] = (notpawn[W] >= 3) && (pcon->msigCount[WQ] >= 1);    // 3 because of king
    posval[B] = phashp->side[B].val;
    posval[W] = phashp->side[W].val;
    otherval[B] = otherval[W] = 0;  // other value ?
    endgval[B] = phashp->side[B].endgval;
    endgval[W] = phashp->side[W].endgval;
//    attackval[B] = attackval[W] = 0;
    DODEBUG(c_mobility[B] = c_mobility[W] = 0;)
    mobility[B] = mobility[W] = 0;
//    c_attack[B] = c_attack[W] = 0;

    computeKingRing(pcon, pste, B, kingRing[B]);
    computeKingRing(pcon, pste, W, kingRing[W]);

    // for xray king attacks
//    moveBlocker[B] = pcon->pieces[B_OCCU|W] | pcon->pieces[PAWN|B] | pcon->pieces[KING|B];
//    moveBlocker[W] = pcon->pieces[B_OCCU|B] | pcon->pieces[PAWN|W] | pcon->pieces[KING|W];
    // for normal attacks
    pawnAttacks[W] = phashp->side[W].pawnatk;
    pawnAttacks[B] = phashp->side[B].pawnatk;
//    targets[B] = pcon->pieces[B_OCCU|W] ^ (pcon->pieces[PAWN|W] & pawnAttacks[W]);
//    targets[W] = pcon->pieces[B_OCCU|B] ^ (pcon->pieces[PAWN|B] & pawnAttacks[B]);
    // king tropism, attack & defense
    kingAtkVal[B] = kingAtkVal[W] = 0;
    c_kingAtk[B] = c_kingAtk[W] = 0;
//    c_kingDef[B] = c_kingDef[W] = 0;
    safetyval[B] = safetyval[W] = 0;
    kingAtkFSlider[B] = kingAtkFSlider[W] = false;

//    const bitboard offenseMapSide[coMAX] = {0x00000000ffffffffULL, 0xffffffff00000000ULL};
    const bitboard offenseMapSide[coMAX] = {
        Rank1Mask|Rank2Mask|Rank3Mask|Rank4Mask|0x0000003C3C000000ULL, 
        Rank8Mask|Rank7Mask|Rank6Mask|Rank5Mask|0x0000003C3C000000ULL};

    // hack alert : change the content of the king square
    // still needed for pawn passer eval
    pcon->pos[pcon->king[B]] = APC_BLACKKING;

//    const U8 *pos = pcon->pos;

    {
	    attackMap[W] = pawnAttacks[W];
	    attackMap[B] = pawnAttacks[B];
    }

#define foreachside2(c,code)    {int co = c; code;}
#define foreachside(code)       foreachside2(B,code) foreachside2(W,code)

    DBGPOS("init");
    evalPAWN(pcon, pste, B);
    evalPAWN(pcon, pste, W);
    DBGPOS("pawn");

    foreachside(
        defSpace[co] = phashp->side[co].c_space;
/*        if( File(pcon->king[co]) >= filD )
            defSpace[co] += phashp->side[co].c_Qspace;
        else if( File(pcon->king[co]) <= filE )
            defSpace[co] += 2*phashp->side[co].c_Kspace;*/
    )
    posval[B] += defSpace[B];
    posval[W] += defSpace[W];
    DBGPOS("space");

    knight_moves[B] = knight_moves[W] = 0;
    tmp_attack[B] = tmp_attack[W] = 0;
//    underdev[B] = underdev[W] = 0;

    evalKNIGHT(pcon, pste, B);
    evalKNIGHT(pcon, pste, W);
    DBGPOS("KNIGHT");

    bishopSquare[B] = bishopSquare[W] = 0;

    evalBISHOP(pcon, pste, B);
    evalBISHOP(pcon, pste, W);
    DBGPOS("BISHOP");

    attackMap[W] |= tmp_attack[W];
    attackMap[B] |= tmp_attack[B];
    tmp_attack[W] = tmp_attack[B] = 0;

    evalROOK(pcon, pste, B);
    evalROOK(pcon, pste, W);
    DBGPOS("ROOK");

    attackMap[W] |= tmp_attack[W];
    attackMap[B] |= tmp_attack[B];
    tmp_attack[W] = tmp_attack[B] = 0;

    evalQUEEN(pcon, pste, B);
    evalQUEEN(pcon, pste, W);
    attackMap[W] |= tmp_attack[W];
    attackMap[B] |= tmp_attack[B];
    tmp_attack[W] = tmp_attack[B] = 0;

#if 0
    foreachside(
        // need complete (but king) attackmap here
        if( queen_castle_attack[co] & ~attackMap[co^1] ) {
            //VDumpPawnMaps("11",
              //  &queen_castle_attack[co], "QA",
              //  &attackMap[co^1], "def");
            c_kingAtk[co]++;
        }
    )
#endif
    foreachside(
        bitboard attack_squares = (attackMap[co] | pcon->pieces[B_OCCU|co]) & kingRing[co^1] & ~ attackMap[co^1];
        int square_count = lowpopCount( attack_squares );
        kingRingPressure[co^1] = square_count;
//        DODEBUG(VDumpPawnMaps("1", &attack_squares, "press");)
    )

    DBGPOS("QUEEN");

    evalKING(pcon, pste, B);
    evalKING(pcon, pste, W);
    DBGPOS("KING");

    attackMap[W] |= tmp_attack[W];
    attackMap[B] |= tmp_attack[B];

    evalPAWNpasser(pcon, pste, B);
    evalPAWNpasser(pcon, pste, W);
    DBGPOS("pawn passer");

    evalOther(pcon, pste, B);
    evalOther(pcon, pste, W);
    DBGPOS("other things");

    posval[W] += phashp->side[W].c_center * (2+notpawn[B]) / 2;
    posval[B] += phashp->side[B].c_center * (2+notpawn[W]) / 2;
    DBGPOS("center");

#if 0
    int medmob = (mobility[W] + mobility[B]) / 2;
    foreachside(
        // b:17#a==1 mob2 := 17-24+30 := 23 activ := 1*4 := 4
        // w:31#a==2 mob2 := 31-24+30 := 37 activ := 2*4 := 8
        int c_atk = c_attack[co];
        // we don't want yoyo scores
        if( co == side )
            c_atk ++;
        int mob = (mobility[co] - medmob + 30);
        if( mob > 0 )
            posval[co] += c_atk * 2;
        posval[co] += c_atk * 1;
    )
    DBGPOS("targets");
#endif

#if 1
    foreachside(
        bitboard atk = attackMap[co] & offenseMapSide[co];
        posval[co] += (2*popCount(atk))/4;
        //DODEBUG(VDumpPawnMaps("1", &atk, "atk"));
    )
//    if( (pste->checkf == 0) && (pste->pinned == 0) && (pawnAttacks[side] & (pcon->pieces[B_OCCU|xside]^pcon->pieces[PAWN|xside])) )
//        posval[side] += 20;
    DBGPOS("rays2");
#endif

//    int valAttacking[coMAX] = {0,0};
//    int valHanging[coMAX] = {0,0};

#define updateThreat(bm,val)   \
            if( bm & attackMap[co^1]) { \
                valAttacking[co ^ 1] ++; \
                if( (bm & attackMap[co]) == 0 ) \
                    valHanging[co] += val;\
                else if( bm & pawnAttacks[co^1] ) \
                    valHanging[co] += val - 100;\
            }

#if 0
    if( true )  { // if not at leaf
        foreachside({
            updateThreat(pcon->pieces[PAWN|co],100);
            updateThreat(pcon->pieces[KNIGHT|co],325);
            updateThreat(pcon->pieces[BISHOP|co],325);
            updateThreat(pcon->pieces[ROOK|co],500);
            updateThreat(pcon->pieces[QUEEN|co],975);
        })
    }
#endif

    // hack alert : restore the content of the king square
    pcon->pos[pcon->king[B]] = KING|B;


    DBGPOS("dummy");

    c_kingAtk[W] += kingRingPressure[B] / 2;    //3;
    c_kingAtk[B] += kingRingPressure[W] / 2;    //3;
    if( kingDanger[B] ) {
//        int bonus = (kingAtkVal[W] + kingRingPressure[B] ) * kingSafetyCurve[ c_kingAtk[W] ] / 64;
        int bonus = kingAtkVal[W] * kingSafetyCurve[ c_kingAtk[W] ] / 64;
        if( pcon->msigCount[QUEEN|W] == 0)
            bonus /= 2;
        safetyval[B] -= bonus;
        posval[B] += safetyval[B];
    }
    if( kingDanger[W] ) {
//        int bonus = (kingAtkVal[B] + kingRingPressure[W] ) * kingSafetyCurve[ c_kingAtk[B] ] / 64;
        int bonus = kingAtkVal[B] * kingSafetyCurve[ c_kingAtk[B] ] / 64;
        if( pcon->msigCount[QUEEN|B] == 0)
            bonus /= 2;
        safetyval[W] -= bonus;
        posval[W] += safetyval[W];
    }
    DBGPOS("KING safety");


    bool same_material = abs(mat[W] - mat[B]) <= 10;
    int pawnDiffCount = abs(pcon->msigCount[PAWN|W] - pcon->msigCount[PAWN|B]);
    int stmBonus = 5;

	for (int co = coBLACK; co <= coWHITE; co++) {
        int pawnCount = pcon->msigCount[PAWN|co];

        // add a bonus for the side with one more piece (and more material value)
        if( ((mat[co] - 100) > mat[co^1]) ) {
            if( (notpawn[co] > notpawn[co^1])) {
                // we need pawns
                int bonus = (notpawn[co]-2) * 8;    // need to check that !
                posval[co] += bonus;
                endgval[co] += bonus;
                if( majors[W] + majors[B] == 0 ) {
                    if( minors[co] == 2 && minors[co^1] == 1 && pawnCount == 0) {
                        // mm vs m[P*] two minors won't win
                        updateWinChance(win_chance[co], WIN_CHANCE_NONE);
                    }
                    else if( minors[co] == 3 && minors[co^1] == 2 && pawnCount == 0) {
                        // mmm vs mm[P*] three minors won't win
                        updateWinChance(win_chance[co], WIN_CHANCE_LOW);
                    }
                    else if( mat[co] == valKNIGHT && pawnCount == 0) {
                        // m vs [P*] 
                        endgval[co] -= 150;
                        updateWinChance(win_chance[co], WIN_CHANCE_NONE);
                    }
                } else {
                    if( mat[co] == valROOK+valBISHOP && mat[co^1] == valROOK && pawnCount == 0) {
                        // RB vs R[P*]  RB won't win
                        updateWinChance(win_chance[co], WIN_CHANCE(5));
                    } else if( mat[co] == valQUEEN+valBISHOP && mat[co^1] == valQUEEN && pawnCount == 0) {
                        // QB vs Q[P*]  QB won't win
                        updateWinChance(win_chance[co], WIN_CHANCE_NONE);
                    }
                }
                DODEBUG(if( Alpha == -20000 ) {printf("not same material %d %d\n", mat[co], mat[co^1]);})
            } else {
                // exchange ?
                posval[co] += 50; // current value
//                posval[co] += 35; // current value
//                endgval[co] += 80; // current value

//                int bonus = (pcon->msigCount[PAWN|co] - 3) * 15;
//                posval[co] += bonus;
//                endgval[co] += bonus;
                if( (pcon->msigCount[PAWN|co] == 0) && (mat[co] == valROOK) ) {
                    if( mat[co^1] == valBISHOP || mat[co^1] == valKNIGHT ) {
                        // R vs m[P*] Rook won't win
                        updateWinChance(win_chance[co], WIN_CHANCE_NONE);
                    }
                } else if( (mat[co] == valROOK+valBISHOP || mat[co] == valROOK+valKNIGHT) && mat[co^1] >= 2*valKNIGHT && pawnCount == 0) {
                    // RM vs MM[P*]  RB won't win
                    updateWinChance(win_chance[co], WIN_CHANCE(5));
                }
                DODEBUG(if( Alpha == -20000 ) {printf("not same material (ex) %d %d\n", mat[co], mat[co^1]);})
            }
        } else if( same_material ) {
            // same material value
            if( (mat[co] == valBISHOP || mat[co] == valKNIGHT) && (pawnCount == 1) ) {
                updateWinChance(win_chance[co], WIN_CHANCE_LOW);
            } else if( mat[co] <= valROOK && (pawnDiffCount <= 1) ) {
                stmBonus = 1;
                if( mat[co] == 0 ) {
                    stmBonus = 1;
                    //updateWinChance(win_chance[co], WIN_CHANCE(75));
                } else
                    updateWinChance(win_chance[co], WIN_CHANCE(75));
            }
        }

        // add the mobility

#ifdef OLD_MOBILITY
        posval[co] += (2*mobility[co]) / 3;
        endgval[co] += mobility[co] / 2;
#else
        posval[co] += mobility[co] / 16;
        endgval[co] += mobility[co] / 16;
#endif
        // knight bonus for closed positions
/*        if( pcon->msigCount[KNIGHT|co] >= 1 && pcon->msigCount[BISHOP|(co^1)] == 1 )
            posval[co] += (phashp->c_center_pawns - 6);*/
    }

    // EG with pawns and bishops of opposite color
    if( same_material && (pcon->msigCount[WB] == 1) && (pcon->msigCount[BB] == 1) && (pawnDiffCount <= 2)
        && (pcon->msigCount[WQ] + pcon->msigCount[BQ] == 0)
        && (pcon->msigCount[WN] + pcon->msigCount[BN] == 0)) {
        if( !sameSquareColor( bishopSquare[W], bishopSquare[B] ) ) {
            int rCount = pcon->msigCount[WR] + pcon->msigCount[BR];
            if( rCount == 0 ) {
                int chance = WIN_CHANCE(50);
                updateWinChance( win_chance[W], chance );
                updateWinChance( win_chance[B], chance );
            } else if (rCount == 2 && pawnDiffCount <= 1) {
                int chance = WIN_CHANCE(90);
                updateWinChance( win_chance[W], chance );
                updateWinChance( win_chance[B], chance );
            } else if (rCount == 4 && pawnDiffCount == 0) {
                int chance = WIN_CHANCE(90);
                updateWinChance( win_chance[W], chance );
                updateWinChance( win_chance[B], chance );
            }
        }
    }

    checkWinChance(pcon, pste, W, win_chance[W]);
    checkWinChance(pcon, pste, B, win_chance[B]);

//    pste->threat[W] = valHanging[W];
//    pste->threat[B] = valHanging[B];

    DBGPOS("...");

    int finalVal[coMAX];
    // xside is the side to move
    val[xside] -= stmBonus;
    val[side] += stmBonus;

	finalVal[W] = val[W] + MidGameValue( posval[W],notpawn[B]) +
           EndGameValue( endgval[W], notpawn[B] ) + otherval[W];
	finalVal[B] = val[B] + MidGameValue( posval[B],notpawn[W]) +
           EndGameValue( endgval[B], notpawn[W] ) + otherval[B];

    int delta = finalVal[side] - finalVal[xside];

    DODEBUG(int beforeScaleScore = delta;)

    if( delta > 0 )
        delta = delta * win_chance[side] / 256;
    if( delta < 0 )
        delta = delta * win_chance[xside] / 256;

#ifdef DEBUG
    if( Alpha == -20000 ) {
//        VDumpPawnMaps("!", attacked_material, "attacked");
        printf("# val=%d chance W/B = %d %%/ %d %%=> %d (hkey=%I64x) centerpawn#=%d side=%d\n", beforeScaleScore,
            (255+win_chance[W] * 100) / 256, (255+win_chance[B] * 100) / 256, delta, pste->hashkPc, phashp->c_center_pawns, side );
    	for (int co = coBLACK; co <= coWHITE; co++) {
            printf("# (%c) val=%d finalval=%d posval=%d endgval=%d other=%d \n",
                "BW"[co], val[co], finalVal[co], posval[co], endgval[co], otherval[co]);
            printf("# dynamics  c_mob=%d mobility=%d\n",
                c_mobility[co], mobility[co]);
            printf("# others : space in side=%d\n", defSpace[co]);
            printf("# pawns : val=%d  passed#=%d candidate#=%d center=%d\n", phashp->side[co].val,
                popCount(phashp->side[co].passed), popCount(phashp->side[co].candidate),
                phashp->side[co].c_center );
            printf("# space total=%d\n",
                phashp->side[co].c_space);
            printf("# Safety : attacks=%d atkval=%d defense=%d press=%d shelter=%d score=%d\n", c_kingAtk[co], kingAtkVal[co], 0/*c_kingDef[co]*/, kingRingPressure[co], valKingCastle[co], safetyval[co]);
#ifdef _DEBUG
            printf("#  mobility for pieces\n# ");
            for(int i = 0; i < mobDebugIdx; i ++)
                if( (mobDebugTable[i].piece & 1) == co )
                    printf("%c (%d)  ", stringPc[mobDebugTable[i].piece], mobDebugTable[i].mobility);
            printf("\n");
            fflush(stdout);
#endif
        }
    }
#endif
    stats.cFullEval++;
    int score = delta;

/*    if( (pste-1)->checkf ) {
        int depth = int(pste - pcon->argste) + 1;
        if( depth >= 5 && (pste-3)->checkf && (pste-5)->checkf ) {
            score = 3 * score / 4;
            if( depth >= 9 && (pste-7)->checkf && (pste-9)->checkf )
                score = 3 * score / 4;
            if( thisEval && thisEval->val != score ) {
                score = score;
            }
            pste->val = score;
            //pste->evalislazy = false;
            pste->evaluated  = true;
            //don't store path dependant eval !!
            //evalCache->set( pste, score);
            return score;
        }
    }*/
    if( score >= 0 )
        score = score & ~(2 - 1);
    else
        score = -((-score) & ~(2 - 1));
/*    if( thisEval && thisEval->val != score ) {
        score = score;
    }*/
    evalCache->set( pste, score);
    pste->val = score;
    pste->evalislazy = false;
    pste->evaluated  = true;
    return score;
}

//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-

/*
//  a   b   c   d   e   f   g   h
	0,  0,  0,  0,  0,  0,  0,  0,
	0,  0,  0,  0,  0,  0,  0,  0,
	0,  0,  0,  0,  0,  0,  0,  0,
	0,  0,  0,  0,  0,  0,  0,  0,
	0,  0,  0,  0,  0,  0,  0,  0,
	0,  0,  0,  0,  0,  0,  0,  0,
	0,  0,  0,  0,  0,  0,  0,  0,
	0,  0,  0,  0,  0,  0,  0,  0,
	//
	//	White rook.
	//
//  a   b   c   d   e   f   g   h
	4,  4,  4,  4,  4,  4,  4,  4,
	4,  4,  4,  4,  4,  4,  4,  4,
	4,  4,  4,  4,  4,  4,  4,  4,
	4,  4,  4,  4,  4,  4,  4,  4,
	4,  4,  4,  4,  4,  4,  4,  4,
	4,  4,  4,  4,  4,  4,  4,  4,
	4,  4,  4,  4,  4,  4,  4,  4,
	4,  4,  4,  4,  4,  4,  4,  4,
	//

*/
#if 0
static U64 kposit[8];
static int pieceToPatBit[16];

static INLINE unsigned int squarePattern(const CON *pcon, int n, int f, int r, int sq) {
        kposit[n] = IdxToU64(sq);
        if(f < 0 || f > 7 || r < 0 || r > 7)
                return 1;
        else if(pcon->pieces[B_OCCU] & IdxToU64(sq))
                return 1;
        return 0;
}
static INLINE unsigned int squarePattern2(const CON *pcon, int n, int sq) {
        kposit[n] = IdxToU64(sq);
//      if(pieces[B_OCCU] & IdxToU64(sq))
//              return 1;
//      return 0;
        return pieceToPatBit[ pcon->pos[sq] ];
}
static INLINE unsigned int squarePattern3(const CON *pcon, int n, int sq) {
        kposit[n] = ((1LL)<<sq);
//      if(pieces[B_OCCU] & (C64(1)<<sq))
//              return 1;
//      return 0;
        // checking for white pieces is ok
//      if(pos[sq] & 1)
//              return 1;
        // checking for black pieces is not ok
        return pieceToPatBit[ pcon->pos[sq] ];
//      if(pos[sq] && !(pos[sq] & 1))
//              return 1;
//      return 0;
}

static unsigned int checkPattern(const CON *pcon) {
        int kpos = pcon->king[B];
        unsigned int pattern = 0;
        int f = File(kpos);
        int r = Rank(kpos);
//      pattern = (pattern << 1) | squarePattern(f,r,kpos);
        pattern = (pattern << 1) | squarePattern(pcon, 0, f-1,r,kpos-1);
        pattern = (pattern << 1) | squarePattern(pcon, 1, f-1,r+1,kpos+7);
        pattern = (pattern << 1) | squarePattern(pcon, 2, f,r+1,kpos+8);
        pattern = (pattern << 1) | squarePattern(pcon, 3, f+1,r+1,kpos+9);
        pattern = (pattern << 1) | squarePattern(pcon, 4, f+1,r,kpos+1);
        pattern = (pattern << 1) | squarePattern(pcon, 5, f-1,r-1,kpos-9);
        pattern = (pattern << 1) | squarePattern(pcon, 6, f,r-1,kpos-8);
        pattern = (pattern << 1) | squarePattern(pcon, 7, f+1,r-1,kpos-7);
        return pattern;
}

static unsigned int checkPattern2(const CON *pcon) {
        int kpos = pcon->king[B];
        unsigned int pattern = 0;
        int f = File(kpos);
        int r = Rank(kpos);
        if( f == 0 || f == 7 || r == 0 || r == 7 )
                return checkPattern(pcon);

        pattern = (pattern << 1) | squarePattern2(pcon, 0, kpos-1);
        pattern = (pattern << 1) | squarePattern2(pcon, 1, kpos+7);
        pattern = (pattern << 1) | squarePattern2(pcon, 2, kpos+8);
        pattern = (pattern << 1) | squarePattern2(pcon, 3, kpos+9);
        pattern = (pattern << 1) | squarePattern2(pcon, 4, kpos+1);
        pattern = (pattern << 1) | squarePattern2(pcon, 5, kpos-9);
        pattern = (pattern << 1) | squarePattern2(pcon, 6, kpos-8);
        pattern = (pattern << 1) | squarePattern2(pcon, 7, kpos-7);
        return pattern;
}

static unsigned int checkPattern3(const CON *pcon) {
        int kpos = pcon->king[B];
        unsigned int pattern = 0;
//        int f = File(kpos);
//        int r = Rank(kpos);

        if( kpos == G8 ) {
                pattern = (pattern << 1) | squarePattern3(pcon, 0, F8);
                pattern = (pattern << 1) | 1;
                pattern = (pattern << 1) | 1;
                pattern = (pattern << 1) | 1;
                pattern = (pattern << 1) | squarePattern3(pcon, 4, H8);
                pattern = (pattern << 1) | squarePattern3(pcon, 5, F7);
                pattern = (pattern << 1) | squarePattern3(pcon, 6, G7);
                pattern = (pattern << 1) | squarePattern3(pcon, 7, H7);
        }
        return pattern;

}

struct bitPat {
        unsigned char bishop_attack : 1;
        unsigned char queen_attack : 1;
        unsigned char pawn_attack : 1;
        unsigned char knight_attack : 1;
        unsigned char square_defended : 1;
} ;

typedef struct {
        struct bitPat   atkdef[8];
        unsigned char   score;
        unsigned char   atk_count;
        unsigned char   def_count;
} patEntry;

static patEntry        chunks[256];

static int checkKingAtk(const CON *pcon) {
        int idx = checkPattern3(pcon);
        patEntry        pat = chunks[idx];
        U64 bishop_attack = rand();
        U64 queen_attack = rand();
        U64 knight_attack = rand();
        U64 all_defense = rand();
        int atk_nok = 0;
        int def_nok = 0;
        for(int i = 0; i < 8 ; i++) {
                U64 sqMask = kposit[i];
                if( pat.atkdef[i].bishop_attack )
                        if( (bishop_attack & sqMask) == 0 )
                                atk_nok++;
                if( pat.atkdef[i].queen_attack )
                        if( (queen_attack & sqMask) == 0 )
                                atk_nok++;
                if( pat.atkdef[i].knight_attack )
                        if( (knight_attack & sqMask) == 0 )
                                atk_nok++;
                if( pat.atkdef[i].square_defended == 0 )
                        if( (all_defense & sqMask) != 0 )
                                def_nok++;
        }

        int atk_ok = (pat.atk_count - atk_nok);
        int def_ok = 8 - def_nok;
//      return (atk_ok / pat.atk_count) * (def_ok) / 8;
        return (32*atk_ok*def_ok) / pat.atk_count;
}
#endif
