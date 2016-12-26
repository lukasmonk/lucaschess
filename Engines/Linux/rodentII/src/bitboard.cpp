/*
Rodent, a UCI chess playing engine derived from Sungorus 1.4
Copyright (C) 2009-2011 Pablo Vazquez (Sungorus author)
Copyright (C) 2011-2016 Pawel Koziol

Rodent is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published
by the Free Software Foundation, either version 3 of the License,
or (at your option) any later version.

Rodent is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty
of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "rodent.h"
#include "magicmoves.h"

#define USE_MAGIC
#define USE_MM_POPCNT

void cBitBoard::Init() {

#ifdef USE_MAGIC
  initmagicmoves();
#endif

  // init pawn attacks

  for (int sq = 0; sq < 64; sq++) {
    p_attacks[WC][sq] = ShiftNE(SqBb(sq)) | ShiftNW(SqBb(sq));
    p_attacks[BC][sq] = ShiftSE(SqBb(sq)) | ShiftSW(SqBb(sq));
  }
 
  // init knight attacks

  for (int sq = 0; sq < 64; sq++) {
	U64 bb_west = ShiftWest(SqBb(sq));
	U64 bb_east = ShiftEast(SqBb(sq));
	n_attacks[sq] = (bb_east | bb_west) << 16;
	n_attacks[sq] |= (bb_east | bb_west) >> 16;
	bb_west = ShiftWest(bb_west);
	bb_east = ShiftEast(bb_east);
	n_attacks[sq] |= (bb_east | bb_west) << 8;
	n_attacks[sq] |= (bb_east | bb_west) >> 8;
  }

  // init king attacks

  for (int sq = 0; sq < 64; sq++) {
    k_attacks[sq] = SqBb(sq);
	k_attacks[sq] |= ShiftSideways(k_attacks[sq]);
    k_attacks[sq] |= (ShiftNorth(k_attacks[sq]) | ShiftSouth(k_attacks[sq]));
  }

  for (int sq1 = 0; sq1 < 64; sq1++) {
    for (int sq2 = 0; sq2 < 64; sq2++) {
      bbBetween[sq1][sq2] = GetBetween(sq1, sq2);
    }
  }

}

// from chessprogramming wiki

U64 cBitBoard::GetBetween(int sq1, int sq2) {

  const U64 m1 = C64(-1);
  const U64 a2a7 = C64(0x0001010101010100);
  const U64 b2g7 = C64(0x0040201008040200);
  const U64 h1b7 = C64(0x0002040810204080); /* Thanks Dustin, g2b7 did not work for c1-a3 */
  U64 btwn, line, rank, file;

  btwn = (m1 << sq1) ^ (m1 << sq2);
  file = (sq2 & 7) - (sq1 & 7);
  rank = ((sq2 | 7) - sq1) >> 3;
  line = ((file & 7) - 1) & a2a7; /* a2a7 if same file */
  line += 2 * (((rank & 7) - 1) >> 58); /* b1g1 if same rank */
  line += (((rank - file) & 15) - 1) & b2g7; /* b2g7 if same diagonal */
  line += (((rank + file) & 15) - 1) & h1b7; /* h1b7 if same antidiag */
  line *= btwn & -btwn; /* mul acts like shift by smaller square */
  return line & btwn;   /* return the bits on that line in-between */
}

#if defined(__GNUC__)

int cBitBoard::PopCnt(U64 bb) {
  return __builtin_popcountll(bb);
}

#elif defined(USE_MM_POPCNT) && defined(_M_AMD64)  // 64 bit windows
#include <nmmintrin.h>

int cBitBoard::PopCnt(U64 bb) {
  return (int)_mm_popcnt_u64(bb);
}

#else

int cBitBoard::PopCnt(U64 bb) // general purpose population count
{
  U64 k1 = (U64)0x5555555555555555;
  U64 k2 = (U64)0x3333333333333333;
  U64 k3 = (U64)0x0F0F0F0F0F0F0F0F;
  U64 k4 = (U64)0x0101010101010101;

  bb -= (bb >> 1) & k1;
  bb = (bb & k2) + ((bb >> 2) & k2);
  bb = (bb + (bb >> 4)) & k3;
  return (bb * k4) >> 56;
}

#endif

int cBitBoard::PopFirstBit(U64 * bb) {

  U64 bbLocal = *bb;
  *bb &= (*bb - 1);
  return FirstOne(bbLocal);
}

U64 cBitBoard::FillNorth(U64 bb) {
  bb |= bb << 8;
  bb |= bb << 16;
  bb |= bb << 32;
  return bb;
}

U64 cBitBoard::FillSouth(U64 bb) {
  bb |= bb >> 8;
  bb |= bb >> 16;
  bb |= bb >> 32;
  return bb;
}

U64 cBitBoard::FillNorthExcl(U64 bb) {
  return FillNorth(ShiftNorth(bb));
}

U64 cBitBoard::FillSouthExcl(U64 bb) {
  return FillSouth(ShiftSouth(bb));
}

U64 cBitBoard::FillNorthSq(int sq) {
  return FillNorth(SqBb(sq));
}

U64 cBitBoard::FillSouthSq(int sq) {
  return FillSouth(SqBb(sq));
}

U64 cBitBoard::GetWPControl(U64 bb) {
  return (ShiftNE(bb) | ShiftNW(bb));
}

U64 cBitBoard::GetBPControl(U64 bb) {
  return (ShiftSE(bb) | ShiftSW(bb));
}

U64 cBitBoard::GetDoubleWPControl(U64 bb) {
  return (ShiftNE(bb) & ShiftNW(bb));
}

U64 cBitBoard::GetDoubleBPControl(U64 bb) {
  return (ShiftSE(bb) & ShiftSW(bb));
}

