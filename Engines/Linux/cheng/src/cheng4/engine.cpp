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

#include "engine.h"
#include "kpk.h"
#include "thread.h"
#include "movegen.h"
#include "tune.h"
#include <memory.h>
#include <iostream>

namespace cheng4
{

// EngineThread

EngineThread::EngineThread() : doneSearch(0), searching(0), shouldQuit(0)
{
	searchMode.reset();
	search.board.reset();
}

void EngineThread::destroy()
{
	if ( searching )
	{
		search.abort(1);
		doneSearch.wait();
	}
	shouldQuit = 1;
	commandEvent.signal();
}

void EngineThread::setMode( const SearchMode &sm )
{
	searchMode = sm;
}

void EngineThread::work()
{
	for (;;)
	{
		commandEvent.wait();
		if ( shouldQuit )
			break;
		doneSearch.reset();
		searching = 1;
		search.iterate( search.board, searchMode );
		doneSearch.signal();
		searching = 0;
	}
}

// Engine

void Engine::init( int npar, const char **par )
{
	Timer::init();
	Tables::init();
	BitOp::init();
	Magic::init();
	Zobrist::init();
	PSq::init();
	KPK::init();
	Eval::init();
	Search::init();
#ifdef USE_TUNING
	// pass parameters:
	for (int i=0; i<npar; i+=2)
		TunableParams::setParam( par[i], par[i+1] );
#else
	(void)npar;
	(void)par;
#endif
}

void Engine::done()
{
	Magic::done();
	Timer::done();
}

Engine::Engine( size_t transMegs ) : ponder(0), pondering(0), ponderMove( mcNone ), stm( ctWhite ),
	uciMode(0), ownBook(1), tt(0)
{
	curBoard.reset();
	startBoard = curBoard;
	pondBoard.clear();

	// hash table init goes here
	tt = new TransTable;
	tt->resize( transMegs * 1048576 );
	tt->clear();

	mainThread = new EngineThread;
	mainThread->search.setHashTable( tt );

	book.open("cheng2015.cb");
}

Engine::~Engine()
{
	abortSearch();
	delete tt;
	mainThread->kill();
}

// run engine (no allocation done until this is called)
void Engine::run()
{
	mainThread->run();
}

// start search
void Engine::startSearch( const SearchMode &mode, bool noabort )
{
  if ( !noabort )
	  abortSearch();
	// NEW: don't use book lookup when pondering!
	if ( ownBook && mode.maxTime && !mode.ponder )
	{
		// probe book here!
		Move mv = book.probe( curBoard );
		if ( mv != mcNone )
		{
			// new: don't use book moves that lead into repetitions to avoid cheap book draws!
			UndoInfo ui;
			curBoard.doMove( mv, ui, curBoard.isCheck(mv, curBoard.discovered()) );
			Signature sig = curBoard.sig();
			curBoard.undoMove( ui );
			// only if not book-rep!
			if ( !mainThread->search.rep.isRep(sig) )
			{
				SearchInfo &info = mainThread->search.info;
				info.reset();
				info.flags |= sifBestMove;
				info.bestMove = mv;
				mainThread->search.sendInfo();
				return;
			}
		}
	}
	pondering = uciMode ? 0 : mode.ponder;
	mainThread->setMode( mode );
	mainThread->search.startSearch.reset();
	mainThread->commandEvent.signal();
	if ( !pondering )
		mainThread->search.startSearch.wait();
}

void Engine::setPonder( bool ponder_ )
{
	ponder = ponder_;
}

// get ponder mode
bool Engine::getPonder() const
{
	return ponder;
}

// stop search
void Engine::stopSearch()
{
	ponderMove = mcNone;
	if ( mainThread->searching )
	{
		mainThread->search.abort();
		mainThread->doneSearch.wait();
	}
	if ( pondering )
	{
		pondering = 0;
		undoMove(1);
	}
}

void Engine::abortSearch()
{
	ponderMove = mcNone;
	if ( mainThread->searching )
	{
		mainThread->search.abort(1);
		mainThread->doneSearch.wait();
	}
	if ( pondering )
	{
		pondering = 0;
		undoMove(1);
	}
}

void Engine::setBoard( const Board &b )
{
	abortSearch();
	startBoard = curBoard = mainThread->search.board = b;
	mainThread->search.rep.clear();
	mainThread->search.rep.push( b.sig(), 1 );
	stm = b.turn();
	moves.clear();
}

// reset board to startpos
void Engine::resetBoard()
{
	Board b;
	b.reset();
	setBoard(b);
}

// tries to undo move
bool Engine::undoMove( bool noabort )
{
	if ( !noabort )
		abortSearch();
	if ( moves.empty() )
		return 0;
	moves.pop_back();
	std::vector< GameMove > omoves( moves );
	setBoard( startBoard );
	for (size_t i=0; i<omoves.size(); i++)
		doMove( omoves[i].move );
	return 1;
}

// set turn (=stm)
bool Engine::setTurn( Color c )
{
	abortSearch();
	Board &b = mainThread->search.board;
	if ( b.turn() == c )
		return 1;
	// play nullmove!
	if ( doMove( mcNull ) )
	{
		stm = mainThread->search.board.turn();
		return 1;
	}
	return 0;
}

// returns current stm
Color Engine::turn() const
{
	return stm;
}

bool Engine::doMoveInternal( Move m, bool nostop, bool *phit )
{
	// must handle pondering here
	if ( !nostop && pondering )
	{
		// in the case of pondermiss, ...
		if ( m == ponderMove )
		{
			// ponderhit! => switch to normal search
			pondering = 0;
			ponderMove = mcNone;
			ponderHit();
			if ( phit )
				*phit = 1;
			return 1;
		}
		abortSearch();
		nostop = 1;
	}

	if ( m == mcNone )
		return 0;
	if ( !nostop )
		abortSearch();
	Board &b = mainThread->search.board;
	if ( m == mcNull )
	{
		if ( b.inCheck() )
			return 0;
		GameMove gm;
		gm.sig = curBoard.sig();
		gm.move = m;

		UndoInfo ui;
		b.doNullMove( ui );
		curBoard.doNullMove( ui );
		moves.push_back( gm );
		if ( curBoard.turn() == ctWhite )
			curBoard.incMove();
		mainThread->search.rep.clear();
		return 1;
	}

	m &= mmNoScore;

	MoveGen mg( b );
	Move gm;
	while ( (gm = mg.next() ) != mcNone )
	{
		if ( gm == m )
			break;
	}
	if ( gm == mcNone )
		return 0;

	GameMove mv;
	mv.sig = curBoard.sig();
	mv.move = m;

	bool irrev = b.isIrreversible(m);

	UndoInfo ui;
	bool ischk = b.isCheck( m, b.discovered() );
	b.doMove( m, ui, ischk );
	curBoard.doMove( m, ui, ischk );
	moves.push_back( mv );
	if ( curBoard.turn() == ctWhite )
		curBoard.incMove();
	if ( irrev )
		mainThread->search.rep.clear();
	mainThread->search.rep.push( b.sig(), !b.fifty() );
	return 1;
}

bool Engine::doMove( Move m, bool nostop, bool *phit )
{
	return doMoveInternal( m, nostop, phit );
}

// set search callback
void Engine::setCallback( SearchCallback cbk, void *param )
{
	abortSearch();
	mainThread->search.setCallback( cbk, param );
}

// clear hashtable
void Engine::clearHash()
{
	abortSearch();
	mainThread->search.clearHash();
	mainThread->search.clearSlots();
}

// set hashtable size in megs
bool Engine::setHash( uint megs )
{
	abortSearch();
	bool res = mainThread->search.tt->resize( (size_t)megs * (size_t)1048576 );
	clearHash();
	return res;
}

// ponderhit!
void Engine::ponderHit()
{
	mainThread->search.ponderHit = 1;
}

const Board &Engine::board() const
{
	return curBoard;
}

const Board &Engine::ponderBoard() const
{
	return pondBoard;
}

const Board &Engine::actualBoard() const
{
	return pondering ? pondBoard : curBoard;
}

// start pondering (if possible)
bool Engine::startPondering( SearchMode mode, bool noabort )
{
	Move pm = ponderMove;
	if ( !ponder || pm == mcNone )
		return 0;
	pondBoard = curBoard;
	// play ponder move
	if ( !doMove( pm, noabort ) )
		return 0;
	mode.ponder = 1;
	startSearch( mode, noabort );
	ponderMove = pm;
	pondering = 1;
	return 1;
}

// set move to ponder on
void Engine::setPonderMove( Move pm )
{
	assert( !pondering );
	ponderMove = pm;
}

// is threefold repetition?
bool Engine::isThreefold() const
{
	Signature sig = board().sig();
	size_t repc = 0;
	for ( i32 i = (i32)moves.size()-2; i >= 0; i-=2 )
	{
		if ( moves[i].sig == sig )
			if ( ++repc >= 2 )
				return 1;
	}
	return 0;
}

// set number of threads, default is 1
void Engine::setThreads( uint nt )
{
	abortSearch();
	if ( nt < 1 )
		nt = 1;
	mainThread->search.setThreads( nt-1 );
}

// limit number of threads, default is 64
void Engine::limitThreads( uint maxt )
{
	abortSearch();
	if ( maxt < 1 )
		maxt = 1;
	mainThread->search.setMaxThreads( maxt-1 );
}

// set multipv mode, default is 1
void Engine::setMultiPV( uint mpv )
{
	if ( mpv < 1 )
		mpv = 1;
	else if ( mpv > 256 )
		mpv = 256;
	mainThread->search.setMultiPV( mpv );
}

void Engine::setUCIMode( bool uci )
{
	uciMode = uci;
}

// set own book mode
void Engine::setOwnBook( bool obk )
{
	ownBook = obk;
}

// set nullmove flag
void Engine::setNullMove( bool nmv )
{
	mainThread->search.enableNullMove( nmv );
}

// set elo limit master flag
void Engine::setLimit( bool limit )
{
	mainThread->search.setEloLimit( limit );
}

// set elo (if limit is enabled)
void Engine::setElo( u32 elo )
{
	mainThread->search.setMaxElo( elo );
}

// get opening book
const Book &Engine::getBook() const
{
	return book;
}

}
