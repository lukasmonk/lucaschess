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

// TODO: get rid of copy-paste code for slow versions somehow

#include "move.h"
#include "tables.h"
#include "board.h"
#include "magic.h"
#include "killer.h"
#include "history.h"

namespace cheng4
{

enum MoveGenMode
{
	mmNormal,			// normal mode
	mmLegal,			// perft: legal-only mode
	mmQCaps,			// quiescence captures and promotions only
	mmQCapsChecks		// quiescence captures, promotions and checks only
};

// movegen phases
enum MoveGenPhase
{
	mpDone,				// all done
	mpHash,				// hashmove
	mpCap,				// captures and promotions (note that only good/winning captures belong here)
	mpCapBuffer,		// buffer phases: picks sorted moves from buffer
	mpQCap,				// quiescence captures and promotions (ignores underpromotions)
	mpQCapBuffer,
	mpQChecks,			// qchecks
	mpQChecksBuffer,
	mpKiller1,			// killer1
	mpKiller2,			// killer2
	mpCastling,			// castling moves
	mpCastlingBuffer,
	mpQuiet,			// quiet moves
	mpQuietBuffer,
	mpBadCap,			// bad captures
	mpBadCapBuffer,
	mpEvas,				// evasions
	mpEvasBuffer,
	// special non-sorting versions for full legal movegen (perft)
	mpCapNoSort,
	mpQuietNoSort,
	mpBufferLegal,
	mpCastlingBufferLegal,
	mpEvasNoSort,
	mpEvasBufferLegal
};

class MoveGen
{
	// can't assign/copy
	MoveGen &operator =( const MoveGen & );
	MoveGen( const MoveGen & );
protected:
	friend class Board;
	template< Color color, bool underpromo, bool capture > static inline Move *genPromo( Square sq,
		Square tsq, Move *moves )
	{
		*moves++ = MovePack::initPromoMove< capture >( sq, tsq, ptQueen );
		if ( underpromo )
		{
			*moves++ = MovePack::initPromoMove< capture >( sq, tsq, ptKnight );
			*moves++ = MovePack::initPromoMove< capture >( sq, tsq, ptBishop );
			*moves++ = MovePack::initPromoMove< capture >( sq, tsq, ptRook );
		}
		return moves;
	}

	template< Color color, bool underpromo > static Move *genPawnCaps( const Board &b, Move *moves )
	{
		// FIXME: better handling of ep square?
		Bitboard opp = b.pieces( flip(color) );
		Bitboard tmp = b.pieces( color, ptPawn );
		Square ep = b.epSquare();
		if ( ep )
			 opp |= BitOp::oneShl( ep );			// a trick to add ep to target squares
		while ( tmp )
		{
			Square sq = BitOp::popBit( tmp );
			Bitboard targ = opp & Tables::pawnAttm[ color ][ sq ];

			while ( targ )
			{
				Square tsq = BitOp::popBit( targ );
				if ( SquarePack::relRank<color>( tsq ) == RANK8 )
					// promo-captures
					moves = genPromo< color, underpromo, 1 >( sq, tsq, moves );
				else
					*moves++ = tsq == ep ? MovePack::initEpCapture( sq, tsq ) : MovePack::initCapture( sq, tsq );
			}

			// now handle standard promotions
			if ( SquarePack::relRank<color>( sq ) == RANK7 )
			{
				Square tsq = SquarePack::advanceRank<color, 1>( sq );
				if ( b.isVacated( tsq ) )
					moves = genPromo< color, underpromo, 0 >( sq, tsq, moves );
			}
		}
		return moves;
	}

