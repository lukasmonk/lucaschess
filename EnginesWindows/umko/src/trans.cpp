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

#include "trans.h"

#include <climits>
#include <iostream>

TransRecord* Trans::record = NULL;
int Trans::free;
int Trans::age;
int Trans::size;
int Trans::hits;
int Trans::m_size = 128;

void Trans::create(){
    if(record != NULL) delete record;
    free = size = (m_size * 1048576) / sizeof(TransRecord);
    record = new TransRecord[size];
    reset();
}

void Trans::destroy(){
	delete [] record;
    size = 0;
}

int Trans::usage(){
    return (int)(((size-free)/(float)size)*1000);
}

void Trans::write(TransRecord* tr, const Key key, const int depth, const int eval, const int flag, const Move move){
    int tt_depth = tr->data.depth;
    int tt_age = tr->data.age;
    if(tt_depth <= depth || tt_age < age){
        if(tt_depth == 0 || tt_age < age) free --;
        tr->data.depth = depth;
        tr->data.flag = flag;
        tr->data.move = move;
        tr->data.age = age;
        tr->data.eval = eval;
		tr->key = key /*^ tr->k_data*/;
	}
}

bool Trans::find(TransRecord ** tr, const Key key, TransData& data){
    *tr = &record[key % size];
    TransRecord rec = *(*tr);
	Key tt_key = rec.key /*^ rec.k_data*/;
    if(tt_key == key){
        hits++;
        data = rec.data;
        if(data.age < age){
            (*tr)->data.age = age;
			//(*tr)->key = key ^ rec.k_data;
            free --;
        }
        return true;
    }
    return false;
}

void Trans::new_age(){
    int i;
    if(age == 32767){
        //Key key;
        for(i=0; i<size; i++){
			//key = record[i].key ^ record[i].k_data;
            record[i].data.age = 0;
			//record[i].key = key ^ record[i].k_data;
        }
        age = 1;
    }
    else age ++;
    free = size;
}

void Trans::reset(){
    for(int i=0; i<size; i++){
        record[i].key = 0;
        record[i].k_data = 0;
    }
    free = size;
    age = 0;
}
