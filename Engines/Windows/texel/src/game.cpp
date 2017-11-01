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
 * game.cpp
 *
 *  Created on: Feb 25, 2012
 *      Author: petero
 */

#include "game.hpp"
#include "moveGen.hpp"
#include "textio.hpp"
#include "util/timeUtil.hpp"

#include <iostream>
#include <iomanip>
#include <cassert>


Game::Game(std::unique_ptr<Player>&& whitePlayer,
           std::unique_ptr<Player>&& blackPlayer)
    : whitePlayer(std::move(whitePlayer)),
      blackPlayer(std::move(blackPlayer)) {
    handleCommand("new");
}

bool
Game::processString(const std::string& str) {
    if (handleCommand(str))
        return true;
    if (getGameState() != ALIVE)
        return false;

    Move m = TextIO::stringToMove(pos, str);
    if (m.isEmpty())
        return false;

    UndoInfo ui;
    pos.makeMove(m, ui);
    TextIO::fixupEPSquare(pos);
    if (currentMove < (int)moveList.size()) {
        moveList.erase(moveList.begin() + currentMove, moveList.end());
        uiInfoList.erase(uiInfoList.begin() + currentMove, uiInfoList.end());
        drawOfferList.erase(drawOfferList.begin() + currentMove, drawOfferList.end());
    }
    moveList.push_back(m);
    uiInfoList.push_back(ui);
    drawOfferList.push_back(pendingDrawOffer);
    pendingDrawOffer = false;
    currentMove++;
    return true;
}

std::string
Game::getGameStateString() {
    switch (getGameState()) {
    case ALIVE:
        return "";
    case WHITE_MATE:
        return "Game over, white mates!";
    case BLACK_MATE:
        return "Game over, black mates!";
    case WHITE_STALEMATE:
    case BLACK_STALEMATE:
        return "Game over, draw by stalemate!";
    case DRAW_REP:
    {
        std::string ret = "Game over, draw by repetition!";
        if (drawStateMoveStr.length() > 0)
            ret += " [" + drawStateMoveStr + "]";
        return ret;
    }
    case DRAW_50:
    {
        std::string ret = "Game over, draw by 50 move rule!";
        if (drawStateMoveStr.length() > 0)
            ret += " [" + drawStateMoveStr + "]";
        return ret;
    }
    case DRAW_NO_MATE:
        return "Game over, draw by impossibility of mate!";
    case DRAW_AGREE:
        return "Game over, draw by agreement!";
    case RESIGN_WHITE:
        return "Game over, white resigns!";
    case RESIGN_BLACK:
        return "Game over, black resigns!";
    default:
        assert(false);
        return "";
    }
}

Move
Game::getLastMove() {
    Move m;
    if (currentMove > 0)
        m = moveList[currentMove - 1];
    return m;
}

Game::GameState
Game::getGameState() {
    MoveList moves;
    MoveGen::pseudoLegalMoves(pos, moves);
    MoveGen::removeIllegal(pos, moves);
    if (moves.size == 0) {
        if (MoveGen::inCheck(pos))
            return pos.isWhiteMove() ? BLACK_MATE : WHITE_MATE;
        else
            return pos.isWhiteMove() ? WHITE_STALEMATE : BLACK_STALEMATE;
    }
    if (insufficientMaterial())
        return DRAW_NO_MATE;
    if (resignState != ALIVE)
        return resignState;
    return drawState;
}

bool
Game::haveDrawOffer() {
    if (currentMove > 0)
        return drawOfferList[currentMove - 1];
    else
        return false;
}

