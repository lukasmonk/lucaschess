/*
 * Demolito, a UCI chess engine.
 * Copyright 2015 lucasart.
 *
 * Demolito is free software: you can redistribute it and/or modify it under the terms of the GNU
 * General Public License as published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 *
 * Demolito is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without
 * even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with this program. If
 * not, see <http://www.gnu.org/licenses/>.
*/
#include <iostream>
#include "bitboard.h"

namespace {

const int PDir[2][2] = {{1,-1},{1,1}};
const int NDir[8][2] = {{-2,-1},{-2,1},{-1,-2},{-1,2},{1,-2},{1,2},{2,-1},{2,1}};
const int KDir[8][2] = {{-1,-1},{-1,0},{-1,1},{0,-1},{0,1},{1,-1},{1,0},{1,1}};

bitboard_t PAttacks[NB_COLOR][NB_SQUARE];
bitboard_t NAttacks[NB_SQUARE];
bitboard_t KAttacks[NB_SQUARE];

bitboard_t BPseudoAttacks[NB_SQUARE];
bitboard_t RPseudoAttacks[NB_SQUARE];

bitboard_t Segment[NB_SQUARE][NB_SQUARE];
bitboard_t Ray[NB_SQUARE][NB_SQUARE];

bitboard_t PawnSpan[NB_COLOR][NB_SQUARE];
bitboard_t PawnPath[NB_COLOR][NB_SQUARE];
bitboard_t AdjacentFiles[NB_FILE];
int KingDistance[NB_SQUARE][NB_SQUARE];

void safe_set_bit(bitboard_t& b, Rank r, File f)
{
    if (0 <= r && r < NB_RANK && 0 <= f && f < NB_FILE)
        bb::set(b, square(r, f));
}

void init_leaper_attacks()
{
    for (Square s = A1; s <= H8; ++s) {
        const Rank r = rank_of(s);
        const File f = file_of(s);

        for (int d = 0; d < 8; d++) {
            safe_set_bit(NAttacks[s], r + NDir[d][0], f + NDir[d][1]);
            safe_set_bit(KAttacks[s], r + KDir[d][0], f + KDir[d][1]);
        }

        for (int d = 0; d < 2; d++) {
            safe_set_bit(PAttacks[WHITE][s], r + PDir[d][0], f + PDir[d][1]);
            safe_set_bit(PAttacks[BLACK][s], r - PDir[d][0], f - PDir[d][1]);
        }
    }
}

void init_eval()
{
    for (Square s = H8; s >= A1; --s) {
        if (rank_of(s) == RANK_8)
            PawnSpan[WHITE][s] = PawnPath[WHITE][s] = 0;
        else {
            PawnSpan[WHITE][s] = PAttacks[WHITE][s] | PawnSpan[WHITE][s + UP];
            PawnPath[WHITE][s] = (1ULL << (s + UP)) | PawnPath[WHITE][s + UP];
        }
    }

    for (Square s = A1; s <= H8; ++s) {
        if (rank_of(s) == RANK_1)
            PawnSpan[BLACK][s] = PawnPath[BLACK][s] = 0;
        else {
            PawnSpan[BLACK][s] = PAttacks[BLACK][s] | PawnSpan[BLACK][s + DOWN];
            PawnPath[BLACK][s] = (1ULL << (s + DOWN)) | PawnPath[BLACK][s + DOWN];
        }
    }

    for (File f = FILE_A; f <= FILE_H; ++f)
        AdjacentFiles[f] = (f > FILE_A ? bb::file(f - 1) : 0)
                           | (f < FILE_H ? bb::file(f + 1) : 0);

    for (Square s1 = A1; s1 <= H8; ++s1)
        for (Square s2 = A1; s2 <= H8; ++s2)
            KingDistance[s1][s2] = std::max(std::abs(rank_of(s1) - rank_of(s2)),
                                            std::abs(file_of(s1) - file_of(s2)));
}

void init_rays()
{
    for (Square s1 = A1; s1 <= H8; ++s1) {
        const Rank r1 = rank_of(s1);
        const File f1 = file_of(s1);

        for (int d = 0; d < 8; d++) {
            bitboard_t mask = 0;
            Rank r2 = r1;
            File f2 = f1;

            while (0 <= r2 && r2 < NB_RANK && 0 <= f2 && f2 < NB_FILE) {
                const Square s2 = square(r2, f2);
                bb::set(mask, s2);
                Segment[s1][s2] = mask;
                r2 += KDir[d][0], f2 += KDir[d][1];
            }

            bitboard_t sqs = mask;

            while (sqs) {
                Square s2 = bb::pop_lsb(sqs);
                Ray[s1][s2] = mask;
            }
        }
    }
}

void init_slider_pseudo_attacks()
{
    for (Square s = A1; s <= H8; ++s) {
        BPseudoAttacks[s] = bb::battacks(s, 0);
        RPseudoAttacks[s] = bb::rattacks(s, 0);
    }
}

}    // namespace

