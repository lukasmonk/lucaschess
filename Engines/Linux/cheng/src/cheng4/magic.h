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

#include "tables.h"
#include "chtypes.h"

namespace cheng4
{

// singleton
class Magic
{
protected:
	static u32 initMagicPtrs( Square sq, bool bishop );
	// rook/bishop relevant occupancy masks
	static Bitboard rookRelOcc[64];
	static Bitboard bishopRelOcc[64];
	// rook/bishop magic shr
	static u8 rookShr[64];
	static u8 bishopShr[64];
	// rook/bishop magic pointers
	static const Bitboard *rookPtr[64];
	static const Bitboard *bishopPtr[64];
	// rook/bishop magic multipliers
	static const Bitboard rookMagic[64];
	static const Bitboard bishopMagic[64];
public:
	static void init();
	static void done();

	// occ: block mask
	static inline Bitboard rookAttm( Square sq, Bitboard occ )
	{
		return rookPtr[ sq ][ (rookMagic[ sq ] * (occ & rookRelOcc[ sq ])) >> rookShr[ sq ] ];
	}

	// occ: block mask
	static inline Bitboard bishopAttm( Square sq, Bitboard occ )
	{
		return bishopPtr[ sq ][ (bishopMagic[ sq ] * (occ & bishopRelOcc[ sq ])) >> bishopShr[ sq ] ];
	}

	// occ: block mask
	static inline Bitboard queenAttm( Square sq, Bitboard occ )
	{
		return rookAttm( sq, occ ) | bishopAttm( sq, occ );
	}

	template< Piece pt > static inline Bitboard sliderAttm( Square sq, Bitboard occ )
	{
		assert( isSlider( pt ) );
		if ( pt == ptBishop )
			return bishopAttm( sq, occ );
		else if ( pt == ptRook )
			return rookAttm( sq, occ );
		else if ( pt == ptQueen )
			return queenAttm( sq, occ );
		else return 0;
	}

};

}
