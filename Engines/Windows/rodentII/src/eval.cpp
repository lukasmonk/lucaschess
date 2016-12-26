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
#include <stdio.h>
#include <math.h>
#include "rodent.h"
#include "eval.h"
#include "param.h"

// parameters for defining game phase [6]

static const int max_phase = 24;
const int phase_value[7] = { 0, 1, 1, 2, 4, 0, 0 };

char *factor_name[] = { "Attack    ", "Mobility  ", "Pst       ", "Pawns     ", "Passers   ", "Tropism   ", "Outposts  ", "Lines     ", "Pressure  ", "Others    "};

sEvalHashEntry EvalTT[EVAL_HASH_SIZE];

void cMask::Init(void) {

  // Init mask for passed pawn detection

  for (int sq = 0; sq < 64; sq++) {
    passed[WC][sq] = BB.FillNorthExcl(SqBb(sq));
    passed[WC][sq] |= BB.ShiftSideways(passed[WC][sq]);
    passed[BC][sq] = BB.FillSouthExcl(SqBb(sq));
    passed[BC][sq] |= BB.ShiftSideways(passed[BC][sq]);
  }

  // Init adjacent mask for isolated pawns detection

  for (int f = 0; f < 8; f++) {
    adjacent[f] = 0ULL;
    if (f > 0) adjacent[f] |= FILE_A_BB << (f - 1);
    if (f < 7) adjacent[f] |= FILE_A_BB << (f + 1);
  }

  // Init supported mask for weak pawns detection

  for (int sq = 0; sq < 64; sq++) {
    supported[WC][sq] = BB.ShiftSideways(SqBb(sq));
    supported[WC][sq] |= BB.FillSouth(supported[WC][sq]);

    supported[BC][sq] = BB.ShiftSideways(SqBb(sq));
    supported[BC][sq] |= BB.FillNorth(supported[BC][sq]);
  }

  // Init king zone

  for (int sq = 0; sq < 64; sq++) {
    king_zone[WC][sq] = king_zone[BC][sq] = BB.KingAttacks(sq);
    king_zone[WC][sq] |= ShiftSouth(king_zone[WC][sq]);
    king_zone[BC][sq] |= ShiftNorth(king_zone[BC][sq]);
  }

};

void SetAsymmetricEval(int sd) {

  int op = Opp(sd);
  Eval.prog_side = sd;

  curr_weights[sd][SD_ATT] = dyn_weights[DF_OWN_ATT];
  curr_weights[op][SD_ATT] = dyn_weights[DF_OPP_ATT];
  curr_weights[sd][SD_MOB] = dyn_weights[DF_OWN_MOB];
  curr_weights[op][SD_MOB] = dyn_weights[DF_OPP_MOB];
}

void ClearEvalHash(void) {

  for (int e = 0; e < EVAL_HASH_SIZE; e++) {
    EvalTT[e].key = 0;
    EvalTT[e].score = 0;
  }
}

void InitWeights(void) {

  // default weights: 100%
  
  for (int fc = 0; fc < N_OF_FACTORS; fc++)
    weights[fc] = 100;

  weights[F_TROPISM] = 20;
  Param.pst_perc = pst_default_perc[Param.pst_style];

  // weights for asymmetric factors

  dyn_weights[DF_OWN_ATT] = 110; 
  dyn_weights[DF_OPP_ATT] = 100;
  dyn_weights[DF_OWN_MOB] = 100;
  dyn_weights[DF_OPP_MOB] = 110;
}

