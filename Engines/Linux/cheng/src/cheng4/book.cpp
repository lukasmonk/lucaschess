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

#include "book.h"
#include "bookzobrist.h"
#include "prng.h"
#include "thread.h"
#include "movegen.h"
#include <memory.h>
#include <vector>

namespace cheng4
{

// don't play moves with percentage score below this
static const u32 probeThresholdPercent	=	8;

// Polyglot utils:

static inline u8 PGPiece( Piece p )
{
	// Polyglot has:
	// bp, wp, bn, wn, bb, wb, br, wr, bq, wq, kb, wk
	u8 res = (PiecePack::type(p)-1) << 1;
	res |= PiecePack::color(p) == ctWhite ? 1 : 0;
	return res;
}

static inline u8 PGSquare( Square sq )
{
	return (u8)sq ^ 0x38;
}

static u64 PGHashPiece( Piece p, Square sq )
{
	u64 res = BookZobrist[ RandomPiece + PGPiece(p)*64 + PGSquare(sq) ];
	return res;
}

static Move PGMove( u16 m, const Board &b )
{
	Square from = SquarePack::flipV( (Square)((m >> 6) & 63) );
	Square to = SquarePack::flipV( (Square)(m & 63) );

	// special case: convert king captures rook => castling (always king captures rook in polyglot!)
	Piece pf = b.piece( from );
	Piece pt = b.piece( to );
	if ( PiecePack::type( pf ) == ptKing )
	{
		if ( PiecePack::type( pt ) == ptRook &&
			PiecePack::color( pf ) == PiecePack::color( pt ) )
		{
			File kf = SquarePack::file( to ) >= SquarePack::file( from )
				? (File)GFILE : (File)CFILE;
			to = SquarePack::setFile( to, kf );
			return MovePack::initCastling( from, to );
		}
	}

	u8 promo = (m >> 12) & 7;
	if ( promo )
		promo += 1;
	Piece pp = (Piece)promo;
	return MovePack::initPromo( from, to, pp );
}

static u64 PGHash( const Board &b )
{
	u64 res = 0;
	for ( Square sq=0; sq<64; sq++ )
	{
		Piece p = b.piece(sq);
		if ( PiecePack::type(p) == ptNone )
			continue;
		res ^= PGHashPiece( p, sq );
	}

	// castling rights
	// polyglot flags:
	// bit 0: white KS
	// bit 1: white QS
	// bit 2: black KS
	// bit 3: black QS

	u8 flags = 0;
	for ( Color c = ctWhite; c <= ctBlack; c++ )
	{
		CastRights cr = b.castRights( c );
		if ( CastPack::allowedShort( cr ) )
			flags |= 1 << (c*2);
		if ( CastPack::allowedLong( cr ) )
			flags |= 2 << (c*2);
	}

	// castling
	for ( u8 i=0; i<4; i++)
	{
		if ( !(flags & (1<<i)) )
			continue;
		res ^= BookZobrist[ RandomCastle + i ];
	}

	// en passant
	Square ep = b.epSquare();
	if ( ep )
	{
		// polyglot: if ep is illegal (can't be captured), it's zeroed!
		File f = SquarePack::file( ep );
		res ^= BookZobrist[ RandomEnPassant + f ];
	}

	// turn
	if ( b.turn() == ctWhite )
		res ^= BookZobrist[ RandomTurn ];

	return res;
}

// Book

static const BookProbe maxCounter = 8;

Book::Book() : counter(0), stream(0), numEntries(0), numPositions(0)
{
	memset( bits.bits, 0, sizeof(bits.bits) );
	rng = new PRNG( Timer::getMillisec() );
}

Book::~Book()
{
	close();
	delete rng;
}

bool Book::open(const char *fnm)
{
	close();
	stream = new std::ifstream( fnm, std::ios::in | std::ios::binary );
	if ( !*stream )
	{
		close();
		return 0;
	}
	stream->seekg( (std::streampos)(16u) );
	stream->read( (char *)&numEntries, sizeof(numEntries) );
	stream->read( (char *)&numPositions, sizeof(numPositions) );
	stream->seekg( (std::streampos)(32u) );
	stream->read( (char *)bits.bits, 65536u );
	if ( stream->fail() )
	{
		close();
		return 0;
	}
	return 1;
}

bool Book::close()
{
	if ( !stream )
		return 0;
	numEntries = 0;
	stream->close();
	delete stream;
	stream = 0;
	return 1;
}

bool Book::readEntry( size_t idx, BookEntry &e )
{
	if ( !stream )
		return 0;
	stream->seekg( (std::streampos)(32u+65536u+12u*idx) );
	stream->read( (char *)&e, 12 );
	if ( stream->fail() )
		return 0;
	return 1;
}

u32 Book::findEntry( u64 sig, BookEntry &ent )
{
	i32 l = 0;
	i32 h = (i32)numEntries-1;
	while ( l<h )
	{
		i32 m = (l+h)/2;
		if ( !readEntry( (u32)m, ent ) )
			return 0;
		if ( sig <= ent.sig )
			h = m;
		else
			l = m+1;
	}
	if ( !readEntry( (u32)l, ent ) )
		return 0;
	return ent.sig == sig ? l : numEntries;
}

// enum moves for current entry
u32 Book::enumEntries( const Board &b, std::vector<Move> &moves, bool avoid, std::vector<u32> *counts )
{
	u64 hash = PGHash(b);
	if ( counts )
		counts->clear();
	moves.clear();
	if ( !bits.probe( hash ) )
		return 0;			// fast-reject

	BookEntry ent;
	u32 ei = findEntry( hash, ent );

	if ( ei >= numEntries )
		return 0;

	Move lm, lmoves[maxMoves];
	MoveGen mg(b);
	uint lcount = 0;
	while ( (lm = mg.next()) != mcNone )
		lmoves[lcount++] = lm;

	u32 sum = 0;

	std::vector< BookEntry > ents;
	while ( ei < numEntries )
	{
		sum += ent.count;
		ents.push_back( ent );
		if ( ++ei >= numEntries )
			break;
		if ( !readEntry( ei, ent ) )
			break;
		if ( ent.sig != hash )
			break;
	}

	if ( !avoid && !sum )
		return 0;

	u32 realSum = 0;
	for (size_t i=0; i<ents.size(); i++)
	{
		const BookEntry &bent = ents[i];
		u32 score = sum ? (bent.count * 100) / sum : 0;	// Scid-compatible score, not quite however(?)...
		bool avoidMove = bent.count < 1 || score < probeThresholdPercent;
		if ( avoid == avoidMove )
		{
			Move hint = PGMove( bent.move, b );
			// now match hint move
			for (uint j=0; j<lcount; j++)
			{
				if ( MovePack::equal(lmoves[j], hint) )
				{
					realSum += bent.count;
					moves.push_back( lmoves[j] );
					if ( counts )
						counts->push_back(bent.count);
					break;
				}
			}
		}
	}
	return realSum;
}

// reset probe counter
void Book::reset()
{
	counter = 0;
}

// increment probe counter
void Book::incCounter()
{
	if ( counter < maxCounter )
		++counter;
}

// book disabled (due to counter?)
bool Book::disabled() const
{
	return counter >= maxCounter;
}

// probe book (returs mcNone if no move is found)
Move Book::probe( const Board &b )
{
	if ( disabled() )
		return mcNone;

	std::vector< Move > moves;
	std::vector< u32 > counts;
	u32 sum = enumEntries( b, moves, 0, &counts );
	assert( moves.size() == counts.size() );

	// choose one move at random, picking better moves with higher probability
	if ( sum && !moves.empty() )
	{
		u32 threshold = (u32)(rng->next64() & 0xffffffffu) % sum;
		u32 msum = 0;
		// iterate in reverse order, assume moves are sorted by count in descending order by converter!
		for (size_t i=moves.size(); i>0; i--)
		{
			msum += counts[i-1];
			if ( msum > threshold )
				return moves[i-1];
		}
	}
	return mcNone;
}

// get number of entries (=moves)
u32 Book::getNumEntries() const
{
	return numEntries;
}

// get number of positions
u32 Book::getNumPositions() const
{
	return numPositions;
}

}
