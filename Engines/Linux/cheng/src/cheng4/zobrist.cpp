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

#include "zobrist.h"
#include "prng.h"
#include <memory.h>

namespace cheng4
{

// Zobrist

Signature	Zobrist::turn;					// turn (stm) xor-hash
Signature	Zobrist::epFile[8];				// en-passant file hash [epfile]
Signature	Zobrist::piece[2][ptMax][64];	// [color][piece][square]
Signature	Zobrist::cast[2][0x88+1];		// castling rights [color][rights]

void Zobrist::init()
{
	PRNG prng;
	// turn
	turn = prng.next64();

	// epFile
	for (int i=0; i<8; i++)
		epFile[i] = prng.next64();

	// piece
	memset( piece, 0, sizeof(piece) );
	for (Color c=ctWhite; c<=ctBlack; c++)
		for (Piece p=ptPawn; p<=ptKing; p++)
			for (Square sq = 0; sq < 64; sq++ )
				piece[c][p][sq] = prng.next64();

	// castling
	cast[ ctWhite ][0] = cast[ ctBlack ][0] = 0;
	for (Color c=ctWhite; c<=ctBlack; c++)
		for (uint i=1; i<=0x88; i++)
			cast[c][i] = prng.next64();
}

}
