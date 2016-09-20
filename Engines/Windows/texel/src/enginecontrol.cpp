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
 * enginecontrol.cpp
 *
 *  Created on: Mar 4, 2012
 *      Author: petero
 */

#define _GLIBCXX_USE_NANOSLEEP

#include "enginecontrol.hpp"
#include "util/random.hpp"
#include "searchparams.hpp"
#include "book.hpp"
#include "textio.hpp"
#include "parameters.hpp"
#include "moveGen.hpp"
#include "util/logger.hpp"
#include "numa.hpp"

#include <iostream>
#include <memory>
#include <chrono>


EngineControl::SearchListener::SearchListener(std::ostream& os0)
    : os(os0)
{
}

void
EngineControl::SearchListener::notifyDepth(int depth) {
//    std::lock_guard<std::mutex> L(Logger::getLogMutex());
    os << "info depth " << depth << std::endl;
}

void
EngineControl::SearchListener::notifyCurrMove(const Move& m, int moveNr) {
//    std::lock_guard<std::mutex> L(Logger::getLogMutex());
    os << "info currmove " << moveToString(m) << " currmovenumber " << moveNr << std::endl;
}

void
EngineControl::SearchListener::notifyPV(int depth, int score, int time, U64 nodes, int nps, bool isMate,
                                        bool upperBound, bool lowerBound, const std::vector<Move>& pv,
                                        int multiPVIndex, U64 tbHits) {
//    std::lock_guard<std::mutex> L(Logger::getLogMutex());
    std::string pvBuf;
    for (size_t i = 0; i < pv.size(); i++) {
        pvBuf += ' ';
        pvBuf += moveToString(pv[i]);
    }
    std::string bound;
    if (upperBound) {
        bound = " upperbound";
    } else if (lowerBound) {
        bound = " lowerbound";
    }
    os << "info depth " << depth << " score " << (isMate ? "mate " : "cp ")
       << score << bound << " time " << time << " nodes " << nodes
       << " nps " << nps;
    if (tbHits > 0)
        os << " tbhits " << tbHits;
    if (multiPVIndex >= 0)
        os << " multipv " << (multiPVIndex + 1);
    os << " pv" << pvBuf << std::endl;
}

void
EngineControl::SearchListener::notifyStats(U64 nodes, int nps, U64 tbHits, int time) {
    os << "info nodes " << nodes << " nps " << nps;
    if (tbHits > 0)
        os << " tbhits " << tbHits;
    os << " time " << time << std::endl;
}

EngineControl::EngineControl(std::ostream& o)
    : os(o),
      shouldDetach(true),
      tt(8),
      pd(tt),
      randomSeed(0)
{
    Numa::instance().bindThread(0);
    hashParListenerId = UciParams::hash->addListener([this]() {
        setupTT();
    });
    clearHashParListenerId = UciParams::clearHash->addListener([this]() {
        tt.clear();
        ht.init();
    }, false);
    et = Evaluate::getEvalHashTables();
}

EngineControl::~EngineControl() {
    UciParams::hash->removeListener(hashParListenerId);
    UciParams::hash->removeListener(clearHashParListenerId);
}

void
EngineControl::startSearch(const Position& pos, const std::vector<Move>& moves, const SearchParams& sPar) {
    stopSearch();
    setupPosition(pos, moves);
    computeTimeLimit(sPar);
    ponder = false;
    infinite = (maxTimeLimit < 0) && (maxDepth < 0) && (maxNodes < 0);
    searchMoves = sPar.searchMoves;
    startThread(minTimeLimit, maxTimeLimit, maxDepth, maxNodes);
}

void
EngineControl::startPonder(const Position& pos, const std::vector<Move>& moves, const SearchParams& sPar) {
    stopSearch();
    setupPosition(pos, moves);
    computeTimeLimit(sPar);
    ponder = true;
    infinite = false;
    startThread(-1, -1, -1, -1);
}

