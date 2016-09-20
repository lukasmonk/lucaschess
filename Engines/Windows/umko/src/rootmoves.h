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

#ifndef ROOTMOVES_H
#define ROOTMOVES_H

#if defined(__MINGW32__)
#include <windows.h>
#endif

#include "thread.h"
#include "types.h"

class RootMove{
    public:
        Move move;
        int eval;
        int depth;
        int flag;
        int time;
        unsigned long long nodes;
        int nps;
        Move pv[MAX_SEARCH_PLY+1];
};

class RootMoves{
    public:
		static void new_search(const Thread& thread);
		static void new_depth();
		static RootMove* get_next(const int alpha, int beta, const int depth);
		static bool first();
		static RootMove* update(const int eval, const int flag, const int depth);

        static RootMove* get(const int i);
        static int get_size();
        static void print_current_move();
		static void print_best_line();
        static void print_best_move();
		static void print();
		static float rtime;
    private:
        static int size;
        static int current;
        static RootMove rmove[300];
		static bool bm;

		#if defined(__MINGW32__)
		static HANDLE bmove_mutex;
		#else
		static pthread_mutex_t bmove_mutex;
		#endif
};

#endif // ROOTMOVES_H
