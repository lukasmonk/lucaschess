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
 * gameTest.hpp
 *
 *  Created on: Feb 25, 2012
 *      Author: petero
 */

#ifndef GAMETEST_HPP_
#define GAMETEST_HPP_

#include "suiteBase.hpp"
#include "util/util.hpp"

class Position;
class Evaluate;

class GameTest : public SuiteBase {
public:
    std::string getName() const override { return "GameTest"; }

    cute::suite getSuite() const override;

private:
    static void testHaveDrawOffer();
    static void testDraw50();
    static void testDrawRep();
    static void testResign();
    static void testProcessString();
    static void testGetGameState();
    static void testInsufficientMaterial();
    static void doTestPerfTFast(Position& pos, int maxDepth, U64 expectedNodeCounts[]);
    static void doTestPerfTExtensive(Position& pos, int maxDepth, U64 expectedNodeCounts[]);
    static U64 perfT(Position& pos, int depth, Evaluate& eval);
    static void testPerfT();
};

#endif /* GAMETEST_HPP_ */
