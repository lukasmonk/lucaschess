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

const int VPieza[2][6] = {
	{0,100,300,300,500,900},
	{0,-100,-300,-300,-500,-900},
};

enum CenterType {
	WIDEOPEN,OPEN,AVERAGE,CLOSED,BLOCKED,ENDGAME
};

extern char Pst[2][2][8][64];  // op=0 eg=1 |color | piece |sq  none= 0 pawn=1 knight=2 bishop 3 rook 4 queen 5 king 6 Passed 7
extern int DuoPst[2][8][64];  // color piece square

typedef int PstType[2][8][64];

class CDiagrama
{
public:
////////////////////////////////////////////////////////////////////////////
	// BITBOARD
	u64 hash;
	int ColorJuegan;
	u64 BOcupadas[2];
  u64 BPiezas[8][2]; // pieza color
	int PosReyes[2]; // posicion de los reyes en el tablero
	int Tablero[64];
	int EstadoEnroque;
	int en_pasant;
//	short PstValues[2];
	int PstDValue;
public:
	int MatCount[2][8];

	int HashJugada;
	int Killer[MAXKILLERS];

	// posibles movimientos en esta posición
	CJugada JugadasPosibles[MAXJUGADAS];  // array de 256 jugadas por bando
	int tope;
	// Nombres cortos piezas
	char NombreColumnas[9];
	char NombreFilas[9]; 
	char Algebra[10];
public:
	// Status de movimiento de rey y torres ( para los 0-0)
	// tomará el valor 0 si no se ha movido y 1 si ha sido movido

	int board(int c);
	int color(int c);


  void InitAtaque();
	int GetHashMove(unsigned int *Jugadas,int tope);
	int EsLegal(CJugada &J);

	void SwitchColor();
	HRESULT MueveAlgebra(char *JugAlg);
	int MuevePgn(char *JugAlg);
	int IdentificaPgn(char *JugAlg);
	int EstoyEnJaque();
	
	int Evalua(int alpha,int beta);

	// variables internas

	inline void QuitaZ(int pieza, int color1,int casilla) {
		ClearBB(BPiezas[pieza][color1],casilla);
		ClearBB(BOcupadas[color1],casilla);
	    hash ^= GetZobKey(casilla,color1,pieza);
		MatCount[color1][pieza]--;
		Tablero[casilla] = ninguna;
		PstDValue -= DuoPst[color1][pieza][casilla];
	};
	inline  void PonZ(int pieza,int color1,int pos) {
		SetBB(BOcupadas[color1],pos);
		SetBB(BPiezas[pieza][color1],pos);
	    hash ^= GetZobKey(pos,color1,pieza);
		MatCount[color1][pieza]++;
		Tablero[pos] = pieza + color1*8;
		PstDValue += DuoPst[color1][pieza][pos];
	};

	inline COLOR GetColor(int pos)
	{
		return (COLOR)color(pos);
	};


	// contructor
	CDiagrama();
	// destructor
	~CDiagrama();
	// Otras
	void Dibuja(FILE *display_file);
	void Dibuja();
	void DibujaP();
	
	inline unsigned int FILA(int s)	{return ((s) >> 3);};
	inline unsigned int COLUMNA(int s)	{ return ((s) & 7);};
	inline int COLORCASILLA(int s)	{return ((1+FILA(s) + COLUMNA(s)) % 2);};
	inline int Maxi(int a,int b) { return a >= b ? a:b;};
	inline int Distancia(int a,int b) { return Maxi(abs((int)(FILA(a)-FILA(b))),abs((int)(COLUMNA(a)-COLUMNA(b))));};
	int Mueve(const CJugada &j);
	int Mueve(int j);
	void DesMueve(int j);
	void DesMueve(const CJugada &j);
	void CalculaJugadasPosibles();

	int EsAtacada(int casilla,int bando);
public:
	void LoadFEN(char *fen,int Simetrico);
	char *SaveFEN();
	u64 GetHash();

	int Es7a(CJugada &J);
	int TotalPieces(int Color);
	int TotalPCtm();
public:
	int Opening,EndGame;
	short SafetyCounters[2];
	short SafetyPiece[2];
	int PosKw,PosKb;
	int r,kb,kn,bb,bn,mb,mn,m,qb,qn,q,fase,fase16,pb,pn,pfase; // char
bool isEndGame;
	CenterType ct;
	int Material[2];
	int DevelopmentEval;
	int ValorPeones[2];
	u64 Ocupadas;
	u64 Bdebiles[2];
	u64 atacadaP[2];
	u64 PPasados[2];
	u64 Sombra[2];
	int EvalCaballos[2],EvalAlfiles[2] ,EvalDamas[2] ,EvalTorres[2] ;
	int PawnCover[2];

	int Eval(int alpha,int beta);
	int GetMaterial();
protected:
	inline int			GetTapVal(int op,int end)
	{
		int Value0 = ((op *(15-fase16) + end *(fase16)) / 15);
		if(ColorJuegan == blanco) // convert to side to move perspective.
			return Value0;
		else
			return -Value0;
	}

	CenterType  IdCenter();
	void		PreEval();
	void		EvalMaterial();
	void		EvaluaPeones();
	void		CalculaDebiles();
	inline void Backward();
	inline void	EvaluaPeonesDoblados();
	void		EvalKnight();
	void		EvaluaAlfiles();
	void		EvaluaDamas();
	void		EvaluaTorres();
	void		PawnCoverage();
	int			DivisorEndGame();
	int			Kpk();
	int			KingSafety();
	void		Development();
public:
static const u64 FlancoRey = 0x0f0f0f0f0f0f0f0full;
static const u64 FlancoDama = 0xf0f0f0f0f0f0f0f0ull;

};
