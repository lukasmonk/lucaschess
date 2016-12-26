
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
#include "gproto.hpp"
#include <stdlib.h>

#ifdef	DEBUG
//static char const s_aszModule[] = __FILE__;
#endif

#ifdef	DEBUG

//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-

//static char s_argbBoardCo[coMAX] = {'/',' '};

//	This routine is probably never used, but if you want an ascii board for
//	debugging purposes, here it is.

void VDumpBoard(PCON pcon)
{
#if 0
	int	rnk;
	int	fil;

	putchar(' ');
	for (fil = filA; fil <= filH; fil++)
		printf("   %c", fil + 'a');
	putchar('\n');
	for (rnk = rnk8; rnk >= rnk1; rnk--) {
		printf("  +");
		for (fil = filA; fil <= filH; fil++)
			printf("---+");
		putchar('\n');
		printf("  |");
		for (fil = filA; fil <= filH; fil++) {
			int	coSq = ((rnk ^ fil) & 1) ? coWHITE : coBLACK;

			putchar(s_argbBoardCo[coSq]);
			putchar(s_argbBoardCo[coSq]);
			putchar(s_argbBoardCo[coSq]);
			putchar('|');
		}
		putchar('\n');
		printf("%c |", rnk + '1');
		for (fil = filA; fil <= filH; fil++) {
			int	isq = IsqFromRnkFil(rnk, fil);
			int	coSq = ((rnk ^ fil) & 1) ? coWHITE : coBLACK;

			putchar(s_argbBoardCo[coSq]);
			if (pcon->argsq[isq].ppi == NULL)
				putchar(s_argbBoardCo[coSq]);
			else
				putchar(s_argbPc[pcon->argsq[isq].ppi->co][
					pcon->argsq[isq].ppi->pc]);
			putchar(s_argbBoardCo[coSq]);
			putchar('|');
		}
		printf(" %c\n", rnk + '1');
		printf("  |");
		for (fil = filA; fil <= filH; fil++) {
			int	coSq = ((rnk ^ fil) & 1) ? coWHITE : coBLACK;

			putchar(s_argbBoardCo[coSq]);
			putchar(s_argbBoardCo[coSq]);
			putchar(s_argbBoardCo[coSq]);
			putchar('|');
		}
		putchar('\n');
	}
	printf("  +");
	for (fil = filA; fil <= filH; fil++)
		printf("---+");
	putchar('\n');
	putchar(' ');
	for (fil = filA; fil <= filH; fil++)
		printf("   %c", fil + 'a');
	putchar('\n');
#endif
}

//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-

#if 0
void VDumpCm(PCON pcon, PSTE pste)
{
	PCM	pcm;

	for (pcm = pste->pcmFirst; pcm < (pste + 1)->pcmFirst; pcm++) {
		printf("CM #%d %c%c%c%c%c",
			pcm - pste->pcmFirst,
			FilFromIsq(pcm->isqFrom) + 'a',
			RnkFromIsq(pcm->isqFrom) + '1',
			(pcm->cmf & cmfCAPTURE) ? 'x' : '-',
			FilFromIsq(pcm->isqTo) + 'a',
			RnkFromIsq(pcm->isqTo) + '1');
		if (pcm->cmf & cmfPR_MASK)
			printf("=%c", s_argbPc[coWHITE][pcm->cmf & cmfPR_MASK]);
		if (pcm->cmf & cmfMAKE_ENP)
			printf(" (EnP)");
		if (pcm->cmf & cmfCASTLE)
			printf(" (Castle)");
		if (pcm->cmf & cmfSET_ENP)
			printf(" (Set EnP)");
		putchar('\n');
	}
}
#endif
//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-

//	This is the guts of the "Assert" macro.

void VAssertFailed(const char * szMod, int iLine)
{
	VPrSendComment("Assert Failed: %s+%d\n", szMod, iLine);
#ifdef _MSC_VER
	_asm	{
		int 3
	}
#else
    asm ("int $3\n\t");

#endif
//	exit(1);
    int x = 0;
}

//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-

#endif	// DEBUG
