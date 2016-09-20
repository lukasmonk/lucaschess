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
#include <stdlib.h>

#include "Ajedrez.h"
#include "Sort.h"
#include "BEval.h"

//#include "kpk.h"

extern u64 ataque[8][64]; 
const u64 ColumnasDE = 0x1818181818181818ull;
const u64 ColumnasCD = 0xc0c0c0c0c0c0c0c0ull;
const u64 ColumnasEF = 0x0303030303030303ull;

static const u64 Columnas[8] = {
	ColumnaA,ColumnaB,ColumnaC,ColumnaD,ColumnaE,ColumnaF,ColumnaG,ColumnaH
};

const u64 FILAS234 = 0xffffff00ull;
const u64 FILAS765 = 0x00ffffff00000000ull;

const int _pPFase = 35; 
const int _pFase = 20;
const int ParAlfiles = 5; 
const int _BishopPair1  = 5;
const int _DoubledBP = 7; 
const int _pFinalPeonesReyFueraCuadrado = 150;
const int _pVALOR_RETRASADO = 14; 
const int _pVALOR_DOBLADO = 3; 
const int PPpasados[8] = {0,4,4,43,82,121,160,0}; 

const int OcupadaC8a = 100;
const int PConectado = 0; 
const int PAislado = 20; 

const int _pValorAvanzadilla = 5; 

const int _KnightAttack = 0;

const int BishopTrapped = 100; 

const int _AlfilMalo = 22; 
const int _AlfilAtaque = 5;
const int _pDamaSeptima = 7; 
const int _damaMob = 3;
const int _damaAtaque = 2;
const int _damaMala = 5;

const int _TAbierta = 22;
const int _TorreSeptima = 5; 
const int _TorreSemiAbierta = 8; 
const int RookTrapped = 50; 

const int _pValorCobertura[16] = {3,2,2,2,2,2,1,1,1,1,1,0,0,0,0,0,}; 

const int _PC0 = 0;
const int _PC1 = 3; // Pe5
const int _PC2 = -1; // Pe4
const int _PC3 = 11; // Pd5
const int _PC4 = 0;  // Pe5d5
const int _PC6 = 1; // Pd4
const int _PC8 = 5; // Pd4e4
const int _PC25 = 2;  // Pd4e5d5 
const int _PC12 = 0;  // PIn
const int _PC5 = 3;
const int _PC7 = 4;
const int _PC9 = 3;
const int _PC10 = 4;
const int _PC11 = 2;
const int _PC13 = -3;
const int _PC14 = 0;const int _PC15 = -5;
const int _PC16 = 8;const int _PC17 = 0;const int _PC18 = 2;const int _PC19 = 5;
const int _PC20 = 1;const int _PC21 = -6;const int _PC22 = -3;const int _PC23 = 6;
const int _PC24 = -1;const int _PC26 = 1;const int _PC27 = 0;
const int _PC28 = -4;const int _PC29 = -3;const int _PC30 = 0;const int _PC31 = -3;
const int _PC32 = 3;const int _PC33 = -9;const int _PC34 = -1;const int _PC35 = 7;
const int _PC36 = -1;

int ValueCenter[81] = {
_PC0,
-_PC6,-_PC3,-_PC2,-_PC8,-_PC5,-_PC1,-_PC7,-_PC4,_PC3,_PC22,
_PC0,_PC10,_PC27,-_PC35,_PC16,_PC31,-_PC34,_PC6,_PC0,-_PC22,
_PC13,-_PC26,-_PC24,_PC19,-_PC25,-_PC23,_PC1,-_PC19,-_PC16,_PC9,
-_PC21,-_PC18,_PC0,-_PC20,-_PC17,_PC4,_PC23,_PC34,_PC11,_PC28,
_PC36,_PC17,_PC32,_PC0,_PC7,_PC25,-_PC31,_PC14,_PC30,-_PC33,
_PC20,_PC0,-_PC32,_PC2,-_PC13,-_PC10,_PC0,-_PC15,-_PC12,-_PC9,
-_PC14,-_PC11,_PC5,_PC24,_PC35,_PC12,_PC29,_PC0,_PC18,_PC33,
-_PC36,_PC8,_PC26,-_PC27,_PC15,-_PC0,-_PC29,_PC21,-_PC30,-_PC28,
};


const int KingOnOpen[16] = {7,6,6,5,5,4,4,3,3,2,2,1,1,0,0,0,};


const int ValuePawn = 100;
const int valuePieceS[4] = {350,360,530,1080};



int BEval::GetMaterial(CDiagrama *board)
{
	Value0 = 0;
	PreEval(*board);

	EvalMaterial();
	if(board->ColorJuegan == blanco)
		return Value0;
	else
		return -Value0;
}

int BEval::Eval(CDiagrama *board)
{
	Value0 = 0;
	PreEval(*board);

	ct = IdCenter(*board);
	EvalMaterial();
	// development 
	Value0 += (EvalDevelopment = Development(*board));

	EvaluaPeones(*board);
	Value0 += (PstValue = EvaluaPst(0,*board));
	// Eval Alfiles 
	Value0 += (EvalAlfiles  = EvaluaAlfiles(*board));
	// EvalDamas 
	Value0 += (EvalDamas  = EvaluaDamas(*board));
	// EvalTorres 
	Value0 += (EvalTorres = EvaluaTorres(*board));
	Value0 += (PawnCover = PawnCoverage(*board));


	Value0 += EvalCenter(*board);

	// factores de finales
	Value0 *= 16;
	Value0 /= DivisorEndGame(*board);

	if(board->ColorJuegan == blanco)
		return Value0;
	else
		return -Value0;
}

