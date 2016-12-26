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

#include <cassert>
#include <string.h>
#include <stdlib.h>
#include "system.h"
#include "CDebug.h"
#include "MoveList.h"
#include "UndoData.h"
#include "Board.h"

#include "TreeNode.h"
#include "MCTS_AB.h"
#include "DumpTree.h"
#include <math.h>

#include "SmpManager.h"

#include "C3FoldRep.h"


extern void Print(const char *fmt, ...);


static	int TransPos = 0;


static int DepthEval = 4;

const int minDepthEval = 1;

extern int Cancel;  // defined in BStar
extern int Running; // defined in BStar
const int MaxExpand = 8000; 


MCTS_AB::MCTS_AB(void)
{
	FixedDepth = 0;
	ProbeDepth = 1; // default 1 minimax+quiesce.
}

MCTS_AB::~MCTS_AB(void)
{
}



const bool DumpData = false;

void MCTS_AB::Run(char *fen)
{
	int TiempoLimiteOld;
	DumpTree *dt;
	ini = TimeElapsed();

	Running = 1;

//	Print("info string MCTS AB\n");

	PrintPVV = true;
	if(DumpData)
	{
		dt = new DumpTree();
	}
	if(FixedDepth)
	{
		TiempoLimiteOld = TimeLimit = 0;
	}
	else
	{
		TiempoLimiteOld = TimeLimit;
	}
	ExpandCount = 0;
	TotalExpand = 0;
	TotalProbes = 0;
	TotalNodes = 0;

	BestMoveAtRoot = NULL;
	ExpandCount = 0;
	TotalExpand = 0;
	if(TiempoLimiteOld == 0)
		TiempoLimiteOld = 60000;
	
	TimeLimit = TiempoLimiteOld-15;
	if(TimeLimit <= 0)
	{
		TimeLimit = TiempoLimiteOld; 
	}



	// antes primero a ver si ya tenemos en memoria el nodo a expandir
	TreeNode *raiz = GetNodeOfFEN(TreeNode::GetRootNode(),fen);
	if(raiz && raiz->SubtreeSize)
	{
		if(DumpData)
			Print("info string position in Cache saving %d nodes\n",raiz->SubtreeSize);
		TreeNode *OldRoot = TreeNode::GetRootNode();
		TreeNode::SetRootNode(raiz);
		OldRoot->Delete();
	}
	else
	{
		if(DumpData)
			Print("info string Position %s not found\n",fen);
		// limpiar el arbol
		raiz = TreeNode::GetRootNode();
		if(raiz)
			raiz->Delete();
		// establecer el nodo raiz
		raiz = TreeNode::GetFree();
		TreeNode::SetRootNode(raiz);
		strcpy(raiz->fen,fen);

		// expand del nodo raiz
		if(Cancel) {Running = 0;return;}
		Expand(raiz,PLAYERMC);
	}

	if(Cancel) {Running = 0;return;}
	if(TimeElapsed()-ini < TimeLimit)
		Search(); 

	if(DumpData)
		Print("info string TransPos %d\n",TransPos);
	if(DumpData)
	{
//		dt->Print("Time used %d Nodes Expanded %d\n",TimeElapsed()-ini,TotalExpand);

		dt->DumpRoot();
		dt->Write();
	}
	// Print the best move...
	int BestMove;
	_Board.LoadFen(TreeNode::GetRootNode()->fen); // para movetoAlgebra
	BestMove = TreeNode::GetRootNode()->SelectBestReal()->Move;
	int ColorMove = _Board.MoveToAlgebra(BestMove,BestMoveStr);
	//if(ColorMove != TreeNode::GetRootNode()->Color)
	//{
	//	// colores cambiados jugada ilegal;
	//	FILE *fdo=fopen("error.txt","a+");
	//	if(fdo)
	//	{
	//		fprintf(fdo,"Error Colores cambiados: fen=%s\nmove %s\n",TreeNode::GetRootNode()->fen,
	//			BestMoveStr);
	//		fprintf(fdo,"info string Time %d Expands %d %s to move\n",TimeElapsed()-ini,TotalExpand,TreeNode::GetRootNode()->Color == White ? "White":"Black");
	//		fclose(fdo);
	//	}
	//	exit(0);
	//}
	//Print("info string Time %d Expands %d %s to move\n",TimeElapsed()-ini,TotalExpand,TreeNode::GetRootNode()->Color == White ? "White":"Black");
	Print("bestmove %s\n",BestMoveStr);
	if(false)
	{
		FILE *fdo = fopen("ucient.txt","a+");
		if(fdo)
		{
			fprintf(fdo,"bestmove %s\n",BestMoveStr);
			fclose(fdo);
		}
	}

	Running = 0;
	if(DumpData)
	{
		delete dt;
	}
}


