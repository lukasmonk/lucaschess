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

#include "stdio.h"
#include "stdlib.h"
#include "memory.h"
#include "string.h"
#include <time.h>

#include "CDebug.h"
#include "MoveList.h"
#include "UndoData.h"
#include "Board.h"

#include "TreeNode.h"
#include "BStar.h"
//#include "Search.h"

#include "MCTS_AB.h"
#include "epd.h"


/*
*	Clase base para distintos tests
*	Esta clase se encarga de recorrer las posiciones de un archivo epd 
*   y cargando estas en el tablero.
*	Una funci�n virtual TestPos se encarga de analizar la posicion alcanzada.
*/


CEpd::CEpd()
{
}
CEpd::~CEpd()
{
}

bool CEpd::AbreArchivos()
{
	fi = fo = fsi = fno = NULL;
		// abrir el archivo
	fi = fopen(epdfile,"r");
	if(fi)
	{
		fo = fopen("./testepd.txt","w+");
		fsi = fopen("./si.txt","w+");
		fno = fopen("./no.txt","w+");
		if(fo)
		{
			struct tm *newtime;
			time_t aclock;
			time( &aclock_ini);
			time( &aclock );   // Get time in seconds
			newtime = localtime( &aclock );   // Convert time to struct tm form

			fprintf(fo,"Test %s\n",epdfile);
			fprintf(fo,"Fecha %s\n",asctime(newtime));
			fflush(fo);
		}

		return true;
	}
	return false;
}
void CEpd::CierraArchivos()
{
	struct tm *newtime;
	if(!fi) return;
	time( &aclock );   // Get time in seconds
	newtime = localtime( &aclock );   // Convert time to struct tm form

	fprintf(fo,"Resumen Test %s\n",epdfile);
	fprintf(fo,"Fecha %s\n",asctime(newtime));
	fprintf(fo,"Tiempo %ld\n",aclock-aclock_ini);
	fprintf(fo,"Branch Factor %f\n",BFMedio/(test_aciertos+test_fallos));
	fflush(fo);
	if((test_aciertos+test_fallos))
		fprintf(fo,"\nTotal %d aciertos %d fallos => %2d%% \n",test_aciertos,test_fallos,(test_aciertos*100)/(test_aciertos+test_fallos));
	if((aclock-aclock_ini) == 0)
		aclock = aclock_ini+1;
	fprintf(fo,"\nNodes %d nps %ld\n",NodeCount,NodeCount /(aclock-aclock_ini));
	printf("\nNodes %d nps %d \n",NodeCount ,NodeCount /(aclock-aclock_ini));
	TestAciertos = test_aciertos;
	TestNodeCount = NodeCount;
	fclose(fsi);
	fclose(fno);
	fclose(fo);
	fclose(fi);
	extern void DumpNodesFaseDepth();
}

void CEpd::SeparaLineaEpd()
{
	int blancos;
	// buscar el ';' atras hasta un ' '
	memset(Epd,0,sizeof(Epd));
	memset(Solucion,0,sizeof(Solucion));
	blancos = 0;
	aux = Test;

	bm = strstr(aux,"bm")!= NULL;
	if(bm)
	{
		aux = strstr(Test,"bm");
		strncpy(Epd,Test,aux-Test);
	}
	else
	{
		aux = strstr(Test,"am");
		strncpy(Epd,Test,aux-Test);
	}
	aux++;
	aux++;
	aux++;
	i = 0;
	while(*aux == ' ')
		aux++;
	while(*aux != ';' && *aux != '\r' && *aux != '\n' && *aux != '\0')
	{
		if(*aux != '-')
		{
			Solucion[i++] = *aux++;
		}
		else
			aux++;
	}
	// eliminamos el \n final
	while(*aux != '\0')aux++;
	aux--;
	*aux = '\0';

	// carga y prueba del test
//	printf("%s\n",Test);

	// obtener la solucion en formato algebraico.
//	Board board;
	int Move;
	board.LoadFen(Test);
	aux = strtok(Solucion," ;");
	strcpy(SolucionAlg[0],aux);

	Move = board.IdentificaPgn(SolucionAlg[0]);
	if(Move)
		board.MoveToAlgebra(Move,SolucionAlg[0]);

	aux = strtok(NULL," ;");
	SolucionAlg[1][0] = '\0';
	if(aux && *aux != ' ')
	{
		strcpy(SolucionAlg[1],aux);
		Move = board.IdentificaPgn(SolucionAlg[1]);
		if(Move)
			board.MoveToAlgebra(Move,SolucionAlg[1]);
	}
	aux = strtok(NULL," ;");
	SolucionAlg[2][0] = '\0';
	if(aux && *aux != ' ')
	{
		strcpy(SolucionAlg[2],aux);
		Move = board.IdentificaPgn(SolucionAlg[2]);
		if(Move)
			board.MoveToAlgebra(Move,SolucionAlg[2]);
	}
}