bool
Game::handleCommand(const std::string& moveStr) {
    if (moveStr == "new") {
        moveList.clear();
        uiInfoList.clear();
        drawOfferList.clear();
        currentMove = 0;
        pendingDrawOffer = false;
        drawState = ALIVE;
        resignState = ALIVE;
        pos = TextIO::readFEN(TextIO::startPosFEN);
        whitePlayer->clearTT();
        blackPlayer->clearTT();
        activateHumanPlayer();
        return true;
    } else if (moveStr == "undo") {
        if (currentMove > 0) {
            pos.unMakeMove(moveList[currentMove - 1], uiInfoList[currentMove - 1]);
            currentMove--;
            pendingDrawOffer = false;
            drawState = ALIVE;
            resignState = ALIVE;
            return handleCommand("swap");
        } else {
            std::cout << "Nothing to undo" << std::endl;
        }
        return true;
    } else if (moveStr == "redo") {
        if (currentMove < (int)moveList.size()) {
            pos.makeMove(moveList[currentMove], uiInfoList[currentMove]);
            currentMove++;
            pendingDrawOffer = false;
            return handleCommand("swap");
        } else {
            std::cout << "Nothing to redo" << std::endl;
        }
        return true;
    } else if (moveStr == "swap" || moveStr == "go") {
        std::swap(whitePlayer, blackPlayer);
        return true;
    } else if (moveStr == "list") {
        listMoves();
        return true;
    } else if (startsWith(moveStr, "setpos ")) {
        std::string fen = moveStr.substr(moveStr.find_first_of(' ') + 1);
        try {
            Position newPos(TextIO::readFEN(fen));
            handleCommand("new");
            pos = newPos;
            activateHumanPlayer();
        } catch (const ChessParseError& ex) {
            std::cout << "Invalid FEN: " << fen << " (" << ex.what() << ")" << std::endl;
        }
        return true;
    } else if (moveStr == "getpos") {
        std::string fen(TextIO::toFEN(pos));
        std::cout << fen << std::endl;
        return true;
    } else if (startsWith(moveStr, "draw ")) {
        if (getGameState() == ALIVE) {
            std::string drawCmd = moveStr.substr(moveStr.find_first_of(' ') + 1);
            return handleDrawCmd(drawCmd);
        } else {
            return true;
        }
    } else if (moveStr == "resign") {
        if (getGameState()== ALIVE) {
            resignState = pos.isWhiteMove() ? RESIGN_WHITE : RESIGN_BLACK;
            return true;
        } else {
            return true;
        }
    } else if (startsWith(moveStr, "book")) {
        std::string bookCmd = moveStr.substr(moveStr.find_first_of(' ') + 1);
        return handleBookCmd(bookCmd);
    } else if (startsWith(moveStr, "time")) {
        std::string timeStr = moveStr.substr(moveStr.find_first_of(' ') + 1);
        int timeLimit;
        if (!str2Num(timeStr, timeLimit)) {
            std::cout << "Can not parse number: " << timeStr << std::endl;
            return false;
        }
        whitePlayer->timeLimit(timeLimit, timeLimit);
        blackPlayer->timeLimit(timeLimit, timeLimit);
        return true;
    } else if (startsWith(moveStr, "perft ")) {
        std::string depthStr = moveStr.substr(moveStr.find_first_of(' ') + 1);
        int depth;
        if (!str2Num(depthStr, depth)) {
            std::cout << "Can not parse number: " << depthStr << std::endl;
            return false;
        }
        S64 t0 = currentTimeMillis();
        U64 nodes = perfT(pos, depth);
        S64 t1 = currentTimeMillis();
        double t = (t1 - t0) * 1e-3;
        std::stringstream ss;
        ss.precision(3);
        std::cout << "perft(" << depth << ") = " << nodes << ", t="
                  << (ss << std::fixed << t).rdbuf()
                  << "s" << std::endl;
        return true;
    } else {
        return false;
    }
}

void
Game::activateHumanPlayer() {
    if (!(pos.isWhiteMove() ? whitePlayer : blackPlayer)->isHumanPlayer())
        std::swap(whitePlayer, blackPlayer);
}

void
Game::getPosHistory(std::vector<std::string> ret) {
    ret.clear();
    Position pos(this->pos);
    for (int i = currentMove; i > 0; i--)
        pos.unMakeMove(moveList[i - 1], uiInfoList[i - 1]);
    ret.push_back(TextIO::toFEN(pos)); // Store initial FEN

    std::string moves;
    UndoInfo ui;
    for (size_t i = 0; i < moveList.size(); i++) {
        const Move& move = moveList[i];
        std::string strMove(TextIO::moveToString(pos, move, false));
        moves += ' ';
        moves += strMove;
        pos.makeMove(move, ui);
    }
    ret.push_back(moves); // Store move list string

    int numUndo = (int)moveList.size() - currentMove;
    ret.push_back(num2Str(numUndo));
}

