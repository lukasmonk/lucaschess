/*
You can use this program under the terms of either the following zlib-compatible license
or as public domain (where applicable)

  Copyright (C) 2014-2015 Martin Sedlak

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

#include "../cheng4/types.h"
#include "../cheng4/engine.h"
#include "../cheng4/book.h"
#include "../cheng4/movegen.h"
#include <stdio.h>
#include "pgzobrist.h"
#include <string>
#include <set>
#include <algorithm>

//#define USE_SEARCH

namespace pgimport
{

struct BookBits
{
	u8 bits[65536];
	void clear()
	{
		memset( bits, 0, sizeof(bits) );
	}
	inline u8 probe( u64 sig )
	{
		u32 idx = (u32)sig & 0x7ffff;
		return bits[ idx>>3 ] & (1u << (idx&7) );
	}
	void set( u64 sig )
	{
		u32 idx = (u32)sig & 0x7ffff;
		bits[ idx>>3 ] |= (1u << (idx&7) );
	}
};

}

template< typename T > static inline void byteSwap( T &v )
{
	u8 *p = (u8 *)&v;
	for ( u32 i=0; i<sizeof(T)/2; i++ )
	{
		u32 j = sizeof(T)-i-1;
		u8 tmp = p[i]; p[i] = p[j]; p[j] = tmp;
	}
}

struct PGEntry {
   u64 key;
   u16 move;
   u16 count;
   u16 n;
   u16 sum;

   void byteSwap()
   {
	   ::byteSwap( key );
	   ::byteSwap( move );
	   ::byteSwap( count );
	   ::byteSwap( n );
	   ::byteSwap( sum );
   }

   // for sorting!
   bool operator <( const PGEntry &o ) const
   {
	   if ( key != o.key )
		   return key < o.key;
	   if ( count != o.count )
		   return count > o.count;
	   return move < o.move;
   }
};

static inline u8 PGPiece( cheng4::Piece p )
{
	// Polyglot has:
	// bp, wp, bn, wn, bb, wb, br, wr, bq, wq, kb, wk
	u8 res = (cheng4::PiecePack::type(p)-1) << 1;
	res |= cheng4::PiecePack::color(p) == cheng4::ctWhite ? 1 : 0;
	return res;
}

static inline u8 PGSquare( cheng4::Square sq )
{
	return (u8)sq ^ 0x38;
}

static u64 PGHashPiece( cheng4::Piece p, cheng4::Square sq )
{
	u64 res = polyglot::Zobrist[ polyglot::RandomPiece + PGPiece(p)*64 + PGSquare(sq) ];
	return res;
}

static cheng4::Move PGMove( u16 m, const cheng4::Board &b )
{
	cheng4::Square from = cheng4::SquarePack::flipV( (cheng4::Square)((m >> 6) & 63) );
	cheng4::Square to = cheng4::SquarePack::flipV( (cheng4::Square)(m & 63) );

	// special case: convert king captures rook => castling (always king captures rook in polyglot!)
	cheng4::Piece pf = b.piece( from );
	cheng4::Piece pt = b.piece( to );
	if ( cheng4::PiecePack::type( pf ) == cheng4::ptKing )
	{
		if ( cheng4::PiecePack::type( pt ) == cheng4::ptRook &&
			cheng4::PiecePack::color( pf ) == cheng4::PiecePack::color( pt ) )
		{
			cheng4::File kf = cheng4::SquarePack::file( to ) >= cheng4::SquarePack::file( from )
				? cheng4::GFILE : cheng4::CFILE;
			to = cheng4::SquarePack::setFile( to, kf );
			return cheng4::MovePack::initCastling( from, to );
		}
	}

	u8 promo = (m >> 12) & 7;
	if ( promo )
		promo += 1;
	return cheng4::MovePack::initPromo( from, to, promo );
}

static u64 PGHash( const cheng4::Board &b )
{
	u64 res = 0;
	for ( cheng4::Square sq=0; sq<64; sq++ )
	{
		cheng4::Piece p = b.piece(sq);
		if ( cheng4::PiecePack::type(p) == cheng4::ptNone )
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
	for ( cheng4::Color c = cheng4::ctWhite; c <= cheng4::ctBlack; c++ )
	{
		cheng4::CastRights cr = b.castRights( c );
		if ( cheng4::CastPack::allowedShort( cr ) )
			flags |= 1 << (c*2);
		if ( cheng4::CastPack::allowedLong( cr ) )
			flags |= 2 << (c*2);
	}

	// castling
	for ( u8 i=0; i<4; i++)
	{
		if ( !(flags & (1<<i)) )
			continue;
		res ^= polyglot::Zobrist[ polyglot::RandomCastle + i ];
	}

	// en passant
	cheng4::Square ep = b.epSquare();
	if ( ep )
	{
		// polyglot: if ep is illegal (can't be captured), it's zeroed!
		cheng4::File f = cheng4::SquarePack::file( ep );
		res ^= polyglot::Zobrist[ polyglot::RandomEnPassant + f ];
	}

	// turn
	if ( b.turn() == cheng4::ctWhite )
		res ^= polyglot::Zobrist[ polyglot::RandomTurn ];

	return res;
}

struct PGBook
{
	u32 entries;
	FILE *f;

	bool readEntry( u32 index, PGEntry &ent )
	{
		if ( index >= entries )
			return 0;
		fseek(f, (long)index*16, SEEK_SET );
		bool ok = fread( &ent, 16, 1, f ) == 1;
		ent.byteSwap();
		//
		// first sum all counts for all moves
		// second, compute score as (double(score)/double(sum))*100.0, where score is count for current move
		return ok;
	}

	// find leftmost entry
	u32 find( u64 sig )
	{
		PGEntry ent;
		i32 l = 0;
		i32 h = (i32)entries-1;
		while ( l<h )
		{
			i32 m = (l+h)/2;
			if ( !readEntry( (u32)m, ent ) )
				return 0;
			if ( sig <= ent.key )
				h = m;
			else
				l = m+1;
		}
		if ( !readEntry( (u32)l, ent ) )
			return 0;
		return ent.key == sig ? l : entries;
	}

	void init( u64 fsz, FILE *f_ )
	{
		f = f_;
		entries = (u32)(fsz/16);
	}
};

void convertEntry( const PGEntry &ent, cheng4::BookEntry &oute, const cheng4::Board &b )
{
	oute.sig = b.sig();
	oute.count = ent.count;
	oute.move = (cheng4::BookMove)(PGMove( ent.move, b ) & 0xffffu);
}

std::set< cheng4::Signature > processed;
std::vector< cheng4::Signature > stack;
std::set< u64 > bookHashes;

bool convertBook( FILE *f2, PGBook &book, cheng4::Board &b, u32 &poscnt, u32 fulldepth = 0 )
{
	for ( size_t i=0; i<stack.size(); i++ )
		if ( stack[i] == b.sig() )
			return 0;
	if ( processed.find( b.sig() ) != processed.end() )
		return 0;
	stack.push_back( b.sig() );
	PGEntry ent;
	u64 sig = PGHash( b );
	u32 e = bookHashes.find(sig) == bookHashes.end() ? book.entries : book.find( sig );
	if ( e >= book.entries && fulldepth <= 1 )
	{
		bool res = 0;
#ifdef USE_SEARCH
		cheng4::MoveGen *mg = new cheng4::MoveGen( b );
		cheng4::Move lm;
		while ( (lm = mg->next()) != cheng4::mcNone )
		{
			cheng4::UndoInfo ui;
			b.doMove( lm, ui, b.isCheck( lm, mg->discovered() ) );
			if ( convertBook( f2, book, b, poscnt, fulldepth+1 ) )
			{
				res = 1;
			}
			b.undoMove( ui );
		}
		delete mg;
/*		std::cout << "entry not found!!!\n";
		b.dump();
		sig = PGHash( b );*/
