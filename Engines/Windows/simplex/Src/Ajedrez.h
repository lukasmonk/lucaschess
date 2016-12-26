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

// include para el ajedrez
#if !defined(AJEDREZ)
#define AJEDREZ

typedef unsigned long long u64;
typedef unsigned char u8;
typedef unsigned int u32;

#ifdef _MSC_VER
#if _MSC_VER >= 1200
#define FORCEINLINE __forceinline
#else
#define FORCEINLINE __inline
#endif
#else
#define FORCEINLINE inline
#endif

extern u64 GetZobKey(int casilla,int color,int pieza);

extern const u64 bcasilla[64];
extern const u64 ncasilla[64];
//#define BCasilla(c) (u64)(1ull << c)
#define BCasilla(c) (bcasilla[c])
#define SetBB(bb,casilla) bb |= BCasilla(casilla)
//#define ClearBB(bb,casilla)	bb &= ((u64)(BCasilla(casilla))^0xffffffffffffffffull)
#define ClearBB(bb,casilla)	bb &= ncasilla[casilla]
//#define ClearBB(bb,casilla)	bb &= (~BCasilla(casilla))

#include <math.h>

#include "HashJugadas.h"

#define CUTOFF

#define MAXJUGADAS 200

#include "memory.h"
#include "string.h"
#include <stdio.h> 

#define PLAYER 0
#define OPPONENT 1

#define ROOTNODE 0

enum PIEZA { ninguna = 0,peon,caballo,alfil,torre,dama,rey,peonn };
enum COLOR { blanco = 0, negro=1 ,neutro=2 };
enum CASILLA {
     A1,B1,C1,D1,E1,F1,G1,H1,
     A2,B2,C2,D2,E2,F2,G2,H2,
     A3,B3,C3,D3,E3,F3,G3,H3,
     A4,B4,C4,D4,E4,F4,G4,H4,
     A5,B5,C5,D5,E5,F5,G5,H5,
     A6,B6,C6,D6,E6,F6,G6,H6,
     A7,B7,C7,D7,E7,F7,G7,H7,
     A8,B8,C8,D8,E8,F8,G8,H8,
     };

#define MATE 30000

#define HRESULT int
#define LPCTSTR (char *)


// estructura de jugada
// debe contener todo lo necesario para realizar la jugada y deshacerla
// sin necesidad de acceder a datos adicionales (board[] color[] etc...)
// al mismo tiempo ha de ser facilmente utilizable como un int

#define DESP_TO 6 
#define DESP_CORONAR 12
#define DESP_CAPTURA 16
#define DESP_PIEZA 20
#define DESP_PESO 23

#define MASCARA_SINPESO 0x7fffff


#define INFINITO 999999

//#define BITS
#define STNORMAL 0
#define STJAQUE 1
#define STMATE 2
#define STAHOGADO 3
#define STDOYMATE 4
#define STINVALIDO 10

// estados del enroque
#define MTDB 1
#define MTRB 2
#define MRB  4
#define MRN  8
#define MTDN 16
#define MTRN 32

extern int PesoPieza[6];
#include "Jugada.h"
#include "Diagrama.h"
#include "Sort.h"

class CHashJugadas;

#define TOPENODOSREPASO 300

extern int ModoEvaluacion;

#define SOLOMATERIAL 1
#define MAXDEPTH 70
#define MAXDEPTHC 65


class CPartida
{
public:
	int NodosVisitados;
	int NodosRepasados;
	int Depth;
	int SelDepth;
	int inicio; // para controlar el tiempo de pensar.
	int cancelado;
	int Unica;
	// parametros de analisis
	int LimiteProfundidad;
	int tiempo;
	int incremento;
	int tiempo_limite; // limite de tiempo.
	int stHistory;
	// moves 
	u64 HashHistory[MAXDEPTH];
	CJugada MoveHistory[MAXDEPTH];
	int StateHistory[MAXDEPTH];

	CDiagrama Taux;  // tablero auxiliar pa pensar
	CHashJugadas HashJ;
	CDiagrama T;	// tablero principal donde jugar y ver
public:
	char JugadaActual[10];
	CJugada Historia[50]; // setMejor path
	int DepthHistoria;// setMejor path

	void InitBranchFactorMesure();
	void DumpBranchFactor();

	void CloseThread();
	int AnalisisFinalizado;
	void Analiza();
	HRESULT Mueve(char *Jugada);
	void Nueva();
	CPartida();
	~CPartida();
private:
	void PrintInfo(int value,char * path);
	void Move(CJugada &J);
	void TakeBack();
public:
	// Analiza la posicion intentando refuta las posiciones
	void IterativeDeepening(void);

	int PVS(int depth, int alpha, int beta, char *Global,int doNull);
	int Quiesce(int alpha,int beta);
	inline int GetEval(int alpha, int beta);
	void MuevePath(char *path);

	void Cancela();
	void LoadEPD(char *fen);
public:

	int ColorJuegan();

