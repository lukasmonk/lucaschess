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
#pragma once
class CDiagrama;
class CSort;

enum CenterType {
	WIDEOPEN,OPEN,AVERAGE,CLOSED,BLOCKED,ENDGAME
};

class BEval
{
public:
int Value0;
int PosKw,PosKb;
char rb,mb,qb,kb,kn,bb,bn;
char r,m,q,fase,pfase;
char rn,mn,qn;
char pb,pn;
char fase16;
bool isEndGame;
	CenterType ct;
	int Material;
u64 Bdebiles[2];
u64 atacadaP[2];
u64 PPasados[2];
u64 Sombra[2];

	// capitulos de la evaluación
	int PreEvalacion;
	int Tie;
	int signo;
	int PstValue;
	int ValorPeones;
	int EvalDevelopment;
	int EvalTorres ,EvalCaballos ,EvalAlfiles ,EvalDamas ,EvalReyes;
	int PawnCover;
	int EvalMob;

	BEval(){};
	~BEval(){};
	int Eval(CDiagrama *board);
	int GetMaterial(CDiagrama *board);
private:
	inline int FILA(int s)	{return ((s) >> 3);};
	inline int COLUMNA(int s)	{ return ((s) & 7);};
	inline int COLORCASILLA(int s)	{return ((FILA(s) + COLUMNA(s)) % 2);};

	CenterType  IdCenter(CDiagrama &Board);
	void		PreEval(CDiagrama &Board);
	void		EvalMaterial();
	void		EvaluaPeones(CDiagrama &Board);
	void		CalculaDebiles(CDiagrama &Board);
	int			Backward(CDiagrama &Board);
	int			EvaluaPeonesDoblados(CDiagrama &Board);
	int			EvaluaPst(int mode,CDiagrama &Board);
	int			EvaluaAlfiles(CDiagrama &Board);
	int			EvaluaDamas(CDiagrama &Board);
	int			EvaluaTorres(CDiagrama &Board);
	int			PieceDevelopped(CDiagrama &Board,int p,int color);
	int			Development(CDiagrama &Board);
	int			PawnCoverage(CDiagrama &Board);
	int			DivisorEndGame(CDiagrama &Board);
	int			EvalCenter(CDiagrama &Board);
};