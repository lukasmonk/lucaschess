/***************************************************************************
 *   Copyright (C) 2009 by Borko Bošković                                  *
 *   borko.boskovic@gmail.com                                              *
 *                                                                         *
 *   This program is free software: you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation, either version 3 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>. *
 ***************************************************************************/

#ifndef TYPES_H
#define TYPES_H

#include <string>
#include <cstdlib>
#include <algorithm>
#include <inttypes.h>


typedef uint64_t Key;
typedef uint64_t Bitboard;

const Bitboard BB_1 = 1;

const Bitboard BB_A1 = BB_1;
const Bitboard BB_B1 = BB_A1<<1;
const Bitboard BB_C1 = BB_A1<<2;
const Bitboard BB_D1 = BB_A1<<3;
const Bitboard BB_E1 = BB_A1<<4;
const Bitboard BB_F1 = BB_A1<<5;
const Bitboard BB_G1 = BB_A1<<6;
const Bitboard BB_H1 = BB_A1<<7;
const Bitboard BB_A2 = BB_A1<<8;
const Bitboard BB_B2 = BB_A1<<9;
const Bitboard BB_C2 = BB_A1<<10;
const Bitboard BB_D2 = BB_A1<<11;
const Bitboard BB_E2 = BB_A1<<12;
const Bitboard BB_F2 = BB_A1<<13;
const Bitboard BB_G2 = BB_A1<<14;
const Bitboard BB_H2 = BB_A1<<15;
const Bitboard BB_A3 = BB_A1<<16;
const Bitboard BB_B3 = BB_A1<<17;
const Bitboard BB_C3 = BB_A1<<18;
const Bitboard BB_D3 = BB_A1<<19;
const Bitboard BB_E3 = BB_A1<<20;
const Bitboard BB_F3 = BB_A1<<21;
const Bitboard BB_G3 = BB_A1<<22;
const Bitboard BB_H3 = BB_A1<<23;
const Bitboard BB_A4 = BB_A1<<24;
const Bitboard BB_B4 = BB_A1<<25;
const Bitboard BB_C4 = BB_A1<<26;
const Bitboard BB_D4 = BB_A1<<27;
const Bitboard BB_E4 = BB_A1<<28;
const Bitboard BB_F4 = BB_A1<<29;
const Bitboard BB_G4 = BB_A1<<30;
const Bitboard BB_H4 = BB_A1<<31;
const Bitboard BB_A5 = BB_A1<<32;
const Bitboard BB_B5 = BB_A1<<33;
const Bitboard BB_C5 = BB_A1<<34;
const Bitboard BB_D5 = BB_A1<<35;
const Bitboard BB_E5 = BB_A1<<36;
const Bitboard BB_F5 = BB_A1<<37;
const Bitboard BB_G5 = BB_A1<<38;
const Bitboard BB_H5 = BB_A1<<39;
const Bitboard BB_A6 = BB_A1<<40;
const Bitboard BB_B6 = BB_A1<<41;
const Bitboard BB_C6 = BB_A1<<42;
const Bitboard BB_D6 = BB_A1<<43;
const Bitboard BB_E6 = BB_A1<<44;
const Bitboard BB_F6 = BB_A1<<45;
const Bitboard BB_G6 = BB_A1<<46;
const Bitboard BB_H6 = BB_A1<<47;
const Bitboard BB_A7 = BB_A1<<48;
const Bitboard BB_B7 = BB_A1<<49;
const Bitboard BB_C7 = BB_A1<<50;
const Bitboard BB_D7 = BB_A1<<51;
const Bitboard BB_E7 = BB_A1<<52;
const Bitboard BB_F7 = BB_A1<<53;
const Bitboard BB_G7 = BB_A1<<54;
const Bitboard BB_H7 = BB_A1<<55;
const Bitboard BB_A8 = BB_A1<<56;
const Bitboard BB_B8 = BB_A1<<57;
const Bitboard BB_C8 = BB_A1<<58;
const Bitboard BB_D8 = BB_A1<<59;
const Bitboard BB_E8 = BB_A1<<60;
const Bitboard BB_F8 = BB_A1<<61;
const Bitboard BB_G8 = BB_A1<<62;
const Bitboard BB_H8 = BB_A1<<63;

