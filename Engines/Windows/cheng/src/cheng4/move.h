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
#include <string>

namespace cheng4
{

enum MoveConstants
{
	maxMoves	=	256,
	maxCaptures	=	128
};

// move is packed u32
// bit 0..5: from
// bit 6..11: to
// bit 12..14: promo: ChessPiece
// bit 15..16: special flags: 0 = normal move, 1 = castling, 2 = capture, 3 = ep capture
// bit 17..31: static score used for move ordering

// move masks
const Move mmFrom		=	63;
const Move mmTo			=	63<<6;
const Move mmPromo		=	7<<12;
const Move mmMove		=	mmFrom | mmTo | mmPromo;
const Move mmBook		=	0xffff;			// including castling
const Move mmSpecial	=	7<<15;
const Move mmNoScore	=	0x3ffff;		// remove score part
const Move mmScore		=	0xfffc0000;

enum MoveShifts
{
	msFrom		=	0,
	msTo		=	6,
	msPromo		=	12,
	msSpecial	=	15,
	msScore		=	18
};

// move flags
const Move mfNone		=	0;
const Move mfCastling	=	1 << msSpecial;
const Move mfCapture	=	2 << msSpecial;
const Move mfEpCapture	=	4 << msSpecial;

// move constants
const Move mcNone		=	0;
const Move mcNull		=	0x1fff;

typedef uint MoveScore;

// singleton
struct MovePack
{
	static inline Square from( Move move )
	{
		return move & mmFrom;
	}

	static inline Move setTo( Move move, Square sq )
	{
		Move res = move & ~( (Move)63 << msTo );
		return res |= (Move)(sq & 63) << msTo;
	}

	static inline Square to( Move move )
	{
		return (Square)( (move & mmTo) >> msTo );
	}

	static inline Piece promo( Move move )
	{
		return (Piece)( (move & mmPromo) >> msPromo );
	}

	static inline Move isCastling( Move move )
	{
		return move & mfCastling;
	}

	static inline Move isCapture( Move move )
	{
		return move & (mfCapture|mfEpCapture);
	}

	static inline Move isEpCapture( Move move )
	{
		return move & mfEpCapture;
	}

	// special move: capture, ep capture, castling, promotion
	static inline Move isSpecial( Move move )
	{
		return move & (mmSpecial | mmPromo);
	}

	static inline Move isPromo( Move move )
	{
		return move & mmPromo;
	}

	// is promotion or ep capture?
	static inline Move isPromoOrEp( Move move )
	{
		return move & (mmPromo | mfEpCapture);
	}

	static inline Move init( Square from, Square to )
	{
		return (Move)(from | (to << msTo));
	}

	static inline Move initCapture( Square from, Square to )
	{
		return (Move)(from | (to << msTo) | mfCapture);
	}

	static inline Move initPromo( Square from, Square to, Piece promo )
	{
		return (Move)(from | (to << msTo) | (promo << msPromo));
	}

	static inline Move initPromoCapture( Square from, Square to, Piece promo )
	{
		return (Move)(from | (to << msTo) | (promo << msPromo) | mfCapture);
	}

	static inline Move initEpCapture( Square from, Square to )
	{
		return (Move)(from | (to << msTo) | mfEpCapture);
	}

	static inline Move initCastling( Square from, Square to )
	{
		return (Move)(from | (to << msTo) | mfCastling);
	}

	static inline Move initScore( MoveScore score )
	{
		return (Move)(score << msScore);
	}

	template< bool capture > static inline Move initMove( Square from, Square to )
	{
		return capture ? initCapture( from, to ) : init( from, to );
	}
	template< bool capture > static inline Move initPromoMove( Square from, Square to, Piece promo )
	{
		return capture ? initPromoCapture( from, to, promo ) : initPromo( from, to, promo );
	}

	static inline bool equal( Move m0, Move m1 )
	{
		return (m0 & mmMove) == (m1 & mmMove);
	}

	static inline bool equalBook( Move m0, BookMove m1 )
	{
		return (BookMove)(m0 & mmBook) == m1;
	}

	// convert to UCI move (doesn't add null terminator)
	static char *toUCI( char *dst, Move m, bool frc = 0 );
	static std::string toUCI( Move m, bool frc = 0 );
	// from UCI move
	static Move fromUCI( const std::string &str );
	static Move fromUCI( const char *&ptr );
};

}
