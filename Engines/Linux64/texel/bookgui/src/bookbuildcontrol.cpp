/*
    Texel - A UCI chess engine.
    Copyright (C) 2016  Peter Österlund, peterosterlund2@gmail.com

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
 * bookbuildcontrol.cpp
 *
 *  Created on: Apr 2, 2016
 *      Author: petero
 */

#include "bookbuildcontrol.hpp"
#include "textio.hpp"
#include "search.hpp"
#include "computerPlayer.hpp"
#include "gametree.hpp"


BookBuildControl::BookBuildControl(ChangeListener& listener0)
    : listener(listener0), nPendingBookTasks(0), tt(27), pd(tt) {
    ComputerPlayer::initEngine();
    setupTB();
    et = Evaluate::getEvalHashTables();
    newBook();
}

BookBuildControl::~BookBuildControl() {
    stopAnalysis();
    stopSearch(true);
    std::unique_lock<std::mutex> L(mutex);
    while (bgThread || bgThread2)
        bgThreadCv.wait(L);
}

void
BookBuildControl::setupTB() {
    UciParams::gtbPath->set("/home/petero/chess/gtb");
    UciParams::gtbCache->set("2047");
    UciParams::rtbPath->set("/home/petero/chess/rtb/wdl:"
                            "/home/petero/chess/rtb/dtz:"
                            "/home/petero/chess/rtb/6wdl:"
                            "/home/petero/chess/rtb/6dtz");
}

void
BookBuildControl::getChanges(std::vector<Change>& changes) {
    std::lock_guard<std::mutex> L(mutex);
    for (Change c : this->changes)
        changes.push_back(c);
    this->changes.clear();
}

void
BookBuildControl::notify(Change change) {
    {
        std::lock_guard<std::mutex> L(mutex);
        changes.insert(change);
    }
    listener.notify();
}

// --------------------------------------------------------------------------------

void
BookBuildControl::newBook() {
    filename.clear();
    book = make_unique<BookBuild::Book>("emptybook.tbin.log", params.bookDepthCost,
                                        params.ownPathErrorCost,
                                        params.otherPathErrorCost);
}

void
BookBuildControl::readFromFile(const std::string& newFileName) {
    std::lock_guard<std::mutex> L(mutex);
    filename = newFileName;
    book = make_unique<BookBuild::Book>(filename + ".log", params.bookDepthCost,
                                        params.ownPathErrorCost,
                                        params.otherPathErrorCost);
    auto f = [this]() {
        book->readFromFile(filename);
        {
            std::lock_guard<std::mutex> L(mutex);
            bgThread->detach();
            bgThread.reset();
            bgThreadCv.notify_all();
        }
        notify(BookBuildControl::Change::OPEN_COMPLETE);
    };
    bgThread = std::make_shared<std::thread>(f);
}

void
BookBuildControl::saveToFile(const std::string& newFileName) {
    std::lock_guard<std::mutex> L(mutex);
    if (bgThread2)
        return;
    if (!newFileName.empty())
        filename = newFileName;

    auto f = [this]() {
        book->writeToFile(filename);
        {
            std::lock_guard<std::mutex> L(mutex);
            bgThread2->detach();
            bgThread2.reset();
            bgThreadCv.notify_all();
        }
        notify(BookBuildControl::Change::PROCESSING_COMPLETE);
    };
    bgThread2 = std::make_shared<std::thread>(f);
}

std::string
BookBuildControl::getBookFileName() const {
    return filename;
}

// --------------------------------------------------------------------------------

void
BookBuildControl::setParams(const Params& params) {
    std::lock_guard<std::mutex> L(mutex);
    this->params = params;
}

void
BookBuildControl::getParams(Params& params) {
    std::lock_guard<std::mutex> L(mutex);
    params = this->params;
}

// --------------------------------------------------------------------------------

void
BookBuildControl::startSearch() {
    std::lock_guard<std::mutex> L(mutex);
    class BookListener : public BookBuild::Book::Listener {
    public:
        BookListener(BookBuildControl& bbc0) : bbc(bbc0) {}
        void queueChanged(int nPendingBookTasks) override {
            {
                std::lock_guard<std::mutex> L(bbc.mutex);
                bbc.nPendingBookTasks = nPendingBookTasks;
            }
            bbc.notify(BookBuildControl::Change::QUEUE);
            bbc.notify(BookBuildControl::Change::TREE);
        }
    private:
        BookBuildControl& bbc;
    };
    book->setListener(make_unique<BookListener>(*this));
    stopFlag.store(false);
    nPendingBookTasks = 1;

    auto f = [this]() {
        book->interactiveExtendBook(params.computationTime,
                                    params.nThreads, tt,
                                    focusHash, stopFlag);
        book->setListener(nullptr);
        {
            std::lock_guard<std::mutex> L(mutex);
            bgThread->detach();
            bgThread.reset();
            bgThreadCv.notify_all();
        }
        notify(BookBuildControl::Change::QUEUE);
        notify(BookBuildControl::Change::TREE);
    };
    bgThread = std::make_shared<std::thread>(f);
}

void
BookBuildControl::stopSearch(bool immediate) {
    stopFlag.store(true);
    if (immediate)
        book->abortExtendBook();
}

void
BookBuildControl::nextGeneration() {
    tt.nextGeneration();
}

int
BookBuildControl::numPendingBookTasks() const {
    std::lock_guard<std::mutex> L(mutex);
    return nPendingBookTasks;
}

