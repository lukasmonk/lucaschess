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

#include "move.h"
#include "magic.h"
#include "zobrist.h"
#include "psq.h"
#include <string>
#include <stdlib.h>
#include <iostream>

// TODO: move large methods to cpp

namespace cheng4
{

class MoveGen;

typedef uint UndoMask;

enum UndoFlags
{
	ufIrreversible	=	1,			// restore pawnhash and fifty rule counter
	ufNPMat			=	2,			// restore non-pawn materials
	ufKingState		=	4,			// restore king state (=kingpos and check flag)
	ufCastling		=	8			// restore castling state
};

class Board;

struct UndoInfo
{
	UndoMask flags;				// undo flags
	// ep square will be restored
	Signature bhash;			// board hash (always)
	Signature phash;			// pawn hash (optional)
	Bitboard bb[7];				// bitboards to restore
	DMat dmat[2];				// original delta-material (always)
	NPMat npmat[2];				// original non-pawn material
	u8 bbi[7];					// indices to restore
	u8 bbCount;					// number of bbs to restore
	Piece pieces[4];			// board pieces to restore
	Square squares[4];			// board squares to restore
	Square ep;					// original ep square (always)
	u8 pieceCount;				// number of board pieces to restore
	FiftyCount fifty;			// original fifty move counter type (if needed)
	CastRights castRights[2];	// original castling rights (if needed)
	u8 kingPos;					// original king position (always)
	bool check;					// original in check flag (always)

	inline void clear()
	{
		flags = 0;
		bbCount = pieceCount = 0;
	}

	inline void saveBB( u8 index, Bitboard bboard )
	{
		assert( index < bbiMax );
		assert( bbCount < 7 );
		bb[ bbCount ] = bboard;
		bbi[ bbCount++ ] = index;
	}

	inline void savePiece( Square square, Piece piece )
	{
		assert( PiecePack::type( piece ) <= ptKing );
		assert( pieceCount < 4 );
		pieces[ pieceCount ] = piece;
		squares[ pieceCount++ ] = square;
	}
};

class Board
{
protected:
	friend class MoveGen;

	Signature bhash;			// current hash signature
	Signature bpawnHash;		// current pawn hash signature
	Bitboard bb[ bbiMax ];		// bitboard for pieces (indexed using BBI)
	Piece bpieces[ 64 ];		// pieces: color in MSBit
	DMat bdmat[2];				// delta-material [gamephase] - in centipawns
	NPMat bnpmat[2];			// non-pawn material (white, black)
	Square bkingPos[2];			// kings not stored using bitboards
	Color bturn;				// ctWhite(0) or ctBlack(1)
	Square bep;					// enpassant square (new: where opp captures; was a flaw in cheng3)
	FiftyCount bfifty;			// fifty move counter
	CastRights bcastRights[2];	// castling rights for white/black, indexed by colorType
	bool bcheck;				// in check flag

	// extra stuff (not used during search)
	bool frc;					// is fischer random?
	bool arenaMode;				// FRC Arena mode
	uint curMove;				// current move number

	// castling move is special
	void doCastlingMove( Move move, UndoInfo &ui, bool ischeck );

	inline void initUndo( UndoInfo &ui ) const
	{
		ui.clear();						// clear undo mask
		ui.bhash = bhash;				// hash signature always preserved
		// delta material always preserved
		ui.dmat[ phOpening ] = bdmat[ phOpening ];
		ui.dmat[ phEndgame ] = bdmat[ phEndgame ];
		ui.ep = bep;					// ep square always preserved
	}

	inline void saveKingState( UndoInfo &ui ) const
	{
		assert( !(ui.flags & ufKingState) );
		ui.flags |= ufKingState;
		ui.check = bcheck;
		ui.kingPos = king( turn() );
	}

	template< bool checkFlag > inline void saveCastling( UndoInfo &ui ) const
	{
		if ( checkFlag && ( ui.flags & ufCastling ) )
			return;						// already saved
		assert( !(ui.flags & ufCastling) );
		ui.flags |= ufCastling;
		ui.castRights[ ctWhite ] = castRights( ctWhite );
		ui.castRights[ ctBlack ] = castRights( ctBlack );
	}

