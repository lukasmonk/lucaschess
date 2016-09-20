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

#define MAX_SEARCH_PLY 100

#ifndef SEARCH_H
#define SEARCH_H

#include <ctime>

#include "thread.h"
#include "position.h"
#include "rootmoves.h"

class Search{
public:
    static void start(const Position& pos, const bool infinite, const int max_depth, const unsigned long long max_nodes);
    static void ids(Position & pos);
	static int rs(Position & pos, int alpha, int beta, const int depth);
	static int pvs(Position & pos, const bool extended, Move pv[], int alpha, int beta, const int ply, int depth);
    static int mws(Position & pos, const bool nmp, int beta, const int ply, int depth);
    static int qs(Position & pos, Move pv[], int alpha, int beta, const int ply, int depth);

	static int pvs_new_depth(const Position& pos, const Move move, const bool single_replay,
						  const bool extended, bool & extend, const int depth);
    static int mws_new_depth(const Position& pos, const Move move, const bool single_replay, const int depth);

    static void stop();

    static std::clock_t start_time;

    static int nps;
    static unsigned long long nodes;
    static int max_ply;
    static int ids_depth;

    static bool infinite;
    static unsigned long long max_nodes;
    static int max_depth;
	static int time;

	static int egbb_ply;
	static bool egbb_root;

	static bool should_stop;
};

#endif // SEARCH_H
