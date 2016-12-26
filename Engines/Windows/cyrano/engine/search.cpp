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

#include "magicmoves.h"
#include "engine.hpp"
#include "gproto.hpp"
#include "hash.hpp"
#include "board.hpp"
#include "moves.hpp"
#include "stats.hpp"
#include "search.hpp"
#include "eval.hpp"
#include "genmagic.hpp"
#include <stdlib.h>

#ifdef	_DEBUG
static char const s_aszModule[] = __FILE__;
#endif

// enables the PVS search, undefine to have standard alpha beta search (for bug hunting)
// reduce the number of nodes
#define NULL_WINDOW

// reduce the number of nodes
#define NULL_MOVE
//#define TWO_NULL

// allways extend depth by one ply when in check
// increase the number of nodes, find mates in less plies
#define EXTEND_CHECK
// extend the pv a bit, the score should be more reliable, and this gives a better move ordering for the next iteration
//#define SOME_PV_EXTENSION
//#define NM_THREAT_DETECTION

#define MINIMIZE

    #define FUTILITY_PRUNING
//    #define RAZORING

    //#define BM_EXTENSION
    // research in reduced disbaled too
#define LESS_CHECK

#define QS_VERIFY_EXTENSION
//#define USE_EFFORT

//#define QS_CHECK_LIMIT  (pste->plyRem > 0)
//#define QS_CHECK_LIMIT  (pste->plyRem >= 0)
//#define QS_CHECK_LIMIT  (pste->plyRem >= 1)
//#define QS_CAPT_LIMIT  (pste->plyRem >= 1)
#define QS_CAPT_LIMIT  (false)
#define QS_CHECK_LIMIT  false

// reduce the number of nodes
#define USE_HASH
// change move ordering => reduce the number of nodes
#define USE_KILLERS

// late move reduction
#define USE_LMR // disabled to test things
// skip descending the tree for some useless xchange in the quiescent search

#define PRUNE_QS
// ...
// this will stop search when we have a score good enought (mate)
#define DISTANCE_PRUNING

#define RECAPTURE_EXTENSION

//#define MOVE_SAFETY_SORT  // this is not so bad
//#define MOVE_PST_SORT     // this is not so good
#define MOVE_HISTORY_SORT

#define HISTORY

#define USE_IID
//#define HASH_NM       // this is a problem in endgame ???
#define EVAL_INTERIOR
#define USE_RECOGNIZER

#define moveisCapture(pcm) (Capt((pcm)->m) != 0)
//#define moveisEqualCapture(pcm) (((pcm)->cmk & (cmkEQUCAPT)) != 0)
// equ capt does not exists anymore
#define moveisGoodCapture(pcm) (((pcm)->cmk & (cmkGOODCAPT|cmkEQUCAPT)) != 0)
#define moveisVeryGoodCapture(pcm) (((pcm)->cmk & (cmkGOODCAPT)) != 0)
#define moveisBadCapture(pcm) (((pcm)->cmk & (cmkBADCAPT)) != 0)
#define moveisPromotion(pcm) ((Pc((pcm)->m)|1) == (B_PROM|1))
#define moveisPawnMove(pcm) ((Pc((pcm)->m)|1) == (PAWN|1))
#define moveto7thRank(pcm)  ((Rank(To((pcm)->m)) == 6) || (Rank(To((pcm)->m)) == 1))
#define moveisGood(pcm) (((pcm)->cmk & (cmkMASK)) >= cmkQUEEN)
//#define moveisGood(pcm) (((pcm)->cmk & (cmkMASK)) >= cmkEQUCAPT)



#define FRONTIER    0
#define isFrontier(pste)    ((pste)->plyRem == FRONTIER)
#define isRazorDepth(pste)    ((pste)->plyRem == (FRONTIER+2))

static int SearchQ(PCON pcon, PSTE pste, int Alpha, int Beta, bool do_check);
static int Search(PCON pcon, PSTE pste, int Alpha, int Beta, nodeType nt);
static int SearchPV(PCON pcon, PSTE pste, int Alpha, int Beta);
static int INLINE capture_value(PCON pcon, const STE *pste, const CM *pcm);

//#define BM_DEPTH    2
#define BM_DEPTH    5
#define MAXEXT(n)   (50)
//#define MAXEXT(n)   (20+(n))
//#define MAXEXT(n)   (12+(n))
//#define MAXEXT(n)   (12)
//#define MAXEXT(n)   (70)
// 1 => 20
// 10 => 30
// 20 => 35
// 20 + n/2     1=>20     5=>22     10=>25    15=>27    20=>30  25=>32  30=>35  (40=>40)
//#define MAXEXT(n)   (20+(n)/2)
// 10 + n/2     1=>10=>11     5=>12=>17     10=>15=>25    15=>17=>32    20=>20  25=>22  30=>25  (20=>20)
//#define MAXEXT(n)   (1)
#define my_swap(a,b,t) {t tmp = a; a = b; b = tmp;}

static bool inSearch;
//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-

static void SortKillers(PSTE pste, int depth)
{
    U32 Killer1 = Killers[depth].Killer1;
    U32 Killer2 = Killers[depth].Killer2;
    U32 Killer3 = Killers[depth-2].Killer1;
    U32 Killer4 = Killers[depth-2].Killer2;
    U32 cm1, cm2;
    CounterMoves.get( (pste-1)->lastMove, cm1, cm2);
    foreachmove(pste, pcm)
/*        if ( (pcm->m == cm1) || (pcm->m == cm2) ) {
			pcm->cmk |= cmkKILLER | 7;
            stats.counterHit++;
        }
        else*/
        if( (pcm->m == MateKillers[depth].Killer1) || (pcm->m == MateKillers[depth].Killer2)) {
			pcm->cmk |= cmkKILLER | 5;
            stats.Killers++;
        } else if ( (pcm->m == Killer1) || (pcm->m == Killer2) ) {
			pcm->cmk |= cmkKILLER | 3;
            stats.Killers++;
		}/* else if ( (pcm->m == Killer3) || (pcm->m == Killer4) ) {
			pcm->cmk |= cmkKILLER | 1;
            stats.Killers++;
		}*/
}
static INLINE int MaterialGain(const CON *pcon, U32 m) {
    // Capt(move) == 0 => no capture, array return 0 in this case
    return b_valPc[ Capt(m) ];
}


//	This will be used to figure out which move to search first.  It is very
//	important to try to search the best move first, since this will produce
//	a huge reduction in tree size.

//	Best moves are typically winning captures and moves that have been shown
//	to be good here in the past (PV move and hash table move).

static void PrepareSort(CON *pcon, const STE *pste, int depth, PHASHPawn phashp) {
    // bitmap of squares attacked by the opponent pawns
//    bitboard notSafe = phashp->side[pste->side ^1 ].pawnatk;

    foreachmove(pste, pcm) {
        pcm->cmk = 0;
        if(Capt(pcm->m)) {
            pcm->cmk = MVV_LVA[Pc(pcm->m)][Capt(pcm->m)];
            if( pcm->cmk & cmkBADCAPT ) {
                // RxN, QxP
                int val = capture_value(pcon, pste, pcm );
                if( val >= 0 )
                    pcm->cmk = cmkGOODCAPT | (pcm->cmk & ~cmkMASK);
                val = val;
            }
        }
/*        else if( moveisPawnMove(pcm) && moveto7thRank(pcm)) {
            // only checking the dest rank because we know it's a pawn
//            pcm->cmk = (cmkKILLER | cmkGOODCAPT) + 10000;
            pcm->cmk = cmkQUEEN;
        } */
        else if( moveisPromotion(pcm) ) {
            if( Prom(pcm->m) == PROM_Q )
                pcm->cmk = cmkGOODCAPT | cmkQUEEN;
            else
                pcm->cmk = cmkQUEEN | (100 - Prom(pcm->m));
        } else {
            unsigned int history_score = 0;

#ifdef MOVE_HISTORY_SORT
            history_score = History.sort_key( pcm );
#else
            +++
            pcm->cmk = cmkNONE + 1000;
            if( AnyColor(Pc(pcm->m)) == AnyColor(B_CASTLE) )
                pcm->cmk = cmkQUEEN;    // will not be reduced

            {
//                pcm->cmk += (dRank - Rank(From(pcm->m))) - (dRank - Rank(To(pcm->m)));
                int d = Rank(To(pcm->m)) - (Rank(From(pcm->m))>>1);
                if( pste->side == B )
                    d = 7-d;
                if( d > 0 && AnyColor(Pc(pcm->m)) == AnyColor(KNIGHT) )
                    d += 10;
                pcm->cmk += d;
            }
#endif
#ifdef MOVE_SAFETY_SORT
            {
                history_score++;
                if(!moveisPawnMove(pcm)) {
                    bitboard from = IdxToU64(From(pcm->m));
                    // leaving a square attacked by a pawn
                    if( notSafe & from )
                        history_score += 0x100000;
                    bitboard to = IdxToU64(To(pcm->m));
                    // moving to a square attacked by a pawn
                    if( notSafe & to )
                        history_score = 0;
                }
            }
#endif
            pcm->cmk = cmkNONE + history_score;

#ifdef MOVE_PST_SORT
        if( history_score == 0 ) {
            // set cmk to delta positional value
            int pc = Pc(pcm->m);
            int neutralPc = pc & (~1);
            int co = pc & 1;
            // neutral color square for positional values
            int nSqr = remapSq[co][To(pcm->m)];
            // basic positional value
            pcm->cmk += staticPosVal[neutralPc >> 1][nSqr];
            nSqr = remapSq[co][From(pcm->m)];
            // basic positional value
            pcm->cmk -= staticPosVal[neutralPc >> 1][nSqr];
        }
#endif
        }

    }
}

static void PrepareSortQS(PCON pcon, const STE *pste, int depth) {
    foreachmove(pste, pcm) {
        U32 m = pcm->m;
        pcm->cmk = 0;
        if( moveisPromotion(pcm) ) {
            if( Prom(pcm->m) == PROM_Q )
                pcm->cmk = cmkGOODCAPT | cmkQUEEN;
            else
                pcm->cmk = cmkQUEEN | (100 - Prom(pcm->m));
        } else if(Capt(m)) {
            pcm->cmk = MVV_LVA[Pc(m)][Capt(m)];
            if( pcm->cmk & cmkBADCAPT ) {
                // RxN, QxP
                int val = capture_value(pcon, pste, pcm );
                if( val >= 0 )
                    pcm->cmk = cmkGOODCAPT | (pcm->cmk & ~cmkMASK);
                val = val;
            }
/*        } else if( moveisPawnMove(pcm) && moveto7thRank(pcm)) {
            if( see(pcon, pste, From(pcm->m), To(pcm->m)) >= 0 )
                pcm->cmk = cmkEQUCAPT | 0;*/
        } else {
            // check evasion, check moves
            unsigned int history_score = 0;
#ifdef MOVE_HISTORY_SORT
            history_score = History.sort_key( pcm );
#endif
            pcm->cmk = cmkNONE + history_score;
//            if( pcm->m == 0 )
//                pcm->cmk = cmkHASH;
        }
    }
}
/** 
    get the next move sorted by score
*/
static INLINE void SortNext(const STE *pste, PCM pcm) {
	PCM	pcmBest = pcm;

	for (PCM pcmI = pcm + 1; pcmI < (pste + 1)->mlist_start ; pcmI++)
		if (pcmI->cmk > pcmBest->cmk)
			pcmBest = pcmI;
	CM cm = *pcm;
	*pcm = *pcmBest;
	*pcmBest = cm;
}

static INLINE void copyPV(PSTE pste, const CM &cm) {
    int how_many = (pste + 1)->ccmPv;
    pste->argcmPv[0] = cm;
    pste->ccmPv = how_many + 1;
    // the loop can copy one more
    for(int i = 0; i < how_many; i += 2) {
        pste->argcmPv[i+1] = (pste+1)->argcmPv[i];
        pste->argcmPv[i+2] = (pste+1)->argcmPv[i+1];
    }
}
static INLINE void copyPVQS(PSTE pste, const CM &cm) {
    int how_many = (pste + 1)->ccmPv;
    pste->argcmPv[0].m  = cm.m;
    pste->argcmPv[0].cmk  = cmkQS;
    pste->ccmPv = how_many + 1;
    // the loop can copy one more
    for(int i = 0; i < how_many; i += 2) {
        pste->argcmPv[i+1] = (pste+1)->argcmPv[i];
        pste->argcmPv[i+2] = (pste+1)->argcmPv[i+1];
    }
}

//  -------------------------------------------------------------------------

