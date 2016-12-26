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

// Uci.cpp: implementation of the CUci class.
//
//////////////////////////////////////////////////////////////////////

#include "Uci.h"


#ifdef DEVELOPMENT
#include "../DevSrc/Test.h"
#endif

#include <stdio.h>
#include <string.h>

#include <stdlib.h>
#include <stdarg.h>
#include <ctype.h>

#include "Ajedrez.h"

#ifdef DEVELOPMENT
#include "../DevSrc/PgnParse.h"
#include "../DevSrc/PgnRunner.h"
#include "../DevSrc/Pgn2Epd.h"
#include "../DevSrc/tune.h"
#include "../DevSrc/Pgn2Cmd.h"
#include "../DevSrc/PerftEpd.h"
#include "Parameters.h"
#include "../DevSrc/TrainEpd.h"
#endif

#define E_INVALIDARG	-1
extern void Print(const char *fmt, ...);


extern CPartida Partida;


//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////
static const int SizeOfInputBuffer = 0x8000;
CUci::CUci()
{
	initDone = 0;
	InitG = 1;
}

CUci::~CUci()
{
}

/**********************************************************************
** Salida hacia la consola y el log
*/

int UseLog = 0;

int DontPrintToLog = 0;

char *LogName()
{
	static char nombre[1024];
	if(nombre[0] == '\0')
	{
		char base[15];
		strcpy(nombre,".");
		strcat(nombre,"\\");
		strcpy(base,"log");
		strcat(nombre,base);
		strcat(nombre,".txt");
	}
	return nombre;
}
void Print(const char *fmt, ...)
{
  va_list   ap;
  FILE *fd;

  va_start(ap, fmt);
  vprintf(fmt, ap);
  fflush(stdout);
  if(DontPrintToLog == 0 && UseLog)
  {
	fd = fopen("c:/test/log.txt" ,"a+"); 
	  if(fd)
	  {
		vfprintf(fd, fmt, ap);
		fclose(fd);
	  }
  }
  va_end(ap);
}
void PrintLog(char *fmt, ...)
{
  va_list   ap;
  FILE *fd;

  if(!UseLog)
	  return;
  va_start(ap, fmt);
  fd = fopen("c:/test/log.txt","a+");
  if(fd)
  {
	vfprintf(fd, fmt, ap);
	fclose(fd);
  }
  va_end(ap);
}

void CUci::start()
{
	extern int UseLog;
	char *token;
	// default values
	memset(InputBuffer,0,SizeOfInputBuffer);
	// bucle principal
	while(gets(InputBuffer))
	{
		PrintLog("%s\n",InputBuffer);
		NewBuffer = 1;
		while((token = GetNextToken()) != NULL )
		{
			if(strcmp(token,"uci")==0)
			{
				Print("id name Simplex 0.9.8\nid author Antonio Torrecillas\n");
//	   "option name Nullmove type check default true\n"
//      "option name Selectivity type spin default 2 min 0 max 4\n"
//	   "option name Style type combo default Normal var Solid var Normal var Risky\n"
//	   "option name NalimovPath type string default c:\\n"
//	   "option name Clear Hash type button\n"
				Print("option name Hash type spin default 32 min 4 max 8192\n");
				Print("option name Clear Hash type button\n");
				Print("option name Ponder type check default false\n");

//				Print("option name param type string\n");
				Print("uciok\n");
			}
			else
			if(strcmp(token,"quit")==0)	{
				Partida.Cancela();
				exit(0);
			}
			else
			if(strcmp(token,"debug")==0)	{		Debug();break;	}
			else
			if(strcmp(token,"isready")==0)	{
				if(initDone)
				{
					if(InitG)	Print("readyok\n");
				}
				else
				{
					initDone = 1;
					Print("readyok\n");
				}

			}
			else 
			if(strcmp(token,"setoption")==0)	{		SetOption();	} 
			else
			if(strcmp(token,"ucinewgame")==0)	{		UciNewG();		} 
			else
			if(strcmp(token,"position")==0)		{		Position();		} 
			else
			if(strcmp(token,"go")==0)			{		Go();			} 
			else
			if(strcmp(token,"stop")==0)			{		Stop();			} 
			else
			if(strcmp(token,"ponderhit")==0)	{
				// switch to normal mode.
				Partida.PonderMode = 0;
			} 
			else
			{
				if(token && *token != '\0')
				Print("info string token <%s>\n",token);
			}
		}
		memset(InputBuffer,0,SizeOfInputBuffer);
	}
}

