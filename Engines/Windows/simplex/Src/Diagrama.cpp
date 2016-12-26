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
	
#include "Ajedrez.h"
#include "zobrist.h"
#include <assert.h>
#include <stdlib.h>
#include <math.h>




#define S_OK 0
#define E_INVALIDARG -1

// matriz de ataque de forma que al hacer
// ataque[casilla][pieza] & BPiezas[pieza][color]
// te indica si una pieza de este color ataca esta casilla
u64 ataque[8][64]; // casilla pieza // distinguiendo peones blancos y negros

CDiagrama::CDiagrama()
{
	// Nombres cortos piezas
	strcpy(Algebra ," PCATDR");
	strcpy(NombreColumnas , "abcdefgh");
	strcpy(NombreFilas , "12345678");

	// carga de los datos desde posicion Inicial
	LoadFEN("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - bm 1;id 1;",0);

	InitAtaque();
}

CDiagrama::~CDiagrama()
{
}

int CDiagrama::board(int t)
{
	return Tablero[t] & 7;
}

int CDiagrama::color(int t)
{
	if(Tablero[t] == 0)
		return neutro;
	return Tablero[t] / 8;
	//u64 mask = BCasilla(t);

	//for(int i = 0; i < 2;i++)
	//for(int j = peon;j <= rey;j++)
	//{
	//	if(BPiezas[j][i]& mask)
	//	{
	//		return i;
	//	}
	//}
	//return neutro;
}

void CDiagrama::InitAtaque()
{
	extern void InitializePST();
	if(ataque[0][0]) // senuelo operacion ya realizada
		return;
	ataque[0][0] = 1;// senuelo operacion ya realizada
	
	InitializePST();

	// ponemos todo a cero

	memset(ataque,0,sizeof(ataque));

	// ataques de peon
	int i,j,f,c;
	for(i = 0; i < 64;i++) // por cada casilla
	{
		// peon blanco
		if(COLUMNA(i) && i > 7) // no estamos en 'a'
		{
			ataque[peon][i] |= ((u64)1 << (i-9));
		}
		if(COLUMNA(i)<7 && i > 7) // no estamos en 'h'
		{
			ataque[peon][i] |= (u64)((u64)1 << (i-7));
		}

		// peon negro
		if(COLUMNA(i) && i < 56) // no estamos en 'a'
		{
			ataque[peonn][i] |= (u64)((u64)1 << (i+7));
		}
		if(COLUMNA(i)<7 && i < 56) // no estamos en 'h'
		{
			ataque[peonn][i] |= (u64)((u64)1 << (i+9));
		}
	}
	// caballos
	for(int i = 0; i < 64;i++)
	{
		j = 0;
		// verificar los ataques de caballo
		f = FILA(i);
		c = COLUMNA(i);
		if(f<=5)
		{
			if(c <= 6)
			{
				ataque[caballo][i] |= (u64)((u64)1 << (i+17));
			}
			if(c >= 1)
			{
				ataque[caballo][i] |= (u64)((u64)1 << (i+15));
			}
		}
		if(f<=6)
		{
			if(c <= 5)
			{
				ataque[caballo][i] |= (u64)((u64)1 << (i+10));
			}
			if(c >= 2)
			{
				ataque[caballo][i] |= (u64)((u64)1 << (i+6));
			}
		}
		
		if(f>=2)
		{
			if(c <= 6)
			{
				ataque[caballo][i] |= (u64)((u64)1 << (i-15));
			}
			if(c >= 1)
			{
				ataque[caballo][i] |= (u64)((u64)1 << (i-17));
			}
		}
		if(f>=1)
		{
			if(c <= 5)
			{
				ataque[caballo][i] |= (u64)((u64)1 << (i-6));
			}
			if(c >= 2)
			{
				ataque[caballo][i] |= (u64)((u64)1 << (i-10));
			}
		}
	}
	// piezas deslizantes
	for(int i = 0; i < 64;i++)
	{
		// FILAS
		if(COLUMNA(i) < 7)
		for(j = i+1;COLUMNA(j) <= 7 && FILA(i) == FILA(j) ;j++)
		{
			ataque[torre][i] |= ((u64)1 << (j));
			ataque[dama][i] |= ((u64)1 << (j));
		}
		if(COLUMNA(i) > 0)
		for(j = i-1;COLUMNA(j) >= 0 && FILA(i) == FILA(j);j--)
		{
			ataque[torre][i] |= ((u64)1 << (j));
			ataque[dama][i] |= ((u64)1 << (j));
		}
		// COLUMNAS
		if(FILA(i) < 7)
		for(j = i+8;FILA(j) <= 7;j+=8)
		{
			ataque[torre][i] |= ((u64)1 << (j));
			ataque[dama][i] |= ((u64)1 << (j));
		}
		if(i> 7)
		for(j = i-8;FILA(j) >= 0 && FILA(j) <= 7;j-=8)
		{
			ataque[torre][i] |= ((u64)1 << (j));
			ataque[dama][i] |= ((u64)1 << (j));
		}
		// DIAGONALES
		if(COLUMNA(i) < 7 && FILA(i) != 7) // no estamos en h
		for(j = i+9;FILA(j) <= 7;j+=9)
		{
			ataque[alfil][i] |= BCasilla(j);//((u64)1 << (j));
			ataque[dama][i] |= ((u64)1 << (j));
			if(COLUMNA(j) == 7)
				break;
		}
		if(COLUMNA(i) > 0 && FILA(i) != 7) // no estamos en a
		for(j = i+7;FILA(j) <= 7;j+=7)
		{
			ataque[alfil][i] |= ((u64)1 << (j));
			ataque[dama][i] |= ((u64)1 << (j));
			if(COLUMNA(j) == 0)
				break;
		}

		if(COLUMNA(i) > 0 && FILA(i) != 0) 
		for(j = i-9;FILA(j) >= 0;j-=9)
		{
			ataque[alfil][i] |= BCasilla(j);//((u64)1 << (j));
			ataque[dama][i] |= ((u64)1 << (j));
			if(COLUMNA(j) == 0)
				break;
		}
		if(COLUMNA(i) < 7 && FILA(i) != 0) // no estamos en a
		for(j = i-7;FILA(j) >= 0;j-=7)
		{
			ataque[alfil][i] |= ((u64)1 << (j));
			ataque[dama][i] |= ((u64)1 << (j));
			if(COLUMNA(j) == 7)
				break;
		}
	}
	// el rey
	for(int i = 0; i < 64;i++)
	{
		if(COLUMNA(i) < 7)
		{
			if(FILA(i) < 7)
				ataque[rey][i] |= ((u64)1 << (i+9));
			ataque[rey][i] |= ((u64)1 << (i+1));
			if(FILA(i) > 0)
				ataque[rey][i] |= ((u64)1 << (i-7));
		}
		if(COLUMNA(i) > 0)
		{
			if(FILA(i) < 7)
				ataque[rey][i] |= ((u64)1 << (i+7));
			ataque[rey][i] |= ((u64)1 << (i-1));
			if(FILA(i) > 0)
				ataque[rey][i] |= ((u64)1 << (i-9));
		}
		if(FILA(i) > 0)
			ataque[rey][i] |= ((u64)1 << (i-8));
		if(FILA(i) < 7)
			ataque[rey][i] |= ((u64)1 << (i+8));
	}
}

