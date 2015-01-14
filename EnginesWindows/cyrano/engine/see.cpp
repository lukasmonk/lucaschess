//
// Cyrano Chess engine
//
// Copyright (C) 2007  Harald JOHNSEN
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License as
// published by the Free Software Foundation; either version 2 of the
// License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA
//
//

#include "magicmoves.h"
#include "engine.hpp"
#include "moves.hpp"
#include "search.hpp"
#include "eval.hpp"
#include "genmagic.hpp"

#ifdef	_DEBUG
static char const s_aszModule[] = __FILE__;
#endif


// this is inspired from Glaurung see (one of the simplest see code)
// Thanks Tord !

// same as the other table except that king has a value here
static const int seeExchangePieceValue[16] = {
	0,  // king and empty square
	0, //2000,
	valQUEEN,
	valQUEEN,
	valROOK,
	valROOK,
	valBISHOP,
	valBISHOP,
	valBISHOP,
	valBISHOP,
	valPAWN,
	valPAWN,
	valPAWN,    // EP
	valPAWN,    // EP
    0,
    0
};

#define seeValues seeExchangePieceValue


int see(PCON pcon, const STE *pste, int from, int to) {

    bitboard attackers, b;

    // Initialize colors:
    int side = pste->side;
    int xside = side ^ 1;

    // Initialize pieces:
    int piece = pcon->pos[ from ];
    int capture = pcon->pos[ to ];

    U64 from_64 = IdxToU64(from);

    // clear the 'from' square so AttacksTo() can give sliders behind the capturing piece
    pcon->pieces[OCCU] &= ~from_64;
    bitboard occ = pcon->pieces[OCCU];

    // find all attackers (both side)
    attackers = AttacksTo(pcon, to );

    // remove the capturing piece (was included in AttacksTo())
    attackers &= occ;

    // the opponent has no attacker
    if( (attackers & pcon->pieces[B_OCCU | xside ]) == 0 ) {
        pcon->pieces[OCCU] |= from_64;
        return seeValues[ capture ];
    }


  // The destination square is defended, which makes things rather more
  // difficult to compute.  We proceed by building up a "swap list" containing
  // the material gain or loss at each stop in a sequence of captures to the
  // destianation square, where the sides alternately capture, and always
  // capture with the least valuable piece.  After each capture, we look for
  // new X-ray attacks from behind the capturing piece.
  int lastCapturingPieceValue = seeValues[piece];
  int swapList[32], n = 1;
  int  c = xside;
  int pt;

  swapList[0] = seeValues[capture];

  do {
    // Locate the least valuable attacker for the side to move.  The loop
    // below looks like it is potentially infinite, but it isn't.  We know
    // that the side to move still has at least one attacker left.
    for(pt = PAWN; (attackers & pcon->pieces[pt | c]) == 0; pt -= 2)
      Assert(pt > (KING|1));

    // Remove the attacker we just found from the 'attackers' bitboard,
    // and scan for new X-ray attacks behind the attacker:
    b = attackers & pcon->pieces[pt | c];
    occ ^= (b & -((long long)b));

    // add new attackers (sliding only)
    attackers |=
		(Bmagic(to, occ) & 
        (pcon->pieces[WB] | pcon->pieces[BB] | pcon->pieces[WQ] | pcon->pieces[BQ]))
		| 
        (Rmagic(to, occ) & 
		(pcon->pieces[WR] | pcon->pieces[BR] | pcon->pieces[WQ] | pcon->pieces[BQ]));

    // adjust because some pieces are allready gone
    attackers &= occ;

    // Add the new entry to the swap list:
    Assert(n < 32);
    swapList[n] = -swapList[n - 1] + lastCapturingPieceValue;
    n++;

    // Remember the value of the capturing piece, and change the side to move
    // before beginning the next iteration:
    lastCapturingPieceValue = seeValues[pt];
    c = c ^ 1;

    // Stop after a king capture:
    if((pt|1) == (KING|1) && ((attackers & pcon->pieces[B_OCCU | c]) != 0)) {
      Assert(n < 32);
      swapList[n++] = 10000;
      break;
    }
  } while((attackers & pcon->pieces[B_OCCU | c]) != 0);

    // Having built the swap list, we negamax through it to find the best
    // achievable score from the point of view of the side to move:
    while(--n) {
        if( -swapList[n] < swapList[n-1] )
            swapList[n-1] = -swapList[n];
    }

    // restore the piece on board bitmap
    pcon->pieces[OCCU] |= from_64;

    return swapList[0];
}

// for checks in QS
// A) move is allready played
// B) side is reversed
int see(const CON *pcon, const STE *pste, int to) {
  bitboard b;

  // Initialize colors: caution inverted colors here
  int xside = pste->side;
//  int side = xside ^ 1;

  // Initialize pieces:
  int piece = pcon->pos[ to ];

  // find all attackers (both side)
  bitboard attackers = AttacksTo(pcon, to );

    // the opponent has no attacker
    if( (attackers & pcon->pieces[B_OCCU | xside ]) == 0 ) {
        return 0;
    }

  bitboard occ = pcon->pieces[OCCU];


  // The destination square is defended, which makes things rather more
  // difficult to compute.  We proceed by building up a "swap list" containing
  // the material gain or loss at each stop in a sequence of captures to the
  // destianation square, where the sides alternately capture, and always
  // capture with the least valuable piece.  After each capture, we look for
  // new X-ray attacks from behind the capturing piece.
  int lastCapturingPieceValue = seeValues[piece];
  int swapList[32], n = 1;
  int  c = xside;
  int pt;

  swapList[0] = 0;

  do {
    // Locate the least valuable attacker for the side to move.  The loop
    // below looks like it is potentially infinite, but it isn't.  We know
    // that the side to move still has at least one attacker left.
    for(pt = PAWN; (attackers & pcon->pieces[pt | c]) == 0; pt -= 2)
      Assert(pt > (KING|1));

    // Remove the attacker we just found from the 'attackers' bitboard,
    // and scan for new X-ray attacks behind the attacker:
    b = attackers & pcon->pieces[pt | c];
    occ ^= (b & -((long long)b));

    // add new attackers (sliding only)
    attackers |=
		(Bmagic(to, occ) & 
        (pcon->pieces[WB] | pcon->pieces[BB] | pcon->pieces[WQ] | pcon->pieces[BQ]))
		| 
        (Rmagic(to, occ) & 
		(pcon->pieces[WR] | pcon->pieces[BR] | pcon->pieces[WQ] | pcon->pieces[BQ]));

    // adjust because some pieces are allready gone
    attackers &= occ;

    // Add the new entry to the swap list:
    Assert(n < 32);
    swapList[n] = -swapList[n - 1] + lastCapturingPieceValue;
    n++;

    // Remember the value of the capturing piece, and change the side to move
    // before beginning the next iteration:
    lastCapturingPieceValue = seeValues[pt];
    c = c ^ 1;

    // Stop after a king capture:
    if((pt|1) == (KING|1) && ((attackers & pcon->pieces[B_OCCU | c]) != 0)) {
      Assert(n < 32);
      swapList[n++] = 10000;
      break;
    }
  } while((attackers & pcon->pieces[B_OCCU | c]) != 0);

  // Having built the swap list, we negamax through it to find the best
  // achievable score from the point of view of the side to move:
  while(--n) {
      if( -swapList[n] < swapList[n-1] )
            swapList[n-1] = -swapList[n];
  }

  return swapList[0];
}
