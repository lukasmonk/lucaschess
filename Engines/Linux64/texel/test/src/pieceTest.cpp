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
 * pieceTest.cpp
 *
 *  Created on: Feb 25, 2012
 *      Author: petero
 */

#include "pieceTest.hpp"

#include "cute.h"

#include "move.hpp"
#include "piece.hpp"

/**
 * Test of isWhite method, of class Piece.
 */
static void
testIsWhite() {
    ASSERT_EQUAL(false, Piece::isWhite(Piece::BBISHOP));
    ASSERT_EQUAL(true , Piece::isWhite(Piece::WBISHOP));
    ASSERT_EQUAL(true , Piece::isWhite(Piece::WKING));
    ASSERT_EQUAL(false, Piece::isWhite(Piece::BKING));
}

static void
testMakeWhiteBlack() {
    for (int p = 0; p < Piece::nPieceTypes; p++) {
        if (p == Piece::EMPTY)
            continue;
        ASSERT(Piece::isWhite(Piece::makeWhite(p)));
        ASSERT(!Piece::isWhite(Piece::makeBlack(p)));
        if (Piece::isWhite(p)) {
            ASSERT_EQUAL(p, Piece::makeWhite(Piece::makeBlack(p)));
        } else {
            ASSERT_EQUAL(p, Piece::makeBlack(Piece::makeWhite(p)));
        }
    }
}

cute::suite
PieceTest::getSuite() const {
    cute::suite s;
    s.push_back(CUTE(testIsWhite));
    s.push_back(CUTE(testMakeWhiteBlack));
    return s;
}
