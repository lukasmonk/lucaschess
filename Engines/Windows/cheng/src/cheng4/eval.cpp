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

#include "eval.h"
#include "utils.h"
#include "chtypes.h"
#include "kpk.h"
#include "tune.h"
#include <memory.h>
#include <algorithm>
#include <new>

namespace cheng4
{

// eval helper
static const Piece ptAll = ptNone;

// stm bonus in cp (=tempo)
static TUNE_CONST Score stmBonus			=	5;

// weights/parameters:
static TUNE_CONST FineScore certainWin		=	128000;
static TUNE_CONST FineScore kpkWin			=	64000;

TUNE_STATIC TUNE_CONST int safetyScale[] = {
	0, 19, 44, 26, 72, 51
};

static TUNE_CONST FineScore shelterFront1 = 224;
static TUNE_CONST FineScore shelterFront2 = 185;

static TUNE_CONST FineScore bishopPairOpening = 351;
static TUNE_CONST FineScore bishopPairEndgame = 527;

static TUNE_CONST FineScore trappedBishopOpening = 157;
static TUNE_CONST FineScore trappedBishopEndgame = 1552;

static TUNE_CONST FineScore unstoppablePasser = 2067;

static TUNE_CONST FineScore isolatedPawnOpening = 152;
static TUNE_CONST FineScore isolatedPawnEndgame = 121;

static TUNE_CONST FineScore doubledPawnOpening = 66;
static TUNE_CONST FineScore doubledPawnEndgame = 102;

// note: only tune index 0
TUNE_STATIC TUNE_CONST Score passerScaleImbalance[2] = {
	225, 256
};

// note: only tune index 0
TUNE_STATIC TUNE_CONST Score passerScaleBlocked[2] = {
	144, 256
};

// note: 7th rank is unimportant
TUNE_STATIC TUNE_CONST FineScore candPasserOpening[8] = {
	0, -18, -28, 59, 169, 108, 0, 0
};

TUNE_STATIC TUNE_CONST FineScore candPasserEndgame[8] = {
	0, -16, -2, 138, 378, 815, 0, 0
};

TUNE_STATIC TUNE_CONST FineScore passerOpening[8] = {
	0, 137, -54, 42, 247, 468, 740, 0
};

TUNE_STATIC TUNE_CONST FineScore passerEndgame[8] = {
	0, 158, 178, 438, 782, 1280, 1849, 0
};

TUNE_STATIC TUNE_CONST i16 knightMobility[phMax][9] = {
	{ -479, -401, -289, -215, -131, -76, -45, -26, 29 },
	{ -495, -497, -263, -158, -59, 31, 56, 62, 46 }
};
static TUNE_CONST FineScore knightHangingOpening = 402;
static TUNE_CONST FineScore knightHangingEndgame = 303;

TUNE_STATIC TUNE_CONST i16 bishopMobility[phMax][14] = {
	{ -228, -184, -82, -28, 37, 66, 82, 95, 99, 160, 162, 260, 332, 366 },
	{ -309, -257, -174, -67, 48, 135, 168, 212, 221, 267, 271, 351, 348, 456 }
};
static TUNE_CONST FineScore bishopHangingOpening = 443;
static TUNE_CONST FineScore bishopHangingEndgame = 352;

TUNE_STATIC TUNE_CONST i16 rookMobility[phMax][15] = {
	{ 38, 67, 112, 100, 83, 129, 168, 182, 217, 270, 253, 261, 286, 298, 318 },
	{ -163, 48, 158, 229, 271, 329, 380, 411, 495, 522, 572, 583, 609, 581, 711 }
};
static TUNE_CONST FineScore rookHangingOpening = 503;
static TUNE_CONST FineScore rookHangingEndgame = 306;

static TUNE_CONST FineScore rookOnOpenOpening = 153;
static TUNE_CONST FineScore rookOnOpenEndgame = 49;

TUNE_STATIC TUNE_CONST i16 queenMobility[phMax][28] = {
	{ 179, 237, 299, 313, 322, 344, 360, 381, 395, 428, 456, 479, 503, 529, 551, 577,
	 591, 627, 648, 672, 694, 721, 747, 770, 793, 815, 840, 866 },
	{ 335, 428, 485, 530, 583, 629, 678, 721, 782, 819, 858, 903, 967, 999, 1045, 1079,
	 1099, 1139, 1168, 1238, 1257, 1328, 1285, 1392, 1356, 1524, 1554, 1620 }
};
static TUNE_CONST FineScore queenHangingOpening = 396;
static TUNE_CONST FineScore queenHangingEndgame = 4;

static TUNE_CONST int kingPasserSupportBase = 1;
static TUNE_CONST FineScore kingPasserSupportScale = 38;

// pawn race (=endgame) => bonus to side with more pawns
static TUNE_CONST FineScore pawnRaceAdvantageEndgame = 1010;

// good/bad bishops
TUNE_STATIC TUNE_CONST i16 goodBishopOpening[17] = {
	374, -10, 84, -112, -110, -79, -67, -39, 9, 80, 81, 205, 212, 319, 412, 456, 749
};

TUNE_STATIC TUNE_CONST i16 goodBishopEndgame[17] = {
	-504, -357, -263, -304, -211, -151, -102, -33, 159, 66, 130, 131, 234, 272, 317, 205, 120
};

// knight outpost bonus scale based on file
TUNE_STATIC TUNE_CONST FineScore outpostBonusFile[8] = {
	3, 20, 90, 171, 249, 84, 99, 125
};

// knight outpost bonus scale based on rank
TUNE_STATIC TUNE_CONST FineScore outpostBonusRank[8] = {
	0, 0, -105, 30, 78, 103, 21, 8
};

// eval helper
template< Color c > static inline int sign()
{
	return c == ctWhite ? 1 : -1;
}

static inline int sign( Color c )
{
	assert( c <= (Color)ctBlack );
	return Tables::sign[c];
}

// EG recognizers

template< Color c > static void kpk( const Board &b, FineScore *fscore )
{
	// FIXME: use getMSB on platforms (ARM) where it's faster?
	assert( b.pieces( c, ptPawn ) );
	Square psq = (Square)BitOp::getLSB( b.pieces( c, ptPawn ) );
	u8 draw = KPK::isDraw( c, b.turn(), b.king( c ), b.king( flip(c) ), psq );
	if ( draw )
		fscore[ phEndgame ] = 0;
	else
		fscore[ phEndgame ] += sign<c>()*kpkWin;
}

static const FineScore kbnTable[64] =
{
	+999, +500, +250, +100, -100, -250, -500, -999,
	+500, +400, +250, +100, -100, -250, -400, -500,
	+250, +250, +150, + 80, - 80, -150, -250, -250,
	+100, +100, + 80, + 50, - 50, - 80, -100, -100,
	-100, -100, - 80, - 50, + 50, + 80, +100, +100,
	-250, -250, -150, - 80, + 80, +150, +250, +250,
	-500, -400, -250, -100, +100, +250, +400, +500,
	-999, -500, -250, -100, +100, +250, +500, +999
};

static void undoStmBonus( const Board &b, FineScore *fscore )
{
	for ( Phase p = phOpening; p <= phEndgame; p++ )
		fscore[ p ] -= ScorePack::initFine( (Score)Tables::sign[b.turn()] * stmBonus );
}

template< Color c > static void kbnk( const Board &b, FineScore *fscore )
{
	b.undoPsq( fscore );
	undoStmBonus( b, fscore );
	Bitboard bishop = b.pieces(c, ptBishop);
	assert( bishop );
	Square ksq = b.king(flip(c));
	if ( bishop & darkSquares )
		ksq = SquarePack::flipH(ksq);
	FineScore bonus = kbnTable[ksq]*3;
	fscore[phEndgame] += sign<c>() * bonus;
}

template< Color c > static void krpkr( const Board &b, FineScore *fscore )
{
	// FIXME: use getMSB on platforms (ARM) where it's faster?
	// only draw is opponent king is in front of pawn
	assert( b.pieces( c, ptPawn ) );
	Bitboard opkm = BitOp::oneShl( b.king( flip(c) ) );
	Square psq = (Square)BitOp::getLSB( b.pieces( c, ptPawn ) );
	Bitboard fm = Tables::frontMask[c][psq];
	fm |= (fm << 1) & L1MASK;
	fm |= (fm >> 1) & R1MASK;
	if ( fm & opkm )
	{
		fscore[ phEndgame ] /= 16;
	}
}

static const size_t numRecognizers = 6;

static Eval::Recognizer recognizers[ numRecognizers ] =
{
	{ matKPk, kpk<ctWhite> },
	{ matKkp, kpk<ctBlack> },
	{ matKBN[ctWhite], kbnk<ctWhite> },
	{ matKBN[ctBlack], kbnk<ctBlack> },
	{ matKRPkr, krpkr<ctWhite> },
	{ matKRkrp, krpkr<ctBlack> }
};

// EG scale recognizers

template< Color c > static void krminor( const Board &b, FineScore *fscore )
{
	MaterialKey mk = b.materialKey() & matNPMask[ flip(c) ];
	if ( sign<c>()*fscore[ phEndgame ] > 0 && mk == matKR[ flip(c) ] )
	{
		fscore[ phOpening ] /= 8;
		fscore[ phEndgame ] /= 8;
	}
}

template< Color c > static void kbn( const Board &b, FineScore *fscore )
{
	MaterialKey mk = b.materialKey() & matNPMask[ flip(c) ];
	if ( sign<c>()*fscore[ phEndgame ] > 0 &&
		(mk == matKN[ flip(c) ] || mk == matKB[ flip(c) ]))
	{
		fscore[ phOpening ] /= 8;
		fscore[ phEndgame ] /= 8;
	}
}

template< Color c > static void kbb( const Board &b, FineScore *fscore )
{
	MaterialKey mk = b.materialKey() & matNPMask[ flip(c) ];
	// note: kbb vs kn is usually a win!
	if ( sign<c>()*fscore[ phEndgame ] > 0 && mk == matKB[ flip(c) ] )
	{
		fscore[ phOpening ] /= 8;
		fscore[ phEndgame ] /= 8;
	}
}

template< Color c > static void kr( const Board &b, FineScore *fscore )
{
	MaterialKey mk = b.materialKey() & matNPMask[ flip(c) ];
	if ( sign<c>()*fscore[ phEndgame ] > 0 &&
		(mk == matKN[ flip(c) ] || mk == matKB[ flip(c) ]))
	{
		fscore[ phOpening ] /= 4;
		fscore[ phEndgame ] /= 4;
	}
}

static const size_t numScaleRecognizers = 5;

static Eval::Recognizer scaleRecognizers[2][ numScaleRecognizers ] =
{
	{
		{ matKRN[ctWhite], krminor<ctWhite> },
		{ matKRB[ctWhite], krminor<ctWhite> },
		{ matKBB[ctWhite], kbb<ctWhite> },
		{ matKBN[ctWhite], kbn<ctWhite> },
		{ matKR[ctWhite], kr<ctWhite> }
	},
	{
		{ matKRN[ctBlack], krminor<ctBlack> },
		{ matKRB[ctBlack], krminor<ctBlack> },
		{ matKBB[ctBlack], kbb<ctBlack> },
		{ matKBN[ctBlack], kbn<ctBlack> },
		{ matKR[ctBlack], kr<ctBlack> }
	}
};

// static initializer (sort recognizers)
void Eval::init()
{
	std::sort( recognizers, recognizers + numRecognizers );
	std::sort( scaleRecognizers[ctWhite], scaleRecognizers[ctWhite] + numScaleRecognizers );
	std::sort( scaleRecognizers[ctBlack], scaleRecognizers[ctBlack] + numScaleRecognizers );

	// Exported tunable params:
	// keeping it here prevent the C++ optimizer to optimize these away
	TUNE_EXPORT(FineScore, certainWin, certainWin);
	TUNE_EXPORT(FineScore, kpkWin, kpkWin);

	TUNE_EXPORT(int, safetyScale1, safetyScale[1]);
	TUNE_EXPORT(int, safetyScale2, safetyScale[2]);
	TUNE_EXPORT(int, safetyScale3, safetyScale[3]);
	TUNE_EXPORT(int, safetyScale4, safetyScale[4]);
	TUNE_EXPORT(int, safetyScale5, safetyScale[5]);

	TUNE_EXPORT(FineScore, shelterFront1, shelterFront1);
	TUNE_EXPORT(FineScore, shelterFront2, shelterFront2);

	TUNE_EXPORT(FineScore, bishopPairOpening, bishopPairOpening);
	TUNE_EXPORT(FineScore, bishopPairEndgame, bishopPairEndgame);

	TUNE_EXPORT(FineScore, trappedBishopOpening, trappedBishopOpening);
	TUNE_EXPORT(FineScore, trappedBishopEndgame, trappedBishopEndgame);

	TUNE_EXPORT(FineScore, unstoppablePasser, unstoppablePasser);

	TUNE_EXPORT(FineScore, doubledPawnOpening, doubledPawnOpening);
	TUNE_EXPORT(FineScore, doubledPawnEndgame, doubledPawnEndgame);

	TUNE_EXPORT(FineScore, isolatedPawnOpening, isolatedPawnOpening);
	TUNE_EXPORT(FineScore, isolatedPawnEndgame, isolatedPawnEndgame);

	TUNE_EXPORT(FineScore, knightHangingOpening, knightHangingOpening );
	TUNE_EXPORT(FineScore, knightHangingEndgame, knightHangingEndgame );

	TUNE_EXPORT(FineScore, bishopHangingOpening, bishopHangingOpening );
	TUNE_EXPORT(FineScore, bishopHangingEndgame, bishopHangingEndgame );

	TUNE_EXPORT(FineScore, rookHangingOpening, rookHangingOpening );
	TUNE_EXPORT(FineScore, rookHangingEndgame, rookHangingEndgame );
	TUNE_EXPORT(FineScore, rookOnOpenOpening, rookOnOpenOpening );
	TUNE_EXPORT(FineScore, rookOnOpenEndgame, rookOnOpenEndgame );

	TUNE_EXPORT(FineScore, queenHangingOpening, queenHangingOpening );
	TUNE_EXPORT(FineScore, queenHangingEndgame, queenHangingEndgame );

	TUNE_EXPORT(int, kingPasserSupportBase, kingPasserSupportBase);
	TUNE_EXPORT(FineScore, kingPasserSupportScale, kingPasserSupportScale);

	TUNE_EXPORT(i16, pawnOpening,	PSq::materialTables[phOpening][ptPawn]);
	TUNE_EXPORT(i16, knightOpening, PSq::materialTables[phOpening][ptKnight]);
	TUNE_EXPORT(i16, bishopOpening, PSq::materialTables[phOpening][ptBishop]);
	TUNE_EXPORT(i16, rookOpening,	PSq::materialTables[phOpening][ptRook]);
	TUNE_EXPORT(i16, queenOpening,	PSq::materialTables[phOpening][ptQueen]);

	TUNE_EXPORT(i16, pawnEndgame,	PSq::materialTables[phEndgame][ptPawn]);
	TUNE_EXPORT(i16, knightEndgame, PSq::materialTables[phEndgame][ptKnight]);
	TUNE_EXPORT(i16, bishopEndgame, PSq::materialTables[phEndgame][ptBishop]);
	TUNE_EXPORT(i16, rookEndgame,	PSq::materialTables[phEndgame][ptRook]);
	TUNE_EXPORT(i16, queenEndgame,	PSq::materialTables[phEndgame][ptQueen]);

	TUNE_EXPORT(FineScore, outpostBonusFile0, outpostBonusFile[0]);
	TUNE_EXPORT(FineScore, outpostBonusFile1, outpostBonusFile[1]);
	TUNE_EXPORT(FineScore, outpostBonusFile2, outpostBonusFile[2]);
	TUNE_EXPORT(FineScore, outpostBonusFile3, outpostBonusFile[3]);
	TUNE_EXPORT(FineScore, outpostBonusFile4, outpostBonusFile[4]);
	TUNE_EXPORT(FineScore, outpostBonusFile5, outpostBonusFile[5]);
	TUNE_EXPORT(FineScore, outpostBonusFile6, outpostBonusFile[6]);
	TUNE_EXPORT(FineScore, outpostBonusFile7, outpostBonusFile[7]);

	TUNE_EXPORT(FineScore, outpostBonusRank0, outpostBonusRank[0]);
	TUNE_EXPORT(FineScore, outpostBonusRank1, outpostBonusRank[1]);
	TUNE_EXPORT(FineScore, outpostBonusRank2, outpostBonusRank[2]);
	TUNE_EXPORT(FineScore, outpostBonusRank3, outpostBonusRank[3]);
	TUNE_EXPORT(FineScore, outpostBonusRank4, outpostBonusRank[4]);
	TUNE_EXPORT(FineScore, outpostBonusRank5, outpostBonusRank[5]);
	TUNE_EXPORT(FineScore, outpostBonusRank6, outpostBonusRank[6]);
	TUNE_EXPORT(FineScore, outpostBonusRank7, outpostBonusRank[7]);

	TUNE_EXPORT(FineScore, pawnRaceAdvantageEndgame, pawnRaceAdvantageEndgame);
}

// Eval

Eval::Eval() : occ(0), pe(0)
{
	fscore[phOpening] = fscore[phEndgame] = 0;
	safetyMask[ctWhite] = safetyMask[ctBlack] = 0;
	attackers[ctWhite] = attackers[ctBlack] = 0;
	memset( attm, 0, sizeof(attm) );
}

bool Eval::resizeEval( size_t sizeBytes )
{
	return ecache.resize( sizeBytes );
}

bool Eval::resizePawn( size_t sizeBytes )
{
	return phash.resize( sizeBytes );
}

bool Eval::resizeMaterial( size_t sizeBytes )
{
	return mhash.resize( sizeBytes );
}

// clear eval/pawn caches
void Eval::clear()
{
	ecache.clear();
	phash.clear();
	mhash.clear();
}

template< Color c > static inline bool isBareKing( const Board &b )
{
	return !(b.materialKey() & matMask[c] );
}

template< Color c > static inline bool hasMatingMaterial( const Board &b )
{
	MaterialKey mk = b.materialKey();
	if ( mk & matPotMating[c] )
		return 1;
	// check whether it's two knights
	uint knights = (uint)(mk >> MATSHIFT( c, ptKnight )) & 63;
	uint bishops = (uint)(mk >> MATSHIFT( c, ptBishop )) & 63;
	if ( knights > 2 || (knights && bishops) )
		return 1;
	if ( bishops <= 1 )
		return 0;
	Bitboard bishopMask = b.pieces( c, ptBishop );
	return (bishopMask & lightSquares) && (bishopMask & darkSquares);
}

template< Color c > bool Eval::isCertainWin( const Board &b ) const
{
	Color opc = flip(c);
	if ( !isBareKing< c^1 >(b) )
		return 0;
	// special eval
	if ( !(b.materialKey() & matRookOrBetter[c]) )
		return 0;
	// make sure the king can't capture non-pawn piece
	Bitboard kmov = attm[ opc ][ ptKing ];
	Bitboard allAttm = attm[ c ][ ptAll ];
	if ( !(kmov & ~allAttm) )
	{
		// potential stalemate
		if ( b.turn() == opc )
		{
			assert( !b.inCheck() );
			return 0;				// stalemate => we don't call eval when in check
		}
		// if we're on move and leading, assume we can do better to avoid the stalemate
	}

	if ( b.turn() == c )
		return 1;				// we have a tempo so it's ok

	Bitboard majors = b.pieces( c, ptRook ) | b.pieces( c, ptQueen );
	if ( !(allAttm & majors) )
	{
		// undefended majors
		// can the opponent capture?
		if ( kmov & majors )
			return 0;
	}
	return 1;
}

template< Color c > void Eval::evalBlindBishop( const Board &b )
{
	if ( !isBareKing<c^1>(b) )
		return;
	MaterialKey mk = b.materialKey();
	// blind bishop check (FIXME: better -- seems too complicated)
	if ( !b.pieces(c, ptBishop) || (mk & matBishopEGMask) )
		return;				// ok - no bishop eg
	Bitboard pawns = b.pieces( c, ptPawn );
	if ( !pawns )
		return;
	if ( !(pawns & ~Tables::fileMask[ HFILE ]) )
	{
		// pawn(s) on H file only
		Bitboard blindMask = c == ctWhite ? darkSquares : lightSquares;
		if ( !(blindMask & b.pieces( c, ptBishop ) ) )
		{
			// blind bishop => last thing to do: only important if king catch the pawn?
			fscore[ phEndgame ] /= 16;
		}
	}
	else if ( !(pawns & ~Tables::fileMask[ AFILE ]) )
	{
		// pawn(s) on A file only
		Bitboard blindMask = c == ctBlack ? darkSquares : lightSquares;
		if ( !(blindMask & b.pieces( c, ptBishop ) ) )
		{
			// blind bishop => last thing to do: only important if king catch the pawn?
			fscore[ phEndgame ] /= 16;
		}
	}
}

Score Eval::eval( const Board &b, Score alpha, Score beta )
{
	Score res = BitOp::hasHwPopCount() ? ieval< pcmHardware >( b, alpha, beta ) : ieval< pcmNormal >( b, alpha, beta );
	// stm bonus
	res += stmBonus;
	return res;
}

template< PopCountMode pcm > Score Eval::ieval( const Board &b, Score /*alpha*/, Score /*beta*/ )
{
	// probe eval cache first
	EvalCacheEntry *ec = ecache.index( b.sig() );
	if ( ec->sig == b.sig() )
		return ec->score;					// hit => nothing to do
	// probe pawn hash
	pe = phash.index( b.pawnSig() );

	// initialize fine scores for game phases
	fscore[ phOpening ] = ScorePack::initFine( b.deltaMat( phOpening ) );
	fscore[ phEndgame ] = ScorePack::initFine( b.deltaMat( phEndgame ) );

	occ = b.occupied();

	// init attack masks
	memset( attm, 0, sizeof(attm) );

	// init pin masks
	pinMask[ctWhite] = b.pinTemplate< ctWhite, ctBlack, 0 >( b.king(ctWhite) );
	pinMask[ctBlack] = b.pinTemplate< ctBlack, ctWhite, 0 >( b.king(ctBlack) );

	// init safety data
	for ( Color c = ctWhite; c <= ctBlack; c++ )
	{
		attackers[c] = 0;
		safetyMask[c] = Tables::kingAttm[ b.king(c) ];
	}

	if ( pe->sig == b.pawnSig() )
	{
		// pawn hash hit
		evalPawns< pcm, ctWhite, 0 >(b);
		evalPawns< pcm, ctBlack, 0 >(b);
	}
	else
	{
		pe->sig = b.pawnSig();
		pe->scores[ phOpening ] = pe->scores[ phEndgame ] = 0;
		evalPawns< pcm, ctWhite, 1 >(b);
		evalPawns< pcm, ctBlack, 1 >(b);
	}
	// apply pawn hash scores
	fscore[ phOpening ] += pe->scores[ phOpening ];
	fscore[ phEndgame ] += pe->scores[ phEndgame ];

	// passers require special handling
	evalPassers< ctWhite >(b);
	evalPassers< ctBlack >(b);

	evalKnights< pcm, ctWhite >(b);
	evalKnights< pcm, ctBlack >(b);
	evalBishops< pcm, ctWhite >(b);
	evalBishops< pcm, ctBlack >(b);
	evalRooks< pcm, ctWhite >(b);
	evalRooks< pcm, ctBlack >(b);
	evalQueens< pcm, ctWhite >(b);
	evalQueens< pcm, ctBlack >(b);
	evalKing< pcm, ctWhite >(b);
	evalKing< pcm, ctBlack >(b);

	fscore[ phEndgame ] > 0 ? evalSpecial< ctWhite >( b ) : evalSpecial< ctBlack >( b );

	// endgame recognizers
	evalRecog( b );

	// compute final score
	Score res = ScorePack::interpolate( b.nonPawnMat(), fscore[ phOpening ], fscore[ phEndgame ] );
	// adjust according to stm
	res *= sign(b.turn());

	// store to eval cache
	ec->sig = b.sig();
	ec->score = res;
	return res;
}

// returns fast evaluation (psq only)
Score Eval::fastEval( const Board &b )
{
	// initialize fine scores for game phases
	FineScore score[2] = {
		ScorePack::initFine( b.deltaMat( phOpening ) ),
		ScorePack::initFine( b.deltaMat( phEndgame ) )
	};

	// compute final score
	Score res = ScorePack::interpolate( b.nonPawnMat(), score[ phOpening ], score[ phEndgame ] );
	// adjust according to stm
	return res * sign(b.turn());
}

template< PopCountMode pcm, Color c, bool slow > void Eval::evalPawns( const Board &b )
{
	Bitboard tmp = b.pieces( c, ptPawn );
	// (fast) attack mask
	attm[c][ptAll] = attm[c][ptPawn] =
		c == ctWhite ?
			(((tmp >> 9) & R1MASK) | ((tmp >> 7) & L1MASK)) :
			(((tmp << 9) & L1MASK) | ((tmp << 7) & R1MASK))
		;
	attackers[flip(c)] += BitOp::popCount< pcm >( safetyMask[ flip(c) ] & attm[c][ptPawn] );

	// slow eval pawns
	if ( slow )
	{
		Bitboard pawns = tmp;
		pe->passers[ c ] = 0;
		while ( tmp )
		{
			Square sq = BitOp::popBit( tmp );
//			bool chained = (Tables::chainMask[ sq ] & pawns) != 0;
			bool passer = !(b.pieces( flip(c), ptPawn ) & Tables::passerMask[ c ][ sq ]);
			bool doubled = (Tables::frontMask[ c ][ sq ] & pawns) != 0;
			bool isolated = !(Tables::isoMask[ SquarePack::file(sq) ] & pawns);

			if ( isolated )
			{
				// isolated pawn
				pe->scores[ phOpening ] -= sign<c>() * isolatedPawnOpening;
				pe->scores[ phEndgame ] -= sign<c>() * isolatedPawnEndgame;
			}
			if ( doubled )
			{
				// doubled pawn
				pe->scores[ phOpening ] -= sign<c>() * doubledPawnOpening;
				pe->scores[ phEndgame ] -= sign<c>() * doubledPawnEndgame;
			}
			if ( !passer && !isolated && !doubled && !(Tables::frontMask[ c ][ sq ] & b.pieces( flip(c), ptPawn )) )
			{
				if ( BitOp::popCount< pcm >( Tables::passerMask[ c ][ sq ] & b.pieces( flip(c), ptPawn ) ) < 2 )
				{
					// candidate passer
					Rank rr = SquarePack::relRank<c>( sq ) ^ RANK1;	// important: use rank1 = 7, ... rank8 = 0
					pe->scores[ phOpening ] += sign<c>() * candPasserOpening[rr];
					pe->scores[ phEndgame ] += sign<c>() * candPasserEndgame[rr];
				}
			}
			if ( passer && !doubled )
			{
				pe->passers[c] |= BitOp::oneShl( sq );
			}
		}
	}
}

template< Color c > void Eval::evalPassers( const Board &b )
{
	Bitboard p = pe->passers[ c ];
	i32 dm = (i32)b.nonPawnMat(c) - b.nonPawnMat(flip(c));
	while ( p )
	{
		Square sq = BitOp::popBit( p );
		Bitboard fwd = BitOp::shiftForward<c>(BitOp::oneShl(sq));
		// eval passer
		// scale bonus down based on material difference
		Score scl = passerScaleImbalance[ dm >= 0 ];
		scl *= passerScaleBlocked[ !(occ & fwd) ];

		Rank rr = SquarePack::relRank<c>( sq ) ^ RANK1;	// important: use rank1 = 7, ... rank8 = 0
		fscore[ phOpening ] += sign<c>() * passerOpening[rr] * scl / 65536;
		fscore[ phEndgame ] += sign<c>() * passerEndgame[rr] * scl / 65536;
	}
}

template< PopCountMode pcm, Color c > void Eval::evalKnights( const Board &b )
{
	Bitboard tmp = b.pieces( c, ptKnight );
	Bitboard pawns = b.pieces( c, ptPawn );
	Bitboard opPawns = b.pieces( flip(c), ptPawn );
	while ( tmp )
	{
		Square sq = BitOp::popBit( tmp );
		Bitboard mob = Tables::knightAttm[ sq ];

		if ( pinMask[c] & BitOp::oneShl(sq) )
			mob = 0;

		attm[c][ptAll] |= attm[c][ptKnight] |= mob;

		// safety helper
		if ( safetyMask[ flip(c) ] & mob )
			attackers[flip(c)]++;

		mob &= ~b.occupied( c );					// exclude friendly pieces
		mob &= ~attm[flip(c)][ptPawn];				// exclude attacked squares
		int popc = BitOp::popCount< pcm >( mob );
		assert( popc >= 0 && popc < 9 );

		fscore[phOpening] += sign<c>() * knightMobility[ phOpening ][ popc ];
		fscore[phEndgame] += sign<c>() * knightMobility[ phEndgame ][ popc ];

		// knight outpost bonus
		if ( (Tables::pawnAttm[flip(c)][sq] & pawns) &&
			 !(Tables::outpostMask[c][sq] & opPawns) )
		{
			File f = SquarePack::file(sq);
			Rank r = SquarePack::relRank<c>(sq) ^ RANK1;
			FineScore bonus = outpostBonusFile[f] + outpostBonusRank[r];
			fscore[phOpening] += sign<c>() * bonus;
			fscore[phEndgame] += sign<c>() * bonus;
		}

		if ( BitOp::oneShl( sq ) & attm[flip(c)][ptPawn] )
		{
			fscore[phOpening] -= sign<c>() * knightHangingOpening;
			fscore[phEndgame] -= sign<c>() * knightHangingEndgame;
		}
	}
}

template< PopCountMode pcm, Color c > void Eval::evalBishops( const Board &b )
{
	Bitboard tmp = b.pieces( c, ptBishop );

	bool pair = 0;
	if ( (tmp & lightSquares) && (tmp & darkSquares) )
	{
		// bishop pair bonus
		fscore[ phOpening ] += sign<c>() * bishopPairOpening;
		fscore[ phEndgame ] += sign<c>() * bishopPairEndgame;
		pair = 1;
	}

	Bitboard tocc = occ & ~(tmp|b.pieces(c, ptQueen));

	Bitboard oppawns = b.pieces( c, ptPawn );

	while ( tmp )
	{
		Square sq = BitOp::popBit( tmp );

		if ( !pair )
		{
			// eval good vs bad bishop
			Bitboard badMask;
			badMask = (BitOp::oneShl(sq) & lightSquares) ? lightSquares : darkSquares;
			Bitboard goodMask = ~badMask;

			int delta = (int)BitOp::popCount<pcm>( oppawns & goodMask ) - (int)BitOp::popCount<pcm>( oppawns & badMask );
			i32 didx = delta + 8;
			didx = std::max( std::min(didx, 16), 0 );
			fscore[ phOpening ] += sign<c>() * goodBishopOpening[didx];
			fscore[ phEndgame ] += sign<c>() * goodBishopEndgame[didx];
		}

		Bitboard mob = Magic::bishopAttm( sq, tocc );

		// determine valid movement mask if pinned
		if ( pinMask[c] & BitOp::oneShl(sq) )
			mob &= Tables::ray[ b.king(c) ][ sq ];

		attm[c][ptAll] |= attm[c][ptBishop] |= mob;

		// safety helper
		if ( safetyMask[ flip(c) ] & mob )
			attackers[flip(c)]++;

		mob &= ~b.occupied( c );					// exclude friendly pieces
		mob &= ~attm[flip(c)][ptPawn];				// exclude attacked squares
		int popc = (int)BitOp::popCount< pcm >( mob );
		assert( popc >= 0 && popc < 14 );

		fscore[phOpening] += sign<c>() * bishopMobility[ phOpening ][ popc ];
		fscore[phEndgame] += sign<c>() * bishopMobility[ phEndgame ][ popc ];

		if ( SquarePack::relRank<c>( sq ) == RANK7 )
		{
			File f = SquarePack::file(sq);
			if ( f == AFILE || f == HFILE )
			{
				Square blocksq = SquarePack::advanceRank< c, -1 >(sq);
				if ( f == AFILE )
					blocksq++;
				else
					blocksq--;
				if ( BitOp::oneShl( blocksq ) & b.pieces( flip(c), ptPawn ) )
				{
					// trapped by pawn
					// confirm with see
					if ( b.see<1>( MovePack::initCapture( sq, blocksq ) ) < 0 )
					{
						fscore[phOpening] -= sign<c>()*trappedBishopOpening;
						fscore[phEndgame] -= sign<c>()*trappedBishopEndgame;
					}
				}
			}
		}
		if ( BitOp::oneShl( sq ) & attm[flip(c)][ptPawn] )
		{
			fscore[phOpening] -= sign<c>() * bishopHangingOpening;
			fscore[phEndgame] -= sign<c>() * bishopHangingEndgame;
		}
	}
}

template< PopCountMode pcm, Color c > void Eval::evalRooks( const Board &b )
{
	Bitboard tmp = b.pieces( c, ptRook );
	Bitboard myrooks = tmp;
	Bitboard pawns = b.pieces( ctWhite, ptPawn ) | b.pieces( ctBlack, ptPawn );
	Bitboard tocc = occ & ~(myrooks|b.pieces(c, ptQueen));
	while ( tmp )
	{
		Square sq = BitOp::popBit( tmp );
		Bitboard mob = Magic::rookAttm( sq, tocc );

		// determine valid movement mask if pinned
		if ( pinMask[c] & BitOp::oneShl(sq) )
			mob &= Tables::ray[ b.king(c) ][ sq ];

		attm[c][ptAll] |= attm[c][ptRook] |= mob;

		// safety helper
		if ( safetyMask[ flip(c) ] & mob )
			attackers[flip(c)]++;

		mob &= ~b.occupied( c );					// exclude friendly pieces
		// exclude attacked squares
		mob &= ~(attm[flip(c)][ptPawn] | attm[flip(c)][ptKnight] | attm[flip(c)][ptBishop]);
		int popc = (int)BitOp::popCount< pcm >( mob );
		assert( popc >= 0 && popc < 15 );

		fscore[phOpening] += sign<c>() * rookMobility[ phOpening ][ popc ];
		fscore[phEndgame] += sign<c>() * rookMobility[ phEndgame ][ popc ];

		// now rook on open file
		File f = SquarePack::file( sq );
		if ( !(Tables::fileMask[ f ] & pawns) && !(Tables::frontMask[c][sq] & myrooks) )
		{
			// if two rooks are here, give bonus to only one
			fscore[phOpening] += sign<c>() * rookOnOpenOpening;
			fscore[phEndgame] += sign<c>() * rookOnOpenEndgame;
		}
		if ( BitOp::oneShl( sq ) & attm[flip(c)][ptPawn] )
		{
			fscore[phOpening] -= sign<c>() * rookHangingOpening;
			fscore[phEndgame] -= sign<c>() * rookHangingEndgame;
		}
	}
}

template< PopCountMode pcm, Color c > void Eval::evalQueens( const Board &b )
{
	Bitboard tmp = b.pieces( c, ptQueen );
	Bitboard tocc = occ & ~tmp;
	Bitboard rooks = b.pieces(c, ptRook);
	Bitboard bishops = b.pieces(c, ptBishop);
	while ( tmp )
	{
		Square sq = BitOp::popBit( tmp );
		Bitboard incl = Tables::orthoAttm[sq] & rooks;
		incl |= Tables::diagAttm[sq] & bishops;
		Bitboard mob = Magic::queenAttm( sq, tocc & ~incl );

		// determine valid movement mask if pinned
		if ( pinMask[c] & BitOp::oneShl(sq) )
			mob &= Tables::ray[ b.king(c) ][ sq ];

		attm[c][ptAll] |= attm[c][ptQueen] |= mob;

		// safety helper
		if ( safetyMask[ flip(c) ] & mob )
			attackers[flip(c)]++;

		mob &= ~b.occupied( c );					// exclude friendly pieces
		// exclude attacked squares
		mob &= ~(attm[flip(c)][ptPawn] | attm[flip(c)][ptKnight] | attm[flip(c)][ptBishop] | attm[flip(c)][ptRook]);
		int popc = (int)BitOp::popCount< pcm >( mob );
		assert( popc >= 0 && popc < 28 );

		fscore[phOpening] += sign<c>() * queenMobility[ phOpening ][ popc ];
		fscore[phEndgame] += sign<c>() * queenMobility[ phEndgame ][ popc ];

		if ( BitOp::oneShl( sq ) & attm[flip(c)][ptPawn] )
		{
			fscore[phOpening] -= sign<c>() * queenHangingOpening;
			fscore[phEndgame] -= sign<c>() * queenHangingEndgame;
		}
	}
}

template< PopCountMode pcm, Color c > void Eval::evalKing( const Board &b )
{
	Square kp = b.king(c);

	// FIXME: better?
	attm[c][ptKing] = Tables::kingAttm[kp];
	attm[c][ptAll] |= attm[c][ptKing];

	// pawn shelter eval
	Bitboard t = BitOp::oneShl(kp);
	t |= ((t << 1) & L1MASK) | ((t >> 1) & R1MASK);
	Bitboard tmp = t;
	t |= (c == ctWhite ? (t >> 8) : (t << 8));
	Bitboard pawns = b.pieces( c, ptPawn );
	uint front1 = BitOp::popCount< pcm >( t & pawns );
	fscore[ phOpening ] += sign<c>() * (FineScore)(front1 * shelterFront1);
	tmp = (c == ctWhite) ? tmp >> 16 : tmp << 16;
	uint front2 = BitOp::popCount< pcm >( tmp & pawns );
	fscore[ phOpening ] += sign<c>() * (FineScore)(front2 * shelterFront2);

	FineScore safety = 0;
	for ( Piece p = ptPawn; p <= ptQueen; p++ )
	{
		Bitboard threats = attm[ flip(c) ][ p ] & safetyMask[c];
		safety += BitOp::popCount< pcm >( threats ) * safetyScale[ p ];
	}

	if ( b.pieces( flip(c), ptQueen ) )
		safety *= 2;

	safety *= attackers[ c ];
	fscore[phOpening] -= sign<c>() * safety;

	// endgame: bonus/penalty for being close/away from passers
	Bitboard pass = pe->passers[c];
//	uint npass = 0;
	u8 bdist = 255;
	while ( pass )
	{
//		npass++;
		Square psq = BitOp::popBit(pass);
		u8 dist = Tables::distance[kp][psq];
		if ( dist < bdist )
			bdist = dist;
	}
	if ( bdist != 255 )
	{
		int dist = kingPasserSupportBase-(int)bdist;
		fscore[phEndgame] += sign<c>() * dist * kingPasserSupportScale;
/*		// add more bonus to passers facing a knight who is a weak defender
		if ( npass > 1 && (b.materialKey() & matNPMask[ flip(c)]) == matKN[ flip(c)] )
		{
			fscore[phEndgame] += sign<c>() * 1000;
		}*/
	}
}

// eval special endgame cases
template< Color c > void Eval::evalSpecial( const Board &b )
{
	// check whether the side which has advantage has enough mating material
	// or whether the other side is bare king and we have a certain win
	if ( !hasMatingMaterial< c >( b ) )
		fscore[ phOpening ] = fscore[ phEndgame ] = 0;
	else if ( isCertainWin< c >( b ) )
		fscore[ phEndgame ] += sign<c>() * certainWin;
	else evalBlindBishop< c >( b );
}

void Eval::execRecog( const Board &b, MaterialKey mk, const Recognizer *recogs, size_t numRecogs )
{
	i32 l = 0;
	i32 h = (i32)numRecogs;
	while ( l <= h )
	{
		i32 m = (l+h) >> 1;
		const Recognizer &r = recogs[m];
		if ( mk == r.key )
		{
			// found!
			r.func( b, fscore );
			return;
		}
		if ( mk < r.key )
			h = m-1;
		else
			l = m+1;
	}
}

// eval eg recognizers
void Eval::evalRecog( const Board &b )
{
	MaterialKey mk = b.materialKey();

	if ( !b.nonPawnMat() )
	{
		// eval difference (Ed's good old idea)
		int dpawn = MaterialPack::count( mk, ctWhite, ptPawn) - MaterialPack::count( mk, ctBlack, ptPawn);
		dpawn = std::max( -1, std::min( dpawn, 1 ) );
		fscore[ phEndgame ] += pawnRaceAdvantageEndgame * dpawn;
		// eval unstoppable passers
		for ( Color c = ctWhite; c <= ctBlack; c++ )
		{
			Square okp = b.king( flip(c) );
			Bitboard p = pe->passers[c];
			while ( p )
			{
				Square sq = BitOp::popBit(p);
				Rank rr = SquarePack::relRank(c, sq) ^ RANK1;	// important: use rank1 = 7, ... rank8 = 0
				Distance pdist = (Distance)8 - (Distance)rr;
				Distance dist = Tables::distance[okp][sq];
				if ( b.turn() == c )
					dist++;
				if ( pdist < dist )
					fscore[ phEndgame ] += sign(c) * unstoppablePasser;
			}
		}
	}

	// scale bishop endgame with opposite color bishops
	if ( !(mk & matBishopEGMask) )
	{
		Bitboard bishops[2] = {
			b.pieces( ctWhite, ptBishop ),
			b.pieces( ctBlack, ptBishop )
		};

		if ( bishops[0] && bishops[1] && abs(b.deltaMat(phOpening)) < 400 )
		{
			if ( ( !(bishops[0] & lightSquares) && !(bishops[1] & darkSquares ) ) ||
				 ( !(bishops[0] & darkSquares) && !(bishops[1] & lightSquares ) ) )
			{
				fscore[ phOpening ] /= 2;
				fscore[ phEndgame ] /= 2;
			}
		}
	}
	// eval other less trivial drawish positions
	execRecog( b, mk & matMask[ctWhite], scaleRecognizers[ctWhite], numScaleRecognizers );
	execRecog( b, mk & matMask[ctBlack], scaleRecognizers[ctBlack], numScaleRecognizers );

	// exec true recognizers
	execRecog( b, mk, recognizers, numRecognizers );
}

}
