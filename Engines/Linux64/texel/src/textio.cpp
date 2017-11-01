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
 * textio.cpp
 *
 *  Created on: Feb 25, 2012
 *      Author: petero
 */

#include "textio.hpp"
#include "moveGen.hpp"
#include <cassert>

const std::string TextIO::startPosFEN = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";


Position
TextIO::readFEN(const std::string& fen) {
    Position pos;

    // Piece placement
    int row = 7;
    int col = 0;
    size_t i;
    for (i = 0; i < fen.length(); i++) {
        char c = fen[i];
        if (c == ' ')
            break;
        switch (c) {
            case '1': col += 1; break;
            case '2': col += 2; break;
            case '3': col += 3; break;
            case '4': col += 4; break;
            case '5': col += 5; break;
            case '6': col += 6; break;
            case '7': col += 7; break;
            case '8': col += 8; break;
            case '/':
                row--; col = 0;
                if (row < 0) throw ChessParseError("Too many rows");
                break;
            case 'P': safeSetPiece(pos, col, row, Piece::WPAWN);   col++; break;
            case 'N': safeSetPiece(pos, col, row, Piece::WKNIGHT); col++; break;
            case 'B': safeSetPiece(pos, col, row, Piece::WBISHOP); col++; break;
            case 'R': safeSetPiece(pos, col, row, Piece::WROOK);   col++; break;
            case 'Q': safeSetPiece(pos, col, row, Piece::WQUEEN);  col++; break;
            case 'K': safeSetPiece(pos, col, row, Piece::WKING);   col++; break;
            case 'p': safeSetPiece(pos, col, row, Piece::BPAWN);   col++; break;
            case 'n': safeSetPiece(pos, col, row, Piece::BKNIGHT); col++; break;
            case 'b': safeSetPiece(pos, col, row, Piece::BBISHOP); col++; break;
            case 'r': safeSetPiece(pos, col, row, Piece::BROOK);   col++; break;
            case 'q': safeSetPiece(pos, col, row, Piece::BQUEEN);  col++; break;
            case 'k': safeSetPiece(pos, col, row, Piece::BKING);   col++; break;
            default: throw ChessParseError("Invalid piece");
        }
    }
    while (i < fen.length() && fen[i] == ' ')
        i++;
    if (i >= fen.length())
        throw ChessParseError("Invalid side");
    pos.setWhiteMove(fen[i++] == 'w');

    // Castling rights
    int castleMask = 0;
    while (i < fen.length() && fen[i] == ' ')
        i++;
    for ( ; i < fen.length(); i++) {
        char c = fen[i];
        if (c == ' ')
            break;
        switch (c) {
        case 'K': castleMask |= (1 << Position::H1_CASTLE); break;
        case 'Q': castleMask |= (1 << Position::A1_CASTLE); break;
        case 'k': castleMask |= (1 << Position::H8_CASTLE); break;
        case 'q': castleMask |= (1 << Position::A8_CASTLE); break;
        case '-': break;
        default: throw ChessParseError("Invalid castling flags");
        }
    }
    pos.setCastleMask(castleMask);

    while (i < fen.length() && fen[i] == ' ')
        i++;

    if (i < fen.length()) {
        // En passant target square
        if (fen[i] != '-') {
            if (i >= fen.length() - 1)
                throw ChessParseError("Invalid en passant square");
            int epSq = getSquare(fen.substr(i, 2));
            if (epSq != -1) {
                if (pos.isWhiteMove()) {
                    if ((Position::getY(epSq) != 5) || (pos.getPiece(epSq) != Piece::EMPTY) ||
                            (pos.getPiece(epSq - 8) != Piece::BPAWN))
                        epSq = -1;
                } else {
                    if ((Position::getY(epSq) != 2) || (pos.getPiece(epSq) != Piece::EMPTY) ||
                            (pos.getPiece(epSq + 8) != Piece::WPAWN))
                        epSq = -1;
                }
                pos.setEpSquare(epSq);
            }
        }
        while (i < fen.length() && fen[i] != ' ')
            i++;
    }

    while (i < fen.length() && fen[i] == ' ')
        i++;
    if (i < fen.length()) {
        int i0 = i;
        while (i < fen.length() && fen[i] != ' ')
            i++;
        int halfMoveClock;
        if (str2Num(fen.substr(i0, i - i0), halfMoveClock))
            pos.setHalfMoveClock(halfMoveClock);
    }
    while (i < fen.length() && fen[i] == ' ')
        i++;
    if (i < fen.length()) {
        int i0 = i;
        while (i < fen.length() && fen[i] != ' ')
            i++;
        int fullMoveCounter;
        if (str2Num(fen.substr(i0, i - i0), fullMoveCounter))
            pos.setFullMoveCounter(fullMoveCounter);
    }

    // Each side must have exactly one king
    int wKings = 0;
    int bKings = 0;
    for (int x = 0; x < 8; x++) {
        for (int y = 0; y < 8; y++) {
            int p = pos.getPiece(Position::getSquare(x, y));
            if (p == Piece::WKING)
                wKings++;
            else if (p == Piece::BKING)
                bKings++;
        }
    }
    if (wKings != 1)
        throw ChessParseError("White must have exactly one king");
    if (bKings != 1)
        throw ChessParseError("Black must have exactly one king");

    // Make sure king can not be captured
    Position pos2(pos);
    pos2.setWhiteMove(!pos.isWhiteMove());
    if (MoveGen::inCheck(pos2))
        throw ChessParseError("King capture possible");

    fixupEPSquare(pos);
    return pos;
}


