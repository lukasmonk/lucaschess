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

#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <stdlib.h>
#include "CDebug.h"
#include "MoveList.h"
#include "UndoData.h"
#include "Board.h"

#include "TreeNode.h"
#include "DumpTree.h"

const int MAXMOVES = 200;

DumpTree::DumpTree(void)
{
		fd = fopen("tree.txt","a+");
		if(fd)
		{
			Board board;
			TreeNode * Root = TreeNode::GetRootNode();
			if(Root)
			{
				// primero el fen y un diagrama
				fprintf(fd,"Root %s\n",Root->fen);
				board.LoadFen(Root->fen);
				board.Display(fd);
			}
		}
}

DumpTree::~DumpTree(void)
{
	fclose(fd);
	fd = NULL;
}

void DumpTree::Write()
{
	TreeNode *Root = TreeNode::GetRootNode();
	if(fd)
	{
		// primero el fen y un diagrama
		fprintf(fd,"Root %s\n",Root->fen);
		fprintf(fd,"\nSelected move %s\n",Root->SelectBestReal()->MoveStr);
		fprintf(fd,"\nRoot Node Value:");
		DumpNode(Root,0);
	}
}

void DumpTree::DumpNode(TreeNode *FromNode)
{
	TreeNode *aux;
	// recorremos y volccamos la info
	for(aux = FromNode->MoveTo(FIRSTCHILD);
		aux; aux = aux->MoveTo(NEXTSIBBLING))
	{
		fprintf(fd,"Jugada %s Real %d Opt %d SubtreeSize %d Prb %lf Fen %s\n",
			aux->MoveStr,
			aux->RealVal,
			aux->OptVal,
			aux->SubtreeSize,
			aux->OptPrb,
			aux->fen 
			);
	}
}

// Dump the path stoping in the reference node and then each move
// is replaced with a dot.
void DumpTree::DumpPath(TreeNode *FromNode,TreeNode *RefNode)
{
	char buf[0x1000];
	char buf1[0x1000];
	buf[0] = '\0';
	sprintf(buf1," %d Opt %d Pess %d Prb %4.3lf Fen %s",FromNode->RealVal,
		FromNode->OptVal,FromNode->PessVal ,FromNode->OptPrb, FromNode->fen );
	strcpy(buf,buf1);
	while(FromNode && FromNode != RefNode && FromNode->Move)
	{
//		J.Set(FromNode->Move);
		sprintf(buf1," %s %d",FromNode->MoveStr, FromNode->SubtreeSize);
		strcat(buf1,buf);
		strcpy(buf,buf1);
		FromNode = FromNode->MoveTo(PARENT);
	}
	while(FromNode)
	{
		strcpy(buf1,"     ");
		strcat(buf1,buf);
		strcpy(buf,buf1);
		FromNode = FromNode->MoveTo(PARENT);
	}
	fprintf(fd,"%s\n",buf);
}

void DumpTree::DumpNode(TreeNode *FromNode,TreeNode *RefNode)
{
	int i,j;
	int MaxValue = -999999;
	TreeNode *Max = 0;
	TreeNode *aux;
	int NumChilds = 0;
	TreeNode *vector[MAXMOVES];
	if(FromNode->Color == 0)
		MaxValue = -999999;
	else
		MaxValue = 999999;
	// first search nodes no show
	// that is all node with children
	for(aux = FromNode->MoveTo(FIRSTCHILD);
		aux; aux = aux->MoveTo(NEXTSIBBLING))
	{
		if(FromNode->Color == 0)
		{
			if(aux->RealVal > MaxValue)
			{
				Max = aux;
				MaxValue = aux->RealVal;
			}
		}
		else
		{
			if(aux->RealVal < MaxValue)
			{
				Max = aux;
				MaxValue = aux->RealVal;
			}
		}
		if(aux->MoveTo(FIRSTCHILD))
		{
			vector[NumChilds++] = aux;
		}
	}
	if(!Max)
	{
		// there is no child
		DumpPath(FromNode,RefNode);
		return;
	}
	Max = FromNode->SelectBestReal();
	// the best value must be output if he does not have child
	if(Max && !Max->MoveTo(FIRSTCHILD))
	{
		vector[NumChilds++] = Max;
		// if there is no child to output
		// dump this best and we are done.
		if(NumChilds == 1)
		{
			DumpPath(Max,RefNode);
			return;
		}
	}
	if(NumChilds == 1)
		DumpNode(vector[0],RefNode);
	else
	{
		// reorder by value
		for(i = 0; i < NumChilds;i++)
		{
			for(j = i+1; j < NumChilds;j++)
			{
				if(vector[j]->RealVal > vector[i]->RealVal )
				{
					// intercambiar
					aux = vector[i];
					vector[i] = vector[j];
					vector[j] = aux;
				}
			}
		}
		// Dump child nodes
		DumpPath(FromNode,RefNode);
		for(i = 0; i < NumChilds;i++)
		{
			DumpNode(vector[i],FromNode);
		}
	}
}

void DumpTree::DumpRoot()
{
	// recorrer raiz;
	DumpNode(TreeNode::GetRootNode());
}

void DumpTree::Print(const char *fmt, ...)
{
  va_list   ap;
  va_start(ap, fmt);
  vfprintf(fd,fmt, ap);
  va_end(ap);
}