void BEval::EvalMaterial()
{
	int signo = 1;
	Material = 0;
	Material += ValuePawn * (pb-pn);
	Material += (valuePieceS[caballo-2]) * (kb-kn);
	Material += (valuePieceS[alfil-2] ) * (bb-bn);
	Material += (valuePieceS[torre-2]) * (rb-rn);
	Material += (valuePieceS[dama-2]) * (qb-qn);
	//// par de alfiles
	if(bb ==2)
		Material += ParAlfiles;
	if(bn ==2)
		Material -= ParAlfiles;

	// //quien tiene ventaja material debe buscar los cambios.
	int Material0 = (rb-rn) * 500 + (mb-mn)*300 + (qb-qn) * 900; 
	if(Material >= 140 || Material <= -140)
	{
		int inc = ((_pPFase * pfase) + (_pFase * fase))/16;
		if(Material > 0)
			Material += inc;
		else
			Material -= inc;
		 //penalizamos si no nos quedan peones
		if(Material > 0 && pb == 0 && pn != 0)
			Material -= 50;
		if(Material < 0 && pn == 0 && pb != 0)
			Material += 50;
	}
	Value0 += Material;
}

CenterType BEval::IdCenter(CDiagrama &Board)
{
	u64 aux;

	if(isEndGame ) return ENDGAME;
	if(pb < 7 && pn < 7)
	{
		// si no hay peones centrales bloqueados
		aux = Board.BPiezas[peon][blanco] & ColumnasDE;
		aux = aux << 8; // avanzamos una casilla
		aux = aux & Board.BPiezas[peon][negro] & ColumnasDE;
		if(!aux)
			return WIDEOPEN;
	}
	// open: one pair of centre pawn have been exchanged
	// and there are no blocked pawn.
	bool candidato = false;
	aux = Board.BPiezas[peon][blanco] & ColumnasDE;
	if(popCount(aux) == 1)
	{
		candidato = true;
	}
	else
	{
		aux = Board.BPiezas[peon][negro] & ColumnasDE;
		if(popCount(aux) == 1)
			candidato = true;

	}
	if(candidato)
	{
		// si no hay peones centrales bloqueados
		aux = Board.BPiezas[peon][blanco] & ColumnasDE;
		aux = aux << 8; // avanzamos una casilla
		aux = aux & Board.BPiezas[peon][negro] & ColumnasDE;
		if(!aux)
			return OPEN;
	}
	if(pb == 8 && pn == 8)
	{
		// si no hay un par de peones centrales bloqueados
		aux = Board.BPiezas[peon][blanco] & ColumnasDE;
		aux = aux << 8; // avanzamos una casilla
		aux = aux & Board.BPiezas[peon][negro] & ColumnasDE;
		int bloqueos = popCount(aux);
		if(bloqueos == 1)
			return CLOSED;
		if(bloqueos > 1)
			return BLOCKED;
		// ahora miramos adyacentes cd ef
		aux = Board.BPiezas[peon][blanco] & ColumnasCD;
		aux = aux << 8; // avanzamos una casilla
		aux = aux & Board.BPiezas[peon][negro] & ColumnasCD;
		bloqueos = popCount(aux);
		if(bloqueos > 1)
			return BLOCKED;
		aux = Board.BPiezas[peon][blanco] & ColumnasEF;
		aux = aux << 8; // avanzamos una casilla
		aux = aux & Board.BPiezas[peon][negro] & ColumnasEF;
		bloqueos = popCount(aux);
		if(bloqueos > 1)
			return BLOCKED;
	}
	return AVERAGE;
}


void BEval::PreEval(CDiagrama &Board)
{
	u64 aux;
	aux = Board.BPiezas[rey][blanco];
	PosKw = bitScanAndReset(aux);
	aux = Board.BPiezas[rey][negro];
	PosKb = bitScanAndReset(aux);

	rb = popCount(Board.BPiezas[torre][blanco]);
	rn = popCount(Board.BPiezas[torre][negro]);
	r = rn+rb;

	kb = popCount(Board.BPiezas[caballo][blanco]);
	kn = popCount(Board.BPiezas[caballo][negro]);
	bb = popCount(Board.BPiezas[alfil][blanco]);
	bn = popCount(Board.BPiezas[alfil][negro]);

	mb = kb+bb; 
	mn = kn+bn; 
	m = mn+mb;
	qb = popCount(Board.BPiezas[dama][blanco]);
	qn = popCount(Board.BPiezas[dama][negro]);
	q = qn+qb;

	fase = 32 - (m + r * 3 + q *6); // 0 -> 32
	if(fase < 0)
		fase = 0; // por si hay más de dos damas.
	int fase20 = 1 + fase;
	if(fase20 >20)
		fase20 = 20;
	fase16 = fase/2;
	if(fase16 == 16) fase16 = 15;

	pb = popCount(Board.BPiezas[peon][blanco]);
	pn = popCount(Board.BPiezas[peon][negro]);
	pfase = pb+pn;
	isEndGame = (rb+mb+qb) < 3 || (rn+mn+qn) < 3;

}


