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
#include <math.h>
#include "TreeNode.h"
#include <stdio.h>

// includes for White
#include "Color.h"

static TreeNode *Nodes;
static TreeNode *RootNode;
static TreeNode *FirstFree;


TreeNode::TreeNode(void)
{
	Reset();
}

TreeNode::~TreeNode(void)
{
}

void TreeNode::InitTree()
{
	int i;
	if(Nodes) delete[] Nodes;
	Nodes = new TreeNode[MAXNODES];

	for(i = MAXNODES-2;i >= 0;i--)
	{
		Nodes[i].Reset();
		Nodes[i].NextSibbling = &Nodes[i+1];
	}
	FirstFree = &Nodes[0];
}

void TreeNode::Reset()
{
	Parent = FirstChild = NextSibbling = 0;
	RealVal = UNDEFINED;
	OptVal = UNDEFINED;
	PessVal = UNDEFINED;
	OptPrb = 0.0;
	MoveCount = 0;
	Color = White;
	Move = 0;
	fen[0] = '\0';
	MoveStr[0] = '\0';
	SubtreeSize = 0;
	NChecks = 0;
	NCaptures = 0;
	Flags = Normal;
}
// Get the first free node
TreeNode *TreeNode::GetFree()
{
	TreeNode *aux = FirstFree;
	if(!FirstFree)
		InitTree();
	aux = FirstFree;
		
	FirstFree = FirstFree->NextSibbling;
	aux->NextSibbling = 0;
	return aux;
}

void TreeNode::SetRootNode(TreeNode *New)
{
	RootNode = New;
}

TreeNode *TreeNode::GetRootNode()
{
	return RootNode;
}

TreeNode *TreeNode::MoveTo(MoveMode mode)
{
	switch(mode)
	{
	case PARENT:
		return Parent;
	case FIRSTCHILD:
		return FirstChild;
	case NEXTSIBBLING:
		return NextSibbling;
	case LASTCHILD:
		TreeNode *aux;
		aux = FirstChild;
		while(aux->NextSibbling)
			aux = aux->NextSibbling;
		return aux;
	}
	return 0;
}

void TreeNode::Add(TreeNode *New)
{
	TreeNode *aux;
	assert(this->Color != New->Color);
	if(!FirstChild)
	{
		FirstChild = New;
	}
	else
	{
		aux = FirstChild;
		while(aux->NextSibbling)
			aux = aux->NextSibbling;
		aux->NextSibbling = New;
	}
	New->Parent = this;
}

void TreeNode::Delete()
{
	// don't delete rootnode subtree
	// recursive on child
	if(!this) return;
	if(FirstChild)
	{
		if(FirstChild == RootNode)
		{
			RootNode->Parent = 0;
			if(RootNode->NextSibbling)
			{
				RootNode->NextSibbling->Delete();
				RootNode->NextSibbling = 0;
			}
		}
		else
		{
			FirstChild->Delete();
		}
	}
	// recursive on Sibblings
	if(NextSibbling)
	{
		if(NextSibbling == RootNode)
		{
			RootNode->Parent = 0;
			if(RootNode->NextSibbling)
			{
				RootNode->NextSibbling->Delete();
				RootNode->NextSibbling = 0;
			}
		}
		else
			NextSibbling->Delete();
	}
	Reset();
	this->NextSibbling = FirstFree;
	FirstFree = this;
}

const int INFINITO = 999999;

TreeNode *TreeNode::SelectBestReal()
{
	int Val;
	int Opt;
	TreeNode *aux;
	TreeNode *Sel = 0;
	// Maximize
	Val = -INFINITO;
	Opt = -INFINITO;
	for(aux = this->MoveTo(FIRSTCHILD);aux; aux = aux->MoveTo(NEXTSIBBLING))
	{
		if(aux->RealVal == UNDEFINED) continue;
		if(aux->RealVal > Val)
		{
			Sel = aux;
			Val = Sel->RealVal;
			Opt = Sel->OptVal;
		}
		else
			if(aux->RealVal == Val && aux->OptVal > Opt)
			{
				Sel = aux;
				Val = Sel->RealVal;
				Opt = Sel->OptVal;
			}
	}
	if(Sel == NULL)
		Sel = MoveTo(FIRSTCHILD);
	return Sel;
}

TreeNode *TreeNode::SelectBestRealNotDraw()
{
	int Val;
	int Opt;
	TreeNode *aux;
	TreeNode *Sel = 0;

	// Maximize
	Val = -INFINITO;
	Opt = -INFINITO;
	for(aux = this->MoveTo(FIRSTCHILD);aux; aux = aux->MoveTo(NEXTSIBBLING))
	{
		if(aux->Stopper) continue;
		if(aux->RealVal == UNDEFINED) continue;
		if(aux->RealVal > Val)
		{
			Sel = aux;
			Val = Sel->RealVal;
			Opt = Sel->OptVal;
		}
		else
			if(aux->RealVal == Val && aux->OptVal > Opt)
			{
				Sel = aux;
				Val = Sel->RealVal;
				Opt = Sel->OptVal;
			}
	}
	if(Sel == NULL)
		Sel = MoveTo(FIRSTCHILD);
	return Sel;
}