//	This does a null-move search to see if the side not to move has a threat.
//	If they can't drive the score above alpha, even if they get to move twice
//	in a row, their positions probably sucks, and we'll prune this variation.
//	It's not called in endgames, because it has problems detecting zugzwang.

//  We don't need to do a null move if we know allready that there is threats around
//  In endgames any pawn move is a potential threat (score will vary a lot)

static INLINE int FThreat(PCON pcon, PSTE pste, int Alpha, int Beta, const nodeType nt)
{
	bNullMove(pcon, pste);
    // Reduced-depth search
    // R = 3 if (rem_ply > 8) || (rem_ply > 6 (and pieces per side >= 3))
    int R = 3; // set to 3 since we do a verification search

    (pste + 1)->plyRem = pste->plyRem - 1 - R;

    int val;
    if( (pste+1)->plyRem < 0 ) {
        // set plyrem to look for check on next move
        (pste+1)->plyRem = -1;
//        (pste+1)->plyRem = 0;
        (pste+1)->ttmove = false;   // TODO rename that
        val = -SearchQ(pcon, pste + 1, - Beta, - Beta + 1, true);
    } else {
        val = -Search(pcon, pste + 1, - Beta, - Beta + 1, nt);
    }
    bUndoNullMove(pcon, pste);
    return val;
}


//  -------------------------------------------------------------------------

static int INLINE capture_value(PCON pcon, const STE *pste, const CM *pcm) {
    if( moveisPromotion( pcm ) )
        return valKNIGHT - valPAWN;
    int pc = Pc(pcm->m);
//    if( (pc|1) == (B_EP|1) )
//        return 0;
    int took = Capt(pcm->m);
    if( exchangePieceValue[ took ] >= exchangePieceValue[ pc ] )
        return exchangePieceValue[ took ] - exchangePieceValue[ pc ];

    // compute see value...
    int val = see(pcon, pste, From(pcm->m), To(pcm->m));
    return val;
}

#define NCMove(from, to, pc) ((from) | ((to) << 6) | ((pc) << 12))
#define CMove(from, to, type, capt) ((from) | ((to) << 6) | ((type) << 12) | ((capt) << 16))

//	Quiescent search.  This lets captures play out at the end of the search,
//	so I don't end up aborting a line at a stupid place.

//#define QS_MARGIN   75
//#define QS_MARGIN   55
//#define QS_MARGIN   95
#define QS_MARGIN   150
//#define QS_MARGIN   (150+75)
//#define QS_MARGIN   (150+250)

static int SearchQpEG(PCON pcon, PSTE pste, int Alpha, int Beta);

static int SearchQ(PCON pcon, PSTE pste, int Alpha, int Beta, bool do_check)
{
//	pcon->ss.nodes++;
    stats.QS_count ++;
    if (++pcon->ss.nodes >= pcon->ss.nodesNext) {
        pcon->ss.nodesNext = pcon->ss.nodes + 10000;
        VPrCallback(pcon);
        VCheckTime(pcon);
        if ((pcon->fAbort) || (pcon->fTimeout))
            return 0;
        if( pcon->smode == smodeTHINK || pcon->smode == smodeANALYZE )
            VPrSendUciCurrentLine(pcon, pste);
	}

	CM cmBest(0,0);
	pste->ccmPv = 0;
    pste->lastMove = 0;
	bool fFound = false;
    bool isPV = Beta != (Alpha+1);
    int iaminCheck = pste->checkf;
    int  f_max   = valMAX;
    int sp = valMIN;
    CM * p_moves;
    int depth = int(pste - pcon->argste) + 1;
    (pste+1)->lastMove = 0;
    if (FRepSet(pcon, pste, pste->hashkPc)) {
        stats.repCount++;
		return 0;
	}

#ifdef DISTANCE_PRUNING
#if 1
    // this is the lowest score I can get from here
    int i_am_mate_score = mateValue(depth + 1);
    if( i_am_mate_score > Alpha ) {
        Alpha = i_am_mate_score;
        // Beta is mate in n, best here is mate in n+m, so return
        if(i_am_mate_score >= Beta)
            return Beta; //i_am_mate_score;
//            return i_am_mate_score;
    }
#endif
#endif
    int OldAlpha = Alpha;
	//
	//	See if static eval will cause a cutoff or raise alpha.
	//
    // this will be the return value if king evasion does no capture
	int posVal = ValEval(pcon, pste, Alpha, Beta);
    // score is valMIN when in check or else it won't play moves that lead to loss of material when nearly mated
    int bestVal = valMIN;

    if( depth >= (csteMAX-2) )
        return posVal;

    // generate captures, if we are in check then generate all moves (evade)
	if (iaminCheck) {
		bGenEvasions(pcon, pste);
        //pste->plyRem++;
        //do_check = do_check || QS_CHECK_LIMIT;
	}
	else {
//        if( pste->valPcUs + pste->valPcThem == 0 ) {
//            return SearchQpEG(pcon, pste, Alpha, Beta);
//        }

#ifdef PRUNE_QS
        f_max = posVal + QS_MARGIN;
#endif
        bestVal = posVal;
        if (bestVal >= Beta)
		    return bestVal;
        if (bestVal > Alpha)
            Alpha = bestVal;
        else {
            //bestVal = Alpha;
        }
		p_moves = bGenCaptures(pcon, pste, pste->mlist_start);
        if( pste->plyRem >= -1 || (pste->valPcUs + pste->valPcThem == 0) )
		    p_moves = bGenPasserPush(pcon, pste, p_moves);
	}

//    if( depth > pcon->ss.plyMaxDepth )
//        pcon->ss.plyMaxDepth = depth;

    PrepareSortQS( pcon, pste, depth );
    //
	//	Iterate through capturing moves, to see if I can improve upon alpha
	//	(which may be the static eval).
	//

    foreachmove(pste, pcm) {

        SortNext(pste, pcm);
        fFound = true;
        bMove(pcon, pste, pcm->m);

#ifdef PRUNE_QS
        if( !iaminCheck
            // end game values
            && ((pste+1)->valPcUs >= valROOK)
            && ((pste+1)->valPcThem >= valROOK)
//            && !moveisPromotion(pcm)
            && !(moveisPawnMove(pcm) && moveto7thRank(pcm))
//            && !moveisPawnMove(pcm)
            ) {
                if( (!QS_CAPT_LIMIT) && moveisBadCapture(pcm) ) {
    		        bUndoMove(pcon, pste, pcm->m);
                    continue;
                }
                if( pste[1].checkf == 0 || (!do_check) ) {
                    int approx = f_max + MaterialGain(pcon, pcm->m);
                    if( moveisPromotion(pcm) ) {
                        if( Prom(pcm->m) == PROM_Q )
                            approx += valQUEEN - valPAWN;
                        else
                            approx += valKNIGHT - valPAWN;
                    }
                    if ( (!isPV) && approx <= Alpha ) {
                        stats.FP_QS++;
    		            bUndoMove(pcon, pste, pcm->m);
                        if( approx > bestVal ) {
                            cmBest.m = 0;
	                        pste->ccmPv = 0;
                            bestVal = approx;
                        }
                        continue;
                    }
                }
        }
#endif

		(pste + 1)->plyRem = pste->plyRem - 1;
		int val = -SearchQ(pcon, pste + 1, -Beta, -Alpha, QS_CHECK_LIMIT);
		bUndoMove(pcon, pste, pcm->m);

        if (val >= Beta) {
            bestVal = val;
            cmBest = *pcm;
            copyPVQS(pste, *pcm );
            goto out;
        }
        if (val > bestVal) {
			bestVal = val;
            cmBest = *pcm;
            copyPVQS(pste, *pcm );
            if (val > Alpha)
			    Alpha = val;
        }
	}
    sp = bestVal;   // stand pat score
    if( !iaminCheck && cmBest.m != 0 ) {
        sp = valMIN;
    }
#if 1
    if( !iaminCheck && do_check //&& (bestVal < (valKNIGHT))
        && (pste->valPcUs >= valROOK)) {
        // gen checks
        {
            p_moves = bGenPseudoCheck(pcon, pste, pste->mlist_start);
            PrepareSortQS( pcon, pste, depth );

            foreachmove(pste, pcm) {

                SortNext(pste, pcm);
                fFound = true;
                bMove(pcon, pste, pcm->m);
                // we only want moves that do check
                if( (pste[1].checkf == 0) ) {
		            bUndoMove(pcon, pste, pcm->m);
                    continue;
                }
                // don't do check if it looses material
                if( (!QS_CHECK_LIMIT) && (see(pcon, pste+1, To(pcm->m)) < 0)) {
                    bUndoMove(pcon, pste, pcm->m);
                    continue;
                }

		        (pste + 1)->plyRem = pste->plyRem - 1;
		        int val = -SearchQ(pcon, pste + 1, -Beta, -Alpha, QS_CHECK_LIMIT);
		        bUndoMove(pcon, pste, pcm->m);
                if (val >= Beta) {
                    bestVal = val;
                    cmBest = *pcm;
                    copyPVQS(pste, *pcm );
			        goto out;
                }
                if (val > bestVal) {
			        bestVal = val;
                    cmBest = *pcm;
                    copyPVQS(pste, *pcm );
                    if (val > Alpha)
			            Alpha = val;
                }
	        }
        }

//        if(!fFound)
//            return 0;
    }
#endif

#ifdef QS_VERIFY_EXTENSION
//#define QS_SP_MARGIN    200
//#define QS_SP_MARGIN    (10+4*5)
//#define QS_SP_MARGIN    25
#define QS_SP_MARGIN    (5+4*5) // can not be lower than stm bonus

    if( !iaminCheck && (pste->plyRem >= -1 || (pste->valPcUs + pste->valPcThem == 0)) && (sp + 25 > bestVal)
//    if( !iaminCheck && (pste->plyRem >= -5) && (sp + 25 > bestVal)
//    if( !iaminCheck && (pste->plyRem >= -2) && (sp + 25 > bestVal)
        && (bestVal > OldAlpha)
//        && !(pste-1)->fNull   // no recursion
//       && !(pste-2)->fNull     // one recursion allowed
        && (depth <= 4 || !(pste-4)->fNull)     // three recursions allowed
        ) {
        stats.qs_sp++;
        // threat check
	    bNullMove(pcon, pste);
        (pste + 1)->plyRem = pste->plyRem - 1;
        int val = -SearchQ(pcon, pste + 1, - Beta + QS_SP_MARGIN, - Alpha + QS_SP_MARGIN, true);
        bUndoNullMove(pcon, pste);
        if( val + QS_SP_MARGIN <= Alpha ) {
            bool oldStore = pcon->hashStore;
            pcon->hashStore = false;    // needed
            pste->plyRem = 0;
//            pste->plyRem = 1;
            pste->fNull = true; // set recursion flag
            pste->reduced = false;
            // extend one ply, note that we are in the PV
            bestVal = SearchPV(pcon, pste, OldAlpha, Beta);
//            bestVal = SearchPV(pcon, pste, val + QS_SP_MARGIN, Beta);
            pcon->hashStore = oldStore;
            pste->fNull = false;
            goto out;
        }
    }
#endif

	if (!fFound) {
        pste->lastMove = 0;
        // We have no move !!
		if (iaminCheck) {
            // Checkmate
			bestVal = mateValue(depth);
            return bestVal;
		}
	}
out:
    Assert(ValIsOk(bestVal));
    pste->lastMove = cmBest.m;
	return bestVal;
}
#if 0
static int SearchQpEG(PCON pcon, PSTE pste, int Alpha, int Beta)
{
//	pcon->ss.nodes++;
    stats.QS_count ++;
    if (++pcon->ss.nodes >= pcon->ss.nodesNext) {
        pcon->ss.nodesNext = pcon->ss.nodes + 10000;
        VPrCallback(pcon);
        VCheckTime(pcon);
        if ((pcon->fAbort) || (pcon->fTimeout))
            return 0;
        if( pcon->smode == smodeTHINK || pcon->smode == smodeANALYZE )
            VPrSendUciCurrentLine(pcon, pste);
	}

	CM cmBest(0,0);
	pste->ccmPv = 0;
    pste->lastMove = 0;
	bool fFound = false;
    bool isPV = Beta != (Alpha+1);
    int iaminCheck = pste->checkf;
    CM * p_moves;
    int depth = int(pste - pcon->argste) + 1;
    (pste+1)->lastMove = 0;
    if (FRepSet(pcon, pste, pste->hashkPc)) {
        stats.repCount++;
		return 0;
	}

    int OldAlpha = Alpha;
	//
	//	See if static eval will cause a cutoff or raise alpha.
	//
    // this will be the return value if king evasion does no capture
	int posVal = ValEval(pcon, pste, Alpha, Beta);
    // score is valMIN when in check or else it won't play moves that lead to loss of material when nearly mated
    int bestVal = valMIN;

    if( depth >= (csteMAX-2) )
        return posVal;

    // generate captures, if we are in check then generate all moves (evade)
	if (iaminCheck) {
		bGenEvasions(pcon, pste);
        //pste->plyRem++;
        //do_check = do_check || QS_CHECK_LIMIT;
        if( legalMoveCount(pste) == 0 )
            return mateValue(depth);
	}
	else {
//        bestVal = posVal;
        // no stand pat
//        if (bestVal >= Beta)
//		    return bestVal;
        if (bestVal > Alpha)
            Alpha = bestVal;

		p_moves = bGenCaptures(pcon, pste, pste->mlist_start);
//        p_moves = bGenPasserPush(pcon, pste, p_moves);
        p_moves = bGenPawnPush(pcon, pste, p_moves);
        Assert( p_moves == (pste+1)->mlist_start );
        *p_moves++ = CM(0,0); // add null move
        (pste+1)->mlist_start++;
	}

    PrepareSortQS( pcon, pste, depth );
    //
	//	Iterate through capturing moves, to see if I can improve upon alpha
	//	(which may be the static eval).
	//

    foreachmove(pste, pcm) {

        SortNext(pste, pcm);
        fFound = true;
        if( pcm->m == 0 ) {
            CM *listSave = (pste+1)->mlist_start;
            bNullMove(pcon, pste);
            (pste+1)->mlist_start = listSave;   // HACK
        } else
            bMove(pcon, pste, pcm->m);

		(pste + 1)->plyRem = pste->plyRem - 1;
        int val;
        if( pste->fNull && (pste+1)->fNull ) {
            val = posVal;  // two pass in a row
        } else {
            if((pste+1)->valPcUs > 0 || (pste+1)->valPcThem > 0)
		        val = -SearchQ(pcon, pste + 1, -Beta, -Alpha, false);
            else
		        val = -SearchQpEG(pcon, pste + 1, -Beta, -Alpha);
        }
        if( pcm->m == 0 )
            bUndoNullMove(pcon, pste);
        else
		    bUndoMove(pcon, pste, pcm->m);

/*        if (val >= Beta) {
            bestVal = val;
            cmBest = *pcm;
            copyPVQS(pste, *pcm );
            goto out;
        }*/
        if (val > bestVal) {
            cmBest = CM(0,0);
			bestVal = val;
            if( pcm->m ) {
                cmBest = *pcm;
                copyPVQS(pste, *pcm );
            }
            if (val > Alpha)
			    Alpha = val;
        }
	}

out:
    Assert(ValIsOk(bestVal));
    pste->lastMove = cmBest.m;
	return bestVal;
}
#endif
//	Full-width search.  This is the heart of the "chess playing" part of this
//	program.  It performs a recursive alpha-beta search.