void CDiagrama::SwitchColor()
{
   	hash ^= GetZobColor(ColorJuegan); 
   	ColorJuegan ^= 1;
   	hash ^= GetZobColor(ColorJuegan); 
}
u64 CDiagrama::GetHash()
{
	// devolver el hash de esta posicion.
	if(!hash)
	{
    	for(int i=0;i < 64;i++)
    	{
    		if(board(i))
    		{
                hash ^= GetZobKey(i,color(i),board(i));
    		}
    	}
    	hash ^= GetZobColor(ColorJuegan); 
    }
	// casilla de captura al paso
	u64 salida = hash;
	assert(en_pasant < 64);
   	salida ^= GetZobEnPasant(en_pasant);
	// estado del enroque
	if(EstadoEnroque < 0)
		EstadoEnroque = 0;  // TODO ver cuando esto ocurre BUG?
    salida ^= GetZobEnroque(EstadoEnroque);
	salida ^= BOcupadas[blanco] | BOcupadas[negro];
	return salida;
}


/*
* FEN : 6 campos separados por un solo caracter blanco ' '
* CAMPO 1: Posicion del material en el tablero
*	PNBRQK para las blancas
*	pnbrqk para las negras
*	/ indica el cambio de fila
*	1-8 indican las casillas en blanco
*
* CAMPO 2: quien juega 'w' o 'b'
* CAMPO 3: que enroques son validos
*	K ->O-O blanco
*	Q -> O-O-O blanco
*	k -> O-O negro
*	q -> O-O-O negro
*	- -> ninguno
*
* CAMPO 4: Casilla de captura la paso
*	'-' si no hay posibilidad
*
* CAMPO 5: halfmove clock
*	numero de Jugadas desde el ultimo mov.peon para la regla de los 50
* CAMPO 6: Fulmove number.
*
* EJEMPLOS: posicion inicial
rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1
*/
void CDiagrama::LoadFEN(char *fen,int Simetrico)
{
	int casilla;
	en_pasant = 0;
	HashJugada = 0;
	Material[0] = Material[1] = 0;
	memset(MatCount,0,sizeof(MatCount));
	memset(Tablero,0,sizeof(Tablero));
	PstDValue = 0;

	for(casilla = 0; casilla < MAXKILLERS;casilla++)
		Killer[casilla] = 0;
	EstadoEnroque = 0;
	casilla = 56;

	// primero limpiamos el tablero antes de llenarlo.
	memset(BPiezas,0,sizeof(BPiezas));
	memset(BOcupadas,0,sizeof(BOcupadas));
	hash = 0ull;
	// campo 1 colocamos el material
	while(*fen != ' ')
	{
		switch(*fen)
		{
		case '1':
			casilla++;
			break;
		case '2':
			casilla+=2;
			break;
		case '3':
			casilla+=3;
			break;
		case '4':
			casilla+=4;
			break;
		case '5':
			casilla+=5;
			break;
		case '6':
			casilla+=6;
			break;
		case '7':
			casilla+=7;
			break;
		case '8':
			casilla+=8;
			break;
		case '/':
			casilla-= 16;
			break;
		case 'p':
			if(Simetrico)
				PonZ(peon,blanco,casilla ^070);
			else
				PonZ(peon,negro,casilla);
			casilla++;
			break;
		case 'P':
			if(Simetrico)
				PonZ(peon,negro,casilla^070);
			else
				PonZ(peon,blanco,casilla);
			casilla++;
			break;
		case 'n':
			if(Simetrico)
				PonZ(caballo,blanco,casilla^070);
			else
				PonZ(caballo,negro,casilla);
			casilla++;
			break;
		case 'N':
			if(Simetrico)
				PonZ(caballo,negro,casilla^070);
			else
				PonZ(caballo,blanco,casilla);
			casilla++;
			break;
		case 'b':
			if(Simetrico)
				PonZ(alfil,blanco,casilla^070);
			else
				PonZ(alfil,negro,casilla);
			casilla++;
			break;
		case 'B':
			if(Simetrico)
				PonZ(alfil,negro,casilla^070);
			else
				PonZ(alfil,blanco,casilla);
			casilla++;
			break;
		case 'r':
			if(Simetrico)
				PonZ(torre,blanco,casilla^070);
			else
				PonZ(torre,negro,casilla);
			casilla++;
			break;
		case 'R':
			if(Simetrico)
				PonZ(torre,negro,casilla^070);
			else
				PonZ(torre,blanco,casilla);
			casilla++;
			break;
		case 'q':
			if(Simetrico)
				PonZ(dama,blanco,casilla^070);
			else
				PonZ(dama,negro,casilla);
			casilla++;
			break;
		case 'Q':
			if(Simetrico)
				PonZ(dama,negro,casilla^070);
			else
				PonZ(dama,blanco,casilla);
			casilla++;
			break;
		case 'k':
			if(Simetrico)
			{
				PonZ(rey,blanco,casilla^070);
				PosReyes[blanco] = casilla^070;
			}
			else
			{
				PonZ(rey,negro,casilla);
				PosReyes[negro] = casilla;
			}
			casilla++;
			break;
		case 'K':
			if(Simetrico)
			{
				PonZ(rey,negro,casilla^070);
				PosReyes[negro] = casilla^070;
			}
			else
			{
				PonZ(rey,blanco,casilla);
				PosReyes[blanco] = casilla;
			}
			casilla++;
			break;
		}
		fen++;
	}
	fen++;
	// campo 2 quien juega
	if(Simetrico)
	{
		if(*fen=='w')
			ColorJuegan = negro;
		if(*fen=='b')
			ColorJuegan = blanco;
	}
	else
	{
		if(*fen=='w')
			ColorJuegan = blanco;
		if(*fen=='b')
			ColorJuegan = negro;
	}
	fen++; // el b o w
	fen++; // el blanco que le sigue
	// campo 3 status enroque
	if(*fen == '-')
	{
		// no hay enroques como si los reyes se han movido
		EstadoEnroque |= MRB|MRN|MTDB|MTRB|MTDN|MTRN;
	}
	else
	{
		EstadoEnroque |= MRB|MRN|MTDB|MTRB|MTDN|MTRN;
		while(*fen != ' ')
		{
			switch(*fen)
			{
			case 'q':
				if(Simetrico)
					EstadoEnroque &= ~(MRB|MTDB);
				else
					EstadoEnroque &= ~(MRN|MTDN);
				break;
			case 'k':
				if(Simetrico)
					EstadoEnroque &= ~(MRB|MTRB);
				else
				EstadoEnroque &= ~(MRN|MTRN);
				break;
			case 'Q':
				if(Simetrico)
					EstadoEnroque &= ~(MRN|MTDN);
				else
					EstadoEnroque &= ~(MRB|MTDB);
				break;
			case 'K':
				if(Simetrico)
					EstadoEnroque &= ~(MRN|MTRN);
				else
					EstadoEnroque &= ~(MRB|MTRB);
				break;
			}
			fen++;
		}
	}
	fen++; // el blanco que le sigue
	while(*fen == ' ')fen++;
	//campo 4 en_pasant
	if(*fen=='-')
	{
		en_pasant = 0;
	}
	else
	{
		en_pasant = 0;
		en_pasant = (*fen)-'a';
		fen++;
		en_pasant += ((*fen)-'1')*8;
		if(Simetrico)
			en_pasant ^= 070;
	}
#ifdef _DEBUG
	if(en_pasant > 63)
	{
		en_pasant = 0;
	}
#endif
	tope = 0;
	hash = 0;
}


