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

#include "move.h"
#include "chtypes.h"
#include "tables.h"
#include "utils.h"

namespace cheng4
{

static Square parseSquare( const char *&ptr )
{
	File f;
	Rank r;
	if ( *ptr < 'a' || *ptr > 'h' )
		return sqInvalid;
	f = *ptr++ - 'a';
	if ( *ptr < '1' || *ptr > '8' )
		return sqInvalid;
	r = ('8' - *ptr++) ^ RANK8;
	return SquarePack::init( f, r );
}

// MovePack

std::string MovePack::toUCI( Move m, bool frc )
{
	char buf[16];
	*toUCI(buf, m, frc) = 0;
	return buf;
}

char *MovePack::toUCI( char *dst, Move m, bool frc )
{
	if ( m == mcNull || m == mcNone )
		return scpy(dst, "0000");

	if ( frc && isCastling(m) )
		return SquarePack::file( MovePack::to(m) ) >= EFILE ? scpy(dst, "O-O") : scpy(dst, "O-O-O");

	Square from = MovePack::from( m );
	Square to = MovePack::to( m );
	*dst++ = (char)( SquarePack::file( from ) + 'a' );
	*dst++ = (char)( '8' - (SquarePack::rank( from ) ^ RANK8) );
	*dst++ = (char)( SquarePack::file( to ) + 'a' );
	*dst++ = (char)( '8' - (SquarePack::rank( to ) ^ RANK8) );
	Piece promo = MovePack::promo( m );
	if ( promo )
		*dst++ = "  nbrq"[promo];
	return dst;
}

Move MovePack::fromUCI( const std::string &str )
{
	const char *ptr = str.c_str();
	return fromUCI( ptr );
}

Move MovePack::fromUCI( const char *&ptr )
{
	const char *old = ptr;		// we don't want to advance on failure
	Square from, to;
	from = parseSquare( ptr );
	if ( from == sqInvalid )
	{
		ptr = old;
		return mcNone;
	}
	to = parseSquare( ptr );
	if ( to == sqInvalid )
	{
		ptr = old;
		return mcNone;
	}
	if ( !*ptr )
		return MovePack::init( from, to );
	Piece promo = ptNone;
	switch( *ptr | 32 )
	{
	case 'n':
		promo = ptKnight;
		ptr++;
		break;
	case 'b':
		promo = ptBishop;
		ptr++;
		break;
	case 'r':
		promo = ptRook;
		ptr++;
		break;
	case 'q':
		promo = ptQueen;
		ptr++;
		break;
	}
	return MovePack::initPromo( from, to, promo );
}

}