	template< bool checkFlag > inline void saveNPMat( UndoInfo &ui ) const
	{
		if ( checkFlag && ( ui.flags & ufNPMat ) )
			return;						// already saved
		assert( !(ui.flags & ufNPMat) );
		ui.flags |= ufNPMat;
		ui.npmat[ ctWhite ] = bnpmat[ ctWhite ];
		ui.npmat[ ctBlack ] = bnpmat[ ctBlack ];
	}

	// also clear fifty rule counter
	inline void saveIrreversible( UndoInfo &ui )
	{
		assert( !(ui.flags & ufIrreversible) );
		ui.phash = bpawnHash;
		ui.fifty = bfifty;
		ui.flags |= ufIrreversible;
		bfifty = 0;
	}

	template< Color c > Draw isDrawByMaterial( MaterialKey mk ) const;

	void calcEvasMask();

public:

	inline uint move() const
	{
		return curMove;
	}

	// increment move counter by 1
	void incMove();

	// assign castling rights automatically
	void autoCastlingRights();

	inline void setFischerRandom( bool frc_ )
	{
		frc = frc_;
	}

	inline bool fischerRandom() const
	{
		return frc;
	}

	// movegen helper only!
	inline Bitboard evasions() const
	{
		return bb[ bbiEvMask ];
	}

	// simply returns hash signature
	inline Signature sig() const
	{
		return bhash;
	}

	// simply returns pawn hash signature
	inline Signature pawnSig() const
	{
		return bpawnHash;
	}

	// note: doMove methods expect a legal move!

	template< u8 flags > void undoTemplate( const UndoInfo &ui )
	{
		if ( flags & ufIrreversible )
		{
			bpawnHash = ui.phash;
			bfifty = ui.fifty;
		}
		bfifty--;
		if ( flags & ufNPMat )
		{
			bnpmat[ ctWhite ] = ui.npmat[ ctWhite ];
			bnpmat[ ctBlack ] = ui.npmat[ ctBlack ];
		}
		if ( flags & ufKingState )
		{
			bkingPos[ turn() ] = ui.kingPos;
			bcheck = ui.check;
		}
		if ( flags & ufCastling )
		{
			bcastRights[ ctWhite ] = ui.castRights[ ctWhite ];
			bcastRights[ ctBlack ] = ui.castRights[ ctBlack ];
		}
	}

