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

#include <assert.h>
#include "rodent.h"
#include "eval.h"

static const U64 bbQSCastle[2] = { SqBb(A1) | SqBb(B1) | SqBb(C1) | SqBb(A2) | SqBb(B2) | SqBb(C2),
                                   SqBb(A8) | SqBb(B8) | SqBb(C8) | SqBb(A7) | SqBb(B7) | SqBb(C7)
};
static const U64 bbKSCastle[2] = { SqBb(F1) | SqBb(G1) | SqBb(H1) | SqBb(F2) | SqBb(G2) | SqBb(H2),
                                   SqBb(F8) | SqBb(G8) | SqBb(H8) | SqBb(F7) | SqBb(G7) | SqBb(H7)
};

static const U64 bbCentralFile = FILE_C_BB | FILE_D_BB | FILE_E_BB | FILE_F_BB;

#define SQ(sq) RelSqBb(sq,sd)
#define opPawns p->Pawns(op)
#define sdPawns p->Pawns(sd)

#define OWN_PAWN(sq) (p->Pawns(sd) & RelSqBb(sq,sd))
#define OPP_PAWN(sq) (p->Pawns(op) & RelSqBb(sq,sd))
#define CONTAINS(bb, s1, s2) (bb & SQ(s1)) && (bb & SQ(s2))

static const int bigChainScore = 18;
static const int smallChainScore = 13;

sPawnHashEntry PawnTT[EVAL_HASH_SIZE];

void ClearPawnHash(void) {

  for (int e = 0; e < PAWN_HASH_SIZE; e++) {
    PawnTT[e].key = 0;
    PawnTT[e].mg_pawns = 0;
    PawnTT[e].eg_pawns = 0;
  }
}

void cEval::FullPawnEval(POS * p, int use_hash) {

  // Try to retrieve score from pawn hashtable

  int addr = p->pawn_key % PAWN_HASH_SIZE;

  if (PawnTT[addr].key == p->pawn_key && use_hash) {
    mg[WC][F_PAWNS]   = PawnTT[addr].mg_pawns;
    eg[WC][F_PAWNS]   = PawnTT[addr].eg_pawns;
    return;
  }

  // Single pawn eval

  ScorePawns(p, WC);
  ScorePawns(p, BC);

  // King's pawn shield and pawn storm on enemy king,
  // including pawn chains

  ScoreKing(p, WC);
  ScoreKing(p, BC);

  // Save stuff in pawn hashtable

  PawnTT[addr].key = p->pawn_key;
  PawnTT[addr].mg_pawns = mg[WC][F_PAWNS] - mg[BC][F_PAWNS];
  PawnTT[addr].eg_pawns = eg[WC][F_PAWNS] - eg[BC][F_PAWNS];
}

void cEval::ScorePawns(POS *p, int sd) {

  U64 bbPieces, bbSpan, fl_phalanx;
  int sq, fl_unopposed, fl_weak, fl_defended; 
  int op = Opp(sd);
  U64 bbOwnPawns = p->Pawns(sd);
  U64 bbOppPawns = p->Pawns(op);

  // Is color OK?

  assert(sd == WC || sd == BC);

  // Loop through the pawns, evaluating each one

  bbPieces = bbOwnPawns;
  while (bbPieces) {
    sq = BB.PopFirstBit(&bbPieces);

    // Get some information about the pawn we are evaluating

    bbSpan = BB.GetFrontSpan(SqBb(sq), sd);
    fl_defended  = ((SqBb(sq) & bbPawnTakes[sd]) != 0);
    fl_unopposed = ((bbSpan & bbOppPawns) == 0);
    fl_weak      = ((Mask.supported[sd][sq] & bbOwnPawns) == 0);
    fl_phalanx   = (BB.ShiftSideways(SqBb(sq)) & bbOwnPawns);

    // Candidate passer

    if (fl_unopposed) {
      if (fl_phalanx) {
      if (BB.PopCnt((Mask.passed[sd][sq] & bbOppPawns)) == 1)
        Add(sd, F_PAWNS, passed_bonus_mg[sd][Rank(sq)] / 3, passed_bonus_eg[sd][Rank(sq)] / 3);
      }
    }

    // Doubled pawn

    if (bbSpan & bbOwnPawns)
      Add(sd, F_PAWNS, -12, -24);

    // Supported pawn

    if (fl_phalanx)       Add(sd, F_PAWNS, Param.phalanx_data[sd][sq] , 2);
    else if (fl_defended) Add(sd, F_PAWNS, Param.defended_data[sd][sq], 1);

    // Weak pawn (two flavours)

    if (fl_weak) {
      if (!(Mask.adjacent[File(sq)] & bbOwnPawns)) 
        Add(sd, F_PAWNS, -10 - 10 * fl_unopposed, -20);                     // isolated pawn
      else 
        Add(sd, F_PAWNS, -8 - file_bonus[File(sq)] - 8 * fl_unopposed, -8); // backward pawn
    }
  }
}

