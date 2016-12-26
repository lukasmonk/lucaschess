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

#include "kpk.h"
#include "tables.h"
#include "magic.h"
#include <memory.h>

#include "board.h"
#include "search.h"
#include "movegen.h"
#include "thread.h"

#ifdef GEN_KPK
#	include <cstdio>
#endif

// TODO: refactor KPK::generate (split into more parts)

namespace cheng4
{

// KPK

u8 KPK::bits[64*64*24*2/8] =
{
#include "kpktable.h"
};

u8 KPK::getBit( uint b )
{
	return (u8)( bits[ b >> 3 ] & (1u << (b&7)) );
}

void KPK::setBit( uint b, uint v )
{
	u8 &val = bits[ b >> 3 ];
	u8 msk = (u8)(1u << (b&7));
	val &= ~msk;
	val |= msk * (v != 0);
}

void KPK::init()
{
}

enum KPKStatus
{
	stUnknown,
	stInvalid,
	stWin,				// win for side with pawn
	stDraw
};

void KPK::generate()
{
#ifdef GEN_KPK
	i32 ms = Timer::getMillisec();

	memset( bits, 0, sizeof(bits) );
	u8 status[ 64*64*24*2 ];
	// status: 0 = unknown, 1 = invalid, 2 = win, 4 = draw
	memset( status, 0, sizeof(status) );
	// 64*64*24
	// king pos, bking pos (all) + half-side pawn positions
	for ( Square kp = 0; kp < 64; kp++ )
	{
		for ( Square bkp = 0; bkp < 64; bkp++ )
		{
			for ( uint p = 0; p < 24; p++ )
			{
				// build left-wing pawn square
				Square psq = (Square)( 8+(p/4)*8 + (p & 3) );
				for ( Color c = ctWhite; c <= ctBlack; c++ )
				{
					KPKStatus stat = stUnknown;
					if ( kp == bkp || kp == psq || bkp == psq )
						stat = stInvalid;
					uint ind = index( kp, bkp, psq, c );

					// classify trivial wins and stalemates
					// if black (from now on side without pawn) can safely capture the pawn, it's a draw
					Bitboard bkatt = Tables::kingAttm[ bkp ];
					Bitboard katt = Tables::kingAttm[ kp ];
					if ( katt & BitOp::oneShl( bkp ) )
						stat = stInvalid;				// kings too close => invalid

					if ( stat == stUnknown )
					{
						bkatt &= ~katt;					// can't approach white king
						Bitboard pbb = BitOp::oneShl( psq );
						if ( c == ctBlack )
						{
							if ( (bkatt & pbb) && !(katt & pbb) )
								stat = stDraw;		// black safely recaptures => draw
							Bitboard attm = Tables::pawnAttm[ ctWhite ][ psq ];
							bkatt &= ~attm;
							// FIXME: it can never happen that kp vs k is a stalemate nor that kp vs k is a checkmate
							if ( !bkatt && !(BitOp::oneShl(bkp) & attm ) )
								stat = stDraw;		// draw (stalemate)
						}
						else
						{
							if ( Tables::pawnAttm[ ctWhite ][ psq ] & BitOp::oneShl( bkp ) )
								stat = stInvalid;		// we are on move but black king is in check => invalid position
							// now check for safe win
							if ( stat == stUnknown && SquarePack::rank(psq) == RANK7 )
							{
								Square promoSq = SquarePack::advanceRank< ctWhite, 1 >( psq );
								if ( promoSq != kp && promoSq != bkp )
								{
									// promotion square clear
									Bitboard promoBb = BitOp::oneShl( promoSq );
									if ( !(bkatt & promoBb) || (katt & promoBb) )
									{
										// black can't recapture or promosquare is protected by white king
										Bitboard occ =
											BitOp::oneShl( kp ) | BitOp::oneShl( bkp ) | BitOp::oneShl( promoSq );
										// it's only certain win if it's not stalemate
										Bitboard qattm = Magic::queenAttm( promoSq, occ );
										// if it's not stalemate or it's a check => win
										if ( (bkatt & ~qattm) || (qattm & BitOp::oneShl( bkp ) ) )
											stat = stWin;
										else
										{
											// check for safe rook promotion
											Bitboard rattm = Magic::rookAttm( promoSq, occ );
											if ( bkatt & ~rattm )
												stat = stWin;
										}
									}
								}
							}
						}
					}
					status[ ind ] = (u8)stat;
				}
			}
		}
	}

	// ready to iterate
	while( iterate( status ) );

	// the rest of unclassified positions are draws
	for (uint i=0; i<64*64*24*2; i++)
		setBit( i, status[i] == stWin ? 0 : 1 );

	ms = Timer::getMillisec() - ms;
	printf("kpk init took %d ms\n", ms);

	FILE *f = fopen("kpktable.h", "w");
	for (size_t i=0; i<sizeof(bits); )
	{
		fprintf(f, "%d", (int)bits[i++]);
		if ( i < sizeof(bits) )
			fprintf(f, ",");
		if ( !(i & 7) )
			fprintf(f, "\n");
	}
	fclose(f);
#endif
}

bool KPK::iterate( u8 *status )
{
	bool res = 0;
	(void)status;
#ifdef GEN_KPK
	for ( Square kp = 0; kp < 64; kp++ )
	{
		for ( Square bkp = 0; bkp < 64; bkp++ )
		{
			for ( uint p = 0; p < 24; p++ )
			{
				// build left-wing pawn square
				Square psq = (Square)( 8+(p/4)*8 + (p & 3) );

				for ( Color c = ctWhite; c <= ctBlack; c++ )
				{
					uint ind = index( kp, bkp, psq, c );
					if ( status[ind] != stUnknown )
						continue;							// already classified => skip
					Bitboard bkattm = Tables::kingAttm[ bkp ];
					Bitboard kattm = Tables::kingAttm[ kp ];
					assert( !(bkattm & BitOp::oneShl( bkp )) );
					// generate all moves
					if ( c == ctWhite )
					{
						kattm &= ~bkattm;
						kattm &= BitOp::noneShl( psq );		// can't capture own pawn
						uint draws = 0, count = 0;
						// king moves
						while ( kattm )
						{
							count++;
							Square nkp = BitOp::popBit( kattm );
							assert( nkp != kp );
							uint nind = index( nkp, bkp, psq, flip(c) );
							// if any move leads to a win, it's a win
							u8 nst = status[nind];
							if ( nst == stWin )
							{
								status[ind] = stWin;
								res = 1;
								break;
							} else if ( nst == stDraw )
								draws++;
						}

						// now pawn pushes
						Bitboard occ = BitOp::oneShl( kp ) | BitOp::oneShl( bkp );
						Rank r = SquarePack::rank( psq );
						Square push1 = SquarePack::advanceRank< ctWhite, 1 >( psq );
						if ( !(BitOp::oneShl(push1) & occ) )
						{
							// can push by one
							count++;
							if ( !SquarePack::isRank1Or8(push1) )
							{
								uint nind = index( kp, bkp, push1, flip(c) );
								// if any move leads to a win, it's a win
								u8 nst = status[nind];
								if ( nst == stWin )
								{
									status[ind] = stWin;
									res = 1;
								} else if ( nst == stDraw )
									draws++;
								if ( r == RANK2 )
								{
									// pawn push by two
									Square push2 = SquarePack::advanceRank< ctWhite, 1 >( push1 );
									if ( !(BitOp::oneShl(push2) & occ) )
									{
										// can push by two
										assert( !SquarePack::isRank1Or8(push2) );
										count++;
										nind = index( kp, bkp, push2, flip(c) );
										// if any move leads to a win, it's a win
										nst = status[nind];
										if ( nst == stWin )
										{
											status[ind] = stWin;
											res = 1;
										} else if ( nst == stDraw )
											draws++;
									}
								}
							}
						}

						if ( !count || draws == count )
						{
							// self-stalemate or every move leads to a draw => it's a draw
							assert( status[ind] == stUnknown );
							status[ind] = stDraw;
							res = 1;
						}
					}
					else
					{
						bkattm &= ~kattm;
						bkattm &= ~Tables::pawnAttm[ ctWhite ][ psq ];
						// it cannot be a checkmate/stalemate at this moment
						if ( !bkattm )
						{
							// FIXME: I think this won't happen
							status[ind] = stDraw;
							res = 1;
							continue;
						}
						// generate all moves
						uint losses = 0, count = 0;
						while ( bkattm )
						{
							count++;
							Square nbkp = BitOp::popBit( bkattm );
							assert( bkp != nbkp );
							uint nind = index( kp, nbkp, psq, flip(c) );
							// if any move leads to a draw, it's a draw
							u8 nst = status[nind];
							if ( nst == stDraw )
							{
								status[ind] = stDraw;
								res = 1;
								break;
							} else if ( nst == stWin )
								losses++;
						}
						if ( losses == count )
						{
							// every move leads to a loss => it's a loss
							status[ind] = stWin;
							res = 1;
						}
					}
				}
			}
		}
	}
#endif
	return res;
}

static const Square xorMskFile[2] = { 0, 7 };

// stm: white = king with pawn
uint KPK::index( Square kp, Square bkp, Square psq, Color stm )
{
	assert( !SquarePack::isRank1Or8( psq ) );
	File f = SquarePack::file( psq );
	// mirror horizontally if necessary
	Square xm = xorMskFile[ f >= EFILE ];
	kp ^= xm;
	bkp ^= xm;
	psq ^= xm;
	f ^= (File)xm;
	// now we can build index
	uint pp = (( (psq & 0x38)-8)>>1)|f;
	assert( pp < 24 );
	return (uint)kp | ((uint)bkp << 6) | ((uint)stm << 12) | ((uint)pp << 13);
}

static const Square xorMsk[2] = { 0, 0x38 };

// is draw?
// color (with pawn), color (stm), king (with pawn) position, bare king position, pawn position
u8 KPK::isDraw( Color c, Color stm, Square kp, Square bkp, Square pp )
{
	Square xm = xorMsk[ c ];
	return getBit( index( kp^xm, bkp^xm, pp^xm, c^stm ) );
}

}
