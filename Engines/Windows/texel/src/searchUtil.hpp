/*
    Texel - A UCI chess engine.
    Copyright (C) 2013-2015  Peter Österlund, peterosterlund2@gmail.com

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

#include "constants.hpp"
#include "move.hpp"
#include "treeLogger.hpp"


class SearchTreeInfo {
public:
    SearchTreeInfo();

    /** Serialize to byte buffer. */
    U8* serialize(U8* buffer) const;
    const U8* deSerialize(const U8* buffer);

    bool allowNullMove;    // Don't allow two null-moves in a row
    bool abdadaExclusive;  // True to make search avoid searching a subtree that
                           // is already being searched.
    Move bestMove;         // Copy of the best found move at this ply
    Move currentMove;      // Move currently being searched
    int currentMoveNo;     // Index of currentMove in move list
    int evalScore;         // Score from evalPos() or UNKNOWN_SCORE if not known
    U64 nodeIdx;           // For tree logging
    Move singularMove;     // Non-empty when searching for second best
                           // move to determine if best move is singular
};


inline
SearchTreeInfo::SearchTreeInfo()
    : allowNullMove(true), abdadaExclusive(false), currentMoveNo(0),
      evalScore(SearchConst::UNKNOWN_SCORE), nodeIdx(0) {
}

inline U8*
SearchTreeInfo::serialize(U8* buffer) const {
    return Serializer::serialize<4096>(buffer, allowNullMove, abdadaExclusive, bestMove.getCompressedMove(),
                                       currentMove.getCompressedMove(), currentMoveNo,
                                       evalScore, nodeIdx, singularMove.getCompressedMove());
}

inline const U8*
SearchTreeInfo::deSerialize(const U8* buffer) {
    U16 m1, m2, m3;
    buffer = Serializer::deSerialize<4096>(buffer, allowNullMove, abdadaExclusive, m1, m2,
                                           currentMoveNo, evalScore, nodeIdx, m3);
    bestMove.setFromCompressed(m1);
    currentMove.setFromCompressed(m2);
    singularMove.setFromCompressed(m3);
    return buffer;
}

#endif /* SEARCHUTIL_HPP_ */