void cEval::ScoreKing(POS *p, int sd) {

  const int startSq[2] = { E1, E8 };
  const int qCastle[2] = { B1, B8 };
  const int kCastle[2] = { G1, G8 };

  U64 bbKingFile, bbNextFile;
  int result = 0;
  int sq = KingSq(p, sd);

  // Normalize king square for pawn shield evaluation,
  // to discourage shuffling the king between g1 and h1.

  if (SqBb(sq) & bbKSCastle[sd]) sq = kCastle[sd];
  if (SqBb(sq) & bbQSCastle[sd]) sq = qCastle[sd];

  // Evaluate shielding and storming pawns on each file.

  bbKingFile = BB.FillNorthSq(sq) | BB.FillSouthSq(sq);
  result += ScoreKingFile(p, sd, bbKingFile);

  bbNextFile = ShiftEast(bbKingFile);
  if (bbNextFile) result += ScoreKingFile(p, sd, bbNextFile);

  bbNextFile = ShiftWest(bbKingFile);
  if (bbNextFile) result += ScoreKingFile(p, sd, bbNextFile);

  mg[sd][F_PAWNS] += result;
  mg[sd][F_PAWNS] += ScoreChains(p, sd);
}

int cEval::ScoreKingFile(POS * p, int sd, U64 bbFile) {

  int shelter = ScoreFileShelter(bbFile & p->Pawns(sd), sd);
  int storm = ScoreFileStorm(bbFile & p->Pawns(Opp(sd)), sd);
  if (bbFile & bbCentralFile) return (shelter / 2) + storm;
  else return shelter + storm;
}

int cEval::ScoreFileShelter(U64 bbOwnPawns, int sd) {

  if (!bbOwnPawns) return -36;
  if (bbOwnPawns & bbRelRank[sd][RANK_2]) return    2;
  if (bbOwnPawns & bbRelRank[sd][RANK_3]) return  -11;
  if (bbOwnPawns & bbRelRank[sd][RANK_4]) return  -20;
  if (bbOwnPawns & bbRelRank[sd][RANK_5]) return  -27;
  if (bbOwnPawns & bbRelRank[sd][RANK_6]) return  -32;
  if (bbOwnPawns & bbRelRank[sd][RANK_7]) return  -35;
  return 0;
}

int cEval::ScoreFileStorm(U64 bbOppPawns, int sd) {

  if (!bbOppPawns) return -16;
  if (bbOppPawns & bbRelRank[sd][RANK_3]) return -32;
  if (bbOppPawns & bbRelRank[sd][RANK_4]) return -16;
  if (bbOppPawns & bbRelRank[sd][RANK_5]) return -8;
  return 0;
}

int cEval::ScoreChains(POS *p, int sd)
{
  int mgResult = 0;
  int sq = p->king_sq[sd];
  int op = Opp(sd);

  // basic pointy chain

  if (SqBb(sq) & bbKSCastle[sd]) {

    if (OPP_PAWN(E4)) {
      if (CONTAINS(opPawns, D5, C6)) { // c6-d5-e4 triad
        mgResult -= (CONTAINS(sdPawns, D4, E3)) ? bigChainScore : smallChainScore;
      }

      if (CONTAINS(opPawns, D5, F3)) { // d5-e4-f3 triad
        mgResult -= (OWN_PAWN(E3)) ? bigChainScore : smallChainScore;
      }
    }

    if (OPP_PAWN(E5)) {
      if (CONTAINS(opPawns, F4, D6)) { // d6-e5-f4 triad
        // storm of a "g" pawn in the King's Indian
      if (OPP_PAWN(G5)) {
            mgResult -= 4; 
            if (OPP_PAWN(H4)) return 0; // this is not how you handle pawn chains
      }
        if (OPP_PAWN(G4)) mgResult -= 12;

        mgResult -= (CONTAINS(sdPawns, E4, D5)) ? bigChainScore : smallChainScore;
      }

      if (CONTAINS(opPawns, G3, F4)) { // e5-f4-g3 triad
        mgResult -= (OWN_PAWN(F3)) ? bigChainScore : smallChainScore;
      }
    }
  }
  
  if (SqBb(sq) & bbQSCastle[sd]) {

    // basic pointy chain

    if (OPP_PAWN(D4)) {
      if (CONTAINS(opPawns, E5, F6)) {
        mgResult -= (CONTAINS(sdPawns, E4, D3)) ? bigChainScore : smallChainScore;
      }
      
      if (CONTAINS(opPawns, F5, C3)) {
        mgResult -= (SQ(D3) & sdPawns) ? bigChainScore : smallChainScore;
      }
    }

    if (OPP_PAWN(D5)) {
      if (CONTAINS(opPawns, C4, E6)) {
        // storm of a "b" pawn
        if (OPP_PAWN(B5)) {
          mgResult -= 4;
          if (OPP_PAWN(A4)) return 0; // this is not how you handle pawn chains
        }
        if (OPP_PAWN(B4)) mgResult -= 12;

        mgResult -= (CONTAINS(sdPawns, E4, D5)) ? bigChainScore : smallChainScore;
      }

      if (CONTAINS(opPawns, B3, C4)) {
        mgResult -= (OWN_PAWN(C3)) ? bigChainScore : smallChainScore;
      }
    }
  }

  return mgResult;
}