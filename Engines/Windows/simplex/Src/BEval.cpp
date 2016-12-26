//    Copyright 2010-2011 Antonio Torrecillas Gonzalez
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
#include <stdlib.h>

#include "Ajedrez.h"

static const bool EvalMobility = true;
extern u64 ataque[8][64]; 
const u64 ColumnasDE = 0x1818181818181818ull;
const u64 ColumnasCD = 0xc0c0c0c0c0c0c0c0ull;
const u64 ColumnasEF = 0x0303030303030303ull;

static const u64 Columnas[8] = {
	ColumnaA,ColumnaB,ColumnaC,ColumnaD,ColumnaE,ColumnaF,ColumnaG,ColumnaH
};

const u64 FILAS234 = 0xffffff00ull;
const u64 FILAS765 = 0x00ffffff00000000ull;

//
// In opening increase value of piece to avoid unsound sacrifices
int  pVal[] = {
Duo(83,101),   // Pawn
Duo(401,308),   // Knight
Duo(401,311),   // Bishop
Duo(505,530),   // Rook
Duo(1088,995), // Queen
Duo(34,46),		// Bishop Pair
};

#include "Parameters.h"

// make Parameter array
#define Param(n,v)	v
short Parameters[MaxParameters] ={
#include "Param.h"
};
#undef Param

int DuoPst[2][8][64];  // color piece square

#include "pst.h"


void InitializeParam()
{
	pVal[0] = Duo(Parameters[PawnOP],Parameters[PawnEG]);
	pVal[1] = Duo(Parameters[KnightOP],Parameters[KnightEG]);
	pVal[2] = Duo(Parameters[BishopOP],Parameters[BishopEG]);
	pVal[3] = Duo(Parameters[RookOP],Parameters[RookEG]);
	pVal[4] = Duo(Parameters[QueenOP],Parameters[QueenEG]);
	pVal[5] = Duo(Parameters[BPairOP],Parameters[BPairEG]);
}

void InitPstP(int p,char *PstOpp,char *PstEnd,int FactorRed,int FactorRedEG,int despOpp,int despEG)
{
	int sq;
	for(sq = 0; sq < 64;sq++)
	{
		DuoPst[0][p][sq] = Duo(((PstOpp[sq] * FactorRed )/256)+despOpp,((PstEnd[sq]*FactorRedEG)/256)+despEG);
		DuoPst[1][p][sq^070] = Duo(-((PstOpp[sq]*FactorRed)/256)-despOpp,-((PstEnd[sq]*FactorRedEG)/256)-despEG);
	}
}

void PrintPstP(FILE *fdo,char *NameOpp,char *NameEG,int p,char *PstOpp,char *PstEnd,int FactorRed,int FactorRedEG,int despOpp,int despEG)
{
	int sq;
	fprintf(fdo,"char %s[64] = {\n",NameOpp);
	for(sq = 0; sq < 64;sq++)
	{
		fprintf(fdo,"%d,",((PstOpp[sq] * FactorRed )/256)+despOpp);
		if((sq %8) == 7)
			fprintf(fdo,"\n");
	}
	fprintf(fdo,"};\n");
	fprintf(fdo,"char %s[64] = {\n",NameEG);
	for(sq = 0; sq < 64;sq++)
	{
		fprintf(fdo,"%d,",((PstEnd[sq]*FactorRedEG)/256)+despEG);
		if((sq %8) == 7)
			fprintf(fdo,"\n");
	}
	fprintf(fdo,"};\n");
}
int Promedia(char *PstOpp)
{
	int sq,media = 0;
	for(sq = 0; sq < 64;sq++)
	{
		media += PstOpp[sq];
	}
	media /= 64;
	return media;
}

void GenPstH()
{
	FILE *fdo = fopen("train/pst.h","w+");
	if(fdo)
	{
	PrintPstP(fdo,"PstPawnOpp","PstPawnEG",peon,PstPawnOpp,PstPawnEG,256,256,0,0);
	PrintPstP(fdo,"PstKnightOpp","PstKnightEG",caballo,PstKnightOpp,PstKnightEG,256,256,0,0);
	PrintPstP(fdo,"PstBishopOpp","PstBishopEG",alfil,PstBishopOpp,PstBishopEG,256,256,0,0);
	PrintPstP(fdo,"PstRookOpp","PstRookEG",torre,PstRookOpp,PstRookEG,256,512,0,0);
	PrintPstP(fdo,"PstQueenOpp","PstQueenEG",dama,PstQueenOpp,PstQueenEG,256,256,0,0);
	PrintPstP(fdo,"PstKingOpp","PstKingEG",rey,PstKingOpp,PstKingEG,256,256,0,0);

		fclose(fdo);
	}
}
void InitializePST() 
{
	InitPstP(peon,PstPawnOpp,PstPawnEG,256,256,0,0);
	InitPstP(caballo,PstKnightOpp,PstKnightEG,256,256,0,0);
	InitPstP(alfil,PstBishopOpp,PstBishopEG,256,256,0,0);
	InitPstP(torre,PstRookOpp,PstRookEG,256,256,0,0);
	InitPstP(dama,PstQueenOpp,PstQueenEG,256,256,0,0);
	InitPstP(rey,PstKingOpp,PstKingEG,256,256,0,0);
	InitializeParam();
}



int CDiagrama::GetMaterial()
{
	Opening = EndGame = 0;
	int Value0 = 0;
	PreEval();

	EvalMaterial();
	Opening += Material[0];
	EndGame += Material[1];

	Opening += OppValue(PstDValue);
	EndGame += EndValue(PstDValue);


	Value0 += ((Opening *(15-fase16) + EndGame *(fase16)) / 15);
	if(ColorJuegan == blanco)
		return Value0;
	else
		return -Value0;
}

