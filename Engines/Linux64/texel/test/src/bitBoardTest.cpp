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
 * bitBoardTest.cpp
 *
 *  Created on: Feb 25, 2012
 *      Author: petero
 */

#include "bitBoardTest.hpp"
#include "bitBoard.hpp"
#include "textio.hpp"

#include <algorithm>

#include "cute.h"


/** Test of kingAttacks, of class BitBoard. */
void
BitBoardTest::testKingAttacks() {
    ASSERT_EQUAL(5, BitBoard::bitCount(BitBoard::kingAttacks[TextIO::getSquare("g1")]));
    ASSERT_EQUAL(3, BitBoard::bitCount(BitBoard::kingAttacks[TextIO::getSquare("h1")]));
    ASSERT_EQUAL(3, BitBoard::bitCount(BitBoard::kingAttacks[TextIO::getSquare("a1")]));
    ASSERT_EQUAL(5, BitBoard::bitCount(BitBoard::kingAttacks[TextIO::getSquare("a2")]));
    ASSERT_EQUAL(3, BitBoard::bitCount(BitBoard::kingAttacks[TextIO::getSquare("h8")]));
    ASSERT_EQUAL(5, BitBoard::bitCount(BitBoard::kingAttacks[TextIO::getSquare("a6")]));
    ASSERT_EQUAL(8, BitBoard::bitCount(BitBoard::kingAttacks[TextIO::getSquare("b2")]));
}

/** Test of knightAttacks, of class BitBoard. */
void
BitBoardTest::testKnightAttacks() {
    ASSERT_EQUAL(3, BitBoard::bitCount(BitBoard::knightAttacks[TextIO::getSquare("g1")]));
    ASSERT_EQUAL(2, BitBoard::bitCount(BitBoard::knightAttacks[TextIO::getSquare("a1")]));
    ASSERT_EQUAL(2, BitBoard::bitCount(BitBoard::knightAttacks[TextIO::getSquare("h1")]));
    ASSERT_EQUAL(4, BitBoard::bitCount(BitBoard::knightAttacks[TextIO::getSquare("h6")]));
    ASSERT_EQUAL(4, BitBoard::bitCount(BitBoard::knightAttacks[TextIO::getSquare("b7")]));
    ASSERT_EQUAL(8, BitBoard::bitCount(BitBoard::knightAttacks[TextIO::getSquare("c6")]));
    ASSERT_EQUAL((1ULL<<TextIO::getSquare("e2")) |
                 (1ULL<<TextIO::getSquare("f3")) |
                 (1ULL<<TextIO::getSquare("h3")),
                 BitBoard::knightAttacks[TextIO::getSquare("g1")]);
}

void
BitBoardTest::testPawnAttacks() {
    for (int sq = 0; sq < 64; sq++) {
        int x = Position::getX(sq);
        int y = Position::getY(sq);
        U64 atk = BitBoard::wPawnAttacksMask(1ULL << sq);
        U64 expected = 0;
        if (y < 7) {
            if (x > 0)
                expected |= 1ULL << (sq + 7);
            if (x < 7)
                expected |= 1ULL << (sq + 9);
        }
        ASSERT_EQUAL(expected, atk);

        atk = BitBoard::bPawnAttacksMask(1ULL << sq);
        expected = 0;
        if (y > 0) {
            if (x > 0)
                expected |= 1ULL << (sq - 9);
            if (x < 7)
                expected |= 1ULL << (sq - 7);
        }
        ASSERT_EQUAL(expected, atk);
    }

    ASSERT_EQUAL(BitBoard::sqMask(A5,B5,C5),
                 BitBoard::wPawnAttacksMask(BitBoard::sqMask(A4,B4)));
    ASSERT_EQUAL(BitBoard::sqMask(A6,C6,E6),
                 BitBoard::wPawnAttacksMask(BitBoard::sqMask(B5,D5)));

    ASSERT_EQUAL(BitBoard::sqMask(B1,G1),
                 BitBoard::bPawnAttacksMask(BitBoard::sqMask(A2,H2)));
    ASSERT_EQUAL(BitBoard::sqMask(F3,H3,F2,H2),
                 BitBoard::bPawnAttacksMask(BitBoard::sqMask(G4,G3)));
}

