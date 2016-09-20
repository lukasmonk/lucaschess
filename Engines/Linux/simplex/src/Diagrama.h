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

#include "BEval.h"

const int VPieza[2][6] = {
	{0,100,300,300,500,900},
	{0,-100,-300,-300,-500,-900},
};

class CDiagrama
{
public:
////////////////////////////////////////////////////////////////////////////
	// BITBOARD
	u64 hash;
	int ColorJuegan;
	int Material;
	u64 BOcupadas[2];
  u64 BPiezas[8][2]; // pieza color
	int PosReyes[2]; // posicion de los reyes en el tablero
	int Tablero[64];
	int EstadoEnroque;
	int en_pasant;

public:
	BEval Bev;
	int MatCount[2][6];

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
	
	int Evalua();

	// variables internas

	inline void QuitaZ(int pieza, int color1,int casilla) {
		ClearBB(BPiezas[pieza][color1],casilla);
		ClearBB(BOcupadas[color1],casilla);
	    hash ^= GetZobKey(casilla,color1,pieza);
		MatCount[color1][pieza]--;
		Material -= VPieza[color1][pieza];
		Tablero[casilla] = ninguna;
	};
	inline  void PonZ(int pieza,int color1,int pos) {
		SetBB(BOcupadas[color1],pos);
		SetBB(BPiezas[pieza][color1],pos);
	    hash ^= GetZobKey(pos,color1,pieza);
		MatCount[color1][pieza]++;
		Material += VPieza[color1][pieza];
		Tablero[pos] = pieza;
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
	
	inline int FILA(int s)	{return ((s) >> 3);};
	inline int COLUMNA(int s)	{ return ((s) & 7);};
	inline int COLORCASILLA(int s)	{return ((1+FILA(s) + COLUMNA(s)) % 2);};
	int Mueve(const CJugada &j);
	int Mueve(int j);
	void DesMueve(int j);
	void DesMueve(const CJugada &j);
	void CalculaJugadasPosibles();

	int EsAtacada(int casilla,int bando);
public:
	void LoadEPD(char *fen,int Simetrico);
	char *SaveEPD();
	u64 GetHash();

	int Es7a(CJugada &J);
	int TotalPieces(int Color);
	int TotalPCtm();
	inline int GetMatValue() {
		if(ColorJuegan == negro)
			return -Material;
		return Material;
	}
};