#define LAZY 1
int CDiagrama::Eval(int alpha,int beta)
{
	Opening = EndGame = 0;
	SafetyCounters[blanco] = SafetyCounters[negro] = 0;
	SafetyPiece[blanco] = SafetyPiece[negro] = 0;

	PreEval();

	ct = IdCenter();
	EvalMaterial();
	Opening += Material[0];
	EndGame += Material[1];
	if(isEndGame)
	{
		if(fase == 32)
		{
			if((MatCount[blanco][peon]+MatCount[negro][peon]) == 0)		return 0;
			if((MatCount[blanco][peon]+MatCount[negro][peon]) == 1)		return Kpk();
		}
		if(fase == 31)
		{
			static const int _BvP = -90;
			static const int _BvPP = -90;
			static const int _BvPPP = -10;
			static const int _NvP = -90;
			static const int _NvPP = 10;
			static const int _NvPPP = 30;
			if((MatCount[blanco][peon]+MatCount[negro][peon]) == 0)		return 0;
			if(MatCount[blanco][peon] == 1 && MatCount[negro][peon] == 0 &&
				MatCount[negro][alfil] == 1 )
				EndGame = _BvP;
			if(MatCount[blanco][peon] == 0 && MatCount[negro][peon] == 1 &&
				MatCount[blanco][alfil] == 1 )
				EndGame = -_BvP;
			if(MatCount[blanco][peon] == 2 && MatCount[negro][peon] == 0 &&
				MatCount[negro][alfil] == 1 )
				EndGame = _BvPP;
			if(MatCount[blanco][peon] == 0 && MatCount[negro][peon] == 2 &&
				MatCount[blanco][alfil] == 1 )
				EndGame = -_BvPP;
			if(MatCount[blanco][peon] == 3 && MatCount[negro][peon] == 0 &&
				MatCount[negro][alfil] == 1 )
				EndGame = _BvPPP;
			if(MatCount[blanco][peon] == 0 && MatCount[negro][peon] == 3 &&
				MatCount[blanco][alfil] == 1 )
				EndGame = -_BvPPP;
			if(MatCount[blanco][peon] == 1 && MatCount[negro][peon] == 0 &&
				MatCount[negro][caballo] == 1 )
				EndGame = _NvP;
			if(MatCount[blanco][peon] == 0 && MatCount[negro][peon] == 1 &&
				MatCount[blanco][caballo] == 1 )
				EndGame = -_NvP;
			if(MatCount[blanco][peon] == 2 && MatCount[negro][peon] == 0 &&
				MatCount[negro][caballo] == 1 )
				EndGame = _NvPP;
			if(MatCount[blanco][peon] == 0 && MatCount[negro][peon] == 2 &&
				MatCount[blanco][caballo] == 1 )
				EndGame = -_NvPP;
			if(MatCount[blanco][peon] == 3 && MatCount[negro][peon] == 0 &&
				MatCount[negro][caballo] == 1 )
				EndGame = _NvPPP;
			if(MatCount[blanco][peon] == 0 && MatCount[negro][peon] == 3 &&
				MatCount[blanco][caballo] == 1 )
				EndGame = -_NvPPP;
		}
		if(fase == 30)
		{
			if(MatCount[blanco][caballo]+MatCount[blanco][alfil] != 2 &&
				MatCount[negro][caballo]+MatCount[negro][alfil] != 2 )
			if((MatCount[blanco][peon]+MatCount[negro][peon]) == 0)		return 0;
		}
		if(fase == 26)
		{
			if(MatCount[blanco][torre] == 1 && MatCount[negro][torre] == 1)
			if((MatCount[blanco][peon]+MatCount[negro][peon]) == 0)		return 0;
		}
	}

	Opening += OppValue(PstDValue);
	EndGame += EndValue(PstDValue);


	Development();
	Opening += DevelopmentEval;

#ifdef LAZY
	// Lazy eval
	int ValueL = GetTapVal(Opening,EndGame);
	const int MARGIN = 320;
	if(ValueL < (alpha - MARGIN)) return ValueL;
	if(ValueL > (beta + MARGIN)) return ValueL;
#endif

	EvaluaPeones();
	Opening += ValorPeones[0];
	EndGame += ValorPeones[1];	

#ifndef RED
#ifdef LAZY
	// Lazy eval
	ValueL = GetTapVal(Opening,EndGame);
	const int MARGIN1 = 120;
	if(ValueL < (alpha - MARGIN1)) return ValueL;
	if(ValueL > (beta + MARGIN1)) return ValueL;
#endif
	// evalua caballos // Evaluation Knight
	EvalKnight();
	Opening += EvalCaballos[0];
	EndGame += EvalCaballos[1];

#ifdef LAZY
	// Lazy eval
	ValueL = GetTapVal(Opening,EndGame);
	const int MARGIN2 = 120;
	if(ValueL < (alpha - MARGIN2)) return ValueL;
	if(ValueL > (beta + MARGIN2)) return ValueL;
#endif
	// Eval Alfiles // Evaluation Bishop
	EvaluaAlfiles();
	Opening += EvalAlfiles[0];
	EndGame += EvalAlfiles[1];
#ifdef LAZY
	// Lazy eval
	ValueL = GetTapVal(Opening,EndGame);
	const int MARGIN3 = 120;
	if(ValueL < (alpha - MARGIN3)) return ValueL;
	if(ValueL > (beta + MARGIN3)) return ValueL;
#endif
	// EvalDamas // Evaluation Queen
	EvaluaDamas();
	Opening += EvalDamas[0];
	EndGame += EvalDamas[1];
#ifdef LAZY
	// Lazy eval
	ValueL = GetTapVal(Opening,EndGame);
	const int MARGIN4 = 120;
	if(ValueL < (alpha - MARGIN4)) return ValueL;
	if(ValueL > (beta + MARGIN4)) return ValueL;
#endif
	// EvalTorres // Evaluation Rooks
	EvaluaTorres();
	Opening += EvalTorres[0];
	EndGame += EvalTorres[1];

#ifdef LAZY
	// Lazy eval
	ValueL = GetTapVal(Opening,EndGame);
	const int MARGIN5 = 120;
	if(ValueL < (alpha - MARGIN5)) return ValueL;
	if(ValueL > (beta + MARGIN5)) return ValueL;
#endif

	Opening += KingSafety();

#endif
	int Value0 = ((Opening *(15-fase16) + EndGame *(fase16)) / 15);

	// factores de finales // Drawish EndGames
	Value0 *= 16;
	Value0 /= DivisorEndGame();


	if(ColorJuegan == blanco) // convert to side to move perspective.
		return Value0;
	else
		return -Value0;
}

void CDiagrama::EvalMaterial()
{
	unsigned int DuoVal = 0;
	DuoVal += pVal[0] * (MatCount[blanco][peon]-MatCount[negro][peon]);
	DuoVal += pVal[1] * (kb-kn);
	DuoVal += pVal[2] * (bb-bn);
	DuoVal += pVal[3] * (MatCount[blanco][torre]-MatCount[negro][torre]);
	DuoVal += pVal[4] * (qb-qn);
//	 Bishop Pair 
	if(bb ==2)
	{
		DuoVal += pVal[5];
	}
	if(bn ==2)
	{
		DuoVal -= pVal[5];
	}
#ifdef DEBUGDUO
	if(OppValue(DuoVal) != Opening || EndValue(DuoVal) != EndGame)
	{
		// algo va mal
		DuoVal = Duo(Opening,EndGame);
	}
#endif
	Material[0] = OppValue(DuoVal);
	Material[1] = EndValue(DuoVal);
}

CenterType CDiagrama::IdCenter()
{
	u64 aux;

	if(isEndGame ) return ENDGAME;
	if(MatCount[blanco][peon] < 7 && MatCount[negro][peon] < 7)
	{
		// si no hay peones centrales bloqueados
		aux = BPiezas[peon][blanco] & ColumnasDE;
		aux = aux << 8; // avanzamos una casilla
		aux = aux & BPiezas[peon][negro] & ColumnasDE;
		if(!aux)
			return WIDEOPEN;
	}
	// open: one pair of centre pawn have been exchanged
	// and there are no blocked pawn.
	bool candidato = false;
	aux = BPiezas[peon][blanco] & ColumnasDE;
	if(popCount(aux) == 1)
	{
		candidato = true;
	}
	else
	{
		aux = BPiezas[peon][negro] & ColumnasDE;
		if(popCount(aux) == 1)
			candidato = true;

	}
	if(candidato)
	{
		// si no hay peones centrales bloqueados
		aux = BPiezas[peon][blanco] & ColumnasDE;
		aux = aux << 8; // avanzamos una casilla
		aux = aux & BPiezas[peon][negro] & ColumnasDE;
		if(!aux)
			return OPEN;
	}
	if(MatCount[blanco][peon] == 8 && MatCount[negro][peon] == 8)
	{
		// si no hay un par de peones centrales bloqueados
		aux = BPiezas[peon][blanco] & ColumnasDE;
		aux = aux << 8; // avanzamos una casilla
		aux = aux & BPiezas[peon][negro] & ColumnasDE;
		int bloqueos = popCount(aux);
		if(bloqueos == 1)
			return CLOSED;
		if(bloqueos > 1)
			return BLOCKED;
		// ahora miramos adyacentes cd ef
		aux = BPiezas[peon][blanco] & ColumnasCD;
		aux = aux << 8; // avanzamos una casilla
		aux = aux & BPiezas[peon][negro] & ColumnasCD;
		bloqueos = popCount(aux);
		if(bloqueos > 1)
			return BLOCKED;
		aux = BPiezas[peon][blanco] & ColumnasEF;
		aux = aux << 8; // avanzamos una casilla
		aux = aux & BPiezas[peon][negro] & ColumnasEF;
		bloqueos = popCount(aux);
		if(bloqueos > 1)
			return BLOCKED;
	}
	return AVERAGE;
}


void CDiagrama::PreEval()
{
	u64 aux;
	aux = BPiezas[rey][blanco];
	PosKw = bitScanAndReset(aux);
	aux = BPiezas[rey][negro];
	PosKb = bitScanAndReset(aux);

	r = (char)(MatCount[negro][torre]+MatCount[blanco][torre]);

	kb = (char)MatCount[blanco][caballo]; 
	kn = (char)MatCount[negro][caballo]; 
	bb = (char)MatCount[blanco][alfil]; 
	bn = (char)MatCount[negro][alfil]; 

	mb = kb+bb; 
	mn = kn+bn; 
	m = mn+mb;
	qb = (char)MatCount[blanco][dama]; 
	qn = (char)MatCount[negro][dama]; 
	q = qn+qb;

	fase = 32 - (m + r * 3 + q *6); // 0 -> 32
	if(fase < 0)
		fase = 0; // por si hay más de dos damas. // more than one queen correction
	fase16 = fase/2;
	if(fase16 == 16) fase16 = 15;

	pb = (char)MatCount[blanco][peon];
	pn = (char)MatCount[negro][peon];
	pfase = pb+pn;
	isEndGame = (MatCount[blanco][torre]+mb+qb) < 3 || (MatCount[negro][torre]+mn+qn) < 3;
}


