//  GREKO Chess Engine
//  (c) 2002-2015 Vladimir Medvedev <vrm@bk.ru>
//  http://greko.su

//  bitboards.cpp: 64-bit position representation
//  modified: 01-Mar-2013

#include "position.h"
#include "utils.h"

U64 Bitboard::m_single[64];
U64 Bitboard::m_dir[64][8];
U64 Bitboard::m_pawnAttacks[64][2];
U64 Bitboard::m_knightAttacks[64];
U64 Bitboard::m_bishopAttacks[64];
U64 Bitboard::m_rookAttacks[64];
U64 Bitboard::m_queenAttacks[64];
U64 Bitboard::m_kingAttacks[64];
U64 Bitboard::m_between[64][64];
U64 Bitboard::m_pawnSquare[64][2];

int Bitboard::m_bOffset[64];
U64* Bitboard::m_bData = NULL;
int Bitboard::m_rOffset[64];
U64* Bitboard::m_rData = NULL;

U8 Bitboard::m_msb16[65536];

const U64 Bitboard::m_bMask[64] =
{
	LL(0x0040201008040200), LL(0x0020100804020000), LL(0x0050080402000000), LL(0x0028440200000000),
	LL(0x0014224000000000), LL(0x000a102040000000), LL(0x0004081020400000), LL(0x0002040810204000),
	LL(0x0000402010080400), LL(0x0000201008040200), LL(0x0000500804020000), LL(0x0000284402000000),
	LL(0x0000142240000000), LL(0x00000a1020400000), LL(0x0000040810204000), LL(0x0000020408102000),
	LL(0x0040004020100800), LL(0x0020002010080400), LL(0x0050005008040200), LL(0x0028002844020000),
	LL(0x0014001422400000), LL(0x000a000a10204000), LL(0x0004000408102000), LL(0x0002000204081000),
	LL(0x0020400040201000), LL(0x0010200020100800), LL(0x0008500050080400), LL(0x0044280028440200),
	LL(0x0022140014224000), LL(0x00100a000a102000), LL(0x0008040004081000), LL(0x0004020002040800),
	LL(0x0010204000402000), LL(0x0008102000201000), LL(0x0004085000500800), LL(0x0002442800284400),
	LL(0x0040221400142200), LL(0x0020100a000a1000), LL(0x0010080400040800), LL(0x0008040200020400),
	LL(0x0008102040004000), LL(0x0004081020002000), LL(0x0002040850005000), LL(0x0000024428002800),
	LL(0x0000402214001400), LL(0x004020100a000a00), LL(0x0020100804000400), LL(0x0010080402000200),
	LL(0x0004081020400000), LL(0x0002040810200000), LL(0x0000020408500000), LL(0x0000000244280000),
	LL(0x0000004022140000), LL(0x00004020100a0000), LL(0x0040201008040000), LL(0x0020100804020000),
	LL(0x0002040810204000), LL(0x0000020408102000), LL(0x0000000204085000), LL(0x0000000002442800),
	LL(0x0000000040221400), LL(0x0000004020100a00), LL(0x0000402010080400), LL(0x0040201008040200)
};

const U64 Bitboard::m_bMult[64] =
{
	LL(0x0040080100420440), LL(0x0000201401060400), LL(0x0000802004900180), LL(0x0200000022024400),
	LL(0x0000000400411091), LL(0x1008002100411000), LL(0x0002104202100200), LL(0x0000240100d01000),
	LL(0x0004810802208000), LL(0x00a0021002008000), LL(0x0200040c08020200), LL(0x0000448405040000),
	LL(0x5000000442020002), LL(0x0020402203100000), LL(0x0000888401600000), LL(0x4000410808400080),
	LL(0x2084008400400100), LL(0x0182040104000200), LL(0x2003200080800100), LL(0x0000200208809400),
	LL(0x0000024200802800), LL(0x1001008040410400), LL(0x0008440220000801), LL(0x0201282004001000),
	LL(0x0004040028004100), LL(0x0001010200142200), LL(0x0020080040008240), LL(0x0001020a00040050),
	LL(0x0100020080180480), LL(0x0008109000080044), LL(0x2002021080200100), LL(0x2010043000041000),
	LL(0x0001020401008080), LL(0x0042040000410800), LL(0x0800420001009200), LL(0x01108c0000802000),
	LL(0x0004014024010002), LL(0x0001221010008200), LL(0x0005100005040800), LL(0x0008048020202200),
	LL(0x0001000080829000), LL(0x0002000141042010), LL(0x8000800048200801), LL(0x0001000890400800),
	LL(0x0018040082004000), LL(0x000c200204040008), LL(0x0020000418028100), LL(0x0020001404042800),
	LL(0x00002200c1041000), LL(0x0000040084242100), LL(0x0024009004200020), LL(0x4001040421008000),
	LL(0x0009042408800000), LL(0x0080080094208000), LL(0x0010204400808100), LL(0x000004100a120400),
	LL(0x0001050800840400), LL(0x0000440220102004), LL(0x0082080208000011), LL(0x0482021080004000),
	LL(0x8008048100010040), LL(0x0090208081000040), LL(0x3002080104008000), LL(0x0410101000802040)
};

