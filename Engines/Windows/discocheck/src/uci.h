/*
 * DiscoCheck, an UCI chess engine. Copyright (C) 2011-2013 Lucas Braesch.
 *
 * DiscoCheck is free software: you can redistribute it and/or modify it under the terms of the GNU
 * General Public License as published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 *
 * DiscoCheck is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without
 * even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with this program. If not,
 * see <http://www.gnu.org/licenses/>.
*/
#pragma once
#include <string>
#include "move.h"

namespace uci {

extern void loop();
extern std::string check_input();

// UCI option values
extern int Hash;
extern int Contempt;
extern bool Ponder;
extern int TimeBuffer;

struct info {
	void clear();

	enum BoundType {EXACT, LBOUND, UBOUND};
	BoundType bound;

	int score, depth, time;
	uint64_t nodes;
	move::move_t *pv;
};

extern std::ostream& operator<< (std::ostream& ostrm, const info& ui);

}	// namespace uci
