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

const int PesoHashJugada = 0x1f0;
const unsigned int PesoCaptura = 0x100;
const int PesoCapturaBuena = 64;
const int PesoKiller = 0x0f0;
const int JaqueBueno = 0x041;
const int PesoCapturaMala = 0x010;
const int JugadaNormal = 0x040;
const int JaqueMalo = 0x015;
const int PesoDudosa = 0x20;
const int MaxProb = 0x1ff;
const int PesoEnroqueCorto = 0x61;
const int PesoEnroqueLargo = 0x60;
const int PesoTorre7 = 0x62;
const int PesoDama7 = 0x63;
const int PesoTorreOp = 0x5F;
const int PesoDamaOp = 0x5E;
const int PesoAmenazada = 0x50;
const int PesoAvancePasado = 0x042;
const int PesoInsegura = 0x09;
const int JugadaBuenaHistoria = 0x041; // de momento un paso delante de una jugada normal
const int ValorPiezas[] = {
	0,100,300,300,500,900,INFINITO };

#define PIEZA(J) (J).desglose.pieza

class CSort
{
public:
	u64 Maps[2][8][8];
	u64 BOcupadas[2];
	u64 BPiezas[8][2]; // pieza color
	u64 atacadaP[2];	// casillas atacadas por un peon
	int PosP[2][8][8]; // casilla donde esta la pieza
	u64 PiezasAmenazadas; // piezas propias amenazadas
	u64 AtFC ,AtDiag;
	int PosReyContrario;
	u64 ReyContrario;

private:
	int OrderQuiesce;
	int FaseGeneracion;
	int HashJugada;
	CJugada JHashJugada;
	int Killer[MAXKILLERS];
	u64 PPasados[2];

	int n;
	int actual;
	int ColorJuegan;
	int EstadoEnroque;
	int en_pasant;

	// asumimos [color][tipo pieza][numero] Hack maximo 2 + 8 caballos...
	int PosReyes[2]; // posicion de los reyes en el tablero
	int Damas[2]; // bando
	int Torres[2];
	int Caballos[2];
	int Alfiles[2]; // bando color
	int Peones[2];

	CJugada JugadasPosibles[MAXJUGADAS];

	int EsValida(int HashJugada);
	void CalculaMaps();
	int GeneraCapturas();
	int GeneraRestoJugadas();
	int board(int t);
	int color(int t);
	inline int FILA(int s)	{return ((s) >> 3);};
	inline int COLUMNA(int s)	{ return ((s) & 7);};


	void CalculaAmenazadas();
	void CalculaPeso(CJugada  Jugadas[MAXJUGADAS], int tope);
	void CalculaPesoQ(CJugada  Jugadas[MAXJUGADAS], int tope);
	void PreEsJaque();
	int  EsJaque(CJugada &j);
	void EvaluaPeso(CJugada &J);
	void EvaluaPesoQ(CJugada &J);
	int EsCapturaBuena(CJugada &J);
	int Cuenta(int casilla);
	int MenorDefensor(int casilla);
	int CuentaC(int casilla,int color1);
	int EsAtacadaMaps(int casilla,int bando);
	void OrdenaParcial();
	int EstoyEnJaque();
	void QuitaHash();
	int EsDoble(CJugada J);
	int EsSegura(CJugada J);
public:
	void Init(CDiagrama &Tablero,int EsQuiesce);
	CJugada GetNext();
	CJugada GetNextQ();
	void Rewind();
	int GeneraJaques();
	
};
enum FASESG {
	FASE_INICIAL =0,FASE_HASH,FASE_CAPBUENAS,FASE_RESTOJUGADAS
};