const int Bitboard::m_bBits[64] =
{
	6,  5,  5,  5,  5,  5,  5,  6,
	5,  5,  5,  5,  5,  5,  5,  5,
	5,  5,  7,  7,  7,  7,  5,  5,
	5,  5,  7,  9,  9,  7,  5,  5,
	5,  5,  7,  9,  9,  7,  5,  5,
	5,  5,  7,  7,  7,  7,  5,  5,
	5,  5,  5,  5,  5,  5,  5,  5,
	6,  5,  5,  5,  5,  5,  5,  6
};

const U64 Bitboard::m_rMask[64] =
{
	LL(0x7e80808080808000), LL(0x3e40404040404000), LL(0x5e20202020202000), LL(0x6e10101010101000),
	LL(0x7608080808080800), LL(0x7a04040404040400), LL(0x7c02020202020200), LL(0x7e01010101010100),
	LL(0x007e808080808000), LL(0x003e404040404000), LL(0x005e202020202000), LL(0x006e101010101000),
	LL(0x0076080808080800), LL(0x007a040404040400), LL(0x007c020202020200), LL(0x007e010101010100),
	LL(0x00807e8080808000), LL(0x00403e4040404000), LL(0x00205e2020202000), LL(0x00106e1010101000),
	LL(0x0008760808080800), LL(0x00047a0404040400), LL(0x00027c0202020200), LL(0x00017e0101010100),
	LL(0x0080807e80808000), LL(0x0040403e40404000), LL(0x0020205e20202000), LL(0x0010106e10101000),
	LL(0x0008087608080800), LL(0x0004047a04040400), LL(0x0002027c02020200), LL(0x0001017e01010100),
	LL(0x008080807e808000), LL(0x004040403e404000), LL(0x002020205e202000), LL(0x001010106e101000),
	LL(0x0008080876080800), LL(0x000404047a040400), LL(0x000202027c020200), LL(0x000101017e010100),
	LL(0x00808080807e8000), LL(0x00404040403e4000), LL(0x00202020205e2000), LL(0x00101010106e1000),
	LL(0x0008080808760800), LL(0x00040404047a0400), LL(0x00020202027c0200), LL(0x00010101017e0100),
	LL(0x0080808080807e00), LL(0x0040404040403e00), LL(0x0020202020205e00), LL(0x0010101010106e00),
	LL(0x0008080808087600), LL(0x0004040404047a00), LL(0x0002020202027c00), LL(0x0001010101017e00),
	LL(0x008080808080807e), LL(0x004040404040403e), LL(0x002020202020205e), LL(0x001010101010106e),
	LL(0x0008080808080876), LL(0x000404040404047a), LL(0x000202020202027c), LL(0x000101010101017e)
};

