#pragma once

#include "board.h"

namespace cheng4
{

struct History
{
	static const i16 historyMax = 2047;
	static const i16 historyMin = -historyMax;

	// history table [stm][piecetype][square]
	i16 history[ ctMax ][ ptMax ][ 64 ];

	inline History() {}
	explicit inline History( void * /*zeroInit*/ ) { clear(); }

	// add move which caused cutoff/sub move which didn't
	void add( const Board &b, Move m, i32 depth );

	// get move ordering score
	inline i32 score( const Board &b, Move m ) const
	{
		Piece p = b.piece( MovePack::from( m ) );
		return history[ b.turn() ][ PiecePack::type(p) ][ MovePack::to(m) ];
	}

	// clear table
	void clear();
};

}
