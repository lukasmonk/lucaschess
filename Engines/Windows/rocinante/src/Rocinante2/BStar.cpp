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
#include "BStar.h"
#include "DumpTree.h"
#include <math.h>

#include "SmpManager.h"

#include "C3FoldRep.h"


extern void Print(const char *fmt, ...);


static	int TransPos = 0;


static int DepthEval = 1;

const int minDepthEval = 1;
const int MinProbeTime = 4;

const double MinAct = 0.17;

//int nps = 300000; //2800000;

int Cancel;
int Running;

const int MaxExpand = 8000; 


BStar::BStar(void)
{
	InGame = 0;
	FixedDepth = 0;
}

BStar::~BStar(void)
{
}


void BStar::SetDepth(int depth)
{
	FixedDepth = depth;
}


const bool DumpData = false;

void BStar::Run(char *fen)
{
	int TiempoLimiteOld;
	DumpTree *dt;
	ini = TimeElapsed();

	Running = 1;
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
	SelectNodes = 72; //72; 
	VerifyNodes = 40; //40; 
	TotalExpand = 0;
	TotalProbes = 0;
	TotalNodes = 0;

	TimeExpand = 0;
	TimeProbe = 0;

	TargetVal = -UNDEFINED;
	BestMoveAtRoot = NULL;
	RealValBest = 0;
	OptVal2ndBest = 0;
	ColorRoot = White;
	ExpandCount = 0;
	TotalExpand = 0;
	if(TiempoLimiteOld == 0)
		TiempoLimiteOld = 60000;
	
	TimeLimit = TiempoLimiteOld-15;
	if(TimeLimit <= 0)
	{
		TimeLimit = TiempoLimiteOld; //60000-15;
	}
	// antes primero a ver si ya tenemos en memoria el nodo a expandir
	TreeNode *raiz = GetNodeOfFEN(TreeNode::GetRootNode(),fen);
	if(raiz && raiz->SubtreeSize && raiz->MoveTo(FIRSTCHILD))
	{
		if(DumpData)
			Print("info string position in Cache saving %d nodes\n",raiz->SubtreeSize);
		TreeNode *OldRoot = TreeNode::GetRootNode();
		TreeNode::SetRootNode(raiz);
		if(OldRoot && OldRoot != raiz)
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
		Expand(raiz,PLAYER);
	}

	if(Cancel) {Running = 0;return;}
//	if(TimeElapsed()-ini < TimeLimit)
		Search(); 

	if(DumpData)
		Print("info string TransPos %d\n",TransPos);
	if(DumpData)
	{
		dt->Print("Probe depth %d, Select Nodes Limit %d, Verify Nodes limit %d\n",
			DepthEval, SelectNodes, VerifyNodes	);
		dt->Print("Time used %d Nodes Expanded %d\n",TimeElapsed()-ini,TotalExpand);
		int TargetVal = ((BestMoveAtRoot->RealVal) + OptVal2Best())/2;
		dt->Print("Target %d\n",TargetVal);

		dt->DumpRoot();
		dt->Write();
	}
	// Print the best move...
	int BestMove;
	_Board.LoadFen(TreeNode::GetRootNode()->fen); // para movetoAlgebra
	BestMove = TreeNode::GetRootNode()->SelectBestReal()->Move;
	int ColorMove = _Board.MoveToAlgebra(BestMove,BestMoveStr);
	Print("bestmove %s\n",BestMoveStr);

	Running = 0;
	if(DumpData)
	{
		delete dt;
	}
}

