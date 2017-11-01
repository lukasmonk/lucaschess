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
 * positionTest.cpp
 *
 *  Created on: Feb 25, 2012
 *      Author: petero
 */

#include "positionTest.hpp"

#include "cute.h"

#include "position.hpp"
#include "piece.hpp"
#include "material.hpp"
#include "textio.hpp"
#include "util/timeUtil.hpp"

#include <vector>
#include <set>

int PositionTest::computeMaterialId(const Position& pos) {
    MatId id;
    for (int sq = 0; sq < 64; sq++)
        id.addPiece(pos.getPiece(sq));
    return id();
}

/**
 * Test of getPiece method, of class Position.
 */
static void
testGetPiece() {
    Position pos;
    int result = pos.getPiece(0);
    ASSERT_EQUAL(result, Piece::EMPTY);

    pos = TextIO::readFEN(TextIO::startPosFEN);
    result = pos.getPiece(0);
    ASSERT_EQUAL(result, Piece::WROOK);
    for (int x = 0; x < 8; x++) {
        for (int y = 0; y < 2; y++) {
            int p1 = pos.getPiece(Position::getSquare(x, y));
            int p2 = pos.getPiece(Position::getSquare(x, 7-y));
            int bwDiff = Piece::BPAWN - Piece::WPAWN;
            ASSERT_EQUAL(p2, p1 + bwDiff);
        }
    }
}

/**
 * Test of getIndex method, of class Position.
 */
static void
testGetIndex() {
    for (int x = 0; x < 8; x++) {
        for (int y = 0; y < 8; y++) {
            int sq = Position::getSquare(x, y);
            int x2 = Position::getX(sq);
            int y2 = Position::getY(sq);
            ASSERT_EQUAL(x, x2);
            ASSERT_EQUAL(y, y2);
            ASSERT_EQUAL(Position::mirrorY(sq), Position::getSquare(x, 7-y));
        }
    }
}

/**
 * Test of setPiece method, of class Position.
 */
static void
testSetPiece() {
    Position instance;
    ASSERT_EQUAL(Piece::EMPTY, instance.getPiece(Position::getSquare(0, 0)));
    instance.setPiece(Position::getSquare(3, 4), Piece::WKING);
    ASSERT_EQUAL(Piece::WKING, instance.getPiece(Position::getSquare(3, 4)));
}

/**
 * Test of makeMove method, of class Position.
 */