	template< Color color, bool capture, Piece ptype >
		void doMoveTemplate( Move move, Square from, Square to, UndoInfo &ui, bool isCheck )
	{
		assert( !MovePack::isCastling( move ) );
		assert( color == bturn );
		assert( from != to );

		initUndo( ui );
		bfifty++;

		if ( inCheck() )
			ui.saveBB( bbiEvMask, bb[ bbiEvMask ] );

		// update hash
		// move piece
		Bitboard tomask;
		Bitboard frommask;
		Bitboard ftmask;

		u8 bbi;

		bbi = bbiWOcc + color;
		ui.saveBB( bbi, bb[ bbi ] );
		bb[ bbi ] ^= ( ftmask = (frommask = BitOp::oneShl( from )) | (tomask = BitOp::oneShl(to)) );

		Piece toPiece;

		// prepare to update pieces on board
		ui.savePiece( from, toPiece = piece(from) );
		ui.savePiece( to, piece(to) );

		assert( PiecePack::type( toPiece ) );

		// hash update
		bhash ^= Zobrist::piece[ color ][ ptype ][ from ];

		if ( capture || ptype == ptPawn )
			// irreversible move
			saveIrreversible( ui );

		if ( capture )
			// save material key
			ui.saveBB( bbiMat, bb[ bbiMat ] );

		if ( ptype == ptPawn )
		{
			bpawnHash ^= Zobrist::piece[ color ][ ptPawn ][ from ];
			Piece promo = MovePack::promo( move );
			if ( promo )
			{
				if ( !capture )
					// save material key
					ui.saveBB( bbiMat, bb[ bbiMat ] );

				assert( promo <= ptQueen );
				toPiece &= pmColor;
				toPiece |= promo;
				bhash ^= Zobrist::piece[ color ][ promo ][ to ];

				bbi = BBI( color, ptPawn );
				ui.saveBB( bbi, bb[ bbi ] );
				bb[ bbi ] ^= frommask;

				bbi = BBI( color, promo );
				ui.saveBB( bbi, bb[ bbi ] );
				bb[ bbi ] ^= tomask;

				// update material key
				bb[ bbiMat ] -= BitOp::oneShl( MATSHIFT( color, ptPawn) );
				bb[ bbiMat ] += BitOp::oneShl( MATSHIFT( color, promo) );

				// update non-pawn material
				saveNPMat<0>( ui );
				bnpmat[ color ] += Tables::npValue[ promo ];
			}
			else
			{
				bbi = BBI( color, ptype );
				ui.saveBB( bbi, bb[ bbi ] );
				bb[ bbi ] ^= ftmask;

				Signature tosig = Zobrist::piece[ color ][ ptPawn ][ to ];
				bhash ^= tosig;
				bpawnHash ^= tosig;
			}
		} else
		{
			if ( ptype != ptKing )
			{
				bbi = BBI( color, ptype );
				ui.saveBB( bbi, bb[ bbi ] );
				bb[ bbi ] ^= ftmask;
			}

			bhash ^= Zobrist::piece[ color ][ ptype ][ to ];
		}

		CastRights cr;

		if ( ptype == ptKing || ptype ==ptRook )
			cr = castRights( color );

		if ( ptype == ptKing )
		{
			saveKingState( ui );
			saveCastling<0>( ui );
			if ( cr )
			{
				// losing all castling rights
				bhash ^= Zobrist::cast[ color ][ cr ];
				bcastRights[ color ] = 0;
			}
		} else if ( ptype == ptRook && cr )
		{
			// lose castling rights
			if ( SquarePack::relRank< color >( from ) == RANK1 )
			{
				File rf = SquarePack::file( from );
				File kf = SquarePack::file( king(color) );
				if ( (rf > kf && rf == CastPack::shortFile(cr))
					|| (rf < kf && rf == CastPack::longFile(cr)) )
				{
					saveCastling<0>(ui);
					bhash ^= Zobrist::cast[ color ][ cr ];
					bcastRights[ color ] = CastPack::loseFile( SquarePack::file( from ), cr );
					bhash ^= Zobrist::cast[ color ][ castRights( color ) ];
				}
			}
		}

		if ( capture )
		{
			// a capture needs even more careful handling
			Square cto = (ptype == ptPawn && MovePack::isEpCapture(move) ) ?
				SquarePack::epTarget( epSquare(), from ) : to;

			Piece cap = piece( cto );

			Piece captype = PiecePack::type( cap );
			assert( captype >= ptPawn && captype < ptKing );

			// update material key
			bb[ bbiMat ] -= BitOp::oneShl( MATSHIFT( flip(color), captype ) );

			// update delta material
			bdmat[ phOpening ] -= PSq::tables[ phOpening ][ flip(color) ][ captype ][ cto ];
			bdmat[ phEndgame ] -= PSq::tables[ phEndgame ][ flip(color) ][ captype ][ cto ];

			if ( captype == ptRook )
			{
				// lose appropriate castling rights (note: opponent loses)
				Color opc = flip(color);
				CastRights tcr = castRights( opc );
				if ( tcr && SquarePack::relRank< (color^1) >( to ) == RANK1 )
				{
					File f = SquarePack::file( to );
					if ( CastPack::longFile( tcr ) == f )
					{
						saveCastling<1>(ui);
						bhash ^= Zobrist::cast[ opc ][ tcr ];
						bcastRights[opc] = CastPack::loseLong( tcr );
						bhash ^= Zobrist::cast[ opc ][ castRights( opc ) ];
					}
					else if ( CastPack::shortFile( tcr ) == f )
					{
						saveCastling<1>(ui);
						bhash ^= Zobrist::cast[ opc ][ tcr ];
						bcastRights[opc] = CastPack::loseShort( tcr );
						bhash ^= Zobrist::cast[ opc ][ castRights( opc ) ];
					}
				}
			}
			Signature tosig = Zobrist::piece[ flip(color) ][ captype ][ cto ];
			bhash ^= tosig;
			if ( captype == ptPawn )
				bpawnHash ^= tosig;

			Bitboard capmask = (ptype == ptPawn ? BitOp::oneShl( cto ) : tomask);

			bbi = bbiBOcc - color;
			ui.saveBB( bbi, bb[bbi] );
			bb[ bbi ] ^= capmask;

			bbi = BBI( flip(color), captype );
			ui.saveBB( bbi, bb[bbi] );
			bb[ bbi ] ^= capmask;

			// update total non-pawn material on board
			saveNPMat< ptype == ptPawn>( ui );
			bnpmat[ flip(color) ] -= Tables::npValue[ captype ];

			if ( ptype == ptPawn && cto != to )
			{
				assert( MovePack::isEpCapture( move ) );
				// ep capture
				ui.savePiece( cto, cap );
				bpieces[ cto ] = ptNone;
			}
		}

		// finish updating board pieces
		bpieces[to] = toPiece;
		bpieces[from] = ptNone;

		// update delta material

		toPiece &= pmType;

		bdmat[ phOpening ] -= PSq::tables[ phOpening ][ color ][ ptype ][ from ];
		bdmat[ phEndgame ] -= PSq::tables[ phEndgame ][ color ][ ptype ][ from ];

		bdmat[ phOpening ] += PSq::tables[ phOpening ][ color ][ toPiece ][ to ];
		bdmat[ phEndgame ] += PSq::tables[ phEndgame ][ color ][ toPiece ][ to ];

		if ( ptype == ptKing )
			bkingPos[ color ] = to;

		// update check state
		if ( bcheck != isCheck )
		{
			if ( ptype != ptKing )
				saveKingState( ui );
			bcheck = isCheck;
		}

		// update ep square
		if ( bep )
		{
			bhash ^= Zobrist::epFile[ SquarePack::file( bep ) ];
			bep = 0;
		}
		if ( ptype == ptPawn )
		{
			Rank rf = SquarePack::rank( from );
			Rank rt = SquarePack::rank( to );
			if ( !((rf - rt) & 1) )
			{
				// pawn push by two => update ep square
				bep = SquarePack::advanceRank< color, -1 >( to );
				// only set ep square if pawn can be ep-captured
				// note: this slows my pawn-push movegen a tiny bit BUT it avoid dup hashes in useless cases,
				// especially useful for books
				if ( Tables::pawnAttm[color][bep] & pieces( flip(color), ptPawn ) )
					bhash ^= Zobrist::epFile[ SquarePack::file( bep ) ];
				else bep = 0;
			}
		}

		// finally change stm
		bhash ^= Zobrist::turn;
		bturn = flip( bturn );

		if ( isCheck )
			calcEvasMask();

		assert( !( (pieces( ctWhite, ptPawn)|pieces( ctBlack, ptPawn)) & pawnPromoSquares ) );
		assert( bcheck == doesAttack<1>( flip(bturn), king( bturn ) ) );
		assert( bhash == recomputeHash() );
		assert( bpawnHash == recomputePawnHash() );
		assert( isValid() );
	}

