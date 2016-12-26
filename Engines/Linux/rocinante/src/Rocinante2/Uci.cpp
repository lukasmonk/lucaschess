#include "CDebug.h"
#include "MoveList.h"
#include "UndoData.h"
#include "Board.h"

#include "TreeNode.h"
#include "BStar.h"
#include "DumpTree.h"
#include "MCTS_AB.h"

#include "Uci.h"

#include "C3FoldRep.h"

#include "SmpManager.h"


#include <stdio.h>
#include <string.h>

#include <stdlib.h>
#include <stdarg.h>
#include "system.h"


#include "epd.h"

#define E_INVALIDARG	-1
extern void Print(const char *fmt, ...);

#ifndef _MSC_VER
#define stricmp strcasecmp
#endif

Board board;
struct DataRunBStar RunData;

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////
static const int SizeOfInputBuffer = 0x8000;
CUci::CUci()
{
	initDone = 0;
	InitG = 1;
	UseMCTS_AB = true; //true;
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
	fd = fopen("c:/test/log.txt" ,"a+"); // LogName()
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
//	fd = fopen(LogName(),"a+");
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

//	UseLog = 1;
	// bucle principal
	while(gets(InputBuffer))
	{
		if(false)
		{
			FILE *fdo = fopen("ucient.txt","a+");
			if(fdo)
			{
				fprintf(fdo,"%s\n",InputBuffer);
				fclose(fdo);
			}
		}
		PrintLog("%s\n",InputBuffer);
		NewBuffer = 1;
		while((token = GetNextToken()) != NULL )
		{
			if(strcmp(token,"uci")==0)
			{
				Print("id name Rocinante 2.0\nid author Antonio Torrecillas\n");
//	   "option name Nullmove type check default true\n"
//      "option name Selectivity type spin default 2 min 0 max 4\n"
//	   "option name Style type combo default Normal var Solid var Normal var Risky\n"
//	   "option name NalimovPath type string default c:\\n"
//	   "option name Clear Hash type button\n"
//				Print("option name Hash type spin default 32 min 4 max 8192\n");
//				Print("option name Clear Hash type button\n");
//				Print("option name Ponder type check default false\n");
				Print("option name cpus type spin default 1 min 0 max 256\n");
				Print("option name threads type spin default 1 min 0 max 256\n");
				Print("option name klevel type spin default 1 min 0 max 1\n");
				Print("option name mcts_ab type check default true\n");
				Print("option name probedepth type spin default 1 min -1 max 10\n");
				Print("uciok\n");
			}
			else
			if(strcmp(token,"quit")==0)	{
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
				board.PonderMode = 0;
			} 
			else
			{
				if(token && *token != '\0')
				Print("info string token <%s>\n",token);
			}
		}
		memset(&InputBuffer[0],0,SizeOfInputBuffer);
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
	else
	if(strcmp(token,"kiwipete")==0)
	{
		board.LoadFen("r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1 ");
		Print("info string kiwipete position loaded\n");
	}
	else
	if(strcmp(token,"pos3")==0)
	{
		board.LoadFen("8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - -");
		Print("info string pos3 position loaded\n");
	}
	else
	if(strcmp(token,"pos4")==0)
	{
		board.LoadFen("r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1");
		Print("info string pos4 position loaded\n");
	}
	else
	if(strcmp(token,"display")==0)
	{
		board.Display();
//	   Print("%s\n",Partida.T.SaveEPD());
	}
	else
	if(strcmp(token,"perft")==0)
	{
	   int d = 0;
		token = GetNextToken();
		if(token)
		{
			d = atoi(token);
			board.DoPerft(d);
		}
		else
		{
			Print("info string Sintax: debug perft depth\n");
		}
	}
	else
	if(strcmp(token,"testperft")==0)
	{
		board.TestPerft();
	}
	else
	if(strcmp(token,"simetry")==0)
	{
		token = GetNextToken();
		if(token)
		{
			board.DebugSimetric(token);
		}
	}
	else
	if(stricmp(token,"dumptree")==0)
	{
		DumpTree dt;
		dt.Write();
	}
	else
	if(stricmp(token,"sel")==0)
	{
		board.TotalNodes = 0;
		board.ProbeDepth = 1;
		int score = board.Search(); 
		printf("Score %d Nodes %d\n",score,board.TotalNodes);
	}
	else
	if(stricmp(token,"opt")==0)
	{
		board.TotalNodes = 0;
		board.ProbeDepth = 1;
		UndoData undo;
		board.DoNullMove(undo);
		int score = board.Search(); 
		board.DoNullMove(undo);
		printf("Score %d Nodes %d\n",score,board.TotalNodes);
	}
	else
	if(stricmp(token,"testepd")==0)
	{
		int depth_time;
		char * path = GetNextToken();
		if(path)
		{
			depth_time = atoi(path);
			path = GetNextToken();
			if(path && *path)
			{
				char local[200];
				strcpy(local,path);
				CEpd unTest;
				unTest.tiempo = depth_time;
				unTest.profundidad = depth_time;
				unTest.epdfile = local;
				unTest.UseMC = UseMCTS_AB;
				unTest.UsePVS =0; // UseMCTS_AB;
				unTest.Start();
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
	if(stricmp(token,"t")==0)
	{
		int depth_time;
		char * path = "todo.txt";
		if(path)
		{
			depth_time = 4;
			if(path && *path)
			{
				char local[200];
				strcpy(local,path);
				CEpd unTest;
				unTest.tiempo = depth_time;
				unTest.profundidad = depth_time;
				unTest.epdfile = local;
				unTest.UseMC = UseMCTS_AB;
				unTest.UsePVS = 0;
				unTest.Start();
			}
		}
	}
	else
	if(stricmp(token,"sts")==0)
	{
		int depth_time;
		char * path = GetNextToken();
		if(path)
		{
			int i,cuenta;
			depth_time = atoi(path);
			cuenta = 0;
			for(i=1;i < 15;i++)
			{
				char local[200];
				sprintf(local,"sts%d.epd",i);
				CEpd unTest;
				unTest.tiempo = depth_time;
				unTest.profundidad = depth_time;
				unTest.epdfile = local;
				unTest.UseMC = UseMCTS_AB;
				unTest.UsePVS = 0;
				unTest.Start();
				cuenta += unTest.TestAciertos;
			}
			printf("Aciertos Totales %d\n",cuenta);
		}
		else
		{
				Print("info string Sintax: debug sts depth(0-50)\n");
				Print("info string Sintax: debug sts time(miliseg)\n");
		}
	}
}

//const int MaxMaterialDetail = 1601;
//extern int MaterialDetail[MaxMaterialDetail];

void CUci::SetOption()
{
	char *name = GetNextToken();
	char *value;
	if(stricmp(name,"name") != 0) return;
//	   "setoption name Nullmove value true\n"
//      "setoption name Selectivity value 3\n"
//	   "setoption name Style value Risky\n"
//	   "setoption name Clear Hash\n"
//	   "setoption name NalimovPath value c:\chess\tb\4;c:\chess\tb\5\n"
	name = GetNextToken();

//	Print("option name mcts type check default true\n");
	if(stricmp(name,"MCTS_AB")==0)
	{
		value = GetNextToken();
		if(stricmp(value,"value")==0)
		{
			value = GetNextToken();
			UseMCTS_AB = true; // default
			if(stricmp(value,"false") == 0)
				UseMCTS_AB = false;
			if(stricmp(value,"0") == 0)
				UseMCTS_AB = false;
		}
	}
	else
	//Print("option name klevel type spin default 1 min 0 max 1\n");
	if(stricmp(name,"klevel")==0)
	{
		value = GetNextToken();
		if(stricmp(value,"value")==0)
		{
			int level;
			value = GetNextToken();
			level = atoi(value);
			board.SetLevel(level);
		}
	}
	else
	if(stricmp(name,"cpus")==0)
	{
		value = GetNextToken();
		if(stricmp(value,"value")==0)
		{
			int ncpus;
			value = GetNextToken();
			ncpus = atoi(value);
			SmpWorkers.InitWorkers(ncpus);
		}
	}
	else
	if(stricmp(name,"threads")==0)
	{
		value = GetNextToken();
		if(stricmp(value,"value")==0)
		{
			int ncpus;
			value = GetNextToken();
			ncpus = atoi(value);
			SmpWorkers.InitWorkers(ncpus);
		}
	}
	else
	//Print("option name probedepth type spin default 1 min -1 max 10\n");
	if(stricmp(name,"probedepth")==0)
	{
		value = GetNextToken();
		if(stricmp(value,"value")==0)
		{
			int depth;
			value = GetNextToken();
			depth = atoi(value);
			mc.ProbeDepth = depth; 
			bt.SetProbeDepth(depth);
		}
	}
	else
//	   "setoption name paramfile value c:/pond/gen1.h"
	if(stricmp(name,"paramfile")==0)
	{
		value = GetNextToken();
		if(stricmp(value,"value")==0)
		{
			extern bool LoadParam(char *name);
			value = GetNextToken();
			if(LoadParam(value))
				printf("parameters file successfully loaded\n");
			else
				printf("Error loading parameters file.\n");
		}
	}
}
#include "HashJugadas.h"
void CUci::UciNewG()
{
	InitG = 0;
	if(UseMCTS_AB && board.TT == NULL)
	{
		board.TT = (void *) new CHashJugadas();
		((CHashJugadas *)board.TT)->SetHashSize(4);
	}
	InitG = 1;
}

// position [fen <fenstring> | startpos ]  moves <move1> .... <movei>

void CUci::Position()
{
	static char *startpos = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq -";
	char *token = GetNextToken();
	char fen[100];
	fen[0] = '\0';
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
		}
	}
	// init to fen pos.
	if(fen[0])
		board.LoadFen(fen);
	ThreeFold.Reset();
	// get moves

	if(!token)
		token = GetNextToken();
	if(token && strcmp(token,"moves")==0)
	{
		token = GetNextToken();
		while(token)
		{
			board.DoMoveAlgebraic(token);
			board.SaveFEN(&fen[0]);
		    ThreeFold.Add(&fen[0]);

			// get next move
			token = GetNextToken();
		}
	}
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
//			Partida.PonderMode = 1;
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
	RunData.fen = NULL;
	RunData.MoveTime = 0;
   // depth limit
	int tiempo= 0;
	int incremento= 0;

   // time limit
	if(board.wtm  == White)
	{
		tiempo= (long)wTime;
		incremento= (long)winc;
	}
	else
	{
		tiempo=(long) bTime;
		incremento=(long) binc;
	}

	//if(Partida.incremento < 0)
	//	Partida.incremento = 0;
   if (movestogo <= 0 || movestogo > 30) movestogo = 30; // HACK

   if (movetime > 0) {

//       fixed time
	  RunData.MoveTime =(long) movetime;

   } 
   else 
   {

//       dynamic allocation
      double time_max = tiempo * 0.95 - 1.0;
      if (time_max < 0.0) time_max = 0.0;

      double alloc = (time_max + incremento * double(movestogo-1)) / double(movestogo);
	  // si estamos en ponder mode podemos alargar más
	  // no tengo claro si se debe tomar del comando go o del setoption ponder...
	  //if(Partida.PonderMode) alloc *= 1.4;

      if (alloc > time_max) alloc = time_max;
	  RunData.MoveTime  =(long) alloc;
   }

   // lanzar el analisis
   extern int Cancel;
	extern int Running;

   static char dest[100];
   board.SaveFEN(&dest[0]);
   if(UseMCTS_AB)
   {
	   if(depth)
		   mc.FixedDepth = depth;
	   else
			mc.TimeLimit = RunData.MoveTime;
	   mc.Run(dest);
   }
   else
   {
	   if(depth)
			bt.SetDepth(depth);
	   else
			bt.TimeLimit = RunData.MoveTime;
	   bt.Run(dest);
   }
   fflush(stdout);
}

void CUci::Stop()
{
	// la impresion de la jugada se realiza en Partida.IterativeDeepening();
	board.PonderMode = 0;
	board.cancelado = 1;
	
	printf("bestmove %s\n",board.JugadaActual);
}