static void
testMakeMove() {
    Position pos = TextIO::readFEN(TextIO::startPosFEN);
    Position origPos(pos);
    ASSERT(pos.equals(origPos));
    Move move(Position::getSquare(4,1), Position::getSquare(4,3), Piece::EMPTY);
    UndoInfo ui;
    pos.makeMove(move, ui);
    ASSERT_EQUAL(pos.isWhiteMove(), false);
    ASSERT_EQUAL(-1, pos.getEpSquare());
    ASSERT_EQUAL(Piece::EMPTY, pos.getPiece(Position::getSquare(4,1)));
    ASSERT_EQUAL(Piece::WPAWN, pos.getPiece(Position::getSquare(4,3)));
    ASSERT(!pos.equals(origPos));
    int castleMask = (1 << Position::A1_CASTLE) |
                     (1 << Position::H1_CASTLE) |
                     (1 << Position::A8_CASTLE) |
                     (1 << Position::H8_CASTLE);
    ASSERT_EQUAL(castleMask,pos.getCastleMask());
    pos.unMakeMove(move, ui);
    ASSERT_EQUAL(pos.isWhiteMove(), true);
    ASSERT_EQUAL(Piece::WPAWN, pos.getPiece(Position::getSquare(4,1)));
    ASSERT_EQUAL(Piece::EMPTY, pos.getPiece(Position::getSquare(4,3)));
    ASSERT(pos.equals(origPos));

    std::string fen = "r1bqk2r/2ppbppp/p1n2n2/1pP1p3/B3P3/5N2/PP1P1PPP/RNBQK2R w KQkq b6 0 2";
    pos = TextIO::readFEN(fen);
    ASSERT_EQUAL(fen, TextIO::toFEN(pos));
    origPos = Position(pos);
    ASSERT_EQUAL(Position::getSquare(1,5), pos.getEpSquare());

    // Test capture
    move = Move(Position::getSquare(0, 3), Position::getSquare(1,4), Piece::EMPTY);
    pos.makeMove(move, ui);
    ASSERT_EQUAL(-1, pos.getEpSquare());
    ASSERT_EQUAL(Piece::WBISHOP, pos.getPiece(Position::getSquare(1,4)));
    ASSERT_EQUAL(Piece::EMPTY, pos.getPiece(Position::getSquare(0,3)));
    pos.unMakeMove(move, ui);
    ASSERT(pos.equals(origPos));

    // Test castling
    move = Move(Position::getSquare(4, 0), Position::getSquare(6,0), Piece::EMPTY);
    pos.makeMove(move, ui);
    ASSERT_EQUAL(Piece::WROOK, pos.getPiece(Position::getSquare(5,0)));
    ASSERT_EQUAL(Piece::EMPTY, pos.getPiece(Position::getSquare(7,0)));
    castleMask = (1 << Position::A8_CASTLE) |
                 (1 << Position::H8_CASTLE);
    ASSERT_EQUAL(castleMask,pos.getCastleMask());
    ASSERT_EQUAL(-1, pos.getEpSquare());
    pos.unMakeMove(move, ui);
    ASSERT(pos.equals(origPos));

    // Test castling rights (king move)
    move = Move(Position::getSquare(4, 0), Position::getSquare(4,1), Piece::EMPTY);
    pos.makeMove(move, ui);
    castleMask = (1 << Position::A8_CASTLE) |
                 (1 << Position::H8_CASTLE);
    ASSERT_EQUAL(castleMask,pos.getCastleMask());
    ASSERT_EQUAL(-1, pos.getEpSquare());
    pos.unMakeMove(move, ui);
    ASSERT(pos.equals(origPos));

    // Test castling rights (rook move)
    move = Move(Position::getSquare(7, 0), Position::getSquare(6,0), Piece::EMPTY);
    pos.makeMove(move, ui);
    castleMask = (1 << Position::A1_CASTLE) |
                 (1 << Position::A8_CASTLE) |
                 (1 << Position::H8_CASTLE);
    ASSERT_EQUAL(castleMask,pos.getCastleMask());
    ASSERT_EQUAL(-1, pos.getEpSquare());
    pos.unMakeMove(move, ui);
    ASSERT(pos.equals(origPos));

    // Test en passant
    move = Move(Position::getSquare(2, 4), Position::getSquare(1,5), Piece::EMPTY);
    pos.makeMove(move, ui);
    ASSERT_EQUAL(Piece::WPAWN, pos.getPiece(Position::getSquare(1,5)));
    ASSERT_EQUAL(Piece::EMPTY, pos.getPiece(Position::getSquare(2,4)));
    ASSERT_EQUAL(Piece::EMPTY, pos.getPiece(Position::getSquare(1,4)));
    pos.unMakeMove(move, ui);
    ASSERT(pos.equals(origPos));

    // Test castling rights loss when rook captured
    pos.setPiece(Position::getSquare(6,2), Piece::BKNIGHT);
    pos.setWhiteMove(false);
    Position origPos2(pos);
    move = Move(Position::getSquare(6,2), Position::getSquare(7,0), Piece::EMPTY);
    pos.makeMove(move, ui);
    castleMask = (1 << Position::A1_CASTLE) |
                 (1 << Position::A8_CASTLE) |
                 (1 << Position::H8_CASTLE);
    ASSERT_EQUAL(castleMask,pos.getCastleMask());
    ASSERT_EQUAL(-1, pos.getEpSquare());
    pos.unMakeMove(move, ui);
    ASSERT(pos.equals(origPos2));
}