	// do move
	void doMove( Move move, UndoInfo &ui, bool isCheck );

	// undo move
	void undoMove( const UndoInfo &ui );

	// do null move
	void doNullMove( UndoInfo &ui );

	// undo null move
	void undoNullMove( const UndoInfo &ui );

	// reset to initial position
	void reset();

	// clear bits
	void clear();

	// returns 1 if stm is in check
	inline bool inCheck() const
	{
		return bcheck;
	}

	inline MaterialKey materialKey() const
	{
		return (MaterialKey)bb[ bbiMat ];
	}

	// returns delta material for given game phase (fine)
	inline Score deltaMat( Phase ph ) const
	{
		assert( ph <= phEndgame );
		return bdmat[ ph ];
	}

	// return total non-pawn material
	inline NPMat nonPawnMat() const
	{
		return bnpmat[ ctWhite ] + bnpmat[ ctBlack ];
	}

	// non-pawn material for side
	inline NPMat nonPawnMat( Color c ) const
	{
		assert( c <= ctBlack );
		return bnpmat[ c ];
	}

	// return en-passant square
	inline Square epSquare() const
	{
		return bep;
	}

	// return fifty move rule counter
	inline uint fifty() const
	{
		return bfifty;
	}

	// return side to move
	inline Color turn() const
	{
		return bturn;
	}

	// castling rights
	inline CastRights castRights( Color color ) const
	{
		assert( color <= ctBlack );
		return bcastRights[ color ];
	}

