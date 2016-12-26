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

#pragma once

class MCTS_AB
{
public:
	Board _Board;
	int LastChange;
	int TotalExpand;
	int TotalProbes;
	int TotalNodes;
	int TimeLimit;
	MCTS_AB(void);
	~MCTS_AB(void);
	void Run();
	void Run(char *fen);
	char BestMoveStr[10];
	int ProbeDepth;
	int FixedDepth;
private:
	long ini;
	TreeNode *BestMoveAtRoot;
	int ExpandCount;
	bool PrintPVV;
	void Expand(TreeNode *SelectedNode,int modo);
	void Backup(int mode,TreeNode *node);
	void Propagate1(TreeNode *nodo,TreeNode *parent,int mode);
	int EsRepeticion(TreeNode *nodo);
	void PropagateReal(TreeNode *parent);
	int GetDepth(TreeNode *node);
	void PrintPV();
	TreeNode *GetNodeOfFEN(TreeNode *parent,char *fen);
	void Search();
	TreeNode *TraceDown(TreeNode *parent);
};

const int PLAYERMC = 0;
const int OPPONENTMC = 1;
