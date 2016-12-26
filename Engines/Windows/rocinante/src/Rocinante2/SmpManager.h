//    Copyright 2009 Antonio Torrecillas Gonzalez
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

const int MaxNumOfThreads = 256;

class SmpManager
{
public:

	SmpManager(void);
public:
	~SmpManager(void);

	bool InitDone;
	int NumThreads;

	static void *RunThread(void *data);

	void InitWorkers(int cpus);
	void StopWorkers();
	void DoWork(TreeNode *w,bool EvalOpt,int Credit);
	int AllIdle();
	int AllStopped();
	int AllStarted();
	void Sleep();
};
extern SmpManager SmpWorkers;