char *CDiagrama::SaveFEN()
{
static	char *material[2][7] = {
		{
			"","P","N","B","R","Q","K"
		},
		{
			"","p","n","b","r","q","k"
		}
	};
	int i= 0,j= 0;
static	char s[250];
	int blancos = 0;
	// blancas
	s[0] = '\0';
	for(j=56;j>=0;j-=8)
	{
		blancos = 0;

		for(i=0;i < 8;i++)
		{
			if(board(i+j) != ninguna)
			{
				if(blancos > 0)
				{
					char tmp[2];
					sprintf(tmp,"%d",blancos);
					strcat(s,tmp);
					blancos = 0;
				}
				if(color(i+j) < neutro)
					strcat(s,material[color(i+j)][board(i+j)]);
			}
			else
			{
				blancos++;
			}
		}
		if(blancos > 0)
		{
			char tmp[3];
			sprintf(tmp,"%d",blancos);
			strcat(s,tmp);
			blancos = 0;
		}
		if((i+j) > 8)
			strcat(s,"/");
	}
	strcat(s," ");
	// segundo campo quien juega
	strcat(s,this->ColorJuegan == blanco? "w ":"b ");
	// tercer campo ... enroques validos
	int se = 0;
	if(!(EstadoEnroque & MRB))
	{
		if(!(EstadoEnroque & MTRB))
		{
			if(board(7) == torre && color(7) == blanco)
			{
				strcat(s,"K");
				se++;
			}
		}
		if(!(EstadoEnroque & MTDB))
		{
			if(board(0) == torre && color(0) == blanco)
			{
			strcat(s,"Q");
			se++;
			}
		}
	}
	if(!(EstadoEnroque & MRN))
	{
		if(!(EstadoEnroque & MTRN))
		{
			if(board(63) == torre && color(63) == negro)
			{
			strcat(s,"k");
			se++;
			}
		}
		if(!(EstadoEnroque & MTDN))
		{
			if(board(56) == torre && color(56) == negro)
			{
			strcat(s,"q");
			se++;
			}
		}
	}
	if(!se)
		strcat(s,"-");
	strcat(s," ");
	// cuarto campo casilla de captura al paso
	if(	en_pasant != 0)
	{
		char casilla[3];
		casilla[0] = (char)('a'+COLUMNA(en_pasant));
		casilla[1] = (char)('1'+FILA(en_pasant));
		casilla[2] = '\0';
		strcat(s,casilla);
	}
	else
	{
		strcat(s,"-");
	}
	strcat(s," ");
	return s;
}