void BStar::Search()
{
	int Cancelar = 0;
	int i,NExpandFD = 0;
	int LastIteration = -1;
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
	ColorRoot = TreeNode::GetRootNode()->Color;

	NExpandFD = 18;
	for(i=1;i < FixedDepth;i++)
	{
		NExpandFD *= 3; // Branch factor 3
	}
	if(FixedDepth)
	{
		SelectNodes = (NExpandFD * 6)/10;
		VerifyNodes = NExpandFD - SelectNodes;
	}

	for(;;)
	{
		if(LastIteration == TotalExpand)
			break;						// avoid cicle without expands.
		LastIteration = TotalExpand;
		if(Cancel||Cancelar)
			break;
//		printf("SELECT Step %d\n",TotalExpand);
		while (GetRealValBestAtRoot() <= OptValAnyOtherMoveAtRoot())
		{
			if(BestMoveAtRoot != CurBestNode)
			{
				if(CurBestNode != NULL)
					PrintPV();
				CurBestNode = BestMoveAtRoot;
				LastChange = TotalExpand;
				PrintPV();
			}
			if(TreeNode::GetRootNode()->RealVal > (MATE-50)
				|| TreeNode::GetRootNode()->RealVal < (50-MATE)
				)
			{
				Cancelar= 1;
				// switch to verify
				break;
			}


			TargetVal = ((BestMoveAtRoot->RealVal) + OptVal2ndBest)/2;
			
			// it is unlikely that any other move can achieve as good a RealVal ?
			if( SecondRootOptPrb() < MinAct)
			{
				if(GetRealValBestAtRoot() >= (RealVal2ndBstMoveAtRoot() + 1))
//					goto salida;
				break;
			}
			if(DumpData )
			{
				dt->Print("SELECT STEP Color %d Best %d Target Value %d\n",ColorRoot,BestMoveAtRoot->RealVal,TargetVal);
				dt->Write();
			}
//			Select Root Node with greatest OptPrb;
			a = GetBestOptPrb(TreeNode::GetRootNode());
			if(!a)
				break;
			if(a != CurNode)
			{
				CurNode = a;
			}
//			Trace down the child subtree selecting
//			For Player-to-Move nodes, child with largest OptPrb
//			For Opponent-to-Move nodes, child with best RealVal
//			Until arriving at a leaf node;
			SelectedNode = a->TraceDownNotStopper(true);
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
				break;
			Expand(SelectedNode,PLAYER);
			// search path down to new expanded
			SelectedNode = a->TraceDown(true);
			a = SelectedNode;

			if(DumpData)
			{
				dt->Print("Backup Node :");
				dt->DumpPath(a,TreeNode::GetRootNode());
			}

//			Back up Values;
			if(
				SelectedNode->RealVal == 0 
				&& SelectedNode->OptVal == 0
				&& SelectedNode->MoveCount == -1
				)
				Backup(PLAYER,a->MoveTo(PARENT));
			else
				Backup(PLAYER,a);

			// best move at root can change after a Backup
			GetRealValBestAtRoot();
			TargetVal = ((BestMoveAtRoot->RealVal) + OptVal2Best())/2;
			
			BackupPrb(a);

			if(TotalExpand > MaxExpand)
			{
				Cancelar = 1;
				break;
			}
			if(FixedDepth)
			{
				if(TotalExpand > NExpandFD ) //18*(3^FixedDepth))
				{
					Cancelar = 1;
					break; // EffortLimitsExceeded
				}
			}
			else
			if((TimeElapsed()-ini) >= TimeLimit )
			{
				Cancelar = 1;
				break; // EffortLimitsExceeded
			}
			if ((ExpandCount > SelectNodes)) 
			{
				ExpandCount = 0;
				break; // EffortLimitsExceeded
			}

		}
		if(Cancel||Cancelar)
			break;


		TargetVal = RealVal2ndBstMoveAtRoot() + 1;

		if(TargetVal == (-UNDEFINED+1))
			goto salida;
//		printf("VERIFY  Step %d\n",TotalExpand);

		if(DumpData)
		{
			dt->Print("VERIFY Step\n");
			dt->Print("Target Value %d\n",TargetVal);
		}
		
		if(!FixedDepth && (TimeElapsed()-ini) >= TimeLimit )
		{
			break;
		}
		if(FixedDepth)
		{
			if(TotalExpand > NExpandFD) //18*(3^FixedDepth))
			{
				break; // EffortLimitsExceeded
			}
		}

		VerifyInit();
		if(!FixedDepth && (TimeElapsed()-ini) >= TimeLimit )
			break;		
		GetRealValBestAtRoot() ;
		if(BestMoveAtRoot->RealVal < TargetVal && GetRealValBestAtRoot() > OptValAnyOtherMoveAtRoot())
			break; 
		
		if(BestMoveAtRoot->RealVal >= MATE-50 || BestMoveAtRoot->RealVal <= 50-MATE)
			break;

		ExpandCount = 0;
		while(BestMoveAtRoot->RealVal >= TargetVal)
		{
			if(BestMoveAtRoot != CurBestNode)
			{
				CurBestNode = BestMoveAtRoot;
				LastChange = TotalExpand;
				// print info
				PrintPV();
			}

			if(DumpData)
			{
				dt->Print("Verify 1:");
			}

//			Select reply Node with Greatest RealVal
			a = BestMoveAtRoot;
			if(DumpData)
				dt->Print("BestMove Real %d Opt %d Pess %d\n",a->RealVal,a->OptVal,a->PessVal);
//			Trace down the child subtree selecting
//			For Opponent-to-Move nodes, child with largest OptPrb
//			For Player-to-Move nodes, child with best RealVal
//			Until arriving at a leaf node;
			SelectedNode = TraceDownVerify(a);
			if(DumpData)
			{
				dt->Print("Verify Selected node:");
				dt->DumpPath(SelectedNode,TreeNode::GetRootNode());
			}
			if(DumpData)
			{
				dt->Print("Verify 2:");
			}
// TODO
//			if(IsOver(a))
//				return;	// Solved
			if(SelectedNode->RealVal == MATE || SelectedNode->RealVal == -MATE)
				break;
//				goto salida; 
//			Get RealVal for each Child Node of this leaf;
//			If it is an Opponent-to-Move node get OptVals for each Child;
			Expand(SelectedNode,OPPONENT);
			if(SelectedNode->MoveCount > 0)
			{
				SelectedNode = TraceDownVerify(SelectedNode); // Search leaf node 
				Backup(OPPONENT,SelectedNode);			    // and Back up Values;
				if(DumpData)
				{
					dt->Print("Verify Selected node:");
					dt->DumpPath(SelectedNode,TreeNode::GetRootNode());
					dt->DumpRoot();
				}
			}
			else
			{
				if(
					SelectedNode->RealVal == 0 
					&& SelectedNode->OptVal == 0
					&& SelectedNode->MoveCount == -1
					)
					break;
			}
			if(DumpData)
			{
				dt->Print("Verify 3:");
			}
			if(TotalExpand > MaxExpand)
			{
				Cancelar = 1;
				break;
			}
			if(FixedDepth)
			{
				if(TotalExpand > NExpandFD) // 18*(3^FixedDepth))
				{
					goto salida;
					break; // EffortLimitsExceeded
				}
			}
			else
			if (((TimeElapsed()-ini) >= TimeLimit && ExpandCount > 3 )|| (ExpandCount > VerifyNodes))
			{
				ExpandCount = 0;
				if((TimeElapsed()-ini) >= TimeLimit )
				{
					goto salida;
				}
				break;
			}
		}
		if(DumpData)
		{
			int TargetVal = ((BestMoveAtRoot->RealVal) + OptVal2Best())/2;
			dt->Print("Target %d\n",TargetVal);
			dt->Write();
		}
	}
salida:
	PrintPV();
	if(DumpData)
	{
		delete dt;
	}

}
void BStar::Reset()
{
	TreeNode::InitTree();
}
// default test implementation
void BStar::Run()
{
	Run("r3kb1r/1pp3p1/p3bp1p/5q2/3QN3/1P6/PBP3P1/3RR1K1 w kq - bm Qd7+;");
	// id "WAC.217";

}