static void
testCastleMask() {
    Position pos = TextIO::readFEN("rnbqk1nr/pppp1ppp/8/4p3/4P3/2N2N2/PPPP1bPP/R1BQKB1R w KQkq - 0 1");
    UndoInfo ui;
    Move m = TextIO::stringToMove(pos, "Kxf2");
    pos.makeMove(m, ui);
    int castleMask = (1 << Position::A8_CASTLE) |
                     (1 << Position::H8_CASTLE);
    ASSERT_EQUAL(castleMask, pos.getCastleMask());
}

/**
 * Test of makeMove method, of class Position.
 */
static void
testPromotion() {
    std::string fen = "r1bqk2r/1Pppbppp/p1n2n2/2P1p3/B3P3/5N2/Pp1P1PPP/R1BQK2R w KQkq - 0 1";
    Position pos = TextIO::readFEN(fen);
    ASSERT_EQUAL(fen, TextIO::toFEN(pos));
    Position origPos(pos);
    ASSERT(origPos.equals(pos));

    Move move(Position::getSquare(1, 6), Position::getSquare(0,7), Piece::WQUEEN);
    UndoInfo ui;
    pos.makeMove(move, ui);
    ASSERT_EQUAL(Piece::EMPTY, pos.getPiece(Position::getSquare(1,6)));
    ASSERT_EQUAL(Piece::WQUEEN, pos.getPiece(Position::getSquare(0,7)));
    pos.unMakeMove(move, ui);
    ASSERT(origPos.equals(pos));

    move = Move(Position::getSquare(1, 6), Position::getSquare(1,7), Piece::WKNIGHT);
    pos.makeMove(move, ui);
    ASSERT_EQUAL(Piece::EMPTY, pos.getPiece(Position::getSquare(1,6)));
    ASSERT_EQUAL(Piece::WKNIGHT, pos.getPiece(Position::getSquare(1,7)));
    pos.unMakeMove(move, ui);
    ASSERT(origPos.equals(pos));

    pos.setWhiteMove(false);
    origPos = pos;

    move = Move(Position::getSquare(1, 1), Position::getSquare(2, 0), Piece::BROOK);
    pos.makeMove(move, ui);
    ASSERT_EQUAL(Piece::EMPTY, pos.getPiece(Position::getSquare(1,1)));
    ASSERT_EQUAL(Piece::BROOK, pos.getPiece(Position::getSquare(2,0)));
    pos.unMakeMove(move, ui);
    ASSERT(origPos.equals(pos));
}

/**
 * Test move counters, of class Position.
 */
