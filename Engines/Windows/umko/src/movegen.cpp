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

#include <cstdlib>

#include "movegen.h"

GenState MoveGenerator::state[22];

int MoveGenerator::GenS;
int MoveGenerator::GenQS;
int MoveGenerator::GenEvasionS;
int MoveGenerator::GenEvasionQS;
int MoveGenerator::GenCheckQS;
int MoveGenerator::Gen;

void MoveGenerator::init(){
    int s = 0;

    GenS = s;
    state[s++] = TRANS;
    state[s++] = GOOD_CAPTURE;
    state[s++] = KILLER;
    state[s++] = QUIET_H;
    state[s++] = BAD_CAPTURE;
    state[s++] = END;

    GenQS = s;
    state[s++] = TRANS;
    state[s++] = CAPTURE_QS;
    state[s++] = END;

    GenCheckQS = s;
    state[s++] = TRANS;
    state[s++] = CAPTURE_QS;
    state[s++] = CHECK_QS;
    state[s++] = END;

    GenEvasionS = s;
    state[s++] = TRANS;
    state[s++] = EVASION;
    state[s++] = BAD_EVASION_CAPTURE;
    state[s++] = END;

    GenEvasionQS = s;
    state[s++] = TRANS;
    state[s++] = EVASION_QS;
    state[s++] = END;

    Gen = s;
    state[s++] = CAPTURE;
    state[s++] = QUIET;
}

MoveGenerator::MoveGenerator(const Position& pos){
    this->pos = &pos;
    index = Gen;
}

MoveGenerator::MoveGenerator(const Thread& thread, const Move tt_move, const int depth){
    this->thread = &thread;
    this->ttMove = tt_move;

    if(this->thread->pos.is_in_check()){
        index = GenEvasionQS;
        this->thread->pos.gen_evasions(ml,this->thread->history);
        if(ml.get_size() == 0){
            index++;
            return;
        }
        ml.sort();
    }
    else if(depth < 0) index = GenQS;
    else index = GenCheckQS;
}

MoveGenerator::MoveGenerator(const Thread& thread, const Move ttMove, const int ply, const int /* depth*/){
    this->thread = &thread;
    this->ttMove = ttMove;
    this->ply = ply;

    if(this->thread->pos.is_in_check()){
        index = GenEvasionS;
        this->thread->pos.gen_evasions(ml,this->thread->history);
        if(ml.get_size() == 0){
            index+=3;
            return;
        }
        ml.sort();
    }
    else index = GenS;

    kindex = -1;
}

Move MoveGenerator::next(int & eval){
    Move move;
	eval = HistoryMax;
    while(state[index] != END){
        switch(state[index]){
            case TRANS:{
                if(ttMove != MoveNull){
#ifndef NDEBUG
                    if(!thread->pos.move_gen_legal(ttMove)){
                        std::cerr<<"Wrong ttMove!"<<std::endl;
                        exit(1);
                    }
#endif
                    index ++;
                    return ttMove;
                }
                break;
            }
           case CAPTURE:{
                if(ml.get_size() == 0){
                    pos->gen_capture(ml);
                    if(ml.get_size() == 0) break;
                    ml.sort();
                }
                while(ml.get_next(move)){
                    if(!pos->move_gen_legal(move))
                        continue;
                    return move;
                }
                ml.clear();
                break;
            }
            case GOOD_CAPTURE:{
                if(ml.get_size() == 0){
                    thread->pos.gen_capture(ml);
                    if(ml.get_size() == 0) break;
                    ml.sort();
                }
                while(ml.get_next(move)){
                    if(move == ttMove || !thread->pos.move_gen_legal(move))
                        continue;
                    if(thread->pos.see(move) < 0){
                        bad.add(move);
                        continue;
                    }
                    return move;
                }
                ml.clear();
                break;
            }
            case KILLER:{
                if(kindex == 2) break;
                if(kindex == -1){
                    killer[0] = thread->killer[ply][0];
                    killer[1] = thread->killer[ply][1];
                    kindex = 0;
                }
                while(kindex < 2){
                    if(killer[kindex] == ttMove ||
                       !thread->pos.move_legal(killer[kindex]) ||
                       !thread->pos.move_gen_legal(killer[kindex])){
                        kindex ++;
                        continue;
                    }
                    return killer[kindex++];
                }
                break;
            }
            case BAD_CAPTURE:{
                while(bad.get_next(move)){
                    if(move != ttMove && move != killer[0] && move != killer[1] &&
                       thread->pos.move_gen_legal(move))
                        return move;
                }
                break;
            }
            case BAD_EVASION_CAPTURE:{
                while(bad.get_next(move)){
                    if(move != ttMove && thread->pos.move_gen_legal(move))
                        return move;
                }
                break;
            }
            case QUIET:{
                if(ml.get_size() == 0){
                    pos->gen_quiet(ml);
                    if(ml.get_size() == 0) break;
                    ml.sort();
                }
                while(ml.get_next(move)){
                    if(move != killer[1] &&
					   pos->move_gen_legal(move)){
						return move;
					}
                }
                ml.clear();
                break;
            }
            case QUIET_H:{
                if(ml.get_size() == 0){
                    thread->pos.gen_quiet(ml,thread->history);
                    if(ml.get_size() == 0) break;
                    ml.sort();
                }
                while(ml.get_next(move)){
                    if(move != ttMove && move != killer[0] && move != killer[1] &&
					   thread->pos.move_gen_legal(move)){
						eval = thread->eval_history_move(move);
                        return move;
					}
                }
                ml.clear();
                break;
            }
            case EVASION:{
                while(ml.get_next(move)){
                    if(move == ttMove || !thread->pos.move_gen_legal(move))
                        continue;
                    if(thread->pos.see(move) < 0){
                        bad.add(move);
                        continue;
                    }
                    return move;
                }
                ml.clear();
                break;
            }
            case CAPTURE_QS:{
                if(ml.get_size() == 0){
                    thread->pos.gen_capture(ml);
                    if(ml.get_size() == 0) break;
                    ml.sort();
                }
                while(ml.get_next(move)){
                    if(move == ttMove ||!thread->pos.move_gen_legal(move) || thread->pos.see(move) < 0)
                        continue;
                    return move;
                }
                ml.clear();
                break;
            }
            case CHECK_QS:{
                if(ml.get_size() == 0){
                    thread->pos.gen_quiet_checks(ml);
                    if(ml.get_size() == 0) break;
                }
                while(ml.get_next(move)){
                    if(move == ttMove || !thread->pos.move_gen_legal(move) || thread->pos.see(move) < 0)
                        continue;
                    return move;
                }
                ml.clear();
                break;
            }
            case EVASION_QS:{
                while(ml.get_next(move)){
                    if(move != ttMove && thread->pos.move_gen_legal(move))
                        return move;
                }
                ml.clear();
                break;
            }
            default:{
                std::cerr<<"State of move generator is wrong!"<<std::endl;
                exit(1);
            }
        }
        index++;
    }
    return MoveNull;
}
