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

extern int Dithering;
const int MAXNODES = 700000;
enum MoveMode {
	PARENT,FIRSTCHILD,NEXTSIBBLING,LASTCHILD
};
const int UNDEFINED = 999999;
enum FlagsNode {
	Normal,InCheck,Drawn,Capture,
};
class TreeNode
{
private:
	TreeNode *Parent;
	TreeNode *FirstChild;
	TreeNode *NextSibbling;
	void Reset();

public:
	int Color;

	int		OptVal;		//optimistic value of the node for the side-on-move
	int		RealVal;	//best estimate of the true value of the node
	int		PessVal;	//optimistic value for the side-not-on-move, backed up from its subtree
	double	OptPrb;		//probability that a certain target value can be achieved
	int		StaticVal;	// Static evaluation

	int MoveCount;
	int NChecks;
	int NCaptures;
	FlagsNode Flags;   // 0 normal 1 InCheck  2 Drawn (repetition ...)
	int Move;
	int IsRepetition;
	char fen[92];
	char MoveStr[6];
	int SubtreeSize;
	int Stopper;
//	int fase;
	int HNCount;// hardware node count


	TreeNode(void);
	~TreeNode(void);
	TreeNode *MoveTo(MoveMode mode);
	void Add(TreeNode *New);
	void Delete();
	TreeNode *TraceDown(int OptReal);
	TreeNode *TraceDownNotStopper(int OptReal);
	TreeNode *SelectBestReal();
	TreeNode *SelectBestRealNotDraw();
	TreeNode *SelectBestOptPrb();
	TreeNode *SelectBestOptPrbNotDraw();
	TreeNode *SelectBestOpt();
	TreeNode *SelectBestPess();
	
	int PVDepth();

	static TreeNode *GetFree();
	static void SetRootNode(TreeNode *New);
	static TreeNode *GetRootNode();
	static void InitTree();
};
