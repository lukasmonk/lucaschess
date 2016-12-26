#include <stdlib.h>
#include "CDebug.h"
#include "MoveList.h"
#include "UndoData.h"
#include "Board.h"
#include <memory.h>

// similar to genallmoves but for both colors and to fill attacks array.
void Board::GenAttacks()
{
   int me, opp;
   int from, to;
   int capture;
   const int * inc_ptr;
   int inc;
   int i;

   me = wtm;
   opp = ColorOpp(me);

   // piece moves
   // reset attacks
   memset(wAttacks,0,sizeof(wAttacks));
   wManBits = 0x0ull;
   memset(bAttacks,0,sizeof(bAttacks));
   bManBits = 0x0ull;
   // White chess man
	// Knight
	for(i=0; i < NumWKnight;i++)
	{
		wManBits += 1ull << (PawnBitMask+i);
		from = wKnightList[i];
		ASSERT(board[from] == wKnight);
        for (inc_ptr = &KnightDir[0]; (inc=*inc_ptr) != Empty; inc_ptr++) {
			to = from + inc;
			capture = board[to];
			wAttacks[to] += 1ull << (KnightBitMask+i);
        }
	}
	// Bishop
	for(i=0; i < NumWBishop;i++)
	{
		wManBits += 1ull << (BishopBitMask+i);
		from = wBishopList[i];
		ASSERT(board[from] == wBishop);
		for (inc_ptr = &BishopDir[0]; (inc=*inc_ptr) != Empty; inc_ptr++) {
			for (to = from+inc; (capture=board[to]) == Empty; to += inc) {
				wAttacks[to] += 1ull << (BishopBitMask+i);
			}
			wAttacks[to] += 1ull << (BishopBitMask+i);
			if(capture == wPawn && inc > 0)
			{
				// hidden attacker
				to = to+inc;
				wAttacks[to] += 1ull << (BishopBitMask+i);
			}
		}
	}
	// Rook
	for(i=0; i < NumWRook;i++)
	{
		wManBits += 1ull << (RookBitMask+i);
		from = wRookList[i];
		ASSERT(board[from] == wRook);
			for (inc_ptr = &RookDir[0]; (inc=*inc_ptr) != Empty; inc_ptr++) {
				for (to = from+inc; (capture=board[to]) == Empty; to += inc) {
					wAttacks[to] += 1ull << (RookBitMask+i);
				}
				wAttacks[to] += 1ull << (RookBitMask+i);
				if(capture == wRook)
				{
					to += inc;
					for (; (capture=board[to]) == Empty; to += inc) {
						wAttacks[to] += 1ull << (RookBitMask+i);
					}
					wAttacks[to] += 1ull << (RookBitMask+i);
				}
			}
	}
	// Queen
	for(i=0; i < NumWQueen;i++)
	{
		wManBits += 1ull << (QueenBitMask+i);
		from = wQueenList[i];
		ASSERT(board[from] == wQueen);
		for (inc_ptr = &QueenDir[0]; (inc=*inc_ptr) != Empty; inc_ptr++) {
			for (to = from+inc; (capture=board[to]) == Empty; to += inc) {
				wAttacks[to] += 1ull << (QueenBitMask+i);
			}
			wAttacks[to] += 1ull << (QueenBitMask+i);
			if(capture == wQueen 
				|| (capture == wRook && CanAttack(wRook,to-from))
				|| (capture == wBishop && CanAttack(wBishop,to-from))
				)
			{
				to += inc;
				for (; (capture=board[to]) == Empty; to += inc) {
					wAttacks[to] += 1ull << (QueenBitMask+i);
				}
				wAttacks[to] += 1ull << (QueenBitMask+i);
			}
			if(capture == wPawn && inc > 0 && CanAttack(bBishop,to-from))
			{
				to += inc;
				wAttacks[to] += 1ull << (QueenBitMask+i);
			}
		}
	}
	// King
	from = wKingPos;
	ASSERT(board[from] == wKing);
	for (inc_ptr = &KingDir[0]; (inc=*inc_ptr) != Empty; inc_ptr++) {
		wManBits += 1ull << (KingBitMask);
		to = from + inc;
		capture = board[to];
		wAttacks[to] += 1ull << (KingBitMask);
	}
	// Pawn
	for(i=0; i < NumWPawn;i++)
	{
		wManBits += 1ull << (PawnBitMask+i);
		from = wPawnList[i];
		ASSERT(board[from] == wPawn);
		to = from + 15;
		wAttacks[to] += 1ull << (PawnBitMask+i);

		to = from + 17;
		wAttacks[to] += 1ull << (PawnBitMask+i);
	}
	// Knight
	for(i=0; i < NumBKnight;i++)
	{
		bManBits += 1ull << (KnightBitMask+i);
		from = bKnightList[i];
		ASSERT(board[from] == bKnight);
        for (inc_ptr = &KnightDir[0]; (inc=*inc_ptr) != Empty; inc_ptr++) {
			to = from + inc;
			capture = board[to];
			bAttacks[to] += 1ull << (KnightBitMask+i);
        }
	}
	// Bishop
	for(i=0; i < NumBBishop;i++)
	{
		bManBits += 1ull << (BishopBitMask+i);
		from = bBishopList[i];
		ASSERT(board[from] == bBishop);
		for (inc_ptr = &BishopDir[0]; (inc=*inc_ptr) != Empty; inc_ptr++) {
			for (to = from+inc; (capture=board[to]) == Empty; to += inc) {
				bAttacks[to] += 1ull << (BishopBitMask+i);
			}
			bAttacks[to] += 1ull << (BishopBitMask+i);
			if(capture == bPawn && inc < 0)
			{
				to +=inc;
				bAttacks[to] += 1ull << (BishopBitMask+i);
			}
		}
	}
	// Rook
	for(i=0; i < NumBRook;i++)
	{
		bManBits += 1ull << (RookBitMask+i);
		from = bRookList[i];
		ASSERT(board[from] == bRook);
			for (inc_ptr = &RookDir[0]; (inc=*inc_ptr) != Empty; inc_ptr++) {
				for (to = from+inc; (capture=board[to]) == Empty; to += inc) {
					bAttacks[to] += 1ull << (RookBitMask+i);
				}
				bAttacks[to] += 1ull << (RookBitMask+i);
				if(capture == bRook)
				{
					to += inc;
					for (;(capture=board[to]) == Empty; to += inc) {
						bAttacks[to] += 1ull << (RookBitMask+i);
					}
					bAttacks[to] += 1ull << (RookBitMask+i);
				}
			}
	}
	for(i=0; i < NumBQueen;i++)
	{
		bManBits += 1ull << (QueenBitMask+i);
		from = bQueenList[i];
		ASSERT(board[from] == bQueen);
		for (inc_ptr = &QueenDir[0]; (inc=*inc_ptr) != Empty; inc_ptr++) {
			for (to = from+inc; (capture=board[to]) == Empty; to += inc) {
				bAttacks[to] += 1ull << (QueenBitMask+i);
			}
			bAttacks[to] += 1ull << (QueenBitMask+i);
			if(capture == bQueen ||
				(capture == bRook && CanAttack(bRook,to-from))
				||( capture == bBishop && CanAttack(bBishop,to-from)))
			{
				to += inc;
				for (; (capture=board[to]) == Empty; to += inc) {
				bAttacks[to] += 1ull << (QueenBitMask+i);
				}
				bAttacks[to] += 1ull << (QueenBitMask+i);
			}
			if(capture == bPawn && inc < 0 && CanAttack(bBishop,to-from))
			{
				to += inc;
				bAttacks[to] += 1ull << (QueenBitMask+i);
			}
		}
	}
	// King
	from = bKingPos;
	ASSERT(board[from] == bKing);
	for (inc_ptr = &KingDir[0]; (inc=*inc_ptr) != Empty; inc_ptr++) {
		bManBits += 1ull << (KingBitMask);
		to = from + inc;
		capture = board[to];
		bAttacks[to] += 1ull << (KingBitMask);
	}
	// Pawn
	for(i=0; i < NumBPawn;i++)
	{
		bManBits += 1ull << (PawnBitMask+i);
		from = bPawnList[i];
		ASSERT(board[from] == bPawn);
		to = from - 15;
		bAttacks[to] += 1ull << (PawnBitMask+i);

		to = from - 17;
		bAttacks[to] += 1ull << (PawnBitMask+i);
	}
}

