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

// TODO: split into more headers? (especially material stuff)

#include "types.h"

namespace cheng4
{

#ifdef _MSC_VER
#	pragma warning( disable:4127 )		// disable conditional expression is constant warning
#endif

#ifndef U64C
#	define U64C(x) ((u64)(x##ull))
#endif

// enums are only used as constants

// trivial draw types
enum DrawType
{
	drawNotDraw,			// not a draw
	drawMaterial,
	drawFifty,
	drawRepetition
};

// directions
enum Direction
{
	dirNone,
	dirUp,
	dirDown,
	dirLeft,
	dirRight,
	dirUpLeft,
	dirUpRight,
	dirDownLeft,
	dirDownRight,
	dirMax					// max enum value
};

enum ColorType
{
	ctWhite,
	ctBlack,
	ctMax
};

enum PieceType
{
	ptNone,
	ptPawn,
	ptKnight,
	ptBishop,
	ptRook,
	ptQueen,
	ptKing,
	ptMax					// max enum value
};

enum PieceMask
{
	pmType			=	7,
	pmColor			=	8
};

enum PieceShift
{
	psType			=	0,
	psColor			=	3
};

// game phases
enum PhaseType
{
	phOpening,
	phEndgame,
	phMax
};

enum BoundType
{
	btNone,				// none (unused)
	btUpper,			// upper bound (alpha)
	btLower,			// lower bound (beta)
	btExact				// exact
};

enum ScoreConstants
{
	scInvalidMask	= ~32767,			// invalid score mask
	scInvalid 		= -32768,			// invalid score
	scInfinity		= 32767-256,		// infinity (note: must have a reserve here when storing rel. mates in 16-bits
										// (hashtable)
	scMate			= scInfinity - 1000,// everything above and including this is a mate
	scDraw			= 0					// draw score
};

// various limits
enum Limits
{
	maxDepth	=	120,
	maxPly		=	240,
	maxStack	=	maxPly+4,
	maxPV		=	maxStack,
	// triangular PV table limit
	maxTriPV	=	(maxStack*(maxStack+1)/2)+4,
	repHashMax	=	256,				// repetition limit

	maxStrength =	2500,				// maximum elo strength for elo limit
	minStrength =	800					// minimum elo strength for elo limit
};

// fractional ply constants
enum FracConsts
{
	fracShift	=	8,					// frac depth shift
	fracOnePly	=	1 << fracShift		// frac one ply
};

// bitboard index
#define BBI( c, t ) (((t)<<1)|c)

enum BBIConstants
{
	bbiWOcc			=	0,						// special bbi: all (=occupied), includes kings!
	bbiBOcc			=	1,
	bbiMat			=	BBI( ctWhite, ptKing ),	// material key index
	bbiEvMask		=	bbiMat+1,				// for evasions only: evasion target mask
	bbiMax			=	bbiEvMask+1
};

enum MaterialConstants
{
	mpcMask			=	63						// material piece count mask
};

enum SquareConstants
{
	sqInvalid		=	255						// invalid square
};

// file
typedef u8 File;
// rank
typedef u8 Rank;
// square
typedef u8 Square;
// piece
typedef u8 Piece;
// distance
typedef u8 Distance;
// bitboard
typedef u64 Bitboard;
// hash signature
typedef u64 Signature;
// material key
typedef u64 MaterialKey;
// material piece count
typedef u8 PieceCount;
// move
typedef u32 Move;
// color
typedef u8 Color;
// phase
typedef u8 Phase;
// move count
typedef u32 MoveCount;
// fifty move rule count
typedef u8 FiftyCount;
// castling rights
// low nybble: short (rookfile+1)
// high nybble: long (rookfile+1)
typedef u8 CastRights;
// hash depth
typedef i8 HashDepth;
// hash move
typedef u16 HashMove;
// hash bound
typedef u8 HashBound;
// value stored in hashtable (centipawns)
typedef i16 HashScore;
// score used throughout search (centipawns)
typedef i32 Score;
// fine score (millipawns)
typedef i32 FineScore;
// fractional depth (fp 8:8)
typedef i32 FracDepth;
// depth
typedef i8 Depth;
// ply id (0 up)
typedef uint Ply;
// draw
typedef u8 Draw;
// nodecount
typedef u64 NodeCount;
// time
typedef u32 Time;
// age
typedef u8 Age;
// delta material
typedef i16 DMat;
// non-pawn material
typedef u16 NPMat;
// abort flag
typedef u8 AbortFlag;
// book move
typedef u16 BookMove;
// book count type
typedef u16 BookCount;
// book probe counter
typedef u32 BookProbe;

struct PiecePack
{
	// assume color already cleared
	static inline bool isSlider( Piece p )
	{
		assert( p >= ptPawn && p <= ptKing );
		return p >= ptBishop && p <= ptQueen;
	}
	static inline Piece type( Piece p )
	{
		assert( (p & pmType) <= ptKing );
		return p & pmType;
	}
	static inline Color color( Piece p )
	{
		return (Color)(p >> psColor);
	}
	static inline Piece init( Color c, Piece p )
	{
		assert( c <= ctBlack && p <= ptKing );
		return p | ((Piece)c << psColor);
	}
};

struct SquarePack
{
	static inline bool isRank1Or8( Square sq )
	{
		sq &= 0x38;
		return sq == 0 || sq == 0x38;
	}

