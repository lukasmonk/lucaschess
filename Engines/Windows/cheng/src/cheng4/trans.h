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

namespace cheng4
{

// size must be power of 2
struct TransEntry
{
	Signature bhash;			// board hash signature
	union u
	{
		struct s
		{
			Move move;					// move
			HashScore score;			// score (matescores are relative)
			HashDepth depth;			// depth
			HashBound bound;			// hash bound and age packed into this
		} s;
		u64 word2;
	} u;
};

class TransTable
{
protected:
	static const uint bucketBits = 2;
	static const uint buckets = 1 << bucketBits;
	TransEntry *allocEntries;	// original block alloc ptr
	TransEntry *entries;		// aligned entries
	size_t size;				// size in entries; must be a power of two
	TransEntry dummy[buckets];	// dummy entry if no space is allocated (1-entry hashtable)

	// alloc buckets entries using dummy
	void dummyAlloc();
	// deallocate
	void dealloc();
public:
	TransTable();
	~TransTable();

	// resize hashtable
	// note: clears table automatically on success
	// if resize fails, hashtable size is not guaranteed to be the same as before reallocation!
	// returns 1 on success
	bool resize( size_t sizeBytes );

	// clear hashtable
	void clear();

	// probe hash table
	// returns scInvalid if probe failed
	inline Score probe( Signature sig, Ply ply, Depth depth, Score alpha, Score beta, Move &mv ) const
	{
		TransEntry lte;
		return probe( sig, ply, depth, alpha, beta, mv, lte );
	}

	// new version returning lte
	Score probe( Signature sig, Ply ply, Depth depth, Score alpha, Score beta, Move &mv, TransEntry &lte ) const;
	static Score probeEval( Signature sig, Ply ply, Score val, const TransEntry &lte );

	// store into hash table
	// TODO: reorder params
	void store( Signature sig, Age age, Move move, Score score, HashBound bound, Depth depth, Ply ply );
};

}
