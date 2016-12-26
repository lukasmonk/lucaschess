//
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

#ifndef _BOARD_HXX
#define _BOARD_HXX

//	I have a function that converts from a CM to a record that allows for
//	display in SAN (standard algebraic notation) format.  The function emits
//	records of this type.

#define	sanfNONE		0x00
#define	sanfRNK_AMBIG	0x01
#define	sanfFIL_AMBIG	0x02
#define	sanfCHECK		0x04
#define	sanfMATE		0x08

typedef	struct tagSAN {
	int	isqFrom;			// Where the moving piece originated.
	int	isqTo;				// Where it is going.
	unsigned	cmf;		// Flags, same flags as in the CM.
	unsigned	sanf;		// San flags, which indicate ambiguity and other
							//  special conditions.
	int	pc;					// The type of piece being moved (in the case of
							//  promotion, this will be pcPAWN, not the
							//  promoted piece).
    U32     m;
}	SAN, * PSAN;


extern bool SetBoardFromFEN(PCON pcon, char const * szFen);
extern void SanToSz(const SAN *psan, char * sz);
extern void CmToSan(PCON pcon, PSTE pste, const CM *pcmSrc, PSAN psan);
extern void CmToSz(const CM *pcm, char * sz);
extern void CmToSz(U32, char * sz);
extern bool move_is_legal(PCON pcon, PSTE pste, U32 m);

#endif //_BOARD_HXX
