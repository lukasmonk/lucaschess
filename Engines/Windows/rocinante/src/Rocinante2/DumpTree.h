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

class DumpTree
{
public:
	DumpTree(void);
	~DumpTree(void);
	void Write();
	void DumpNode(TreeNode *FromNode);
	void DumpPath(TreeNode *FromNode,TreeNode *RefNode);
	void DumpRoot();
	void Print(const char *fmt, ...);
private:
	void DumpNode(TreeNode *FromNode,TreeNode *RefNode);
	FILE *fd;
};
