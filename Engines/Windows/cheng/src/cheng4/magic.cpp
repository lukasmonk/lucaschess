/*
You can use this program under the terms of either the following zlib-compatible license
or as public domain (where applicable)

  Copyright (C) 2012-2015 Martin Sedlak

  This software is provided 'as-is', without any express or implied
  warranty.  In no event will the authors be held liable for any damages
  arising from the use of this software.

  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely, subject to the following restrictions:

  1. The origin of this software must not be misrepresented; you must not
     claim that you wrote the original software. If you use this software
     in a product, an acknowledgement in the product documentation would be
     appreciated but is not required.
  2. Altered source versions must be plainly marked as such, and must not be
     misrepresented as being the original software.
  3. This notice may not be removed or altered from any source distribution.
*/

// based on Tord Romstad's Looking for magics: https://chessprogramming.wikispaces.com/Looking+for+Magics

#include "magic.h"
#include <memory.h>

namespace cheng4
{

#define STOP_RAY(i)	\
			(i) = ((i)+7) / 7 * 7;	\
			(i)--;

typedef struct MovTab
{
	i8 next;				// next_index (-1 => done)
	i8 skip;				// skip_index (sliders only, -1 => done)
} MovTab;

static const i8 nmtab[][2] =
{
	{ -1, +2 },
	{ -1, -2 },
	{ -2, +1 },
	{ -2, -1 },
	{ +1, +2 },
	{ +1, -2 },
	{ +2, +1 },
	{ +2, -1 },
	{ 127, 127 }
};

static const i8 bmtab[][2] =
{
	{ +1, +1 },
	{ +2, +2 },
	{ +3, +3 },
	{ +4, +4 },
	{ +5, +5 },
	{ +6, +6 },
	{ +7, +7 },
	{ +1, -1 },
	{ +2, -2 },
	{ +3, -3 },
	{ +4, -4 },
	{ +5, -5 },
	{ +6, -6 },
	{ +7, -7 },
	{ -1, +1 },
	{ -2, +2 },
	{ -3, +3 },
	{ -4, +4 },
	{ -5, +5 },
	{ -6, +6 },
	{ -7, +7 },
	{ -1, -1 },
	{ -2, -2 },
	{ -3, -3 },
	{ -4, -4 },
	{ -5, -5 },
	{ -6, -6 },
	{ -7, -7 },
	{ 127, 127 }
};

static const i8 rmtab[][2] =
{
	{ +0, +1 },
	{ +0, +2 },
	{ +0, +3 },
	{ +0, +4 },
	{ +0, +5 },
	{ +0, +6 },
	{ +0, +7 },
	{ +0, -1 },
	{ +0, -2 },
	{ +0, -3 },
	{ +0, -4 },
	{ +0, -5 },
	{ +0, -6 },
	{ +0, -7 },
	{ +1, +0 },
	{ +2, +0 },
	{ +3, +0 },
	{ +4, +0 },
	{ +5, +0 },
	{ +6, +0 },
	{ +7, +0 },
	{ -1, +0 },
	{ -2, +0 },
	{ -3, +0 },
	{ -4, +0 },
	{ -5, +0 },
	{ -6, +0 },
	{ -7, +0 },
	{ 127, 127 }
};

static const i8 qmtab[][2] =
{
	{ +1, +1 },
	{ +2, +2 },
	{ +3, +3 },
	{ +4, +4 },
	{ +5, +5 },
	{ +6, +6 },
	{ +7, +7 },
	{ +1, -1 },
	{ +2, -2 },
	{ +3, -3 },
	{ +4, -4 },
	{ +5, -5 },
	{ +6, -6 },
	{ +7, -7 },
	{ -1, +1 },
	{ -2, +2 },
	{ -3, +3 },
	{ -4, +4 },
	{ -5, +5 },
	{ -6, +6 },
	{ -7, +7 },
	{ -1, -1 },
	{ -2, -2 },
	{ -3, -3 },
	{ -4, -4 },
	{ -5, -5 },
	{ -6, -6 },
	{ -7, -7 },
	{ +0, +1 },
	{ +0, +2 },
	{ +0, +3 },
	{ +0, +4 },
	{ +0, +5 },
	{ +0, +6 },
	{ +0, +7 },
	{ +0, -1 },
	{ +0, -2 },
	{ +0, -3 },
	{ +0, -4 },
	{ +0, -5 },
	{ +0, -6 },
	{ +0, -7 },
	{ +1, +0 },
	{ +2, +0 },
	{ +3, +0 },
	{ +4, +0 },
	{ +5, +0 },
	{ +6, +0 },
	{ +7, +0 },
	{ -1, +0 },
	{ -2, +0 },
	{ -3, +0 },
	{ -4, +0 },
	{ -5, +0 },
	{ -6, +0 },
	{ -7, +0 },
	{ 127, 127 }
};

// [original][current]
static MovTab qmnew[64][64];
static MovTab rmnew[64][64];
static MovTab bmnew[64][64];
static MovTab nmnew[64][64];	// note: only next used

static i32 findSkip( i32 x, i32 y, i32 i, i32 num, const i8 mtab[][2] )
{
	while ( i < num )
	{
		STOP_RAY(i);
		i++;
		if ( i >= num )
			break;
		i32 nx = x + mtab[i][0], ny = y + mtab[i][1];
		if ( (nx & ~7) || (ny & ~7) )
		{
			continue;
		}
		return (ny << 3) + nx;
	}
	return -1;
}

static void initNewMovTab()
{
	memset( qmnew, -1, sizeof(qmnew) );
	memset( rmnew, -1, sizeof(rmnew) );
	memset( bmnew, -1, sizeof(bmnew) );
	memset( nmnew, -1, sizeof(nmnew) );
	i32 i;
	for (i=0; i<64; i++)
	{
		i32 x = i & 7;
		i32 y = i >> 3;
		i32 j;
		i32 nextpos, nextposj;

		// queen
		for ( nextpos = i, nextposj = j=0; j<8*7; j++)
		{
			i32 nx = x + qmtab[j][0], ny = y + qmtab[j][1];
			if ( (nx & ~7) || (ny & ~7 ) )
			{
				STOP_RAY(j);
				continue;
			}
			i32 ni = (ny << 3) | nx;
			qmnew[ i ][ nextpos ].next = (i8)ni;
			qmnew[ i ][ nextpos ].skip = (i8)findSkip( x, y, nextposj, 8*7, qmtab );
			nextposj = j;
			nextpos = ni;
		}
		// rook
		for ( nextpos = i, nextposj = j=0; j<4*7; j++)
		{
			i32 nx = x + rmtab[j][0], ny = y + rmtab[j][1];
			if ( (nx & ~7) || (ny & ~7 ) )
			{
				STOP_RAY(j);
				continue;
			}
			i32 ni = (ny << 3) | nx;
			rmnew[ i ][ nextpos ].next = (i8)ni;
			rmnew[ i ][ nextpos ].skip = (i8)findSkip( x, y, nextposj, 4*7, rmtab );
			nextposj = j;
			nextpos = ni;
		}
		// bishop
		for ( nextpos = i, nextposj = j=0; j<4*7; j++)
		{
			i32 nx = x + bmtab[j][0], ny = y + bmtab[j][1];
			if ( (nx & ~7) || (ny & ~7 ) )
			{
				STOP_RAY(j);
				continue;
			}
			i32 ni = (ny << 3) | nx;
			bmnew[ i ][ nextpos ].next = (i8)ni;
			bmnew[ i ][ nextpos ].skip = (i8)findSkip( x, y, nextposj, 4*7, bmtab );
			nextposj = j;
			nextpos = ni;
		}
		// knight
		for ( nextpos = i, j=0; j<8; j++)
		{
			i32 nx = x + nmtab[j][0], ny = y + nmtab[j][1];
			if ( (nx & ~7) || (ny & ~7 ) )
			{
				continue;
			}
			i32 ni = (ny << 3) | nx;
			nmnew[ i ][ nextpos ].next = (i8)ni;
			nextpos = ni;
		}
	}
}

Bitboard Magic::rookRelOcc[64];
Bitboard Magic::bishopRelOcc[64];
u8 Magic::rookShr[64];
u8 Magic::bishopShr[64];
const Bitboard *Magic::rookPtr[64];
const Bitboard *Magic::bishopPtr[64];

const Bitboard Magic::rookMagic[64] = {
	U64C(0x80001820804000),
	U64C(0xa440100040002003),
	U64C(0x3080200010000880),
	U64C(0x100210008041000),
	U64C(0x100040800021100),
	U64C(0xa600080922001410),
	U64C(0x4080020000800100),
	U64C(0x8300044120810002),
	U64C(0x6012800040012280),
	U64C(0x400401000402000),
	U64C(0x241002000410011),
	U64C(0xa2002040081201),
	U64C(0x1280800800040080),
	U64C(0x1042000402001008),
	U64C(0x404002118041092),
	U64C(0xa001802480004100),
	U64C(0x8190208000804014),
	U64C(0x401000c040012000),
	U64C(0x16820020420014),
	U64C(0x800090020100100),
	U64C(0x3010008000410),
	U64C(0x8002080110400420),
	U64C(0x800808002000100),
	U64C(0x825020024408409),
	U64C(0x8030802880004010),
	U64C(0x40010100208050),
	U64C(0x240104200208200),
	U64C(0x9043002100100408),
	U64C(0x208000500110008),
	U64C(0x27000300040008),
	U64C(0x8130088400020110),
	U64C(0x50088200110444),
	U64C(0xc0400020800080),
	U64C(0x402000401000),
	U64C(0xa100200101001048),
	U64C(0x1008008008801000),
	U64C(0xc01000413000800),
	U64C(0x20040080800200),
	U64C(0x10021004000801),
	U64C(0x8144910062000084),
	U64C(0x200248440048002),
	U64C(0x201000404001),
	U64C(0x200100020008080),
	U64C(0x6800801000800c),
	U64C(0x4080100850010),
	U64C(0x882020004008080),
	U64C(0x7840011042840008),
	U64C(0x4000045085220004),
	U64C(0x4482106008200),
	U64C(0x100209100400100),
	U64C(0x200100104100),
	U64C(0x1080010008180),
	U64C(0x4008004200040040),
	U64C(0x1003040080020080),
	U64C(0x9002108162080400),
	U64C(0x201040080410200),
	U64C(0x12c304103208001),
	U64C(0x8000110480284001),
	U64C(0x4000110020000c41),
	U64C(0x9023009000620409),
	U64C(0x8022002010080402),
	U64C(0x1000804008241),
	U64C(0x444802032c9004),
	U64C(0x5000004401002082)
};

const Bitboard Magic::bishopMagic[64] = {
	U64C(0x22823a0401041104),
	U64C(0x804100409003024),
	U64C(0x4810202000400),
	U64C(0x1004440088000144),
	U64C(0x204042040019080),
	U64C(0x1402111048004004),
	U64C(0xc000542404400022),
	U64C(0x4000220200a44004),
	U64C(0x49410408120043),
	U64C(0x41102202204200),
	U64C(0xb000043822084003),
	U64C(0x40424881a00),
	U64C(0x4040420010290),
	U64C(0x2108220600004),
	U64C(0x8002108410021000),
	U64C(0x20c04222100240),
	U64C(0x8222008304883),
	U64C(0x50800802080044),
	U64C(0x84809001020208),
	U64C(0x1000804110000),
	U64C(0x4204002210220000),
	U64C(0x1080201012011),
	U64C(0x80108004118a000),
	U64C(0x6040082008200),
	U64C(0x150a40028585042),
	U64C(0x2084142301404),
	U64C(0x2008040208104014),
	U64C(0x8208080100202020),
	U64C(0x402040002008201),
	U64C(0xa208008001100092),
	U64C(0x2002020104010122),
	U64C(0x8080888005004810),
	U64C(0x8008200910840860),
	U64C(0x140490580404a80a),
	U64C(0x650240400184803),
	U64C(0x100020080080080),
	U64C(0x400408020420200),
	U64C(0x4a01010201090041),
	U64C(0x42083c8081b10800),
	U64C(0x924e02006010d402),
	U64C(0x4104808010402),
	U64C(0x66c022104801040),
	U64C(0x8881040024082200),
	U64C(0x20404200800802),
	U64C(0xa080104001040),
	U64C(0x20381010a0200201),
	U64C(0x5448320084150200),
	U64C(0x48680080800024),
	U64C(0x181008221200010),
	U64C(0x1008090081004),
	U64C(0x896d80485c102031),
	U64C(0x2000110042020004),
	U64C(0x5082803041040),
	U64C(0x4000401082308131),
	U64C(0x40828214050002),
	U64C(0x4010214011412),
	U64C(0x52010c01240280),
	U64C(0x94404042105004),
	U64C(0x8000c00042080400),
	U64C(0x80000e84042e),
	U64C(0xa0c080060424408),
	U64C(0x4400b00810219201),
	U64C(0x8912220284110400),
	U64C(0x284100220540484)
};

static Bitboard indexToU64( int index, int bits, u64 m )
{
	Bitboard result = 0;
	for (int i = 0; i < bits; i++)
	{
		int j = m ? BitOp::getLSB( m ) : -1;
		if ( j < 0 )
			break;
		if (index & (1 << i)) result |= BitOp::oneShl( (Square)j );
		m &= BitOp::noneShl( (Square)j );
	}
	return result;
}

static u64 ratt(int sq, u64 block)
{
	u64 result = 0ULL;
	int rk = sq/8, fl = sq%8, r, f;
	for (r = rk+1; r <= 7; r++) {
		result |= (1ULL << (fl + r*8));
		if (block & (1ULL << (fl + r*8))) break;
	}
	for (r = rk-1; r >= 0; r--) {
		result |= (1ULL << (fl + r*8));
		if (block & (1ULL << (fl + r*8))) break;
	}
	for (f = fl+1; f <= 7; f++) {
		result |= (1ULL << (f + rk*8));
		if (block & (1ULL << (f + rk*8))) break;
	}
	for (f = fl-1; f >= 0; f--) {
		result |= (1ULL << (f + rk*8));
		if (block & (1ULL << (f + rk*8))) break;
	}
	return result;
}

static u64 batt(int sq, u64 block)
{
	u64 result = 0;
	int rk = sq/8, fl = sq%8, r, f;
	for (r = rk+1, f = fl+1; r <= 7 && f <= 7; r++, f++) {
		result |= (1ULL << (f + r*8));
		if (block & (1ULL << (f + r * 8))) break;
	}
	for (r = rk+1, f = fl-1; r <= 7 && f >= 0; r++, f--) {
		result |= (1ULL << (f + r*8));
		if (block & (1ULL << (f + r * 8))) break;
	}
	for (r = rk-1, f = fl+1; r >= 0 && f <= 7; r--, f++) {
		result |= (1ULL << (f + r*8));
		if (block & (1ULL << (f + r * 8))) break;
	}
	for (r = rk-1, f = fl-1; r >= 0 && f >= 0; r--, f--) {
		result |= (1ULL << (f + r*8));
		if (block & (1ULL << (f + r * 8))) break;
	}
	return result;
}

u32 Magic::initMagicPtrs( Square sq, bool bishop )
{
	i32 i, n;
	const u64 &mask = bishop ? bishopRelOcc[sq] : rookRelOcc[sq];
	n = BitOp::popCount(mask);

	// max 12 bits!
	Bitboard *b = new Bitboard[ 4096 ];
	Bitboard *a = new Bitboard[ (size_t)1u << n ];

	memset( a, 0, ((size_t)1u << n) * sizeof(Bitboard) );

	for (i = 0; i < (i32)(1 << n); i++) {
		b[i] = indexToU64(i, n, mask);

		i32 j;
		if ( bishop )
			j = (i32)((b[i] * bishopMagic[sq]) >> bishopShr[sq]);
		else
			j = (i32)((b[i] * rookMagic[sq]) >> rookShr[sq]);

		a[j] = bishop ? batt(sq, b[i]) : ratt(sq, b[i]);
	}
	if ( bishop )
	{
		bishopPtr[ sq ] = a;
	}
	else
	{
		rookPtr[ sq ] = a;
	}
	delete[] b;
	return 8 * (1<<n);
}

void Magic::init()
{
	initNewMovTab();

	for (uint i=0; i<64; i++)
	{
		i8 cur;
		// rook
		Bitboard msk = 0;
		cur = rmnew[ i ][ i ].next;
		while ( cur >= 0 )
		{
			if ( rmnew[ i ][ cur ].next != rmnew[ i ][ cur ].skip )
			{
				// relevant!
				msk |= BitOp::oneShl( cur );
			}
			cur = rmnew[ i ][ cur ].next;
		}
		rookRelOcc[ i ] = msk;
		rookShr[ i ] = (u8)(64-BitOp::popCount(msk));

		// bishop
		msk = 0;
		cur = bmnew[ i ][ i ].next;
		while ( cur >= 0 )
		{
			if ( bmnew[ i ][ cur ].next != bmnew[ i ][ cur ].skip )
			{
				// relevant!
				msk |= BitOp::oneShl( cur );
			}
			cur = bmnew[ i ][ cur ].next;
		}
		bishopRelOcc[ i ] = msk;
		bishopShr[ i ] = (u8)(64-BitOp::popCount(msk));
	}

	for (Square i=0; i<64; i++)
		for (uint j=0; j<2; j++)
			initMagicPtrs( i, j ? 1 : 0 );
}

void Magic::done()
{
	for (Square i=0; i<64; i++)
	{
		delete[] bishopPtr[i];
		delete[] rookPtr[i];
	}
}

}