int BStar::EsRepeticion(TreeNode *nodo)
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

void BStar::BackupSubtreeSize(TreeNode *node)
{
	TreeNode *aux,*childs;
	int sum;
	int stop;

	// while not root
	// goto parent
	for(aux = node->MoveTo(PARENT);aux;aux = aux->MoveTo(PARENT))
	{
		// sum child subtreesize
		stop = 1;
		sum = 1;
		for(childs = aux->MoveTo(FIRSTCHILD);childs;childs = childs->MoveTo(NEXTSIBBLING))
		{
			sum += childs->SubtreeSize;
			stop *= childs->Stopper; // if all childs are stopper this is a stopper.
		}
		aux->SubtreeSize = sum;
		aux->Stopper = stop;
	}
}
//			Get RealVal for each Child Node of this leaf;
//			If it is a Player-to-Move node get OptVals for each Child;
void BStar::Expand(TreeNode *SelectedNode,int modo)
{
int EsRaiz;
	EsRaiz = SelectedNode == TreeNode::GetRootNode();
	//PLY ply(0);
	//CJugada J;
	int NMoves = 0;
	bool EvalOpt;
//	int StatusEnroque; 
//	int alpaso;
	int TotalNodesPVS = 0;
	int fase = 0;
	TreeNode *aux;

	// it that position a transposition ?
	//aux= GetNodeOfFEN(TreeNode::GetRootNode(),SelectedNode->fen);
	//if(aux && aux != SelectedNode)
	//{
	//	TransPos++;
	//	TransPos += aux->SubtreeSize;
	//}

	ExpandCount++;
	TotalExpand++;
	if(!EsRaiz && EsRepeticion(SelectedNode))
	{
		SelectedNode->RealVal = 0;
		SelectedNode->OptVal = 0;
		SelectedNode->OptPrb = 0;
		SelectedNode->PessVal = 0;
		SelectedNode->MoveCount = -1;
		SelectedNode->SubtreeSize++;
		SelectedNode->Stopper = 1;
		RecalProb(SelectedNode);
		BackupSubtreeSize(SelectedNode);
		return;
	}
	if(SelectedNode->MoveCount != 0) return; // already expanded ?

	_Board.LoadFen(SelectedNode->fen);

	if(EsRaiz)
	{
		SelectedNode->Color = _Board.wtm;
		SelectedNode->OptVal = UNDEFINED;
		SelectedNode->PessVal = -UNDEFINED;
	}


	SelectedNode->Color = _Board.wtm;
	SelectedNode->SubtreeSize = 1;
	DepthNode = GetDepth(SelectedNode);
	// determinar si necesitamos optvalues
	EvalOpt = (SelectedNode->Color == TreeNode::GetRootNode()->Color);

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
			// agregamos un nodo.
			aux = TreeNode::GetFree();
			aux->Color = _Board.wtm;
			aux->Stopper = 0;
			aux->Move = Move;
			_Board.MoveToAlgebra(Move,aux->MoveStr);
			aux->Flags = Normal;
			if(undo.capture || undo.IsPromote)
				aux->Flags = Capture;
			else
			if(_Board.IsCheck())
				aux->Flags = InCheck;
			assert(aux->OptVal <= UNDEFINED);
			assert(aux->PessVal <= UNDEFINED);

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
		int ParamJob = DepthEval;
		// segunda fase calculamos el valor real
		// Calculamos el valor real y el optimista.
		// Get Real and optimistic value.
		for(aux = SelectedNode->MoveTo(FIRSTCHILD);aux; aux = aux->MoveTo(NEXTSIBBLING))
		{
			// verify repets
			if(EsRepeticion(aux))
			{
				aux->RealVal = 0;
				aux->OptVal = 0;
				aux->OptPrb = 0;
				aux->PessVal = 0;
				aux->MoveCount = -1;
				aux->Stopper = 1;
			}
			else
			{
				aux->OptVal = -SelectedNode->PessVal;
				aux->PessVal = -SelectedNode->OptVal;
				aux->StaticVal = UNDEFINED;
				aux->RealVal = -MATE;
				SmpWorkers.DoWork(aux,EvalOpt,ParamJob);
			}
		}
	}
	// wait for all workers end his job
	while(!SmpWorkers.AllIdle())SmpWorkers.Sleep();

	for(aux = SelectedNode->MoveTo(FIRSTCHILD);aux; aux = aux->MoveTo(NEXTSIBBLING))
	{
		TotalNodes += aux->HNCount;
	}
	if(SelectedNode->MoveCount == 0)
	{
		if(_Board.IsCheck())
		{
			// mate
			SelectedNode->RealVal = -MATE;
			SelectedNode->OptVal = -MATE;
			SelectedNode->PessVal = -MATE;
			SelectedNode->Stopper = 1;
		}
		else
		{
			// stalemate
			SelectedNode->RealVal = 0;
			SelectedNode->OptVal = 0;
			SelectedNode->PessVal = 0;
			SelectedNode->Stopper = 1;
		}
	}
	else
	{
		aux = SelectedNode->SelectBestReal();
		SelectedNode->RealVal = -aux->RealVal;
	}
	BackupSubtreeSize(SelectedNode);
}


