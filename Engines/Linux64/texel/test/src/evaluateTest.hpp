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
 * evaluateTest.hpp
 *
 *  Created on: Feb 25, 2012
 *      Author: petero
 */

#ifndef EVALUATETEST_HPP_
#define EVALUATETEST_HPP_

#include "suiteBase.hpp"

class EvaluateTest : public SuiteBase {
public:
    std::string getName() const override { return "EvaluateTest"; }

    cute::suite getSuite() const override;

private:
    static void testEvalPos();
    static void testPieceSquareEval();
    static void testTradeBonus();
    static void testMaterial();
    static void testKingSafety();
    static void testEndGameEval();
    static void testEndGameSymmetry();
    static void testEndGameCorrections();
    static void testPassedPawns();
    static void testBishAndRookPawns();
    static void testBishAndPawnFortress();
    static void testTrappedBishop();
    static void testKQKP();
    static void testKQKRP();
    static void testKRKP();
    static void testKRPKR();
    static void testKPK();
    static void testKPKP();
    static void testKBNK();
    static void testKBPKB();
    static void testKBPKN();
    static void testKNPKB();
    static void testKNPK();
    static void testCantWin();
    static void testPawnRace();
    static void testKnightOutPost();
    static void testUciParam();
    static void testUciParamTable();
    static void testSwindleScore();
    static void testStalePawns();
    static int getNContactChecks(const std::string& fen);
    static void testContactChecks();
};

class Position;
class Evaluate;
int swapSquareY(int square);
Position swapColors(const Position& pos);
Position mirrorX(const Position& pos);
int evalWhite(const Position& pos, bool testMirror = false);
int evalWhite(Evaluate& eval, const Position& pos, bool testMirror = false);


#endif /* EVALUATETEST_HPP_ */