static void
testMoveCounters()  {
    std::string fen = "r1bqk2r/2ppbppp/p1n2n2/1pP1p3/B3P3/5N2/PP1P1PPP/RNBQK2R w KQkq b6 0 7";
    Position pos = TextIO::readFEN(fen);

    Move move = TextIO::stringToMove(pos, "Nc3");
    UndoInfo ui;
    pos.makeMove(move, ui);
    ASSERT_EQUAL(1, pos.getHalfMoveClock());
    ASSERT_EQUAL(7, pos.getFullMoveCounter());
    pos.unMakeMove(move, ui);

    move = TextIO::stringToMove(pos, "O-O");
    pos.makeMove(move, ui);
    ASSERT_EQUAL(1, pos.getHalfMoveClock());     // Castling does not reset 50 move counter
    ASSERT_EQUAL(7, pos.getFullMoveCounter());
    pos.unMakeMove(move, ui);

    move = TextIO::stringToMove(pos, "a3");
    pos.makeMove(move, ui);
    ASSERT_EQUAL(0, pos.getHalfMoveClock());     // Pawn move resets 50 move counter
    ASSERT_EQUAL(7, pos.getFullMoveCounter());
    pos.unMakeMove(move, ui);

    move = TextIO::stringToMove(pos, "Nxe5");
    pos.makeMove(move, ui);
    ASSERT_EQUAL(0, pos.getHalfMoveClock());     // Capture move resets 50 move counter
    ASSERT_EQUAL(7, pos.getFullMoveCounter());
    pos.unMakeMove(move, ui);

    move = TextIO::stringToMove(pos, "cxb6");
    pos.makeMove(move, ui);
    ASSERT_EQUAL(0, pos.getHalfMoveClock());     // EP capture move resets 50 move counter
    ASSERT_EQUAL(7, pos.getFullMoveCounter());
    pos.unMakeMove(move, ui);

    move = TextIO::stringToMove(pos, "Kf1");
    pos.makeMove(move, ui);
    ASSERT_EQUAL(1, pos.getHalfMoveClock());     // Loss of castling rights does not reset 50 move counter
    ASSERT_EQUAL(7, pos.getFullMoveCounter());
    pos.unMakeMove(move, ui);

    Move firstMove = TextIO::stringToMove(pos, "Nc3");
    UndoInfo firstUi;
    pos.makeMove(move, firstUi);
    move = TextIO::stringToMove(pos, "O-O");
    pos.makeMove(move, ui);
    ASSERT_EQUAL(2, pos.getHalfMoveClock());
    ASSERT_EQUAL(8, pos.getFullMoveCounter());   // Black move increases fullMoveCounter
    pos.unMakeMove(move, ui);
    pos.unMakeMove(firstMove, firstUi);

    fen = "8/8/8/4k3/8/8/2p5/5K2 b - - 47 68";
    pos = TextIO::readFEN(fen);
    move = TextIO::stringToMove(pos, "c1Q");
    ASSERT(!move.isEmpty());
    pos.makeMove(move, ui);
    ASSERT_EQUAL(0, pos.getHalfMoveClock());     // Pawn promotion resets 50 move counter
    ASSERT_EQUAL(69, pos.getFullMoveCounter());
}

/**
 * Test of drawRuleEquals, of class Position.
 */
static void
testDrawRuleEquals() {
    Position pos = TextIO::readFEN(TextIO::startPosFEN);
    Position origPos(pos);
    UndoInfo ui;
    pos.makeMove(TextIO::stringToMove(pos, "Nf3"), ui);
    ASSERT_EQUAL(false, pos.drawRuleEquals(origPos));
    pos.makeMove(TextIO::stringToMove(pos, "Nf6"), ui);
    ASSERT_EQUAL(false, pos.drawRuleEquals(origPos));
    pos.makeMove(TextIO::stringToMove(pos, "Ng1"), ui);
    ASSERT_EQUAL(false, pos.drawRuleEquals(origPos));
    pos.makeMove(TextIO::stringToMove(pos, "Ng8"), ui);
    ASSERT_EQUAL(true, pos.drawRuleEquals(origPos));
    ASSERT_EQUAL(false, pos.equals(origPos));       // Move counters have changed

    std::string fen = "r1bqkb1r/pppp1ppp/2n2n2/1B2p3/4P3/5N2/PPPP1PPP/RNBQK2R w KQkq - 0 1";
    pos = TextIO::readFEN(fen);
    origPos = pos;
    pos.makeMove(TextIO::stringToMove(pos, "Ke2"), ui);
    ASSERT_EQUAL(false, pos.drawRuleEquals(origPos));
    pos.makeMove(TextIO::stringToMove(pos, "Be7"), ui);
    ASSERT_EQUAL(false, pos.drawRuleEquals(origPos));
    pos.makeMove(TextIO::stringToMove(pos, "Ke1"), ui);
    ASSERT_EQUAL(false, pos.drawRuleEquals(origPos));
    pos.makeMove(TextIO::stringToMove(pos, "Bf8"), ui);
    ASSERT_EQUAL(false, pos.drawRuleEquals(origPos));   // Not equal, castling rights lost

    pos = TextIO::readFEN(TextIO::startPosFEN);
    pos.makeMove(TextIO::stringToMove(pos, "c4"), ui);
    pos.makeMove(TextIO::stringToMove(pos, "a6"), ui);
    pos.makeMove(TextIO::stringToMove(pos, "c5"), ui);
    pos.makeMove(TextIO::stringToMove(pos, "b5"), ui);
    ASSERT_EQUAL(Position::getSquare(1, 5), pos.getEpSquare());
    origPos = pos;
    pos.makeMove(TextIO::stringToMove(pos, "Nc3"), ui);
    pos.makeMove(TextIO::stringToMove(pos, "Nc6"), ui);
    pos.makeMove(TextIO::stringToMove(pos, "Nb1"), ui);
    pos.makeMove(TextIO::stringToMove(pos, "Nb8"), ui);
    ASSERT_EQUAL(false, pos.drawRuleEquals(origPos));   // Not equal, en passant rights lost
}

