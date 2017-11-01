/*
    Texel - A UCI chess engine.
    Copyright (C) 2012-2015  Peter Österlund, peterosterlund2@gmail.com

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
 * killerTable.hpp
 *
 *  Created on: Feb 25, 2012
 *      Author: petero
 */

#ifndef KILLERTABLE_HPP_
#define KILLERTABLE_HPP_

#include "move.hpp"
#include "constants.hpp"
#include <cassert>


/**
 * Implement a table of killer moves for the killer heuristic.
 */
class KillerTable {
public:
    /** Create an empty killer table. */
    KillerTable();

    /** Clear killer table. */
    void clear();

    /** Add a killer move to the table. Moves are replaced on an LRU basis. */
    void addKiller(int ply, const Move& m);

    /**
     * Get a score for move m based on hits in the killer table.
     * The score is 4 for primary   hit at ply.
     * The score is 3 for secondary hit at ply.
     * The score is 0 otherwise.
     */
    int getKillerScore(int ply, const Move& m) const;

private:
    /** There is one KTEntry for each ply in the search tree. */
    struct KTEntry {
        KTEntry();
        int move0;
        int move1;
    };
    KTEntry ktList[SearchConst::MAX_SEARCH_DEPTH * 2];
};

inline
KillerTable::KillerTable() {
}

inline
KillerTable::KTEntry::KTEntry()
    : move0(0), move1(0) {
}

inline void
KillerTable::addKiller(int ply, const Move& m) {
    if (ply >= (int)COUNT_OF(ktList))
        return;
    int move = (short)(m.from() + (m.to() << 6) + (m.promoteTo() << 12));
    KTEntry& ent = ktList[ply];
    if (move != ent.move0) {
        ent.move1 = ent.move0;
        ent.move0 = move;
    }
}

inline int
KillerTable::getKillerScore(int ply, const Move& m) const {
    int move = (short)(m.from() + (m.to() << 6) + (m.promoteTo() << 12));
    if (ply < (int)COUNT_OF(ktList)) {
        const KTEntry& ent = ktList[ply];
        if (move == ent.move0) {
            return 4;
        } else if (move == ent.move1) {
            return 3;
        }
    }
    return 0;
}

#endif /* KILLERTABLE_HPP_ */