	// returns true if stm can castle
	inline bool canCastle() const
	{
		return castRights( turn() ) != 0;
	}

	// return occupied mask (all)
	inline Bitboard occupied() const
	{
		return bb[ bbiWOcc ] | bb[ bbiBOcc ];
	}

	// return occupied mask for color
	inline Bitboard occupied( Color c ) const
	{
		assert( c <= ctBlack );
		return bb[ bbiWOcc + c ];
	}

	inline Bitboard pieces( Color color ) const
	{
		assert( color <= ctBlack );
		return bb[ bbiWOcc + color ];
	}

	inline Bitboard pieces( Color color, Piece pt ) const
	{
		assert( pt < ptKing && color <= ctBlack );
		return bb[ BBI( color, pt ) ];
	}

	inline Piece piece( Square sq ) const
	{
		assert( sq < 64 );
		return bpieces[ sq ];
	}

	// clear pieces (used in xboard edit mode)
	void clearPieces();

	// set side to move (used in xboard edit mode)
	void setTurn( Color c );

	// update bitboards from board state (used in xboard edit mode)
	// impl. note: calls update() when done
	void updateBitboards();

	// swap white<=>black
	void swap();

	// set piece of color at square (used in xboard edit mode)
	bool setPiece( Color c, Piece p, Square sq );

	inline Square king( Color color ) const
	{
		assert( color <= ctBlack );
		return bkingPos[ color ];
	}

	inline bool isVacated( Square sq ) const
	{
		return PiecePack::type( piece( sq ) ) == ptNone;
	}

	inline Bitboard diagSliders( Color color ) const
	{
		return pieces( color, ptBishop ) | pieces( color, ptQueen );
	}

	inline Bitboard orthoSliders( Color color ) const
	{
		return pieces( color, ptRook ) | pieces( color, ptQueen );
	}

	inline Bitboard diagSliders() const
	{
		return pieces( ctWhite, ptBishop ) | pieces( ctBlack, ptBishop ) |
			pieces( ctWhite, ptQueen ) | pieces( ctBlack, ptQueen );
	}

	inline Bitboard orthoSliders() const
	{
		return pieces( ctWhite, ptRook ) | pieces( ctBlack, ptRook ) |
			pieces( ctWhite, ptQueen ) | pieces( ctBlack, ptQueen );
	}

	// see helper: get mask for all attacks (both sides) to sq (use occ mask)
	inline Bitboard allAttacksTo( Square sq, Bitboard occ ) const
	{
		Bitboard mask;

		// knights
		mask = Tables::knightAttm[ sq ] & (pieces( ctWhite, ptKnight ) | pieces( ctBlack, ptKnight ));

		// pawns
		mask |= (Tables::pawnAttm[ ctBlack ][ sq ] & pieces( ctWhite, ptPawn )) |
				(Tables::pawnAttm[ ctWhite ][ sq ] & pieces( ctBlack, ptPawn ));

		// sliders
		Bitboard queens = pieces( ctWhite, ptQueen ) | pieces( ctBlack, ptQueen );
		Bitboard orthoSliders = pieces( ctWhite, ptRook ) | pieces( ctBlack, ptRook ) | queens;
		Bitboard diagSliders = pieces( ctWhite, ptBishop ) | pieces( ctBlack, ptBishop ) | queens;

		mask |= diagSliders & Magic::bishopAttm( sq, occ );
		mask |= orthoSliders & Magic::rookAttm( sq, occ );

		// and finally kings
		mask |= Tables::kingAttm[ sq ] & (BitOp::oneShl(king( ctWhite )) | BitOp::oneShl(king( ctBlack )) );

		return mask & occ;
	}

	// returns 1 if square is attacked by color
	template< bool checksliders > bool doesAttack( Color color, Square sq ) const
	{
		Color opc = flip(color);

		// check pawns
		if ( Tables::pawnAttm[ opc ][ sq ] & pieces( color, ptPawn ) )
			return 1;

		// check knights
		if ( Tables::knightAttm[sq] & pieces( color, ptKnight) )
			return 1;

		// check opp king
		if ( Tables::neighbor[ sq ][ king(color) ] )
			return 1;

		if ( checksliders )
		{
			// check sliders
			Bitboard occ = occupied();
			Bitboard sliders;
			sliders = diagSliders( color );
			if ( (Tables::diagAttm[sq] & sliders) &&
				 (Magic::bishopAttm( sq, occ ) & sliders) )
				return 1;
			sliders = orthoSliders( color );
			return
				(Tables::orthoAttm[sq] & sliders) &&
				(Magic::rookAttm( sq, occ ) & sliders);
		} else return 0;
	}

