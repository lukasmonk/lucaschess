#include <iostream>

#include "Lime.h"
#include "data.h"
#include "prototypes.h"

using namespace std;




int index[64] =
{
110,111,112,113,114,115,116,117,
98,99,100,101,102,103,104,105,
86,87,88,89,90,91,92,93,
74,75,76,77,78,79,80,81,
62,63,64,65,66,67,68,69,
50,51,52,53,54,55,56,57,
38,39,40,41,42,43,44,45,
26,27,28,29,30,31,32,33
};

void  setboard(const char *ptr)
{


    int r;

    clearboard();

    int WK = 0;int BK = 0;int WP = 0;int BP = 0;
    int sq = 0;//the board square counter

    for (; ; *ptr++)
    {

              if (sq > 63) {break;}

              if(*ptr == 'K') {
                p->board[index[sq]].typ = wK;
                p->board[index[sq]].col = wpco;
                p->k[white] = index[sq];
                //p->material[white] += vK;
                WK++;
                sq++;

                continue;
              }
              if(*ptr == 'Q') {
                p->board[index[sq]].typ = wQ;
                p->board[index[sq]].col = wpco;
                sq++;
                p->material[white] += vQ;
                continue;
              }
              if(*ptr == 'R'){
                p->board[index[sq]].typ = wR;
                p->board[index[sq]].col = wpco;
                sq++;
                p->material[white] += vR;
                continue;
              }
              if(*ptr == 'B') {
                p->board[index[sq]].typ = wB;
                p->board[index[sq]].col = wpco;
                sq++;
                p->material[white] += vB;
                continue;
              }
              if(*ptr == 'N') {
                p->board[index[sq]].typ = wN;
                p->board[index[sq]].col = wpco;
                sq++;
                p->material[white] += vN;
                continue;
              }
              if(*ptr == 'P') {
                p->board[index[sq]].typ = wP;
                p->board[index[sq]].col = wpco;
                sq++;
                p->material[white] += vP;
                WP++;
               /* if(ranks[index[sq]] > 6)
                  {cout<<"\n FEN ILLEGAL WP. Press a key to close me.";
                  system("PAUSE");
                  delete TTable;
                  TTable = NULL;
                  exit(0);
                   }*/
                continue;
              }
              if(*ptr == 'k') {
                p->board[index[sq]].typ = bK;
                p->board[index[sq]].col = bpco;
                p->k[black] = index[sq];
                sq++;
                //p->material[black] += vK;
                BK++;
                continue;
              }
              if(*ptr == 'q') {
                p->board[index[sq]].typ = bQ;
                p->board[index[sq]].col = bpco;
                sq++;
                p->material[black] += vQ;
                continue;
              }
              if(*ptr == 'r') {
                p->board[index[sq]].typ = bR;
                p->board[index[sq]].col = bpco;
                sq++;
                p->material[black] += vR;
                continue;
              }
              if(*ptr == 'b') {
                p->board[index[sq]].typ = bB;
                p->board[index[sq]].col = bpco;
                sq++;
                p->material[black] += vB;
                continue;
              }
              if(*ptr == 'n') {
                p->board[index[sq]].typ = bN;
                p->board[index[sq]].col = bpco;
                sq++;
                p->material[black] += vN;
                continue;
              }
              if(*ptr == 'p') {
                p->board[index[sq]].typ = bP;
                p->board[index[sq]].col = bpco;
                sq++;
                p->material[black] += vP;
                BP++;
               /*  if(ranks[index[sq]] < 1)
                  {cout<<"\n FEN ILLEGAL BP. Press a key to close me.";
                  system("PAUSE");
                  delete TTable;
                  TTable = NULL;
                  exit(0);
                   }*/
                continue;
              }
              if(*ptr == '1') {sq+=1;continue;}
              if(*ptr == '2') {sq+=2;continue;}
              if(*ptr == '3') {sq+=3;continue;}
              if(*ptr == '4') {sq+=4;continue;}
              if(*ptr == '5') {sq+=5;continue;}
              if(*ptr == '6') {sq+=6;continue;}
              if(*ptr == '7') {sq+=7;continue;}
              if(*ptr == '8') {sq+=8;continue;}
              if(*ptr == '/') {continue;}
              if(*ptr == '\0') {break;}
    }

    if( (WK !=1 ) || (BK !=1 ) || (WP > 8) || (BP > 8) )
    {cout<<"\n FEN ILLEGAL NUM. Press a key to close me.";
     system("PAUSE");
     delete TTable;
     TTable = NULL;
     exit(0);
     }

    while(*ptr == ' ') *ptr++;
   // cout<<"\n setting side, ptr = "<<*ptr<<" as ";
    for (r = 0; ;++r)
    {
        if (*ptr == '\0') {break;}
        if (*ptr == 'w')
        { p->side = white;break;}
        if (*ptr == 'b')
        { p->side = black;break;}
    }

    *ptr++;
    while(*ptr == ' ') *ptr++;

    //cout<<"\n setting flags, ptr = "<<*ptr;
    p->castleflags = 0;
    while(*ptr != ' ')
    {

        if (*ptr == '\0') {break;}
        if (*ptr == '-') {break;}
        if (*ptr == 'K') {p->castleflags |= 8;}
        if (*ptr == 'Q') {p->castleflags |= 4;}
        if (*ptr == 'k') {p->castleflags |= 2;}
        if (*ptr == 'q') {p->castleflags |= 1;}
        *ptr++;
    }

    *ptr++;
    while(*ptr == ' ') *ptr++;

    if (*ptr == '-')
    {
              p->en_pas = noenpas;
    }
    else
    {
              p->en_pas = fileranktosquare(chartofile(*ptr), chartorank(*ptr));

    }
         if ( p->en_pas < A4 ||  p->en_pas > H6) {p->en_pas = noenpas;}

    *ptr++;
    while(*ptr == ' ') *ptr++;


    //----------set up the fifty move rule---------------------
	 p->fifty = (*ptr-'0')*10;
	 *ptr++;
	 p->fifty += (*ptr-'0');
	//---check it's ok
    if ( p->fifty < 0 ||  p->fifty > 49) {  p->fifty = 0;}
	p->fifty *=2;

	//--------now, set up the global position variables
    init_piecelists();
   	fullhashkey();
   // printboard();
	p->ply = 0;
	histply = 0;
	p->listc[0] = 0;

	cout<<"\n setboard, fifty = "<<p->fifty<<endl;
	//cout<<"\nboard set, fifty = "<<p->fifty;
/*
	//debug checking
	if(isattacked(p->k[p->side^1], p->side)) exit(0);*/


}