int BStar::OptVal2Best()
{
	TreeNode *aux,*Root;
	int Value = -UNDEFINED;
	int opt = 0;
	Root = TreeNode::GetRootNode();
	Value = -UNDEFINED;
	for(aux = Root->MoveTo(FIRSTCHILD);aux;aux = aux->MoveTo(NEXTSIBBLING))
	{
		if(aux != BestMoveAtRoot )
		{
			if(aux->RealVal > Value)
			{
				Value = aux->RealVal; 
				opt = aux->OptVal;
			}
			else
			if(aux->RealVal == Value && aux->OptVal > opt)
			{
				Value = aux->RealVal; 
				opt = aux->OptVal;
			}
		}
	}
	return opt;
}

int BStar::GetRealValBestAtRoot()
{
	BestMoveAtRoot = TreeNode::GetRootNode()->SelectBestReal();
	if(!BestMoveAtRoot)
		return RealValBest = 0;
	return RealValBest = BestMoveAtRoot->RealVal;
}

// Best OptVal but not BestAtRoot
int BStar::OptValAnyOtherMoveAtRoot()
{
	TreeNode *FromNode = TreeNode::GetRootNode();
	TreeNode *aux;
	int MaxValue = -UNDEFINED;
	for(aux = FromNode->MoveTo(FIRSTCHILD);
		aux; aux = aux->MoveTo(NEXTSIBBLING))
	{
		if(aux == BestMoveAtRoot)
			continue;
		if(aux->OptVal > MaxValue )
		{
			MaxValue = aux->OptVal;
		}
	}
	OptVal2ndBest = MaxValue;
	return OptVal2ndBest;
}

