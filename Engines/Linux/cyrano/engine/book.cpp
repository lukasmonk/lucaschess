
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
#include "board.hpp"
#include "moves.hpp"
#include "hash.hpp"
#include <stdlib.h>
#include <ctype.h>

#ifdef	_DEBUG
static char const s_aszModule[] = __FILE__;
#endif

//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-

//	The program's book is read from a text file at startup.  Positions that
//	can be reached by following the lines in the book are hashed.

//	When the program wants to play a book move, it checks to see if it can
//	reach any positions that are in the hash table.

//	If it finds any, it picks one at random.

//	Note that the program won't try to reach book positions that are in the
//	game history already.  This prevents it from going for a draw after
//	1. e4 e5 2. Nf3 Nc6 3. Ng1.

//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-

typedef	struct	tagBook {
	HASHK	hashk;
}	HBook, * PHBook;

PHBook s_rgbn;			// The book hash table.
int	s_cbnMax;		// Number of elements in the table, minus one.

//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-

//	Sticks this position into the book hash table, unless it is already there.
//	Returns TRUE if it added a node.

bool FHashBn(HASHK hashk)
{
	unsigned	ihashk;

	ihashk = (unsigned)hashk % s_cbnMax;
	for (;;) {
		if (s_rgbn[ihashk].hashk == hashk)
			return false;
		if (!s_rgbn[ihashk].hashk) {
			s_rgbn[ihashk].hashk = hashk;
			return true;
		}
		ihashk = (ihashk + 1) % s_cbnMax;
	}
}

//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-

//	Checks to see if this position is known.

bool FBnHashSet(HASHK hashk)
{
	unsigned ihashk = (unsigned)hashk % s_cbnMax;
	for (;;) {
		if (s_rgbn[ihashk].hashk == hashk)
			return true;
		if (!s_rgbn[ihashk].hashk)
			return false;
		ihashk = (ihashk + 1) % s_cbnMax;
	}
}

//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-

//	Takes the '\n' off the end of a string.

void VStrip(char * sz)
{
	int	i;

	for (i = 0; sz[i]; i++)
		if (sz[i] == '\n') {
			sz[i] = '\0';
			break;
		}
}

//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-

//	Breaks a string up into pieces.  If it finds a "(", this routine handles
//	this specially by taking everything after that as a single (and final)
//	token.

int	CszVectorizeBookLine(char * sz, char * rgsz[])
{
	int	i;
	int	csz;

	for (csz = 0, i = 0; sz[i]; i++)
		if (sz[i] == '(') {
			rgsz[csz++] = sz + i;
			break;
		} else if ((sz[i] != ' ') && (sz[i] != '\t')) {
			rgsz[csz++] = sz + i;
			for (;; i++) {
				if ((sz[i] == ' ') || (sz[i] == '\t'))
					break;
				if (sz[i] == '\0')
					break;
			}
			if (sz[i] == '\0')
				break;
			sz[i] = '\0';
		}
	return csz;
}

//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-

//	If I find an error while loading the book, I'll spit it out with this
//	function.

void VBookError(char * szFile, int cLine, char * sz)
{
	VPrSendComment("Book error (%s, line %d) : %s.", szFile, cLine, sz);
}

//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-

//	Load the book from a text file.

//	The book format is a line of opening description followed by zero or more
//	lines, each of which has one or two moves in e2e4 format, followed by a
//	line containing only a "!".

//	I took the book format TSP.  A quirk is that rather than storing castling
//	as e1g1, it stores it as "o-o" (lower case).  I handle this and "O-O"
//	(upper case) properly.

//	I also added an enhancement in that if a move is followed by a "?", it
//	and the rest of the moves for that side in this line will be not be
//	hashed, which will cause the program to not play any move moves for that
//	side in this line, although it will still play book moves in response.

//	If the "?" move appears in any subsequent lines, without a "?", it will
//	be hashed like normal.