// default test implementation
void MCTS_AB::Run()
{
	Run("r3kb1r/1pp3p1/p3bp1p/5q2/3QN3/1P6/PBP3P1/3RR1K1 w kq - bm Qd7+;");
	// id "WAC.217";

}



int MCTS_AB::EsRepeticion(TreeNode *nodo)
{
	TreeNode *aux;
	for(aux = nodo->MoveTo(PARENT);aux;aux = aux->MoveTo(PARENT))
	{
		if(strcmp(nodo->fen,aux->fen)==0)
		{
			return 1;
		}
	}
	if(ThreeFold.IsRep(nodo->fen))
		return 1;
	return 0;
}

//			Get RealVal for each Child Node of this leaf;
//			If it is a Player-to-Move node get OptVals for each Child;
void MCTS_AB::Expand(TreeNode *SelectedNode,int modo)
{
int EsRaiz;
	EsRaiz = SelectedNode == TreeNode::GetRootNode();
	int NMoves = 0;
//	bool EvalOpt;
	int TotalNodesPVS = 0;
	int fase = 0;
	TreeNode *aux;
	ExpandCount++;
	TotalExpand++;
	if(!EsRaiz && EsRepeticion(SelectedNode))
	{
		SelectedNode->RealVal = 0;
		SelectedNode->OptVal = 0;
		SelectedNode->OptPrb = 0;
		SelectedNode->PessVal = 0;
		SelectedNode->MoveCount = -1;
		return;
	}
	assert(SelectedNode->MoveCount == 0);

	_Board.LoadFen(SelectedNode->fen);

	SelectedNode->Color = _Board.wtm;
	SelectedNode->SubtreeSize = 1;


	UndoData undo;
	MoveList List;
	int Move;
	int i;

	_Board.GenPseudoMoves(List);

	// order by pst values.
	for(i = 0; i < List.Length();i++)
	{
		Move = List.Move(i);

		_Board.DoMove(Move,undo);
		// Test legality
		if(_Board.IsLegal())
		{
			// agregamos un nodo.
			aux = TreeNode::GetFree();
			aux->Color = _Board.wtm;
			aux->Move = Move;

			aux->PessVal = SelectedNode->OptVal;
			aux->OptVal = UNDEFINED;

			_Board.MoveToAlgebra(Move,aux->MoveStr);
			aux->Flags = Normal;
			if(undo.capture || undo.IsPromote)
				aux->Flags = Capture;
			else
			if(_Board.IsCheck())
				aux->Flags = InCheck;

			char dest[100];
			_Board.SaveFEN(&dest[0]);
			strcpy(aux->fen,dest);
			SelectedNode->Add(aux);
			NMoves++;
		}
		_Board.UndoMove(Move,undo);
	}
	SelectedNode->MoveCount = NMoves;

	if(NMoves)
	{

		// segunda fase calculamos el valor real
		// Calculamos el valor real y el optimista.
		// Get Real and optimistic value.
		for(aux = SelectedNode->MoveTo(FIRSTCHILD);aux; aux = aux->MoveTo(NEXTSIBBLING))
		{
			// verify repets
			if(EsRepeticion(aux))
			{
				aux->RealVal = 0;
				aux->MoveCount = -1;
			}
			else
			{
				aux->StaticVal = UNDEFINED;
				aux->RealVal = -MATE;
				SmpWorkers.DoWork(aux,0,ProbeDepth);   
			}
		}
	}
	// wait for all workers end his job
	while(!SmpWorkers.AllIdle())SmpWorkers.Sleep();

	for(aux = SelectedNode->MoveTo(FIRSTCHILD);aux; aux = aux->MoveTo(NEXTSIBBLING))
	{
		aux->StaticVal = aux->StaticVal;
		aux->RealVal = aux->RealVal;
		TotalNodes += aux->HNCount;
	}
	if(SelectedNode->MoveCount == 0)
	{
		if(_Board.IsCheck())
		{
			// mate
			SelectedNode->RealVal = -MATE;
		}
		else
		{
			// stalemate
			SelectedNode->RealVal = 0;
		}
	}
	else
	{
		aux = SelectedNode->SelectBestReal();
		SelectedNode->RealVal = -SelectedNode->SelectBestReal()->RealVal;
	}
}

