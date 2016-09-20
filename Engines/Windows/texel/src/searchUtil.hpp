/*
    Texel - A UCI chess engine.
    Copyright (C) 2013  Peter Österlund, peterosterlund2@gmail.com

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
 * searchUtil.hpp
 *
 *  Created on: Jul 15, 2013
 *      Author: petero
 */

#ifndef SEARCHUTIL_HPP_
#define SEARCHUTIL_HPP_

#include "move.hpp"


class SearchTreeInfo {
public:
    SearchTreeInfo();

    bool allowNullMove;    // Don't allow two null-moves in a row
    Move bestMove;         // Copy of the best found move at this ply
    Move currentMove;      // Move currently being searched
    int currentMoveNo;     // Index of currentMove in move list
    int lmr;               // LMR reduction amount
    U64 nodeIdx;           // For tree logging
    Move singularMove;     // Non-empty when searching for second best
                           // move to determine if best move is singular
};


inline
SearchTreeInfo::SearchTreeInfo()
    : allowNullMove(true), currentMoveNo(0),
      lmr(0), nodeIdx(0) {
}


#endif /* SEARCHUTIL_HPP_ */
