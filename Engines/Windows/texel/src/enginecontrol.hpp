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
 * enginecontrol.hpp
 *
 *  Created on: Mar 4, 2012
 *      Author: petero
 */

#ifndef ENGINECONTROL_HPP_
#define ENGINECONTROL_HPP_

#include "transpositionTable.hpp"
#include "position.hpp"
#include "move.hpp"
#include "parallel.hpp"
#include "history.hpp"
#include "killerTable.hpp"

#include <vector>
#include <map>
#include <iosfwd>
#include <thread>
#include <mutex>
#include <memory>
#include <atomic>

class MoveList;
class Search;
class SearchParams;
class SearchListener;
class EngineControl;

/** State needed by the main engine search thread. */
class EngineMainThread {
public:
    EngineMainThread();
    ~EngineMainThread();
    EngineMainThread(const EngineMainThread&) = delete;
    EngineMainThread& operator=(const EngineMainThread&) = delete;

    /** Called by the main search thread. Waits for and acts upon start and quit
     *  calls from another thread. */
    void mainLoop();

    /** Tells the main loop to terminate. */
    void quit();

    void setupTT();
    TranspositionTable& getTT();

    /** Tell the search thread to start searching. */
    void startSearch(EngineControl* engineControl,
                     std::shared_ptr<Search>& sc, const Position& pos,
                     std::shared_ptr<MoveList>& moves,
                     bool ownBook, bool analyseMode,
                     int maxDepth, int maxNodes,
                     int maxPV, int minProbeDepth,
                     std::atomic<bool>& ponder, std::atomic<bool>& infinite);

    /** Wait for the search thread to stop searching. */
    void waitStop();

    /** Set UCI option as soon as search threads are idle. */
    void setOptionWhenIdle(const std::string& optionName,
                           const std::string& optionValue);

    /** Wait until all changes requested by setOptionWhenIdle() have been made. */
    void waitOptionsSet();

    Communicator* getCommunicator() const;

    /** Clear history tables in all helper threads when starting next search. */
    void setClearHistory();

private:
    void doSearch();
    void setOptions();

    /** Wait for notifier. If cluster is enabled, only wait a short period of time
     *  since MPI communication needs polling. */
    void notifierWait();

    Notifier notifier;
    TranspositionTable tt;
    std::unique_ptr<ThreadCommunicator> comm;
    std::vector<std::shared_ptr<WorkerThread>> children;

    std::mutex mutex;
    std::condition_variable searchStopped;
    std::condition_variable optionsSet;    // To wait for UCI options to be set
    std::atomic<bool> search { false };
    std::atomic<bool> quitFlag { false };

    EngineControl* engineControl = nullptr;
    std::shared_ptr<Search> sc;
    Position pos;
    std::shared_ptr<MoveList> moves;
    bool ownBook = false;
    bool analyseMode = false;
    int maxDepth = -1;
    int maxNodes = -1;
    int maxPV = 1;
    int minProbeDepth = 0;
    std::atomic<bool>* ponder = nullptr;
    std::atomic<bool>* infinite = nullptr;
    bool clearHistory = false;

    std::map<std::string, std::string> pendingOptions;
    bool optionsSetFinished = true;
};

/**
 * Control the search thread.
 */
class EngineControl {
public:
    EngineControl(std::ostream& o, EngineMainThread& engineThread, SearchListener& listener);
    ~EngineControl();

    void startSearch(const Position& pos, const std::vector<Move>& moves, const SearchParams& sPar);

    void startPonder(const Position& pos, const std::vector<Move>& moves, const SearchParams& sPar);

    void ponderHit();

    void stopSearch();

    void newGame();

    static void printOptions(std::ostream& os);

    void setOption(const std::string& optionName, const std::string& optionValue);

    /** If the engine is not searching, wait until all pending options have been processed. */
    void waitReady();

    void finishSearch(Position& pos, const Move& bestMove);

private:
    /**
     * Compute thinking time for current search.
     */
    void computeTimeLimit(const SearchParams& sPar);

    void startThread(int minTimeLimit, int maxTimeLimit, int earlyStopPercentage,
                     int maxDepth, int maxNodes);

    void stopThread();

    void setupPosition(Position pos, const std::vector<Move>& moves);

    /**
     * Try to find a move to ponder from the transposition table.
     */
    Move getPonderMove(Position pos, const Move& m);


    std::ostream& os;

    int hashParListenerId;
    int clearHashParListenerId;

    EngineMainThread& engineThread;
    SearchListener& listener;
    std::shared_ptr<Search> sc;
    KillerTable kt;
    History ht;
    std::unique_ptr<Evaluate::EvalHashTables> et;
    TreeLogger treeLog;

    Position pos;
    std::vector<U64> posHashList;
    int posHashListSize;
    std::atomic<bool> ponder;     // True if currently doing pondering
    bool onePossibleMove;
    std::atomic<bool> infinite;

    int minTimeLimit;
    int maxTimeLimit;
    int earlyStopPercentage;
    int maxDepth;
    int maxNodes;
    std::vector<Move> searchMoves;

    // Random seed for reduced strength
    U64 randomSeed;
};

inline TranspositionTable&
EngineMainThread::getTT() {
    return tt;
}

inline Communicator*
EngineMainThread::getCommunicator() const {
    return comm.get();
}

inline void
EngineMainThread::setClearHistory() {
    clearHistory = true;
}

#endif /* ENGINECONTROL_HPP_ */
