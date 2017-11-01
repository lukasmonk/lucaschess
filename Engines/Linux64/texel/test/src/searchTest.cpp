/*
    Texel - A UCI chess engine.
    Copyright (C) 2012-2016  Peter Österlund, peterosterlund2@gmail.com

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
 * searchTest.cpp
 *
 *  Created on: Feb 25, 2012
 *      Author: petero
 */

#include "searchTest.hpp"
#include "evaluateTest.hpp"
#include "positionTest.hpp"
#include "tbTest.hpp"
#include "constants.hpp"
#include "position.hpp"
#include "moveGen.hpp"
#include "move.hpp"
#include "textio.hpp"

#include <vector>
#include <memory>

#include "cute.h"

std::vector<U64> SearchTest::nullHist(200);
TranspositionTable SearchTest::tt(19);
ParallelData SearchTest::pd(SearchTest::tt);
static KillerTable kt;
static History ht;
static auto et = Evaluate::getEvalHashTables();
Search::SearchTables SearchTest::st(tt, kt, ht, *et);
TreeLogger SearchTest::treeLog;

Move
SearchTest::idSearch(Search& sc, int maxDepth, int minProbeDepth) {
    MoveList moves;
    MoveGen::pseudoLegalMoves(sc.pos, moves);
    MoveGen::removeIllegal(sc.pos, moves);
    sc.scoreMoveList(moves, 0);
    sc.timeLimit(-1, -1);
    Move bestM = sc.iterativeDeepening(moves, maxDepth, -1, false, 1, false, minProbeDepth);
    ASSERT_EQUAL(sc.pos.materialId(), PositionTest::computeMaterialId(sc.pos));
    return bestM;
}

/**
 * Test of negaScout method, of class Search.
 */
void
SearchTest::testNegaScout() {
    const int ply = 1;
    const int mate0 = SearchConst::MATE0;

    Position pos = TextIO::readFEN("3k4/8/3K2R1/8/8/8/8/8 w - - 0 1");
    Search sc(pos, nullHist, 0, st, pd, nullptr, treeLog);
    sc.setMinProbeDepth(100);
    int score = sc.negaScout(-mate0, mate0, ply, 2, -1, MoveGen::inCheck(pos)) + ply;
    ASSERT_EQUAL(mate0 - 2, score);     // depth 2 is enough to find mate in 1
    int score2 = idSearch(sc, 2).score();
    ASSERT_EQUAL(score, score2);

    pos = TextIO::readFEN("8/1P6/k7/2K5/8/8/8/8 w - - 0 1");
    sc.init(pos, nullHist, 0);
    sc.setMinProbeDepth(100);
    score = sc.negaScout(-mate0, mate0, ply, 4, -1, MoveGen::inCheck(pos)) + ply;
    ASSERT_EQUAL(mate0 - 4, score);     // depth 4 is enough to find mate in 2
    score2 = idSearch(sc, 4).score();
    ASSERT_EQUAL(score, score2);

    pos = TextIO::readFEN("8/5P1k/5K2/8/8/8/8/8 w - - 0 1");
    sc.init(pos, nullHist, 0);
    sc.setMinProbeDepth(100);
    score = sc.negaScout(-mate0, mate0, ply, 5, -1, MoveGen::inCheck(pos)) + ply;
    ASSERT_EQUAL(mate0 - 4, score);     // must avoid stale-mate after f8Q
    score2 = idSearch(sc, 5).score();
    ASSERT_EQUAL(score, score2);

    pos = TextIO::readFEN("4k3/8/3K1Q2/8/8/8/8/8 b - - 0 1");
    sc.init(pos, nullHist, 0);
    sc.setMinProbeDepth(100);
    score = sc.negaScout(-mate0, mate0, ply, 2, -1, MoveGen::inCheck(pos));
    ASSERT_EQUAL(0, score);             // Position is stale-mate

    pos = TextIO::readFEN("3kB3/8/1N1K4/8/8/8/8/8 w - - 0 1");
    sc.init(pos, nullHist, 0);
    sc.setMinProbeDepth(100);
    score = sc.negaScout(-mate0, mate0, ply, 3, -1, MoveGen::inCheck(pos));
    ASSERT(std::abs(score) < 50);   // Stale-mate trap
    score2 = idSearch(sc, 5).score();
    ASSERT_EQUAL(score, score2);

    pos = TextIO::readFEN("8/8/2K5/3QP3/P6P/1q6/8/k7 w - - 31 51");
    sc.init(pos, nullHist, 0);
    sc.setMinProbeDepth(100);
    Move bestM = idSearch(sc, 2);
    ASSERT(TextIO::moveToString(pos, bestM, false) != "Qxb3");
}

