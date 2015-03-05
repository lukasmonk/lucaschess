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
 * parallel.hpp
 *
 *  Created on: Jul 9, 2013
 *      Author: petero
 */

#ifndef PARALLEL_HPP_
#define PARALLEL_HPP_

#include "killerTable.hpp"
#include "history.hpp"
#include "transpositionTable.hpp"
#include "evaluate.hpp"
#include "searchUtil.hpp"
#include "constants.hpp"
#include "util/timeUtil.hpp"
#include "util/heap.hpp"

#include <memory>
#include <vector>
#include <set>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>


class Search;
class SearchTreeInfo;

class ParallelData;
class SplitPoint;
class SplitPointMove;
class FailHighInfo;
class DepthNpsInfo;


class WorkerThread {
public:
    /** Constructor. Does not start new thread. */
    WorkerThread(int threadNo, ParallelData& pd, TranspositionTable& tt);

    /** Destructor. Waits for thread to terminate. */
    ~WorkerThread();

    WorkerThread(const WorkerThread&) = delete;
    WorkerThread& operator=(const WorkerThread&) = delete;

    /** Start thread. */
    void start();

    /** Wait for thread to stop. */
    void join();

    /** Return true if thread is running. */
    bool threadRunning() const;

    /** Return thread number. The first worker thread is number 1. */
    int getThreadNo() const;

    /** For debugging. */
    double getPUseful() const;

private:
    /** Thread main loop. */
    void mainLoop(int minProbeDepth);

    int threadNo;
    ParallelData& pd;
    std::shared_ptr<std::thread> thread;

    std::shared_ptr<Evaluate::EvalHashTables> et;
    std::shared_ptr<KillerTable> kt;
    std::shared_ptr<History> ht;
    TranspositionTable& tt;

    double pUseful; // Probability that thread is currently doing something useful, for debugging
};


/** Priority queue of pending search tasks. Handles thread safety. */
class WorkQueue {
    friend class ParallelTest;
public:
    /** Constructor. */
    WorkQueue(const FailHighInfo& fhInfo, const DepthNpsInfo& npsInfo);

    /** Set/get stopped flag. */
    void setStopped(bool stop);
    bool isStopped() const;

    /** Reset dynamic minimum split depth to default value. */
    void resetSplitDepth();

    /** Add SplitPoint to work queue. */
    void addWork(const std::shared_ptr<SplitPoint>& sp);

    /** Try to add SplitPoint to work queue. If queue is contended add SplitPoint
     * to pending instead. */
    void tryAddWork(const std::shared_ptr<SplitPoint>& sp,
                    std::vector<std::shared_ptr<SplitPoint>>& pending);

    /** Get best move for helper thread to work on. */
    std::shared_ptr<SplitPoint> getWork(int& moveNo, ParallelData& pd, int threadNo);
    std::shared_ptr<SplitPoint> getWork(int& moveNo, ParallelData& pd, int threadNo,
                                        int& prio, double& pUseful);

    /** A helper thread stopped working on a move before it was finished. */
    void returnMove(const std::shared_ptr<SplitPoint>& sp, int moveNo);

    /** Set which move number the SplitPoint owner is currently searching.
     * @return Score from helper search thread. UNKNOWN_SCORE if no helper has
     *         searched the move. */
    int setOwnerCurrMove(const std::shared_ptr<SplitPoint>& sp, int moveNo, int alpha);

    /** Cancel this SplitPoint and all children. */
    void cancel(const std::shared_ptr<SplitPoint>& sp);

    /** Set move to canceled after helper thread finished searching it. */
    void moveFinished(const std::shared_ptr<SplitPoint>& sp, int moveNo,
                      bool cancelRemaining, int score);

    /** Return probability that the best unstarted move needs to be searched.
     *  Also return the corresponding SplitPoint. */
    double getBestProbability(std::shared_ptr<SplitPoint>& bestSp) const;
    double getBestProbability() const;
    int getBestPrio() const;