void BStar::RecalProb(TreeNode *node)
{
	const double MinOpt = 0.0;
	int MTargetVal;
	if(node->Color == ColorRoot) MTargetVal = -TargetVal; else MTargetVal = TargetVal;
	if(MTargetVal >= node->OptVal) node->OptPrb = 0.0;
	else if(MTargetVal <= node->RealVal) node->OptPrb = 1.0;
	else 
		node->OptPrb = (node->OptVal- MTargetVal)*1.00/(1.0*(node->OptVal-node->RealVal));
	if(node->OptPrb  > 1.0)
		node->OptPrb  = 1.0;
	if(node->OptPrb  < 0.0)
		node->OptPrb  = MinOpt;
}

TreeNode *BStar::GetBestOptPrb(TreeNode *parent)
{
	double MaxValue = 0.0;
	double PrbD = 0.0;
	TreeNode *aux = 0;
	TreeNode *Sel = 0;

	int DPV = 0; // depth PV
	int DOpt = 0;
	for(aux = parent->SelectBestReal();aux; aux = aux->SelectBestReal())
	{
		DPV++;
	}


	double np = parent->SubtreeSize;
	const double Cucb = 2.0; //0.0000001; //2000.0;

	for(aux = parent->MoveTo(FIRSTCHILD);
		aux; aux = aux->MoveTo(NEXTSIBBLING))
	{
		PrbD = aux->OptPrb ;
		if(	PrbD > MaxValue	)
		{
			MaxValue = PrbD; 
			Sel = aux ;
		}
	}
	
	// PV can not be shorter than opt path
	if(Sel == NULL) Sel = parent->SelectBestReal();
	aux = Sel->TraceDown(1);
	for(;aux && aux != TreeNode::GetRootNode(); aux = aux->MoveTo(PARENT))
		DOpt++;
	if(DOpt > DPV)
		return parent->SelectBestReal();
	return Sel;
}

double BStar::SecondRootOptPrb()
{
	double MaxValue = 0.0;
	TreeNode *aux = 0;
	for(aux = TreeNode::GetRootNode()->MoveTo(FIRSTCHILD);
		aux; aux = aux->MoveTo(NEXTSIBBLING))
	{
		RecalProb(aux);
		if(aux->OptPrb >= MaxValue 
			&& aux != BestMoveAtRoot
			)
		{
			MaxValue = aux->OptPrb;
		}
	}
	return MaxValue ;
}

