/*--------------------------------------------------------------------------
    Pawny 1.2, chess engine (source code).
    Copyright (C) 2009 - 2016 by Mincho Georgiev.
    
    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
    
    contact: pawnychess@gmail.com 
    web: http://www.pawny.netii.net/
----------------------------------------------------------------------------*/

#include "data.h"

#define MAXHISTORY 9999
static int search_pv(int alpha, int beta, int depth);
static int qsearch(int alpha, int beta, int checks, int pvnode);
const int rmargin[4] = {0, 200, 300, 300};
const int fmargin[4] = {0, 150, 300, 300};
const int lmp_moves[8] = {0, 4, 8, 12, 16, 20, 24, 28};
const int fullmoves_nws  = 2;
const int fullmoves_pv   = 3;
const int fullmoves_root = 4; 

static void checkup()
{ si.lastcheck = si.nodes;
  check_for_poll();
  if(!opt.analyze) time_check();
}

static void search_init()
{ pos->ply = 0;
  si.nodes = 1;
  si.rootmove.p = tt_get_hashmove();
  si.stop_search = false;
  si.lastcheck = 0;
  memset(si.history, 0, sizeof(si.history));
  memset(si.killer, 0, sizeof(si.killer));
  init_rootlist();
  si.num_hash_saves = 0;
  tt_age();
  #ifdef GTB
  tbhits = 0;
  #endif
}

static void update_history(int depth, move_t move)
{ int i, j;
  if(move.type & (CAP|PROM)) return;
  i = pos->square[move.from];
  si.history[i][move.to] += (depth * 4);
  if(si.history[i][move.to] > MAXHISTORY)
  { for(i = 0; i < 16; i++)
      for(j = 0; j < 64; j++)
        si.history[i][j] /= 2;
  }
  if(move.p == si.killer[0][pos->ply]) return;
  si.killer[1][pos->ply] = si.killer[0][pos->ply];
  si.killer[0][pos->ply] = move.p;
}


static bool is_in_mate()
{ move_t ms[MSSIZE];
  if(is_in_check(pos->side) && !move_gen_evasions(ms))
    return true;
  return false;
}

static bool is_draw()
{ int i;
  if(pos->mr50 > 100 || (pos->mr50 == 100 && !is_in_mate())) 
    return true;
  for(i = pos->sply - 2; i >= 0; i--)
  { if(pos->stack[i].hash == pos->hash)
      return true;
  }
  return false;
}

static int ok_to_reduce(move_t m)
{ if(m.type & PROM)
    return false;
  if(m.p == si.killer[0][pos->ply - 1])
    return false;
  if(m.p == si.killer[1][pos->ply - 1])
    return false;
  if((m.type & CAP) && (m.score >= 0))
    return false;
  if(is_pawn_endgame() || is_king_alone())
    return false;
  return true;
}

static int ok_to_reduce_root(rootmove_t rm)
{ if(rm.m.type & (CAP|PROM))
    return false;
  if(is_pawn_endgame() || is_king_alone())
    return false;
  return true;
}

static int ok_to_cut(move_t m)
{ if((m.type & (CAP|PROM|CASTLE))
  || (is_pawn_endgame()) || (is_king_alone()))
    return false;
  if(m.p == si.killer[0][pos->ply - 1])
    return false;
  if(m.p == si.killer[1][pos->ply - 1])
    return false;
  return true;
}

static void pick_move(move_t *ms,int count, int index)
{	
  int i,best_score = -INF,
  best_index = index;
  move_t temp;

  for(i = index; i < count; i++)
  { if(ms[i].score > best_score)
    { best_score = ms[i].score;
      best_index = i;
    }
  }	
  temp = ms[index];
  ms[index] = ms[best_index];
  ms[best_index] = temp;
}

static int qsearch_evasions(int alpha, int beta, int pvnode)
{   
  int i, value;
  int move_count;
  move_t *ms = gms[pos->ply];
  int moves = 0;
  
  if(si.nodes - si.lastcheck >= TIMECHECK) checkup();
  if(si.stop_search) return 0;
  if(pos->ply >= MAXPLY - 1) return beta;
  if(alpha > MATEVALUE - pos->ply - 1) return alpha;
  if(is_draw()) return 0;
  
  move_count = move_gen_evasions(ms);
  ordering(ms, move_count, tt_get_hashmove());
  for(i = 0; i < move_count; i++)
  { pick_move(ms, move_count, i);
    if(move_illegal(ms[i])) continue;
    move_make(ms[i]);
    si.nodes++;
    moves++;
    if(is_in_check(pos->side))
      value = -qsearch_evasions(-beta, -alpha, pvnode);
    else value = -qsearch(-beta, -alpha, 0, pvnode);
    move_unmake();
    if(si.stop_search) return 0;
    if(value > alpha)
    { if(value >= beta) 
        return value;
      alpha = value;
    }
  }
  if(!moves) return (-MATEVALUE + pos->ply);
  return alpha;
}