void BEval::EvaluaPeones(CDiagrama &Board)
{
	register int i;
	bool ReyObstruye;
	int j;

	u64 p;
	u64 aux;

	

	int Valor = 0;
	if(pfase == 0)
	{
		Bdebiles[0] = 0ull;
		Bdebiles[1] = 0ull;
		atacadaP[0] = 0ull;
		atacadaP[1] = 0ull;
		PPasados[0] = 0ull;
		PPasados[1] = 0ull;
		Sombra[0] = 0ull;
		Sombra[1] = 0ull;
		return;
	}

	CalculaDebiles(Board);


	const bool EvalRetrasado = true;
	const bool UsaDoblados = true;
	// pasados
	int HayPasados = 0;
	p = PPasados[blanco];
	if(p)
	{
	// solo evalua el primer pasado en pasados doblados
		aux = p >> 8;	// marcamos la estela de los peones pasados
		aux |= aux >> 8;
		aux |= aux >> 16;
		aux |= aux >> 32;
		aux = ~aux;
		p &= aux;	// quitamos los que estan en la estela
	}

	const int PasadoDefendido = 0;
	while(p)
	{
		HayPasados = 1;
		i = bitScanAndReset(p);
		Valor += PPpasados[FILA(i)];
		// penalizar pieza propia en casilla promocion con p7.
		if(FILA(i) == 6)
		{
			if(Board.BOcupadas[blanco] & BCasilla(i+8))
				Valor -= OcupadaC8a;
		}

		// el defensor no tiene piezas y el rey propio no obstruye el paso del peon
		ReyObstruye = false;
		for(j=i; j < 64; j+= 8)
		{
			if(Board.board(j) == rey)
				ReyObstruye = true;
		}
		if((rn+mn+qn) == 0)
		{	
			// final de peones verificamos si el rey esta en el cuadrado
			int fm,cm,cM; // fila minima, columna minima columna maxima
			fm = FILA(i);
			if(Board.ColorJuegan == negro)
				fm--;
			cm = COLUMNA(i) -(7- fm);
			cM = COLUMNA(i) +(7- fm);
			if(FILA(PosKb) < fm || COLUMNA(PosKb) < cm || COLUMNA(PosKb) > cM)
			{
				// el rey esta fuera del cuadrado
				if(ReyObstruye)
					Valor += _pFinalPeonesReyFueraCuadrado*(FILA(i)-1); 
				else
					Valor += _pFinalPeonesReyFueraCuadrado*(FILA(i)); 
			}
			else
			{
				// apoyado por el rey y en 7a
				if(FILA(i) == 6)
				{
					if(BCasilla(i+8) & ataque[rey][PosKw])
					{
						Valor += _pFinalPeonesReyFueraCuadrado*(FILA(i));
					}
				}
				// en 6º y con el rey que apoya el avance hasta coronar
				if(FILA(i) == 5)
				{
					if((PosKw == (i+7) || PosKw == (i+9)) && ataque[rey][PosKw] & BCasilla(i))
						Valor += _pFinalPeonesReyFueraCuadrado*(FILA(i));
				}
				// en 6º y con el rey que apoya el avance hasta coronar
				if(FILA(i) == 4)
				{
					int cRey = COLUMNA(PosKw);
					if((cRey > 0 && PosKw == (i+15))
						|| (cRey < 7 && PosKw == (i+17)))
					{
						Valor += _pFinalPeonesReyFueraCuadrado*(FILA(i));
					}
				}
			}
		}
		if((rn+mn+qn) == 0 && ReyObstruye) // rey en la casilla de promocion que no sea de torre
		{
			if(PosKw == (i+8) && COLUMNA(i) != 0 && COLUMNA(i) != 7 && FILA(i) == 6)
			{
				Valor += _pFinalPeonesReyFueraCuadrado*(FILA(i));
			}
		}
	}
	p = PPasados[negro];
	if(p)
	{
	// solo evalua el primer pasado en pasados doblados
		aux = p << 8;	// marcamos la estela de los peones pasados
		aux |= aux << 8;
		aux |= aux << 16;
		aux |= aux << 32;
		aux = ~aux;
		p &= aux;	// quitamos los que estan en la estela
	}
	while(p)
	{
		HayPasados = 1;
		i = bitScanAndReset(p);
		Valor -= PPpasados[(7-FILA(i))];
		// penalizar pieza propia en casilla promocion con p7.
		if(FILA(i) == 1)
		{
			if(Board.BOcupadas[negro] & BCasilla(i-8))
				Valor += OcupadaC8a;
		}

		ReyObstruye = false;
		for(j=i; j >= 0; j-= 8)
		{
			if(Board.board(j) == rey)
				ReyObstruye = true;
		}
		if((rb+mb+qb) == 0 )
		{
			// final de peones verificamos si el rey esta en el cuadrado
			int fm,cm,cM; // fila minima, columna minima columna maxima
			fm = FILA(i);
			if(Board.ColorJuegan == blanco)
				fm++;
			cm = COLUMNA(i) - fm;
			cM = COLUMNA(i) + fm;
			if(FILA(PosKw) > fm || COLUMNA(PosKw) < cm || COLUMNA(PosKw) > cM)
			{
				// el rey esta fuera del cuadrado
				if(ReyObstruye)
					Valor -= _pFinalPeonesReyFueraCuadrado*(6-FILA(i)); 
				else
					Valor -= _pFinalPeonesReyFueraCuadrado*(7-FILA(i)); 
			}
			else
			{
				// apoyado por el rey y en 7a
				if(FILA(i) == 1 )
				{
					if(BCasilla(i-8) & ataque[rey][PosKb])
					{
						Valor -= _pFinalPeonesReyFueraCuadrado*(7-FILA(i)); 
					}
				}
				// en 6º y con el rey que apoya el avance hasta coronar
				if(FILA(i) == 2)
				{
					if((PosKb == (i-7) || PosKb == (i-9) )&& ataque[rey][PosKb] & BCasilla(i))
						Valor -= _pFinalPeonesReyFueraCuadrado*(7-FILA(i));
				}
				// en 6º y con el rey que apoya el avance hasta coronar
				if(FILA(i) == 3)
				{
					int cRey = COLUMNA(PosKb);
					if((cRey > 0 && PosKb == (i-17))
						|| (cRey < 7 && PosKb == (i-15)))
					{
						Valor -= _pFinalPeonesReyFueraCuadrado*(7-FILA(i));
					}
				}
			}
		}
		if((rb+mb+qb) == 0 && ReyObstruye) // rey en la casilla de promocion que no sea de torre
		{
			if(PosKb == (i-8) && COLUMNA(i) != 0 && COLUMNA(i) != 7 && FILA(i) == 1)
			{
				Valor -= _pFinalPeonesReyFueraCuadrado*(7-FILA(i));
			}
		}
	}

	Valor += Backward(Board);
	Valor += EvaluaPeonesDoblados(Board);

	//// conectados avanzados
	aux = (Board.BPiezas[peon][blanco] & (~PPasados[blanco]));
	while(aux)
	{
		i = bitScanAndReset(aux);
		// peones conectados
		if((BCasilla(i) & atacadaP[blanco]) || (BCasilla(i+8) & atacadaP[blanco])
			|| (((BCasilla(i) & 0x7F7F7F7F7F7F7F7Full) << 9) & Board.BPiezas[peon][blanco])
			|| (((BCasilla(i) & 0xFEFEFEFEFEFEFEFEull) << 7) & Board.BPiezas[peon][blanco])
			)
			; 
		else
			if(FILA(i) == 1)
			{
				i+=16;
				if((BCasilla(i) & atacadaP[blanco]) || (BCasilla(i+8) & atacadaP[blanco])
					|| (((BCasilla(i) & 0x7F7F7F7F7F7F7F7Full) << 9) & Board.BPiezas[peon][blanco])
					|| (((BCasilla(i) & 0xFEFEFEFEFEFEFEFEull) << 7) & Board.BPiezas[peon][blanco])
					)
				;
				else
					Valor -= PAislado; 
			}
			else
			if(FILA(i) == 4 && BCasilla(i-16) & atacadaP[blanco])
				;
			else // aislado
			{
				Valor -= PAislado; 
			}

	}

	aux = (Board.BPiezas[peon][negro] & (~PPasados[negro]));
	while(aux)
	{
		i = bitScanAndReset(aux);
		// peones conectados
		if((BCasilla(i) & atacadaP[negro]) || (BCasilla(i-8) & atacadaP[negro])
			|| (((BCasilla(i) & 0x7F7F7F7F7F7F7F7Full) >> 7) & Board.BPiezas[peon][negro])
			|| (((BCasilla(i) & 0xFEFEFEFEFEFEFEFEull) >> 9) & Board.BPiezas[peon][negro])
			)
			; 
		else
			if(FILA(i) == 6)
			{
				i-=16;
				if((BCasilla(i) & atacadaP[negro]) || (BCasilla(i-8) & atacadaP[negro])
					|| (((BCasilla(i) & 0x7F7F7F7F7F7F7F7Full) >> 7) & Board.BPiezas[peon][negro])
					|| (((BCasilla(i) & 0xFEFEFEFEFEFEFEFEull) >> 9) & Board.BPiezas[peon][negro])
					)
					; 
				else
					Valor += PAislado; 
			}
			else
			if(FILA(i) == 3 && BCasilla(i+16) & atacadaP[negro])
				; 
			else // aislado
			{
				Valor += PAislado; 
			}
	}

	ValorPeones = Valor;
	Value0 += Valor;
}
void BEval::CalculaDebiles(CDiagrama &Board)
{
  u64 mask, passedPawns; 

  passedPawns = mask = 0ull;

/* Generamos los movimientos de peon y adyacentes */ 
passedPawns = Board.BPiezas[peon][negro];
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
passedPawns = ~mask & Board.BPiezas[peon][blanco];
PPasados[blanco] = passedPawns;
/* Generamos los movimientos de peon y adyacentes */ 
passedPawns = Board.BPiezas[peon][blanco];
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
passedPawns = (~mask) & Board.BPiezas[peon][negro];
PPasados[negro] = passedPawns;

// calculamos las sombras
mask = Board.BPiezas[peon][blanco];
Sombra[blanco] =  mask >> 8 | mask >> 16 | mask >> 32 | mask >> 24 | mask >> 40 | mask >> 48;
mask = Board.BPiezas[peon][negro];
Sombra[negro] = mask << 8 | mask << 16 | mask << 24 | mask << 32 | mask << 40 | mask << 48;

}