extern char *LogName();
void CDiagrama::Dibuja()
{
	FILE *fd;
	fd = fopen("c:/test/log.txt","a+"); // LogName()
	if(fd)
	{
		Dibuja(fd);
		fclose(fd);
	}
}
void CDiagrama::DibujaP()
{
	Dibuja(stdout);
}
void CDiagrama::Dibuja(FILE *display_file)
{
  char display_board[64][4];
  	strcpy(Algebra ," PCATDR");

  int       i, j;

  for (i = 0; i < 64; i++) {
	  display_board[i][3] = '\0';
		if(color(i) == blanco)
		{
			// encontrado
			display_board[i][0] = '<';
			display_board[i][1] = Algebra[board(i)];
			display_board[i][2] = '>';
		}
		else
		if(color(i) == negro)
		{
			// encontrado
			display_board[i][0] = '-';
			display_board[i][1] = Algebra[board(i)];
			display_board[i][2] = '-';
		}
		else
		{
			display_board[i][0] = ' ';
			display_board[i][1] = COLORCASILLA(i)?'.':' ';
			display_board[i][2] = ' ';
		}

  }
  fprintf(display_file, "\n       +---+---+---+---+---+---+---+---+\n");
  for (i = 7; i >= 0; i--) {
    fprintf(display_file, "   %2d  ", i + 1);
    for (j = 0; j < 8; j++)
      fprintf(display_file, "|%s", display_board[i * 8 + j]);
    fprintf(display_file, "|\n");
    fprintf(display_file, "       +---+---+---+---+---+---+---+---+\n");
  }
  fprintf(display_file, "         a   b   c   d   e   f   g   h\n\n");
}

int CDiagrama::Mueve(int j)
{
	CJugada J;
	J.Set(j);
	return Mueve(J);
}