void CDiagrama::EvaluaPeones()
{
	register int i;
	bool ReyObstruye;
	int j;
	int Opening,EndGame;
	u64 p;
	u64 aux;
	ValorPeones[0] = ValorPeones[1] = 0;

	Opening = EndGame = 0;
	Ocupadas = BOcupadas[blanco]|BOcupadas[negro];
	
	if(pfase == 0)
	{
		Bdebiles[0] = 0ull;		Bdebiles[1] = 0ull;
		atacadaP[0] = 0ull;		atacadaP[1] = 0ull;
		PPasados[0] = 0ull;		PPasados[1] = 0ull;
		Sombra[0] = 0ull;		Sombra[1] = 0ull;
		return;
	}

	CalculaDebiles();

	u64 ConnectedW;
	u64 ConnectedB;
	// connected if defensed or with 
	ConnectedW = atacadaP[blanco] & BPiezas[peon][blanco];
	aux = BPiezas[peon][blanco];
	ConnectedW |= aux & ((aux & 0xFEFEFEFEFEFEFEFEull) >>1);
	ConnectedW |= aux & ((aux & 0x7F7F7F7F7F7F7F7Full)  <<1);

	ConnectedB = atacadaP[negro] & BPiezas[peon][negro];
	aux = BPiezas[peon][negro];
	ConnectedB |= aux & ((aux & 0xFEFEFEFEFEFEFEFEull) >>1);
	ConnectedB |= aux & ((aux & 0x7F7F7F7F7F7F7F7Full)  <<1);
	// pasados
	int HayPasados = 0;
	p = PPasados[blanco];
	
	j = popCount((p << 8) & Ocupadas);
	Opening += j * Parameters[pn_PassedPInMob];
	EndGame += j * Parameters[pn_PassedPInMobEG];

	int HayPiezas;
	while(p)
	{
		HayPasados = 1;
		i = bitScanAndReset(p);

		j = FILA(i);
		if(ConnectedW & (1ull << i)) // is connected
		{
			Opening += Parameters[_PassedConnected1+j]; 
			EndGame += Parameters[_PassedConnectedEG1+j];
		}
		else
		{
		Opening += Parameters[_Passed1+j]; 
		EndGame += Parameters[_PassedEG1+j];
		}
		// distancias de los reyes a la casilla delante del peon pasado
		if(isEndGame )
		{
			EndGame += Distancia(PosKb,i+8) * Parameters[pn_DistDefEG];
			EndGame += Distancia(PosKw,i+8) * Parameters[pn_DistAttEG];
		}
		// el defensor no tiene piezas y el rey propio no obstruye el paso del peon
		ReyObstruye = false;
		for(j=i; j < 64; j+= 8)
		{
			if(board(j) == rey)
				ReyObstruye = true;
		}
		if((MatCount[negro][torre]+mn+qn) == 0)
			HayPiezas = 0;
		else
			HayPiezas = 1;
		if(isEndGame && true )
		{	
			// final de peones verificamos si el rey esta en el cuadrado
			int fm,cm,cM; // fila minima, columna minima columna maxima
			fm = FILA(i);
			if(ColorJuegan == negro)
				fm--;
			cm = COLUMNA(i) -(7- fm);
			if(cm < 0) cm = 0;
			cM = COLUMNA(i) +(7- fm);
			if(cM >7) cM=7;
			if(FILA(PosKb) < (unsigned int)fm || COLUMNA(PosKb) < (unsigned int)cm || COLUMNA(PosKb) > (unsigned int)cM)
			{
				// el rey esta fuera del cuadrado
				j = FILA(i);
				if(ReyObstruye && j>1)
					EndGame += Parameters[pn_pFPRFCP2-2+j]; 
				else
					EndGame += Parameters[pn_pFPRFCP2-1+j]; 
			}
			else
			{
				if((MatCount[negro][torre]+mn+qn) == 0)
				{
					// apoyado por el rey y en 7a
					if(FILA(i) == 6)
					{
						if(BCasilla(i+8) & ataque[rey][PosKw])
						{
							j = FILA(i);
							EndGame += Parameters[pn_pFPRFCP2-1+j];
						}
					}
					// en 6º y con el rey que apoya el avance hasta coronar
					if(FILA(i) == 5)
					{
						if((PosKw == (i+7) || PosKw == (i+9)) && ataque[rey][PosKw] & BCasilla(i))
						{
							j = FILA(i);
							EndGame += Parameters[pn_pFPRFCP2-1+j];
						}
					}
					// en 6º y con el rey que apoya el avance hasta coronar
					if(FILA(i) == 4)
					{
						int cRey = COLUMNA(PosKw);
						if((cRey > 0 && PosKw == (i+15))
							|| (cRey < 7 && PosKw == (i+17)))
						{
							j = FILA(i);
							EndGame += Parameters[pn_pFPRFCP2-1+j];
						}
					}
				}
			}
		}
		if(isEndGame)
		if((MatCount[negro][torre]+mn+qn) == 0 && ReyObstruye) // rey en la casilla de promocion que no sea de torre
		{
			if(PosKw == (i+8) && COLUMNA(i) != 0 && COLUMNA(i) != 7 && FILA(i) == 6)
			{
				j = FILA(i);
				EndGame += Parameters[pn_pFPRFCP2-2+j];
			}
		}
	}
	p = PPasados[negro];
	j = popCount((p >> 8) & Ocupadas);
	Opening -= j * Parameters[pn_PassedPInMob];
	EndGame -= j * Parameters[pn_PassedPInMobEG];

	while(p)
	{
		HayPasados = 1;
		i = bitScanAndReset(p);

		j = 7-FILA(i);
		if(ConnectedB & (1ull << i)) // is connected
		{
			Opening -= Parameters[_PassedConnected1+j]; 
			EndGame -= Parameters[_PassedConnectedEG1+j];
		}
		else
		{
			Opening -= Parameters[_Passed1+j]; 
			EndGame -= Parameters[_PassedEG1+j];
		}
		if(isEndGame )
		{
			EndGame -= Distancia(PosKw,i-8) * Parameters[pn_DistDefEG];
			EndGame -= Distancia(PosKb,i-8) * Parameters[pn_DistAttEG];
		}

		ReyObstruye = false;
		for(j=i; j >= 0; j-= 8)
		{
			if(board(j) == rey)
				ReyObstruye = true;
		}
		if((MatCount[blanco][torre]+mb+qb) == 0 )
			HayPiezas = 0;
		else
			HayPiezas = 1;
		if(isEndGame && true)
		{
			// final de peones verificamos si el rey esta en el cuadrado
			int fm,cm,cM; // fila minima, columna minima columna maxima
			fm = FILA(i);
			if(ColorJuegan == blanco)
				fm++;
			cm = COLUMNA(i) - fm;
			if(cm < 0) cm = 0;
			cM = COLUMNA(i) + fm;
			if(cM > 7) cM = 7;
			if(FILA(PosKw) > (unsigned int)fm || COLUMNA(PosKw) < (unsigned int)cm || COLUMNA(PosKw) > (unsigned int)cM)
			{
				// el rey esta fuera del cuadrado
				j = 7-FILA(i);
				if(ReyObstruye && j>1)
					EndGame -= Parameters[pn_pFPRFCP2-2+j];
				else
					EndGame -= Parameters[pn_pFPRFCP2-1+j];
			}
			else
			{
				if((MatCount[blanco][torre]+mb+qb) == 0 )
				{
					// apoyado por el rey y en 7a
					if(FILA(i) == 1 )
					{
						if(BCasilla(i-8) & ataque[rey][PosKb])
						{
							j = 7-FILA(i);
							EndGame -= Parameters[pn_pFPRFCP2-1+j];
						}
					}
					// en 6º y con el rey que apoya el avance hasta coronar
					if(FILA(i) == 2)
					{
						if((PosKb == (i-7) || PosKb == (i-9) )&& ataque[rey][PosKb] & BCasilla(i))
						{
							j = 7-FILA(i);
							EndGame -= Parameters[pn_pFPRFCP2-1+j];
						}
					}
					// en 6º y con el rey que apoya el avance hasta coronar
					if(FILA(i) == 3)
					{
						int cRey = COLUMNA(PosKb);
						if((cRey > 0 && PosKb == (i-17))
							|| (cRey < 7 && PosKb == (i-15)))
						{
							j = 7-FILA(i);
							EndGame -= Parameters[pn_pFPRFCP2-1+j]; 
						}
					}
				}
			}
		}
		if(isEndGame)
		if((MatCount[blanco][torre]+mb+qb) == 0 && ReyObstruye) // rey en la casilla de promocion que no sea de torre
		{
			if(PosKb == (i-8) && COLUMNA(i) != 0 && COLUMNA(i) != 7 && FILA(i) == 1)
			{
					j = 7-FILA(i);
					EndGame -= Parameters[pn_pFPRFCP2-2+j]; 
			}
		}
	}
	Backward();
	EvaluaPeonesDoblados();
	//// conectados avanzados
	// aislados en filas 2 3 4
	
	aux = (BPiezas[peon][blanco] & Fila2) & ~ConnectedW;
	i = popCount(aux);
	Opening += i * Parameters[pn_PAislado2]; 
	EndGame += i * Parameters[pn_PAisladoEG2]; 
	aux = (BPiezas[peon][blanco] & Fila3) & ~ConnectedW;
	i = popCount(aux);
	Opening += i * Parameters[pn_PAislado3]; 
	EndGame += i * Parameters[pn_PAisladoEG3]; 
	aux = (BPiezas[peon][blanco] & Fila4) & ~ConnectedW;
	aux &= ~(atacadaP[blanco] << 8);
	i = popCount(aux);
	Opening += i * Parameters[pn_PAislado4]; 
	EndGame += i * Parameters[pn_PAisladoEG4]; 

	aux = (BPiezas[peon][negro] & Fila7) & ~ConnectedB;
	i = popCount(aux);
	Opening -= i * Parameters[pn_PAislado2]; 
	EndGame -= i * Parameters[pn_PAisladoEG2]; 
	aux = (BPiezas[peon][negro] & Fila6) & ~ConnectedB;
	i = popCount(aux);
	Opening -= i * Parameters[pn_PAislado3]; 
	EndGame -= i * Parameters[pn_PAisladoEG3]; 
	aux = (BPiezas[peon][negro] & Fila5) & ~ConnectedB;
	aux &= ~(atacadaP[negro] >> 8);
	i = popCount(aux);
	Opening -= i * Parameters[pn_PAislado4]; 
	EndGame -= i * Parameters[pn_PAisladoEG4]; 

	int frb,frn,fdb,fdn;
	int frrb,frrn,fdrb,fdrn;
	frrb = (BPiezas[rey][blanco] & FlancoRey & ~Sombra[blanco]) != 0ull;
	frrn = (BPiezas[rey][negro] & FlancoRey & ~Sombra[negro]) != 0ull;
	fdrb = (BPiezas[rey][blanco] & FlancoDama & ~Sombra[blanco]) != 0ull;
	fdrn = (BPiezas[rey][negro] & FlancoDama & ~Sombra[negro]) != 0ull;

	aux = (BPiezas[peon][blanco] & (~PPasados[blanco]))& ~Sombra[blanco];
	frb = popCount(aux & FlancoRey);
	fdb = popCount(aux & FlancoDama);
	aux = (BPiezas[peon][negro] & (~PPasados[negro]))& ~Sombra[negro];
	frn = popCount(aux & FlancoRey);
	fdn = popCount(aux & FlancoDama);

	Opening += (frb -frn)*Parameters[pn_Mayoria];
	Opening += (fdb -fdn)*Parameters[pn_Mayoria];
	EndGame += (frb -frn)*Parameters[pn_MayoriaEG];
	EndGame += (fdb -fdn)*Parameters[pn_MayoriaEG];

	ValorPeones[0] += Opening;
	ValorPeones[1] += EndGame;
}
void CDiagrama::CalculaDebiles()
{
  u64 mask, passedPawns; 

  passedPawns = mask = 0ull;

/* Generamos los movimientos de peon y adyacentes */ 
passedPawns = BPiezas[peon][negro];
mask = (passedPawns & 0x7f7f7f7f7f7f7f7full) >> 7 | (passedPawns & 0xfefefefefefefefeull) >> 9;
atacadaP[negro] = mask;

/* duplicamos avanzando los peones para asegurarnos que llegamos al borde del tablero
    */ 
mask |= mask >> 8; 
mask |= mask >> 16; 
mask |= mask >> 32; 
Bdebiles[negro] = ~mask;

passedPawns |= passedPawns >> 8;  
passedPawns |= passedPawns >> 16;  
passedPawns |= passedPawns >> 32;  
mask |= passedPawns;
/* Obtenemos los peones pasados blancos*/ 
passedPawns = ~mask & BPiezas[peon][blanco];
PPasados[blanco] = passedPawns;
/* Generamos los movimientos de peon y adyacentes */ 
passedPawns = BPiezas[peon][blanco];
mask = (passedPawns & 0xfefefefefefefefeull) << 7 | (passedPawns & 0x7f7f7f7f7f7f7f7full)<< 9 ;
atacadaP[blanco] = mask;

/* duplicamos avanzando los peones para asegurarnos que llegamos al borde del tablero
    */ 
mask |= (mask << 8); 
mask |= (mask << 16); 
mask |= (mask << 32); 
Bdebiles[blanco] = ~mask;

passedPawns |= (passedPawns << 8);  
passedPawns |= (passedPawns << 16);  
passedPawns |= (passedPawns << 32);  
mask |= passedPawns;

/* get white's passed pawn mask */ 
passedPawns = (~mask) & BPiezas[peon][negro];
PPasados[negro] = passedPawns;

// calculamos las sombras
mask = BPiezas[peon][blanco];
Sombra[blanco] =  mask >> 8 | mask >> 16 | mask >> 32 | mask >> 24 | mask >> 40 | mask >> 48;
mask = BPiezas[peon][negro];
Sombra[negro] = mask << 8 | mask << 16 | mask << 24 | mask << 32 | mask << 40 | mask << 48;

}



