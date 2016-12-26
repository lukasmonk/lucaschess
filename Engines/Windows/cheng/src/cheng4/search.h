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

#include "board.h"
#include "history.h"
#include "killer.h"
#include "trans.h"
#include "eval.h"
#include "repetition.h"
#include "thread.h"
#include <vector>

namespace cheng4
{

struct SearchMode
{
	std::vector< Move > moves;				// search only these moves (if not empty)
	uint mateSearch;						// if nonzero, search for mate in x moves
	Depth maxDepth;							// maximum search depth (only if nonzero)
	NodeCount maxNodes;						// maximum nodes to search (only if nonzero)
	uint multiPV;							// multiPV (default: 1 => no multipv)
	i32 maxTime;							// maximum time in milliseconds (0 if infinite)
	i32 absLimit;							// absolute maximum time in msec (0 if infinite)
	volatile bool ponder;					// pondering?
	bool fixedTime;							// fixed time per move

	void reset();
};

// extra search flags
enum SearchFlags
{
	sfNoTimeout		=	1,
	sfNoNullMove	=	2
};

enum SearchInfoFlags
{
	sifNodes		=	1,
	sifNPS			=	2,
	sifTB			=	4,
	sifDepth		=	8,
	sifSelDepth		=	16,
	sifCurIndex		=	32,
	sifCurMove		=	64,
	sifTime			=	128,
	sifPV			=	256,
	sifHashFull		=	512,
	sifBestMove		=	1024,
	sifPonderMove	=	2048,
	sifRaw			=	4096		// protocol send mode hack: force raw (direct)mode
};

struct SearchInfo
{
	const Move *pv;				// PV
	NodeCount nodes;			// nodes
	NodeCount nps;				// nps
	u64 tbHits;					// TB hits
	Ply selDepth;				// selective depth

	MoveCount curIndex;			// zero-based
	MoveCount curCount;			// total move count

	Move curMove;				// current move
	Time time;					// total time searched so far

	uint pvIndex;				// multi pv mode: zero-based index
	Score pvScore;				// PV score
	uint pvCount;				// number of moves in PV

	uint hashFull;				// hash full (permill)
	Move bestMove;				// best move
	Move ponderMove;			// ponder move

	uint flags;					// flags valid entries
	enum BoundType pvBound;		// bound type
	Depth depth;				// current nominal depth

	// reset search info
	void reset();
};

class LazySMPThread;

typedef void (*SearchCallback)( const SearchInfo &si, void *param );

struct Search
{
	struct Stack
	{
		Move current;				// current move
		Killer killers;				// killers
		FracDepth reduction;		// reducing
		u32 pad[2];					// pad structure to 32 bytes
	};

	Board board;					// board
	History history;				// history table
	Eval eval;						// eval

	Stack stack[ maxStack ];		// search stack

	RepHash rep;					// repetition stack
	i32 startTicks;					// start ticks
	i32 nodeTicks;					// last reported node counts
	uint timeOutCounter;			// time out node counter
	Move *triPV;					// triangular PV table
	volatile uint newMultiPV;		// next iteration multiPV (0 = none)
	Ply selDepth;					// selective depth for this thread

	// shared data
	TransTable *tt;					// transposition table (shared)
	SearchMode mode;				// search mode
	NodeCount nodes;
	Age age;

	// lazy SMP helper threads (always desired_threads-1)
	std::vector< LazySMPThread * > smpThreads;

	SearchInfo info;
	SearchInfo infoPV[maxMoves];
	SearchCallback callback;
	void *callbackParam;

	bool canStop;					// can stop search (at least iteration 1 must complete)
	bool abortRequest;				// abort search requested?
	AbortFlag volatile aborting;	// aborting search?
	AbortFlag volatile abortingSmp;	// aborting because another thread has finished root window
	bool volatile outputBest;		// output bestmove? (used in conjunction with aborting flag)
	bool volatile ponderHit;		// ponderhit!

	size_t maxThreads;				// maximum number of helper threads allowed (i.e 0 = none; 1 thread total)
									// defaults to 63
	volatile bool eloLimit;			// elo limit master flag
	volatile u32 maxElo;			// 2500 = full

	Depth minQsDepth;				// min qsearch depth (limits qs explosions)

	bool verbose;					// can send verbose commands (currmove etc.)?
	bool verboseFixed;				// if this is set, verboseLimit is ignored and verbose is fixed
	volatile u8 searchFlags;		// search flags

	Move iterBest, iterPonder;		// best/ponder move from last iteration

	// root data
	struct RootMove
	{
		Move move;
		Score score;
		NodeCount nodes;
		Move pv[maxPV];
		uint pvCount;
	};

	struct RootMovePtrPred
	{
		inline bool operator()( const RootMove *m0, const RootMove *m1 ) const
		{
			if ( m0->score != m1->score )
				return m0->score > m1->score;
			return m0->nodes > m1->nodes;
		}
	};

	struct RootMoves
	{
		Bitboard discovered;			// discovered checkers mask
		RootMove moves[ maxMoves ];
		RootMove *sorted[ maxMoves ];
		size_t count;
		Move bestMove;					// root search best move
		Score bestScore;				// root search best score

		inline RootMoves() {}
		inline RootMoves( const RootMoves &o ) { *this = o; }
		RootMoves &operator =( const RootMoves &o );
	};

	RootMoves rootMoves;
	RootMovePtrPred rootPred;

	Event startSearch;					// start search event (signaled when it's safe to set abort flag!)
										// note: manual reset event

	// timed out?
	bool timeOut();