/**
 * Test of hashCode method, of class Position.
 */
static void
testHashCode() {
    Position pos = TextIO::readFEN(TextIO::startPosFEN);
    U64 h1 = pos.zobristHash();
    ASSERT_EQUAL(h1, pos.computeZobristHash());
    ASSERT_EQUAL(pos.materialId(), PositionTest::computeMaterialId(pos));
    UndoInfo ui;
    Move move = TextIO::stringToMove(pos, "e4");
    pos.makeMove(move, ui);
    ASSERT(h1 != pos.zobristHash());
    pos.unMakeMove(move, ui);
    ASSERT(h1 == pos.zobristHash());

    pos.setWhiteMove(!pos.isWhiteMove());
    U64 h4 = pos.zobristHash();
    ASSERT_EQUAL(h4, pos.computeZobristHash());
    ASSERT_EQUAL(pos.materialId(), PositionTest::computeMaterialId(pos));
    ASSERT(h1 != pos.zobristHash());
    pos.setWhiteMove(!pos.isWhiteMove());
    ASSERT(h1 == pos.zobristHash());

    pos.setCastleMask(0);
    ASSERT(h1 != pos.zobristHash());

    pos = TextIO::readFEN("rnbqkbnr/pppp1ppp/8/2P1p3/8/8/PP1PPPPP/RNBQKBNR b KQkq - 0 1");
    h1 = pos.zobristHash();
    ASSERT_EQUAL(h1, pos.computeZobristHash());
    ASSERT_EQUAL(pos.materialId(), PositionTest::computeMaterialId(pos));

    std::string moves[] = {
        "b5", "Nc3", "Nf6", "Nb1", "Ng8", "Nc3", "Nf6", "Nb1", "Ng8", "Nc3", "d5",
        "cxd6", "Qxd6", "h4", "Be6", "h5", "Nc6", "h6", "o-o-o", "hxg7", "Nf6", "gxh8Q", "Be7"
    };
    std::vector<UndoInfo> uiList;
    std::vector<U64> hashList;
    std::vector<Move> moveList;
    for (size_t i = 0; i < COUNT_OF(moves); i++) {
        uiList.push_back(UndoInfo());
        Move m = TextIO::stringToMove(pos, moves[i]);
        moveList.push_back(m);
        pos.makeMove(m, uiList[i]);
        U64 h = pos.zobristHash();
        ASSERT_EQUAL(h, pos.computeZobristHash());
        ASSERT_EQUAL(pos.materialId(), PositionTest::computeMaterialId(pos));
        hashList.push_back(h);
    }
    ASSERT(hashList[0] != hashList[4]);
    ASSERT(hashList[4] == hashList[8]);
    for (int i = (int)COUNT_OF(moves) - 1; i >= 0; i--) {
        pos.unMakeMove(moveList[i], uiList[i]);
        U64 h = pos.zobristHash();
        ASSERT_EQUAL(h, pos.computeZobristHash());
        ASSERT_EQUAL(h, i > 0 ? hashList[i - 1] : h1);
        ASSERT_EQUAL(pos.materialId(), PositionTest::computeMaterialId(pos));
    }
}