#endif
		return res;
	}
	bool res = 1;
	bool firstTime = 1;
	while ( book.readEntry( e, ent ) && ent.key == sig )
	{
		fulldepth = 0;
		if ( firstTime )
		{
			processed.insert( b.sig() );
			firstTime = 0;
		}
		std::set< cheng4::Move > movesProcessed;
		cheng4::Move m = PGMove( ent.move, b );
		cheng4::Move lm;
		cheng4::MoveGen *mg = new cheng4::MoveGen( b );
		while ( (lm = mg->next()) != cheng4::mcNone )
		{
			if ( cheng4::MovePack::equalBook( lm, m ) )
				break;
		}
		if ( lm == cheng4::mcNone )
		{
			std::cout << "Error! illegal move in pg book!" << std::endl;
			b.dump();
			std::cout << "move: " << cheng4::MovePack::toUCI( m ) << std::endl;
			abort();
		}
		movesProcessed.insert( lm );
		cheng4::BookEntry be;
		convertEntry( ent, be, b );
		fwrite( &be, sizeof(be), 1, f2 );

		if ( !(poscnt++ & 1023) )
		{
			std::cout << poscnt << " ";
		}

		cheng4::UndoInfo ui;
		b.doMove( lm, ui, b.isCheck(lm, mg->discovered()) );
		convertBook( f2, book, b, poscnt, fulldepth+1 );
		b.undoMove( ui );

		delete mg;

#ifdef USE_SEARCH
		// do the rest
		{
			cheng4::MoveGen *mg = new cheng4::MoveGen( b );
			cheng4::Move lm;
			while ( (lm = mg->next()) != cheng4::mcNone )
			{
				if ( movesProcessed.find(lm) != movesProcessed.end() )
					continue;
				cheng4::UndoInfo ui;
				b.doMove( lm, ui, b.isCheck( lm, mg->discovered() ) );
				convertBook( f2, book, b, poscnt, fulldepth+1 );
				b.undoMove(ui);
			}
			delete mg;
		}
#endif

		e++;
	}
	stack.pop_back();
	return res;
}

static bool entcmp( const cheng4::BookEntry &e1, const cheng4::BookEntry &e2 )
{
	if ( e1.sig != e2.sig )
		return e1.sig < e2.sig;
	if ( e1.count != e2.count )
		return e1.count > e2.count;
	return e1.move < e2.move;
}