#define LOWERBOUND  OldAlpha


static INLINE bool isNodeUseless(CON *pcon, STE *pste, U32 hashMoveNodeCount, PCM pcm) {
//    if( pste->plyRem <= 2)
        if( moveisGood(pcm) )
            return false;
//    if( moveisBadCapture(pcm) )
//        return true;
    if( moveisCapture(pcm) )
        return false;
/*    if( (pste->threatMove != U32(-1)) && (To(pste->threatMove) == From(pcm->m)) ) {
        return false;
    }
*/
    if( (pste-1)->threatMove == pcm->m )
        return false;

    return true;

}
#if 0
static void CountStuff(CON *pcon, STE *pste, nodeType nt) {

    if( nt == nodeCUT )
        return;
    if( pste->plyRem <= 1 )
        return;

    return;
    foreachmove(pste,pcm) {
        // sort quiescent moves and 'bad' captures
        U32 mask = pcm->cmk & cmkMASK;
        if( mask <= cmkGOODCAPT ) {

	        bMove(pcon, pste, pcm->m);
            PHASH new_hash_entry = ProbeHashValue(pste+1);
            if( mask == cmkKILLER || mask == cmkQUEEN )
                mask = cmkNONE;
            if( new_hash_entry ) {
                U32 effort = new_hash_entry->count;
                if( effort >= cmkBADCAPT )
                    effort = cmkBADCAPT - 1;
//                    if( - new_hash_entry->val < -valMATEscore )
//                        effort = 0; // look that last

/*                    if( nt == nodeCUT ) {
                    effort = cmkBADCAPT - effort - 1;
                }*/
                pcm->cmk = mask + effort;
            }
            bUndoMove(pcon, pste, pcm->m);
        }
    }
}
#endif

static INLINE void extension_check(const CON *pcon, PSTE pste, const CM *pcm, int &extension, int a, int depth, bool isPV) {

#ifdef EXTEND_CHECK
    // extend everywhere in the tree
    {
        // stop extending if I lead the score
#ifdef LESS_CHECK
//    if( (a < valKNIGHT) || isFrontier(pste)) 
//    if( (a < valKNIGHT) && ( true || (depth) >= (pcon->ss.plyDepth / 4) ) 
//        || (pcon->ss.plyDepth <= 5) ) 
//    if( (a < valROOK) || isFrontier(pste) )
//    if( (a < valKNIGHT) /*|| (pcon->ss.plyDepth <= 5)*/ ) 
/*        if( isPV || (!moveisBadCapture(pcm) 
                            && (moveisCapture(pcm) || moveisPromotion(pcm) || see(pcon, pste, To(pcm->m)) >= 0))
            )*/
#endif
        {
            extension = 1;
            stats.c_extcheck++;
        }
    }
#else
    // only extend on check at the frontier
    if( isFrontier(pste) ) {
        // stop extending if I lead the score
        if( a < valKNIGHT ) {
            extension = 1;
            stats.c_extcheck++;
        }
    }
#endif
}

static INLINE void extension_pawn(const CON *pcon, PSTE pste, const HASHPawn *phashp, const CM *pcm, int &extension, bool &can_reduce, int a, bool isPV) {
    if( moveisPawnMove(pcm) ) {
        // Pawn push to 7th rank
        if( moveto7thRank(pcm) ) {
//            pste->danger = true;
            can_reduce = false;
            if( isPV )
                extension = 1;
        // no reduction of pawn moves in end game
        } else if (((pste->valPcUs + pste->valPcThem) <= 1*valQUEEN)) {
//            pste->danger = true;
            can_reduce = false;
        } else {
            // don't reduce push of passed pawns
            int from = From(pcm->m);
            bitboard from_64 = IdxToU64(from);
            if( from_64 & phashp->side[pste->side].passed )
                can_reduce = false;
        }
    } else if( moveisPromotion(pcm) )
        can_reduce = false;
}

static INLINE void extension_castle(const CON *pcon, PSTE pste, const CM *pcm, int &extension, bool &can_reduce, int a, bool isPV, bitboard kingRing) {
#if 0
    if( isPV )
//        if( !moveisBadCapture(pcm) && (kingRing & IdxToU64( To(pcm->m) )) ) {
        if( !moveisBadCapture(pcm) && ((Capt(pcm->m)|1) == (PAWN|1)) && (kingRing & IdxToU64( To(pcm->m) )) ) {
//        if( (Capt(pcm->m)|1) == (PAWN|1) && (kingRing & IdxToU64( To(pcm->m) )) ) {
    //        pste->danger = true;
            can_reduce = false;
            extension = 1;
        }
#else
    #ifndef MINIMIZE
        if( (kingRing & IdxToU64( To(pcm->m) )) ) {
            pste->danger = true;
            can_reduce = false;
        }
    #endif
#endif
}

static INLINE void before_search(const CON *pcon, PSTE pste, bool isPV, bitboard &kingRing, bool & forced, int & preExtend, int extCount, int depth) {

    U8 otherside = FlipColor(pste->side);
    kingRing = king_attacks[pcon->king[otherside]];
    if( otherside == B )
        kingRing |= soutOne(kingRing);
    else
        kingRing |= nortOne(kingRing);
//    if( -5*(extCount-1)/2 >= pcon->ss.plyDepth ) {
//        forced = true;
//    }
#ifndef MINIMIZE
    if( !pste->checkf && pste->evaluated && !pste->evalislazy && (pste-1)->evaluated && !(pste-1)->evalislazy
        && (pste->plyRem >= 1)
        && (pste)->prevCaptValue == 0) {
            if( abs(pste->val +(pste-1)->val) >= 50 ) {
                forced = true;
            }
        }
#endif
#if 1
    Killers[depth+2+2].raz();
    MateKillers[depth+2+2].raz();
//    Killers[depth+2].raz();
//    MateKillers[depth+2].raz();
#endif

}

static INLINE bool noThreat(const CON *pcon, PSTE pste, const CM *pcm, bitboard pawnAttack) {
    int from = From(pcm->m);
    if( From(pcm->m) == To(pste->threatMove) )
        return false;
    if( IdxToU64(from) & pawnAttack )
        return false;
    return true;
}

#ifdef USE_EFFORT
static INLINE bool checkEffort(PCON pcon, PSTE pste, bool & can_reduce, int & extension, bool isPV) {
    if( can_reduce && (isPV || pste->plyRem >= 2) ) {
        PHASH hash_entry = ProbeHashValue(pste+1);
        if( hash_entry ) {
            if( hash_entry->getForced() ) {
                can_reduce = false;
                stats.c_fmext++;
            }
            if( isPV && hash_entry->getExt() 
                /*&& pste->plyRem <= 2*/ ) {
                can_reduce = false;
//                extension = 1;
                return true;
            }
        }
    }
    return false;
}
#endif

//  hash count      nodecount       effect
//      1000        1500            setforced(true)
//      1000        1000            //
//      1000        500             setforced(false)

#ifdef USE_EFFORT
static U32 updateEffort(PCON pcon, PSTE pste, U64 thisMoveNodeCount) {
//    return 0;
#if 1
    {
        PHASH hash_entry = ProbeHashValue(pste+1);
        if( hash_entry ) {
            if( inSearch )
                thisMoveNodeCount /= 1;
            else {
/*                //if( hash_entry->count > 20 && hash_entry->count > 3.0f * thisMoveNodeCount )
                //    thisMoveNodeCount = thisMoveNodeCount;
                if( hash_entry->count > 20 && hash_entry->count * 1.0f < thisMoveNodeCount )
                    hash_entry->setForced( true );
                else if( hash_entry->count > 20 && hash_entry->count > 2.0f * thisMoveNodeCount )
                    hash_entry->setForced( false );*/
            }
            hash_entry->count += U32(thisMoveNodeCount);
            hash_entry->setForced( false );
            hash_entry->setExt( false );
            return hash_entry->count;
        } else
            return U32(thisMoveNodeCount);
    }
#endif
}

static void updateEffortAll(PCON pcon, PSTE pste, U32 *nodeList, U32 *moveList, int moveListCount, int limit) {
    if( inSearch || pste->checkf )
        return;     // needed because of null move

#if 1
    int last = limit;
    int total = 0;
    if( last >= int(moveListCount) )
        last = moveListCount-1;
    if( moveListCount < 4 )
        return;
    for(int ii = 0 ; ii < int(moveListCount) ; ii++)
        total += nodeList[ii];
    for(int n = 0; n <= last; n++) {
        int most = n;
        for (int i = n+1 ; i < int(moveListCount) ; i++)
            if( nodeList[i] > nodeList[most] )
	            most = i;
        my_swap(nodeList[most], nodeList[n], U32);
        my_swap(moveList[most], moveList[n], U32);
    }
    bool sf = true;
    if( last == 0 )
        sf = false;
//    if( nodeList[last/2] * 4 >= nodeList[0] )
    if( nodeList[last/2] * 3 >= nodeList[0] )
        sf = false;
//    if( (nodeList[0]*moveListCount/6.0f + nodeList[1]*moveListCount/12.0f + nodeList[2]*moveListCount/24.0f) < total / 2 )
//        return;
    int med = 1+(total - nodeList[0] - nodeList[1] - nodeList[2]) / (moveListCount-3);
/*    if( nodeList[0] * 2 > total )
        total = total;
    if( nodeList[0] * 3 > total )
        total = total;
    if( nodeList[0] * 4 > total )
        total = total;*/

    for(int n = 0; n <= last; n++) {
        if( nodeList[n] < U32(4*med) ) {
            break;
        }
        if( nodeList[n] * 8 < nodeList[0] && n > 0) {
            break;
        }
/*        if( nodeList[n] * 4 < nodeList[0] ) {
            sf = false;
            break;
        }*/
        bMove(pcon, pste, moveList[n]);
        PHASH hash_entry = ProbeHashValue(pste+1);
        if( hash_entry ) {
/*            if( (pste+1)->hashkPc == 0x000007aefbeb597a 
//moveList[n] == CMove(B8, G3, BISHOP|B, PAWN|W)
) {
                hash_entry = hash_entry;
                VDumpLine(pcon, pste, "Bxg3");
            }*/
            hash_entry->setForced( true );
            if( nodeList[0] * 4 > U32(total) )
                if( n == 0 || nodeList[n] * 4 > nodeList[0] )
                    hash_entry->setExt( true );
        }
        bUndoMove(pcon, pste, moveList[n]);
    }
#endif
}
#endif

