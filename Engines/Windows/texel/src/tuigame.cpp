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
 * tuigame.cpp
 *
 *  Created on: Mar 4, 2012
 *      Author: petero
 */

#include "tuigame.hpp"
#include "uciprotocol.hpp"
#include "textio.hpp"
#include "evaluate.hpp"
#include "computerPlayer.hpp"

#include <iostream>
#include <fstream>
#include <iomanip>

TUIGame::TUIGame(std::unique_ptr<Player>&& whitePlayer,
                 std::unique_ptr<Player>&& blackPlayer)
    : Game(std::move(whitePlayer), std::move(blackPlayer)) {
}

bool
TUIGame::handleCommand(const std::string& moveStr) {
    if (Game::handleCommand(moveStr))
        return true;
    if (startsWith(moveStr, "testsuite ")) {
        std::string testSuiteCmd = moveStr.substr(moveStr.find_first_of(' ') + 1);
        handleTestSuite(testSuiteCmd);
        return true;
    } else if (moveStr == "uci") {
        whitePlayer.reset();
        blackPlayer.reset();
        UCIProtocol::main(true);
        exit(0);
        return false;
    } else if (moveStr == "help") {
        showHelp();
        return true;
    }

    return false;
}

void
TUIGame::showHelp() {
    std::cout << "Enter a move, or one of the following special commands:" << std::endl;
    std::cout << "  new             - Start a new game" << std::endl;
    std::cout << "  undo            - Undo last half-move" << std::endl;
    std::cout << "  redo            - Redo next half-move" << std::endl;
    std::cout << "  swap            - Swap sides" << std::endl;
    std::cout << "  go              - Same as swap" << std::endl;
    std::cout << "  list            - List all moves in current game" << std::endl;
    std::cout << "  setpos FEN      - Set a position using a FEN string" << std::endl;
    std::cout << "  getpos          - Print current position in FEN notation" << std::endl;
    std::cout << "  draw rep [move] - Claim draw by repetition" << std::endl;
    std::cout << "  draw 50 [move]  - Claim draw by 50-move rule" << std::endl;
    std::cout << "  draw offer move - Play move and offer draw" << std::endl;
    std::cout << "  draw accept     - Accept a draw offer" << std::endl;
    std::cout << "  resign          - Resign the current game" << std::endl;
    std::cout << "  testsuite filename maxtime" << std::endl;
    std::cout << "  book on|off     - Turn opening book on/off" << std::endl;
    std::cout << "  time t          - Set computer thinking time, ms" << std::endl;
    std::cout << "  perft d         - Run perft test to depth d" << std::endl;
    std::cout << "  uci             - Switch to uci protocol." << std::endl;
    std::cout << "  help            - Show this help" << std::endl;
    std::cout << "  quit            - Terminate program" << std::endl;
}

