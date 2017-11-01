/*
    Texel - A UCI chess engine.
    Copyright (C) 2012-2013,2015  Peter Österlund, peterosterlund2@gmail.com

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
 * computerPlayerTest.cpp
 *
 *  Created on: Feb 25, 2012
 *      Author: petero
 */

#include "computerPlayerTest.hpp"
#include "computerPlayer.hpp"
#include "humanPlayer.hpp"
#include "textio.hpp"
#include "game.hpp"

#include <memory>

#include "cute.h"

/**
 * Test of getCommand method, of class ComputerPlayer.
 */
void
ComputerPlayerTest::testGetCommand() {
    std::vector<Position> nullHist;

    Position pos(TextIO::readFEN("7k/5Q2/p5K1/8/8/8/8/8 b - - 99 80"));
    ComputerPlayer cp;
    cp.maxDepth = 1;
    cp.maxTimeMillis = -1;
    cp.verbose = false;
    std::string result = cp.getCommand(pos, false, nullHist);
    ASSERT_EQUAL("a5", result);     // Only one legal move

    pos = TextIO::readFEN("7k/5Q2/p5K1/8/8/8/8/8 b - - 100 80");
    result = cp.getCommand(pos, false, nullHist);
    ASSERT_EQUAL("draw 50", result);    // Should claim draw without making a move

    pos = TextIO::readFEN("3k4/1R6/R7/8/8/8/8/1K6 w - - 100 80");
    result = cp.getCommand(pos, false, nullHist);
    ASSERT_EQUAL("Ra8#", result);       // Can claim draw, but should not

    pos = TextIO::readFEN("8/1R5k/R7/8/8/8/B7/1K6 b - - 99 80");
    result = cp.getCommand(pos, false, nullHist);
    ASSERT_EQUAL("draw 50 Kh8", result);     // Should claim draw by 50-move rule

    // Only one possible move. Should realize that draw claim is possible, but very bad
    pos = TextIO::readFEN("6Nk/8/5K1R/q7/q7/q7/8/8 b - - 100 80");
    result = cp.getCommand(pos, false, nullHist);
    ASSERT_EQUAL("Kxg8", result);
}

/**
 * Test of draw by repetition, of class ComputerPlayer.
 */
void
ComputerPlayerTest::testDrawRep() {
    HumanPlayer hp1, hp2;

    Game game(make_unique<HumanPlayer>(), make_unique<HumanPlayer>());
    ComputerPlayer cp;
    cp.maxDepth = 3;
    cp.maxTimeMillis = -1;
    cp.verbose = false;
    game.processString("setpos 7k/5RR1/8/8/8/8/q3q3/2K5 w - - 0 1");
    game.processString("Rh7");
    game.processString("Kg8");
    game.processString("Rhg7");
    std::vector<Position> hist;
    game.getHistory(hist);
    std::string result = cp.getCommand(game.getPos(), false, hist);
    ASSERT_EQUAL("Kh8", result); // Not valid to claim draw here
    game.processString("Kh8");
    game.processString("Rh7");
    game.processString("Kg8");
    game.processString("Rhg7");
    game.getHistory(hist);
    result = cp.getCommand(game.getPos(), false, hist);
    ASSERT_EQUAL("draw rep Kh8", result);   // Can't win, but can claim draw.

    game.processString("setpos 7k/R7/1R6/8/8/8/8/K7 w - - 0 1");
    game.processString("Ra8");
    game.processString("Kh7");
    game.getHistory(hist);
    result = cp.getCommand(game.getPos(), false, hist);
    ASSERT_EQUAL("Ra7+", result);       // Ra7 is mate-in-two
    game.processString("Ra7");
    game.processString("Kh8");
    game.processString("Ra8");
    game.processString("Kh7");
    game.getHistory(hist);
    result = cp.getCommand(game.getPos(), false, hist);
    ASSERT(result != "Ra7+"); // Ra7 now leads to a draw by repetition
}

cute::suite
ComputerPlayerTest::getSuite() const {
    cute::suite s;
    s.push_back(CUTE(testGetCommand));
    s.push_back(CUTE(testDrawRep));
    return s;
}