bool FGenBook(PCON pcon, char * szFile, int cbMaxBook, int bdm)
{
	FILE    * pfI;
	int     cLine;
	bool    f;
	CM      argcmPrev[256];
	bool    argfAvoid[256];
	int     cplyPrev;
	bool    fResult;
	int     cbn = 0;

	//	The book stuff is not high performance, so I'm going to use all the
	//	book memory specified by the user, rather than trying to break it
	//	down to the next lower power of two.  I'll use at least one node.
	//
	s_cbnMax = cbMaxBook / sizeof(HBook);
	if (s_cbnMax < 1)
		s_cbnMax = 1;
	//
	//	Allocate the memory and open the file.  This is not cleaned up if I
	//	leave this routine early, because it's assumed that the program is
	//	immediately ending.
	//
	if ((s_rgbn = (PHBook) malloc(s_cbnMax * sizeof(HBook))) == NULL) {
		VPrSendComment("Can't allocate book memory: %d bytes",
			s_cbnMax * sizeof(HBook));
		return false;
	}
	memset(s_rgbn, 0, s_cbnMax * sizeof(HBook));
	if ((pfI = fopen(szFile, "r")) == NULL) {
		VPrSendComment("Can't open old book: \"%s\"", szFile);
		return false;
	}
	//	Eat the file.  This loop is kind of complicated.
	//
	cplyPrev = 0;
	fResult = true;
	for (cLine = 0;;) {
		char	aszBuf[512];
		char	aszOut[512];
		char *	szOut = aszOut;
		char *	argsz[256];
		int	csz;
		PSTE	pste = pcon->argste;
		int	isz;
		int	i;
		bool	fFollowing;
		int	cply;

		//	Get a line from the file.
		//
		if (fgets(aszBuf, sizeof(aszBuf), pfI) != aszBuf)	// Opening name.
			break;
		cLine++;
		VStrip(aszBuf);
		if ((aszBuf[0] == '/') ||		// Ignore some probable comment lines.
			(aszBuf[0] == '#') ||
			(aszBuf[0] == '*') ||
			(aszBuf[0] == ';') ||				
			(aszBuf[0] == '\0')) {
			if (bdm != bdmNONE)
				printf("%s\n", aszBuf);
			continue;
		}
		//	Set up the board.
		//
		f = SetBoardFromFEN(pcon, s_aszFenDefault);
		Assert(f);
		//
		//	Break the line up.
		//
		csz = CszVectorizeBookLine(aszBuf, argsz);
		isz = cply = 0;
		//
		//	Look at the first string and see if it's a move number.  If it's
		//	a move number other than one, it's assumed that the moves between
		//	one and there are taken from the previous line.  "cply" is set to
		//	the proper ply value based upon the move number (0 is white's
		//	first move, 1 is black's first, 2 is white's second, etc.).
		//
		//	If the move number is "1.", it's assumed to be a white move.  If
		//	there are extra dots appended (as in "1.." or "1..."), it's
		//	assumed that black is on the move.
		//
		if ((isz < csz) && (isdigit(argsz[isz][0]))) {
			int	cmov;

			cmov = 0;
			for (i = 0; argsz[isz][i]; i++)
				if (isdigit(argsz[isz][i]))
					cmov = cmov * 10 + argsz[isz][i] - '0';
				else
					break;
			cply = (cmov - 1) * 2;
			if ((argsz[isz][i++] == '.') && (argsz[isz][i] == '.'))
				cply++;
			isz++;
		} else
			cply = 0;
		//
		//	If white is to move, check the next string for "...", which
		//	indicates that it's black's move.
		//
		//	The deal here is that I'm trapping the case "1. ..." and marking
		//	it as black to move.
		//
		if ((isz < csz) && (!(cply & 1)))
			if (!strcmp(argsz[isz], "...")) {
				cply++;
				isz++;
			}
		//
		//	If I set this up to start other than on move 1 for white, I have
		//	to have enough previous context lying around from the previous
		//	line.  I gap is not allowed.
		//
		if (cplyPrev < cply) {
			VBookError(szFile, cLine, "Discontiguous opening line");
			fResult = false;
			goto lblDone;
		}
		//	Skip over any previous context.  If I'm outputting in "full" mode,
		//	write this stuff to stdout.  If I'm outputting in "compact" mode,
		//	I'm going to emit a couple of spaces per ply, so it indents in an
		//	attractive way.
		//
		for (i = 0; i < cply; i++) {
			char    aszMov[32];
			bool    f;

			if (bdm == bdmFULL) {
				char	aszSan[32];
				SAN	san;

				bGenMoves(pcon, pste);
				CmToSan(pcon, pste, &argcmPrev[i], &san);
				SanToSz(&san, aszSan);
				if (!(i & 1))
					szOut += sprintf(szOut, "%d. ", i / 2 + 1);
				szOut += sprintf(szOut, "%s", aszSan);
				if (argfAvoid[i])
					*szOut++ = '?';
				*szOut++ = ' ';
			} else if (bdm != bdmNONE)
				szOut += sprintf(szOut, "  ");
			CmToSz(&argcmPrev[i], aszMov);
			f = FAdvance(pcon, aszMov);
			Assert(f);
		}
		//	I'm going to take care of the new stuff now.
		//
		fFollowing = true;
		for (; isz < csz; isz++) {
			char	aszSan[32];
			char	aszMov[32];
			int	j;
			PCM	pcm;
			CM	cm;
			SAN	san;

			if (atoi(argsz[isz]))		// Skip move numbers.
				continue;
			if (argsz[isz][0] == '(')	// Variation name starts with "(" and
				break;					//  is last arg.  The vectorize
										//  routine used in here handles "("
										//  specially, so this works.
			//
			//	If the move some even number of plies back was a "?" move,
			//	this is a "?" move, too, otherwise it isn't.
			//
			if (cply < 2)	// Too close to the start to have a move before.
				argfAvoid[cply] = false;
			else
				argfAvoid[cply] = argfAvoid[cply - 2];
			//
			//	Look for an explicit "?", which marks this as a move to avoid.
			//
			for (j = 0; argsz[isz][j]; j++)
				if (argsz[isz][j] == '?') {
					argsz[isz][j] = '\0';
					argfAvoid[cply] = true;
					break;
				}
			//
			//	Find the move in the list of legal moves, essentially
			//	converting from SAN to internal format.
			//
			bGenMoves(pcon, pste);
			pcm = pste->mlist_start;
			for (; pcm < (pste + 1)->mlist_start; pcm++) {
				CmToSan(pcon, pste, pcm, &san);
				SanToSz(&san, aszSan);
				if (!strcmp(aszSan, argsz[isz]))
					break;
			}
			if (pcm >= (pste + 1)->mlist_start) {
#ifdef _DEBUG
//                VDumpBoard( pcon );
//                VDumpCm( pcon, pste );
#endif
				sprintf(aszBuf, "Illegal move for %s, token %d on line",
					(pste->side == coWHITE) ? "white" : "black", isz + 1);
				VBookError(szFile, cLine, aszBuf);
				fResult = false;
				goto lblDone;
			}
			cm = *pcm;
			//
			//	Output the move if necssary.  This is some tricky and nasty
			//	code, depending upon what mode I'm in, what I've already
			//	outputed on this line, and whether the book I'm reading in
			//	is "full" or "compact".
			//
			if (bdm != bdmNONE) {
				if (fFollowing)
					if ((cplyPrev <= cply) || (memcmp(&cm,
						&argcmPrev[cply], sizeof(CM)))) {
						if ((cply & 1) && (bdm == bdmCOMPACT))
							szOut += sprintf(szOut, "%d. ... ", cply / 2 + 1);
						fFollowing = false;
					} else if (bdm == bdmCOMPACT)
						szOut += sprintf(szOut, "  ");
				if ((!fFollowing) || (bdm == bdmFULL)) {
					if (!(cply & 1))
						szOut += sprintf(szOut, "%d. ", cply / 2 + 1);
					szOut += sprintf(szOut, "%s", aszSan);
					if (argfAvoid[cply])
						*szOut++ = '?';
					*szOut++ = ' ';
				}
			}
			//	Make the move on the internal board.
			//
			CmToSz(&cm, aszMov);
			f = FAdvance(pcon, aszMov);
			Assert(f);
			//
			//	Hash it unless it is a "?" move.  This is how "avoid" moves
			//	are handled -- they are simply not hashed in the book file.
			//
			//	What this means of course is that every instance of the
			//	position after this move is made must be marked with a "?",
			//	or it's like none of them are.
			//
			//	With a "compact" book, this isn't a problem, unless there are
			//	transpositions.
			//
			if (!argfAvoid[cply]) {
				HASHK	hashkFull = pste->hashkPc;

				//	Note that the previous line generates moves, which is
				//	annoying.
				//
				//	I'm hashing the "full" keys, so the en-passant and
				//	castling don't cause problems.
				//
				if (FHashBn(hashkFull))
					if (++cbn >= s_cbnMax * 3 / 4) {
						VBookError(szFile, cLine, "Book memory full");
						fResult = false;
						goto lblDone;
					}
			}
			//	Remember the move in the "last line" array, which will be used
			//	to diff the next line against this one.
			//
			argcmPrev[cply++] = cm;
		}
		//	Record the length of the current line.
		//
		cplyPrev = cply;
		//
		//	Deal with the variation name field at the end of the line, which
		//	means just printing it out if I'm in dump mode.
		//
		if (bdm != bdmNONE) {
			if (isz < csz) {		// <-- "isz < csz" if I found a "(".
				//
				//	I'm going to print the line in 128 characters, followed by
				//	the variation name.  This is pretty wide, but lots of
				//	lines are longer than 80 characters.
				//
				while (szOut - aszOut < 128)
					*szOut++ = ' ';
				szOut += sprintf(szOut, "%s", argsz[isz]);
			}
			*szOut = '\0';
			VStrip(szOut);
			printf("%s\n", aszOut);
		}
	}
	if (bdm == bdmNONE)
		VPrSendComment("%d (old)book positions", cbn);
	else
		fResult = false;	// If I dumped this book, I'm going to exit the
lblDone:					//  program after this returns.
	fclose(pfI);
	return fResult;
}