int CDiagrama::Mueve(const CJugada &j)
{
	int c = ColorJuegan;
	int rival = c ^1;

	int f = j.desglose.f;
	int t = j.desglose.t;
	int captura = j.desglose.captura;
	int coronar = j.desglose.coronar;
	int alpaso = j.desglose.alpaso;
	int Pieza = j.desglose.pieza;

	assert(f >= 0 && f < 64);
	assert(t >= 0 && t < 64);
	assert(ColorJuegan == blanco || ColorJuegan == negro);
	assert(Pieza != ninguna);
	assert(captura != rey);
	assert(board(PosReyes[blanco]) == rey);
	assert(board(PosReyes[negro]) == rey);
	assert(en_pasant < 64);


#ifdef _DEBUG
	if(color(j.desglose.f) != ColorJuegan)
		Print("Color Cambiado\n");

	if(board(PosReyes[blanco]) != rey)
		PrintLog("Error no hay rey\n");
	if(board(PosReyes[negro]) != rey)
	{
		Dibuja();
		Print("PosReyes[negro] = %d\n",PosReyes[negro]);
		Print("J.f %d j.t %d\n",f,t);
		PrintLog("Error no hay rey\n");
	}
	if(Pieza != board(j.desglose.f))
	{
		Print("ERROR EN GENERACION EN MUEVE1\n");
	}
#endif
	// primero quitamos la pieza que se mueve
	QuitaZ(Pieza,c,f);
	if(captura)
	{
		// quitamos la pieza capturada
		if(alpaso)
		{
			if(c == blanco)
				QuitaZ(peon,rival,t-8);
			else
				QuitaZ(peon,rival,t+8);
		}
		else
		{
			assert(captura != rey);
			QuitaZ(captura,rival,t);
		}
	}
	else
	{
		// puede ser un enroque
		if(Pieza == rey)
		{
			if(f == E1 && t == G1)
			{
				// enroque corto blanco
				QuitaZ(torre,blanco,H1);
				PonZ(torre,blanco,F1);
			}
			if(f == E8 && t == G8)
			{
				// enroque corto negro
				QuitaZ(torre,negro,H8);
				PonZ(torre,negro,F8);
			}
			if(f == E1 && t == C1)
			{
				// enroque largo blanco
				QuitaZ(torre,blanco,A1);
				PonZ(torre,blanco,D1);
			}
			if(f == E8 && t == C8)
			{
				// enroque largo negro
				QuitaZ(torre,negro,A8);
				PonZ(torre,negro,D8);
			}
		}
	}
	if(coronar)
		PonZ(coronar,c,t);
	else
		PonZ(Pieza,c,t);

	en_pasant = 0;
	// actualizar estado enroque
	if(Pieza == rey)
	{
		PosReyes[c] = t;
		// ya no se puede enrocar mas
		if(c == blanco)
			EstadoEnroque |= MRB | MTDB | MTRB;
		else
			EstadoEnroque |= MRN | MTDN | MTRN;
	}
	else
	if(Pieza == torre)
	{
		if(f == A1)
			EstadoEnroque |= MTDB;
		if(f == H1)
			EstadoEnroque |= MTRB;
		if(f == A8)
			EstadoEnroque |= MTDN;
		if(f == H8)
			EstadoEnroque |= MTRN;
	}
	else
	if(Pieza == peon)
	{
		if((t-f) == 16)
			en_pasant = f+8;
		if((f-t) == 16)
			en_pasant = f-8;
	}

	SwitchColor();

#ifdef _DEBUG
			if(board(PosReyes[blanco]) != rey)
				PrintLog("Error no hay rey\n");
			if(board(PosReyes[negro]) != rey)
			{
				Dibuja();
				Print("PosReyes[negro] = %d\n",PosReyes[negro]);
				Print("J.f %d j.t %d\n",f,t);
				PrintLog("Error no hay rey\n");
			}
#endif
			assert(board(t) != ninguna);
			assert(board(f) == ninguna);
	assert(en_pasant < 64);

	assert(board(PosReyes[blanco]) == rey);
	assert(board(PosReyes[negro]) == rey);

	return 1;
}

void CDiagrama::DesMueve(int j)
{
	CJugada J;
	J.Set(j);
	DesMueve(J);
	assert(en_pasant < 64);
}

void CDiagrama::DesMueve(const CJugada &j)
{
	int f,t,c,pieza,rival,captura,coronar,alpaso;
	f = j.desglose.f;
	t = j.desglose.t;
	rival = ColorJuegan;
	c = ColorJuegan^1;
	captura = j.desglose.captura;
	coronar = j.desglose.coronar;
	alpaso = j.desglose.alpaso ;
	pieza = j.desglose.pieza;


	assert(en_pasant < 64);
	assert(f >= 0 && f < 64);
	assert(t >= 0 && t < 64);
	assert(j.desglose.captura != rey);

#ifdef _DEBUG
	if(color(j.desglose.t) == ColorJuegan)
	{
		Dibuja(); // deshacemos una jugada ilegal
		Print("Desmueve ColorCambiado Jugada %s\n",j.ToString());
	}

	if(board(j.desglose.t) == ninguna)
	{
		Dibuja();
		Print("Desmueve Jugada %s\n",j.ToString());
		return;
	}
#endif

	// quitamos la pieza que tenemos en destino
	if(coronar)
	{
		QuitaZ(coronar,c,t);
	}
	else
	{
		QuitaZ(pieza,c,t);
	}
	// ponemos la pieza capturada si toca
	if(captura)
	{
		if(alpaso)
		{
			if(c == blanco)
				PonZ(peon,negro,t-8);
			else
				PonZ(peon,blanco,t+8);
		}
		else
		{
			PonZ(captura,rival,t);
		}
	}
	else
	{
		// verificamos posibles enroques
		if(pieza == rey)
		{
			if(f == E1 && t == G1)
			{
				// enroque corto blanco
				PonZ(torre,blanco,H1);
				QuitaZ(torre,blanco,F1);
			}
			if(f == E8 && t == G8)
			{
				// enroque corto negro
				PonZ(torre,negro,H8);
				QuitaZ(torre,negro,F8);
			}
			if(f == E1 && t == C1)
			{
				// enroque largo blanco
				PonZ(torre,blanco,A1);
				QuitaZ(torre,blanco,D1);
			}
			if(f == E8 && t == C8)
			{
				// enroque largo negro
				PonZ(torre,negro,A8);
				QuitaZ(torre,negro,D8);
			}
		}
	}
	// colocamos la pieza en su origen
	PonZ(pieza,c,f);

	SwitchColor();

	if(pieza == rey)
		PosReyes[c] = f;

	assert(board(PosReyes[blanco]) == rey);
	assert(board(PosReyes[negro]) == rey);
	assert(en_pasant < 64);

}


