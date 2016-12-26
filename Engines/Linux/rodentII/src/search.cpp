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

#include <stdio.h>
#include <string.h>
#include <math.h>
#include <assert.h>
#include "rodent.h"
#include "param.h"
#include "timer.h"
#include "book.h"

#define PV_NODE   0
#define CUT_NODE  1
#define ALL_NODE -1
#define NEW_NODE(type)     (-(type))

double lmr_size[2][MAX_PLY][MAX_MOVES];
int lmp_limit[6] = { 0, 4, 8, 12, 36, 48 };
int fut_margin[7] = { 0, 100, 150, 200, 250, 300, 350 };
int razor_margin[5] = { 0, 300, 360, 420, 480 };
int root_side;
int fl_has_choice;

// switches to facilitate debugging

static const int use_aspiration = 1;
static const int use_nullmove = 1;
static const int use_null_verification = 1;
static const int use_beta_pruning = 1;
static const int use_futility = 1;
static const int use_razoring = 1;
static const int use_lmp = 1;
static const int use_lmr = 1;
static const int lmr_hist_adjustement = 1;

void InitSearch(void) {

  // Set depth of late move reduction using modified Stockfish formula

  for (int dp = 0; dp < MAX_PLY; dp++)
    for (int mv = 0; mv < MAX_MOVES; mv++) {

      double r = log((double)dp) * log((double)Min(mv,63)) / 2;
      if (r < 0.80) r = 0;

      lmr_size[0][dp][mv] = r;            // zero window node
      lmr_size[1][dp][mv] = Max(r -1, 0); // principal variation node

      for (int node = 0; node <= 1; node++) {
        if (lmr_size[node][dp][mv] < 1) lmr_size[node][dp][mv] = 0; // ultra-small reductions make no sense

        if (lmr_size[node][dp][mv] > dp - 1) // reduction cannot exceed actual depth
          lmr_size[node][dp][mv] = dp - 1;
      }
    }
}

void Think(POS *p, int *pv) {

  pv[1] = 0; // fixing rare glitch

  // Play a move from opening book, if applicable

  if (use_book) {
    pv[0] = GuideBook.GetPolyglotMove(p, 1);
    if (pv[0]) return;

    pv[0] = MainBook.GetPolyglotMove(p, 1);
    if (pv[0]) return;
  }

  // Set basic data

  ClearHist();
  tt_date = (tt_date + 1) & 255;
  nodes = 0;
  abort_search = 0;
  verbose = 1;
  Timer.SetStartTime();

  // Search

  Iterate(p, pv);
}

void Iterate(POS *p, int *pv) {

  int val = 0, cur_val = 0;
  U64 nps = 0;
  Timer.SetIterationTiming();
  
  int max_root_depth = Timer.GetData(MAX_DEPTH);

  root_side = p->side;
  SetAsymmetricEval(p->side);

  // Are we operating in slowdown mode or on node limit?

  Timer.special_mode = 0;
  if (Timer.nps_limit 
  || Timer.GetData(MAX_NODES) > 0) Timer.special_mode = 1;

  // Search with increasing depth

  for (root_depth = 1; root_depth <= max_root_depth; root_depth++) {
    int elapsed = Timer.GetElapsedTime();
    if (elapsed) nps = nodes * 1000 / elapsed;

#if defined _WIN32 || defined _WIN64 
    printf("info depth %d time %d nodes %I64d nps %I64d\n", root_depth, elapsed, nodes, nps);
#else
    printf("info depth %d time %d nodes %lld nps %lld\n", root_depth, elapsed, nodes, nps);
#endif

    if (use_aspiration) cur_val = Widen(p, root_depth, pv, cur_val);
    else                cur_val = SearchRoot(p, 0, -INF, INF, root_depth, pv); // full window search

    // don't search too deep with only one move available

    if (root_depth == 8 
    && !fl_has_choice 
    && !Timer.IsInfiniteMode() ) break;

    if (abort_search || Timer.FinishIteration()) break;
    val = cur_val;
  }
}

