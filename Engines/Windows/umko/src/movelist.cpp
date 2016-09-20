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
#include <climits>

#include "movelist.h"

void MoveList::sort(){
    Move m;
    int e, i, j;
    eval[size] = -INT_MAX;
    for (i = size-2; i >= 0; i--) {
        m = move[i];
        e = eval[i];
        for (j = i; e < eval[j+1]; j++) {
            move[j] = move[j+1];
            eval[j] = eval[j+1];
        }
        move[j] = m;
        eval[j] = e;
   }
}

void MoveList::sort_s(){
    std::string ms[MAX_LIST_SIZE], stmp;
    Move mtmp;

    for(int i=0; i<size; i++)
        ms[i] = move_to_string(move[i]);

    bool swapped;
    do{
        swapped = false;
        for(int i  = 0; i< size-1; i++){
            if(strcmp(ms[i].c_str(),ms[i+1].c_str())>0){

              stmp = ms[i];
              ms[i] = ms[i+1];
              ms[i+1] = stmp;

              mtmp = move[i];
              move[i] = move[i+1];
              move[i+1] = mtmp;

              swapped = true;
          }
        }
    }while(swapped);
}
