/*
Rodent, a UCI chess playing engine derived from Sungorus 1.4
Copyright (C) 2009-2011 Pablo Vazquez (Sungorus author)
Copyright (C) 2011-2016 Pawel Koziol

Rodent is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published
by the Free Software Foundation, either version 3 of the License,
or (at your option) any later version.

Rodent is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty
of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "rodent.h"

static const U64 bbKingBlockH[2] = { SqBb(H8) | SqBb(H7) | SqBb(G8) | SqBb(G7),
                                     SqBb(H1) | SqBb(H2) | SqBb(G1) | SqBb(G2) };

static const U64 bbKingBlockA[2] = { SqBb(A8) | SqBb(A7) | SqBb(B8) | SqBb(B7),
                                     SqBb(A1) | SqBb(A2) | SqBb(B1) | SqBb(B2) };

static const int BN_wb[64] = {
    0,    0,  15,  30,  45,  60,  85, 100,
    0,   15,  30,  45,  60,  85, 100,  85,
    15,  30,  45,  60,  85, 100,  85,  60,
    30,  45,  60,  85, 100,  85,  60,  45,
    45,  60,  85, 100,  85,  60,  45,  30,
    60,  85, 100,  85,  60,  45,  30,  15,
    85, 100,  85,  60,  45,  30,  15,   0,
   100,  85,  60,  45,  30,  15,   0,   0
};

static const int BN_bb[64] = {
    100, 85,  60,  45,  30,  15,   0,   0,
    85, 100,  85,  60,  45,  30,  15,   0,
    60,  85, 100,  85,  60,  45,  30,  15,
    45,  60,  85, 100,  85,  60,  45,  30,
    30,  45,  60,  85, 100,  85,  60,  45,
    15,  30,  45,  60,  85, 100,  85,  60,
    0,   15,  30,  45,  60,  85, 100,  85,
    0,   0,   15,  30,  45,  60,  85, 100
};


int GetDrawFactor(POS *p, int sd) 
{
  int op = Opp(sd);

  // Case 1: KPK with edge pawn (else KBPK recognizer would break)

  if (PcMatNone(p, sd)
  && PcMatNone(p, op)
  && p->cnt[sd][P] == 1    // TODO: all pawns of a stronger side on a rim
  && p->cnt[op][P] == 0) { // TODO: accept pawns for a weaker side

    if (p->Pawns(sd) & FILE_H_BB
    &&  p->Kings(op) & bbKingBlockH[sd]) return 0;

    if (p->Pawns(sd) & FILE_A_BB
    &&  p->Kings(op) & bbKingBlockA[sd]) return 0;
  }

  // Case 2: KBPK(P) draws with edge pawn and wrong bishop

  if (PcMatB(p, sd)
	  && PcMatNone(p, op)
	  && p->cnt[sd][P] == 1) { // TODO: all pawns of a stronger side on a rim

	  if (p->Pawns(sd) & FILE_H_BB
		  && NotOnBishColor(p, sd, REL_SQ(H8, sd))
		  && p->Kings(op)  & bbKingBlockH[sd]) return 0;

	  if (p->Pawns(sd) & FILE_A_BB
		  && NotOnBishColor(p, sd, REL_SQ(A8, sd))
		  && p->Kings(op)  & bbKingBlockA[sd]) return 0;
  }

  // Case 3: KBP vs Km
  // drawn when defending king stands on pawn's path and can't be driven out 
  // by a bishop (must be dealt with before opposite bishops ending)

  if (PcMatB(p, sd)
  && PcMat1Minor(p, op)
  && p->cnt[sd][P] == 1
  && p->cnt[op][P] == 0
  && (SqBb(p->king_sq[op]) & BB.GetFrontSpan(p->Pawns(sd), sd))
  && NotOnBishColor(p, sd, p->king_sq[op]) ) 
     return 0;

  // Case 4: Bishops of opposite color

  if (PcMatB(p, sd) && PcMatB(p, op) && DifferentBishops(p)) {

     // 4a: single bishop cannot win without pawns
     if (p->cnt[sd][P] == 0) return 0;

     // 4b: different bishops with a single pawn on the own half of the board
     if (p->cnt[sd][P] == 1
     &&  p->cnt[op][P] == 0) {
       if (bbHomeZone[sd] & p->Pawns(sd)) return 0;

     // TODO: 4c: distant bishop controls a square on pawn's path
     }

     // 4d: halve the score for pure BOC endings
     return 32;
  }

  if (p->cnt[sd][P] == 0) {

    // Case 5: low and almost equal material with no pawns

    if (p->cnt[op][P] == 0) {
      if (PcMatRm(p, sd) && PcMatRm(p, op)) return 8;
      if (PcMatR (p, sd) && PcMatR (p, op)) return 8;
      if (PcMatQ (p, sd) && PcMatQ (p, op)) return 8;
      if (PcMat2Minors(p, sd) && PcMatR(p, op)) return 8;
    }

	// Case 6: two knights
	if (PcMatNN(p, sd)) {
		if (p->cnt[op][P] == 0) return 0;
		else return 4;
	}
     
    // Case 7: K(m) vs K(m) or Km vs Kp(p)
    if (p->cnt[sd][Q] + p->cnt[sd][R] == 0 && p->cnt[sd][B] + p->cnt[sd][N] < 2 ) 
    return 0;

    // Case 8: KR vs Km(p)
    if (PcMatR(p, sd) && PcMat1Minor(p, op) ) return 16;

    // Case 9: KRm vs KR(p)
    if (p->cnt[sd][R] == 1 && p->cnt[sd][Q] == 0 &&  p->cnt[sd][B] + p->cnt[sd][N] == 1
    &&  PcMatR(p, op) ) return 16;

    // Case 10: KQm vs KQ(p)
    if (p->cnt[sd][Q] == 1 && p->cnt[sd][R] == 0 && p->cnt[sd][B] + p->cnt[sd][N] == 1
    &&  PcMatQ(p, op) ) return 32;

    // Case 11: Kmm vs KB(p)
    if (PcMat2Minors(p,sd) &&  PcMatB(p, op) ) return 16;

    // Case 12: KBN vs Km(p)
    if (PcMatBN(p, sd) &&  PcMat1Minor(p, op) ) return 16;

	// Case 13: KRR vs KRm(p)
	if (PcMatRR(p, sd) && PcMatRm(p, op)) return 16;

	// Case 14: KRRm vs KRR(p)
	if (p->cnt[sd][R] == 2 && p->cnt[sd][Q] == 0 && p->cnt[sd][B] + p->cnt[sd][N] == 1
    && PcMatRR(p, op)) return 16;
  }

  // Case 15: KRP vs KR

  if (PcMatR(p, sd) && PcMatR(p, op) && p->cnt[sd][P] == 1 && p->cnt[op][P] == 0) {

    // 15a: good defensive position with a king on pawn's path increases drawing chances

    if ((SqBb(p->king_sq[op]) & BB.GetFrontSpan(p->Pawns(sd), sd)))
      return 32; // 1/2

    // 15b: draw code for rook endgame with edge pawn

    if ((RelSqBb(A7, sd) & p->Pawns(sd))
    && ( RelSqBb(A8, sd) & p->Rooks(sd))
    && ( FILE_A_BB & p->Rooks(op))
    && ((RelSqBb(H7, sd) & p->Kings(op)) || (RelSqBb(G7, sd) & p->Kings(op)))
    ) return 0; // dead draw

    if ((RelSqBb(H7, sd) & p->Pawns(sd))
    && ( RelSqBb(H8, sd) & p->Rooks(sd))
    && ( FILE_H_BB & p->Rooks(op))
    && ((RelSqBb(A7, sd) & p->Kings(op)) || (RelSqBb(B7, sd) & p->Kings(op)))
    ) return 0; // dead draw

  }

  return 64;
}

int DifferentBishops(POS * p) {

  if ((bbWhiteSq & p->Bishops(WC)) && (bbBlackSq & p->Bishops(BC))) return 1;
  if ((bbBlackSq & p->Bishops(WC)) && (bbWhiteSq & p->Bishops(BC))) return 1;
  return 0;
}

int NotOnBishColor(POS * p, int bishSide, int sq) {

  if (((bbWhiteSq & p->Bishops(bishSide)) == 0)
  && (SqBb(sq) & bbWhiteSq)) return 1;

  if (((bbBlackSq & p->Bishops(bishSide)) == 0)
  && (SqBb(sq) & bbBlackSq)) return 1;

  return 0;
}

int CheckmateHelper(POS *p)
{
   int result = 0;

   if (p->cnt[WC][P] == 0
   &&  p->cnt[BC][P] == 0) {

      if (PcMatBN(p, WC) && PcMatNone(p,BC) ) { // mate with bishop and knight
         if ( p->Bishops(WC) & bbWhiteSq) result -= 2*BN_bb[p->king_sq[BC]];
         if ( p->Bishops(WC) & bbBlackSq) result -= 2*BN_wb[p->king_sq[BC]];
      }
        
      if (PcMatBN(p, BC) && PcMatNone(p,WC)) { // mate with bishop and knight
         if ( p->Bishops(BC) & bbWhiteSq) result += 2*BN_bb[p->king_sq[WC]];
         if ( p->Bishops(BC) & bbBlackSq) result += 2*BN_wb[p->king_sq[WC]];
      }  
}

    return result;
}

int PcMatNone(POS * p, int sd) {
  return (p->cnt[sd][B] + p->cnt[sd][N] + p->cnt[sd][Q] + p->cnt[sd][R] == 0);
}

int PcMat1Minor(POS *p, int sd) {
  return (p->cnt[sd][B] + p->cnt[sd][N] == 1 && p->cnt[sd][Q] + p->cnt[sd][R] == 0);
}

int PcMat2Minors(POS *p, int sd) {
  return (p->cnt[sd][B] + p->cnt[sd][N] == 2 && p->cnt[sd][Q] + p->cnt[sd][R] == 0);
}

int PcMatNN(POS *p, int sd) {
	return (p->cnt[sd][N] == 2 && p->cnt[sd][B] + p->cnt[sd][Q] + p->cnt[sd][R] == 0);
}

int PcMatBN(POS *p, int sd) {
  return (p->cnt[sd][B] == 1 && p->cnt[sd][N] == 1 && p->cnt[sd][Q] + p->cnt[sd][R] == 0);
}

int PcMatB(POS *p, int sd) {
  return (p->cnt[sd][B] == 1 && p->cnt[sd][N] + p->cnt[sd][Q] + p->cnt[sd][R] == 0);
}

int PcMatQ(POS *p, int sd) {
  return (p->cnt[sd][Q] == 1 && p->cnt[sd][N] + p->cnt[sd][B] + p->cnt[sd][R] == 0);
}

int PcMatR(POS *p, int sd) {
  return (p->cnt[sd][R] == 1 && p->cnt[sd][N] + p->cnt[sd][B] + p->cnt[sd][Q] == 0);
}

int PcMatRm(POS *p, int sd) {
  return (p->cnt[sd][R] == 1 && p->cnt[sd][N] + p->cnt[sd][B] == 1 && p->cnt[sd][Q] == 0);
}

int PcMatRR(POS *p, int sd) {
	return (p->cnt[sd][R] == 2 && p->cnt[sd][N] + p->cnt[sd][B] + p->cnt[sd][Q] == 0);
}

int PcMatRRm(POS *p, int sd) {
	return (p->cnt[sd][R] == 2 && p->cnt[sd][N] + p->cnt[sd][B] == 1 && p->cnt[sd][Q] == 0);
}