void MCTS_AB::PropagateReal(TreeNode *parent)
{
	int Value;
	TreeNode *Best;
	Best = parent->SelectBestReal();
	if(!Best)
		return; // defensive

	Value = -Best->RealVal;
	if(Value > MATE-50)
	{
		Value--;
	}
	else
	if(Value < 50-MATE)
	{
		Value++;
	}
	parent->RealVal = Value;
}

void MCTS_AB::Propagate1(TreeNode *nodo,TreeNode *parent,int mode)
{
	PropagateReal(parent);
}

void MCTS_AB::Backup(int mode,TreeNode *node)
{
	TreeNode *aux = 0;
	TreeNode *aux1 = node;

	for(aux = node->MoveTo(PARENT);
		aux; aux = aux->MoveTo(PARENT))
	{
		Propagate1(aux1,aux,mode);
		aux1 = aux;
	}
}



int MCTS_AB::GetDepth(TreeNode *node)
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



void MCTS_AB::PrintPV()
{
	if(!PrintPVV)return;
	int timeUsed;
	char Path[1024];
	int SelDepth = 0;
	TreeNode *aux;
	Path[0] = '\0';
	for(aux = TreeNode::GetRootNode()->SelectBestReal();aux; aux = aux->SelectBestReal())
	{
		strcat(Path,aux->MoveStr);
		strcat(Path," ");
		SelDepth++;
		if(SelDepth > 20) break; // avoid buffer overrun on Path
	}

	timeUsed = TimeElapsed()-ini;
	if(timeUsed <=0)
		timeUsed = 1;
	int nps;
	nps = (TotalExpand * 1000)/ timeUsed;
	int mate;

	int value =  BestMoveAtRoot->RealVal;

	mate = 0;
	if(value >= MATE -50)
	{
		mate = MATE -value;
	}
	if(value <= -MATE +50)
	{
		mate = MATE +value;
		mate = -mate;
	}
	if(!mate)
	{
		Print("info depth %d seldepth %d pv %s score cp %d nodes %d nps %d hashfull %d time %ld\n",
			SelDepth,
			SelDepth,
			Path,
			value,
			TotalExpand ,
			nps,
			0,	
			timeUsed);
	}
	else
	{
		Print("info depth %d seldepth %d pv %s score mate %d nodes %d nps %d hashfull %d time %ld\n",
			SelDepth,
			SelDepth,
			Path,
			mate/2,
			TotalExpand ,
			nps,
			0,	
			timeUsed);
	}
}

TreeNode *MCTS_AB::GetNodeOfFEN(TreeNode *parent,char *fen)
{
	TreeNode *aux,*aux1;
	if(!parent)
		return NULL;

	for(aux = parent->MoveTo(FIRSTCHILD);aux;aux = aux->MoveTo(NEXTSIBBLING))
	{
		if(strcmp(aux->fen,fen)==0)
			return aux;
		aux1 = GetNodeOfFEN(aux,fen);
		if(aux1)
			return aux1;
	}
	return NULL;
}


