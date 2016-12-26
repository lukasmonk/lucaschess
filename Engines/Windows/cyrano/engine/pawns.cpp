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
#include "eval.hpp"
#include "moves.hpp"
#include "genmagic.hpp"
#include "stats.hpp"

#include <stdlib.h>
#include <stdarg.h>
#include <ctype.h>

#ifdef	DEBUG
static char const s_aszModule[] = __FILE__;
#endif

//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-


static HASHPawn *s_rghashp;			// Pawn hash table.
static unsigned	s_chashpMac = 0;	// Number of elements in the table, minus one.  I
							//  do the minus one thing in order to avoid an
							//  inner-loop subtract, which is no big deal, but
							//  every little bit helps.
static bitboard centerControlMask[2];
static bitboard pawnon6or7Mask[2];
bitboard pawnFileMask[8];
static bitboard pawnAdjacentFileMask[8];
static bitboard pawnAttackSpanMask[2][64];
static bitboard pawnAttackRearSpanMask[2][64];
static bitboard pawnFrontSpanMask[2][64];

#ifdef _DEBUG
#define DODEBUG(i)  i
#else
#define DODEBUG(i)
#endif

//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-

#ifdef _DEBUG

//	I sat here and thought about how to write this routine, for about five
//	minutes.  I could have written something that would have required
//	modification any time the pawn structure stuff is messed with, but I
//	didn't like that idea, since the code I've used in the past for this
//	routine gets kind of annyong to change.

//	On the other hand, this program is supposed to be simple and direct.

//	Finally, I could not resist doing something I have never done -- write
//	something that works like "printf".

//	The format string is comprised of commands.  The commands are as follows:

//		digit ('0'..'9')		The next argument is an array of "digit"
//								bitmaps, to be displayed vertically, with
//								"*" for any set bit and "." for any zero bit.
//								The argument after this is a string pointing
//								to the column header.

//		'!'						The next argument is an array of two bitmaps,
//								which should be displayed as one bitmap with
//								"P" if a bit from the first map is set, and
//								"p" if a bit from the second map is set, and
//								"." if neither bit is set.  There is nothing
//								special if both maps are set, that comes up
//								"P".  The argument after this is a string
//								pointing to the column header.

//	The column header strings should be <= 8 characters long.

//	Here's an example call:

//		VDumpPawnMaps("!1", argbmLoc, "Loc", &bmDoubled, "Doubled");

//	This would be somewhat different, but would work:

//		VDumpPawnMaps("21", argbmLoc, "Loc", &bmDoubled, "Doubled");

//	There's an example line somewhere in the code below, unless I accidentally
//	deleted it.  You can comment it out and mess with it, if you decide you
//	want to add more pawn bitmaps and dump them.

//	I'm glad I wrote this routine, rather than just assuming that my code
//	worked.  It found a bunch of bugs.



void VDumpPawnMaps(char * szFmt, ...)
{
	va_list	pvaArg;
	char * sz;

	//	Display headers and figure out how many bitmaps in the largest array.
	//
	int cbmMax = 0;
	va_start(pvaArg, szFmt);
	for (sz = szFmt; *sz != '\0'; sz++) {
		char *	szArg;
		bitboard *pbmArg;
		int	cbm;

		if (isdigit(*sz))
			cbm = *sz - '0';
		else {
			Assert(*sz == '!');
			cbm = 1;
		}
		if (cbm > cbmMax)
			cbmMax = cbm;					// <-- New largest.
		pbmArg = va_arg(pvaArg, bitboard *);		// Pull bitmap pointer.
		szArg = va_arg(pvaArg, char *);		// Pull column header.
		if (sz != szFmt)
			putchar(' ');
		printf("%-8s", szArg);				// Display arg.
	}
	putchar('\n');
	//
	//	Display the bitmaps.  The largest argument I got was "cbmMax", so I
	//	will loop that many times, displaying one row of bitmaps each time.
	//
	for (int ibm = 0; ibm < cbmMax; ibm++) {

		if (ibm != 0)
			putchar('\n');
		//
		//	Display a row of bitmaps.
		//
		for (int rnk = rnk8; rnk >= rnk1; rnk--) {
			//
			//	Display one rank from all the bitmaps.
			//
			va_start(pvaArg, szFmt);
			for (sz = szFmt; *sz != '\0'; sz++) {
				char *	szArg;

				if (sz != szFmt)
					putchar(' ');
				int cbm = (isdigit(*sz)) ? *sz - '0' : 1;
				bitboard *pbmArg = va_arg(pvaArg, bitboard *);
				szArg = va_arg(pvaArg, char *);
				if (ibm >= cbm)			// This sometimes leaves trailing
					printf("        ");	//  spaces, but it's not worth fixing.
				else		// Display one rank from one bitmap.
					for (int fil = filA; fil <= filH; fil++) {
						bitboard bm;

						int isq = Idx(rnk, fil);
						bm = IdxToU64(isq);
						if (*sz != '!')
							putchar((bm & pbmArg[ibm]) ? '*' : '.');
						else if (bm & pbmArg[coWHITE])
							putchar('P');
						else
							putchar((bm & pbmArg[coBLACK]) ? 'p' : '.');
					}
			}
			putchar('\n');
		}
	}
}
#endif