	// extensions (before move is made)
	template< bool pv > FracDepth extend( Move m, bool isCheck, Bitboard dc ) const
	{
		if ( isCheck )
		{
			(void)dc;		// no warnings about unreferenced param
			// extend non-discovered checks in non-pv nodes less
			if ( !pv && !(BitOp::oneShl(MovePack::from(m)) & dc) )
				return fracOnePly/2;
			return fracOnePly;
		}

		if ( MovePack::isCapture(m) )
		{
			NPMat npm = board.nonPawnMat();
			if ( npm && npm == Tables::npValue[ PiecePack::type(board.piece(MovePack::to(m))) ] )
				return fracOnePly;				// extend pawn ending one ply
		}
		FracDepth ext = 0;

		Piece p = board.piece( MovePack::from(m) );
		if ( PiecePack::type( p ) != ptPawn )
			return ext;
		Square to = MovePack::to(m);
		if ( (board.turn() == ctWhite ? SquarePack::relRank<ctWhite>(to) : SquarePack::relRank<ctBlack>(to)) != RANK7 )
			return ext;
		if ( Tables::passerMask[ board.turn() ][ to ] & board.pieces( flip(board.turn()), ptPawn ) )
			return ext;
		// extend passer pushes to 7th rank 1 ply
		return fracOnePly;
	}

	// compute triangular PV base index (see CPW)
	static inline uint triIndex( Ply ply )
	{
		return ply*(2*maxPV+1-ply)/2;
	}

	// copy underlying PV
	void copyPV( Ply ply );

	template<bool pv> inline uint initPly( Ply ply )
	{
		uint res;
		stack[ ply ].current = mcNone;
#ifdef USE_TUNING
		stack[ ply ].killers.hashMove = mcNone;		// note: this is only necessary if we don't hash qsearch
#endif
		stack[ ply+2 ].killers.clear();
		nodes++;
		if ( pv )
			triPV[res = triIndex(ply)] = mcNone;
		else
			res = 0;
		return res;
	}

	// is draw?
	inline bool isDraw() const
	{
		return (board.isDraw() != drawNotDraw) || rep.isRep( board.sig() );
	}

	// quiescence search
	template< bool pv, bool incheck > Score qsearch( Ply ply, Depth depth, Score alpha, Score beta );

	// alpha-beta search
	template< bool pv, bool incheck, bool donull >
		Score search( Ply ply, FracDepth fdepth, Score alpha, Score beta );

	// send PV info
	void sendPV( const RootMove &rm, Depth depth, Score score, Score alpha, Score beta, uint mpvindex = 0 );

	// this is for delaying PV results
	void flushCachedPV( size_t totalMoves );

	// do root search
	Score root( Depth depth, Score alpha = -scInfinity, Score beta = +scInfinity );

	// start new search and iterate
	Score iterate( Board &b, const SearchMode &sm, bool nosendbest = 0 );

	// clear all helper slots
	void clearSlots( bool clearEval = 1 );

	// clear hashtable (and slots)
	void clearHash();

	// set hashtable
	void setHashTable( TransTable *tt_ );

	// abort search
	// nobest: when set, don't output bestmove
	inline void abort( bool nobest = 0 )
	{
		if ( !nobest && !canStop )
			abortRequest = 1;
		else
			aborting = 1;
		outputBest = !nobest;
	}

	void setCallback( SearchCallback cbk, void *param = 0 );

	// execute info callback
	void sendInfo() const;
	void sendInfo( const SearchInfo &sinfo ) const;

	// get bestmove/pondermove from last iteration
	void getBest( SearchInfo  &sinfo );

	// send bestmove/pondermove from last iteration
	void sendBest();

	// extract PV for rm
	void extractPV( RootMove &rm ) const;

	Move extractPonderFromHash( Move best );

	Search( size_t evalKilo = 256, size_t pawnKilo = 512, size_t matKilo = 64 );
	~Search();

	// set helper threads
	void setThreads( size_t nt );

	// set maximum # of helper threads
	void setMaxThreads( size_t maxt );

	// set multipv (can also be called while analyzing!)
	void setMultiPV( uint mpv );

	// set elo limit master flag
	void setEloLimit( bool limit );

	// set maximum elo limit (2500 = full)
	void setMaxElo( u32 elo );

	// enable timeout flag
	void enableTimeOut( bool enable );

	// enable nullmove flag
	void enableNullMove( bool enable );

	// start root smp search
	void smpStart( Depth depth, Score alpha, Score beta );
	// stop root smp search
	void smpStop();
	// sync smp threads (before iteration starts)
	void smpSync() const;

	// return number of nodes searched
	inline NodeCount smpNodes() const;

	// static init
	static void init();

protected:
	Search *master;							// SMP master
	i32 initIteration();
};

class LazySMPThread : public Thread
{
public:
	Search search;

	struct CommandData
	{
		Depth depth;
		Score alpha;
		Score beta;
		Search::RootMoves rootMoves;
		uint multiPV;
	} commandData;

	Event startedSearch;		// set when search has started
	Event doneSearch;			// set when done with search
	Event commandEvent;			// set when command is pending
	Event quitEvent;			// quit event
	volatile bool searching;	// searching flag
	volatile bool shouldQuit;	// should quit?

	LazySMPThread();
	void destroy();

	void start( Depth depth, Score alpha, Score beta, const Search &master );
	void abort();

	void work();
};

inline NodeCount Search::smpNodes() const
{
	NodeCount res = nodes;
	for ( size_t i=0; i<smpThreads.size(); i++)
		res += smpThreads[i]->search.nodes;
	return res;
}

}
