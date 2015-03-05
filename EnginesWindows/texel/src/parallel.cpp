/*
    Texel - A UCI chess engine.
    Copyright (C) 2013-2014  Peter Österlund, peterosterlund2@gmail.com

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
 * parallel.cpp
 *
 *  Created on: Jul 9, 2013
 *      Author: petero
 */

#include "parallel.hpp"
#include "numa.hpp"
#include "search.hpp"
#include "tbprobe.hpp"
#include "textio.hpp"
#include "util/logger.hpp"

#include <cmath>
#include <cassert>

using namespace Logger;

U64 SplitPoint::nextSeqNo = 0;

// ----------------------------------------------------------------------------

WorkerThread::WorkerThread(int threadNo0, ParallelData& pd0,
                           TranspositionTable& tt0)
    : threadNo(threadNo0), pd(pd0), tt(tt0),
      pUseful(0.0) {
}

WorkerThread::~WorkerThread() {
    assert(!thread);
}

void
WorkerThread::start() {
    assert(!thread);
    const int minProbeDepth = TBProbe::tbEnabled() ? UciParams::minProbeDepth->getIntPar() : 100;
    thread = std::make_shared<std::thread>([this,minProbeDepth](){ mainLoop(minProbeDepth); });
}

void
WorkerThread::join() {
    if (thread) {
        thread->join();
        thread.reset();
    }
}

class ThreadStopHandler : public Search::StopHandler {
public:
    ThreadStopHandler(WorkerThread& wt, ParallelData& pd,
                      const SplitPoint& sp, const SplitPointMove& spm,
                      const Search& sc, int initialAlpha,
                      S64 totalNodes, int myPrio);

    /** Destructor. Report searched nodes to ParallelData object. */
    ~ThreadStopHandler();

    ThreadStopHandler(const ThreadStopHandler&) = delete;
    ThreadStopHandler& operator=(const ThreadStopHandler&) = delete;

    bool shouldStop();

private:
    /** Report searched nodes since last call to ParallelData object. */
    void reportNodes(bool force);

    const WorkerThread& wt;
    ParallelData& pd;
    const SplitPoint& sp;
    const SplitPointMove& spMove;
    const Search& sc;
    int counter;             // Counts number of calls to shouldStop
    int nextProbCheck;       // Next time test for SplitPoint switch should be performed
    S64 lastReportedNodes;
    S64 lastReportedTbHits;
    int initialAlpha;
    const S64 totalNodes;
    const int myPrio;
};

ThreadStopHandler::ThreadStopHandler(WorkerThread& wt0, ParallelData& pd0,
                                     const SplitPoint& sp0, const SplitPointMove& spm0,
                                     const Search& sc0, int initialAlpha0,
                                     S64 totalNodes0, int myPrio0)
    : wt(wt0), pd(pd0), sp(sp0), spMove(spm0),
      sc(sc0), counter(0), nextProbCheck(1),
      lastReportedNodes(0), lastReportedTbHits(0),
      initialAlpha(initialAlpha0), totalNodes(totalNodes0), myPrio(myPrio0) {
}

ThreadStopHandler::~ThreadStopHandler() {
    reportNodes(true);
}

bool
ThreadStopHandler::shouldStop() {
    if (pd.wq.isStopped() || spMove.isCanceled())
        return true;
    if (sp.getAlpha() != initialAlpha)
        return true;

    counter++;
    if (counter >= nextProbCheck) {
        nextProbCheck = counter + 1 + counter / 4;
        int bestPrio = pd.wq.getBestPrio();
//        log([&](std::ostream& os){os << "shouldStop, th:" << wt.getThreadNo() << " myP:" << myPrio << " bestP:" << bestPrio;});
        if ((bestPrio > myPrio + 20) && (bestPrio >= (myPrio + (1000 - myPrio) * 0.25)) &&
            (sp.owningThread() != wt.getThreadNo()))
            return true;
        reportNodes(false);
    }

    return false;
}