void
TUIGame::handleTestSuite(const std::string& cmd) {
    std::ifstream fr;
    int lineNo = -1;
    try {
        size_t idx = cmd.find_first_of(' ');
        if (idx == cmd.npos)
            return;
        std::string filename(cmd.substr(0, idx));
        std::string timeStr(cmd.substr(idx + 1));
        int timeLimit;
        if (!str2Num(timeStr, timeLimit)) {
            std::cout << "Error parsing number: " << timeStr << std::endl;
            return;
        }
//        std::cout << "file:" << filename << " time:" << timeStr << " (" << timeLimit << ")" << std::endl;
        fr.open(filename.c_str());
        Player& pl = whitePlayer->isHumanPlayer() ? *blackPlayer : *whitePlayer;
        if (pl.isHumanPlayer()) {
            std::cout << "No computer player available" << std::endl;
            return;
        }
        ComputerPlayer& cp = static_cast<ComputerPlayer&>(pl);
        int numRight = 0;
        int numTotal = 0;
        std::string line;
        lineNo = 0;
        while (getline(fr, line).good()) {
            lineNo++;
            if (startsWith(line, "#") || (line.length() == 0))
                continue;
            size_t idx1 = line.find(" bm ");
            if (idx1 == line.npos) {
                std::cout << "Parse error, line:" << lineNo << std::endl;
                return;
            }
            std::string fen = line.substr(0, idx1);
            size_t idx2 = line.find(";", idx1);
            if (idx2 == line.npos) {
                std::cout << "Parse error, line:" << lineNo << std::endl;
                return;
            }
            std::string bm = line.substr(idx1+4, idx2 - (idx1+4));
//            std::cout << "Line " << std::setw(3) << lineNo << ": fen:" << fen << " bm:" << bm << std::endl;
            Position testPos = TextIO::readFEN(fen);
            cp.clearTT();
            std::pair<Move, std::string> ret = cp.searchPosition(testPos, timeLimit);
            Move sm = ret.first;
            std::string PV = ret.second;
            Move m(sm);
            std::vector<std::string> answers;
            splitString(bm, answers);
            bool correct = false;
            for (size_t i = 0; i < answers.size(); i++) {
                const std::string& a = answers[i];
                Move am(TextIO::stringToMove(testPos, a));
                if (am.isEmpty())
                    throw ChessParseError("Invalid move " + a);
                if (am.equals(m)) {
                    correct = true;
                    break;
                }
            }
            if (correct)
                numRight++;
            numTotal++;
            std::cout << std::setw(3) << lineNo
                      << ' ' << std::setw(6) << TextIO::moveToString(testPos, sm, false)
                      << ' ' << std::setw(6) << sm.score()
                      << ' ' << (correct ? 1 : 0)
                      << ' ' << std::setw(3) << numRight
                      << '/' << std::setw(3) << numTotal
                      << ' ' << bm << " : " << PV << std::endl;
        }
        fr.close();
    } catch (const std::ifstream::failure& ex) {
        std::cout << "IO error: " << ex.what() << std::endl;
    } catch (const ChessParseError& cpe) {
        std::cout << "Parse error, line " << lineNo << ": " << cpe.what() << std::endl;
    }
}

void
TUIGame::play() {
    handleCommand("new");
    while (true) {
        // Print last move
        if (currentMove > 0) {
            Position prevPos(getPos());
            prevPos.unMakeMove(moveList[currentMove - 1], uiInfoList[currentMove - 1]);
            std::string moveStr= TextIO::moveToString(prevPos, moveList[currentMove - 1], false);
            if (haveDrawOffer())
                moveStr += " (offer draw)";
            std::cout << "Last move: " << prevPos.getFullMoveCounter()
                    << (prevPos.isWhiteMove() ? "." : "...")
                    << ' ' << moveStr << std::endl;
        }
        /*
        {
            std::stringstream ss;
            ss << "Hash: " << std::hex << std::setw(16) << std::setfill('0') << pos.zobristHash();
            std::cout << ss.str() << std::endl;
        }
        */
        {
            auto et = Evaluate::getEvalHashTables();
            Evaluate eval(*et);
            int evScore = eval.evalPos(getPos()) * (getPos().isWhiteMove() ? 1 : -1);
            std::stringstream ss;
            ss.precision(2);
            ss << std::fixed << "Eval: " << (evScore / 100.0);
            std::cout << ss.str() << std::endl;
        }

        // Check game state
        std::cout << TextIO::asciiBoard(getPos());
        std::string stateStr = getGameStateString();
        if (stateStr.length() > 0)
            std::cout << stateStr << std::endl;
        if (getGameState() != Game::ALIVE)
            activateHumanPlayer();

        // Get command from current player and act on it
        Player& pl = getPos().isWhiteMove() ? *whitePlayer : *blackPlayer;
        std::vector<Position> posList;
        getHistory(posList);
        std::string moveStr = pl.getCommand(getPos(), haveDrawOffer(), posList);
        if (moveStr == "quit")
            return;
        bool ok = processString(moveStr);
        if (!ok)
            std::cout << "Invalid move: " << moveStr << std::endl;
    }
}