const Bitboard BB_FILE_A = BB_A1 | BB_A2 | BB_A3 | BB_A4 | BB_A5 | BB_A6 | BB_A7 | BB_A8;
const Bitboard BB_FILE_B = BB_FILE_A<<1;
const Bitboard BB_FILE_C = BB_FILE_A<<2;
const Bitboard BB_FILE_D = BB_FILE_A<<3;
const Bitboard BB_FILE_E = BB_FILE_A<<4;
const Bitboard BB_FILE_F = BB_FILE_A<<5;
const Bitboard BB_FILE_G = BB_FILE_A<<6;
const Bitboard BB_FILE_H = BB_FILE_A<<7;

const Bitboard BB_FILE[8] =
    {BB_FILE_A, BB_FILE_B, BB_FILE_C, BB_FILE_D, BB_FILE_E, BB_FILE_F, BB_FILE_G, BB_FILE_H};

const Bitboard BB_NOT_FILE_A = ~ BB_FILE_A;
const Bitboard BB_NOT_FILE_H = ~ BB_FILE_H;
const Bitboard BB_NOT_FILES_AB = ~(BB_FILE_A | BB_FILE_B);
const Bitboard BB_NOT_FILES_GH = ~(BB_FILE_G | BB_FILE_H);

const Bitboard BB_RANK_1 = BB_A1 | BB_B1 | BB_C1 | BB_D1 | BB_E1 | BB_F1 | BB_G1 | BB_H1;
const Bitboard BB_RANK_2 = BB_RANK_1<<8;
const Bitboard BB_RANK_3 = BB_RANK_2<<8;
const Bitboard BB_RANK_4 = BB_RANK_3<<8;
const Bitboard BB_RANK_5 = BB_RANK_4<<8;
const Bitboard BB_RANK_6 = BB_RANK_5<<8;
const Bitboard BB_RANK_7 = BB_RANK_6<<8;
const Bitboard BB_RANK_8 = BB_RANK_7<<8;

const Bitboard BB_RANK[8] =
    {BB_RANK_1, BB_RANK_2, BB_RANK_3, BB_RANK_4, BB_RANK_5, BB_RANK_6, BB_RANK_7, BB_RANK_8};

enum Piece{
    W = 0, B = 1,
    WP = 2, BP = 3,
    WN = 4, BN = 5,
    WB = 6, BB = 7,
    WR = 8, BR = 9,
    WQ = 10, BQ = 11,
    WK = 12, BK = 13,
    NO_PIECE = 14
};

enum PieceType {
    PAWN = 2, KNIGHT = 4,
    BISHOP = 6, ROOK = 8,
    QUEEN = 10, KING = 12
};

enum Square {
    A1, B1, C1, D1, E1, F1, G1, H1,
    A2, B2, C2, D2, E2, F2, G2, H2,
    A3, B3, C3, D3, E3, F3, G3, H3,
    A4, B4, C4, D4, E4, F4, G4, H4,
    A5, B5, C5, D5, E5, F5, G5, H5,
    A6, B6, C6, D6, E6, F6, G6, H6,
    A7, B7, C7, D7, E7, F7, G7, H7,
    A8, B8, C8, D8, E8, F8, G8, H8,
    NO_SQ
};

enum File {
    FILE_A, FILE_B, FILE_C, FILE_D, FILE_E, FILE_F, FILE_G, FILE_H, FILE_NONE
};

enum Rank {
    RANK_1, RANK_2, RANK_3, RANK_4, RANK_5, RANK_6, RANK_7, RANK_8, RANK_NONE
};

enum Color{
    White = 0, Black = 1
};

enum Castle{
    N0_CASTLE = 0,
    WK_CASTLE = 1, WQ_CASTLE = 2, W_CASTLE = 3,
    BK_CASTLE = 4, BQ_CASTLE = 8, B_CASTLE = 12
};

/**
  * Move:
  * 0000000000001111 - type
  * 0000001111110000 - from (0 - 63)
  * 1111110000000000 - to (0 - 63)
  */

enum Move{
    MoveNull = 0, MoveQuiet = 1, MoveCapture = 2, MoveCastle = 3,
    MovePN = 4, MovePB = 6, MovePR = 8, MovePQ = 10,
    MoveEP = 11, NO_Move = 12
};

