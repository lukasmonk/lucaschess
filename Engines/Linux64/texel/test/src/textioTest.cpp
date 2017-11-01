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
 * textioTest.cpp
 *
 *  Created on: Feb 25, 2012
 *      Author: petero
 */

#include "textioTest.hpp"
#include "textio.hpp"

#include "cute.h"

/**
 *  Tests if trying to parse a FEN string causes an error.
 *  */
static bool
testFENParseError(const std::string& fen) {
    bool wasError = false;
    try {
        TextIO::readFEN(fen);
    } catch (const ChessParseError& err) {
        wasError = true;
    }
    return wasError;
}

/**
 * Test of readFEN method, of class TextIO.
 */
static void
testReadFEN() {
    std::string fen = "rnbqk2r/1p3ppp/p7/1NpPp3/QPP1P1n1/P4N2/4KbPP/R1B2B1R b kq - 0 1";
    Position pos = TextIO::readFEN(fen);
    ASSERT_EQUAL(fen, TextIO::toFEN(pos));
    ASSERT_EQUAL(Piece::WQUEEN, pos.getPiece(Position::getSquare(0, 3)));
    ASSERT_EQUAL(Piece::BKING, pos.getPiece(Position::getSquare(4, 7)));
    ASSERT_EQUAL(Piece::WKING, pos.getPiece(Position::getSquare(4, 1)));
    ASSERT_EQUAL(false, pos.isWhiteMove());
    ASSERT_EQUAL(false, pos.a1Castle());
    ASSERT_EQUAL(false, pos.h1Castle());
    ASSERT_EQUAL(true, pos.a8Castle());
    ASSERT_EQUAL(true, pos.h8Castle());

    fen = "8/3k4/8/5pP1/1P6/1NB5/2QP4/R3K2R w KQ f6 1 2";
    pos = TextIO::readFEN(fen);
    ASSERT_EQUAL(fen, TextIO::toFEN(pos));
    ASSERT_EQUAL(1, pos.getHalfMoveClock());
    ASSERT_EQUAL(2, pos.getFullMoveCounter());

    // Must have exactly one king
    bool wasError = testFENParseError("8/8/8/8/8/8/8/kk1K4 w - - 0 1");
    ASSERT_EQUAL(true, wasError);

    // Must not be possible to capture the king
    wasError = testFENParseError("8/8/8/8/8/8/8/k1RK4 w - - 0 1");
    ASSERT_EQUAL(true, wasError);

    // Make sure bogus en passant square information is removed
    fen = "rnbqkbnr/pppppppp/8/8/4P3/8/PPPP1PPP/RNBQKBNR b KQkq e3 0 1";
    pos = TextIO::readFEN(fen);
    ASSERT_EQUAL(-1, pos.getEpSquare());

    // Test for too many rows (slashes)
    wasError = testFENParseError("8/8/8/8/4k3/8/8/8/KBN5 w - - 0 1");
    ASSERT_EQUAL(true, wasError);

    // Test for too many columns
    wasError = testFENParseError("8K/8/8/8/4k3/8/8/8 w - - 0 1");
    ASSERT_EQUAL(true, wasError);

    // Pawns must not be on first/last rank
    wasError = testFENParseError("kp6/8/8/8/8/8/8/K7 w - - 0 1");
    ASSERT_EQUAL(true, wasError);

    wasError = testFENParseError("kr/pppp/8/8/8/8/8/KBR w");
    ASSERT_EQUAL(false, wasError);  // OK not to specify castling flags and ep square

    wasError = testFENParseError("k/8/8/8/8/8/8/K");
    ASSERT_EQUAL(true, wasError);   // Error side to move not specified

    wasError = testFENParseError("");
    ASSERT_EQUAL(true, wasError);

    wasError = testFENParseError("    |");
    ASSERT_EQUAL(true, wasError);

    wasError = testFENParseError("1B1B4/6k1/7r/7P/6q1/r7/q7/7K b - - acn 6; acs 0;");
    ASSERT_EQUAL(false, wasError);  // Extra stuff after FEN string is allowed

    // Test invalid en passant square detection
    pos = TextIO::readFEN("rnbqkbnr/pp1ppppp/8/8/2pPP3/8/PPP2PPP/RNBQKBNR b KQkq d3 0 1");
    ASSERT_EQUAL(TextIO::getSquare("d3"), pos.getEpSquare());

    pos = TextIO::readFEN("rnbqkbnr/pp1ppppp/8/8/2pPP3/8/PPP2PPP/RNBQKBNR w KQkq d3 0 1");
    ASSERT(pos.equals(TextIO::readFEN("rnbqkbnr/pp1ppppp/8/8/2pPP3/8/PPP2PPP/RNBQKBNR w KQkq - 0 1")));

    pos = TextIO::readFEN("rnbqkbnr/ppp2ppp/8/2Ppp3/8/8/PP1PPPPP/RNBQKBNR w KQkq d6 0 1");
    ASSERT_EQUAL(TextIO::getSquare("d6"), pos.getEpSquare());

    pos = TextIO::readFEN("rnbqkbnr/ppp2ppp/8/2Ppp3/8/8/PP1PPPPP/RNBQKBNR b KQkq d6 0 1");
    ASSERT(pos.equals(TextIO::readFEN("rnbqkbnr/ppp2ppp/8/2Ppp3/8/8/PP1PPPPP/RNBQKBNR b KQkq - 0 1")));

    pos = TextIO::readFEN("rnbqkbnr/pppppppp/8/8/3PP3/8/PPP2PPP/RNBQKBNR b KQkq d3 0 1");
    ASSERT_EQUAL(-1, pos.getEpSquare());

    pos = TextIO::readFEN("rnbqkbnr/ppp2ppp/8/3pp3/8/8/PPPPPPPP/RNBQKBNR w KQkq e6 0 1");
    ASSERT_EQUAL(-1, pos.getEpSquare());

    pos = TextIO::readFEN("rnbqkbnr/pp1ppppp/8/8/2pPP3/3P4/PP3PPP/RNBQKBNR b KQkq d3 0 1");
    ASSERT(pos.equals(TextIO::readFEN("rnbqkbnr/pp1ppppp/8/8/2pPP3/3P4/PP3PPP/RNBQKBNR b KQkq - 0 1")));
}