void
ThreadStopHandler::reportNodes(bool force) {
    S64 totNodes = sc.getTotalNodesThisThread();
    S64 nodes = totNodes - lastReportedNodes;
    if (force || (nodes * 1024 > totalNodes)) {
        lastReportedNodes = totNodes;
        pd.addSearchedNodes(nodes);
        S64 totTbHits = sc.getTbHitsThisThread();
        S64 tbHits = totTbHits - lastReportedTbHits;
        lastReportedTbHits = totTbHits;
        pd.addTbHits(tbHits);
    }
}

void
WorkerThread::mainLoop(int minProbeDepth) {
//    log([&](std::ostream& os){os << "mainLoop, th:" << threadNo;});
    Numa::instance().bindThread(threadNo);
    if (!et)
        et = Evaluate::getEvalHashTables();
    if (!kt)
        kt = std::make_shared<KillerTable>();
    if (!ht)
        ht = std::make_shared<History>();

    TreeLogger logFile;
    logFile.open("/home/petero/treelog.dmp", pd, threadNo);

//    UtilizationTimer uTimer;
    std::mutex m;
    std::unique_lock<std::mutex> lock(m);
    Position pos;
    std::shared_ptr<SplitPoint> sp;
    for (int iter = 0; ; iter++) {
        const bool doTiming = (iter & 15) == 0;
        int moveNo = -1;
//        uTimer.setPUseful(-1);
        const double t0 = doTiming ? currentTime() : -1;
        int prio;
        std::shared_ptr<SplitPoint> newSp = pd.wq.getWork(moveNo, pd, threadNo, prio, pUseful);
        if (!newSp)
            break;
        const double tStart = doTiming ? currentTime() : -1;

        const SplitPointMove& spMove = newSp->getSpMove(moveNo);
        const int depth = spMove.getDepth();
        if (depth < 0) { // Move skipped by forward pruning or legality check
            pd.wq.moveFinished(newSp, moveNo, false, SearchConst::UNKNOWN_SCORE);
            continue;
        }
        if (sp != newSp) {
            sp = newSp;
            *ht = sp->getHistory();
            *kt = sp->getKillerTable();
        }
        Search::SearchTables st(tt, *kt, *ht, *et);
        sp->getPos(pos, spMove.getMove());
        std::vector<U64> posHashList;
        int posHashListSize;
        sp->getPosHashList(pos, posHashList, posHashListSize);
        Search sc(pos, posHashList, posHashListSize, st, pd, sp, logFile);
        const U64 rootNodeIdx = logFile.logPosition(pos, sp->owningThread(),
                                                    sp->getSearchTreeInfo().nodeIdx, moveNo);
        sc.setThreadNo(threadNo);
        sc.setMinProbeDepth(minProbeDepth);
        const int alpha = sp->getAlpha();
        const int beta = sp->getBeta();
        const S64 nodes0 = pd.getNumSearchedNodes();
        auto stopHandler(std::make_shared<ThreadStopHandler>(*this, pd, *sp, spMove,
                                                             sc, alpha, nodes0, prio));
        sc.setStopHandler(stopHandler);
        const int ply = sp->getPly();
        const int lmr = spMove.getLMR();
        const int captSquare = spMove.getRecaptureSquare();
        const bool inCheck = spMove.getInCheck();
        sc.setSearchTreeInfo(ply, sp->getSearchTreeInfo(), spMove.getMove(), moveNo, lmr, rootNodeIdx);
        try {
//            log([&](std::ostream& os){os << "th:" << threadNo << " seqNo:" << sp->getSeqNo() << " ply:" << ply
//                                         << " c:" << sp->getCurrMoveNo() << " m:" << moveNo
//                                         << " a:" << alpha << " b:" << beta
//                                         << " d:" << depth/SearchConst::plyScale
//                                         << " p:" << sp->getPMoveUseful(pd.fhInfo, moveNo) << " start";});
//            uTimer.setPUseful(pUseful);
            const bool smp = pd.numHelperThreads() > 1;
            int score = -sc.negaScout(smp, true, -(alpha+1), -alpha, ply+1,
                                      depth, captSquare, inCheck);
            if (((lmr > 0) && (score > alpha)) ||
                    ((score > alpha) && (score < beta))) {
                sc.setSearchTreeInfo(ply, sp->getSearchTreeInfo(), spMove.getMove(), moveNo, 0, rootNodeIdx);
                score = -sc.negaScout(smp, true, -beta, -alpha, ply+1,
                                      depth + lmr, captSquare, inCheck);
            }
//            uTimer.setPUseful(0);
            bool cancelRemaining = score >= beta;
//            log([&](std::ostream& os){os << "th:" << threadNo << " seqNo:" << sp->getSeqNo() << " ply:" << ply
//                                         << " c:" << sp->getCurrMoveNo() << " m:" << moveNo
//                                         << " a:" << alpha << " b:" << beta << " s:" << score
//                                         << " d:" << depth/SearchConst::plyScale << " n:" << sc.getTotalNodesThisThread();});
            pd.wq.moveFinished(sp, moveNo, cancelRemaining, score);
        } catch (const Search::StopSearch&) {
//            log([&](std::ostream& os){os << "th:" << threadNo << " seqNo:" << sp->getSeqNo() << " m:" << moveNo
//                                         << " aborted n:" << sc.getTotalNodesThisThread();});
            if (!spMove.isCanceled() && !pd.wq.isStopped())
                pd.wq.returnMove(sp, moveNo);
        }
        if (doTiming) {
            const double tEnd = currentTime();
            pd.npsInfo.addData(sp->getDepth(), sc.getTotalNodesThisThread(), tStart - t0, tEnd - tStart);
        }
        pUseful = 0.0;
    }
//    double tElapsed, tUseful, tSleep;
//    uTimer.getStats(tElapsed, tUseful, tSleep);
//    log([&](std::ostream& os){
//        os << "~mainLoop, th:" << threadNo << " useful:" << tUseful / tElapsed
//           << " sleep:" << tSleep / tElapsed;
//    });
//    log([&](std::ostream& os){os << "~mainLoop, th:" << threadNo;});
}