    /** Return current dynamic minimum split depth. */
    int getMinSplitDepth() const;

    /** For performance measurements on queue operations. */
    void resetStat();
    TimeSampleStatistics& getAddWorkStat(int th);
    TimeSampleStatistics& getGetWorkStat(int th);
    void printStats(std::ostream& os, int nThreads);

private:
    /** Insert sp in queue. Notify condition variable if queue becomes non-empty. */
    void insertInQueue(const std::shared_ptr<SplitPoint>& sp);

    /** Recompute probabilities for "sp" and all children. Update "queue" and "waiting". */
    void updateProbabilities(const std::shared_ptr<SplitPoint>& sp);

    /** Cancel "sp" and all children. Assumes mutex already locked. */
    void cancelInternal(const std::shared_ptr<SplitPoint>& sp);

    void printSpTree(std::ostream& os, const ParallelData& pd,
                     int threadNo, const std::shared_ptr<SplitPoint> selectedSp,
                     int selectedMove);
    void findLeaves(const std::shared_ptr<SplitPoint>& sp, std::vector<int>& parentThreads,
                    std::vector<std::shared_ptr<SplitPoint>>& leaves);

    /** Helper for addWork() and tryAddWork(). */
    void addWorkInternal(const std::shared_ptr<SplitPoint>& sp);

    /** Scoped lock that measures lock contention and adjusts minSplitDepth accordingly. */
    class Lock {
    public:
        Lock(const WorkQueue* wq0);
        void wait(std::condition_variable& cv);
    private:
        const WorkQueue& wq;
        std::unique_lock<std::mutex> lock;
    };
    friend class Lock;
    std::atomic<bool> stopped;

    mutable int minSplitDepth;      // Dynamic minimum split depth
    mutable U64 nContended;         // Number of times mutex has been contended
    mutable U64 nNonContended;      // Number of times mutex has not been contended

    std::condition_variable cv;     // Notified when wq becomes non-empty and when search should stop
    const FailHighInfo& fhInfo;
    const DepthNpsInfo& npsInfo;
    mutable std::mutex mutex;

    // SplitPoints with unstarted SplitPointMoves
    // SplitPoints with no unstarted moves have negative priority.
    Heap<SplitPoint> queue;

    // For performance debugging
    static const int maxStatThreads = 64;
    TimeSampleStatisticsVector<maxStatThreads*2> wqStat;
};


/** Fail high statistics, for estimating SplitPoint usefulness probabilities. */
class FailHighInfo {
public:
    /** Constructor. */
    FailHighInfo();

    /** Return probability that move moveNo needs to be searched.
     * @param parentMoveNo  Move number of move leading to this position.
     * @param currMoveNo    Move currently being searched.
     * @param moveNo        Move number to get probability for.
     * @param allNode       True if this is an expected ALL node. */
    double getMoveNeededProbability(int parentMoveNo, int currMoveNo, int moveNo, bool allNode) const;

    /** Return probability that move moveNo needs to be searched in a PV node.
     * @param currMoveNo    Move currently being searched.
     * @param moveNo        Move number to get probability for.
     * @param allNode       True if this is an expected ALL node. */
    double getMoveNeededProbabilityPv(int currMoveNo, int moveNo) const;

    /** Add fail high information.
     * @param parentMoveNo  Move number of move leading to this position.
     * @param nSearched     Number of moves searched at this node.
     * @param failHigh      True if the node failed high.
     * @param allNode       True if this is an expected ALL node. */
    void addData(int parentMoveNo, int nSearched, bool failHigh, bool allNode);

    /** Add "alpha increased" information for PV nodes.
     * @param  nSearched     Number of moves searched at this node.
     * @param  alphaChanged  True if alpha increased after searching move. */
    void addPvData(int nSearched, bool alphaChanged);

    /** Rescale the counters so that future updates have more weight. */
    void reScale();