void CDiagrama::Backward()
{
	u64 p;
	int i;
	int Opening,EndGame;
	Opening = EndGame = 0;
	if(MatCount[blanco][peon] > 1 && MatCount[negro][peon] > 1)// && fase < 20)
	{
		u64 aux;
		// peones retrasados
		{
			// un peon no pasado o atacado por otro peon
			aux = BPiezas[peon][blanco] & ~PPasados[blanco];
			aux &= ~atacadaP[negro];
			// no defendido por otro peon
			aux &= ~atacadaP[blanco];
			aux <<=8; // una fila adelante
			// casilla libre atacada por un peon contrario y no defendida por uno propio
			aux &= ~BPiezas[peon][blanco];
			aux &= ~BPiezas[peon][negro];
			aux &= atacadaP[negro];
			aux &= ~atacadaP[blanco];
			aux &= Bdebiles[blanco];
			aux >>=8; // una fila atras
			aux &= FILAS234;
			while(aux )
			{
				i = bitScanAndReset(aux);
				Opening -= Parameters[pn_pVALOR_RETRASADO]; 
				EndGame -= Parameters[pn_pVALOR_RETRASADOEG]; 
			}
		}
		// ahora peones retrasados del negro
		{
			p = Bdebiles[negro];
			aux = BPiezas[peon][negro] & ~PPasados[negro];
			aux &= ~atacadaP[blanco];
			// no defendido por otro peon
			aux &= ~atacadaP[negro];
			aux >>=8; // una fila adelante
			aux &= ~BPiezas[peon][blanco];
			aux &= ~BPiezas[peon][negro];
			aux &= atacadaP[blanco];
			aux &= ~atacadaP[negro];
			aux &= Bdebiles[negro];
			aux <<=8; // una fila atras
			aux &= FILAS765;
			while(aux)
			{
				i = bitScanAndReset(aux);
				Opening += Parameters[pn_pVALOR_RETRASADO]; 
				EndGame += Parameters[pn_pVALOR_RETRASADOEG]; 
			}
		}
	}
	ValorPeones[0] += Opening;
	ValorPeones[1] += EndGame;
}