int Board::SeeSquare(u64 bits1,u64 bits2,int bet)
{
static const int WeightMan[KingBitMask+1] = {
	100,100,100,100,100,100,100,100,
	320,320,320,320,320,320,320,320,
	330,330,330,330,330,330,330,330,
	500,500,500,500,500,500,500,500,
	900,900,900,900,900,900,900,900,
	32000
};
	int Value = 0;
	int BestValue = 0;
	int i;
	for(i=0; i <=KingBitMask;i++)
	{
		if(bits1 & (1ull <<i)) // move possible
		{
			// do move
			bits1 -= 1ull<<i;
			// eval
			Value = bet-SeeSquare(bits2,bits1,WeightMan[i]);
			// takeback
			bits1 += 1ull<<i;
			if(Value > BestValue)
				BestValue = Value;
		}
	}
	return BestValue;
}

//
// SeeBoard must try to predict the outcome of a quiesce applied to a given position
//
int Board::SeeBoard()
{
	int Value = 0;
	int BestValue = 0;
	int sq,i;
	u64 wBits,bBits;
	// simple See
	if(wtm == White)
	{
		// foreach black man 
		// see that square.
		for(i=0; i < NumBQueen;i++)
		{
			sq = bQueenList[i];
			wBits = wAttacks[sq];
			bBits = bAttacks[sq];
			Value = SeeSquare(wBits,bBits,900);
			if(Value > BestValue)
				BestValue = Value;
		}
		for(i=0; i < NumBRook;i++)
		{
			sq = bRookList[i];
			wBits = wAttacks[sq];
			bBits = bAttacks[sq];
			Value = SeeSquare(wBits,bBits,500);
			if(Value > BestValue)
				BestValue = Value;
		}
		for(i=0; i < NumBBishop;i++)
		{
			sq = bBishopList[i];
			wBits = wAttacks[sq];
			bBits = bAttacks[sq];
			Value = SeeSquare(wBits,bBits,330);
			if(Value > BestValue)
				BestValue = Value;
		}
		for(i=0; i < NumBKnight;i++)
		{
			sq = bKnightList[i];
			wBits = wAttacks[sq];
			bBits = bAttacks[sq];
			Value = SeeSquare(wBits,bBits,320);
			if(Value > BestValue)
				BestValue = Value;
		}
		for(i=0; i < NumBPawn;i++)
		{
			sq = bPawnList[i];
			wBits = wAttacks[sq];
			bBits = bAttacks[sq];
			Value = SeeSquare(wBits,bBits,100);
			if(Value > BestValue)
				BestValue = Value;
		}
	}
	else
	{
		for(i=0; i < NumWQueen;i++)
		{
			sq = wQueenList[i];
			wBits = wAttacks[sq];
			bBits = bAttacks[sq];
			Value = SeeSquare(bBits,wBits,900);
			if(Value > BestValue)
				BestValue = Value;
		}
		for(i=0; i < NumWRook;i++)
		{
			sq = wRookList[i];
			wBits = wAttacks[sq];
			bBits = bAttacks[sq];
			Value = SeeSquare(bBits,wBits,500);
			if(Value > BestValue)
				BestValue = Value;
		}
		for(i=0; i < NumWBishop;i++)
		{
			sq = wBishopList[i];
			wBits = wAttacks[sq];
			bBits = bAttacks[sq];
			Value = SeeSquare(bBits,wBits,330);
			if(Value > BestValue)
				BestValue = Value;
		}
		for(i=0; i < NumWKnight;i++)
		{
			sq = wKnightList[i];
			wBits = wAttacks[sq];
			bBits = bAttacks[sq];
			Value = SeeSquare(bBits,wBits,320);
			if(Value > BestValue)
				BestValue = Value;
		}
		for(i=0; i < NumWPawn;i++)
		{
			sq = wPawnList[i];
			wBits = wAttacks[sq];
			bBits = bAttacks[sq];
			Value = SeeSquare(bBits,wBits,100);
			if(Value > BestValue)
				BestValue = Value;
		}
	}
	return BestValue;
}
int Board::Undefend()
{
	int Value = 0;
	int Undefend = 0;
	int BestValue = 0;
	int sq,i;
	u64 wBits,bBits;
	// simple See
	if(wtm == Black)
	{
		// foreach black man 
		// see that square.
		for(i=0; i < NumBQueen;i++)
		{
			sq = bQueenList[i];
			wBits = wAttacks[sq];
			bBits = bAttacks[sq];
			if(bBits == 0ull)
			{
				Undefend += Parameters[QUndefend];
			}
			Value = SeeSquare(wBits,bBits,900);
			if(Value > BestValue)
				BestValue = Value;
		}
		for(i=0; i < NumBRook;i++)
		{
			sq = bRookList[i];
			wBits = wAttacks[sq];
			bBits = bAttacks[sq];
			if(bBits == 0ull)
			{
				Undefend += Parameters[RUndefend];
			}
			Value = SeeSquare(wBits,bBits,500);
			if(Value > BestValue)
				BestValue = Value;
		}
		for(i=0; i < NumBBishop;i++)
		{
			sq = bBishopList[i];
			wBits = wAttacks[sq];
			bBits = bAttacks[sq];
			if(bBits == 0ull)
			{
				Undefend += Parameters[BUndefend];
			}
			Value = SeeSquare(wBits,bBits,330);
			if(Value > BestValue)
				BestValue = Value;
		}
		for(i=0; i < NumBKnight;i++)
		{
			sq = bKnightList[i];
			wBits = wAttacks[sq];
			bBits = bAttacks[sq];
			Value = SeeSquare(wBits,bBits,320);
			if(bBits == 0ull)
			{
				Undefend += Parameters[NUndefend];
			}
			if(Value > BestValue)
				BestValue = Value;
		}
		for(i=0; i < NumBPawn;i++)
		{
			sq = bPawnList[i];
			wBits = wAttacks[sq];
			bBits = bAttacks[sq];
			if(bBits == 0ull)
			{
				Undefend += Parameters[PUndefend];
			}
			Value = SeeSquare(wBits,bBits,100);
			if(Value > BestValue)
				BestValue = Value;
		}
	}
	else
	{
		for(i=0; i < NumWQueen;i++)
		{
			sq = wQueenList[i];
			wBits = wAttacks[sq];
			bBits = bAttacks[sq];
			if(wBits == 0ull)
			{
				Undefend += Parameters[QUndefend];
			}
			Value = SeeSquare(bBits,wBits,900);
			if(Value > BestValue)
				BestValue = Value;
		}
		for(i=0; i < NumWRook;i++)
		{
			sq = wRookList[i];
			wBits = wAttacks[sq];
			bBits = bAttacks[sq];
			if(wBits == 0ull)
			{
				Undefend += Parameters[RUndefend];
			}
			Value = SeeSquare(bBits,wBits,500);
			if(Value > BestValue)
				BestValue = Value;
		}
		for(i=0; i < NumWBishop;i++)
		{
			sq = wBishopList[i];
			wBits = wAttacks[sq];
			bBits = bAttacks[sq];
			if(wBits == 0ull)
			{
				Undefend += Parameters[BUndefend];
			}
			Value = SeeSquare(bBits,wBits,330);
			if(Value > BestValue)
				BestValue = Value;
		}
		for(i=0; i < NumWKnight;i++)
		{
			sq = wKnightList[i];
			wBits = wAttacks[sq];
			bBits = bAttacks[sq];
			if(wBits == 0ull)
			{
				Undefend += Parameters[NUndefend];
			}
			Value = SeeSquare(bBits,wBits,320);
			if(Value > BestValue)
				BestValue = Value;
		}
		for(i=0; i < NumWPawn;i++)
		{
			sq = wPawnList[i];
			wBits = wAttacks[sq];
			bBits = bAttacks[sq];
			Value = SeeSquare(bBits,wBits,100);
			if(wBits == 0ull)
			{
				Undefend += Parameters[PUndefend];
			}
			if(Value > BestValue)
				BestValue = Value;
		}
	}
	BestValue /= 16;
	BestValue += Undefend;
	return BestValue;
}
int Board::SeeEval()
{
	if(IsInCheck())
	{
		ResetHashStack();
		return AlphaBeta(1,-MATE-1,MATE+1);
	}
	int Value = GetEval();
	GenAttacks();
	Value += SeeBoard();
	Value -= Undefend();
	return Value;
}