int BEval::Backward(CDiagrama &Board)
{
	int Valor = 0;
	u64 p;
	int i;
	if(pb > 1 && pn > 1)// && fase < 20)
	{
		u64 aux;
		// peones retrasados
		{
			// un peon no pasado o atacado por otro peon
			aux = Board.BPiezas[peon][blanco] & ~PPasados[blanco];
			aux &= ~atacadaP[negro];
			// no defendido por otro peon
			aux &= ~atacadaP[blanco];
			aux <<=8; // una fila adelante
			// casilla libre atacada por un peon contrario y no defendida por uno propio
			aux &= ~Board.BPiezas[peon][blanco];
			aux &= ~Board.BPiezas[peon][negro];
			aux &= atacadaP[negro];
			aux &= ~atacadaP[blanco];
			aux &= Bdebiles[blanco];
			aux >>=8; // una fila atras
			aux &= FILAS234;
			while(aux )
			{
				i = bitScanAndReset(aux);
				Valor -= _pVALOR_RETRASADO; 
			}
		}
		// ahora peones retrasados del negro
		{
			p = Bdebiles[negro];
			aux = Board.BPiezas[peon][negro] & ~PPasados[negro];
			aux &= ~atacadaP[blanco];
			// no defendido por otro peon
			aux &= ~atacadaP[negro];
			aux >>=8; // una fila adelante
			aux &= ~Board.BPiezas[peon][blanco];
			aux &= ~Board.BPiezas[peon][negro];
			aux &= atacadaP[blanco];
			aux &= ~atacadaP[negro];
			aux &= Bdebiles[negro];
			aux <<=8; // una fila atras
			aux &= FILAS765;
			while(aux)
			{
				i = bitScanAndReset(aux);
				Valor += _pVALOR_RETRASADO; //-fase;
			}
		}
	}
	return Valor;
}