void CDiagrama::EvaluaPeonesDoblados()
{
	int hayDob[2];
	u64 aux;
	int Opening,EndGame;
	Opening = EndGame = 0;
	hayDob[blanco] = hayDob[negro] = 0;
	aux = (BPiezas[peon][blanco] & Sombra[blanco]);
	hayDob[blanco] = popCount(aux);
	aux = (BPiezas[peon][negro] & Sombra[negro]);
	hayDob[negro] = popCount(aux);

	Opening += (hayDob[blanco]-hayDob[negro]) * Parameters[pn_pVALOR_DOBLADO];
	EndGame += (hayDob[blanco]-hayDob[negro]) * Parameters[pn_pVALOR_DOBLADOEG];

	if(hayDob[blanco] == 0 && bb == 2)
	{
		Opening += Parameters[pn_BishopPair1];
		EndGame += Parameters[pn_BishopPair1EG];
	}
	if(hayDob[blanco] == 0 && bb != 2)
	{
		Opening -= Parameters[pn_DoubledBP];
		EndGame -= Parameters[pn_DoubledBPEG];
	}
	
	if(hayDob[negro] == 0 && bn == 2)
	{
		Opening -= Parameters[pn_BishopPair1];
		EndGame -= Parameters[pn_BishopPair1EG];
	}
	if(hayDob[negro] == 0 && bn != 2)
	{
		Opening += Parameters[pn_DoubledBP];
		EndGame += Parameters[pn_DoubledBPEG];
	}
	// promediar fase.
	ValorPeones[0] += Opening;
	ValorPeones[1] += EndGame;
}

void CDiagrama::EvaluaAlfiles()
{
static const int factorMobBishop = 1;
static const int factorMobBishopEG = 1;
	int Opening,EndGame;
	Opening = EndGame = 0;
	EvalAlfiles[0] = EvalAlfiles[1] = 0;
	if((bb + bn) > 0)
	{
		int c,square,sign;
		u64 piezas,ocupa;
		u64 map;
		ocupa = Ocupadas;
		for(c=blanco,sign = 1;c <= negro;c++,sign*=-1)
		{
			piezas = BPiezas[alfil][c];
			map = 0ull;
			while(piezas)
			{
				square = bitScanAndReset(piezas);
				// en casilla tenemos lo buscado
				map = (diagonalAttacks(ocupa,square)) | (antiDiagAttacks(ocupa,square));
				if(c == blanco)
				{
					if(map & ataque[rey][PosKb])
					{
						SafetyCounters[negro] += Parameters[pn_ATTBISHOP];
						SafetyPiece[negro]++;
					}
				}
				else
				{
					if(map & ataque[rey][PosKw])
					{
						SafetyCounters[blanco] += Parameters[pn_ATTBISHOP];
						SafetyPiece[blanco]++;
					}
				}

				square = popCount(map  & (~atacadaP[c^0x1] & ~BOcupadas[c]));
				Opening += sign * factorMobBishop * Parameters[pn_MOBBISHOP0+square];
				EndGame += sign * factorMobBishopEG * Parameters[pn_MOBBISHOPEG0+square]; 
			}
		}
		if(ct == BLOCKED)
		{
			int ColorPawn,sq;
			// obtenemos el color de los peones centrales propios. (10%-)
			ColorPawn = 0;
			for(sq = E2; sq < E8; sq +=16)
			{
				if( board(sq) == peon && board(sq+8) == peon)
				{
					ColorPawn = COLORCASILLA(sq);
					break;
				}
			}
			if(ColorPawn == blanco)
			{
				if(BPiezas[alfil][blanco] & CasillasBlancas)
				{
					Opening += Parameters[_BadBishop];
					EndGame += Parameters[_BadBishopEG];
				}
				if(BPiezas[alfil][negro] & CasillasNegras)
				{
					Opening -= Parameters[_BadBishop];
					EndGame -= Parameters[_BadBishopEG];
				}
			}
			else
			{
				if(BPiezas[alfil][blanco] & CasillasNegras)
				{
					Opening += Parameters[_BadBishop];
					EndGame += Parameters[_BadBishopEG];
				}
				if(BPiezas[alfil][negro] & CasillasBlancas)
				{
					Opening -= Parameters[_BadBishop];
					EndGame -= Parameters[_BadBishopEG];
				}
			}
		}
	}
	EvalAlfiles[0] = Opening;
	EvalAlfiles[1] = EndGame;
}

