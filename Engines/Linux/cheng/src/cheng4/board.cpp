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

#include "board.h"
#include "movegen.h"
#include "utils.h"
#include <memory.h>

namespace cheng4
{

// Board

// increment move counter by 1
void Board::incMove()
{
	++curMove;
}

template< Color c > Draw Board::isDrawByMaterial( MaterialKey mk ) const
{
	// if other side has only a knight => draw
	if ( mk == matKNK[ c ] )
		return drawMaterial;
	if ( mk & matKnightMask[ c ] )
		return drawNotDraw;				// more than one knight => not draw

	// examine bishops...
	Bitboard bishops = pieces( c, ptBishop );
	assert( bishops );
	return (Draw)( !( bishops & lightSquares ) || !( bishops & darkSquares ) ? drawMaterial : drawNotDraw );
}

// is trivial draw? (material or fifty rule)
Draw Board::isDraw() const
{
	if ( bfifty >= 100 )
	{
		// we must not be checkmated already, while this can be considered a performance penalty,
		// it doesn't happen often so should be ok
		MoveGen mg( *this );
		if ( mg.next() != mcNone )
			return drawFifty;				// trivial draw by fifty move rule
	}

	// examine material key first
	MaterialKey mk = materialKey();

	if ( mk & matNoDrawMask )
		return drawNotDraw;					// pawns/rooks/queens => not draw

	if ( !mk )
		return drawMaterial;				// king versus king => most trivial draw

	if ( !(mk & matMask[ ctWhite ]) )
		// white is bare king
		return isDrawByMaterial< ctBlack >( mk );
	else if ( !(mk & matMask[ ctBlack ] ) )
		// black is bare king
		return isDrawByMaterial< ctWhite >( mk );
	return drawNotDraw;
}

void Board::clear()
{
	memset( this, 0, sizeof(*this) );
	frc = arenaMode = 0;
	curMove = 1;
	bb[ BBI( ctWhite, ptPawn ) ]	= 0;
	bb[ BBI( ctBlack, ptPawn ) ]	= 0;
	bb[ BBI( ctWhite, ptKnight ) ]	= 0;
	bb[ BBI( ctBlack, ptKnight ) ]	= 0;
	bb[ BBI( ctWhite, ptBishop ) ]	= 0;
	bb[ BBI( ctBlack, ptBishop ) ]	= 0;
	bb[ BBI( ctWhite, ptRook ) ]	= 0;
	bb[ BBI( ctBlack, ptRook ) ]	= 0;
	bb[ BBI( ctWhite, ptQueen ) ]	= 0;
	bb[ BBI( ctBlack, ptQueen ) ]	= 0;
	bb[ bbiWOcc ]					= 0;
	bb[ bbiBOcc ]					= 0;

	bkingPos[ ctWhite ] = E1;
	bkingPos[ ctBlack ] = E8;

	bcastRights[ ctBlack ] = bcastRights[ ctWhite ] = 0;
	bfifty = 0;
	bturn = ctWhite;
	bep = 0;

	update();
}

void Board::reset()
{
	memset( this, 0, sizeof(*this) );
	frc = arenaMode = 0;
	curMove = 1;
	bb[ BBI( ctWhite, ptPawn ) ]	= U64C( 0x00ff000000000000 );
	bb[ BBI( ctBlack, ptPawn ) ]	= U64C( 0x000000000000ff00 );
	bb[ BBI( ctWhite, ptKnight ) ]	= U64C( 0x4200000000000000 );
	bb[ BBI( ctBlack, ptKnight ) ]	= U64C( 0x0000000000000042 );
	bb[ BBI( ctWhite, ptBishop ) ]	= U64C( 0x2400000000000000 );
	bb[ BBI( ctBlack, ptBishop ) ]	= U64C( 0x0000000000000024 );
	bb[ BBI( ctWhite, ptRook ) ]	= U64C( 0x8100000000000000 );
	bb[ BBI( ctBlack, ptRook ) ]	= U64C( 0x0000000000000081 );
	bb[ BBI( ctWhite, ptQueen ) ]	= U64C( 0x0800000000000000 );
	bb[ BBI( ctBlack, ptQueen ) ]	= U64C( 0x0000000000000008 );
	bb[ bbiWOcc ]					= U64C( 0xffff000000000000 );
	bb[ bbiBOcc ]					= U64C( 0x000000000000ffff );

	bkingPos[ ctWhite ] = E1;
	bkingPos[ ctBlack ] = E8;

	bcastRights[ ctBlack ] = bcastRights[ ctWhite ] = CastPack::init( HFILE, AFILE );
	bfifty = 0;
	bturn = ctWhite;
	bep = 0;

	update();
}

// recompute hash (debug)
Signature Board::recomputeHash() const
{
	Signature h = bturn == ctWhite ? 0 : Zobrist::turn;

	// pieces
	for (Color c = ctWhite; c <= ctBlack; c++ )
		for (Piece p = ptPawn; p <= ptQueen; p++ )
		{
			Bitboard tmp = pieces( c, p );
			while ( tmp )
				h ^= Zobrist::piece[ c ][ p ][ BitOp::popBit(tmp) ];
		}

	// king
	for ( Color c = ctWhite; c <= ctBlack; c++ )
	{
		Square kp = king(c);
		h ^= Zobrist::piece[ c ][ ptKing ][ kp ];
	}

	// validate ep square
	assert ( !bep || (Tables::pawnAttm[ flip(bturn) ][ bep ] & pieces( bturn, ptPawn) ) );

	// en passant
	if ( bep )
		h ^= Zobrist::epFile[ SquarePack::file( epSquare() ) ];

	// castling rights
	for ( Color c = ctWhite; c <= ctBlack; c++ )
		h ^= Zobrist::cast[ c ][ castRights(c) ];
	return h;
}

// recompute pawn hash (debug)
Signature Board::recomputePawnHash() const
{
	Signature ph = 0;

	for (Color c = ctWhite; c <= ctBlack; c++ )
	{
		Bitboard tmp = pieces( c, ptPawn );
		while ( tmp )
		{
			Square sq = BitOp::popBit(tmp);

			Signature sig = Zobrist::piece[ c ][ ptPawn ][ sq ];
			ph ^= sig;
		}
	}

	return ph;
}

// recompute hashes, material and update board squares from bitboards
void Board::update()
{
	Signature h = bturn == ctWhite ? 0 : Zobrist::turn;
	Signature ph = 0;		// pawn hash

	memset( bpieces, 0, sizeof(bpieces) );

	bdmat[ phOpening ] = bdmat[ phEndgame ] = 0;

	bb[ bbiWOcc ] = bb[ bbiBOcc ] = 0;
	bb[ bbiMat ] = 0;

	bcheck = 0;

	// pieces
	for (Color c = ctWhite; c <= ctBlack; c++ )
	{
		bnpmat[ c ] = 0;
		for (Piece p = ptPawn; p <= ptQueen; p++ )
		{
			Bitboard tmp = pieces( c, p );
			while ( tmp )
			{
				Square sq = BitOp::popBit(tmp);
				bpieces[ sq ] = PiecePack::init( c, p );

				bb[ bbiWOcc + c ] |= BitOp::oneShl( sq );

				bnpmat[ c ] += Tables::npValue[ p ];

				// update hash
				Signature sig = Zobrist::piece[ c ][ p ][ sq ];
				h ^= sig;
				if ( p == ptPawn )
					ph ^= sig;

				// update dmat
				bdmat[ phOpening ] += PSq::tables[ phOpening ][ c ][ p ][ sq ];
				bdmat[ phEndgame ] += PSq::tables[ phEndgame ][ c ][ p ][ sq ];

				// update material key
				bb[ bbiMat ] += BitOp::oneShl( MATSHIFT( c, p ) );
			}
		}
	}

	// king
	for ( Color c = ctWhite; c <= ctBlack; c++ )
	{
		Square kp = king(c);
		bpieces[ kp ] = PiecePack::init( c, ptKing );
		h ^= Zobrist::piece[ c ][ ptKing ][ kp ];
		bb[ bbiWOcc + c ] |= BitOp::oneShl( kp );

		// update dmat
		bdmat[ phOpening ] += PSq::tables[ phOpening ][ c ][ ptKing ][ kp ];
		bdmat[ phEndgame ] += PSq::tables[ phEndgame ][ c ][ ptKing ][ kp ];
	}

	// valiate ep square
	if ( bep && !(Tables::pawnAttm[ flip(bturn) ][ bep ] & pieces( bturn, ptPawn) ) )
		bep = 0;

	// en passant
	if ( bep )
		h ^= Zobrist::epFile[ SquarePack::file( epSquare() ) ];

	// castling rights
	for ( Color c = ctWhite; c <= ctBlack; c++ )
		h ^= Zobrist::cast[ c ][ castRights(c) ];

	bcheck = doesAttack<1>( flip( turn() ), king( turn() ) );
	if ( bcheck )
		calcEvasMask();

	bhash = h;
	bpawnHash = ph;

	// finally set frc flag
	frc = 0;
	CastRights cr[2] = { castRights( ctWhite ), castRights( ctBlack ) };
	for ( Color c=ctWhite; c<=ctBlack; c++ )
	{
		File kf = SquarePack::file( king(c) );
		CastRights r = cr[c];
		if ( CastPack::allowedShort( r ) )
		{
			File rf = CastPack::shortFile( r );
			if ( rf != HFILE || kf != EFILE )
				frc = 1;
		}
		if ( CastPack::allowedLong( r ) )
		{
			File rf = CastPack::longFile( r );
			if ( rf != AFILE || kf != EFILE )
				frc = 1;
		}
	}
}

void Board::updateDeltaMaterial()
{
	bdmat[ phOpening ] = bdmat[ phEndgame ] = 0;
	for (Color c = ctWhite; c <= ctBlack; c++ )
	{
		for (Piece p = ptPawn; p <= ptQueen; p++ )
		{
			Bitboard tmp = pieces( c, p );
			while ( tmp )
			{
				Square sq = BitOp::popBit(tmp);

				// update dmat
				bdmat[ phOpening ] += PSq::tables[ phOpening ][ c ][ p ][ sq ];
				bdmat[ phEndgame ] += PSq::tables[ phEndgame ][ c ][ p ][ sq ];
			}
		}
	}

	// king
	for ( Color c = ctWhite; c <= ctBlack; c++ )
	{
		Square kp = king(c);
		// update dmat
		bdmat[ phOpening ] += PSq::tables[ phOpening ][ c ][ ptKing ][ kp ];
		bdmat[ phEndgame ] += PSq::tables[ phEndgame ][ c ][ ptKing ][ kp ];
	}
}

// undo psq values from scores (opening, endgames)
void Board::undoPsq( FineScore *scores ) const
{
	DMat pdmat[ phMax ];
	pdmat[ phOpening ] = pdmat[ phEndgame ] = 0;
	for (Color c = ctWhite; c <= ctBlack; c++ )
	{
		for (Piece p = ptPawn; p <= ptQueen; p++ )
		{
			Bitboard tmp = pieces( c, p );
			while ( tmp )
			{
				BitOp::popBit(tmp);

				// update dmat
				pdmat[ phOpening ] += (DMat)Tables::sign[c] * PSq::materialTables[ phOpening ][ p ];
				pdmat[ phEndgame ] += (DMat)Tables::sign[c] * PSq::materialTables[ phEndgame ][ p ];
			}
		}
	}

	// king
	for ( Color c = ctWhite; c <= ctBlack; c++ )
	{
		// update dmat
		pdmat[ phOpening ] += (DMat)Tables::sign[c] * PSq::materialTables[ phOpening ][ ptKing ];
		pdmat[ phEndgame ] += (DMat)Tables::sign[c] * PSq::materialTables[ phEndgame ][ ptKing ];
	}
	// FIXME: actually this will undo everything and just keep pure material
	// but that's actually what we want for special endgames
	for ( Phase p = phOpening; p <= phEndgame; p++ )
		scores[ p ] = ScorePack::initFine( Score(pdmat[ p ] ) );
		//scores[ p ] -= scores[ p ] - ScorePack::initFine( Score(pdmat[ p ] ) );
}

// parse from fen
const char *Board::fromFEN( const char *fen )
{
	// TODO: better validation
	assert( fen );

	clear();

	File x = 0;
	Rank y = 0;
	while ( *fen )
	{
		if ( *fen <= 32 )
		{
			fen++;
			continue;				// skip blanks
		}

		Rank ry = y^RANK8;

		switch ( *fen )
		{
		case '1':
		case '2':
		case '3':
		case '4':
		case '5':
		case '6':
		case '7':
		case '8':
			x += *fen - '0';
			break;
		case 'p':
			bb[ BBI( ctBlack, ptPawn ) ] |= BitOp::oneShl( SquarePack::init( x, ry ) );
			x++;
			break;
		case 'n':
			bb[ BBI( ctBlack, ptKnight ) ] |= BitOp::oneShl( SquarePack::init( x, ry ) );
			x++;
			break;
		case 'b':
			bb[ BBI( ctBlack, ptBishop ) ] |= BitOp::oneShl( SquarePack::init( x, ry ) );
			x++;
			break;
		case 'r':
			bb[ BBI( ctBlack, ptRook ) ] |= BitOp::oneShl( SquarePack::init( x, ry ) );
			x++;
			break;
		case 'q':
			bb[ BBI( ctBlack, ptQueen ) ] |= BitOp::oneShl( SquarePack::init( x, ry ) );
			x++;
			break;
		case 'k':
			bkingPos[ ctBlack ] = SquarePack::init( x, ry );
			x++;
			break;
		case 'P':
			bb[ BBI( ctWhite, ptPawn ) ] |= BitOp::oneShl( SquarePack::init( x, ry ) );
			x++;
			break;
		case 'N':
			bb[ BBI( ctWhite, ptKnight ) ] |= BitOp::oneShl( SquarePack::init( x, ry ) );
			x++;
			break;
		case 'B':
			bb[ BBI( ctWhite, ptBishop ) ] |= BitOp::oneShl( SquarePack::init( x, ry ) );
			x++;
			break;
		case 'R':
			bb[ BBI( ctWhite, ptRook ) ] |= BitOp::oneShl( SquarePack::init( x, ry ) );
			x++;
			break;
		case 'Q':
			bb[ BBI( ctWhite, ptQueen ) ] |= BitOp::oneShl( SquarePack::init( x, ry ) );
			x++;
			break;
		case 'K':
			bkingPos[ ctWhite ] = SquarePack::init( x, ry );
			x++;
			break;
		case '/':
			x = 0;
			y++;
			break;
		default:
			return 0;
		}
		fen++;
		if ( y >= 8 || (y == 7 && x >= 8) )
			break;
	}
	// consume extra chars
	while ( *fen && (*fen == '/' || *fen <= 32) )
		fen++;

	// parse stm
	bturn = (Color)( (*fen | 32) == 'b' ? ctBlack : ctWhite );
	if ( *fen )
		fen++;

	// skip blanks
	while ( *fen && *fen <= 32 )
		fen++;

	bool standardCastling = 0;
	while ( *fen && (*fen|32) >= 'a' && (*fen|32) <= 'q' )
	{
		char ch = *fen;
		switch( ch )
		{
		case 'K':
			standardCastling = 1;
			ch = 'H';
			break;
		case 'Q':
			standardCastling = 1;
			ch = 'A';
			break;
		case 'k':
			standardCastling = 1;
			ch = 'h';
			break;
		case 'q':
			standardCastling = 1;
			ch = 'a';
			break;
		}
		// always scan for outermost rooks if standardCastling is set - because of x-fen and Arena
		if ( !(ch & 32) )
		{
			// white
			File rf = (File)ch - 'A';
			File orf = rf;
			File kf = SquarePack::file( king( ctWhite ) );

			Square rsq = SquarePack::init( rf, RANK1 );
			Bitboard rooks = pieces( ctWhite, ptRook );
			if ( standardCastling || !(rooks & BitOp::oneShl(rsq) ) )
			{
				ch = 0;
				if ( rf >= EFILE )
				{
					// scan for kingside rook
					for ( File f = HFILE; f > kf; f-- )
					{
						Square sq = SquarePack::setFile( rsq, f );
						if ( rooks & BitOp::oneShl( sq ) )
						{
							rf = f;
							ch = 'A'+f;
							break;
						}
					}
				}
				else
				{
					// scan for queenside rook
					for ( File f = AFILE; f < kf; f++ )
					{
						Square sq = SquarePack::setFile( rsq, f );
						if ( rooks & BitOp::oneShl( sq ) )
						{
							rf = f;
							ch = 'A'+f;
							break;
						}
					}
				}
				if ( rf != orf )
					arenaMode = 1;
			}

			if ( ch )
			{
				bcastRights[ ctWhite ] = CastPack::loseRights( rf>kf, bcastRights[ ctWhite ] );
				bcastRights[ ctWhite ] |= CastPack::initFile( rf>kf, rf );
			}
		} else
		{
			// black
			File rf = (File)ch - 'a';
			File orf = rf;
			File kf = SquarePack::file( king( ctBlack ) );

			Square rsq = SquarePack::init( rf, RANK8 );
			Bitboard rooks = pieces( ctBlack, ptRook );
			if ( standardCastling || !(rooks & BitOp::oneShl(rsq) ) )
			{
				ch = 0;
				if ( rf >= EFILE )
				{
					// scan for kingside rook
					for ( File f = HFILE; f > kf; f-- )
					{
						Square sq = SquarePack::setFile( rsq, f );
						if ( rooks & BitOp::oneShl( sq ) )
						{
							rf = f;
							ch = 'a'+f;
							break;
						}
					}
				}
				else
				{
					// scan for queenside rook
					for ( File f = AFILE; f < kf; f++ )
					{
						Square sq = SquarePack::setFile( rsq, f );
						if ( rooks & BitOp::oneShl( sq ) )
						{
							rf = f;
							ch = 'a'+f;
							break;
						}
					}
				}
				if ( rf != orf )
					arenaMode = 1;
			}
			if ( ch )
			{
				bcastRights[ ctBlack ] = CastPack::loseRights( rf>kf, bcastRights[ ctBlack ] );
				bcastRights[ ctBlack ] |= CastPack::initFile( rf>kf, rf );
			}
		}
		// last Arena mode check: only if king is not E1/E8
		if ( standardCastling )
		{
			if ( castRights( ctWhite ) && SquarePack::file( king(ctWhite) ) != EFILE )
				arenaMode = 1;
			if ( castRights( ctBlack ) && SquarePack::file( king(ctBlack) ) != EFILE )
				arenaMode = 1;
		}
		fen++;
	}
	if ( *fen == '-' )
	{
		bcastRights[ ctWhite ] = bcastRights[ ctBlack ] = 0;
		fen++;
	}

	// skip blanks
	while ( *fen && *fen <= 32 )
		fen++;

	if ( *fen == '-' )
		// no ep
		fen++;
	else
		if ( (*fen|32) >= 'a' && (*fen|32) <= 'h' )
		{
			bep |= (*fen|32) - 'a';
			fen++;
			if ( *fen >= '1' || *fen <= '8' )
			{
				bep |= (('8' - *fen)^RANK8) << 3;
				fen++;
			}
		}

	// skip blanks
	while ( *fen && *fen <= 32 )
		fen++;

	// parse fifty move rule counter!
	if ( *fen >= '0' && *fen <= '9' )
		bfifty = (FiftyCount)strtol( fen, (char **)&fen, 10 );

	// skip blanks
	while ( *fen && *fen <= 32 )
		fen++;

	// consume move number
	if ( *fen >= '0' && *fen <= '9' )
	{
		long mn = strtol( fen, (char **)&fen, 10 );		// kept here - might be useful later
		curMove = mn > 0 ? (uint)mn : (uint)1;
	}

	update();
	if ( doesAttack<1>( turn(), king( flip(turn()) ) ) )
		return 0;
	return fen;
}

// store unsigned integer num=>buffer
// doesn't include null terminator
template<typename T> static void snum( char *&dst, T num)
{
	if  (!num)
	{
		*dst++ = '0';
		return;
	}
	char buf[256];
	char *rev = buf;
	while ( num > 0 )
	{
		*rev++ = (char)('0' + num % (T)10);
		num /= 10;
	}
	while (rev > buf)
	{
		*dst++ = rev[-1];
		rev--;
	}
}

std::string Board::toFEN() const
{
	char buf[256];
	*toFEN(buf) = 0;
	return buf;
}

char *Board::toFEN( char *dst ) const
{
	uint count = 0;					// space count
	for ( Square sq = 0; sq < 64; sq++ )
	{
		File f = SquarePack::file(sq);
		Rank r = SquarePack::rank(sq);
		if ( f == AFILE && r != RANK8 )
		{
			if ( count )
			{
				*dst++ = (char)('0' + count);
				count = 0;
			}
			*dst++ = '/';
		}
		Piece pt = PiecePack::type( piece(sq) );
		Color c = PiecePack::color( piece(sq) );
		if ( pt != ptNone )
		{
			if ( count )
			{
				*dst++ = (char)('0' + count);
				count = 0;
			}
			char ch = " PNBRQK"[pt];
			if ( c == ctBlack )
				ch |= 32;
			*dst++ = ch;
		}
		else
		{
			// delta-compress spaces
			count++;
		}
	}
	// flush last count
	if ( count )
		*dst++ = (char)('0' + count);

	*dst++ = ' ';
	*dst++ = turn() == ctWhite ? 'w' : 'b';
	*dst++ = ' ';

	// now add castling rights
	CastRights cr[2] = { castRights( ctWhite ), castRights( ctBlack ) };
	char xorm[2] = {0, 32};
	bool emptyCast = 1;
	for ( Color c = ctWhite; c <= ctBlack; c++ )
	{
		if ( !CastPack::allowed( cr[c] ) )
			continue;
		// do_cr
		emptyCast = 0;
		if ( CastPack::allowedShort( cr[c] ) )
		{
			if ( !frc )
				*dst++ = 'K' | xorm[c];
			else
				*dst++ = ('A' + CastPack::shortFile( cr[c] )) | xorm[c];
		}
		if ( CastPack::allowedLong( cr[c] ) )
		{
			if ( !frc )
				*dst++ = 'Q' | xorm[c];
			else
				*dst++ = ('A' + CastPack::longFile( cr[c] )) | xorm[c];
		}
	}
	if ( emptyCast )
		*dst++ = '-';
	*dst++ = ' ';

	// now ep square
	Square sq = epSquare();
	if ( !sq )
		*dst++ = '-';
	else
	{
		// add ep square
		*dst++ = (char)('a' + SquarePack::file(sq));
		*dst++ = '8' - (SquarePack::rank(sq) ^ RANK8);
	}
	*dst++ = ' ';

	// finally add fifty counter and move number
	snum( dst, fifty() );
	*dst++ = ' ';
	snum( dst, move() );

	return dst;
}

// do nullmove
void Board::doNullMove( UndoInfo &ui )
{
	assert( !inCheck() );

	initUndo( ui );
	bfifty++;

	// update ep square
	if ( bep )
	{
		bhash ^= Zobrist::epFile[ SquarePack::file( bep ) ];
		bep = 0;
	}

	// finally change stm
	bhash ^= Zobrist::turn;
	bturn = flip( bturn );
	assert( isValid() );
}

// undo nullmove
void Board::undoNullMove( const UndoInfo &ui )
{
	undoMove( ui );
}

// castling move is special
void Board::doCastlingMove( Move move, UndoInfo &ui, bool ischeck )
{
	assert( MovePack::isCastling( move ) && !inCheck() );
	assert( bhash == recomputeHash() );

	initUndo( ui );
	bfifty++;

	Color color = turn();
	Square kfrom = MovePack::from( move );
	Square kto = MovePack::to( move );

	assert( kfrom == king( color) );

	CastRights cr = castRights( color );

	b8 isShort = kto & 4;
	Square rto = isShort ? kto-1 : kto+1;
	Square rfrom = CastPack::rookSquare( kto, cr );

	// update delta material
	bdmat[ phOpening ] -= PSq::tables[ phOpening ][ color ][ ptKing ][ kfrom ];
	bdmat[ phEndgame ] -= PSq::tables[ phEndgame ][ color ][ ptKing ][ kfrom ];
	bdmat[ phOpening ] -= PSq::tables[ phOpening ][ color ][ ptRook ][ rfrom ];
	bdmat[ phEndgame ] -= PSq::tables[ phEndgame ][ color ][ ptRook ][ rfrom ];
	bdmat[ phOpening ] += PSq::tables[ phOpening ][ color ][ ptKing ][ kto ];
	bdmat[ phEndgame ] += PSq::tables[ phEndgame ][ color ][ ptKing ][ kto ];
	bdmat[ phOpening ] += PSq::tables[ phOpening ][ color ][ ptRook ][ rto ];
	bdmat[ phEndgame ] += PSq::tables[ phEndgame ][ color ][ ptRook ][ rto ];

	// note: xor is important here
	Bitboard kft = BitOp::oneShl( kfrom ) ^ BitOp::oneShl( kto );
	Bitboard rft = BitOp::oneShl( rfrom ) ^ BitOp::oneShl( rto );
//	assert( !(kft & rft) );	// can happen in frc

	// save occ and rook bitboards
	u8 bbi;
	bbi = bbiWOcc + color;
	ui.saveBB( bbi, bb[ bbi ] );
	bb[bbi] ^= kft ^ rft;

	bbi = BBI( color, ptRook );
	ui.saveBB( bbi, bb[ bbi ] );
	bb[bbi] ^= rft;

	// update zobrist hashes
	bhash ^= Zobrist::piece[ color ][ ptKing ][ kfrom ];
	bhash ^= Zobrist::piece[ color ][ ptKing ][ kto ];
	bhash ^= Zobrist::piece[ color ][ ptRook ][ rfrom ];
	bhash ^= Zobrist::piece[ color ][ ptRook ][ rto ];

	// save pieces
	ui.savePiece( kfrom, piece( kfrom ) );
	ui.savePiece( kto, piece( kto ) );
	ui.savePiece( rfrom, piece( rfrom ) );
	ui.savePiece( rto, piece( rto ) );

	// move pieces
	Piece kfp = bpieces[ kfrom ];
	Piece rfp = bpieces[ rfrom ];
	bpieces[ kfrom ] = ptNone;
	bpieces[ rfrom ] = ptNone;
	bpieces[ kto ] = kfp;
	bpieces[ rto ] = rfp;

	// save king state
	saveKingState( ui );
	// castling is irreversible
	// FIXME: while it's true, according to FIDE rules castling doesn't reset 50 rule counter!
	// => but I'm keeping it for now, don't wan't to lose because of GUIs that reset 50 rule on castling!
	// => NO! I want cheng to be 100% compliant
////	saveIrreversible( ui );
	// castling
	saveCastling<0>( ui );

	// update king position
	bkingPos[ color ] = kto;

	// lose castling rights
	bhash ^= Zobrist::cast[ color ][ cr ];
	bcastRights[ color ] = 0;

	// don't forget to update ep square
	if ( bep )
	{
		bhash ^= Zobrist::epFile[ SquarePack::file( bep ) ];
		bep = 0;
	}

	bcheck = ischeck;

	// finally flip turn
	bhash ^= Zobrist::turn;
	bturn = flip( turn() );

	if ( bcheck )
	{
		ui.saveBB( bbiEvMask, bb[ bbiEvMask ] );
		calcEvasMask();
	}

	assert( bcheck == doesAttack<1>( flip(bturn), king( bturn ) ) );
	assert( bhash == recomputeHash() );
	assert( bpawnHash == recomputePawnHash() );
	assert( isValid() );
}

void Board::doMove( Move move, UndoInfo &ui, bool isCheck )
{
	// note: usually there is 2x more special moves than there is quiet moves
	if ( MovePack::isCastling( move ) )
		return doCastlingMove( move, ui, isCheck );

	Square from = MovePack::from( move );
	Square to = MovePack::to( move );

	Piece pt = PiecePack::type( piece( from ) );		// piece type

	assert( pt >= ptPawn );

	Move isCap = MovePack::isCapture( move );
	Color stm = turn();

	switch( pt )
	{
	case ptPawn:
		if ( isCap )
			if ( stm == ctWhite )
				doMoveTemplate< ctWhite, 1, ptPawn >( move, from, to, ui, isCheck );
			else
				doMoveTemplate< ctBlack, 1, ptPawn >( move, from, to, ui, isCheck );
		else
			if ( stm == ctWhite )
				doMoveTemplate< ctWhite, 0, ptPawn >( move, from, to, ui, isCheck );
			else
				doMoveTemplate< ctBlack, 0, ptPawn >( move, from, to, ui, isCheck );
		break;
	case ptKnight:
		if ( isCap )
			if ( stm == ctWhite )
				doMoveTemplate< ctWhite, 1, ptKnight >( move, from, to, ui, isCheck );
			else
				doMoveTemplate< ctBlack, 1, ptKnight >( move, from, to, ui, isCheck );
		else
			if ( stm == ctWhite )
				doMoveTemplate< ctWhite, 0, ptKnight >( move, from, to, ui, isCheck );
			else
				doMoveTemplate< ctBlack, 0, ptKnight >( move, from, to, ui, isCheck );
		break;
	case ptBishop:
		if ( isCap )
			if ( stm == ctWhite )
				doMoveTemplate< ctWhite, 1, ptBishop >( move, from, to, ui, isCheck );
			else
				doMoveTemplate< ctBlack, 1, ptBishop >( move, from, to, ui, isCheck );
		else
			if ( stm == ctWhite )
				doMoveTemplate< ctWhite, 0, ptBishop >( move, from, to, ui, isCheck );
			else
				doMoveTemplate< ctBlack, 0, ptBishop >( move, from, to, ui, isCheck );
		break;
	case ptRook:
		if ( isCap )
			if ( stm == ctWhite )
				doMoveTemplate< ctWhite, 1, ptRook >( move, from, to, ui, isCheck );
			else
				doMoveTemplate< ctBlack, 1, ptRook >( move, from, to, ui, isCheck );
		else
			if ( stm == ctWhite )
				doMoveTemplate< ctWhite, 0, ptRook >( move, from, to, ui, isCheck );
			else
				doMoveTemplate< ctBlack, 0, ptRook >( move, from, to, ui, isCheck );
		break;
	case ptQueen:
		if ( isCap )
			if ( stm == ctWhite )
				doMoveTemplate< ctWhite, 1, ptQueen >( move, from, to, ui, isCheck );
			else
				doMoveTemplate< ctBlack, 1, ptQueen >( move, from, to, ui, isCheck );
		else
			if ( stm == ctWhite )
				doMoveTemplate< ctWhite, 0, ptQueen >( move, from, to, ui, isCheck );
			else
				doMoveTemplate< ctBlack, 0, ptQueen >( move, from, to, ui, isCheck );
		break;
	case ptKing:
		if ( isCap )
			if ( stm == ctWhite )
				doMoveTemplate< ctWhite, 1, ptKing >( move, from, to, ui, isCheck );
			else
				doMoveTemplate< ctBlack, 1, ptKing >( move, from, to, ui, isCheck );
		else
			if ( stm == ctWhite )
				doMoveTemplate< ctWhite, 0, ptKing >( move, from, to, ui, isCheck );
			else
				doMoveTemplate< ctBlack, 0, ptKing >( move, from, to, ui, isCheck );
		break;
	}
}

// undo move
void Board::undoMove( const UndoInfo &ui )
{
	// first restore hash, ep square and delta material
	bhash = ui.bhash;
	bdmat[ phOpening ] = ui.dmat[ phOpening ];
	bdmat[ phEndgame ] = ui.dmat[ phEndgame ];
	bep = ui.ep;

	// restore bitboards
	for (u8 i=0; i<ui.bbCount; i++)
		bb[ ui.bbi[i] ] = ui.bb[ i ];

	// restore pieces
	for (u8 i=0; i<ui.pieceCount; i++)
	{
		assert( PiecePack::type( ui.pieces[i] ) <= ptKing );
		bpieces[ ui.squares[i] ] = ui.pieces[i];
	}

	// restore turn flag
	bturn = flip( bturn );

	switch( ui.flags )
	{
	case 0:
		undoTemplate< 0 >( ui );
		break;
	case 1:
		undoTemplate< 1 >( ui );
		break;
	case 2:
		undoTemplate< 2 >( ui );
		break;
	case 3:
		undoTemplate< 3 >( ui );
		break;
	case 4:
		undoTemplate< 4 >( ui );
		break;
	case 5:
		undoTemplate< 5 >( ui );
		break;
	case 6:
		undoTemplate< 6 >( ui );
		break;
	case 7:
		undoTemplate< 7 >( ui );
		break;
	case 8:
		undoTemplate< 8 >( ui );
		break;
	case 9:
		undoTemplate< 9 >( ui );
		break;
	case 10:
		undoTemplate< 10 >( ui );
		break;
	case 11:
		undoTemplate< 11 >( ui );
		break;
	case 12:
		undoTemplate< 12 >( ui );
		break;
	case 13:
		undoTemplate< 13 >( ui );
		break;
	case 14:
		undoTemplate< 14 >( ui );
		break;
	case 15:
		undoTemplate< 15 >( ui );
		break;
	}
}

// returns 1 if move does check opponent king (from stm point of view)
bool Board::isCheck( Move m, Bitboard discovered ) const
{
	// TODO: rewrite to save instructions...
	Square from = MovePack::from(m);
	bool dc = (discovered & BitOp::oneShl( from )) != 0;
	Piece pt = PiecePack::type( piece( from ) );

	Square to = MovePack::to(m);
	Bitboard tom = BitOp::oneShl( to );
	Square okp = king( flip( turn() ) );
	Bitboard occ;

	switch( pt )
	{
	case ptPawn:
		if ( Tables::pawnAttm[ flip(turn()) ][ okp ] & tom )
			return 1;				// direct pawn check
		// if diagonal doesn't match => discovered check
		if ( dc && !(tom & Tables::ray[ okp ][ from ]) )
			return 1;
		// direct checks via promotion
		switch( MovePack::promo( m ) )
		{
		case ptKnight:
			return ( Tables::knightAttm[ okp ] & tom ) != 0;
		case ptBishop:
			return ( Magic::bishopAttm( okp, occupied() ^ BitOp::oneShl(from) ) & tom ) != 0;
		case ptRook:
			return ( Magic::rookAttm( okp, occupied() ^ BitOp::oneShl(from) ) & tom ) != 0;
		case ptQueen:
			return ( Magic::queenAttm( okp, occupied() ^ BitOp::oneShl(from) ) & tom ) != 0;
		}
		if ( !MovePack::isEpCapture( m ) )
			return 0;
		// en-passant check
		// FIXME: better than that
		occ = occupied();
		occ ^= BitOp::oneShl( from ) | BitOp::oneShl( SquarePack::epTarget( epSquare(), from ) )
			| BitOp::oneShl( epSquare() );
		if ( Magic::bishopAttm( okp, occ ) & diagSliders( turn() ) )
			return 1;
		return (Magic::rookAttm( okp, occ ) & orthoSliders( turn() )) != 0;
	case ptKnight:
		return dc || (tom & Tables::knightAttm[ okp ]);
	case ptBishop:
		return dc || ( (tom & Tables::diagAttm[okp] ) && (tom & Magic::bishopAttm( okp, occupied() )) );
	case ptRook:
		return dc || ( (tom & Tables::orthoAttm[okp] ) && (tom & Magic::rookAttm( okp, occupied() )) );
	case ptQueen:
		return (tom & Tables::queenAttm[okp]) && (tom & Magic::queenAttm( okp, occupied() ));
	case ptKing:
		// if diagonal doesn't match => discovered check
		// note: we don't have to check castling here first (FIXME: think about it some more)
		if ( dc && !(tom & Tables::ray[ okp ][ from ]) )
			return 1;
		if ( !MovePack::isCastling( m ) )
			return 0;
		// now check whether castling does check opponent king
		// FIXME: think about simplifying all this?
		{
			occ = occupied();
			File fto = SquarePack::file(to);
			File rf;
			Square rto = to;
			if ( fto >= EFILE )
			{
				// short (ks)
				assert( CastPack::allowedShort( castRights( turn() ) ) );
				rf = CastPack::shortFile( castRights( turn() ) );
				rto--;
			}
			else
			{
				// long (qs)
				assert( CastPack::allowedLong( castRights( turn() ) ) );
				rf = CastPack::longFile( castRights( turn() ) );
				rto++;
			}
			Square rfrom = SquarePack::setFile( to, rf );
			// we now have all the bits we need to update occupancy
			occ ^= BitOp::oneShl( rfrom ) ^ BitOp::oneShl( rto );
			occ ^= BitOp::oneShl( from ) ^ BitOp::oneShl( to );
			// only a just castled rook can give a direct check
			return (Magic::rookAttm( rto, occ ) & BitOp::oneShl( okp )) != 0;
		}
	}
	// keep compiler happy
	return 0;
}


// move to SAN
std::string Board::toSAN( Move m ) const
{
	char buf[256];
	*toSAN(buf, m) = 0;
	return buf;
}

char *Board::toSAN( char *dst, Move m ) const
{
	assert( (inCheck() ? isLegal<1, 0>( m, pins() ) : isLegal<0, 0>( m, pins() )) );

	m &= mmNoScore;

	if ( m == mcNull )
		return scpy( dst, "..." );

	bool ischeck = isCheck(m, discovered() );
	if ( MovePack::isCastling( m ) )
		dst = scpy( dst, SquarePack::file( MovePack::to( m ) ) >= EFILE ? "O-O" : "O-O-O" );
	else
	{
		Piece ptype = PiecePack::type( piece( MovePack::from( m ) ) );
		char c = "  NBRQK"[ptype];
		if ( c != ' ')
			*dst++ = c;

		Square f, t;
		f = MovePack::from( m );
		t = MovePack::to( m );
		File ff = SquarePack::file( f );
		Rank fr = SquarePack::rank( f );

		u8 diffFile = 0, diffRank = 0;

		MoveGen mg( *this );
		Move lm;
		while ( (lm = mg.next() ) != mcNone )
		{
			if ( m == lm )
				continue;						// skip self
			Square lf, lt;
			lf = MovePack::from( lm );
			lt = MovePack::to( lm );
			if ( lt != t )
				continue;						// not same target => skip
			Piece pt = PiecePack::type( piece( MovePack::from(lm) ) );
			if ( pt != ptype )
				continue;						// not same piece type => skip
			if ( MovePack::isPromo(m) && MovePack::isPromo(lm) && MovePack::promo(m) != MovePack::promo(lm) )
				continue;						// no need for ambiguity check for promotions to different piece
			File lff = SquarePack::file( lf );
			Rank lfr = SquarePack::rank( lf );

			if ( ff != lff )
				diffFile++;
			if ( fr != lfr )
				diffRank++;
		}

		if ( ptype == ptPawn && MovePack::isCapture(m) )
			diffFile = 1;						// force diffile for pawn captures

		if ( diffFile )
		{
			*dst++ = 'a' + ff;
			if ( diffRank == 1 )				// prefer file disambiguation
				diffRank = 0;
		}
		if ( diffRank )
			*dst++  = '8' - (fr ^ RANK8);
		if ( MovePack::isCapture(m) )
			*dst++ = 'x';

		*dst++ = 'a' + SquarePack::file(t);
		*dst++ = '8' - (SquarePack::rank(t) ^ RANK8);
	}
	Piece promo = MovePack::promo( m );
	if ( promo )
	{
		*dst++ = '=';
		*dst++ = "  NBRQ "[ promo ];
	}
	if ( ischeck )
	{
		// check for checkmate!!
		// FIXME: performance?!
		Board tb(*this);
		UndoInfo ui;
		tb.doMove( m, ui, 1 );
		MoveGen mg( tb );
		if ( mg.next() == mcNone )
			// checkmate
			*dst++ = '#';
		else
			*dst++ = '+';
	}
	return dst;
}

std::string Board::toUCI( Move m ) const
{
	char buf[16];
	*toUCI(buf, m) = 0;
	return buf;
}

char *Board::toUCI( char *dst, Move m ) const
{
	if ( frc && !arenaMode && MovePack::isCastling( m ) )
	{
		// handle FRC castling here (king captures rook)
		// fortunately this is compatible with both Arena and Shredder
		CastRights cr = castRights( turn() );
		m = MovePack::init( MovePack::from(m), CastPack::rookSquare( MovePack::to(m), cr ) );
		return MovePack::toUCI( dst, m, frc );
	}
	return MovePack::toUCI( dst, m, frc );
}

// move from UCI
Move Board::fromUCI( const std::string &str ) const
{
	const char *ptr = str.c_str();
	return fromUCI(ptr);
}

Move Board::fromUCI( const char *&c ) const
{
	if ( !*c )
		return mcNone;
	const char *old = c;			// we don't want to advance in case of failure
	if ( *c && (*c|32) == 'o' )
	{
		// parse castling move (frc usually)
		c++;
		if ( *c == '-' )
			c++;
		if ( !*c || (*c|32) != 'o' )
		{
			c = old;
			return mcNone;			// invalid
		}
		c++;
		if ( *c == '-' )
			c++;
		bool qs = *c && (*c|32) == 'o';	// queenside castling flag
		Color col = turn();
		CastRights cr = castRights(col);
		Square kp = king(col);
		Square to;
		if ( !qs )
		{
			// short
			if ( !CastPack::allowedShort(cr) )
			{
				c = old;
				return mcNone;
			}
			to = SquarePack::setFile( kp, GFILE );
		}
		else
		{
			// long
			if ( !CastPack::allowedLong(cr) )
			{
				c = old;
				return mcNone;
			}
			to = SquarePack::setFile( kp, CFILE );
		}
		return MovePack::initCastling( kp, to );
	}
	Move res = MovePack::fromUCI( c );
	if ( res == mcNone )
	{
		c = old;
		return res;
	}
	Square from = MovePack::from( res );
	Square to = MovePack::to( res );
	Piece pf = piece(from);
	if ( PiecePack::type( pf ) == ptKing )
	{
		// could be (frc) castling!
		Piece pt = piece(to);
		if ( abs(SquarePack::file(from) - SquarePack::file(to)) > 1 ||
			(PiecePack::type( pt ) == ptRook && PiecePack::color( pf ) == PiecePack::color( pt )) )
		{
			// try castling (king takes own rook)
			assert( PiecePack::color( pf ) == turn() && from == king( turn() ) );
			CastRights cr = castRights( turn() );
			if ( to > from )
			{
				// short
				if ( !CastPack::allowedShort(cr) )
				{
					c = old;
					return mcNone;
				}
				to = SquarePack::setFile( from, GFILE );
			}
			else
			{
				// long
				if ( !CastPack::allowedLong(cr) )
				{
					c = old;
					return mcNone;
				}
				to = SquarePack::setFile( from, CFILE );
			}
			return MovePack::initCastling( from, to );
		}
	}
	if ( res == mcNone )
	{
		c = old;
		return res;
	}
	Piece pt = piece(to);
	if ( PiecePack::type(pt) != ptNone )
		res |= mfCapture;
	else if ( PiecePack::type(pf) == ptPawn )
	{
		Square ep = epSquare();
		if ( ep && to == ep )
			res = MovePack::initEpCapture(from, to);
	}
	if ( res == mcNone )
		c = old;
	return res;
}

static bool parseSANSquare( const char *&ptr, int &file, int &rank )
{
	if ( !*ptr )
		return 0;

	rank = -1;
	file = (*ptr | 32)-'a';
	if ( file >= 0 && file < 8 )
	{
		if ( !*++ptr )
			return 1;
	}
	else
		file = -1;

	int rnk = *ptr - '1';
	if ( rnk >= 0 && rnk < 8 )
	{
		rank = rnk ^ RANK1;
		++ptr;
	}
	return 1;
}

// move from SAN
Move Board::fromSAN( const char *&ptr ) const
{
	Move res = mcNone;
	const char *old = ptr;	// save old pointer => we don't want to move ptr in case of failure!

	// piece to move
	Piece pt = ptPawn;		// default to pawn
	Piece promo = ptNone;	// default to none
	switch ( *ptr )
	{
	case 'P':				// I think some engines use this
		pt = ptPawn;
		ptr++;
		break;
	case 'N':
		pt = ptKnight;
		ptr++;
		break;
	case 'B':
		pt = ptBishop;
		ptr++;
		break;
	case 'R':
		pt = ptRook;
		ptr++;
		break;
	case 'Q':
		pt = ptQueen;
		ptr++;
		break;
	case 'K':
		pt = ptKing;
		ptr++;
		break;
	case 'O':
	case 'o':
		// castling!
		{
			ptr++;
			if ( !*ptr )
				return mcNone;
			if ( *ptr == '-' )
				ptr++;
			if ( !*ptr )
				return mcNone;
			if ( (*ptr|32) != 'o' )
				return mcNone;
			ptr++;
			// ok we have at least kingside castling now
			File toFile = GFILE;

			// but could be queenside castling
			if ( *ptr && (*ptr|32) == 'o' )
			{
				toFile = CFILE;
				ptr++;
			}
			else if ( ptr[1] && *ptr == '-' && (ptr[1]|32) == 'o' )
			{
				// long castling
				toFile = CFILE;
				ptr += 2;
			}

			Square from = king(turn());
			Square to = SquarePack::setFile(from, toFile);
			res = MovePack::initCastling(from, to);
			// finally check if legal (we're strict here!)
			bool legal = inCheck() ? isLegal<1, 0>(res, pins()) : isLegal<0, 0>(res, pins());
			if ( !legal )
				ptr = old;
			return legal ? res : mcNone;
		}
	default:;
	}
	// now the tricky part ... handle from and to... (squares and files)
	// pawn captures could be as simple as cxd or cd

	// parse from square (actually could be to square)
	int ff = -1, fr = -1;
	parseSANSquare( ptr, ff, fr );

	// skip -, x, :
	if ( *ptr == '-' || *ptr == 'x' || *ptr == ':' )
		++ptr;

	// parse to square
	int tf = -1, tr = -1;
	parseSANSquare( ptr, tf, tr );

	if ( ff >= 0 && fr >= 0 && (tf >= 0 || tr >= 0) )
	{
		// have full from square => fix piece type
		Square sq = SquarePack::init((File)ff, (Rank)fr);
		Piece p = piece(sq);
		pt = PiecePack::type(p);
		if ( pt == ptNone || PiecePack::color(p) != turn() )
			return mcNone;		// illegal from square
	}

	// only have from square => it's actually to square
	if ( tf < 0 && tr < 0 )
	{
		tf = ff; tr = fr;
		ff = fr = -1;
	}

	if ( tf < 0 || tr < 0 )
	{
		// illegal => we always need full destination square
		ptr = old;
		return mcNone;
	}

	// if we're moving a pawn, optionally parse promotion (OR: parse always?!)
	if ( pt == ptPawn && *ptr )
	{
		const char *optr = ptr;
		if ( *ptr == '=' )
			++ptr;
		if ( *ptr )
		{
			switch( *ptr | 32 )
			{
			case 'n':
				promo = ptKnight;
				break;
			case 'b':
				promo = ptBishop;
				break;
			case 'r':
				promo = ptRook;
				break;
			case 'q':
				promo = ptQueen;
				break;
			}
		}
		ptr = promo == ptNone ? optr : ptr+1;
	}

	// this STUPID useless statement here is just to silence vs2012 static analyzer which doesn't probably understand
	// const char *&
	if ( !ptr )
		return mcNone;

	// consume optional check sign
	if ( *ptr == '+' )
		ptr++;

	// consume optional doublecheck (not part of SAN) or checkmate sign
	if ( *ptr == '+' || *ptr == '#' )
		ptr++;

	// generate legal moves
	MoveGen mg(*this);
	Move moves[ maxMoves + 4 ];
	Move rmoves[ maxMoves + 4 ];
	MoveCount count = 0;
	MoveCount candCount = 0;
	Move m;
	while ( (m = mg.next()) != mcNone )
	{
		moves[count] = rmoves[count] = m;
		count++;
		if ( !MovePack::isCastling(m) )
			continue;

		// generate castling alternatives (king captures rook and rook captures king)
		Square from = MovePack::from(m);
		Square to = MovePack::to(m);

		CastRights cr = castRights(turn());
		// determine rookSquare
		Square rsq = CastPack::rookSquare(to, cr);

		// generate optional target file for king captures rook
		moves[count] = MovePack::setTo( m, rsq );
		rmoves[count++] = m;

		// generate optional target file for king captures rook
		moves[count] = MovePack::init( rsq, from ) | mfCastling;
		rmoves[count++] = m;
	}

	// finally try to resolve the move!
	for ( MoveCount i=0; i<count; i++ )
	{
		m = moves[i];
		Square from = MovePack::from(m);
		Square to = MovePack::to(m);
		Piece mp = PiecePack::type( piece(from) );
		if ( mp != pt )
			continue;			// not our move
		if ( MovePack::promo(m) != promo )
			continue;			// not our move => promo mismatch

		File mff = SquarePack::file(from);
		Rank mfr = SquarePack::rank(from);
		File mtf = SquarePack::file(to);
		Rank mtr = SquarePack::rank(to);

		if ( ff >= 0 && mff != ff )
			continue;
		if ( fr >= 0 && mfr != fr )
			continue;
		if ( tf >= 0 && mtf != tf )
			continue;
		if ( tr >= 0 && mtr != tr )
			continue;
		// ok we have a candidate move here
		res = rmoves[i];
		if ( ++candCount > 1 )
		{
			ptr = old;
			return mcNone;			// reject => ambiguous!
		}
	}

	if ( res == mcNone )
		ptr = old;

	return res;
}

Move Board::fromSAN( const std::string &str ) const
{
	const char *ptr = str.c_str();
	return fromSAN( ptr );
}

bool Board::compare( const Board &tmp ) const
{
	if ( tmp.bhash != bhash )
		return 0;
	if ( tmp.bpawnHash != bpawnHash )
		return 0;
	if ( memcmp( tmp.bb, bb, inCheck() ? sizeof(bb) : sizeof(Bitboard) * bbiEvMask ) != 0 )
		return 0;
	if ( memcmp( tmp.bpieces, bpieces, sizeof(bpieces) ) != 0 )
		return 0;
	if ( memcmp( tmp.bnpmat, bnpmat, sizeof(bnpmat) ) != 0 )
		return 0;
	if ( memcmp( tmp.bdmat, bdmat, sizeof(bdmat) ) != 0 )
		return 0;
	if ( memcmp( tmp.bkingPos, bkingPos, sizeof(bkingPos) ) != 0 )
		return 0;
	if ( tmp.bturn != bturn )
		return 0;
	if ( tmp.bep != bep )
		return 0;
/*	if ( tmp.bfifty != bfifty )
		return 0;*/
	if ( memcmp( tmp.bcastRights, bcastRights, sizeof(bcastRights) ) != 0 )
		return 0;
	if ( tmp.bcheck != bcheck )
		return 0;
	return 1;
}

// validate board (debug)
bool Board::isValid() const
{
	Board tmp(*this);

	Piece opieces[64];
	memcpy( opieces, bpieces, sizeof(opieces) );

	// make sure bit representations match squares
	for ( Square sq=0; sq<64; sq++)
	{
		Piece p = piece(sq);
		Piece pt = PiecePack::type( p );
		Color c = PiecePack::color( p );
		if ( pt == ptNone )
		{
			// make sure occ is clear
			if ( occupied() & BitOp::oneShl(sq) )
				return 0;
			continue;
		}
		if ( pt == ptKing )
		{
			// make sure king position matches
			if ( king(c) != sq )
				return 0;
			continue;
		}
		if ( !(pieces( c, pt ) & BitOp::oneShl(sq) ) )
			return 0;
	}

	tmp.update();

	if ( memcmp( tmp.bpieces, opieces, sizeof(opieces) ) != 0 )
		return 0;

	if ( tmp.bhash != bhash )
		return 0;
	if ( tmp.bpawnHash != bpawnHash )
		return 0;
	if ( memcmp( tmp.bb, bb, inCheck() ? sizeof(bb) : sizeof(Bitboard) * bbiEvMask ) != 0 )
		return 0;
	if ( memcmp( tmp.bpieces, bpieces, sizeof(bpieces) ) != 0 )
		return 0;
	if ( memcmp( tmp.bnpmat, bnpmat, sizeof(bnpmat) ) != 0 )
		return 0;
	if ( memcmp( tmp.bdmat, bdmat, sizeof(bdmat) ) != 0 )
		return 0;
	if ( memcmp( tmp.bkingPos, bkingPos, sizeof(bkingPos) ) != 0 )
		return 0;
	if ( tmp.bturn != bturn )
		return 0;
	if ( tmp.bep != bep )
		return 0;
	if ( tmp.bfifty != bfifty )
		return 0;
	if ( memcmp( tmp.bcastRights, bcastRights, sizeof(bcastRights) ) != 0 )
		return 0;
	if ( tmp.bcheck != bcheck )
		return 0;
	return 1;
}

// debug dump
void Board::dump() const
{
	for ( Square sq=0; sq<64; sq++)
	{
		File f = SquarePack::file( sq );
		if ( !f )
		{
			for ( File tf = 0; tf < 8; tf++ )
			{
				bool color = (BitOp::oneShl( sq + tf ) & lightSquares ) != 0;
				std::cout << (color ? "#####" : "     ");
			}
			std::cout << std::endl;
		}
		bool color = (BitOp::oneShl( sq ) & lightSquares ) != 0;
		const char *symbols = " PNBRQK  pnbrqk";
		std::cout << (color ? "# " : "  ") << symbols[ piece(sq) ] << (color ? " #" : "  ");
		if ( f == 7 )
		{
			std::cout << std::endl;
			for ( File tf = 0; tf < 8; tf++ )
			{
				color = (BitOp::oneShl( sq + tf - 7 ) & lightSquares ) != 0;
				std::cout << (color ? "#####" : "     ");
			}
			std::cout << std::endl;
		}
	}
	std::cout << "fen: " << toFEN() << std::endl;
}

template< bool evasion > bool Board::pseudoIsLegal( Move m, Bitboard pins ) const
{
	assert( !MovePack::isCastling(m) );

	Square kp = king( turn() );
	Square from = MovePack::from( m );
	Square to = MovePack::to( m );

	if ( MovePack::isEpCapture( m ) )
	{
		// FIXME: could do much better than that ...

		// en-passant requires special handling
		// make sure we don't leave our king in check
		Bitboard occ = occupied();
		occ ^= BitOp::oneShl( from ) | BitOp::oneShl( to ) |
			BitOp::oneShl( SquarePack::epTarget( epSquare(), from ) );

		// check for sliders only
		if ( Magic::bishopAttm( kp, occ ) & diagSliders( flip( turn() ) ) )
			return 0;

		return !( Magic::rookAttm( kp, occ ) & orthoSliders( flip( turn() ) ) );
	}

	if ( from == kp )
	{
		if ( evasion )
		{
			assert( from != to );
			Bitboard occ = occupied();
			occ ^= BitOp::oneShl( from ) | BitOp::oneShl( to );

			// check for sliders only
			if ( Magic::bishopAttm( to, occ ) & diagSliders( flip( turn() ) ) )
				return 0;

			if ( Magic::rookAttm( to, occ ) & orthoSliders( flip( turn() ) ) )
				return 0;
			// non-castling king move -> make sure target square isn't attacked
			return !doesAttack<0>( flip(turn()), to );
		}
		// non-castling king move -> make sure target square isn't attacked
		else return !doesAttack<1>( flip(turn()), to );
	}

	if ( evasion )
	{
		assert( from != to );
		Bitboard occ = occupied();
		Bitboard tomask = BitOp::oneShl( to );
		occ ^= BitOp::oneShl( from );
		if ( !MovePack::isCapture( m ) )
			occ ^= tomask;

		// check for sliders only
		if ( Magic::bishopAttm( kp, occ ) & diagSliders( flip( turn() ) ) & ~tomask )
			return 0;

		if ( Magic::rookAttm( kp, occ ) & orthoSliders( flip( turn() ) ) & ~tomask )
			return 0;
	}

	if ( !(BitOp::oneShl( from ) & pins) )
		return 1;				// not pinned => legal

	// pinned => make sure we're moving along the ray
	assert( Tables::direction[ kp ][ from ] != dirNone );
	return Tables::direction[ kp ][ from ] == Tables::direction[ kp ][ to ];
}

template< bool evasion, bool killer > bool Board::iisLegal( Move m, Bitboard pins ) const
{
	assert( !killer || !MovePack::isSpecial(m) );
	// FIXME: refactor?
	if ( !killer && MovePack::isCastling( m ) )
	{
		// FIXME: better or ok?
		if ( inCheck() || (m & (mfCapture | mfEpCapture | mmPromo) ) )
			return 0;	// ignore junk flags too
		CastRights cr = castRights( turn() );
		Move cm = mcNone;
		File tf = SquarePack::file( MovePack::to(m) );
		if ( tf >= EFILE )
		{
			if ( !CastPack::allowedShort( cr ) )
				return 0;
			// short
			Move *nm = (turn() == ctWhite) ?
				MoveGen::genCastling< ctWhite >( CastPack::shortFile(cr), *this, &cm )
			:
				MoveGen::genCastling< ctBlack >( CastPack::shortFile(cr), *this, &cm );
			if ( nm == &cm )
				return 0;
		}
		else
		{
			// long
			if ( !CastPack::allowedLong( castRights( turn() ) ) )
				return 0;
			Move *nm = ( turn() == ctWhite ) ?
				MoveGen::genCastling< ctWhite >( CastPack::longFile(cr), *this, &cm )
			:
				MoveGen::genCastling< ctBlack >( CastPack::longFile(cr), *this, &cm );
			if ( nm == &cm )
				return 0;
		}
		return cm == m;
	}
	Piece pp = MovePack::promo( m );
	if ( pp == ptPawn || pp >= ptKing )
		return 0;			// junk value in promotion
	Square from = MovePack::from(m);
	Square to = MovePack::to(m);

	if ( from == to )
		return 0;			// illegal

	Bitboard fm = BitOp::oneShl( from );
	if ( !(fm & occupied(turn()) ) )
		return 0;			// illegal from
	Bitboard tm = BitOp::oneShl( to );
	// for killers exclude captures as well
	if ( tm & (killer ? occupied() : occupied(turn()) ) )
		return 0;			// exclude (self)-captures
	Piece ptype = PiecePack::type( piece( from ) );
	assert( ptype >= ptPawn && ptype <= ptKing );

	if ( ptype == ptPawn )
	{
		// assume position (from) is valid
		if ( MovePack::isEpCapture(m) )
		{
			if (m & (mfCapture | mfCastling | mmPromo) )
				return 0;				// illegal flags
			// the following check is useless BUT I want this to be bullet proof
			Square ep = epSquare();
			if ( !ep || to != ep )
				return 0;				// illegal -> no ep or bad to square
			Bitboard epbb = BitOp::oneShl( ep );
			if ( (occupied() & epbb) || !(BitOp::shiftBackward( turn(), epbb ) & pieces(flip(turn()), ptPawn) ) )
				return 0;				// illegal: ep square occupied OR doesn't have opponent pawn where expected

			return (Tables::pawnAttm[ turn() ][ from ] & epbb) &&
					pseudoIsLegal< evasion >( m, pins );
		}
		// ok en-passant validation was piece of cake
		if ( evasion && !(tm & evasions()) )
			return 0;							// non-doublecheck evasion needs proper target square
		if ( MovePack::isCapture(m) )
		{
			// pawn capture
			if ( m & (mfEpCapture | mfCastling) )
				return 0;			// illegal junk flags
			if ( king( flip(turn()) ) == to )
				return 0;			// illegal king capture
			if ( !( tm & occupied(flip(turn())) & Tables::pawnAttm[ turn() ][ from ] ) )
				return 0;			// not a capture/bad dir
			// finally check promotion
			if ( !pp )
				return (tm & pawnPromoSquares) ? 0 : pseudoIsLegal< evasion >( m, pins );
			return (turn() == ctWhite ?
				SquarePack::relRank< ctWhite >( from ) == RANK7 :
				SquarePack::relRank< ctBlack >( from ) == RANK7)
				? pseudoIsLegal< evasion >( m, pins ) : 0;
		}
		else
		{
			// normal pawn move
			if ( m & mfCastling )
				return 0;			// illegal junk flags

			if ( SquarePack::isRank1Or8( from ) )
				return 0;

			Rank irank = (turn() == ctWhite) ?
				SquarePack::relRank< ctWhite >( from ) : SquarePack::relRank< ctBlack >( from );

			// advance by one
			from = ( turn() == ctWhite ) ?
				SquarePack::advanceRank< ctWhite, 1 >( from )
			:
				SquarePack::advanceRank< ctBlack, 1 >( from );

			if ( BitOp::oneShl( from ) & occupied() )
				return 0;				// forward square blocked
			if ( to == from )
			{
				// is advance by one
				Rank tr = turn() == ctWhite ?
					SquarePack::relRank< ctWhite >( to ) :
					SquarePack::relRank< ctBlack >( to );

				return ( pp ? tr == RANK8 : tr != RANK8 )
					? pseudoIsLegal< evasion >( m, pins ) : 0;
			}
			if ( pp )
				return 0;		// two push must not be promotion

			if ( SquarePack::isRank1Or8( from ) )
				return 0;

			if ( irank != RANK2 )
				return 0;		// push by two not possible

			// further advance by one
			from = ( turn() == ctWhite ) ?
				SquarePack::advanceRank< ctWhite, 1 >( from )
			:
				SquarePack::advanceRank< ctBlack, 1 >( from );
			return ( from == to && !(BitOp::oneShl( from ) & occupied()) )
				? pseudoIsLegal< evasion >( m, pins ) : 0;
		}
	}
	else
	{
		if ( m & (mfEpCapture | mfCastling | mmPromo) )
			return 0;				// junk flags
		if ( evasion && ptype != ptKing && !(tm & evasions()) )
			return 0;							// non-doublecheck evasion needs proper target square

		if ( !killer )
		{
			if ( MovePack::isCapture(m) )
			{
				if ( king( flip(turn()) ) == to )
					return 0;			// illegal king capture
				if ( !( tm & occupied(flip(turn())) ) )
					return 0;			// not a capture
			}
			else
				// not a capture
				if ( tm & occupied(flip(turn())) )
					return 0;			// not a capture but capturing
		}
		// check direction/target validity
		if ( !(Tables::moveValid[from][to] & (1U << ptype)) )
			return 0;
		// and finally check between mask validity for sliders
		if ( PiecePack::isSlider( ptype ) && (Tables::between[from][to] & ~(fm | tm) & occupied()) )
			return 0;
	}
	return pseudoIsLegal< evasion >( m, pins );
}

template< bool evasion, bool killer > bool Board::isLegal( Move m, Bitboard pins ) const
{
	return iisLegal< evasion, killer >( m, pins );
}

// set side to move (used in xboard edit mode)
void Board::setTurn( Color c )
{
	bturn = c;
}

// clear pieces (used in xboard edit mode)
void Board::clearPieces()
{
	clear();
	memset( bpieces, 0, sizeof(bpieces) );
}

// set piece of color at square (used in xboard edit mode)
bool Board::setPiece( Color c, Piece p, Square sq )
{
	if ( c > ctBlack || p > ptKing || sq > 63 )
		return 0;
	bpieces[ sq ] = PiecePack::init( c, p );
	return 1;
}

// update bitboards from board state (used in xboard edit mode)
void Board::updateBitboards()
{
	memset( bb, 0, sizeof(bb) );
	bkingPos[ ctWhite ] = bkingPos[ ctBlack ] = 0;
	for ( Square sq = 0; sq < 64; sq++ )
	{
		Piece p = piece( sq );
		Piece pt = PiecePack::type( p );
		if ( pt == ptNone )
			continue;
		Bitboard msk = BitOp::oneShl(sq);
		Color pc = PiecePack::color( p );
		if ( pt == ptKing )
		{
			bb[ bbiWOcc + pc ] |= msk;
			bkingPos[ pc ] = sq;
			continue;
		}
		bb[ BBI( pc, pt ) ] |= msk;
		bb[ bbiWOcc + pc ] |= msk;
	}
	update();
}

// swap white<=>black
void Board::swap()
{
	for ( Square s=0; s<32; s++)
	{
		Square fs = SquarePack::flipV(s);
		std::swap( bpieces[s], bpieces[fs] );
	}
	for ( Square s=0; s<64; s++)
	{
		Piece p = bpieces[s];
		if ( PiecePack::type(p) == ptNone )
			continue;
		bpieces[s] ^= (Piece)1 << psColor;
	}
	std::swap( bcastRights[ ctWhite ], bcastRights[ ctBlack ] );
	if ( bep )
		bep = SquarePack::flipV(bep);
	bturn = flip(bturn);
	updateBitboards();
}

void Board::calcEvasMask()
{
	assert( inCheck() );
	// evasions -- prepare some variables
	Bitboard tmp = checkers();
	assert( tmp );
	if ( (tmp & ((Bitboard)0-tmp)) == tmp )		// BitOp::popCount( tmp ) < 2
	{
		assert( BitOp::popCount( tmp ) < 2 );
		// generate target mask
		Square kp = king( turn() );
		// necessary to use orig checkers because of knight checks
		bb[ bbiEvMask ] = tmp;
		Square sq = BitOp::popBit( tmp );
		// & ~kp to avoid self-captures of king
		bb[ bbiEvMask ] |= Tables::between[ kp ][ sq ] & BitOp::noneShl(kp);
	} else bb[ bbiEvMask ] = 0;
}

// instantiate
template bool Board::pseudoIsLegal<0>( Move, Bitboard ) const;
template bool Board::pseudoIsLegal<1>( Move, Bitboard ) const;
template bool Board::isLegal<0, 0>( Move, Bitboard ) const;
template bool Board::isLegal<0, 1>( Move, Bitboard ) const;
template bool Board::isLegal<1, 0>( Move, Bitboard ) const;
template bool Board::isLegal<1, 1>( Move, Bitboard ) const;

// assign castling rights automatically
void Board::autoCastlingRights()
{
	// TODO: implement

}

}
