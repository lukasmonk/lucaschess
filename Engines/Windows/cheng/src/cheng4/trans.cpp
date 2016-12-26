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

#include "trans.h"
#include "utils.h"
#include "move.h"
#include <memory.h>
#include <new>

namespace cheng4
{

// TransTable

// TT align size in bytes; must be power of two
static const uint alignSize = 4096;

TransTable::TransTable() : allocEntries(0)
{
	memset( dummy, 0, sizeof(dummy) );
	dummyAlloc();
}

void TransTable::clear()
{
	memset( entries, 0, sizeof(TransEntry) * size );
}

TransTable::~TransTable()
{
	dealloc();
}

void TransTable::dealloc()
{
	if ( allocEntries )
	{
		delete[] allocEntries;
		allocEntries = 0;
	}
}

void TransTable::dummyAlloc()
{
	dealloc();
	entries = dummy;
	size = buckets;
	clear();
}

bool TransTable::resize( size_t sizeBytes )
{
	size_t sizeEntries = (sizeBytes + sizeof(TransEntry)-1)/sizeof(TransEntry);
	if ( sizeEntries <= buckets )
	{
		// do dummy alloc (buckets entries)
		dummyAlloc();
		return 1;
	}
	if ( !roundPow2( sizeEntries ) )
		return 0;					// bad size
	if ( size == sizeEntries )
		return 1;
	// realloc!
	dealloc();
	allocEntries = new(std::nothrow) TransEntry[ sizeEntries + alignSize/sizeof(TransEntry) ];
	if ( !allocEntries )
	{
		dummyAlloc();
		return 0;
	}
	// align entries
	entries = static_cast<TransEntry *>(alignPtr( allocEntries, alignSize ));
	size = sizeEntries;
	return 1;
}

// return entry
Score TransTable::probe( Signature sig, Ply ply, Depth depth, Score alpha, Score beta, Move &mv, TransEntry &lte ) const
{
	mv = mcNone;
	size_t ei = ((size_t)sig & (size-1) & ~(buckets-1));
	const TransEntry *te = entries + ei;
	for ( uint i=0; i<buckets; i++, te++)
	{
		lte = *te;
		lte.bhash ^= lte.u.word2;
		if ( lte.bhash == sig )
		{
			mv = lte.u.s.move;
			if ( lte.u.s.depth < depth )
				return scInvalid;
			BoundType bt = (BoundType)(lte.u.s.bound & 3);
			Score score = ScorePack::unpackHash( lte.u.s.score, ply );
			switch( bt )
			{
			case btExact:
				return score;
			case btUpper:
				if ( score <= alpha )
					return score;
				break;
			case btLower:
				if ( score >= beta )
					return score;
				break;
			default:;
			}
			return scInvalid;
		}
	}
	return scInvalid;
}

Score TransTable::probeEval( Signature sig, Ply ply, Score val, const TransEntry &lte )
{
	if ( lte.bhash != sig || lte.u.s.depth < (Depth)-1 )
		return scInvalid;
	BoundType bt = (BoundType)(lte.u.s.bound & 3);
	Score score = ScorePack::unpackHash( lte.u.s.score, ply );
	switch( bt )
	{
	case btExact:
		return score;
	case btUpper:
		if ( score <= val )
			return score;
		break;
	case btLower:
		if ( score >= val )
			return score;
		break;
	default:;
	}
	return scInvalid;
}

// store into hash table
void TransTable::store( Signature sig, Age age, Move move, Score score, HashBound bound, Depth depth, Ply ply )
{
	assert( !( bound & ~3) );
	assert( ScorePack::isValid(score) );
	assert( score != -scInfinity );

	size_t ei = ((size_t)sig & (size-1) & ~(buckets-1));
	TransEntry *te = entries + ei;
	TransEntry *be = 0;				// best entry
	i32 beScore = -0x7fffffff;

	age <<= 2;

	TransEntry lte;
	for ( uint i=0; i<buckets; i++, te++ )
	{
		lte = *te;
		lte.bhash ^= lte.u.word2;
		if ( lte.bhash == sig )
		{
			// same entry found => use that!
			if ( move == mcNone )
				move = lte.u.s.move;
			be = te;
			break;
		}
		// replace based on depth and age
		// FIXME: better?
		i32 escore = ((i32)depth-lte.u.s.depth)*2 + ((Age)(lte.u.s.bound & 0xfc) != age) * 256 -
			((lte.u.s.bound & 3) == btExact);
		if ( escore > beScore )
		{
			be = te;
			beScore = escore;
		}
	}
	assert( be );
	lte = *be;
	lte.u.s.bound = age | bound;
	lte.u.s.depth = depth;
	lte.u.s.move = move;
	lte.u.s.score = ScorePack::packHash( score, ply );
	lte.bhash = sig ^ lte.u.word2;
	*be = lte;
}

}