// ----------------------------------------------------------------------------

void
WorkQueue::setStopped(bool stop) {
    Lock L(this);
    stopped = stop;
    if (stopped)
        cv.notify_all();
}

void
WorkQueue::addWork(const std::shared_ptr<SplitPoint>& sp) {
    Lock L(this);
//    ScopedTimeSample sts { getAddWorkStat(sp->owningThread()) };
    addWorkInternal(sp);
}

void
WorkQueue::tryAddWork(const std::shared_ptr<SplitPoint>& sp,
                      std::vector<std::shared_ptr<SplitPoint>>& pending) {
    std::unique_lock<std::mutex> lock(mutex, std::defer_lock);
    if (!lock.try_lock()) {
        pending.push_back(sp);
        return;
    }
    for (const auto& sp : pending)
        addWorkInternal(sp);
    pending.clear();
    addWorkInternal(sp);
}

void
WorkQueue::addWorkInternal(const std::shared_ptr<SplitPoint>& sp) {
    sp->setSeqNo();
    std::shared_ptr<SplitPoint> parent = sp->getParent();
    if (parent) {
        if (parent->isCanceled())
            sp->cancel();
        else
            parent->addChild(sp);
    }
    if (sp->hasUnFinishedMove()) {
        sp->computeProbabilities(fhInfo, npsInfo);
        insertInQueue(sp);
    }
}

std::shared_ptr<SplitPoint>
WorkQueue::getWork(int& spMove, ParallelData& pd, int threadNo) {
    int prio;
    double pUseful;
    return getWork(spMove, pd, threadNo, prio, pUseful);
}

std::shared_ptr<SplitPoint>
WorkQueue::getWork(int& spMove, ParallelData& pd, int threadNo, int& prio, double& pUseful) {
    Lock L(this);
    while (true) {
        while (queue.empty() && !isStopped())
            L.wait(cv);
//        ScopedTimeSample sts { getGetWorkStat(threadNo) };
        if (isStopped())
            return nullptr;
        std::shared_ptr<SplitPoint> ret = queue.front();
        spMove = ret->getNextMove(pd.fhInfo);
        if (spMove < 0) {
            L.wait(cv);
            continue;
        }
//        log([&](std::ostream& os){printSpTree(os, pd, threadNo, ret, spMove);});
        prio = ret->getPrio();
        updateProbabilities(ret);
        pUseful = ret->getPMoveUseful(pd.fhInfo, spMove);
        return ret;
    }
}