static int Search(PCON pcon, PSTE pste, int Alpha, int Beta, nodeType nt)
{
	int	val;
    pste->ccmPv = 0;
    pste->reduced = false;
	CM cmBest(0,0);
    bool king_attak = false;
    int preExtend = 0;
    int bestVal = valMIN;
    int iaminCheck = pste->checkf;
    bool nm_threat = false;
    pste->ttmove = false;   // TODO rename that
    U32 moveList[256], moveListCount = 0;
#ifdef USE_EFFORT
    U32 nodeList[256];
    int moveScore[256];
#endif
    Assert(Beta == (Alpha+1));

    if (pste->plyRem < 0) {
        return SearchQ(pcon, pste,  Alpha, Beta, true);
    }

    int depth = int(pste - pcon->argste) + 1;
    if( depth >= (csteMAX-5) ) {
        return SearchQ(pcon, pste,  Alpha, Beta, true);
    }
    if( depth > pcon->ss.plyMaxDepth )
        pcon->ss.plyMaxDepth = depth;

    //	Increment global node counter, and figure out if it is time to call
    //	the interface callback, which is what allows for an interruptible
    //	search.  It's crucial that this be called now and then.
    //
    if (++pcon->ss.nodes >= pcon->ss.nodesNext) {
        //
        //	20000 nodes is maybe 1/10 to 1/30 of a second.
        //
        pcon->ss.nodesNext = pcon->ss.nodes + 10000;
        VPrCallback(pcon);
        VCheckTime(pcon);
        if ((pcon->fAbort) || (pcon->fTimeout))
            return 0;
        if( pcon->smode == smodeTHINK || pcon->smode == smodeANALYZE )
            VPrSendUciCurrentLine(pcon, pste);
	}
    //
    //	I can return draws and cut off if I'm somewhere other than the
    //	first ply of depth.
    //
    //	Return draw score if this is a repeated node.
    //
    if (depth > 2 && FRepSet(pcon, pste, pste->hashkPc)) {
        stats.repCount++;
        return 0;
    }

    // extCount is the net number of extension done (all extensions - all reductions done)
    int extCount = depth - pcon->ss.plyDepth + pste->plyRem; // extCount < 0 => reduced or wanted depth not reached yet
    int maxExt = MAXEXT(depth - extCount);

    //	Check hash table, in order to get "best" moves, and try to cut
    //	off.  Don't cut off within two plies of the root, otherwise the
    //	program might allow stupid rep draws.
    //
    PHASH hash_entry = 0;
    bool forced = false;
    U32 hashMoveNodeCount = 0;

#ifdef USE_HASH
    hash_entry = ProbeHashValue(pste);

    if( hash_entry && hash_entry->val >= valMATEscore && hash_entry->getPlyRem() < pste->plyRem
        && (hash_entry->getBound() == hashfEXACT || hash_entry->getBound() == hashfBETA)) {
            int v = hash_entry->val - (depth - 1);
            if( v >= Beta )
//            if( Beta < valMATEscore )
                return v;
    }
    if( hash_entry && (hash_entry->getPlyRem() >= pste->plyRem) && (depth > 2) ) {
        PHASH phash = hash_entry;
        int val = phash->val;
#if 1
        if( val >= valMATEscore ) {
            val -= depth - 1;
        } else if( val <= -valMATEscore ) {
            val += depth - 1;
        }
#endif
		switch(phash->getBound())
        {
			case hashfEXACT:
                stats.Hcutoff ++;
                return val;
                break;
			case hashfBETA:
                if( val >= Beta ) {
                    stats.Hcutoff ++;
					return val;
                }
				break;
			case hashfALPHA:
                if( val <= Alpha ) {
                    stats.Hcutoff ++;
					return val;
                }
				break;
            default:
                hash_entry = 0;
                //Assert(0);
                break;
        }
	}
#endif

//    if( hash_entry )
//        nm_threat = hash_entry->getThreat();

	//
	// Check alpha/beta window against a mate score
    // Thx for the idea
	//
    int OldAlpha = Alpha;
#ifdef DISTANCE_PRUNING
#if 1
    // this is the lowest score I can get from here
    int i_am_mate_score = mateValue(depth + 1);
    if( i_am_mate_score > Alpha ) {
        //Alpha = i_am_mate_score;
        // Beta is mate in n, best here is mate in n+m, so return
        if(i_am_mate_score >= Beta)
            return Beta; //i_am_mate_score;
//            return i_am_mate_score;
    }
#endif
#if 1
    // this is the best score we can get from here
    int i_will_mate_score =  -mateValue(depth);
    if( i_will_mate_score < Beta ) {
        //Beta = i_will_mate_score;
        // if I set + 0 then I don't see the final move (
            // I will not see something better than Alpha so just return
        // Alpha is mate in n, best here is mate in n+m, so return
        if( i_will_mate_score <= Alpha )
            return Alpha;
//            return i_will_mate_score;
    }
#endif
#endif


#ifdef EVAL_INTERIOR
    // gathering stats
    int nmPosVal = pste->val;
    if(!pste->evaluated )
	    nmPosVal = ValEval(pcon, pste, Alpha, Beta);
#else
    int nmPosVal = MaterialValue(pste) + valPAWN;
#endif


    //
    // try recognizers
    //
#ifdef USE_RECOGNIZER
    // the recognizers of bitbases where allready probed in the eval
    if( true
        && (pste->bound != fail_bound) && pste->evaluated) {
        recog_result ret = {recog_bound(pste->bound), pste->val, recog_allways_valid,
            WIN_CHANCE(100), WIN_CHANCE(100)  };
        if( ret.bound == exact_bound )
            return ret.score;
        if( ret.bound == lower_bound ) {
            if( ret.score >= Beta )
                return ret.score;
//            if( ret.score > Alpha )
//                Alpha = ret.score;
        }
        if( ret.bound == upper_bound ) {
            if( ret.score <= Alpha )
                return ret.score;
            if( ret.score < Beta )
                Beta = ret.score;
        }
    }
#endif

    PHASHPawn phashp;
    EvalPawns(pcon, pste, phashp);

    static int deltaRazor[] = {150,225,300};

    pste->threatMove = U32(-1);
    pste->threatTarget = 0;
#ifdef NULL_MOVE
	//
	//	Do null-move search, which is a reduced depth that tries to
	//	figure out if the opponent has no threats.
	//
    // Note :
    // - if Beta is a mate score then it is not probable that I can do better without doing anything
    // - only done if not a PV (what about branches that will become a pv later ?)
    // - nm_threat caching does not work for an unknown reason (more nodes sometimes)
    // - material constraint reduced, I hope this works well with the verification search
    // - reductions are now allowed after an extension (so no test on extCount)
    // - if the stand pat score is < Beta then doing nothing will not raise it to Beta or more
    if (
        (!iaminCheck)
        && (pste->valPcUs > 0)
        && (nmPosVal > Beta)
//        && (Beta < valMATEscore)    // redondant with above test
        && (pste->plyRem >= 1)      // need more tests

#ifdef TWO_NULL
        && (!( pste->fNull && (pste-1)->fNull) )
#else
        && (!pste->fNull)
#endif
        ) {
        {
            stats.NM1++;
            // don't forget to eval when the call to eval will be after bMove()
            bool ir = inSearch;
            inSearch = true;
            int val = FThreat(pcon, pste, Alpha, Beta, nt);
            inSearch = ir;
            Assert(ValIsOk(val));
            if( val >= Beta ) {
                // why not store result in hash ?
                // TODO check this: this does not work well on some position (more nodes)
                stats.NM2++;
                // verification search (for end game mostly)
                // still need to do NM at lower depth (higher remaining depth)
                const int Rv = 1 + 4;
                if(true && pste->plyRem >= Rv) {
                    stats.NM3++;
                    pste->fNull = true; // set or else we will loop :)
                    pste->plyRem -= Rv;
                    val = Search(pcon, pste, Alpha, Beta, nodeCUT /*nt*/);
                    pste->plyRem += Rv;
                    //if( val > nmPosVal + 40)
                    //    val = Alpha;
                    pste->fNull = false;
                } else if(false && pste->plyRem >= 3) {
                    stats.NM3++;
                    pste->fNull = true; // set or else we will loop :)
                    pste->plyRem -= 3;
                    val = Search(pcon, pste, Alpha, Beta, nodeCUT /*nt*/);
                    pste->plyRem += 3;
                    pste->fNull = false;
                } else if(false && pste->plyRem >= 1) {
                    stats.NM3++;
                    pste->fNull = true; // set or else we will loop :)
                    pste->plyRem -= 1;
                    val = Search(pcon, pste, Alpha, Beta, nodeCUT /*nt*/);
                    pste->plyRem += 1;
                    pste->fNull = false;
                }
            }
            if( val >= Beta ) {
/*                if(pcon->ss.plyDepth <= 5 ) {
                    char tmp[200];
                    sprintf(tmp," NM val=%d [%d,%d]", val, Alpha, Beta);
                    VDumpLine(pcon, pste, tmp);
                }*/
#ifdef HASH_NM
                if ((pcon->fAbort) || (pcon->fTimeout)) {
                    return val;
                }
                // store because we don't want to do this same search *again*
                RecordHashValue(pcon, pste, cmBest, val/*Beta*/, Alpha, Beta, depth, false);
#endif
                pste->lastMove = 0;
                return val;
            }
//            pste->threatMove = (pste+1)->argcmPv[0].m;
            pste->threatMove = (pste+1)->lastMove;
            if(pste->threatMove)
                pste->threatTarget = pcon->pos[ To(pste->threatMove) ];
#ifdef BM_EXTENSION
            if( true && (pste->plyRem <= BM_DEPTH) && pste->threatMove 
                && (val + /*valPAWN +*/ 50 < Alpha 
//                && Alpha > -valKNIGHT
                )
                && (((pste->threatTarget == (pste-2)->threatTarget))
                    || To(pste->threatMove) == To((pste-2)->threatMove))
/*                && !( (To((pste-2)->lastMove) == To(pste->threatMove))
                    || (To((pste-1)->lastMove) == From(pste->threatMove)))*/
                 ){
//                    preExtend = 1;
                    king_attak = true;
                    stats.c_bmext++;
/*                    pste->lastMove = pste->threatMove; // for debug only
                    char tmp[256];
                    sprintf(tmp, "same threat...  rem=%d targ=%d %x %x %d [%d]", pste->plyRem, pste->threatTarget, To(pste->threatMove), To((pste-2)->threatMove), val, Alpha);
                    VDumpLine(pcon, pste, tmp);
                    if( (pste-1)->reduced )
                        printf(" and father is reduced\n");
                    if( (pste-2)->reduced )
                        printf(" and father 2 is reduced\n");
                    if( (pste-3)->reduced )
                        printf(" and father 3 is reduced\n");*/
                    if( (pste-1)->reduced )
                        return valMIN;
            } else
#endif
            if( false ) {
            }
            else if( true && (val <= Alpha) && pste->threatMove
                //&& (pste->plyRem <= 3 || (val + 50 < Alpha))
                && (pste->plyRem <= 4)
                // there will be a re-search
                && (pste-1)->reduced ) {
                //char tmp[256];
                pste->lastMove = pste->threatMove; // for debug only
                if(To((pste-1)->lastMove) == From(pste->threatMove)) {
                    // sequence is : P1(x->t) null P1(t->y) common
//                    sprintf(tmp, "from(thr) == to(m-1) : targ=%d from=%x to=%x %d [%d]", pste->threatTarget, From(pste->threatMove), To((pste-1)->lastMove), val, Alpha);
//                    VDumpLine(pcon, pste, tmp);
                    return valMIN;
                }
                if(From((pste-1)->lastMove) == To(pste->threatMove)) {
                    // sequence is : P1(t->x) null P2(y->t) uncommon
//                    sprintf(tmp, "To(thr) == From(m-1) : targ=%d to=%x to=%x %d [%d]", pste->threatTarget, To(pste->threatMove), From((pste-1)->lastMove), val, Alpha);
//                    VDumpLine(pcon, pste, tmp);
                    return valMIN;
                }
                if(pste->threatTarget && (pste->threatTarget == (pste-2)->threatTarget)) {
                    // same threat target : not uncommon
//                    sprintf(tmp, "same target : targ=%d to=%x %d [%d]", pste->threatTarget, To(pste->threatMove), val, Alpha);
//                    VDumpLine(pcon, pste, tmp);
                    return valMIN;
                }
/*                if( !( moveisVeryGoodCapture(&(pste+1)->argcmPv[0])
                    || (To((pste-2)->lastMove) == To(pste->threatMove))
                    || (To((pste-1)->lastMove) == From(pste->threatMove))
                    ) ) {
                    return valMIN;
                }*/
                pste->lastMove = 0;
            }
            else if( false && (val <= Alpha)
                && (pste->plyRem <= 5 )
                // there will be a re-search
                && (pste-1)->reduced
                && !( moveisVeryGoodCapture(&(pste+1)->argcmPv[0]))
                && ( (To((pste-2)->lastMove) == To(pste->threatMove))
                    || (To((pste-1)->lastMove) == From(pste->threatMove))
                    ) ) {
                    return valMIN;
            }
            else if( true && val <= -valMATEscore 
                && ( pste->plyRem <= 2 ) ) {
                // extend one ply if there is a mate threat
                // ok storing threat in hash does not work for some reason...
                // only setting king_attack so that we don't reduce the search
                // if we extend then we can search a lot deeper for no reason when
                // there is a mate but winning side does not play the winnin move
                if( (pste-1)->reduced )
                    return valMIN;
                //king_attak = true;
                // threat flag should be stored in hash too for next search
            }
        }
	}
#endif


#ifdef USE_HASH
#ifdef USE_IID
    if( (hash_entry == NULL || (hash_entry->getMove() == 0 && (hash_entry->getPlyRem() < pste->plyRem-2) )) 
        && (pste->plyRem >= 4)
        && (nmPosVal + 100 >= Beta)
        ) {
        int reduc = 2;
        pste->plyRem -= reduc;
        stats.c_iid++;
//        int iidval = Search(pcon, pste, Alpha, Beta, nt);
        bool ir = inSearch;
        inSearch = true;
        int iidval = Search(pcon, pste, Alpha-100, Beta-100, nt);
        if( iidval >= Beta-100 )
            iidval = Search(pcon, pste, Alpha, Beta, nt);
        pste->plyRem += reduc;
        hash_entry = ProbeHashValue(pste);
        inSearch = ir;
        pste->lastMove = 0;
    }
#endif
#endif
    if( hash_entry ) {
        hashMoveNodeCount = hash_entry->count;
        //forced = hash_entry->getForced();
        if( forced )
            stats.c_fmext++;
//        if( pste->plyRem < (pste-1)->plyRem && hash_entry->getExt() )
//            preExtend = 1;
//        if( pste->plyRem < (pste-1)->plyRem && hash_entry->getExt() )
//            pste->plyRem++;
//        if( /*pste->plyRem < (pste-1)->plyRem && */hash_entry->getExt() )
//            return SearchPV(pcon, pste, Alpha, Beta);
    }

    //	Generate moves.
    //
    bGenMoves(pcon, pste);
    //
    //	I know how many reversible moves have been made up until now.
    //	I just tried to make another move.  If I am here, I didn't
    //	leave myself in check, so I succeeded.
    //
    //	If the number of reversible moves made before this is at
    //	least 50 for each side, this was a draw.
    //
    int lmc = legalMoveCount(pste);

    if ( (pste->plyFifty >= 50 * 2) && (lmc > 0) ) {
        pste->ccmPv = 0;
        return 0;
    }
    // single move extension
    if( (lmc == 1) && iaminCheck)
        preExtend = 1;

    bitboard kingRing = 0;
    before_search(pcon, pste, /*isPV*/ false, kingRing, forced, preExtend, extCount, depth);

    PrepareSort( pcon, pste, depth, phashp );

	//	Mark the hash moves so it sorts first.
	//
#ifdef USE_HASH
	if (hash_entry != NULL)
		SortHash(pste, hash_entry);
#endif
#ifdef USE_KILLERS
    SortKillers(pste, depth);
#endif

    int a = Alpha;
    int b = Beta;

    unsigned int prevCaptureSqr = pste->prevCaptSqr;

    bool fFound = false;
    int  c_move = 0;

    int  f_margin = valMAX;
    bool can_prune = false;

#define FUTILITY_MARGIN (100)

#ifdef FUTILITY_PRUNING
    if( !iaminCheck
//        && ((pste->valPcUs + pste->valPcThem) >= 2*valQUEEN)
//        && ((pste->valPcUs + pste->valPcThem) >= 1*valQUEEN)
//        && !phashp->pawnon6or7
//            && true )
//            && depth + 3 >= pcon->ss.plyDepth  )
//        && (pste->plyRem <= 4) )
        && (pste->plyRem <= 2) )
//        && (pste->plyRem <= 1) )
        can_prune = true;
    if( pste->plyRem == 0 )
        f_margin = FUTILITY_MARGIN;
    else if( pste->plyRem <= 2 )
        f_margin = 3*FUTILITY_MARGIN;
    else
        f_margin = 6*FUTILITY_MARGIN;
/*    f_margin = FUTILITY_MARGIN * (pste->plyRem + 1);
    f_margin = FUTILITY_MARGIN * (pste->plyRem + 1) - (FUTILITY_MARGIN*extCount)/2;
    if( f_margin < FUTILITY_MARGIN )
        f_margin = FUTILITY_MARGIN;
*/
#endif

    bool good_capt_done = false;
//    pste->ttmove = hash_entry && (hash_entry->getMove() != 0);
//    int altMoves = 0;

//    CountStuff(pcon, pste, nt);
    U32 cm1, cm2;
    CounterMoves.get( (pste-1)->lastMove, cm1, cm2);

    //
    //	Iterate through the move list, trying everything out.
    //
    foreachmove(pste,pcm) {
        //
        //	All moves are sorted on the cmk field
        //	cmk is set in PrepareSort() and in other sort functions

        SortNext(pste, pcm);

        int extension = preExtend;
        // Note : now I allow the reduction after the extension,
        // this is needed for the check extension
        bool can_reduce = (extension) > 0 ? false : true;
        can_reduce = can_reduce && !king_attak;
//        can_reduce = can_reduce && !forced;


        if( iaminCheck && !good_capt_done && moveisVeryGoodCapture(pcm) ) {
            // reduce when capturing in check
            if( !isFrontier(pste) ) {
                int prevCapt = 0;
                if( Capt((pste-1)->lastMove) )
                    prevCapt = MaterialGain(pcon, (pste-1)->lastMove);
                int exchange = see(pcon, pste, From(pcm->m), To(pcm->m));
                exchange -= prevCapt;
                if( exchange > 200 )
                    pste->plyRem--;
            }
            good_capt_done = true;
        }
/*        if( Capt(pcm->m) ) {
            int cVal = see(pcon, pste, From(pcm->m), To(pcm->m));
            if( 
                (a < valKNIGHT ) && 
    			((cVal > 0) && pste->plyRem <= 4)
                ) {
                stats.recapture++;
                extension = 1;
                can_reduce = false;
            }
        }*/

        //
        //	Make the move
        bMove(pcon, pste, pcm->m);

//        if( pste->plyRem <= 2 )
            if ( (pcm->m == cm1) || (pcm->m == cm2) )
                can_reduce = false;

        stats.c_int++;
//	    int posVal = -ValEval(pcon, pste+1, -b, -a);

        if( (pste + 1)->checkf ) {
            can_reduce = false;
            extension_check(pcon, pste, pcm, extension, a, depth, /*isPV*/true);
        }
        extension_pawn(pcon, pste, phashp, pcm, extension, can_reduce, a, /*isPV*/false);

#ifdef USE_EFFORT
        checkEffort(pcon, pste, can_reduce, extension, /*isPV*/false);
#endif

//        if( Capt(pcm->m) ) {
//            extension_castle(pcon, pste, pcm, extension, can_reduce, a, /*isPV*/ false, kingRing);
//        }
        //  ==========================================================================
		//	Set up for recursion.
		//
		(pste + 1)->plyRem = pste->plyRem - 1;
        if( extension && (extCount < maxExt) ) {
            (pste + 1)->plyRem++;
        }

        //
		//	Recurse into normal search or quiescent search, as appropriate.
		//
        nodeType succNType = nodeCUT;
        if(nt == nodeCUT )
            succNType = nodeALL;

        U64 thisMoveNodeCount = pcon->ss.nodes;

#ifdef FUTILITY_PRUNING

        if( (fFound)
            && can_reduce && can_prune && !forced
            && !moveisPromotion(pcm)
            && isNodeUseless(pcon,pste,0,pcm)
            && !moveisCapture(pcm)
            ) {
                fFound = true;
//            int approx = posVal + f_margin;
            int approx = nmPosVal + f_margin;
            if( approx <= a ) {
                stats.FP_frontier++;
                //approx = a;
                if( approx > bestVal) {
                    bestVal = approx;
                    cmBest.m = 0;
                    pste->ccmPv = 0;
                }
/*                if( pste->plyRem >= 2 ) {
                    moveList[moveListCount++] = pcm->m;
                    moveScore[ moveListCount-1 ] = 0;
                    thisMoveNodeCount = 0;
                    nodeList[ moveListCount-1 ] = updateEffort(pcon, pste, thisMoveNodeCount);
                }*/
                bUndoMove(pcon, pste, pcm->m);
                //c_move++;
                continue;
            }
        }
        if( (fFound) && true
            && can_reduce 
            && !iaminCheck && !forced
            && (pste->plyRem <= 4)
            //&& (depth*2 >= pcon->ss.plyDepth )
            && (pste->plyRem+1 <= pcon->ss.plyDepth/2)
            && ( (c_move >= 4) /*&& (c_move*3 >= lmc)*/ )
            && (a > -valMATEscore)
            && !moveisPromotion(pcm)
            && !moveisCapture(pcm)
            && isNodeUseless(pcon, pste, 0/*hashMoveNodeCount*/, pcm)
            && History.can_prune(pcm, pste->plyRem)
            && noThreat(pcon, pste, pcm, 0)
            ) {
            stats.LMRhint++;
/*            if( pste->plyRem >= 2 ) {
                moveList[moveListCount++] = pcm->m;
                moveScore[ moveListCount-1 ] = 0;
                thisMoveNodeCount = 0;
                nodeList[ moveListCount-1 ] = updateEffort(pcon, pste, thisMoveNodeCount);
            }*/
            bUndoMove(pcon, pste, pcm->m);
            //c_move++;
            continue;
        }

#endif

#ifdef USE_LMR
        // Note : LMR done on all non pv nodes moves
        //
        // move order is [H] [good capt] [equ capt] [killers] others [bad capt]
        if(fFound
//            && (c_move >= 1)
#if 1
            && (c_move >= 4)
#else
            && (c_move >= 4)
#endif
//            && (depth >= 3)
//            && (depth >= 2)
            && can_reduce && !forced
            && !iaminCheck
//            && (pste->plyRem >= 1)
            && (pste->plyRem >= 2)
            && (a > -valMATEscore)
            && !moveisPromotion(pcm)
            && isNodeUseless(pcon, pste, 0/*hashMoveNodeCount*/, pcm)
//            && History.can_reduce(pcm)
            ) {
            //stats.LMRhint++;
            // search with a reduced depth
            stats.LMR++;
            pste->reduced = true;

		    (pste + 1)->plyRem --;
        }
#endif

        moveList[moveListCount++] = pcm->m;

        val = -Search(pcon, pste + 1, - Beta, - a, succNType);

        if( pste->reduced ) {
            pste->reduced = false;
            if( val > a ) {
                bool ir = inSearch;
                //inSearch = true;
                (pste + 1)->plyRem = pste->plyRem - 1;
//                Assert( (pste + 1)->plyRem == (pste->plyRem - 1) );
                // re-search at normal depth
                stats.LMRreSearch++;
//                if( val >= valMAX )
                    val = -Search(pcon, pste + 1, - Beta, - a, succNType);
//                else
//                    val = -Search(pcon, pste + 1, - Beta, - a, nodeALL);
                inSearch = ir;
            }
        }
#ifdef USE_EFFORT
    moveScore[ moveListCount-1 ] = val;
    if( pste->plyRem >= 2 ) {
        thisMoveNodeCount = pcon->ss.nodes - thisMoveNodeCount;
        nodeList[ moveListCount-1 ] = updateEffort(pcon, pste, thisMoveNodeCount);
    }
#endif

/*    thisMoveNodeCount = pcon->ss.nodes - thisMoveNodeCount;
    if( (pcon->ss.plyDepth >= 11) && (thisMoveNodeCount > 100000) ) {
        char tmp[256];
        sprintf(tmp, "#> fat node : d=%d n=" U64_FORMAT " ", pcon->ss.plyDepth+1, thisMoveNodeCount);
        VDumpLine(pcon, pste, tmp);
    }
*/
/*    if( (depth > 25)  ) {
        char tmp[256];
        sprintf(tmp, "# long pv : d=%d [%d %d] %d", depth, a, Beta, val);
        VDumpLine(pcon, pste, tmp);
    }*/

        bUndoMove(pcon, pste, pcm->m);
        if ((pcon->fAbort) || (pcon->fTimeout)) {
            return 0;
        }
        //	We got a value back.  We unmade the move.  We're not dead.  Let's
        //	see how good this move was.  If it was >= "beta", it was so good
        //	that we don't need to search for anything better, so we'll leave.
        //
        Assert(ValIsOk(val));
        if( val > bestVal ) {
            bestVal = val;
            if (val > a) {
                cmBest = *pcm;
                copyPV( pste, cmBest );
                if (val >= Beta) {
                    //
                    //	This move failed high
                    //
                    goto cutNode;
                }
                a = bestVal;
            }
        }
        fFound = true;
        c_move++;

#ifdef NULL_WINDOW
        b = a + 1;              // set the new null window
#endif
//        if( nt == nodeCUT && (bestVal < Alpha))
        nt = nodeALL;
    }

    if (!fFound) {
        pste->lastMove = 0;
        // We have no move !!
        if (iaminCheck) {
            // Checkmate
            bestVal = mateValue(depth);
        } else {
            // Draw
            bestVal = 0;
        }
        // ==================================
        return bestVal;
        // ==================================
    }
    // if I have a move, then this is not an all nodes
cutNode:;
    if ((pcon->fAbort) || (pcon->fTimeout)) {
        return 0;
    }
    pste->lastMove = cmBest.m;

    if( bestVal > Alpha ) {
        pste->lastMove = cmBest.m;
        // for nm refutation
        (pste+1)->lastMove = cmBest.cmk;

        if( cmBest.cmk & cmkHASH )
            stats.c_cut_H++;
        else if( cmBest.cmk & cmkKILLER )
            stats.c_cut_killer++;
        else if( moveisGoodCapture(&cmBest) )
            stats.c_cut_capt++;
        else if( cmBest.cmk & cmkBADCAPT )
            stats.c_cut_badcapt++;
        else
            stats.c_cut_quies++;

        if( bestVal >= Beta ) {
            stats.cCUT++;
            if(iMove == 0)
                stats.cCUTrank1++;
            if(iMove < 5)
                stats.cCUTrankn++;
#ifdef USE_EFFORT
            if( pste->plyRem >= 2 && !moveisCapture(&cmBest) )
                updateEffortAll(pcon, pste, nodeList, moveList, moveListCount, 10);
#endif
        } else
            stats.c_alpha_cut++;

        if (!pste->fNull && !iaminCheck) {

            History.update( &cmBest, depth );
#ifdef HISTORY
            if( bestVal >= Beta && !moveisCapture(&cmBest) ) {   // don't update on good captures
                for(unsigned int i = 0; i < moveListCount ; i++ ) {
                    if( Capt(moveList[i]) == 0 )
                        History.update_bad( moveList[i] );
                }
                History.update_good( cmBest.m );
            }
#endif
#ifdef USE_KILLERS
            if( ! moveisCapture(&cmBest) ) {
                CounterMoves.set( (pste-1)->lastMove, cmBest.m);
                if( bestVal >= valMATEscore )
                    MateKillers[depth].set(cmBest.m);
                else if( !moveisCapture(&cmBest) )
                    Killers[depth].set(cmBest.m);
            }
#endif
        }
//        if( (oldBound == hashfALPHA) )
//            stats.c_tot_alpha++;
        RecordHashValue(pcon, pste, cmBest, bestVal, hashfBETA, depth);
    } else {
//        if( pste->plyRem >= 0 )
//            stats.c_exp_alpha++;
//        if( pste->plyRem >= 2 ) {
//            updateEffortAll(pcon, pste, nodeList, moveList, moveListCount, 10);
//        }
//        if( oldBound == hashfBETA )
//            stats.c_tot_alpha++;
        stats.c_all++;
        RecordHashValue(pcon, pste, cmBest, bestVal, hashfALPHA, depth);
    }


    Assert(ValIsOk(bestVal));
    return bestVal;
}