enum {
    DRAW = 0,
    EGBB_EVAL = 5000,
    MATE_EVAL = 29000,
    MATE = 30000
};

static const int EGBB_PLY = 1;

enum EPD{ EPD_EVAL, EPD_TIME, EPD_DEPTH, EPD_NODES };

enum GenState {
   EVASION, TRANS, GOOD_CAPTURE, BAD_CAPTURE, BAD_EVASION_CAPTURE, KILLER,
   QUIET_H, EVASION_QS, CAPTURE_QS, CHECK_QS, QUIET, CAPTURE, END
};

enum Phase{ Opening = 0, Endgame = 1 };

enum Endgame{
    EG_NONE,
    EG_KK,
    EG_KBK, EG_KKB,
    EG_KNK, EG_KKN,
    EG_KPK, EG_KKP,
    EG_KQKQ, EG_KQKP, EG_KPKQ,
    EG_KRKR, EG_KRKP, EG_KPKR,
    EG_KBKB, EG_KBKP, EG_KPKB, EG_KBPK, EG_KKBP,
    EG_KNKN, EG_KNKP, EG_KPKN, EG_KNPK, EG_KKNP,
    EG_KRPKR, EG_KRKRP,
    EG_KBPKB, EG_KBKBP,
    EG_NB
};

enum SearchFlag{ Alpha = 0, Beta = 1, Exact = 2, NoFlag = 3 };


const int PieceValue[15] = {
    0, 0,
   100, 100,
   300, 300,
   300, 300,
   500, 500,
   1000, 1000,
   10000, 10000,
   0
};

inline int piece_value(const Piece p){ return PieceValue[p]; }

inline Phase operator++ (Phase &x, int y) { y = int(x); x = Phase(int(x) + 1); return Phase(y); }

inline GenState operator++ (GenState &x, int y) { y = int(x); x = GenState(int(x) + 1); return GenState(y); }

inline Square operator+ (Square sq, int i) { return Square(int(sq) + i); }

inline Square operator- (Square sq, int i) { return Square(int(sq) - i); }

inline Square operator++ (Square &x, int y) { y = int(x); x = Square(int(x) + 1); return Square(y); }

inline Square operator-- (Square &x, int y) { y = int(x); x = Square(int(x) - 1); return Square(y); }

inline void operator+= (Square &x, int y) { x = Square(int(x) + y); }

inline Square operator++ (Square &x) { x = Square(int(x) + 1); return x; }

inline Square operator-- (Square &x) { x = Square(int(x) - 1); return x; }

inline void operator-= (Square &sq1, int sq2) { sq1 = Square(int(sq1) - sq2); }

inline void operator++ (Piece &x, int) { x = Piece(int(x) + 1); }

inline void operator+= (Piece &p, int x) { p = Piece(int(p) + x); }

inline bool operator>= (Piece p, PieceType x) { return int(p) >= int(x); }

inline Piece operator+ (PieceType pt, int x) { return Piece(int(pt) + x); }

inline File operator++ (File &x, int y) { y = int(x); x = File(int(x) + 1); return File(y); }

inline File operator++ (File &x) { x = File(int(x) + 1); return x; }

inline File operator-- (File &x) { x = File(int(x) - 1); return x; }

inline Rank operator-- (Rank &x, int y) { y= x; x = Rank(int(x) - 1); return Rank(y); }

inline Rank operator-- (Rank &x) { x = Rank(int(x) - 1); return x; }

inline Rank operator++ (Rank &x, int y) { y = int(x); x = Rank(int(x) + 1); return Rank(y); }

inline Rank operator++ (Rank &x) { x = Rank(int(x) + 1); return x; }

inline void operator&= (Castle& c, int x) { c = Castle(int(c) & x); }

inline void operator|= (Castle& c, int x) { c = Castle(int(c) | x); }

inline Color operator++ (Color &x, int y) { y = int(x); x = Color(int(x) + 1); return Color(y); }

inline void operator^= (Color& c, int x) { c = Color(int(c) ^ x); }

inline Color operator^ (Color c, int x) { return Color(int(c) ^ x); }

inline void operator-= (Move &m, int x) { m = Move(int(m) - x); }

inline Move operator| (Move m, int x) { return Move(int(m) | x); }