//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-
//  http://www.exeterchessclub.org.uk/pawnform.html

//	Penalties for pawn defects.

//#define	valDOUBLED_ISO	20	// Doubled isolated pawn.
//#define	valISOLATED		10  // Strictly isolated pawn (no pawns of this color
							//  on either adjacent file).
//#define valISOLATED_OPEN 6 // isolated pawn on semi-open file (*added* to other penalities)

#define bBACKWARDMG     10
#define bBACKWARDweak   (20-bBACKWARDMG)
#define bBACKWARDEG     10

#define bIsolatedMG     10
#define bIsolatedOpen   (20-bIsolatedMG)    // 25
#define bIsolatedEG     10  //15

#define bDoubledMG      15
#define bDoubledEG      20  //was 30

// Bonus for pawns

//#define valCONNECTED    3   // this will be counted twice (once for each pawn)
//#define valCONNECTED    6   // this will be counted twice (once for each pawn)
//#define valCONNECTED    4   // this will be counted twice (once for each pawn)

// duo bonus indexed by file
//static const int bDuoBonus[8] = {2,2,6,9,9,6,2,2};
static const int bDuoBonus[8] = {1,1,2,3,3,2,1,1};

// Normal doubled pawn, not strictly isolated. (idx=pawn count)
//static const int doubledPenality[9] = {0,0,30,25,18,15,12,11,10};
static const int doubledPenality[9] = {0,0,30,25,18,15,11,9,6};
//static const int doubledPenality[9] = {0,0,15,13,11,9,7,6,5};

// todo:add bad features counter

// file bonus for pawns
// this could be included in pst but it's simpler here
//static int pawnFileValMG[8] = {-5,0,0,0,0,0,0,-5};
static int pawnFileValMG[8] = {-10,-5,0,5,5,0,-5,-10};
//static int pawnFileValEG[8] = {10,5,0,0,0,0,5,10};
static int pawnFileValEG[8] = {0,0,0,0,0,0,0,0};

//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-
static int tempTable[64];

// SetDistance/SetNextDistance is used to compute a path for a king from a square to another square
// The distance between a square and the destination square is stored in a square array
// so the distance can be known for each square of the board with a simple table lookup
//
void setNextDistance(int rank, int file, const CON *pcon, HASHPawn *phashp, int side, int distance) {
    // check if on board
    if( rank < 0 || rank > 7 || file < 0 || file > 7 )
        return;
    int from = Idx(rank, file);
    if( distance ) {
        // exit if square is unavailable (pawn or attacked by a pawn)
        if( (pcon->pos[from]|1) == (PAWN|1) )
            return;
        bitboard thisSqr = IdxToU64(from);
        if( phashp->side[side^1].pawnatk & thisSqr )
            return;
    }
    // update distance (depending on path taken)
    // exit if a shorter was leading to this square
    if( distance >= tempTable[from] )
        return;
    tempTable[from] = distance;
    if( distance < 15 )
        distance ++;
    setNextDistance(rank+1, file+0, pcon, phashp, side, distance);
    setNextDistance(rank-1, file+0, pcon, phashp, side, distance);
    setNextDistance(rank+0, file+1, pcon, phashp, side, distance);
    setNextDistance(rank+0, file-1, pcon, phashp, side, distance);
    setNextDistance(rank+1, file+1, pcon, phashp, side, distance);
    setNextDistance(rank+1, file-1, pcon, phashp, side, distance);
    setNextDistance(rank-1, file+1, pcon, phashp, side, distance);
    setNextDistance(rank-1, file-1, pcon, phashp, side, distance);
}
void setDistance(const CON *pcon, HASHPawn *phashp, int from, squareDist *keySq, int winningSide) {
    keySq->square = from;
    keySq->winningSide = winningSide;
    for(int side = 0; side <= 1; side ++) {
        for(int i = 0; i <= 63; i++)
            tempTable[i] = 15;
        setNextDistance(Rank(from), File(from), pcon, phashp, side, 0);
        memcpy(keySq->distance[side], tempTable, sizeof(keySq->distance[side]));
#ifdef DEBUG
        printf("\n");
	    for (int rnk = rnk8; rnk >= rnk1; rnk--) {
		    for (int fil = filA; fil <= filH; fil++) {
			    int isq = Idx(rnk, fil);
                if( tempTable[isq] == 15)
                    printf(" -");
                else if( tempTable[isq] )
			        printf(" %d", tempTable[isq]);
                else
                    printf(" o");
		    }
		    printf("\n");
	    }
#endif
    }
}