/** Test of squaresBetween[][], of class BitBoard. */
void
BitBoardTest::testSquaresBetween() {
    // Tests that the set of nonzero elements is correct
    for (int sq1 = 0; sq1 < 64; sq1++) {
        for (int sq2 = 0; sq2 < 64; sq2++) {
            int d = BitBoard::getDirection(sq1, sq2);
            if (d == 0) {
                ASSERT_EQUAL(0, BitBoard::squaresBetween[sq1][sq2]);
            } else {
                int dx = Position::getX(sq1) - Position::getX(sq2);
                int dy = Position::getY(sq1) - Position::getY(sq2);
                if (std::abs(dx * dy) == 2) { // Knight direction
                    ASSERT_EQUAL(0, BitBoard::squaresBetween[sq1][sq2]);
                } else {
                    if ((std::abs(dx) > 1) || (std::abs(dy) > 1)) {
                        ASSERT(BitBoard::squaresBetween[sq1][sq2] != 0);
                    } else {
                        ASSERT_EQUAL(0, BitBoard::squaresBetween[sq1][sq2]);
                    }
                }
            }
        }
    }

    ASSERT_EQUAL(0x0040201008040200ULL, BitBoard::squaresBetween[0][63]);
    ASSERT_EQUAL(0x000000001C000000ULL, BitBoard::squaresBetween[TextIO::getSquare("b4")][TextIO::getSquare("f4")]);
}

/**
 * If there is a piece type that can move from "from" to "to", return the
 * corresponding direction, 8*dy+dx.
 */
static int computeDirection(int from, int to) {
    int dx = Position::getX(to) - Position::getX(from);
    int dy = Position::getY(to) - Position::getY(from);
    if (dx == 0) {                   // Vertical rook direction
        if (dy == 0) return 0;
        return (dy > 0) ? 8 : -8;
    }
    if (dy == 0)                    // Horizontal rook direction
        return (dx > 0) ? 1 : -1;
    if (std::abs(dx) == std::abs(dy)) // Bishop direction
        return ((dy > 0) ? 8 : -8) + (dx > 0 ? 1 : -1);
    if (std::abs(dx * dy) == 2)     // Knight direction
        return dy * 8 + dx;
    return 0;
}

void
BitBoardTest::testGetDirection() {
    for (int from = 0; from < 64; from++)
        for (int to = 0; to < 64; to++)
            ASSERT_EQUAL(computeDirection(from, to), BitBoard::getDirection(from, to));
}

static int computeDistance(int from, int to, bool taxi) {
    int dx = Position::getX(to) - Position::getX(from);
    int dy = Position::getY(to) - Position::getY(from);
    if (taxi)
        return std::abs(dx) + std::abs(dy);
    else
        return std::max(std::abs(dx), std::abs(dy));
}

void
BitBoardTest::testGetDistance() {
    for (int from = 0; from < 64; from++) {
        for (int to = 0; to < 64; to++) {
            ASSERT_EQUAL(computeDistance(from, to, false), BitBoard::getKingDistance(from, to));
            ASSERT_EQUAL(computeDistance(from, to, true ), BitBoard::getTaxiDistance(from, to));
        }
    }
}

void
BitBoardTest::testTrailingZeros() {
    for (int i = 0; i < 64; i++) {
        U64 mask = 1ULL << i;
        ASSERT_EQUAL(i, BitBoard::firstSquare(mask));
        U64 mask2 = mask;
        ASSERT_EQUAL(i, BitBoard::extractSquare(mask2));
        ASSERT_EQUAL(0, mask2);
    }
    U64 mask = 0xffffffffffffffffULL;
    int cnt = 0;
    while (mask) {
        ASSERT_EQUAL(cnt, BitBoard::extractSquare(mask));
        cnt++;
    }
    ASSERT_EQUAL(64, cnt);
}