void cParam::Default(void) {

  int r_delta, f_delta;

  // Init distance tables (for evaluating king tropism and unstoppable passers)

  for (int sq1 = 0; sq1 < 64; ++sq1) {
    for (int sq2 = 0; sq2 < 64; ++sq2) {
      r_delta = Abs(Rank(sq1) - Rank(sq2));
      f_delta = Abs(File(sq1) - File(sq2));
      dist[sq1][sq2] = 14 - (r_delta + f_delta);
      chebyshev_dist[sq1][sq2] = Max(r_delta, f_delta);
    }
  }

 // Init parameter values

  elo = 2850;
  fl_weakening = 0;
  pc_value[P] = 100;
  pc_value[N] = 325;
  pc_value[B] = 335;
  pc_value[R] = 500;
  pc_value[Q] = 1000;
  pc_value[K] = 0;
  pc_value[NO_PC] = 0;
  keep_pc[P] = 0;
  keep_pc[N] = 0;
  keep_pc[B] = 0;
  keep_pc[R] = 0;
  keep_pc[Q] = 0;
  keep_pc[K] = 0;
  keep_pc[NO_PC] = 0;
  pst_style = 0;
  mob_style = 0;
  mat_perc = 100;
  pst_perc = 80;
  shield_perc = 120;
  storm_perc = 100;
  bish_pair = 50;
  knight_pair = -10;
  exchange_imbalance = 25;
  np_bonus = 6;
  rp_malus = 3;
  rook_pair_malus = -5;
  doubled_malus_mg = -12;
  doubled_malus_eg = -24;
  isolated_malus_mg = -10;
  isolated_malus_eg = -20;
  isolated_open_malus = -10;
  backward_malus_base = -8;
  backward_malus_mg[FILE_A] = -5;
  backward_malus_mg[FILE_B] = -7;
  backward_malus_mg[FILE_C] = -9;
  backward_malus_mg[FILE_D] = -11;
  backward_malus_mg[FILE_E] = -11;
  backward_malus_mg[FILE_F] = -9;
  backward_malus_mg[FILE_G] = -7;
  backward_malus_mg[FILE_H] = -5;
  backward_malus_eg = -8;
  backward_open_malus = -8;
  minorBehindPawn = 5;
  minorVsQueen = 5;
  bishConfined = -5;
  rookOn7thMg = 16;
  rookOn7thEg = 32;
  twoRooksOn7thMg = 8;
  twoRooksOn7thEg = 16;
  rookOnQueen = 5;
  rookOnOpenMg = 14;
  rookOnOpenEg = 10;
  rookOnBadHalfOpenMg = 6;
  rookOnBadHalfOpenEg = 4;
  rookOnGoodHalfOpenMg = 8;
  rookOnGoodHalfOpenEg = 6;
  queenOn7thMg = 4;
  queenOn7thEg = 8;
  draw_score = 0;
  forwardness = 0;
  book_filter = 20;
  eval_blur = 0;
}

// @DynamicInit() - here we initialize stuff that might be changed 
// when user fiddles with UCI options.

void cParam::DynamicInit(void) {

  Eval.prog_side = NO_CL;
  ResetEngine();

  // Init piece/square values together with material value of the pieces.

  for (int sq = 0; sq < 64; sq++) {
    for (int sd = 0; sd < 2; sd++) {
 
      mg_pst[sd][P][REL_SQ(sq, sd)] = SCALE(pc_value[P], mat_perc) + SCALE(pstPawnMg[pst_style][sq], pst_perc);
      eg_pst[sd][P][REL_SQ(sq, sd)] = SCALE(pc_value[P], mat_perc) + SCALE(pstPawnEg[pst_style][sq], pst_perc);
      mg_pst[sd][N][REL_SQ(sq, sd)] = SCALE(pc_value[N], mat_perc) + SCALE(pstKnightMg[pst_style][sq], pst_perc);
      eg_pst[sd][N][REL_SQ(sq, sd)] = SCALE(pc_value[N], mat_perc) + SCALE(pstKnightEg[pst_style][sq], pst_perc);
      mg_pst[sd][B][REL_SQ(sq, sd)] = SCALE(pc_value[B], mat_perc) + SCALE(pstBishopMg[pst_style][sq], pst_perc);
      eg_pst[sd][B][REL_SQ(sq, sd)] = SCALE(pc_value[B], mat_perc) + SCALE(pstBishopEg[pst_style][sq], pst_perc);
      mg_pst[sd][R][REL_SQ(sq, sd)] = SCALE(pc_value[R], mat_perc) + SCALE(pstRookMg[pst_style][sq], pst_perc);
      eg_pst[sd][R][REL_SQ(sq, sd)] = SCALE(pc_value[R], mat_perc) + SCALE(pstRookEg[pst_style][sq], pst_perc);
      mg_pst[sd][Q][REL_SQ(sq, sd)] = SCALE(pc_value[Q], mat_perc) + SCALE(pstQueenMg[pst_style][sq], pst_perc);
      eg_pst[sd][Q][REL_SQ(sq, sd)] = SCALE(pc_value[Q], mat_perc) + SCALE(pstQueenEg[pst_style][sq], pst_perc);
      mg_pst[sd][K][REL_SQ(sq, sd)] = pstKingMg[pst_style][sq];
      eg_pst[sd][K][REL_SQ(sq, sd)] = pstKingEg[pst_style][sq];

      phalanx[sd][REL_SQ(sq, sd)] = pstPhalanxPawn[sq];
      defended[sd][REL_SQ(sq, sd)] = pstDefendedPawn[sq];

      sp_pst_data[sd][N][REL_SQ(sq, sd)] = pstKnightOutpost[sq];
      sp_pst_data[sd][B][REL_SQ(sq, sd)] = pstBishopOutpost[sq];
    }
  }

  // Init tables for adjusting piece values 
  // according to the number of own pawns

  for (int i = 0; i < 9; i++) {
    np_table[i] = adj[i] * np_bonus;
    rp_table[i] = adj[i] * rp_malus;
  }

  // Init backward pawns table, adding file offset to base value

  for (int i = 0; i < 8; i++) {
    backward_malus_mg[i] = backward_malus_base + file_adj[i];
  }

  // Init imbalance table, so that we can expose option for exchange delta

  for (int i = 0; i < 9; i++) {
    for (int j = 0; j < 9; j++) {

      // insert original values
      imbalance[i][j] = imbalance_data[i][j];

      // insert value defined by the user
      if (imbalance[i][j] == Ex) imbalance[i][j] = exchange_imbalance;
      if (imbalance[i][j] == -Ex) imbalance[i][j] = -exchange_imbalance;
    }
  }

  // Init mobility tables

  if (mob_style == 0) {
    for (int i = 0; i < 9; i++) {
      n_mob_mg[i] = 4 * (i-4);
      n_mob_eg[i] = 4 * (i-4);
    }

    for (int i = 0; i < 14; i++) {
      b_mob_mg[i] = 5 * (i-7);
      b_mob_eg[i] = 5 * (i-7);
    }

    for (int i = 0; i < 15; i++) {
      r_mob_mg[i] = 2 * (i-7);
      r_mob_eg[i] = 4 * (i-7);
    }

    for (int i = 0; i < 28; i++) {
      q_mob_mg[i] = 1 * (i-14);
      q_mob_eg[i] = 2 * (i-14);
    }

  }

  if (mob_style == 1) {
    for (int i = 0; i < 9; i++) {
      n_mob_mg[i] = n_mob_mg_decreasing[i];
      n_mob_eg[i] = n_mob_eg_decreasing[i];
    }

    for (int i = 0; i < 14; i++) {
      b_mob_mg[i] = b_mob_mg_decreasing[i];
      b_mob_eg[i] = b_mob_eg_decreasing[i];
    }

    for (int i = 0; i < 15; i++) {
      r_mob_mg[i] = r_mob_mg_decreasing[i];
      r_mob_eg[i] = r_mob_eg_decreasing[i];
    }

    for (int i = 0; i < 28; i++) {
      q_mob_mg[i] = q_mob_mg_decreasing[i];
      q_mob_eg[i] = q_mob_eg_decreasing[i];
    }

  }

  // Init king attack table

  for (int t = 0, i = 1; i < 512; ++i) {
    t = Min(maxAttScore, Min(int(attCurveMult * i * i), t + maxAttStep));
    danger[i] = (t * 100) / 256; // rescale to centipawns
    // TODO: init separately for Black and White in SetAsymmetricEval() to gain some speed
  }
}

