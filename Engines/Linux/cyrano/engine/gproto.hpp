
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

//	-------------------------------------------------------------------------

//	The following functions must be implemented in the interface side.  They
//	are called by the engine.

//	-------------------------------------------------------------------------

//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-

//	This should be called often when thinking.  A decent place to put it is in
//	the eval function, or at the top of "search" or "qsearch".  After it
//	returns, you should check to see if the search has been aborted.  If the
//	search has been aborted, you shouldn't call this function anymore until
//	thinking has started again.

void VPrCallback(PCON pv);

//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-

//	Send the search result to the interface.  This is the move the program
//	intends to make, in the format "e2e4", "e1g1", "e7e8q", etc.

void VPrSendMove(const char * szMove, const char *szPonder);

//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-

//	If the engine is going to ponder, after it makes its move it will send
//	the move it is pondering on.

void VPrSendHint(const char * szMove);

//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-

//	This is called in response to "VPrBk".  Each call sends one line to
//	(currently) Winboard.

void VPrSendBookLine(const char * szLine, ...);

//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-

//	Sends a line of analysis to the interface.  "tm" is time searched in
//	milliseconds.  "szLine" is not particularly well defined, it can be
//	anything you want.
//
//	The "prsa" indicates whether the search is in a normal state, is failing
//	high, or is failing low.

#define	prsaNORMAL		0
#define	prsaFAIL_HIGH	1
#define	prsaFAIL_LOW	2

void VPrSendAnalysis(int depth, int seldepath, int val, unsigned long tm,
	U64 nodes, int prsa, char * szLine);

//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-

//	This should send a result when the game ends.  "szResult" will be
//	something like "1/2-1/2".  "szWhy" will be something like "Drawn by
//	50-move rule".  The "szWhy" pointer can be NULL if desired.
//
//	If an engine wants to resign, it can send a "1-0" or "0-1" as appropriate.

void VPrSendResult(const char * szResult, const char * szWhy);

//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-

//	This is just informational text sent by the engine.  The interface may
//	ignore it or display it or whatever.

void VPrSendComment(const char * szComment, ...);

//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-

//	This is called to offer a draw, or to accept a draw offer.

void VPrOfferDraw(void);

// feedback for the UCI protocol

void VPrSendUciDepth(const SEARCH_STATUS &ss);

void VPrSendUciNodeCount(bool force, const SEARCH_STATUS &ss);

void VPrSendUciCurrMove(const SEARCH_STATUS &ss);

void VPrSendUciCurrentLine(const CON *pcon, const STE *pste);

//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-

//	-------------------------------------------------------------------------

//	These functions are implemented in the engine.  The interface calls them
//	to make the engine do various things.

//	-------------------------------------------------------------------------

//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-

//	This will cause the engine to analyze the current position.  It should
//	never make a move, and it should not return from this call until the
//	search has been aborted, although it should periodically call
//	"VPrCallback".
//
//	This will never be called if the engine is thinking.

void VPrAnalyze(PCON pv);

//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-

//	This will cause the engine to think about the current position, and
//	eventually move.  It should not return from this call until the
//	search has been aborted or it otherwise decides not to think anymore (it
//	should stay in here if it wants to ponder).
//
//	This will never be called if the engine is thinking.

void VPrThink(PCON pv, unsigned long tmUs, unsigned long tmThem);

//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-

//	A request to clear hash tables and put the engine into a deterministic
//	state.  The engine can ignore this if it desires.
//
//	This will never be called if the engine is thinking.

void VPrReset(PCON pv);

//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-

//	The engine will set up the position specified by the FEN.  It will not
//	think about the position, it will just get ready to think and then exit.
//
//	This will never be called if the engine is thinking.

bool FPrSetboard(PCON pv, char * szFen);

//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-

//	The "szMove" parameter is the move that is going to be made on the board.
//	The engine should check to see if it is pondering on this move.  If so, it
//	should fiddle with its internal state such that it thinks that it is doing
//	a normal search.  It has "tmRemaining" time left.  When it's fiddled with
//	its state, it should return TRUE.
//
//	If the move passed in is not the move it is thinking about, it should
//	continue thinking and return FALSE.
//
//	If the engine is not pondering, it should return FALSE.

bool FPrPonderHit(PCON pv, char * szMove, unsigned long tmUs,
	unsigned long tmThem);

//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-

//	This adds "szMove" to the end of the game move list.  If the move is not
//	legal, the engine should return FALSE, otherwise TRUE.
//
//	This will never be called if the engine is thinking.

bool FPrAdvance(PCON pv, char * szMove);

//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-

//	This tells the engine to turn its analysis output on or off.  The engine
//	may or may not be thinking at the time.

void VPrSetPost(PCON pv, bool fPost);

//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-

//	This tells the engine what happened in the game that just finished.  A win
//	will be "1-0" or "0-1", and a draw will be "1/2-1/2".  There might be
//	other results such as "*", depending upon the implementation.
//
//	It is not guaranteed that a result will be sent at the end of a game that
//	is not finished due to mate, a formal draw, or a formal resignation.
//
//	This will never be called if the engine is thinking.

void VPrResult(const char * szResult, const char * szWhy);

//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-

//	This is called at startup before initialization.  If the engine wants to
//	print something on the screen, it is welcome to.  The engine is welcome
//	to completely ignore this.

void VPrBanner(void);

//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-