namespace bb {

void init_slider_attacks();    // in magic.cc

void init()
{
    init_rays();
    init_leaper_attacks();
    init_slider_attacks();
    init_slider_pseudo_attacks();
    init_eval();
}

/* Bitboard Accessors */

bitboard_t rank(Rank r)
{
    BOUNDS(r, NB_RANK);

    return 0xFFULL << (8 * r);
}

bitboard_t file(File f)
{
    BOUNDS(f, NB_FILE);

    return 0x0101010101010101ULL << f;
}

bitboard_t pattacks(Color c, Square s)
{
    BOUNDS(s, NB_SQUARE);

    return PAttacks[c][s];
}

bitboard_t nattacks(Square s)
{
    BOUNDS(s, NB_SQUARE);

    return NAttacks[s];
}

bitboard_t kattacks(Square s)
{
    BOUNDS(s, NB_SQUARE);

    return KAttacks[s];
}

bitboard_t bpattacks(Square s)
{
    BOUNDS(s, NB_SQUARE);

    return BPseudoAttacks[s];
}

bitboard_t rpattacks(Square s)
{
    BOUNDS(s, NB_SQUARE);

    return RPseudoAttacks[s];
}

bitboard_t segment(Square s1, Square s2)
{
    BOUNDS(s1, NB_SQUARE);
    BOUNDS(s2, NB_SQUARE);

    return Segment[s1][s2];
}

bitboard_t ray(Square s1, Square s2)
{
    BOUNDS(s1, NB_SQUARE);
    BOUNDS(s2, NB_SQUARE);
    assert(s1 != s2);    // Ray[s][s] is undefined

    return Ray[s1][s2];
}

bitboard_t pawn_span(Color c, Square s)
{
    BOUNDS(c, NB_COLOR);
    BOUNDS(s, NB_SQUARE);

    return PawnSpan[c][s];
}

bitboard_t pawn_path(Color c, Square s)
{
    BOUNDS(c, NB_COLOR);
    BOUNDS(s, NB_SQUARE);

    return PawnPath[c][s];
}

bitboard_t adjacent_files(File f)
{
    BOUNDS(f, NB_FILE);

    return AdjacentFiles[f];
}

int king_distance(Square s1, Square s2)
{
    BOUNDS(s1, NB_SQUARE);
    BOUNDS(s2, NB_SQUARE);

    return KingDistance[s1][s2];
}

/* Bit manipulation */

bool test(bitboard_t b, Square s)
{
    BOUNDS(s, NB_SQUARE);

    return b & (1ULL << s);
}

void clear(bitboard_t& b, Square s)
{
    BOUNDS(s, NB_SQUARE);
    assert(test(b, s));

    b ^= 1ULL << s;
}

void set(bitboard_t& b, Square s)
{
    BOUNDS(s, NB_SQUARE);
    assert(!test(b, s));

    b ^= 1ULL << s;
}

bitboard_t shift(bitboard_t b, int i)
{
    assert(-63 <= i && i <= 63);    // forbid oversized shift (undefined behaviour)

    return i > 0 ? b << i : b >> -i;
}

Square lsb(bitboard_t b)
{
    assert(b);

    return Square(__builtin_ffsll(b) - 1);
}

Square msb(bitboard_t b)
{
    assert(b);

    return Square(63 - __builtin_clzll(b));
}

Square pop_lsb(bitboard_t& b)
{
    Square s = lsb(b);
    b &= b - 1;
    return s;
}

bool several(bitboard_t b)
{
    return b & (b - 1);
}

int count(bitboard_t b)
{
    return __builtin_popcountll(b);
}

/* Debug print */

void print(bitboard_t b)
{
    for (Rank r = RANK_8; r >= RANK_1; --r) {
        char line[] = ". . . . . . . .";

        for (File f = FILE_A; f <= FILE_H; ++f) {
            if (test(b, square(r, f)))
                line[2 * f] = 'X';
        }

        std::cout << line << '\n';
    }

    std::cout << std::endl;
}

}    // namespace bb