int Widen(POS *p, int depth, int * pv, int lastScore) {
  
  // Function performs aspiration search, progressively widening the window.
  // Code structure modelled after Senpai 1.0.

  int cur_val = lastScore, alpha, beta;

  if (depth > 6 && lastScore < MAX_EVAL) {
    for (int margin = 10; margin < 500; margin *= 2) {
      alpha = lastScore - margin;
      beta  = lastScore + margin;
      cur_val = SearchRoot(p, 0, alpha, beta, depth, pv);
      if (abort_search) break;
      if (cur_val < alpha) Timer.OnFailLow();
      if (cur_val > alpha && cur_val < beta) 
      return cur_val;                // we have finished within the window
      if (cur_val > MAX_EVAL) break; // verify mate searching with infinite bounds
    }
  }

  cur_val = SearchRoot(p, 0, -INF, INF, root_depth, pv); // full window search
  return cur_val;
}

int SearchRoot(POS *p, int ply, int alpha, int beta, int depth, int *pv) {

  int best, score, move, new_depth, new_pv[MAX_PLY];
  int fl_check, fl_prunable_move, fl_mv_type, reduction;
  int mv_tried = 0, quiet_tried = 0;
  int mv_played[MAX_MOVES];
  int mv_hist_score;
  int victim, last_capt;
  int move_change = 0;
  
  fl_has_choice = 0;
  
  MOVES m[1];
  UNDO u[1];

  // Periodically check for timeout, ponderhit or stop command

  nodes++;
  CheckTimeout();

  // Quick exit
  
  if (abort_search) return 0;

  // Retrieving data from transposition table. We hope for a cutoff
  // or at least for a move to improve move ordering.

  move = 0;
  if (TransRetrieve(p->hash_key, &move, &score, alpha, beta, depth, ply)) {
    
    // For move ordering purposes, a cutoff from hash is treated
    // exactly like a cutoff from search

    if (score >= beta) UpdateHistory(p, -1, move, depth, ply);

    // Root node is a pv node, so we return only exact scores

    if (score > alpha && score < beta)
      return score;
  }
  
  // Are we in check? Knowing that is useful when it comes 
  // to pruning/reduction decisions

  fl_check = InCheck(p);

  // Init moves and variables before entering main loop
  
  best = -INF;
  InitMoves(p, m, move, Refutation(-1), ply);
  
  // Main loop
  
  while ((move = NextMove(m, &fl_mv_type))) {

    mv_hist_score = history[p->pc[Fsq(move)]][Tsq(move)];
	victim = TpOnSq(p, Tsq(move));
	if (victim != NO_TP) last_capt = Tsq(move);
	else last_capt = -1;

    p->DoMove(move, u);
    if (Illegal(p)) { p->UndoMove(move, u); continue; }

    // Update move statistics (needed for reduction/pruning decisions)

    mv_played[mv_tried] = move;
    mv_tried++;
    if (mv_tried > 1) fl_has_choice = 1; // we have a choice between at least two root moves
    if (depth > 16 && verbose) DisplayCurrmove(move, mv_tried);
    if (fl_mv_type == MV_NORMAL) quiet_tried++;
    fl_prunable_move = !InCheck(p) && (fl_mv_type == MV_NORMAL);

    // Set new search depth

    new_depth = depth - 1 + InCheck(p);

    // Late move reduction
  
    reduction = 0;
  
    if (use_lmr
    && depth >= 2
    && mv_tried > 3
    && mv_hist_score < hist_limit
    && alpha > -MAX_EVAL && beta < MAX_EVAL
    && !fl_check 
    &&  fl_prunable_move
    && lmr_size[1][depth][mv_tried] > 0
    && MoveType(move) != CASTLE ) {

    reduction = lmr_size[1][depth][mv_tried];

    // increase reduction on bad history score

    if (mv_hist_score < 0 
    && new_depth - reduction > 2 
    && lmr_hist_adjustement) 
       reduction++;

    new_depth -= reduction;
  }

  re_search:
   
  // PVS

  if (best == -INF)
    score = -Search(p, ply + 1, -beta, -alpha, new_depth, 0, move, last_capt, NEW_NODE(PV_NODE), new_pv);
  else {
    score = -Search(p, ply + 1, -alpha - 1, -alpha, new_depth, 0, move, last_capt, CUT_NODE, new_pv);
    if (!abort_search && score > alpha && score < beta)
      score = -Search(p, ply + 1, -beta, -alpha, new_depth, 0, move, last_capt, PV_NODE, new_pv);
  }

  // Reduced move scored above alpha - we need to re-search it

  if (reduction && score > alpha) {
    new_depth += reduction;
    reduction = 0;
    goto re_search;
  }

    p->UndoMove(move, u);
    if (abort_search) return 0;

  // Beta cutoff

    if (score >= beta) {
      if (!fl_check) {
        UpdateHistory(p, -1, move, depth, ply);
        for (int mv = 0; mv < mv_tried; mv++)
          DecreaseHistory(p, mv_played[mv], depth);
      }
      TransStore(p->hash_key, move, score, LOWER, depth, ply);

      // Update search time depending on whether the first move has changed

      if (depth > 4) {
        if (pv[0] != move) Timer.OnNewRootMove();
        else               Timer.OnOldRootMove();
      }

      // Change the best move and show the new pv

      BuildPv(pv, new_pv, move);
      DisplayPv(score, pv);

      return score;
    }

    // Updating score and alpha

    if (score > best) {
      best = score;

      if (score > alpha) {
        alpha = score;

        // Update search time depending on whether the first move has changed

        if (depth > 4) {
          if (pv[0] != move) Timer.OnNewRootMove();
          else               Timer.OnOldRootMove();
        }

        // Change the best move and show the new pv

        BuildPv(pv, new_pv, move);
        DisplayPv(score, pv);
      }
    }

  } // end of the main loop

  // Return correct checkmate/stalemate score

  if (best == -INF)
    return InCheck(p) ? -MATE + ply : DrawScore(p);

  // Save score in the transposition table

  if (*pv) {
    if (!fl_check) {
      UpdateHistory(p, -1, *pv, depth, ply);
      for (int mv = 0; mv < mv_tried; mv++)
        DecreaseHistory(p, mv_played[mv], depth);
    }
    TransStore(p->hash_key, *pv, best, EXACT, depth, ply);
    
  } else
    TransStore(p->hash_key, 0, best, UPPER, depth, ply);

  return best;
}

