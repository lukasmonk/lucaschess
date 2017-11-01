/*
    Texel - A UCI chess engine.
    Copyright (C) 2013-2015  Peter Österlund, peterosterlund2@gmail.com

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
 * parallelTest.cpp
 *
 *  Created on: Jul 17, 2013
 *      Author: petero
 */

#define _GLIBCXX_USE_NANOSLEEP

#include "parallelTest.hpp"
#include "parallel.hpp"
#include "position.hpp"
#include "textio.hpp"
#include "searchUtil.hpp"

#include <vector>
#include <memory>
#include <thread>
#include <chrono>

#include "cute.h"

using namespace SearchConst;


void
ParallelTest::testFailHighInfo() {
    const double eps = 1e-8;
    FailHighInfo fhi;

    // Probability 0.5 when no data available
    for (int i = 0; i < 20; i++) {
        for (int j = 0; i < 20; i++) {
            ASSERT_EQUAL_DELTA(0.5, fhi.getMoveNeededProbability(0, i, j, true), eps);
            ASSERT_EQUAL_DELTA(0.5, fhi.getMoveNeededProbability(1, i, j, true), eps);
        }
    }

    for (int i = 0; i < 15; i++)
        fhi.addData(0, i, true, true);
    for (int curr = 0; curr < 15; curr++) {
        for (int m = curr; m < 15; m++) {
            double e = (15 - m) / (double)(15 - curr);
            ASSERT_EQUAL_DELTA(e, fhi.getMoveNeededProbability(0, curr, m, true), eps);
            ASSERT_EQUAL_DELTA(0.5, fhi.getMoveNeededProbability(1, curr, m, true), eps);
        }
    }
    for (int i = 0; i < 15; i++)
        fhi.addData(0, i, false, true);
    for (int curr = 0; curr < 15; curr++) {
        for (int m = curr; m < 15; m++) {
            double e = (15 - m + 15) / (double)(15 - curr + 15);
            ASSERT_EQUAL_DELTA(e, fhi.getMoveNeededProbability(0, curr, m, true), eps);
            ASSERT_EQUAL_DELTA(0.5, fhi.getMoveNeededProbability(1, curr, m, true), eps);
        }
    }

    for (int i = -1; i < 10; i++)
        for (int j = 0; j < (1<<(10-i)); j++)
            fhi.addPvData(i, true);
    for (int curr = 0; curr < 10; curr++) {
        double prob = 1.0;
        for (int m = curr; m < 10; m++) {
            ASSERT_EQUAL_DELTA(prob, fhi.getMoveNeededProbabilityPv(curr, m), eps);
            prob *= 1.0 - (1<<(10-m)) / (double)(1<<11);
        }
    }
}

void
ParallelTest::testNpsInfo() {
    DepthNpsInfo npsInfo;
    const double eps = 1e-8;
    const int nSmooth = 500;
    const int maxDepth = DepthNpsInfo::maxDepth;
    for (int i = 0; i < maxDepth * 2; i++)
        ASSERT_EQUAL_DELTA(1.0, npsInfo.efficiency(i), eps);
    npsInfo.setBaseNps(1000);
    for (int i = 0; i < maxDepth; i++)
        ASSERT_EQUAL_DELTA(1.0, npsInfo.efficiency(i), eps);
    npsInfo.addData(0, 100, 0, 1);
    ASSERT_EQUAL_DELTA((0.1 + nSmooth) / (1 + nSmooth), npsInfo.efficiency(0), eps);
    npsInfo.addData(0, 100, 0, 0.1);
    ASSERT_EQUAL_DELTA((200/1.1/1000 * 2 + nSmooth) / (2 + nSmooth), npsInfo.efficiency(0), eps);

    npsInfo.addData(1, 100, 0, 0.5);
    ASSERT_EQUAL_DELTA((200/1.1/1000 * 2 + nSmooth) / (2 + nSmooth), npsInfo.efficiency(0), eps);
    ASSERT_EQUAL_DELTA((200.0/1000 + nSmooth) / (1 + nSmooth), npsInfo.efficiency(1), eps);

    npsInfo.setBaseNps(190);
    ASSERT_EQUAL_DELTA((200/1.1/190 * 2 + nSmooth) / (2 + nSmooth), npsInfo.efficiency(0), eps);
    ASSERT_EQUAL_DELTA(1.0, npsInfo.efficiency(1), eps);

    npsInfo.reset();
    for (int i = 0; i < maxDepth; i++)
        ASSERT_EQUAL_DELTA(1.0, npsInfo.efficiency(i), eps);

    npsInfo.setBaseNps(1000);
    npsInfo.addData(0, 100, 0.5, 0.5);
    npsInfo.addData(1, 200, 0.3, 1.0);
    ASSERT_EQUAL_DELTA((100/(0.5+(0.5+0.3)/2)/1000 + nSmooth) / (1 + nSmooth), npsInfo.efficiency(0), eps);
    ASSERT_EQUAL_DELTA((200/(1.0+(0.5+0.3)/2)/1000 + nSmooth) / (1 + nSmooth), npsInfo.efficiency(1), eps);
    npsInfo.addData(0, 100, 0.5, 0.5);
    ASSERT_EQUAL_DELTA((200/(1.0+(0.5*2+0.3)/3*2)/1000 * 2 + nSmooth) / (2 + nSmooth), npsInfo.efficiency(0), eps);
}