inline Square move_from(const Move m) { return Square((int(m) >> 4) & 0x3F); }

inline Square move_to(const Move m) { return Square((int(m) >> 10) & 0x3F); }

inline Move move_type(const Move m) { return Move(m & 15); }

inline Move move_create(const Move m, Square from, Square to){
    return m | from << 4 | to << 10;
}

inline Rank rank(const Square sq){ return Rank(sq>>3); }

inline File file(const Square sq){ return File(sq&7); }

inline Color color(const Square sq){ return Color(((sq^(sq>>3))&1)^1); }

inline Square opposite(const Square sq){ return Square(sq^070); }

inline Square get_square(const File f, const Rank r) { return Square((r<<3)|f); }

inline Square rank_mirror_sq(const Square sq) { return Square(sq^0x38); }

inline Square file_mirror_sq(const Square sq) { return Square(sq^0x7); }

inline Bitboard shift_down(const Bitboard b){ return b>>8; }

inline Bitboard shift_2_down(const Bitboard b){ return b>>16; }

inline Bitboard shift_up(const Bitboard b){ return b<<8; }

inline Bitboard shift_2_up(const Bitboard b){ return (b<<16); }

inline Bitboard shift_right(const Bitboard b){ return (b<<1) & BB_NOT_FILE_A; }

inline Bitboard shift_2_right(const Bitboard b){ return (b<<2) & BB_NOT_FILES_AB; }

inline Bitboard shift_left(const Bitboard b){ return (b>>1) & BB_NOT_FILE_H; }

inline Bitboard shift_2_left(const Bitboard b){ return (b>>2) & BB_NOT_FILES_GH; }

inline Bitboard shift_up_left(const Bitboard b){ return (b<<7) & BB_NOT_FILE_H; }

inline Bitboard shift_up_right(const Bitboard b){ return (b<<9) & BB_NOT_FILE_A; }

inline Bitboard shift_down_left(const Bitboard b){ return (b>>9) & BB_NOT_FILE_H; }

inline Bitboard shift_down_right(const Bitboard b){ return (b>>7) & BB_NOT_FILE_A; }

inline Bitboard shift_2_up_right(const Bitboard b){ return (b<<17)& BB_NOT_FILE_A; }

inline Bitboard shift_2_up_left(const Bitboard b){ return (b<<15)& BB_NOT_FILE_H; }

inline Bitboard shift_2_down_left(const Bitboard b){ return (b>>17)& BB_NOT_FILE_H; }

inline Bitboard shift_2_down_right(const Bitboard b){ return (b>>15)& BB_NOT_FILE_A; }

inline bool bit_le(const Bitboard b, const int r){
    if (b << ((8-r)<<3)) return true; return false;
}

inline bool bit_le_eq(const Bitboard b, const int r){
    if (b << ((7-r)<<3)) return true; return false;
}

inline bool bit_gt(const Bitboard b, const int r){
    if (b >> ((r+1)<<3)) return true; return false;
}

inline bool bit_gt_eq(const Bitboard b, const int r){
    if (b >> (r<<3)) return true; return false;
}

inline Bitboard bb_file(const Bitboard b, const int f){
        if (f < FILE_A || f > FILE_H) return 0;
        return b & BB_FILE[f];
}

#ifdef __x86_64__
inline Square __attribute__((always_inline)) first_bit(const Bitboard bb) {
  Square sq;
  __asm__("bsf %1, %%rcx\n\t": "=c"(sq): "r"(bb));
  return sq;
}

inline Square __attribute__((always_inline)) last_bit(const Bitboard bb){
    Square sq;
    __asm__("bsr %1,%%rcx\n\t":"=c" (sq): "r"(bb));
    return sq;
}

inline Square __attribute__((always_inline)) first_bit_clear(Bitboard& bb) {
  const Square s = first_bit(bb);
  bb &= ~(1ULL<<s);
  return s;
}
#elif __i386__
inline Square __attribute__((always_inline)) first_bit(Bitboard bb) {
  Square sq;
  __asm__(
    "bsf %1,%%ecx\n\t"
    "jnz 1f\n\t"
    "bsf %2,%%ecx\n\t"
    "add $32,%%ecx\n"
    "1:\n"
    :"=c" (sq)
    :"r" (bb), "r"(bb>>32)
  );
  return sq;
}