int BEval::EvaluaPeonesDoblados(CDiagrama &Board)
{
	int hayDob[2];
	int Valor = 0;
	int i = 0;
	int col =0;
	u64 aux;
	hayDob[blanco] = hayDob[negro] = 0;
	aux = (Board.BPiezas[peon][blanco] & Sombra[blanco]);
	hayDob[blanco] = popCount(aux);
	aux = (Board.BPiezas[peon][negro] & Sombra[negro]);
	hayDob[negro] = popCount(aux);
	Valor += (hayDob[blanco]-hayDob[negro]) * _pVALOR_DOBLADO;

	if(hayDob[blanco] == 0 && bb == 2)
		Valor += _BishopPair1;
	if(hayDob[blanco] == 0 && bb != 2)
		Valor -= _DoubledBP;
	
	if(hayDob[negro] == 0 && bn == 2)
		Valor -= _BishopPair1;
	if(hayDob[negro] == 0 && bn != 2)
		Valor += _DoubledBP;

	return Valor;
}

extern char PstPn[64];
extern char PstPk[16][64];


int BEval::EvaluaPst(int mode,CDiagrama &Board)
{
	int Value = 0;
	int casilla;
	u64 piezas;
	piezas = Board.BPiezas[rey][blanco];
	while(piezas)
	{
		casilla = bitScanAndReset(piezas);
		Value +=PstPk[fase16][casilla];
	}
	piezas = Board.BPiezas[rey][negro];
	while(piezas)
	{
		casilla = bitScanAndReset(piezas);
		Value -= PstPk[fase16][casilla^070];
	}

	piezas = Board.BPiezas[caballo][blanco];
	// buscamos la casilla donde se ubica el caballo
	while(piezas)
	{
		casilla = bitScanAndReset(piezas);
		Value += PstPn[casilla];
	}
	piezas = Board.BPiezas[caballo][negro];
	// buscamos la casilla donde se ubica el caballo
	while(piezas)
	{
		casilla = bitScanAndReset(piezas);
		Value -= PstPn[casilla^070];
	}
	return Value;
}

int BEval::EvaluaAlfiles(CDiagrama &Board)
{
	int Valor = 0;
	if((bb + bn) > 0)
	{
		{
			u64 piezas;
			int casilla;
			const u64 TargetsWhiteBishop = 0x00ffa1a100000000ull;
			const u64 TargetsBlackBishop = 0xa1a1ff00ull;
			u64 targets;
			u64 Diagonal;
			u64 Peones = Board.BPiezas[peon][blanco] | Board.BPiezas[peon][negro];
			Peones &= 0x0000ffffffff0000ull;
			int TargetSquare;
			int AlfilMalo = 1;
			piezas = Board.BPiezas[alfil][blanco];
			// buscamos la casilla donde se ubica el alfil
			while(piezas)
			{
				casilla = bitScanAndReset(piezas);
				// buscamos las diagonales principales.
				targets = ataque[alfil][casilla] & TargetsWhiteBishop;
				AlfilMalo = 1;
				while(targets)
				{
					TargetSquare = bitScanAndReset(targets);
					Diagonal = ataque[alfil][casilla] & ataque[alfil][TargetSquare];
					// si en la diagonal no hay peon alguno
					// estamos en una diagonal libre.
					// catalogamos el alfil como bueno
					if((Diagonal & Peones) == 0ull)
					{
						// alfil bueno
						AlfilMalo = 0;
						if((Diagonal & ataque[rey][Board.PosReyes[negro]]) != 0ull)
							Valor += _AlfilAtaque;
					}
				}
				if(AlfilMalo)
					Valor -= _AlfilMalo;

			}
			piezas = Board.BPiezas[alfil][negro];
			// buscamos la casilla donde se ubica el rey
			while(piezas)
			{
				casilla = bitScanAndReset(piezas);
				// buscamos las diagonales principales.
				targets = ataque[alfil][casilla] & TargetsBlackBishop;
				AlfilMalo = 1;
				while(targets)
				{
					TargetSquare = bitScanAndReset(targets);
					Diagonal = ataque[alfil][casilla] & ataque[alfil][TargetSquare];
					// si en la diagonal no hay peon alguno
					// estamos en una diagonal libre.
					// catalogamos el alfil como bueno
					if((Diagonal & Peones) == 0ull)
					{
						// alfil bueno
						AlfilMalo = 0;
						if((Diagonal & ataque[rey][Board.PosReyes[blanco]]) != 0ull)
							Valor -= _AlfilAtaque;
					}
				}
				if(AlfilMalo)
					Valor += _AlfilMalo;

			}
		}
	}
	return Valor;
}