void sortBook( const char *fnm )
{
	std::cout << "sorting..." << std::endl;
	FILE *f = fopen(fnm, "rb");
	fseek(f, 0, SEEK_END);
	long fsz = ftell(f);
	fseek(f, 0, SEEK_SET );
	u32 ent = (u32)(fsz/16);
	cheng4::BookEntry *e = new cheng4::BookEntry[ ent ];
	fread( e, 16, ent, f );
	fclose(f);

	std::sort( e, e+ent, entcmp );
	f = fopen("test.cbs", "wb");
	fwrite( e, 16, ent, f );
	fclose(f);

	std::cout << "validating.." << std::endl;
	for ( u32 i=1; i<ent; i++ )
	{
		if ( e[i].sig == e[i-1].sig && e[i].move == e[i-1].move )
		{
			std::cout << "DUP_ENTRY_FOUND!" << std::endl;
		}
	}

	delete[] e;
}

static u16 toPGMove( Move m, const Board &b )
{
	u16 res;
	Square from = MovePack::from(m);
	Square to = MovePack::to(m);
	if ( MovePack::isCastling(m) )
	{
		// convert to => rook capture
		CastRights cr = b.castRights( b.turn() );
		to = CastPack::rookSquare( to, cr );
	}
	from = SquarePack::flipV( from );
	to = SquarePack::flipV( to );
	res = (from << 6) | to;
	if ( MovePack::isPromo(m) )
	{
		Piece promo = MovePack::promo( m );
		res |= (promo-1) << 12;
	}
	return res;
}

void convertLine( const char *buf, std::set< PGEntry > &book )
{
	Board b;
	b.reset();
	for (;;)
	{
		while (isspace(*buf))
			buf++;
		Move m = b.fromSAN(buf);
		if ( m == mcNone )
			break;
		PGEntry e;
		e.n = e.sum = 0;
		e.count = 2;
		e.move = toPGMove( m, b );
		e.key = PGHash(b);
		book.insert(e);

		UndoInfo ui;
		bool isCheck = b.isCheck( m, b.discovered() );
		b.doMove( m, ui, isCheck );
	}
}

void convertBookLines( const char *fnm, const char *ofnm )
{
	std::set< PGEntry > book;
	char buf[4096];
	FILE *f = fopen(fnm, "r");
	while ( fgets(buf, sizeof(buf), f) )
		convertLine( buf, book );
	fclose(f);
	// now: write out!
	FILE *f2 = fopen(ofnm, "wb");
	std::set< PGEntry >::const_iterator ci;
	for ( ci=book.begin(); ci != book.end(); ci++ )
	{
		PGEntry ent = *ci;
		ent.byteSwap();
		fwrite( &ent, sizeof(ent), 1, f2 );
	}
	fclose(f2);
}

int main( int argc, char **argv )
{
	cheng4::Engine::init();

/*	// new: use generic book lines...
	convertBookLines("booklines.txt", "booklines.bin");
	return 0;*/

	PGBook book;
	FILE *f2 = fopen("cheng2015.cb", "wb");
	FILE *f = fopen(argc > 1 ? argv[1] : "booklines.bin", "rb");
	if ( !f )
	{
		std::cout << "unable to open input book" << std::endl;
		fclose(f2);
		return 1;
	}
	fseek(f, 0, SEEK_END);
	long fsz = ftell(f);
	book.init( fsz, f );
	std::cout << "PG Book: " << book.entries << " entries" << std::endl;

	cheng4::Board b;
	b.reset();

	for (u32 i=0; i<book.entries; i++)
	{
		PGEntry ent;
		book.readEntry( i, ent );
		bookHashes.insert( ent.key );
	}

	std::cout << "Book positions: " << bookHashes.size() << std::endl;

	u32 poscnt = 0;

	char header[16] = "generic book   ";
	fwrite( header, 16, 1, f2 );
	u32 sz = (u32)book.entries;
	fwrite( &sz, 4, 1, f2 );
	u32 npos = (u32)bookHashes.size();
	fwrite( &npos, 4, 1, f2 );
	u32 pad[2] = {0, 0};
	fwrite( pad, sizeof(pad), 1, f2 );
	pgimport::BookBits bbits;
	bbits.clear();
	fwrite( &bbits, sizeof(bbits), 1, f2 );
	std::vector< PGEntry > entries(book.entries);
	for (u32 i=0; i<book.entries; i++)
	{
		PGEntry ent;
		book.readEntry( i, ent );
		entries[i] = ent;
		poscnt++;
	}
	// sort now!
	std::sort( entries.begin(), entries.end() );
	for (u32 i=0; i<book.entries; i++)
	{
		PGEntry &ent = entries[i];
		bbits.set( ent.key );
		fwrite( &ent, 12, 1, f2 );
	}
	fseek(f2, 32, SEEK_SET );
	fwrite( &bbits, sizeof(bbits), 1, f2 );

	std::cout << "Done: " << poscnt << " entries converted" << std::endl;

	fclose(f);
	fclose(f2);

	cheng4::Engine::done();
	return 0;
}
