/*
    Texel - A UCI chess engine.
    Copyright (C) 2012-2014  Peter Österlund, peterosterlund2@gmail.com

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
 * bookTest.cpp
 *
 *  Created on: Feb 25, 2012
 *      Author: petero
 */

#include "bookTest.hpp"
#include "book.hpp"
#include "textio.hpp"
#include "moveGen.hpp"

#include "cute.h"

/** Check that move is a legal move in position pos. */
static void
checkValid(Position& pos, const Move& move) {
    ASSERT(!move.isEmpty());
    MoveList moveList;
    MoveGen::pseudoLegalMoves(pos, moveList);
    MoveGen::removeIllegal(pos, moveList);
    bool contains = false;
    for (int mi = 0; mi < moveList.size; mi++)
        if (moveList[mi].equals(move)) {
            contains = true;
            break;
        }
    ASSERT(contains);
}

/**
 * Test of getBookMove method, of class Book.
 */
static void
testGetBookMove() {
    Position pos(TextIO::readFEN(TextIO::startPosFEN));
    Book book(true);
    Move move;
    book.getBookMove(pos, move);
    checkValid(pos, move);
}

/**
 * Test of getAllBookMoves method, of class Book.
 */
static void
testGetAllBookMoves() {
    Position pos = TextIO::readFEN(TextIO::startPosFEN);
    Book book(true);
    std::string moveListString = book.getAllBookMoves(pos);
    std::vector<std::string> strMoves;
    splitString(moveListString, strMoves);
    ASSERT(strMoves.size() > 1);
    for (size_t i = 0; i < strMoves.size(); i++) {
        std::string strMove = strMoves[i];
        size_t idx = strMove.find_first_of('(');
        ASSERT(idx != strMove.npos);
        ASSERT(idx > 0);
        strMove = strMove.substr(0, idx);
        Move m = TextIO::stringToMove(pos, strMove);
        checkValid(pos, m);
    }
}


cute::suite
BookTest::getSuite() const {
    cute::suite s;
    s.push_back(CUTE(testGetBookMove));
    s.push_back(CUTE(testGetAllBookMoves));
    return s;
}