void
WorkQueue::returnMove(const std::shared_ptr<SplitPoint>& sp, int moveNo) {
    Lock L(this);
    sp->returnMove(moveNo);
    updateProbabilities(sp);
}

int
WorkQueue::setOwnerCurrMove(const std::shared_ptr<SplitPoint>& sp, int moveNo, int alpha) {
    Lock L(this);
    int score = sp->setOwnerCurrMove(moveNo, alpha);
    updateProbabilities(sp);
    return score;
}

void
WorkQueue::cancel(const std::shared_ptr<SplitPoint>& sp) {
    Lock L(this);
    cancelInternal(sp);
}

void
WorkQueue::moveFinished(const std::shared_ptr<SplitPoint>& sp, int moveNo,
                        bool cancelRemaining, int score) {
    Lock L(this);
    sp->moveFinished(moveNo, cancelRemaining, score);
    updateProbabilities(sp);
}

double
WorkQueue::getBestProbability(std::shared_ptr<SplitPoint>& bestSp) const {
    Lock L(this);
    if (queue.empty())
        return 0.0;
    bestSp = queue.front();
    return bestSp->getPNextMoveUseful();
}

int
WorkQueue::getBestPrio() const {
    Lock L(this);
    if (queue.empty())
        return -1;
    return queue.front()->getPrio();
}

double
WorkQueue::getBestProbability() const {
    std::shared_ptr<SplitPoint> bestSp;
    return getBestProbability(bestSp);
}

void
WorkQueue::updateProbabilities(const std::shared_ptr<SplitPoint>& sp) {
    if (!sp->hasUnFinishedMove())
        queue.remove(sp);
    sp->computeProbabilities(fhInfo, npsInfo);
}

void
WorkQueue::cancelInternal(const std::shared_ptr<SplitPoint>& sp) {
    sp->cancel();
    queue.remove(sp);

    for (const auto& wChild : sp->getChildren()) {
        std::shared_ptr<SplitPoint> child = wChild.lock();
        if (child)
            cancelInternal(child);
    }
}

static std::string toPercentStr(double p) {
    std::stringstream ss;
    int pc = (int)(p * 100);
    if (pc == 100)
        pc = 99;
    ss << std::setfill('0') << std::setw(2) << pc;
    return ss.str();
}

void
WorkQueue::printSpTree(std::ostream& os, const ParallelData& pd,
                       int threadNo, const std::shared_ptr<SplitPoint> selectedSp,
                       int selectedMove) {
    const int numThreads = pd.numHelperThreads() + 1;
    std::shared_ptr<SplitPoint> rootSp = queue.front();
    assert(rootSp);
    while (rootSp->getParent())
        rootSp = rootSp->getParent();
    std::vector<int> parentThreads(numThreads, -1);
    std::vector<std::shared_ptr<SplitPoint>> leaves(numThreads, nullptr);
    findLeaves(rootSp, parentThreads, leaves);

    os << "th:" << threadNo << " m: " << selectedMove << ':'
       << TextIO::moveToUCIString(selectedSp->getSpMove(selectedMove).getMove())
       << " p:" << selectedSp->getPMoveUseful(pd.fhInfo, selectedMove)
       << std::endl;
    for (int i = 0; i < numThreads; i++) {
        std::vector<std::shared_ptr<SplitPoint>> thVec;
        for (auto sp = leaves[i]; sp; sp = sp->getParent())
            thVec.push_back(sp);
        std::reverse(thVec.begin(), thVec.end());
        os << "th " << i << ' ';
        if (parentThreads[i] < 0)
            os << '-';
        else
            os << parentThreads[i];
        os << ' ' << toPercentStr(i == 0 ? 1 : pd.getHelperThread(i-1).getPUseful());
        if (!thVec.empty()) {
            for (const auto& sp : thVec)
                if (sp->owningThread() == i) {
                    os << ' ' << std::setw(6) << sp->getSeqNo();
                    break;
                }
        }
        for (const auto& sp : thVec) {
            if (sp->owningThread() == i) {
                int pMove = sp->getParentMoveNo();
                os << ' ' << std::setw(2) << pMove << (sp == selectedSp ? '*' : ':');
                if (pMove < 0)
                    os << "null";
                else if (sp->getParent())
                    os << TextIO::moveToUCIString(sp->getParent()->getSpMove(pMove).getMove());
                else
                    os << TextIO::moveToUCIString(pd.topMove);
                os << ',' << toPercentStr(sp->getPSpUseful())
                   << ':' << toPercentStr(sp->getPNextMoveUseful());
                os << ',' << std::setw(2) << sp->getCurrMoveNo() << ':' << std::setw(2) << sp->findNextMove(pd.fhInfo);
            } else {
                os << "                    ";
            }
        }
        os << std::endl;
    }
}