int BEval::EvaluaDamas(CDiagrama &Board)
{
	int Valor = 0;
	if((qb+qn) == 0)
		return 0;

	u64 piezas;
	int casilla;
	const u64 TargetsWhite = 0x00ffa1a100000000ull;
	const u64 TargetsBlack = 0xa1a1ff00ull;
	u64 targets;
	u64 Diagonal;
	u64 Peones = Board.BPiezas[peon][blanco] | Board.BPiezas[peon][negro];
	Peones &= 0x0000ffffffff0000ull;
	int TargetSquare;
	int Bad;
	piezas = Board.BPiezas[dama][blanco];
	while(piezas)
	{
		casilla = bitScanAndReset(piezas);
		Bad = 1;
		// buscamos las diagonales principales.
		targets = ataque[dama][casilla] & TargetsWhite;
		while(targets)
		{
			TargetSquare = bitScanAndReset(targets);
			Diagonal = ataque[dama][casilla] & ataque[dama][TargetSquare];
			// si en la diagonal no hay peon alguno
			// estamos en una diagonal libre.
			// catalogamos el alfil como bueno
			if((Diagonal & Peones) == 0ull)
			{
				Bad = 0;
				Valor += _damaMob;
				if((Diagonal & ataque[rey][Board.PosReyes[negro]]) != 0ull)
					Valor += _damaAtaque;
			}
		}
		if(Bad)
			Valor -= _damaMala;
	}
	piezas = Board.BPiezas[dama][negro];
	while(piezas)
	{
		casilla = bitScanAndReset(piezas);
		Bad = 1;
		// buscamos las diagonales principales.
		targets = ataque[dama][casilla] & TargetsBlack;
		while(targets)
		{
			TargetSquare = bitScanAndReset(targets);
			Diagonal = ataque[dama][casilla] & ataque[dama][TargetSquare];
			// si en la diagonal no hay peon alguno
			// estamos en una diagonal libre.
			if((Diagonal & Peones) == 0ull)
			{
				Bad = 0;
				Valor -= _damaMob;
				if((Diagonal & ataque[rey][Board.PosReyes[blanco]]) != 0ull)
					Valor -= _damaAtaque;
			}
		}
		if(Bad)
			Valor += _damaMala;
	}
	if((Board.BPiezas[peon][negro] & Fila7)||(Board.BPiezas[rey][negro] & Fila8))
	{
		if(Board.BPiezas[dama][blanco] & Fila7)
		{
			Valor += _pDamaSeptima;
		}
	}

	if((Board.BPiezas[peon][blanco] & Fila2)||(Board.BPiezas[rey][blanco] & Fila1))
	{
		if(Board.BPiezas[dama][negro] & Fila2)
		{
			Valor -= _pDamaSeptima;
		}
	}
	return Valor;
}
int BEval::EvaluaTorres(CDiagrama &Board)
{
	if((rb+rn) == 0)
		return 0;
	int Valor = 0;
	{
		{
			u64 piezas = 0x0ull;
			int casilla = 0;
			int crey = Board.COLUMNA(Board.PosReyes[negro]);
			int c;
			piezas = Board.BPiezas[torre][blanco];
			u64 peonesC = (Board.BPiezas[peon][blanco] | Board.BPiezas[peon][negro]);
			// buscamos la casilla donde se ubica la torre
			while(piezas)
			{
				casilla = bitScanAndReset(piezas);
				// torre en columna abierta
				c = Board.COLUMNA(casilla);
				if(!((ColumnaA << c) & peonesC))
				{
					Valor += _TAbierta; 
				}
				else
				{
					if(((ColumnaA << c) & ~Sombra[blanco]) & BCasilla(casilla))
					{
						Valor += _TorreSemiAbierta; 
					}
					else
					{
						// no es una columna abierta o semi ver si tenemos la torre atrapada por el rey
						if(casilla == H1)
						{
							// torre atrapada
							if(Board.board(G1) == rey || Board.board(F1) == rey)
							{
								Valor -= RookTrapped;
							}
						}
						if(casilla == A1)
						{
							// torre atrapada
							if(Board.board(C1) == rey || Board.board(B1) == rey)
							{
								Valor -= RookTrapped;
							}
						}
					}
				}
			}
			piezas = Board.BPiezas[torre][negro];
			crey = Board.COLUMNA(Board.PosReyes[blanco]);
			// buscamos la casilla donde se ubica la torre
			while(piezas)
			{
				casilla = bitScanAndReset(piezas);
				c = COLUMNA(casilla);
				if(!((ColumnaA << c) & peonesC))
				{
					Valor -= _TAbierta; 
				}
				else
				{
					if(((ColumnaA << c) & ~Sombra[negro]) & BCasilla(casilla))
					{
						Valor -= _TorreSemiAbierta; 
					}
					else
					{
						// no es una columna abierta o semi ver si tenemos la torre atrapada por el rey
						if(casilla == H8)
						{
							// torre atrapada
							if(Board.board(G8) == rey || Board.board(F8) == rey)
							{
								Valor += RookTrapped;
							}
						}
						if(casilla == A8)
						{
							// torre atrapada
							if(Board.board(C8) == rey || Board.board(B8) == rey)
							{
								Valor += RookTrapped;
							}
						}
					}
				}
			}
		}
	}
	// Torres en columnas semi y abiertas
	if((Board.BPiezas[peon][negro] & Fila7)||(Board.BPiezas[rey][negro] & Fila8))
	{
		if(Board.BPiezas[torre][blanco] & Fila7)
		{
			Valor += _TorreSeptima; 
		}
	}

	if((Board.BPiezas[peon][blanco] & Fila2)||(Board.BPiezas[rey][blanco] & Fila1))
	{
		if(Board.BPiezas[torre][negro] & Fila2)
		{
			Valor -= _TorreSeptima; 
		}
	}

	return Valor;
}


