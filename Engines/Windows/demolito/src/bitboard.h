#pragma once
#include "types.h"

typedef uint64_t bitboard_t;

namespace bb {

void init();

/* Bitboard Accessors */

bitboard_t rank(Rank r);
bitboard_t file(File f);

// Leaper attacks
bitboard_t pattacks(Color c, Square s);
bitboard_t nattacks(Square s);
bitboard_t kattacks(Square s);

// Slider attacks
bitboard_t battacks(Square s, bitboard_t occ);
bitboard_t rattacks(Square s, bitboard_t occ);
bitboard_t bpattacks(Square s);    // pseudo-attacks (empty board)
bitboard_t rpattacks(Square s);    // pseudo-attacks (empty board)

bitboard_t segment(Square s1, Square s2);
bitboard_t ray(Square s1, Square s2);

// Precalculated arrays for evaluation
bitboard_t pawn_span(Color c, Square s);
bitboard_t pawn_path(Color c, Square s);
bitboard_t adjacent_files(File f);
int king_distance(Square s1, Square s2);

/* Bit manipulation */

bool test(bitboard_t b, Square s);
void clear(bitboard_t& b, Square s);
void set(bitboard_t& b, Square s);
bitboard_t shift(bitboard_t b, int i);

Square lsb(bitboard_t b);
Square msb(bitboard_t b);
Square pop_lsb(bitboard_t& b);

bool several(bitboard_t b);
int count(bitboard_t b);

/* Debug */

void print(bitboard_t b);

}    // namespace bb