void cEval::ScoreMaterial(POS * p, eData *e, int sd) {

  int op = Opp(sd);

  // Piece configurations

  int tmp = Param.np_table[p->cnt[sd][P]] * p->cnt[sd][N]   // knights lose value as pawns disappear
          - Param.rp_table[p->cnt[sd][P]] * p->cnt[sd][R];  // rooks gain value as pawns disappear

  if (p->cnt[sd][N] > 1) tmp += Param.knight_pair;
  if (p->cnt[sd][R] > 1) tmp += Param.rook_pair_malus;

  if (p->cnt[sd][B] > 1)                                    // Bishop pair
    Add(e, sd, F_OTHERS, SCALE(Param.bish_pair, Param.mat_perc), SCALE((Param.bish_pair+10), Param.mat_perc));

  // "elephantiasis correction" for queen, idea by H.G.Mueller (nb. rookVsQueen doesn't help)

  if (p->cnt[sd][Q])
    tmp -= Param.minorVsQueen * (p->cnt[op][N] + p->cnt[op][B]);

  Add(e, sd, F_OTHERS, SCALE(tmp, Param.mat_perc));
}

void cEval::ScorePieces(POS *p, eData *e, int sd) {

  U64 bbPieces, bbMob, bbAtt, bbFile, bbContact;
  int op, sq, cnt, tmp, ksq, att = 0, wood = 0;
  int own_pawn_cnt, opp_pawn_cnt;
  int r_on_7th = 0;
  int fwd_weight = 0;
  int fwd_cnt = 0;

  // Is color OK?

  assert(sd == WC || sd == BC);

  // Init variables

  op = Opp(sd);
  ksq = KingSq(p, op);
  U64 bbExcluded = p->Pawns(sd) /*| p->Kings(sd)*/;

  // Init enemy king zone for attack evaluation. We mark squares where the king
  // can move plus two or three more squares facing enemy position.

  U64 bbZone = Mask.king_zone[sd][ksq];

  // Init bitboards to detect check threats
  
  U64 bbKnightChk = BB.KnightAttacks(ksq);
  U64 bbStr8Chk = BB.RookAttacks(OccBb(p), ksq);
  U64 bbDiagChk = BB.BishAttacks(OccBb(p), ksq);
  U64 bbQueenChk = bbStr8Chk | bbDiagChk;
  
  // Knight

  bbPieces = p->Knights(sd);
  while (bbPieces) {
    sq = BB.PopFirstBit(&bbPieces);

    // Knight tropism to enemy king

    Add(e, sd, F_TROPISM, tropism_mg[N] * Param.dist[sq][ksq], tropism_eg[N] * Param.dist[sq][ksq]);

	// Knight forwardness

    if (SqBb(sq) & bbAwayZone[sd]) {
      fwd_weight += 1;
      fwd_cnt += 1;
	}
    
    // Knight mobility

    bbMob = BB.KnightAttacks(sq) & ~p->cl_bb[sd];  // knight is tricky, 
    cnt = BB.PopCnt(bbMob &~e->bbPawnTakes[op]);   // better to have it mobile than defending stuff
    
    Add(e, sd, F_MOB, Param.n_mob_mg[cnt], Param.n_mob_eg[cnt]);  // mobility bonus

    if ((bbMob &~e->bbPawnTakes[op]) & bbKnightChk) 
       att += chk_threat[N];                          // check threat bonus

    e->bbAllAttacks[sd] |= bbMob;
    e->bbEvAttacks[sd]  |= bbMob;

    // Knight attacks on enemy king zone

    bbAtt = BB.KnightAttacks(sq);
    if (bbAtt & bbZone) {
      wood++;
      att += king_att[N] * BB.PopCnt(bbAtt & bbZone);
    }

    // Knight outpost

    ScoreOutpost(p, e, sd, N, sq);

  } // end of knight eval

  // Bishop

  bbPieces = p->Bishops(sd);
  while (bbPieces) {
    sq = BB.PopFirstBit(&bbPieces);

    // Bishop tropism to enemy king

    Add(e, sd, F_TROPISM, tropism_mg[B] * Param.dist[sq][ksq], tropism_eg[B] * Param.dist[sq][ksq]);

    // Bishop forwardness

    if (SqBb(sq) & bbAwayZone[sd]) {
      fwd_weight += 1;
      fwd_cnt += 1;
    }

    // Bishop mobility

    bbMob = BB.BishAttacks(OccBb(p), sq);

    if (!(bbMob & bbAwayZone[sd]))               // penalty for bishops unable to reach enemy half of the board
       Add(e, sd, F_MOB, Param.bishConfined);    // (idea from Andscacs)

    cnt = BB.PopCnt(bbMob &~e->bbPawnTakes[op] &~bbExcluded);
    
    Add(e, sd, F_MOB, Param.b_mob_mg[cnt], Param.b_mob_eg[cnt]);   // mobility bonus

    if ((bbMob &~e->bbPawnTakes[op]) & ~p->cl_bb[sd] & bbDiagChk)
      att += chk_threat[B];                            // check threat bonus

    e->bbAllAttacks[sd] |= bbMob;
    e->bbEvAttacks[sd]  |= bbMob;

    // Bishop attacks on enemy king zone (including attacks through a queen)

    bbAtt = BB.BishAttacks(OccBb(p) ^ p->Queens(sd) , sq);
    if (bbAtt & bbZone) {
      wood++;
      att += king_att[B] * BB.PopCnt(bbAtt & bbZone);
    }

    // Bishop outpost

    ScoreOutpost(p, e, sd, B, sq);

    // Bishops side by side

    if (ShiftNorth(SqBb(sq)) & p->Bishops(sd) )
      Add(e, sd, F_OTHERS, 4);
    if (ShiftEast(SqBb(sq)) & p->Bishops(sd))
      Add(e, sd, F_OTHERS, 4);

    // Pawns on the same square color as our bishop
  
    if (bbWhiteSq & SqBb(sq)) {
      own_pawn_cnt = BB.PopCnt(bbWhiteSq & p->Pawns(sd)) - 4;
      opp_pawn_cnt = BB.PopCnt(bbWhiteSq & p->Pawns(op)) - 4;
    } else {
      own_pawn_cnt = BB.PopCnt(bbBlackSq & p->Pawns(sd)) - 4;
      opp_pawn_cnt = BB.PopCnt(bbBlackSq & p->Pawns(op)) - 4;
    }

    Add(e, sd, F_OTHERS, -3 * own_pawn_cnt - opp_pawn_cnt);

  } // end of bishop eval

  // Rook

  bbPieces = p->Rooks(sd);
  while (bbPieces) {
    sq = BB.PopFirstBit(&bbPieces);

    // Rook tropism to enemy king

    Add(e, sd, F_TROPISM, tropism_mg[R] * Param.dist[sq][ksq], tropism_eg[R] * Param.dist[sq][ksq]);

	// Rook forwardness

	if (SqBb(sq) & bbAwayZone[sd]) {
      fwd_weight += 2;
	  fwd_cnt += 1;
	}
  
    // Rook mobility

    bbMob = BB.RookAttacks(OccBb(p), sq);
    cnt = BB.PopCnt(bbMob &~bbExcluded);
    Add(e, sd, F_MOB, Param.r_mob_mg[cnt], Param.r_mob_eg[cnt]);    // mobility bonus
    if (((bbMob &~e->bbPawnTakes[op]) & ~p->cl_bb[sd] & bbStr8Chk)  // check threat bonus
    && p->cnt[sd][Q]) {
      att += chk_threat[R]; 

      bbContact = (bbMob & BB.KingAttacks(ksq)) & bbStr8Chk;

      while (bbContact) {
        int contactSq = BB.PopFirstBit(&bbContact);

        // rook exchanges are accepted as contact checks

        if (Swap(p, sq, contactSq) >= 0) {
          att += r_contact_check;
          break;
        }
      }
    }

    e->bbAllAttacks[sd] |= bbMob;
    e->bbEvAttacks[sd]  |= bbMob;

    // Rook attacks on enemy king zone (also through a rook or through a queen)

    bbAtt = BB.RookAttacks(OccBb(p) ^ p->StraightMovers(sd), sq);
    if (bbAtt & bbZone) {
      wood++;
      att += king_att[R] * BB.PopCnt(bbAtt & bbZone);
    }

    // Get rook file

    bbFile = BB.FillNorthSq(sq) | BB.FillSouthSq(sq); // better this way than using front span

    // Queen on rook's file (which might be closed)

    if (bbFile & p->Queens(op)) Add(e, sd, F_LINES, Param.rookOnQueen);

    // Rook on (half) open file

    if (!(bbFile & p->Pawns(sd))) {
      if (!(bbFile & p->Pawns(op))) {
        Add(e, sd, F_LINES, Param.rookOnOpenMg, Param.rookOnOpenEg);
        //if (BB.GetFrontSpan(SqBb(sq), sd) & p->Rooks(sd)) Add(e, sd, F_LINES, 4, 2); // equal
      }
      else {
        // score differs depending on whether half-open file is blocked by defended enemy pawn
        if ((bbFile & p->Pawns(op)) & e->bbPawnTakes[op])
          Add(e, sd, F_LINES, Param.rookOnBadHalfOpenMg, Param.rookOnBadHalfOpenEg);
        else {
          Add(e, sd, F_LINES, Param.rookOnGoodHalfOpenMg, Param.rookOnGoodHalfOpenEg);
        }
      }
    }

    // Rook on the 7th rank attacking pawns or cutting off enemy king

    if (SqBb(sq) & bbRelRank[sd][RANK_7]) {
      if (p->Pawns(op) & bbRelRank[sd][RANK_7]
      ||  p->Kings(op) & bbRelRank[sd][RANK_8]) {
        Add(e, sd, F_LINES, Param.rookOn7thMg, Param.rookOn7thEg);
        r_on_7th++;
      }
    }

  } // end of rook eval

  // Queen

  bbPieces = p->Queens(sd);
  while (bbPieces) {
    sq = BB.PopFirstBit(&bbPieces);

    // Queen tropism to enemy king

    Add(e, sd, F_TROPISM, tropism_mg[Q] * Param.dist[sq][ksq], tropism_eg[Q] * Param.dist[sq][ksq]);

	// Queen forwardness

	if (SqBb(sq) & bbAwayZone[sd]) {
       fwd_weight += 1;
	   fwd_cnt += 1;
	}

    // Queen mobility

    bbMob = BB.QueenAttacks(OccBb(p), sq);
    cnt = BB.PopCnt(bbMob &~bbExcluded);
    Add(e, sd, F_MOB, Param.q_mob_mg[cnt], Param.q_mob_eg[cnt]);  // mobility bonus

    if ((bbMob &~e->bbPawnTakes[op]) & ~p->cl_bb[sd] & bbQueenChk) {  // check threat bonus
      att += chk_threat[Q];

    // Queen contact checks

    bbContact = bbMob & BB.KingAttacks(ksq);
    while (bbContact) {
      int contactSq = BB.PopFirstBit(&bbContact);

        // queen exchanges are accepted as contact checks

        if (Swap(p, sq, contactSq) >= 0) {
          att += q_contact_check;
          break;
        }
      }
    }

    e->bbAllAttacks[sd] |= bbMob;

    // Queen attacks on enemy king zone
   
    bbAtt  = BB.BishAttacks(OccBb(p) ^ p->DiagMovers(sd), sq);
    bbAtt |= BB.RookAttacks(OccBb(p) ^ p->StraightMovers(sd), sq);
    if (bbAtt & bbZone) {
      wood++;
      att += king_att[Q] * BB.PopCnt(bbAtt & bbZone);
    }

    // Queen on 7th rank

    if (SqBb(sq) & bbRelRank[sd][RANK_7]) {
      if (p->Pawns(op) & bbRelRank[sd][RANK_7]
      || p->Kings(op) & bbRelRank[sd][RANK_8]) {
        Add(e, sd, F_LINES, Param.queenOn7thMg, Param.queenOn7thEg);
      }
    }

  } // end of queen eval

  // Score terms using information gathered during piece eval

  if (r_on_7th == 2)                 // two rooks on 7th rank
    Add(e, sd, F_LINES, Param.twoRooksOn7thMg, Param.twoRooksOn7thEg);

  // forwardness (from Toga II 3.0)
  Add(e, sd, (fwd_bonus[fwd_cnt] * fwd_weight * Param.forwardness) / 100, 0); 

  // Score king attacks if own queen is present and there are at least 2 attackers

  if (wood > 1 && p->cnt[sd][Q]) {
    if (att > 399) att = 399;
    Add(e, sd, F_ATT, Param.danger[att]);
  }

}