	// because of evasions
	template< Color color, bool underpromo > static Move *genPawnCapsSlow( Bitboard oopp, const Board &b, Move *moves )
	{
		// FIXME: better handling of ep square?
		Bitboard opp = oopp;
		opp &= b.pieces( flip(color) );
		Bitboard tmp = b.pieces( color, ptPawn );
		Square ep = b.epSquare();
		if ( ep )
			 opp |= BitOp::oneShl( ep );			// a trick to add ep to target squares
		while ( tmp )
		{
			Square sq = BitOp::popBit( tmp );
			Bitboard targ = opp & Tables::pawnAttm[ color ][ sq ];

			while ( targ )
			{
				Square tsq = BitOp::popBit( targ );
				if ( SquarePack::relRank<color>( tsq ) == RANK8 )
					// promo-captures
					moves = genPromo< color, underpromo, 1 >( sq, tsq, moves );
				else
					*moves++ = tsq == ep ? MovePack::initEpCapture( sq, tsq ) : MovePack::initCapture( sq, tsq );
			}

			// now handle standard promotions
			if ( SquarePack::relRank<color>( sq ) == RANK7 )
			{
				Square tsq = SquarePack::advanceRank<color, 1>( sq );
				if ( b.isVacated( tsq ) && (oopp & BitOp::oneShl( tsq )) )
					moves = genPromo< color, underpromo, 0 >( sq, tsq, moves );
			}
		}
		return moves;
	}

	template< Color color > static Move *genPawnPushes( const Board &b, Move *moves )
	{
		Bitboard tmp = b.pieces( color, ptPawn );
		while ( tmp )
		{
			Square sq = BitOp::popBit( tmp );
			Square front = SquarePack::advanceRank< color, 1 >( sq );

			if ( b.isVacated( front ) && SquarePack::relRank< color >( sq ) != RANK7 )
			{
				// push forward (one)
				*moves++ = MovePack::init( sq, front );
				if ( SquarePack::relRank< color >( sq ) == RANK2 )
				{
					Square front2 = SquarePack::advanceRank< color, 1 >( front );
					if ( b.isVacated( front2 ) )
						// push forward (two)
						*moves++ = MovePack::init( sq, front2 );
				}
			}
		}
		return moves;
	}

	// because of evasions
	template< Color color > static Move *genPawnPushesSlow( Bitboard targm, const Board &b, Move *moves )
	{
		Bitboard tmp = b.pieces( color, ptPawn );
		while ( tmp )
		{
			Square sq = BitOp::popBit( tmp );
			Square front = SquarePack::advanceRank< color, 1 >( sq );

			if ( b.isVacated( front ) && SquarePack::relRank< color >( sq ) != RANK7 )
			{
				// push forward (one)
				if ( targm & BitOp::oneShl( front ) )
					*moves++ = MovePack::init( sq, front );
				if ( SquarePack::relRank< color >( sq ) == RANK2 )
				{
					Square front2 = SquarePack::advanceRank< color, 1 >( front );
					if ( b.isVacated( front2 ) && ( targm & BitOp::oneShl( front2 ) ) )
						// push forward (two)
						*moves++ = MovePack::init( sq, front2 );
				}
			}
		}
		return moves;
	}

	template< Color color > static Move *genPawnChecks( Bitboard dc, Square ekp,
		Bitboard targm, const Board &b, Move *moves )
	{
		Bitboard tmp = b.pieces( color, ptPawn );
		while ( tmp )
		{
			Square sq = BitOp::popBit( tmp );
			Square front = SquarePack::advanceRank< color, 1 >( sq );

			if ( b.isVacated( front ) && SquarePack::relRank< color >( sq ) != RANK7 )
			{
				// push forward (one)
				Bitboard tom = BitOp::oneShl( front );
				Bitboard tm = targm;
				if ( BitOp::oneShl(sq) & dc )
					// discovered checker
					if ( !(tom & Tables::ray[ ekp ][ sq ] ) )
						tm = ~U64C(0);

				if ( tm & BitOp::oneShl( front ) )
					*moves++ = MovePack::init( sq, front );
				if ( SquarePack::relRank< color >( sq ) == RANK2 )
				{
					Square front2 = SquarePack::advanceRank< color, 1 >( front );

					tom = BitOp::oneShl( front2 );
					tm = targm;
					if ( BitOp::oneShl(sq) & dc )
						// discovered checker
						if ( !(tom & Tables::ray[ ekp ][ sq ] ) )
							tm = ~U64C(0);

					if ( b.isVacated( front2 ) && ( tm & tom ) )
						// push forward (two)
						*moves++ = MovePack::init( sq, front2 );
				}
			}
		}
		return moves;
	}