/**
 * Test of moveToString method, of class TextIO.
 */
static void
testMoveToString() {
    Position pos = TextIO::readFEN(TextIO::startPosFEN);
    ASSERT_EQUAL(TextIO::startPosFEN, TextIO::toFEN(pos));
    Move move(Position::getSquare(4, 1), Position::getSquare(4, 3), Piece::EMPTY);
    bool longForm = true;
    std::string result = TextIO::moveToString(pos, move, longForm);
    ASSERT_EQUAL("e2-e4", result);

    move = Move(Position::getSquare(6, 0), Position::getSquare(5, 2), Piece::EMPTY);
    result = TextIO::moveToString(pos, move, longForm);
    ASSERT_EQUAL("Ng1-f3", result);

    move = Move(Position::getSquare(4, 7), Position::getSquare(2, 7), Piece::EMPTY);
    result = TextIO::moveToString(pos, move, longForm);
    ASSERT_EQUAL("O-O-O", result);

    std::string fen = "1r3k2/2P5/8/8/8/4K3/8/8 w - - 0 1";
    pos = TextIO::readFEN(fen);
    ASSERT_EQUAL(fen, TextIO::toFEN(pos));
    move = Move(Position::getSquare(2,6), Position::getSquare(1,7), Piece::WROOK);
    result = TextIO::moveToString(pos, move, longForm);
    ASSERT_EQUAL("c7xb8R+", result);

    move = Move(Position::getSquare(2,6), Position::getSquare(2,7), Piece::WKNIGHT);
    result = TextIO::moveToString(pos, move, longForm);
    ASSERT_EQUAL("c7-c8N", result);

    move = Move(Position::getSquare(2,6), Position::getSquare(2,7), Piece::WQUEEN);
    result = TextIO::moveToString(pos, move, longForm);
    ASSERT_EQUAL("c7-c8Q+", result);
}