//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-

//	This just makes a list of book moves the program is considering choosing
//	from, and spits it out to the interface.

void VDumpBookPv(PCON pcon, PCM rgcm, int ccm, int icm)
{
	PSTE pste = pcon->argste;
	char asz[256];
	char * sz;
	SAN	san;
	int	i;

	if (!pcon->fPost)
		return;
	sz = asz;
	*sz++ = '(';
	for (i = 0; i < ccm; i++) {
		if (i) {
			*sz++ = ',';
			*sz++ = ' ';
		}
		CmToSan(pcon, pste, &rgcm[i], &san);
		SanToSz(&san, sz);
		sz += strlen(sz);
	}
	*sz++ = ')';
	*sz++ = '\0';
	VPrSendAnalysis(0, 0, 0, 0, 0, prsaNORMAL, asz);
}

//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-

//	Finds possible book positions.  If it finds any, it picks one at random
//	and returns it.  It won't pick a position that's already appeared on the
//	board in this game.  This is done to keep the engine from taking draws
//	in situations where the opponent tries to get cute.

bool FBookMove(PCON pcon, PCM pcmMove)
{
	PSTE pste = &pcon->argste[0];
	CM	argcm[256];
	int	ccm;

	bGenMoves(pcon, pste);
	ccm = 0;
    foreachmove(pste,pcm) {
//	for (pcm = pste->pcmFirst; pcm < (pste + 1)->pcmFirst; pcm++) {
		HASHK	hashkFull;

		bMove(pcon, pste, pcm->m);
		hashkFull = (pste+1)->hashkPc;
		if (FBnHashSet(hashkFull)) {
			int	i;

			for (i = 0; i <= pcon->gc.ccm; i++)
				if (pcon->gc.arghashk[i] == hashkFull)
					break;
			if (i >= pcon->gc.ccm)
				argcm[ccm++] = *pcm;
		}
		bUndoMove(pcon, pste, pcm->m);
	}
	if (ccm) {
		int	icm = rand() % ccm;

		VDumpBookPv(pcon, argcm, ccm, icm);
		*pcmMove = argcm[icm];
		return true;
	}
	return false;
}

//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-
