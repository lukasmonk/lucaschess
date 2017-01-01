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

#pragma once

#include "search.h"
#include "book.h"
#include "thread.h"
#include <vector>

namespace cheng4
{

class Protocol;

class EngineThread : public Thread
{
public:
	Event doneSearch;			// set when done with search
	Event commandEvent;			// set when command is pending
	SearchMode searchMode;		// current search mode
	volatile bool searching;	// searching flag
	volatile bool shouldQuit;	// should quit?

	// main search
	Search search;

	EngineThread();
	void destroy();

	// set search mode
	void setMode( const SearchMode &sm );

	void work();
};

class Engine
{
	Engine( const Engine & )
	{
		assert( 0 && "Engine cannot be copied!" );
	}
	Engine &operator =( const Engine & )
	{
		assert( 0 && "Engine cannot be copied!" );
		return *this;
	}
protected:
	struct GameMove
	{
		Signature sig;				// signature before move was made
		Move move;
		Move pad;
	};

	friend class Protocol;
	bool volatile ponder;
	bool volatile pondering;
	Move volatile ponderMove;
	Color stm;						// current stm
	bool volatile uciMode;			// uci mode?
	Board curBoard;					// current board
	Board startBoard;				// startpos board
	Board pondBoard;				// ponder board (FIXME: this is ugly and I should probably redesign it)
	std::vector< GameMove > moves;	// game moves
	bool doMoveInternal( Move m, bool nostop = 0, bool *phit = 0 );
	Book book;						// book!
	bool ownBook;					// use own book? (default: true)
	TransTable *tt;					// transposition table

public:
	// static init
	static void init( int npar = 0, const char **par = 0 );
	static void done();

	EngineThread *mainThread;

	Engine( size_t transMegs = 4 );
	~Engine();

	const Board &board() const;
	const Board &ponderBoard() const;
	// actual board: before pondering in ponder on mode
	const Board &actualBoard() const;

	// run engine (no allocation done until this is called)
	void run();

	// start search
	void startSearch( const SearchMode &mode, bool noabort = 0 );

	// stop search (=move now)
	// does output bestmove
	void stopSearch();

	// abort search
	// does NOT output bestmove
	void abortSearch();

	// set board
	void setBoard( const Board &b );

	// reset board to startpos
	void resetBoard();

	// tries to undo move
	bool undoMove( bool noabort = 0 );

	// set turn (=stm)
	// returns 0 if ok
	bool setTurn( Color c );

	// returns current stm
	Color turn() const;

	// doMove
	// returns 0 if illegal
	bool doMove( Move m, bool noabort = 0, bool *phit = 0 );

	// set search callback
	void setCallback( SearchCallback cbk, void *param = 0 );

	// ponderhit!
	void ponderHit();

	// set ponder mode
	void setPonder( bool ponder );
	// get ponder mode
	bool getPonder() const;
	// set hashtable size in megs
	bool setHash( uint megs );
	// clear hashtable
	void clearHash();

	// get pondering flag
	inline bool isPondering()
	{
		return pondering;
	}

	// start pondering (if possible)
	bool startPondering( SearchMode mode, bool nostop = 0 );

	// set move to ponder on
	void setPonderMove( Move pm );

	// set ponder move
	inline Move getPonderMove() const
	{
		return ponderMove;
	}

	// is threefold repetition?
	bool isThreefold() const;

	// set number of threads, default is 1
	void setThreads( uint nt );

	// limit number of threads, default is 64
	void limitThreads( uint maxt );

	// set multipv mode, default is 1
	void setMultiPV( uint mpv );

	// set uci mode (no extra pondering)
	void setUCIMode( bool uci );

	// set own book mode
	void setOwnBook( bool obk );

	// set nullmove flag
	void setNullMove( bool nmv );

	// set elo limit master flag
	void setLimit( bool limit );

	// set elo (if limit is enabled)
	void setElo( u32 elo );

	// get opening book
	const Book &getBook() const;
};

}
