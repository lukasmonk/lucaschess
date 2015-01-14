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

#include <cstring>

#include "rootmoves.h"
#include "movegen.h"
#include "trans.h"
#include "egbb.h"

int RootMoves::size;
int RootMoves::current;
RootMove RootMoves::rmove[300];
float RootMoves::rtime;
bool RootMoves::bm;

#if defined(__MINGW32__)
HANDLE RootMoves::bmove_mutex = CreateMutex(NULL, FALSE, "MutexName");
#else
pthread_mutex_t RootMoves::bmove_mutex = PTHREAD_MUTEX_INITIALIZER;
#endif

void RootMoves::new_search(const Thread& thread){
    MoveGenerator mg(thread,MoveNull,0,0);
    current = size = 0;
    Move m;
	int eval;
	while((m = mg.next(eval)) != MoveNull){
        rmove[size].depth = 0;
        rmove[size].eval = 0;
        rmove[size].flag = NoFlag;
        rmove[size].time = 0;
        rmove[size].nodes = 0;
        rmove[size].nps = 0;
        rmove[size++].move = m;
    }
	bm = true;
}

void RootMoves::new_depth(){
    for(int i=0; i<size; i++)
        rmove[i].flag = NoFlag;
}

RootMove* RootMoves::get_next(const int alpha, int beta, const int depth){
    for(int i=0; i<size; i++){
		if(rmove[i].depth < depth ||
		   (rmove[i].flag == Alpha && rmove[i].eval < beta) ||
		   (rmove[i].flag == Beta && rmove[i].eval > alpha)){
            current = i;
            return &rmove[i];
        }
    }
    return NULL;
}

bool RootMoves::first(){
	return current == 0;
}

RootMove* RootMoves::update(const int eval, const int flag, const int depth){
    rmove[current].eval = eval;
    rmove[current].flag = flag;
    rmove[current].depth = depth;
    RootMove r_tmp;

	for(; current>0; current--){
		if(rmove[current].depth == rmove[current-1].depth &&
		   rmove[current].eval > rmove[current-1].eval){
				if(current == 1)
					rtime = (std::clock() - Search::start_time)/(double)CLOCKS_PER_SEC;
				r_tmp = rmove[current-1];
				rmove[current-1] = rmove[current];
				rmove[current] = r_tmp;
		}
		else break;
	}

	if(current == 0){
		double time = std::clock() - Search::start_time;
		if(time > 0) rmove[current].time = int((time/(double)CLOCKS_PER_SEC)*1000);
		else rmove[current].time = 1;
		if(rmove[current].time == 0) rmove[current].time = 1;
		rmove[current].nodes = Search::nodes;
		rmove[current].nps = int(Search::nodes/rmove[current].time)*1000;
		print_best_line();
	}

	return &rmove[current];
}

RootMove* RootMoves::get(const int i){
    return &rmove[i];
}

int RootMoves::get_size(){
    return size;
}

void RootMoves::print_best_line(){
	if(Search::ids_depth < 5) return;
	RootMove* rm = get(0);
	std::cout<<"info score cp "<<rm->eval<<" depth "<<rm->depth;
	std::cout<<" time "<<rm->time<<" nodes "<<rm->nodes<<" nps "<<rm->nps;
	std::cout<<" pv "<<move_to_string(rm->move)<<" ";
	int index = 0;
	while(rm->pv[index] != MoveNull){
		std::cout<<move_to_string(rm->pv[index])<<" ";
		index++;
	}
	std::cout<<std::endl;
}

void RootMoves::print(){
	for(int i=0; i< size; i++){
		RootMove* rm = get(i);
		std::cout<<"info debug multipv "<<i+1<<" score cp "<<rm->eval<<" depth "<<rm->depth;
		std::cout<<" pv "<<move_to_string(rm->move);
		if(rm->flag == Alpha) std::cout<<" flag alpha";
		if(rm->flag == Beta) std::cout<<" flag beta";
		if(rm->flag == Exact) std::cout<<" flag exact";
		if(rm->flag == NoFlag) std::cout<<" flag noflag";
		std::cout<<std::endl;
	}
}

void RootMoves::print_current_move(){
	if(Search::ids_depth < 10) return;
    double time = std::clock() - Search::start_time;
	if(time < 1) time = 1;
    time = time/(double)CLOCKS_PER_SEC;
    if(time == 0) time = 1;
    std::cout<<"info depth "<<Search::ids_depth<<" seldepth "<<Search::max_ply;
    std::cout<<" time "<<int(time*1000)<<" nodes "<<Search::nodes;
    std::cout<<" nps "<<int(Search::nodes/time);
    std::cout<<" hashfull "<<Trans::usage();
    std::cout<<" tbhits "<<EGBB::hits<<std::endl;
    std::cout<<"info currmovenumber "<<current+1;
    std::cout<<" currmove "<<move_to_string(rmove[current].move)<<std::endl;
}

void RootMoves::print_best_move(){
	#if defined(__MINGW32__)
	WaitForSingleObject(bmove_mutex, INFINITE);
	#else
	pthread_mutex_lock(&bmove_mutex);
	#endif
	if(bm){
		double time = (std::clock() - Search::start_time);
		if(time == 0) time = 1;
		time = time/(double)CLOCKS_PER_SEC;
		if(time == 0) time = 1;
		std::cout<<"info depth "<<Search::ids_depth<<" seldepth "<<Search::max_ply;
		std::cout<<" time "<<int(time*1000)<<" nodes "<<Search::nodes;
		std::cout<<" nps "<<int(Search::nodes/time);
		std::cout<<" hashfull "<<Trans::usage()<<std::endl;
		std::cout<<"bestmove "<<move_to_string(get(0)->move)<<std::endl;
		bm = false;
	}
	#if defined(__MINGW32__)
	ReleaseMutex(bmove_mutex);
	#else
	pthread_mutex_unlock(&bmove_mutex);
	#endif
}