/**
 * Test of moveToString method, of class TextIO, mate/stalemate tests.
 */
static void
testMoveToStringMate() {
    Position pos = TextIO::readFEN("3k4/1PR5/3N4/8/4K3/8/8/8 w - - 0 1");
    bool longForm = true;

    Move move(Position::getSquare(1, 6), Position::getSquare(1, 7), Piece::WROOK);
    std::string result = TextIO::moveToString(pos, move, longForm);
    ASSERT_EQUAL("b7-b8R+", result);    // check

    move = Move(Position::getSquare(1, 6), Position::getSquare(1, 7), Piece::WQUEEN);
    result = TextIO::moveToString(pos, move, longForm);
    ASSERT_EQUAL("b7-b8Q#", result);    // check mate

    move = Move(Position::getSquare(1, 6), Position::getSquare(1, 7), Piece::WKNIGHT);
    result = TextIO::moveToString(pos, move, longForm);
    ASSERT_EQUAL("b7-b8N", result);

    move = Move(Position::getSquare(1, 6), Position::getSquare(1, 7), Piece::WBISHOP);
    result = TextIO::moveToString(pos, move, longForm);
    ASSERT_EQUAL("b7-b8B", result);     // stalemate
}

/**
 * Test of moveToString method, of class TextIO, short form.
 */
static void
testMoveToStringShortForm() {
    std::string fen = "r4rk1/2pn3p/2q1q1n1/8/2q2p2/6R1/p4PPP/1R4K1 b - - 0 1";
    Position pos = TextIO::readFEN(fen);
    ASSERT_EQUAL(fen, TextIO::toFEN(pos));
    bool longForm = false;

    Move move = Move(Position::getSquare(4,5), Position::getSquare(4,3), Piece::EMPTY);
    std::string result = TextIO::moveToString(pos, move, longForm);
    ASSERT_EQUAL("Qee4", result);   // File disambiguation needed

    move = Move(Position::getSquare(2,5), Position::getSquare(4,3), Piece::EMPTY);
    result = TextIO::moveToString(pos, move, longForm);
    ASSERT_EQUAL("Qc6e4", result);  // Full disambiguation needed

    move = Move(Position::getSquare(2,3), Position::getSquare(4,3), Piece::EMPTY);
    result = TextIO::moveToString(pos, move, longForm);
    ASSERT_EQUAL("Q4e4", result);   // Row disambiguation needed

    move = Move(Position::getSquare(2,3), Position::getSquare(2,0), Piece::EMPTY);
    result = TextIO::moveToString(pos, move, longForm);
    ASSERT_EQUAL("Qc1+", result);   // No disambiguation needed

    move = Move(Position::getSquare(0,1), Position::getSquare(0,0), Piece::BQUEEN);
    result = TextIO::moveToString(pos, move, longForm);
    ASSERT_EQUAL("a1Q", result);    // Normal promotion

    move = Move(Position::getSquare(0,1), Position::getSquare(1,0), Piece::BQUEEN);
    result = TextIO::moveToString(pos, move, longForm);
    ASSERT_EQUAL("axb1Q#", result); // Capture promotion and check mate

    move = Move(Position::getSquare(0,1), Position::getSquare(1,0), Piece::BKNIGHT);
    result = TextIO::moveToString(pos, move, longForm);
    ASSERT_EQUAL("axb1N", result);  // Capture promotion

    move = Move(Position::getSquare(3,6), Position::getSquare(4,4), Piece::EMPTY);
    result = TextIO::moveToString(pos, move, longForm);
    ASSERT_EQUAL("Ne5", result);    // Other knight pinned, no disambiguation needed

    move = Move(Position::getSquare(7,6), Position::getSquare(7,4), Piece::EMPTY);
    result = TextIO::moveToString(pos, move, longForm);
    ASSERT_EQUAL("h5", result);     // Regular pawn move

    move = Move(Position::getSquare(5,7), Position::getSquare(3,7), Piece::EMPTY);
    result = TextIO::moveToString(pos, move, longForm);
    ASSERT_EQUAL("Rfd8", result);     // File disambiguation needed
}

