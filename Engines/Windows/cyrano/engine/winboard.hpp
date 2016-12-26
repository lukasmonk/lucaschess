
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

#define	wmodeANALYZE	0
#define	wmodeTHINK		1

#define	bfXBOARD		0x01
#define	bfCMDLINE		0x02

typedef	struct	tagPCCO {
	char	pc;
	char	co;
}	PCCO;

typedef enum {WB1 = 1, WB2 = 2, UCI = 3} protover;

typedef	struct	tagIVARS {
    int     wmode;
    bool    fForce;
//    bool    fDidInitEngine;
    unsigned long tmMyTime;
    unsigned long tmTheirTime;
    unsigned long tmIncr;
    protover iProtover;
    int     coOnMove;           // If in protover 1 or UCI, the side to move now.
    int     coEditing;          // If in protover 1, the color whose pieces I
                                //  am adding.
    PCCO    argpcco[64];        // If it protover 1, the pieces being added
    bool    bfMode;
    bool    fEditing;
}	IVARS;

extern IVARS	s_ivars;

//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-

//void VAssertFailedW(const char * szMod, int iLine);

//#define	Assert(cond)		if (!(cond)) VAssertFailedW(s_aszModule, __LINE__)

//#define	fTRUE	1
//#define	fFALSE	0

void VSendToWinboard(const char * szFmt, ...);
void VProcessEngineCommand(PCON pv, bool fThinking);
bool FInitEngineThread(int argc, char * argv[]);
void VSendToEngine(const char * szFmt, ...);
void VStripWb(char * sz);
int	CszVectorizeWb(char * sz, char * rgsz[]);
void VErrorWb(const char * szMsg, const char * szCmd);
