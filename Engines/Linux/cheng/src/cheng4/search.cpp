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

#include "search.h"
#include "movegen.h"
#include "thread.h"
#include "tune.h"
#include <algorithm>
#include <cassert>
#include <memory.h>
#include <cmath>

namespace cheng4
{

// SearchMode

void SearchMode::reset()
{
	moves.clear();
	mateSearch = 0;
	maxDepth = 0;
	maxTime = 0;
	maxNodes = 0;
	multiPV = 1;
	absLimit = 0;
	ponder = 0;
	fixedTime = 0;
}

// SearchInfo

void SearchInfo::reset()
{
	flags = 0;
}

// Search::RootMoves

Search::RootMoves &Search::RootMoves::operator =( const RootMoves &o )
{
	discovered = o.discovered;
	count = o.count;
	for ( size_t i=0; i<count; i++ ) {
		moves[i] = o.moves[i];
		sorted[i] = moves + (o.sorted[i] - o.moves);
	}
	bestMove = o.bestMove;
	bestScore = o.bestScore;
	return *this;
}

// Search

enum SearchOpts
{
	useLMR		=	1,
	useNull		=	1,
	useRazoring	=	1,
	useFutility	=	1
};

// verbose limit (currently none)
static const i32 verboseLimit = 1000;
// only start sending currmove after this limit
static const i32 currmoveLimit = 1000;

// beta razoring margins
static TUNE_CONST Score betaMargins[] = {
	0, 100, 150, 250, 500
};

// futility margins
static TUNE_CONST Score futMargins[] = {
	0, 100, 150, 250, 500
};

// razoring margins
static TUNE_CONST Score razorMargins[] = {
	0, 150, 200, 250
};

// timed out?
bool Search::timeOut()
{
	++timeOutCounter;
	if ( !(timeOutCounter &= 1023) )
	{
		// check time
		i32 ms = Timer::getMillisec();
		if ( !mode.ponder || ponderHit )
		{
			if ( mode.maxTime )
			{
				i32 delta = ms - startTicks;
				if ( delta >= mode.maxTime )
					return 1;
			}
			if ( mode.maxNodes && nodes >= mode.maxNodes )
				return 1;
		}
		if ( ms - nodeTicks >= 1000 )
		{
			// report nodes now
			i32 dt = ms - startTicks;
			info.reset();
			info.flags |= sifNodes | sifNPS;
			info.nodes = smpNodes();
			info.nps = dt ? info.nodes * 1000 / dt : 0;
			sendInfo();
			nodeTicks = ms;
		}
	}
	return 0;
}

template< bool pv, bool incheck > Score Search::qsearch( Ply ply, Depth depth, Score alpha, Score beta )
{
	assert( incheck == board.inCheck() );
	assert( alpha >= -scInfinity && beta <= scInfinity && alpha < beta );

	if ( aborting | abortingSmp )
		return scInvalid;

	uint pvIndex = initPly<pv>( ply );
	if (!pv)
		(void)pvIndex;

	// update selective depth
	if ( ply > selDepth )
		selDepth = ply;

	// mate distance pruning
	alpha = std::max( alpha, ScorePack::checkMated(ply) );
	beta = std::min( beta, ScorePack::mateIn(ply) );
	if ( alpha >= beta )
		return alpha;

	// check for timeout
	if ( !(searchFlags & sfNoTimeout) && timeOut() )
	{
		aborting = 1;
		return scInvalid;
	}

	// check for draw first
	if ( isDraw() )
		return scDraw;

	// maximum ply reached?
	if ( ply >= maxPly )
		return scDraw;

	bool qchecks = !incheck && !depth;
#ifndef USE_TUNING
	Depth ttDepth = qchecks ? 0 : -1;

	Score ttScore = tt->probe( board.sig(), ply, ttDepth, alpha, beta, stack[ply].killers.hashMove );
	if ( !pv && ttScore != scInvalid )
	{
		assert( ScorePack::isValid( ttScore ) );
		stack[ply].current = stack[ply].killers.hashMove;
		return ttScore;
	}
#endif

	Score oalpha;
	if ( pv )
		oalpha = alpha;

	Score ev;
	Score best;
	if ( incheck )
		best = -scInfinity;
	else
	{
		best = ev =  eval.eval( board, alpha, beta );

#ifndef NDEBUG
		Board tb(board);
		tb.swap();
		// FIXME: ok?
		Score sev = eval.eval( tb, -beta, -alpha );
		if ( sev != ev )
		{
			eval.clear();
			ev =  eval.eval( board, alpha, beta );
			sev = eval.eval( tb, -beta, -alpha );
			std::cout << "eval_symmetry_bug!" << std::endl;
			board.dump();
			tb.dump();
		}
#endif

		assert( !ScorePack::isMate( ev ) );
		if ( best >= beta )
			return best;			// stand pat
		if ( best > alpha )
			alpha = best;
	}

	// qsearch limit
	if ( !incheck && depth < minQsDepth )
		return best;

	MoveGen mg( board, stack[ply].killers, history, qchecks ? mmQCapsChecks : mmQCaps );
	Move m;
	Move bestMove = mcNone;

	size_t count = 0;
	while ( (m = mg.next()) != mcNone )
	{
		stack[ ply ].current = m;
		count++;

		bool ischeck = board.isCheck( m, mg.discovered() );

		// delta/qsearch futility
#ifndef USE_TUNING
		if ( useFutility && !pv && !incheck && !ischeck && MovePack::isCapture(m) )
		{
			Score fscore = ev + board.moveGain( m );
			if ( fscore + 200 <= alpha )
				continue;
		}
#endif

		UndoInfo ui;
		board.doMove( m, ui, ischeck );
		rep.push( board.sig(), !board.fifty() );

		Score score = ischeck ?
			-qsearch< pv, 1 >( ply+1, depth-1, -beta, -alpha ) :
			-qsearch< pv, 0 >( ply+1, depth-1, -beta, -alpha );

		rep.pop();
		board.undoMove( ui );

		if ( aborting | abortingSmp )
			return scInvalid;

		if ( score > best )
		{
			best = score;
			if ( score > alpha )
			{
				bestMove = m;
				alpha = score;
				if (pv)
				{
					// copy underlying PV
					triPV[pvIndex] = bestMove;
					copyPV(ply);
				}
				if ( score >= beta )
				{
					// no history here => depth is <= 0
					if ( !MovePack::isSpecial( m ) )
						stack[ply].killers.addKiller( m );
#ifndef USE_TUNING
					tt->store( board.sig(), age, m, score, btLower, ttDepth, ply );
#endif
					return score;
				}
			}
		}
	}

	if ( incheck && !count )
		// checkmated
		return ScorePack::checkMated( ply );

	assert( best > -scInfinity );

#ifndef USE_TUNING
	tt->store( board.sig(), age, bestMove, best, (HashBound)(pv ? (best > oalpha ? btExact : btUpper) : btUpper),
		ttDepth, ply );
#endif

	return best;
}

template< bool pv, bool incheck, bool donull >
	Score Search::search( Ply ply, FracDepth fdepth, Score alpha, Score beta )
{
	assert( incheck == board.inCheck() );
	assert( alpha >= -scInfinity && beta <= scInfinity && alpha < beta );

	if ( aborting | abortingSmp )
		return scInvalid;

	Depth depth = (Depth)(fdepth / fracOnePly);

	// qsearch
	if ( depth <= 0 )
		return qsearch< pv, incheck >( ply, 0, alpha, beta );

	uint pvIndex = initPly<pv>( ply );
	if (!pv)
		(void)pvIndex;

	// update selective depth
	if ( ply > selDepth )
		selDepth = ply;

	// mate distance pruning
	alpha = std::max( alpha, ScorePack::checkMated(ply) );
	beta = std::min( beta, ScorePack::mateIn(ply) );
	if ( alpha >= beta )
		return alpha;

	// check for timeout
	if ( !(searchFlags & sfNoTimeout) && timeOut() )
	{
		aborting = 1;
		return scInvalid;
	}

	// check for draw first
	if ( isDraw() )
		return scDraw;

	// maximum ply reached?
	if ( ply >= maxPly )
		return scDraw;

	// probe hashtable
	TransEntry lte;
	Score ttScore = tt->probe( board.sig(), ply, depth, alpha, beta, stack[ply].killers.hashMove, lte );
	if ( !pv && ttScore != scInvalid )
	{
		assert( ScorePack::isValid( ttScore ) );
		stack[ply].current = stack[ply].killers.hashMove;
		return ttScore;					// tt cutoff
	}

	Score fscore;
	if ( !pv && !incheck )
	{
		fscore = eval.eval(board);
		// use more precise tt score if possible
		Score ttBetter = TransTable::probeEval( board.sig(), ply, fscore, lte );
		if ( ttBetter != scInvalid )
			fscore = ttBetter;
		// beta razoring
		Score razEval;
		if ( useRazoring && depth <= 4 && (razEval = fscore - betaMargins[depth]) > alpha && !ScorePack::isMate(beta) )
			return razEval;
	}

	if ( useRazoring && !pv && !incheck && depth <= 3 && stack[ply].killers.hashMove == mcNone &&
		!ScorePack::isMate(alpha) )
	{
		// razoring
		Score margin = razorMargins[depth];

		Score razEval = fscore;
		if ( razEval + margin < alpha )
		{
			Score scout = alpha - margin;
			Score score = qsearch< 0, 0 >( ply, 0, scout-1, scout );
			if ( score < scout )
				return score;
		}
	}

	if ( useNull && !pv && !incheck && donull && depth > 1
		&& board.canDoNull() && fscore > alpha && !(searchFlags & sfNoNullMove) )
	{
		// null move pruning
		Depth R = 2 + depth/4;

		UndoInfo ui;
		board.doNullMove( ui );
		stack[ ply ].current = mcNull;

		rep.push( board.sig(), 1 );
		Score score = -search< 0, 0, 0 >( ply+1, (depth-R-1) * fracOnePly, -beta, -alpha);
		rep.pop();

		board.undoNullMove( ui );

		if ( score >= beta )
		{
			if ( depth <= 6 )
				return ScorePack::isMate(score) ? beta : score;

			// using nullmove reductions instead!
			depth = depth*2/3;
			fdepth = fdepth*2/3;
			if ( depth <= 0 )
				return qsearch< pv, incheck >( ply, 0, alpha, beta );
		}
	}

	// bench-tuned depths (not a great idea but still)
	if ( pv && depth > 2 && stack[ply].killers.hashMove == mcNone )
	{
		// IID at pv nodes
		search< pv, incheck, 0 >( ply, (depth/3) * fracOnePly, alpha, beta );
		tt->probe( board.sig(), ply, depth, alpha, beta, stack[ply].killers.hashMove );
	}
	if ( !pv && depth > 8 && stack[ply].killers.hashMove == mcNone )
	{
		// IID at nonpv nodes
		search< pv, incheck, 0 >( ply, (depth/3) * fracOnePly, alpha, beta );
		tt->probe( board.sig(), ply, depth, alpha, beta, stack[ply].killers.hashMove );
	}

	Score best = -scInfinity;

	Score oalpha;
	if ( pv )
		oalpha = alpha;

	MoveGen mg( board, stack[ply].killers, history, mmNormal );
	Move m;
	Move bestMove = mcNone;
	size_t count = 0;			// move count
	size_t lmrCount = 0;
	Move failHist[maxMoves];
	MoveCount failHistCount = 0;

	while ( (m = mg.next()) != mcNone )
	{
		stack[ ply ].current = m;
		count++;

		if ( !MovePack::isSpecial(m) && mg.phase() >= mpQuietBuffer )
			lmrCount++;

		if ( !MovePack::isSpecial( m ) )
			failHist[ failHistCount++ ] = m;

		bool ischeck = board.isCheck( m, mg.discovered() );

		Score score;

		// extend
		FracDepth extension = std::min( (FracDepth)fracOnePly, extend<pv>( m, ischeck, mg.discovered() ) );
		FracDepth newDepth = fdepth - fracOnePly + extension;

		if ( useFutility && !pv && !incheck && mg.phase() >= mpQuietBuffer &&
			!extension && depth <= 4 && !MovePack::isSpecial(m) && !ScorePack::isMate(beta) &&
			board.canPrune(m) )
		{
			// futility pruning
			Score futScore = fscore + futMargins[depth] - (Score)(20*lmrCount);
			if ( futScore <= alpha )
				continue;
		}

		i32 hist;
		if ( !incheck )
			hist = history.score(board, m);

		UndoInfo ui;
		board.doMove( m, ui, ischeck );
		rep.push( board.sig(), !board.fifty() );

		score = alpha+1;
		if ( pv && count > 1 )
		{
			if ( useLMR && !incheck && lmrCount >= 3 && mg.phase() >= mpQuietBuffer && !MovePack::isSpecial(m)
				&& (depth > 6 || hist <= 0) && !ischeck && depth > 2 && !extension && board.canReduce(m) )
			{
				// LMR at pv nodes
				stack[ ply ].reduction = (FracDepth)( fracOnePly*std::min((size_t)3, lmrCount/3 ) );
				score = -search< 0, 0, 1 >( ply+1, newDepth - stack[ ply ].reduction, -alpha-1, -alpha );
				stack[ ply ].reduction = 0;
			}

			if ( score > alpha )
				score = ischeck ?
					-search< 0, 1, 1 >( ply+1, newDepth, -alpha-1, -alpha ) :
					-search< 0, 0, 1 >( ply+1, newDepth, -alpha-1, -alpha );
		}
		// new: reduce bad captures as well
		if ( useLMR && !pv && !incheck && lmrCount >= 3 && mg.phase() >= mpQuietBuffer &&
			/*!MovePack::isSpecial(m) &&*/ (depth > 6 || hist <= 0) && !ischeck && depth > 2 &&
			!extension && board.canReduce(m) )
		{
			// LMR at nonpv nodes
			stack[ ply ].reduction = (FracDepth)( fracOnePly*std::min((size_t)3, lmrCount/3) );
			score = -search< 0, 0, 1 >( ply+1, newDepth - stack[ ply ].reduction, -alpha-1, -alpha );
			stack[ ply ].reduction = 0;
		}

		if ( score > alpha )
			score = ischeck ?
				-search< pv, 1, !pv >( ply+1, newDepth, -beta, -alpha ) :
				-search< pv, 0, !pv >( ply+1, newDepth, -beta, -alpha );

		rep.pop();
		board.undoMove( ui );

		if ( aborting | abortingSmp )
			return scInvalid;

		if ( score > best )
		{
			best = score;
			if ( score > alpha )
			{
				bestMove = m;
				alpha = score;
				if (pv)
				{
					// copy underlying PV
					triPV[pvIndex] = bestMove;
					copyPV(ply);
				}
				if ( score >= beta )
				{
					if ( !MovePack::isSpecial( m ) )
					{
						stack[ply].killers.addKiller( m );
						history.add( board, m, depth );
						assert( failHistCount > 0 );
						// this useless if is here only to silence msc static analyzer
						if ( failHistCount > 0 )
							failHistCount--;
					}
					for (MoveCount i=0; i<failHistCount; i++)
						history.add( board, failHist[i], -depth);
					tt->store( board.sig(), age, m, score, btLower, depth, ply );
					return score;
				}
			}
		}
	}

	if ( !count )
		// stalemate or checkmate
		return incheck ? ScorePack::checkMated(ply) : scDraw;

	// very important -- we pruned some moves but it's possible that we didn't actually try a move
	// in order to avoid storing wrong mate score to tt, this is necessary
	if ( best == -scInfinity )
	{
		best = alpha;
	}

	assert( best > -scInfinity );

	tt->store( board.sig(), age, bestMove, best,
		(HashBound)(!pv ? btUpper :
		(best > oalpha ? btExact : btUpper)), depth, ply );

	return best;
}

Search::Search( size_t evalKilo, size_t pawnKilo, size_t matKilo ) : startTicks(0), nodeTicks(0),
	timeOutCounter(0), triPV(0), newMultiPV(0), selDepth(0), tt(0), nodes(0), age(0), callback(0),
	callbackParam(0), canStop(0), abortRequest(0), aborting(0), abortingSmp(0),
	outputBest(1), ponderHit(0), maxThreads(63), eloLimit(0), maxElo(2500),
	minQsDepth(-maxDepth), verbose(1), verboseFixed(1), searchFlags(0), startSearch(0), master(0)
{
	board.reset();
	mode.reset();
	info.reset();
	for ( int i=0; i<maxMoves; i++ )
		infoPV[i].reset();
	iterBest = iterPonder = mcNone;

	assert( evalKilo * 1024 / 1024 == evalKilo );
	assert( pawnKilo * 1024 / 1024 == pawnKilo );
	assert( matKilo * 1024 / 1024 == matKilo );
	eval.resizeEval( evalKilo * 1024 );
	eval.resizePawn( pawnKilo * 1024 );
	eval.resizeMaterial( matKilo * 1024 );
	eval.clear();

	// init stack
	memset( stack, 0, sizeof(stack) );
	// allocate triPV
	triPV = new Move[maxTriPV];
	memset( triPV, 0, sizeof(Move)*maxTriPV );

	memset( &rootMoves, 0, sizeof(rootMoves) );

	history.clear();
}

Search::~Search()
{
	setThreads(0);
	delete[] triPV;
}

void Search::setHashTable(cheng4::TransTable *tt_)
{
	tt = tt_;
}

void Search::clearHash()
{
	assert( tt );
	tt->clear();
}

void Search::clearSlots( bool clearEval )
{
	if ( clearEval )
		eval.clear();
	history.clear();
	memset( stack, 0, sizeof(stack) );
}

void Search::sendPV( const RootMove &rm, Depth depth, Score score, Score alpha, Score beta, uint mpvindex )
{
	if ( verbose || !verboseFixed )
	{
		i32 dt = Timer::getMillisec() - startTicks;
		SearchInfo &si = verbose ? info : infoPV[mpvindex];

		si.reset();
		// depth required by stupid UCI
		si.flags |= sifDepth | sifSelDepth | sifPV | sifTime | sifNodes | sifNPS;
		si.pvScore = score;
		si.pvBound = (score >= beta) ? btLower : (score <= alpha) ? btUpper : btExact;
		si.pvIndex = mpvindex;
		si.pvCount = rm.pvCount;
		si.pv = rm.pv;
		si.nodes = smpNodes();
		si.nps = dt ? si.nodes * 1000 / dt : 0;
		si.time = (Time)dt;
		Ply sd = selDepth;
		for (size_t i=0; i<smpThreads.size(); i++)
			sd = std::max( sd, smpThreads[i]->search.selDepth);
		si.depth = depth;
		si.selDepth = (Ply)(sd+1);
		if ( verbose )
			sendInfo();
	}
}

void Search::flushCachedPV( size_t totalMoves )
{
	for ( size_t i=0; i<totalMoves; i++ ) {
		SearchInfo &si = infoPV[i];
		if ( si.flags ) {
			sendInfo( si );
			si.reset();
		}
	}
}

// do root search
Score Search::root( Depth depth, Score alpha, Score beta )
{
	// ply = 0 here

	// qsearch explosion guard
	minQsDepth = (Depth)-std::min( (int)maxDepth, (int)(depth*3));

	rootMoves.bestMove = mcNone;
	rootMoves.bestScore = scInvalid;

	initPly<1>( 0 );

	Score oalpha = alpha;

	FracDepth fd = (FracDepth)depth << fracShift;

	Score best = scInvalid;
	Move bestm = mcNone;

	// first thing to do: sort root moves
	std::stable_sort( rootMoves.sorted, rootMoves.sorted + rootMoves.count, rootPred );
	for (size_t i=0; i<rootMoves.count; i++)
	{
		RootMove &rm = *rootMoves.sorted[i];
		if ( i >= mode.multiPV )
			rm.score = -scInfinity;
	}

	size_t count = 0;

	for (size_t i=0; i<rootMoves.count; i++)
	{
		count++;
		RootMove &rm = *rootMoves.sorted[i];

		if ( verbose )
		{
			i32 dt = Timer::getMillisec() - startTicks;
			if ( dt >= currmoveLimit )
			{
				info.reset();
				info.flags |= sifCurIndex | sifCurMove;
				info.curIndex = (MoveCount)i;
				info.curCount = (MoveCount)rootMoves.count;
				info.curMove = rm.move;
				sendInfo();
			}
		}

		NodeCount onodes = nodes;

		Score score;

		bool isCheck = board.isCheck( rm.move, rootMoves.discovered );

		// extend
		FracDepth extension = extend<1>( rm.move, isCheck, rootMoves.discovered );
		FracDepth newDepth = fd - fracOnePly + extension;

		UndoInfo ui;

		board.doMove( rm.move, ui, isCheck );
		rep.push( board.sig(), !board.fifty() );

		score = alpha+1;
		if ( count > mode.multiPV )
		{
			score = isCheck ?
				-search< 0, 1, 1 >( 1, newDepth, -alpha-1, -alpha ) :
				-search< 0, 0, 1 >( 1, newDepth, -alpha-1, -alpha );
		}
		if ( score > alpha )
		{
			score = isCheck ?
				-search< 1, 1, 0 >( 1, newDepth, -beta, -alpha ) :
				-search< 1, 0, 0 >( 1, newDepth, -beta, -alpha );
		}
		rep.pop();
		board.undoMove( ui );

		if ( aborting )
			return scInvalid;
		if ( abortingSmp )
			break;

		rm.nodes = nodes - onodes;

		if ( count == 1 && score <= alpha )
		{
			// FIXME: break here?!
			rootMoves.bestMove = triPV[0] = rm.move;
			triPV[1] = mcNone;
			// extract pv now
			extractPV( rm );
			sendPV( rm, depth, score, oalpha, beta );
			if ( master )
			{
				master->abortingSmp = 1;
				rm.score = score;
			}
			return score;		// early exit => fail low!
		}

		if ( score > best )
		{
			bestm = rm.move;
			best = score;
		}
		if ( score > alpha )
		{
			// copy underlying PV
			triPV[0] = rm.move;
			copyPV(0);

			alpha = score;
			rm.score = score;

			// extract pv now
			extractPV( rm );

			if ( mode.multiPV <= 1 )
			{
				sendPV( rm, depth, score, oalpha, beta );

				// make sure rm is first now!
				for (size_t j=i; j>0; j--)
					rootMoves.sorted[j] = rootMoves.sorted[j-1];
				rootMoves.sorted[0] = &rm;
			}
			else
			{
				// special (multipv mode)
				// first, determine if this move makes it into new multipv
				// if not, don't do anything
				uint mpv = std::min( (uint)count, mode.multiPV );
				uint pvcount = 0;
				bool ok = 0;
				for ( size_t j=0; j<mpv; j++ )
				{
					Score mscore = rootMoves.sorted[j]->score;
					if ( mscore != -scInfinity )
						pvcount++;
					if ( score >= mscore )
						ok = 1;
				}
				mpv = std::min( mpv, pvcount );
				if ( ok )
				{
					// yes, we're updating multipv move score
					std::stable_sort( rootMoves.sorted, rootMoves.sorted + count, rootPred );
					// FIXME: if in xboard mode, could send the PV right away
					// doing the stupid UCI stuff shouldn't hurt probably
					if ( mpv >= mode.multiPV )
					{
						// send PVs
						for (size_t j=0; j<mpv; j++)
						{
							sendPV(*rootMoves.sorted[j], depth, rootMoves.sorted[j]->score,
								-scInfinity, scInfinity, (uint)j);
						}
					}
					// make sure alpha is up to date
					if ( mpv < mode.multiPV )
						alpha = -scInfinity;
					else
					{
						// we're really only interested in moves that beat worst multiPV move
						Score newAlpha = scInfinity;
						for (size_t j=0; j<mpv; j++)
							newAlpha = std::min( newAlpha, rootMoves.sorted[j]->score );
						alpha = newAlpha;
					}
				}
			}

			// set -inf score to uninteresting moves
			for (size_t j=mode.multiPV; j<rootMoves.count; j++)
					rootMoves.sorted[j]->score = -scInfinity;

			if ( score >= beta )
			{
				// cutoff
				rootMoves.bestMove = bestm;
				rootMoves.bestScore = best;

				// don't store if we're searching a subset of root moves!
				if ( mode.moves.empty() )
					tt->store( board.sig(), age, bestm, best, btLower, depth, 0 );

				if ( master )
					master->abortingSmp = 1;

				return best;
			}
		}
	}

	if ( abortingSmp )
	{
		for (size_t i=0; i<smpThreads.size(); i++)
		{
			const RootMoves &rm = smpThreads[i]->search.rootMoves;
			if ( rm.bestMove == mcNone )
				continue;
			best = rm.bestScore;
			rootMoves = rm;

			// we have to reset cached pvs if any
			for (size_t i=0; i<rootMoves.count; i++)
				infoPV[i].reset();

			if ( rootMoves.count )
				for (uint j=0; j<mode.multiPV; j++)
					sendPV( *rootMoves.sorted[j], depth, rootMoves.sorted[j]->score, oalpha, beta, j );

			// FIXME: break here?
			return best;
		}
		assert( bestm != mcNone );
		if ( bestm == mcNone )
			return scInvalid;				// paranoid
	}

	rootMoves.bestMove = bestm;
	rootMoves.bestScore = best;

	// don't store if we're searching a subset of root moves!
	if ( rootMoves.count && mode.moves.empty() )
		tt->store( board.sig(), age, bestm, best, (HashBound)(best > oalpha ? btExact : btUpper), depth, 0 );

	if ( master )
		master->abortingSmp = 1;

	return best;
}

i32 Search::initIteration()
{
	i32 sticks = Timer::getMillisec();

	iterBest = iterPonder = mcNone;
	canStop = 0;
	abortRequest = 0;
	aborting = 0;
	outputBest = 1;
	ponderHit = 0;
	verbose = 0;
	verboseFixed = 1;

	timeOutCounter = 1023;
	nodeTicks = startTicks = sticks;

	nodes = 0;

	// increment age
	age++;

	return sticks;
}

Score Search::iterate( Board &b, const SearchMode &sm, bool nosendbest )
{
	assert( tt );

	Score res = scInvalid;

	selDepth = 0;
	for (size_t i=0; i<smpThreads.size(); i++)
		smpThreads[i]->search.selDepth = 0;

	initIteration();
	startSearch.signal();

	// don't output anything if we should think for a limited amount of time
	verbose = verboseFixed = !sm.maxTime;

	board = b;
	mode = sm;

	Killer killers(0);
	History hist(0);

	rootMoves.moves[0].move = mcNone;
	rootMoves.count = 0;

	// init with hashmove if available
	tt->probe( b.sig(), 0, 0, scDraw, scDraw, killers.hashMove );

	// init root moves
	MoveGen mg( b, killers, hist );
	Move m;
	rootMoves.discovered = mg.discovered();

	while ( (m = mg.next()) != mcNone )
	{
		if ( !sm.moves.empty() && std::find(sm.moves.begin(), sm.moves.end(), m) == sm.moves.end() )
			continue;
		if ( !verboseFixed )
			infoPV[ rootMoves.count ].reset();
		RootMove &rm = rootMoves.moves[ rootMoves.count++ ];
		rm.nodes = 0;
		rm.score = -scInfinity;
		rm.move = m;
		rm.pv[0] = mcNone;
		rm.pvCount = 0;
	}

	for (size_t i=0; i<rootMoves.count; i++)
		rootMoves.sorted[i] = rootMoves.moves + i;

	Depth depthLimit = maxDepth;

	if ( mode.maxTime && rootMoves.count == 1 && sm.moves.empty() ) {
		// play only move fast. using depth 2 to have (at least) something to ponder on
		depthLimit = 2;
	}

	if ( mode.maxDepth )
		depthLimit = std::min( depthLimit, mode.maxDepth );

	// limit multiPV to number of available moves!
	mode.multiPV = std::min( mode.multiPV, (uint)rootMoves.count );

	smpSync();

	Score lastIteration = scDraw;		// last iteration score

	i32 lastIterationStart = startTicks;

	for ( Depth d = 1; rootMoves.count && d <= depthLimit; d++ )
	{
		i32 curTicks = Timer::getMillisec();
		i32 lastIterationDelta = curTicks - lastIterationStart;
		lastIterationStart = curTicks;

		// update multiPV on the fly if needed
		if ( newMultiPV )
		{
			// limit multiPV to number of available moves!
			mode.multiPV = newMultiPV;
			mode.multiPV = std::min( mode.multiPV, (uint)rootMoves.count );
			newMultiPV = 0;
		}

		i32 total = curTicks - startTicks;
		if ( !verboseFixed && !verbose && total >= verboseLimit )
		{
			// turning on verbose;
			verbose = 1;
			flushCachedPV( rootMoves.count );
		}

		if ( d > 1 && (!mode.ponder || ponderHit) && mode.maxTime && !mode.fixedTime )
		{
			// make sure we can at least finish first move on this iteration,
			// assuming it will take 50% of current iteration (=100% previous iteration)
			if ( total + lastIterationDelta > mode.maxTime )
				break;
		}

		i32 limitStart = 0;
		if ( eloLimit && maxElo < (u32)maxStrength )
			limitStart = curTicks;

		if ( verbose )
		{
			info.reset();
			info.flags |= sifDepth | sifTime;
			info.depth = d;
			info.time = (Time)total;
			sendInfo();
		}

		if ( d == 1 )
		{
			// depth 1: always full
			// disable timeout here
			abortingSmp = 0;
			enableTimeOut(0);
			res = lastIteration = root( d, -scInfinity, +scInfinity );
			enableTimeOut(1);
			canStop = 1;
			if ( abortRequest )
				aborting = 1;
		}
		else if ( mode.multiPV > 1 )
		{
			// lazySMP kicks in here
			smpStart( d, -scInfinity, scInfinity );
			res = root( d, -scInfinity, scInfinity );
			smpStop();
		}
		else
		{
			// aspiration search
			Score prevScore = lastIteration;

			Score alpha = lastIteration - 15;
			Score beta = lastIteration + 15;

			i32 maxTime = mode.maxTime;
			bool blunderCheck = 0;

			for (;;)
			{
				alpha = std::max( -scInfinity, alpha );
				beta = std::min( +scInfinity, beta );
				assert( alpha < beta );

				// lazySMP kicks in here
				smpStart( d, alpha, beta );
				Score score = root( d, alpha, beta );
				smpStop();

				if ( aborting )
					break;

				res = score;

				if ( score > alpha && score < beta )
				{
					lastIteration = score;
					break;
				}
				if ( score <= alpha )
				{
					// fail low
					alpha = (alpha - lastIteration)*2;
					alpha += lastIteration;
					if ( abs( score - prevScore ) >= 30 )
					{
						// blunder warning => give more time to resolve iteration (if possible)
						blunderCheck = 1;
						mode.maxTime = mode.absLimit;
					}
				}
				else
				{
					// fail high
					beta = (beta - lastIteration)<<1;
					beta += lastIteration;
				}
			}
			if ( blunderCheck )
				mode.maxTime = maxTime;
		}
		if ( eloLimit && maxElo < (u32)maxStrength )
		{
			// we're in elo limit mode elo => add artificial slowdown!
			i32 ticks = Timer::getMillisec();
			i32 delta = ticks - limitStart;
			i32 slowdown = (i32)maxStrength - (i32)maxElo;
			// -100 = 2x slower
			// -200 = 4x slower
			// -300 = 8x slower
			// -400 = 16x slower
			// -500 = 32x slower
			// -600 = 64x slower
			// -700 = 128x slower
			// -800 = 256x slower
			// -900 = 512x slower
			// -1000 = 1024x slower
			// -1100 = 2048x slower
			// -1200 = 4096x slower
			// -1300 = 8192x slower
			// -1400 = 16384x slower
			// -1500 = 32768x slower
			// -1600 = 65536x slower
			// -1700 = 131072x slower
			// .. etc
			i32 delay = (i32)(pow(2.0, slowdown/100.0) * delta);
			delay -= delta + 1;			// adjust for sleep granularity
			if ( sm.maxTime && ticks + delay - startTicks >= sm.maxTime )
				aborting = 1;			// early exit => we won't be able to reach next iteration anyway
			while ( delay > 0 && !aborting && eloLimit && maxElo < (u32)maxStrength  )
			{
				Thread::sleep(1);
				// hack to force timeout check
				timeOutCounter = 1023;
				if ( timeOut() )
				{
					aborting = 1;
					break;
				}
				i32 current = Timer::getMillisec() - ticks;
				if ( current >= delay )
					break;
			}
		}
		if ( aborting )
			break;
	}

	flushCachedPV( rootMoves.count );

	if ( mode.ponder )
	{
		// wait for stop or ponderhit!
		// FIXME: better! (use event)
		while ( !aborting && !ponderHit )
			Thread::sleep(1);
	}

	// finally report total nodes and time
	if ( verbose )
	{
		i32 dt = Timer::getMillisec() - startTicks;
		info.reset();
		info.flags |= sifTime | sifNodes | sifNPS;
		info.nodes = smpNodes();
		info.nps = dt ? info.nodes * 1000 / dt : 0;
		info.time = (Time)dt;
		sendInfo();
	}

	if ( outputBest )
	{
		// return best move and ponder move (if available)
		if ( !rootMoves.sorted[0] )
		{
			iterBest = iterPonder = mcNone;
			if ( !nosendbest )
				sendBest();
			return res;
		}
		const RootMove &rm = *rootMoves.sorted[0];
		iterBest = rm.move;
		if ( rm.pvCount > 1 )
			iterPonder = rm.pv[1];
		if ( iterPonder == mcNone && iterBest != mcNone && rootMoves.count )
		{
			// we don't have a move to ponder on!
			// this can happen if we're in the middle of resolving a fail-low or fail-high and time is up
			// so try to extract it from hashtable!
			iterPonder = extractPonderFromHash( iterBest );
		}
		if ( !nosendbest )
			sendBest();
	}
	return res;
}

void Search::getBest( SearchInfo  &sinfo )
{
	sinfo.reset();
	sinfo.flags |= sifBestMove;
	sinfo.bestMove = iterBest;
	if ( iterPonder != mcNone )
	{
		sinfo.flags |= sifPonderMove;
		sinfo.ponderMove = iterPonder;
	}
	iterBest = iterPonder = mcNone;
}

void Search::sendBest()
{
	getBest( info );
	sendInfo();
}

void Search::setCallback( SearchCallback cbk, void *param )
{
	callback = cbk;
	callbackParam = param;
}

// execute info callback
void Search::sendInfo( const SearchInfo &sinfo ) const
{
	if ( !sinfo.flags || !callback )
		return;
	callback( sinfo, callbackParam );
}

void Search::sendInfo() const
{
	sendInfo( info );
}

// copy underlying PV
void Search::copyPV( Ply ply )
{
	uint index = triIndex(ply);
	Move *dst = triPV + index + 1;
	const Move *src = triPV + index + (maxPV-ply);
	assert( index + (maxPV-ply) == triIndex(ply+1) );
	do
	{
		*dst++ = *src;
	} while ( *src++ );
}

// extract PV for rm
void Search::extractPV( RootMove &rm ) const
{
	rm.pvCount = 0;
	const Move *src = triPV;
	assert( *src );
	while ( *src )
		rm.pv[ rm.pvCount++ ] = *src++;
	rm.pv[ rm.pvCount ] = mcNone;
}

Move Search::extractPonderFromHash( Move best )
{
	Move ponder;
	UndoInfo ui;
	board.doMove( best, ui, board.isCheck( best, board.discovered()) );
	tt->probe(board.sig(), 0, 0, scDraw, scDraw, ponder);
	if ( ponder != mcNone )
	{
		// found - now make sure it's legal
		bool legal = board.inCheck() ? board.isLegal< 1, 0 >(ponder, board.pins())
					: board.isLegal< 0, 0 >(ponder, board.pins());
		if ( !legal )
			ponder = mcNone;
	}
	board.undoMove(ui);
	return ponder;
}

// set helper threads
void Search::setThreads( size_t nt )
{
	if ( nt > maxThreads )
		nt = maxThreads;
	if ( smpThreads.size() == nt )
		return;
	for ( size_t i=0; i < smpThreads.size(); i++)
		smpThreads[i]->kill();
	smpThreads.clear();
	for ( size_t i=0; i < nt; i++)
	{
		LazySMPThread *smpt = new LazySMPThread;
		smpt->search.master = this;
		smpt->search.setHashTable( tt );
		smpt->run();
		smpThreads.push_back( smpt );
	}
}

// set maximum # of helper threads
void Search::setMaxThreads( size_t maxt )
{
	maxThreads = maxt;
	setThreads( smpThreads.size() );
}

// set multipv (can also be called while analyzing!)
void Search::setMultiPV( uint mpv )
{
	newMultiPV = mpv;
}

void Search::setEloLimit( bool limit )
{
	eloLimit = limit;
}

void Search::setMaxElo( u32 elo )
{
	maxElo = elo;
}

// enable timeout
void Search::enableTimeOut( bool enable )
{
	if ( !enable )
		searchFlags |= sfNoTimeout;
	else
		searchFlags &= ~sfNoTimeout;
}


// enable nullmove
void Search::enableNullMove( bool enable )
{
	if ( !enable )
		searchFlags |= sfNoNullMove;
	else
		searchFlags &= ~sfNoNullMove;
}

void Search::smpStart( Depth depth, Score alpha, Score beta )
{
	abortingSmp = 0;
	for ( size_t i=0; i<smpThreads.size(); i++ )
		smpThreads[i]->start( depth + (Depth)((i&1)^1), alpha, beta, *this );
}

void Search::smpStop()
{
	for ( size_t i=0; i<smpThreads.size(); i++ )
		smpThreads[i]->abort();
}

void Search::smpSync() const
{
	for ( size_t i=0; i<smpThreads.size(); i++)
	{
		// synchronize smp threads
		assert( !smpThreads[i]->searching );
		Search &s = smpThreads[i]->search;
		s.initIteration();
		s.age = age;
		s.board = board;
		// FIXME: better?
		s.rep.copyFrom(rep);
		s.history = history;
		s.rootMoves = rootMoves;
		// never use timeout for smp helper threads!
		s.searchFlags = searchFlags | sfNoTimeout;
	}
}

// static init
void Search::init()
{
	TUNE_EXPORT(Score, razorMargin1, razorMargins[1]);
	TUNE_EXPORT(Score, razorMargin2, razorMargins[2]);
	TUNE_EXPORT(Score, razorMargin3, razorMargins[3]);

	TUNE_EXPORT(Score, futMargin1, futMargins[1]);
	TUNE_EXPORT(Score, futMargin2, futMargins[2]);
	TUNE_EXPORT(Score, futMargin3, futMargins[3]);
	TUNE_EXPORT(Score, futMargin4, futMargins[4]);

	TUNE_EXPORT(Score, betaMargin1, betaMargins[1]);
	TUNE_EXPORT(Score, betaMargin2, betaMargins[2]);
	TUNE_EXPORT(Score, betaMargin3, betaMargins[3]);
	TUNE_EXPORT(Score, betaMargin4, betaMargins[4]);
}

// LazySMPThread

LazySMPThread::LazySMPThread() : searching(0), shouldQuit(0)
{
	memset( &commandData, 0, sizeof(commandData) );
}

void LazySMPThread::destroy()
{
	search.abort(1);
	shouldQuit = 1;
	commandEvent.signal();
	quitEvent.wait();
}

void LazySMPThread::work()
{
	SearchMode sm;
	sm.reset();
	search.mode = sm;

	for (;;)
	{
		commandEvent.wait();
		// search or quit!
		if ( shouldQuit )
			break;

		Depth depth;
		Score alpha, beta;
		const CommandData &c = commandData;
		depth = c.depth;
		alpha = c.alpha;
		beta = c.beta;
		search.mode.multiPV = c.multiPV;
		search.rootMoves = c.rootMoves;
		search.abortRequest = 0;
		search.aborting = 0;
		search.rootMoves.bestMove = mcNone;
		searching = 1;
		startedSearch.signal();

		assert( search.searchFlags & sfNoTimeout );
		search.root( depth, alpha, beta );

		searching = 0;
		doneSearch.signal();
	}
	quitEvent.signal();
}

void LazySMPThread::abort()
{
	search.abort(1);
	doneSearch.wait();
}

void LazySMPThread::start( Depth depth, Score alpha, Score beta, const Search &master )
{
	CommandData &cd = commandData;
	cd.depth = depth;
	cd.alpha = alpha;
	cd.beta = beta;
	cd.rootMoves = master.rootMoves;
	cd.multiPV = master.mode.multiPV;

	commandEvent.signal();
	startedSearch.wait();
}

}