	template< Color color, bool capture > static inline Move *genKnights( Bitboard tmask,
		const Board &b, Move *moves )
	{
		Bitboard tmp = b.pieces( color, ptKnight );
		while ( tmp )
		{
			Square sq = BitOp::popBit( tmp );
			Bitboard targ = Tables::knightAttm[ sq ] & tmask;

			while ( targ )
				*moves++ = MovePack::initMove< capture >( sq, BitOp::popBit( targ ) );
		}
		return moves;
	}

	template< Color color, bool capture > static inline Move *genKnightChecks(
		Bitboard dc, Bitboard dctmask, Bitboard tmask, const Board &b, Move *moves )
	{
		Bitboard tmp = b.pieces( color, ptKnight );
		while ( tmp )
		{
			Square sq = BitOp::popBit( tmp );
			Bitboard targ = Tables::knightAttm[ sq ] & ( (BitOp::oneShl(sq) & dc) ? dctmask : tmask );

			while ( targ )
				*moves++ = MovePack::initMove< capture >( sq, BitOp::popBit( targ ) );
		}
		return moves;
	}

	// slow version for evasions
	template< Color color > static inline Move *genKnightsSlow( Bitboard opp, Bitboard tmask,
		const Board &b, Move *moves )
	{
		Bitboard tmp = b.pieces( color, ptKnight );
		while ( tmp )
		{
			Square sq = BitOp::popBit( tmp );
			Bitboard targ = Tables::knightAttm[ sq ] & tmask;

			while ( targ )
			{
				Square tsq = BitOp::popBit( targ );
				*moves++ = ( BitOp::oneShl( tsq ) & opp ) ?
					MovePack::initCapture( sq, tsq )
					: MovePack::init( sq, tsq );
			}
		}
		return moves;
	}

	template< Color color, Piece piece, bool capture > static inline Move *genSliders( Bitboard occ,
		Bitboard tmask, const Board &b, Move *moves )
	{
		Bitboard tmp = b.pieces( color, piece );
		while ( tmp )
		{
			Square sq = BitOp::popBit( tmp );
			Bitboard targ = Magic::sliderAttm<piece>( sq, occ ) & tmask;
			while ( targ )
				*moves++ = MovePack::initMove< capture >( sq, BitOp::popBit( targ ) );
		}
		return moves;
	}

	template< Color color, Piece piece, bool capture > static inline Move *genSliderChecks(
		Bitboard dc, Bitboard dctmask, Bitboard occ, Bitboard tmask, const Board &b, Move *moves )
	{
		Bitboard tmp = b.pieces( color, piece );
		while ( tmp )
		{
			Square sq = BitOp::popBit( tmp );
			Bitboard targ = Magic::sliderAttm<piece>( sq, occ ) &
				( (BitOp::oneShl(sq) & dc) ? dctmask : tmask );
			while ( targ )
				*moves++ = MovePack::initMove< capture >( sq, BitOp::popBit( targ ) );
		}
		return moves;
	}

	// slow version for evasions
	template< Color color, Piece piece > static inline Move *genSlidersSlow( Bitboard occ,
		Bitboard opp, Bitboard tmask, const Board &b, Move *moves )
	{
		Bitboard tmp = b.pieces( color, piece );
		while ( tmp )
		{
			Square sq = BitOp::popBit( tmp );
			Bitboard targ = Magic::sliderAttm<piece>( sq, occ ) & tmask;
			while ( targ )
			{
				Square tsq = BitOp::popBit( targ );
				*moves++ = ( BitOp::oneShl( tsq ) & opp ) ?
					MovePack::initCapture( sq, tsq )
					: MovePack::init( sq, tsq );
			}
		}
		return moves;
	}