void
ParallelTest::testWorkQueue() {
    const double eps = 1e-8;
    TranspositionTable tt(10);
    ParallelData pd(tt);
    WorkQueue& wq = pd.wq;
    FailHighInfo& fhi = pd.fhInfo;
    int moveNo = -1;
    double prob = wq.getBestProbability();
    ASSERT_EQUAL_DELTA(0.0, prob, eps);
    ASSERT_EQUAL(0, wq.queue.size());

    for (int m = 0; m < 2; m++) {
        for (int i = 0; i < 10; i++) {
            for (int cnt = 0; cnt < (1<<(9-i)); cnt++) {
                fhi.addData(m, i, true, true);
                if (m > 0)
                    fhi.addData(m, i, false, true);
            }
        }
    }

    std::shared_ptr<SplitPoint> nullRoot;
    Position pos = TextIO::readFEN(TextIO::startPosFEN);
    std::vector<U64> posHashList(200);
    posHashList[0] = pos.zobristHash();
    int posHashListSize = 1;
    SearchTreeInfo sti;
    KillerTable kt;
    History ht;

    auto sp1 = std::make_shared<SplitPoint>(0, nullRoot, 0,
                                            pos, posHashList, posHashListSize,
                                            sti, kt, ht, 10, 11, 1, 1);
    ASSERT_EQUAL(-1, sp1->getNextMove(fhi));
    sp1->addMove(0, SplitPointMove(TextIO::uciStringToMove("e2e4"), 0, 4, -1, false));
    sp1->addMove(1, SplitPointMove(TextIO::uciStringToMove("d2d4"), 0, 4, -1, false));
    sp1->addMove(2, SplitPointMove(TextIO::uciStringToMove("g1f3"), 0, 4, -1, false));
    ASSERT_EQUAL(10, sp1->getAlpha());
    ASSERT_EQUAL(11, sp1->getBeta());
    ASSERT_EQUAL(1, sp1->getPly());
    ASSERT_EQUAL(0, sp1->getChildren().size());
    ASSERT_EQUAL(3, sp1->spMoves.size());
    ASSERT(pos.equals(sp1->pos));

    wq.addWork(sp1);
    ASSERT_EQUAL(1, sp1->findNextMove(fhi));
    ASSERT_EQUAL(1, wq.queue.size());
    ASSERT_EQUAL_DELTA(511 / 1023.0, wq.getBestProbability(), eps);

    std::shared_ptr<SplitPoint> sp = wq.getWork(moveNo, pd, 0);
    ASSERT_EQUAL(1, moveNo);
    ASSERT_EQUAL(sp1, sp);
    ASSERT_EQUAL(2, sp1->findNextMove(fhi));
    ASSERT_EQUAL(1, wq.queue.size());
    ASSERT_EQUAL_DELTA(255 / 1023.0, wq.getBestProbability(), eps);

    int score = wq.setOwnerCurrMove(sp1, 1, 10);
    ASSERT_EQUAL(2, sp1->findNextMove(fhi));
    ASSERT_EQUAL(1, wq.queue.size());
    ASSERT_EQUAL_DELTA(255 / 511.0, wq.getBestProbability(), eps);
    ASSERT_EQUAL(UNKNOWN_SCORE, score);

    sp = wq.getWork(moveNo, pd, 0);
    ASSERT_EQUAL(2, moveNo);
    ASSERT_EQUAL(sp1, sp);
    ASSERT_EQUAL(-1, sp1->findNextMove(fhi));
    ASSERT_EQUAL(1, wq.queue.size());
    ASSERT_EQUAL_DELTA(0.0, wq.getBestProbability(), eps);

    wq.returnMove(sp, 2);
    ASSERT_EQUAL(2, sp1->findNextMove(fhi));
    ASSERT_EQUAL(1, wq.queue.size());
    ASSERT_EQUAL_DELTA(255 / 511.0, wq.getBestProbability(), eps);

    sp = wq.getWork(moveNo, pd, 0);
    ASSERT_EQUAL(2, moveNo);
    ASSERT_EQUAL(sp1, sp);
    ASSERT_EQUAL(-1, sp1->findNextMove(fhi));
    ASSERT_EQUAL(1, wq.queue.size());
    ASSERT_EQUAL_DELTA(0.0, wq.getBestProbability(), eps);

    wq.moveFinished(sp1, 2, false, 17);
    ASSERT_EQUAL(-1, sp1->findNextMove(fhi));
    ASSERT_EQUAL(0, wq.queue.size());
    ASSERT_EQUAL_DELTA(0.0, wq.getBestProbability(), eps);

    score = wq.setOwnerCurrMove(sp1, 2, 5);
    ASSERT_EQUAL(17, score);

    // Split point contains no moves, should not be added to queue/waiting
    sp.reset();
    sp1 = std::make_shared<SplitPoint>(0, nullRoot, 0,
                                       pos, posHashList, posHashListSize,
                                       sti, kt, ht, 10, 11, 1, 1);
    wq.addWork(sp1);
    ASSERT_EQUAL(0, wq.queue.size());

    // Split point contains only one move, should not be added to queue/waiting
    sp1 = std::make_shared<SplitPoint>(0, nullRoot, 0,
                                       pos, posHashList, posHashListSize,
                                       sti, kt, ht, 10, 11, 1, 1);
    sp1->addMove(0, SplitPointMove(TextIO::uciStringToMove("f2f4"), 0, 4, -1, false));
    wq.addWork(sp1);
    ASSERT_EQUAL(0, wq.queue.size());


    // Test return non-last currently searched move
    sp1 = std::make_shared<SplitPoint>(0, nullRoot, 1,
                                       pos, posHashList, posHashListSize,
                                       sti, kt, ht, 10, 11, 1, 1);
    sp1->addMove(0, SplitPointMove(TextIO::uciStringToMove("a2a4"), 0, 4, -1, false));
    sp1->addMove(1, SplitPointMove(TextIO::uciStringToMove("b2b4"), 0, 4, -1, false));
    sp1->addMove(2, SplitPointMove(TextIO::uciStringToMove("c2c4"), 0, 4, -1, false));
    sp1->addMove(3, SplitPointMove(TextIO::uciStringToMove("d2d4"), 0, 4, -1, false));
    wq.addWork(sp1);
    ASSERT_EQUAL(1, wq.queue.size());
    sp = wq.getWork(moveNo, pd, 0);
    sp = wq.getWork(moveNo, pd, 0);
    ASSERT_EQUAL(2, moveNo);
    ASSERT_EQUAL(sp1, sp);
    ASSERT_EQUAL(3, sp1->findNextMove(fhi));
    ASSERT_EQUAL_DELTA((127 + 1023) / (1023.0*2), wq.getBestProbability(), eps);
    wq.returnMove(sp, 1);
    ASSERT_EQUAL(1, sp1->findNextMove(fhi));
    ASSERT_EQUAL_DELTA((511 + 1023) / (1023.0*2), wq.getBestProbability(), eps);
    sp = wq.getWork(moveNo, pd, 0);
    ASSERT_EQUAL(1, moveNo);
    ASSERT_EQUAL(3, sp1->findNextMove(fhi));
    ASSERT_EQUAL_DELTA((127 + 1023) / (1023.0*2), wq.getBestProbability(), eps);
    wq.moveFinished(sp1, 2, true, -123);
    ASSERT_EQUAL(1, wq.queue.size());
    ASSERT_EQUAL_DELTA(0.0, wq.getBestProbability(), eps);
    wq.moveFinished(sp1, 1, true, 4321);
    ASSERT_EQUAL(0, wq.queue.size());
    ASSERT_EQUAL_DELTA(0.0, wq.getBestProbability(), eps);

    score = wq.setOwnerCurrMove(sp, 1, 5);
    ASSERT_EQUAL(4321, score);
    score = wq.setOwnerCurrMove(sp, 2, 5);
    ASSERT_EQUAL(-123, score);
}