/**
 * Test of draw by 50 move rule, of class Search.
 */
void
SearchTest::testDraw50() {
    const int ply = 1;
    const int mate0 = SearchConst::MATE0;
    const int mateInOne = mate0 - 2;
    const int matedInOne = -mate0 + 3;
    const int mateInTwo = mate0 - 4;
    const int mateInThree = mate0 - 6;

    Position pos = TextIO::readFEN("8/1R2k3/R7/8/8/8/8/1K6 b - - 0 1");
    Search sc(pos, nullHist, 0, st, pd, nullptr, treeLog);
    sc.maxTimeMillis = -1;
    sc.setMinProbeDepth(100);
    int score = sc.negaScout(-mate0, mate0, ply, 2, -1, MoveGen::inCheck(pos));
    ASSERT_EQUAL(matedInOne, score - ply);

    pos = TextIO::readFEN("8/1R2k3/R7/8/8/8/8/1K6 b - - 99 80");
    sc.init(pos, nullHist, 0);
    sc.maxTimeMillis = -1;
    sc.setMinProbeDepth(100);
    score = sc.negaScout(-mate0, mate0, ply, 2, -1, MoveGen::inCheck(pos));
    ASSERT_EQUAL(0, score);     // Draw by 50-move rule

    pos = TextIO::readFEN("8/1R2k3/R7/8/8/8/8/1K6 b - - 98 80");
    sc.init(pos, nullHist, 0);
    sc.maxTimeMillis = -1;
    sc.setMinProbeDepth(100);
    score = sc.negaScout(-mate0, mate0, ply, 2, -1, MoveGen::inCheck(pos));
    ASSERT_EQUAL(matedInOne, score - ply);     // No draw

    pos = TextIO::readFEN("8/1R2k3/R7/8/8/8/8/1K6 b - - 99 80");
    sc.init(pos, nullHist, 0);
    sc.maxTimeMillis = -1;
    sc.setMinProbeDepth(100);
    score = idSearch(sc, 3).score();
    ASSERT_EQUAL(0, score);

    pos = TextIO::readFEN("3k4/1R6/R7/8/8/8/8/1K6 w - - 100 80");
    sc.init(pos, nullHist, 0);
    sc.maxTimeMillis = -1;
    sc.setMinProbeDepth(100);
    score = idSearch(sc, 2).score();
    ASSERT_EQUAL(mateInOne, score); // Black forgot to claim draw. Now it's too late.

    pos = TextIO::readFEN("8/7k/1R6/R7/8/7P/8/1K6 w - - 0 1");
    sc.init(pos, nullHist, 0);
    sc.maxTimeMillis = -1;
    sc.setMinProbeDepth(100);
    score = idSearch(sc, 3).score();
    ASSERT_EQUAL(mateInTwo, score);

    pos = TextIO::readFEN("8/7k/1R6/R7/8/7P/8/1K6 w - - 98 1");
    sc.init(pos, nullHist, 0);
    sc.maxTimeMillis = -1;
    sc.setMinProbeDepth(100);
    score = idSearch(sc, 6).score();
    ASSERT_EQUAL(mateInThree, score);   // Need an extra pawn move to avoid 50-move rule

    pos = TextIO::readFEN("8/7k/1R6/R7/8/7P/8/1K6 w - - 125 1");
    sc.init(pos, nullHist, 0);
    sc.maxTimeMillis = -1;
    sc.setMinProbeDepth(100);
    score = idSearch(sc, 6).score();
    ASSERT_EQUAL(mateInThree, score);   // Need an extra pawn move to avoid 50-move rule

    pos = TextIO::readFEN("3k4/8/2R1K3/8/8/8/8/8 w - - 97 1");
    sc.init(pos, nullHist, 0);
    sc.maxTimeMillis = -1;
    sc.setMinProbeDepth(100);
    score = idSearch(sc, 3).score();
    ASSERT_EQUAL(mateInTwo, score);   // White can claim draw or deliver mate at second move

    pos = TextIO::readFEN("3k4/8/2R1K3/8/8/8/8/8 w - - 98 1");
    sc.init(pos, nullHist, 0);
    sc.maxTimeMillis = -1;
    sc.setMinProbeDepth(100);
    score = idSearch(sc, 3).score();
    ASSERT_EQUAL(0, score);   // Black can claim draw at first move
}

