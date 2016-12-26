//  GREKO Chess Engine
//  (c) 2002-2015 Vladimir Medvedev <vrm@bk.ru>
//  http://greko.su

//  bitboards.h: 64-bit position representation
//  modified: 01-Mar-2013

#ifndef BITBOARDS_H
#define BITBOARDS_H

#include "types.h"
#define MAGIC

class Bitboard
{
private:
	static U64 m_single[64];
	static U64 m_dir[64][8];
	static U64 m_between[64][64];
	static U64 m_pawnAttacks[64][2];
	static U64 m_knightAttacks[64];
	static U64 m_bishopAttacks[64];
	static U64 m_rookAttacks[64];
	static U64 m_queenAttacks[64];
	static U64 m_kingAttacks[64];
	static U64 m_pawnSquare[64][2];
	static U8 m_msb16[65536];
	static const U64 m_bMask[64];
	static const U64 m_bMult[64];
	static const int m_bBits[64];
	static const U64 m_rMask[64];
	static const U64 m_rMult[64];
	static const int m_rBits[64];
	static int m_bOffset[64];
	static U64* m_bData;
	static int m_rOffset[64];
	static U64* m_rData;

	static U64 BishopAttacksTrace(int f, const U64& occ);
	static U64 EnumBits(U64 mask, U64 n);
	static U64 RookAttacksTrace(int f, const U64& occ);
public:
	static void InitBitboards();
	static void Print(U64 b);
	static U64 Single(int f) { return m_single[f]; }
	static U64 Dir(int f, int dir) { return m_dir[f][dir]; }
	static U64 Between(int f1, int f2) { return m_between[f1][f2]; }
	static U64 PawnAttacks(int f, int side) { return m_pawnAttacks[f][side]; }
	static U64 KnightAttacks(int f) { return m_knightAttacks[f]; }
	static U64 BishopAttacks(int f) { return m_bishopAttacks[f]; }
	static U64 RookAttacks(int f) { return m_rookAttacks[f]; }
	static U64 QueenAttacks(int f) { return m_queenAttacks[f]; }
	static U64 KingAttacks(int f) { return m_kingAttacks[f]; }
	static U64 PawnSquare(int f, int side) { return m_pawnSquare[f][side]; }

	static U64 Up(const U64& b) { return b << 8; }
	static U64 Down(const U64& b) { return b >> 8; }
	static U64 Right(const U64& b) { return (b & LL(0xfefefefefefefefe)) >> 1; }
	static U64 Left(const U64& b) { return (b & LL(0x7f7f7f7f7f7f7f7f)) << 1; }
	static U64 UpRight(const U64& b) { return (b & LL(0x00fefefefefefefe)) << 7; }
	static U64 UpLeft(const U64& b) { return (b & LL(0x007f7f7f7f7f7f7f)) << 9; }
	static U64 DownRight(const U64& b) { return (b & LL(0xfefefefefefefe00)) >> 9; }
	static U64 DownLeft(const U64& b) { return (b & LL(0x7f7f7f7f7f7f7f00)) >> 7; }
	
	static U64 BishopAttacks(int f, const U64& occ)
	{
#ifdef MAGIC
		int index = m_bOffset[f];
		index += int(((occ & m_bMask[f]) * m_bMult[f]) >> (64 - m_bBits[f]));
		return Bitboard::m_bData[index];
#else
		U64 att = m_bishopAttacks[f];
		U64 x = m_dir[f][DIR_UR] & occ;
		if (x)
			att ^= m_dir[LSB(x)][DIR_UR];
		x = m_dir[f][DIR_UL] & occ;
		if (x)
			att ^= m_dir[LSB(x)][DIR_UL];
		x = m_dir[f][DIR_DL] & occ;
		if (x)
			att ^= m_dir[MSB(x)][DIR_DL];
		x = m_dir[f][DIR_DR] & occ;
		if (x)
			att ^= m_dir[MSB(x)][DIR_DR];
		return att;
#endif
	}

	static U64 RookAttacks(int f, const U64& occ)
	{
#ifdef MAGIC
		int index = m_rOffset[f];
		index += int(((occ & m_rMask[f]) * m_rMult[f]) >> (64 - m_rBits[f]));
		return Bitboard::m_rData[index];
#else
		U64 att = m_rookAttacks[f];
		U64 x = m_dir[f][DIR_R] & occ;
		if (x)
			att ^= m_dir[MSB(x)][DIR_R];
		x = m_dir[f][DIR_U] & occ;
		if (x)
			att ^= m_dir[LSB(x)][DIR_U];
		x = m_dir[f][DIR_L] & occ;
		if (x)
			att ^= m_dir[LSB(x)][DIR_L];
		x = m_dir[f][DIR_D] & occ;
		if (x)
			att ^= m_dir[MSB(x)][DIR_D];
		return att;
#endif
	}

	static U64 QueenAttacks(int f, const U64& occ)
	{
		return BishopAttacks(f, occ) | RookAttacks(f, occ);
	}

	static int CountBits(U64 b)
	{
		const U64 mask1 = LL(0x5555555555555555); // 01010101 01010101 ...
		const U64 mask2 = LL(0x3333333333333333); // 00110011 00110011 ...
		const U64 mask4 = LL(0x0f0f0f0f0f0f0f0f); // 00001111 00001111 ...
		const U64 mask8 = LL(0x00ff00ff00ff00ff); // 00000000 11111111 ...

		U64 x = (b & mask1) + ((b >> 1) & mask1);
		x = (x & mask2) + ((x >> 2) & mask2);
		x = (x & mask4) + ((x >> 4) & mask4);
		x = (x & mask8) + ((x >> 8) & mask8);

		U32 y = U32(x) + U32(x >> 32);
		return U8(y + (y >> 16));
	}

	static FLD LSB(const U64& bb)
	{
		assert(bb != 0);

		static const FLD LSB_Magic[64] =
		{
			A8, B4, E1, H5, E8, B2, E2, G5,
			D8, H4, F7, G2, A7, E3, C3, F5,
			C8, C4, F1, C7, E7, A3, G6, F3,
			H8, D4, G1, E6, B6, E4, H1, E5,
			B8, A4, F8, D1, C1, G7, B7, B1,
			A2, D7, D2, H6, A1, F6, C6, H3,
			G4, G8, H7, C2, F2, A5, H2, D6,
			D3, A6, B5, B3, G3, C5, D5, F4
		};

		U64 lsb = bb ^ (bb - 1);
		unsigned int foldedLSB = ((int) lsb) ^ ((int)(lsb>>32));
		int ind = (foldedLSB * 0x78291ACF) >> (32-6); // range is 0..63
		return LSB_Magic[ind];
	}

	static FLD PopLSB(U64& bb)
	{
		assert(bb != 0);
		FLD f = LSB(bb);
		bb ^= Single(f);
		return f;
	}

	static int MSB(const U64& b)
	{
		assert (b != 0);
		U32 high = U32(b >> 32);
		if (high)
		{
			if (high >> 16)
				return m_msb16[high >> 16];
			else
				return 16 + m_msb16[high];
		}
		else
		{
			U32 low = U32(b);
			if (low >> 16)
				return 32 + m_msb16[low >> 16];
			else
				return 48 + m_msb16[low];
		}
	}

	static int PopMSB(U64& b)
	{
		assert (b != 0);
		int f = MSB(b);
		b ^= Single(f);
		return f;
	}
};

#endif
