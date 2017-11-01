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
 * moveTest.cpp
 *
 *  Created on: Feb 25, 2012
 *      Author: petero
 */

#include "moveTest.hpp"

#include "cute.h"

#include "position.hpp"
#include "move.hpp"
#include "piece.hpp"

/**
 * Test of move constructor, of class Move.
 */
static void
testMoveConstructor() {
    int f = Position::getSquare(4, 1);
    int t = Position::getSquare(4, 3);
    int p = Piece::WROOK;
    Move move(f, t, p);
    ASSERT_EQUAL(move.from(), f);
    ASSERT_EQUAL(move.to(), t);
    ASSERT_EQUAL(move.promoteTo(), p);
}

/**
 * Test of equals, of class Move.
 */
static void
testEquals() {
    Move m1(Position::getSquare(0, 6), Position::getSquare(1, 7), Piece::WROOK);
    Move m2(Position::getSquare(0, 6), Position::getSquare(0, 7), Piece::WROOK);
    Move m3(Position::getSquare(1, 6), Position::getSquare(1, 7), Piece::WROOK);
    Move m4(Position::getSquare(0, 6), Position::getSquare(1, 7), Piece::WKNIGHT);
    Move m5(Position::getSquare(0, 6), Position::getSquare(1, 7), Piece::WROOK);
    ASSERT(!m1.equals(m2));
    ASSERT(!m1.equals(m3));
    ASSERT(!m1.equals(m4));
    ASSERT(m1.equals(m5));
}

cute::suite
MoveTest::getSuite() const {
    cute::suite s;
    s.push_back(CUTE(testMoveConstructor));
    s.push_back(CUTE(testEquals));
    return s;
}