static int qsearch(int alpha, int beta, int checks, int pvnode)
{
  int i, value, delta;
  int move_count;
  move_t *ms = gms[pos->ply];
  
  if(si.nodes - si.lastcheck >= TIMECHECK) checkup();
  if(si.stop_search) return 0;
  if(pos->ply >= MAXPLY - 1) return beta;
  if(alpha > MATEVALUE - pos->ply - 1) return alpha;
  if(is_draw()) return 0;
 
  value = eval();
  if(value >= beta) return value;
  if(value > alpha) alpha = value;
  move_count = move_gen_caps(ms);
  caps_ordering(ms, move_count);
  delta = alpha - P_VALUE - value;
  for(i = 0; i < move_count;i ++)
  { pick_move(ms, move_count, i);
    if(move_illegal(ms[i])) continue;

    //delta pruning:
    if(!pvnode && !ms[i].promoted && ms[i].score <= 0 && ms[i].score < delta) 
      continue;
    
    move_make(ms[i]);
    si.nodes++;
    if(is_in_check(pos->side))
      value = -qsearch_evasions(-beta, -alpha, pvnode);
    else value = -qsearch(-beta, -alpha, 0, pvnode);
    move_unmake();
    if(si.stop_search) return 0;
    if(value > alpha)
    { if(value >= beta) 
        return value;
      alpha = value;
    }
  }
  if(checks)
  { move_count = move_gen_checks(ms);
    for(i = 0; i < move_count; i++)
    { if(ms[i].type == DC) ms[i].score = 3000 - pval[Piece(ms[i].from)];
      else ms[i].score = 1000 - pval[Piece(ms[i].from)];
    }
    
    for(i = 0; i < move_count; i++)
    { pick_move(ms, move_count, i);
      if(move_illegal(ms[i])) continue;
      if(ms[i].type != DC && (see_squares(ms[i].from, ms[i].to) < 0)) 
        continue;
      move_make(ms[i]);
      si.nodes++;
      value = -qsearch_evasions(-beta, -alpha, pvnode);
      move_unmake();
      if(si.stop_search) return 0;
      if(value > alpha)
      { if(value >= beta) 
          return value;
        alpha = value;
      }
    }
  }
  return alpha;
}