void CDiagrama::CalculaJugadasPosibles()
{
	int i;
	tope = 0;
	CSort Sort;
	// de lo contrario la hashjugada puede ser basura
	HashJugada = 0;
	for(i = 0; i < MAXKILLERS;i++)
		Killer[i] = 0;

	Sort.Init((*this),false);
	CJugada J;
	for(i=0,J = Sort.GetNext();J.ToInt();J = Sort.GetNext(),i++)
	{
		JugadasPosibles[i].Set(J.ToInt());

	}
	tope = i;
}
int CDiagrama::EsAtacada(int casilla,int bando)
{
	// determina si el bando contrario al 'bando' ataca la casilla definida
	u64 AtC = 0;
	int c = bando^1;
	// ahora piezas deslizantes
	// hay que tener en cuenta las sombras y tal
	u64 ocupa;
	u64 aux;
	ocupa = BOcupadas[blanco] |BOcupadas[negro];
	aux = BPiezas[dama][c] | BPiezas[torre][c];
	if(aux)
	{
		if(ataque[torre][casilla] & aux) // Hay Candidatos
		{
			if(rankAttacks(ocupa,casilla) & aux) 
				return 1;
			if(fileAttacks(ocupa,casilla) & aux)
					return 1;
		}
	}
	// ahora lo mismo con las diagonales
	aux = BPiezas[dama][c] | BPiezas[alfil][c];
	if(aux)
	{

		if(ataque[alfil][casilla] & aux) // Hay Candidatos
		{
			if(diagonalAttacks(ocupa,casilla) & aux)
				return 1;
			if(antiDiagAttacks(ocupa,casilla) & aux)
				return 1;
		}
	}
	if(bando == blanco)
	{
		AtC = ataque[peonn][casilla] & BPiezas[peon][negro];
	}
	else
	{
		AtC = ataque[peon][casilla] & BPiezas[peon][blanco];
	}
	if(AtC)
		return 1;

	AtC = ataque[caballo][casilla] & BPiezas[caballo][c];
	if(AtC)
		return 1;
	AtC = ataque[rey][casilla] & BPiezas[rey][c];
	if(AtC)
		return 1;
	return 0;
}

int CDiagrama::EstoyEnJaque()
{
	// ver si esta atacado
	return EsAtacada(PosReyes[ColorJuegan],ColorJuegan);
}


int CDiagrama::Evalua(int alpha,int beta)
{
//	return Bev.Eval(this);
	return Eval(alpha,beta);
}
int CDiagrama::MuevePgn(char *JugAlg)
{
	int i = IdentificaPgn(JugAlg);
	if(i)
	{
		Mueve(JugadasPosibles[i-1]);
		CalculaJugadasPosibles();
	}
	return i;
}
int CDiagrama::EsLegal(CJugada &J)
{
	// salvamos el estado
	int alpaso = en_pasant ;
	int StatusEnroque = EstadoEnroque ;
	int yo = ColorJuegan;
	int esvalido = 0;
	Mueve(J);
	if(!EsAtacada(PosReyes[yo],yo))
		esvalido = 1;

	DesMueve(J);
	// restaurar estatus...
	en_pasant = alpaso;
	EstadoEnroque = StatusEnroque;

	return esvalido;
}

