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

#include <string>
#include <sstream>
#include "search.h"
#include "thread.h"
#include "epd.h"

namespace cheng4
{

enum ProtoType
{
	ptNative,
	ptUCI,
	ptCECP
};

class Engine;
struct SearchInfo;

class Protocol
{
	std::stringstream sendStr;				// send string
	Mutex mutex;							// send mutex
	Mutex parseMutex;						// parse mutex

	// CECP specific stuff
	struct Level
	{
		uint moves;							// 0 = sudden death
		uint mins;							// minutes
		uint secs;							// seconds
		Time inc;							// increment in msec
		uint move;							// base move

		void reset();
	};

	uint multiPV;							// current multipv value (both protocols)

	int protover;							// CECP protocol version
	bool analyze;							// analyze mode
	bool force;								// force mode
	bool clocksRunning;						// clock is running
	Color engineColor;						// engine color
	SearchMode searchMode;					// current searchmode
	Time etime, otime;						// time/opponent time in ms
	Time timeLimit;							// time limit in ms, 0 = none
	NodeCount npsLimit;						// NPS limit, 0 = none
	Depth depthLimit;						// depth limit, 0 = none
	bool invalidState;						// invalid board state
	bool frc;								// frc variant
	bool edit;								// edit mode
	Board editBoard;						// edit board
	Level level;							// current level (tc)
	bool post;								// post (=show thinking)
	bool fixedTime;							// fixed time per move
	i32 startTicks;							// CECP search start ticks (ms)
	uint maxCores;							// CECP maximum # of coress allowed
	int adjudicated;						// game over (adjudicated) bit 1: adj_flag, msbits: result

	// try to adjudicate game after engine/user move
	// if it's win/draw/loss, result is nonzero and appropriate string is sent to GUI
	bool adjudicate();
	void abortSearch();
	void startPondering( bool nostop = 0 );
	void resetClocks();
	void stopClocks();
	bool setTurn( Color c );
	void leaveForce();
	void enterForce();
	// start CECP-mode search
	bool startSearchCECP();
	// set time to think etc.
	void setSearchModeCECP( SearchMode &sm );
	// CECP end

	// generic error message
	void error( const std::string &msg, const std::string &line );
	// illegal move message
	void illegalMove( Move m, const std::string &line );

	// parse UCI command
	bool parseUCI( const std::string &line );
	// parse UCI options
	bool parseUCIOptions( const std::string &line, size_t &pos );
	// parse CECP command
	bool parseCECP( const std::string &line );
	// internal version
	bool parseCECPInternal( const std::string &line );
	// parse edit commands
	bool parseCECPEdit( const std::string &line );
	// parse special (nonstd) command
	bool parseSpecial( const std::string &token, const std::string &line, size_t &pos );

	// allocate think time based on TC
	void allocTime( i32 mytime, i32 myinc, i32 optime, i32 opinc, i32 movestogo, SearchMode &sm );

	EPDFile epd;
public:
	ProtoType type;
	Engine &engine;
	bool volatile quitFlag;

	Protocol( Engine &eng );
	~Protocol();

	void newIteration();

	// called from search
	void searchCallback( const SearchInfo &si );

	// start sending (reset buffer)
	void sendStart( uint flags );
	// start sending
	void sendEnd();

	// send custom info message
	void sendInfo( const std::string &msg );

	// send raw mesage
	void sendRaw( const std::string &msg );

	// send raw EOL
	void sendEOL();

	// send current depth
	void sendDepth( Depth d );
	// send selective depth
	void sendSelDepth( Ply d );
	// send PV
	// passing si because of CECP
	// index: k-best(multipv) zero-based index
	void sendPV( const SearchInfo &si, uint index, Score score, enum BoundType bound, size_t pvCount, const Move *pv );
	// send nodes
	void sendNodes( NodeCount total );
	// send nps
	void sendNPS( NodeCount nps );
	// send time
	void sendTime( Time time );
	// send best move
	void sendBest( Move best, Move ponder = mcNone );
	// called after best move is sent
	void finishBest();
	// send current move index (zero based)
	void sendCurIndex( MoveCount index, MoveCount count );
	// send current move
	void sendCurMove( Move move );
	// send hash full (permill)
	void sendHash( uint full );
	// send TB hits
	void sendTBHits( u64 hits );

	// parse line from GUI
	bool parse( const std::string &line );

	// should parser quit?
	bool shouldQuit() const;

	// is game over (CECP)? (returns adjudicated flag)
	int isGameOverCECP() const;
};

}
