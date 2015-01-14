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

#ifndef TRANS_H
#define TRANS_H

#include "types.h"


class TransData{
public:
    Move move;
    int8_t depth;
    int8_t flag;
    int16_t age;
    int16_t eval;
};

class TransRecord{
public:
    Key key;
    union{
        TransData data;
        Key k_data;
    };
};


class Trans
{
public:
    static void create();
    static void destroy();
    static void reset();
    static int usage();
    static void write(TransRecord* tr, const Key key, const int depth, const int eval, const int flag, const Move move);
    static bool find(TransRecord ** tr, const Key key, TransData& data);
    static void new_age();
	static inline bool rewritable(const TransRecord* tr, const int depth);

    static int hits;
    static int m_size;
private:
    static TransRecord* record;
    static int free;
    static int age;
    static int size;
};

inline bool Trans::rewritable(const TransRecord* tr, int depth){
	return (tr->data.depth <= depth) || (tr->data.age < age);
}

#endif // TRANS_H