/**
 * Test of draw by repetition rule, of class Search.
 */
void
SearchTest::testDrawRep() {
    const int ply = 1;
    const int mate0 = SearchConst::MATE0;
    Position pos = TextIO::readFEN("7k/5RR1/8/8/8/8/q3q3/2K5 w - - 0 1");
    std::shared_ptr<Search> sc = std::make_shared<Search>(pos, nullHist, 0, st, pd, nullptr, treeLog);
    sc->maxTimeMillis = -1;
    sc->setMinProbeDepth(100);
    int score = sc->negaScout(-mate0, mate0, ply, 3, -1, MoveGen::inCheck(pos));
    ASSERT_EQUAL(0, score);

    pos = TextIO::readFEN("7k/5RR1/8/8/8/8/q3q3/2K5 w - - 0 1");
    sc = std::make_shared<Search>(pos, nullHist, 0, st, pd, nullptr, treeLog);
    sc->maxTimeMillis = -1;
    sc->setMinProbeDepth(100);
    score = idSearch(*sc.get(), 3).score();
    ASSERT_EQUAL(0, score);

    pos = TextIO::readFEN("7k/5RR1/8/8/8/8/1q3q2/3K4 w - - 0 1");
    sc = std::make_shared<Search>(pos, nullHist, 0, st, pd, nullptr, treeLog);
    sc->maxTimeMillis = -1;
    sc->setMinProbeDepth(100);
    score = idSearch(*sc.get(), 4).score();
    ASSERT(score < 0);

    pos = TextIO::readFEN("7k/5RR1/8/8/8/8/1q3q2/3K4 w - - 0 1");
    sc = std::make_shared<Search>(pos, nullHist, 0, st, pd, nullptr, treeLog);
    sc->maxTimeMillis = -1;
    sc->setMinProbeDepth(100);
    score = sc->negaScout(-mate0, mate0, ply, 3, -1, MoveGen::inCheck(pos));
    ASSERT(score < 0);

    pos = TextIO::readFEN("qn6/qn4k1/pp3R2/5R2/8/8/8/K7 w - - 0 1");
    sc = std::make_shared<Search>(pos, nullHist, 0, st, pd, nullptr, treeLog);
    sc->maxTimeMillis = -1;
    sc->setMinProbeDepth(100);
    score = idSearch(*sc.get(), 9).score();
    ASSERT_EQUAL(0, score); // Draw, black can not escape from perpetual checks
}

/**
 * Test of hash table, of class Search.
 */
void
SearchTest::testHashing() {
    Position pos = TextIO::readFEN("/k/3p/p2P1p/P2P1P///K w - -");  // Fine #70
    Search sc(pos, nullHist, 0, st, pd, nullptr, treeLog);
    sc.setMinProbeDepth(100);
    Move bestM = idSearch(sc, 28);
    ASSERT_EQUAL(TextIO::stringToMove(pos, "Kb1"), bestM);
}

void
SearchTest::testLMP() {
    Position pos(TextIO::readFEN("2r2rk1/6p1/p3pq1p/1p1b1p2/3P1n2/PP3N2/3N1PPP/1Q2RR1K b"));  // WAC 174
    Search sc(pos, nullHist, 0, st, pd, nullptr, treeLog);
    sc.setMinProbeDepth(100);
    Move bestM = idSearch(sc, 2);
    ASSERT(!SearchConst::isWinScore(bestM.score()));
}

void
SearchTest::testCheckEvasion() {
    Position pos = TextIO::readFEN("6r1/R5PK/2p5/1k6/8/8/p7/8 b - - 0 62");
    Search sc(pos, nullHist, 0, st, pd, nullptr, treeLog);
    Move bestM = idSearch(sc, 3);
    ASSERT(bestM.score() < 0);

    pos = TextIO::readFEN("r1bq2rk/pp3pbp/2p1p1pQ/7P/3P4/2PB1N2/PP3PPR/2KR4 w - -"); // WAC 004
    sc.init(pos, nullHist, 0);
    sc.setMinProbeDepth(100);
    bestM = idSearch(sc, 1);
    ASSERT_EQUAL(SearchConst::MATE0 - 4, bestM.score());
    ASSERT_EQUAL(TextIO::stringToMove(pos, "Qxh7+"), bestM);
}

