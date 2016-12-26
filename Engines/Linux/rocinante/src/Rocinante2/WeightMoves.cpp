#include <stdlib.h>
#include "CDebug.h"
#include "MoveList.h"
#include "UndoData.h"
#include "Board.h"

//const int PawnMoveScore = 100;
const int PromoteScore = 9100;

void Board::WeightMoves(MoveList &List)
{
	// MVV/LVA
	// PstDiff
	//Mvv-lva
static const int piece_values[16]={0,1000,1000,3000,3000,3000,3000,5000,5000,9000,9000}; 
static const int aggressor_order[16]={0,60,60,50,50,40,40,30,30,20,20,10,10} ;
	int Tope = List.Size();
	int i,move,to,from,score;
	for(i =0; i < Tope;i++)
	{
		move = List.Move(i);
		to = MoveTo(move);
		from = MoveFrom(move);
		if(board[to] != Empty && board[from] != Empty)
		{
			score=piece_values[piece_to_12(board[to])]+aggressor_order[piece_to_12(board[from])]; 
		}
		else
		{
			score = 0;
			if(MoveIsPromote(move) && PromotePiece(move) == Queen)
				score = PromoteScore;
		}
		if(score < 100) score = 0;
		List.SetScore(i,score);
	}
}

void Board::WeightPstMoves(MoveList &List)
{
	int Tope = List.Size();
	int i,move,to,from,score;
	int Piece;
	isEndGame = (NumWRook+NumWKnight+NumWBishop+NumWQueen) < 3 || (NumBRook+NumBKnight+NumBBishop+NumBQueen) < 3;
	for(i =0; i < Tope;i++)
	{
		move = List.Move(i);
		to = MoveTo(move);
		from = MoveFrom(move);
		Piece = board[from];
		to = sq16x12to64(to);
		from = sq16x12to64(from);
		if(isEndGame)
			score = Pst[Piece][to][1] - Pst[Piece][from][1];
		else
			score = Pst[Piece][to][0] - Pst[Piece][from][0];
		List.SetScore(i,score);
	}
}

void Board::WeightMovesPVS(MoveList &List)
{
	// MVV/LVA
	// PstDiff
	//Mvv-lva
static const int piece_values[16]={0,1000,1000,3000,3000,3000,3000,5000,5000,9000,9000}; 
static const int aggressor_order[16]={0,60,60,50,50,40,40,30,30,20,20,10,10} ;
	int Tope = List.Size();
	int i,move,to,from,score;
	for(i =0; i < Tope;i++)
	{
		move = List.Move(i);
		to = MoveTo(move);
		from = MoveFrom(move);
		if(board[to] != Empty && board[from] != Empty)
		{
			score=piece_values[piece_to_12(board[to])]+aggressor_order[piece_to_12(board[from])]; 
		}
		else
		{
			score = 0;
			if(MoveIsPromote(move) && PromotePiece(move) == Queen)
				score = PromoteScore;
		}
		if(score < 100) score = 0;
		// hash move 10000 killers 9000
		if(move == HashJugada)
			score = 10000;
		if(move == Killer[0])
			score = 9000;
		if(move == Killer[1])
			score = 9000;

		List.SetScore(i,score);
	}
}
