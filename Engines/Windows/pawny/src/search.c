/*--------------------------------------------------------------------------
    Pawny 0.3.1, chess engine (source code).
    Copyright (C) 2009 - 2011 by Mincho Georgiev.
    
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

#define FUTILITY_MARGIN_1 (N_VALUE + P_VALUE)
#define FUTILITY_MARGIN_2 (R_VALUE + P_VALUE + P_VALUE)
#define FUTILITY_MARGIN_3 (Q_VALUE)
#define FULLDEPTHMOVES 4
#define REDUCTION1 (2*PLY)
#define REDUCTION2 (3*PLY)

static void search_init()
{	
  si->nodes = 0;
  si->rootmove.p = 0;
  si->stop_search = false;
  memset(si->history, 0, sizeof(si->history));
  si->num_hash_saves = 0;
  tt_check();
  #ifdef GTB
  tbhits = 0;
  #endif
}

static bool is_draw()
{
  int i;	
  if(board->mr50 >= 100) return true;
  for(i = board->hply - 1; i >= 0; i--)
  { if(board->hs[i].hash == board->hash)
      return true;
  }
  return false;
}

static int adapt_R(int depth)
{
  state_t *state = &board->state;
  if(depth > (6*PLY))
  {	if(state->material - \
    (state->pawn_value[WHITE] + \
    state->pawn_value[BLACK]) > 4*R_VALUE)
      return (3*PLY);
    return (2*PLY);
  }
  else return (2*PLY);
}

static void pick_move(move_t ms[],int count, int index)
{
  int i,best_score = -INF,
  best_index = index;
  move_t temp;
  
  for(i = index; i < count; i++)
  {	if(ms[i].score > best_score)
    {	best_score = ms[i].score;
      best_index = i;
    }
  }	
  temp = ms[index];
  ms[index] = ms[best_index];
  ms[best_index] = temp;
}


static void update_killers(move_t move)
{
  int ply = board->ply;
  if((move.type & CAP) || (move.type & PROM)) return;
  if(move.p == si->killer[0][ply]) return;
  si->killer[1][ply] = si->killer[0][ply];
  si->killer[0][ply] = move.p;
}

static int ok_to_reduce(move_t m)
{
  int ply = board->ply;
  if((m.type & PROM) || pawn_endgame || king_alone)
    return false;
  if(m.p == si->killer[0][ply])
    return false;
  if(m.p == si->killer[1][ply])
    return false;
  if((m.type & CAP) && (m.score >= 0))
    return false;
  return true;
}

static void checkup()
{
  check_for_poll();
  if(!opt->analyze) time_check();
}

static int qsearch(int alpha, int beta)
{
  int i,value,incheck,delta;
  int move_count;
  move_t ms[MOVE_STACK];
  
  if(!(si->nodes & 0x1fff)) checkup();
  if(si->stop_search) return 0;
  si->nodes++;
  
  value = eval();
  if(value >= beta) return value;
  if(value > alpha) alpha = value;
  
  incheck = is_in_check(board->side);
  if(incheck)
  { move_count = move_gen_evasions(&ms[0]);
    ordering(&ms[0], move_count, tt_get_hashmove());
  }
  else
  { move_count = move_gen_caps(&ms[0]);
    caps_ordering(&ms[0],move_count);
  }

  delta = alpha - P_VALUE - value;
  for(i = 0; i < move_count;i ++)
  { pick_move(&ms[0],move_count, i);
    //delta pruning:
    if(!incheck && !ms[i].promoted
    && (ms[i].score < delta)) continue;
    if(!move_make(ms[i])) continue;
    
    value = -qsearch(-beta,-alpha);
    
    move_undo();
    if(si->stop_search) return 0;
    if(value > alpha)
    { if(value >= beta) return value;
      alpha = value;
    }
  }
  return alpha;
}

static int search(int alpha, int beta, int depth, bool null_ok)
{
  int i,value,move_count;
  nullstack_t ns;
  move_t ms[MOVE_STACK];
  bool incheck, cut;
  bool pvnode = (bool)((beta - alpha) > 1);
  uint8 val_flag = TT_ALPHA;
  uint32 best_move = 0;
  int legal_moves = 0, searched_moves = 0, 
  ext_1 = 0, ext_2 = 0;
  int ply = board->ply,fmb, fut_value = 0;
  bool futility_ok = false;
  bool mate_threat = false;
  state_t *state = &board->state;
  
  if(!(si->nodes & 0x1fff)) checkup();
  if(si->stop_search) return 0;
  if(alpha > MATE_VALUE - ply - 1) return alpha;
  if(ply && is_draw()) return 0;

  //transposition table probe:
  switch(tt_probe(depth,&value, beta, &best_move, &null_ok))
  { case TT_ALPHA:  if(value <= alpha) return alpha; 
                    if(value < beta)   beta = value; break;
    case TT_BETA:   if(value >= beta) return value; 
                    if(value > alpha) alpha = value; break;
    case TT_EXACT:  return value;
    default:break;
  }
  
  #ifdef GTB
  if(gtb_ok && MenCount <= gtb_max_men)
  { if(gtb_probe(&value))
      return value;
  }
  #endif

  if(depth < PLY || depth >= 255) return qsearch(alpha,beta);
  si->nodes++;
  incheck = is_in_check(board->side);
  
  //nullmove pruning:
  if(!incheck && null_ok)
  { int R = adapt_R(depth);
    if(depth > R && !pawn_endgame && !king_alone)
    { move_make_null(&ns);
      value = -search(-beta, -beta + 1, depth - R - PLY, false);
      move_undo_null(&ns);
      if(si->stop_search) return 0;
      if(value >= beta) return value;
      if(value < -MATE_VALUE + MAX_PLY)
        mate_threat = true;
    }
  }
  
  //check extension:
  if(incheck)
  { ext_1 += (PLY);
    if((move_count = move_gen_evasions(&ms[0])) == 1)
      ext_1 += (PLY);//singular reply extension
  }
  else move_count = move_gen(&ms[0]);
  
  //Internal Iterative Deepening:
  if(!best_move && pvnode && depth >= (3*PLY))
  { value = search(alpha,beta,depth - (2*PLY),true);
    if(value <= alpha) search(-INF,beta,depth - (2*PLY),true);
    if(si->stop_search) return 0;
    best_move = tt_get_hashmove();
  }
  
  //determine the futility value:
  if(!pvnode && !ext_1 && !mate_threat)
  { if(depth == (3*PLY))
    { fmb = full_material_ballance(board->side);
      if((fmb + FUTILITY_MARGIN_3) <= alpha 
      && (state->piece_count[board->xside] > 3)) 
        depth -= (PLY);
    }
    if(depth == (2*PLY))
    { fmb = full_material_ballance(board->side);
      if(fmb + FUTILITY_MARGIN_2 <= alpha)
      { futility_ok = true;
        fut_value = fmb + FUTILITY_MARGIN_2;
      }
    }
    if(depth == (PLY))
    { fmb = full_material_ballance(board->side);
      if(fmb + FUTILITY_MARGIN_1 <= alpha)
      { futility_ok = true;
        fut_value = fmb + FUTILITY_MARGIN_1;
      }
    }
  }
  
  //move ordering:
  ordering(&ms[0],move_count,best_move);
  
  for(i = 0; i < move_count; i++)
  { pick_move(&ms[0],move_count, i);
    if(!move_make(ms[i])) continue;
    cut = !is_in_check(board->side);
    ext_2 = 0;
    
    //queen promotion extension:
    if(ms[i].promoted == Coloured(QUEEN)) ext_2 += (PLY/2);
    
    //pawn push extension/off-reduction:
    if(PieceType(ms[i].to)  == WP)
    { if(calc_rank(ms[i].to) == RANK_7) ext_2 += (PLY/2);
      if(cut && calc_rank(ms[i].to) == RANK_6
      && !(board->bb_pawns[B] & passer_mask[W][rsz[ms[i].to]]))
        cut = false;
    }
    if(PieceType(ms[i].to)  == BP)
    { if(calc_rank(ms[i].to) == RANK_2) ext_2 += (PLY/2);
      if(cut && calc_rank(ms[i].to) == RANK_3
      && !(board->bb_pawns[W] & passer_mask[B][rsz[ms[i].to]]))
        cut = false;
    }
    
    //futility pruning:
    if(futility_ok && cut && !ext_2)
    { if(fut_value + material_gain(ms[i]) <= alpha)
      { move_undo();
        legal_moves++;
        continue;
      }
    }
    
    //LMR:
    if(legal_moves > FULLDEPTHMOVES && depth > REDUCTION1
    && cut && !ext_1 && !ext_2 && !mate_threat)
    { if(ok_to_reduce(ms[i]))
      { if(ms[i].type & CAP) 
          value = -search(-alpha - 1, -alpha, depth - REDUCTION1,true);
        else
        { if(depth > REDUCTION2)
            value = -search(-alpha - 1, -alpha, depth - REDUCTION2,true);
          else value = -search(-alpha - 1, -alpha, depth - REDUCTION1,true);
        }
      }
      else value = alpha + 1;
      if(value > alpha)
      { value = -search(-alpha - 1,-alpha,depth - PLY,true);
        if((value > alpha) && (value < beta))
          value = -search(-beta,-alpha,depth - PLY,true);
      }
      goto rec_value;
    }
    
    //PVS:
    if(!legal_moves)
      value = -search(-beta,-alpha,depth - PLY + ext_1 + ext_2,true);
    else 
    { value = -search(-alpha - 1,-alpha,depth - PLY + ext_1 + ext_2,true);
      if((value > alpha) && (value < beta))
        value = -search(-beta,-alpha,depth - PLY + ext_1 + ext_2,true);
    }
    
    rec_value:
    move_undo();
    if(si->stop_search) return 0;
    legal_moves++;
    searched_moves++;
    if(value > alpha)	
    { best_move = ms[i].p;
      if(value >= beta)
      { update_killers(ms[i]);
        tt_save(depth,value,TT_BETA,best_move);
        return value;
      }
      alpha = value;
      val_flag = TT_EXACT;
      si->history[ms[i].from][ms[i].to] += (depth/PLY);
    }
  }
  
  if(!legal_moves)
  { if(incheck) return (-MATE_VALUE + ply); //checkmate
    else return STALEMATE_VALUE; //stalemate
  }

  //try for "all-moves cut"
  if(legal_moves && !searched_moves) return qsearch(alpha,beta);
  
  //transposition update if it's not a mate or stalemate!
  tt_save(depth,alpha,val_flag,best_move);
  return alpha;
}

static int root_search(int alpha, int beta, int depth)
{
  int i,value,move_count;
  bool incheck, cut;
  move_t ms[MOVE_STACK];
  int ext_1, ext_2 = 0;
  #ifdef GTB
  state_t *state = &board->state;
  #endif
  
  si->nodes++;
  board->ply = 0;
  si->root_moves = 0;
  incheck = is_in_check(board->side);
  
  //check extension:
  ext_1 = incheck * (PLY);
  move_count = move_gen_legal(&ms[0]);
  
  if(si->rootmove.p == 0) si->rootmove.p = tt_get_hashmove();
  
  //ordering
  ordering(&ms[0], move_count, si->rootmove.p);
  
  for(i = 0; i < move_count; i++)
  { pick_move(&ms[0],move_count, i);
    if(!move_make(ms[i])) continue;
    si->root_moves++;
    
    if(opt->mode == UCI_MODE)
    { printf("info currmove ");
      print_move(ms[i].p);
      printf(" currmovenumber %d\n", si->root_moves);
      if(!(si->root_moves % 7)) update_info();
    }
    
    #ifdef GTB
    if(gtb_ok && MenCount <= gtb_max_men)
    { if(gtb_root_probe(&value))
      { if(value != 0) 
        { if(is_draw()) value = 0;
          else value = -value; 
        }
        goto root_value;
      }
    }
    #endif
    
    ext_2 = 0;
    cut = !is_in_check(board->side);
    
    //queen promotion extension:
    if(ms[i].promoted == Coloured(QUEEN)) ext_2 += (PLY/2);
    //pawn push extension/off-reduction:
    if(PieceType(ms[i].to)  == WP)
    { if(calc_rank(ms[i].to) == RANK_7) ext_2 += (PLY/2);
      if(cut && calc_rank(ms[i].to) == RANK_6
      && !(board->bb_pawns[B] & passer_mask[W][rsz[ms[i].to]]))
        cut = false;
    }
    if(PieceType(ms[i].to)  == BP)
    { if(calc_rank(ms[i].to) == RANK_2) ext_2 += (PLY/2);
      if(cut && calc_rank(ms[i].to) == RANK_3
      && !(board->bb_pawns[W] & passer_mask[B][rsz[ms[i].to]]))
        cut = false;
    }
    //LMR:
    if(si->root_moves > FULLDEPTHMOVES && depth > REDUCTION1
    && cut && !ext_1 && !ext_2)
    { if(ok_to_reduce(ms[i]))
      { if(ms[i].type & CAP)
          value = -search(-alpha - 1, -alpha, depth - REDUCTION1,true);
        else
        { if(depth > REDUCTION2)
            value = -search(-alpha - 1, -alpha, depth - REDUCTION2,true);
          else value = -search(-alpha - 1, -alpha, depth - REDUCTION1,true);
        }
      }
      else value = alpha + 1;
      if(value > alpha)
      { value = -search(-alpha - 1,-alpha,depth - PLY,true);
        if((value > alpha) && (value < beta))
          value = -search(-beta,-alpha,depth - PLY,true);
      }
      goto root_value;
    }
    //PVS:
    if(si->root_moves==1)
      value = -search(-beta,-alpha,depth - PLY + ext_1 + ext_2,true);
    else 
    { value = -search(-alpha - 1,-alpha,depth - PLY + ext_1 + ext_2,true);
      if((value > alpha) && (value < beta))
        value = -search(-beta,-alpha,depth - PLY + ext_1 + ext_2,true);
    }
    
    root_value:
    move_undo();
    if(si->stop_search) return 0;
    if(value > alpha)
    { si->rootmove.p = ms[i].p;
      print_info(value, (depth/PLY));
      if(value >= beta)
      { update_killers(ms[i]);
        return value;
      }
      alpha = value;
      si->history[ms[i].from][ms[i].to] += (depth/PLY);
    }
  }
  if(!si->root_moves)
  { if(incheck) return -MATE_VALUE;
    else return STALEMATE_VALUE;
  }
  return alpha;
}

void think()
{
  int i,depth,r = 0;
  float t1;
  int milliseconds,kns;
  move_t ponder;
  uint64 total_nodes = 0; //used for calc. of the Kn/s
  int alpha = -INF;
  int beta = INF;

  //getting the initial time
  milliseconds = get_time();
  depth = opt->max_depth;
  if(!depth) depth = MAX_DEPTH;
  
  search_init();
  opt->startup_time = get_time();
  
  //book move:
  if(opt->book_active)
  { if(book_move(&si->rootmove))
    { if(move_make(si->rootmove))
      { if(opt->mode == UCI_MODE)
        { printf("bestmove ");
          print_move(si->rootmove.p);
          printf("\n");
          move_undo();
        }
        else
        { printf("\nmove ");
          print_move(si->rootmove.p);
          printf("\n\n");
        }
        return;
      }
    }
  }
  //iterative deepening
  for(i = 1 ; i <= depth; i++)
  {	
    if(opt->mode == CONSOLE_MODE)
    { total_nodes += si->nodes;
      si->nodes = 0;
    }
    
    r = root_search(alpha,beta,(i * PLY));
    if(si->stop_search) break;
    
    if(r <= alpha || r >= beta) 
    { r = root_search(-INF,INF,(i * PLY));
      if(si->stop_search) break;
    }
    
    //if the expended time so far exceeds 
    //~0.6 of the lower time boundary, 
    //and this is not a fixed time search,
    //new iteration is omited.
    if(i > 7 && !opt->analyze && !opt->time_fixed
    && get_time() - opt->startup_time > (opt->time_low*80)/128)
      break;
    
    alpha = r - opt->aspiration;
    beta =  r + opt->aspiration;
    if(alpha < -INF) alpha = -INF;
    if(beta > INF) beta = INF; 
  }
  
  if(opt->mode == UCI_MODE)
  { printf("bestmove ");
    print_move(si->rootmove.p);
    if(tt_retrieve_ponder(&ponder))
    { printf("ponder ");
      print_move(ponder.p);
    }
    printf("\n");
  }
  if(opt->mode == CONSOLE_MODE)
  { //time and kilonodes per second calculations:
    t1 = ((get_time() - milliseconds) / (float)1000); //seconds
    kns = (int)((total_nodes/1000)/t1);
    if(kns > 0) printf ("   %d Kn/s,  %.2f sec\n",kns,t1);
    //mate or stalemate:
    if(abs(r) >= MATE_VALUE - MAX_PLY)
    { if(abs(r) == MATE_VALUE - 1)
      { move_make(si->rootmove);
        printf("\nmove ");
        print_move(si->rootmove.p);
        printf("\n\n");
        printf("\ncheckmate.\n\n");
        return;
      }
      else
      { move_make(si->rootmove);
        printf("\nmove ");
        print_move(si->rootmove.p);
        printf("\n\n");
        if(r>0) printf("\ngives mate in %d.\n\n",(MATE_VALUE - r + 1) / 2);
        else printf("\nreceives mate in %d.\n\n",(MATE_VALUE + r) / 2);
        return;
      }
    }
    if(r == STALEMATE_VALUE && !si->root_moves)
    { printf("\nstalemate.\n\n");
      return;
    }
    //making the best move and return
    move_make(si->rootmove);
    printf("\nmove ");
    print_move(si->rootmove.p);
    printf("\n\n");
  }
}
