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

#include "board.h"
#include "utils.h"

namespace cheng4
{

// generic eval hash (cache)
template< typename Signature, typename Entry > struct EvalHash
{
	EvalHash() : allocEntries(0)
	{
		memset( &dummy, 0, sizeof(dummy) );
		dummyAlloc();
	}
	~EvalHash()
	{
		dealloc();
	}

	// resize hash table
	// note: automatically clears table on success
	// returns 0 on failure
	bool resize( size_t sizeBytes )
	{
		size_t sizeEntries = (sizeBytes + sizeof(Entry)-1)  / sizeof(Entry);
		if ( sizeEntries <= 1 )
		{
			dummyAlloc();
			return 1;
		}
		if ( !roundPow2( sizeEntries ) )
			return 0;
		dealloc();
		allocEntries = new(std::nothrow) Entry[sizeEntries+1];
		if ( !allocEntries )
		{
			dummyAlloc();
			return 0;
		}
		entries = static_cast<Entry *>(alignPtr( allocEntries, sizeof(Entry) ));
		size = sizeEntries;
		return 1;
	}

	// clear pawn hash table
	void clear()
	{
		assert( entries != 0 );
		memset( entries, 0, size * sizeof(Entry) );
	}

	// get hashentry ptr from index
	inline Entry *index( Signature sig )
	{
		return entries + ((size_t)sig & (size-1));
	}
private:
	Entry *entries;			// entries
	Entry *allocEntries;	// allocated entries
	size_t size;			// must be a power of two

	Entry dummy;

	void dummyAlloc()
	{
		dealloc();
		entries = &dummy;
		size = 1;
		clear();
	}
	void dealloc()
	{
		if ( allocEntries )
		{
			delete[] allocEntries;
			allocEntries = 0;
		}
	}
};

// size must be power of 2
struct EvalCacheEntry
{
	Signature sig;			// signature
	Score score;			// score
	Score pad;				// pad to 16-byte struct
};

typedef EvalHash<Signature, EvalCacheEntry> EvalCache;

// size must be power of 2
struct PawnHashEntry
{
	Signature sig;				// signature
	Bitboard passers[phMax];	// passers for each color
	FineScore scores[phMax];	// pawn scores for each game phase
};

typedef EvalHash<Signature, PawnHashEntry> PawnHash;

// recognizer function type
typedef void (*RecogFunc)( const Board &b, FineScore *fscore );

struct MaterialHashEntry
{
	MaterialKey sig;		// signature
	RecogFunc recog;		// recognizer function (can be 0)
	FineScore fscore[phMax];
	union u
	{
		u8 div[phMax];			// scale dividers
		u16 packed;				// packed dividers
	};
};

typedef EvalHash<MaterialKey, MaterialHashEntry> MaterialHash;

struct Eval
{
	struct Recognizer
	{
		MaterialKey key;
		RecogFunc func;
		inline bool operator <( const Recognizer &o ) const
		{
			return key < o.key;
		}
	};

	Eval();

	// static initializer (sort recognizers)
	static void init();

	// returns final centipawn evaluation from stm's point of view
	Score eval( const Board &b, Score alpha = -scInfinity, Score beta = +scInfinity );

	// returns fast evaluation (psq only)
	Score fastEval( const Board &b );

	// resize eval cache to sz bytes
	// returns !0 on success
	bool resizeEval( size_t sizeBytes );
	// resize pawn hash to sz bytes
	// returns !0 on success
	bool resizePawn( size_t sizeBytes );
	// resize material hash to sz bytes
	// returns !0 on success
	bool resizeMaterial( size_t sizeBytes );

	// clear eval/pawn/material caches
	void clear();

private:
	// scores for game phases
	FineScore fscore[phMax];
	// precomputed values for evaluation
	Bitboard occ;				// occupied
	Bitboard safetyMask[ctMax];	// safety masks for each king
	u32 attackers[ctMax];		// attackers for each stm
	Bitboard pinMask[ctMax];

	PawnHashEntry *pe;			// pawn hash entry pointer

	// attack mask [color][piece]
	Bitboard attm[ctMax][ptMax];

	EvalCache ecache;
	PawnHash phash;
	MaterialHash mhash;

	template< PopCountMode pcm > Score ieval( const Board &b, Score alpha = -scInfinity, Score beta = +scInfinity );

	template< PopCountMode pcm, Color c, bool slow > void evalPawns( const Board &b );
	template< Color c > void evalPassers( const Board &b );
	template< PopCountMode pcm, Color c > void evalKnights( const Board &b );
	template< PopCountMode pcm, Color c > void evalBishops( const Board &b );
	template< PopCountMode pcm, Color c > void evalRooks( const Board &b );
	template< PopCountMode pcm, Color c > void evalQueens( const Board &b );
	template< PopCountMode pcm, Color c > void evalKing( const Board &b );

	template< Color c > bool isCertainWin( const Board &b ) const;
	// eval special endgame cases
	template< Color c > void evalSpecial( const Board &b );

	// eval eg recognizers
	void evalRecog( const Board &b );

	// exec binary search on recognizers array
	void execRecog( const Board &b, MaterialKey mk, const Recognizer *recogs, size_t numRecogs );

	// evaluate blind bishop scaling
	template< Color c > void evalBlindBishop( const Board &b );
};

}