static char res[1024];

char * CUci::GetNextToken()
{
static char *aux = NULL;
char *pres;
char car;
	if(aux == NULL)
		aux = InputBuffer;
	if(NewBuffer)
	{
		NewBuffer = 0;
		aux = InputBuffer;
	}
	if(!(*aux))
		return NULL;
	pres = &res[0];
	while((car = *aux) != '\0')
	{
		if(car == ' ' || car == '\t' || car == '\n' || car == '\r' || car == '\0')
			break;
		*pres++ = car;
		aux++;
	}

	if(*aux)
		aux++;
	*pres = '\0';
	return &res[0];
}

void CUci::Debug()
{
	// on off
	char *token ; 
	token = GetNextToken();
	if(strcmp(token,"on")==0)	{		Print("info string debug mode on\n");	}
	else
	if(strcmp(token,"off")==0)	{		Print("info string debug mode off\n");	}
#ifdef DEVELOPMENT
	else
	if(strcmp(token,"test")==0)
	{
		Print("info string debug test on\n");
		CTest t;
		t.Test();
		Print("info string debug test off\n");
	}
	else
	if(strcmp(token,"kiwipete")==0)
	{
		Partida.LoadEPD("r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1 ");
		Print("info string kiwipete position loaded\n");
	}
	else
	if(strcmp(token,"estudia")==0)
	{

		CTest t;
		char * path = GetNextToken();
		t.Estudia(path);
	}
	else
	if(strcmp(token,"perftepd")==0)
	{
		PerftEpd t;
		char * path = GetNextToken();
		t.epdfile = path;
		t.Start();
	}
	else
	if(strcmp(token,"testepd")==0)
	{
		int depth_time;
		char * path = GetNextToken();
		if(path)
		{
			depth_time = atoi(path);
			path = GetNextToken();
			if(path)
			{
				char local[200];
				strcpy(local,path);
				CTest t;
				t.TestEpd(depth_time,local);
			}
			else
			{
				Print("info string Sintax: debug testepd depth(0-50) file.pgn\n");
				Print("info string Sintax: debug testepd time(miliseg) file.pgn\n");
			}
		}
		else
		{
				Print("info string Sintax: debug testepd depth(0-50) file.pgn\n");
				Print("info string Sintax: debug testepd time(miliseg) file.pgn\n");
		}
	}
	else
	if(strcmp(token,"t")==0)
	{
		char local[200];
		strcpy(local,"twic663.epd");
		CTest t;
		t.TestEpd(4,local);
	}
	else
	if(strcmp(token,"perft")==0)
	{
	   int d = 0;
		token = GetNextToken();
		if(token)
		{
			d = atoi(token);
			CTest t;
		   t.DoPerft(d);
		}
		else
		{
			Print("info string Sintax: debug perft depth\n");
		}
	}
	else
	if(strcmp(token,"display")==0)
	{
	   Partida.T.DibujaP();
	   Print("%s\n",Partida.T.SaveFEN());
	}
	else
	if(strcmp(token,"score")==0)
	{
		CSort Sort;
		Sort.Init(Partida.T,3);
		Print("%s;%d\n",Partida.T.SaveFEN(),Partida.T.Evalua(-99999,99999));
		printf("pst:\t\t%d\t%d \n", OppValue(Partida.T.PstDValue),EndValue(Partida.T.PstDValue));
		printf("Material:\t%d\t%d\n",Partida.T.Material[0],Partida.T.Material[1] );
		printf("Peones:\t\t%d\t%d\n",Partida.T.ValorPeones[0],Partida.T.ValorPeones[1] );
		printf("Caballos:\t%d\t%d\n",Partida.T.EvalCaballos[0],Partida.T.EvalCaballos[1] );
		printf("Alfiles:\t%d\t%d\n",Partida.T.EvalAlfiles[0],Partida.T.EvalAlfiles[1]  );
		printf("Torres:\t\t%d\t%d\n",Partida.T.EvalTorres[0],Partida.T.EvalTorres[1] );
		printf("Damas:\t\t%d\t%d\n",Partida.T.EvalDamas[0],Partida.T.EvalDamas[1] );
		printf("Cob Peones:\t%d\t%d\n",Partida.T.PawnCover[0],Partida.T.PawnCover[1] );
		printf("Total:\t\t%d\t%d\n",Partida.T.Opening,Partida.T.EndGame);
		
    }
	else
	if(strcmp(token,"split")==0)
	{
		token = GetNextToken();
		if(token)
		{
			CTest t;
			t.Split(token);
		}
	}
	else
	if(strcmp(token,"suite")==0)
	{
		CTest t;
		t.TestSuite();
	}
	else
	if(strcmp(token,"sts")==0)
	{
		CTest t;
		t.STS(100);
	}
	else
	if(strcmp(token,"sts1")==0)
	{
		CTest t;
		t.STS(1000);
	}
	else
	if(strcmp(token,"sts10")==0)
	{
		CTest t;
		t.STS10();
	}
	else
	if(strcmp(token,"simetria")==0)
	{
		token = GetNextToken();
		if(token)
		{
			CTest t;
			t.TestSimetria(token);
		}
	}
	else
	if(strcmp(token,"eval")==0)
	{
		extern void epdeval();
		epdeval();
	}
	else
	if(strcmp(token,"pgn2epd")==0)
	{
		token = GetNextToken();
		Pgn2Epd t;
		t.Parsea(token);
	}
	else
	if(strcmp(token,"train")==0)
	{
		token = GetNextToken();
		TrainEpd t;
		t.Start();
	}
	else
	if(strcmp(token,"stat")==0)
	{
		token = GetNextToken();
		TrainEpd t;
		t.Statistics();
	}
	else
	if(strcmp(token,"pgn2cmd")==0)
	{
		token = GetNextToken();
		Pgn2Cmd t;
		t.Parsea(token);
	}
	else // Branch Factor
	if(strcmp(token,"bf")==0)
	{
		Partida.DumpBranchFactor();
	}
#endif
}
void ToLowerCase(char *cadena)
{
	while(*cadena)
	{
		*cadena = tolower(*cadena);
		cadena++;
	}
}
void CUci::SetOption()
{
	char name[128];
	char value[128];
	strcpy(name,GetNextToken());
	if(strcmp(name,"name") != 0) return;
//	   "setoption name Nullmove value true\n"
//      "setoption name Selectivity value 3\n"
//	   "setoption name Style value Risky\n"
//	   "setoption name Clear Hash\n"
//	   "setoption name NalimovPath value c:\chess\tb\4;c:\chess\tb\5\n"
	strcpy(name,GetNextToken());
	strcpy(value,GetNextToken());
	ToLowerCase(name);
	ToLowerCase(value);
	if(strcmp(value,"value")==0)
	{
		strcpy(value,GetNextToken());
	}
	if(strcmp(name,"clear")==0)
	{
		if(strcmp(value,"hash")==0)
		{
			Partida.HashJ.Inicializa();
			return;
		}
	}
	if(strcmp(name,"hash")==0)
	{
		int Mb = atoi(value);
		Partida.HashJ.SetHashSize(Mb);
	}
	//if(_stricmp(name,"ponder")==0)
	//{
	//	parece razonable hacer algo con esto pero...el estandar dice que este flag sirve para gestionar el tiempo
	//  y nada más. Yo eso lo hago en función del comando go..ponder...
	//  tal vez solo sea un indicador para el GUI de que el motor soporta el modo.
	//}
}