void CDiagrama::EvaluaDamas()
{
static const int factorMobQueen = 1; 
static const int factorMobQueenEG = 1;

	EvalDamas[0] = EvalDamas[1] = 0;
	if((qb+qn) == 0)
		return;
	int Opening,EndGame;
	Opening = EndGame = 0;
	u64 piezas;
	int casilla;
	const u64 TargetsWhite = 0x00ff810000000000ull;
	const u64 TargetsBlack = 0x0081ff00ull;
	u64 Peones = BPiezas[peon][blanco] | BPiezas[peon][negro];
	Peones &= 0x0000ffffffff0000ull;
	piezas = BPiezas[dama][blanco];
	while(piezas)
	{
		casilla = bitScanAndReset(piezas);
		// movilidad
		u64 map = rankAttacks(Ocupadas ,casilla) |(fileAttacks(Ocupadas ,casilla))|(diagonalAttacks(Ocupadas,casilla)) | (antiDiagAttacks(Ocupadas,casilla));
		if(map & ataque[rey][PosKb])
		{
			SafetyCounters[negro] += Parameters[pn_ATTQUEEN];
			SafetyPiece[negro]++;
		}

		int square = popCount(map  & (~atacadaP[negro] & ~BOcupadas[blanco]));
		Opening += factorMobQueen * Parameters[pn_MOBQUEEN0+square]; 
		EndGame += factorMobQueenEG * Parameters[pn_MOBQUEENEG0+square]; 
	}
	piezas = BPiezas[dama][negro];
	while(piezas)
	{
		casilla = bitScanAndReset(piezas);
		// movilidad
		u64 map = rankAttacks(Ocupadas ,casilla) |(fileAttacks(Ocupadas ,casilla))|(diagonalAttacks(Ocupadas,casilla)) | (antiDiagAttacks(Ocupadas,casilla));
		if(map & ataque[rey][PosKw])
		{
			SafetyCounters[blanco] += Parameters[pn_ATTQUEEN];
			SafetyPiece[blanco]++;
		}

		int square = popCount(map & (~atacadaP[blanco] & ~BOcupadas[negro]));
		Opening -= factorMobQueen * Parameters[pn_MOBQUEEN0+square]; 
		EndGame -= factorMobQueenEG * Parameters[pn_MOBQUEENEG0+square]; 
	}
	if((BPiezas[peon][negro] & Fila7)||(BPiezas[rey][negro] & Fila8))
	{
		if(BPiezas[dama][blanco] & Fila7)
		{
			Opening += Parameters[pn_pDamaSeptima];
			EndGame += Parameters[pn_pDamaSeptimaEG];
		}
	}

	if((BPiezas[peon][blanco] & Fila2)||(BPiezas[rey][blanco] & Fila1))
	{
		if(BPiezas[dama][negro] & Fila2)
		{
			Opening -= Parameters[pn_pDamaSeptima];
			EndGame -= Parameters[pn_pDamaSeptimaEG];
		}
	}
	EvalDamas[0] = Opening;
	EvalDamas[1] = EndGame;
}
void CDiagrama::EvaluaTorres()
{
static const int factorMobRook = 1; 
static const int factorMobRookEG = 1;
static const int TorresComunicadas = 4;
static const int TorresComunicadasEG = 0;
static const int TorresComunicadas1 = 4;
static const int TorresComunicadas1EG = 0;
static const int TorresComunicadas2 = 1;
static const int TorresComunicadas2EG = 0;
static const int TorresComunicadas3 = 3;
static const int TorresComunicadas3EG = 0;

	EvalTorres[0] = EvalTorres[1] = 0;
	if((MatCount[blanco][torre]+MatCount[negro][torre]) == 0)
		return;
	int Opening,EndGame;
	int Comunicada;
	Opening = EndGame = 0;
	{
		{
			u64 piezas = 0x0ull;
			int casilla = 0;
			int crey = COLUMNA(PosReyes[negro]);
			int c;
			piezas = BPiezas[torre][blanco];
			u64 peonesC = (BPiezas[peon][blanco] | BPiezas[peon][negro]);
			// buscamos la casilla donde se ubica la torre
			while(piezas)
			{
				casilla = bitScanAndReset(piezas);
				// movilidad
				u64 map = rankAttacks(Ocupadas ,casilla) |(fileAttacks(Ocupadas ,casilla));
				if(map & ataque[rey][PosKb])
				{
					SafetyCounters[negro] += Parameters[pn_ATTROOK];
					SafetyPiece[negro]++;
				}
				Comunicada = 0;
				if(map & BPiezas[torre][blanco])
				{
					Opening += TorresComunicadas;
					EndGame += TorresComunicadasEG;
					Comunicada = 1;
				}


				int square = popCount(map & (~atacadaP[negro] & ~BOcupadas[blanco]));
				Opening += factorMobRook * Parameters[pn_MOBROOK0+square]; 
				EndGame += factorMobRookEG * Parameters[pn_MOBROOKEG0+square];
				// torre en columna abierta
				c = COLUMNA(casilla);
				if(((ColumnaA << c) & peonesC)== 0ull)
				{
					Opening += Parameters[pn_TAbierta] ; 
					EndGame += Parameters[pn_TAbiertaEG] ; 
					if(abs(c-crey) < 2)
					{
						Opening += Parameters[pn_TCercaR]+ Comunicada * TorresComunicadas1; 
						EndGame += Parameters[pn_TCercaREG]+ Comunicada * TorresComunicadas1EG; 
					}
				}
				else
				{
					if(((ColumnaA << c) & ~Sombra[blanco]) & BCasilla(casilla))
					{
						Opening += Parameters[pn_TorreSemiAbierta]+ Comunicada * TorresComunicadas2; 
						EndGame += Parameters[pn_TorreSemiAbiertaEG]+ Comunicada * TorresComunicadas2EG; 
						if(abs(c-crey) < 2)
						{
							Opening += Parameters[pn_TSCercaR]+ Comunicada * TorresComunicadas3; 
							EndGame += Parameters[pn_TSCercaREG]+ Comunicada * TorresComunicadas3EG; 
						}
					}
					else
					{
						// no es una columna abierta o semi ver si tenemos la torre atrapada por el rey
						if(casilla == H1)
						{
							// torre atrapada
							if(board(G1) == rey || board(F1) == rey)
							{
								Opening -= Parameters[pn_RookTrapped];
								EndGame -= Parameters[pn_RookTrappedEG];
							}
						}
						if(casilla == A1)
						{
							// torre atrapada
							if(board(C1) == rey || board(B1) == rey)
							{
								Opening -= Parameters[pn_RookTrapped];
								EndGame -= Parameters[pn_RookTrappedEG];
							}
						}
					}
				}
			}
			piezas = BPiezas[torre][negro];
			crey = COLUMNA(PosReyes[blanco]);
			// buscamos la casilla donde se ubica la torre
			while(piezas)
			{
				casilla = bitScanAndReset(piezas);
				// movilidad
				u64 map = rankAttacks(Ocupadas ,casilla) |(fileAttacks(Ocupadas ,casilla));
				if(map & ataque[rey][PosKw])
				{
					SafetyCounters[blanco] += Parameters[pn_ATTROOK];
					SafetyPiece[blanco]++;
				}
				Comunicada = 0;
				if(map & BPiezas[torre][negro])
				{
					Opening -= TorresComunicadas;
					EndGame -= TorresComunicadasEG;
					Comunicada = 1;
				}

				int square = popCount(map & (~atacadaP[blanco] & ~BOcupadas[negro]));
				Opening -= factorMobRook * Parameters[pn_MOBROOK0+square];
				EndGame -= factorMobRookEG * Parameters[pn_MOBROOKEG0+square];
				c = COLUMNA(casilla);
				if(((ColumnaA << c) & peonesC) == 0ull)
				{
					Opening -= Parameters[pn_TAbierta] ; 
					EndGame -= Parameters[pn_TAbiertaEG] ; 
					if(abs(c-crey) < 2)
					{
						Opening -= Parameters[pn_TCercaR]+ Comunicada * TorresComunicadas1; 
						EndGame -= Parameters[pn_TCercaREG]+ Comunicada * TorresComunicadas1EG; 
					}
				}
				else
				{
					if(((ColumnaA << c) & ~Sombra[negro]) & BCasilla(casilla))
					{
						Opening -= Parameters[pn_TorreSemiAbierta]+ Comunicada * TorresComunicadas2; 
						EndGame -= Parameters[pn_TorreSemiAbiertaEG]+ Comunicada * TorresComunicadas2EG; 
						if(abs(c-crey) < 2)
						{
							Opening -= Parameters[pn_TSCercaR]+ Comunicada * TorresComunicadas3; 
							EndGame -= Parameters[pn_TSCercaREG]+ Comunicada * TorresComunicadas3EG; 
						}
					}
					else
					{
						// no es una columna abierta o semi ver si tenemos la torre atrapada por el rey
						if(casilla == H8)
						{
							// torre atrapada
							if(board(G8) == rey || board(F8) == rey)
							{
								Opening += Parameters[pn_RookTrapped];
								EndGame += Parameters[pn_RookTrappedEG];
							}
						}
						if(casilla == A8)
						{
							// torre atrapada
							if(board(C8) == rey || board(B8) == rey)
							{
								Opening += Parameters[pn_RookTrapped];
								EndGame += Parameters[pn_RookTrappedEG];
							}
						}
					}
				}
			}
		}
	}
	// Torres en 7ª
	if((BPiezas[peon][negro] & Fila7)||(BPiezas[rey][negro] & Fila8))
	{
		if(BPiezas[torre][blanco] & Fila7)
		{
			Opening += Parameters[pn_TorreSeptima]; 
			EndGame += Parameters[pn_TorreSeptimaEG]; 
		}
	}

	if((BPiezas[peon][blanco] & Fila2)||(BPiezas[rey][blanco] & Fila1))
	{
		if(BPiezas[torre][negro] & Fila2)
		{
			Opening -= Parameters[pn_TorreSeptima]; 
			EndGame -= Parameters[pn_TorreSeptimaEG]; 
		}
	}
	EvalTorres[0] = Opening;
	EvalTorres[1] = EndGame;
}


void CDiagrama::PawnCoverage()
{
	int Valor = 0;
	PawnCover[0] = PawnCover[1] = 0;
	if(isEndGame) return;

	// value from 0 to 8
	// 1 2 1   |2 2
	// 2 4 2   |4 4
	//   K     |K

const int MinCov = 10;
static const int Cov4 = 4;
static const int Cov2 = 2;
static const int Cov1 = 1;

	u64 Raux;
	int casillaRey;

	Raux = BPiezas[rey][blanco];
	casillaRey = bitScanAndReset(Raux);
	// rey en columna abierta
	if((MatCount[negro][torre]+qn) > 0)
	{
		if((Columnas[COLUMNA(casillaRey)] & (BPiezas[peon][blanco] | BPiezas[peon][negro])) == 0ull)
		{
			PawnCover[0] -= Parameters[pn_KOOpp];
			PawnCover[1] -= Parameters[pn_KOEG];
		}
	}
	if(FILA(casillaRey) < 3)
	{
	if(COLUMNA(casillaRey) == 0)
	{
		// A Column;
		Raux = BCasilla(casillaRey+8) | BCasilla(casillaRey+9);
		Valor += popCount(Raux & BPiezas[peon][blanco]) * Cov4;
		Raux = BCasilla(casillaRey+16) | BCasilla(casillaRey+17);
		Valor += popCount(Raux & BPiezas[peon][blanco]) * Cov2;
	}
	else
		if(COLUMNA(casillaRey) == 7)
		{
			// Columna H
			Raux = BCasilla(casillaRey+8) | BCasilla(casillaRey+7);
			Valor += popCount(Raux & BPiezas[peon][blanco]) * Cov4;
			Raux = BCasilla(casillaRey+16) | BCasilla(casillaRey+15);
			Valor += popCount(Raux & BPiezas[peon][blanco]) * Cov2;
		}
		else
		{ 
			// General case
			Raux = BCasilla(casillaRey+8) ;
			Valor += popCount(Raux & BPiezas[peon][blanco]) * Cov4;
			Raux = BCasilla(casillaRey+7) | BCasilla(casillaRey+9) | BCasilla(casillaRey+16);
			Valor += popCount(Raux & BPiezas[peon][blanco]) * Cov2;
			Raux = BCasilla(casillaRey+17) | BCasilla(casillaRey+15);
			Valor += popCount(Raux & BPiezas[peon][blanco]) * Cov1;
		}
	}
	Valor -= MinCov; // 0->8 => -8 -> 0
	if(Valor > 0) Valor = 0;
	PawnCover[0] += Valor * Parameters[pn_pPCovOpp];
	PawnCover[1] += Valor * Parameters[pn_pPCovEG];

	Valor = 0;
	Raux = BPiezas[rey][negro];
	casillaRey = bitScanAndReset(Raux);
	if((MatCount[blanco][torre]+qb) > 0)
	{
		if((Columnas[COLUMNA(casillaRey)] & (BPiezas[peon][negro] | BPiezas[peon][blanco])) == 0ull)
		{
			PawnCover[0] += Parameters[pn_KOOpp];
			PawnCover[1] += Parameters[pn_KOEG];
		}
	}

	if(FILA(casillaRey) > 4)
	{
		if(COLUMNA(casillaRey) == 0)
		{
			// A Column;
			Raux = BCasilla(casillaRey-8) | BCasilla(casillaRey-7);
			Valor += popCount(Raux & BPiezas[peon][negro]) * Cov4;
			Raux = BCasilla(casillaRey-16) | BCasilla(casillaRey-15);
			Valor += popCount(Raux & BPiezas[peon][negro]) * Cov2;
		}
		else
			if(COLUMNA(casillaRey) == 7)
			{
				// Columna H
				Raux = BCasilla(casillaRey-8) | BCasilla(casillaRey-9);
				Valor += popCount(Raux & BPiezas[peon][negro]) * Cov4;
				Raux = BCasilla(casillaRey-16) | BCasilla(casillaRey-17);
				Valor += popCount(Raux & BPiezas[peon][negro]) * Cov2;
			}
			else
			{ 
				// General case
				Raux = BCasilla(casillaRey-8) ;
				Valor += popCount(Raux & BPiezas[peon][negro]) * Cov4;
				Raux = BCasilla(casillaRey-7) | BCasilla(casillaRey-9) | BCasilla(casillaRey-16);
				Valor += popCount(Raux & BPiezas[peon][negro]) * Cov2;
				Raux = BCasilla(casillaRey-17) | BCasilla(casillaRey-15);
				Valor += popCount(Raux & BPiezas[peon][negro]) * Cov1;
			}
	}
	Valor -= MinCov; // 0->8 => -8 -> 0
	if(Valor > 0) Valor = 0;

	PawnCover[0] -= Valor * Parameters[pn_pPCovOpp];
	PawnCover[1] -= Valor * Parameters[pn_pPCovEG];
}