void
EngineControl::ponderHit() {
    std::shared_ptr<Search> mySearch;
    {
        std::lock_guard<std::mutex> L(threadMutex);
        mySearch = sc;
    }
    if (mySearch) {
        if (onePossibleMove) {
            if (minTimeLimit > 1) minTimeLimit = 1;
            if (maxTimeLimit > 1) maxTimeLimit = 1;
        }
        mySearch->timeLimit(minTimeLimit, maxTimeLimit);
    }
    infinite = (maxTimeLimit < 0) && (maxDepth < 0) && (maxNodes < 0);
    ponder = false;
}

void
EngineControl::stopSearch() {
    stopThread();
}

void
EngineControl::newGame() {
    randomSeed = Random().nextU64();
    tt.clear();
    ht.init();
}

/**
 * Compute thinking time for current search.
 */
void
EngineControl::computeTimeLimit(const SearchParams& sPar) {
    minTimeLimit = -1;
    maxTimeLimit = -1;
    maxDepth = -1;
    maxNodes = -1;
    if (sPar.infinite) {
        minTimeLimit = -1;
        maxTimeLimit = -1;
        maxDepth = -1;
    } else {
        if (sPar.depth > 0)
            maxDepth = sPar.depth;
        if (sPar.mate > 0) {
            int md = sPar.mate * 2 - 1;
            maxDepth = maxDepth == -1 ? md : std::min(maxDepth, md);
        }
        if (sPar.nodes > 0)
            maxNodes = sPar.nodes;

        if (sPar.moveTime > 0) {
             minTimeLimit = maxTimeLimit = sPar.moveTime;
        } else if (sPar.wTime || sPar.bTime) {
            int moves = sPar.movesToGo;
            if (moves == 0)
                moves = 999;
            moves = std::min(moves, static_cast<int>(timeMaxRemainingMoves)); // Assume at most N more moves until end of game
            bool white = pos.isWhiteMove();
            int time = white ? sPar.wTime : sPar.bTime;
            int inc  = white ? sPar.wInc : sPar.bInc;
            const int margin = std::min(static_cast<int>(bufferTime), time * 9 / 10);
            int timeLimit = (time + inc * (moves - 1) - margin) / moves;
            minTimeLimit = (int)(timeLimit * minTimeUsage * 0.01);
            if (UciParams::ponder->getBoolPar()) {
                const double ponderHitRate = timePonderHitRate * 0.01;
                minTimeLimit = (int)ceil(minTimeLimit / (1 - ponderHitRate));
            }
            maxTimeLimit = (int)(minTimeLimit * clamp(moves * 0.5, 2.5, static_cast<int>(maxTimeUsage) * 0.01));

            // Leave at least 1s on the clock, but can't use negative time
            minTimeLimit = clamp(minTimeLimit, 1, time - margin);
            maxTimeLimit = clamp(maxTimeLimit, 1, time - margin);
        }
    }
}