static std::vector<std::shared_ptr<SplitPoint>>
extractQueue(Heap<SplitPoint>& heap) {
    std::vector<std::shared_ptr<SplitPoint>> ret;
    std::vector<int> prio;
    while (!heap.empty()) {
        auto e = heap.front();
        ret.push_back(e);
        prio.push_back(e->getPrio());
        heap.remove(e);
    }
    for (int i = 0; i < (int)ret.size(); i++)
        heap.insert(ret[i], prio[i]);
    return ret;
}

void
ParallelTest::testWorkQueueParentChild() {
    const double eps = 1e-8;
    TranspositionTable tt(10);
    ParallelData pd(tt);
    WorkQueue& wq = pd.wq;
    FailHighInfo& fhi = pd.fhInfo;

    for (int m = -1; m < 2; m++) {
        for (int i = 0; i < 10; i++) {
            for (int cnt = 0; cnt < (1<<(9-i)); cnt++) {
                fhi.addData(m, i, true, true);
                if (m <= 0) {
                    fhi.addData(m, i, false, false);
                    if (m == 0) {
                        fhi.addData(m, i, false, true);
                        fhi.addData(m, i, true, false);
                    }
                }
            }
        }
    }

    std::shared_ptr<SplitPoint> nullRoot;
    Position pos = TextIO::readFEN(TextIO::startPosFEN);
    UndoInfo ui;
    std::vector<U64> posHashList(200);
    posHashList[0] = pos.zobristHash();
    int posHashListSize = 1;
    SearchTreeInfo sti;
    KillerTable kt;
    History ht;

    auto sp1 = std::make_shared<SplitPoint>(0, nullRoot, 0,
                                            pos, posHashList, posHashListSize,
                                            sti, kt, ht, 10, 11, 1, 1);
    sp1->addMove(0, SplitPointMove(TextIO::uciStringToMove("e2e4"), 0, 4, -1, false));
    sp1->addMove(1, SplitPointMove(TextIO::uciStringToMove("c2c4"), 0, 4, -1, false));
    sp1->addMove(2, SplitPointMove(TextIO::uciStringToMove("d2d4"), 0, 4, -1, false));
    wq.addWork(sp1);
    ASSERT(sp1->isAncestorTo(*sp1));

    pos.makeMove(TextIO::uciStringToMove("e2e4"), ui);
    posHashList[posHashListSize++] = pos.zobristHash();
    auto sp2 = std::make_shared<SplitPoint>(0, sp1, 0,
                                            pos, posHashList, posHashListSize,
                                            sti, kt, ht, 10, 11, 1, 1);
    sp2->addMove(0, SplitPointMove(TextIO::uciStringToMove("e7e5"), 0, 4, -1, false));
    sp2->addMove(1, SplitPointMove(TextIO::uciStringToMove("c7c5"), 0, 4, -1, false));
    wq.addWork(sp2);
    ASSERT( sp1->isAncestorTo(*sp2));
    ASSERT(!sp2->isAncestorTo(*sp1));

    pos.makeMove(TextIO::uciStringToMove("e7e5"), ui);
    posHashList[posHashListSize++] = pos.zobristHash();
    auto sp3 = std::make_shared<SplitPoint>(0, sp2, 0,
                                            pos, posHashList, posHashListSize,
                                            sti, kt, ht, 10, 11, 1, 1);
    sp3->addMove(0, SplitPointMove(TextIO::uciStringToMove("g1f3"), 0, 4, -1, false));
    sp3->addMove(1, SplitPointMove(TextIO::uciStringToMove("d2d4"), 0, 4, -1, false));
    sp3->addMove(2, SplitPointMove(TextIO::uciStringToMove("c2c3"), 0, 4, -1, false));
    wq.addWork(sp3);
    ASSERT( sp1->isAncestorTo(*sp3));
    ASSERT( sp2->isAncestorTo(*sp3));
    ASSERT(!sp3->isAncestorTo(*sp1));
    ASSERT(!sp3->isAncestorTo(*sp2));

    pos = TextIO::readFEN(TextIO::startPosFEN);
    posHashListSize = 1;
    pos.makeMove(TextIO::uciStringToMove("d2d4"), ui);
    posHashList[posHashListSize++] = pos.zobristHash();
    auto sp4 = std::make_shared<SplitPoint>(0, sp1, 2,
                                            pos, posHashList, posHashListSize,
                                            sti, kt, ht, 10, 11, 1, 1);
    sp4->addMove(0, SplitPointMove(TextIO::uciStringToMove("d7d5"), 0, 4, -1, false));
    sp4->addMove(1, SplitPointMove(TextIO::uciStringToMove("g8f6"), 0, 4, -1, false));
    wq.addWork(sp4);
    ASSERT( sp1->isAncestorTo(*sp4));
    ASSERT(!sp2->isAncestorTo(*sp4));
    ASSERT(!sp3->isAncestorTo(*sp4));
    ASSERT(!sp4->isAncestorTo(*sp1));
    ASSERT(!sp4->isAncestorTo(*sp2));
    ASSERT(!sp4->isAncestorTo(*sp3));

    ASSERT_EQUAL(4, wq.queue.size());
    ASSERT_EQUAL(2, sp1->getChildren().size());
    ASSERT_EQUAL(1, sp2->getChildren().size());
    ASSERT_EQUAL(0, sp3->getChildren().size());
    ASSERT_EQUAL(0, sp4->getChildren().size());
//    sp1->print(std::cout, 0, fhi);
//    std::cout << std::endl;
    ASSERT_EQUAL_DELTA(1.0, sp1->getPSpUseful(), eps);
    ASSERT_EQUAL_DELTA((511+1023)/(1023.0*2), sp1->getPNextMoveUseful(), eps);
    ASSERT_EQUAL_DELTA(1.0, sp2->getPSpUseful(), eps);
    ASSERT_EQUAL_DELTA((511+1023)/(1023.0*2), sp2->getPNextMoveUseful(), eps);
    ASSERT_EQUAL_DELTA(1.0, sp3->getPSpUseful(), eps);
    ASSERT_EQUAL_DELTA((511+1023)/(1023.0*2), sp3->getPNextMoveUseful(), eps);
    ASSERT_EQUAL_DELTA((255+1023)/(1023.0*2), sp4->getPSpUseful(), eps);
    ASSERT_EQUAL_DELTA((255+1023)/(1023.0*2)*511/1023, sp4->getPNextMoveUseful(), eps);

    std::vector<std::shared_ptr<SplitPoint>> q = extractQueue(wq.queue);
    ASSERT_EQUAL(sp1, q[0]);
    ASSERT(q[1] == sp2 || q[1] == sp3);
    ASSERT(q[2] == sp2 || q[2] == sp3);
    ASSERT(q[1] != q[2]);
    ASSERT_EQUAL(sp4, q[3]);

    int score = wq.setOwnerCurrMove(sp3, 1, 10);
    ASSERT_EQUAL(4, wq.queue.size());
//    sp1->print(std::cout, 0, fhi);
//    std::cout << std::endl;
    ASSERT_EQUAL_DELTA(1.0, sp1->getPSpUseful(), eps);
    ASSERT_EQUAL_DELTA((511+1023)/(1023.0*2), sp1->getPNextMoveUseful(), eps);
    ASSERT_EQUAL_DELTA(1.0, sp2->getPSpUseful(), eps);
    ASSERT_EQUAL_DELTA((511+1023)/(1023.0*2), sp2->getPNextMoveUseful(), eps);
    ASSERT_EQUAL_DELTA(1.0, sp3->getPSpUseful(), eps);
    ASSERT_EQUAL_DELTA((255+1023)/(511+1023.0), sp3->getPNextMoveUseful(), eps);
    ASSERT_EQUAL_DELTA((255+1023)/(1023.0*2), sp4->getPSpUseful(), eps);
    ASSERT_EQUAL_DELTA((255+1023)/(1023.0*2)*511/1023, sp4->getPNextMoveUseful(), eps);
    ASSERT_EQUAL(UNKNOWN_SCORE, score);

    std::vector<std::shared_ptr<SplitPoint>> q2 = extractQueue(wq.queue);
    ASSERT_EQUAL(sp3, q2[0]);
    ASSERT(q2[1] == sp1 || q2[1] == sp2);
    ASSERT(q2[2] == sp1 || q2[2] == sp2);
    ASSERT(q2[1] != q2[2]);
    ASSERT_EQUAL(sp4, q2[3]);

    wq.cancel(sp2);
//    sp1->print(std::cout, 0, fhi);
//    std::cout << std::endl;
    ASSERT_EQUAL(2, wq.queue.size());

    std::vector<std::shared_ptr<SplitPoint>> q3 = extractQueue(wq.queue);
    ASSERT_EQUAL(sp1, q3[0]);
    ASSERT_EQUAL(sp4, q3[1]);
    ASSERT_EQUAL_DELTA(1.0, sp1->getPSpUseful(), eps);
    ASSERT_EQUAL_DELTA((511+1023)/(1023.0*2), sp1->getPNextMoveUseful(), eps);
    ASSERT_EQUAL_DELTA((255+1023)/(1023.0*2), sp4->getPSpUseful(), eps);
    ASSERT_EQUAL_DELTA((255+1023)/(1023.0*2)*511/1023, sp4->getPNextMoveUseful(), eps);

    score = wq.setOwnerCurrMove(sp1, 1, 10);
//    sp1->print(std::cout, 0, fhi);
//    std::cout << std::endl;
    ASSERT_EQUAL(2, wq.queue.size());
    ASSERT_EQUAL(UNKNOWN_SCORE, score);

    std::vector<std::shared_ptr<SplitPoint>> q4 = extractQueue(wq.queue);
    ASSERT_EQUAL(sp1, q4[0]);
    ASSERT_EQUAL(sp4, q4[1]);
    ASSERT_EQUAL_DELTA(1.0, sp1->getPSpUseful(), eps);
    ASSERT_EQUAL_DELTA((255+1023)/(511+1023.0), sp1->getPNextMoveUseful(), eps);
    ASSERT_EQUAL_DELTA((255+1023)/(511+1023.0), sp4->getPSpUseful(), eps);
    ASSERT_EQUAL_DELTA((255+1023)/(511+1023.0)*511/1023, sp4->getPNextMoveUseful(), eps);
}

