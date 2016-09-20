/***************************************************************************
 *   Copyright (C) 2009 by Borko Bošković                                  *
 *   borko.boskovic@gmail.com                                              *
 *                                                                         *
 *   This program is free software: you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation, either version 3 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>. *
 ***************************************************************************/

#include <iostream>
#include <cstring>
#include <limits>
#include <cmath>

#include "search.h"
#include "movegen.h"
#include "trans.h"
#include "book.h"
#include "timer.h"
#include "engine.h"
#include "egbb.h"

int Search::nps;
unsigned long long Search::nodes;
int Search::max_ply;
bool Search::infinite;
unsigned long long Search::max_nodes;
int Search::max_depth;
std::clock_t Search::start_time;
int Search::ids_depth;
int Search::time;
bool Search::should_stop;
int Search::egbb_ply;
bool Search::egbb_root;

void Search::start(const Position& pos, const bool infinite, const int max_depth, const unsigned long long max_nodes){
    start_time = std::clock();
    Search::infinite = infinite;
    Search::max_nodes = max_nodes;
    Search::max_depth = max_depth;
    Thread::start_search(pos);
}

void Search::ids(Position & pos){
	int depth, window, alpha, beta, eval, flag;
    max_ply = 0;

	Trans::new_age();
    RootMoves::new_search(*pos.thread);
	if(!infinite && max_depth == 0 && max_nodes == 0){
		if((Book::is_loaded() && Book::find_move(pos)) || RootMoves::get_size() == 1){
			RootMoves::print_best_move();
            return;
        }
    }
	Timer::start(time);

	int piece_num = pos.num_pieces();
	egbb_root = false;
	if(piece_num <= 5 && EGBB::isLoaded())
		egbb_root = true;

	RootMoves::rtime = 0;
    nodes = 0;
    EGBB::hits = 0;
	eval = 0;

    if(max_depth == 0) depth = MAX_SEARCH_PLY;
    else depth = max_depth;
	for(ids_depth=1; ids_depth<=depth && !should_stop; ids_depth++){
		window = 40;
		alpha = eval - window/2;
		beta = eval + window/2;
        RootMoves::new_depth();
		flag = NoFlag;
		egbb_ply = (2 * ids_depth) / 3;
		do{
			eval = rs(pos,alpha,beta,ids_depth);
			if(eval >= beta){
				if(flag == Beta) break;
				flag = Alpha;
				alpha = eval;
				beta = eval + window;
				window += window;
			}
			else if(eval <= alpha){
				if(flag == Alpha) break;
				flag = Beta;
				alpha = eval - window;
				beta = eval;
				window += window;
			}
		}while(eval <= alpha || eval >= beta);
    }
    ids_depth --;
	if(!should_stop && (
			(max_depth != 0 || max_nodes != 0) ||
			(!infinite && max_depth == 0 && max_nodes == 0))){
        RootMoves::print_best_move();
    }
}


int Search::rs(Position & pos, int alpha, int beta, const int depth){
	RootMove* r_move;
	int eval, new_depth;
	bool cap_extend;

	//std::cout<<"rs["<<alpha<<","<<beta<<"]"<<std::endl;
	while(alpha < beta && (r_move = RootMoves::get_next(alpha, beta, depth)) != NULL && !should_stop){
		RootMoves::print_current_move();
		new_depth = pvs_new_depth(pos, r_move->move, false, false, cap_extend, depth);
		//std::cout<<"move:"<<move_to_string(r_move->move)<<std::endl;
		pos.move_do(r_move->move);
		if(RootMoves::first()){
			//std::cout<<"pvs("<<alpha<<","<<beta<<")=";
			eval = -pvs(pos, cap_extend, r_move->pv,-beta, -alpha, 1,new_depth);
			if(should_stop) break;
			//std::cout<<eval<<std::endl;
		}
		else{
			//std::cout<<"mws("<<alpha<<","<<alpha+1<<")=";
			eval = -mws(pos,true,-alpha,1,new_depth);
			if(should_stop) break;
			//std::cout<<eval<<std::endl;
			if(eval > alpha){
				//r_move = RootMoves::update(eval,Alpha,depth);
				//std::cout<<"pvs("<<alpha<<","<<beta<<")=";
				eval = -pvs(pos, cap_extend, r_move->pv,-beta, -alpha, 1,new_depth);
				if(should_stop) break;
				//std::cout<<eval<<std::endl;
			}
		}
		pos.move_undo();
		if(eval > alpha){
			alpha = eval;
			if(eval >= beta) r_move = RootMoves::update(eval,Alpha,depth);
			else r_move = RootMoves::update(eval,Exact,depth);
		}
		else r_move = RootMoves::update(eval,Beta,depth);
		//RootMoves::print();
	}

	//pos.thread->move_good(RootMoves::get(0)->move,0,depth);

	return alpha;
}

