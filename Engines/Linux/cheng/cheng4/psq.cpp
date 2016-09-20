/*
You can use this program under the terms of either the following zlib-compatible license
or as public domain (where applicable)

  Copyright (C) 2014 Martin Sedlak

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

#include "psq.h"
#include <memory.h>
#include <stdlib.h>

namespace cheng4
{

// PSq

static TUNE_CONST i32 pieceScale = 256;		// Fixedpoint 8:8
static TUNE_CONST i32 psqScale = 256;		// Fixedpoint 8:8
static TUNE_CONST i32 psqScales[2][ptMax] =
{
	// opening
	{
	0,
	256,		// pawn
	256,		// knight
	256,		// bishop
	256,		// rook
	256,		// queen
	256			// king
	},
	// endgame
	{
	0,
	256,		// pawn
	256,		// knight
	256,		// bishop
	256,		// rook
	256,		// queen
	256			// king
	}
};

TUNE_CONST i16 PSq::materialTables[2][ptMax] =
{
	// opening
	{
		0, 75, 325, 325, 500, 975, 0
	},
	// endgame
	{
		0, 100, 325, 325, 500, 975, 0
	}
};

const i16 PSq::pawnTables[2][64] =
{
	// opening
	{
		+  0, +  0, +  0, +  0, +  0, +  0, +  0, +  0,
		+  0, +  0, +  0, +  0, +  0, +  0, +  0, +  0,
		+  0, +  0, +  0, +  0, +  0, +  0, +  0, +  0,
		+  0, +  0, +  0, +  0, +  0, +  0, +  0, +  0,
		+  0, +  0, +  0, + 10, + 10, +  0, +  0, +  0,
		+  0, +  0, +  0, +  5, +  5, +  0, +  0, +  0,
		+  0, +  0, +  0, +  0, +  0, +  0, +  0, +  0,
		+  0, +  0, +  0, +  0, +  0, +  0, +  0, +  0
	},
	// endgame
	{
		+  0, +  0, +  0, +  0, +  0, +  0, +  0, +  0,
		+  0, +  0, +  0, +  0, +  0, +  0, +  0, +  0,
		+  0, +  0, +  0, +  0, +  0, +  0, +  0, +  0,
		+  0, +  0, +  0, +  0, +  0, +  0, +  0, +  0,
		+  0, +  0, +  0, +  0, +  0, +  0, +  0, +  0,
		+  0, +  0, +  0, +  0, +  0, +  0, +  0, +  0,
		+  0, +  0, +  0, +  0, +  0, +  0, +  0, +  0,
		+  0, +  0, +  0, +  0, +  0, +  0, +  0, +  0
	}
};

const i16 PSq::knightTables[2][64] =
{
	// opening
	{
		- 20, - 10, - 10, - 10, - 10, - 10, - 10, - 20,
		- 10, +  0, +  0, +  0, +  0, +  0, +  0, - 10,
		- 10, +  0, + 10, + 10, + 10, + 10, +  0, - 10,
		- 10, +  0, + 10, + 15, + 15, + 10, +  0, - 10,
		- 10, +  0, + 10, + 15, + 15, + 10, +  0, - 10,
		- 10, +  0, + 10, + 10, + 10, + 10, +  0, - 10,
		- 10, +  0, +  0, +  0, +  0, +  0, +  0, - 10,
		- 20, - 10, - 10, - 10, - 10, - 10, - 10, - 20
	},
	// endgame
	{
		- 20, - 10, - 10, - 10, - 10, - 10, - 10, - 20,
		- 10, +  0, +  0, +  0, +  0, +  0, +  0, - 10,
		- 10, +  0, +  5, +  5, +  5, +  5, +  0, - 10,
		- 10, +  0, +  5, + 10, + 10, +  5, +  0, - 10,
		- 10, +  0, +  5, + 10, + 10, +  5, +  0, - 10,
		- 10, +  0, +  5, +  5, +  5, +  5, +  0, - 10,
		- 10, +  0, +  0, +  0, +  0, +  0, +  0, - 10,
		- 20, - 10, - 10, - 10, - 10, - 10, - 10, - 20
	}
};

const i16 PSq::bishopTables[2][64] =
{
	// opening
	{
		- 10, - 10, - 10, - 10, - 10, - 10, - 10, - 10,
		- 10, +  0, +  0, +  0, +  0, +  0, +  0, - 10,
		- 10, +  0, +  0, +  0, +  0, +  0, +  0, - 10,
		- 10, +  0, +  0, +  0, +  0, +  0, +  0, - 10,
		- 10, +  0, +  0, +  0, +  0, +  0, +  0, - 10,
		- 10, +  0, +  0, +  0, +  0, +  0, +  0, - 10,
		- 10, +  5, +  0, +  5, +  5, +  0, +  5, - 10,
		- 10, - 10, - 10, - 10, - 10, - 10, - 10, - 10
	},
	// endgame
	{
		- 10, - 10, - 10, - 10, - 10, - 10, - 10, - 10,
		- 10, +  0, +  0, +  0, +  0, +  0, +  0, - 10,
		- 10, +  0, +  5, +  5, +  5, +  5, +  0, - 10,
		- 10, +  0, +  5, +  5, +  5, +  5, +  0, - 10,
		- 10, +  0, +  5, +  5, +  5, +  5, +  0, - 10,
		- 10, +  0, +  5, +  5, +  5, +  5, +  0, - 10,
		- 10, +  0, +  0, +  0, +  0, +  0, +  0, - 10,
		- 10, - 10, - 10, - 10, - 10, - 10, - 10, - 10
	}
};

const i16 PSq::rookTables[2][64] =
{
	// opening
	{
		- 20, - 10, +  0, +  0, +  0, +  0, - 10, - 20,
		- 20, - 10, +  0, +  0, +  0, +  0, - 10, - 20,
		- 20, - 10, +  0, +  0, +  0, +  0, - 10, - 20,
		- 20, - 10, +  0, +  0, +  0, +  0, - 10, - 20,
		- 20, - 10, +  0, +  0, +  0, +  0, - 10, - 20,
		- 20, - 10, +  0, +  0, +  0, +  0, - 10, - 20,
		- 20, - 10, +  0, +  0, +  0, +  0, - 10, - 20,
		- 20, - 10, +  0, +  0, +  0, +  0, - 10, - 20
	},
	// endgame
	{
		+  0, +  0, +  0, +  0, +  0, +  0, +  0, +  0,
		+  0, +  0, +  0, +  0, +  0, +  0, +  0, +  0,
		+  0, +  0, +  0, +  0, +  0, +  0, +  0, +  0,
		+  0, +  0, +  0, +  0, +  0, +  0, +  0, +  0,
		+  0, +  0, +  0, +  0, +  0, +  0, +  0, +  0,
		+  0, +  0, +  0, +  0, +  0, +  0, +  0, +  0,
		+  0, +  0, +  0, +  0, +  0, +  0, +  0, +  0,
		+  0, +  0, +  0, +  0, +  0, +  0, +  0, +  0
	}
};

const i16 PSq::queenTables[2][64] =
{
	// opening
	{
		+  0, +  0, +  0, +  0, +  0, +  0, +  0, +  0,
		+  0, +  0, +  0, +  0, +  0, +  0, +  0, +  0,
		+  0, +  0, +  0, +  0, +  0, +  0, +  0, +  0,
		+  0, +  0, +  0, +  0, +  0, +  0, +  0, +  0,
		+  0, +  0, +  0, +  0, +  0, +  0, +  0, +  0,
		+  0, +  0, +  0, +  0, +  0, +  0, +  0, +  0,
		+  0, +  0, +  0, +  0, +  0, +  0, +  0, +  0,
		+  0, +  0, +  0, +  0, +  0, +  0, +  0, +  0
	},
	// endgame
	{
		+  0, +  0, +  0, +  0, +  0, +  0, +  0, +  0,
		+  0, +  0, +  0, +  0, +  0, +  0, +  0, +  0,
		+  0, +  0, +  0, +  0, +  0, +  0, +  0, +  0,
		+  0, +  0, +  0, +  0, +  0, +  0, +  0, +  0,
		+  0, +  0, +  0, +  0, +  0, +  0, +  0, +  0,
		+  0, +  0, +  0, +  0, +  0, +  0, +  0, +  0,
		+  0, +  0, +  0, +  0, +  0, +  0, +  0, +  0,
		+  0, +  0, +  0, +  0, +  0, +  0, +  0, +  0
	}
};

const i16 PSq::kingTables[2][64] =
{
	// opening
	{
		- 90, - 90, - 90, - 90, - 90, - 90, - 90, - 90,
		- 90, - 90, - 90, - 90, - 90, - 90, - 90, - 90,
		- 90, - 90, - 90, - 90, - 90, - 90, - 90, - 90,
		- 90, - 90, - 90, - 90, - 90, - 90, - 90, - 90,
		- 90, - 40, - 40, - 40, - 40, - 40, - 40, - 90,
		- 40, - 20, - 20, - 20, - 20, - 20, - 20, - 40,
		- 10, +  0, - 10, - 15, - 15, - 10, +  0, - 10,
		+  0, + 20, - 10, - 15, - 15, - 10, + 20, +  0
	},
	// endgame
	{
		- 30, - 20, - 20, - 20, - 20, - 20, - 20, - 30,
		- 20, +  0, +  0, +  0, +  0, +  0, +  0, - 20,
		- 20, +  0, + 15, + 15, + 15, + 15, +  0, - 20,
		- 20, +  0, + 15, + 25, + 25, + 15, +  0, - 20,
		- 20, +  0, + 15, + 25, + 25, + 15, +  0, - 20,
		- 20, +  0, + 15, + 15, + 15, + 15, +  0, - 20,
		- 20, +  0, +  0, +  0, +  0, +  0, +  0, - 20,
		- 30, - 20, - 20, - 20, - 20, - 20, - 20, - 30
	}
};

i16 PSq::tables[ 2 ][ 2 ][ ptMax ][ 64 ];

// init PSQ table
// (bakes material into it)
void PSq::init( u8 phase, Color color, Piece piece, const i16 * const material, const i16 * const table )
{
	assert( phase <= phEndgame && piece <= ptKing && piece >= ptPawn );
	assert( material && table );
	i32 base = (i32)material[ piece ];
	if ( piece > ptPawn )
	{
		base *= pieceScale;
		base /= 256;
	}
	for (Square sq = 0; sq < 64; sq++)
	{
		i16 &val = tables[ phase ][ color ][ piece ][ squareIndex( color, sq ) ];
		i32 res = base + table[sq];
		res *= psqScales[ phase ][ piece ];
		res /= 256;
		res *= psqScale;
		res /= 256;
		if ( color == ctBlack )
			res = -res;
		assert( res >= -32768 && res <= 32767 );
		val = (i16)res;
	}
}

void PSq::init()
{
	TUNE_EXPORT(i32, pieceScale, pieceScale);
	TUNE_EXPORT(i32, psqScale, psqScale);

	// individual scales
	TUNE_EXPORT(i32, psqScalePawnOpening,	psqScales[phOpening][ptPawn]);
	TUNE_EXPORT(i32, psqScaleKnightOpening, psqScales[phOpening][ptKnight]);
	TUNE_EXPORT(i32, psqScaleBishopOpening, psqScales[phOpening][ptBishop]);
	TUNE_EXPORT(i32, psqScaleRookOpening,	psqScales[phOpening][ptRook]);
	TUNE_EXPORT(i32, psqScaleQueenOpening,	psqScales[phOpening][ptQueen]);
	TUNE_EXPORT(i32, psqScaleKingOpening,	psqScales[phOpening][ptKing]);

	TUNE_EXPORT(i32, psqScalePawnEndgame,	psqScales[phEndgame][ptPawn]);
	TUNE_EXPORT(i32, psqScaleKnightEndgame, psqScales[phEndgame][ptKnight]);
	TUNE_EXPORT(i32, psqScaleBishopEndgame, psqScales[phEndgame][ptBishop]);
	TUNE_EXPORT(i32, psqScaleRookEndgame,	psqScales[phEndgame][ptRook]);
	TUNE_EXPORT(i32, psqScaleQueenEndgame,	psqScales[phEndgame][ptQueen]);
	TUNE_EXPORT(i32, psqScaleKingEndgame,	psqScales[phEndgame][ptKing]);

	memset( tables, 0, sizeof(tables) );
	for (Phase ph = phOpening; ph <= phEndgame; ph++ )
		for (Color c = ctWhite; c <= ctBlack; c++)
		{
			init( ph, c, ptPawn, materialTables[ph], pawnTables[ ph ] );
			init( ph, c, ptKnight, materialTables[ph], knightTables[ ph ] );
			init( ph, c, ptBishop, materialTables[ph], bishopTables[ ph ] );
			init( ph, c, ptRook, materialTables[ph], rookTables[ ph ] );
			init( ph, c, ptQueen, materialTables[ph], queenTables[ ph ] );
			init( ph, c, ptKing, materialTables[ph], kingTables[ ph ] );
		}
}

}
