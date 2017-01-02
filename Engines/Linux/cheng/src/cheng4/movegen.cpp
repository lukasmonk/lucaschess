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

#include "movegen.h"
#include <algorithm>
#include <functional>

namespace cheng4
{

// use insertion sort instead of std::sort (0.8% faster on Win/icc)
static void isort( Move *buf, MoveCount count )
{
	for ( MoveCount i = 1; i<count; i++ )
	{
		Move m = buf[i];
		MoveCount j = i;
		while ( j > 0 && buf[j-1] < m )
		{
			buf[j] = buf[j-1];
			j--;
		}
		buf[j] = m;
	}
}

// MoveGen

static const uint phaseNormalLegal[] = {
	mpCapNoSort,
	mpBufferLegal,
	mpCastling,
	mpCastlingBuffer,
	mpQuietNoSort,
	mpBufferLegal,
	mpDone
};

static const uint phaseNormalNoCastlingLegal[] = {
	mpCapNoSort,
	mpBufferLegal,
	mpQuietNoSort,
	mpBufferLegal,
	mpDone
};

static const uint phaseEvasLegal[] = {
	mpEvasNoSort,
	mpEvasBufferLegal,
	mpDone
};

static const uint phaseNormal[] = {
	mpHash,
	mpCap,
	mpCapBuffer,
	mpKiller1,
	mpKiller2,
	mpCastling,
	mpCastlingBuffer,
	mpQuiet,
	mpQuietBuffer,
	mpBadCap,
	mpBadCapBuffer,
	mpDone
};

static const uint phaseNormalNoCastling[] = {
	mpHash,
	mpCap,
	mpCapBuffer,
	mpKiller1,
	mpKiller2,
	mpQuiet,
	mpQuietBuffer,
	mpBadCap,
	mpBadCapBuffer,
	mpDone
};

static const uint phaseEvas[] = {
	mpHash,
	mpKiller1,
	mpKiller2,
	mpEvas,
	mpEvasBuffer,
	mpDone
};

static const uint phaseQCapsNoCastling[] = {
	mpHash,
	mpQCap,
	mpQCapBuffer,
	mpDone
};

static const uint phaseQCaps[] = {
	mpHash,
	mpQCap,
	mpQCapBuffer,
	mpCastling,
	mpCastlingBuffer,
	mpDone
};

static const uint phaseQCapsChecks[] = {
	mpHash,
	mpQCap,
	mpQCapBuffer,
	mpQChecks,
	mpQChecksBuffer,
	mpDone
};

// legal only version
MoveGen::MoveGen( const Board &b_ ) : mode( mmLegal ), board(b_), killer(0), history(0), genMoveCount(0)
{
	dcMask = board.discovered();
	pin = board.pins();
	phPtr = board.inCheck() ? phaseEvasLegal : (board.canCastle() ? phaseNormalLegal : phaseNormalNoCastlingLegal );
}

MoveGen::MoveGen(const Board &b_, const Killer &killer_, const History &history_, uint mode_)
	: mode(mode_), board(b_), killer(&killer_), history(&history_), genMoveCount(0)
{
	dcMask = board.discovered();
	pin = board.pins();

	assert( !((killer->hashMove | killer->killers[0] | killer->killers[1]) & mmScore) );

	switch( mode )
	{
	case mmNormal:
		phPtr = board.inCheck() ? phaseEvas : (board.canCastle() ? phaseNormal : phaseNormalNoCastling);
		break;
	case mmLegal:
		phPtr = board.inCheck() ? phaseEvasLegal : (board.canCastle() ? phaseNormalLegal : phaseNormalNoCastlingLegal );
		break;
	case mmQCaps:
		phPtr = board.inCheck() ? phaseEvas : (board.canCastle() ? phaseQCaps : phaseQCapsNoCastling);
		break;
	case mmQCapsChecks:
		phPtr = board.inCheck() ? phaseEvas : phaseQCapsChecks;
		break;
	}
}

bool MoveGen::alreadyGenerated( Move m )
{
	m &= mmNoScore;
	for (MoveCount i=0; i<genMoveCount; i++)
	{
		assert( !(genMoves[i] & ~mmNoScore) );
		if ( m == genMoves[i] )
			return 1;
	}
	return 0;
}

Move MoveGen::next()
{
	Move res;
loop:
	switch( *phPtr )
	{
	case mpHash:
		res = killer->hashMove;
		phPtr++;
		if ( !res || !(board.inCheck() ? board.isLegal<1,0>( res, pins() ) : board.isLegal<0,0>( res, pins() ) ) )
			goto loop;

		genMoves[ genMoveCount++ ] = res;
		break;
	case mpKiller1:
		res = killer->killers[0];
		phPtr++;
		if ( !res || alreadyGenerated(res) ||
			!(board.inCheck() ? board.isLegal<1,1>( res, pins() ) : board.isLegal<0,1>( res, pins() ) ) )
			goto loop;
		genMoves[ genMoveCount++ ] = res;
		break;
	case mpKiller2:
		res = killer->killers[1];
		phPtr++;
		if ( !res || alreadyGenerated(res) ||
			!(board.inCheck() ? board.isLegal<1,1>( res, pins() ) : board.isLegal<0,1>( res, pins() ) ) )
			goto loop;
		genMoves[ genMoveCount++ ] = res;
		break;
	case mpCapNoSort:
		count = generateCaptures( board, moveBuf );
		index = 0;
		phPtr++;
		goto loop;
		break;
	case mpCap:
		count = generateCaptures( board, moveBuf );
		index = badCapCount = 0;
		scoreCaptures();
		phPtr++;
		goto loop;
		break;
	case mpBadCap:
		index = 0;
		phPtr++;
		goto loop;
		break;
	case mpQCap:
		count = generateCaptures( board, moveBuf, 0 );
		index = 0;
		scoreCaptures();
		phPtr++;
		goto loop;
		break;
	case mpQChecks:
		count = board.turn() == ctWhite ?
			generateChecks< ctWhite >( moveBuf ) : generateChecks< ctBlack >( moveBuf );
		scoreChecks();
		index = 0;
		phPtr++;
		goto loop;
		break;
	case mpCastling:
		assert( board.canCastle() );
		index = 0;
		count = board.turn() == ctWhite ?
			generateCastling< ctWhite >( board, moveBuf )
			: generateCastling< ctBlack >( board, moveBuf );
		phPtr++;
		goto loop;
		break;
	case mpQuietNoSort:
		count = generateQuiet( board, moveBuf );
		index = 0;
		phPtr++;
		goto loop;
		break;
	case mpQuiet:
		count = generateQuiet( board, moveBuf );
		scoreQuiet();
		index = 0;
		phPtr++;
		goto loop;
		break;
	case mpEvasNoSort:
		count = generateEvasions( board, moveBuf );
		index = 0;
		phPtr++;
		goto loop;
		break;
	case mpEvas:
		count = generateEvasions( board, moveBuf );
		scoreEvasions();
		index = 0;
		phPtr++;
		goto loop;
		break;
	case mpCapBuffer:
		// here we'll ocassionally move to badcaps so it's different
		do
		{
loopcap:
			if ( index >= count )
			{
				phPtr++;
				goto loop;
			}
			res = moveBuf[ index++ ];

			if ( alreadyGenerated(res) )
				goto loopcap;

			// do fast(sign) see
			if ( board.see<1>(res) < 0 )
			{
				// bad capture detected
				assert( badCapCount < maxCaptures );
				badCaps[ badCapCount++ ] = res;
				goto loopcap;
			}
		} while( !board.pseudoIsLegal<0>( res, pin ) );
		break;
	case mpQCapBuffer:
		// here we'll ocassionally move to badcaps so it's different
		do
		{
qloopcap:
			if ( index >= count )
			{
				phPtr++;
				goto loop;
			}
			res = moveBuf[ index++ ];

			if ( alreadyGenerated(res) )
				goto qloopcap;

			// do fast(sign) see
			if ( board.see<1>( res ) < 0 )
				// bad capture detected
				goto qloopcap;
		} while( !board.pseudoIsLegal<0>( res, pin ) );
		break;
	case mpCastlingBuffer:
		if ( index >= count )
		{
			phPtr++;
			goto loop;
		}
		res = moveBuf[ index++ ];
		if ( alreadyGenerated(res) )
			goto loop;
		break;
	case mpCastlingBufferLegal:
		if ( index >= count )
		{
			phPtr++;
			goto loop;
		}
		res = moveBuf[ index++ ];
		break;
	case mpEvasBuffer:
		do
		{
			if ( index >= count )
			{
				phPtr++;
				goto loop;
			}
			res = moveBuf[ index++ ];
		} while ( alreadyGenerated(res) || !board.pseudoIsLegal<1>( res, pin ) );
		break;
	case mpEvasBufferLegal:
		do
		{
			if ( index >= count )
			{
				phPtr++;
				goto loop;
			}
			res = moveBuf[ index++ ];
		} while ( !board.pseudoIsLegal<1>( res, pin ) );
		break;
	case mpBufferLegal:
		do
		{
			if ( index >= count )
			{
				phPtr++;
				goto loop;
			}
			res = moveBuf[ index++ ];
		} while ( !board.pseudoIsLegal<0>( res, pin ) );
		break;
	case mpQuietBuffer:
		do
		{
			if ( index >= count )
			{
				phPtr++;
				goto loop;
			}
			res = moveBuf[ index++ ];
		} while ( alreadyGenerated(res) || !board.pseudoIsLegal<0>( res, pin ) );
		break;
	case mpQChecksBuffer:
		do
		{
checksloop:
			if ( index >= count )
			{
				phPtr++;
				goto loop;
			}
			res = moveBuf[ index++ ];
			if ( alreadyGenerated(res) )
				goto checksloop;
			if ( MovePack::isCastling(res) )
				break;
			if ( board.see<1>( res ) < 0 )
				continue;						// skip bad see checks
		} while ( !board.pseudoIsLegal<0>( res, pin ) );
		assert( board.isCheck( res, dcMask ) );
		break;
	case mpBadCapBuffer:
		do
		{
			if ( index >= badCapCount )
			{
				phPtr++;
				goto loop;
			}
			res = badCaps[ index++ ];
		} while( alreadyGenerated(res) || !board.pseudoIsLegal<0>( res, pin ) );
		break;
	default:			//case mpDone:
		res = mcNone;
		break;
	}
	return res & mmNoScore;				// remove score
}

// scoring and sorting

void MoveGen::scoreCaptures()
{
	// MVV/LVA-based
	Move *mp = moveBuf;
	const Move *me = mp + count;
	for ( ;mp < me; mp++ )
	{
		assert( !(*mp & mmScore) );
		if ( !MovePack::isCapture( *mp ) )
		{
			*mp += MovePack::promo( *mp ) << msScore;
			continue;
		}
		// apply MVV/LVA
		Piece mvv = Tables::mvvValue[ PiecePack::type( board.piece( MovePack::to( *mp ) ) ) ];
		Piece lva = Tables::lvaValue[ PiecePack::type( board.piece( MovePack::from( *mp ) ) ) ];
		assert( mvv );
		*mp += ( (((mvv << 3) - lva)<<3) + MovePack::promo( *mp ) ) << msScore;
	}
	isort( moveBuf, count );
}

void MoveGen::scoreEvasions()
{
	Move *mp = moveBuf;
	const Move *me = moveBuf + count;
	for ( ;mp < me; mp++ )
	{
		assert( !(*mp & mmScore) );
		if ( board.see<0>(*mp) >= 0 )
		{
			if ( MovePack::isCapture(*mp) )
			{
				// captures by MVV/LVA
				Piece mvv = Tables::mvvValue[ PiecePack::type( board.piece( MovePack::to( *mp ) ) ) ];
				Piece lva = Tables::lvaValue[ PiecePack::type( board.piece( MovePack::from( *mp ) ) ) ];
				*mp += ( ((mvv << 3) - lva) + 1 + 2*History::historyMax ) << msScore;
			}
			else
				// history
				*mp += (history->score( board, *mp ) + History::historyMax) << msScore;
		}
	}
	isort( moveBuf, count );
}

void MoveGen::scoreChecks()
{
//	isort( moves, count );
}

void MoveGen::scoreQuiet()
{
	// history-based
	Move *mp = moveBuf;
	const Move *me = mp + count;
	for ( ;mp < me; mp++ )
	{
		assert( !(*mp & mmScore) );
		// apply history
		*mp += (history->score( board, *mp ) + History::historyMax) << msScore;
	}
	isort( moveBuf, count );
}

static Board dummyBoard;

// can't copy
MoveGen::MoveGen( const MoveGen & ) : board(dummyBoard), killer(0), history(0)
{
	assert( 0 && "illegal copying of MoveGen class!" );
}

// can't assign
MoveGen &MoveGen::operator =( const MoveGen & )
{
	assert( 0 && "illegal assignment of MoveGen class!" );
	return *this;
}

}