int Search(POS *p, int ply, int alpha, int beta, int depth, int was_null, int last_move, int last_capt_sq, int node_type, int *pv) {

  eData e;
  int best, score, null_score, move, new_depth, new_pv[MAX_PLY];
  int fl_check, fl_prunable_node, fl_prunable_move, fl_mv_type, reduction;
  int is_pv = (node_type == PV_NODE);
  int mv_tried = 0, quiet_tried = 0, fl_futility = 0;

  int mv_played[MAX_MOVES];
  int mv_hist_score;
  int victim, last_capt;

  MOVES m[1];
  UNDO u[1];

  assert(ply > 0);

  // Quiescence search entry point

  if (depth <= 0)
    return QuiesceChecks(p, ply, alpha, beta, pv);

  // Periodically check for timeout, ponderhit or stop command

  nodes++;
  CheckTimeout();

  // Quick exit on a timeout or on a statically detected draw
  
  if (abort_search) return 0;
  if (ply) *pv = 0;
  if (IsDraw(p)) return DrawScore(p);

  // Mate distance pruning

  int checkmatingScore = MATE - ply;
  if (checkmatingScore < beta) {
    beta = checkmatingScore;
    if (alpha >= checkmatingScore)
    return alpha;
  }

  int checkmatedScore = -MATE + ply;
  if (checkmatedScore > alpha) {
    alpha = checkmatedScore;
    if (beta <= checkmatedScore)
    return beta;
  }

  // Retrieving data from transposition table. We hope for a cutoff
  // or at least for a move to improve move ordering.

  move = 0;
  if (TransRetrieve(p->hash_key, &move, &score, alpha, beta, depth, ply)) {
    
    // For move ordering purposes, a cutoff from hash is treated
    // exactly like a cutoff from search

    if (score >= beta) UpdateHistory(p, last_move, move, depth, ply);

    // In pv nodes only exact scores are returned. This is done because
    // there is much more pruning and reductions in zero-window nodes,
    // so retrieving such scores in pv nodes works like retrieving scores
    // from slightly lower depth.

    if (!is_pv || (score > alpha && score < beta))
      return score;
  }

  // Safeguard against exceeding ply limit
  
  if (ply >= MAX_PLY - 1)
    return Eval.Return(p, &e, 1);

  // Are we in check? Knowing that is useful when it comes 
  // to pruning/reduction decisions

  fl_check = InCheck(p);

  // INTERNAL ITERATIVE DEEPENING - we try to get a hash move to improve move ordering

  if (!move && is_pv && depth >= 6 && !fl_check) {
    Search(p, ply, alpha, beta, depth - 2, 0, 0, -1, PV_NODE, new_pv);
    if (abort_search) return 0;
    TransRetrieve(p->hash_key, &move, &score, alpha, beta, depth, ply);
  }

  if (!move && node_type == CUT_NODE && depth >= 6 && !fl_check) {
    Search(p, ply, alpha, beta, depth - 4, 0, 0, -1, CUT_NODE, new_pv);
    if (abort_search) return 0;
    TransRetrieve(p->hash_key, &move, &score, alpha, beta, depth, ply);
  }

  // Can we prune this node?

  fl_prunable_node = !fl_check 
                   && !is_pv 
                   && alpha > -MAX_EVAL
                   && beta < MAX_EVAL;

  // Beta pruning / static null move

  if (use_beta_pruning
  && fl_prunable_node
  && depth <= 3
  && !was_null) {
    int sc = Eval.Return(p, &e, 1) - 120 * depth; // TODO: Tune me!
    if (sc > beta) return sc;
  }

  // Null move

  if (use_nullmove
  && fl_prunable_node
  && depth > 1
  && !was_null
  && MayNull(p)
  ) {
    int eval = Eval.Return(p, &e, 1);
    if (eval > beta) {

      new_depth = depth - ((823 + 67 * depth) / 256); // simplified Stockfish formula

      // omit null move search if normal search to the same depth wouldn't exceed beta
      // (sometimes we can check it for free via hash table)

      if (TransRetrieve(p->hash_key, &move, &null_score, alpha, beta, new_depth, ply)) {
        if (null_score < beta) goto avoid_null;
      }

      p->DoNull(u);
      if (new_depth > 0) score = -Search(p, ply + 1, -beta, -beta + 1, new_depth, 1, 0, -1, NEW_NODE(node_type), new_pv);
      else               score = -QuiesceChecks(p, ply + 1, -beta, -beta + 1, new_pv);
      p->UndoNull(u);

      // Verification search (nb. immediate null move within it is prohibited)

      if (new_depth > 6 && score >= beta && use_null_verification)
         score = Search(p, ply, alpha, beta, new_depth - 5, 1, move, -1, CUT_NODE, new_pv);

      if (abort_search ) return 0;
      if (score >= beta) return score;
    }
  } 
  
  avoid_null:

  // end of null move code

  // Razoring based on Toga II 3.0

  if (use_razoring
  && fl_prunable_node
  && !move
  && !was_null
  && !(p->Pawns(p->side) & bbRelRank[p->side][RANK_7]) // no pawns to promote in one move
  && depth <= 4) {
    int threshold = beta - razor_margin[depth];
    int eval = Eval.Return(p, &e, 1);

    if (eval < threshold) {
      score = QuiesceChecks(p, ply, alpha, beta, pv);
      if (score < threshold) return score;
    }
  }

  // end of razoring code 

  // Init moves and variables before entering main loop
  
  best = -INF;
  InitMoves(p, m, move, Refutation(last_move), ply);
  
  // Main loop
  
  while ((move = NextMove(m, &fl_mv_type))) {

    // Gather data about the move

    mv_hist_score = history[p->pc[Fsq(move)]][Tsq(move)];
	victim = TpOnSq(p, Tsq(move));
	if (victim != NO_TP) last_capt = Tsq(move);
	else last_capt = -1;

    // Set futility pruning flag before the first applicable move is tried

    if (fl_mv_type == MV_NORMAL 
    && quiet_tried == 0) {
      if (use_futility
      && fl_prunable_node
      && depth <= 6) {
        if (Eval.Return(p, &e, 1) + fut_margin[depth] < beta) fl_futility = 1;
      }
    }

    p->DoMove(move, u);
    if (Illegal(p)) { p->UndoMove(move, u); continue; }

  // Update move statistics 
  // (needed for reduction/pruning decisions and for updating history score)

  mv_played[mv_tried] = move;
  mv_tried++;
  if (fl_mv_type == MV_NORMAL) quiet_tried++;

  // Can we prune this move?

  fl_prunable_move = !InCheck(p)
                  && (fl_mv_type == MV_NORMAL)
                  && (mv_hist_score < hist_limit);
  
  // Set new search depth

  new_depth = depth - 1;

  // Extensions (applied at pv node or at relatively low depth)

  if (is_pv || depth < 9) {
      new_depth += InCheck(p);                                // check extension, pv or low depth
      if (is_pv && Tsq(move) == last_capt_sq) new_depth += 1; // recapture extension in pv
      if ( is_pv && depth < 6 && TpOnSq(p,Tsq(move)) == P     // pawn to 7th extension at the tips of pv
      && (SqBb(Tsq(move)) & (RANK_2_BB | RANK_7_BB) ) ) new_depth += 1;
  }

  // Futility pruning

  if (fl_futility
  &&  fl_prunable_move
  &&  mv_tried > 1) {
    p->UndoMove(move, u); continue;
  }

  // Late move pruning

  if (use_lmp
  && fl_prunable_node
  && fl_prunable_move
  && quiet_tried > lmp_limit[depth]
  && depth <= 3
  && MoveType(move) != CASTLE ) {
    p->UndoMove(move, u); continue;
  }

  // Late move reduction

  reduction = 0;

  if (use_lmr 
  && depth >= 2
  && mv_tried > 3
  && alpha > -MAX_EVAL && beta < MAX_EVAL
  && !fl_check 
  &&  fl_prunable_move
  && lmr_size[is_pv][depth][mv_tried] > 0
  && MoveType(move) != CASTLE ) {
    
    // read reduction size from the table

    reduction = lmr_size[is_pv][depth][mv_tried];

    // increase reduction on bad history score

    if (mv_hist_score < 0
    && new_depth - reduction > 2
    && lmr_hist_adjustement)
       reduction++;

    // reduce search depth

    new_depth -= reduction;
  }

  if (use_lmr 
  && depth >= 2
  && mv_tried > 3
  && alpha > -MAX_EVAL && beta < MAX_EVAL
  && !fl_check 
  && !InCheck(p)
  && (fl_mv_type == MV_BADCAPT)
  && lmr_size[is_pv][depth][mv_tried] > 0
  && !is_pv) {
     reduction = 1;
	 new_depth -= reduction;
  }

  // a place to come back if reduction scores above alpha

  re_search:
   
  // PVS

  if (best == -INF)
    score = -Search(p, ply + 1, -beta, -alpha, new_depth, 0, move, last_capt, NEW_NODE(node_type), new_pv);
  else {
    score = -Search(p, ply + 1, -alpha - 1, -alpha, new_depth, 0, move, last_capt, CUT_NODE, new_pv);
    if (!abort_search && score > alpha && score < beta)
      score = -Search(p, ply + 1, -beta, -alpha, new_depth, 0, move, last_capt, PV_NODE, new_pv);
  }

  // Reduced move scored above alpha - we need to re-search it

  if (reduction
  && score > alpha) {
    new_depth += reduction;
    reduction = 0;
	if (node_type == ALL_NODE) node_type = CUT_NODE;
    goto re_search;
  }

  // Undo move

  p->UndoMove(move, u);
  if (abort_search) return 0;

  // Beta cutoff

    if (score >= beta) {
      if (!fl_check) {
        UpdateHistory(p, last_move, move, depth, ply);
        for (int mv = 0; mv < mv_tried; mv++)
          DecreaseHistory(p, mv_played[mv], depth);
      }
      TransStore(p->hash_key, move, score, LOWER, depth, ply);

      return score;
    }

  // Updating score and alpha

    if (score > best) {
      best = score;
      if (score > alpha) {
        alpha = score;
        BuildPv(pv, new_pv, move);
      }
    }

  } // end of the main loop

  // Return correct checkmate/stalemate score

  if (best == -INF)
    return InCheck(p) ? -MATE + ply : DrawScore(p);

  // Save score in the transposition table

  if (*pv) {
    if (!fl_check) {
      UpdateHistory(p, last_move, *pv, depth, ply);
      for (int mv = 0; mv < mv_tried; mv++)
        DecreaseHistory(p, mv_played[mv], depth);
    }
    TransStore(p->hash_key, *pv, best, EXACT, depth, ply);
  } else
    TransStore(p->hash_key, 0, best, UPPER, depth, ply);

  return best;
}