int CDiagrama::IdentificaPgn(char *JugAlg1)
{
	char JugAlg[15];
	char p = ninguna;
	unsigned int fc= 0xffff,ff=0xffff;
	unsigned int corona = ninguna;

	int i;
	unsigned int from = 0xffff,to= 0xffff;

	strcpy(JugAlg,JugAlg1);
	for(i=3 ; i < 7;i++)
	{
		if(JugAlg[i] == '+' || JugAlg[i] == '#')
		{
			JugAlg[i] = '\0';
		}
		if(JugAlg[i] == '\0')
			break;
	}

	if((JugAlg[0]>= 'a')&& JugAlg[0] <= 'z')
	{
		// movimiento de peon
		p = peon;
		if(JugAlg[1] == 'x') // captura
		{
			fc = JugAlg[0] -'a';
			to = ((JugAlg[3] - '1') << 3) + JugAlg[2] - 'a';
			if(JugAlg[4] == '=')
			{	// corona
				switch(JugAlg[5])
				{
				case 'B':
				case 'b':
					corona = alfil;
					break;
				case 'N':
				case 'n':
					corona = caballo;
					break;
				case 'R':
				case 'r':
					corona = torre;
					break;
				case 'Q':
				case 'q':
					corona = dama;
					break;
				}
			}
		}
		else
		{
			if(strlen(JugAlg) == 4)
			{
				if(JugAlg[2] == '=') // a1=q a1=Q ...
				{		
					to = ((JugAlg[1] - '1') << 3) + JugAlg[0] - 'a';
					switch(JugAlg[3])
					{
					case 'B':
					case 'b':
						corona = alfil;
						break;
					case 'N':
					case 'n':
						corona = caballo;
						break;
					case 'R':
					case 'r':
						corona = torre;
						break;
					case 'Q':
					case 'q':
						corona = dama;
						break;
					}
				}
				else
				{
					from = ((JugAlg[1] - '1') << 3) + JugAlg[0] - 'a';
					to = ((JugAlg[3] - '1') << 3) + JugAlg[2] - 'a';
				}
			}
			else
				if(strlen(JugAlg) == 5 && JugAlg[2] == 'x')
			{
				from = ((JugAlg[1] - '1') << 3) + JugAlg[0] - 'a';
				to = ((JugAlg[4] - '1') << 3) + JugAlg[3] - 'a';
			}
				else
				if(strlen(JugAlg) == 5)
			{
				from = ((JugAlg[1] - '1') << 3) + JugAlg[0] - 'a';
				to = ((JugAlg[3] - '1') << 3) + JugAlg[2] - 'a';
				switch(JugAlg[4])
				{
				case 'B':
				case 'b':
					corona = alfil;
					break;
				case 'N':
				case 'n':
					corona = caballo;
					break;
				case 'R':
				case 'r':
					corona = torre;
					break;
				case 'Q':
				case 'q':
					corona = dama;
					break;
				}
			}
				else
				if(strlen(JugAlg) == 6 && JugAlg[2] == 'x')
			{
				from = ((JugAlg[1] - '1') << 3) + JugAlg[0] - 'a';
				to = ((JugAlg[5] - '1') << 3) + JugAlg[4] - 'a';
				switch(JugAlg[5])
				{
				case 'B':
				case 'b':
					corona = alfil;
					break;
				case 'N':
				case 'n':
					corona = caballo;
					break;
				case 'R':
				case 'r':
					corona = torre;
					break;
				case 'Q':
				case 'q':
					corona = dama;
					break;
				}
			}

			else
			{
				//
				// PENDIENTE a6a7=Q strlen == (6)
				//
				to = ((JugAlg[1] - '1') << 3) + JugAlg[0] - 'a';
				if(JugAlg[2] == '=')
				{	// corona
					switch(JugAlg[3])
					{
					case 'B':
					case 'b':
						corona = alfil;
						break;
					case 'N':
					case 'n':
						corona = caballo;
						break;
					case 'R':
					case 'r':
						corona = torre;
						break;
					case 'Q':
					case 'q':
						corona = dama;
						break;
					}
				}
			}
		}
	}
	else
	{
		from = 0xffff;
		to = 0xffff;
		// movimiento de piezas
		if(JugAlg[0] == 'O' || JugAlg[0] == '0') // enroques
		{
			p = rey; // enroques
			if(strcmp(JugAlg,"O-O") == 0 || strcmp(JugAlg,"0-0") == 0 ||strcmp(JugAlg,"00")==0 ||strcmp(JugAlg,"OO")==0 )
			{
				if(ColorJuegan == blanco)
				{
					from = 4;
					to = 6;
				}
				else
				{
					from = (7 <<3) + 4;
					to = (7<< 3)+6;
				}
			}
			if(strcmp(JugAlg,"O-O-O") == 0 || strcmp(JugAlg,"0-0-0") == 0 || strcmp(JugAlg,"000") == 0 || strcmp(JugAlg,"OOO") == 0 )
			{
				if(ColorJuegan == blanco)
				{
					from = 4;
					to = 2;
				}
				else
				{
					from = (7 <<3) + 4;
					to = (7<< 3)+2;
				}
			}
		}
		else
		{
			switch(JugAlg[0])
			{
			case 'B':
				p = alfil;
				break;
			case 'N':
				p = caballo;
				break;
			case 'R':
				p = torre;
				break;
			case 'Q':
				p = dama;
				break;
			case 'K':
				p = rey;
				break;
			}

			if(strlen(JugAlg) == 3)
			{
				// jugada simple Xa1
				to = ((JugAlg[2] - '1') << 3) + JugAlg[1] - 'a';
			}
			else
			{
				if(strlen(JugAlg) == 4) // Cxa6 C3b5 Ccb5
				{
					to = ((JugAlg[3] - '1') << 3) + JugAlg[2] - 'a';
					if(JugAlg[1] != 'x') // no es captura simple
					{
						if((JugAlg[1]>='0')&& (JugAlg[1]<='9'))
						{
							ff = JugAlg[1] -'1';
						}
						else
						{
							fc = JugAlg[1] -'a';
						}
					}
				}
				else
				{
					// Ccxb5 C3xb5 Cc6d4
					if(strlen(JugAlg) == 5)
					{
						if(JugAlg[2] == 'x')
						{
							to = ((JugAlg[4] - '1') << 3) + JugAlg[3] - 'a';
							if((JugAlg[1] >= '0')&& (JugAlg[1] <='9'))
							{
								ff = JugAlg[1] -'1';
							}
							else
							{
								fc = JugAlg[1] -'a';
							}
						}
						else
						{
							to = ((JugAlg[4] - '1') << 3) + JugAlg[3] - 'a';
							ff = JugAlg[2] -'1';
							fc = JugAlg[1] -'a';
						}
					}
					else
					if(strlen(JugAlg) == 6) // Nc6xd4
					{
						if(JugAlg[3] == 'x')
						{
							to = ((JugAlg[5] - '1') << 3) + JugAlg[4] - 'a';
							ff = JugAlg[2] -'1';
							fc = JugAlg[1] -'a';
						}
					}
				}
			}
		}
	}

	// si era algebraico corregir posible error
	if(p == peon)
	{
		if(from < 64)
			p = (char)(board(from));
		else
			from = from;
	}
	// buscar en la lista de jugadas
	// buscar la jugada
	for(i=0;i < tope;i++)
	{

		if(JugadasPosibles[i].desglose.t == to)
		{
			if(board(JugadasPosibles[i].desglose.f) == p)
			{
				if(ff < 64)
				{
					if(FILA(JugadasPosibles[i].desglose.f) == ff)
					{
						// encontrado
						if(EsLegal(JugadasPosibles[i]))
							return(i+1);
					}
				}
				else
				if(fc < 64)
				{
					if(COLUMNA(JugadasPosibles[i].desglose.f) == fc && JugadasPosibles[i].desglose.coronar == corona )
					{
						// encontrado
						if(EsLegal(JugadasPosibles[i]))
							return(i+1);
					}
				}
				else
				{
					if((from < 64) && from == JugadasPosibles[i].desglose.f)
					{
						if(p != peon || corona == ninguna || corona == JugadasPosibles[i].desglose.coronar)
							if(EsLegal(JugadasPosibles[i]))
							return(i+1);
					}
					else
					{
						if(p != peon || corona == ninguna || corona == JugadasPosibles[i].desglose.coronar)
							if(EsLegal(JugadasPosibles[i]))
							return(i+1);
					}
						
				}
			}
		}
	}	
	// si no devolver Error
	return 0;
}