inline Square __attribute__((always_inline)) last_bit(Bitboard bb) {
  Square sq;
  __asm__(
    "bsr %2,%%ecx\n\t"
    "jnz 1f\n\t"
    "bsr %1,%%ecx\n\t"
    "jmp 2f\n\t"
    "1:\n"
    "add $32,%%ecx\n"
    "2:\n"
    :"=c" (sq)
    :"r" (bb), "r"(bb>>32)
  );
  return sq;
}


inline Square __attribute__((always_inline)) first_bit_clear(Bitboard& bb) {
  const Square s = first_bit(bb);
  bb &= ~(1ULL<<s);
  return s;
}

#else
extern Square first_bit(Bitboard b);
extern Square last_bit(Bitboard b);
extern Square first_bit_clear(Bitboard& b);
#endif

inline int bit_count(Bitboard b) {
  b -= ((b>>1) & 0x5555555555555555ULL);
  b = ((b>>2) & 0x3333333333333333ULL) + (b & 0x3333333333333333ULL);
  b = ((b>>4) + b) & 0x0F0F0F0F0F0F0F0FULL;
  b *= 0x0101010101010101ULL;
  return int(b >> 56);
}

inline int bit_count_15(Bitboard b) {
  b -= (b>>1) & 0x5555555555555555ULL;
  b = ((b>>2) & 0x3333333333333333ULL) + (b & 0x3333333333333333ULL);
  b *= 0x1111111111111111ULL;
  return int(b >> 60);
}

inline int bit_count_3(Bitboard b){
    int r;
    for(r = 0; b; r++, b &= b - 1){};
    return r;
}

inline int bit_num_le_eq(const Bitboard b, const int r){
    return bit_count_3(b << ((7-r)<<3));
}

inline int bit_num_le(const Bitboard b, const int r){
    return bit_count_3(b << ((8-r)<<3));
}

inline int bit_num_gt(const Bitboard b, const int r){
    return bit_count_3(b >> ((r+1)<<3));
}

inline int bit_num_gt_eq(const Bitboard b, const int r){
    return bit_count_3(b >> (r<<3));
}

inline int bit_num_eq(const Bitboard b, const int r){
    return bit_count_3(b & BB_RANK[r]);
}

inline int distance(const Square from, const Square to){
    int dist1 = abs(file(from) - file(to));
    int dist2 = abs(rank(from) - rank(to));
    return std::max(dist1,dist2);
}

char piece_to_char(const Piece p);

char piece_to_san_char(const Piece p);

std::string square_to_string(const Square sq);

void bb_print(Bitboard x);

Piece char_to_piece(const char c);

Square string_to_sq(const std::string s);

std::string move_to_string(const Move m);

inline int eval_to_tt(const int eval, const int ply) {
    if(eval >= EGBB_EVAL){
        if(eval >= MATE_EVAL) return eval + ply;
        return eval + ply * EGBB_PLY;
    }
    else if(eval <= -EGBB_EVAL){
        if(eval <= - MATE_EVAL) return eval - ply;
        return eval - ply * EGBB_PLY;
    }
    return eval;
}

inline int eval_from_tt(const int eval, const int ply) {
    if(eval >= EGBB_EVAL){
        if(eval >= MATE_EVAL) return eval - ply;
        return eval - ply * EGBB_PLY;
    }
    else if(eval <= - EGBB_EVAL){
        if(eval <= -MATE_EVAL) return eval + ply;
        return eval + ply * EGBB_PLY;
    }
    return eval;
}

inline bool mate_value(const int eval){
    return ((eval < -MATE_EVAL) || (eval > MATE_EVAL));
}

inline bool is_tactical(const Move move){
    int type = move_type(move);
    return (type == MoveCapture || type >= MovePN);
}

inline bool operator == (const Move m, const int bm) {
    return (move_to(m) | (move_from(m)<<6)) == bm;
}

inline unsigned int myrand32(){
	#if defined(__MINGW32__)
	return ((unsigned int)rand()<<30) | ((unsigned int)rand()<<15) | rand();
	#else
	return rand();
	#endif
}

inline Key myrand64(){
	return myrand32() | (Key(myrand32())<<31) | (Key(myrand32())<<62);
}

#endif // TYPES_H