int IsDraw(POS *p) {

  // Draw by 50 move rule

  if (p->rev_moves > 100) return 1;

  // Draw by repetition

  for (int i = 4; i <= p->rev_moves; i += 2)
    if (p->hash_key == p->rep_list[p->head - i])
      return 1;

  // With no major pieces on the board, we have some heuristic draws to consider

  if (p->cnt[WC][Q] + p->cnt[BC][Q] + p->cnt[WC][R] + p->cnt[BC][R] == 0) {

    // Draw by insufficient material (bare kings or Km vs K)

    if (!Illegal(p)) {
      if (p->cnt[WC][P] + p->cnt[BC][P] == 0) {
        if (p->cnt[WC][N] + p->cnt[BC][N] + p->cnt[WC][B] + p->cnt[BC][B] <= 1) return 1; // KmK
      }
    }

    // Trivially drawn KPK endgames

    if (p->PawnEndgame() ) {
      if (p->cnt[WC][P] + p->cnt[BC][P] == 1) {

        if (p->cnt[WC][P] == 1 ) return KPKdraw(p, WC); // exactly one white pawn
        if (p->cnt[BC][P] == 1 ) return KPKdraw(p, BC); // exactly one black pawn
      }
    } // pawns only
  }


  return 0; // default: no draw
}