const U64 Bitboard::m_rMult[64] =
{
	LL(0x0000010824004082), LL(0x0001004200008c01), LL(0x0401008400020801), LL(0x0082001020040802),
	LL(0x0000040861001001), LL(0x0000084100600011), LL(0x0001001040820022), LL(0x00008000c1001021),
	LL(0x0010408400410200), LL(0x0100050810020400), LL(0x8400800200840080), LL(0x0200940080080080),
	LL(0x0080082090010100), LL(0x0018802000100880), LL(0x0010114008200040), LL(0x0008248000c00080),
	LL(0x2800008044020001), LL(0x2000020810040001), LL(0x0011000804010042), LL(0x0000842801010010),
	LL(0x2010008100080800), LL(0x0010200100430010), LL(0x3008201000404000), LL(0x0005400060808000),
	LL(0x4000088042000401), LL(0x0000020104001810), LL(0x0802008002804400), LL(0x0101140080800800),
	LL(0x0220801000800800), LL(0x0800881000802001), LL(0x0808601000404000), LL(0x4000804000801120),
	LL(0x0020040200004481), LL(0x0041000100820004), LL(0x8200040080800600), LL(0x0201180080040080),
	LL(0x1000402200120008), LL(0x0a00100080802000), LL(0x0120002180400080), LL(0x4020248080004000),
	LL(0x0080020005008044), LL(0x0820010100020004), LL(0x0040818004002200), LL(0x0400808008000401),
	LL(0x0808008010008008), LL(0x0000410020001504), LL(0x2008808040002001), LL(0x2008808000224000),
	LL(0x0000800048800100), LL(0x0020800100802200), LL(0x2000802200808400), LL(0x0002000600201810),
	LL(0x0908808010000800), LL(0x0101801004200080), LL(0x00204010022000c0), LL(0x0080800120804004),
	LL(0x0080004031000880), LL(0x1280008001004200), LL(0x0200020001440810), LL(0x1001000840208010),
	LL(0x010020100009000c), LL(0x0880100008200080), LL(0x0040084020005002), LL(0x0080002882104000)
};

const int Bitboard::m_rBits[64] =
{
	12, 11, 11, 11, 11, 11, 11, 12,
	11, 10, 10, 10, 10, 10, 10, 11,
	11, 10, 10, 10, 10, 10, 10, 11,
	11, 10, 10, 10, 10, 10, 10, 11,
	11, 10, 10, 10, 10, 10, 10, 11,
	11, 10, 10, 10, 10, 10, 10, 11,
	11, 10, 10, 10, 10, 10, 10, 11,
	12, 11, 11, 11, 11, 11, 11, 12
};

U64 Bitboard::EnumBits(U64 mask, U64 n)
{
	U64 x = 0;
	while (mask != 0 && n != 0)
	{
		int f = Bitboard::PopLSB(mask);
		int digit = int(n & 1);
		n >>= 1;
		x |= digit * Bitboard::Single(f);
	}
	return x;
}

#define TRACE(Shift)                \
    x = Shift(Bitboard::Single(f)); \
    while (x)                       \
    {                               \
        att |= x;                   \
        if (x & occ) break;         \
        x = Shift(x);               \
    }

U64 Bitboard::BishopAttacksTrace(int f, const U64& occ)
{
	U64 att = 0;
	U64 x = 0;
	TRACE(Bitboard::UpRight);
	TRACE(Bitboard::UpLeft);
	TRACE(Bitboard::DownLeft);
	TRACE(Bitboard::DownRight);
	return att;
}

U64 Bitboard::RookAttacksTrace(int f, const U64& occ)
{
	U64 att = 0;
	U64 x = 0;
	TRACE(Bitboard::Right);
	TRACE(Bitboard::Up);
	TRACE(Bitboard::Left);
	TRACE(Bitboard::Down);
	return att;
}
#undef TRACE

