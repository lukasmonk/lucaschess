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
 * book.hpp
 *
 *  Created on: Feb 25, 2012
 *      Author: petero
 */

#ifndef BOOK_HPP_
#define BOOK_HPP_

#include "move.hpp"
#include "util/util.hpp"
#include "util/random.hpp"

#include <map>
#include <vector>
#include <cmath>

class Position;

/**
 * Implements an opening book.
 */
class Book {
public:
    /** Constructor. */
    explicit Book(bool verbose0);

    /** Return a random book move for a position, or empty move if out of book. */
    void getBookMove(Position& pos, Move& out);

    /** Return a string describing all book moves. */
    std::string getAllBookMoves(const Position& pos);

private:

    void initBook();

    /** Add a move to a position in the opening book. */
    void addToBook(const Position& pos, const Move& moveToAdd);

    struct BookEntry {
        Move move;
        int count;
        BookEntry(const Move& m, int cnt = 1) : move(m), count(cnt) { }
    };

    /** Get all book entries for a position. */
    void getBookEntries(const Position& pos, std::vector<BookEntry>& bookMoves) const;

    /** Return transformed count used for weighted random selection. */
    int getWeight(int count, bool pgBook);

    static void createBinBook(std::vector<S8>& binBook);

    /** Add a sequence of moves, starting from the initial position, to the binary opening book. */
    static bool addBookLine(const std::string& line, std::vector<S8>& binBook);

    static int pieceToProm(int p);

    static int promToPiece(int prom, bool whiteMove);


    using BookMap = std::map<U64, std::vector<BookEntry>>;
    static BookMap bookMap;
    static Random rndGen;
    static int numBookMoves;
    bool verbose;

    static const char* bookLines[];
};

inline
Book::Book(bool verbose0)
    : verbose(verbose0) {
}

#endif /* BOOK_HPP_ */
