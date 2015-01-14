/*--------------------------------------------------------------------------
    Pawny 1.0, chess engine (source code).
    Copyright (C) 2009 - 2013 by Mincho Georgiev.
    
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

#define FUTILITY_MARGIN_1 150
#define FUTILITY_MARGIN_2 300

#define FULLDEPTHMOVES 4
#define REDUCTION1 (2*PLY)
#define REDUCTION2 (3*PLY)
#define MAXHISTORY 9999

static void checkup()
{ check_for_poll();
  if(!opt.analyze) time_check();
}

static void search_init()
{ pos->ply = 0;
  si.nodes = 1;
  si.rootmove.p = 0;
  si.stop_search = false;
  memset(si.history, 0, sizeof(si.history));
  memset(si.killer, 0, sizeof(si.killer));
  si.num_hash_saves = 0;
  tt_age();
  #ifdef GTB
  tbhits = 0;
  #endif
}

static void update_history(int depth, move_t move)
{ int i, j;
  if((move.type & CAP) || (move.type & PROM)) return;
  i = pos->square[move.from];
  si.history[i][move.to] += (depth);
  if(si.history[i][move.to] > MAXHISTORY)
  { for(i = 0; i < 16; i++)
      for(j = 0; j < 64; j++)
        si.history[i][j] /= 2;
  }
  if(move.p == si.killer[0][pos->ply]) return;
  si.killer[1][pos->ply] = si.killer[0][pos->ply];
  si.killer[0][pos->ply] = move.p;
}

static bool is_draw()
{ int i;	
  if(pos->mr50 >= 100) return true;
  for(i = pos->sply - 1; i >= 0; i--)
  { if(pos->stack[i].hash == pos->hash)
      return true;
  }
  return false;
}

static int adapt_R(int depth)
{ if(depth > (6*PLY))
  { if(get_phase() > (4 * R_VALUE))
      return (3*PLY);
    return (2*PLY);
  }
  else return (2*PLY);
}

static int ok_to_reduce(move_t m)
{
  if((m.type & PROM) || (is_pawn_endgame()) || (is_king_alone()))
    return false;
  if(m.p == si.killer[0][pos->ply])
    return false;
  if(m.p == si.killer[1][pos->ply])
    return false;
  if((m.type & CAP) && (m.score >= 0))
    return false;
  return true;
}

static int ok_to_cut(move_t m)
{ if((m.type & PROM) || (m.type & CAP)
  || (is_pawn_endgame()) || (is_king_alone()))
    return false;
  if(si.history[pos->square[m.to]][m.to])
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

static int qsearch(int alpha, int beta)
{
  int i, value, bestval, incheck, delta;
  int move_count;
  move_t ms[MSSIZE];
  
  if(!(si.nodes & 0x1fff)) checkup();
  if(si.stop_search) return 0;
  if(pos->ply >= MAXPLY - 1) return beta;
  if(alpha > MATEVALUE - pos->ply - 1) return alpha;
  if(pos->ply > 1 && is_draw()) return 0;
  si.nodes++;
 
  incheck = is_in_check(pos->side);
  if(incheck)
  { bestval = -MATEVALUE-PLY;
    move_count = move_gen(&ms[0]);
    if(!move_count) return (-MATEVALUE + pos->ply);
    ordering(&ms[0], move_count, tt_get_hashmove());
    for(i = 0; i < move_count;i ++)
    { pick_move(&ms[0],move_count, i);
      move_make(ms[i]);
      value = -qsearch(-beta,-alpha);
      move_unmake();
      if(si.stop_search) return 0;
      if(value > bestval)
      { bestval = value;
        if(value >= beta) return value;
        if(value > alpha) alpha = value;
      }
    }
  }
  else
  { value = eval();
    if(value >= beta) return value;
    if(value > alpha) alpha = value;
    move_count = move_gen_caps(&ms[0]);
    caps_ordering(&ms[0],move_count);
    delta = alpha - P_VALUE - value;
    for(i = 0; i < move_count;i ++)
    { pick_move(&ms[0],move_count, i);
      //delta pruning:
      if(!ms[i].promoted && (ms[i].score < delta)) continue;
      move_make(ms[i]);
      value = -qsearch(-beta,-alpha);
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

static int search(int alpha, int beta, int depth, uint8 null_ok)
{
  int i,value,move_count;
  nullstack_t ns;
  move_t ms[MSSIZE];
  bool incheck, cut;
  int ev = 0, futility_value, futility_margin_1 = 0, futility_margin_2 = 0;
  bool pvnode = (bool)((beta - alpha) > 1);
  uint8 val_flag = TT_ALPHA;
  uint32 best_move = 0;
  int legal_moves = 0, searched_moves = 0, 
  ext = 0, ext_1 = 0, ext_2 = 0;
  int ply = pos->ply;
  bool futility_ok = false;
  bool mate_threat = false;
  move_t m;
  uint32 tested_move = 0;
  
  if(!(si.nodes & 0x1fff)) checkup();
  if(si.stop_search) return 0;
  if(ply >= MAXPLY - 1) return beta;
  if(alpha > MATEVALUE - ply - 1) return alpha;
  if(is_draw()) return 0;
  
  //transposition table probe:
  switch(tt_probe(depth, &value, beta, &best_move, &null_ok))
  { case TT_ALPHA:  if(value <= alpha) return alpha; 
                    if(value < beta)   beta = value; break;
    case TT_BETA:   if(value >= beta) return value; 
                    if(value > alpha) alpha = value; break;
    case TT_EXACT:  return value;
    default:break;
  }
  
  #ifdef GTB
  if(gtb_ok && gtb_men() <= gtb_max_men)
  { if(gtb_probe(&value))
      return value;
  }
  #endif

  if(depth < PLY || depth >= 255) return qsearch(alpha,beta);
  si.nodes++;
  incheck = is_in_check(pos->side);

  //nullmove pruning:
  if(!incheck && null_ok)
  { int R = adapt_R(depth);
    if(depth > R && !is_pawn_endgame() && !is_king_alone())
    { move_make_null(&ns);
      value = -search(-beta, -beta + 1, depth - R - PLY, false);
      move_unmake_null(&ns);
      if(si.stop_search) return 0;
      if(value >= beta) return value;
      if(value < -MATEVALUE + MAXPLY)
        mate_threat = true;
    }
  }
  
  //Internal Iterative Deepening:
  if(!incheck && !best_move && pvnode && depth >= (3*PLY))
  { value = search(alpha,beta,depth - (2*PLY),true);
    if(value <= alpha) search(-INF,beta,depth - (2*PLY),true);
    if(si.stop_search) return 0;
    best_move = tt_get_hashmove();
  }
   
  //Try best move before anything else:
  if(best_move)
  { m.p = best_move;
    ext = incheck * PLY;
    if(m.promoted == Coloured(QUEEN,pos->side)) ext += (PLY/2);
    if(((Piece(m.from) == WP) && (Rank(m.to) == RANK_7)) 
    || ((Piece(m.from) == BP) && (Rank(m.to) == RANK_2)))
        ext += (PLY/2);
    move_make(m);
    value = -search(-beta, -alpha, depth - PLY + ext, true);
    move_unmake();
    if(value > alpha)	
    { if(value >= beta)
      { update_history(depth, m);
        tt_save(depth,value,TT_BETA, m.p);
        return value;
      }
      alpha = value;
      val_flag = TT_EXACT;
    }
    tested_move = m.p;
    legal_moves++;
    searched_moves++;
  }
  
  //check extension:
  if(incheck)
  { ext_1 += (PLY);
    if((move_count = move_gen(&ms[0])) == 1)
      ext_1 += (PLY);//singular reply extension
  }
  else move_count = move_gen(&ms[0]);

  //determine the futility usage:
  if(!ext_1 && !pvnode && depth <= (3*PLY) && !mate_threat
  && !pos->stack[pos->sply - 1].m.promoted)
  { ev = eval();
    if(ev > -(2*Q_VALUE))
    { futility_ok = true;
      if(get_phase() > (2*Q_VALUE+2*N_VALUE))
      { futility_margin_1 = FUTILITY_MARGIN_1;
        futility_margin_2 = FUTILITY_MARGIN_2;
      }
      else
      { futility_margin_1 = (FUTILITY_MARGIN_1 * 2);
        futility_margin_2 = (FUTILITY_MARGIN_2 * 2);
      }
    }
  }

  //move ordering:
  ordering(&ms[0],move_count,best_move);
  
  for(i = 0; i < move_count; i++)
  { pick_move(&ms[0],move_count, i);
    if(ms[i].p == tested_move) continue;
    move_make(ms[i]);
    cut = !is_in_check();
    ext_2 = 0;
    
    //queen promotion extension:
    if(ms[i].promoted == Coloured(QUEEN,pos->side)) ext_2 += (PLY/2);
    
    //pawn push extension/off-reduction:
    if(Piece(ms[i].to)  == WP)
    { if(Rank(ms[i].to) == RANK_7) ext_2 += (PLY/2);
      if(cut && Rank(ms[i].to) == RANK_6
      && !(pos->occ[BP] & passer_mask[W][ms[i].to]))
        cut = false;
    }
    if(Piece(ms[i].to)  == BP)
    { if(Rank(ms[i].to) == RANK_2) ext_2 += (PLY/2);
      if(cut && Rank(ms[i].to) == RANK_3
      && !(pos->occ[WP] & passer_mask[B][ms[i].to]))
        cut = false;
    }
    
    //futility pruning:
    if(futility_ok && cut && !ext_2)
    { futility_value = ev + material_gain(ms[i]);
      if((depth < (3*PLY) && futility_value < (alpha - futility_margin_2)) ||
         (depth < (2*PLY) && futility_value < (alpha - futility_margin_1)) ||
         (depth < (4*PLY) && legal_moves > (FULLDEPTHMOVES/2) && ok_to_cut(ms[i])))
      { move_unmake();
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
    move_unmake();
    if(si.stop_search) return 0;
    legal_moves++;
    searched_moves++;
    if(value > alpha)	
    { best_move = ms[i].p;
      if(value >= beta)
      { update_history(depth, ms[i]);
        tt_save(depth,value,TT_BETA,best_move);
        return value;
      }
      alpha = value;
      val_flag = TT_EXACT;
    }
  }
  
  if(!legal_moves)
  { if(incheck) return (-MATEVALUE + ply); //checkmate
    else return STALEMATEVALUE; //stalemate
  }

  //try for "all-moves cut"
  if(legal_moves && !searched_moves) return qsearch(alpha, beta);
  
  //transposition update if it's not a mate or stalemate!
  tt_save(depth,alpha,val_flag,best_move);
  return alpha;
}

static int root_search(move_t *rms, int rmcount, int alpha, int beta, int depth)
{
  int i,value;
  bool incheck, cut;
  int ext_1, ext_2 = 0;
  
  si.root_moves = 0;
  incheck = is_in_check(pos->side);
  
  //check extension:
  ext_1 = incheck * (PLY);
  
  //ordering:
  if(si.rootmove.p == 0) si.rootmove.p = tt_get_hashmove();
  ordering(&rms[0], rmcount, si.rootmove.p);
  
  for(i = 0; i < rmcount; i++)
  { pick_move(&rms[0],rmcount, i);
    
    move_make(rms[i]);
    si.root_moves++;
    
    if(opt.mode == UCI_MODE)
    { printf("info currmove ");
      print_move(rms[i].p);
      printf(" currmovenumber %d\n", si.root_moves);
      if(!(si.root_moves % 7)) update_info();
    }
    
    #ifdef GTB
    if(gtb_ok && gtb_men() <= gtb_max_men)
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
    cut = !is_in_check(pos->side);
    
    //queen promotion extension:
    if(rms[i].promoted == Coloured(QUEEN,pos->side)) ext_2 += (PLY/2);
    //pawn push extension/off-reduction:
    if(Piece(rms[i].to)  == WP)
    { if(Rank(rms[i].to) == RANK_7) ext_2 += (PLY/2);
      if(cut && Rank(rms[i].to) == RANK_6
      && !(pos->occ[BP] & passer_mask[W][rms[i].to]))
        cut = false;
    }
    if(Piece(rms[i].to)  == BP)
    { if(Rank(rms[i].to) == RANK_2) ext_2 += (PLY/2);
      if(cut && Rank(rms[i].to) == RANK_3
      && !(pos->occ[WP] & passer_mask[B][rms[i].to]))
        cut = false;
    }
    
    //LMR:
    if(si.root_moves > FULLDEPTHMOVES && depth > REDUCTION1
    && cut && !ext_1 && !ext_2)
    { if(ok_to_reduce(rms[i]))
      { if(rms[i].type & CAP)
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
    if(si.root_moves==1)
      value = -search(-beta,-alpha,depth - PLY + ext_1 + ext_2,true);
    else 
    { value = -search(-alpha - 1,-alpha,depth - PLY + ext_1 + ext_2,true);
      if((value > alpha) && (value < beta))
        value = -search(-beta,-alpha,depth - PLY + ext_1 + ext_2,true);
    }
    
    root_value:
    move_unmake();
    if(si.stop_search) return 0;
    if(value > alpha)
    { si.rootmove.p = rms[i].p;
      print_info(value, (depth/PLY));
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
  int i,depth,r = 0;
  float t1;
  int milliseconds,kns;
  move_t ponder;
  uint64 total_nodes = 0; //used for calc. of the Kn/s
  int alpha = -INF;
  int beta = INF;
  move_t rms[MSSIZE];
  int rmscore[MSSIZE];
  int rmcount;

  //getting the initial time
  milliseconds = get_time();
  depth = opt.max_depth;
  if(!depth) depth = MAXDEPTH;
  
  search_init();
  opt.startup_time = get_time();
  
  /*
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
  */
  
  rmcount = move_gen(rms);
  for(i = 0; i < rmcount; i++)
  { move_make(rms[i]);
    rmscore[i] = -qsearch(-beta, -alpha);
    move_unmake();    
  }
  rootsort(rms, rmcount, rmscore);
  
  //iterative deepening
  for(i = 1 ; i <= depth; i++)
  {	
    if(opt.mode == CONSOLE_MODE)
    { total_nodes += si.nodes;
      si.nodes = 0;
    }
    
    r = root_search(rms, rmcount, alpha, beta,(i * PLY));
    if(si.stop_search) break;
    
    if(r <= alpha || r >= beta) 
    { r = root_search(rms, rmcount, -INF, INF,(i * PLY));
      if(si.stop_search) break;
    }
    
    //if the expended time so far exceeds 
    //~0.6 of the lower time boundary, 
    //and this is not a fixed time search,
    //new iteration is omited.
    if(i > 7 && !opt.analyze && !opt.time_fixed
    && get_time() - opt.startup_time > (opt.time_low*80)/128)
      break;
    
    alpha = r - opt.aspiration;
    beta =  r + opt.aspiration;
    if(alpha < -INF) alpha = -INF;
    if(beta > INF) beta = INF; 
  }
  
  if(opt.mode == UCI_MODE)
  { printf("bestmove ");
    print_move(si.rootmove.p);
    if(tt_retrieve_ponder(&ponder))
    { printf("ponder ");
      print_move(ponder.p);
    }
    printf("\n");
  }
  if(opt.mode == CONSOLE_MODE)
  { //time and kilonodes per second calculations:
    t1 = ((get_time() - milliseconds) / (float)1000); //seconds
    kns = (int)((total_nodes/1000)/t1);
    if(kns > 0) printf ("   %d Kn/s,  %.2f sec\n",kns,t1);
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