std::string
Game::getMoveListString(bool compressed) {
    std::string ret;

    // Undo all moves in move history.
    Position pos(this->pos);
    for (int i = currentMove; i > 0; i--)
        pos.unMakeMove(moveList[i - 1], uiInfoList[i - 1]);

    // Print all moves
    std::string whiteMove;
    std::string blackMove;
    for (int i = 0; i < currentMove; i++) {
        const Move& move = moveList[i];
        std::string strMove = TextIO::moveToString(pos, move, false);
        if (drawOfferList[i])
            strMove += " (d)";
        if (pos.isWhiteMove()) {
            whiteMove = strMove;
        } else {
            blackMove = strMove;
            if (whiteMove.length() == 0)
                whiteMove = "...";
            if (compressed) {
                ret += num2Str(pos.getFullMoveCounter()) + ". " + whiteMove +
                       " " + blackMove + " ";
            } else {
                std::stringstream ss;
                ss << std::setw(3) << pos.getFullMoveCounter() << ".  "
                   << std::setw(10) << std::left << whiteMove << " "
                   << std::setw(10) << std::left << blackMove << std::endl;
                ret += ss.str();
            }
            whiteMove.clear();
            blackMove.clear();
        }
        UndoInfo ui;
        pos.makeMove(move, ui);
    }
    if ((whiteMove.length() > 0) || (blackMove.length() > 0)) {
        if (whiteMove.length() == 0)
            whiteMove = "...";
        if (compressed) {
            ret += num2Str(pos.getFullMoveCounter()) + ". " + whiteMove +
                   " " + blackMove + " ";
        } else {
            std::stringstream ss;
            ss << std::setw(3) << pos.getFullMoveCounter() << ".  "
               << std::setw(10) << std::left << whiteMove << " "
               << std::setw(10) << std::left << blackMove << std::endl;
            ret += ss.str();
        }
    }
    std::string gameResult = getPGNResultString();
    if (gameResult != "*") {
        ret += gameResult;
        if (!compressed)
            ret += '\n';
    }
    return ret;
}

std::string
Game::getPGNResultString() {
    std::string gameResult = "*";
    switch (getGameState()) {
    case ALIVE:
        break;
    case WHITE_MATE:
    case RESIGN_BLACK:
        gameResult = "1-0";
        break;
    case BLACK_MATE:
    case RESIGN_WHITE:
        gameResult = "0-1";
        break;
    case WHITE_STALEMATE:
    case BLACK_STALEMATE:
    case DRAW_REP:
    case DRAW_50:
    case DRAW_NO_MATE:
    case DRAW_AGREE:
        gameResult = "1/2-1/2";
        break;
    }
    return gameResult;
}

/** Return a list of previous positions in this game, back to the last "zeroing" move. */
void
Game::getHistory(std::vector<Position>& posList) {
    posList.clear();
    Position pos(this->pos);
    for (int i = currentMove; i > 0; i--) {
        if (pos.getHalfMoveClock() == 0)
            break;
        pos.unMakeMove(moveList[i - 1], uiInfoList[i - 1]);
        posList.push_back(pos);
    }
    std::reverse(posList.begin(), posList.end());
}

void
Game::listMoves() {
    std::string movesStr = getMoveListString(false);
    std::cout << movesStr;
}

