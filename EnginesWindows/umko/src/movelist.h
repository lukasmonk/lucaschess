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

#ifndef MOVELIST_H
#define MOVELIST_H

#include "types.h"

class MoveList{
public:
    inline MoveList();
    inline void clear();
    inline void add(const Move m);
    inline void add(const Move m, const int v);
    inline Move get(const int i) const;
    inline bool get_next(Move &move);
    inline int get_size() const;
    void sort();
    void sort_s();
private:
    int size;
    int index;
    static const int MAX_LIST_SIZE = 300;
    Move move[MAX_LIST_SIZE];
    int eval[MAX_LIST_SIZE];
};

inline MoveList::MoveList(){ index = size = 0; }

inline void MoveList::add(const Move m, const int v){ eval[size] = v; move[size++] = m; }

inline void MoveList::add(const Move m){ move[size++] = m; }

inline Move MoveList::get(const int i) const{ return move[i]; }

inline void MoveList::clear() { index = size = 0; }

inline int MoveList::get_size() const { return size; }

inline bool MoveList::get_next(Move &move){
    if(index < size){
        move = this->move[index++];
        return true;
    }
    return false;
}


#endif // MOVELIST_H