int	CDiagrama::DivisorEndGame()
{
	if(fase16 < 14) return 16;
	// si final de alfiles de distinto color
	if(bb==1 && bn == 1)
	{
		if(BPiezas[alfil][blanco] & CasillasBlancas && BPiezas[alfil][negro] & CasillasNegras)
			return 17+fase16;
		if(BPiezas[alfil][negro] & CasillasBlancas && BPiezas[alfil][blanco] & CasillasNegras)
			return 17+fase16;
	}
	return 16;
}

// 
// From the book: pawn endgame by J.Maizelis
// chapter 1.
//

int CDiagrama::Kpk()
{
	const int FINALPEONESGANADO = 600;
	int Valor  = 100; //abs(Material);
	int bandoFuerte,bandoDebil;
	if(MatCount[blanco][peon])
	{
		bandoFuerte = blanco;
	}
	else
	{
		bandoFuerte = negro;
	}
	bandoDebil = bandoFuerte ^1;
	int CasillaPeon = 0;
	// primero miramos si el peon puede coronar sin el rey
	u64 p;
	p = BPiezas[peon][bandoFuerte];
	int fm,cm,cM; // fila minima, columna minima columna maxima
	int Color2Play = ColorJuegan;
	int PosReyDebil = PosReyes[bandoDebil];

	// si el bando debil juega y captura el peon
	if(Color2Play == bandoDebil)
	{
		if(ataque[rey][PosReyDebil] & p) // rey debil ataca el peon
		{
			if(!(ataque[rey][PosReyes[bandoFuerte]] & p)) // rey fuerte no defiende
			{
				return 0;
			}
		}
	}

	fm = 0;
	while(p)
	{
		CasillaPeon = bitScanAndReset(p);
		// final de peones verificamos si el rey está en el cuadrado
		fm = FILA(CasillaPeon);
		if(bandoFuerte == blanco)
		{
			if(Color2Play == bandoDebil)
				fm--; // el cuadrado tiene una casilla mas
			cm = COLUMNA(CasillaPeon) -(7- fm);
			if(cm < 0) cm = 0;
			cM = COLUMNA(CasillaPeon) +(7- fm);
			if(cM > 7) cM = 7;
			if(FILA(PosReyDebil) < (unsigned int)fm || COLUMNA(PosReyDebil) < (unsigned int)cm || COLUMNA(PosReyDebil) > (unsigned int)cM)
			{
				// el rey esta fuera del cuadrado
				Valor += FINALPEONESGANADO;
			}
			if(Color2Play == bandoDebil)
				fm++; // restauramos el cuadrado tiene una casilla mas
		}
		else
		{
			if(Color2Play == bandoDebil)
				fm++; // el cuadrado tiene una casilla mas
			cm = COLUMNA(CasillaPeon) -(fm);
			if(cm < 0) cm = 0;
			cM = COLUMNA(CasillaPeon) +(fm);
			if(cM > 7) cM = 7;
			if(FILA(PosReyDebil) > (unsigned int)fm || COLUMNA(PosReyDebil) < (unsigned int)cm || COLUMNA(PosReyDebil) > (unsigned int)cM)
			{
				// el rey esta fuera del cuadrado
				Valor += FINALPEONESGANADO;
			}
			if(Color2Play == bandoDebil)
				fm--; // el cuadrado tiene una casilla mas
		}
	}
	// Calculamos las casilla críticas del bando fuerte y del bando debil
	// Bando debil la casilla crítica es la que está delante del peón.
	if(bandoDebil == blanco)
	{
		if(PosReyes[blanco] == (CasillaPeon-8) && fm > 1)
		{
			return 0;
		}
	}
	else
	{
		if(PosReyes[negro] == (CasillaPeon+8) && fm < 6)
		{
			return 0;
		}
	}
	// Bando Fuerte
	int cp;
	cp = COLUMNA(CasillaPeon);
	int PosWhiteKing = PosReyes[bandoFuerte];
	if(bandoFuerte == blanco)
	{
		if(cp == 0 || cp == 7)
		{
			// solo hay una crítica b7 y g7
			if(cp == 0 && PosWhiteKing == B7)
					Valor += FINALPEONESGANADO;
			if(cp == 7 && PosWhiteKing == G7)
					Valor += FINALPEONESGANADO;
		}
		else
		{
			if(PosWhiteKing == CasillaPeon+15
				||PosWhiteKing == CasillaPeon+16
				||PosWhiteKing == CasillaPeon+17
				)
					Valor += FINALPEONESGANADO;
			if(FILA(CasillaPeon) > 3)
			{
				// bonnus de casillas críticas
				if(PosWhiteKing == CasillaPeon+7
					||PosWhiteKing== CasillaPeon+8
					||PosWhiteKing == CasillaPeon+9
					)
						Valor += FINALPEONESGANADO;
			}
			if(FILA(CasillaPeon) == 6)
			{
				if(PosWhiteKing == CasillaPeon+1
					||PosWhiteKing == CasillaPeon-1)
					Valor += FINALPEONESGANADO;

			}
		}
	}
	else
	{
		int PosKingBlack = PosReyes[negro];
		if(cp == 0 || cp == 7)
		{
			// solo hay una crítica b7 y g7
			if(cp == 0 && PosKingBlack == B2)
					Valor += FINALPEONESGANADO;
			if(cp == 7 && PosKingBlack == G2)
					Valor += FINALPEONESGANADO;
		}
		else
		{
			if(PosKingBlack == CasillaPeon-15
				||PosKingBlack == CasillaPeon-16
				||PosKingBlack == CasillaPeon-17
				)
					Valor += FINALPEONESGANADO;
			if(FILA(CasillaPeon) < 4) 
			{
				// bonnus de casillas críticas
				if(PosKingBlack == CasillaPeon-7
					||PosKingBlack == CasillaPeon-8
					||PosKingBlack == CasillaPeon-9
					)
						Valor += FINALPEONESGANADO;
				if(FILA(CasillaPeon) == 1)
				{
					if(PosKingBlack == CasillaPeon+1
						||PosKingBlack == CasillaPeon-1)
						Valor += FINALPEONESGANADO;
				}
			}
		}
	}
	// Valoramos la progresión hacia la coronación.
	if(Valor >= FINALPEONESGANADO)
	{
		if(bandoFuerte == blanco)
		{
			Valor += 20 * fm;
		}
		else
			Valor += 20 *(7-fm);
	}
	if(Color2Play == bandoFuerte)
		return Valor;	
	else
		return -Valor;
}