    /** Print object state to "os", for debugging. */
    void print(std::ostream& os) const;

private:
    void reScaleInternal(int factor);
    void reScalePv(int factor);

    int getNodeType(int moveNo, bool allNode) const;

    mutable std::mutex mutex;

    static const int NUM_NODE_TYPES = 4;
    static const int NUM_STAT_MOVES = 15;

    RangeSumArray<NUM_STAT_MOVES> failHiCount[NUM_NODE_TYPES]; // [parentMoveNo>0?1:0][moveNo]
    std::atomic<int> failLoCount[NUM_NODE_TYPES];              // [parentMoveNo>0?1:0]
    std::atomic<int> totCount;                                 // Sum of all counts

    std::atomic<int> newAlpha[NUM_STAT_MOVES];
    std::atomic<int> totPvCount;
};

class DepthNpsInfo {
public:
    /** Constructor. */
    DepthNpsInfo();

    /** Reset */
    void reset();

    /** Set thread 0 estimated nps. Used to scale efficiency calculations
     *  to [0,1] interval. */
    void setBaseNps(double nps);

    /** Add one data point. */
    void addData(int depth, U64 nodes, double waitTime, double searchTime);

    /** Return estimate search efficiency when searching to a given depth. */
    double efficiency(int depth) const;

    /** Print object state to "os", for debugging. */
    void print(std::ostream& os, int iterDepth);

    /** Maximum depth statistics is kept for. */
    static const int maxDepth = 48;

private:
    /** Helper method for efficiency() and print(). */
    double efficiencyInternal(int depth) const;

    mutable std::mutex mutex;

    struct NpsData {
        NpsData();
        U32 nSearches;
        U64 nodes;
        double time;
    };
    NpsData npsData[maxDepth];
    double nps0;
    U32 nSearches;      // Total number of searches
    double waitTime;    // Total waiting time
};

/** Top-level parallel search data structure. */
class ParallelData {
public:
    /** Constructor. */
    ParallelData(TranspositionTable& tt);

    /** Create/delete worker threads so that there are numWorkers in total. */
    void addRemoveWorkers(int numWorkers);

    /** Start all worker threads. */
    void startAll();

    /** Stop all worker threads. */
    void stopAll();


    /** Return number of helper threads in use. */
    int numHelperThreads() const;

    /** Get number of nodes searched by all helper threads. */
    S64 getNumSearchedNodes() const;

    /** Get number of TB hits for all helper threads. */
    S64 getTbHits() const;

    /** Add nNodes to total number of searched nodes. */
    void addSearchedNodes(S64 nNodes);

    /** Add nTbHits to number of TB hits. */
    void addTbHits(S64 nTbHits);


    /** For debugging. */
    const WorkerThread& getHelperThread(int i) const;

    FailHighInfo fhInfo;
    DepthNpsInfo npsInfo;

    WorkQueue wq;

    /** Move played in Search::iterativeDeepening before calling negaScout. For debugging. */
    Move topMove;

    /** Current node index in thread 0. Used by tree logging code. */
    std::atomic<U32> t0Index;

private:
    /** Vector of helper threads. Master thread not included. */
    std::vector<std::shared_ptr<WorkerThread>> threads;

    TranspositionTable& tt;

    std::atomic<S64> totalHelperNodes; // Number of nodes searched by all helper threads
    std::atomic<S64> helperTbHits;     // Number of TB hits for all helper threads
};


/** SplitPoint does not handle thread safety, WorkQueue must do that.  */
class SplitPoint : public Heap<SplitPoint>::HeapObject {
    friend class ParallelTest;
public:
    /** Constructor. */
    SplitPoint(int threadNo,
               const std::shared_ptr<SplitPoint>& parentSp, int parentMoveNo,
               const Position& pos, const std::vector<U64>& posHashList,
               int posHashListSize, const SearchTreeInfo& sti,
               const KillerTable& kt, const History& ht,
               int alpha, int beta, int ply, int depth);

