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
 * enginecontrol.hpp
 *
 *  Created on: Mar 4, 2012
 *      Author: petero
 */

#ifndef ENGINECONTROL_HPP_
#define ENGINECONTROL_HPP_

#include "search.hpp"
#include "transpositionTable.hpp"
#include "position.hpp"
#include "move.hpp"

#include <vector>
#include <iosfwd>
#include <thread>
#include <mutex>
#include <memory>
#include <atomic>

class SearchParams;


/**
 * Control the search thread.
 */
class EngineControl {
public:
    EngineControl(std::ostream& o);
    ~EngineControl();

    void startSearch(const Position& pos, const std::vector<Move>& moves, const SearchParams& sPar);

    void startPonder(const Position& pos, const std::vector<Move>& moves, const SearchParams& sPar);

    void ponderHit();

    void stopSearch();

    void newGame();

    /**
     * Compute thinking time for current search.
     */
    void computeTimeLimit(const SearchParams& sPar);

    static void printOptions(std::ostream& os);

    void setOption(const std::string& optionName, const std::string& optionValue,
                   bool deferIfBusy);

private:
    /**
     * This class is responsible for sending "info" strings during search.
     */
    class SearchListener : public Search::Listener {
    public:
        SearchListener(std::ostream& os0);

        void notifyDepth(int depth) override;

        void notifyCurrMove(const Move& m, int moveNr) override;

        void notifyPV(int depth, int score, int time, U64 nodes, int nps, bool isMate,
                      bool upperBound, bool lowerBound, const std::vector<Move>& pv,
                      int multiPVIndex, U64 tbHits) override;

        void notifyStats(U64 nodes, int nps, U64 tbHits, int time) override;

    private:
        std::ostream& os;
    };

    void startThread(int minTimeLimit, int maxTimeLimit, int maxDepth, int maxNodes);

    void stopThread();

    void setupTT();

    void setupPosition(Position pos, const std::vector<Move>& moves);

    /**
     * Try to find a move to ponder from the transposition table.
     */
    Move getPonderMove(Position pos, const Move& m);

    static std::string moveToString(const Move& m);


    std::ostream& os;

    int hashParListenerId;
    int clearHashParListenerId;
    std::map<std::string, std::string> pendingOptions;

    std::shared_ptr<std::thread> engineThread;
    std::mutex threadMutex;
    std::atomic<bool> shouldDetach;
    std::shared_ptr<Search> sc;
    TranspositionTable tt;
    ParallelData pd;
    KillerTable kt;
    History ht;
    std::shared_ptr<Evaluate::EvalHashTables> et;
    TreeLogger treeLog;

    Position pos;
    std::vector<U64> posHashList;
    int posHashListSize;
    std::atomic<bool> ponder;     // True if currently doing pondering
    bool onePossibleMove;
    std::atomic<bool> infinite;

    int minTimeLimit;
    int maxTimeLimit;
    int maxDepth;
    int maxNodes;
    std::vector<Move> searchMoves;

    // Random seed for reduced strength
    U64 randomSeed;
};


#endif /* ENGINECONTROL_HPP_ */