static int search_nws(int alpha, int beta, int depth, int do_null)
{
  int i, value, move_count;
  nullstack_t ns;
  move_t *ms = gms[pos->ply];
  bool incheck, cut;
  int ev = 0, margin = 0;
  uint8 val_flag = TT_ALPHA;
  uint32 best_move = 0;
  int ply = pos->ply;
  bool futility_ok = false;
  bool mate_threat = false;
  int prev_prom = false;
  uint8 tt_null_ok = true;
  uint32 tested_move = 0;
  move_t m;
  int moves = 0;
  int R = ADAPT_R(depth);
  
  if(si.nodes - si.lastcheck >= TIMECHECK) checkup();
  if(si.stop_search) return 0;
  if(ply >= MAXPLY - 1) return beta;
  if(alpha > MATEVALUE - ply - 1) return alpha;
  if(is_draw()) return 0;

  //transposition table probe:
  i = tt_probe(depth, &value, beta, &best_move, &tt_null_ok, R);
  if((i == TT_EXACT) || (i == TT_ALPHA && value <= alpha) 
  || (i == TT_BETA  && value >= beta)) 
    return value;

  #ifdef GTB
  if(gtb_ok && gtb_men() <= gtb_max_men)
  { if(gtb_probe(&value))
      return value;
  }
  #endif
  
  //is stm in check?:
  incheck = is_in_check(pos->side);
  
  if(incheck)
  { if(depth < 1 || depth > MAXPLY - 4) 
      return qsearch_evasions(alpha, beta, 0);
    
    //check extension (nw nodes):
    if(depth <= 4) depth += 1;
  }
  else
  { if(depth < 1 || depth > MAXPLY - 4) 
      return qsearch(alpha, beta, 1, 0);
    
    if(do_null) prev_prom = pos->stack[pos->sply - 1].m.promoted;
   
    //razoring:
    if(depth < 4 && !best_move && !prev_prom
    && !(pos->occ[WP] & RMASK_7) && !(pos->occ[BP] & RMASK_2))
    { ev = eval();
      if(ev + rmargin[depth] <= alpha)
      { i = alpha - rmargin[depth];
        value = qsearch(i, i + 1, 1, 0);
        if(value <= i) return value;
      }
    }
    //nullmove pruning:
    if(do_null && tt_null_ok)
    { if(depth > R && !is_pawn_endgame() && !is_king_alone())
      { move_make_null(&ns);
        si.nodes++;
        value = -search_nws(-beta, -beta + 1, depth - R - 1, 0);
        move_unmake_null(&ns);
        if(si.stop_search) return 0;
        if(value >= beta) return value;
        if(value < -MATEVALUE + MAXPLY)
          mate_threat = true;
      }
    }
  }
  
  //Try best move first:
  if(validate_move(best_move))
  { m.p = best_move;
    move_make(m);
    si.nodes++;
    value = -search_nws(-beta, -alpha, depth - 1, 1);
    move_unmake();
    if(si.stop_search) return 0;
    if(value > alpha)	
    { if(value >= beta)
      { update_history(depth, m);
        tt_save(depth, value, TT_BETA, m.p);
        return value;
      }
      alpha = value;
      val_flag = TT_EXACT;
    }
    tested_move = m.p;
  }
  
  //generate moves:
  if(incheck) move_count = move_gen_evasions(ms);
  else
  { move_count = move_gen_all(ms);
    //determine the futility usage:
    if(depth < 4 && !mate_threat && !prev_prom)
    { futility_ok = true;
      if(get_phase() > 10) margin = fmargin[depth];
      else margin = (fmargin[depth] * 2);
    }
  }
  //move ordering:
  ordering(ms, move_count, best_move);
  
  for(i = 0; i < move_count; i++)
  { pick_move(ms, move_count, i);
    if(move_illegal(ms[i])) continue;
    moves++;
    if(ms[i].p == tested_move) continue;
    move_make(ms[i]);
    si.nodes++;
    cut = true;
    
    if(is_in_check() || passer_push(ms[i]))
      cut = false;
  
    //futility pruning:
    if(moves > 1 && futility_ok && cut)
    { if((depth < 3 && -material_value() + margin < alpha)
     || (moves > lmp_moves[depth] && ok_to_cut(ms[i]) && alpha > -MATEVALUE + MAXPLY))
      { move_unmake();
        continue;
      }
    }
 
    //LMR:
    if(moves > fullmoves_nws && lmr_R[min(31, depth)][min(63, moves)]
    && cut && !incheck && ok_to_reduce(ms[i]))
    { value = -search_nws(-beta, -alpha, depth - lmr_R[min(31, depth)][min(63, moves)] - 1, 1);
      if(value > alpha) value = -search_nws(-beta, -alpha, depth - 1, 1);
      goto rec_value;
    }
    
    //NWS:
    value = -search_nws(-beta, -alpha, depth - 1, 1);
    
    rec_value:
    move_unmake();
    if(si.stop_search) return 0;
    if(value > alpha)	
    { best_move = ms[i].p;
      if(value >= beta)
      { update_history(depth, ms[i]);
        tt_save(depth, value, TT_BETA, best_move);
        return value;
      }
      alpha = value;
      val_flag = TT_EXACT;
    }
  }
  
  if(!moves)
  { if(incheck) return (-MATEVALUE + ply);
    else return STALEMATEVALUE;
  }
  
  tt_save(depth, alpha, val_flag, best_move);
  return alpha;
}