void cEval::ScoreOutpost(POS * p, eData * e, int sd, int pc, int sq) {

  int mul = 0;
  int tmp = Param.sp_pst_data[sd][pc][sq];
  if (tmp) {
    if (SqBb(sq) & ~e->bbPawnCanTake[Opp(sd)]) mul += 2;  // in the hole of enemy pawn structure
    if (SqBb(sq) & e->bbPawnTakes[sd]) mul += 1;          // defended by own pawn
    if (SqBb(sq) & e->bbTwoPawnsTake[sd]) mul += 1;       // defended by two pawns

    tmp *= mul;
    tmp /= 2;

    Add(e, sd, F_OUTPOST, tmp);
  }

  // Pawn in front of a minor

  if (SqBb(sq) & bbHomeZone[sd]) {
    U64 bbStop = BB.ShiftFwd(SqBb(sq), sd);
    if (bbStop & PcBb(p, sd, P))
      Add(e, sd, F_OUTPOST, Param.minorBehindPawn);
  }
}

void cEval::ScoreHanging(POS *p, eData *e, int sd) {

  int pc, sq, sc;
  int op = Opp(sd);
  U64 bbHanging = p->cl_bb[op]    & ~e->bbPawnTakes[op];
  U64 bbThreatened = p->cl_bb[op] & e->bbPawnTakes[sd];
  bbHanging |= bbThreatened;      // piece attacked by our pawn isn't well defended
  bbHanging &= e->bbAllAttacks[sd];  // hanging piece has to be attacked
  bbHanging &= ~p->Pawns(op);     // currently we don't evaluate threats against pawns

  U64 bbDefended = p->cl_bb[op] & e->bbAllAttacks[op];
  bbDefended &= e->bbEvAttacks[sd];  // N, B, R attacks (pieces attacked by pawns are scored as hanging)
  bbDefended &= ~e->bbPawnTakes[sd]; // no defense against pawn attack
  bbDefended &= ~p->Pawns(op);    // currently we don't evaluate threats against pawns

  // hanging pieces (attacked and undefended)

  while (bbHanging) {
    sq = BB.PopFirstBit(&bbHanging);
    pc = TpOnSq(p, sq);
    sc = tp_value[pc] / 64;
    Add(e, sd, F_PRESSURE, 10 + sc, 18 + sc);
  }

  // defended pieces under attack

  while (bbDefended) {
    sq = BB.PopFirstBit(&bbDefended);
    pc = TpOnSq(p, sq);
    sc = tp_value[pc] / 96;
    Add(e, sd, F_PRESSURE, 5 + sc, 9 + sc);
  }
}