//	This tells the engine the current time control.  The engine may or may
//	not be thinking at the time.  If in analysis mode, this does not mean that
//	the engine should start thinking and make a move.
//
//	The Winboard code I'm providing will assure that this is not sent while
//	the engine is thinking, but it's not good to assume that this will always
//	be the case.

void VPrSetTimeControl(PCON pv, int cMoves,
	unsigned long tmBase, unsigned long tmIncr);

//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-

//	This is analogous to "VPrSetTimeControl", except that this is considered
//	a secondary sort of time control.  The primary is the one set by the
//	above function.  This is cleared by sending a "plyDepth" of <= 0.
//
//	This will never be called if the engine is thinking.

void VPrSetFixedDepthTimeControl(PCON pv, int plyDepth);

void VPrSetFixedNodeCount(PCON pcon, U64 nodeCount);

//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-

//	If the engine is thinking about making a move, it should make it pretty
//	soon (it doesn't have to make it before returning from this call).
//
//	This will *only* be called if the engine is thinking.  If the engine is
//	pondering or analyzing, it should ignore this.

void VPrMoveNow(PCON pv);

//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-

//	This tells the engine to undo the last move in its move list.
//
//	This will never be called if the engine is thinking.

void VPrUndoMove(PCON pv);

//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-

//	This informs the engine of a change in the pondering preferences.  If
//	"fPonder" is TRUE, the user has indicated that they would like the engine
//	to think even when it's not it's turn to move, and if it's FALSE, they
//	only want it thinking when its clock is running.
//
//	This function is not a request to begin pondering or to abort pondering.
//	It just tells the engine what it should do from now on.
//
//	The Winboard code I'm providing will assure that this is not sent while
//	the engine is thinking, but it's not good to assume that this will always
//	be the case.

void VPrSetPonder(PCON pv, bool fPonder);


void VPrSetUciMode(PCON pcon, bool isUCI);

//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-

//	This tells the engine directly to abort the search.  The engine should
//	*immediately* abort the search, meaning that it should unwind its stack
//	and exit its "think" routine immediately after returning from this call,
//	without calling any of the functions in this include file.
//
//	The engine *must* not ignore this.
//
//	This will *only* be called if the engine is thinking.

void VPrAbort(PCON pv);

//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-

//	This is a request for a hint, which may be ignored.  It's assumed that
//	this will be sent while the engine is pondering, and the idea is that
//	the engine should call "VPrSendHint" with the move it is thinking about.
//
//	The engine is free to ignore this request.
//
//	This will *only* be called if the engine is thinking, although it may not
//	be pondering.

void VPrHint(PCON pv);

//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-

//	This is a request for a book information.  If possible, the engine should
//	respond by calling "VPrSendBookLine" with possible book moves.
//
//	This will never be called if the engine is thinking.

void VPrBk(PCON pv);

//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-

//	The engine should fill in all of the fields in the record, or return FALSE
//	if it doesn't want to deal with this request.  The move being searched can
//	be returned in SAN or coordinate notation.
//
//	It is fine to ignore this request.
//
//	This will *only* be called if the engine is thinking.

typedef struct tagSTAT_REC {
	unsigned long tmElapsed;		// Time of search in milliseconds.  This
									//  may include time spent pondering.
	U64 nodes;					// Nodes searched to get here.
	int	plyDepth;					// Current depth.
    int selDepth;
	int	iMoveSearching;				// Index of move being searched (0..N-1).
	int	cLegalMoves;				// Number of legal moves here.
	char aszSearching[16];			// Move being searched now.
}	STAT_REC, * PSTAT_REC;

bool FPrStatus(PCON pv, PSTAT_REC psr);

//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-

//	This will be called if the user offers the engine a draw.
//
//	It may be passed when the engine is thinking.  The engine can attempt to
//	accept an offered draw by calling "VPrOfferDraw".
//
//	This command can be expected at any time, whether or not the engine is
//	thinking.

void VPrDrawOffered(PCON pv);

//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-

//	This command informs the engine that it is playing against another
//	computer, as opposed to a human.
//
//	When the engine gets a "setboard" command, it should assume that it is
//	playing against a human.  If it is playing against a computer, this
//	command will be invoked before the engine is told to think about anything.
//
//	This will never be called if the engine is thinking.

void VPrOpponentComputer(PCON pv);

//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-

//	This command informs the engine of the name of the opponent.
//
//	This will never be called if the engine is thinking.

void VPrOpponentName(PCON pv, char * szName);

//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-

//	This command asks the engine to return its name.
//
//	This may be called before the engine is initialized.

void VPrMyName(char * szName);

//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-

//	This command informs the engine of its own rating and the rating of its
//	opponent.
//
//	This will never be called if the engine is thinking.

void VPrRating(PCON pv, int eloUs, int eloThem);

//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-

//	This command informs the engine that it is playing on an ICS server.
//
//	This will never be called if the engine is thinking.

void VPrIcs(PCON pv, char * szIcs);

//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-

//	This will be called once at boot.  The engine should return a pointer to
//	its context, which will be passed to all of the above calles, or NULL if
//	it can't initialize itself.
//
//	This function also should set up the default position and a sensible time
//	control, and generally get things ready to go.
//
//	The arguments passed to this are intended to be C command line switches,
//	etc.
//
//	In my Gerbil implementation this is not called from the same thread that
//	will be performing searches.

void * PvPrEngineInit(int argc, char * argv[]);

//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-

void VPrPerft(PCON pcon, int depth);

void VShowBook(PCON pcon);