void
SearchTest::testStalemateTrap() {
    Position pos = TextIO::readFEN("7k/1P3R1P/6r1/5K2/8/8/6R1/8 b - - 98 194");
    Search sc(pos, nullHist, 0, st, pd, nullptr, treeLog);
    sc.setMinProbeDepth(100);
    Move bestM = idSearch(sc, 3);
    ASSERT_EQUAL(0, bestM.score());
}

void
SearchTest::testKQKRNullMove() {
    Position pos = TextIO::readFEN("7K/6R1/5k2/3q4/8/8/8/8 b - - 0 1");
    Search sc(pos, nullHist, 0, st, pd, nullptr, treeLog);
    sc.setMinProbeDepth(100);
    Move bestM = idSearch(sc, 12);
    ASSERT_EQUAL(SearchConst::MATE0-18, bestM.score());
}

/** Compute SEE(m) and assure that signSEE and negSEE give matching results. */
int
SearchTest::getSEE(Search& sc, const Move& m) {
    int see = sc.SEE(m);
    bool neg = sc.negSEE(m);
    ASSERT_EQUAL(neg, see < 0);
    int sign = sc.signSEE(m);
    if (sign > 0) {
        ASSERT(see > 0);
    } else if (sign == 0) {
        ASSERT_EQUAL(0, see);
    } else {
        ASSERT(see < 0);
    }
    return see;
}

/**
 * Test of SEE method, of class Search.
 */