void cEval::ScorePassers(POS * p, eData *e, int sd) 
{
  U64 bbPieces = p->Pawns(sd);
  int sq, mul, mg_tmp, eg_tmp;
  int op = Opp(sd);
  U64 bbOwnPawns = p->Pawns(sd);
  U64 bbStop;

  while (bbPieces) {
    sq = BB.PopFirstBit(&bbPieces);

    // Passed pawn

    if (!(Mask.passed[sd][sq] & p->Pawns(op))) {

      bbStop = BB.ShiftFwd(SqBb(sq), sd);
      mg_tmp = passed_bonus_mg[sd][Rank(sq)];
      eg_tmp = passed_bonus_eg[sd][Rank(sq)] 
             - ((passed_bonus_eg[sd][Rank(sq)] * Param.dist[sq][p->king_sq[op]]) / 30);

      mul = 100;

      // blocked passers score less

      if (bbStop & OccBb(p)) mul -= 20; // TODO: only with a blocker of opp color

      // our control of stop square
    
      else if ( (bbStop & e->bbAllAttacks[sd]) 
      &&   (bbStop & ~e->bbAllAttacks[op]) ) mul += 10;
  
      // add final score
  
      Add(e, sd, F_PASSERS, (mg_tmp * mul) / 100, (eg_tmp * mul) / 100);
    }
  }
}