/**
 * Test of stringToMove method, of class TextIO.
 */
static void
testStringToMove() {
    Position pos = TextIO::readFEN("r4rk1/2pn3p/2q1q1n1/8/2q2p2/6R1/p4PPP/1R4K1 b - - 0 1");

    Move mNe5(Position::getSquare(3, 6), Position::getSquare(4, 4), Piece::EMPTY);
    Move m = TextIO::stringToMove(pos, "Ne5");
    ASSERT_EQUAL(mNe5, m);
    m = TextIO::stringToMove(pos, "ne");
    ASSERT_EQUAL(mNe5, m);
    m = TextIO::stringToMove(pos, "N");
    ASSERT(m.isEmpty());

    Move mQc6e4(Position::getSquare(2, 5), Position::getSquare(4, 3), Piece::EMPTY);
    m = TextIO::stringToMove(pos, "Qc6-e4");
    ASSERT_EQUAL(mQc6e4, m);
    m = TextIO::stringToMove(pos, "Qc6e4");
    ASSERT_EQUAL(mQc6e4, m);
    m = TextIO::stringToMove(pos, "Qce4");
    ASSERT(m.isEmpty());
    m = TextIO::stringToMove(pos, "Q6e4");
    ASSERT(m.isEmpty());

    Move maxb1Q(Position::getSquare(0, 1), Position::getSquare(1, 0), Piece::BQUEEN);
    m = TextIO::stringToMove(pos, "axb1Q");
    ASSERT_EQUAL(maxb1Q, m);
    m = TextIO::stringToMove(pos, "axb1Q#");
    ASSERT_EQUAL(maxb1Q, m);
    m = TextIO::stringToMove(pos, "axb1Q+");
    ASSERT_EQUAL(maxb1Q, m);

    Move mh5(Position::getSquare(7, 6), Position::getSquare(7, 4), Piece::EMPTY);
    m = TextIO::stringToMove(pos, "h5");
    ASSERT_EQUAL(mh5, m);
    m = TextIO::stringToMove(pos, "h7-h5");
    ASSERT_EQUAL(mh5, m);
    m = TextIO::stringToMove(pos, "h");
    ASSERT(m.isEmpty());

    pos = TextIO::readFEN("r1b1k2r/1pqpppbp/p5pn/3BP3/8/2pP4/PPPBQPPP/R3K2R w KQkq - 0 12");
    m = TextIO::stringToMove(pos, "bxc3");
    ASSERT_EQUAL(TextIO::getSquare("b2"), m.from());
    m = TextIO::stringToMove(pos, "Bxc3");
    ASSERT_EQUAL(TextIO::getSquare("d2"), m.from());
    m = TextIO::stringToMove(pos, "bxc");
    ASSERT_EQUAL(TextIO::getSquare("b2"), m.from());
    m = TextIO::stringToMove(pos, "Bxc");
    ASSERT_EQUAL(TextIO::getSquare("d2"), m.from());

    // Test castling. o-o is a substring of o-o-o, which could cause problems.
    pos = TextIO::readFEN("5k2/p1pQn3/1p2Bp1r/8/4P1pN/2N5/PPP2PPP/R3K2R w KQ - 0 16");
    Move kCastle(Position::getSquare(4,0), Position::getSquare(6,0), Piece::EMPTY);
    Move qCastle(Position::getSquare(4,0), Position::getSquare(2,0), Piece::EMPTY);
    m = TextIO::stringToMove(pos, "o");
    ASSERT(m.isEmpty());
    m = TextIO::stringToMove(pos, "o-o");
    ASSERT_EQUAL(kCastle, m);
    m = TextIO::stringToMove(pos, "O-O");
    ASSERT_EQUAL(kCastle, m);
    m = TextIO::stringToMove(pos, "o-o-o");
    ASSERT_EQUAL(qCastle, m);

    // Test 'o-o+'
    pos.setPiece(Position::getSquare(5,1), Piece::EMPTY);
    pos.setPiece(Position::getSquare(5,5), Piece::EMPTY);
    m = TextIO::stringToMove(pos, "o");
    ASSERT(m.isEmpty());
    m = TextIO::stringToMove(pos, "o-o");
    ASSERT_EQUAL(kCastle, m);
    m = TextIO::stringToMove(pos, "o-o-o");
    ASSERT_EQUAL(qCastle, m);
    m = TextIO::stringToMove(pos, "o-o+");
    ASSERT_EQUAL(kCastle, m);

    // Test d8=Q+ syntax
    pos = TextIO::readFEN("1r3r2/2kP2Rp/p1bN1p2/2p5/5P2/2P5/P5PP/3R2K1 w - -");
    m = TextIO::stringToMove(pos, "d8=Q+");
    Move m2 = TextIO::stringToMove(pos, "d8Q");
    ASSERT_EQUAL(m2, m);

    // Test non-standard castling syntax
    pos = TextIO::readFEN("r3k2r/pppqbppp/2npbn2/4p3/2B1P3/2NPBN2/PPPQ1PPP/R3K2R w KQkq - 0 1");
    m = TextIO::stringToMove(pos, "0-0");
    ASSERT_EQUAL(Move(TextIO::getSquare("e1"), TextIO::getSquare("g1"), Piece::EMPTY), m);
    m = TextIO::stringToMove(pos, "0-0-0");
    ASSERT_EQUAL(Move(TextIO::getSquare("e1"), TextIO::getSquare("c1"), Piece::EMPTY), m);
    pos.setWhiteMove(false);
    m = TextIO::stringToMove(pos, "0-0");
    ASSERT_EQUAL(Move(TextIO::getSquare("e8"), TextIO::getSquare("g8"), Piece::EMPTY), m);
    m = TextIO::stringToMove(pos, "0-0-0");
    ASSERT_EQUAL(Move(TextIO::getSquare("e8"), TextIO::getSquare("c8"), Piece::EMPTY), m);

    // Test non-standard disambiguation
    pos = TextIO::readFEN("1Q6/1K2q2k/1QQ5/8/7P/8/8/8 w - - 3 88");
    m = TextIO::stringToMove(pos, "Qb8c7");
    ASSERT_EQUAL(Move(TextIO::getSquare("b8"), TextIO::getSquare("c7"), Piece::EMPTY), m);
    m2 = TextIO::stringToMove(pos, "Q8c7");
    ASSERT_EQUAL(m2, m);

    // Test extra characters
    pos = TextIO::readFEN(TextIO::startPosFEN);
    Move mNf3(TextIO::getSquare("g1"), TextIO::getSquare("f3"), Piece::EMPTY);
    ASSERT_EQUAL(mNf3, TextIO::stringToMove(pos, "Ngf3"));
    ASSERT_EQUAL(mNf3, TextIO::stringToMove(pos, "Ng1f3"));
    ASSERT_EQUAL(mNf3, TextIO::stringToMove(pos, "Ng1-f3"));
    ASSERT_EQUAL(mNf3, TextIO::stringToMove(pos, "g1f3"));
    ASSERT_EQUAL(mNf3, TextIO::stringToMove(pos, "N1f3"));
    ASSERT_EQUAL(mNf3, TextIO::stringToMove(pos, "Ngf"));
    ASSERT_EQUAL(mNf3, TextIO::stringToMove(pos, "Nf"));
}