void BStar::PropagateReal(TreeNode *parent)
{
	int Value;
	TreeNode *Best;
	Best = parent->SelectBestReal();
	if(!Best)
		return; // defensive
	Value = Best->RealVal;
	if(Value > MATE-50)
	{
		Value--;
	}
	else
	if(Value < 50-MATE)
	{
		Value++;
	}
	parent->RealVal = -Value;
}
void BStar::PropagateOpt(TreeNode *parent)
{
	int Value;
	TreeNode *son = parent->SelectBestOpt();
	if(!son) return; // defensive
	Value = son->OptVal;
	if(Value > MATE || Value < -MATE)
		return;
	if(Value > MATE-50)
	{
		Value--;
	}
	else
	if(Value < 50-MATE)
	{
		Value++;
	}
	parent->PessVal = -Value; 
}
void BStar::PropagatePess(TreeNode *parent)
{
	int Value;
	TreeNode *son = parent->SelectBestPess();
	if(!son)return; // defensive

	Value = son->PessVal;
	if(Value > MATE || Value < -MATE)
		return;
	if(Value > MATE-50)
	{
		Value--;
	}
	else
	if(Value < 50-MATE)
	{
		Value++;
	}
	parent->OptVal = -Value; 
}

void BStar::Propagate1(TreeNode *nodo,TreeNode *parent,int mode)
{
	if(mode == PLAYER)
	{
		if(nodo->Color == ColorRoot)
		{
			TreeNode *BP = parent->SelectBestReal();
			parent->OptVal = -BP->PessVal;
			parent->OptPrb = Product(parent,0);
			parent->RealVal = -BP->RealVal;
		}
		else
		{
			TreeNode *BP = parent->SelectBestOptPrb();
			parent->OptPrb = GetBestChildOptPrb(parent);
			BP = parent->SelectBestReal();
			parent->PessVal = -BP->OptVal;
			parent->RealVal = -BP->RealVal;
		}
	}
	else
	{
		if(nodo->Color != ColorRoot )
		{
			TreeNode *BP = parent->SelectBestReal();
			parent->OptVal = -BP->PessVal;
			parent->OptPrb = Product(parent,1);
			parent->RealVal = -BP->RealVal;
		}
		else
		{
			TreeNode *BP = parent->SelectBestReal();
			parent->PessVal = -BP->OptVal;
			parent->OptPrb = GetBestChildOptPrb(parent);
			parent->RealVal = -BP->RealVal;
		}
	}
}
void BStar::CalcVerifPrbP(TreeNode *parent)
{
	int NumMoves,StartPos,pos,i;
	TreeNode *aux;
	aux = parent->MoveTo(FIRSTCHILD);
	for(;aux; aux = aux->MoveTo(NEXTSIBBLING))
	{
		CalcVerifPrb(aux);
	}
}
void BStar::CalcVerifPrb(TreeNode *nodo)
{
	if(nodo == TreeNode::GetRootNode())
	{
		nodo->OptPrb = 0.999;
		return;
	}

	if(TargetVal >= nodo->RealVal)
	{
		if(TargetVal < nodo->OptVal)
		{
			nodo->OptPrb = (1.0*(TargetVal - nodo->OptVal ))/(nodo->RealVal - nodo->OptVal);
		}
		else
		{
			nodo->OptPrb = 0.0;
		}
	}
	else // (TargetVal >= nodo->RealVal)
	{
		if(TargetVal > nodo->OptVal)
		{
			nodo->OptPrb = (1.0*(TargetVal - nodo->OptVal ))/(nodo->RealVal - nodo->OptVal);
		}
		else
		{
			nodo->OptPrb = 1.0;
		}
	}

	if(nodo->OptPrb > 1.0)
		nodo->OptPrb = 1.0;
	if(nodo->OptPrb < 0.0)
		nodo->OptPrb = 0.0;
}

void BStar::Backup(int mode,TreeNode *node)
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

double BStar::Product(TreeNode *parent,int modo)
{
	double producto = 1.0;
	TreeNode *aux;
	aux = parent->MoveTo(FIRSTCHILD);
	for(;aux; aux = aux->MoveTo(NEXTSIBBLING))
	{
		assert(aux->RealVal  < (UNDEFINED-50));
		producto *= aux->OptPrb;
		if(producto == 0.0)break;
	}
	return producto;
}

double BStar::GetBestChildOptPrb(TreeNode *parent)
{
	double MaxValue = -1.0;
	TreeNode *aux;

	for(aux = parent->MoveTo(FIRSTCHILD);
		aux; aux = aux->MoveTo(NEXTSIBBLING))
	{
		if(aux->OptPrb > MaxValue)
		{
			MaxValue = aux->OptPrb;
		}
	}
	return MaxValue;
}