/**
 * Test of getKingSq method, of class Position.
 */
static void
testGetKingSq() {
    Position pos = TextIO::readFEN(TextIO::startPosFEN);
    ASSERT_EQUAL(TextIO::getSquare("e1"), pos.getKingSq(true));
    ASSERT_EQUAL(TextIO::getSquare("e8"), pos.getKingSq(false));
    pos = TextIO::readFEN("r1bq1bnr/ppppkppp/2n5/1B2p3/4P3/5N2/PPPP1PPP/RNBQK2R w KQ - 0 4");
    ASSERT_EQUAL(TextIO::getSquare("e1"), pos.getKingSq(true));
    ASSERT_EQUAL(TextIO::getSquare("e7"), pos.getKingSq(false));
    UndoInfo ui;
    pos.makeMove(TextIO::stringToMove(pos, "o-o"), ui);
    ASSERT_EQUAL(TextIO::getSquare("g1"), pos.getKingSq(true));
    ASSERT_EQUAL(TextIO::getSquare("e7"), pos.getKingSq(false));
    pos.makeMove(TextIO::stringToMove(pos, "Kd6"), ui);
    ASSERT_EQUAL(TextIO::getSquare("g1"), pos.getKingSq(true));
    ASSERT_EQUAL(TextIO::getSquare("d6"), pos.getKingSq(false));
}

struct Mtrl {
    int p, r, n, b, q;
    Mtrl(int p0, int r0, int n0, int b0, int q0) :
        p(p0), r(r0), n(n0), b(b0), q(q0) {}
};


/** Tests if a series of integers are unique. */
class UniqCheck {
public:
    UniqCheck(int nEntries) {
        int hSize = 1;
        while (hSize < nEntries*2)
            hSize *= 2;
        table.resize(hSize, -1);
    }

    bool uniq(int value) {
        ASSERT(value != -1);
        const int N = table.size();
        int h1 = (value) & (N - 1);
        int h2 = (((value >> 14) + (value << 14)) * 2 + 1) & (N - 1);
        int idx = h1;
        while (table[idx] != -1) {
            if (table[idx] == value)
                return false;
            idx = (idx + h2) & (N - 1);
        }
        table[idx] = value;
        return true;
    }

private:
    std::vector<int> table;
};

/**
 * Test that the material identifier is unique for all legal material configurations.
 */
