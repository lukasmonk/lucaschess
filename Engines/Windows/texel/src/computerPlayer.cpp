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
 * computerPlayer.cpp
 *
 *  Created on: Feb 25, 2012
 *      Author: petero
 */

#include "computerPlayer.hpp"
#include "search.hpp"
#include "history.hpp"
#include "killerTable.hpp"
#include "parallel.hpp"
#include "clustertt.hpp"
#include "textio.hpp"
#include "tbprobe.hpp"

#include <iostream>

std::string ComputerPlayer::engineName;


static StaticInitializer<ComputerPlayer> cpInit;

void
ComputerPlayer::staticInitialize() {
    std::string name = "Texel 1.07";
    if (sizeof(char*) == 4)
        name += " 32-bit";
    engineName = name;
}

void
ComputerPlayer::initEngine() {
    Parameters::instance();

    auto tbInit = []() {
        TBProbe::initialize(UciParams::gtbPath->getStringPar(),
                            UciParams::gtbCache->getIntPar(),
                            UciParams::rtbPath->getStringPar());
    };
    UciParams::gtbPath->addListener(tbInit);
    UciParams::gtbCache->addListener(tbInit, false);
    UciParams::rtbPath->addListener(tbInit, false);

    knightMobScore.addListener(Evaluate::updateEvalParams);
    castleFactor.addListener(Evaluate::updateEvalParams, false);
//    bV.addListener([]() { Parameters::instance().set("KnightValue", num2Str((int)bV)); });
    pV.addListener([]() { pieceValue[Piece::WPAWN]   = pieceValue[Piece::BPAWN]   = pV; });
    nV.addListener([]() { pieceValue[Piece::WKNIGHT] = pieceValue[Piece::BKNIGHT] = nV; });
    bV.addListener([]() { pieceValue[Piece::WBISHOP] = pieceValue[Piece::BBISHOP] = bV; });
    rV.addListener([]() { pieceValue[Piece::WROOK]   = pieceValue[Piece::BROOK]   = rV; });
    qV.addListener([]() { pieceValue[Piece::WQUEEN]  = pieceValue[Piece::BQUEEN]  = qV; });
    kV.addListener([]() { pieceValue[Piece::WKING]   = pieceValue[Piece::BKING]   = kV; });
}

ComputerPlayer::ComputerPlayer()
    : tt(15), book(false) {
    initEngine();
    et = Evaluate::getEvalHashTables();
    minTimeMillis = 10000;
    maxTimeMillis = 10000;
    maxDepth = 100;
    maxNodes = -1;
    bookEnabled = true;
    currentSearch = nullptr;
}

std::string
ComputerPlayer::getCommand(const Position& posIn, bool drawOffer, const std::vector<Position>& history) {
    // Create a search object
    std::vector<U64> posHashList(SearchConst::MAX_SEARCH_DEPTH * 2 + history.size());
    int posHashListSize = 0;
    for (size_t i = 0; i < history.size(); i++)
        posHashList[posHashListSize++] = history[i].zobristHash();
    tt.nextGeneration();
    Position pos(posIn);
    KillerTable kt;
    History ht;
    TreeLogger treeLog;
    Notifier notifier;
    ThreadCommunicator comm(nullptr, tt, notifier, false);
    Search::SearchTables st(comm.getCTT(), kt, ht, *et);
    Search sc(pos, posHashList, posHashListSize, st, comm, treeLog);

    // Determine all legal moves
    MoveList moves;
    MoveGen::pseudoLegalMoves(pos, moves);
    MoveGen::removeIllegal(pos, moves);
    sc.scoreMoveList(moves, 0);

    // Test for "game over"
    if (moves.size == 0) {
        // Switch sides so that the human can decide what to do next.
        return "swap";
    }

    if (bookEnabled) {
        Move bookMove;
        book.getBookMove(pos, bookMove);
        if (!bookMove.isEmpty()) {
            std::cout << "Book moves: " << book.getAllBookMoves(pos) << std::endl;
            return TextIO::moveToString(pos, bookMove, false);
        }
    }

    // Find best move using iterative deepening
    currentSearch = &sc;
    Move bestM;
    if ((moves.size == 1) && (canClaimDraw(pos, posHashList, posHashListSize, moves[0]) == "")) {
        bestM = moves[0];
        bestM.setScore(0);
    } else {
        sc.timeLimit(minTimeMillis, maxTimeMillis);
        bestM = sc.iterativeDeepening(moves, maxDepth, maxNodes, 1, false, 100);
    }
    currentSearch = nullptr;
    //        tt.printStats();
    std::string strMove = TextIO::moveToString(pos, bestM, false);

    // Claim draw if appropriate
    if (bestM.score() <= 0) {
        std::string drawClaim = canClaimDraw(pos, posHashList, posHashListSize, bestM);
        if (drawClaim != "")
            strMove = drawClaim;
    }
    return strMove;
}

std::string
ComputerPlayer::canClaimDraw(Position& pos, std::vector<U64>& posHashList,
                             int posHashListSize, const Move& move) {
    std::string drawStr;
    if (Search::canClaimDraw50(pos)) {
        drawStr = "draw 50";
    } else if (Search::canClaimDrawRep(pos, posHashList, posHashListSize, posHashListSize)) {
        drawStr = "draw rep";
    } else {
        std::string strMove = TextIO::moveToString(pos, move, false);
        posHashList[posHashListSize++] = pos.zobristHash();
        UndoInfo ui;
        pos.makeMove(move, ui);
        if (Search::canClaimDraw50(pos)) {
            drawStr = "draw 50 " + strMove;
        } else if (Search::canClaimDrawRep(pos, posHashList, posHashListSize, posHashListSize)) {
            drawStr = "draw rep " + strMove;
        }
        pos.unMakeMove(move, ui);
    }
    return drawStr;
}

void
ComputerPlayer::timeLimit(int minTimeLimit, int maxTimeLimit) {
    minTimeMillis = minTimeLimit;
    maxTimeMillis = maxTimeLimit;
    if (currentSearch != nullptr)
        currentSearch->timeLimit(minTimeLimit, maxTimeLimit);
}

std::pair<Move, std::string>
ComputerPlayer::searchPosition(Position& pos, int maxTimeMillis) {
    // Create a search object
    std::vector<U64> posHashList(SearchConst::MAX_SEARCH_DEPTH * 2);
    tt.nextGeneration();
    KillerTable kt;
    History ht;
    TreeLogger treeLog;
    Notifier notifier;
    ThreadCommunicator comm(nullptr, tt, notifier, false);
    Search::SearchTables st(comm.getCTT(), kt, ht, *et);
    Search sc(pos, posHashList, 0, st, comm, treeLog);

    // Determine all legal moves
    MoveList moves;
    MoveGen::pseudoLegalMoves(pos, moves);
    MoveGen::removeIllegal(pos, moves);
    sc.scoreMoveList(moves, 0);

    // Find best move using iterative deepening
    sc.timeLimit(maxTimeMillis, maxTimeMillis);
    Move bestM = sc.iterativeDeepening(moves, -1, -1);

    // Extract PV
    std::string PV = TextIO::moveToString(pos, bestM, false) + " ";
    UndoInfo ui;
    pos.makeMove(bestM, ui);
    PV += tt.extractPV(pos);
    pos.unMakeMove(bestM, ui);

//    tt.printStats();

    // Return best move and PV
    return std::pair<Move, std::string>(bestM, PV);
}
