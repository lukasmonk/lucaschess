#ifndef WINGLET_HASH_H_
#define WINGLET_HASH_H_

#include "defines.h"

// random  64-bit keys to give every position an 'almost' unique signature:

struct HashKeys
{
	// total size = 1093 * 8 = 8744 bytes (minimum required is 6312):
	U64 keys[64][16];  // position, piece (only 12 out of 16 piece are values used)
	U64 side;          // side to move (black)
	U64 ep[64];        // ep targets (only 16 used)
	U64 wk;            // white king-side castling right
	U64 wq;            // white queen-side castling right
	U64 bk;            // black king-side castling right
	U64 bq;            // black queen-side castling right

	void init();       // initialize the random data
	U64 rand64();      // 64-bit random number generator
};

#endif