    SplitPoint(const SplitPoint&) = delete;
    SplitPoint& operator=(const SplitPoint&) = delete;

    /** Add a child SplitPoint */
    void addChild(const std::weak_ptr<SplitPoint>& child);

    /** Add a move to the SplitPoint. */
    void addMove(int moveNo, const SplitPointMove& spMove);

    /** Assign sequence number. */
    void setSeqNo();

    /** compute pSpUseful and pnextMoveUseful. */
    void computeProbabilities(const FailHighInfo& fhInfo, const DepthNpsInfo& npsInfo);

    /** Get parent SplitPoint, or null for root SplitPoint. */
    std::shared_ptr<SplitPoint> getParent() const;

    /** Get children SplitPoints. */
    const std::vector<std::weak_ptr<SplitPoint>>& getChildren() const;


    U64 getSeqNo() const;
    double getPSpUseful() const;
    double getPNextMoveUseful() const;
    const History& getHistory() const;
    const KillerTable& getKillerTable() const;
    const SplitPointMove& getSpMove(int moveNo) const;

    /** Return probability that moveNo needs to be searched. */
    double getPMoveUseful(const FailHighInfo& fhInfo, int moveNo) const;

    void getPos(Position& pos, const Move& move) const;
    void getPosHashList(const Position& pos, std::vector<U64>& posHashList,
                        int& posHashListSize) const;
    const SearchTreeInfo& getSearchTreeInfo() const;
    int getAlpha() const;
    int getBeta() const;
    int getPly() const;
    int getDepth() const;


    /** Get index of first unstarted move. Mark move as being searched.
     * Return -1 if there are no unstarted moves. */
    int getNextMove(const FailHighInfo& fhInfo);

    /** A helper thread stopped working on a move before it was finished. */
    void returnMove(int moveNo);

    /** Set which move number the SplitPoint owner is currently searching.
     * @return Score from helper search thread. UNKNOWN_SCORE if no helper has
     *         searched the move. */
    int setOwnerCurrMove(int moveNo, int alpha);

    /** Cancel this SplitPoint and all children. */
    void cancel();

    /** Return true if this SplitPoint has been canceled. */
    bool isCanceled() const;

    /** Set move to canceled after helper thread finished searching it. */
    void moveFinished(int moveNo, bool cancelRemaining, int score);

    /** Return true if there are moves that have not been started to be searched. */
    bool hasUnStartedMove() const;

    /** Return true if there are moves that have not been finished (canceled) yet. */
    bool hasUnFinishedMove() const;

    /** Return true if this SplitPoint is an ancestor to "sp". */
    bool isAncestorTo(const SplitPoint& sp) const;

    /** Return true if some other thread is helping the SplitPoint owner. */
    bool hasHelperThread() const;

    /** Return true if the held SplitPoint is an estimated ALL node. */
    bool isAllNode() const;

    /** Return true if beta > alpha + 1. */
    bool isPvNode() const;

    /** Compute SplitPoint priority. The SplitPoint with highest
     * priority will be selected first by the next available thread. */
    int getSpPrio(const DepthNpsInfo& npsInfo) const;

    /** Thread that created this SplitPoint. */
    int owningThread() const;

    /** Return true if object is or has been inserted in WorkQueue. */
    bool wasInserted() const;

    /** Mark SplitPoint as inserted in WorkQueue. */
    void setInserted();

    /** Print object state to "os", for debugging. */
    void print(std::ostream& os, int level, const FailHighInfo& fhInfo) const;

    /** For debugging. */
    int getParentMoveNo() const;

    /** For debugging. */
    int getCurrMoveNo() const;

    /** Get index of first unstarted move, or -1 if there is no unstarted move. */
    int findNextMove(const FailHighInfo& fhInfo) const;

private:
    /** Return probability that moveNo needs to be searched, by calling corresponding
     * function in fhInfo. */
    double getMoveNeededProbability(const FailHighInfo& fhInfo, int moveNo) const;

