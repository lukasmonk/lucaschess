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

#pragma once

#include "chtypes.h"

#ifndef __ANDROID__
#	define USE_POPCNT
#endif

#if defined(_MSC_VER)
#	include <intrin.h>
#endif

namespace cheng4
{

enum PopCountMode
{
	pcmNormal,
	pcmHardware
};

// files
enum Files
{
	AFILE, BFILE, CFILE, DFILE, EFILE, FFILE, GFILE, HFILE
};

// ranks
enum Ranks
{
	RANK8, RANK7, RANK6, RANK5, RANK4, RANK3, RANK2, RANK1
};

// squares
enum Squares
{
	A8, B8, C8, D8, E8, F8, G8, H8,
	A7, B7, C7, D7, E7, F7, G7, H7,
	A6, B6, C6, D6, E6, F6, G6, H6,
	A5, B5, C5, D5, E5, F5, G5, H5,
	A4, B4, C4, D4, E4, F4, G4, H4,
	A3, B3, C3, D3, E3, F3, G3, H3,
	A2, B2, C2, D2, E2, F2, G2, H2,
	A1, B1, C1, D1, E1, F1, G1, H1
};

// square-color masks
static const Bitboard lightSquares = U64C( 0xaa55aa55aa55aa55 );
static const Bitboard darkSquares = U64C( 0x55aa55aa55aa55aa );
static const Bitboard pawnPromoSquares = U64C( 0xff000000000000ff );

// shift-adjust masks
static const Bitboard L1MASK = U64C( 0xfefefefefefefefe );
static const Bitboard L2MASK = U64C( 0xfcfcfcfcfcfcfcfc );
static const Bitboard R1MASK = U64C( 0x7f7f7f7f7f7f7f7f );
static const Bitboard R2MASK = U64C( 0x3f3f3f3f3f3f3f3f );

struct BitOp;

// singleton
struct Tables
{
	static Bitboard oneShlTab[64];
	static Bitboard noneShlTab[256];		// FIXME: 256 instead of 64 due to silly gcc compiler warning?
	static Bitboard kingAttm[64];
	static Bitboard knightAttm[64];
	static Bitboard pawnAttm[2][64];
	static bool neighbor[64][64];
	static Bitboard between[64][64];
	static Bitboard ray[64][64];			// semi-infinite ray [from][to], goes beyond to
	static Bitboard diagAttm[64];			// diagonal pseudo attack mask
	static Bitboard orthoAttm[64];			// orthogonal pseudo attack mask
	static Bitboard queenAttm[64];			// queen pseudo attack mask
	static Bitboard passerMask[2][64];		// passer mask [color][square]
	static Bitboard frontMask[2][64];		// in front mask [color][square]
	static Bitboard isoMask[8];				// isolated mask [file]
	static Bitboard chainMask[64];			// chained mask [file]
	static Bitboard outpostMask[2][64];		// outpost mask [color][square]: if there are no opp pawns in this mask
											// then it may be an outpost
	static u8 direction[64][64];			// [from][to]
	static u8 moveValid[64][64];			// move valid: [from][to] & (1<<piece_type) (doesn't work for pawns)
	static u8 distance[64][64];				// distance between squares (0-8)
	static const Bitboard seventhRank[2];	// seventh rank mask
	static const Bitboard eighthRank[2];	// eighth rank mask
	static Bitboard fileMask[8];			// file mask
	static i8 advance[ dirMax ];
	static const u8 seeValue[ ptMax ];		// pawn = 1 ... queen = 9
	static const u8 mvvValue[ ptMax ];		// none = 1 (enpassant), pawn = 1 ... queen = 5
	static const u8 lvaValue[ ptMax ];		// pawn = 1 ..
	static const Score gainPromo[ ptMax ];	// promotion score gain table
	static const Score gainCap[ ptMax ];	// capture score gain table

	// non-pawn material value
	static const u8 npValue[ ptMax ];		// pawn = 0 ... queen = 9

	static const int sign[ ctMax ];			// ctWhite = 1, ctBlack = -1