	template< Color color, bool capture > static inline Move *genKingMoves( Bitboard tmask, const Board &b,
		Move *moves )
	{
		Square sq = b.king( color );
		Bitboard tmp = Tables::kingAttm[ b.king( color ) ] & tmask;
		while ( tmp )
			*moves++ = MovePack::initMove<capture>( sq, BitOp::popBit( tmp ) );
		return moves;
	}

	// slow version because of evasions
	template< Color color > static inline Move *genKingMovesSlow( Bitboard opp, const Board &b,
		Move *moves )
	{
		Square sq = b.king( color );
		Bitboard tmp = Tables::kingAttm[ b.king( color ) ];
		tmp &= ~b.pieces( color );			// avoid self-captures
		while ( tmp )
		{
			Square tsq = BitOp::popBit( tmp );
			*moves++ = ( BitOp::oneShl( tsq ) & opp ) ?
				MovePack::initCapture( sq, tsq )
				: MovePack::init( sq, tsq );
		}
		return moves;
	}

	// generate pseudolegal captures and promotions
	// returns number of moves generated
	template< Color color, bool underpromo > static MoveCount genCaps( const Board &b, Move *moves )
	{
		Move *om = moves;

		Bitboard occ = b.occupied();
		Bitboard opp = b.pieces( flip(color) );

		// generate pawn captures
		moves = genPawnCaps< color, underpromo >( b, moves );

		// knight captures
		moves = genKnights< color, 1 >( opp, b, moves );

		// bishop captures
		moves = genSliders< color, ptBishop, 1 >( occ, opp, b, moves );

		// rook captures
		moves = genSliders< color, ptRook, 1 >( occ, opp, b, moves );

		// queen captures
		moves = genSliders< color, ptQueen, 1 >( occ, opp, b, moves );

		// king captures
		moves = genKingMoves< color, 1 >( opp, b, moves );

		assert( moves - om < maxMoves );
		return (MoveCount)(moves - om);
	}

	// generate castling move
	template< Color color > static Move *genCastling( File rookfile, const Board &b, Move *moves )
	{
		Square kp = b.king( color );
		File kf = SquarePack::file(kp);
		Square ktarg = SquarePack::setFile( kp, (File)( (kf < rookfile) ? GFILE : CFILE ) );
		Square rtarg = SquarePack::setFile( kp, (File)( (kf < rookfile) ? FFILE : DFILE ) );
		Square rp = SquarePack::setFile( kp, rookfile );

		assert( PiecePack::type( b.piece( rp ) ) == ptRook );

		Color opc = flip( color );

		Bitboard occ = b.occupied();

		occ &= BitOp::noneShl( kp );
		occ &= BitOp::noneShl( rp );

		// king/rook path
		Bitboard kpath = Tables::between[ kp ][ ktarg ];
		Bitboard rpath = Tables::between[ rp ][ rtarg ];

		if ( (kpath|rpath) & occ )
			return moves;				// path blocked

		*moves = MovePack::initCastling( kp, ktarg );

		// make sure all squares between kp and ktarg aren't attacked
		int kpinc = (kp < ktarg) ? +1 : -1;
		for( ;kp != ktarg ;kp = (Square)((int)kp + kpinc) )
		{
			if ( b.doesAttack<1>( opc, kp, occ ) )
				return moves;
		}
		return moves + !b.doesAttack<1>( opc, kp, occ );
	}

