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

#ifndef MOVEGEN_H
#define MOVEGEN_H

#include "position.h"
#include "thread.h"
#include "movelist.h"

class MoveGenerator{
    public:
        MoveGenerator(const Position& pos);
        MoveGenerator(const Thread& thread, const Move tt_move, const int depth);
        MoveGenerator(const Thread& thread, const Move ttMove, const int ply, const int depth);
		Move next(int & eval);
        inline int size();
        static void init();
        static GenState state[22];

        static int GenS;
        static int GenQS;
        static int GenEvasionS;
        static int GenEvasionQS;
        static int GenCheckQS;
        static int Gen;
    private:
        const Thread* thread;
        const Position* pos;
        int index;
        int ply;
        int kindex;
        MoveList ml;
        MoveList bad;
        Move ttMove;
        Move killer[2];
};

inline int MoveGenerator::size(){ return ml.get_size(); }

#endif // MOVEGEN_H
