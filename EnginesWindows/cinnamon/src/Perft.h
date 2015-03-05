/*
    Cinnamon is a UCI chess engine
    Copyright (C) 2011-2014 Giuseppe Cannella

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef PERFT_H_
#define PERFT_H_

#include "Search.h"
#include "Thread.h"
#include <iomanip>
#include <atomic>
#include <fstream>
#include <unistd.h>
#include "Timer.h"
#include <mutex>


/*

rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1
 Depth  Perft
 1      20              verified
 2      400             verified
 3      8902            verified
 4      197281          verified
 5      4865609         verified
 6      119060324       verified
 7      3195901860      verified
 8      84998978956     verified
 9      2439530234167   verified
 10     69352859712417  verified

r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq -
Depth   Perft
 1      48              verified
 2      2039            verified
 3      97862           verified
 4      4085603         verified
 5      193690690       verified
 6      8031647685      verified
 7      374190009323    verified
 8      15493944087984  verified

rnbqkbnr/pp1ppppp/8/2pP4/8/8/PPP1PPPP/RNBQKBNR w KQkq c6 0 2
Depth   Perft
 1      30              verified
 2      631             verified
 3      18825           verified
 4      437149          verified
 5      13787913        verified
*/

class Perft {

public:

    const static int secondsToDump = 60 * 180;
    Perft ( string fen, int depth, int nCpu, u64 mbSize, string dumpFile );
    ~Perft();

private:

#pragma pack(push)
#pragma pack(1)
    typedef struct {
        u64 key;
        u64 nMoves;
    } _ThashPerft;
#pragma pack(pop)
    mutex updateHash;
    mutex mutexPrint;
    Timer* timer = nullptr;
    string fen;
    string dumpFile;
    int depth, nCpu;
    u64 mbSize;
    constexpr static u64 RANDSIDE[2] = {0x1cf0862fa4118029ULL, 0xd2a5cab966b3d6cULL};
    _ThashPerft** hash = nullptr;
    u64 sizeAtDepth[255];
    atomic_ullong totMoves;
    void alloc();
    void dump();
    bool load();
    void setResult ( u64 result ) {
        totMoves += result;
    }

    class PerftThread: public Thread, public GenMoves {
    public:

        PerftThread ( int, string fen, int from, int to, Perft* Perft );
        PerftThread();
        virtual ~PerftThread();

    private:
        virtual void run();
        void setDump();
        template <int side, bool useHash> u64 search ( const int depth );
        int from, to, cpuID;
        Perft* perft;
    };
    vector<PerftThread*> threadList;
};
#endif