TreeNode *TreeNode::SelectBestOpt()
{
	int Val,Real;
	TreeNode *aux;
	TreeNode *Sel = 0;
	// Maximize
	Real = Val = -INFINITO;
	for(aux = this->MoveTo(FIRSTCHILD);aux; aux = aux->MoveTo(NEXTSIBBLING))
	{
		if(aux->OptVal != UNDEFINED)
		if(aux->OptVal > Val)
		{
			Sel = aux;
			Val = Sel->OptVal;
			Real = Sel->RealVal;
		}
		else
			if(aux->OptVal == Val && aux->RealVal > Real)
			{
				Sel = aux;
				Val = Sel->OptVal;
				Real = Sel->RealVal;
			}

	}
	if(!Sel)  // no best optval found
		Sel = SelectBestReal();
	return Sel;
}
TreeNode *TreeNode::SelectBestPess()
{
	int Val;
	TreeNode *aux;
	TreeNode *Sel = 0;
	// Maximize
	Val = -INFINITO;
	for(aux = this->MoveTo(FIRSTCHILD);aux; aux = aux->MoveTo(NEXTSIBBLING))
	{
		if(aux->PessVal != UNDEFINED)
		if(aux->PessVal > Val)
		{
			Sel = aux;
			Val = Sel->PessVal;
		}
	}
	if(!Sel)  
		Sel = SelectBestReal();
	return Sel;
}

const int MATE = 30000;

TreeNode *TreeNode::SelectBestOptPrb()
{
	double Val;
	TreeNode *aux;
	TreeNode *BestReal = SelectBestReal();
	int dpv = BestReal->PVDepth()+1;
	TreeNode *Sel = 0;
	int dsel = 0;
	double PrbD = 0.0;

	// Maximize
	Val = 0;
	for(aux = this->MoveTo(FIRSTCHILD);aux; aux = aux->MoveTo(NEXTSIBBLING))
	{
		if(aux->SubtreeSize)
		PrbD = aux->OptPrb;// / aux->SubtreeSize; 
		else
			PrbD = aux->OptPrb;
		if(  PrbD > Val	)
		{
			Sel = aux;
			Val = PrbD; 
		}
	}
	// PV debe ser tan largo como cualquier otro.
	if(!Sel) Sel = BestReal; 
	dsel = Sel->PVDepth();
	if(dpv < dsel)
		return BestReal;
	return Sel;
}

TreeNode *TreeNode::SelectBestOptPrbNotDraw()
{
	double Val;
	int Real = -MATE;
	TreeNode *aux,*aux2;
	TreeNode *BestReal = SelectBestReal();
	int dpv = BestReal->PVDepth()+1;
	TreeNode *Sel = 0;
	int dsel = 0;
	double PrbD = 0.0;

	// Maximize
	Val = 0;
	for(aux = this->MoveTo(FIRSTCHILD);aux; aux = aux->MoveTo(NEXTSIBBLING))
	{
		if(aux->Stopper)
			continue;
		aux2 = aux->SelectBestReal();
		if(aux2 && aux2->Stopper)
			continue;

		PrbD = aux->OptPrb;
		if(  PrbD > Val	)
		{
			Sel = aux;
			Val = PrbD; 
			Real = aux->RealVal;
		}
		else
		{
			if(PrbD == Val && aux->RealVal > Real)
			{
				Sel = aux;
				Val = PrbD; 
				Real = aux->RealVal;
			}
		}
	}
	// PV debe ser tan largo como cualquier otro.
	if(!Sel) Sel = BestReal; 
	dsel = Sel->PVDepth();
	if(dpv < dsel)
		return BestReal;
	return Sel;
}
#include <stdlib.h>
TreeNode *TreeNode::TraceDown(int OptReal)
{
	TreeNode *Sel = 0;
	if(!OptReal)
	{	// optimism
		Sel = SelectBestOptPrbNotDraw();
	}
	else
	{	// real
		Sel = SelectBestRealNotDraw();
	}
	if(Sel)
		return Sel->TraceDown(!OptReal);
	return this;
}
TreeNode *TreeNode::TraceDownNotStopper(int OptReal)
{
	TreeNode *Sel = 0;
	if(!OptReal)
	{	// optimism
		Sel = SelectBestOptPrbNotDraw();
	}
	else
	{	// real
		Sel = SelectBestReal();
	}
	if(Sel)
	{
		if(Sel->Stopper)
		{
			this->Stopper = 1;
		}
		return Sel->TraceDownNotStopper(!OptReal);
	}
	return this;
}

int TreeNode::PVDepth()
{
	int depth = 0;
	TreeNode *aux;
	for(aux = TreeNode::GetRootNode()->SelectBestReal();aux; aux = aux->SelectBestReal())
	{
		depth++;
	}
	return depth;
}