void cEval::ScoreUnstoppable(eData *e, POS * p) {

  U64 bbPieces, bbSpan, bbProm;
  int w_dist = 8;
  int b_dist = 8;
  int sq, ksq, psq, tempo, prom_dist;

  // White unstoppable passers

  if (p->cnt[BC][N] + p->cnt[BC][B] + p->cnt[BC][R] + p->cnt[BC][Q] == 0) {
    ksq = KingSq(p, BC);
    if (p->side == BC) tempo = 1; else tempo = 0;
    bbPieces = p->Pawns(WC);
    while (bbPieces) {
      sq = BB.PopFirstBit(&bbPieces);
      if (!(Mask.passed[WC][sq] & p->Pawns(BC))) {
        bbSpan = BB.GetFrontSpan(SqBb(sq), WC);
        psq = ((WC - 1) & 56) + (sq & 7);
        prom_dist = Min(5, Param.chebyshev_dist[sq][psq]);

        if (prom_dist < (Param.chebyshev_dist[ksq][psq] - tempo)) {
          if (bbSpan & p->Kings(WC)) prom_dist++;
          w_dist = Min(w_dist, prom_dist);
        }
      }
    }
  }

  // Black unstoppable passers

  if (p->cnt[WC][N] + p->cnt[WC][B] + p->cnt[WC][R] + p->cnt[WC][Q] == 0) {
    ksq = KingSq(p, WC);
    if (p->side == WC) tempo = 1; else tempo = 0;
    bbPieces = p->Pawns(BC);
    while (bbPieces) {
      sq = BB.PopFirstBit(&bbPieces);
      if (!(Mask.passed[BC][sq] & p->Pawns(WC))) {
        bbSpan = BB.GetFrontSpan(SqBb(sq), BC);
        if (bbSpan & p->Kings(WC)) tempo -= 1;
        psq = ((BC - 1) & 56) + (sq & 7);
        prom_dist = Min(5, Param.chebyshev_dist[sq][psq]);

        if (prom_dist < (Param.chebyshev_dist[ksq][psq] - tempo)) {
          if (bbSpan & p->Kings(BC)) prom_dist++;
          b_dist = Min(b_dist, prom_dist);
        }
      }
    }
  }

  if (w_dist < b_dist-1) Add(e, WC, F_PASSERS, 0, 500);
  if (b_dist < w_dist-1) Add(e, BC, F_PASSERS, 0, 500);
}