const int UseBStar = true;

//extern int BestMoveSearch;

bool CEpd::ProcesaTestEpd()
{
	char *aux;
	char moveStr[10];
	if(UsePVS)
	{
		board.LoadFen(Epd);
		if(tiempo < 10)
		{
			board.tiempo_limite = 0;
			board.LimiteProfundidad = tiempo;
		}
		else
		{
			board.tiempo_limite =  tiempo;
			board.LimiteProfundidad = 0;
		}
		board.IterativeDeepening();
		aux = board.JugadaActual;
	}
	else
	if(UseMC)
	{
		mc.TotalExpand = 0;
		if(tiempo < 10)
		{
//			mc.TimeLimit = 0;
		}
		else
		{
			mc.TimeLimit = tiempo;
		}
		mc.Run(Epd);
		NodeCount += mc.TotalExpand;
		ProbeCount += mc.TotalProbes; 
		ExpandToSolve = Bt.LastChange;
		// es OK
		// quitamos el + final si lo hay
		aux = mc.BestMoveStr;
	}
	else
	if(UseBStar)
	{
		//if(profundidad && profundidad < 25)
		//	Bt.SetProbeDepth(profundidad);
		Bt.TotalExpand = 0;
		if(tiempo < 10)
		{
			Bt.SetDepth(tiempo);
			Bt.TimeLimit = 0;
		}
		else
		{
			Bt.SetDepth(0);
			Bt.TimeLimit = tiempo;
		}
		Bt.Run(Epd);
		NodeCount += Bt.TotalExpand;
		ProbeCount += Bt.TotalProbes; 
		ExpandToSolve = Bt.LastChange;
		// es OK
		// quitamos el + final si lo hay
		aux = Bt.BestMoveStr;
	}
	else
	{
		aux = &moveStr[0];
		*aux = '\0';
		board.LoadFen(Epd);
		board.Search();
		board.MoveToAlgebra(board.BestMoveSearch,aux);
	}
	if(*aux == '\0')
		return false;
	//while(*aux)
	//{
	//	if(*aux == '+')
	//		*aux = '\0';
	//	aux++;
	//}
	if(bm)
	{

		return !((strcmp(SolucionAlg[0],aux)!=0 )	&& 
			(strcmp(SolucionAlg[1],aux)!=0 )		&&
			(strcmp(SolucionAlg[2],aux)!=0 ) 		);

	}
	else
	{	// avoid move
		return ((strcmp(SolucionAlg[0],aux)!=0 )	&& 
			(strcmp(SolucionAlg[1],aux)!=0 )		&&
			(strcmp(SolucionAlg[2],aux)!=0 ) 		);
	}
	return false;
}

void CEpd::Start()
{
	extern void Print(const char *fmt, ...);
	extern int UseLog;
	aclock_ini = 0;	aclock = 0;	Dm = 0; // depth media
	NodeCount = 0;	BFMedio = 0; ProbeCount = 0;
	test_aciertos = 0;	test_fallos = 0;

	if(!epdfile)return;


#ifdef _DEBUG
	UseLog = 1;
#endif
	score = 0;
	if(AbreArchivos())
	{
	// cada linea es un test individual
		while(fgets(Test,sizeof(Test),fi))
		{
			if(Test[0] == '\r')
				break;
			if(Test[0] == '\n')
				break;
			//if(Test[0] == ';') // saltamos los comentarios
			//{
			//	PrintLog(Test);
			//	continue;
			//}
			// separa epd de la solucion
			SeparaLineaEpd();
			if(UseBStar)
				Print("PARCIAL %d - %d\n",test_aciertos,test_fallos);
			if(Test[0] == '\0')
				break;
			if(UseBStar)
			{
				Print(Test);
				Print("\n");
			}
			// analizar
			if(ProcesaTestEpd())
			{
				// test positivo
				//fprintf(fsi,"%s\n",Test);
				//fflush(fsi);
				test_aciertos++;
				//score += 500 - ExpandToSolve;
			}
			else
			{
				// test negativo
				test_fallos++;
				//fprintf(fno,"%s\n",Test);
				//fflush(fno);
			}
			Test[0] = '\0';
		}
	}
	CierraArchivos();
	Print("Test Finalizado %d - %d %f score %d t=%d\n",test_aciertos,test_fallos,(test_aciertos*1.0)/(1.0*(test_aciertos+test_fallos)),score,aclock-aclock_ini);
}
