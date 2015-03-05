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

#include "Hash.h"

Hash::Hash()  : HASH_SIZE ( 0 ) {
    hash_array_greater = hash_array_always = nullptr;
#ifdef DEBUG_MODE
    n_cut_hashA = n_cut_hashE = n_cut_hashB = cutFailed = probeHash = 0;
    n_cut_hash = nRecordHashA = nRecordHashB = nRecordHashE = collisions = 0;
#endif
    setHashSize ( 64 );
}

void Hash::clearAge() {
    for ( int i = 0; i < HASH_SIZE; i++ ) {
        hash_array_greater[i].entryAge = 0;
    }
}

void Hash::clearHash() {
    if ( !HASH_SIZE ) { return; }

    memset ( hash_array_greater, 0, sizeof ( _Thash ) *HASH_SIZE );
}

int Hash::getHashSize() {
    return HASH_SIZE / ( 1024 * 1000 / ( sizeof ( _Thash ) * 2 ) );
}

bool Hash::setHashSize ( int mb ) {
    if ( mb < 1 || mb > 32768 ) { return false; }

    dispose();

    if ( mb ) {
        HASH_SIZE = mb * 1024 * 1000 / ( sizeof ( _Thash ) * 2 );
        hash_array_greater = ( _Thash * ) calloc ( HASH_SIZE, sizeof ( _Thash ) );
        assert ( hash_array_greater );
        hash_array_always = ( _Thash * ) calloc ( HASH_SIZE, sizeof ( _Thash ) );
        assert ( hash_array_always );
    }

    return true;
}

void Hash::recordHash ( bool running, _Thash* phashe_greater, _Thash* phashe_always, const char depth, const char flags, const u64 key, const int score, _Tmove* bestMove ) {
    ASSERT ( key );

    if ( !running ) {
        return;
    }

    ASSERT ( abs ( score ) <= 32200 );
    _Thash* phashe = phashe_greater;
    phashe->key = key;
    phashe->score = score;
    phashe->flags = flags;
    phashe->depth = depth;

    if ( bestMove && bestMove->from != bestMove->to ) {
        phashe->from = bestMove->from;
        phashe->to = bestMove->to;
    }
    else {
        phashe->from = phashe->to = 0;
    }

    phashe = phashe_always;

    if ( phashe->key && phashe->depth >= depth  && phashe->entryAge ) {
        INC ( collisions );
        return;
    }

#ifdef DEBUG_MODE

    if ( flags == hashfALPHA ) {
        nRecordHashA++;
    }
    else if ( flags == hashfBETA ) {
        nRecordHashB++;
    }
    else {
        nRecordHashE++;
    }

#endif
    phashe->key = key;
    phashe->score = score;
    phashe->flags = flags;
    phashe->depth = depth;
    phashe->entryAge = 1;

    if ( bestMove && bestMove->from != bestMove->to ) {
        phashe->from = bestMove->from;
        phashe->to = bestMove->to;
    }
    else {
        phashe->from = phashe->to = 0;
    }
}

void Hash::dispose() {
    if ( hash_array_greater ) {
        free ( hash_array_greater );
    }

    if ( hash_array_always ) {
        free ( hash_array_always );
    }

    hash_array_greater = hash_array_always = nullptr;
    HASH_SIZE = 0;
}

Hash::~Hash() {
    dispose();
}