static int SearchPV(PCON pcon, PSTE pste, int Alpha, int Beta)
{
	int	val;
    pste->ccmPv = 0;
    pste->reduced = false;
	CM cmBest(0,0);
    int preExtend = 0;
    int bestVal = valMIN;
    int iaminCheck = pste->checkf;
    pste->ttmove = false;   // TODO rename that
    U32 moveList[256];
#ifdef USE_EFFORT
    U32 nodeList[256];
    int nodeListCount = 0;
#endif

    if (pste->plyRem < 0) {
        return SearchQ(pcon, pste,  Alpha, Beta, true);
    }

    int depth = int(pste - pcon->argste) + 1;
    if( depth >= (csteMAX-5) ) {
        return SearchQ(pcon, pste,  Alpha, Beta, true);
    }

	//	Increment global node counter, and figure out if it is time to call
	//	the interface callback, which is what allows for an interruptible
	//	search.  It's crucial that this be called now and then.
	//
	if (++pcon->ss.nodes >= pcon->ss.nodesNext) {
		//
		//	2000 nodes is maybe 1/50to 1/100 of a second.
		//
		pcon->ss.nodesNext = pcon->ss.nodes + 10000;
		VPrCallback(pcon);
		VCheckTime(pcon);
        if( pcon->smode == smodeTHINK || pcon->smode == smodeANALYZE )
            VPrSendUciCurrentLine(pcon, pste);
	}
	if ((pcon->fAbort) || (pcon->fTimeout))
		return 0;
	//
	//	I can return draws and cut off if I'm somewhere other than the
	//	first ply of depth.
	//
	//	Return draw score if this is a repeated node.
	//
	if (depth > 2 && FRepSet(pcon, pste, pste->hashkPc)) {
        stats.repCount++;
		return 0;
	}

    // gathering stats
    int nmPosVal = pste->val;
    if(!pste->evaluated )
	    nmPosVal = ValEval(pcon, pste, Alpha, Beta);

    // extCount is the net number of extension done (all extensions - all reductions done)
    int extCount = depth - pcon->ss.plyDepth + pste->plyRem; // extCount < 0 => reduced or wanted depth not reached yet
    int maxExt = MAXEXT(depth - extCount);

	//	Check hash table, in order to get "best" moves, and try to cut
	//	off.  Don't cut off within two plies of the root, otherwise the
	//	program might allow stupid rep draws.
	//
    PHASH hash_entry = 0;
#ifdef USE_HASH
    hash_entry = ProbeHashValue(pste);
    if( hash_entry /*&& pste->plyRem >= 2*/)
        hash_entry->touch();
#endif

	//
	// Check alpha/beta window against a mate score
    // Thx for the idea
	//
    int OldAlpha = Alpha;
#ifdef DISTANCE_PRUNING
#if 1
    // this is the lowest score I can get from here
    int i_am_mate_score = mateValue(depth + 1);
    if( i_am_mate_score > Alpha ) {
        Alpha = i_am_mate_score;
        // Beta is mate in n, best here is mate in n+m, so return
        if(i_am_mate_score >= Beta)
            return Beta; //i_am_mate_score;
//            return i_am_mate_score;
    }
#endif
#if 1
    // this is the best score we can get from here
    int i_will_mate_score =  -mateValue(depth);
    if( i_will_mate_score < Beta ) {
        Beta = i_will_mate_score;
        // if I set + 0 then I don't see the final move (
            // I will not see something better than Alpha so just return
        // Alpha is mate in n, best here is mate in n+m, so return
        if( i_will_mate_score <= Alpha )
            return Alpha;
//            return i_will_mate_score;
    }
#endif
#endif

    //
    // try recognizers
    //
#ifdef USE_RECOGNIZER
    // the recognizers of bitbases where allready probed in the eval
    if( true && (depth > 4)
        && (pste->bound != fail_bound) && pste->evaluated) {
        recog_result ret = {recog_bound(pste->bound), pste->val, recog_allways_valid,
            WIN_CHANCE(100), WIN_CHANCE(100)  };
        if( ret.bound == exact_bound )
            return ret.score;
        if( ret.bound == lower_bound ) {
            if( ret.score >= Beta )
                return ret.score;
        }
        if( ret.bound == upper_bound ) {
            if( ret.score <= Alpha )
                return ret.score;
            if( ret.score < Beta )
                Beta = ret.score;
        }
    }
#endif
    pste->threatMove = U32(-1);
    pste->threatTarget = 0;

    PHASHPawn phashp;
    EvalPawns(pcon, pste, phashp);

#ifdef USE_HASH
#ifdef USE_IID
//    if( (hash_entry == NULL || (hash_entry->getMove() == 0)) && (pste->plyRem >= (4)) ) {
//    if( (hash_entry == NULL || (hash_entry->getMove() == 0)) && (pste->plyRem >= (3)) ) {
    if( (hash_entry == NULL || (hash_entry->getMove() == 0)) && (pste->plyRem >= (2)) ) {
        // depth /2 gives worse results (more nodes)
//        int reduc = pste->plyRem / 2;
//        int reduc = 2;
        int reduc = 1;
        pste->plyRem -= reduc;
        stats.c_iid++;
        bool ir = inSearch;
        inSearch = true;
        int iidval = SearchPV(pcon, pste, Alpha, Beta);
#if 1
        /* frombeowulf : Re-search properly if the previous search failed low, so that we know we're getting
         * a good move, not just the move with the highest upper bound (which is essentially
         * random and depends on the search order.) */
        if( (iidval <= Alpha) ) {
            iidval = SearchPV(pcon, pste, valMIN, Beta);
            stats.c_iidbad++;
        }
#endif
        inSearch = ir;
        pste->plyRem += reduc;
        hash_entry = ProbeHashValue(pste);
        // Is it possible to not have a hash move here ? (how many times ?)
        pste->lastMove = 0;
    }
#endif
#endif
    bool forced = false;
    U64 hashMoveNodeCount = 0;
    if( hash_entry ) {
        hashMoveNodeCount = hash_entry->count;
//        forced = hash_entry->getForced();
//        if( forced )
//            preExtend = 1;
//        if( forced && (pste->plyRem < pcon->ss.plyDepth/3) && (depth <= pcon->ss.plyDepth*2) )
//            preExtend = 1;
//        if( pste->plyRem < (pste-1)->plyRem && hash_entry->getExt() )
//            preExtend = 1;
    }

	//	Generate moves.
	//
    bGenMoves(pcon, pste);
	//
	//	I know how many reversible moves have been made up until now.
	//	I just tried to make another move.  If I am here, I didn't
	//	leave myself in check, so I succeeded.
	//
	//	If the number of reversible moves made before this is at
	//	least 50 for each side, this was a draw.
	//
    int lmc = legalMoveCount(pste);
    int rankCut = lmc;
	if ( (pste->plyFifty >= 50 * 2) && (lmc > 0) ) {
		pste->ccmPv = 0;
		return 0;
	}
    // single move extension
    if( (lmc == 1) && iaminCheck)
        preExtend = 1;

    bitboard kingRing = 0;
    before_search(pcon, pste, /*isPV*/ true, kingRing, forced, preExtend, extCount, depth);

    PrepareSort( pcon, pste, depth, phashp );
	//	Mark the PV and hash moves so they sort first.
	//
    // this should not be needed anymore
	if (lastPV[depth].m != 0) {
		SortPV(pste, depth);
		lastPV[depth].m = 0;
	}
#ifdef USE_HASH
	if (hash_entry != NULL)
		SortHash(pste, hash_entry);
#endif
#ifdef USE_KILLERS
    SortKillers(pste, depth);
#endif

    int a = Alpha;
    int b = Beta;

    unsigned int prevCaptureSqr = pste->prevCaptSqr;

    bool fFound = false;
    int  c_move = 0;

#if 0
    {
        // we will sort the moves on their real score : get it from the hash table or do some search
        // this is an iid on all moves
        foreachmove(pste,pcm) {
            // sort quiescent moves and 'bad' captures
            U32 mask = pcm->cmk & cmkMASK;
//            if( mask <= cmkKILLER ) {
            if( mask <= cmkGOODCAPT ) {

		        bMove(pcon, pste, pcm->m);
                PHASH new_hash_entry = ProbeHashValue(pste+1);
                if( mask == cmkKILLER || mask == cmkQUEEN )
                    mask = cmkNONE;
                if( new_hash_entry ) {
                    int val = - new_hash_entry->val;
                    if( (pste+1)->checkf )
                        val -= 5;
                    pcm->cmk = mask + valMAX + val;
                } else if( true && pste->plyRem >= 4 ) {    // 2
                    // >= 4 - 2
                    // >= 2 / 2
                    // >= 3 - 3
//                    (pste+1)->plyRem = pste->plyRem / 2;
#if 1
                    (pste+1)->plyRem = pste->plyRem - 3;
                    // full windows does not give a lot more nodes, TT filled with 'good' values
                    int val = -SearchPV(pcon, pste + 1, -b, -a);
#else
//                    (pste+1)->plyRem = 0;
//                    int val = -SearchPV(pcon, pste + 1, -b, -a);
                    int val = -SearchQ(pcon, pste + 1, -b, -a, true);
#endif
//                    val = -Search(pcon, pste + 1, beta_lim - 100, beta_lim, nodeCUT);
                    if( (pste+1)->checkf )
                        val -= 5;
                    pcm->cmk = mask + valMAX + val;
                }
                bUndoMove(pcon, pste, pcm->m);
            }
        }
    }
#else
//    if( pste->plyRem >= 1 )
    if( false )
    {
        // we will sort the moves on their real score : get it from the hash table or do some search
        // this is an iid on all moves
        foreachmove(pste,pcm) {
            // sort quiescent moves and 'bad' captures
            U32 mask = pcm->cmk & cmkMASK;
            if( mask < cmkGOODCAPT ) {
//            if( mask <= cmkEQUCAPT ) {
		        bMove(pcon, pste, pcm->m);
                PHASH new_hash_entry = ProbeHashValue(pste+1);
                if( mask == cmkKILLER || mask == cmkQUEEN )
                    mask = cmkNONE;
                if( new_hash_entry ) {
                    U32 effort = new_hash_entry->count;
                    if( effort >= cmkBADCAPT )
                        effort = cmkBADCAPT - 1;
                    effort |= (cmkBADCAPT >> 1);
//                    if( - new_hash_entry->val < -valMATEscore )
//                        effort = 0; // look that last

/*                    if( (depth & 1) == 0 ) {
                        effort = cmkBADCAPT - effort - 1;
                    }*/
                    pcm->cmk = mask | effort;
                }
                bUndoMove(pcon, pste, pcm->m);
            }
        }
    }

#endif
    U32 cm1, cm2;
    CounterMoves.get( (pste-1)->lastMove, cm1, cm2);

    //
    //	Iterate through the move list, trying everything out.
    //
    foreachmove(pste,pcm) {
        //
        //	All moves are sorted on the cmk field
        //	cmk is set in PrepareSort() and in other sort functions

        SortNext(pste, pcm);
        //
        //	Make the move

        int extension = preExtend;
        // Note : now I allow the reduction after the extension,
        // this is needed for the check extension
        bool can_reduce = (extension) > 0 ? false : true;
        can_reduce = can_reduce && !forced;

#ifdef RECAPTURE_EXTENSION
//        int cVal;
        if( Capt(pcm->m) ) {
            int cVal = see(pcon, pste, From(pcm->m), To(pcm->m));
            if( 
    //            && (To(pcm->m) == prevCaptureSqr)
    //            && see(pcon, pste, From(pcm->m), To(pcm->m)) > -350
                ((cVal > -200) && (To(pcm->m) == prevCaptureSqr)
    //            && (a < valKNIGHT )
    //			    || (pcon->ss.plyDepth - depth >= 6) 
//    			    || ((cVal > -200) && pste->plyRem <= 4) 
                )

                ) {
                stats.recapture++;
                extension = 1;
                can_reduce = false;
            }
        }
#endif

        bMove(pcon, pste, pcm->m);
        moveList[iMove] = pcm->m;

        if ( (pcm->m == cm1) || (pcm->m == cm2) )
            can_reduce = false;

//        stats.c_int++;
	    //int posVal = -ValEval(pcon, pste+1, -b, -a);

        if( (pste + 1)->checkf ) {
            can_reduce = false;
            extension_check(pcon, pste, pcm, extension, a, depth, /*isPV*/false);
        }
        extension_pawn(pcon, pste, phashp, pcm, extension, can_reduce, a, /*isPV*/true);

//        if(fFound && !moveisGoodCapture(pcm))
#ifdef USE_EFFORT
        checkEffort(pcon, pste, can_reduce, extension, /*isPV*/true);
#endif

        if( Capt(pcm->m) ) {
            extension_castle(pcon, pste, pcm, extension, can_reduce, a, /*isPV*/ true, kingRing);
        }

/*        if( fFound && pste->plyRem >= 2 ) {
            PHASH hash_entry = ProbeHashValue(pste+1);
            if( hash_entry ) {
                U64 thisMoveNodeCount = hash_entry->count;
                if( hashMoveNodeCount > 4 && thisMoveNodeCount > hashMoveNodeCount/2 ) {
                    extension = 1;
                    can_reduce = false;
                }
            }
        }*/

        //  ==========================================================================
		//	Set up for recursion.
		//
		(pste + 1)->plyRem = pste->plyRem - 1;
        if( extension && (extCount < maxExt) ) {
            (pste + 1)->plyRem++;
        }

        U64 thisMoveNodeCount = pcon->ss.nodes;
        //
		//	Recurse into normal search or quiescent search, as appropriate.
		//
#ifdef USE_LMR
        // Note : LMR done on all non pv nodes moves
        //
        // move order is [H] [good capt] [killers] others [bad capt]
        if( false && 
            (c_move >= 8)
//            && (depth >= 3)
//            && (depth >= 2)
//            && (bestVal >= a)
            && can_reduce
            && !iaminCheck
//            && (pste->plyRem >= 2)
            && (pste->plyRem >= 1)
            && (depth*2 >= pcon->ss.plyDepth )
            && (a > -valMATEscore)
            && !moveisPromotion(pcm)
            && isNodeUseless(pcon, pste, 0/*hashMoveNodeCount*/, pcm)
            && History.can_reduce(pcm)
            ) {
//            stats.LMRhint++;
            // search with a reduced depth
            stats.LMR++;
            pste->reduced = true;

		    (pste + 1)->plyRem --;
        }
#endif
        if (!fFound) {
            val = -SearchPV(pcon, pste + 1, - Beta, - a);
            stats.c_pv++;
        } else {
            stats.c_pv_succ++;
            val = -Search(pcon, pste + 1, - b, - a, nodeCUT); // cut nodes

            if( pste->reduced ) {
                pste->reduced = false;
                if( val > a ) {
        		    //(pste + 1)->plyRem ++;
                    (pste + 1)->plyRem = pste->plyRem - 1;
                    //Assert( (pste + 1)->plyRem == (pste->plyRem - 1) );
                    // re-search at normal depth
                    stats.LMRreSearch++;
                    val = -Search(pcon, pste + 1, - b, - a, nodeCUT);
                }
            }
/*            if( (extension == 0) && (val <= a) && (pste+1)->threatMove != U32(-1)
                && !moveisCapture(pcm) && val > nmPosVal + 50) {
                extension = 1;
                (pste + 1)->plyRem += 2;
                val = -Search(pcon, pste + 1, - b, - a, nodeCUT); // cut nodes
            }*/
            if( val > a /*&& val < Beta*/) { // re-search
                stats.reSearch++;
                bool ir = inSearch;
                //inSearch = true;
                val = -SearchPV(pcon, pste + 1, - Beta, - a); // new PV
                inSearch = ir;
            }

        }

#ifdef USE_EFFORT
    thisMoveNodeCount = pcon->ss.nodes - thisMoveNodeCount;
    //updateEffort(pcon, pste, thisMoveNodeCount);
    nodeList[ iMove ] = updateEffort(pcon, pste, thisMoveNodeCount);
    nodeListCount = iMove+1;
#endif

/*    if( (depth <= 3) && (thisMoveNodeCount > 1) ) {
        char tmp[256];
        sprintf(tmp, "#> stats PV : d=%d #=%d v=%d n=" U64_FORMAT " ", pcon->ss.plyDepth, c_move, val, thisMoveNodeCount);
        VDumpLine(pcon, pste, tmp);
    }
*/

        bUndoMove(pcon, pste, pcm->m);
        if ((pcon->fAbort) || (pcon->fTimeout)) {
            return 0;
        }
        //	We got a value back.  We unmade the move.  We're not dead.  Let's
        //	see how good this move was.  If it was >= "beta", it was so good
        //	that we don't need to search for anything better, so we'll leave.
        //
        //	If it was not >= "beta", but it was > "alpha", this is better than
        //	anything else we've found before, but not so good we have to
        //	leave.  These kinds of moves are actually quite rare.  If I find
        //	one of these, I have to store it in the PV (main-line) that I'm
        //	constructing.  This might end up being the main-line for the whole
        //	search, if it gets backed up all the way to the root.
        //
        if( val > bestVal ) {
            bestVal = val;
            if (val > a) {
                cmBest = *pcm;
                copyPV( pste, cmBest );
                if (val >= Beta) {
                    //
                    //	This move failed high
                    //
                    goto cutNode;
                }
                //	This move is between alpha and beta, which is actually pretty
                //	rare.  If this happens I have to add a PV move, and append the
                //	returned PV to it
                //
                a = bestVal;
                rankCut = iMove+1;
            }
        }
        fFound = true;
        c_move++;

#ifdef NULL_WINDOW
        b = a + 1;              // set the new null window
#endif
    }

    if (!fFound) {
        pste->lastMove = 0;
        // We have no move !!
        if (iaminCheck) {
            // Checkmate
            bestVal = mateValue(depth);
        } else {
            // Draw
            bestVal = 0;
        }
        // ==================================
        return bestVal;
        // ==================================
    }
    // if I have a move, then this is not an all nodes
cutNode:;

    pste->lastMove = cmBest.m;
    if( bestVal > Alpha ) {
        pste->lastMove = cmBest.m;
        // for nm refutation
        (pste+1)->lastMove = cmBest.cmk;

        if( cmBest.cmk & cmkHASH )
            stats.c_cut_H++;
        else if( cmBest.cmk & cmkKILLER )
            stats.c_cut_killer++;
        else if( moveisGoodCapture(&cmBest) )
            stats.c_cut_capt++;
        else if( cmBest.cmk & cmkBADCAPT )
            stats.c_cut_badcapt++;
        else
            stats.c_cut_quies++;
/*
        if( bestVal >= Beta ) {
            stats.cCUT++;
            if(iMove == 0)
                stats.cCUTrank1++;
            if(iMove < 5)
                stats.cCUTrankn++;
        } else {
            if( depth == pcon->ss.plyDepth ) {
                stats.cCUTa += lmc;
                stats.cCUTaRank += rankCut;
            }
            stats.c_alpha_cut++;
        }
*/
        if ( !iaminCheck) {

            History.update( &cmBest, depth );
#ifdef HISTORY
            if( bestVal >= Beta && !moveisCapture(&cmBest) ) {   // don't update on good captures
                for(int i = 0; i < iMove ; i++ ) {
                    if( Capt(moveList[i]) == 0 )
                        History.update_bad( moveList[i] );
                }
                History.update_good( cmBest.m );
            }
#endif

#ifdef USE_KILLERS
            //  Set the new killer move only if it was not marked as good allready
            //  'Good' moves are allready sorted first so storing them in the killer
            //  array is counter productive
            if( ! Capt(cmBest.m) ) {
                CounterMoves.set( (pste-1)->lastMove, cmBest.m);
                if( bestVal >= valMATEscore )
                    MateKillers[depth].set(cmBest.m);
                else if( !moveisCapture(&cmBest) )
                    Killers[depth].set(cmBest.m);
            }
#endif
        }
    } /*else
        stats.c_all++;*/
#ifdef USE_EFFORT
    updateEffortAll(pcon, pste, nodeList, moveList, nodeListCount, 10);
#endif
    if( ! pcon->fTimeout ) {
        RecordHashValue(pcon, pste, cmBest, bestVal, LOWERBOUND, Beta, depth, false);
    }
    Assert(ValIsOk(bestVal));
	return bestVal;
}