	template< Color color > static MoveCount genQuiet( const Board &b, Move *moves )
	{
		Move *om = moves;

		Bitboard occ = b.occupied();
		Bitboard nocc = ~occ;

		// now generate pawn pushes
		moves = genPawnPushes< color >( b, moves );

		// knight moves
		moves = genKnights< color, 0 >( nocc, b, moves );

		// bishop moves
		moves = genSliders< color, ptBishop, 0 >( occ, nocc, b, moves );

		// rook moves
		moves = genSliders< color, ptRook, 0 >( occ, nocc, b, moves );

		// queen moves
		moves = genSliders< color, ptQueen, 0 >( occ, nocc, b, moves );

		// king moves
		moves = genKingMoves< color, 0 >( nocc, b, moves );

		assert( moves - om < maxMoves );
		return (MoveCount)(moves - om);
	}

	template< Color color > static MoveCount genEvas( const Board &b, Move *moves )
	{
		/*
			let's think a bit about it:
			if not doublecheck:
				possible evasions: capture checker
				if checker is a slider, move piece to mask between king and checker
			finally king moves
		*/

		Move *om = moves;
		Bitboard opp = b.pieces( flip(color) );
		Bitboard evasMask = b.evasions();
		if ( evasMask )
		{
			// generate non-king moves

			Bitboard occ = b.occupied();

			// pawns
			// FIXME: better!
			moves = genPawnCapsSlow< color, 1 >( evasMask, b, moves );
			moves = genPawnPushesSlow< color >( evasMask, b, moves );

			// knights
			moves = genKnightsSlow< color >( opp, evasMask, b, moves );

			// bishops
			moves = genSlidersSlow< color, ptBishop >( occ, opp, evasMask, b, moves );

			// rooks
			moves = genSlidersSlow< color, ptRook >( occ, opp, evasMask, b, moves );

			// queens
			moves = genSlidersSlow< color, ptQueen >( occ, opp, evasMask, b, moves );
		}
		// finally generate king moves (castling is not an option here)
		moves = genKingMovesSlow< color >( opp, b, moves );

		assert( moves - om < maxMoves );
		return (MoveCount)(moves - om);
	}

	static inline MoveCount generateCaptures( const Board &b, Move *moves, bool underpromo = 1 )
	{
		if ( underpromo )
			return b.turn() == ctWhite ? genCaps< ctWhite, 1 >( b, moves ) : genCaps< ctBlack, 1 >( b, moves );
		else
			return b.turn() == ctWhite ? genCaps< ctWhite, 0 >( b, moves ) : genCaps< ctBlack, 0 >( b, moves );
	}

	static inline MoveCount generateQuiet( const Board &b, Move *moves )
	{
		// promotions already generated
		// now generate castling moves and quiet moves
		return b.turn() == ctWhite ? genQuiet< ctWhite >( b, moves ) : genQuiet< ctBlack >( b, moves );
	}

	template< Color color > static inline MoveCount generateCastling( const Board &b, Move *moves )
	{
		const Move *om = moves;
		CastRights cr = b.castRights( color );
		if ( CastPack::allowedShort( cr ) )
			// do short castling (kingside)
			moves = genCastling< color >( CastPack::shortFile(cr), b, moves );

		if ( CastPack::allowedLong( cr ) )
			// do long castling (queenside)
			moves = genCastling< color >( CastPack::longFile(cr), b, moves );
		return (MoveCount)(moves - om);
	}