    /** Remove null entries from children vector. */
    void cleanUpChildren();

    const Position pos;
    const std::vector<U64> posHashList; // List of hashes for previous positions up to the last "zeroing" move.
    const int posHashListSize;
    const SearchTreeInfo searchTreeInfo;   // For ply-1
    const KillerTable& kt;
    const History& ht;

    RelaxedShared<int> alpha;
    const int beta;
    const int ply;
    const int depth;
    const bool isPV;

    double pSpUseful;       // Probability that this SplitPoint is needed. 100% if parent is null.
    double pNextMoveUseful; // Probability that next unstarted move needs to be searched.

    const int threadNo;     // Owning thread
    const std::shared_ptr<SplitPoint> parent;
    const int parentMoveNo; // Move number in parent SplitPoint that generated this SplitPoint
    std::vector<std::weak_ptr<SplitPoint>> children;

    static U64 nextSeqNo;
    U64 seqNo;      // To break ties when two objects have same priority. Set by addWork() under lock
    int currMoveNo;
    std::vector<SplitPointMove> spMoves;

    bool inserted; // True if sp has been inserted in WorkQueue. Remains true after deletion.
    bool canceled;
};


/** Represents one move at a SplitPoint. */
class SplitPointMove {
public:
    /** Constructor */
    SplitPointMove(const Move& move, int lmr, int depth,
                   int captSquare, bool inCheck);

    const Move& getMove() const;
    int getLMR() const;
    int getDepth() const;
    int getRecaptureSquare() const;
    bool getInCheck() const;

    bool isCanceled() const;
    void setCanceled(bool value);

    bool isSearching() const;
    void setSearching(bool value);

    void setScore(int s);
    int getScore() const;

private:
    Move move;      // Position defined by sp->pos + move
    int lmr;        // Amount of LMR reduction
    int depth;
    int captSquare; // Recapture square, or -1 if no recapture
    bool inCheck;   // True if side to move is in check

    RelaxedShared<bool> canceled; // Result is no longer needed
    bool searching; // True if currently searched by a helper thread
    int score;
};


/** Handle a SplitPoint object using RAII. */
class SplitPointHolder {
public:
    /** Constructor. */
    SplitPointHolder(ParallelData& pd, std::vector<std::shared_ptr<SplitPoint>>& spVec,
                     std::vector<std::shared_ptr<SplitPoint>>& pending);

    /** Destructor. Cancel SplitPoint. */
    ~SplitPointHolder();

    SplitPointHolder(const SplitPointHolder&) = delete;
    SplitPointHolder& operator=(const SplitPointHolder&) = delete;

    /** Set the SplitPoint object. */
    void setSp(const std::shared_ptr<SplitPoint>& sp);

    /** Add a move to the SplitPoint. */
    void addMove(int moveNo, const SplitPointMove& spMove);

    /** Add SplitPoint to work queue. If the queue is contended, store SplitPoint
     * in pending vector instead. If queue is not contended, also insert all
     * objects from the pending vector and clear the pending vector. */
    void addToQueue();

    /** Set which move number the SplitPoint owner is currently searching.
     * @return Score from helper search thread. UNKNOWN_SCORE if no helper has
     *         searched the move. */
    int setOwnerCurrMove(int moveNo, int alpha);

    /** For debugging. */
    U64 getSeqNo() const;

    /** Return true if the held SplitPoint is an estimated ALL node. */
    bool isAllNode() const;

    /** Return true if some other thread is helping the help SplitPoint. */
    bool hasHelperThread() const;

private:
    ParallelData& pd;
    std::vector<std::shared_ptr<SplitPoint>>& spVec;
    std::vector<std::shared_ptr<SplitPoint>>& pending;
    std::shared_ptr<SplitPoint> sp;
    enum class State { EMPTY, CREATED, QUEUED } state;
};