static int search_pv(int alpha, int beta, int depth)
{
  int i, value, move_count, cut;
  move_t *ms = gms[pos->ply];
  bool incheck;
  uint8 val_flag = TT_ALPHA;
  uint32 best_move = 0;
  int ply = pos->ply;
  int moves = 0;
  
  if(si.nodes - si.lastcheck >= TIMECHECK) checkup();
  if(si.stop_search) return 0;
  if(ply >= MAXPLY - 1) return beta;
  if(alpha > MATEVALUE - ply - 1) return alpha;
  if(is_draw()) return 0;

  //transposition table probe:
  best_move = tt_get_hashmove();

  #ifdef GTB
  if(gtb_ok && gtb_men() <= gtb_max_men)
  { if(gtb_probe(&value))
      return value;
  }
  #endif
  
  //is stm in check?:
  incheck = is_in_check(pos->side);
  if(incheck)
  { if(depth < 1 || depth > MAXPLY - 4) 
      return qsearch_evasions(alpha, beta, 1);
    //check extension:
    depth += 1;
    //generate evasions:
    move_count = move_gen_evasions(ms);
  }
  
  else
  { if(depth < 1 || depth > MAXPLY - 4) 
      return qsearch(alpha, beta, 1, 1);
    //Internal Iterative Deepening:
    if(!best_move && depth >= 3)
    { value = search_pv(alpha, beta, depth - 2);
      if(value <= alpha) search_pv(-INF, beta, depth - 2);
      if(si.stop_search) return 0;
      best_move = tt_get_hashmove();
    }
    //generate moves:
    move_count = move_gen_all(ms);
  }

  //move ordering:
  ordering(ms, move_count, best_move);
  
  for(i = 0; i < move_count; i++)
  { pick_move(ms, move_count, i);
    if(move_illegal(ms[i])) continue;
    move_make(ms[i]);
    si.nodes++;
    moves++;
    cut = true;
  
    if(is_in_check() || passer_push(ms[i])) 
      cut = false;
    
    //LMR:
    if(moves > fullmoves_pv && lmr_R[min(31, depth)][min(63, moves)]
    && cut && !incheck && ok_to_reduce(ms[i]))
    { value = -search_nws(-alpha - 1, -alpha, depth - lmr_R[min(31, depth)][min(63, moves)] - 1, 1);
      if(value > alpha)
      { value = -search_nws(-alpha - 1, -alpha, depth - 1, 1);
        if((value > alpha) && (value < beta))
          value = -search_pv(-beta, -alpha, depth - 1);
      }
      goto rec_value;
    }
    
    //PVS:
    if(moves == 1)
      value = -search_pv(-beta, -alpha, depth - 1);
    else 
    { value = -search_nws(-alpha - 1, -alpha, depth - 1, 1);
      if((value > alpha) && (value < beta))
        value = -search_pv(-beta, -alpha, depth - 1);
    }
    
    rec_value:
    move_unmake();
    if(si.stop_search) return 0;
    if(value > alpha)	
    { best_move = ms[i].p;
      if(value >= beta)
      { update_history(depth, ms[i]);
        tt_save(depth, value, TT_BETA, best_move);
        return value;
      }
      alpha = value;
      val_flag = TT_EXACT;
    }
  }
  
  if(!moves)
  { if(incheck) return (-MATEVALUE + ply);
    else return STALEMATEVALUE;
  }
  
  tt_save(depth, alpha, val_flag, best_move);
  return alpha;
}

static int root_search(int alpha, int beta, int depth)
{
  int i, value, incheck, cut;
  uint64 nodes_old;
  
  si.root_moves = 0;

  //check extension:
  incheck = is_in_check(pos->side);
  if(incheck) depth += 1;
  
  //root moves re/ordering:
  root_ordering();
  
  for(i = 0; i < root_list_size; i++)
  { move_make(rms[i].m);
    si.nodes++;
    si.root_moves++;
    nodes_old = si.nodes;
    cut = true;
    
    if(opt.mode == UCI_MODE)
    { printf("info currmove ");
      print_move(rms[i].m.p);
      printf(" currmovenumber %d\n", si.root_moves);
      if(!(si.root_moves % 7)) update_info();
    }
        
    #ifdef GTB
    if(gtb_ok && gtb_men() <= gtb_max_men)
    { if(gtb_root_probe(&value))
      { value = -value;
        goto root_value;
      }
    }
    #endif
    
    if(is_in_check() || passer_push(rms[i].m)) 
      cut = false;
    
    //LMR:
    if(si.root_moves > fullmoves_root && lmr_R[min(31, depth)][min(63, si.root_moves)]
    && cut && !incheck && ok_to_reduce_root(rms[i]))
    { value = -search_nws(-alpha - 1, -alpha, depth - lmr_R[min(31, depth)][min(63, si.root_moves)] - 1, 1);
      if(value > alpha)
      { value = -search_nws(-alpha - 1, -alpha, depth - 1, 1);
        if((value > alpha) && (value < beta))
          value = -search_pv(-beta, -alpha, depth - 1);
      }
      goto root_value;
    }
    
    //PVS:
    if(si.root_moves==1)
      value = -search_pv(-beta, -alpha, depth - 1);
    else 
    { value = -search_nws(-alpha - 1, -alpha, depth - 1, 1);
      if((value > alpha) && (value < beta))
        value = -search_pv(-beta, -alpha, depth - 1);
    }

    root_value:
    move_unmake();
    if(si.stop_search) return 0;
    rms[i].score = (sint64)(si.nodes - nodes_old);
    if(value > alpha)
    { si.rootmove.p = rms[i].m.p;
      print_info(value, depth);
      if(value >= beta) return value;
      alpha = value;
    }
  }
  if(!si.root_moves)
  { if(incheck) return -MATEVALUE;
    else return STALEMATEVALUE;
  }
  return alpha;
}