void
TextIO::fixupEPSquare(Position& pos) {
    int epSquare = pos.getEpSquare();
    if (epSquare >= 0) {
        MoveList moves;
        MoveGen::pseudoLegalMoves(pos, moves);
        MoveGen::removeIllegal(pos, moves);
        bool epValid = false;
        for (int mi = 0; mi < moves.size; mi++) {
            const Move& m = moves[mi];
            if (m.to() == epSquare) {
                if (pos.getPiece(m.from()) == (pos.isWhiteMove() ? Piece::WPAWN : Piece::BPAWN)) {
                    epValid = true;
                    break;
                }
            }
        }
        if (!epValid)
            pos.setEpSquare(-1);
    }
}


std::string
TextIO::toFEN(const Position& pos) {
    std::string ret;
    // Piece placement
    for (int r = 7; r >=0; r--) {
        int numEmpty = 0;
        for (int c = 0; c < 8; c++) {
            int p = pos.getPiece(Position::getSquare(c, r));
            if (p == Piece::EMPTY) {
                numEmpty++;
            } else {
                if (numEmpty > 0) {
                    ret += (char)('0' + numEmpty);
                    numEmpty = 0;
                }
                switch (p) {
                    case Piece::WKING:   ret += 'K'; break;
                    case Piece::WQUEEN:  ret += 'Q'; break;
                    case Piece::WROOK:   ret += 'R'; break;
                    case Piece::WBISHOP: ret += 'B'; break;
                    case Piece::WKNIGHT: ret += 'N'; break;
                    case Piece::WPAWN:   ret += 'P'; break;
                    case Piece::BKING:   ret += 'k'; break;
                    case Piece::BQUEEN:  ret += 'q'; break;
                    case Piece::BROOK:   ret += 'r'; break;
                    case Piece::BBISHOP: ret += 'b'; break;
                    case Piece::BKNIGHT: ret += 'n'; break;
                    case Piece::BPAWN:   ret += 'p'; break;
                    default: assert(false); break;
                }
            }
        }
        if (numEmpty > 0)
            ret += (char)('0' + numEmpty);
        if (r > 0)
            ret += '/';
    }
    ret += (pos.isWhiteMove() ? " w " : " b ");

    // Castling rights
    bool anyCastle = false;
    if (pos.h1Castle()) {
        ret += 'K';
        anyCastle = true;
    }
    if (pos.a1Castle()) {
        ret += 'Q';
        anyCastle = true;
    }
    if (pos.h8Castle()) {
        ret += 'k';
        anyCastle = true;
    }
    if (pos.a8Castle()) {
        ret += 'q';
        anyCastle = true;
    }
    if (!anyCastle) {
        ret += '-';
    }

    // En passant target square
    {
        ret += ' ';
        if (pos.getEpSquare() >= 0) {
            int x = Position::getX(pos.getEpSquare());
            int y = Position::getY(pos.getEpSquare());
            ret += ((char)(x + 'a'));
            ret += ((char)(y + '1'));
        } else {
            ret += '-';
        }
    }

    // Move counters
    ret += ' ';
    ret += num2Str(pos.getHalfMoveClock());
    ret += ' ';
    ret += num2Str(pos.getFullMoveCounter());

    return ret;
}