void
SearchTest::testSEE() {
    const int pV = ::pV;
    const int nV = ::nV;
    const int bV = ::bV;
    const int rV = ::rV;

    // Basic tests
    Position pos = TextIO::readFEN("r2qk2r/ppp2ppp/1bnp1nb1/1N2p3/3PP3/1PP2N2/1P3PPP/R1BQRBK1 w kq - 0 1");
    Search sc(pos, nullHist, 0, st, pd, nullptr, treeLog);
    ASSERT_EQUAL(0, getSEE(sc, TextIO::stringToMove(pos, "dxe5")));
    ASSERT_EQUAL(pV - nV, getSEE(sc, TextIO::stringToMove(pos, "Nxe5")));
    ASSERT_EQUAL(pV - rV, getSEE(sc, TextIO::stringToMove(pos, "Rxa7")));
    ASSERT_EQUAL(pV - nV, getSEE(sc, TextIO::stringToMove(pos, "Nxa7")));
    ASSERT_EQUAL(pV - nV, getSEE(sc, TextIO::stringToMove(pos, "Nxd6")));
    ASSERT_EQUAL(0, getSEE(sc, TextIO::stringToMove(pos, "d5")));
    ASSERT_EQUAL(-bV, getSEE(sc, TextIO::stringToMove(pos, "Bf4")));
    ASSERT_EQUAL(-bV, getSEE(sc, TextIO::stringToMove(pos, "Bh6")));
    ASSERT_EQUAL(-rV, getSEE(sc, TextIO::stringToMove(pos, "Ra5")));
    ASSERT_EQUAL(-rV, getSEE(sc, TextIO::stringToMove(pos, "Ra6")));

    pos.setWhiteMove(false);
    sc.init(pos, nullHist, 0);
    ASSERT(nV <= bV);   // Assumed by following test
    ASSERT_EQUAL(pV - nV, getSEE(sc, TextIO::stringToMove(pos, "Nxd4")));
    ASSERT_EQUAL(pV - bV, getSEE(sc, TextIO::stringToMove(pos, "Bxd4")));
    ASSERT_EQUAL(0, getSEE(sc, TextIO::stringToMove(pos, "exd4")));
    ASSERT_EQUAL(pV, getSEE(sc, TextIO::stringToMove(pos, "Nxe4")));
    ASSERT_EQUAL(pV, getSEE(sc, TextIO::stringToMove(pos, "Bxe4")));
    ASSERT_EQUAL(0, getSEE(sc, TextIO::stringToMove(pos, "d5")));
    ASSERT_EQUAL(-nV, getSEE(sc, TextIO::stringToMove(pos, "Nd5")));
    ASSERT_EQUAL(0, getSEE(sc, TextIO::stringToMove(pos, "a6")));

    // Test X-ray attacks
    pos = TextIO::readFEN("3r2k1/pp1q1ppp/1bnr1nb1/1Np1p3/1P1PP3/2P1BN2/1Q1R1PPP/3R1BK1 b - - 0 1");
    sc.init(pos, nullHist, 0);
    // 1 1 1 1 3 3 3 3 3 5 5 9 5 5
    // 0 1 0 1 0 3 0 3 0 5 0 9 0 5
    ASSERT_EQUAL(0, getSEE(sc, TextIO::stringToMove(pos, "exd4")));
    // 1 3 1 1 3 1 3 3 5 5 5 9 9
    //-1 2 1 0 3 0 3 0 5 0 5 0 9
    ASSERT_EQUAL(2 * pV - nV, getSEE(sc, TextIO::stringToMove(pos, "Nxd4")));

    pos.setPiece(TextIO::getSquare("b2"), Piece::EMPTY);  // Remove white queen
    sc.init(pos, nullHist, 0);
    // 1 1 1 1 3 3 3 3 3 5 5 9 5
    // 0 1 0 1 0 3 0 3 0 4 1 4 5
    ASSERT_EQUAL(0, getSEE(sc, TextIO::stringToMove(pos, "exd4")));
    ASSERT_EQUAL(pV, getSEE(sc, TextIO::stringToMove(pos, "cxb4")));

    pos.setPiece(TextIO::getSquare("b5"), Piece::EMPTY);  // Remove white knight
    sc.init(pos, nullHist, 0);
    // 1 1 1 1 3 3 3 3 5 5 5
    // 1 0 1 0 3 0 3 0 5 0 5
    ASSERT_EQUAL(pV, getSEE(sc, TextIO::stringToMove(pos, "exd4")));

    pos.setPiece(TextIO::getSquare("b2"), Piece::WQUEEN);  // Restore white queen
    sc.init(pos, nullHist, 0);
    // 1 1 1 1 3 3 3 3 5 5 5 9 9
    // 1 0 1 0 3 0 3 0 5 0 5 0 9
    ASSERT_EQUAL(pV, getSEE(sc, TextIO::stringToMove(pos, "exd4")));

    pos.setPiece(TextIO::getSquare("b6"), Piece::EMPTY);  // Remove black bishop
    pos.setPiece(TextIO::getSquare("c6"), Piece::EMPTY);  // Remove black knight
    sc.init(pos, nullHist, 0);
    ASSERT_EQUAL(-pV, getSEE(sc, TextIO::stringToMove(pos, "a5")));

    // Test EP capture
    pos = TextIO::readFEN("2b3k1/1p3ppp/8/pP6/8/2PB4/5PPP/6K1 w - a6 0 2");
    sc.init(pos, nullHist, 0);
    // 1 1 1 3
    // 0 1 0 3
    ASSERT_EQUAL(0, getSEE(sc, TextIO::stringToMove(pos, "bxa6")));

    pos.setPiece(TextIO::getSquare("b7"), Piece::EMPTY);  // Remove black pawn
    sc.init(pos, nullHist, 0);
    // 1 1 3
    // 1 0 3
    ASSERT_EQUAL(pV, getSEE(sc, TextIO::stringToMove(pos, "bxa6")));


    // Test king capture
    pos = TextIO::readFEN("8/8/8/4k3/r3P3/4K3/8/4R3 b - - 0 1");
    sc.init(pos, nullHist, 0);
    // 1 5 99
    // 1 0 99
    ASSERT_EQUAL(pV, getSEE(sc, TextIO::stringToMove(pos, "Rxe4+")));

    pos = TextIO::readFEN("8/8/8/4k3/r3P1R1/4K3/8/8 b - - 0 1");
    const int kV = ::kV;
    sc.init(pos, nullHist, 0);
    // 1 5 5 99
    //-4 5 0 99
    ASSERT_EQUAL(pV - rV, getSEE(sc, TextIO::stringToMove(pos, "Rxe4+")));
    //  1 99
    //-98 99
    ASSERT_EQUAL(pV - kV, getSEE(sc, Move(TextIO::getSquare("e5"), TextIO::getSquare("e4"), Piece::EMPTY)));

    pos = TextIO::readFEN("8/8/4k3/8/r3P3/4K3/8/8 b - - 0 1");
    sc.init(pos, nullHist, 0);
    ASSERT_EQUAL(pV - rV, getSEE(sc, TextIO::stringToMove(pos, "Rxe4+")));

    // Test king too far away
    pos = TextIO::readFEN("8/8/4k3/8/r3P3/8/4K3/8 b - - 0 1");
    sc.init(pos, nullHist, 0);
    ASSERT_EQUAL(pV, getSEE(sc, TextIO::stringToMove(pos, "Rxe4+")));

    pos = TextIO::readFEN("8/3k4/8/8/r1K5/8/8/2R5 w - - 0 1");
    pos.setWhiteMove(false);
    sc.init(pos, nullHist, 0);
    ASSERT_EQUAL(kV, getSEE(sc, Move(TextIO::getSquare("a4"), TextIO::getSquare("c4"), Piece::EMPTY)));


    // Test blocking pieces
    pos = TextIO::readFEN("r7/p2k4/8/r7/P7/8/4K3/R7 b - - 0 1");
    sc.init(pos, nullHist, 0);
    ASSERT_EQUAL(pV - rV, getSEE(sc, TextIO::stringToMove(pos, "Rxa4")));    // Ra8 doesn't help

    pos.setPiece(TextIO::getSquare("a7"), Piece::BBISHOP);
    sc.init(pos, nullHist, 0);
    ASSERT_EQUAL(pV - rV, getSEE(sc, TextIO::stringToMove(pos, "Rxa4")));    // Ra8 doesn't help

    pos.setPiece(TextIO::getSquare("a7"), Piece::BPAWN);
    sc.init(pos, nullHist, 0);
    ASSERT_EQUAL(pV - rV, getSEE(sc, TextIO::stringToMove(pos, "Rxa4")));    // Ra8 doesn't help

    pos.setPiece(TextIO::getSquare("a7"), Piece::BQUEEN);
    sc.init(pos, nullHist, 0);
    ASSERT_EQUAL(pV, getSEE(sc, TextIO::stringToMove(pos, "Rxa4")));         // Ra8 does help

    pos = TextIO::readFEN("8/3k4/R7/r7/P7/8/4K3/8 b - - 0 1");
    sc.init(pos, nullHist, 0);
    ASSERT_EQUAL(pV - rV, getSEE(sc, TextIO::stringToMove(pos, "Rxa4")));

    pos = TextIO::readFEN("Q7/q6k/R7/r7/P7/8/4K3/8 b - - 0 1");
    sc.init(pos, nullHist, 0);
    ASSERT_EQUAL(pV - rV, getSEE(sc, TextIO::stringToMove(pos, "Rxa4")));

    pos = TextIO::readFEN("8/3k4/5R2/8/4pP2/8/8/3K4 b - f3 0 1");
    sc.init(pos, nullHist, 0);
    int score1 = evalWhite(sc.pos);
    U64 h1 = sc.pos.zobristHash();
    ASSERT_EQUAL(0, getSEE(sc, TextIO::stringToMove(pos, "exf3")));
    int score2 = evalWhite(sc.pos);
    U64 h2 = sc.pos.zobristHash();
    ASSERT_EQUAL(score1, score2);
    ASSERT_EQUAL(h1, h2);
}