//
// Mover la jugada algebraica indicada.
HRESULT CDiagrama::MueveAlgebra(char *JugAlg)
{
	int i;
	unsigned int from,to;
	char promo;
	unsigned int promop;
	from = JugAlg[0] - 'a';
	from += (JugAlg[1] - '1') <<3;

	to = ((JugAlg[3] - '1') << 3) + JugAlg[2] - 'a';
	promo = JugAlg[4];
	switch(promo)
	{
	case 'q':
		promop = dama;
		break;
	case 'r':
		promop = torre;
		break;
	case 'n':
		promop = caballo;
		break;
	case 'b':
		promop = alfil;
		break;
	default:
		promop = ninguna;
		break;
	}
	// buscar en la lista de jugadas
	// buscar la jugada
	for(i=0;i < tope;i++)
	{

		if(JugadasPosibles[i].desglose.f  == from && JugadasPosibles[i].desglose.t == to 
			&& (promop == ninguna || promop ==  JugadasPosibles[i].desglose.coronar )
			)
		{
			// si encontrada
			// mover y devolver S_OK
			Mueve(JugadasPosibles[i]);
			CalculaJugadasPosibles();
			return(S_OK);
		}
	}	
	// si no devolver Error
	return(E_INVALIDARG);

}


int CDiagrama::Es7a(CJugada &J)
{
	if(FILA(J.desglose.t) == 6)
	{
		if(color(J.desglose.f) == blanco && board(J.desglose.f) == peon)
			return 1;
	}
	else if(FILA(J.desglose.t) == 1)
	{
		if(color(J.desglose.f) == negro && board(J.desglose.f) == peon)
			return 1;
	}
	return 0;
}




// Matriz de pesos para las jugadas from to
// busca la jugada hash y si la encuentra la deja en la posicion 0 del array
int CDiagrama::GetHashMove(unsigned int *Jugadas,int tope)
{
	register int i;
	register unsigned int Hash = HashJugada & 0x0000ffff;
	if(Hash == 0)
		return 1;
	for(i = 0; i < tope;i++)
	{
		if((Jugadas[i] & 0x0000ffff) == Hash)
		{
			// intercambiar y salir
			Hash = Jugadas[0];
			Jugadas[0] = Jugadas[i];
			Jugadas[i] = Hash;
			Jugadas[0] |= PesoHashJugada << 20;
			return 0;
		}
	}
	return 1;
}

int CDiagrama::TotalPieces(int Color)
{
	int i;
	int cuenta = 0;
	for(i=caballo;i < rey;i++)
	{
		cuenta += MatCount[Color][i];
	}
	return cuenta;
}

int CDiagrama::TotalPCtm()
{
	return TotalPieces(ColorJuegan);
}