std::string
TextIO::moveToUCIString(const Move& m) {
    std::string ret = squareToString(m.from());
    ret += squareToString(m.to());
    switch (m.promoteTo()) {
        case Piece::WQUEEN:
        case Piece::BQUEEN:
            ret += "q";
            break;
        case Piece::WROOK:
        case Piece::BROOK:
            ret += "r";
            break;
        case Piece::WBISHOP:
        case Piece::BBISHOP:
            ret += "b";
            break;
        case Piece::WKNIGHT:
        case Piece::BKNIGHT:
            ret += "n";
            break;
        default:
            break;
    }
    return ret;
}

Move
TextIO::uciStringToMove(const std::string& move) {
    Move m;
    if ((move.length() < 4) || (move.length() > 5))
        return m;
    int fromSq = TextIO::getSquare(move.substr(0, 2));
    int toSq   = TextIO::getSquare(move.substr(2, 2));
    if ((fromSq < 0) || (toSq < 0)) {
        return m;
    }
    char prom = ' ';
    bool white = true;
    if (move.length() == 5) {
        prom = move[4];
        if (Position::getY(toSq) == 7) {
            white = true;
        } else if (Position::getY(toSq) == 0) {
            white = false;
        } else {
            return m;
        }
    }
    int promoteTo;
    switch (prom) {
        case ' ':
            promoteTo = Piece::EMPTY;
            break;
        case 'q':
            promoteTo = white ? Piece::WQUEEN : Piece::BQUEEN;
            break;
        case 'r':
            promoteTo = white ? Piece::WROOK : Piece::BROOK;
            break;
        case 'b':
            promoteTo = white ? Piece::WBISHOP : Piece::BBISHOP;
            break;
        case 'n':
            promoteTo = white ? Piece::WKNIGHT : Piece::BKNIGHT;
            break;
        default:
            return m;
    }
    return Move(fromSq, toSq, promoteTo);
}

static bool
isCapture(const Position& pos, const Move& move) {
    if (pos.getPiece(move.to()) != Piece::EMPTY)
        return true;
    int p = pos.getPiece(move.from());
    return (p == (pos.isWhiteMove() ? Piece::WPAWN : Piece::BPAWN)) &&
           (move.to() == pos.getEpSquare());
}

static std::string
pieceToChar(int p) {
    switch (p) {
        case Piece::WQUEEN:  case Piece::BQUEEN:  return "Q";
        case Piece::WROOK:   case Piece::BROOK:   return "R";
        case Piece::WBISHOP: case Piece::BBISHOP: return "B";
        case Piece::WKNIGHT: case Piece::BKNIGHT: return "N";
        case Piece::WKING:   case Piece::BKING:   return "K";
    }
    return "";
}