U64 cBitBoard::GetFrontSpan(U64 bb, int sd) {

  if (sd == WC) return BB.FillNorthExcl(bb);
  else          return BB.FillSouthExcl(bb);
}

U64 cBitBoard::ShiftFwd(U64 bb, int sd) {

  if (sd == WC) return ShiftNorth(bb);
  return ShiftSouth(bb);
}

U64 cBitBoard::ShiftSideways(U64 bb) {
  return (ShiftWest(bb) | ShiftEast(bb));
}

U64 cBitBoard::PawnAttacks(int sd, int sq) {
  return p_attacks[sd][sq];
}

U64 cBitBoard::FillOcclSouth(U64 bbStart, U64 bbBlock) {

  bbStart |= bbBlock & (bbStart >> 8);
  bbBlock &= (bbBlock >> 8);
  bbStart |= bbBlock & (bbStart >> 16);
  bbBlock &= (bbBlock >> 16);
  bbStart |= bbBlock & (bbStart >> 32);
  return bbStart;
}

U64 cBitBoard::FillOcclNorth(U64 bbStart, U64 bbBlock) {

  bbStart |= bbBlock & (bbStart << 8);
  bbBlock &= (bbBlock << 8);
  bbStart |= bbBlock & (bbStart << 16);
  bbBlock &= (bbBlock << 16);
  bbStart |= bbBlock & (bbStart << 32);
  return bbStart;
}

U64 cBitBoard::FillOcclEast(U64 bbStart, U64 bbBlock) {

  bbBlock &= bbNotA;
  bbStart |= bbBlock & (bbStart << 1);
  bbBlock &= (bbBlock << 1);
  bbStart |= bbBlock & (bbStart << 2);
  bbBlock &= (bbBlock << 2);
  bbStart |= bbBlock & (bbStart << 4);
  return bbStart;
}

U64 cBitBoard::FillOcclNE(U64 bbStart, U64 bbBlock) {

  bbBlock &= bbNotA;
  bbStart |= bbBlock & (bbStart << 9);
  bbBlock &= (bbBlock << 9);
  bbStart |= bbBlock & (bbStart << 18);
  bbBlock &= (bbBlock << 18);
  bbStart |= bbBlock & (bbStart << 36);
  return bbStart;
}

U64 cBitBoard::FillOcclSE(U64 bbStart, U64 bbBlock) {

  bbBlock &= bbNotA;
  bbStart |= bbBlock & (bbStart >> 7);
  bbBlock &= (bbBlock >> 7);
  bbStart |= bbBlock & (bbStart >> 14);
  bbBlock &= (bbBlock >> 14);
  bbStart |= bbBlock & (bbStart >> 28);
  return bbStart;
}

U64 cBitBoard::FillOcclWest(U64 bbStart, U64 bbBlock) {

  bbBlock &= bbNotH;
  bbStart |= bbBlock & (bbStart >> 1);
  bbBlock &= (bbBlock >> 1);
  bbStart |= bbBlock & (bbStart >> 2);
  bbBlock &= (bbBlock >> 2);
  bbStart |= bbBlock & (bbStart >> 4);
  return bbStart;
}

U64 cBitBoard::FillOcclSW(U64 bbStart, U64 bbBlock) {

  bbBlock &= bbNotH;
  bbStart |= bbBlock & (bbStart >> 9);
  bbBlock &= (bbBlock >> 9);
  bbStart |= bbBlock & (bbStart >> 18);
  bbBlock &= (bbBlock >> 18);
  bbStart |= bbBlock & (bbStart >> 36);
  return bbStart;
}

U64 cBitBoard::FillOcclNW(U64 bbStart, U64 bbBlock) {

  bbBlock &= bbNotH;
  bbStart |= bbBlock & (bbStart << 7);
  bbBlock &= (bbBlock << 7);
  bbStart |= bbBlock & (bbStart << 14);
  bbBlock &= (bbBlock << 14);
  bbStart |= bbBlock & (bbStart << 28);
  return bbStart;
}

U64 cBitBoard::KnightAttacks(int sq) {
  return n_attacks[sq];
}

U64 cBitBoard::RookAttacks(U64 bbOcc, int sq) {

#ifdef USE_MAGIC
  return Rmagic(sq, bbOcc);
#else
  U64 bbStart = SqBb(sq);
  U64 result = ShiftNorth(FillOcclNorth(bbStart, ~bbOcc))
             | ShiftSouth(FillOcclSouth(bbStart, ~bbOcc))
             | ShiftEast(FillOcclEast(bbStart, ~bbOcc))
             | ShiftWest(FillOcclWest(bbStart, ~bbOcc));
  return result;
#endif
}

U64 cBitBoard::BishAttacks(U64 bbOcc, int sq) {
#ifdef USE_MAGIC
  return Bmagic(sq, bbOcc);
#else
  U64 bbStart = SqBb(sq);
  U64 result = ShiftNE(FillOcclNE(bbStart, ~bbOcc))
             | ShiftNW(FillOcclNW(bbStart, ~bbOcc))
             | ShiftSE(FillOcclSE(bbStart, ~bbOcc))
             | ShiftSW(FillOcclSW(bbStart, ~bbOcc));
  return result;
#endif
}

U64 cBitBoard::QueenAttacks(U64 bbOcc, int sq) {

#ifdef USE_MAGIC
  return Rmagic(sq, bbOcc) | Bmagic(sq, bbOcc);
#else
  return RookAttacks(bbOcc, sq) | BishAttacks(bbOcc, sq);
#endif
}

U64 cBitBoard::KingAttacks(int sq) {
	return k_attacks[sq];
}