void think()
{
  int i, kns, depth, r = 0;
  float t1;
  move_t ponder;
  int alpha = -INF;
  int beta = INF;
  char mstr[8];
  const int threat_margin = 30;
  int time_add;
  int time_ext = 0;

  //book move:
  if(opt.book_active)
  { if(book_move(&si.rootmove))
    { if(opt.mode == UCI_MODE)
      { printf("bestmove ");
        print_move(si.rootmove.p);
        printf("\n");
        return;
      }
      else
      { move_make(si.rootmove);
        printf("\nmove ");
        print_move(si.rootmove.p);
        printf("\n\n");
        return;
      }
    }
  }
  
  //getting the initial time
  depth = opt.max_depth;
  if(!depth) depth = MAXDEPTH - 1;
  opt.startup_time = get_time();
  time_add = opt.time_low;
  
  //init:
  search_init();
  
  //iterative deepening
  for(i = (depth < 4) ? (1) : (3); i <= depth; i++)
  {	
    r = root_search(alpha, beta, i);
    if(si.stop_search) break;
    
    if(r >= beta)
    { r = root_search(alpha, INF, i);
      if(si.stop_search) break;
    }
    else if(r <= alpha) 
    { r = root_search(-INF, beta, i);
      if(si.stop_search) break;
    }
    
    //time increase on fail-low (max = 2 X time_low):
    if(i > 7 && (r + threat_margin < alpha) && time_ext < 2)
    { opt.time_low += time_add;
      time_ext++;
    }
    
    //if the expended time so far exceeds 65% 
    //of the lower time boundary, including added time,
    //and this is not a fixed time search, new iteration is omited.
    if(i > 7 && !opt.analyze && !opt.time_fixed
    && ((get_time() - opt.startup_time) > (65 * opt.time_low) / 100))
      break;
    
    //boundaries update:
    alpha = r - opt.aspiration;
    beta =  r + opt.aspiration;
    if(alpha < -INF) alpha = -INF;
    if(beta > INF) beta = INF;
  }
  
  if(opt.mode == UCI_MODE)
  { move_to_str(mstr, si.rootmove);
    printf("bestmove %s", mstr);
    if(tt_retrieve_ponder(&ponder))
    { move_to_str(mstr, ponder);
      printf(" ponder %s", mstr);
    }
    printf("\n");
  }
  if(opt.mode == CONSOLE_MODE)
  { //time and kilonodes per second calculations:
    t1 = ((float)(get_time() - opt.startup_time) / (float)1000.0); //seconds
    kns = (int)((si.nodes / 1000) / t1);
    if(kns > 0) printf ("   %d Kn/s,  %.2f sec\n", kns, t1);
    //mate or stalemate:
    if(abs(r) >= MATEVALUE - MAXPLY)
    { if(abs(r) == MATEVALUE - 1)
      { move_make(si.rootmove);
        printf("\nmove ");
        print_move(si.rootmove.p);
        printf("\n\n");
        printf("\ncheckmate.\n\n");
        return;
      }
      else
      { move_make(si.rootmove);
        printf("\nmove ");
        print_move(si.rootmove.p);
        printf("\n\n");
        if(r>0) printf("\ngives mate in %d.\n\n",(MATEVALUE - r + 1) / 2);
        else printf("\nreceives mate in %d.\n\n",(MATEVALUE + r) / 2);
        return;
      }
    }
    if(r == STALEMATEVALUE && !si.root_moves)
    { printf("\nstalemate.\n\n");
      return;
    }
    //making the best move and return
    move_make(si.rootmove);
    printf("\nmove ");
    print_move(si.rootmove.p);
    printf("\n\n");
  }
}
