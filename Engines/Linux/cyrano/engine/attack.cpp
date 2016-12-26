
//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-
//
//	Gerbil
//
//	Copyright (c) 2001, Bruce Moreland.  All rights reserved.
//
//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-
//
//	This file is part of the Gerbil chess program project.
//
//	Gerbil is free software; you can redistribute it and/or modify it under
//	the terms of the GNU General Public License as published by the Free
//	Software Foundation; either version 2 of the License, or (at your option)
//	any later version.
//
//	Gerbil is distributed in the hope that it will be useful, but WITHOUT ANY
//	WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
//	FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
//	details.
//
//	You should have received a copy of the GNU General Public License along
//	with Gerbil; if not, write to the Free Software Foundation, Inc.,
//	59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//
//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-

#include "engine.hpp"

#ifdef	DEBUG
static char const s_aszModule[] = __FILE__;
#endif

//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-
#if 0
//	These are initialized once at boot.  After that they are constant.

static	unsigned c_argbfRay[256];
static	int	c_argdStep[256];

//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-

//	A few defines and some constant data whose usage shouldn't be difficult
//	to figure out.

#define	bfNONE		0x00
#define	bfWPAWN		0x01
#define	bfBPAWN		0x02
#define	bfKNIGHT	0x04
#define	bfBISHOP	0x08
#define	bfROOK		0x10
#define	bfQUEEN		0x20
#define	bfKING		0x40

static	unsigned const c_argbfPc[pcMAX][coMAX] = {
	bfWPAWN,	bfBPAWN,
	bfKNIGHT,	bfKNIGHT,
	bfBISHOP,	bfBISHOP,
	bfROOK,		bfROOK,
	bfQUEEN,	bfQUEEN,
	bfKING,		bfKING,
};

static bool const c_argfSliding[] = {	// A "sliding" piece can traverse a ray.
	false,	// Pawn.
	false,	// Knight.
	true,	// Bishop.
	true,	// Rook.
	true,	// Queen.
	false,	// King.
};

//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-

//	These two functions initialize the two arrays that are used by the
//	attack detection code.  "VInitAttack" is called once at boot.

static void VRay(int cIter, int dDelta, unsigned bf)
{
	int	isq = 0;

	for (int i = 0; i < cIter; i++) {
		isq += dDelta;
		Assert((isq + 128 >= 0) && (isq + 128 < 256));
		c_argbfRay[isq + 128] |= bf;
		c_argdStep[isq + 128] = dDelta;
	}
}

void VInitAttackData(void)
{
	for (int co = coWHITE; co <= coBLACK; co++) {
		
		for (int i = 0; i < 2; i++)	// Pawn left-attack and right-attack.
			VRay(1, s_argpvecPawn[co][i], c_argbfPc[pcPAWN][co]);
	}
	for (int pc = pcKNIGHT; pc <= pcKING; pc++) {
		int cIter = c_argfSliding[pc] ? 7 : 1;
		for (int const *pvec = s_argpvecPiece[pc]; *pvec; pvec++)
			VRay(cIter, *pvec, c_argbfPc[pc][coWHITE]);
	}
}

//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-

//	This determines if "isqDef" is attacked by side "coAtk".
//
//	1. For each	non-dead piece, I will see if it could conceivably attack the
//	target square.  A piece can attack a square if one of its rays intersects
//	the square.  The 128-element board makes this very easy.  I can take a
//	square delta between the attacker and defender, and simply look up in a
//	table ("c_argbfRay") to see if a piece of given type has a ray that
//	intersects.
//
//	With a 64-element board, this will not work, because the delta between h1
//	and a2 is the same as the delta between d3 and e3.  With a 128-element
//	board, every delta defines a unique relationship between squares.
//
//	2. Once I know that a piece has a ray that intersects, if the piece is not
//	a sliding piece, I am done.  Non-sliding pieces are pawns, knights, and
//	kings (a king can slide while castling, but that's not a capture).
//
//	3. In the case of sliding pieces (bishop, rook, queen) that have a ray
//	that intersects the target square, I look up a step value ("dStep") in
//	another table ("c_argdStep"), and just walk the appropriate ray until I
//	either hit an interposed piece or arrive at the destination square.
//
//	This is an efficient technique because in most cases you know right away
//	that an attack can't exist.  In the cases where you know that one might
//	exist, it is easy to figure out if one does.

bool FAttacked(PCON pcon, int isqDef, int coAtk)
{
	for (int ipi = 0; ipi < pcon->argcpi[coAtk]; ipi++) {
		if (pcon->argpi[coAtk][ipi].fDead)
			continue;
		int isqAtk = pcon->argpi[coAtk][ipi].isq;
		int pcAtk = pcon->argpi[coAtk][ipi].pc;
		if (c_argbfRay[isqDef - isqAtk + 128] & c_argbfPc[pcAtk][coAtk]) {
			if (!c_argfSliding[pcAtk])
				return true;
			int dStep = c_argdStep[isqDef - isqAtk + 128];
			Assert(dStep != 0);	// Corrupt data check for impossible delta.
			for (;;) {
				isqAtk += dStep;
				Assert(!(isqAtk & 0x88)); // Corrupt data check for off-board.
				if (isqAtk == isqDef)
					return true;
				if (pcon->argsq[isqAtk].ppi != NULL)
					break;
			}
		}
	}
	return false;
}

#endif