void
WorkQueue::findLeaves(const std::shared_ptr<SplitPoint>& sp,
                      std::vector<int>& parentThreads,
                      std::vector<std::shared_ptr<SplitPoint>>& leaves) {
    bool isLeaf = true;
    const std::vector<std::weak_ptr<SplitPoint>>& children = sp->getChildren();
    for (const auto& wChild : children) {
        std::shared_ptr<SplitPoint> child = wChild.lock();
        if (child && !child->isCanceled()) {
            if (child->owningThread() == sp->owningThread()) {
                isLeaf = false;
            } else {
                assert(parentThreads[child->owningThread()] == -1);
                parentThreads[child->owningThread()] = sp->owningThread();
            }
            findLeaves(child, parentThreads, leaves);
        }
    }
    if (isLeaf) {
        assert(sp->owningThread() >= 0);
        assert(sp->owningThread() < (int)leaves.size());
        leaves[sp->owningThread()] = sp;
    }
}

WorkQueue::Lock::Lock(const WorkQueue* wq0)
    : wq(*wq0), lock(wq.mutex, std::defer_lock) {
    bool contended = false;
    if (!lock.try_lock()) {
        contended = true;
        lock.lock();
    }
    if (wq.queue.empty())
        contended = false;
    U64 c = wq.nContended;
    U64 n = wq.nNonContended;
    if (contended)
        c++;
    else
        n++;
    if (n + c > 30000) {
        c /= 2;
        n /= 2;
        if (c * 100 > n * 50) {
            wq.minSplitDepth++;
//            std::cout << "contended stat: " << wq.minSplitDepth << " " << c << " " << n << std::endl;
        } else if ((c * 100 < n * 25) && (wq.minSplitDepth > SearchConst::MIN_SMP_DEPTH)) {
            wq.minSplitDepth--;
//            std::cout << "contended stat: " << wq.minSplitDepth << " " << c << " " << n << std::endl;
        }
    }
    wq.nContended = c;
    wq.nNonContended = n;
}

// ----------------------------------------------------------------------------

void
ParallelData::addRemoveWorkers(int numWorkers) {
    while (numWorkers < (int)threads.size()) {
        assert(!threads.back()->threadRunning());
        threads.pop_back();
    }
    for (int i = threads.size(); i < numWorkers; i++)
        threads.push_back(std::make_shared<WorkerThread>(i+1, *this, tt));
}

void
ParallelData::startAll() {
    totalHelperNodes = 0;
    helperTbHits = 0;
    wq.setStopped(false);
    for (auto& thread : threads)
        thread->start();
}

void
ParallelData::stopAll() {
    wq.setStopped(true);
    for (auto& thread : threads)
        thread->join();
}

// ----------------------------------------------------------------------------

SplitPoint::SplitPoint(int threadNo0,
                       const std::shared_ptr<SplitPoint>& parentSp0, int parentMoveNo0,
                       const Position& pos0, const std::vector<U64>& posHashList0,
                       int posHashListSize0, const SearchTreeInfo& sti0,
                       const KillerTable& kt0, const History& ht0,
                       int alpha0, int beta0, int ply0, int depth0)
    : pos(pos0), posHashList(posHashList0), posHashListSize(posHashListSize0),
      searchTreeInfo(sti0), kt(kt0), ht(ht0),
      alpha(alpha0), beta(beta0), ply(ply0), depth(depth0),
      isPV(beta0 > alpha0 + 1),
      pSpUseful(0.0), pNextMoveUseful(0.0),
      threadNo(threadNo0), parent(parentSp0), parentMoveNo(parentMoveNo0),
      seqNo(0), currMoveNo(0), inserted(false), canceled(false) {
}