int Search::pvs(Position & pos, const bool extended, Move pv[], int alpha, int beta, const int ply, int depth){
    const int old_alpha = alpha;
    const int old_beta = beta;
    Move move, best_move = MoveNull, tt_move;
    int eval, new_depth;

	if(pos.thread->id == 0) nodes ++;

	if(should_stop || pos.draw()) return DRAW;

	if(ply > max_ply) max_ply = ply;

    pv[0] = MoveNull;

	int piece_num = pos.num_pieces();
	if(piece_num <= 5 && EGBB::isLoaded()){
		if(egbb_root){
			if(ply >= egbb_ply && EGBB::probe(pos,eval)){
				return eval_from_tt(eval,ply);
			}
		}
		else{
			if(ply <= egbb_ply && EGBB::probe(pos,eval)){
				return eval_from_tt(eval,ply);
			}
		}
	}

    TransRecord * tr;
    TransData td;
    const Key key = pos.get_key();
	if(Trans::find(&tr,key,td))	tt_move = td.move;
    else tt_move = MoveNull;

    if(depth <= 0) return qs(pos,pv,alpha,beta,ply,0);

    bool is_check = pos.is_in_check();

    if(!is_check) eval = - MATE + (ply+2);
    else eval = - MATE + ply;

    if(alpha < eval){
        alpha = eval;
        if(beta <= eval) return eval;
    }

    eval = MATE - ply;
    if(beta > eval){
        beta = eval;
        if(alpha >= eval) return eval;
    }

    if(ply == MAX_SEARCH_PLY - 1) return pos.eval();

    const int pv_size = MAX_SEARCH_PLY+1-ply;
    Move new_pv[MAX_SEARCH_PLY+1-ply];

    if(depth >= 3 && tt_move == MoveNull) {
        new_depth = std::min(depth-3,depth/2);
		eval = pvs(pos,false,new_pv,alpha,beta,ply,new_depth);
		if (eval <= alpha) eval = pvs(pos,false,new_pv,-MATE,beta,ply,new_depth);
        tt_move = new_pv[0];
   }

    MoveGenerator mg(*pos.thread,tt_move,ply,depth);

    bool single_replay = false;

    if(is_check){
        if(mg.size() == 0) return -MATE + ply;
        if(mg.size() == 1) single_replay = true;
    }

	int move_nb = 0;
	bool cap_extend, reduce, history_pruning = false;
	Move history_move[256];

	while(alpha < beta && (move = mg.next(eval)) != MoveNull){
		reduce = false;
		new_depth = pvs_new_depth(pos, move, single_replay, extended, cap_extend, depth);
		history_move[move_nb ++] = move;

		if(!is_check && new_depth < depth && move_nb >= 4
		   && depth >= 3 && !pos.is_dangerous(move)){
            if(!history_pruning && !is_tactical(move)){
                history_pruning = true;
            }
			if(history_pruning && move_nb >= 7){
                reduce = true;
                new_depth --;
            }
        }

        pos.move_do(move);

		if(move_nb == 1) eval = - pvs(pos,cap_extend,new_pv,-beta,-alpha,ply+1,new_depth);
        else{
            eval = - mws(pos,true,-alpha,ply+1,new_depth);
			if(eval > alpha) eval = - pvs(pos,cap_extend,new_pv,-beta,-alpha,ply+1,new_depth);
        }

        if(reduce && eval >= beta){
            new_depth ++;
			eval = - pvs(pos,cap_extend,new_pv,-beta,-alpha,ply+1,new_depth);
        }

        pos.move_undo();

        if(eval > alpha){
			best_move = move;
            pv[0] = move;
            memcpy(pv+1,new_pv,(pv_size-1)*sizeof(Move));
            alpha = eval;
		}
    }

	if(move_nb == 0 || should_stop) return DRAW;

	if(alpha >= old_beta){
		pos.thread->move_good(best_move,history_move,move_nb-1,ply,depth);
        Trans::write(tr,key,depth,eval_to_tt(alpha,ply),Alpha,best_move);
    }
	else if(alpha <= old_alpha){
		Trans::write(tr,key,depth,eval_to_tt(alpha,ply),Beta,tt_move);
	}
	else{
		pos.thread->move_good(best_move,history_move,move_nb-1,ply,depth);
		Trans::write(tr,key,depth,eval_to_tt(alpha,ply),Exact,best_move);
	}

    return alpha;
}

