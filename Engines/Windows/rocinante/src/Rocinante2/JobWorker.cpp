//    Copyright 2009-2012 Antonio Torrecillas Gonzalez
//
//    This file is part of Rocinante.
//
//    Rocinante is free software: you can redistribute it and/or modify
//    it under the terms of the GNU General Public License as published by
//    the Free Software Foundation, either version 3 of the License, or
//    (at your option) any later version.
//
//    Rocinante is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//    GNU General Public License for more details.
//
//    You should have received a copy of the GNU General Public License
//    along with Rocinante.  If not, see <http://www.gnu.org/licenses/>
//

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include "CDebug.h"
#include "MoveList.h"
#include "UndoData.h"
#include "Board.h"
#include "TreeNode.h"
#include "JobWorker.h"

const int MateOpt = 5000; 
const int CheckOpt = 2000;
const int MateMove = 70; 

const int CheckValueDepth = 130;
const int PesoLegales = 40; 
const int ValorReyForzado = 600; 

JobWorker::JobWorker(void)
{
}
JobWorker::~JobWorker(void)
{
}
void JobWorker::DoJob(void)
{
	extern int Cancel;
	if(Cancel == 1) return;
	Node->HNCount = 0;

	_Board.LoadFen(Node->fen);
	int mc = Node->MoveTo(PARENT)->MoveCount;
	if(CreditNps)
	{
		_Board.ProbeDepth = CreditNps;
	}
	else
	if(mc > 18)
		_Board.ProbeDepth = 1;
	else
	if(mc > 9)
		_Board.ProbeDepth = 2; 
	else
		_Board.ProbeDepth = 3; 
	

	if(Node->StaticVal == UNDEFINED)
	{
		Node->RealVal = GetRealVal();
		Node->StaticVal = Node->RealVal;
		Node->HNCount = _Board.TotalNodes;
	}
	if(EvalOptimism)
	{
		if(Node->RealVal > (MATE-50) ||
			Node->RealVal < (50-MATE))
		{
			Node->OptVal = Node->RealVal;
		}
		else
		{
			Node->OptVal = GetOptVal(Node->RealVal);
			Node->HNCount += _Board.TotalNodes;
		}
	}
}
//const int ProbeDepth = 3;

int JobWorker::GetOptVal(int RealVal)
{
	int Valor = 0;
	if(_Board.IsCheck())
	{
		return EvalCheck();
	}
	UndoData undo;
	_Board.DoNullMove(undo);
	_Board.TotalNodes = 0;
	int score = _Board.Optimism(); 
	_Board.UndoNullMove(undo);
	Valor = score;

	Valor = DeSmooth(Valor);
	assert(Valor <= UNDEFINED);

	return Valor;
}


int JobWorker::GetRealVal(void)
{
	int Valor = 0;
	_Board.TotalNodes = 0;
	Valor = _Board.Search(); 
	return Valor;
}

int JobWorker::EvalCheck(void)
{
	int Valor = 0;

	int neutras = 0,legales = 0;
	int ReyForzado = 1;

	UndoData undo;
	MoveList List;
	int Move;
	int i;

	_Board.GenPseudoMoves(List);
	for(i = 0; i < List.Length();i++)
	{
		Move = List.Move(i);
		_Board.DoMove(Move,undo);
		// Test legality
		if(_Board.IsLegal())
		{
			legales++;
		}
		_Board.UndoMove(Move,undo);
	}
	if(legales == 0)
		Valor = MATE;
	else
	{
		Valor = _Board.GetEval();
		Valor += CheckOpt; 
		if(ReyForzado && legales < 3)
			Valor += ValorReyForzado;
		if(Valor < 0)
			Valor = 0;
	}

	return Valor;
}
int JobWorker::DeSmooth(int Valor)
{
	const bool ModeDeS = false;
	int DepthNode;
	if(ModeDeS)
		DepthNode = GetDepth(Node);

	if(Valor > (MATE-50))
	{
		if(ModeDeS)
		{
			Valor = MateOpt - DepthNode * MateMove;
		}
		else
		{
			Valor -= MATE;
			Valor-=1;
			Valor *= MateMove;
			Valor = MateOpt - Valor ; 
		}
	}
	else
	if(Valor < (50-MATE))
	{
		if(ModeDeS)
		{
			Valor = -MateOpt + DepthNode * MateMove;
		}
		else
		{
			Valor += MATE;
			Valor-=1;
			Valor *= MateMove;
			Valor = -MateOpt + Valor ;
		}
	}
	return Valor;
}

int JobWorker::GetDepth(TreeNode *node)
{
	int depth = 1;
	TreeNode *aux,*Root;
	for(aux = node->MoveTo(PARENT),Root = TreeNode::GetRootNode(); 
		aux != NULL && aux != Root; 
			aux = aux->MoveTo(PARENT))
	{
		depth++;
	}
	return depth;
}