void
SplitPoint::addMove(int moveNo, const SplitPointMove& spMove) {
    assert(moveNo >= (int)spMoves.size());
    while ((int)spMoves.size() < moveNo)
        spMoves.push_back(SplitPointMove(Move(), 0, -1, -1, false));
    spMoves.push_back(spMove);
}

void
SplitPoint::computeProbabilities(const FailHighInfo& fhInfo, const DepthNpsInfo& npsInfo) {
    if (parent) {
        double pMoveUseful = 1.0;
        if (parentMoveNo >= 0)
            pMoveUseful = parent->getMoveNeededProbability(fhInfo, parentMoveNo);
        pSpUseful = parent->pSpUseful * pMoveUseful;
    } else {
        pSpUseful = 1.0;
    }
    double pNextUseful = getMoveNeededProbability(fhInfo, findNextMove(fhInfo));
    pNextMoveUseful = pSpUseful * pNextUseful;
    newPrio(getSpPrio(npsInfo));

    bool deleted = false;
    for (const auto& wChild : children) {
        std::shared_ptr<SplitPoint> child = wChild.lock();
        if (child)
            child->computeProbabilities(fhInfo, npsInfo);
        else
            deleted = true;
    }
    if (deleted)
        cleanUpChildren();
}

double
SplitPoint::getPMoveUseful(const FailHighInfo& fhInfo, int moveNo) const {
    return pSpUseful * getMoveNeededProbability(fhInfo, moveNo);
}

void
SplitPoint::getPos(Position& pos, const Move& move) const {
    pos = this->pos;
    UndoInfo ui;
    pos.makeMove(move, ui);
}

void
SplitPoint::getPosHashList(const Position& pos, std::vector<U64>& posHashList,
                           int& posHashListSize) const {
    posHashList = this->posHashList;
    posHashListSize = this->posHashListSize;
    posHashList[posHashListSize++] = pos.zobristHash();
}

int
SplitPoint::getNextMove(const FailHighInfo& fhInfo) {
    int m = findNextMove(fhInfo);
    if (m < 0)
        return m;
    spMoves[m].setSearching(true);
    return m;
}

void
SplitPoint::moveFinished(int moveNo, bool cancelRemaining, int score) {
    assert((moveNo >= 0) && (moveNo < (int)spMoves.size()));
    spMoves[moveNo].setScore(score);
    spMoves[moveNo].setSearching(false);
    spMoves[moveNo].setCanceled(true);
    if (cancelRemaining)
        for (int i = moveNo+1; i < (int)spMoves.size(); i++)
            spMoves[i].setCanceled(true);
}

bool
SplitPoint::hasUnStartedMove() const {
    if (canceled)
        return false;
    for (int i = currMoveNo + 1; i < (int)spMoves.size(); i++)
        if (!spMoves[i].isCanceled() && !spMoves[i].isSearching())
            return true;
    return false;
}

bool
SplitPoint::hasUnFinishedMove() const {
    if (canceled)
        return false;
    for (int i = currMoveNo + 1; i < (int)spMoves.size(); i++)
        if (!spMoves[i].isCanceled())
            return true;
    return false;
}

int
SplitPoint::findNextMove(const FailHighInfo& fhInfo) const {
    int i0 = -1;
    const double pGood = 0.98;
    for (int i = currMoveNo+1; i < (int)spMoves.size(); i++)
        if (!spMoves[i].isCanceled() && !spMoves[i].isSearching()) {
            if ((getPNextMoveUseful() > pGood) && (i0 == -1))
                i0 = i;
            else {
                if ((i0 != -1) && (getPMoveUseful(fhInfo, i) <= pGood))
                    return i0;
                return i;
            }
        }
    return i0;
}