void CUci::UciNewG()
{
	InitG = 0;
	Partida.Nueva();
	// to do new g....
	Partida.HashJ.Inicializa();
	InitG = 1;
}

// position [fen <fenstring> | startpos ]  moves <move1> .... <movei>

void CUci::Position()
{
	static char *startpos = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq -";
	char *token = GetNextToken();
	char fen[80];
	if(strcmp(token,"startpos")==0)
	{
		strcpy(fen,startpos);
		token = GetNextToken(); // 
	}
	else
	{	if(strcmp(token,"fen")==0)
		{
			fen[0] = '\0';
			token = GetNextToken();
			if(token)
			strcpy(fen,token);
			token = GetNextToken(); // w b
			strcat(fen," ");
			if(token)
			strcat(fen,token);
			token = GetNextToken(); // casting
			strcat(fen," ");
			if(token)
			strcat(fen,token);
			token = GetNextToken(); // e.p.
			strcat(fen," ");
			if(token)
			strcat(fen,token);
			token = GetNextToken(); // 
			strcat(fen," ");
			if(token)
			strcat(fen,token);
			token = GetNextToken(); // 
			strcat(fen," ");
			if(token)
			strcat(fen,token);
			token = GetNextToken(); // 

		}
		else
		{
			strcpy(fen,Partida.T.SaveFEN());
		}
	}
	// init to fen pos.

	Partida.Nueva();
	Partida.LoadEPD((char*)fen);

	// get moves

	if(!token)
		token = GetNextToken();
	if(token && strcmp(token,"moves")==0)
	{
		token = GetNextToken();
		while(token)
		{
			// process move
		   if(Partida.Mueve(token)== E_INVALIDARG)
		   {
			   PrintLog("Error en Movimiento %s\n",token);
		   }
			// get next move
			token = GetNextToken();
		}
	}
	Partida.Taux.LoadFEN(Partida.T.SaveFEN(),0);
}