void BStar::CalcPrb(TreeNode *parent)
{
	int lTargetVal = (parent->Color != ColorRoot) ? -TargetVal : TargetVal;
	if(parent->OptVal != UNDEFINED)
	{
		assert(parent->OptVal != UNDEFINED);
		// (o-t)/(o-r)
		if(parent->OptVal == parent->RealVal)
			parent->OptPrb = 0;
		else
			parent->OptPrb = ((parent->OptVal-lTargetVal)*1.0)/(1.0*(parent->OptVal-parent->RealVal));
		if(parent->OptPrb < 0)
			parent->OptPrb = 0;
		if(parent->OptPrb > 1)
			parent->OptPrb = 1;
	}
	else
	{
		assert(parent->PessVal != UNDEFINED);
		// (p-t)/(p-r)
		if(parent->PessVal == parent->RealVal)
			parent->OptPrb = 0;
		else
			parent->OptPrb = ((parent->PessVal-lTargetVal)*1.0)/(1.0*(parent->PessVal-parent->RealVal));
		if(parent->OptPrb < 0)
			parent->OptPrb = 0;
		if(parent->OptPrb > 1)
			parent->OptPrb = 1;
	}
}

void BStar::BackupPrb(TreeNode *node)
{
	TreeNode *aux = 0;
	TreeNode *aux1 = node;

	for(aux = node->MoveTo(PARENT);
		aux; aux = aux->MoveTo(PARENT))
	{
		PropagatePrb1(aux1,aux);
		aux1 = aux;
	}
}

void BStar::PropagatePrb1(TreeNode *nodo,TreeNode *parent)
{

	if(nodo->Color != ColorRoot ) 
		parent->OptPrb = Product(parent,0);
	else
		parent->OptPrb =  GetBestChildOptPrb(parent);
}

//			Trace down the child subtree selecting
//			For Opponent-to-Move nodes, child with largest OptPrb
//			For Player-to-Move nodes, child with best RealVal
//			Until arriving at a leaf node;

TreeNode *BStar::TraceDownVerify(TreeNode *a)
{
	TreeNode *SelectedNode,*aux,*aux2;
	int dpv,dleaf;

	// calculate depth of PV
	dpv = 1;
	for(SelectedNode = a;SelectedNode; SelectedNode = SelectedNode->SelectBestReal())
		dpv++;

	SelectedNode = a;
	while( SelectedNode && SelectedNode->MoveTo(FIRSTCHILD)) // while(childs)
	{
		a = SelectedNode;
		if(SelectedNode->Color != ColorRoot)
		{
			aux = GetBestOptPrbVerify(SelectedNode);
			// continue to end
			aux2 = TraceDownVerify(aux);
			// a mate node can't be expanded
			if(aux2 && (aux2->RealVal == MATE || aux2->RealVal == -MATE))
			{
				SelectedNode = GetRealValBestVerify(SelectedNode);
			}
			else
			{
				// mesure depth
				dleaf = 0;
				while(aux2 && aux2 != TreeNode::GetRootNode())
				{
					dleaf++;
					aux2 = aux2->MoveTo(PARENT);
				}
				if(dleaf >=dpv)
				{
					// search best path
					SelectedNode = GetRealValBestVerify(SelectedNode);
				}
				else
					SelectedNode = aux;
			}
		}
		else
		{
			SelectedNode = GetRealValBestVerify(SelectedNode);
		}
	}
	if(SelectedNode)
		return SelectedNode;
	return a;
}

int BStar::RealVal2ndBstMoveAtRoot()
{
	TreeNode *aux = 0;
	int MaxValue;
	MaxValue = -UNDEFINED;
	for(aux = TreeNode::GetRootNode()->MoveTo(FIRSTCHILD);
		aux; aux = aux->MoveTo(NEXTSIBBLING))
	{
		if(aux->RealVal > MaxValue && aux != BestMoveAtRoot)
		{
			MaxValue = aux->RealVal;
		}
	}
	return MaxValue ;
}