double
SplitPoint::getMoveNeededProbability(const FailHighInfo& fhInfo, int moveNo) const {
    if (isPvNode())
        return fhInfo.getMoveNeededProbabilityPv(currMoveNo, moveNo);
    else
        return fhInfo.getMoveNeededProbability(parentMoveNo,
                                               currMoveNo,
                                               moveNo, isAllNode());
}

void
SplitPoint::cleanUpChildren() {
    std::vector<std::weak_ptr<SplitPoint>> toKeep;
    for (const auto& wChild : children)
        if (wChild.lock())
            toKeep.push_back(wChild);
    children = toKeep;
}

bool
SplitPoint::hasHelperThread() const {
    for (const auto& wChild : children) {
        std::shared_ptr<SplitPoint> child = wChild.lock();
        if (child && child->owningThread() != owningThread())
            return true;
    }
    return false;
}

bool
SplitPoint::isAncestorTo(const SplitPoint& sp) const {
    const SplitPoint* tmp = &sp;
    while (tmp) {
        if (tmp == this)
            return true;
        tmp = &*(tmp->parent);
    }
    return false;
}

bool
SplitPoint::isAllNode() const {
    int nFirst = 0;
    const SplitPoint* tmp = this;
    while (tmp) {
        if (tmp->parentMoveNo == 0)
            nFirst++;
        else
            break;
        tmp = &*(tmp->parent);
    }
    return (nFirst % 2) != 0;
}

void
SplitPoint::print(std::ostream& os, int level, const FailHighInfo& fhInfo) const {
    std::string pad(level*2, ' ');
    os << pad << "seq:" << seqNo << " pos:" << TextIO::toFEN(pos) << std::endl;
    os << pad << "parent:" << parentMoveNo << " hashListSize:" << posHashListSize <<
        " a:" << alpha << " b:" << beta << " ply:" << ply << " canceled:" << canceled << std::endl;
    os << pad << "p1:" << pSpUseful << " p2:" << pNextMoveUseful << " curr:" << currMoveNo << std::endl;
    os << pad << "moves:";
    for (int mi = 0; mi < (int)spMoves.size(); mi++) {
        const auto& spm = spMoves[mi];
        os << ' ' << TextIO::moveToUCIString(spm.getMove());
        if (spm.isCanceled())
            os << ",c";
        if (spm.isSearching())
            os << ",s";
        os << "," << getMoveNeededProbability(fhInfo, mi);
    }
    os << std::endl;
    for (const auto& wChild : children) {
        std::shared_ptr<SplitPoint> child = wChild.lock();
        if (child)
            child->print(os, level+1, fhInfo);
    }
}

// ----------------------------------------------------------------------------

void
SplitPointHolder::setSp(const std::shared_ptr<SplitPoint>& sp0) {
    assert(state == State::EMPTY);
    assert(sp0);
    sp = sp0;
    if (sp->getBeta() > sp->getAlpha() + 1)
        pd.fhInfo.addPvData(-1, false);
    state = State::CREATED;
}

void
SplitPointHolder::addToQueue() {
    assert(state == State::CREATED);
    pd.wq.tryAddWork(sp, pending);
    spVec.push_back(sp);
//    log([&](std::ostream& os){os << "add seqNo:" << sp->getSeqNo() << " ply:" << sp->getPly()
//                                 << " pNext:" << sp->getPNextMoveUseful()
//                                 << " pMove:" << sp->getParentMoveNo() << " vec:" << spVec.size();});
    state = State::QUEUED;
}

// ----------------------------------------------------------------------------

FailHighInfo::FailHighInfo()
    : totCount(0) {
    for (int i = 0; i < NUM_NODE_TYPES; i++)
        failLoCount[i] = 0;
    for (int j = 0; j < NUM_STAT_MOVES; j++)
        newAlpha[j] = 0;
    totPvCount = 0;
}

double
FailHighInfo::getMoveNeededProbability(int parentMoveNo,
                                       int currMoveNo, int moveNo, bool allNode) const {
    const int pIdx = getNodeType(parentMoveNo, allNode);
    moveNo = std::min(moveNo, NUM_STAT_MOVES-1);
    if (moveNo < 0)
        return 0.0;

    int nNeeded = failLoCount[pIdx] + failHiCount[pIdx].sum(moveNo, NUM_STAT_MOVES);
    int nTotal = nNeeded + failHiCount[pIdx].sum(currMoveNo, moveNo);

    return (nTotal > 0) ? nNeeded / (double)nTotal : 0.5;
}