int cEval::Return(POS *p, eData * e, int use_hash) {

  assert(prog_side == WC || prog_side == BC);

  // Try to retrieve score from eval hashtable

  int addr = p->hash_key % EVAL_HASH_SIZE;

  if (EvalTT[addr].key == p->hash_key && use_hash) {
    int hashScore = EvalTT[addr].score;
    return p->side == WC ? hashScore : -hashScore;
  }

  // Clear eval

  int score = 0;
  int mg_score = 0;
  int eg_score = 0;

  for (int sd = 0; sd < 2; sd++) {
    for (int fc = 0; fc < N_OF_FACTORS; fc++) {
      e->mg[sd][fc] = 0;
      e->eg[sd][fc] = 0;
    }
  }

  // Init eval with incrementally updated stuff

  e->mg[WC][F_PST] = p->mg_sc[WC];
  e->mg[BC][F_PST] = p->mg_sc[BC];
  e->eg[WC][F_PST] = p->eg_sc[WC];
  e->eg[BC][F_PST] = p->eg_sc[BC];

  // Calculate variables used during evaluation

  e->bbPawnTakes[WC] = BB.GetWPControl(p->Pawns(WC));
  e->bbPawnTakes[BC] = BB.GetBPControl(p->Pawns(BC));
  e->bbTwoPawnsTake[WC] = BB.GetDoubleWPControl(p->Pawns(WC));
  e->bbTwoPawnsTake[BC] = BB.GetDoubleBPControl(p->Pawns(BC));
  e->bbAllAttacks[WC] = e->bbPawnTakes[WC] | BB.KingAttacks(p->king_sq[WC]);
  e->bbAllAttacks[BC] = e->bbPawnTakes[BC] | BB.KingAttacks(p->king_sq[BC]);
  e->bbEvAttacks[WC] = e->bbEvAttacks[BC] = 0ULL;
  e->bbPawnCanTake[WC] = BB.FillNorth(e->bbPawnTakes[WC]);
  e->bbPawnCanTake[BC] = BB.FillSouth(e->bbPawnTakes[BC]);

  // Tempo bonus

  Add(e, p->side, F_OTHERS, 10, 5);

  // Evaluate pieces and pawns

  ScoreMaterial(p, e, WC);
  ScoreMaterial(p, e, BC);
  ScorePieces(p, e, WC);
  ScorePieces(p, e, BC);
  FullPawnEval(p, e, use_hash);
  ScoreHanging(p, e, WC);
  ScoreHanging(p, e, BC);
  ScorePatterns(p, e);
  ScorePassers(p, e, WC);
  ScorePassers(p, e, BC);
  ScoreUnstoppable(e, p);

  // Add asymmetric bonus for keeping certain type of pieces

  e->mg[prog_side][F_OTHERS] += Param.keep_pc[Q] * p->cnt[prog_side][Q];
  e->mg[prog_side][F_OTHERS] += Param.keep_pc[R] * p->cnt[prog_side][R];
  e->mg[prog_side][F_OTHERS] += Param.keep_pc[B] * p->cnt[prog_side][B];
  e->mg[prog_side][F_OTHERS] += Param.keep_pc[N] * p->cnt[prog_side][N];
  e->mg[prog_side][F_OTHERS] += Param.keep_pc[P] * p->cnt[prog_side][P];

  // Sum all the symmetric eval factors
  // (we start from 2 so that we won't touch king attacks 
  // and mobility, both of which are asymmetric)

  for (int fc = 2; fc < N_OF_FACTORS; fc++) {
    mg_score += (e->mg[WC][fc] - e->mg[BC][fc]) * weights[fc] / 100;
    eg_score += (e->eg[WC][fc] - e->eg[BC][fc]) * weights[fc] / 100;
  }

  // Add asymmetric eval factors

  mg_score += e->mg[WC][F_ATT] * curr_weights[WC][SD_ATT] / 100;
  mg_score -= e->mg[BC][F_ATT] * curr_weights[BC][SD_ATT] / 100;
  eg_score += e->eg[WC][F_ATT] * curr_weights[WC][SD_ATT] / 100;
  eg_score -= e->eg[BC][F_ATT] * curr_weights[BC][SD_ATT] / 100;

  mg_score += e->mg[WC][F_MOB] * curr_weights[WC][SD_MOB] / 100;
  mg_score -= e->mg[BC][F_MOB] * curr_weights[BC][SD_MOB] / 100;
  eg_score += e->eg[WC][F_MOB] * curr_weights[WC][SD_MOB] / 100;
  eg_score -= e->eg[BC][F_MOB] * curr_weights[BC][SD_MOB] / 100;

  // Merge mg/eg scores

  int mg_phase = Min(max_phase, p->phase);
  int eg_phase = max_phase - mg_phase;

  score += (((mg_score * mg_phase) + (eg_score * eg_phase)) / max_phase);

  // Material imbalance table

  int minorBalance = p->cnt[WC][N] - p->cnt[BC][N] + p->cnt[WC][B] - p->cnt[BC][B];
  int majorBalance = p->cnt[WC][R] - p->cnt[BC][R] + 2 * p->cnt[WC][Q] - 2 * p->cnt[BC][Q];

  int x = Max(majorBalance + 4, 0);
  if (x > 8) x = 8;

  int y = Max(minorBalance + 4, 0);
  if (y > 8) y = 8;

  score += SCALE(Param.imbalance[x][y], Param.mat_perc);

  score += CheckmateHelper(p);

  // Scale down drawish endgames

  int draw_factor = 64;
  if (score > 0) draw_factor = GetDrawFactor(p, WC);
  else           draw_factor = GetDrawFactor(p, BC);
  score *= draw_factor;
  score /= 64;

  // Make sure eval doesn't exceed mate score

  if (score < -MAX_EVAL)
    score = -MAX_EVAL;
  else if (score > MAX_EVAL)
    score = MAX_EVAL;

  // Weakening: add pseudo-random value to eval score

  if (Param.eval_blur) {
    int randomMod = (Param.eval_blur / 2) - (p->hash_key % Param.eval_blur);
    score += randomMod;
  }

  // Save eval score in the evaluation hash table

  EvalTT[addr].key = p->hash_key;
  EvalTT[addr].score = score;

  // Return score relative to the side to move

  return p->side == WC ? score : -score;
}

