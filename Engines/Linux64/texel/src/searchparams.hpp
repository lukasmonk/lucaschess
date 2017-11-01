/*
    Texel - A UCI chess engine.
    Copyright (C) 2012  Peter Österlund, peterosterlund2@gmail.com

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

/*
 * searchparams.hpp
 *
 *  Created on: Mar 4, 2012
 *      Author: petero
 */

#ifndef SEARCHPARAMS_HPP_
#define SEARCHPARAMS_HPP_

#include <vector>

/**
 * Store search parameters (times, increments, max depth, etc).
 */
class SearchParams {
public:
    std::vector<Move> searchMoves; // If non-empty, search only these moves
    int wTime;                     // White remaining time, ms
    int bTime;                     // Black remaining time, ms
    int wInc;                      // White increment per move, ms
    int bInc;                      // Black increment per move, ms
    int movesToGo;                 // Moves to next time control
    int depth;                     // If >0, don't search deeper than this
    int nodes;                     // If >0, don't search more nodes than this
    int mate;                      // If >0, search for mate-in-x
    int moveTime;                  // If >0, search for exactly this amount of time, ms
    bool infinite;

    SearchParams()
        : wTime(0), bTime(0), wInc(0), bInc(0), movesToGo(0),
          depth(0), nodes(0), mate(0), moveTime(0), infinite(false)
    { }
};

#endif /* SEARCHPARAMS_HPP_ */