int BEval::PieceDevelopped(CDiagrama &Board,int p,int color)
{
	u64 aux;
	int retorno = 0;
	int pos,f;
	if(Board.BPiezas[p][color])
	{
		aux = Board.BPiezas[p][color];
		while(aux)
		{
			pos = bitScanAndReset(aux);
			if(color == negro) pos = pos ^070;
			f = FILA(pos);
			switch(p)
			{
			case caballo:
				if(f != 0 && f != 7)
					retorno++;
				break;
			case alfil:
				if(f != 0 && f != 7)
					retorno++;
				break;
			case torre:
				// a rook is considered developped if the pawn of his own side 
				// in front of it is advanced at least to the 5th or gone altogether.
				if(color == blanco)
				{
					if((CampoBlanco & Columnas[COLUMNA(pos)] & Board.BPiezas[peon][blanco]) == 0ull)
						retorno++;
				}
				else
				{
					if((CampoNegro & Columnas[COLUMNA(pos)] & Board.BPiezas[peon][negro]) == 0ull)
						retorno++;
				}
				break;
			case dama:
				if(f != 0 && f != 7)
					retorno++;
				break;
			}
		}
	}
	return retorno;
}
int BEval::Development(CDiagrama &Board)
{
	int Valor = 0;
	int bd,nd;
	bd = nd = -10;
	if(isEndGame ) return 0;

	//
	// a) 1 point for each piece  that is in a good developped position 
	//		(including those that have not moved yet).
	//		a rook is considered developped if the pawn of its own side in front of it 
	//		is advanced al least to the 5th rank or gone altogether.
	
	// cada pieza que no esta en el tablero esta desarrollada
	bd += 7 - (rb+mb+qb);
	nd += 7 - (rn+mn+qn);
	//
	// tomamos el pst de la pieza calculamos la media si en su pos está por encima de la media es ok
	bd += PieceDevelopped(Board,caballo,blanco);
	bd += PieceDevelopped(Board,alfil,blanco);
	bd += PieceDevelopped(Board,torre,blanco);

	nd += PieceDevelopped(Board,caballo,negro);
	nd += PieceDevelopped(Board,alfil,negro);
	nd += PieceDevelopped(Board,torre,negro);

	// b) 1 point for having the king in a safe position.
	//		castling get 1 point.
	//
	if(ct == BLOCKED) bd++;
	if(PosKw == G1 || PosKw == H1)		bd++;
	else
	{
		if(PosKw == A1 || PosKw == B1)	bd++;
	}
	if(ct == BLOCKED) nd++;
	if(PosKb == G8 || PosKb == H8)		nd++;
	else
	{
		if(PosKb == A8 || PosKb == B8)	nd++;
	}

	//
	// c) 1 for each of the two centre pawns advanced beyond their original square
	//
	if(Board.board(E2) != peon)
		bd++;
	// fianchetto
	if(Board.board(E2) == peon && Board.board(G3) == peon && Board.board(G2) == alfil )
		bd++;
	if(Board.board(D2) != peon)
		bd++;
	if(Board.board(D2) == peon && Board.board(B3) == peon && Board.board(B2) == alfil )
		bd++;
	if(Board.board(E7) != peon)
		nd++;
	if(Board.board(E7) == peon && Board.board(G6) == peon && Board.board(G7) == alfil )
		nd++;
	if(Board.board(D7) != peon)
		nd++;
	if(Board.board(D7) == peon && Board.board(B6) == peon && Board.board(B7) == alfil )
		nd++;

	// TODO
	// d) 1 extra point for a knight in the centre, as long as it cannot be driven
	//		away, and the player has already developed over half his pieces.


	// calibramos
const int baseValue = 33; 
const int DecrementValue = 4; 

	if(bd > 0) bd = 0;
	if(nd > 0) nd = 0;
	int factor;
	if(bd > nd )
	{
		factor = baseValue + DecrementValue * bd;
		if(factor > 0)
			Valor = factor * (bd-nd);
	}
	if(nd > bd )
	{
		factor = baseValue + DecrementValue * nd;
		if(factor > 0)
			Valor = factor * (bd-nd);
	}

	return Valor;
}