// from Steffan Westcott / Gerd Isenberg
// http://chessprogramming.wikispaces.com/All+Shortest+Paths
/////////////////////////////////////////////////////////////////////////
//
// Returns length of shortest path of set bits present in 'path' that
// 8-way connect any set bit in sq1 to any set bit of sq2. 0 is returned
// if no such path exists. Also fills a sequence of bitboards asp[length]
// that describes all such shortest paths.

int allShortestPaths(U64 sq1, U64 sq2, U64 path, U64* asp)
{
    // Do an 8-way flood fill with sq1, masking off bits not in path and
    // storing the fill frontier at every step. Stop when fill reaches
    // any set bit in sq2 or quit if fill cannot progress any further.
    // Then do 8-way flood fill from reached bits in sq2, ANDing the
    // frontiers with those from the first fill in reverse order.

    if (!(sq1 &= path) || !(sq2 &= path)) return 0;
                        // Drop bits not in path
                        // Early exit if sq1 or sq2 not on any path
    int i = 1;
    asp[0] = sq1;

    while(1)  // Fill from all set bits in sq1, to any set bit in sq2
    {
        if (sq1 & sq2) break;                     // Found good path
        U64 temp = sq1;
        sq1 |= eastOne(sq1) | westOne(sq1);       // Set all 8 neighbours
        sq1 |= soutOne(sq1) | nortOne(sq1);
        sq1 &= path;                              // Drop bits not in path
        if (sq1 == temp) return 0;                // Fill has stopped
        asp[i++] = sq1 & ~temp;                   // Store fill frontier
    }

    int length = i;                               // Remember path length
    asp[--i] = (sq2 &= sq1);                      // Drop unreached bits

    while(i)  // Fill from reached bits in sq2
    {
        U64 temp = sq2;
        sq2 |= eastOne(sq2) | westOne(sq2);       // Set all 8 neighbours
        sq2 |= soutOne(sq2) | nortOne(sq2);
        sq2 &= path;                              // Drop bits not in path
        asp[--i] &= sq2 & ~temp;                  // Intersect frontiers
    }
    return length;
}

// This function returns the length of the shortest path, not the path.
// This is for pawn endgames.
// The initial path should contain the whole board minus occupied squares, minus attacked squares
// also it is not called for pawns defended by other pawns (sq2 is not in the path in this case)
int pathLength(U64 sq1, U64 sq2, U64 path)
{
    // Do an 8-way flood fill with sq1, masking off bits not in path and
    // storing the fill frontier at every step. Stop when fill reaches
    // any set bit in sq2 or quit if fill cannot progress any further.
    // Then do 8-way flood fill from reached bits in sq2, ANDing the
    // frontiers with those from the first fill in reverse order.

    if (!(sq1 &= path) || !(sq2 &= path)) return 99;
                        // Drop bits not in path
                        // Early exit if sq1 or sq2 not on any path
    int i = 0; //1;

    while(1)  // Fill from all set bits in sq1, to any set bit in sq2
    {
        if (sq1 & sq2) break;                     // Found good path
        U64 temp = sq1;
        sq1 |= eastOne(sq1) | westOne(sq1);       // Set all 8 neighbours
        sq1 |= soutOne(sq1) | nortOne(sq1);
        sq1 &= path;                              // Drop bits not in path
        if (sq1 == temp) return 99;                // Fill has stopped
        i++;
    }
//    VDumpPawnMaps("1", &sq1, "path");
    int length = i;                               // Remember path length
    return length;
}