void
ParallelTest::testSplitPointHolder() {
    TranspositionTable tt(10);
    ParallelData pd(tt);
    WorkQueue& wq = pd.wq;
    FailHighInfo& fhi = pd.fhInfo;
    std::vector<std::shared_ptr<SplitPoint>> spVec, pending;

    for (int m = 0; m < 2; m++) {
        for (int i = 0; i < 10; i++) {
            for (int cnt = 0; cnt < (1<<(9-i)); cnt++) {
                fhi.addData(m, i, true, true);
                if (m == 0)
                    fhi.addData(m, i, false, true);
            }
        }
    }

    std::shared_ptr<SplitPoint> nullRoot;
    Position pos = TextIO::readFEN(TextIO::startPosFEN);
    std::vector<U64> posHashList(200);
    posHashList[0] = pos.zobristHash();
    int posHashListSize = 1;
    SearchTreeInfo sti;
    KillerTable kt;
    History ht;

    {
        SplitPointHolder sph(pd, spVec, pending);
        ASSERT_EQUAL(0, wq.queue.size());
        ASSERT_EQUAL(0, spVec.size());
        sph.setSp(std::make_shared<SplitPoint>(0, nullRoot, 0,
                                               pos, posHashList, posHashListSize,
                                               sti, kt, ht, 10, 11, 1, 1));
        ASSERT_EQUAL(0, wq.queue.size());
        ASSERT_EQUAL(0, spVec.size());
        sph.addMove(0, SplitPointMove(TextIO::uciStringToMove("e2e4"), 0, 4, -1, false));
        sph.addMove(1, SplitPointMove(TextIO::uciStringToMove("c2c4"), 0, 4, -1, false));
        sph.addMove(2, SplitPointMove(TextIO::uciStringToMove("d2d4"), 0, 4, -1, false));
        ASSERT_EQUAL(0, wq.queue.size());
        ASSERT_EQUAL(0, spVec.size());
        sph.addToQueue();
        ASSERT_EQUAL(1, wq.queue.size());
        ASSERT_EQUAL(1, spVec.size());
        {
            SplitPointHolder sph2(pd, spVec, pending);
            ASSERT_EQUAL(1, wq.queue.size());
            ASSERT_EQUAL(1, spVec.size());
        }
        ASSERT_EQUAL(1, wq.queue.size());
        ASSERT_EQUAL(1, spVec.size());
        {
            SplitPointHolder sph2(pd, spVec, pending);
            ASSERT_EQUAL(1, wq.queue.size());
            ASSERT_EQUAL(1, spVec.size());
            sph2.setSp(std::make_shared<SplitPoint>(0, spVec.back(), 0,
                                                    pos, posHashList, posHashListSize,
                                                    sti, kt, ht, 10, 11, 1, 1));
            ASSERT_EQUAL(1, wq.queue.size());
            ASSERT_EQUAL(1, spVec.size());
            sph2.addMove(0, SplitPointMove(TextIO::uciStringToMove("g8f6"), 0, 4, -1, false));
            sph2.addMove(1, SplitPointMove(TextIO::uciStringToMove("c7c6"), 0, 4, -1, false));
            ASSERT_EQUAL(1, wq.queue.size());
            ASSERT_EQUAL(1, spVec.size());
            sph2.addToQueue();
            ASSERT_EQUAL(2, wq.queue.size());
            ASSERT_EQUAL(2, spVec.size());
        }
        ASSERT_EQUAL(1, wq.queue.size());
        ASSERT_EQUAL(1, spVec.size());
    }
    ASSERT_EQUAL(0, wq.queue.size());
    ASSERT_EQUAL(0, spVec.size());
}