int BEval::PawnCoverage(CDiagrama &Board)
{
	int ReturnValue = 0;
	int Valor = 0;

	// valor de 0 a 8
	// 1 2 1   |2 2
	// 2 4 2   |4 4
	//   R     |R

const int MinCov = 10;

	u64 Raux;
	int casillaRey;

	Raux = Board.BPiezas[rey][blanco];
	casillaRey = bitScanAndReset(Raux);
	// rey en columna abierta
	if((rn+qn) > 0)
	{
		if((Columnas[COLUMNA(casillaRey)] & (Board.BPiezas[peon][blanco] | Board.BPiezas[peon][negro])) == 0ull)
		{
			Valor -= KingOnOpen[fase16];
		}
	}
	if(FILA(casillaRey) < 3)
	{
	if(COLUMNA(casillaRey) == 0)
	{
		// A Column;
		Raux = BCasilla(casillaRey+8) | BCasilla(casillaRey+9);
		Valor += popCount(Raux & Board.BPiezas[peon][blanco]) * 4;
		Raux = BCasilla(casillaRey+16) | BCasilla(casillaRey+17);
		Valor += popCount(Raux & Board.BPiezas[peon][blanco]) * 2;
	}
	else
		if(COLUMNA(casillaRey) == 7)
		{
			// Columna H
			Raux = BCasilla(casillaRey+8) | BCasilla(casillaRey+7);
			Valor += popCount(Raux & Board.BPiezas[peon][blanco]) * 4;
			Raux = BCasilla(casillaRey+16) | BCasilla(casillaRey+15);
			Valor += popCount(Raux & Board.BPiezas[peon][blanco]) * 2;
		}
		else
		{ 
			// General case
			Raux = BCasilla(casillaRey+8) ;
			Valor += popCount(Raux & Board.BPiezas[peon][blanco]) * 4;
			Raux = BCasilla(casillaRey+7) | BCasilla(casillaRey+9) | BCasilla(casillaRey+16);
			Valor += popCount(Raux & Board.BPiezas[peon][blanco]) * 2;
			Raux = BCasilla(casillaRey+17) | BCasilla(casillaRey+15);
			Valor += popCount(Raux & Board.BPiezas[peon][blanco]) * 1;
		}
	}
	Valor -= MinCov; // 0->8 => -8 -> 0
	if(Valor > 0) Valor = 0;
	ReturnValue = Valor * _pValorCobertura[fase16];


	Valor = 0;
	Raux = Board.BPiezas[rey][negro];
	casillaRey = bitScanAndReset(Raux);
	if((rb+qb) > 0)
	{
		if((Columnas[COLUMNA(casillaRey)] & (Board.BPiezas[peon][negro] | Board.BPiezas[peon][blanco])) == 0ull)
		{
			Valor -= KingOnOpen[fase16];
		}
	}

	if(FILA(casillaRey) > 4)
	{
		if(COLUMNA(casillaRey) == 0)
		{
			// A Column;
			Raux = BCasilla(casillaRey-8) | BCasilla(casillaRey-7);
			Valor += popCount(Raux & Board.BPiezas[peon][negro]) * 4;
			Raux = BCasilla(casillaRey-16) | BCasilla(casillaRey-15);
			Valor += popCount(Raux & Board.BPiezas[peon][negro]) * 2;
		}
		else
			if(COLUMNA(casillaRey) == 7)
			{
				// Columna H
				Raux = BCasilla(casillaRey-8) | BCasilla(casillaRey-9);
				Valor += popCount(Raux & Board.BPiezas[peon][negro]) * 4;
				Raux = BCasilla(casillaRey-16) | BCasilla(casillaRey-17);
				Valor += popCount(Raux & Board.BPiezas[peon][negro]) * 2;
			}
			else
			{ 
				// General case
				Raux = BCasilla(casillaRey-8) ;
				Valor += popCount(Raux & Board.BPiezas[peon][negro]) * 4;
				Raux = BCasilla(casillaRey-7) | BCasilla(casillaRey-9) | BCasilla(casillaRey-16);
				Valor += popCount(Raux & Board.BPiezas[peon][negro]) * 2;
				Raux = BCasilla(casillaRey-17) | BCasilla(casillaRey-15);
				Valor += popCount(Raux & Board.BPiezas[peon][negro]) * 1;
			}
	}
	Valor -= MinCov; // 0->8 => -8 -> 0
	if(Valor > 0) Valor = 0;
	ReturnValue -= Valor * _pValorCobertura[fase16];

	return ReturnValue;
}

int	BEval::DivisorEndGame(CDiagrama &Board)
{
	if(fase16 < 14) return 16;
	// si final de alfiles de distinto color
	if(bb==1 && bn == 1)
	{
		if(Board.BPiezas[alfil][blanco] & CasillasBlancas && Board.BPiezas[alfil][negro] & CasillasNegras)
			return 17+fase16;
		if(Board.BPiezas[alfil][negro] & CasillasBlancas && Board.BPiezas[alfil][blanco] & CasillasNegras)
			return 17+fase16;
	}
	return 16;
}

int	BEval::EvalCenter(CDiagrama &Board)
{
int Tie1 = 0;
	if(ct == ENDGAME) return 0;
	if(Board.board(D4) == peon)
	{
		Tie1 += Board.color(D4)+1;
	}
	if(Board.board(E4) == peon)
	{
		Tie1 += (Board.color(E4)+1)*3;
	}
	if(Board.board(D5) == peon)
	{
		Tie1 += (Board.color(D5)+1)*9;
	}
	if(Board.board(E5) == peon)
	{
		Tie1 += (Board.color(E5)+1)*27;
	}

	return ValueCenter[Tie1];
}

