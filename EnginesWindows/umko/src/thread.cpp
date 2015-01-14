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

#include "thread.h"
#include "search.h"

Thread Thread::s_thread(512,512,1024);

Thread::Thread(const int m_hash, const int p_hash, const int q_hash){
    ph_size = p_hash;
    mh_size = m_hash;
    qh_size = q_hash;
    material_hash = new MaterialHash[mh_size];
    pawn_hash = new PawnHash[ph_size];
    hash = new QHash[qh_size];
    pos.set_thread(this);
    joinable = false;
}

Thread::~Thread(){
    delete[] material_hash;
    delete[] pawn_hash;
    delete[] hash;
}

void Thread::set_position(const Position& pos){
    this->pos = pos;
    this->pos.set_thread(&s_thread);
    this->pos.update_current();
}

#if defined(__MINGW32__)
DWORD WINAPI thread_run(LPVOID thread){
#else
void *thread_run(void *thread) {
#endif
    Thread * t = (Thread*)thread;
    t->joinable = true;
    t->reset();
    Search::ids(t->pos);
    return NULL;
}

void Thread::start_search(const Position pos){
    if(s_thread.joinable){
		Search::should_stop = true;
#if defined(__MINGW32__)
		WaitForSingleObject(s_thread.wthread, INFINITE);
#else
        pthread_join(s_thread.pthread, NULL);
#endif
		Search::should_stop = false;
    }
    s_thread.set_position(pos);
#if defined(__MINGW32__)
	DWORD iID[1];
	s_thread.wthread = CreateThread(NULL, 0, thread_run, (LPVOID)(&s_thread), 0, iID);
#else
	pthread_create(&s_thread.pthread, NULL, thread_run, (void*)(&s_thread));
#endif
}

void Thread::stop_search(){
    if(s_thread.joinable){
		Search::should_stop = true;
#if defined(__MINGW32__)
		WaitForSingleObject(s_thread.wthread, INFINITE);
#else
        pthread_join(s_thread.pthread, NULL);
#endif
		Search::should_stop = false;
    }
}

void Thread::move_good(const Move b_move, const Move h_move[256], const int h_size, const int ply, const int depth){
	Piece p;
	Square from, to;

	if(!is_tactical(b_move) && b_move != MoveNull){

		if(killer[ply][0] != b_move) {
		  killer[ply][1] = killer[ply][0];
		  killer[ply][0] = b_move;
		}

		p = pos.get_piece(move_from(b_move));
		to = move_to(b_move);
		from = move_from(b_move);

		#ifndef NDEBUG
		if(p>13 || to > 63 || from > 63){
			std::cerr<<"Wrong good move!"<<std::endl;
			pos.print();
			std::cerr<<"move:"<<move_to_string(b_move)<<std::endl;
			exit(1);
		}
		#endif

		history[p][from][to] += depth*depth;

		if (history[p][from][to] >= HistoryMax) {
			for (Piece p = WP; p <= BK; p++) {
				for(Square sq_f=A1; sq_f<=H8; sq_f++){
					for(Square sq_t=A1; sq_t<=H8; sq_t++){
						history[p][sq_f][sq_t] /= 2;
					}
				}
			}
		}

		his_tot[p][from][to]++;
		his_hit[p][from][to]++;

		if (his_tot[p][from][to] >= HistoryMax) {
			his_tot[p][from][to] = (his_tot[p][from][to] + 1) / 2;
			his_hit[p][from][to] = (his_hit[p][from][to] + 1) / 2;
		}
	}

	for(int i=0; i<h_size; i++){
		if(is_tactical(h_move[i])) continue;
		p = pos.get_piece(move_from(h_move[i]));
		to = move_to(h_move[i]);
		from = move_from(h_move[i]);

		#ifndef NDEBUG
		if(p>13 || to > 63 || from > 63){
			std::cerr<<"Wrong good move!"<<std::endl;
			pos.print();
			std::cerr<<"move:"<<move_to_string(h_move[i])<<std::endl;
			exit(1);
		}
		#endif

		his_tot[p][from][to]++;

		if (his_tot[p][from][to] >= HistoryMax) {
			his_tot[p][from][to] = (his_tot[p][from][to] + 1) / 2;
			his_hit[p][from][to] = (his_hit[p][from][to] + 1) / 2;
		}
	}
}

int Thread::eval_history_move(const Move move) const {
	Piece p = pos.get_piece(move_from(move));
	Square to = move_to(move);
	Square from = move_from(move);
	return (his_hit[p][from][to] * 16384) / his_tot[p][from][to];
}

void Thread::reset(){
    for(int i = 0; i < MAX_SEARCH_PLY; i++)
        killer[i][0] = killer[i][1] = MoveNull;
    for(Piece p = Piece(PAWN); p<=BK; p++){
		for(Square sq_f = A1; sq_f<=H8; sq_f++){
			for(Square sq_t = A1; sq_t<=H8; sq_t++){
				history[p][sq_f][sq_t] = 0;
				his_hit[p][sq_f][sq_t] = 1;
				his_tot[p][sq_f][sq_t] = 1;
			}
        }
    }
    m_hits = 0;
    p_hits = 0;
    h_hits = 0;
}

QHash* Thread::find(const Key key){
    QHash* qh = &hash[key%qh_size];
    if(qh->key == key){
        h_hits ++;
        return qh;
    }
    return NULL;
}

void Thread::write(const Key key, const int eval, const int depth, const int flag, const Move move){
    QHash* qh = &hash[key%qh_size];
    qh->key = key;
    qh->eval = eval;
    qh->depth = depth;
    qh->flag = flag;
    qh->move = move;
}
