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
#include "board.h"
#include <fstream>
#include <vector>

namespace cheng4
{

struct BookBits
{
	u8 bits[65536];
	inline u8 probe( u64 sig )
	{
		u32 idx = (u32)(sig & 0x7ffffu);
		return bits[ idx>>3 ] & (1u << (idx&7) );
	}
};

struct BookEntry
{
	Signature sig;
	BookMove move;
	BookCount count;		// 0 = avoid
};

struct PRNG;

class Book
{
	Book( const Book & )
	{
		assert( 0 && "Book cannot be copied!" );
	}
	Book &operator =( const Book & )
	{
		assert( 0 && "Book cannot be copied!" );
		return *this;
	}
protected:
	BookProbe counter;
	BookBits bits;
	std::ifstream *stream;
	u32 numEntries;
	u32 numPositions;

	PRNG *rng;

	bool readEntry( size_t idx, BookEntry &e );

	// find leftmost entry
	u32 findEntry( u64 sig, BookEntry &e );

public:

	Book();
	~Book();

	// open book file
	bool open( const char *fnm );
	// close book file
	bool close();

	// reset probe counter
	void reset();

	// increment probe counter
	void incCounter();

	// book disabled (due to counter?)
	bool disabled() const;

	// probe book (returs mcNone if no move is found)
	Move probe( const Board &b );

	// enum moves for current entry (ignores disabled flag)
	// if avoid is true, it enumerates moves with zero weight
	// returns count_sum
	u32 enumEntries( const Board &b, std::vector<Move> &moves, bool avoid = 0, std::vector<u32> *counts = 0 );

	// get number of entries (=moves)
	u32 getNumEntries() const;
	// get number of positions
	u32 getNumPositions() const;
};

}
