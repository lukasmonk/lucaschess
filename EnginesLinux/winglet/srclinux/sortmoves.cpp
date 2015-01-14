#include <iostream>
#include "defines.h"
#include "extglobals.h"
#include "protos.h"
#include "board.h"

void Board::selectmove(int &ply, int &i, int &depth, BOOLTYPE &followpv)
{
	int j, k;
	unsigned int best;
	Move temp;

	// re-orders the move list so that the best move is selected as the next move to try.
	if (followpv && depth > 1)
	{
		for (j = i; j < moveBufLen[ply+1]; j++)
		{
			if (moveBuffer[j].moveInt == lastPV[ply].moveInt)
			{
				temp.moveInt = moveBuffer[j].moveInt;
				moveBuffer[j].moveInt = moveBuffer[i].moveInt;
				moveBuffer[i].moveInt = temp.moveInt;
				return;
			}
		}
	}

	if (nextMove) 
	{
		best = blackHeuristics[moveBuffer[i].getFrom()][moveBuffer[i].getTosq()];
		j = i;
		for (k = i + 1; k < moveBufLen[ply+1]; k++)
		{
			if (blackHeuristics[moveBuffer[k].getFrom()][moveBuffer[k].getTosq()] > best)
			{
				best = blackHeuristics[moveBuffer[k].getFrom()][moveBuffer[k].getTosq()];
				j = k;					
			}
		}
		if (j > i)
		{
			temp.moveInt = moveBuffer[j].moveInt;
			moveBuffer[j].moveInt = moveBuffer[i].moveInt;
			moveBuffer[i].moveInt = temp.moveInt;
		}
	}
	else
	{
		best = whiteHeuristics[moveBuffer[i].getFrom()][moveBuffer[i].getTosq()];
		j = i;
		for (k = i + 1; k < moveBufLen[ply+1]; k++)
		{
			if (whiteHeuristics[moveBuffer[k].getFrom()][moveBuffer[k].getTosq()] > best)
			{
				best = whiteHeuristics[moveBuffer[k].getFrom()][moveBuffer[k].getTosq()];
				j = k;					
			}
		}
		if (j > i)
		{
			temp.moveInt = moveBuffer[j].moveInt;
			moveBuffer[j].moveInt = moveBuffer[i].moveInt;
			moveBuffer[i].moveInt = temp.moveInt;
		}
	}

	return;
}