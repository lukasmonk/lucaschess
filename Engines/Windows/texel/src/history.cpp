/*
    Texel - A UCI chess engine.
    Copyright (C) 2012-2013  Peter Österlund, peterosterlund2@gmail.com

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
 * history.cpp
 *
 *  Created on: Feb 25, 2012
 *      Author: petero
 */

#include "history.hpp"

int History::depthTable[] = {
    0, 1, 6, 19, 42, 56
};

void
History::init() {
    for (int p = 0; p < Piece::nPieceTypes; p++) {
        for (int sq = 0; sq < 64; sq++) {
            Entry& e = ht[p][sq];
            e.countSuccess = 0;
            e.countFail = 0;
            e.score = -1;
        }
    }
}

void
History::reScale() {
    for (int p = 0; p < Piece::nPieceTypes; p++) {
        for (int sq = 0; sq < 64; sq++) {
            Entry& e = ht[p][sq];
            e.countSuccess = e.countSuccess / 4;
            e.countFail = e.countFail / 4;
        }
    }
}