static std::string
moveToString(Position& pos, const Move& move, bool longForm, const MoveList& moves) {
    std::string ret;
    int wKingOrigPos = Position::getSquare(4, 0);
    int bKingOrigPos = Position::getSquare(4, 7);
    if (move.from() == wKingOrigPos && pos.getPiece(wKingOrigPos) == Piece::WKING) {
        // Check white castle
        if (move.to() == Position::getSquare(6, 0))
            ret += "O-O";
        else if (move.to() == Position::getSquare(2, 0))
            ret += "O-O-O";
    } else if (move.from() == bKingOrigPos && pos.getPiece(bKingOrigPos) == Piece::BKING) {
        // Check black castle
        if (move.to() == Position::getSquare(6, 7))
            ret += "O-O";
        else if (move.to() == Position::getSquare(2, 7))
            ret += "O-O-O";
    }
    if (ret.length() == 0) {
        int p = pos.getPiece(move.from());
        ret += pieceToChar(p);
        int x1 = Position::getX(move.from());
        int y1 = Position::getY(move.from());
        int x2 = Position::getX(move.to());
        int y2 = Position::getY(move.to());
        if (longForm) {
            ret += (char)(x1 + 'a');
            ret += (char)(y1 + '1');
            ret += isCapture(pos, move) ? 'x' : '-';
        } else {
            if (p == (pos.isWhiteMove() ? Piece::WPAWN : Piece::BPAWN)) {
                if (isCapture(pos, move))
                    ret += (char)(x1 + 'a');
            } else {
                int numSameTarget = 0;
                int numSameFile = 0;
                int numSameRow = 0;
                for (int mi = 0; mi < moves.size; mi++) {
                    const Move& m = moves[mi];
                    if (m.isEmpty())
                        break;
                    if ((pos.getPiece(m.from()) == p) && (m.to() == move.to())) {
                        numSameTarget++;
                        if (Position::getX(m.from()) == x1)
                            numSameFile++;
                        if (Position::getY(m.from()) == y1)
                            numSameRow++;
                    }
                }
                if (numSameTarget < 2) {
                    // No file/row info needed
                } else if (numSameFile < 2) {
                    ret += (char)(x1 + 'a');   // Only file info needed
                } else if (numSameRow < 2) {
                    ret += (char)(y1 + '1');   // Only row info needed
                } else {
                    ret += (char) (x1 + 'a');   // File and row info needed
                    ret += (char) (y1 + '1');
                }
            }
            if (isCapture(pos, move))
                ret += 'x';
        }
        ret += (char)(x2 + 'a');
        ret += (char)(y2 + '1');
        if (move.promoteTo() != Piece::EMPTY)
            ret += pieceToChar(move.promoteTo());
    }
    UndoInfo ui;
    if (MoveGen::givesCheck(pos, move)) {
        pos.makeMove(move, ui);
        MoveList nextMoves;
        MoveGen::pseudoLegalMoves(pos, nextMoves);
        MoveGen::removeIllegal(pos, nextMoves);
        if (nextMoves.size == 0)
            ret += '#';
        else
            ret += '+';
        pos.unMakeMove(move, ui);
    }

    return ret;
}

std::string
TextIO::moveToString(const Position& pos, const Move& move, bool longForm) {
    MoveList moves;
    MoveGen::pseudoLegalMoves(pos, moves);
    Position tmpPos(pos);
    MoveGen::removeIllegal(tmpPos, moves);
    return ::moveToString(tmpPos, move, longForm, moves);
}

namespace {
    struct MoveInfo {
        int piece = -1;             // -1 for unspecified
        int fromX = -1, fromY = -1; // -1 for unspecified
        int toX = -1, toY = -1;     // -1 for unspecified
        int promPiece = -1;         // -1 for unspecified
    };
}