//  -------------------------------------------------------------------------

//  Full-width search.  This is the heart of the "chess playing" part of this
//  program.  It performs a recursive alpha-beta search.

static struct {
    char    move[12];
    int     c_nodes;
    int     hval;
} sortDebug[200];
static int c_sortDebug;

#define maxRootMoves    200
static struct _rootMove {
    char szmove[8];
    int score;
    int hint;
    U64 nodes;
    bool    evaluated;
    CM cm;
} rootMoves[maxRootMoves];
static int rootMovesCount;

static int sortScore( struct _rootMove & move) {
    return move.hint * 10000 + move.score;
}

int ValSearchRoot(PCON pcon, PSTE pste, int Alpha, int Beta)
{
    int a = Alpha;
    int b = Beta;
    int bestVal = valMIN;
    int	val;
    int depth = 1;
    pste->lastMove = 0;
    pste->reduced = false;
    pste->c_ttpair = 0;
    pste->threatMove = U32(-1);
    pste->threatTarget = 0;
    pste->prevCaptSqr = -1;
//    pste->danger = false;
    pste->fNull = false;
    pste->ttmove = false;   // TODO rename that
    pcon->hashStore = true;
    inSearch = false;
    c_sortDebug = 0;
#ifdef USE_EFFORT
    U32 moveList[256];
    U32 nodeList[256];
    int nodeListCount = 0;
#endif

    //	Increment global node counter
    //
    ++pcon->ss.nodes;

    //	Stuff is doone a little differently at the root than at any other
    //	ply.
    //
    //
    //	Mark this move so if I find it in a sub-tree I can detect a rep
    //	draw.  I don't check to see if a rep is already set, because if
    //	we are here, we have to be making a move, not whining about a
    //	blown chance for a draw.  I also don't try to cut off from the
    //	hash table, for the same reason.  I want a move, not a score.
    //
    //	I do probe the table though, in case there is "best move" info
    //	in there.
    //
    PHASH hash_entry = ProbeHashValue(pste);
    //
    //	If it's the first time we come here (depth == 0)
    //	generate the moves
    //
    if( pste->plyRem == 0 ) {
        // first time with this position
        bGenMoves(pcon, pste);
        pcon->ss.ccmLegalMoves = legalMoveCount(pste);
        rootMovesCount = pcon->ss.ccmLegalMoves;

        PHASHPawn phashp;
        EvalPawns(pcon, pste, phashp);
        PrepareSort( pcon, pste, depth, phashp );

        foreachmove(pste,pcm) {
            CmToSz( pcm, rootMoves[iMove].szmove );
            rootMoves[iMove].cm     = *pcm;
            rootMoves[iMove].hint   = 0;
            rootMoves[iMove].evaluated = false;

            bMove(pcon, pste, pcm->m);
            (pste+1)->plyRem = -2;  // -2 no check, no nm search
            int rootScore = -ValEval(pcon, pste+1, valMIN, valMAX);
//            int val = -SearchQ(pcon, pste + 1, -rootScore-500, -rootScore+500+1, true);
//            int val = -rootScore;
            int val = -SearchQ(pcon, pste + 1, -rootScore, -rootScore+1, true);
            bUndoMove(pcon, pste, pcm->m);
            rootMoves[iMove].score  = val; // keep the generation order

            rootMoves[iMove].nodes  = 0;
        }
    }
    {
        // after the first time (first depth iteration)
        // sort the moves by node count
#ifdef USE_HASH
        PHASH phash = hash_entry;
        U32 hm = phash ? phash->getMove() : 0;
#endif
        for(int i = 0; i < rootMovesCount; i++) {
            PCM pcm = &rootMoves[i].cm;
            if ( rootMoves[i].cm.m == lastPV[1].m )
                rootMoves[i].hint = 10;
#ifdef USE_HASH
		    else if ( pcm->m == hm )
                rootMoves[i].hint = 1;
#endif
        }
    }
	//	Mark the PV and hash moves so they sort first.
	//
    // bubble sort
    for(int i = 0; i < rootMovesCount - 1; i++) {

        for(int j = i+1 ; j < rootMovesCount; j++)
            // most moves have the same score because of null window search
            if( (sortScore(rootMoves[j]) > sortScore(rootMoves[i]) ||
                (sortScore(rootMoves[j]) == sortScore(rootMoves[i])) && (rootMoves[j].nodes > rootMoves[i].nodes) )) {
                _rootMove swap = rootMoves[i];
                rootMoves[i] = rootMoves[j];
                rootMoves[j] = swap;

            }
    }
#if 0
    printf("# SORT is ");
    for(int i = 0; i < rootMovesCount && i < 10; i++) {
        if( rootMoves[i].nodes < 10000 )
            printf("%s (%d) ", rootMoves[i].szmove, rootMoves[i].nodes );
        else
            printf("%s (%dk) ", rootMoves[i].szmove, rootMoves[i].nodes / 1000 );
    }
    printf("\n");
#endif

    PHASHPawn phashp;
    EvalPawns(pcon, pste, phashp);
    bool forced = false;
    int preExtend = 0;
    bitboard kingRing = 0;

    before_search(pcon, pste, /*isPV*/ true, kingRing, forced, preExtend, 0, depth);

    CM cmBest(0,0);
	bool fFound = false;
	//
	//	Iterate through the move list, trying everything out.
	//
    for(int i = 0; i < rootMovesCount; i++) {
        PCM pcm = &rootMoves[i].cm;
//        pcm->cmk = i;

        //	Record information about which move I am searching ...
        //
        SAN	san;
        pcon->ss.icmSearching = i+1;
        CmToSan(pcon, pste, pcm, &san);
        SanToSz(&san, pcon->ss.aszSearching);
        CmToSz(pcm, pcon->ss.aszSearchSan);
        U64 oldNodes = pcon->ss.nodes;
        VPrSendUciCurrMove( pcon->ss );

        //
        //	... then go make the move.
        //
        bMove(pcon, pste, pcm->m);
        bool can_reduce = true;
        int extension = 0;
	    (void) ValEval(pcon, pste, a, b);

        if( (pste + 1)->checkf ) {
            can_reduce = false;
            extension_check(pcon, pste, pcm, extension, a, depth, /*isPV*/true);
        }
        extension_pawn(pcon, pste, phashp, pcm, extension, can_reduce, a, /*isPV*/true);
//        if( Capt(pcm->m) ) {
//            extension_castle(pcon, pste, pcm, extension, can_reduce, a, /*isPV*/ true, kingRing);
//        }

        //
		//	Set up for recursion.
		//
		(pste + 1)->plyRem = pste->plyRem - 1 + extension;

		//
		//	Recurse into normal search or quiescent search, as appropriate.
		//
        U64 thisMoveNodeCount = pcon->ss.nodes;
        nodeType succNType;
        if(fFound)
            succNType = nodeCUT;
        else
            succNType = nodePV;

        if (!fFound) {
            //pste->plyRem++;
            val = -SearchPV(pcon, pste + 1, - Beta, - a);
            //pste->plyRem--;
        } else {
/*            if( i == 1 )
                val = -SearchPV(pcon, pste + 1, - b, - a);
            else*/
            val = -Search(pcon, pste + 1, - b, - a, succNType); // cut nodes
//            val = -SearchPV(pcon, pste + 1, - (a+1), - a); // cut nodes
            if( val > a && !((pcon->fAbort) || (pcon->fTimeout))) { // re-search
//                int usedTime = TmNow() - pcon->ss.tmStart;
//                int remaingingTime = pcon->ss.tmEnd  - TmNow();
//                if( usedTime < remaingingTime )
                    pcon->ss.rootHasChanged = true;
                pcon->ss.easyMove = false;
                //inSearch = true;
                stats.reSearch++;
                val = -SearchPV(pcon, pste + 1, - Beta, - a); // new PV
                inSearch = false;
            }
        }
        thisMoveNodeCount = pcon->ss.nodes - thisMoveNodeCount;
#ifdef USE_EFFORT
        //updateEffort(pcon, pste, thisMoveNodeCount);
        moveList[ i ] = pcm->m;
        nodeList[ i ] = updateEffort(pcon, pste, thisMoveNodeCount);
        nodeListCount = i+1;
#endif
        // update the next sort key
        rootMoves[i].hint  = 0;
        if( ! rootMoves[i].evaluated )
            rootMoves[i].score = val;
//        rootMoves[i].nodes = pcon->ss.nodes - oldNodes;
//        rootMoves[i].nodes += pcon->ss.nodes - oldNodes;
        rootMoves[i].nodes = 0*rootMoves[i].nodes/4 + (pcon->ss.nodes - oldNodes);

        // debugging stuff
        sortDebug[c_sortDebug].c_nodes = int(pcon->ss.nodes) - sortDebug[c_sortDebug].c_nodes;
        c_sortDebug++;

		bUndoMove(pcon, pste, pcm->m);
		if ((pcon->fAbort) || (pcon->fTimeout)) {
			return 0;
		}
		//	We got a value back.  We unmade the move.  We're not dead.  Let's
		//	see how good this move was.  If it was >= "beta", it was so good
		//	that we don't need to search for anything better, so we'll leave.
		//
		//	If it was not >= "beta", but it was > "alpha", this is better than
		//	anything else we've found before, but not so good we have to
		//	leave.  These kinds of moves are actually quite rare.  If I find
		//	one of these, I have to store it in the PV (main-line) that I'm
		//	constructing.  This might end up being the main-line for the whole
		//	search, if it gets backed up all the way to the root.
        //
        if( val > bestVal ) {
            bestVal = val;
            if (val > a) {
                if (val >= Beta) {
                    // Note : not reached if not doing aspiration search
                    //
                    //	This move failed high, so we are going to return beta,
                    //	but if we're sitting at the root of the search I will set
                    //	up a one-move PV (if this isn't already the move I'm
                    //	planning to make), so this move will be made if I run out
                    //	of time before resolving the fail-high.
                    //
                    copyPV( pste, *pcm );

                    //	This function needs the root moves generated, which
                    //	they are.
                    //
                    RecordHashValue(pcon, pste, *pcm, bestVal, Alpha, Beta, depth, false);
                    Beta = val;
                    VDumpPv(pcon, pcon->ss.plyDepth, Beta, prsaFAIL_HIGH);
                    return bestVal;
			    }
                //	This move is between alpha and beta, which is actually pretty
                //	rare.  If this happens I have to add a PV move, and append the
                //	returned PV to it, and if I'm at the root I'll send the PV to
                //	the interface so it can display it.
                //
                rootMoves[i].score = val;
                rootMoves[i].evaluated = true;
                cmBest = *pcm;
                copyPV( pste, cmBest );
                a = bestVal;
                //
                //	This function needs the root moves generated, and they
                //	are.
                //
                // clear failed high, can be set again in VDumpPV is val is bad
                pcon->ss.failed = false;
                VDumpPv(pcon, pcon->ss.plyDepth, a, prsaNORMAL);
#if 1 //_DEBUG
                if( TmNow() - pcon->ss.tmStart > 1000 )
                    stats.report();
#endif
            }
        }
#ifdef NULL_WINDOW
        b = a + 1;              // null window
#endif
        fFound = true;
    }
#if 0
    printf("# SORT was ");
    for(int i = 0; i < c_sortDebug && i < 10; i++) {
        if( sortDebug[i].c_nodes < 10000 )
            printf("%s (%d) ", sortDebug[i].move, sortDebug[i].c_nodes );
        else
            printf("%s (%dk) ", sortDebug[i].move, sortDebug[i].c_nodes / 1000 );
        if( sortDebug[i].hval > 0 )
            printf("%d ", sortDebug[i].hval );
    }
    printf("\n");
#endif
    if (!fFound) {
        // We have no move !!
        if (pste->checkf) {
            // Checkmate
            bestVal = mateValue(depth);
        } else {
            // Draw
            bestVal = 0;
        }
    }
    for(int i = 0; i < rootMovesCount; i++) {
        // only use node count
        rootMoves[i].score = 0;
        rootMoves[i].evaluated = false;
    }
#ifdef USE_EFFORT
    updateEffortAll(pcon, pste, nodeList, moveList, nodeListCount, 10);
#endif
    if( ! pcon->fTimeout )
        RecordHashValue(pcon, pste, cmBest, bestVal, Alpha, Beta, depth, false);
    Assert(ValIsOk(bestVal));
    return bestVal;
}