static INLINE int pawnSpace(const CON *pcon, int co, int file) {
    int space = 0;
    int dir = 1;
    if(co == B)
        dir = -1;
    for(int rank = 1; rank <= 6; rank++) {
        int r = co == W ? rank : 7-rank;
        U8 c = pcon->pos[ Idx(r,file) ];
        if( AnyColor(c) == AnyColor(PAWN) ) {
            U8 oppPawn = PAWN|FlipColor(co);
            // look if my pawn is blocked with an opposite color pawn
            if( c != oppPawn )
                if( pcon->pos[ Idx(r+dir,file) ] == oppPawn )
                    space--;
            break;
        }
        space++;
    }
    return space;
}


//                               A B C D E F G H
static int fileDefectCount[8] = {3,2,2,2,2,1,2,3};
static int fileDefectValue[8] = {9,6,6,6,6,2,6,9};

// friendly pawns in front of the king
// ennemy pawns in front of the king
// Same as the old code, except we do not compute the final score
static void pawnOnKingCastle(const CON *pcon, const STE *pste, int co, int sqr, short &val, const HASHPawn *phashp) {
    const U8 *pos = pcon->pos;
    int dir = co == W ? 1 : -1;
    int dir8 = co == W ? 8 : -8;
    int r = Rank(sqr);
    int f = File(sqr);

    U8 myPawn = U8(PAWN|co);
    U8 theirPawn = U8(PAWN|(co^1));

    val = 0;
    if( (r+dir*3>=0 && r+dir*3<=7) )
    for(int ff = -1; ff <= 1; ff++) {
        int fp = f + ff;
        if( fp >= 0 && fp <= 7 ) {
            int defect = 0;
            int sq = Idx(r,fp);
            // shelter
            if( pos[sq+dir8] == myPawn ) {
                defect = 0; //6+a; //8;
            } else if( pos[sq+dir8*2] == myPawn ) {
                defect = 12; //3+a; //3;
            } else if( pos[sq+dir8*3] == myPawn ) {
                defect = 14+fileDefectValue[fp];
            } else {
                defect = 14+fileDefectValue[fp] * 2;
            }
            val -= defect;
            if( ff == 0 )   // king file
                val -= defect;
            // pawn storm
            if( pos[sq+dir8] == theirPawn ) {   // 7th
//                val -= 20; //50; // allready counted as passed pawn
            } else if( pos[sq+dir8*2] == theirPawn ) {  // 6th
                val -= 25; //12;
            } else if( pos[sq+dir8*3] == theirPawn ) {  // 5th
                val -= 15; //7
            } else if( pos[sq+dir8*4] == theirPawn ) {  // 4th
                val -= 5; //7
            } else if( phashp->side[co^1].c_pawn[fp] == 0 ) {
                val -= fileDefectValue[fp];
            }
        }
    }
}

//	This gets the pawn structure info from the hash table if it's there,
//	otherwise it generates it.

//	In most cases, the stuff is in the hash table.  The hash table is big
//	enough, and the pawn structures don't change that much.

//	Since the pawn computation stuff isn't done that much, I don't take
//	special pains to make sure that it's fast.  It's better to be clear and
//	accurate.  It would be very possible to generate the defect values via
//	less complicated data structures, and fewer passes over the data, but
//	the excess time shoudn't matter.

//	On the other hand, the whole thing stays flexible so it should be easy
//	to add to.

//	This routine returns values from the perspective of the side to move,
//	and stores values in the hash table from white's perspective.

//	There are a lot of other pawn features that can be detected, but doubled
//	and isolated pawns should be good enough for now.  The trick is to play
//	solidly enough that the program doesn't drop pawns a few moves over its
//	horizon.

//	An obvious addition is *anything* that would help with the passed pawn
//	problem, which is always severe in a simple program.  The program could
//	also benefit from an understanding of backwardness and artificial
//	isolation.  Additionally, the values given for doubled and isolated pawns
//	could vary depending upon where the pawns are on the board, and the
//	condition of the opponent's pawns (a doubled isolated pawn on an open file
//	is worse than one on a closed file).

//	In a heavier implementation, some of the pawn maps might end up being
//	stored in the hash table itself, which is another level of complication.

