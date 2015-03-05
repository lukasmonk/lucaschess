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

#ifndef HASH_H_
#define HASH_H_
#include <iostream>
#include <string.h>
#include "namespaces.h"
using namespace _board;

class Hash {
public:
    enum : char { hashfEXACT = 1, hashfALPHA = 0, hashfBETA = 2 };
    Hash();
    virtual ~Hash();
    bool setHashSize ( int mb );
    int getHashSize();
    void clearHash();

protected:

#pragma pack(push)
#pragma pack(1)
    typedef struct {
        u64 key;
        short score;
        char depth;
        uchar from : 6;
        uchar to : 6;
        uchar entryAge : 1;
        uchar flags: 2;
    } _Thash;
#pragma pack(pop)

    int HASH_SIZE;
    _Thash * hash_array_greater;
    _Thash * hash_array_always;
    void recordHash ( bool running, _Thash* phashe_greater, _Thash* phashe_always, const char depth, const char flags, const u64 key, const int score, _Tmove* bestMove );
    void clearAge();
#ifdef DEBUG_MODE
    unsigned nRecordHashA, nRecordHashB, nRecordHashE, collisions;
    unsigned n_cut_hash;
    int n_cut_hashA, n_cut_hashE, n_cut_hashB, cutFailed, probeHash;
#endif

private:
    void dispose();
};

#endif