int KPKdraw(POS *p, int sd) {

  int op = Opp(sd);
  U64 bbPawn = p->Pawns(sd);
  U64 bbStrongKing = p->Kings(sd);
  U64 bbWeakKing = p->Kings(op);

  // opposition through a pawn

  if (p->side == sd
  && (bbWeakKing & BB.ShiftFwd(bbPawn, sd))
  && (bbStrongKing & BB.ShiftFwd(bbPawn, op))
  ) return 1;
  
  // weaker side can create opposition through a pawn in one move

  if (p->side == op
  && (BB.KingAttacks(p->king_sq[op]) & BB.ShiftFwd(bbPawn, sd))
  && (bbStrongKing & BB.ShiftFwd(bbPawn, op))
  ) if (!Illegal(p)) return 1;

  // opposition next to a pawn
  
  if (p->side == sd
  && (bbStrongKing & BB.ShiftSideways(bbPawn))
  && (bbWeakKing & BB.ShiftFwd(BB.ShiftFwd(bbStrongKing,sd) ,sd)) 
  ) return 1;

  // TODO: pawn checks king

  return 0;
}

void DisplayCurrmove(int move, int tried) {

  printf("info currmove ");
  PrintMove(move);
  printf(" currmovenumber %d \n", tried);
}

void DisplaySpeed(void) {

  int elapsed = Timer.GetElapsedTime();
  U64 nps = GetNps(elapsed);
#if defined _WIN32 || defined _WIN64 
  printf("info time %d nodes %I64d nps %I64d \n", elapsed, nodes, nps);
#else
  printf("info time %d nodes %lld nps %lld \n", elapsed, nodes, nps);
#endif
  
}

