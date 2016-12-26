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
		0, 61, 327, 309, 435, 1063, 0
	},
	// endgame
	{
		0, 98, 337, 339, 539, 999, 0
	}
};

TUNE_CONST i16 PSq::pawnTables[2][64] =
{
	// opening
	{
		+  0, +  0, +  0, +  0, +  0, +  0, +  0, +  0,
		 132,   97,   99,  109,   11,    6,  -98, -100,
		  -1,    5,   37,   22,   65,   96,    2,   38,
		  -5,   11,    4,   20,   32,   28,    2,    0,
		  -1,    1,   12,   18,   15,   18,   -5,    8,
		   0,    8,    0,   -4,   15,   -2,    6,   -3,
		  -5,  -13,  -12,   -7,  -10,   15,   11,   -9,
		+  0, +  0, +  0, +  0, +  0, +  0, +  0, +  0
	},
	// endgame
	{
		+  0, +  0, +  0, +  0, +  0, +  0, +  0, +  0,
		  38,   47,   38,    7,    8,   11,   38,   14,
		  37,   33,   16,  -14,  -10,    5,   12,    7,
		  23,   12,    1,  -18,   -7,   -7,    6,    1,
		  10,    5,   -7,   -8,   -9,   -1,    2,   -6,
		   3,   -1,   -6,   -9,   -2,    1,    1,   -7,
		   9,    4,   -2,   -4,    3,    9,    4,   -8,
		+  0, +  0, +  0, +  0, +  0, +  0, +  0, +  0
	}
};

TUNE_CONST i16 PSq::knightTables[2][64] =
{
	// opening
	{
		-183, -111,  -57,  -13,   91,  -83,   -9, -145,
		 -25,  -23,   21,   22,    1,   26,   -5,  -28,
		 -52,  -12,   12,   24,   86,   90,   12,  -33,
		  -9,    3,   16,   31,  -13,   35,  -12,   35,
		  -4,   14,   13,    4,   13,   19,   27,    2,
		 -17,   -4,   -4,   11,   30,    0,    9,   -6,
		 -10,   -6,    0,   10,    8,   13,  -15,    0,
		  14,  -21,   -9,    2,    6,   14,  -11,  -60
	},
	// endgame
	{
		 -18,   -5,    6,    6,   19,    4,    2,  -55,
		   9,   13,    8,   22,   19,   -2,    1,    0,
		   1,    3,   14,   19,   12,   16,    8,    6,
		   7,    8,   20,   23,   21,   27,   22,   16,
		  -1,   12,   18,   16,   25,   19,   16,    6,
		  -26,  -5,   -2,   14,   11,   -2,    3,   -5,
		  -28, -10,  -13,   -5,    0,   -9,   -4,   -7,
		  -41, -24,  -16,  -15,   -9,  -10,  -11,  -75
	}
};

TUNE_CONST i16 PSq::bishopTables[2][64] =
{
	// opening
	{
		 -26,  -43,  -68,  -49,  -25,  -61,  -61,  -12,
		 -36,   -1,   -9,   -3,  -21,  -36,  -53,  -67,
		 -11,    9,   20,    7,    5,   66,   -5,    6,
		 -18,   -9,  -12,   33,    0,   -3,   -5,  -18,
		 -15,    1,  -11,   15,   10,   -9,   -2,    7,
		  -8,    0,    8,   -7,    3,    2,   14,    9,
		   1,    2,    4,   -5,   -2,    5,   14,    7,
		  -7,   16,  -12,  -22,    9,  -15,    9,  -32
	},
	// endgame
	{
		  13,   20,   17,   14,   -2,    2,    7,    3,
		   5,    6,   11,    8,    6,   -1,    5,   -2,
		   5,   15,    3,   10,    8,   19,   10,    9,
		  -3,    6,    9,   17,   14,    9,    5,    3,
		 -11,    7,   10,   16,    8,    7,    0,   -2,
		  -2,    6,    9,    8,   20,   -1,   -1,   -8,
		 -10,   -4,   -2,    0,   -1,    0,    3,  -29,
		 -13,  -10,   -9,   -7,   -4,   -2,   -2,  -14
	}
};