int Search::mws(Position & pos, const bool pruning, int beta, const int ply, int depth){
	Move move, best_move = MoveNull, tt_move;
    int eval, alpha = beta-1, new_depth;

	if(pos.thread->id == 0) nodes ++;

	if(should_stop || pos.draw()) return DRAW;

    if(ply > max_ply) max_ply = ply;

	int piece_num = pos.num_pieces();
	if(piece_num <= 5 && EGBB::isLoaded()){
		if(egbb_root){
			if(ply >= egbb_ply && EGBB::probe(pos,eval)){
				return eval_from_tt(eval,ply);
			}
		}
		else{
			if(ply <= egbb_ply && EGBB::probe(pos,eval)){
				return eval_from_tt(eval,ply);
			}
		}
	}


    TransRecord * tr;
    TransData td;
    const Key key = pos.get_key();
    if(Trans::find(&tr,key,td)){
        tt_move = td.move;
        if(td.depth < depth){
            if(td.eval < -MATE + MAX_SEARCH_PLY && (td.flag == Beta || td.flag == Exact)){
                td.depth = depth;
                td.flag = Beta;
            }
            if(td.eval > MATE - MAX_SEARCH_PLY && (td.flag == Alpha || td.flag == Exact)){
                td.depth = depth;
                td.flag = Alpha;
            }
        }
        if(td.depth >= depth){
			eval = eval_from_tt(td.eval,ply);
			if(td.flag == Exact){ return eval; }
            else if(td.flag == Alpha && eval > alpha){ return eval; }
            else if(td.flag == Beta && eval < beta) { return eval; }
        }
    }
    else tt_move = MoveNull;
    
	if(depth <= 0) return qs(pos,NULL,alpha,beta,ply,0);

    bool is_check = pos.is_in_check();

    if(!is_check) eval = - MATE + (ply+2);
    else eval = - MATE + ply;

    if(alpha < eval) return eval;

    eval = MATE - ply;
    if(beta > eval) return eval;

    if(ply == MAX_SEARCH_PLY - 1) return pos.eval();

	if(pruning && !is_check && depth >= 2 && !mate_value(beta) &&
	   pos.doNullMove() && (depth <= 4 || pos.eval() >= beta)){
        new_depth = depth -4;

        pos.move_do(MoveNull);
		eval = - mws(pos,true,-alpha,ply+1,new_depth);
        pos.move_undo();

        if(depth > 5 && eval >= beta){
            new_depth = depth - 5;
            eval = mws(pos,false,alpha,ply+1,new_depth);
        }

		if(eval >= beta){
			if(!should_stop) Trans::write(tr,key,depth,eval_to_tt(eval,ply),Alpha,tt_move);
            return eval;
        }
    }
	else if(depth <= 3 &&  pos.eval() < beta - 300){
        eval = qs(pos,NULL,alpha,beta,ply,0);
        if(eval < beta) return eval;
    }

    MoveGenerator mg(*pos.thread,tt_move,ply,depth);

    bool single_replay = false;

    if(is_check){
        if(mg.size() == 0) return -MATE + ply;
        if(mg.size() == 1) single_replay = true;
    }

	int move_nb = 0, history_nb = 0, f_eval = MATE, f_margin;
    bool reduce, history_pruning = false;
	Move history_move[256];
	while((move = mg.next(eval)) != MoveNull){
        reduce = false;
        new_depth = mws_new_depth(pos, move, single_replay,depth);
		move_nb ++;

		if(pruning && !is_check && depth <= 6 && new_depth < depth && history_nb >= 1 + depth
		   && eval < 2*(HistoryMax*0.6)/(depth+depth%2) && !pos.is_dangerous(move)){
			continue;
		}

		if(pruning && depth <= 5 && !is_check && new_depth < depth
		   && !is_tactical(move) && !pos.is_dangerous(move)){
            if(f_eval == MATE){
                if(depth >= 2) f_margin = 200 + (depth % 2) * 100;
                else f_margin = 100;
                f_eval = pos.eval() + f_margin;
            }
            if(f_eval < alpha){
                continue;
            }
        }

		if(pruning && !is_check && new_depth < depth && history_nb >= 3
		   && depth >= 3 && !pos.is_dangerous(move)){
            if(!history_pruning && !is_tactical(move)){
                history_pruning = true;
            }
            if(history_pruning){
                reduce = true;
                new_depth --;
            }
        }

        pos.move_do(move);

        eval = - mws(pos,true,-alpha,ply+1,new_depth);

        if(reduce && eval >= beta){
            new_depth ++;
            eval = - mws(pos,true,-alpha,ply+1,new_depth);
        }

        pos.move_undo();

        if(eval > alpha){
            best_move = move;
            alpha = eval;
            break;
        }
		history_move[history_nb ++] = move;
	}

	if(move_nb == 0 || should_stop) return DRAW;

    if(alpha >= beta){
		pos.thread->move_good(best_move,history_move,history_nb,ply,depth);
        Trans::write(tr,key,depth,eval_to_tt(alpha,ply),Alpha,best_move);
    }
    else Trans::write(tr,key,depth,eval_to_tt(alpha,ply),Beta,tt_move);

    return alpha;
}