/** Dummy version of SplitPointHolder class. */
struct DummySplitPointHolder {
    DummySplitPointHolder(ParallelData& pd, std::vector<std::shared_ptr<SplitPoint>>& spVec,
                          std::vector<std::shared_ptr<SplitPoint>>& pending) {}
    void setSp(const std::shared_ptr<SplitPoint>& sp) {}
    void addMove(int moveNo, const SplitPointMove& spMove) {}
    void addToQueue() {}
    int setOwnerCurrMove(int moveNo, int alpha) { return SearchConst::UNKNOWN_SCORE; }
    bool isAllNode() const { return false; }
};

template <bool smp> struct SplitPointTraits {
};
template<> struct SplitPointTraits<true> {
    using SpHolder = SplitPointHolder;
};
template<> struct SplitPointTraits<false> {
    using SpHolder = DummySplitPointHolder;
};


inline bool
WorkerThread::threadRunning() const {
    return thread != nullptr;
}

inline int
WorkerThread::getThreadNo() const {
    return threadNo;
}

inline double
WorkerThread::getPUseful() const {
    return pUseful;
}

inline
WorkQueue::WorkQueue(const FailHighInfo& fhInfo0, const DepthNpsInfo& npsInfo0)
    : stopped(false), fhInfo(fhInfo0), npsInfo(npsInfo0) {
    resetSplitDepth();
}

inline bool
WorkQueue::isStopped() const {
    return stopped;
}

inline void
WorkQueue::resetSplitDepth() {
    minSplitDepth = SearchConst::MIN_SMP_DEPTH;
    nContended = 0;
    nNonContended = 0;
}

inline int
WorkQueue::getMinSplitDepth() const {
    return minSplitDepth;
}

inline void
WorkQueue::insertInQueue(const std::shared_ptr<SplitPoint>& sp) {
    bool wasEmpty = queue.empty() || (queue.front()->getPrio() < 0);
    queue.insert(sp, sp->getSpPrio(npsInfo));
    sp->setInserted();
    if (wasEmpty)
        cv.notify_all();
}

inline void
WorkQueue::resetStat() {
    for (auto& s : wqStat)
        s.reset();
}

inline TimeSampleStatistics&
WorkQueue::getAddWorkStat(int th) {
    assert(th < maxStatThreads);
    return wqStat[th];
}

inline TimeSampleStatistics&
WorkQueue::getGetWorkStat(int th) {
    assert(th < maxStatThreads);
    return wqStat[maxStatThreads+th];
}

inline void
WorkQueue::printStats(std::ostream& os, int nThreads) {
    for (int i = 0; i < nThreads; i++) {
        os << "th:" << i << " add: ";
        getAddWorkStat(i).printNs(os);
        os << " get: ";
        getGetWorkStat(i).printNs(os);
        os << std::endl;
    }
}


inline int
FailHighInfo::getNodeType(int moveNo, bool allNode) const {
    if (moveNo == 0)
        return allNode ? 0 : 3;
    else if (moveNo > 0)
        return 1;
    else
        return 2;
}

inline
DepthNpsInfo::DepthNpsInfo() {
    reset();
}

inline
DepthNpsInfo::NpsData::NpsData()
    : nSearches(0), nodes(0), time(0) {
}

inline void
DepthNpsInfo::reset() {
    std::lock_guard<std::mutex> L(mutex);
    for (int i = 0; i < maxDepth; i++) {
        npsData[i].nSearches = 0;
        npsData[i].nodes = 0;
        npsData[i].time = 0;
    }
    nps0 = 0;
    nSearches = 0;
    waitTime = 0;
}

inline void
DepthNpsInfo::setBaseNps(double nps) {
    std::lock_guard<std::mutex> L(mutex);
    nps0 = nps;
}

inline void
DepthNpsInfo::addData(int depth, U64 nodes, double wTime, double searchTime) {
    if (depth >= maxDepth)
        depth = maxDepth - 1;
    std::lock_guard<std::mutex> L(mutex);
    npsData[depth].nSearches++;
    npsData[depth].nodes += nodes;
    npsData[depth].time += searchTime;
    nSearches++;
    waitTime += wTime;
}

