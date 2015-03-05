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

#include "psq.h"
#include <memory.h>
#include <stdlib.h>

namespace cheng4
{

// PSq

TUNE_CONST i16 PSq::materialTables[phMax][ptMax] =
{
	// opening
	{
		0, 65, 338, 333, 387, 978, 0
	},
	// endgame
	{
		0, 101, 329, 335, 527, 976, 0
	}
};

TUNE_CONST i16 PSq::pawnTables[2][64] =
{
	// opening
	{
		+  0, +  0, +  0, +  0, +  0, +  0, +  0, +  0,
		 112,   92,  115,   11,   -1,  -42,  -33, -142,
		  38,   52,   29,   -1,   60,   89,   20,   71,
		  15,    6,    6,	21,   20,   17,  -28,    8,
		   5,   12,   11,   13,   12,    5,  -14,    0,
		  -5,    2,    6,   -2,    7,  -25,    2,    6,
		  -4,   -7,  -11,  -19,   -8,   15,    7,   -8,
		+  0, +  0, +  0, +  0, +  0, +  0, +  0, +  0
	},
	// endgame
	{
		+  0, +  0, +  0, +  0, +  0, +  0, +  0, +  0,
		  24,   36,   23,   -5,    1,   -7,   26,   19,
		  36,   23,   13,   -7,  -16,   -1,   14,   18,
		  19,    7,    3,  -12,   -9,  -10,    7,    6,
		   5,    4,   -7,  -14,   -9,   -3,   -1,   -4,
		  -3,   -1,   -4,  -12,   -4,   -2,   -2,   -1,
		   4,    4,   -2,  -19,    5,   11,    7,   -8,
		+  0, +  0, +  0, +  0, +  0, +  0, +  0, +  0
	}
};

TUNE_CONST i16 PSq::knightTables[2][64] =
{
	// opening
	{
		-163, -112,   -9,   48,   88,   -8,    3,  -46,
		 -10,    2,   13,   22,    2,   93,   -1,  -10,
		  -7,   -2,   20,   34,  104,  116,   40,  -20,
		  19,   10,   32,   42,   14,   54,    1,   45,
		  -4,   20,   11,   15,   33,   35,   49,   10,
		 -24,    1,   -2,   17,   27,   -1,    3,   -9,
		 -13,  -17,  -14,   10,    5,   41,  -36,    3,
		 -56,  -20,  -41,   -6,  -29,   -3,   -8,  -30
	},
	// endgame
	{
		 -69,  -14,    6,   14,   -4,   13,    2,  -54,
		   0,    9,   10,   26,   14,   10,  -14,   -6,
		  -4,   15,   15,   12,   15,   11,   16,    6,
		  15,    4,   17,   21,   28,   25,   22,   11,
		   6,    7,   14,   23,   25,   18,   11,    6,
		 -12,   -6,   -7,    6,    5,   -2,    0,   -8,
		 -38,  -12,  -13,   -7,   -4,  -18,   -2,  -10,
		 -45,  -12,  -25,  -13,  -10,  -20,  -17,  -45
	}
};

TUNE_CONST i16 PSq::bishopTables[2][64] =
{
	// opening
	{
		 -13,  -42,  -56,  -51,  -14,  -20,  -62,   67,
		 -25,  -47,    0,   -5,  -43,  -17,  -49,  -52,
		 -16,   -7,  -20,  -18,   24,   77,   32,    5,
		 -41,   -4,   -7,   35,    7,    7,   -3,   -8,
		  -8,    0,   -2,   23,   16,   -7,    6,    7,
		 -13,   -8,    8,    2,    8,    0,   -4,    7,
		   4,   -4,   12,   -9,   -2,   19,   16,    7,
		  -6,   -1,  -10,  -16,  -13,  -22,    6,  -32
	},
	// endgame
	{
		  20,    8,    7,    4,   -2,   -2,    2,   12,
		  -2,    7,    0,    1,    0,   -4,    8,  -14,
		   2,    1,    4,    5,    9,   16,   22,    6,
		  -2,    3,    9,   14,   17,   15,    9,   11,
		  -3,   -2,    9,   17,   11,    9,    7,    7,
		  -8,    5,    9,    5,   15,    2,    0,   -8,
		  -3,  -13,    3,    3,   -2,    0,    0,  -15,
		  -7,   -3,  -10,   -7,  -17,  -11,   -5,  -19
	}
};

TUNE_CONST i16 PSq::rookTables[2][64] =
{
	// opening
	{
		  46,   16,   12,    0,   -2,  115,   32,  152,
		  47,   24,   72,   87,   61,   58,   13,   53,
		  29,   79,   73,   81,  143,  121,  187,   74,
		  -3,   18,   40,   54,   61,   64,   64,   32,
		 -13,   -3,   11,    5,   -4,  -17,   15,    8,
		 -27,   -6,    1,   -4,    3,  -20,   -9,  -32,
		 -41,  -18,  -10,  -16,   -7,  -10,  -33,  -59,
		 -14,   -2,    2,    4,    4,   -6,  -17,  -23
	},
	// endgame
	{
		  -7,    9,    9,    6,    8,   10,   15,    8,
		  11,   18,   21,   20,   17,   18,   14,   12,
		  16,   17,   22,   21,   14,   20,   12,   13,
		  20,   21,   20,   20,   18,   23,   18,   16,
		  13,   13,   13,    8,    7,   10,    8,    1,
		  -3,   -4,    2,    0,    0,   -5,   -3,   -8,
		  -6,   -3,   -3,   -6,   -4,   -7,   -7,   -3,
		  -7,   -7,   -3,   -4,   -9,    2,   -8,  -19
	}
};

TUNE_CONST i16 PSq::queenTables[2][64] =
{
	// opening
	{
		 -15,   -9,    0,   35,   51,  159,  163,   97,
		  -4,  -30,    7,    2,   16,   31,   41,   88,
		   5,    0,    7,   30,   56,  103,  121,   19,
		   0,   -9,    1,   10,   18,   55,   55,   18,
		 -13,    0,    0,    1,   15,    5,   31,   26,
		  -7,   -7,    5,   -2,    7,    6,   14,    8,
		  -5,  -16,    1,   11,   10,    0,  -20,   -5,
		 -13,  -10,   -6,   -4,    2,  -56,  -11,  -48
	},
	// endgame
	{
		 -11,   -1,    8,    7,   21,   23,   19,   21,
		   3,    8,   14,   28,   29,   34,   37,   34,
		   3,    9,   18,   28,   28,   40,   30,   39,
		   7,   16,   21,   25,   28,   39,   38,   33,
		   4,    9,    7,   27,   16,   22,   25,   22,
		  -4,   13,   11,    1,    9,   12,    6,   -6,
		 -14,    0,   -5,  -10,   -9,  -20,  -41,   -5,
		 -12,  -30,  -21,  -10,  -17,  -33,  -47,  -40
	}
};

TUNE_CONST i16 PSq::kingTables[2][64] =
{
	// opening
	{

		 -92,  -89,  -86,  -90,  -58,  -92,  -87,  -92,
		 -88,  -93,  -89,  -88,  -93,  -86,  -87,   17,
		 -90,  -90,  -85,  -90,  -84,  -84,  -67,  -88,
		 -89,  101,  -86,  -87,  -95,  -89,  -37,  -89,
		 -95,   -5,  -23, -107,  -40,  -40,  -37,  -90,
		 -40,  -12,  -21,  -29,  -52,  -27,   10,  -29,
		   4,  -45,  -21,  -60,  -38,  -19,    0,   18,
		 -22,   10,   -1,  -84,    4,  -52,   18,   15
	},
	// endgame
	{
		-123,   -3,   -2,   -4,    6,    8,   29,  -78,
		 -42,   -3,   17,    8,   34,   38,   23,  -11,
		 -18,   20,   30,   30,   37,   49,   43,   19,
		  -7,   20,   23,   34,   36,   39,   35,   10,
		 -19,    4,   20,   27,   26,   27,   15,   -9,
		 -28,   -2,    9,   19,   21,   13,    2,  -14,
		 -20,   -7,    0,   -3,    3,    2,   -4,  -17,
		 -50,  -25,  -14,  -25,  -39,  -26,  -20,  -41
	}
};

i16 PSq::tables[ phMax ][ ctMax ][ ptMax ][ 64 ];

// init PSQ table
// (bakes material into it)
void PSq::init( u8 phase, Color color, Piece piece, const i16 * const material, const i16 * const table )
{
	assert( phase <= phEndgame && piece <= ptKing && piece >= ptPawn );
	assert( material && table );
	i32 base = (i32)material[ piece ];
	for (Square sq = 0; sq < 64; sq++)
	{
		i16 &val = tables[ phase ][ color ][ piece ][ squareIndex( color, sq ) ];
		i32 res = base + table[sq];
		if ( color == ctBlack )
			res = -res;
		assert( res >= -32768 && res <= 32767 );
		val = (i16)res;
	}
}

void PSq::init()
{
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