/**
 * Test of getSquare method, of class TextIO.
 */
static void
testGetSquare() {
    ASSERT_EQUAL(Position::getSquare(0, 0), TextIO::getSquare("a1"));
    ASSERT_EQUAL(Position::getSquare(1, 7), TextIO::getSquare("b8"));
    ASSERT_EQUAL(Position::getSquare(3, 3), TextIO::getSquare("d4"));
    ASSERT_EQUAL(Position::getSquare(4, 3), TextIO::getSquare("e4"));
    ASSERT_EQUAL(Position::getSquare(3, 1), TextIO::getSquare("d2"));
    ASSERT_EQUAL(Position::getSquare(7, 7), TextIO::getSquare("h8"));
}

/**
 * Test of squareToString method, of class TextIO.
 */
static void
testSquareToString() {
    ASSERT_EQUAL("a1", TextIO::squareToString(Position::getSquare(0, 0)));
    ASSERT_EQUAL("h6", TextIO::squareToString(Position::getSquare(7, 5)));
    ASSERT_EQUAL("e4", TextIO::squareToString(Position::getSquare(4, 3)));
}

static int countSubStr(const std::string& str, const std::string& sub) {
    int N = sub.length();
    int ret = 0;
    size_t pos = 0;
    while (pos < str.length()) {
        pos = str.find(sub, pos);
        if (pos == std::string::npos)
            break;
        else {
            pos += N;
            ret++;
        }
    }
    return ret;
}

