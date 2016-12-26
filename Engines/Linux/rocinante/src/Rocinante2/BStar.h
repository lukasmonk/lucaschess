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

class BStar
{
public:
	Board _Board;
	int LastChange;
	int TotalExpand;
	int TotalProbes;
	int TotalNodes;
	int InGame;
	int TimeLimit;
	BStar(void);
	~BStar(void);
	void Run();
	void Run(char *fen);
	static void SetProbeDepth(int d);
	void SetDepth(int depth);
	char BestMoveStr[10];
private:
	long ini;
	int TargetVal;
	TreeNode *BestMoveAtRoot;
	int RealValBest;
	int OptVal2ndBest;
	int ColorRoot;
	int ExpandCount;
	int SelectNodes;
	int VerifyNodes;
	int DepthNode;
	int HitAndRun;
	int IsHitAndRun; 
	bool PrintPVV;
	int FixedDepth;
	int TimeExpand;
	int TimeProbe;

	void Expand(TreeNode *SelectedNode,int modo);
	void Search();
	int GetRealValBestAtRoot();
	int OptValAnyOtherMoveAtRoot();
	void RecalProb(TreeNode *node);
	TreeNode *GetBestOptPrb(TreeNode *parent);
	double SecondRootOptPrb();
	void Backup(int mode,TreeNode *node);
	void Propagate1(TreeNode *nodo,TreeNode *parent,int mode);
	double Product(TreeNode *parent,int modo);
	double GetBestChildOptPrb(TreeNode *parent);
	void BackupPrb(TreeNode *node);
	void PropagatePrb1(TreeNode *nodo,TreeNode *parent);
	TreeNode *TraceDownVerify(TreeNode *a);	
	int RealVal2ndBstMoveAtRoot();
	void VerifyInit();
	TreeNode *GetBestOptPrbVerify(TreeNode *parent);
	TreeNode *GetRealValBestVerify(TreeNode *n);
	void VerifyInitNode(int ColorRoot,TreeNode *parent);
	void PropagateDown(TreeNode *parent);
	int EsRepeticion(TreeNode *nodo);
	void Reset();
	void PropagateReal(TreeNode *parent);
	void PropagateOpt(TreeNode *parent);
	void PropagatePess(TreeNode *parent);
	void CalcPrb(TreeNode *parent);
	int GetDepth(TreeNode *node);
	void BackupSubtreeSize(TreeNode *node);
	int OptVal2Best();
	void RecalcSubtreeOptPrb(TreeNode *parent);
	void PrintPV();
	TreeNode *GetNodeOfFEN(TreeNode *parent,char *fen);
	int GetToSq(char *movestr);
	void RecalcSubtreeOptPrbVerify(TreeNode *parent);
	void CalcVerifPrbP(TreeNode *parent);
	void CalcVerifPrb(TreeNode *nodo);
};

const int PLAYER = 0;
const int OPPONENT = 1;