	// returns 1 if square is attacked by color
	template< bool checksliders > bool doesAttack( Color color, Square sq, Bitboard occ ) const
	{
		Color opc = flip(color);

		// check pawns
		if ( Tables::pawnAttm[ opc ][ sq ] & pieces( color, ptPawn ) )
			return 1;

		// check knights
		if ( Tables::knightAttm[sq] & pieces( color, ptKnight) )
			return 1;

		// check opp king
		if ( Tables::neighbor[ sq ][ king(color) ] )
			return 1;

		if ( checksliders )
		{
			// check sliders
			Bitboard sliders;
			sliders = diagSliders( color );
			if ( (Tables::diagAttm[sq] & sliders) &&
				(Magic::bishopAttm( sq, occ ) & sliders) )
				return 1;
			sliders = orthoSliders( color );
			return
				(Tables::orthoAttm[sq] & sliders) &&
				(Magic::rookAttm( sq, occ ) & sliders);
		} else return 0;
	}

	// returns pin mask for pincolor and attackColor
	template< Color pinColor, Color attackColor, bool noqueens > Bitboard pinTemplate( Square sq ) const
	{
		// FIXME: perhaps this could be completely rewritten to make it faster
		// (consider all rays separately)

		Bitboard occ = occupied();

		Bitboard queens = pieces( attackColor, ptQueen );
		Bitboard candidates = pieces( pinColor );
		if ( noqueens )
			candidates &= ~queens;
		Bitboard diagSliders = pieces( attackColor, ptBishop ) | queens;
		Bitboard orthoSliders = pieces( attackColor, ptRook ) | queens;
		Bitboard res = 0;

		if ( diagSliders & Tables::diagAttm[ sq ] )
		{
			Bitboard tmp = Magic::bishopAttm( sq, occ ) & candidates;
			if ( tmp )
			{
				Bitboard tmp2 = Magic::bishopAttm( sq, occ & ~tmp ) & diagSliders;
				while( tmp2 )
					res |= Tables::between[ sq ][ BitOp::popBit( tmp2 ) ] & tmp;
			}
		}

		if ( orthoSliders & Tables::orthoAttm[ sq ] )
		{
			Bitboard tmp = Magic::rookAttm( sq, occ ) & candidates;
			if ( tmp )
			{
				Bitboard tmp2 = Magic::rookAttm( sq, occ & ~tmp ) & orthoSliders;
				while( tmp2 )
					res |= Tables::between[ sq ][ BitOp::popBit( tmp2 ) ] & tmp;
			}
		}

		return res;
	}

	// get checkers from stm's point of view
	inline Bitboard checkers() const
	{
		Bitboard res;

		Color color = turn();
		Square kp = king( color );
		Color opcolor = flip( color );

		// pawns
		res = Tables::pawnAttm[ color ][ kp ] & pieces( opcolor, ptPawn );

		// knights
		res |= Tables::knightAttm[ kp ] & pieces( opcolor, ptKnight );

		// sliders
		Bitboard occ = occupied();

		Bitboard opqueens = pieces( opcolor, ptQueen );
		Bitboard opdiag = pieces( opcolor, ptBishop ) | opqueens;
		Bitboard oportho = pieces( opcolor, ptRook ) | opqueens;

		res |= Magic::bishopAttm( kp, occ ) & opdiag;
		res |= Magic::rookAttm( kp, occ ) & oportho;

		// note: king can't check directly so we're done now

		return res;
	}

	// returns discovered checkers from stm point of view (=stm's dcs)
	inline Bitboard discovered() const
	{
		Color color = turn();
		Square okp = king( flip( color ) );							// opponent's kingpos

		return color == ctWhite ?
			pinTemplate< ctWhite, ctWhite, 1 >( okp )
			: pinTemplate< ctBlack, ctBlack, 1 >( okp );
	}

