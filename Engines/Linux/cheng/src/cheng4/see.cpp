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
#include <algorithm>

namespace cheng4
{

	// Board/see

	// static exchange evaluator
	template< bool fast > int Board::see( Move m ) const
	{
		if ( MovePack::isCastling( m ) || MovePack::isEpCapture( m ) )
			return 0;
		Square from = MovePack::from( m );
		Piece pieceFrom = piece( from );
		Piece pfrom = PiecePack::type( pieceFrom );
		Square to = MovePack::to( m );

		Piece pto = from == to ? (Piece)ptNone : PiecePack::type( piece( to ) );

		int score = Tables::seeValue[ pto ];

		// early exit when using fast (sign) see
		if ( fast && Tables::seeValue[ pfrom ] <= score )
			return 0;

		Bitboard occ = occupied();
		// get attack mask
		// do initial capture/move
		occ &= BitOp::noneShl( from );
		// note: no need to clear 'to' bit
		// now get all attackers
		Bitboard attk = allAttacksTo( to, occ );

		// FIXME: use turn() instead? => NO (so that I can use see in eval)
		Color stm = flip( PiecePack::color( pieceFrom ) );

		// exit sooner if undefended (FIXME: remove this? => TODO: measure)
		if ( !(attk & pieces(stm)) )
			// undefended => winning
			return score;

		// score list (swap)
		int scores[64];
		int index = 1;

		Bitboard diagSliders = this->diagSliders();
		Bitboard orthoSliders = this->orthoSliders();
		Bitboard allSliders = diagSliders | orthoSliders;

		scores[ 0 ] = score;
		int value = Tables::seeValue[ pfrom ];
		do
		{
			// TODO: refactor/simplify inner loop!
			Piece p;
			for ( p = ptPawn; p <= ptQueen; p++ )
			{
				Bitboard tmp = attk & pieces( stm, p );
				if ( !tmp )
					continue;

				Bitboard attacker = (tmp & ((Bitboard)0-tmp));		// isolate LSBit of attacker
				assert( attk & attacker );
				assert( occ & attacker );
				attk ^= attacker;
				occ ^= attacker;

				// add hidden attackers
				// FIXME: use smart extraction using dir and getLSB/getMSB?
				// probably not as these may be slow on certain machines
				switch(p)
				{
				case ptPawn:
				case ptBishop:
					attk |= Magic::bishopAttm( to, occ ) & diagSliders & occ;
					break;
				case ptRook:
					attk |= Magic::rookAttm( to, occ ) & orthoSliders & occ;
					break;
				case ptQueen:
					// only queen can hide both diagonal and orthogonal sliders
					attk |= Magic::queenAttm( to, occ ) & allSliders & occ;
					break;
				}

				assert( index < 64 );
				scores[index] = -scores[ index-1 ] + value;
				index++;

				value = Tables::seeValue[ p ];
				break;
			}

			if ( p == ptKing )
			{
				if ( (attk & BitOp::oneShl(king( stm ))) )
				{
					// king does capture
					assert( index < 64 );
					scores[index] = -scores[ index-1 ] + value;
					index++;

					if ( attk & pieces( flip(stm) ) )
					{
						scores[index] = Tables::seeValue[p];		// king recaptured!
						index++;
					}
				}
				break;
			}
			stm = flip( stm );
		} while ( attk & pieces(stm) );

		// finally negamax score list
		while ( --index > 0 )
			scores[ index-1 ] = std::min( scores[ index-1 ], -scores[ index ] );

		return scores[0];
	}

	template int Board::see<0>( Move m ) const;
	template int Board::see<1>( Move m ) const;

}