static void
testMaterialId() {
    std::vector<Mtrl> configs;
    for (int p = 0; p <= 8; p++) {
        int maxP1 = 8;
        for (int r = 0; r <= 10; r++) {
            int maxP2 = maxP1 - std::max(0, r - 2);
            if (p > maxP2)
                continue;
            for (int n = 0; n <= 10; n++) {
                int maxP3 = maxP2 - std::max(0, n - 2);
                if (p > maxP3)
                    continue;
                for (int b = 0; b <= 10; b++) {
                    int maxP4 = maxP3 - std::max(0, b - 2);
                    if (p > maxP4)
                        continue;
                    for (int q = 0; q <= 9; q++) {
                        int maxP5 = maxP4 - std::max(0, q - 1);
                        if (p > maxP5)
                            continue;
                        configs.push_back(Mtrl(p,r,n,b,q));
                    }
                }
            }
        }
    }

    {
        std::set<int> ids;
        for (const Mtrl& w : configs) {
            MatId id;
            for (int i = 0; i < w.p; i++) id.addPiece(Piece::WPAWN);
            for (int i = 0; i < w.r; i++) id.addPiece(Piece::WROOK);
            for (int i = 0; i < w.n; i++) id.addPiece(Piece::WKNIGHT);
            for (int i = 0; i < w.b; i++) id.addPiece(Piece::WBISHOP);
            for (int i = 0; i < w.q; i++) id.addPiece(Piece::WQUEEN);
            auto res = ids.insert(id());
            ASSERT_EQUAL(res.second, true);
        }
    }

    {
        std::set<int> ids;
        for (const Mtrl& b : configs) {
            MatId id;
            for (int i = 0; i < b.p; i++) id.addPiece(Piece::BPAWN);
            for (int i = 0; i < b.r; i++) id.addPiece(Piece::BROOK);
            for (int i = 0; i < b.n; i++) id.addPiece(Piece::BKNIGHT);
            for (int i = 0; i < b.b; i++) id.addPiece(Piece::BBISHOP);
            for (int i = 0; i < b.q; i++) id.addPiece(Piece::BQUEEN);
            auto res = ids.insert(id());
            ASSERT_EQUAL(res.second, true);
        }
    }

    {
        S64 t0 = currentTimeMillis();
        UniqCheck ids(configs.size()*configs.size());
        for (const Mtrl& w : configs) {
            MatId id;
            for (int i = 0; i < w.p; i++) id.addPiece(Piece::WPAWN);
            for (int i = 0; i < w.r; i++) id.addPiece(Piece::WROOK);
            for (int i = 0; i < w.n; i++) id.addPiece(Piece::WKNIGHT);
            for (int i = 0; i < w.b; i++) id.addPiece(Piece::WBISHOP);
            for (int i = 0; i < w.q; i++) id.addPiece(Piece::WQUEEN);
            for (const Mtrl& b : configs) {
                MatId id2(id);
                id2.addPieceCnt(Piece::BPAWN, b.p);
                id2.addPieceCnt(Piece::BROOK, b.r);
                id2.addPieceCnt(Piece::BKNIGHT, b.n);
                id2.addPieceCnt(Piece::BBISHOP, b.b);
                id2.addPieceCnt(Piece::BQUEEN, b.q);
                bool u = ids.uniq(id2());
                if (!u) {
                    std::cout << "w:" << w.p << ' ' << w.r << ' ' << w.n << ' ' << w.b << ' ' << w.q << std::endl;
                    std::cout << "b:" << b.p << ' ' << b.r << ' ' << b.n << ' ' << b.b << ' ' << b.q << std::endl;
                }
                ASSERT(u);
            }
        }
        S64 t1 = currentTimeMillis();
        std::cout << "time:" << t1 - t0 << std::endl;
    }
}

static void
testSerialize() {
    Position pos = TextIO::readFEN("rnbqkb1r/3ppp1p/p4np1/1PpP4/8/5N2/PP2PPPP/RNBQKB1R w KQkq - 0 1");
    Position pos2;
    Position::SerializeData data;
    pos.serialize(data);
    pos2.deSerialize(data);
    ASSERT(pos.equals(pos2));
    ASSERT_EQUAL(pos.wMtrl(), pos2.wMtrl());
    ASSERT_EQUAL(pos.bMtrl(), pos2.bMtrl());
    ASSERT_EQUAL(pos.wMtrlPawns(), pos2.wMtrlPawns());
    ASSERT_EQUAL(pos.bMtrlPawns(), pos2.bMtrlPawns());
}

cute::suite
PositionTest::getSuite() const {
    cute::suite s;
    s.push_back(CUTE(testGetPiece));
    s.push_back(CUTE(testGetIndex));
    s.push_back(CUTE(testSetPiece));
    s.push_back(CUTE(testMakeMove));
    s.push_back(CUTE(testCastleMask));
    s.push_back(CUTE(testPromotion));
    s.push_back(CUTE(testMoveCounters));
    s.push_back(CUTE(testDrawRuleEquals));
    s.push_back(CUTE(testHashCode));
    s.push_back(CUTE(testGetKingSq));
    s.push_back(CUTE(testMaterialId));
    s.push_back(CUTE(testSerialize));
    return s;
}