	// init tables
	static void init();

	template< Piece pt > static inline Bitboard sliderAttm( Square sq )
	{
		assert( isSlider( pt ) );
		if ( pt == ptBishop )
			return diagAttm[ sq ];
		if ( pt == ptRook )
			return orthoAttm[ sq ];
		if ( pt == ptQueen )
			return queenAttm[ sq ];
		return 0;
	}

protected:
	friend struct BitOp;
	// pop counts for each 16-bit parts
	static u8 popCount16[ 65536 ];
	static u8 popCount8[ 256 ];
	// lsb/msb stuff
	static u8 lsBit16[ 65536 ];
	static u8 msBit16[ 65536 ];
};

#if (defined(__GNUC__) && (defined(__LP64__) || defined(__x86_64__))) || (defined(_MSC_VER) && (defined(_M_AMD64) || defined(_M_X64)))
#	define IS_X64 1
#endif

// singleton
struct BitOp
{
	// TODO: rename to something more logical
	static inline Bitboard oneShl( Square x )
	{
		assert( !( x & ~63 ) );
#ifdef IS_X64
		return U64C(1) << x;
#else
		return Tables::oneShlTab[ x ];
#endif
	}

	static inline Bitboard noneShl( Square x )
	{
		assert( !( x & ~63 ) );
#ifdef IS_X64
		return ~(U64C(1) << x);
#else
		return Tables::noneShlTab[ x ];
#endif
	}

	static inline uint getLSB( u64 m )
	{
		assert( m );
#ifndef _MSC_VER
	#if defined(__GNUC__) && defined(__i386__)
		i32 res;
		asm(
			"bsfl (%1),%%eax;"
			"jnz 0f;"
			"bsfl 4(%1),%%eax;"
			"orl $0x20,%%eax;"
			"0:movl %%eax,%0" :
			"=r" (res) :
			"r" (&m) :
		"cc", "eax"
		);
		return res;
	#elif defined(__GNUC__) && defined(IS_X64)
		u64 res;
		asm(
			"bsfq %1, %0" :
			"=r" (res) :
			"rm" (m) : "cc"
		);
		return (uint)(res & 0xffffffffu);
//		return __builtin_ffsl(m);	 // FIXME: doesn't work here => I wonder why?
	#else
		for (uint i=0; m && i<4*16; i+=16, m >>= 16)
		{
			u16 p = (u16)m;
			if ( p )
				return Tables::lsBit16[ p ] + i;
		}
		return 0u-1u;
	#endif
#else
	#if defined ( _M_AMD64 ) || defined ( _M_X64 )
		unsigned long index;
		_BitScanForward64(&index, m);
		return index;
	#else
		__asm
		{
			bsf eax, dword ptr [m]
			jnz done
			bsf eax, dword ptr [m+4]
			or eax, 32
done:
		}
	#endif
#endif
	}

	static inline Square popBit( u64 &m )
	{
		assert( m );
		Square res = (Square)getLSB(m);
		m &= m - 1;
		return res;
	}

	static inline uint getMSB( u64 m )
	{
		assert( m );
#ifndef _MSC_VER
	#if defined(__GNUC__) && defined(__i386__)
		u32 res;
		asm(
			"bsrl 4(%1),%%eax;"
			"jnz 1f;"
			"bsrl (%1),%%eax;"
			"jnz 0f;"
			"1:orl $0x20,%%eax;"
			"0:movl %%eax,%0" :
		"=r" (res) :
		"r" (&m) :
		"cc", "eax"
			);
		return (uint)res;
	#elif defined(__GNUC__) && defined(IS_X64)
		u64 res;
		asm(
			"bsrq %1, %0" :
			"=r" (res) :
			"rm" (m) : "cc"
		);
		return (uint)(res & 0xffffffffu);
	#else
		for (uint i=3*16; m; i-=16, m <<= 16)
		{
			u16 p = (u16)((m >> 48) & 0xffffu);
			if ( p )
				return (uint)(Tables::msBit16[ p ] + i);
		}
		return (uint)(0u-1u);
	#endif
#else
	#if defined ( _M_AMD64 ) || defined ( _M_X64 )
		unsigned long index;
		unsigned char isNonzero;
		isNonzero = _BitScanReverse64(&index, m);
		return (uint)(index & 0xffffffffu);
	#else
		__asm
		{
			bsr eax, dword ptr [m+4]	//edi
			jnz done1
			bsr eax, dword ptr [m]		//esi
			jnz done
done1:
			or eax, 32
done:
		}
	#endif
#endif
	}