static void ValPawns(const CON *pcon, const STE *pste)
{
    HASHPawn *phashp = &s_rghashp[pste->hashkPn & s_chashpMac];

    bitboard argbmLoc[coMAX];       // Bits set for pawns that exist.
    bitboard bmDoubled = 0;                  // Bit set indicates doubled pawn.
    bitboard bmIsolated = 0;                 // Bit set indicates isolated pawn.
    bitboard    blocked[coMAX];

    argbmLoc[W] = pcon->pieces[WP];
    argbmLoc[B] = pcon->pieces[BP];
//    phashp->side[W].location = argbmLoc[W];
//    phashp->side[B].location = argbmLoc[B];
    phashp->pawnon6or7 = 0 != ((pawnon6or7Mask[B] | pawnon6or7Mask[W]) & (argbmLoc[B] | argbmLoc[W]));

    phashp->c_center_pawns = popCount( (argbmLoc[W] | argbmLoc[B]) & centerMask);
    // new version with more setwise operations, thx Gerd
    bitboard bFrontSpansBB = bFrontSpans(argbmLoc[B]);
    bitboard wFrontSpansBB = wFrontSpans(argbmLoc[W]);
    // blocked pawns
    blocked[W] = argbmLoc[W] & bFrontSpansBB;
    blocked[B] = argbmLoc[B] & wFrontSpansBB;
    // attacks
    {
	    // - Pawns seven-shift -
	    phashp->side[W].pawnatk = ((argbmLoc[W] & CLEAR_LEFT) << 7) |
	    // - Pawns nine-shift -
	        ((argbmLoc[W] & CLEAR_RIGHT) << 9);
	    // - Pawns seven-shift -
	    phashp->side[B].pawnatk = ((argbmLoc[B] & CLEAR_RIGHT) >> 7) |
	    // - Pawns nine-shift -
	        ((argbmLoc[B] & CLEAR_LEFT) >> 9);
    }
    // passed pawns
    U64 allFrontSpans;
    allFrontSpans = bFrontSpansBB | eastOne(bFrontSpansBB) | westOne(bFrontSpansBB);
    phashp->side[W].passed = argbmLoc[W] & ~allFrontSpans;
    // caution, span color inverted, eliminating doubled pawns, only the front one is passed
    phashp->side[W].passed &= ~bFrontSpans(phashp->side[W].passed); // HACK
    allFrontSpans = wFrontSpansBB | eastOne(wFrontSpansBB) | westOne(wFrontSpansBB);
    phashp->side[B].passed = argbmLoc[B] & ~allFrontSpans;
    phashp->side[B].passed &= ~wFrontSpans(phashp->side[B].passed); // HACK

    int	argval[coMAX] = {0,0};              // Total value of pawn features.
    int argendgval[2] = {0,0};

    for (int coUs = coBLACK; coUs <= coWHITE; coUs++) {
//        phashp->side[coUs].c_doubled = 0;
//        phashp->side[coUs].c_islands = 0;
        phashp->side[coUs].candidate = 0;
//        phashp->side[coUs].c_backward= 0;
//        phashp->side[coUs].c_connected=0;
        phashp->side[coUs].c_center  = 0;
        // count the number of pawns on this file
        for(int f = 0; f < 8 ; f++)
            phashp->side[coUs].c_pawn[f] = 0;

        // examine passed pawn
        bitboard pseudo_candidates = argbmLoc[coUs] & ~blocked[coUs] &~phashp->side[coUs].passed;
        for (bitboard pawns = pseudo_candidates; pawns; ) {
            int sqr = PopLSB(pawns);
            bitboard helpers = pawnAttackRearSpanMask[coUs][sqr] & argbmLoc[coUs];
            int guard = 0;
            if( helpers ) { // one helper or more
                guard --;
                if(helpers & (helpers - 1) )    // a second helper of more
                    guard--;
            }
            if(guard < 0) {
                bitboard tmp = pawnAttackSpanMask[coUs][sqr] & argbmLoc[coUs^1];
                guard ++;  // we know there is at least one guard since the pawn is not passed
                if( tmp & (tmp - 1) )   // two guards or more
                    guard++;
                if( tmp & (tmp - 1) )   // there can be more than 2 guards (doubled pawns)
                    guard++;
                // if we have a more or equal number of helpers it's a candidate pawn
                // guard == 0 => 2v1 or 3vs2
                if( guard <= 0 ) {
                    bitboard thispawn = IdxToU64(sqr);
                    phashp->side[coUs].candidate |= thispawn;
                    // give a little bonus for candidate pawns
                    // protected candidate
                    if( thispawn & phashp->side[coUs].pawnatk ) {
                        argval[coUs] += 3;
                        argendgval[coUs] += 3;
                    }
                    unsigned int rankBonus = pawn_bonus_to_7[coUs][Rank(sqr)] / 2;
                    argval[coUs] += rankBonus;
                    argendgval[coUs] += (rankBonus);   // allready added once (was 2)
                }
            }
        }

        //	Collect doubled pawn info.  I'm see if there is more than one
        //	pawn of a certain color on the a-file, if there is I will OR
        //	all of those pawns into the "bmDoubled" bitmap, then I'll try
        //	the b-file, and so on.
        //
        // for each file...
        for (int f = 0; f <= 7; f++) {
            //	Get all pawns of this color on this file.
            //
            bitboard bmS = argbmLoc[coUs] & pawnFileMask[f];
            //
            //	If more than one bit set in this map, all the pawns of
            //	this color on this file are doubled.
            //
            if( bmS ) {
                phashp->side[coUs].c_pawn[f] = 1;
                if( bmS & (bmS - 1) )
                    bmDoubled |= bmS;
            }
            //
            if (!(pawnAdjacentFileMask[f] & argbmLoc[coUs]))
                bmIsolated |= bmS;
        }
        // count space for each side and for each flank
        // space is the number of squares behind our pawns, or the number of squares
        // in front of the opponent pawns if we don't have a pawn on the file
        // So an open or semi-open file gives a lot of space and advancing pawns
        // gives some space too
/*        phashp->side[coUs].c_Qspace  = pawnSpace(pcon, coUs, filA)
                                    + pawnSpace(pcon, coUs, filB)
                                    + pawnSpace(pcon, coUs, filC);
        phashp->side[coUs].c_Cspace  = pawnSpace(pcon, coUs, filD)
                                    + pawnSpace(pcon, coUs, filE);
        phashp->side[coUs].c_Kspace  = pawnSpace(pcon, coUs, filF)
                                    + pawnSpace(pcon, coUs, filG)
                                    + pawnSpace(pcon, coUs, filH);
        phashp->side[coUs].c_space = (phashp->side[coUs].c_Qspace +
            1*phashp->side[coUs].c_Cspace +
            phashp->side[coUs].c_Kspace);*/
	}
    int space = 0;
    for(int f = filA; f <= filH ; f++)
        space += pawnSpace(pcon, W, f);
    phashp->side[W].c_space = space;
    space = 0;
    for(int f = filA; f <= filH ; f++)
        space += pawnSpace(pcon, B, f);
    phashp->side[B].c_space = space;

    // queen side pawn majority
    bool QS_candidates[2];
    QS_candidates[W] = (phashp->side[W].candidate & QSideMask) != 0;
    QS_candidates[B] = (phashp->side[B].candidate & QSideMask) != 0;

    DODEBUG(bitboard backward_pawns = 0;)
	for (int coUs = coBLACK; coUs <= coWHITE; coUs++) {
//        phashp->side[coUs].passed = argbmLoc[coUs] &
//            (~blocked[coUs]) & (~phashp->side[coUs ^ 1].influence) ;
        // control of the center
        phashp->side[coUs].c_center = popCount( phashp->side[coUs].pawnatk & centerControlMask[coUs] );

        // for each pawn of the board...
//        int valDoubled = doubledPenality[ pcon->msigCount[PAWN|coUs] ];
        int val = 0;
        int endgval = 0;
        for (bitboard pawns = argbmLoc[coUs]; pawns; ) {
            int sqr = PopLSB(pawns);
            bitboard bmLoc = IdxToU64(sqr);
            int f = File(sqr);
            // next line is not so correct (for doubled pawns and special positions)
//            bool fileIsOpen = (phashp->side[coUs^1].c_pawn[f] == 0);
            bool fileIsOpen = (pawnFrontSpanMask[coUs][sqr] & (argbmLoc[W]|argbmLoc[B])) == 0;
#if 0
            printf("befor pawn %c%c val=%d endg=%d\n", File(sqr) + 'a', Rank(sqr) + '1', val, endgval);
#endif

            val += pstVal[PAWN|coUs][sqr];
            val += pawnFileValMG[ f ];
            endgval += pawnFileValEG[ f ];
            int rankBonus = pawn_bonus_to_7[coUs][Rank(sqr)] / 2;
            endgval += rankBonus;

            if( bmLoc & phashp->side[coUs].passed ) {
                val += rankBonus;
                endgval += (rankBonus); // allready added once (was 2)
/*                if (bmLoc & bmDoubled) {
                    val -= valDoubled / 2;
                    endgval -= valDoubled / 2;
                }
                if (bmLoc & bmIsolated) {
                    val -= valISOLATED;
                    endgval -= valISOLATED / 2;
                }*/
            }

            if (bmLoc & (bmIsolated | bmDoubled) ) {
                if(bmLoc & bmDoubled) {
                    // will be applied twice
                    val -= bDoubledMG;
                    endgval -= bDoubledEG;
                }
                if(bmLoc & bmIsolated) {
                    if( fileIsOpen ) {
                        val -= bIsolatedOpen;
                        endgval -= bIsolatedOpen / 2;
                    }
                    val -= bIsolatedMG;
                    endgval -= bIsolatedEG;
                }
            } else {
                int dir = coUs == W ? -1 : 1;
                int r = Rank(sqr);
                bool hasFriends = false;
                while(r > 0 && r < 7 ) {
                    if( f > 0 && pcon->pos[Isq64FromRnkFil(r,f - 1)] == (PAWN|coUs)) {
                        hasFriends = true;
                        break;
                    }
                    if( f < 7 && pcon->pos[Isq64FromRnkFil(r,f + 1)] == (PAWN|coUs)) {
                        hasFriends = true;
                        break;
                    }
                    r += dir;
                }
                // connected pawns
                if( hasFriends ) {
                    if( r == Rank(sqr) ) {
//                        phashp->side[coUs].c_connected++;
                        val += bDuoBonus[ f ];
                        endgval += bDuoBonus[ f ];
                    }
                // non isolated pawn on an open file
                } else {
                    // no helpers around, see who attacks the square in front of the pawn
                    bitboard frontSqr = IdxToU64(sqr);
                    bool is_backward = false;
                    do {
                        if( coUs == coBLACK )
                            frontSqr = soutOne(frontSqr);
                        else
                            frontSqr = nortOne(frontSqr);
                        if( phashp->side[coUs].pawnatk & frontSqr )
                            break;
                        if( phashp->side[coUs^1].pawnatk & frontSqr ) {
                            is_backward = true;
                            break;
                        }
                    } while(frontSqr);
                    if( is_backward ) {
                        if( f <= 2 )
                            QS_candidates[coUs] = false;
                        DODEBUG(backward_pawns |= IdxToU64(sqr);)
                        val -= bBACKWARDMG;
                        endgval -= bBACKWARDEG;
                        if( fileIsOpen )
                            val -= bBACKWARDweak;
                    }
                }
            }
#if 0
            printf("after pawn %c%c val=%d endg=%d\n", File(sqr) + 'a', Rank(sqr) + '1', val, endgval);
#endif
		}
        argval[coUs] += val;
        argendgval[coUs] += endgval;
	}
    for(int f = 0 ; f <= 7 ; f++) {
        pawnOnKingCastle(pcon, pste, W, A1+f, phashp->side[W].KSI[f].val, phashp);
        pawnOnKingCastle(pcon, pste, B, A8+f, phashp->side[B].KSI[f].val, phashp);
    }

    // majority if we have a candidate pawn and not the opponent
    if( QS_candidates[W] && !QS_candidates[B] ) {
        argval[W] += 7;
        argendgval[W] += 10;
    }
    if( QS_candidates[B] && !QS_candidates[W] ) {
        argval[B] += 7;
        argendgval[B] += 10;
    }

#ifdef _DEBUG
    static int dump_count = 0;
    dump_count++;
    if( dump_count <= 1 ) {

//        bitboard blocked[2] = {phashp->side[0].blocked,phashp->side[1].blocked};
        bitboard passed[2] = {phashp->side[0].passed, phashp->side[1].passed };
        bitboard candidates[2] = {phashp->side[0].candidate, phashp->side[1].candidate };
        VDumpPawnMaps("!11!!1",
            argbmLoc, "Loc",
            &bmDoubled, "Doubled",
            &bmIsolated, "Isolated",
            &passed, "passed",
            &candidates, "candidate",
            &backward_pawns, "backw"

        );
    }
#endif
    phashp->hashk = pste->hashkPn;
    // keep a score for each side
    phashp->side[W].val = argval[W];
    phashp->side[B].val = argval[B];
    phashp->side[B].endgval = argendgval[B];
    phashp->side[W].endgval = argendgval[W];


    // now we know if a pawn is
    // - isolated, doubled, blocked/candidate, passed
    // we also have the count of pawn per file
    // we can do rook_file_attack & location(xside) to know if a rook is on a semi open file
}

