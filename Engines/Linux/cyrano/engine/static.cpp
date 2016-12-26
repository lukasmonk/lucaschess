
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
#include "deprec.hpp"

/*char const s_argbPc[coMAX][pcMAX] = {
	'P',	'N',	'B',	'R',	'Q',	'K',
	'p',	'n',	'b',	'r',	'q',	'k',
};
*/
/*
const int KING		= 0;
const int QUEEN		= 2;
const int ROOK		= 4;
const int BISHOP	= 6;
const int KNIGHT	= 8;
const int PAWN		= 10;
*/
int	const b_valPc[16] = {
	0,          // 0 here so I don't have to test if Capt(move) == 0
	0,
	valQUEEN,
	valQUEEN,
	valROOK,
	valROOK,
	valBISHOP,
	valBISHOP,
	valKNIGHT,
	valKNIGHT,
	valPAWN,
	valPAWN,
	valPAWN,    // EP
	valPAWN,    // EP
    0,          // PROM
    0           // PROM
};
// for setboard
int	const s_argvalPc[] = {
	valPAWN,
	valKNIGHT,
	valBISHOP,
	valROOK,
	valQUEEN,
	valKING,
};

int	const s_argvalPcOnly[] = {
	0,
	valKNIGHT,
	valBISHOP,
	valROOK,
	valQUEEN,
	0,
};

int	const s_argvalPnOnly[] = {
	valPAWN,
	0,
	0,
	0,
	0,
	0,
};

char const * s_argszCo[] = { "Black", "White" };

char const s_aszFenDefault[] =
	"rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";

char const * c_argszNoYes[] = { "No", "Yes" };

//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-

//	I have 64-bit maps, but my "isq" value is 0..127.  That's bad, but I can
//	easily map it by using this array, which converts from a 7-bit isq to a
//	6-bit isq.

int const c_argisq64[128] = {
	0,	1,	2,	3,	4,	5,	6,	7,	0,	0,	0,	0,	0,	0,	0,	0,
	8,	9,	10,	11,	12,	13,	14,	15,	0,	0,	0,	0,	0,	0,	0,	0,
	16,	17,	18,	19,	20,	21,	22,	23,	0,	0,	0,	0,	0,	0,	0,	0,
	24,	25,	26,	27,	28,	29,	30,	31,	0,	0,	0,	0,	0,	0,	0,	0,
	32,	33,	34,	35,	36,	37,	38,	39,	0,	0,	0,	0,	0,	0,	0,	0,
	40,	41,	42,	43,	44,	45,	46,	47,	0,	0,	0,	0,	0,	0,	0,	0,
	48,	49,	50,	51,	52,	53,	54,	55,	0,	0,	0,	0,	0,	0,	0,	0,
	56,	57,	58,	59,	60,	61,	62,	63,	0,	0,	0,	0,	0,	0,	0,	0,
};

