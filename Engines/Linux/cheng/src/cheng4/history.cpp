#include "history.h"
#include <memory.h>

namespace cheng4
{

// History

void History::clear()
{
	memset( history, 0, sizeof(history) );
}

void History::add( const Board &b, Move m, i32 depth )
{
	assert( !MovePack::isSpecial(m) );

	Piece p = b.piece( MovePack::from(m) );
	Color c = PiecePack::color( p );
	Piece pt = PiecePack::type(p);
	assert( pt != ptNone );
	assert( c == b.turn() );

	i32 val = depth*depth;			// causing a cutoff higher in the tree is more important
	if ( depth < 0 )
		val = -val;

	i16 &h = history[ c ][ pt ][ MovePack::to(m) ];
	i32 nval = (i32)h + val;
	while ( abs(nval) > historyMax )
	{
		nval /= 2;
		for (p=ptPawn; p<=ptKing; p++)
			for ( uint i=0; i<64; i++ )
				history[c][p][i] /= 2;
	}
	h = (i16)nval;
}

}