void CDiagrama::EvalKnight()
{
	EvalCaballos[0] = EvalCaballos[1] = 0;
	if((kb+kn) == 0) return; // si no hay caballos no perdamos tiempo.

	int Value = 0;
	// casillas alcanzables por el caballo no ocupadas por piezas propias y no atacadas por peones contrarios
	u64 aux;
	u64 piezas;
	int casilla;
	piezas = BPiezas[caballo][blanco];
	
	int lOpening,lEndGame;
	lOpening = lEndGame = 0;
	// buscamos la casilla donde se ubica el caballo
	while(piezas)
	{
		casilla = bitScanAndReset(piezas);
		aux = ataque[caballo][casilla] & (~this->atacadaP[negro] & ~this->BOcupadas[blanco]);
		lOpening += Parameters[pn_MOBKNIGHT0+popCount(aux)]; 
		lEndGame += Parameters[pn_MOBKNIGHTEG0+popCount(aux)]; 
		if(ataque[caballo][casilla] & ataque[rey][PosKb])
		{
			SafetyCounters[negro] += Parameters[pn_ATTKNIGHT];
			SafetyPiece[negro]++;
		}
	}
	piezas = BPiezas[caballo][negro];
	// buscamos la casilla donde se ubica el caballo
	while(piezas)
	{
		casilla = bitScanAndReset(piezas);
		aux = ataque[caballo][casilla] & (~this->atacadaP[blanco] & ~this->BOcupadas[negro]);
		lOpening -= Parameters[pn_MOBKNIGHT0+popCount(aux)]; 
		lEndGame -= Parameters[pn_MOBKNIGHTEG0+popCount(aux)]; 
		if(ataque[caballo][casilla] & ataque[rey][PosKw])
		{
			SafetyCounters[blanco] += Parameters[pn_ATTKNIGHT];
			SafetyPiece[blanco]++;
		}
	}
	EvalCaballos[0] += lOpening;
	EvalCaballos[1] += lEndGame;

		// caballo apoyado por un peon.
	piezas = BPiezas[caballo][blanco] & (atacadaP[blanco] & (~atacadaP[negro]));
	if(piezas)
	{
		Value = popCount(piezas);
		EvalCaballos[0] +=  Value * Parameters[pn_KnightPawn];
		EvalCaballos[1] += Value * Parameters[pn_KnightPawnEG];
	}
	piezas = BPiezas[caballo][negro] & (atacadaP[negro] & (~atacadaP[blanco]));
	if(piezas)
	{
		Value = popCount(piezas);
		EvalCaballos[0] -=  Value * Parameters[pn_KnightPawn];
		EvalCaballos[1] -= Value * Parameters[pn_KnightPawnEG];
	}

	int c;
	piezas = 0x00007e7e00000000ull & BPiezas[caballo][blanco];
	while(piezas)
	{
		casilla = bitScanAndReset(piezas);
		c = COLUMNA(casilla);
		if(((ColumnaA << c) & ~Sombra[blanco]) & BCasilla(casilla))
		{
			Opening += Parameters[pn_OutPost]; 
			EndGame += Parameters[pn_OutPostEG]; 
		}
	}
	piezas = 0x000000007e7e0000ull & BPiezas[caballo][negro];
	while(piezas)
	{
		casilla = bitScanAndReset(piezas);
		c = COLUMNA(casilla);
		if(((ColumnaA << c) & ~Sombra[negro]) & BCasilla(casilla))
		{
			Opening -= Parameters[pn_OutPost]; 
			EndGame -= Parameters[pn_OutPostEG]; 
		}
	}
}


int	CDiagrama::KingSafety()
{
	int value = 0;

	PawnCoverage();
	value += PawnCover[0];

	// factor (n-1)/n
	// 1 piece -> 0
	// 2 piece -> 1/2
	// 3 piece -> 2/3
	// 4 piece -> 3/4
	if(atacadaP[blanco] & ataque[rey][PosKb])
	{
		SafetyCounters[negro] += Parameters[pn_ATTPAWN];
		SafetyPiece[negro]++;
	}
	if(atacadaP[negro] & ataque[rey][PosKw])
	{
		SafetyCounters[blanco] += Parameters[pn_ATTPAWN];
		SafetyPiece[blanco]++;
	}
	if(ataque[rey][PosKb] & ataque[rey][PosKw])
	{
		SafetyCounters[blanco] += Parameters[pn_ATTPAWN];
		SafetyCounters[negro] += Parameters[pn_ATTPAWN];
		SafetyPiece[blanco]++;
		SafetyPiece[negro]++;
	}
	static const int factor[8] = {0,0,64,85,96,102,106,128};
	if(SafetyPiece[blanco] < 8)
	{
		value -= (SafetyCounters[blanco] * (factor[SafetyPiece[blanco]])) / 128;
	}
	if(SafetyPiece[negro] < 8 )
	{
		value += (SafetyCounters[negro] * (factor[SafetyPiece[negro]])) / 128;
	}
	return value;
}
void CDiagrama::Development()
{
	int bd,nd;
	bd = nd = -10;
	DevelopmentEval = 0;
	if(isEndGame) return;
	// 1 point side on move
	if(ColorJuegan == blanco) bd++;
	else nd++;

	//
	// a) 1 point for each piece  that is in a good developped position 
	//		(including those that have not moved yet).
	//		a rook is considered developped if the pawn of its own side in front of it 
	//		is advanced al least to the 5th rank or gone altogether.
	
	// cada pieza que no esta en el tablero esta desarrollada
	bd += 7 - (MatCount[blanco][torre]+MatCount[blanco][caballo]+MatCount[blanco][alfil]+ MatCount[blanco][dama]);
	nd += 7 - (MatCount[negro][torre]+MatCount[negro][caballo]+MatCount[negro][alfil]+ MatCount[negro][dama]);
	//
	// tomamos el pst de la pieza calculamos la media si en su pos está por encima de la media es ok
	// AveragePst
	//bd += PieceDevelopped(Board,caballo,blanco);
	//bd += PieceDevelopped(Board,alfil,blanco);
	//bd += PieceDevelopped(Board,torre,blanco);
	//bd += PieceDevelopped(Board,dama,blanco);

	//nd += PieceDevelopped(Board,caballo,negro);
	//nd += PieceDevelopped(Board,alfil,negro);
	//nd += PieceDevelopped(Board,torre,negro);
	//nd += PieceDevelopped(Board,dama,negro);
	// minor out of 1º rank
	bd += 4;
	if(board(B1) == caballo)
		bd--;
	if(board(G1) == caballo)
		bd--;
	if(board(C1) == alfil)
		bd--;
	if(board(F1) == alfil)
		bd--;
	nd += 4;
	if(board(B8) == caballo)
		nd--;
	if(board(G8) == caballo)
		nd--;
	if(board(C8) == alfil)
		nd--;
	if(board(F8) == alfil)
		nd--;

	// b) 1 point for having the king in a safe position.
	//		castling get 1 point.
	//
	if(board(G1) == rey || board(H1) == rey)
	{
		if(board(G2) == peon && (board(H2) == peon || board(H3) == peon))
			bd++;
	}
	else
	{
		if(board(A1) == rey || board(B1) == rey)
		{
			if(board(B2) == peon && (board(A2) == peon || board(A3) == peon))
				bd++;
		}
	}
	if(board(G8) == rey || board(H8) == rey)
	{
		if(board(G7) == peon && (board(H7) == peon || board(H6) == peon))
			nd++;
	}
	else
	{
		if(board(A8) == rey || board(B8) == rey)
		{
			if(board(B7) == peon && (board(A7) == peon || board(A6) == peon))
				nd++;
		}
	}

	//
	// c) 1 for each of the two centre pawns advanced beyond their original square
	//
	if(board(E2) != peon)
		bd++;
	if(board(D2) != peon)
		bd++;
	if(board(E7) != peon)
		nd++;
	if(board(D7) != peon)
		nd++;
	// ATG penalty for a a blocked pawn on original square
	if(board(E2) == peon && board(E3) != ninguna)
		bd--;
	if(board(D2) == peon && board(D3) != ninguna)
		bd--;
	if(board(E7) == peon && board(E6) != ninguna)
		nd--;
	if(board(D7) == peon && board(D6) != ninguna)
		nd--;

	// d) 1 extra point for a knight in the centre, as long as it cannot be driven
	//		away, and the player has already developed over half his pieces.


	// calibramos

	if(bd > 0) bd = 0;
	if(nd > 0) nd = 0;

	DevelopmentEval= 0;

	DevelopmentEval = Parameters[pn_DevelopFactor] * (bd-nd);
}

