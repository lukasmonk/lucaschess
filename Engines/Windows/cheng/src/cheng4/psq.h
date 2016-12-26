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
#include "tune.h"

namespace cheng4
{

struct PSq
{
	// default tables
	static TUNE_CONST i16 materialTables[2][ptMax];

	static TUNE_CONST i16 pawnTables[2][64];
	static TUNE_CONST i16 knightTables[2][64];
	static TUNE_CONST i16 bishopTables[2][64];
	static TUNE_CONST i16 rookTables[2][64];
	static TUNE_CONST i16 queenTables[2][64];
	static TUNE_CONST i16 kingTables[2][64];

	// [phase][color][piece][square]
	static i16 tables[ 2 ][ 2 ][ ptMax ][ 64 ];

	// init PSQ tables (using default values)
	static void init();

	// init PSQ table
	// (bakes material into it)
	static void init( u8 phase, Color color, Piece piece, const i16 * const material, const i16 * const table );

	// psq-index based on color and square
	template< Color color > static inline Square index( Square sq )
	{
		return color == ctWhite ? sq : sq ^ 0x38;
	}
	// function version
	static inline Square squareIndex( Color c, Square sq )
	{
		return c == ctWhite ? index<ctWhite>(sq) : index<ctBlack>(sq);
	}
};

}