	int HayRepeticion(u64 hash); // verifica si esta posicion ya la hemos visto
	void SetHashHistory(u64 hash); // asigna una firma de la posicion
	void PopHistory();

private:
	int ValorMate(int signo);

	void IncNodesQ();
	void IncNodes();
	void ParsePonder(char *pv);
	CJugada BestMoveActual;
public:
	void ResetHistory();
	char ponderMove[8];
	int ValueSearch;
	int PonderMode;

};



extern void Print(char *fmt, ...);
extern void PrintLog(char *fmt, ...);
extern int DontPrintToLog;

#define Max(a,b)  (((a) > (b)) ? (a) : (b))
#define Min(a,b)  (((a) < (b)) ? (a) : (b))

extern u64 rankAttacks(u64 occ, u32 sq);
extern u64 fileAttacks(u64 occ, u32 sq);
extern u64 diagonalAttacks(u64 occ, u32 sq);
extern u64 antiDiagAttacks(u64 occ, u32 sq);
extern u32 bitScanAndReset(u64 &bb);
//extern FORCEINLINE int LastOne(u64 a);
//extern FORCEINLINE int FirstOne(u64 a);
//extern FORCEINLINE u32 popCount (u64 b);
#define m1 ((u64) 0x5555555555555555ull)
#define m2 ((u64) 0x3333333333333333ull)
FORCEINLINE u32 popCount (u64 b) {
/*
	register u32 n;
    u64 a = b - ((b >> 1) & m1);
    u64 c = (a & m2) + ((a >> 2) & m2);
    n = ((u32) c) + ((u32) (c >> 32));
    n = (n & 0x0F0F0F0F) + ((n >> 4) & 0x0F0F0F0F);
    n = (n & 0xFFFF) + (n >> 16);
    n = (n & 0xFF) + (n >> 8);
    return n;
*/
	unsigned int c; // c accumulates the total bits set in b
	for (c = 0; b; c++)
	{
	b &= b - 1; // clear the least significant bit set
	}
	return c;
} 
extern int Log2(int v);
extern int MSB(u64 a);

#define NOEGTB 111000
#if defined (_MSC_VER)

#  undef    TB_CDECL
#  define   TB_CDECL    __cdecl
#  define   TB_FASTCALL __fastcall
#  if _MSC_VER >= 1200
#    define INLINE      __forceinline
#  endif

#else

#  define   TB_CDECL
#  define   TB_FASTCALL
#define INLINE 
#endif

#endif 

const u64 Fila8 = 0xff00000000000000ull;
const u64 Fila7 = 0x00ff000000000000ull;
const u64 Fila6 = 0x0000ff0000000000ull;
const u64 Fila5 = 0x000000ff00000000ull;
const u64 Fila4 = 0x00000000ff000000ull;
const u64 Fila3 = 0x0000000000ff0000ull;
const u64 Fila2 = 0x000000000000ff00ull;
const u64 Fila1 = 0x00000000000000ffull;
const u64 ColumnaA = 0x0101010101010101ull;
const u64 ColumnaB = 0x0202020202020202ull;
const u64 ColumnaC = 0x0404040404040404ull;
const u64 ColumnaD = 0x0808080808080808ull;
const u64 ColumnaE = 0x1010101010101010ull;
const u64 ColumnaF = 0x2020202020202020ull;
const u64 ColumnaG = 0x4040404040404040ull;
const u64 ColumnaH = 0x8080808080808080ull;
const u64 CasillasBlancas = 0x55aa55aa55aa55aaull;
const u64 CasillasNegras = 0xaa55aa55aa55aa55ull;
const u64 Periferia = 0xff818181818181ffull;
const u64 CampoNegro = 0xffffffff00000000ull;
const u64 CampoBlanco = 0xffffffffull;

const u64 ColAH = 0x8181818181818181ull;
const u64 ColBG = 0x4242424242424242ull;
const u64 ColCF = 0x2424242424242424ull;
const u64 ColDE = 0x1818181818181818ull;
const u64 CentroAmpliado = 0x00003c3c3c3c0000ull;

extern CPartida Partida;

#define MAX_TIE 30 

// duo values keeps a midgame and an endgame value in a single
// integer, lower 16 bits are used to store endgame
// next upper 16 bits are used for midgame value. 
inline int Duo(int mg, int eg) { return (mg << 16) + eg; }

// TODO: branchless (lazy : used only one time on each eval).
inline int OppValue(int duo) { int v = duo & 0xffff0000; if(duo & 0x1000) v+= 0x10000;return (v <= 0x10000000) ? (v/ 0x10000) : (v/0x10000) - 0x1000; }
inline int EndValue(int duo) { int v = duo & 0xffff;  return (v <= 0x1000) ? v : v - 0x10000; }


//
// Portabilidad
//
extern long TiempoTranscurrido();
extern void LanzaAnalisis(void *pArg);
extern void CierraThreads();
extern void Espera();
void StartThreads(void *lpParam );
void go(void);
void Stop(void);
void Quit(void);

extern void Print(const char *fmt, ...);
extern void PrintLog(char *fmt, ...);