void CUci::Go()
{
	int wTime,bTime,winc,binc,movestogo,depth,nodes,mate,movetime;
	char movesList[1024];
	char *token = GetNextToken();
	wTime = bTime = winc = binc = movestogo = depth = nodes = mate = movetime = 0;
	while(token)
	{
		if(strcmp(token,"infinite") == 0)
		{
			// to do no depth no time limit go
		}
		else
		if(strcmp(token,"searchmoves") == 0)
		{
			token = GetNextToken();
			while(token)
			{
				strcat(movesList,token);
				strcat(movesList," ");
				token = GetNextToken();
			}
		}
		else
		if(strcmp(token,"ponder") == 0)
		{
			Partida.PonderMode = 1;
		}
		else
		if(strcmp(token,"wtime") == 0)
		{
			token = GetNextToken();
			wTime = atoi(token);
		}	
		else
		if(strcmp(token,"btime") == 0)
		{
			token = GetNextToken();
			bTime = atoi(token);
		}
		else
		if(strcmp(token,"winc") == 0)
		{
			token = GetNextToken();
			winc = atoi(token);
		}
		else
		if(strcmp(token,"binc") == 0)
		{
			token = GetNextToken();
			binc = atoi(token);
		}
		else
		if(strcmp(token,"movestogo") == 0)
		{
			token = GetNextToken();
			movestogo = atoi(token);
		}
		else
		if(strcmp(token,"depth") == 0)
		{
			token = GetNextToken();
			depth = atoi(token);
		}
		else
		if(strcmp(token,"nodes") == 0)
		{
			token = GetNextToken();
			nodes = atoi(token);
		}
		else
		if(strcmp(token,"mate") == 0)
		{
			token = GetNextToken();
			mate = atoi(token);
		}
		else
		if(strcmp(token,"movetime") == 0)
		{
			token = GetNextToken();
			movetime = atoi(token);
		}
		token = GetNextToken();
	}
	// execute go
   // depth limit
	Partida.LimiteProfundidad = 0;
	Partida.tiempo= 0;
	Partida.incremento= 0;

   if (depth >= 0) {
	   Partida.LimiteProfundidad = depth;
   } else if (mate >= 0) {
	   Partida.LimiteProfundidad = mate*2-1; // conversion de jugadas a movimientos
   }

   // time limit
	if(Partida.ColorJuegan() == blanco)
	{
		Partida.tiempo= (long)wTime;
		Partida.incremento= (long)winc;
	}
	else
	{
		Partida.tiempo=(long) bTime;
		Partida.incremento=(long) binc;
	}

	if(Partida.incremento < 0)
		Partida.incremento = 0;
   if (movestogo <= 0 || movestogo > 30) movestogo = 30; // HACK

   if (movetime > 0) {

      // fixed time
	  Partida.tiempo_limite =(long) movetime;

   } 
   else 
   {

      // dynamic allocation
      double time_max = Partida.tiempo * 0.95 - 1.0;
      if (time_max < 0.0) time_max = 0.0;

      double alloc = (time_max + Partida.incremento * double(movestogo-1)) / double(movestogo);
	  // si estamos en ponder mode podemos alargar más
	  // no tengo claro si se debe tomar del comando go o del setoption ponder...
	  if(Partida.PonderMode) alloc *= 1.4;

      if (alloc > time_max) alloc = time_max;
	  Partida.tiempo_limite =(long) alloc;
   }

   // lanzar el analisis
   	Partida.Cancela(); // cancel a previous search ?
   	Partida.Analiza();

}

void CUci::Stop()
{
	// la impresion de la jugada se realiza en Partida.IterativeDeepening();
	Partida.PonderMode = 0;
	Partida.Cancela();
}