TUNE_CONST i16 PSq::rookTables[2][64] =
{
	// opening
	{
		  55,   19,   31,   28,   -2,  115,   39,  165,
		  23,   12,   37,   68,   53,   59,   13,   85,
		   4,   29,   22,   44,   98,  116,  109,   58,
		  -3,    4,    5,   27,   31,   31,   39,   -1,
		 -23,  -12,   -3,    3,   -7,  -10,    7,  -22,
		 -24,   -7,   -9,   -4,   -3,    2,    9,  -14,
		 -41,  -33,   -5,    7,   -4,    5,   -4,  -59,
		 -12,   -7,   -2,    3,    5,   -6,   -4,  -19
	},
	// endgame
	{
		   2,   13,   17,   13,   16,   21,   21,   14,
		  21,   24,   26,   27,   24,   18,   20,   19,
		  26,   22,   26,   25,   24,   26,   17,   16,
		  23,   29,   27,   25,   22,   26,   25,   19,
		  13,   16,   17,   13,   13,   22,   16,    6,
		  -3,    2,    4,   -1,   -4,    0,    3,   -8,
		 -13,   -4,   -2,   -8,   -7,   -7,   -8,  -14,
		 -11,  -10,   -3,   -6,  -10,   -5,   -7,  -22
	}
};

TUNE_CONST i16 PSq::queenTables[2][64] =
{
	// opening
	{
		-15,   18,    2,   36,   33,  155,  151,  116,
		-17,  -29,   11,    8,   -6,   22,    8,   88,
		  5,    6,    4,   -3,   18,   81,   80,    8,
		 -4,    2,  -17,  -12,    2,  -12,   15,   -5,
		 -9,  -10,   -2,   -8,   -4,   -2,   12,   -1,
		-11,   -6,   -6,   -3,    3,    0,   10,    7,
		-23,   -9,   -3,    8,   10,   13,   10,   -7,
		 -8,    1,   -7,   -1,    8,  -28,  -43,  -25
	},
	// endgame
	{
		  -4,   10,   19,   21,   29,   32,   21,   25,
		   9,   17,   29,   36,   42,   39,   37,   40,
		   7,   13,   25,   42,   44,   51,   46,   49,
		   3,   22,   32,   32,   40,   51,   56,   44,
		   5,   16,   16,   26,   20,   33,   29,   23,
		  -3,    6,    5,    1,    9,   16,    6,   -6,
		 -10,   -8,   -5,  -11,   -9,  -27,  -39,  -27,
		 -18,  -27,  -16,  -19,  -31,  -46,  -63,  -55
	}
};

TUNE_CONST i16 PSq::kingTables[2][64] =
{
	// opening
	{
		 -90,  -84,  -87,  -86,  -59,  -89,  -92, -258,
		 -82,  -89,  -78,  -82,  -93,  -85,  -87,   17,
		 -89,  -84,  -85,  -84,  -85,  -84,  -52,  -89,
		 -90,  101,  -81,  -88,  -95,  -89,  -38,  -95,
		-100,   -5,  -29, -106,  -49,  -88,  -67,  -92,
		 -39,  -15,  -43,  -33,  -62,  -38,  -20,  -43,
		  12,  -47,   -1,  -62,  -38,  -38,    4,   15,
		 -29,    3,    9,  -64,    2,  -52,   18,   15
	},
	// endgame
	{
		-105,    3,    7,    7,   10,   27,   27,  -72,
		 -45,   28,   33,   18,   25,   40,   51,  -13,
		  14,   36,   35,   33,   35,   56,   52,   21,
		   3,   28,   35,   36,   38,   41,   36,    7,
		 -18,   13,   26,   30,   29,   26,   15,  -11,
		 -24,   -2,   12,   20,   20,   11,    0,  -17,
		 -25,  -11,    2,    1,    5,    2,   -4,  -20,
		 -37,  -22,  -16,  -27,  -37,  -21,  -20,  -44
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
