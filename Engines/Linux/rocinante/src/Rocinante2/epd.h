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

#include "MCTS_AB.h"

class CEpd
{
public:
	int TestAciertos;
	long TestNodeCount;
	Board board;
	CEpd();
	~CEpd();
	int profundidad;
	int tiempo;
	char *epdfile;
	virtual void Start();
	bool UseMC;
	bool UsePVS;
private:
protected:
#ifdef _MSC_VER
	time_t aclock_ini;
	time_t aclock;
#else
	long aclock_ini;
	long aclock;
#endif
	double Dm; // depth media
	long NodeCount;
	long ProbeCount;
	char Test[0x300];
	char Epd[0x300];
	char Solucion[0x30];
	char SolucionAlg[3][0x30];
	char *aux;
	double BFMedio;
	int bm;	// es un bm o un am best move avoid move
	int i;
	int test_aciertos;
	int test_fallos;
	FILE * fi;
	FILE *fo;
	FILE *fsi;
	FILE *fno;
	int ExpandToSolve;
	int score;
protected:
	virtual bool AbreArchivos();
	virtual void CierraArchivos();
	void SeparaLineaEpd();
	virtual bool ProcesaTestEpd();
	BStar Bt;
	MCTS_AB mc;
};