double
FailHighInfo::getMoveNeededProbabilityPv(int currMoveNo, int moveNo) const {
    moveNo = std::min(moveNo, NUM_STAT_MOVES-1);
    if (moveNo < 0)
        return 0.0;
    if (totPvCount <= 0)
        return 0.5;

    double prob = 1.0;
    double inv = 1.0 / totPvCount;
    for (int i = currMoveNo; i < moveNo; i++)
        prob *= std::max(0.0, 1.0 - newAlpha[i] * inv);
    return prob;
}

void
FailHighInfo::addData(int parentMoveNo, int nSearched, bool failHigh, bool allNode) {
    if (nSearched < 0)
        return;
    const int pIdx = getNodeType(parentMoveNo, allNode);
    if (failHigh) {
        nSearched = std::min(nSearched, NUM_STAT_MOVES-1);
        failHiCount[pIdx].add(nSearched, 1);
    } else {
        failLoCount[pIdx]++;
    }
    totCount++;
    if (totCount >= 1000000) {
        std::lock_guard<std::mutex> L(mutex);
        if (totCount >= 1000000)
            reScaleInternal(2);
    }
}

void
FailHighInfo::addPvData(int nSearched, bool alphaChanged) {
    if (nSearched >= 0) {
        if (alphaChanged && nSearched < NUM_STAT_MOVES)
            newAlpha[nSearched]++;
    } else {
        totPvCount++;
        if (totPvCount >= 10000) {
            std::lock_guard<std::mutex> L(mutex);
            if (totPvCount >= 10000)
                reScalePv(2);
        }
    }
}

void
FailHighInfo::reScale() {
    reScaleInternal(4);
    reScalePv(4);
}

void
FailHighInfo::reScaleInternal(int factor) {
    for (int i = 0; i < NUM_NODE_TYPES; i++) {
        for (int j = 0; j < NUM_STAT_MOVES; j++) {
            int val = failHiCount[i].get(j);
            failHiCount[i].add(j, val / factor - val);
        }
        failLoCount[i] = failLoCount[i] / factor;
    }
    totCount = totCount / factor;
}

void
FailHighInfo::reScalePv(int factor) {
    for (int j = 0; j < NUM_STAT_MOVES; j++)
        newAlpha[j] = newAlpha[j] / factor;
    totPvCount = totPvCount / factor;
}

void
FailHighInfo::print(std::ostream& os) const {
    for (int i = 0; i < NUM_NODE_TYPES; i++) {
        os << "fhInfo: " << i << ' ' << std::setw(6) << failLoCount[i];
        for (int j = 0; j < NUM_STAT_MOVES; j++)
            os << ' ' << std::setw(6) << failHiCount[i].get(j);
        os << std::endl;
    }
    os << "fhInfo: " << NUM_NODE_TYPES << ' ' << std::setw(6) << totPvCount;
    for (int j = 0; j < NUM_STAT_MOVES; j++)
        os << ' ' << std::setw(6) << newAlpha[j];
    os << std::endl;
}

// ----------------------------------------------------------------------------

void
DepthNpsInfo::print(std::ostream& os, int iterDepth) {
    std::lock_guard<std::mutex> L(mutex);
    os << "npsInfo: depth:" << iterDepth << " nps0:" << nps0
       << " wait:" << waitTime / nSearches << std::endl;
    for (int i = SearchConst::MIN_SMP_DEPTH; i < maxDepth; i++) {
        os << "npsInfo: d:" << i
           << " n:" << npsData[i].nSearches
           << " nodes:" << npsData[i].nodes
           << " time:" << npsData[i].time
           << " nps:" << npsData[i].nodes / npsData[i].time
           << " ts:" << npsData[i].nodes / (double)npsData[i].nSearches
           << " eff:" << efficiencyInternal(i) << std::endl;
    }
}
