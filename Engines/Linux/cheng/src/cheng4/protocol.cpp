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

#include "protocol.h"
#include "engine.h"
#include "version.h"
#include "movegen.h"
#include "thread.h"
#include "tune.h"
#include "utils.h"
#include "filterpgn.h"
#include <deque>
#include <cctype>
#include <algorithm>

// these because of pbook
#include <set>
#include <vector>
#include <iostream>
#include <fstream>

#ifdef IS_X64
#	define maxHash "16384"
#else
#	define maxHash	"1024"
#endif

// TODO:	- refactor into separate methods, simplify

namespace cheng4
{

static u64 pbook( std::ofstream &ofs, std::set< Signature > &procmap, Board &b, Search &s,
	std::vector<Move> &line, Ply p, Depth d )
{
	u64 res = 0;

	std::set< Signature >::const_iterator ci = procmap.find(b.sig());
	if ( ci != procmap.end() )
		return res;
	SearchMode sm;
	sm.reset();
	sm.maxDepth = 18;
	sm.multiPV = 16 >> p;
	if ( sm.multiPV > 8 )
		sm.multiPV = 8;
	if ( sm.multiPV < 2 )
		sm.multiPV = 2;
	if ( d == 1 )
		sm.multiPV = 1;
	s.clearHash();
	s.clearSlots();
	s.iterate(b, sm, 1);
	procmap.insert( b.sig() );

	std::vector< Move > moves;

	size_t num = std::min( (size_t)sm.multiPV, s.rootMoves.count );
	for (size_t i=0; i<num; i++)
	{
		if ( abs(s.rootMoves.sorted[i]->score) > 40 )
			continue;
		moves.push_back( s.rootMoves.sorted[i]->move );
	}
	if ( moves.empty() )
		return res;

	if ( d <= 0 )
	{
		// dump line
		std::string l;
		Board tb;
		tb.reset();
		for (size_t i=0; i<line.size(); i++)
		{
			l += tb.toSAN(line[i]);
			l += ' ';
			UndoInfo ui;
			bool isCheck = tb.isCheck( line[i], tb.discovered() );
			tb.doMove( line[i], ui, isCheck );
		}
		ofs << l << std::endl;
		std::cout << '.';
		return 1;
	}
	Bitboard dc = b.discovered();
	Move m;
	for ( size_t i=0; i<moves.size(); i++ )
	{
		m = moves[i];
		UndoInfo ui;
		bool isCheck = b.isCheck( m, dc );
		b.doMove( m, ui, isCheck );
		line.push_back( m );

		res += pbook( ofs, procmap, b, s, line, p+1, d-1 );

		line.pop_back();
		b.undoMove( ui );
	}
	return res;
}

static void pbook()
{
	std::set< Signature > okmap;
	TransTable tt;
	Search *s = new Search;
	tt.resize( 64*4096*1024 );	// 256 MB
	s->setHashTable( &tt );
	Board b;
	b.reset();
	std::ofstream ofs("booklines.txt");
	std::vector< Move > line;
	u64 res = pbook( ofs, okmap, b, *s, line, 0, 8 );
	if ( !ofs )
	{
		std::cout << "can't write outfile" << std::endl;
	}
	std::cout << "positions: " << res << std::endl;
	delete s;
}

static void runEPDSuite( const EPDFile &epd, Depth depth )
{
	TransTable tt;
	Search *s = new Search;
	tt.resize( 4096*1024 );
	s->setHashTable( &tt );
	size_t matching = 0;
	size_t total = epd.positions.size();
	for ( size_t i=0; i<total; i++ )
	{
		const EPDPosition &ep = epd.positions[i];
		s->clearSlots();
		s->clearHash();
		Board b;
		if ( !b.fromFEN( ep.fen.c_str() ) )
			continue;
		SearchMode sm;
		sm.reset();
		sm.maxDepth = depth;
		s->iterate( b, sm, 1 );
		Move bm = s->iterBest;
		size_t j;
		for ( j=0; j<ep.avoid.size(); j++ )
			if ( bm == ep.avoid[j] )
				break;
		if ( j < ep.avoid.size() )
			continue;					// avoid_failed
		for ( j=0; j<ep.best.size(); j++ )
			if ( bm == ep.best[j] )
				break;
		if ( (!ep.avoid.empty() && ep.best.empty()) || j < ep.best.size() )
		{
			// matching!
			matching++;
		}
	}
	std::cout << matching << " out of " << total << " positions passed" << std::endl;
	delete s;
}

struct PerfTest
{
	const char *fen;
	Depth depth;
	NodeCount count;
};

static const PerfTest suite[] =
{
	// my artificial perft suite:

	// avoid en passant capture:
	{	"8/5bk1/8/2Pp4/8/1K6/8/8 w - d6 0 1", 6, 824064ull },
	{	"8/8/1k6/8/2pP4/8/5BK1/8 b - d3 0 1", 6, 824064ull },
	// en passant capture checks opponent:
	{	"8/8/1k6/2b5/2pP4/8/5K2/8 b - d3 0 1", 6, 1440467ull },
	{	"8/5k2/8/2Pp4/2B5/1K6/8/8 w - d6 0 1", 6, 1440467ull },
	// short castling gives check:
	{	"5k2/8/8/8/8/8/8/4K2R w K - 0 1", 6, 661072ull },
	{	"4k2r/8/8/8/8/8/8/5K2 b k - 0 1", 6, 661072ull },
	// long castling gives check:
	{	"3k4/8/8/8/8/8/8/R3K3 w Q - 0 1", 6, 803711ull },
	{	"r3k3/8/8/8/8/8/8/3K4 b q - 0 1", 6, 803711ull },
	// castling (including losing cr due to rook capture):
	{	"r3k2r/1b4bq/8/8/8/8/7B/R3K2R w KQkq - 0 1", 4, 1274206ull },
	{	"r3k2r/7b/8/8/8/8/1B4BQ/R3K2R b KQkq - 0 1", 4, 1274206ull },
	// castling prevented:
	{	"r3k2r/8/3Q4/8/8/5q2/8/R3K2R b KQkq - 0 1", 4, 1720476ull },
	{	"r3k2r/8/5Q2/8/8/3q4/8/R3K2R w KQkq - 0 1", 4, 1720476ull },
	// promote out of check:
	{	"2K2r2/4P3/8/8/8/8/8/3k4 w - - 0 1", 6, 3821001ull },
	{	"3K4/8/8/8/8/8/4p3/2k2R2 b - - 0 1", 6, 3821001ull },
	// discovered check:
	{	"8/8/1P2K3/8/2n5/1q6/8/5k2 b - - 0 1", 5, 1004658ull },
	{	"5K2/8/1Q6/2N5/8/1p2k3/8/8 w - - 0 1", 5, 1004658ull },
	// promote to give check:
	{	"4k3/1P6/8/8/8/8/K7/8 w - - 0 1", 6, 217342ull },
	{	"8/k7/8/8/8/8/1p6/4K3 b - - 0 1", 6, 217342ull },
	// underpromote to check:
	{	"8/P1k5/K7/8/8/8/8/8 w - - 0 1", 6, 92683ull },
	{	"8/8/8/8/8/k7/p1K5/8 b - - 0 1", 6, 92683ull },
	// self stalemate:
	{	"K1k5/8/P7/8/8/8/8/8 w - - 0 1", 6, 2217ull },
	{	"8/8/8/8/8/p7/8/k1K5 b - - 0 1", 6, 2217ull },
	// stalemate/checkmate:
	{	"8/k1P5/8/1K6/8/8/8/8 w - - 0 1", 7, 567584ull },
	{	"8/8/8/8/1k6/8/K1p5/8 b - - 0 1", 7, 567584ull },
	// double check:
	{	"8/8/2k5/5q2/5n2/8/5K2/8 b - - 0 1", 4, 23527ull },
	{	"8/5k2/8/5N2/5Q2/2K5/8/8 w - - 0 1", 4, 23527ull },

	{	0, 0, 0	}
};

// TODO: more positions?
static const char *benchFens[] =
{
	"2kr2nr/pp1n1ppp/2p1p3/q7/1b1P1B2/P1N2Q1P/1PP1BPP1/R3K2R w KQ - 1",
	"rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",
	"r5n1/p2q3k/4b2p/3pB3/2PP1QpP/8/PP4P1/5RK1 w - - 1",
	"2kr1b1r/ppp3p1/5nbp/4B3/2P5/3P2Nq/PP2BP2/R2Q1RK1 b - - 0",
	"3r4/8/8/P7/1P6/1K3kpR/8/8 b - - 0 70",
	"8/8/8/P7/1n6/1P4k1/3K2p1/6B1 w - - 5 71",
	"8/1P2Qr1k/2b3pp/2pBp3/5q2/1N2R3/6KP/8 w - - 5 54",
	"r1b1rbk1/1p3p1p/p2ppBp1/4P3/q4P1Q/P2B4/1PP3PP/1R3R1K w - - 0 24",
	0
};

static void bench()
{
	NodeCount total = 0;
	Search *s = new Search;
	TransTable *tt = new TransTable;
	tt->resize( 1*1048576 );
	s->setHashTable(tt);
	Board b;
	SearchMode sm;
	sm.reset();
	sm.maxDepth = 15;

	const char **p = benchFens;

	i32 ticks = Timer::getMillisec();
	while ( *p )
	{
		s->clearHash();
		s->clearSlots();
		b.fromFEN( *p++ );
		s->iterate(b, sm, 1 );
		total += s->nodes;
	}
	ticks = Timer::getMillisec() - ticks;
	delete tt;
	delete s;
	std::cout << total << " nodes in "
			  << ticks << " msec (" << std::fixed
			  << total*1000/(ticks ? ticks : 1) << " nps)" << std::endl;
}

static NodeCount perft( cheng4::Board &b, Depth depth )
{
	if ( !depth )
		return 1;
	NodeCount res = 0;

	MoveGen mg( b );

	if ( depth == 1 )
	{
		// bulk counting only
		while ( mg.next() != mcNone )
			res++;
		return res;
	}

	Move m;
	UndoInfo ui;

	while ( (m = mg.next()) != mcNone )
	{
		bool isCheck = b.isCheck(m, mg.discovered() );

		b.doMove( m, ui, isCheck );
		res += perft( b, depth-1 );
		b.undoMove( ui );
	}

	return res;
}

static bool pbench()
{
	uint count = 0;
	for (const PerfTest *pt = suite; pt->fen; pt++ )
		count++;

	i32 totticks = Timer::getMillisec();

	uint index = 0;
	uint fails = 0;
	for (const PerfTest *pt = suite; pt->fen; pt++, index++ )
	{
		std::cout << "position " << 1+index << "/" << count << " : " << pt->fen << std::endl;

		cheng4::Board b;
		if ( !b.fromFEN( pt->fen ) )
		{
			std::cout << "ILLEGAL FEN!" << std::endl;
			return 0;
		}

		NodeCount result;
		i32 ticks = Timer::getMillisec();
		std::cout << "perft(" << (int)pt->depth << ") = " << (result = perft( b, pt->depth )) << std::endl;
		ticks = Timer::getMillisec() - ticks;
		std::cout << "took " << (double)ticks / 1000.0 << " sec, " << (double)result / 1000.0 / (double)ticks << " Mnps" << std::endl;

		if ( result != pt->count )
			fails++;

		std::cout << ((result == pt->count) ? "passed" : "FAILED!") << std::endl;
	}
	totticks = Timer::getMillisec() - totticks;
	std::cout << "total " << (double)totticks / 1000.0 << " sec" << std::endl;
	if ( fails )
		std::cout << fails << " / " << count << " FAILED!" << std::endl;
	else
		std::cout << "ALL OK" << std::endl;
  return !fails;
}

static void filterPgn( const char *fname )
{
	FilterPgn fp;
	if (!fp.parse(fname))
	{
		std::cout << "failed to parse " << fname << std::endl;
		return;
	}
	if (!fp.write("filterPgn_out.fen"))
		std::cout << "failed to write filterPgn_out.fen" << std::endl;
	else
		std::cout << "all ok" << std::endl;
}

// Protocol

void Protocol::Level::reset()
{
	moves = 0;
	mins = 0;
	secs = 0;
	inc = 0;
	move = 1;
}

static void protoCallback( const SearchInfo &si, void *param )
{
	static_cast<Protocol *>(param)->searchCallback( si );
}

Protocol::Protocol( Engine &eng ) : multiPV(1), protover(1), analyze(0), force(0), clocksRunning(0), engineColor(ctBlack),
	invalidState(0), frc(0), edit(0), post(1), fixedTime(0), maxCores(64), adjudicated(0), type( ptNative ),
	engine(eng), quitFlag(0)
{
	level.reset();

	npsLimit = 0;
	depthLimit = 0;
	timeLimit = 0;
	etime = otime = 60*1000;
	searchMode.reset();
	eng.setCallback( protoCallback, this );
}

Protocol::~Protocol()
{
	engine.setCallback( 0, 0 );
}

bool Protocol::parse( const std::string &line )
{
	MutexLock _( parseMutex );
	switch( type )
	{
	case ptUCI:
		return parseUCI( line );
	case ptCECP:
		return parseCECP( line );
	case ptNative:
		break;
	}
	if ( line == "uci" )
	{
		type = ptUCI;
		return parseUCI( line );
	}
	if ( line == "xboard" )
	{
		type = ptCECP;
		return parseCECP( line );
	}
	if ( line == "quit" )
	{
		quitFlag = 1;
		return 1;
	}
	// new: force uci mode if not otherwise specified
	type = ptUCI;
	engine.setUCIMode(1);
	return parseUCI(line);
}

bool Protocol::shouldQuit() const
{
	return quitFlag;
}

// start sending (reset buffer)
void Protocol::sendStart( uint flags )
{
	mutex.lock();
	sendStr.str( std::string() );
	switch( type )
	{
	case ptUCI:
		if ( !(flags & (sifRaw | sifBestMove | sifPonderMove)) )
			sendStr << "info";
		break;
	case ptCECP:
		break;
	case ptNative:
		break;
	}
}

// start sending (reset buffer)
void Protocol::sendEnd()
{
	// coding around rdbuf()->in_avail() which doesn't always work!
	std::string strg = sendStr.str();
	if ( !strg.empty() )
	{
		std::cout << strg << std::endl;
		std::cout.flush();		  // FIXME: this is necessary under Linux to work properly under xboard
	}
	mutex.unlock();
}

// called from search
void Protocol::searchCallback( const SearchInfo &si )
{
	sendStart( si.flags );

	if ( si.flags & sifDepth )
		sendDepth( si.depth );
	if ( si.flags & sifSelDepth )
		sendSelDepth( si.selDepth );
	if ( si.flags & sifTime )
		sendTime( si.time );
	if ( si.flags & sifNodes )
		sendNodes( si.nodes );
	if ( si.flags & sifNPS )
		sendNPS( si.nps );
	if ( si.flags & sifCurIndex )
		sendCurIndex( si.curIndex, si.curCount );
	if ( si.flags & sifCurMove )
		sendCurMove( si.curMove );
	if ( si.flags & sifPV )
	{
		assert( (si.flags & sifNodes) && (si.flags & sifTime) && (si.flags & sifDepth) );
		sendPV( si, si.pvIndex, si.pvScore, si.pvBound, (size_t)si.pvCount, si.pv );
	}
	if ( si.flags & sifBestMove )
		sendBest( si.bestMove, (si.flags & sifPonderMove) ? si.ponderMove : mcNone );

	sendEnd();

	if ( si.flags & sifBestMove )
		finishBest();
}

// send current depth
void Protocol::sendDepth( Depth d )
{
	switch( type )
	{
	case ptUCI:
		sendStr << " depth " << (int)d;
		break;
	case ptCECP:
	case ptNative:
		break;
	}
}

// send selective depth
void Protocol::sendSelDepth( Ply d )
{
	switch( type )
	{
	case ptUCI:
		sendStr << " seldepth " << (int)d;
		break;
	case ptCECP:
	case ptNative:
		break;
	}
}

// send time
void Protocol::sendTime( Time time )
{
	switch( type )
	{
	case ptUCI:
		sendStr << " time " << time;
		break;
	case ptCECP:
	case ptNative:
		break;
	}
}

// send nodes
void Protocol::sendNodes( NodeCount n )
{
	switch( type )
	{
	case ptUCI:
		sendStr << " nodes " << n;
		break;
	case ptCECP:
	case ptNative:
		break;
	}
}

// send nodes
void Protocol::sendNPS( NodeCount n )
{
	switch( type )
	{
	case ptUCI:
		sendStr << " nps " << n;
		break;
	case ptCECP:
	case ptNative:
		break;
	}
}

// send current move index (zero based)
void Protocol::sendCurIndex( MoveCount index, MoveCount /*count*/ )
{
	switch( type )
	{
	case ptUCI:
		sendStr << " currmovenumber " << (index+1);
		break;
	case ptCECP:
	case ptNative:
		break;
	}
}

// send current move
void Protocol::sendCurMove( Move move )
{
	switch( type )
	{
	case ptUCI:
		sendStr << " currmove " << engine.board().toUCI(move);
		break;
	case ptCECP:
	case ptNative:
		break;
	}
}

static std::string uciScore( Score score )
{
	std::stringstream res;
	if ( ScorePack::isMate( score ) )
	{
		int msc = score >= 0 ? (scInfinity - score)/2 + 1 : (-scInfinity - score + 1)/2 - 1;
		assert( msc );
		assert( (msc >= 0 && score >= 0) || (msc < 0 && score < 0) );
		res << "mate " << msc;
	} else res << "cp " << score;
	return res.str();
}

// index: k-best(multipv) zero-based index
void Protocol::sendPV( const SearchInfo &si, uint index, Score score, enum BoundType bound, size_t pvCount,
	const Move *pv )
{
	switch( type )
	{
	case ptUCI:
		if ( bound == btLower )
			sendStr << " lowerbound ";
		else if ( bound == btUpper )
			sendStr << " upperbound ";
		sendStr << " multipv " << (1+index) << " score " << uciScore( score ) << " pv";
		for (size_t i=0; i<pvCount; i++)
			sendStr << " " << engine.board().toUCI( pv[i] );
		break;
	case ptCECP:
		if ( !post )
			break;
		{
			Depth d = 0;
			Time t = 0;
			NodeCount n = 0;
			if ( si.flags & sifDepth )
				d = si.depth;
			if ( si.flags & sifTime )
				t = si.time;
			if ( si.flags & sifNodes )
				n = si.nodes;

			sendStr.width(3);
			sendStr << (int)d;
			bound = btExact;		// FIXME: WinBoard seems to have problems with additional + or - => disabled
			sendStr << (bound == btUpper ? '-' : bound == btLower ? '+' : ' ');
			sendStr << " ";
			sendStr.width(5);
			// TODO: convert score to pseudo-standard mate scores
			sendStr << score << " ";
			sendStr.width(8);
			sendStr << (t/10) << " ";
			sendStr.width(12);
			sendStr << n;
			Board b( engine.board() );

			Move ponder = mcNone;
			if ( engine.isPondering() )
				ponder = engine.getPonderMove();

			if ( ponder != mcNone )
				sendStr << " (" << engine.ponderBoard().toSAN( ponder ) << ')';

			for (size_t i=0; i<pvCount; i++)
			{
				sendStr << " " << b.toSAN( pv[i] );
				UndoInfo ui;
				b.doMove( pv[i], ui, b.isCheck( pv[i], b.discovered() ) );
			}
			sendStr.width(1);
		}
		break;
	case ptNative:
		break;
	}
}

// send best move
void Protocol::sendBest( Move best, Move ponder )
{
	switch( type )
	{
	case ptUCI:
		sendStr << "bestmove " << engine.board().toUCI( best );
		if ( ponder != mcNone )
			sendStr << " ponder " << engine.board().toUCI( ponder );
		break;
	case ptCECP:
		if ( !analyze )
		{
			sendStr << "move " << engine.board().toUCI( best );
			engine.doMoveInternal( best, 1 );
			engine.setPonderMove( ponder );
		}
		break;
	case ptNative:
		break;
	}
}

void Protocol::finishBest()
{
	if ( type != ptCECP || analyze )
		return;
	// called from another thread!
	MutexLock _( parseMutex );
	// now check to see whether it's already game over
	if ( adjudicate() )
		return;
	etime -= Timer::getMillisec() - startTicks;
	startPondering(1);
}

// send raw mesage
void Protocol::sendRaw( const std::string &msg )
{
	sendStr << msg;
}

// send raw EOL
void Protocol::sendEOL()
{
	sendStr << std::endl;
}

// send custom info message
void Protocol::sendInfo( const std::string &msg )
{
	switch( type )
	{
	case ptUCI:
		sendStr << " string " << msg;
		break;
	case ptCECP:
	case ptNative:
		break;
	}
}

// UCI parser

static std::string nextToken( const std::string &str, size_t &start )
{
	std::string res;
	const char *c = str.c_str() + start;
	// consume initial whitespace
	while ( *c && isspace(*c & 255) )
		c++;
	const char *beg = c;
	// token
	while ( *c && !isspace(*c & 255) )
		c++;
	if ( c > beg )
		res = std::string( beg, c );
	start = c - str.c_str();
	return res;
}

// to simplify option parsing
static std::string nextOptToken( const std::string &str, size_t &start )
{
	std::string res;
	const char *c = str.c_str() + start;
	// consume initial whitespace/=
	while ( *c && isspace(*c & 255) )
		c++;
	const char *beg = c;
	// token
	while ( *c && *c != '=' )
		c++;
	if ( c > beg )
		res = std::string( beg, c );
	start = c - str.c_str();
	if ( *c == '=' )
		start++;
	return res;
}

// parse UCI command
bool Protocol::parseUCI( const std::string &line )
{
	size_t pos = 0;
	std::string token = nextToken( line, pos );
	if ( token.empty() )
		return 0;

	if ( token == "ponderhit" )
	{
		engine.ponderHit();
		return 1;
	}
	if ( token == "stop" )
	{
		engine.stopSearch();
		return 1;
	}
	if ( token == "isready" )
	{
		sendStart( sifRaw );
		sendRaw( "readyok" );
		sendEnd();
		return 1;
	}
	if ( token == "go" )
	{
		SearchMode sm;
		sm.reset();
		sm.multiPV = multiPV;

		i32 wtime, btime, winc, binc, movestogo;
		wtime = btime = winc = binc = movestogo = 0;

		for (;;)
		{
			token = nextToken( line, pos );
			if ( token.empty() )
				break;
			if ( token == "ponder" )
			{
				sm.ponder = 1;
			}
			else
			if ( token == "infinite" )
			{
				sm.reset();
				sm.multiPV = multiPV;
			}
			else if ( token == "movetime" )
			{
				// parse movetime
				token = nextToken( line, pos );
				if ( token.empty() )
				{
					error( "movetime requires time parameter", line );
					return 0;
				}
				long mt = strtol( token.c_str(), 0, 10 );
				if ( mt <= 0 )
					mt = -1;
				sm.absLimit = sm.maxTime = (i32)mt;
				sm.fixedTime = 1;
			}
			else if ( token == "movestogo" )
			{
				// parse movestogo
				token = nextToken( line, pos );
				if ( token.empty() )
				{
					error( "movestogo requires int parameter", line );
					return 0;
				}
				long mt = strtol( token.c_str(), 0, 10 );
				movestogo = (i32)mt;
			}
			else if ( token == "winc" )
			{
				// parse winc
				token = nextToken( line, pos );
				if ( token.empty() )
				{
					error( "winc requires int parameter", line );
					return 0;
				}
				long i = strtol( token.c_str(), 0, 10 );
				winc = (i32)i;
			}
			else if ( token == "binc" )
			{
				// parse binc
				token = nextToken( line, pos );
				if ( token.empty() )
				{
					error( "binc requires int parameter", line );
					return 0;
				}
				long i = strtol( token.c_str(), 0, 10 );
				binc = (i32)i;
			}
			else if ( token == "wtime" )
			{
				// parse wtime
				token = nextToken( line, pos );
				if ( token.empty() )
				{
					error( "wtime requires time parameter", line );
					return 0;
				}
				long t = strtol( token.c_str(), 0, 10 );
				if ( t <= 0 )
					t = -1;
				wtime = (i32)t;
			}
			else if ( token == "btime" )
			{
				// parse btime
				token = nextToken( line, pos );
				if ( token.empty() )
				{
					error( "btime requires time parameter", line );
					return 0;
				}
				long t = strtol( token.c_str(), 0, 10 );
				if ( t <= 0 )
					t = -1;
				btime = (i32)t;
			}
			else if ( token == "depth" )
			{
				// parse depth
				token = nextToken( line, pos );
				if ( token.empty() )
				{
					error( "depth requires int parameter", line );
					return 0;
				}
				long i = strtol( token.c_str(), 0, 10 );
				sm.maxDepth = (Depth)std::min( (int)maxDepth, (int)i );
			}
			else if ( token == "mate" )
			{
				// parse mate
				token = nextToken( line, pos );
				if ( token.empty() )
				{
					error( "mate requires int parameter", line );
					return 0;
				}
				long i = strtol( token.c_str(), 0, 10 );
				sm.mateSearch = (uint)std::max(0l, i);
			}
			else if ( token == "nodes" )
			{
				// parse nodes
				token = nextToken( line, pos );
				if ( token.empty() )
				{
					error( "nodes requires int parameter", line );
					return 0;
				}
				std::stringstream ss;
				ss.str( token );
				ss >> sm.maxNodes;
			} else if ( token == "searchmoves" )
			{
				// parse moves to restrict search to
				for (;;)
				{
					size_t opos = pos;
					token = nextToken( line, pos );
					if ( token.empty() )
						break;
					// now parse UCI move
					const Board &b = engine.board();
					Move m = b.fromUCI( token );
					bool legal = b.inCheck() ? b.isLegal<1, 0>(m, b.pins() ) :
						b.isLegal<0, 0>(m, b.pins() );
					if ( !legal )
					{
						pos = opos;
						break;
					}
					sm.moves.push_back( m );
				}
			}
		}

		if ( wtime || btime )
		{
			// tournament time controls!
			engine.abortSearch();
			Color turn = engine.mainThread->search.board.turn();

			i32 mytime, myinc, optime, opinc;
			if ( turn == ctWhite )
			{
				mytime = wtime;
				myinc = winc;
				optime = btime;
				opinc = binc;
			}
			else
			{
				mytime = btime;
				myinc = binc;
				optime = wtime;
				opinc = winc;
			}
			allocTime( mytime, myinc, optime, opinc, movestogo, sm );
		}

		engine.startSearch( sm );
		return 1;
	}
	if ( token == "position" )
	{
		engine.abortSearch();
		token = nextToken( line, pos );
		if ( token.empty() )
			return 0;
		if ( token == "startpos" )
		{
			engine.resetBoard();
		} else if ( token == "fen" )
		{
			Board b;
			const char *c = line.c_str() + pos;
			c = b.fromFEN( c );
			if ( !c )
			{
				error( "illegal fen from GUI", line );
				return 0;
			}
			engine.setBoard( b );
			pos = c - line.c_str();
		} else {
			error( "invalid position command", line );
			return 0;
		}
		token = nextToken( line, pos );
		if ( token == "moves" )
		{
			// parse moves
			const char *c = line.c_str() + pos;
			for (;;)
			{
				skipSpaces(c);
				// now parse UCI move
				Move m = engine.board().fromUCI( c );
				if ( m == mcNone )
					break;
				if ( !engine.doMove( m ) )
				{
					illegalMove( m, line );
					return 0;
				}
			}
			return 1;
		}
		return 1;
	}
	if ( token == "quit" )
	{
		quitFlag = 1;
		return 1;
	}
	if ( token == "uci" )
	{
#ifdef USE_TUNING
		PSq::init();
#endif
		sendStart( sifRaw );
		sendRaw( "id name "); sendRaw( Version::version() ); sendEOL();
		sendRaw( "id author "); sendRaw( Version::author() ); sendEOL();
		sendRaw( "option name Hash type spin min 1 max " maxHash " default 4" ); sendEOL();
		sendRaw( "option name Clear Hash type button" ); sendEOL();
		sendRaw( "option name Ponder type check default false" ); sendEOL();
		sendRaw( "option name OwnBook type check default true" ); sendEOL();
		sendRaw( "option name MultiPV type spin min 1 max 256 default 1" ); sendEOL();
		sendRaw( "option name UCI_Chess960 type check default false" ); sendEOL();
		sendRaw( "option name Threads type spin min 1 max 64 default 1" ); sendEOL();
		sendRaw( "option name UCI_LimitStrength type check default false" ); sendEOL();
		sendRaw( "option name UCI_Elo type spin min 800 max 2500 default 2500" ); sendEOL();
		sendRaw( "option name NullMove type check default true" ); sendEOL();
#ifdef USE_TUNING
		for ( size_t i=0; i<TunableParams::paramCount(); i++ )
		{
			const TunableBase *par = TunableParams::getParam(i);
			std::string str = "option name " + par->name() + " type spin min -1000000 max 1000000 default " + par->get();
			sendRaw( str ); sendEOL();
		}
#endif
		sendRaw( "uciok" );
		sendEnd();
		engine.setUCIMode(1);
		return 1;
	}
	if ( token == "setoption" )
	{
		parseUCIOptions( line, pos );
		return 1;
	}
	if ( token == "ucinewgame" )
	{
		engine.abortSearch();
		engine.clearHash();
		return 1;
	}
	if ( parseSpecial( token, line, pos ) )
		return 1;
	error( "unknown command", line );
	return 0;
}

bool Protocol::parseUCIOptions( const std::string &line, size_t &pos )
{
	std::string token;
	token = nextToken( line, pos );
	if ( token != "name" )
	{
		error( "name expected", line );
		return 0;
	}
	std::string key;
	for (;;)
	{
		token = nextToken( line, pos );
		if ( token.empty() )
			break;
		if ( token == "value" )
			break;
		if ( key.empty() )
			key = token;
		else
			key = key + " " + token;
	}
	std::string value;
	const char *c = line.c_str() + pos;
	// skip blanks
	while ( *c && isspace(*c & 255) )
		c++;
	value = c;
	// examine key
	if ( key == "Clear Hash" )
	{
		engine.clearHash();
		return 1;
	}
	if ( key == "Hash" )
	{
		long hm = strtol( value.c_str(), 0, 10 );
		return engine.setHash( (uint)std::max(1l, hm) );
	}
	if ( key == "Ponder" )
	{
		engine.setPonder( value == "true" );
		return 1;
	}
	if ( key == "Threads" )
	{
		long thr = strtol( value.c_str(), 0, 10 );
		engine.setThreads( (uint)std::max( 1l, thr ) );
		return 1;
	}
	if ( key == "MultiPV" )
	{
		long mpv = strtol( value.c_str(), 0, 10 );
		multiPV = (uint)std::max( 1l, std::min( 256l, mpv ) );
		engine.setMultiPV( multiPV );
		return 1;
	}
	if ( key == "OwnBook" )
	{
		engine.setOwnBook( value != "false" );	// was value == "true", this is just to fix an annoying Arena bug!
		return 1;
	}
	if ( key == "UCI_Chess960" )
	{
		// handled automatically
		return 1;
	}
	if ( key == "UCI_LimitStrength")
	{
		engine.setLimit( value == "true" );
		return 1;
	}
	if ( key == "UCI_Elo")
	{
		long elo = strtol( value.c_str(), 0, 10 );
		elo = std::max( 800L, elo );
		elo = std::min( 2500L, elo );
		engine.setElo( (u32)elo );
		return 1;
	}
	if ( key == "NullMove")
	{
		engine.setNullMove( value != "false" );
		return 1;
	}
#ifdef USE_TUNING
	if ( TunableParams::setParam(key.c_str(), value.c_str()) )
	{
		PSq::init();
		return 1;
	}
#endif
	error( "unknown option", line );
	return 0;
}

void Protocol::setSearchModeCECP( SearchMode &sm )
{
	sm.reset();
	sm.multiPV = multiPV;
	if ( timeLimit )
	{
		sm.absLimit = sm.maxTime = timeLimit;
		sm.fixedTime = fixedTime;
	}
	else
	{
		i32 movestogo = 0;
		if ( level.moves )
		{
			// calculate movestogo
			uint move = engine.board().move();
			move -= level.move;
			move %= level.moves;
			move = level.moves - move;
			movestogo = (i32)move;
		}
		i32 einc = level.inc, oinc = level.inc;
		allocTime( etime, einc, otime, oinc, movestogo, sm );
	}
	if ( npsLimit )
		sm.maxNodes = npsLimit * sm.maxTime;
	sm.maxDepth = depthLimit;
}

// start CECP-mode search
bool Protocol::startSearchCECP()
{
	if ( force )
		return 0;
	Color stm = engine.board().turn();
	if ( stm != engineColor )
		return 0;
	startTicks = Timer::getMillisec();
	SearchMode sm;
	sm.reset();
	setSearchModeCECP( sm );
	engine.startSearch( sm );
	return 1;
}

bool Protocol::parseCECP( const std::string &line )
{
	bool res = parseCECPInternal( line );
	if ( res && !invalidState && !edit && analyze )
	{
		SearchMode sm;
		sm.reset();
		sm.multiPV = multiPV;
		engine.startSearch( sm );
	}
	return res;
}

bool Protocol::parseCECPEdit( const std::string &line )
{
	size_t pos = 0;
	std::string token = nextToken( line, pos );
	if ( token.empty() )
		return 0;
	if ( token == "c" )
	{
		// flip color
		editBoard.setTurn( flip( editBoard.turn() ) );
		return 1;
	}
	if ( token == "#" )
	{
		editBoard.clearPieces();
		return 1;
	}
	if ( token == "." )
	{
		edit = 0;
		editBoard.updateBitboards();
		if ( !editBoard.isValid() )
		{
			sendStart( sifRaw );
			sendRaw( "tellusererror Illegal position" );
			sendEnd();
			invalidState = 1;
			error( "edit: invalid position", line );
			return 0;
		}
		// assign castling rights automatically!
		editBoard.setFischerRandom( frc );
		editBoard.autoCastlingRights();

		engine.setBoard( editBoard );
		level.move = editBoard.move();
		return 1;
	}
	// assume it's in the format Pa4 (xa4 is extended and should clear square)
	if ( token.length() == 3 )
	{
		const char *c = token.c_str();
		char ch = c[0] | 32;
		int f = (int)((c[1] | 32) - 'a');
		bool invalid = 0;
		if ( f < AFILE || f > HFILE )
			invalid = 1;
		if ( c[2] < '1' || c[2] > '8' )
			invalid = 1;
		Piece pt = ptNone;
		int r = -1;
		if ( !invalid )
		{
			r = ('8' - c[2]) ^ RANK8;
			switch( ch )
			{
			case 'x':
				pt = ptNone;
				break;
			case 'p':
				pt = ptPawn;
				break;
			case 'n':
				pt = ptKnight;
				break;
			case 'b':
				pt = ptBishop;
				break;
			case 'r':
				pt = ptRook;
				break;
			case 'q':
				pt = ptQueen;
				break;
			case 'k':
				pt = ptKing;
				break;
			default:
				invalid = 1;
			}
		}
		if ( !invalid )
		{
			editBoard.setPiece( editBoard.turn(), pt, SquarePack::init( (File)f, (Rank)r ) );
			return 1;
		}
	}

	error( "unknown command", line );
	return 0;
}

void Protocol::abortSearch()
{
	engine.abortSearch();
}

void Protocol::startPondering( bool nostop )
{
	if ( force || analyze || edit || engine.board().turn() == engineColor )
		return;
	if ( engine.getPonder() && !engine.isPondering() )
	{
		SearchMode sm;
		sm.reset();
		setSearchModeCECP( sm );
		engine.startPondering( sm, nostop );
	}
}

bool Protocol::parseCECPInternal( const std::string &line )
{
	if ( edit )
		return parseCECPEdit( line );
	size_t pos = 0;
	std::string token = nextToken( line, pos );
	if ( token.empty() )
		return 0;

	// look for move first...
	// note that the move could be a rook capture => castling!
	Move move = engine.actualBoard().fromUCI( token );
	if ( move != mcNone )
	{
		bool phit = 0;
		if ( !invalidState && engine.doMove( move, 0, &phit ) )
		{
			if ( adjudicate() )
				return 1;
			// only start new search if move was not ponderhit
			if ( !phit )
				startSearchCECP();
			return 1;
		}
		illegalMove( move, line );
		return 0;
	}

	if ( token == "go" )
	{
		if ( invalidState )
		{
			error( "current board state is invalid", line );
			return 0;
		}
		leaveForce();
		engineColor = engine.board().turn();
		startSearchCECP();
		return 1;
	}

	if ( token == "time" )
	{
		fixedTime = 0;
		token = nextToken( line, pos );
		if ( token.empty() )
		{
			error( "time arg expected", line );
			return 0;
		}
		long tmp = strtol( token.c_str(), 0, 10 );
		etime = (tmp > 0) ? (Time)(tmp * 10) : (Time)-1;
		return 1;
	}
	if ( token == "otim" )
	{
		fixedTime = 0;
		token = nextToken( line, pos );
		if ( token.empty() )
		{
			error( "otim arg expected", line );
			return 0;
		}
		long tmp = strtol( token.c_str(), 0, 10 );
		otime = (tmp > 0) ? (Time)(tmp * 10) : (Time)-1;
		return 1;
	}
	if ( token == "force" )
	{
		enterForce();
		return 1;
	}
	if ( token == "white" )
	{
		stopClocks();
		engineColor = ctBlack;
		if ( setTurn( ctWhite ) )
			return 1;
		error( "can't set stm to white", line );
		return 0;
	}
	if ( token == "black" )
	{
		stopClocks();
		engineColor = ctWhite;
		if ( setTurn( ctBlack ) )
			return 1;
		error( "can't set stm to black", line );
		return 0;
	}
	if ( token == "level" )
	{
		// parse level...
		// only increment is float according to hgm
		// the format is:
		// moves min(:sec) inc
		timeLimit = 0;			// override time limit
		fixedTime = 0;
		Level lev;
		lev.reset();
		lev.move = engine.board().move();

		token = nextToken( line, pos );
		if ( token.empty() )
		{
			error( "level moves expected", line );
			return 0;
		}
		long lmoves = std::max( 0l, strtol( token.c_str(), 0, 10 ) );
		lev.moves = (uint)lmoves;

		token = nextToken( line, pos );
		if ( token.empty() )
		{
			error( "level time expected", line );
			return 0;
		}
		const char *c = token.c_str();
		long lmins = std::max( 0l, strtol( c, (char **)&c, 10 ) );
		lev.mins = (uint)lmins;
		lev.secs = 0;
		if ( *c == ':' )
		{
			// parse secs
			c++;
			long lsecs = std::max( 0l, strtol( c, 0, 10 ) );
			lev.secs = (uint)lsecs;
		}
		token = nextToken( line, pos );
		if ( token.empty() )
		{
			error( "level inc expected", line );
			return 0;
		}
		double dinc = std::max( 0.0, strtod( token.c_str(), 0 ) );
		dinc *= 1000;
		lev.inc = (Time)dinc;
		level = lev;
		return 1;
	}
	if ( token == "st" )
	{
		token = nextToken( line, pos );
		if ( token.empty() )
		{
			error( "st arg expected", line );
			return 0;
		}
		long tmp = strtol( token.c_str(), 0, 10 );
		timeLimit = (tmp > 0) ? (Time)(tmp * 1000) : (Time)-1;
		fixedTime = 1;
		return 1;
	}
	if ( token == "sd" )
	{
		token = nextToken( line, pos );
		if ( token.empty() )
		{
			error( "sd arg expected", line );
			return 0;
		}
		long tmp = strtol( token.c_str(), 0, 10 );
		depthLimit = (tmp > 0) ? (Depth)tmp : 1;
		return 1;
	}
	if ( token == "nps" )
	{
		token = nextToken( line, pos );
		if ( token.empty() )
		{
			error( "nps arg expected", line );
			return 0;
		}
		std::stringstream ss( token );
		ss.str( token );
		ss >> npsLimit;
		return 1;
	}
	if ( token == "ping" )
	{
		token = nextToken( line, pos );
		if ( token.empty() )
		{
			error( "ping arg expected", line );
			return 0;
		}
		sendStart( sifRaw );
		sendRaw( "pong " );
		sendRaw( token );
		sendEnd();
		return 1;
	}
	if ( token == "memory" )
	{
		token = nextToken( line, pos );
		if ( token.empty() )
		{
			error( "memory arg expected", line );
			return 0;
		}
		uint memory;
		std::stringstream ss( token );
		ss.str( token );
		ss >> memory;
		if ( memory < 1 )
			memory = 1;
		if ( !engine.setHash( memory ) )
		{
			error( "couldn't adjust memory", line );
			return 0;
		}
		return 1;
	}
	if ( token == "protover" )
	{
		token = nextToken( line, pos );
		if ( token.empty() )
		{
			error( "protover arg expected", line );
			return 0;
		}
		long pver = strtol( token.c_str(), 0, 10 );
		protover = (int)pver;

		// FIXME: ping temporarily disabled as it probably causes connection stalls in xboard mode in cutechess
		sendStart( sifRaw );
		sendRaw(
			"feature name=0 san=0 usermove=0 time=1 sigint=0 sigterm=0 pause=0 reuse=1 analyze=1 colors=0 setboard=1 "
			"nps=1 smp=1 debug=0 draw=0 playother=1 variants=\"normal,fischerandom\" ics=0 memory=1 ping=0 "
			"option=\"Clear Hash -button\" option=\"Hash -spin 4 1 " maxHash "\" option=\"Threads -spin 1 1 64\" "
			"option=\"OwnBook -check 1\" option=\"LimitStrength -check 0\" option=\"Elo -spin 2500 800 2500\" "
			"option=\"MultiPV -spin 1 1 256\" option=\"NullMove -check 1\" myname=\""
		);
		sendRaw( Version::version() );
		sendRaw( "\" "
			"done=1"
		);
		sendEnd();
		return 1;
	}
	if ( token == "variant" )
	{
		token = nextToken( line, pos );
		if ( token.empty() )
		{
			error( "variant arg expected", line );
			return 0;
		}
		if ( token != "normal" && token != "fischerandom" )
		{
			error( "unsupported variant", line );
			return 0;
		}
		frc = token == "fischerandom";
		return 1;
	}
	if ( token == "easy" )
	{
		if ( engine.getPonder() )
		{
			// abort search if pondering
			if ( engine.isPondering() )
				abortSearch();
			engine.setPonder(0);
		}
		return 1;
	}
	if ( token == "hard" )
	{
		if ( !engine.getPonder() )
		{
			engine.setPonder(1);
			startPondering();
		}
		return 1;
	}
	if ( token == "exit" )
	{
		if ( analyze )
		{
			abortSearch();
			analyze = 0;
			return 1;
		}
		error( "not analyzing", line );
		return 0;
	}
	if ( token == "cores" )
	{
		token = nextToken( line, pos );
		if ( token.empty() )
		{
			error( "cores arg expected", line );
			return 0;
		}
		long cores = strtol( token.c_str(), 0, 10 );
		maxCores = (uint)std::max( 1l, cores );
		engine.limitThreads( maxCores );
		return 1;
	}
	if ( token == "option" )
	{
		token = nextOptToken( line, pos );
		if ( token.empty() )
		{
			error( "option arg expected", line );
			return 0;
		}
		// parse options here
		// FIXME: separate method to handle this
		if ( token == "Clear Hash" )
		{
			engine.clearHash();
			return 1;
		}
		if ( token == "Hash" )
		{
			long hsz = std::max( 1l, strtol( line.c_str() + pos, 0, 10 ) );
			if ( !engine.setHash( (uint)hsz ) )
				error( "couldn't set hash size", line );
			return 1;
		}
		if ( token == "Threads" )
		{
			long thr = strtol( line.c_str() + pos, 0, 10 );
			engine.setThreads( (uint)std::max( 1l, thr ) );
			return 1;
		}
		if ( token == "MultiPV" )
		{
			long mpv = strtol( line.c_str() + pos, 0, 10 );
			multiPV = (uint)std::max( 1l, std::min( 256l, mpv ) );
			engine.setMultiPV( multiPV );
			return 1;
		}
		if ( token == "OwnBook" )
		{
			long obk = strtol( line.c_str() + pos, 0, 10 );
			engine.setOwnBook( obk != 0 );
			return 1;
		}
		if ( token == "NullMove" )
		{
			long nm = strtol( line.c_str() + pos, 0, 10 );
			engine.setNullMove( nm != 0 );
			return 1;
		}
		if ( token == "LimitStrength" )
		{
			long lst = strtol( line.c_str() + pos, 0, 10 );
			engine.setLimit( lst != 0 );
			return 1;
		}
		if ( token == "Elo" )
		{
			long elo = strtol( line.c_str() + pos, 0, 10 );
			engine.setElo( (u32)std::max( 2500l, std::min( 800l, elo ) ) );
			return 1;
		}
		error( "uknown option", line );
		return 0;
	}
	if ( token == "quit" )
	{
		quitFlag = 1;
		return 1;
	}
	if ( token == "analyze" )
	{
		if ( !invalidState )
		{
			analyze = 1;
			return 1;
		}
		error( "current board state is invalid", line );
		return 0;
	}
	if ( token == "new" )
	{
		leaveForce();
		npsLimit = 0;
		depthLimit = 0;
		timeLimit = 0;
		fixedTime = 0;

		invalidState = 0;

		abortSearch();
		engine.clearHash();
		engine.resetBoard();
		engineColor = ctBlack;
		searchMode.reset();
		// reset clocks
		resetClocks();

		adjudicated = 0;
		return 1;
	}
	if ( token == "undo" )
	{
		if ( !invalidState && engine.undoMove() )
			return 1;
		error( "can't undo", line );
		return 0;
	}
	if ( token == "remove" )
	{
		// this complicated expression is to silence some static analyzers
		if ( !invalidState && engine.undoMove() )
			if ( engine.undoMove() )
				return 1;
		error( "can't remove", line );
		return 0;
	}
	if ( token == "playother" )
	{
		leaveForce();
		engine.abortSearch();
		Color oec = engineColor;
		engineColor = flip( engine.turn() );
		if ( oec != engineColor )
			std::swap( etime, otime );
		startSearchCECP();
		// start pondering if enabled...
		startPondering();
		return 1;
	}
	if ( token == "?" )
	{
		// FIXME: what to do if ? is received while pondering (doing nothing ATM)
		if ( !analyze && !engine.isPondering() )
		{
			// FIXME: this causes deadlocks now that I've fixed UCI pondering bug...
			// FIXME: this is super ugly, redesign and rewrite completely!!! (really?)
			parseMutex.unlock();
			engine.stopSearch();
			parseMutex.lock();
		}
		return 1;
	}
	if ( token == "result" )
	{
		// don't waste system resource just in case the engine is pondering
		if ( !analyze )
			abortSearch();
		return 1;
	}
	if ( token == "setboard" )
	{
		invalidState = 0;
		Board b;
		if ( !b.fromFEN( line.c_str() + pos ) )
		{
			sendStart( sifRaw );
			sendRaw( "tellusererror Illegal position" );
			sendEnd();
			invalidState = 1;
			error( "invalid fen", line );
			return 0;
		}
		engine.setBoard( b );
		level.move = b.move();
		return 1;
	}
	if ( token == "edit" )
	{
		abortSearch();
		invalidState = 0;
		editBoard = engine.board();
		edit = 1;
		return 1;
	}
	if ( token == "accepted" )
	{
		// silently ignore
		return 1;
	}
	if ( token == "post" )
	{
		post = 1;
		return 1;
	}
	if ( token == "nopost" )
	{
		post = 0;
		return 1;
	}
	if ( token == "uci" )
	{
		// switch to the (preferred atm) UCI protocol
		type = ptUCI;
		return parseUCI( line );
	}
	if ( token == "xboard" )
	{
#ifdef USE_TUNING
		PSq::init();
#endif
		return 1;				// consume silently
	}
	if ( parseSpecial( token, line, pos ) )
		return 1;
	error( "unknown command", line );
	return 0;
}

void Protocol::error( const std::string &msg, const std::string &line )
{
	switch( type )
	{
	case ptUCI:
		sendStart(0);
		sendInfo( msg );
		sendEnd();
		break;
	case ptCECP:
		sendStart( sifRaw );
		sendRaw( "Error (" );
		sendRaw( msg );
		sendRaw( "): ");
		sendRaw( line );
		sendEnd();
		break;
	case ptNative:
		break;
	}
}

// CECP: reset clock
void Protocol::resetClocks()
{
	// TODO: implement...

}

void Protocol::stopClocks()
{
	abortSearch();
	clocksRunning = 0;
}

bool Protocol::setTurn( Color c )
{
	return engine.setTurn( c );
}

void Protocol::leaveForce()
{
	if ( force )
	{
		force = 0;
	}
}

void Protocol::enterForce()
{
	stopClocks();
	force = 1;
}

void Protocol::illegalMove( Move m, const std::string &line )
{
	switch( type )
	{
	case ptUCI:
		error( "illegal move from GUI", line );
		break;
	case ptCECP:
		sendStart( sifRaw );
		sendRaw( "Illegal move: " );
		sendRaw( engine.board().toUCI( m ) );
		sendEnd();
		break;
	case ptNative:
		break;
	}
}

// try to adjudicate game after engine/user move
// if it's win/draw/loss, result is nonzero and appropriate string is sent to GUI
bool Protocol::adjudicate()
{
	const Board &b = engine.board();
	Draw dr = b.isDraw();
	if ( dr == drawMaterial )
	{
		sendStart( sifRaw );
		sendRaw( "result 1/2-1/2 {Insufficient material}" );
		sendEnd();
		adjudicated = 1;
		return 1;
	}
	if ( dr == drawFifty )
	{
		sendStart( sifRaw );
		sendRaw( "result 1/2-1/2 {Fifty move rule}" );
		sendEnd();
		adjudicated = 1;
		return 1;
	}
	MoveGen mg( b );
	Move m = mg.next();
	if ( m == mcNone )
	{
		if ( b.inCheck() )
		{
			// mated!
			sendStart( sifRaw );
			if ( b.turn() == ctBlack )
			{
				sendRaw( "result 1-0 {White mates}" );
				adjudicated = 1*2;
			}
			else
			{
				sendRaw( "result 0-1 {Black mates}" );
				adjudicated = -1*2;
			}
			sendEnd();
			adjudicated |= 1;
		} else
		{
			// stalemate
			sendStart( sifRaw );
			sendRaw( "result 1/2-1/2 {Stalemate}" );
			sendEnd();
			adjudicated = 1;
		}
		return 1;
	}
	// check for draw by repetition!
	if ( engine.isThreefold() )
	{
		sendStart( sifRaw );
		sendRaw( "result 1/2-1/2 {Threefold repetition}" );
		sendEnd();
		adjudicated = 1;
		return 1;
	}
	return 0;
}

// parse special (nonstd) command
bool Protocol::parseSpecial( const std::string &token, const std::string &line, size_t &pos )
{
	if ( token == "dump" )
	{
		engine.abortSearch();
		engine.board().dump();
		Score evl = engine.mainThread->search.eval.eval( engine.board() );
		std::cout << "eval: " << evl << std::endl;
		std::cout.flush();
		return 1;
	}
	if ( token == "bench" )
	{
		engine.abortSearch();
		bench();
		return 1;
	}
	if ( token == "pbench" )
	{
		engine.abortSearch();
		pbench();
		return 1;
	}
	if ( token == "perft" )
	{
		std::string t = nextToken( line, pos );
		if ( t.empty() )
		{
			error( "perft arg expected", line );
			return 1;
		}
		engine.abortSearch();
		long tmp = strtol( t.c_str(), 0, 10 );
		Depth d = (Depth)( std::max( 1l, std::min( (long)maxDepth, tmp ) ) );
		Board b( engine.board() );
		i32 ms = Timer::getMillisec();
		NodeCount res = perft( b, d );
		ms = Timer::getMillisec() - ms;
		std::cout << res << " nodes" << std::endl;
		std::cout << "took " << ms << " ms" << std::endl;
		std::cout.flush();
		return 1;
	}
	if ( token == "divide" )
	{
		std::string t = nextToken( line, pos );
		if ( t.empty() )
		{
			error( "divide arg expected", line );
			return 1;
		}
		engine.abortSearch();
		long tmp = strtol( t.c_str(), 0, 10 );
		Depth d = (Depth)( std::max( 1l, std::min( (long)maxDepth, tmp ) ) );
		Board b( engine.board() );
		MoveGen mg( b );
		Move m;
		NodeCount total = 0;
		MoveCount mc = 0;
		i32 ms = Timer::getMillisec();
		while ( (m = mg.next()) != mcNone )
		{
			mc++;
			bool isCheck = b.isCheck( m, mg.discovered() );
			std::cout << b.toSAN( m ) << ' ';
			UndoInfo ui;
			b.doMove( m, ui, isCheck );
			NodeCount nc = perft( b, d-1 );
			b.undoMove( ui );
			total += nc;
			std::cout << nc << std::endl;
		}
		ms = Timer::getMillisec() - ms;
		std::cout << (uint)mc << " moves " << total << " nodes" << std::endl;
		std::cout << "took " << ms << " ms" << std::endl;
		std::cout.flush();
		return 1;
	}
	if ( token == "book" )
	{
		// special book debug
		engine.abortSearch();
		Board b( engine.board() );
		std::vector< Move > moves;
		std::vector< u32 > counts;
		u32 sum;
		sum = std::max(1u, engine.book.enumEntries(b, moves, 0, &counts));
		assert( moves.size() == counts.size() );
		for ( size_t i=0; i<moves.size(); i++ )
		{
			std::string move = b.toSAN( moves[i] );
			u32 score = sum ? counts[i]*100 / sum : 0;
			std::cout << move << " count " << counts[i] << " score " << score << std::endl;
		}
		// enumerate moves to avoid
		std::max(1u, engine.book.enumEntries(b, moves, 1, &counts));
		assert( moves.size() == counts.size() );
		if ( moves.empty() )
			return 1;
		std::cout << "avoid moves:" << std::endl;
		for ( size_t i=0; i<moves.size(); i++ )
		{
			std::string move = b.toSAN( moves[i] );
			std::cout << move << " count " << counts[i] << std::endl;
		}
		return 1;
	}
	if ( token == "filterpgn" )
	{
		// filter pgn
		filterPgn( line.c_str() + pos );
		return 1;
	}
	if ( token == "loadepd" )
	{
		// epd debug
		if ( !epd.load( line.c_str() + pos ) )
		{
			std::cout << "failed to load epd!" << std::endl;
		}
		return 1;
	}
	if ( token == "runepd" )
	{
		// epd debug run
		runEPDSuite( epd, 2 );
		return 1;
	}
	if ( token == "pbook" )
	{
		pbook();
		return 1;
	}
#ifdef USE_TUNING
	if ( token == "params" )
	{
		TunableParams::get()->dump();
		std::cout.flush();
		return 1;
	}
#endif
	return 0;
}

void Protocol::allocTime(i32 mytime, i32 myinc, i32 /*optime*/, i32 /*opinc*/, i32 movestogo, SearchMode &sm)
{
	sm.absLimit = 0;
	i32 mvl;
	bool suddenDeath = 0;
	if ( movestogo == 0 )
	{
		// sudden death
		suddenDeath = 1;
		movestogo = 25;
		mytime -= 100;		// reserve
	}
	mvl = (mytime + movestogo * myinc) / movestogo;
	if ( suddenDeath && mvl > mytime/2 )
		mvl = mytime/2;

	if ( movestogo == 1 )
		sm.absLimit = mvl -= 100;	// reserve
	else
		sm.absLimit = mytime/4;

	if ( mvl <= 0 )
		mvl = -1;
	if ( sm.absLimit <= 0 )
		sm.absLimit = -1;
	sm.maxTime = mvl;
}

// is game over (CECP)?
int Protocol::isGameOverCECP() const
{
	return adjudicated;
}

}