	// qsearch generate_checks helper
	template< Color color > inline MoveCount generateChecks( Move *moves )
	{
		const Board &b = board;
		assert( !b.inCheck() );
		// we don't generate any captures/underpromotions here
		// three possibilities:
		// - direct checks
		// - discovered checks
		// - castling into check
		const Move *om = moves;
		Square ekp = b.king( flip(color) );
		Bitboard occ = b.occupied();
		Bitboard nocc = ~occ;
		Bitboard diagMask = Magic::bishopAttm( ekp, occ );
		Bitboard orthoMask = Magic::rookAttm( ekp, occ );

		// direct pawn checks
		// note that this shouldn't generate promotions anyway!
		moves = genPawnChecks< color >( dcMask, ekp, Tables::pawnAttm[ flip(color) ][ ekp ] & nocc, b, moves );

		// knight checks
		moves = genKnightChecks< color, 0 >( dcMask, nocc, Tables::knightAttm[ ekp ] & nocc, b, moves );

		// bishop checks
		moves = genSliderChecks< color, ptBishop, 0 >( dcMask, nocc, occ, diagMask & nocc, b, moves );

		// rook checks
		moves = genSliderChecks< color, ptRook, 0 >( dcMask, nocc, occ, orthoMask & nocc, b, moves );

		// direct queen checks (can't give discovered checks)
		moves = genSliders< color, ptQueen, 0 >( occ, (orthoMask | diagMask) & nocc, b, moves );

		// king can only give a discovered check
		Square kp = b.king( color );
		if ( dcMask & BitOp::oneShl( kp ) )
			moves = genKingMoves< color, 0 >( Tables::kingAttm[ kp ] & nocc & ~Tables::ray[ ekp ][ kp ], b, moves );

		// last thing to try: castling into check!
		CastRights cr = b.castRights( color );
		if ( cr )
		{
			// FIXME: better?!
			Move cm;
			if ( CastPack::allowedShort(cr) )
			{
				Move *tmp = color == ctWhite ?
					MoveGen::genCastling< ctWhite >( CastPack::shortFile(cr), b, &cm ) :
					MoveGen::genCastling< ctBlack >( CastPack::shortFile(cr), b, &cm );
				if ( tmp > &cm && b.isCheck( cm, dcMask ) )
					*moves++ = cm;
			}
			if ( CastPack::allowedLong(cr) )
			{
				Move *tmp = color == ctWhite ?
					MoveGen::genCastling< ctWhite >( CastPack::longFile(cr), b, &cm ) :
					MoveGen::genCastling< ctBlack >( CastPack::longFile(cr), b, &cm );
				if ( tmp > &cm && b.isCheck( cm, dcMask ) )
					*moves++ = cm;
			}
		}
		return (MoveCount)(moves - om);
	}

	// generate (pseudolegal) evasion moves (=moves out of check)
	static inline MoveCount generateEvasions( const Board &b, Move *moves )
	{
		return b.turn() == ctWhite ? genEvas< ctWhite >( b, moves ) : genEvas< ctBlack >( b, moves );
	}

	uint mode;						// movegen mode
	const Board &board;				// board ref
	const Killer * const killer;	// killer ref (includes hashmove)
	const History * const history;	// history ref
	Bitboard dcMask;				// discovered checkers
	Bitboard pin;					// pins
	const uint *phPtr;				// phase ptr
	MoveCount index;				// move buffer index
	MoveCount count;				// move buffer count
	MoveCount badCapCount;			// bad capture buffer move count
	Move moveBuf[ maxMoves ];		// move buffer
	Move badCaps[ maxCaptures ];	// bad captures buffer

	Move genMoves[ 4 ];				// already generated moves (hash, killers) (FIXME: use constant, rename)
	MoveCount genMoveCount;			// number of already generated moves

	// score and sort specific moves
	void scoreCaptures();
	void scoreEvasions();
	void scoreChecks();
	void scoreQuiet();

	bool alreadyGenerated( Move m );

public:
	MoveGen( const Board &b, const Killer &killer, const History &history, uint mode = mmNormal );
	// legal only version
	MoveGen( const Board &b );

	// generate next move, if mcNone is returned => no more moves available
	Move next();

	// returns discovered checkers mask (always)
	inline Bitboard discovered() const
	{
		return dcMask;
	}

	// returns pin mask (always)
	inline Bitboard pins() const
	{
		return pin;
	}

	// current movegen phase
	inline uint phase() const
	{
		return *phPtr;
	}
};

}