Move
TextIO::stringToMove(Position& pos, const std::string& strMoveIn) {
    std::string strMove;
    for (size_t i = 0; i < strMoveIn.length(); i++) {
        switch (strMoveIn[i]) {
        case '=':
        case '+':
        case '#':
            break;
        default:
            strMove += strMoveIn[i];
            break;
        }
    }

    Move move;
    if (strMove == "--")
        return move;

    const bool wtm = pos.isWhiteMove();

    MoveInfo info;
    bool capture = false;
    if ((strMove == "O-O") || (strMove =="0-0") || (strMove == "o-o")) {
        info.piece = wtm ? Piece::WKING : Piece::BKING;
        info.fromX = 4;
        info.toX = 6;
        info.fromY = info.toY = wtm ? 0 : 7;
        info.promPiece = Piece::EMPTY;
    } else if ((strMove == "O-O-O") || (strMove == "0-0-0") || (strMove == "o-o-o")) {
        info.piece = wtm ? Piece::WKING : Piece::BKING;
        info.fromX = 4;
        info.toX = 2;
        info.fromY = info.toY = wtm ? 0 : 7;
        info.promPiece = Piece::EMPTY;
    } else {
        bool atToSq = false;
        for (size_t i = 0; i < strMove.length(); i++) {
            char c = strMove[i];
            if (i == 0) {
                int piece = charToPiece(wtm, c);
                if (piece >= 0) {
                    info.piece = piece;
                    continue;
                }
            }
            int tmpX = c - 'a';
            if ((tmpX >= 0) && (tmpX < 8)) {
                if (atToSq || (info.fromX >= 0))
                    info.toX = tmpX;
                else
                    info.fromX = tmpX;
            }
            int tmpY = c - '1';
            if ((tmpY >= 0) && (tmpY < 8)) {
                if (atToSq || (info.fromY >= 0))
                    info.toY = tmpY;
                else
                    info.fromY = tmpY;
            }
            if ((c == 'x') || (c == '-')) {
                atToSq = true;
                if (c == 'x')
                    capture = true;
            }
            if (i == strMove.length() - 1) {
                int promPiece = charToPiece(wtm, c);
                if (promPiece >= 0) {
                    info.promPiece = promPiece;
                }
            }
        }
        if ((info.fromX >= 0) && (info.toX < 0)) {
            info.toX = info.fromX;
            info.fromX = -1;
        }
        if ((info.fromY >= 0) && (info.toY < 0)) {
            info.toY = info.fromY;
            info.fromY = -1;
        }
        if (info.piece < 0) {
            bool haveAll = (info.fromX >= 0) && (info.fromY >= 0) &&
                           (info.toX >= 0) && (info.toY >= 0);
            if (!haveAll)
                info.piece = wtm ? Piece::WPAWN : Piece::BPAWN;
        }
        if (info.promPiece < 0)
            info.promPiece = Piece::EMPTY;
    }

    MoveList moves;
    MoveGen::pseudoLegalMoves(pos, moves);
    MoveGen::removeIllegal(pos, moves);

    std::vector<Move> matches;
    for (int i = 0; i < moves.size; i++) {
        const Move& m = moves[i];
        int p = pos.getPiece(m.from());
        bool match = true;
        if ((info.piece >= 0) && (info.piece != p))
            match = false;
        if ((info.fromX >= 0) && (info.fromX != Position::getX(m.from())))
            match = false;
        if ((info.fromY >= 0) && (info.fromY != Position::getY(m.from())))
            match = false;
        if ((info.toX >= 0) && (info.toX != Position::getX(m.to())))
            match = false;
        if ((info.toY >= 0) && (info.toY != Position::getY(m.to())))
            match = false;
        if ((info.promPiece >= 0) && (info.promPiece != m.promoteTo()))
            match = false;
        if (match)
            matches.push_back(m);
    }
    int nMatches = matches.size();
    if (nMatches == 0)
        return move;
    else if (nMatches == 1)
        return matches[0];
    if (!capture)
        return move;
    for (size_t i = 0; i < matches.size(); i++) {
        const Move& m = matches[i];
        int capt = pos.getPiece(m.to());
        if (capt != Piece::EMPTY) {
            if (move.isEmpty()) {
                move = m;
            } else {
                move = Move();
                return move;
            }
        }
    }
    return move;
}

std::string
TextIO::asciiBoard(const Position& pos) {
    std::string ret;
    ret += "    +----+----+----+----+----+----+----+----+\n";
    for (int y = 7; y >= 0; y--) {
        ret += "    |";
        for (int x = 0; x < 8; x++) {
            ret += ' ';
            int p = pos.getPiece(Position::getSquare(x, y));
            if (p == Piece::EMPTY) {
                bool dark = Position::darkSquare(x, y);
                ret.append(dark ? ".. |" : "   |");
            } else {
                ret += Piece::isWhite(p) ? ' ' : '*';
                std::string pieceName = pieceToChar(p);
                if (pieceName.length() == 0)
                    pieceName = "P";

                ret += pieceName;
                ret += " |";
            }
        }

        ret += ("\n    +----+----+----+----+----+----+----+----+\n");
    }

    return ret;
}

std::string
TextIO::asciiBoard(U64 mask) {
    std::string ret;
    for (int y = 7; y >= 0; y--) {
        for (int x = 0; x < 8; x++) {
            int sq = Position::getSquare(x, y);
            ret += (mask & (1ULL << sq)) ? '1' : '0';
        }
        ret += '\n';
    }
    return ret;
}

std::string
TextIO::squareList(U64 mask) {
    std::string ret;
    while (mask) {
        int sq = BitBoard::extractSquare(mask);
        if (!ret.empty())
            ret += ',';
        ret += squareToString(sq);
    }
    return ret;
}