void
EngineControl::startThread(int minTimeLimit, int maxTimeLimit, int maxDepth, int maxNodes) {
    Search::SearchTables st(tt, kt, ht, *et);
    sc = std::make_shared<Search>(pos, posHashList, posHashListSize, st, pd, nullptr, treeLog);
    sc->setListener(std::make_shared<SearchListener>(os));
    sc->setStrength(UciParams::strength->getIntPar(), randomSeed);
    std::shared_ptr<MoveList> moves(std::make_shared<MoveList>());
    MoveGen::pseudoLegalMoves(pos, *moves);
    MoveGen::removeIllegal(pos, *moves);
    if (searchMoves.size() > 0)
        moves->filter(searchMoves);
    onePossibleMove = false;
    if ((moves->size < 2) && !infinite) {
        onePossibleMove = true;
        if (!ponder) {
            if (maxTimeLimit > 0) {
                maxTimeLimit = clamp(maxTimeLimit/100, 1, 100);
                minTimeLimit = clamp(minTimeLimit/100, 1, 100);
            } else {
                if ((maxDepth < 0) || (maxDepth > 2))
                    maxDepth = 2;
            }
        }
    }
    pd.addRemoveWorkers(UciParams::threads->getIntPar() - 1);
    pd.wq.resetSplitDepth();
    pd.startAll();
    sc->timeLimit(minTimeLimit, maxTimeLimit);
    tt.nextGeneration();
    bool ownBook = UciParams::ownBook->getBoolPar();
    bool analyseMode = UciParams::analyseMode->getBoolPar();
    int maxPV = (infinite || analyseMode) ? UciParams::multiPV->getIntPar() : 1;
    int minProbeDepth = UciParams::minProbeDepth->getIntPar();
    if (analyseMode) {
        Evaluate eval(*et);
        int evScore = eval.evalPosPrint(pos) * (pos.isWhiteMove() ? 1 : -1);
        std::stringstream ss;
        ss.precision(2);
        ss << std::fixed << (evScore / 100.0);
        os << "info string Eval: " << ss.str() << std::endl;
    }
    auto f = [this,ownBook,analyseMode,moves,maxDepth,maxNodes,maxPV,minProbeDepth]() {
        Numa::instance().bindThread(0);
        Move m;
        if (ownBook && !analyseMode) {
            Book book(false);
            book.getBookMove(pos, m);
        }
        if (m.isEmpty())
            m = sc->iterativeDeepening(*moves, maxDepth, maxNodes, false, maxPV, false, minProbeDepth);
        while (ponder || infinite) {
            // We should not respond until told to do so. Just wait until
            // we are allowed to respond.
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
        Move ponderMove = getPonderMove(pos, m);
        std::lock_guard<std::mutex> L(threadMutex);
        os << "bestmove " << moveToString(m);
        if (!ponderMove.isEmpty())
            os << " ponder " << moveToString(ponderMove);
        os << std::endl;
        if (shouldDetach) {
            engineThread->detach();
            pd.stopAll();
            pd.fhInfo.reScale();
        }
        engineThread.reset();
        sc.reset();
        for (auto& p : pendingOptions)
            setOption(p.first, p.second, false);
        pendingOptions.clear();
    };
    shouldDetach = true;
    {
        std::lock_guard<std::mutex> L(threadMutex);
        engineThread = std::make_shared<std::thread>(f);
    }
}

void
EngineControl::stopThread() {
    std::shared_ptr<std::thread> myThread;
    {
        std::lock_guard<std::mutex> L(threadMutex);
        myThread = engineThread;
        if (myThread) {
            sc->timeLimit(0, 0);
            infinite = false;
            ponder = false;
            shouldDetach = false;
        }
    }
    if (myThread)
        myThread->join();
    pd.stopAll();
    pd.fhInfo.reScale();
}

void
EngineControl::setupTT() {
    int hashSizeMB = UciParams::hash->getIntPar();
    U64 nEntries = hashSizeMB > 0 ? ((U64)hashSizeMB) * (1 << 20) / sizeof(TranspositionTable::TTEntry)
	                          : (U64)1024;
    int logSize = 0;
    while (nEntries > 1) {
        logSize++;
        nEntries /= 2;
    }
    logSize++;
    while (true) {
        try {
            logSize--;
            if (logSize <= 0)
                break;
            tt.reSize(logSize);
            break;
        } catch (const std::bad_alloc& ex) {
        }
    }
}

void
EngineControl::setupPosition(Position pos, const std::vector<Move>& moves) {
    UndoInfo ui;
    posHashList.resize(200 + moves.size());
    posHashListSize = 0;
    for (size_t i = 0; i < moves.size(); i++) {
        const Move& m = moves[i];
        posHashList[posHashListSize++] = pos.zobristHash();
        pos.makeMove(m, ui);
        if (pos.getHalfMoveClock() == 0)
            posHashListSize = 0;
    }
    this->pos = pos;
}

/**
 * Try to find a move to ponder from the transposition table.
 */
Move
EngineControl::getPonderMove(Position pos, const Move& m) {
    Move ret;
    if (m.isEmpty())
        return ret;
    UndoInfo ui;
    pos.makeMove(m, ui);
    TranspositionTable::TTEntry ent;
    ent.clear();
    tt.probe(pos.historyHash(), ent);
    if (ent.getType() != TType::T_EMPTY) {
        ent.getMove(ret);
        MoveList moves;
        MoveGen::pseudoLegalMoves(pos, moves);
        MoveGen::removeIllegal(pos, moves);
        bool contains = false;
        for (int mi = 0; mi < moves.size; mi++)
            if (moves[mi].equals(ret)) {
                contains = true;
                break;
            }
        if  (!contains)
            ret = Move();
    }
    pos.unMakeMove(m, ui);
    return ret;
}

std::string
EngineControl::moveToString(const Move& m) {
    if (m.isEmpty())
        return "0000";
    std::string ret = TextIO::squareToString(m.from());
    ret += TextIO::squareToString(m.to());
    switch (m.promoteTo()) {
    case Piece::WQUEEN:
    case Piece::BQUEEN:
        ret += 'q';
        break;
    case Piece::WROOK:
    case Piece::BROOK:
        ret += 'r';
        break;
    case Piece::WBISHOP:
    case Piece::BBISHOP:
        ret += 'b';
        break;
    case Piece::WKNIGHT:
    case Piece::BKNIGHT:
        ret += 'n';
        break;
    default:
        break;
    }
    return ret;
}

void
EngineControl::printOptions(std::ostream& os) {
    std::vector<std::string> parNames;
    Parameters::instance().getParamNames(parNames);
    for (const auto& pName : parNames) {
        std::shared_ptr<Parameters::ParamBase> p = Parameters::instance().getParam(pName);
        switch (p->type) {
        case Parameters::CHECK: {
            const Parameters::CheckParam& cp = dynamic_cast<const Parameters::CheckParam&>(*p.get());
            os << "option name " << cp.name << " type check default "
               << (cp.defaultValue?"true":"false") << std::endl;
            break;
        }
        case Parameters::SPIN: {
            const Parameters::SpinParam& sp = dynamic_cast<const Parameters::SpinParam&>(*p.get());
            os << "option name " << sp.name << " type spin default "
               << sp.getDefaultValue() << " min " << sp.getMinValue()
               << " max " << sp.getMaxValue() << std::endl;
            break;
        }
        case Parameters::COMBO: {
            const Parameters::ComboParam& cp = dynamic_cast<const Parameters::ComboParam&>(*p.get());
            os << "option name " << cp.name << " type combo default " << cp.defaultValue;
            for (size_t i = 0; i < cp.allowedValues.size(); i++)
                os << " var " << cp.allowedValues[i];
            os << std::endl;
            break;
        }
        case Parameters::BUTTON:
            os << "option name " << p->name << " type button" << std::endl;
            break;
        case Parameters::STRING: {
            const Parameters::StringParam& sp = dynamic_cast<const Parameters::StringParam&>(*p.get());
            os << "option name " << sp.name << " type string default "
               << sp.defaultValue << std::endl;
            break;
        }
        }
    }
}

void
EngineControl::setOption(const std::string& optionName, const std::string& optionValue,
                         bool deferIfBusy) {
    Parameters& params = Parameters::instance();
    if (deferIfBusy) {
        std::lock_guard<std::mutex> L(threadMutex);
        if (engineThread) {
            if (params.getParam(optionName))
                pendingOptions[optionName] = optionValue;
            return;
        }
    }
    std::shared_ptr<Parameters::ParamBase> par = params.getParam(optionName);
    if (par && par->type == Parameters::STRING && optionValue == "<empty>")
        params.set(optionName, "");
    else
        params.set(optionName, optionValue);
}