void BStar::VerifyInitNode(int ColorRoot,TreeNode *parent)
{
	TreeNode *aux;

	if(!(parent->MoveCount)) return;

	int ParamJob = DepthEval;
	for(aux = parent->MoveTo(FIRSTCHILD);
		aux; aux = aux->MoveTo(NEXTSIBBLING))
	{
		if(!FixedDepth)
		if((TimeElapsed()-ini) >= TimeLimit )
			break;

		if(aux->OptVal == UNDEFINED)
		{
			SmpWorkers.DoWork(aux,true,ParamJob);
		}
	}
// wait for all workers end his job
	while(!SmpWorkers.AllIdle())SmpWorkers.Sleep();

	for(aux = parent->MoveTo(FIRSTCHILD);
		aux; aux = aux->MoveTo(NEXTSIBBLING))
	{
		TotalNodes += aux->HNCount;
		CalcVerifPrb(aux);
		VerifyInitNode(ColorRoot,aux);
	}
}
// Compute OptVal for all Opp nodes and calc OptPrb
void BStar::VerifyInit()
{
	// recorrer el arbol
	TreeNode *aux = TreeNode::GetRootNode();
	int ColorRoot = aux->Color;
	VerifyInitNode(ColorRoot,aux);
}

TreeNode *BStar::GetBestOptPrbVerify(TreeNode *parent)
{
	double MaxValue = -1.0;
	double PrbD = 0.0;

	TreeNode *aux,*aux2;
	TreeNode *Sel = 0;
	for(aux = parent->MoveTo(FIRSTCHILD);
		aux; aux = aux->MoveTo(NEXTSIBBLING))
	{
		// don't allow draw and mate eval leaf nodes... why expand?
		if(aux->Stopper)
			continue;
		aux2 = aux->SelectBestReal();
		if(aux2 && aux2->Stopper)
			continue;

		PrbD = aux->OptPrb;
		if(	PrbD > MaxValue	)
		{
			MaxValue = PrbD; 
			Sel = aux ;
		}
	}
	return Sel;
}

// Best for player -> maximize
TreeNode *BStar::GetRealValBestVerify(TreeNode *n)
{
	return n->SelectBestReal(); 
}

void BStar::PropagateDown(TreeNode *parent)
{
	TreeNode *aux;

	for(aux = parent->MoveTo(FIRSTCHILD);
		aux; aux = aux->MoveTo(NEXTSIBBLING))
	{
		aux->PessVal = -parent->OptVal;
	}
}
int BStar::GetDepth(TreeNode *node)
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


void BStar::RecalcSubtreeOptPrb(TreeNode *parent)
{
	TreeNode *aux;
	aux = parent->MoveTo(FIRSTCHILD);
	if(!aux)
	{
		// if leaf recalc OptPrb
		RecalProb(parent);
	}
	else
	{
		// if not leaf recalc each child
		for(;aux;aux = aux->MoveTo(NEXTSIBBLING))
		{
			RecalcSubtreeOptPrb(aux);
		}
		// backup value
		if(parent->Color != ColorRoot )
		{
			parent->OptPrb =  Product(parent,0);
		}
		else
			parent->OptPrb = GetBestChildOptPrb(parent);
	}
}

void BStar::RecalcSubtreeOptPrbVerify(TreeNode *parent)
{
	TreeNode *aux;
	aux = parent->MoveTo(FIRSTCHILD);
	if(!aux)
	{
		// if leaf recalc OptPrb
		RecalProb(parent);
	}
	else
	{
		// if not leaf recalc each child
		for(;aux;aux = aux->MoveTo(NEXTSIBBLING))
		{
			RecalcSubtreeOptPrb(aux);
		}
		// backup value
		if(parent->Color != ColorRoot )
		{
			parent->OptPrb = GetBestChildOptPrb(parent);
		}
		else
			parent->OptPrb =  Product(parent,0);
	}
}

void BStar::PrintPV()
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
			//TotalNodes, //TotalExpand ,
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
			//TotalNodes, //TotalExpand ,
			nps,
			0,	
			timeUsed);
	}
}
void BStar::SetProbeDepth(int d)
{
//	FixedDepth = true;
	DepthEval = d;
}

TreeNode *BStar::GetNodeOfFEN(TreeNode *parent,char *fen)
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
int BStar::GetToSq(char *movestr)
{
	int sq = 0;
	sq = (movestr[2]-'a')*8+(movestr[3]-'1');
	return sq;
}