/**
 * Test of asciiBoard method, of class TextIO.
 */
static void
testAsciiBoard() {
    Position pos = TextIO::readFEN("r4rk1/2pn3p/2q1q1n1/8/2q2p2/6R1/p4PPP/1R4K1 b - - 0 1");
    std::string aBrd = TextIO::asciiBoard(pos);
    //        printf("%s\n", aBrd.c_str());
    ASSERT_EQUAL(12, countSubStr(aBrd, "*")); // 12 black pieces
    ASSERT_EQUAL(3, countSubStr(aBrd, "*Q")); // 3 black queens
    ASSERT_EQUAL(3, countSubStr(aBrd, " P")); // 3 white pawns

    aBrd = TextIO::asciiBoard(BitBoard::sqMask(A1,C2,D4));
    ASSERT_EQUAL(3, countSubStr(aBrd, "1"));
    std::string sqList = TextIO::squareList(BitBoard::sqMask(A1,C2,D4));
    ASSERT_EQUAL("a1,c2,d4", sqList);
}

/**
 * Test of uciStringToMove method, of class TextIO.
 */
static void
testUciStringToMove() {
    Position pos = TextIO::readFEN(TextIO::startPosFEN);
    Move m = TextIO::uciStringToMove("e2e4");
    ASSERT_EQUAL(TextIO::stringToMove(pos, "e4"), m);
    m = TextIO::uciStringToMove("e2e5");
    ASSERT_EQUAL(Move(12, 12+8*3, Piece::EMPTY), m);

    m = TextIO::uciStringToMove("e2e5q");
    ASSERT(m.isEmpty());

    m = TextIO::uciStringToMove("e7e8q");
    ASSERT_EQUAL(Piece::WQUEEN, m.promoteTo());
    m = TextIO::uciStringToMove("e7e8r");
    ASSERT_EQUAL(Piece::WROOK, m.promoteTo());
    m = TextIO::uciStringToMove("e7e8b");
    ASSERT_EQUAL(Piece::WBISHOP, m.promoteTo());
    m = TextIO::uciStringToMove("e2e1n");
    ASSERT_EQUAL(Piece::BKNIGHT, m.promoteTo());
    m = TextIO::uciStringToMove("e7e8x");
    ASSERT(m.isEmpty());  // Invalid promotion piece
    m = TextIO::uciStringToMove("i1i3");
    ASSERT(m.isEmpty());  // Outside board
    m = TextIO::uciStringToMove("h8h9");
    ASSERT(m.isEmpty());  // Outside board
    m = TextIO::uciStringToMove("c1c0");
    ASSERT(m.isEmpty());  // Outside board
}


cute::suite
TextIOTest::getSuite() const {
    cute::suite s;
    s.push_back(CUTE(testReadFEN));
    s.push_back(CUTE(testMoveToString));
    s.push_back(CUTE(testMoveToStringMate));
    s.push_back(CUTE(testMoveToStringShortForm));
    s.push_back(CUTE(testStringToMove));
    s.push_back(CUTE(testGetSquare));
    s.push_back(CUTE(testSquareToString));
    s.push_back(CUTE(testAsciiBoard));
    s.push_back(CUTE(testUciStringToMove));
    return s;
}