static void
probeTT(Position& pos, const Move& m, TranspositionTable& tt, TranspositionTable::TTEntry& ent) {
    UndoInfo ui;
    pos.makeMove(m, ui);
    ent.clear();
    tt.probe(pos.historyHash(), ent);
    pos.unMakeMove(m, ui);
}

void
ParallelTest::testWorkerThread() {
    const int minProbeDepth = UciParams::minProbeDepth->getIntPar();
    UciParams::minProbeDepth->set("100");

    TranspositionTable tt(18);
    ParallelData pd(tt);
    FailHighInfo& fhi = pd.fhInfo;
    std::vector<std::shared_ptr<SplitPoint>> spVec, pending;

    for (int m = 0; m < 2; m++) {
        for (int i = 0; i < 10; i++) {
            for (int cnt = 0; cnt < (1<<(9-i)); cnt++) {
                fhi.addData(m, i, true, true);
                if (m == 0)
                    fhi.addData(m, i, false, true);
            }
        }
    }

    std::shared_ptr<SplitPoint> nullRoot;
    Position pos = TextIO::readFEN(TextIO::startPosFEN);
    std::vector<U64> posHashList(200);
    posHashList[0] = pos.zobristHash();
    int posHashListSize = 1;
    SearchTreeInfo sti;
    KillerTable kt;
    History ht;

    pd.addRemoveWorkers(3);

    {
        SplitPointHolder sph(pd, spVec, pending);
        auto sp = std::make_shared<SplitPoint>(0, nullRoot, 0,
                                               pos, posHashList, posHashListSize,
                                               sti, kt, ht, 10, 11, 1, 1);
        sph.setSp(sp);
        int depth = 10;
        sph.addMove(0, SplitPointMove(TextIO::uciStringToMove("e2e4"), 0, depth, -1, false));
        sph.addMove(1, SplitPointMove(TextIO::uciStringToMove("c2c4"), 0, depth, -1, false));
        sph.addMove(2, SplitPointMove(TextIO::uciStringToMove("f2f4"), 0, depth, -1, false));
        sph.addToQueue();
        pd.startAll();
        while (sp->hasUnFinishedMove()) {
            std::cout << "waiting..." << std::endl;
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
        pd.stopAll();

        TranspositionTable::TTEntry ent;
        probeTT(pos, TextIO::uciStringToMove("e2e4"), tt, ent);
        ASSERT(ent.getType() == TType::T_EMPTY); // Only searched by "master" thread, which does not exist in this test

        probeTT(pos, TextIO::uciStringToMove("c2c4"), tt, ent);
        ASSERT(ent.getType() != TType::T_EMPTY);
        ASSERT(ent.getDepth() >= 6);
        probeTT(pos, TextIO::uciStringToMove("f2f4"), tt, ent);
        ASSERT(ent.getType() != TType::T_EMPTY);
        ASSERT(ent.getDepth() >= 5);
    }

    UciParams::minProbeDepth->set(num2Str(minProbeDepth));
}

cute::suite
ParallelTest::getSuite() const {
    cute::suite s;
    s.push_back(CUTE(testFailHighInfo));
    s.push_back(CUTE(testNpsInfo));
    s.push_back(CUTE(testWorkQueue));
    s.push_back(CUTE(testWorkQueueParentChild));
    s.push_back(CUTE(testSplitPointHolder));
    s.push_back(CUTE(testWorkerThread));
    return s;
}