void EvalPawns(const CON *pcon, const STE *pste, PHASHPawn & phashp) {
	phashp = &s_rghashp[pste->hashkPn & s_chashpMac];

	if (phashp->hashk != pste->hashkPn) {
        stats.hp_eval++;
        ValPawns(pcon, pste);
    }
    stats.hp_hit++;
}


//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-

//	Zero all pawn hash memory.

void VClearHashp(void)
{
    if( s_chashpMac )
	    memset(s_rghashp, 0, (s_chashpMac + 1) * sizeof(HASHPawn));
}

//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-

//	This allocates the pawn hash memory according to the max value passed in
//	by the user, then clears the table.

bool FInitHashp(PCON pcon, int cbMaxPawnHash)
{
    centerControlMask[W] = centerControlMask[B] = 0;
    for(int f = 2; f <= 5; f++)
        for(int r = 3; r <= 6; r++) {
            centerControlMask[W] |= IdxToU64( Idx(r,f) );
            centerControlMask[B] |= IdxToU64( Idx(7-r,f) );
        }
    pawnon6or7Mask[B] = pawnon6or7Mask[W] = 0;
    for(int r = 0; r <= 7; r++) {
        pawnon6or7Mask[B] |= IdxToU64( A3+r );
        pawnon6or7Mask[B] |= IdxToU64( A2+r );
        pawnon6or7Mask[W] |= IdxToU64( A6+r );
        pawnon6or7Mask[W] |= IdxToU64( A7+r );
    }
    bitboard bmI = C64(0x0101010101010101);	// Start with a1...a8.
    for(int f = 0; f <= 7; f++) {
        pawnFileMask[f] = bmI;
        pawnAdjacentFileMask[f] = 0;
        if (!(bmI & 0x01))
            pawnAdjacentFileMask[f] |= bmI >> 1;
        if (!(bmI & 0x80))
            pawnAdjacentFileMask[f] |= bmI << 1;
        bmI <<= 1; // next file
    }
    for(int co = 0; co <= 1; co++)
        for(int sqr = 0; sqr <= 63; sqr++) {
            if(co == W )
                pawnAttackSpanMask[co][sqr] = nortFill(pawn_attacks[co][sqr]);
            else
                pawnAttackSpanMask[co][sqr] = soutFill(pawn_attacks[co][sqr]);
            if(co == W )
                pawnAttackRearSpanMask[co][sqr] = soutOne( soutFill(pawn_attacks[co][sqr]) );
            else
                pawnAttackRearSpanMask[co][sqr] = nortOne( nortFill(pawn_attacks[co][sqr]) );
            if( co == W )
                pawnFrontSpanMask[co][sqr] = wFrontSpans( IdxToU64(sqr) );
            else
                pawnFrontSpanMask[co][sqr] = bFrontSpans( IdxToU64(sqr) );
        }

    if( cbMaxPawnHash <= 0 || cbMaxPawnHash > 256*1024*1024 )
        cbMaxPawnHash = 1*1024*1024;

    int chashpMax = 1;
	for (;;) {
		if (chashpMax * 2 * (int)sizeof(HASHPawn) > cbMaxPawnHash)
			break;
		chashpMax *= 2;
	}
    int Hsize = chashpMax * sizeof(HASHPawn);
	VPrSendComment("%d Kbytes pawn hash memory (%d entries of %d bytes)", Hsize/1024, chashpMax, sizeof(HASHPawn));
    s_rghashp = (HASHPawn *) malloc(Hsize);
	if ( s_rghashp == NULL) {
		VPrSendComment("Can't allocate pawn hash memory: %d bytes", Hsize);
        chashpMax = 1024;
        Hsize = chashpMax * sizeof(HASHPawn);
	    VPrSendComment("%d Kbytes pawn hash memory", Hsize/1024);
        s_rghashp = (HASHPawn *) malloc(Hsize);
	    if ( s_rghashp == NULL) {
		    VPrSendComment("Can't allocate pawn hash memory: %d bytes", Hsize);
		    return false;
        }
	}
	s_chashpMac = chashpMax - 1;
	VClearHashp();
	return true;
}

//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-