void DisplayPv(int score, int *pv) {

  char *type, pv_str[512];
  int elapsed = Timer.GetElapsedTime();
  U64 nps = GetNps(elapsed);

  type = "mate";
  if (score < -MAX_EVAL)
    score = (-MATE - score) / 2;
  else if (score > MAX_EVAL)
    score = (MATE - score + 1) / 2;
  else
    type = "cp";

  PvToStr(pv, pv_str);
#if defined _WIN32 || defined _WIN64 
  printf("info depth %d time %d nodes %I64d nps %I64d score %s %d pv %s\n",
      root_depth, elapsed, nodes, nps, type, score, pv_str);
#else
  printf("info depth %d time %d nodes %lld nps %lld score %s %d pv %s\n",
      root_depth, elapsed, nodes, nps, type, score, pv_str);
#endif
}

void CheckTimeout(void) {

  char command[80];
  int time;
  U64 nps;

  // Report search speed

  if (!(nodes % 1000000)) DisplaySpeed();

  // We check for timeout or new commands only every so often, 
  // to save some time, unless the engine is operating
  // in the weakening mode or has received "go nodes" command. 
  // In that cases, we check for timeout as often as we can.
  
  if (!Timer.special_mode || Timer.nps_limit > 65535) {
    if (nodes & 4095 || root_depth == 1)
      return;
  }

  if (Timer.GetData(MAX_NODES) > 0
  && nodes >= Timer.GetData(MAX_NODES) ) {
     abort_search = 1;
     return;
  }

  // Slowdown loop

  if (Timer.nps_limit && root_depth > 1) {
    time = Timer.GetElapsedTime() + 1;
    nps = GetNps(time);
    while ((int)nps > Timer.nps_limit) {
      Timer.WasteTime(10);
      time = Timer.GetElapsedTime() + 1;
      nps = GetNps(time);
      if (Timeout()) {
        abort_search = 1;
        return;
      }
    }
  }

  // Process commands that might terminate the search

  if (InputAvailable()) {
    ReadLine(command, sizeof(command));

    if (strcmp(command, "stop") == 0)
      abort_search = 1;
    else if (strcmp(command, "ponderhit") == 0)
      pondering = 0;
  }

  // Have we already used our allocated time?

  if (Timeout()) abort_search = 1;
}

int Timeout() {

  return (!pondering && !Timer.IsInfiniteMode() && Timer.TimeHasElapsed());
}

U64 GetNps(int elapsed) {

  U64 nps = 0;
  if (elapsed) nps = (nodes * 1000) / elapsed;
  return nps;
}

int DrawScore(POS * p) {

  if (p->side == root_side) return -Param.draw_score;
  else                      return  Param.draw_score;
}