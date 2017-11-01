/*
    Texel - A UCI chess engine.
    Copyright (C) 2016  Peter Österlund, peterosterlund2@gmail.com

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
 * matchbookcreator.hpp
 *
 *  Created on: May 15, 2016
 *      Author: petero
 */

#ifndef MATCHBOOKCREATOR_HPP_
#define MATCHBOOKCREATOR_HPP_

#include "util/util.hpp"
#include "move.hpp"
#include <ostream>
#include <vector>
#include <map>

class Position;

/** Create an opening book suitable for engine-engine matches. */
class MatchBookCreator {
public:
    /** Constructor. */
    MatchBookCreator();

    /** Create match book consisting of all unique positions after
     * playing "depth" half-moves from the starting position. A score
     * is computed for each position by searching for "searchTime"
     * milliseconds. */
    void createBook(int depth, int searchTime, std::ostream& os);

    /** Count number of unique positions in pgnFile at depth <= d
     *  for all d up to the longest game in pgnFile.
     *  PGN variations are ignored. */
    void countUniq(const std::string& pgnFile, std::ostream& os);

    /** Print statistics about all games in pgnFile. */
    void pgnStat(const std::string& pgnFile, bool pairMode, std::ostream& os);

private:
    struct BookLine {
        BookLine() = default;
        BookLine(const std::vector<Move>& m) : moves(m) {}
        std::vector<Move> moves;
        int score = 0;
    };

    /** Create a book line for each unique position reachable from the starting
     *  position in exactly depth plies. */
    void createBookLines(int depth);

    /** Recursive helper. */
    void createBookLines(Position& pos, std::vector<Move>& moveList, int depth);

    /** Compute a search score for each book line. */
    void evaluateBookLines(std::vector<BookLine>& lines, int searchTime,
                           std::ostream& os);

    /** Extract search depth from a PGN comment having the format "score/depth time".
     *  Depths corresponding to mate scores are ignored. */
    static bool getCommentDepth(const std::string& comment, int& depth);


    std::map<U64, BookLine> bookLines;
};


#endif /* TESTBOOKCREATOR_HPP_ */