// --------------------------------------------------------------------------------

bool
BookBuildControl::getTreeData(const Position& pos,
                              BookBuild::Book::TreeData& treeData) const {
    return book->getTreeData(pos, treeData);
}

void
BookBuildControl::getBookData(BookData& bookData) const {

}

void
BookBuildControl::getQueueData(BookBuild::Book::QueueData& queueData) const {
    book->getQueueData(queueData);
}

// --------------------------------------------------------------------------------

void
BookBuildControl::setFocus(const Position& pos) {
    focusHash.store(pos.bookHash());
}

bool
BookBuildControl::getFocus(Position& pos, std::vector<Move>& movesBefore,
                           std::vector<Move>& movesAfter) {
    return book->getBookPV(focusHash.load(), pos, movesBefore, movesAfter);
}

U64
BookBuildControl::getFocusHash() const {
   return focusHash.load();
}

bool
BookBuildControl::getBookPV(U64 bookHash, Position& pos, std::vector<Move>& movesBefore,
                            std::vector<Move>& movesAfter) const {
    return book->getBookPV(bookHash, pos, movesBefore, movesAfter);
}

// --------------------------------------------------------------------------------

void
BookBuildControl::importPGN(const GameTree& gt, int maxPly) {
    std::lock_guard<std::mutex> L(mutex);
    if (bgThread2)
        return;
    auto f = [this, gt, maxPly]() {
        int nAdded = 0;
        GameNode gn = gt.getRootNode();
        book->addToBook(maxPly, gn, nAdded);
        {
            std::lock_guard<std::mutex> L(mutex);
            bgThread2->detach();
            bgThread2.reset();
            bgThreadCv.notify_all();
        }
        notify(BookBuildControl::Change::PROCESSING_COMPLETE);
        notify(BookBuildControl::Change::TREE);
    };
    bgThread2 = std::make_shared<std::thread>(f);
}

// --------------------------------------------------------------------------------

void
BookBuildControl::startAnalysis(const std::vector<Move>& moves) {
    if (engineThread)
        stopAnalysis();

    Position pos = TextIO::readFEN(TextIO::startPosFEN);
    UndoInfo ui;
    std::vector<U64> posHashList(200 + moves.size());
    int posHashListSize = 0;
    for (size_t i = 0; i < moves.size(); i++) {
        const Move& m = moves[i];
        posHashList[posHashListSize++] = pos.zobristHash();
        pos.makeMove(m, ui);
        if (pos.getHalfMoveClock() == 0)
            posHashListSize = 0;
    }

    class SearchListener : public Search::Listener {
    public:
        SearchListener(BookBuildControl& bbc0, const Position& pos) : bbc(bbc0), pos0(pos) {}
        void notifyDepth(int depth) override {}
        void notifyCurrMove(const Move& m, int moveNr) override {}
        void notifyPV(int depth, int score, int time, U64 nodes, int nps,
                              bool isMate, bool upperBound, bool lowerBound,
                              const std::vector<Move>& pv, int multiPVIndex,
                              U64 tbHits) override {
            Position pos(pos0);
            std::stringstream ss;
            ss << "[" << depth << "] ";
            bool negateScore = !pos.isWhiteMove();
            if (upperBound || lowerBound) {
                bool upper = upperBound ^ negateScore;
                ss << (upper ? "<=" : ">=");
            }
            int s = negateScore ? -score : score;
            if (isMate) {
                ss << 'm' << s;
            } else {
                ss.precision(2);
                ss << std::fixed << (s / 100.0);
            }
            UndoInfo ui;
            for (const Move& m : pv) {
                ss << ' ' << TextIO::moveToString(pos, m, false);
                pos.makeMove(m, ui);
            }
            {
                std::lock_guard<std::mutex> L(bbc.mutex);
                bbc.analysisPV = ss.str();
            }
            bbc.notify(BookBuildControl::Change::PV);
        }
        void notifyStats(U64 nodes, int nps, U64 tbHits, int time) override {}
    private:
        BookBuildControl& bbc;
        Position pos0;
    };

    Search::SearchTables st(tt, kt, ht, *et);
    sc = std::make_shared<Search>(pos, posHashList, posHashListSize, st, pd, nullptr, treeLog);
    sc->setListener(make_unique<SearchListener>(*this, pos));
    std::shared_ptr<MoveList> moveList(std::make_shared<MoveList>());
    MoveGen::pseudoLegalMoves(pos, *moveList);
    MoveGen::removeIllegal(pos, *moveList);
    pd.addRemoveWorkers(0);
    pd.wq.resetSplitDepth();
    pd.startAll();
    sc->timeLimit(-1, -1);
    int minProbeDepth = UciParams::minProbeDepth->getIntPar();
    auto f = [this,moveList,minProbeDepth]() {
        sc->iterativeDeepening(*moveList, -1, -1, false, 1, false, minProbeDepth);
    };
    engineThread = std::make_shared<std::thread>(f);
}

void
BookBuildControl::stopAnalysis() {
    if (engineThread) {
        sc->timeLimit(0, 0);
        engineThread->join();
        engineThread.reset();
        sc.reset();
        pd.stopAll();
    }
}

void
BookBuildControl::getPVInfo(std::string& pv) {
    std::lock_guard<std::mutex> L(mutex);
    pv = analysisPV;
}

// --------------------------------------------------------------------------------
