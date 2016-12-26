//    Copyright 2010 Antonio Torrecillas Gonzalez
//
//    This file is part of Simplex.
//
//    Simplex is free software: you can redistribute it and/or modify
//    it under the terms of the GNU General Public License as published by
//    the Free Software Foundation, either version 3 of the License, or
//    (at your option) any later version.
//
//    Simplex is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//    GNU General Public License for more details.
//
//    You should have received a copy of the GNU General Public License
//    along with Simplex.  If not, see <http://www.gnu.org/licenses/>
//

// Uci.h: interface for the CUci class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_UCI_H__E4D47841_2867_4BA6_9635_BF4E6B292AB6__INCLUDED_)
#define AFX_UCI_H__E4D47841_2867_4BA6_9635_BF4E6B292AB6__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

class CUci  
{
public:
	void Stop();
	void Go();
	void Position();
	void UciNewG();
	void SetOption();
	void Debug();
	int NewBuffer;
	int initDone;
	int InitG;
	char *GTbPath;
	int GTbSize;
	char * GetNextToken();
	char InputBuffer[0x16000];
	void start();
	CUci();
	virtual ~CUci();
private:
	Board board;
	BStar bt;
	MCTS_AB  mc;
	bool UseMCTS_AB;
};

#endif // !defined(AFX_UCI_H__E4D47841_2867_4BA6_9635_BF4E6B292AB6__INCLUDED_)