void
BitBoardTest::testMaskAndMirror() {
    ASSERT_EQUAL(BitBoard::sqMask(A1,H1,A8,H8), BitBoard::maskCorners);
    ASSERT_EQUAL(BitBoard::sqMask(A1,B1,C1,D1,E1,F1,G1,H1), BitBoard::maskRow1);
    ASSERT_EQUAL(BitBoard::sqMask(A2,B2,C2,D2,E2,F2,G2,H2), BitBoard::maskRow2);
    ASSERT_EQUAL(BitBoard::sqMask(A3,B3,C3,D3,E3,F3,G3,H3), BitBoard::maskRow3);
    ASSERT_EQUAL(BitBoard::sqMask(A4,B4,C4,D4,E4,F4,G4,H4), BitBoard::maskRow4);
    ASSERT_EQUAL(BitBoard::sqMask(A5,B5,C5,D5,E5,F5,G5,H5), BitBoard::maskRow5);
    ASSERT_EQUAL(BitBoard::sqMask(A6,B6,C6,D6,E6,F6,G6,H6), BitBoard::maskRow6);
    ASSERT_EQUAL(BitBoard::sqMask(A7,B7,C7,D7,E7,F7,G7,H7), BitBoard::maskRow7);
    ASSERT_EQUAL(BitBoard::sqMask(A8,B8,C8,D8,E8,F8,G8,H8), BitBoard::maskRow8);

    ASSERT_EQUAL(BitBoard::sqMask(A1,B1,C1,D1,E1,F1,G1,H1,
                                  A8,B8,C8,D8,E8,F8,G8,H8), BitBoard::maskRow1Row8);
    ASSERT_EQUAL(BitBoard::mirrorX(BitBoard::maskRow1Row8), BitBoard::maskRow1Row8);
    ASSERT_EQUAL(BitBoard::mirrorY(BitBoard::maskRow1Row8), BitBoard::maskRow1Row8);

    ASSERT_EQUAL(BitBoard::mirrorX(BitBoard::maskDarkSq), BitBoard::maskLightSq);
    ASSERT_EQUAL(BitBoard::mirrorY(BitBoard::maskDarkSq), BitBoard::maskLightSq);
    ASSERT_EQUAL(BitBoard::mirrorX(BitBoard::maskLightSq), BitBoard::maskDarkSq);
    ASSERT_EQUAL(BitBoard::mirrorY(BitBoard::maskLightSq), BitBoard::maskDarkSq);

    ASSERT_EQUAL(BitBoard::sqMask(A1,B1,C1), 7);
    ASSERT_EQUAL(BitBoard::sqMask(B1,C1,D1,F1,G1), 0x6E);
    ASSERT_EQUAL(BitBoard::sqMask(F1,G1), 0x60L);
    ASSERT_EQUAL(BitBoard::sqMask(B1,C1,D1), 0xEL);
    ASSERT_EQUAL(BitBoard::sqMask(G1,H1), 0xC0L);
    ASSERT_EQUAL(BitBoard::sqMask(B1,C1), 0x6L);
    ASSERT_EQUAL(BitBoard::sqMask(A1,B1), 0x3L);
    ASSERT_EQUAL(BitBoard::sqMask(F8,G8), 0x6000000000000000L);
    ASSERT_EQUAL(BitBoard::sqMask(G8,H8), 0xC000000000000000L);
    ASSERT_EQUAL(BitBoard::sqMask(B8,C8), 0x600000000000000L);
    ASSERT_EQUAL(BitBoard::sqMask(A8,B8), 0x300000000000000L);

    ASSERT_EQUAL(BitBoard::sqMask(C2,B3,F2,G3,B6,C7,G6,F7), 0x24420000422400ULL);
    ASSERT_EQUAL(BitBoard::sqMask(A8,B8,A7,B7), 0x0303000000000000ULL);

    ASSERT_EQUAL(BitBoard::sqMask(G8,H8,G7,H7), 0xC0C0000000000000ULL);
    ASSERT_EQUAL(BitBoard::sqMask(A1,B1,A2,B2), 0x0303ULL);
    ASSERT_EQUAL(BitBoard::sqMask(G1,H1,G2,H2), 0xC0C0ULL);
    ASSERT_EQUAL(BitBoard::sqMask(A8,B8,A7), 0x0301000000000000ULL);
    ASSERT_EQUAL(BitBoard::sqMask(G8,H8,H7), 0xC080000000000000ULL);
    ASSERT_EQUAL(BitBoard::sqMask(A1,B1,A2), 0x0103ULL);
    ASSERT_EQUAL(BitBoard::sqMask(G1,H1,H2), 0x80C0ULL);
    ASSERT_EQUAL(BitBoard::sqMask(A8,B8,C8,D8,D7), 0x0F08000000000000ULL);
    ASSERT_EQUAL(BitBoard::sqMask(A8,B8,A7), 0x0301000000000000ULL);
    ASSERT_EQUAL(BitBoard::sqMask(E8,F8,G8,H8,E7), 0xF010000000000000ULL);
    ASSERT_EQUAL(BitBoard::sqMask(G8,H8,H7), 0xC080000000000000ULL);
    ASSERT_EQUAL(BitBoard::sqMask(A1,B1,C1,D1,D2), 0x080FULL);
    ASSERT_EQUAL(BitBoard::sqMask(A1,B1,A2), 0x0103ULL);
    ASSERT_EQUAL(BitBoard::sqMask(E1,F1,G1,H1,E2), 0x10F0ULL);
    ASSERT_EQUAL(BitBoard::sqMask(G1,H1,H2), 0x80C0ULL);

    for (int sq = 0; sq < 64; sq++) {
        U64 m = 1ULL << sq;
        switch (Position::getX(sq)) {
        case 0:
            ASSERT((m & BitBoard::maskFileA) != 0);
            ASSERT((m & BitBoard::maskFileB) == 0);
            ASSERT((m & BitBoard::maskFileC) == 0);
            ASSERT((m & BitBoard::maskFileD) == 0);
            ASSERT((m & BitBoard::maskFileE) == 0);
            ASSERT((m & BitBoard::maskFileF) == 0);
            ASSERT((m & BitBoard::maskFileG) == 0);
            ASSERT((m & BitBoard::maskFileH) == 0);
            ASSERT((m & BitBoard::maskAToDFiles) != 0);
            ASSERT((m & BitBoard::maskEToHFiles) == 0);
            break;
        case 1:
            ASSERT((m & BitBoard::maskFileA) == 0);
            ASSERT((m & BitBoard::maskFileB) != 0);
            ASSERT((m & BitBoard::maskFileC) == 0);
            ASSERT((m & BitBoard::maskFileD) == 0);
            ASSERT((m & BitBoard::maskFileE) == 0);
            ASSERT((m & BitBoard::maskFileF) == 0);
            ASSERT((m & BitBoard::maskFileG) == 0);
            ASSERT((m & BitBoard::maskFileH) == 0);
            ASSERT((m & BitBoard::maskAToDFiles) != 0);
            ASSERT((m & BitBoard::maskEToHFiles) == 0);
            break;
        case 2:
            ASSERT((m & BitBoard::maskFileA) == 0);
            ASSERT((m & BitBoard::maskFileB) == 0);
            ASSERT((m & BitBoard::maskFileC) != 0);
            ASSERT((m & BitBoard::maskFileD) == 0);
            ASSERT((m & BitBoard::maskFileE) == 0);
            ASSERT((m & BitBoard::maskFileF) == 0);
            ASSERT((m & BitBoard::maskFileG) == 0);
            ASSERT((m & BitBoard::maskFileH) == 0);
            ASSERT((m & BitBoard::maskAToDFiles) != 0);
            ASSERT((m & BitBoard::maskEToHFiles) == 0);
            break;
        case 3:
            ASSERT((m & BitBoard::maskFileA) == 0);
            ASSERT((m & BitBoard::maskFileB) == 0);
            ASSERT((m & BitBoard::maskFileC) == 0);
            ASSERT((m & BitBoard::maskFileD) != 0);
            ASSERT((m & BitBoard::maskFileE) == 0);
            ASSERT((m & BitBoard::maskFileF) == 0);
            ASSERT((m & BitBoard::maskFileG) == 0);
            ASSERT((m & BitBoard::maskFileH) == 0);
            ASSERT((m & BitBoard::maskAToDFiles) != 0);
            ASSERT((m & BitBoard::maskEToHFiles) == 0);
            break;
        case 4:
            ASSERT((m & BitBoard::maskFileA) == 0);
            ASSERT((m & BitBoard::maskFileB) == 0);
            ASSERT((m & BitBoard::maskFileC) == 0);
            ASSERT((m & BitBoard::maskFileD) == 0);
            ASSERT((m & BitBoard::maskFileE) != 0);
            ASSERT((m & BitBoard::maskFileF) == 0);
            ASSERT((m & BitBoard::maskFileG) == 0);
            ASSERT((m & BitBoard::maskFileH) == 0);
            ASSERT((m & BitBoard::maskAToDFiles) == 0);
            ASSERT((m & BitBoard::maskEToHFiles) != 0);
            break;
        case 5:
            ASSERT((m & BitBoard::maskFileA) == 0);
            ASSERT((m & BitBoard::maskFileB) == 0);
            ASSERT((m & BitBoard::maskFileC) == 0);
            ASSERT((m & BitBoard::maskFileD) == 0);
            ASSERT((m & BitBoard::maskFileE) == 0);
            ASSERT((m & BitBoard::maskFileF) != 0);
            ASSERT((m & BitBoard::maskFileG) == 0);
            ASSERT((m & BitBoard::maskFileH) == 0);
            ASSERT((m & BitBoard::maskAToDFiles) == 0);
            ASSERT((m & BitBoard::maskEToHFiles) != 0);
            break;
        case 6:
            ASSERT((m & BitBoard::maskFileA) == 0);
            ASSERT((m & BitBoard::maskFileB) == 0);
            ASSERT((m & BitBoard::maskFileC) == 0);
            ASSERT((m & BitBoard::maskFileD) == 0);
            ASSERT((m & BitBoard::maskFileE) == 0);
            ASSERT((m & BitBoard::maskFileF) == 0);
            ASSERT((m & BitBoard::maskFileG) != 0);
            ASSERT((m & BitBoard::maskFileH) == 0);
            ASSERT((m & BitBoard::maskAToDFiles) == 0);
            ASSERT((m & BitBoard::maskEToHFiles) != 0);
            break;
        case 7:
            ASSERT((m & BitBoard::maskFileA) == 0);
            ASSERT((m & BitBoard::maskFileB) == 0);
            ASSERT((m & BitBoard::maskFileC) == 0);
            ASSERT((m & BitBoard::maskFileD) == 0);
            ASSERT((m & BitBoard::maskFileE) == 0);
            ASSERT((m & BitBoard::maskFileF) == 0);
            ASSERT((m & BitBoard::maskFileG) == 0);
            ASSERT((m & BitBoard::maskFileH) != 0);
            ASSERT((m & BitBoard::maskAToDFiles) == 0);
            ASSERT((m & BitBoard::maskEToHFiles) != 0);
            break;
        }
    }
}

cute::suite
BitBoardTest::getSuite() const {
    cute::suite s;
    s.push_back(CUTE(testKingAttacks));
    s.push_back(CUTE(testKnightAttacks));
    s.push_back(CUTE(testPawnAttacks));
    s.push_back(CUTE(testSquaresBetween));
    s.push_back(CUTE(testGetDirection));
    s.push_back(CUTE(testGetDistance));
    s.push_back(CUTE(testTrailingZeros));
    s.push_back(CUTE(testMaskAndMirror));
    return s;
}