bool
Game::handleDrawCmd(std::string drawCmd) {
    if (startsWith(drawCmd, "rep") || startsWith(drawCmd, "50")) {
        bool rep = startsWith(drawCmd, "rep");
        Move m;
        size_t idx = drawCmd.find_first_of(' ');
        std::string ms;
        if (idx != drawCmd.npos) {
            ms = drawCmd.substr(idx + 1);
            if (ms.length() > 0)
                m = TextIO::stringToMove(pos, ms);
        }
        bool valid;
        if (rep) {
            valid = false;
            std::vector<Position> oldPositions;
            if (!m.isEmpty()) {
                UndoInfo ui;
                Position tmpPos(pos);
                tmpPos.makeMove(m, ui);
                oldPositions.push_back(tmpPos);
            }
            oldPositions.push_back(pos);
            Position tmpPos(pos);
            for (int i = currentMove - 1; i >= 0; i--) {
                tmpPos.unMakeMove(moveList[i], uiInfoList[i]);
                oldPositions.push_back(tmpPos);
            }
            int repetitions = 0;
            Position firstPos = oldPositions[0];
            for (size_t i = 0; i < oldPositions.size(); i++) {
                if (oldPositions[i].drawRuleEquals(firstPos))
                    repetitions++;
            }
            if (repetitions >= 3) {
                valid = true;
            }
        } else {
            Position tmpPos(pos);
            if (!m.isEmpty()) {
                UndoInfo ui;
                tmpPos.makeMove(m, ui);
            }
            valid = tmpPos.getHalfMoveClock() >= 100;
        }
        if (valid) {
            drawState = rep ? DRAW_REP : DRAW_50;
            drawStateMoveStr.clear();
            if (!m.isEmpty())
                drawStateMoveStr = TextIO::moveToString(pos, m, false);
        } else {
            pendingDrawOffer = true;
            if (!m.isEmpty())
                processString(ms);
        }
        return true;
    } else if (startsWith(drawCmd, "offer ")) {
        pendingDrawOffer = true;
        size_t idx = drawCmd.find_first_of(' ');
        if (idx != drawCmd.npos) {
            std::string ms = drawCmd.substr(idx + 1);
            if (!TextIO::stringToMove(pos, ms).isEmpty())
                processString(ms);
        }
        return true;
    } else if (drawCmd == "accept") {
        if (haveDrawOffer())
            drawState = DRAW_AGREE;
        return true;
    } else {
        return false;
    }
}

bool
Game::handleBookCmd(const std::string& bookCmd) {
    if (bookCmd == "off") {
        whitePlayer->useBook(false);
        blackPlayer->useBook(false);
        return true;
    } else if (bookCmd == "on") {
        whitePlayer->useBook(true);
        whitePlayer->useBook(true);
        return true;
    }
    return false;
}

bool
Game::insufficientMaterial() {
    if (pos.pieceTypeBB(Piece::WQUEEN) != 0) return false;
    if (pos.pieceTypeBB(Piece::WROOK)  != 0) return false;
    if (pos.pieceTypeBB(Piece::WPAWN)  != 0) return false;
    if (pos.pieceTypeBB(Piece::BQUEEN) != 0) return false;
    if (pos.pieceTypeBB(Piece::BROOK)  != 0) return false;
    if (pos.pieceTypeBB(Piece::BPAWN)  != 0) return false;
    int wb = BitBoard::bitCount(pos.pieceTypeBB(Piece::WBISHOP));
    int wn = BitBoard::bitCount(pos.pieceTypeBB(Piece::WKNIGHT));
    int bb = BitBoard::bitCount(pos.pieceTypeBB(Piece::BBISHOP));
    int bn = BitBoard::bitCount(pos.pieceTypeBB(Piece::BKNIGHT));
    if (wb + wn + bb + bn <= 1)
        return true;    // King + bishop/knight vs king is draw
    if (wn + bn == 0) {
        // Only bishops. If they are all on the same color, the position is a draw.
        U64 bMask = pos.pieceTypeBB(Piece::WBISHOP) | pos.pieceTypeBB(Piece::BBISHOP);
        if (((bMask & BitBoard::maskDarkSq) == 0) ||
                ((bMask & BitBoard::maskLightSq) == 0))
            return true;
    }
    return false;
}

U64
Game::perfT(Position& pos, int depth) {
    if (depth == 0)
        return 1;
    U64 nodes = 0;
    MoveList moves;
    MoveGen::pseudoLegalMoves(pos, moves);
    MoveGen::removeIllegal(pos, moves);
    if (depth == 1)
        return moves.size;
    UndoInfo ui;
    for (int mi = 0; mi < moves.size; mi++) {
        const Move& m = moves[mi];
        pos.makeMove(m, ui);
        nodes += perfT(pos, depth - 1);
        pos.unMakeMove(m, ui);
    }
    return nodes;
}