int Search::qs(Position & pos, Move pv[], int alpha, int beta, const int ply, int depth){
    int eval, f_eval, pv_size;
    const int old_alpha = alpha;
    const int old_beta = beta;
    bool is_check;
    Move move, best_move = MoveNull;

	if(should_stop) return DRAW;

    if(pos.thread->id == 0) nodes ++;

    if(pv) pv[0] = MoveNull;

    if(ply > max_ply) max_ply = ply;

    if(pos.draw()) return DRAW;

    QHash* hash = pos.thread->find(pos.get_key());
    if(hash != NULL){
        if(pv){
            pv[0] = Move(hash->move);
            pv[1] = MoveNull;
        }
        if((depth < 0 || (depth == 0 && hash->depth == 0))){
            eval = eval_from_tt(hash->eval,ply);
            if(hash->flag == QHash::EXACT)
                return eval;
            if(hash->flag == QHash::ALPHA){
                if(eval >= beta)return eval;
                if(eval > alpha) alpha = eval;
            }
            if(hash->flag == QHash::BETA){
                if(eval <= alpha) return eval;
                if(eval < beta) beta = eval;
            }
        }
        best_move = Move(hash->move);
    }

    is_check = pos.is_in_check();

    if(!is_check) eval = - MATE + (ply+2);
    else eval = - MATE + ply;

    if(alpha < eval){
        alpha = eval;
        if(beta <= eval) return eval;
    }

    eval = MATE - ply;
    if(beta > eval){
        beta = eval;
        if(alpha >= eval) return eval;
    }

    if(ply == MAX_SEARCH_PLY - 1) return pos.eval();

    if(!is_check){
        if(hash != NULL && hash->flag == QHash::EVAL)
            eval = hash->eval;
        else eval = pos.eval();

        if(eval > alpha){
            if(eval >= beta){
                pos.thread->write(pos.get_key(),eval,depth,QHash::EVAL,best_move);
                return eval;
            }
            alpha = eval;
        }
        f_eval = eval + 50;
    }
    else{
        f_eval = MATE;
        depth++;
    }

    MoveGenerator mg(*(pos.thread),best_move, depth);

    if(is_check && mg.size() == 0) return -MATE+ply;

    if(pv) pv_size = MAX_SEARCH_PLY+1-ply;
    else pv_size = 1;
    Move new_pv[pv_size];

	while(alpha < beta && (move = mg.next(eval)) != MoveNull){
        if(!is_check && !pos.is_capture_dangerous(move) && !pos.is_move_check(move)){
            eval = f_eval + pos.move_value(move);
            if (eval <= alpha) continue;
        }
        pos.move_do(move);
        if(pv) eval = - qs(pos, new_pv, -beta, -alpha, ply+1, depth-1);
        else eval = - qs(pos, NULL, -beta, -alpha, ply+1, depth-1);
        pos.move_undo();
        if(eval > alpha){
            best_move = move;
            if(pv){
                pv[0] = move;
                memcpy(pv+1,new_pv,(pv_size-1)*sizeof(Move));
            }
            alpha = eval;
        }
    }

	if(should_stop) return DRAW;

    if(alpha >= old_beta) pos.thread->write(pos.get_key(),eval_to_tt(alpha,ply),depth,QHash::ALPHA,best_move);
    else if(alpha <= old_alpha) pos.thread->write(pos.get_key(),eval_to_tt(alpha,ply),depth,QHash::BETA,best_move);
    else pos.thread->write(pos.get_key(),eval_to_tt(alpha,ply),depth,QHash::EXACT,best_move);

    return alpha;
}

int Search::pvs_new_depth(const Position& pos, const Move move, const bool single_replay,
						  const bool extended, bool & extend, const int depth){

	extend = false;

	if(single_replay || pos.is_move_check(move)) return depth;

	Piece p = pos.get_piece(move_from(move));
	if(p == WP || p == BP){
		if(pos.is_passed(move_to(move))) return depth;
	}

	p = pos.get_piece(move_to(move));
	if (p != NO_PIECE && p > BP && pos.endgame()) return depth;

	if(pos.get_captured() != NO_PIECE && !extended && pos.see(move) >= -100){
		extend = true;
		return depth;
	}

	return depth-1;
}

int Search::mws_new_depth(const Position& pos, const Move move, const bool single_replay, const int depth){
	if(single_replay || (pos.is_move_check(move) && pos.see(move) >= -100))
		return depth;

    return depth-1;
}


void Search::stop(){
    Thread::stop_search();
}