	template< Color c, int step > static inline Square advanceRank( Square sq )
	{
		if ( c == ctWhite )
			sq -= (u8)(step*8);
		else
			sq += (u8)(step*8);
		assert( sq < 64 );
		return sq;
	}

	static inline bool neighborFile( Square sq0, Square sq1 )
	{
		File f0 = file(sq0);
		File f1 = file(sq1);
		return f0+1 == f1 || f1+1 == f0;
	}

	// build opp pawn square from ep square and from square
	static inline Square epTarget( Square ep, Square from )
	{
		return (from & 0x38) | (ep & 7);
	}

	static inline File file( Square sq )
	{
		return sq & 7;
	}
	static inline Rank rank( Square sq )
	{
		return (Rank)(sq >> 3);
	}
	template< Color c > static inline Rank relRank( Square sq )
	{
		assert( c <= ctBlack && sq < 64 );
		return (sq >> 3) ^ (c == ctBlack ? 7 : 0);
	}
	static inline Rank relRank( Color c, Square sq )
	{
		return c == ctWhite ? relRank<ctWhite>(sq) : relRank<ctBlack>(sq);
	}
	static inline Square init( File f, Rank r )
	{
		assert( f < 8 && r < 8 );
		return ((Square)r << 3) | f;
	}
	template< Color c > static inline Square initRel( File f, Rank r )
	{
		assert( f < 8 && r < 8 );
		return (((Square)r << 3) | f) ^ (c == ctBlack ? 0x38 : 0);
	}
	static inline bool isValid( Square s )
	{
		return !(s & ~63);
	}
	static inline Square setRank( Square s, Rank r )
	{
		assert( r < 8 );
		return (s & 7) | (r << 3);
	}
	static inline Square setFile( Square s, File f )
	{
		assert( f < 8 );
		return (s & ~7) | f;
	}
	// flip horizontally
	static inline Square flipH( Square s )
	{
		return s ^ 7;
	}
	// flip vertically
	static inline Square flipV( Square s )
	{
		return s ^ 0x38;
	}
	// flip about main diagonal
	static inline Square flipD( Square s )
	{
		return (s >> 3) | ((s & 7) << 3);
	}
};

struct CastPack
{
	static inline bool allowed( CastRights cr )
	{
		return cr != 0;
	}
	static inline bool allowedShort( CastRights cr )
	{
		return (cr & 0xf) != 0;
	}
	static inline bool allowedLong( CastRights cr )
	{
		return (cr >> 4) != 0;
	}
	static inline File shortFile( CastRights cr )
	{
		return (File)((cr & 0xf)-1);
	}
	static inline File longFile( CastRights cr )
	{
		return (File)((cr >> 4)-1);
	}
	static inline CastRights init( File shortf, File longf )
	{
		return ((1+longf)<<4) | (1+shortf);
	}
	static inline CastRights initShort( File shortf )
	{
		return 1+shortf;
	}
	static inline CastRights initLong( File longf )
	{
		return (1+longf)<<4;
	}
	static inline CastRights loseShort( CastRights cr )
	{
		return cr & ~15;
	}
	static inline CastRights loseLong( CastRights cr )
	{
		return cr & 15;
	}
	static inline CastRights loseRights( bool kingside, CastRights cr )
	{
		return kingside ? loseShort( cr ) : loseLong( cr );
	}
	static inline CastRights loseFile( File rf, CastRights cr )
	{
		return (cr & 15) == rf+1 ? loseShort(cr) : loseLong(cr);
	}
	static inline CastRights initFile( bool kingside, File f )
	{
		return kingside ? initShort( f ) : initLong( f );
	}
	// build rook square from king target square
	static inline Square rookSquare( Square kto, CastRights cr )
	{
		return (Square)((kto & 0x38) | ( (kto & 4) ? shortFile(cr) : longFile(cr) ));
	}
};

// flip color
static inline Color flip( Color c )
{
	assert( c <= ctBlack );
	return c ^ 1;
}

// is slider pice?
static inline bool isSlider( Piece pt )
{
	return pt >= ptBishop && pt <= ptQueen;
}

struct ScorePack
{
	// init fine (mp) score from score (cp)
	static inline FineScore initFine( Score score )
	{
		return (FineScore)score * 10;
	}
	// init normal (cp) score from fine score (mp)
	static inline Score init( FineScore fscore )
	{
		return (Score)(fscore / 10);
	}
	// pack hash score
	static inline HashScore packHash( Score score, Ply ply )
	{
		// convert to relative if mate score
		if ( isMate( score ) )
			score += score < 0 ? -(Score)ply : (Score)ply;
		assert( score >= -32768 && score <= 32767 );
		return (HashScore)score;
	}
	// unpack hash score
	static inline Score unpackHash( HashScore score, Ply ply )
	{
		// convert to absolute if mate score
		if ( isMate( score ) )
			return (Score)score + (score < 0 ? (HashScore)ply : -(HashScore)ply);
		return (Score)score;
	}
	// interpolate fine score
	static inline Score interpolate( NPMat npmat, FineScore opscore, FineScore endscore )
	{
		const int egnp = 20;
		const int opnp = 62;

		FineScore d = (((FineScore)npmat - egnp) * 256) / (opnp-egnp);
		d = (d < 0) ? 0 : (d > 256) ? 256 : d;
		return init( endscore + d * (opscore - endscore) / 256 );
	}
	// is mate score?
	static inline bool isMate( Score score )
	{
		return abs( score ) >= scMate;
	}
	// is valid score?
	static inline bool isValid( Score score )
	{
		return !(abs(score) & scInvalidMask);
	}
	// checkmated in ply plies
	static inline Score checkMated( Ply ply )
	{
		return -scInfinity + ply;
	}
	// mate in ply plies
	static inline Score mateIn( Ply ply )
	{
		return scInfinity - ply - 1;
	}
};

// material key
// organized as follows: (6-bit entries)
// bq, wq, br, wr, bb, wb, bn, wn, bp, wp
// total: 10*6 = 60 bits, 4 MSBits unused
#define MATSHIFT( c, t ) (( (((t)-1)<<1) | (c) ) * 6)

struct MaterialPack
{
	static inline PieceCount count( MaterialKey mk, Color color, Piece type )
	{
		return (mk >> MATSHIFT( color, type )) & mpcMask;
	}
};

// predefined material constants
static const MaterialKey matMask[ctMax] = {
	(U64C(63) << MATSHIFT( ctWhite, ptPawn)) |
	(U64C(63) << MATSHIFT( ctWhite, ptKnight)) |
	(U64C(63) << MATSHIFT( ctWhite, ptBishop)) |
	(U64C(63) << MATSHIFT( ctWhite, ptRook)) |
	(U64C(63) << MATSHIFT( ctWhite, ptQueen)),

	(U64C(63) << MATSHIFT( ctBlack, ptPawn)) |
	(U64C(63) << MATSHIFT( ctBlack, ptKnight)) |
	(U64C(63) << MATSHIFT( ctBlack, ptBishop)) |
	(U64C(63) << MATSHIFT( ctBlack, ptRook)) |
	(U64C(63) << MATSHIFT( ctBlack, ptQueen))
};

static const MaterialKey matNPMask[ctMax] = {
	(U64C(63) << MATSHIFT( ctWhite, ptKnight)) |
	(U64C(63) << MATSHIFT( ctWhite, ptBishop)) |
	(U64C(63) << MATSHIFT( ctWhite, ptRook)) |
	(U64C(63) << MATSHIFT( ctWhite, ptQueen)),

	(U64C(63) << MATSHIFT( ctBlack, ptKnight)) |
	(U64C(63) << MATSHIFT( ctBlack, ptBishop)) |
	(U64C(63) << MATSHIFT( ctBlack, ptRook)) |
	(U64C(63) << MATSHIFT( ctBlack, ptQueen))
};

static const MaterialKey matKnightMask[ctMax] = {
	(U64C(63) << MATSHIFT( ctWhite, ptKnight )),
	(U64C(63) << MATSHIFT( ctBlack, ptKnight ))
};

static const MaterialKey matBishopMask[ctMax] = {
	(U64C(63) << MATSHIFT( ctWhite, ptBishop )),
	(U64C(63) << MATSHIFT( ctBlack, ptBishop ))
};

static const MaterialKey matRookOrBetter[ctMax] = {
	(U64C(63) << MATSHIFT( ctWhite, ptRook )) |
	(U64C(63) << MATSHIFT( ctWhite, ptQueen )),
	(U64C(63) << MATSHIFT( ctBlack, ptRook )) |
	(U64C(63) << MATSHIFT( ctBlack, ptQueen ))
};

// no-draw-mask
static const MaterialKey matNoDrawMask =
{
	(U64C(63) << MATSHIFT( ctWhite, ptPawn)) |
	(U64C(63) << MATSHIFT( ctWhite, ptRook)) |
	(U64C(63) << MATSHIFT( ctWhite, ptQueen)) |
	(U64C(63) << MATSHIFT( ctBlack, ptPawn)) |
	(U64C(63) << MATSHIFT( ctBlack, ptRook)) |
	(U64C(63) << MATSHIFT( ctBlack, ptQueen))
};

// potential mating material mask [color]
static const MaterialKey matPotMating[ctMax] = {
	(U64C(63) << MATSHIFT( ctWhite, ptPawn)) |
	(U64C(63) << MATSHIFT( ctWhite, ptRook)) |
	(U64C(63) << MATSHIFT( ctWhite, ptQueen)),
	(U64C(63) << MATSHIFT( ctBlack, ptPawn)) |
	(U64C(63) << MATSHIFT( ctBlack, ptRook)) |
	(U64C(63) << MATSHIFT( ctBlack, ptQueen))
};

// major pieces mask
static const MaterialKey matMajors[ctMax] = {
	(U64C(63) << MATSHIFT( ctWhite, ptRook)) |
	(U64C(63) << MATSHIFT( ctWhite, ptQueen)),
	(U64C(63) << MATSHIFT( ctBlack, ptRook)) |
	(U64C(63) << MATSHIFT( ctBlack, ptQueen))
};

// special constants
static const MaterialKey matKk = 0;			// bare kings
static const MaterialKey matKNk = U64C(1) << MATSHIFT( ctWhite, ptKnight );
static const MaterialKey matKkn = U64C(1) << MATSHIFT( ctBlack, ptKnight );
static const MaterialKey matKNK[ctMax] = { matKNk, matKkn };
static const MaterialKey matKPk = U64C(1) << MATSHIFT( ctWhite, ptPawn );
static const MaterialKey matKkp = U64C(1) << MATSHIFT( ctBlack, ptPawn );
static const MaterialKey matKRN[ctMax] =
{
	(U64C(1) << MATSHIFT( ctWhite, ptRook )) | (U64C(1) << MATSHIFT( ctWhite, ptKnight )),
	(U64C(1) << MATSHIFT( ctBlack, ptRook )) | (U64C(1) << MATSHIFT( ctBlack, ptKnight ))
};
static const MaterialKey matKRB[ctMax] =
{
	(U64C(1) << MATSHIFT( ctWhite, ptRook )) | (U64C(1) << MATSHIFT( ctWhite, ptBishop )),
	(U64C(1) << MATSHIFT( ctBlack, ptRook )) | (U64C(1) << MATSHIFT( ctBlack, ptBishop ))
};
static const MaterialKey matKR[ctMax] =
{
	U64C(1) << MATSHIFT( ctWhite, ptRook ), U64C(1) << MATSHIFT( ctBlack, ptRook )
};
static const MaterialKey matKB[ctMax] =
{
	U64C(1) << MATSHIFT( ctWhite, ptBishop ), U64C(1) << MATSHIFT( ctBlack, ptBishop )
};
static const MaterialKey matKN[ctMax] =
{
	U64C(1) << MATSHIFT( ctWhite, ptKnight ), U64C(1) << MATSHIFT( ctBlack, ptKnight )
};
static const MaterialKey matKBB[ctMax] =
{
	U64C(2) << MATSHIFT( ctWhite, ptBishop ), U64C(2) << MATSHIFT( ctBlack, ptBishop )
};
static const MaterialKey matKNN[ctMax] =
{
	U64C(2) << MATSHIFT( ctWhite, ptKnight ), U64C(2) << MATSHIFT( ctBlack, ptKnight )
};
static const MaterialKey matKBN[ctMax] =
{
	(U64C(1) << MATSHIFT( ctWhite, ptBishop )) | (U64C(1) << MATSHIFT( ctWhite, ptKnight )),
	(U64C(1) << MATSHIFT( ctBlack, ptBishop )) | (U64C(1) << MATSHIFT( ctBlack, ptKnight ))
};
static const MaterialKey matBishopEGMask =
	(U64C(63) << MATSHIFT( ctWhite, ptKnight)) |
	(U64C(63) << MATSHIFT( ctWhite, ptRook)) |
	(U64C(63) << MATSHIFT( ctWhite, ptQueen)) |
	(U64C(63) << MATSHIFT( ctBlack, ptKnight)) |
	(U64C(63) << MATSHIFT( ctBlack, ptRook)) |
	(U64C(63) << MATSHIFT( ctBlack, ptQueen));
static const MaterialKey matKRPkr =
	(U64C(1) << MATSHIFT( ctWhite, ptPawn )) |
	(U64C(1) << MATSHIFT( ctWhite, ptRook )) |
	(U64C(1) << MATSHIFT( ctBlack, ptRook ));
static const MaterialKey matKRkrp =
	(U64C(1) << MATSHIFT( ctBlack, ptPawn )) |
	(U64C(1) << MATSHIFT( ctBlack, ptRook )) |
	(U64C(1) << MATSHIFT( ctWhite, ptRook ));

}