	// returns pins from stm point of view (=stm's pins)
	inline Bitboard pins() const
	{
		Color color = turn();
		Square kp = king( color );								// my kingpos

		return color == ctWhite ?
			pinTemplate< ctWhite, ctBlack, 0 >( kp )
			: pinTemplate< ctBlack, ctWhite, 0 >( kp );
	}

	// returns 1 if move does check opponent king (from stm point of view)
	bool isCheck( Move m, Bitboard discovered ) const;

	// FIXME: merge later. iisLegal for debugging purposes ATM

	// returns 1 if stm's move is legal
	// used for hashmove/killer validation
	template< bool evasion, bool killer > bool iisLegal( Move m, Bitboard pins ) const;
	template< bool evasion, bool killer > bool isLegal( Move m, Bitboard pins ) const;

	// returns 1 if stm's pseudolegal move is actually legal
	// assume castling moves are always legal => DON'T pass any castling moves!
	template< bool evasion > bool pseudoIsLegal( Move m, Bitboard pins ) const;

	// recompute hashes, material and update board squares from bitboards
	void update();

	// necessary for tuning, fast-update material
	void updateDeltaMaterial();

	// undo psq values from scores (opening, endgames)
	void undoPsq( FineScore *scores ) const;

	// parse from fen
	// returns 0 on error
	const char *fromFEN( const char *fen );

	// returns fen for current position
	std::string toFEN() const;
	// fast version, doesn't add null terminator
	char *toFEN(char *dst) const;

	// move to SAN
	std::string toSAN( Move m ) const;
	// fast version, doesn't add null terminator
	char *toSAN( char *dst, Move m ) const;
	// move to UCI
	std::string toUCI( Move m ) const;
	// fast version, doesn't add null terminator
	char *toUCI( char *dst, Move m ) const;
	// move from UCI
	Move fromUCI( const char *&c ) const;
	Move fromUCI( const std::string &str ) const;
	// move from SAN (includes legality check)
	Move fromSAN( const char *&ptr ) const;
	Move fromSAN( const std::string &str ) const;

	// recompute hash (debug)
	Signature recomputeHash() const;
	// recompute pawn hash (debug)
	Signature recomputePawnHash() const;
	// validate board (debug)
	bool isValid() const;

	// is trivial draw? (material or fifty rule)
	Draw isDraw() const;

	// debug dump
	void dump() const;

	// can do nullmove?
	inline bool canDoNull() const
	{
		// Fonzy's trick
		if ((materialKey() & matNPMask[turn()]) == matKN[turn()])
			return 0;
		return nonPawnMat( turn() ) != 0;
	}

	// can prune move?
	// note: before move is made
	inline bool canPrune( Move m ) const
	{
		// don't prune passer pushes
		Square to = MovePack::to( m );
		Piece p = piece(to);
		if ( PiecePack::type( p ) != ptPawn )
			return 1;
		// don't prune passer pushes
		return (Tables::passerMask[ turn() ][ to ] & pieces( flip(turn()), ptPawn )) != 0;
	}

	// can reduce move?
	// note: move already made
	inline bool canReduce( Move m ) const
	{
		Square to = MovePack::to( m );
		Piece p = piece(to);
		if ( PiecePack::type( p ) != ptPawn )
			return 1;
		// don't reduce passer pushes
		return (Tables::passerMask[ flip(turn()) ][ to ] & pieces( turn(), ptPawn )) != 0;
	}

	// static exchange evaluator
	// fast => used in movegen (where only bad/good capture = sign matters)
	template< bool fast > int see( Move m ) const;

	// returns 1 if move is irreversible
	inline bool isIrreversible( Move m ) const
	{
		return MovePack::isSpecial(m) ||
			PiecePack::type( piece( MovePack::from(m) ) ) == ptPawn;
	}

	bool compare( const Board &tmp ) const;

	// move gain ( used in qs delta(futility) )
	Score moveGain( Move m ) const
	{
		assert( !MovePack::isCastling(m) && MovePack::isCapture(m) );
		Score res = Tables::gainPromo[ MovePack::promo(m) ];
		return res + Tables::gainCap[ PiecePack::type( piece( MovePack::to(m) ) ) ];
	}
};

}