/**
 * Test of scoreMoveList method, of class Search.
 */
void
SearchTest::testScoreMoveList() {
    Position pos = TextIO::readFEN("r2qk2r/ppp2ppp/1bnp1nb1/1N2p3/3PP3/1PP2N2/1P3PPP/R1BQRBK1 w kq - 0 1");
    Search sc(pos, nullHist, 0, st, pd, nullptr, treeLog);
    MoveList moves;
    MoveGen::pseudoLegalMoves(pos, moves);
    sc.scoreMoveList(moves, 0);
    for (int i = 0; i < moves.size; i++) {
        Search::selectBest(moves, i);
        if (i > 0) {
            int sc1 = moves[i - 1].score();
            int sc2 = moves[i].score();
            ASSERT(sc2 <= sc1);
        }
    }

    moves.clear();
    MoveGen::pseudoLegalMoves(pos, moves);
    moves[0].setScore(17);
    moves[1].setScore(666);
    moves[2].setScore(4711);
    sc.scoreMoveList(moves, 0, 2);
    ASSERT_EQUAL(17, moves[0].score());
    ASSERT_EQUAL(666, moves[1].score());
    for (int i = 1; i < moves.size; i++) {
        Search::selectBest(moves, i);
        if (i > 1) {
            int sc1 = moves[i - 1].score();
            int sc2 = moves[i].score();
            ASSERT(sc2 <= sc1);
        }
    }

    // The hashMove should be first in the list
    Move m = TextIO::stringToMove(pos, "Ra6");
    moves.clear();
    MoveGen::pseudoLegalMoves(pos, moves);
    bool res = Search::selectHashMove(moves, m);
    ASSERT_EQUAL(true, res);
    ASSERT_EQUAL(m, moves[0]);
}

