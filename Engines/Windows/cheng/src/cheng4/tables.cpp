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

#include "tables.h"
#include <memory.h>
#include <algorithm>

// TODO: split init parts into more functions

namespace cheng4
{

// Tables

u8 Tables::lsBit16[ 65536 ];
u8 Tables::msBit16[ 65536 ];
u8 Tables::popCount16[ 65536 ];
u8 Tables::popCount8[ 256 ];
Bitboard Tables::oneShlTab[ 64 ];
Bitboard Tables::noneShlTab[ 256 ];
Bitboard Tables::kingAttm[ 64 ];
Bitboard Tables::knightAttm[ 64 ];
Bitboard Tables::pawnAttm[ 2 ][ 64 ];
bool Tables::neighbor[ 64 ][ 64 ];
u8 Tables::direction[ 64 ][ 64 ];
i8 Tables::advance[ dirMax ];
Bitboard Tables::between[ 64 ][ 64 ];
Bitboard Tables::ray[ 64 ][ 64 ];
Bitboard Tables::diagAttm[ 64 ];
Bitboard Tables::orthoAttm[ 64 ];
Bitboard Tables::queenAttm[ 64 ];
Bitboard Tables::passerMask[ 2 ][ 64 ];
Bitboard Tables::frontMask[ 2 ][ 64 ];
Bitboard Tables::isoMask[8];
Bitboard Tables::chainMask[ 64 ];
Bitboard Tables::outpostMask[ 2 ][ 64 ];
u8 Tables::moveValid[ 64 ][ 64 ];
u8 Tables::distance[ 64 ][ 64 ];
const Bitboard Tables::seventhRank[2] = {
	U64C(0x000000000000ff00),
	U64C(0x00ff000000000000)
};
const Bitboard Tables::eighthRank[2] = {
	U64C(0x00000000000000ff),
	U64C(0xff00000000000000)
};
Bitboard Tables::fileMask[8];
const u8 Tables::seeValue[] = { 0, 1, 3, 3, 5, 9, 99 };
const u8 Tables::npValue[] = { 0, 0, 3, 3, 5, 9, 0 };
const int Tables::sign[ctMax] = {1, -1};
// at least nonzero for king - safeguard
const u8 Tables::mvvValue[ ptMax ] = { 1, 1, 2, 2, 3, 4, 1 };
const u8 Tables::lvaValue[ ptMax ] = {0, 1, 2, 2, 3, 4, 0 };
const Score Tables::gainPromo[ ptMax ] = { 0, 0, 225, 225, 400, 875, 0 };	// promotion score gain table
const Score Tables::gainCap[ ptMax ] = { 100, 100, 325, 325, 500, 975, 0};	// capture score gain table

void Tables::init()
{
	// bitcount/lsbit/msbit tables
	for (u32 i=0; i<65536; i++)
	{
		uint bc = 0;
		for (uint j=0; j<16; j++)
			if ( i & (1 << j) )
				bc++;

		popCount16[ i ] = (u8)bc;
		if ( i < 256 )
			popCount8[ i ] = (u8)bc;

		for (uint j=0; j<16; j++)
		{
			if ( i & (1<<j) )
				lsBit16[i] = (u8)j;
			if ( i & (1<<(15-j)) )
				msBit16[i] = (u8)(15-j);
		}
	}

	memset( noneShlTab, 255, sizeof(noneShlTab) );

	// shift tables + king/knight attack masks
	for (Square i=0; i<64; i++)
	{
		oneShlTab[i] = U64C(1) << i;
		noneShlTab[i] = ~(U64C(1) << i);
		Bitboard b = oneShlTab[i];
		b |= (b << 1) & L1MASK;
		b |= (b >> 1) & R1MASK;
		b |= b >> 8;
		b |= b << 8;
		kingAttm[i] = b & noneShlTab[i];
		b = oneShlTab[i];
		Bitboard km = 0;
		km |= (b << 17) & L1MASK;
		km |= (b << 15) & R1MASK;
		km |= (b >> 17) & R1MASK;
		km |= (b >> 15) & L1MASK;
		km |= (b << 10) & L2MASK;
		km |= (b << 6) & R2MASK;
		km |= (b >> 10) & R2MASK;
		km |= (b >> 6) & L2MASK;
		knightAttm[i] = km;
	}

	// init neighbor table
	for (Square i=0; i<64; i++)
		for (Square j=0; j<64; j++)
			neighbor[ i ][ j ] = (oneShlTab[i] & kingAttm[j]) != 0;

	// init advance table
	advance[ dirNone	  ]	= +0;
	advance[ dirUp		]	= -8;
	advance[ dirDown	  ]	= +8;
	advance[ dirLeft	  ]	= -1;
	advance[ dirRight	 ]	= +1;
	advance[ dirUpLeft	]	= -9;
	advance[ dirUpRight	  ]	= -7;
	advance[ dirDownLeft  ]	= +7;
	advance[ dirDownRight ]	= +9;

	// init direction table
	for (Square i=0; i<64; i++)
	{
		File f0 = SquarePack::file( i );
		Rank r0 = SquarePack::rank( i );
		for (Square j=0; j<64; j++)
		{
			direction[ i ][ j ] = dirNone;
			if ( i == j )
				continue;
			File f1 = SquarePack::file( j );
			Rank r1 = SquarePack::rank( j );
			if ( r0 == r1 )
				direction[ i ][ j ] = (u8)( f1 > f0 ? dirRight : dirLeft );
			else if ( f0 == f1 )
				direction[ i ][ j ] = (u8)( r1 > r0 ? dirDown : dirUp );
			else
			{
				int dx = (int)f1 - (int)f0;
				int dy = (int)r1 - (int)r0;
				if ( abs(dx) != abs(dy) )
					continue;
				if ( dx < 0 && dy < 0 )
					direction[ i ][ j ] = dirUpLeft;
				else if ( dx > 0 && dy < 0 )
					direction[ i ][ j ] = dirUpRight;
				else if ( dx < 0 && dy > 0 )
					direction[ i ][ j ] = dirDownLeft;
				else
					direction[ i ][ j ] = dirDownRight;
			}
		}
	}

	// init between/ray masks
	for (Square i=0; i<64; i++)
	{
		for (Square j=0; j<64; j++)
		{
			between[i][j] = ray[i][j] = 0;
			u8 dir = direction[ i ][ j ];
			if ( dir == dirNone )
				continue;
			Bitboard mask = oneShlTab[i];
			Square s = i;
			while ( s != j )
			{
				s = (Square)(s+advance[ dir ]);
				if ( s < 64 )					// keep static analyzer happy
					mask |= oneShlTab[ s ];
			}
			between[ i ][ j ] = mask;

			// now ray
			Bitboard rm = oneShlTab[i];
			for (uint k=0; k<8; k++)
			{
				switch( dir )
				{
				case dirUp:
					rm |= (rm >> 8);
					break;
				case dirDown:
					rm |= (rm << 8);
					break;
				case dirLeft:
					rm |= (rm >> 1) & R1MASK;
					break;
				case dirRight:
					rm |= (rm << 1) & L1MASK;
					break;
				case dirUpLeft:
					rm |= (rm >> 9) & R1MASK;
					break;
				case dirDownRight:
					rm |= (rm << 9) & L1MASK;
					break;
				case dirUpRight:
					rm |= (rm >> 7) & L1MASK;
					break;
				case dirDownLeft:
					rm |= (rm << 7) & R1MASK;
					break;
				}
			}
			rm &= noneShlTab[i];
			ray[ i ][ j ] = rm;
			assert( between[i][j] & ray[i][j] );
		}
	}

	// pawn attack mask
	for (Square i=0; i<64; i++)
	{
		Bitboard b = oneShlTab[i];
		Bitboard m;
		m = (b >> 9) & R1MASK;
		m |= (b >> 7) & L1MASK;
		pawnAttm[ ctWhite ][ i ] = m;
		m = (b << 9) & L1MASK;
		m |= (b << 7) & R1MASK;
		pawnAttm[ ctBlack ][ i ] = m;
	}

	// diagonal/orthogonal attack masks
	for (Square i=0; i<64; i++)
	{
		Bitboard m = 0;
		Bitboard bit = oneShlTab[i];
		Bitboard b;
		b = bit;
		for (uint j=0; j<8; j++)
		{
			b |= (b >> 9) & R1MASK;
			b |= (b << 9) & L1MASK;
		}
		m |= b;
		b = bit;
		for (uint j=0; j<8; j++)
		{
			b |= (b >> 7) & L1MASK;
			b |= (b << 7) & R1MASK;
		}
		m |= b;
		m &= ~bit;
		diagAttm[ i ] = m;

		m = 0;
		b = bit;
		for (uint j=0; j<8; j++)
		{
			b |= (b << 1) & L1MASK;
			b |= (b >> 1) & R1MASK;
		}
		m |= b;
		b = bit;
		for (uint j=0; j<8; j++)
		{
			b |= b >> 8;
			b |= b << 8;
		}
		m |= b;
		m &= ~bit;
		orthoAttm[ i ] = m;
		queenAttm[ i ] = m | diagAttm[i];
	}

	// init movevalid table
	for (Square i=0; i<64; i++)
		for (Square j=0; j<64; j++)
		{
			u8 &m = moveValid[i][j];
			m = 0;
			if ( i == j )
				continue;
			if ( neighbor[i][j] )
				m |= 1 << ptKing;
			if ( knightAttm[i] & oneShlTab[j] )
				m |= 1 << ptKnight;
			if ( diagAttm[i] & oneShlTab[j] )
				m |= 1 << ptBishop;
			if ( orthoAttm[i] & oneShlTab[j] )
				m |= 1 << ptRook;
			if ( queenAttm[i] & oneShlTab[j] )
				m |= 1 << ptQueen;
		}

	// init passerMask table
	for ( Color c=ctWhite; c <= ctBlack; c++ )
		for ( Square sq = 0; sq < 64; sq++ )
		{
			Bitboard b = oneShlTab[sq];
			if ( c == ctWhite )
			{
				// white
				b >>= 8;
				b |= (b >> 1) & R1MASK;
				b |= (b << 1) & L1MASK;
				for (int i=0; i<8; i++)
					b |= b >> 8;
			}
			else
			{
				// black
				b <<= 8;
				b |= (b >> 1) & R1MASK;
				b |= (b << 1) & L1MASK;
				for (int i=0; i<8; i++)
					b |= b << 8;
			}
			passerMask[c][sq] = b;
		}

	// init frontMask table
	for ( Color c=ctWhite; c <= ctBlack; c++ )
		for ( Square sq = 0; sq < 64; sq++ )
		{
			Bitboard b = oneShlTab[sq];
			for (uint i=0; i<8; i++)
				b |= BitOp::shiftForward(c, b);
			b &= noneShlTab[sq];
			frontMask[c][sq] = b;
		}

	// init outpostMask table
	for ( Color c=ctWhite; c <= ctBlack; c++ )
		for ( Square sq = 0; sq < 64; sq++ )
		{
			Bitboard b = pawnAttm[c][sq];
			for (uint i=0; i<8; i++)
				b |= BitOp::shiftForward(c, b);
			outpostMask[c][sq] = b;
		}
	// init fileMask table
	for ( File f = AFILE; f <= HFILE; f++ )
	{
		Bitboard b = oneShlTab[(Square)f];
		for ( uint i=0; i<8; i++ )
			b |= b << 8;
		fileMask[f] = b;
	}
	// init distance table
	for ( Square i = 0; i < 64; i++ )
	{
		File fi = SquarePack::file( i );
		Rank ri = SquarePack::rank( i );
		for ( Square j = 0; j < 64; j++ )
		{
			File fj = SquarePack::file( j );
			Rank rj = SquarePack::rank( j );
			int dx = (int)fj - (int)fi;
			int dy = (int)rj - (int)ri;
			int adx = abs(dx);
			int ady = abs(dy);
			int sqd = std::min( adx, ady );
			int dist = sqd + (adx - sqd) + (ady - sqd);
			distance[ i ][ j ] = (u8)dist;
		}
	}
	// init isoPawnMask
	for ( File i = 0; i<8; i++ )
	{
		Bitboard tmp = fileMask[i];
		isoMask[i] = BitOp::shiftLeft(tmp) | BitOp::shiftRight(tmp);
	}
	// init chainMask
	for ( Square i = 0; i<64; i++ )
	{
		Bitboard tmp = BitOp::oneShl(i);
		Bitboard b = BitOp::shiftLeft(tmp) | BitOp::shiftRight(tmp);
		b |= b << 8;
		b |= b >> 8;
		chainMask[i] = b;
	}
}

// BitOp

bool BitOp::hwPopCnt = 0;

// disable hardware popcount
void BitOp::disableHwPopCount()
{
	hwPopCnt = 0;
}

// static init (detects hw popcount)
void BitOp::init()
{
#ifdef _MSC_VER
	int id[4] = {0};
	__cpuid(id, 0);
	int nids = id[0];
	if ( nids >= 2 )
	{
		id[2] = 0;
		__cpuid(id, 1);
		hwPopCnt = (id[2] & 0x800000) != 0;
	}
#elif defined(__GNUC__) && !defined(__ANDROID__)
	int id[4] = {0};
	asm(
		"cpuid":
		"=a" (id[0]),
		"=b" (id[1]),
		"=c" (id[2]),
		"=d" (id[3]) :
		"a" (0)
	);
	int nids = id[0];
	if ( nids >= 2 )
	{
		id[2] = 0;
		asm(
			"cpuid":
			"=a" (id[0]),
			"=b" (id[1]),
			"=c" (id[2]),
			"=d" (id[3]) :
			"a" (1)
		);
		hwPopCnt = (id[2] & 0x800000) != 0;
	}
#endif
}

}