inline double
DepthNpsInfo::efficiency(int depth) const {
    if (depth >= maxDepth)
        depth = maxDepth - 1;
    std::lock_guard<std::mutex> L(mutex);
    return efficiencyInternal(depth);
}

inline double
DepthNpsInfo::efficiencyInternal(int depth) const {
    if ((npsData[depth].time > 0) && (nps0 > 0)) {
        U64 nodes = npsData[depth].nodes;
        double time = npsData[depth].time;
        const U32 ns = npsData[depth].nSearches;
        if (nSearches > 0)
            time += waitTime / nSearches * ns;
        double nps = nodes / time;
        double eff = nps / nps0;
        if (eff > 1.0)
            eff = 1.0;
        return (eff * ns + 1 * 500) / (ns + 500);
    } else
        return 1.0;
}

inline void
WorkQueue::Lock::wait(std::condition_variable& cv) {
    cv.wait(lock);
}


inline ParallelData::ParallelData(TranspositionTable& tt0)
    : wq(fhInfo, npsInfo), t0Index(0), tt(tt0),
      totalHelperNodes(0), helperTbHits(0) {
}

inline int
ParallelData::numHelperThreads() const {
    return (int)threads.size();
}

inline S64
ParallelData::getNumSearchedNodes() const {
    return totalHelperNodes;
}

inline S64
ParallelData::getTbHits() const {
    return helperTbHits;
}

inline void
ParallelData::addSearchedNodes(S64 nNodes) {
    totalHelperNodes += nNodes;
}

inline void
ParallelData::addTbHits(S64 nTbHits) {
    helperTbHits += nTbHits;
}

inline const WorkerThread&
ParallelData::getHelperThread(int i) const {
    return *threads[i];
}


inline bool
SplitPoint::isCanceled() const {
    return canceled;
}

inline void
SplitPoint::setSeqNo() {
    seqNo = nextSeqNo++;
}

inline std::shared_ptr<SplitPoint>
SplitPoint::getParent() const {
    return parent;
}

inline const std::vector<std::weak_ptr<SplitPoint>>&
SplitPoint::getChildren() const {
    return children;
}

inline U64
SplitPoint::getSeqNo() const {
    return seqNo;
}

inline double
SplitPoint::getPSpUseful() const {
    return pSpUseful;
}

inline double
SplitPoint::getPNextMoveUseful() const {
    return pNextMoveUseful;
}

inline const History&
SplitPoint::getHistory() const {
    return ht;
}

inline const KillerTable&
SplitPoint::getKillerTable() const {
    return kt;
}

inline const SplitPointMove&
SplitPoint::getSpMove(int moveNo) const {
    return spMoves[moveNo];
}

inline const SearchTreeInfo&
SplitPoint::getSearchTreeInfo() const {
    return searchTreeInfo;
}

inline int
SplitPoint::getAlpha() const {
    return alpha;
}

inline int
SplitPoint::getBeta() const {
    return beta;
}

inline int
SplitPoint::getPly() const {
    return ply;
}

inline int
SplitPoint::getDepth() const {
    return depth;
}

inline void
SplitPoint::returnMove(int moveNo) {
    assert((moveNo >= 0) && (moveNo < (int)spMoves.size()));
    SplitPointMove& spm = spMoves[moveNo];
    spm.setSearching(false);
}

inline int
SplitPoint::setOwnerCurrMove(int moveNo, int newAlpha) {
    assert((moveNo >= 0) && (moveNo < (int)spMoves.size()));
    int score = spMoves[moveNo].getScore();
    spMoves[moveNo].setCanceled(true);
    currMoveNo = moveNo;
    if (newAlpha > alpha)
        alpha = newAlpha;
    return score;
}

inline void
SplitPoint::cancel() {
    canceled = true;
    for (SplitPointMove& spMove : spMoves)
        spMove.setCanceled(true);
}

