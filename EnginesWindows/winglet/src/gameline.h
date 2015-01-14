#ifndef WINGLET_GAMELINE_H_
#define WINGLET_GAMELINE_H_
 
#include "move.h"
 
struct GameLineRecord
{
	Move move;
	unsigned char castleWhite;     // White's castle status, CANCASTLEOO = 1, CANCASTLEOOO = 2
	unsigned char castleBlack;     // Black's castle status, CANCASTLEOO = 1, CANCASTLEOOO = 2
	int epSquare;                  // En-passant target square after double pawn move
	int fiftyMove;                 // Moves since the last pawn move or capture
	U64 key;					   // hash key of the position	
};
 
#endif