void Bitboard::InitBitboards()
{
	int i, j, from, to;
	U64 x, y;

	// Bit scan
	for (i = 1; i < 65536; ++i)
	{
		int x = 0x8000;
		for (j = 0; j < 16; ++j)
		{
			if (i & x)
			{
				m_msb16[i] = static_cast<U8>(j);
				break;
			}
			x >>= 1;
		}
	}

	x = ((U64) 1) << 63;
	for (i = 0; i < 64; i++)
	{
		m_single[i] = x;
		x >>= 1;
	}

	for (from = 0; from < 64; from++)
	{
		for (to = 0; to < 64; ++to)
		{
			m_between[from][to] = 0;
		}

#define TRACE_DIR(dir, Shift, delta) \
    x = Shift(m_single[from]);       \
    y = 0;                           \
    to = from + (delta);             \
    while (x)                        \
    {                                \
        m_between[from][to] = y;     \
        y |= x;                      \
        x = Shift(x);                \
        to += (delta);               \
    }                                \
    m_dir[from][dir] = y;

		TRACE_DIR (DIR_R,  Right,     1)
			TRACE_DIR (DIR_UR, UpRight,  -7)
			TRACE_DIR (DIR_U,  Up,       -8)
			TRACE_DIR (DIR_UL, UpLeft,   -9)
			TRACE_DIR (DIR_L,  Left,     -1)
			TRACE_DIR (DIR_DL, DownLeft,  7)
			TRACE_DIR (DIR_D,  Down,      8)
			TRACE_DIR (DIR_DR, DownRight, 9)

			x = m_single[from];
		y = 0;
		y |= Right(UpRight(x));
		y |= Up(UpRight(x));
		y |= Up(UpLeft(x));
		y |= Left(UpLeft(x));
		y |= Left(DownLeft(x));
		y |= Down(DownLeft(x));
		y |= Down(DownRight(x));
		y |= Right(DownRight(x));
		m_knightAttacks[from] = y;

		x = m_single[from];
		y = 0;
		y |= UpRight(x);
		y |= Up(x);
		y |= UpLeft(x);
		y |= Left(x);
		y |= DownLeft(x);
		y |= Down(x);
		y |= DownRight(x);
		y |= Right(x);
		m_kingAttacks[from] = y;

		x = m_single[from];
		y = 0;
		y |= UpRight(x);
		y |= UpLeft(x);
		m_pawnAttacks[from][WHITE] = y;

		x = m_single[from];
		y = 0;
		y |= DownRight(x);
		y |= DownLeft(x);
		m_pawnAttacks[from][BLACK] = y;

		m_bishopAttacks[from] = m_dir[from][DIR_UR] | m_dir[from][DIR_UL] | m_dir[from][DIR_DL] | m_dir[from][DIR_DR];
		m_rookAttacks[from] = m_dir[from][DIR_R] | m_dir[from][DIR_U] | m_dir[from][DIR_L] | m_dir[from][DIR_D];
		m_queenAttacks[from] = m_rookAttacks[from] | m_bishopAttacks[from];
	}

	// pawn squares
	for (int f = 0; f < 64; f++)
	{
		x = m_dir[f][DIR_U] | m_single[f];
		for (j = 0; j < Row(f); j++)
		{
			x |= Right(x);
			x |= Left(x);
		}
		m_pawnSquare[f][WHITE] = x;

		x = m_dir[f][DIR_D] | m_single[f];
		for (j = 0; j < 7 - Row(f); j++)
		{
			x |= Right(x);
			x |= Left(x);
		}
		m_pawnSquare[f][BLACK] = x;
	}

#ifdef MAGIC

	int offset = 0;
	for (int f = 0; f < 64; ++f)
	{
		m_bOffset[f] = offset;
		offset += (1 << m_bBits[f]);
	}
	m_bData = new U64[offset];

	for (int f = 0; f < 64; ++f)
	{
		U64 mask = m_bMask[f];
		int bits = m_bBits[f];
		for (int n = 0; n < (1 << bits); ++n)
		{
			U64 occ = EnumBits(mask, n);
			U64 att = BishopAttacksTrace(f, occ);
			int index = m_bOffset[f];
			index += int((occ * m_bMult[f]) >> (64 - bits));
			m_bData[index] = att;
		}
	}

	offset = 0;
	for (int f = 0; f < 64; ++f)
	{
		m_rOffset[f] = offset;
		offset += (1 << m_rBits[f]);
	}
	m_rData = new U64[offset];

	for (int f = 0; f < 64; ++f)
	{
		U64 mask = m_rMask[f];
		int bits = m_rBits[f];
		for (int n = 0; n < (1 << bits); ++n)
		{
			U64 occ = EnumBits(mask, n);
			U64 att = RookAttacksTrace(f, occ);
			int index = m_rOffset[f];
			index += int((occ * m_rMult[f]) >> (64 - bits));
			m_rData[index] = att;
		}
	}
#endif
}

void Bitboard::Print(U64 b)
{
	out("\n");
	for (int f = 0; f < 64; f++)
	{
		if (b & Single(f))
			out(" 1");
		else
			out(" -");
		if (Col(f) == 7) out("\n");
	}
	out("\n");
}