void MCTS_AB::Search()
{
	int Cancelar = 0;
	Cancelar = 0;
	DumpTree *dt;
	TreeNode *CurNode = NULL;		// last move expanded
	TreeNode *CurBestNode = NULL;	// last best
	if(DumpData)
	{
		dt = new DumpTree();
	}

	TreeNode *a = NULL;
	TreeNode *SelectedNode = NULL;
	for(;;)
	{
		if(Cancel||Cancelar)
		{
			break;
		}
		BestMoveAtRoot = TreeNode::GetRootNode()->SelectBestReal();
		if(BestMoveAtRoot != CurBestNode)
		{
			CurBestNode = BestMoveAtRoot;
			LastChange = TotalExpand;
			PrintPV();
		}
		if(TreeNode::GetRootNode()->RealVal > (MATE-50)
			|| TreeNode::GetRootNode()->RealVal < (50-MATE)
			)
		{
			Cancelar= 1;
			break;
		}

		a = TreeNode::GetRootNode(); 
		if(!a)
			break;
		if(a != CurNode)
		{
			CurNode = a;
		}
//			Trace down the child subtree selecting
//			Until arriving at a leaf node;
		SelectedNode = TraceDown(a); 
		if(DumpData)
		{
			dt->Print("Selected node:");
			dt->DumpPath(SelectedNode,TreeNode::GetRootNode());
			dt->Print("Color root %d\n",TreeNode::GetRootNode()->Color);
			dt->DumpRoot();
		}
		if(SelectedNode->MoveCount != 0) // already expanded, mate pos?
		{
			break;
		}
//			Get RealVal for each Child Node of this leaf;
//			If it is a Player-to-Move node get OptVals for each Child;
		if(Cancelar||Cancel)
		{
			break;
		}
		Expand(SelectedNode,PLAYERMC);
		// search path down to new expanded
		a = SelectedNode;

		if(DumpData)
		{
			dt->Print("Backup Node :");
			dt->DumpPath(a,TreeNode::GetRootNode());
		}

//			Back up Values;
		if(
			SelectedNode->RealVal == 0 
//			&& SelectedNode->OptVal == 0
			&& SelectedNode->MoveCount == -1
			)
			Backup(PLAYERMC,a->MoveTo(PARENT));
		else
			Backup(PLAYERMC,a);

		if(FixedDepth)
		{
			if(TotalExpand > 18*(3^FixedDepth))
			{
				Cancelar = 1;
			}
		}
		if(TotalExpand > MaxExpand)
		{
			Cancelar = 1;
			break;
		}
		if ((TimeElapsed()-ini) >= TimeLimit) 
		{
			ExpandCount = 0;
			if(FixedDepth)
			{
				if(TotalExpand > 18*(3^FixedDepth))
				{
					Cancelar = 1;
				}
			}
			else
			if((TimeElapsed()-ini) >= TimeLimit )
			{
				Cancelar = 1;
			}
			break; // EffortLimitsExceeded
		}
		if(Cancel||Cancelar)
			break;
	}
salida:
	PrintPV();
	if(DumpData)
	{
		delete dt;
	}

}

TreeNode *MCTS_AB::TraceDown(TreeNode *parent)
{
	static const int MaxNodos = 220;
// primero recogemos toda la info
	int i,indexPV,MinValue,NodesNum,indexE;
	TreeNode *aux;
	TreeNode *Nodos[MaxNodos];
	int Real[MaxNodos];
	int Area[MaxNodos];
	i = 0;
	indexPV = 0;
	NodesNum = 0;
	double np = parent->SubtreeSize;
	// PB*
	const double CteHeu = -400.0; //-400.0;
	const double Cucb = 2000.0; //0.0000001; //2000.0;
	for(aux = parent->MoveTo(FIRSTCHILD);aux; aux = aux->MoveTo(NEXTSIBBLING))
	{
		if(aux->MoveCount == -1) continue; // don't expand draws by repetition.

		Nodos[i] = aux;
		Real[i] = aux->RealVal;
		if(Real[i] > Real[indexPV])
		{
			indexPV = i;
		}
		int visits;
		visits = aux->SubtreeSize+1;
		Area[i] =  (Real[i] )+ sqrt((Cucb * log(np))/(visits)); 
		i++;
	}
	NodesNum = i;
	if(NodesNum == 0)
	{
		if(parent->MoveCount > 0)
			parent->MoveCount = -1; // no new move to expand here.
		return parent;
	}
	parent->SubtreeSize++;

	// select the best uct score
	MinValue = -MATE-1;
	indexE =0;
	for(i=0; i < NodesNum;i++)
	{
		if(Area[i] > MinValue)
		{
			indexE = i;
			MinValue = Area[i];
		}
	}

	return TraceDown(Nodos[indexE]);
}