void cEval::Add(eData *e, int sd, int factor, int mg_bonus, int eg_bonus) {

  e->mg[sd][factor] += mg_bonus;
  e->eg[sd][factor] += eg_bonus;
}

void cEval::Add(eData *e, int sd, int factor, int bonus) {

  e->mg[sd][factor] += bonus;
  e->eg[sd][factor] += bonus;
}

void cEval::Print(POS * p) {

  eData e;
  int mg_score, eg_score, total;
  int mg_phase = Min(max_phase, p->phase);
  int eg_phase = max_phase - mg_phase;

  // BUG: eval asymmetry not shown
  // TODO: fix me!

  printf("Total score: %d\n", Return(p, &e, 0));
  printf("-----------------------------------------------------------------\n");
  printf("Factor     | Val (perc) |   Mg (  WC,   BC) |   Eg (  WC,   BC) |\n");
  printf("-----------------------------------------------------------------\n");
  for (int fc = 0; fc < N_OF_FACTORS; fc++) {
  mg_score = ((e.mg[WC][fc] - e.mg[BC][fc]) * weights[fc]) / 100;
  eg_score = ((e.eg[WC][fc] - e.eg[BC][fc]) * weights[fc]) / 100;
  total = (((mg_score * mg_phase) + (eg_score * eg_phase)) / max_phase);

    printf(factor_name[fc]);
    printf(" | %4d (%3d) | %4d (%4d, %4d) | %4d (%4d, %4d) |\n", total, weights[fc], mg_score, e.mg[WC][fc], e.mg[BC][fc], eg_score, e.eg[WC][fc], e.eg[BC][fc]);
  }
  printf("-----------------------------------------------------------------\n");
}