void
SearchTest::testTBSearch() {
    const int mate0 = SearchConst::MATE0;
    Position pos = TextIO::readFEN("R5Q1/8/6k1/8/4q3/8/8/K7 b - - 0 1"); // DTM path wins
    Search sc(pos, nullHist, 0, st, pd, nullptr, treeLog);
    int score = idSearch(sc, 4, 2).score();
    ASSERT_EQUAL(-(mate0 - 23), score);

    pos = TextIO::readFEN("R5Q1/8/6k1/8/4q3/8/8/K7 b - - 92 1"); // DTZ path needed
    sc.init(pos, nullHist, 0);
    score = idSearch(sc, 6, 1).score();
    ASSERT(SearchConst::isLoseScore(score));
    ASSERT(score > -(mate0 - 23));

    {
        TBTest::initTB("", 0, rtbDefaultPath);
        pos = TextIO::readFEN("R5Q1/8/6k1/8/4q3/8/8/K7 b - - 93 1"); // No way to avoid draw
        sc.init(pos, nullHist, 0);
        tt.clear();
        score = idSearch(sc, 4, 3).score();
        ASSERT(std::abs(score) < 900);
        tt.clear();
    }

    {
        TBTest::initTB("", 0, "");
        pos = TextIO::readFEN("8/8/8/3rk3/8/8/8/KQ6 w - - 0 1"); // KQKR long mate
        sc.init(pos, nullHist, 0);
        MoveList moves;
        MoveGen::pseudoLegalMoves(sc.pos, moves);
        MoveGen::removeIllegal(sc.pos, moves);
        sc.scoreMoveList(moves, 0);
        sc.timeLimit(10000, 20000); // Should take less than 2s to generate the TB
        Move bestM = sc.iterativeDeepening(moves, -1, -1, false, 1, false, -1);
        ASSERT_EQUAL(sc.pos.materialId(), PositionTest::computeMaterialId(sc.pos));
        ASSERT_EQUAL(mate0 - 33 * 2, bestM.score());
        TBTest::initTB(gtbDefaultPath, gtbDefaultCacheMB, rtbDefaultPath);
        tt.clear();
    }
}

void
SearchTest::testFortress() {
    Position pos = TextIO::readFEN("3B4/1r2p3/r2p1p2/bkp1P1p1/1p1P1PPp/p1P4P/PPB1K3/8 w - - 0 1");
    Search sc(pos, nullHist, 0, st, pd, nullptr, treeLog);
    Move bestM = idSearch(sc, 10);
    ASSERT(TextIO::moveToUCIString(bestM) == "c2a4");
    ASSERT(bestM.score() > -600);
}

cute::suite
SearchTest::getSuite() const {
    cute::suite s;
    s.push_back(CUTE(testNegaScout));
    s.push_back(CUTE(testDraw50));
    s.push_back(CUTE(testDrawRep));
    s.push_back(CUTE(testHashing));
    s.push_back(CUTE(testLMP));
    s.push_back(CUTE(testCheckEvasion));
    s.push_back(CUTE(testStalemateTrap));
    s.push_back(CUTE(testKQKRNullMove));
    s.push_back(CUTE(testSEE));
    s.push_back(CUTE(testScoreMoveList));
    s.push_back(CUTE(testTBSearch));
    s.push_back(CUTE(testFortress));
    return s;
}