inline void
SplitPoint::addChild(const std::weak_ptr<SplitPoint>& child) {
    children.push_back(child);
}

inline bool
SplitPoint::isPvNode() const {
    return isPV;
}

inline int
SplitPoint::getSpPrio(const DepthNpsInfo& npsInfo) const {
    if (!hasUnStartedMove())
        return -1;
    return (int)(getPNextMoveUseful() * npsInfo.efficiency(getDepth()) * 1000);
}

inline int
SplitPoint::owningThread() const {
    return threadNo;
}

inline bool
SplitPoint::wasInserted() const {
    return inserted;
}

inline void
SplitPoint::setInserted() {
    inserted = true;
}

inline int
SplitPoint::getParentMoveNo() const {
    return parentMoveNo;
}

inline int
SplitPoint::getCurrMoveNo() const {
    return currMoveNo;
}


inline
SplitPointMove::SplitPointMove(const Move& move0, int lmr0, int depth0,
                              int captSquare0, bool inCheck0)
    : move(move0), lmr(lmr0), depth(depth0), captSquare(captSquare0),
      inCheck(inCheck0), canceled(false), searching(false),
      score(SearchConst::UNKNOWN_SCORE) {
}

inline const Move&
SplitPointMove::getMove() const {
    return move;
}

inline int
SplitPointMove::getLMR() const {
    return lmr;
}

inline int
SplitPointMove::getDepth() const {
    return depth;
}

inline int
SplitPointMove::getRecaptureSquare() const {
    return captSquare;
}

inline bool
SplitPointMove::getInCheck() const {
    return inCheck;
}

inline bool
SplitPointMove::isCanceled() const {
    return canceled;
}

inline void
SplitPointMove::setCanceled(bool value) {
    canceled = value;
}

inline bool
SplitPointMove::isSearching() const {
    return searching;
}

inline void
SplitPointMove::setSearching(bool value) {
    searching = value;
}

inline void
SplitPointMove::setScore(int s) {
    score = s;
}

inline int
SplitPointMove::getScore() const {
    return score;
}


inline
SplitPointHolder::SplitPointHolder(ParallelData& pd0,
                                   std::vector<std::shared_ptr<SplitPoint>>& spVec0,
                                   std::vector<std::shared_ptr<SplitPoint>>& pending0)
    : pd(pd0), spVec(spVec0), pending(pending0),
      state(State::EMPTY) {
}

inline
SplitPointHolder::~SplitPointHolder() {
    if (state == State::QUEUED) {
//        log([&](std::ostream& os){os << "cancel seqNo:" << sp->getSeqNo();});
        if (sp->wasInserted())
            pd.wq.cancel(sp);
        else {
            if (pending.size() > 0) {
                assert(pending[pending.size()-1] == sp);
                pending.pop_back();
            }
        }
        assert(!spVec.empty());
        spVec.pop_back();
    }
}

inline void
SplitPointHolder::addMove(int moveNo, const SplitPointMove& spMove) {
    assert(state == State::CREATED);
    sp->addMove(moveNo, spMove);
}

inline int
SplitPointHolder::setOwnerCurrMove(int moveNo, int alpha) {
//    if (sp->hasHelperThread())
//        log([&](std::ostream& os){os << "seqNo:" << sp->getSeqNo() << " currMove:" << moveNo
//                                     << " a:" << alpha;});
    if (sp->wasInserted())
        return pd.wq.setOwnerCurrMove(sp, moveNo, alpha);
    else
        return sp->setOwnerCurrMove(moveNo, alpha);
}

inline U64
SplitPointHolder::getSeqNo() const {
    return sp->getSeqNo();
}

inline bool
SplitPointHolder::isAllNode() const {
    return sp->isAllNode();
}

inline bool
SplitPointHolder::hasHelperThread() const {
    return sp->hasHelperThread();
}

#endif /* PARALLEL_HPP_ */