	// return pop count for a 64-bit rep
	template< PopCountMode pcm > static inline uint popCount( u64 val )
	{
#ifdef USE_POPCNT
		if ( pcm == pcmHardware )
		{
		#ifdef _MSC_VER
			#if defined ( _M_AMD64 ) || defined ( _M_X64 )
				// stupid Intel is slower using __popcnt64!!!
				#if defined(__INTEL_COMPILER)
					return (uint)_mm_popcnt_u64( val );
				#else
					return (uint)__popcnt64( val );
				#endif
			#else
				// stupid Intel is slower using __popcnt64!!!
				#if defined(__INTEL_COMPILER)
					return (uint)(_mm_popcnt_u32( (u32)(val & 0xffffffffu) ) + _mm_popcnt_u32( (u32)(val >> 32) ));
				#else
					return (uint)(__popcnt( (u32)(val & 0xffffffffu) ) + __popcnt( (u32)(val >> 32) ));
				#endif
			#endif
		#elif defined(__GNUC__)
			#if defined(IS_X64)
				u64 res;
				asm(
					"popcnt %1, %0" :
					"=r" (res) :
					"rm" (val) : "cc"
				);
				return (uint)(res & 0xffffffffu);
			#else
				u32 res, res2;
				asm(
					"popcnt %1, %0" :
					"=r" (res) :
					"rm" ((u32)(val & 0xffffffffu)) : "cc"
				);
				asm(
					"popcnt %1, %0" :
					"=r" (res2) :
					"rm" ((u32)(val >> 32)) : "cc"
				);
				return (uint)(res+res2);
			#endif
		#else
			assert( 0 && "HW popcount not supported" );
			return 0;
		#endif
		}
		else
#endif
		{
			return (uint)(
				Tables::popCount16[ val & 65535 ] +
				Tables::popCount16[ (val >> 16) & 65535 ] +
				Tables::popCount16[ (val >> 32) & 65535 ] +
				Tables::popCount16[ (val >> 48) & 65535 ]);
		}
	}

	static inline uint popCount( u64 val )
	{
		return popCount< pcmNormal >( val );
	}

	// shift bitboard one rank forward
	template< Color c > static inline Bitboard shiftForward( Bitboard b )
	{
		return c == ctWhite ? b >> 8 : b << 8;
	}
	static inline Bitboard shiftForward( Color c, Bitboard b )
	{
		return c == ctWhite ? shiftForward<ctWhite>(b) : shiftForward<ctBlack>(b);
	}
	// shift bitboard one rank backward
	template< Color c > static inline Bitboard shiftBackward( Bitboard b )
	{
		return c != ctWhite ? b >> 8 : b << 8;
	}
	static inline Bitboard shiftBackward( Color c, Bitboard b )
	{
		return c == ctWhite ? shiftBackward<ctWhite>(b) : shiftBackward<ctBlack>(b);
	}
	// shift bitboard one file right (no wraparound)
	static inline Bitboard shiftRight( Bitboard b )
	{
		return (b << 1) & L1MASK;
	}
	// shift bitboard one file left (no wraparound)
	static inline Bitboard shiftLeft( Bitboard b )
	{
		return (b >> 1) & R1MASK;
	}

	// has hardware popcount?
	static inline bool hasHwPopCount()
	{
		return hwPopCnt;
	}

	// disable hardware popcount
	static void disableHwPopCount();

	// static init (detects hw popcount)
	static void init();


private:
	static bool hwPopCnt;
};

}
