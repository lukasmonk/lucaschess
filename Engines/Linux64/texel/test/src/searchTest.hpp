/*
    Texel - A UCI chess engine.
    Copyright (C) 2012,2014  Peter Österlund, peterosterlund2@gmail.com

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
 * searchTest.hpp
 *
 *  Created on: Feb 25, 2012
 *      Author: petero
 */

#ifndef SEARCHTEST_HPP_
#define SEARCHTEST_HPP_

#include "transpositionTable.hpp"
#include "search.hpp"

#include "suiteBase.hpp"

class Move;

class SearchTest : public SuiteBase {
public:
    std::string getName() const override { return "SearchTest"; }

    cute::suite getSuite() const override;

    static Move idSearch(Search& sc, int maxDepth, int minProbeDepth = 100);

    static std::vector<U64> nullHist;
    static TranspositionTable tt;
    static ParallelData pd;
    static Search::SearchTables st;
    static TreeLogger treeLog;

private:
    static void testNegaScout();
    static void testDraw50();
    static void testDrawRep();
    static void testHashing();
    static void testLMP();
    static void testCheckEvasion();
    static void testStalemateTrap();
    static void testKQKRNullMove();
    static int getSEE(Search& sc, const Move& m);
    static void testSEE();
    static void testScoreMoveList();
    static void testTBSearch();
    static void testFortress();
};

#endif /* SEARCHTEST_HPP_ */
