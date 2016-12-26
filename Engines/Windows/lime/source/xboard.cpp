#include <iostream>

#include "Lime.h"
#include "data.h"
#include "prototypes.h"
#include "string.h"

#define noside 2

using namespace std;


string col[2] = {"black","white"};
void inifile()
{
     numelem = 16;
     char size[5];

     FILE *ini = fopen("Lime.ini", "r");

    if (!ini)
		{
          ini = NULL;
          return;
        }
    fgets(size , 5 , ini);
    numelem = atoi(size);
    cout<<"\nnumelem set to "<<numelem;
    delete TTable;
    TTable = NULL;
    init_hash_tables();
}

void settime(double t, double ot, int compside)
{
    if(compside == white)
    {
        searchparam->wtime = t;
        searchparam->btime = ot;
    }
    else
    {
        searchparam->btime = t;
        searchparam->wtime = ot;
    }

}

int repetition()
{
    int rep = 0;
    for (int i = 0; i < histply; ++i)
	{
	   	if (hist[i].hashkey == p->hashkey)
		{
		  rep++;
		}
    }
	return rep;
}


int drawmaterial()
{
    gameeval();
    if(eval->wpawns || eval->bpawns) {return FALSE;}
    if(eval->wRc || eval->wQc || eval->bRc || eval->bQc) {return FALSE;}
    if(eval->wBc > 1 || eval->bBc > 1) {return FALSE;}
    if(eval->wNc && eval->wBc) {return FALSE;}
    if(eval->bNc && eval->bBc) {return FALSE;}
    return TRUE;
}

int checkresult()
{
    if (p->fifty > 100)
    {
     cout<<"\n1/2-1/2 {fifty move rule (claimed by Lime)}"<<endl;
    }
    if (repetition() >= 2)
    {
     cout<<"\n1/2-1/2 {3-fold repetition (claimed by Lime)}"<<endl;
    }

    movegen();

	int played = 0;// no. moves we have played
	for (int i = p->listc[p->ply]; i < p->listc[p->ply+1]; ++i)
	{
		if (makemove(&p->list[i]))
		{
		    takemove();
			continue;
		}

		//we have a legal move, so increment 'played'
        played++;
        takemove();
	}
	if(played) return FALSE;

	int inc = isattacked(p->k[p->side], p->side^1);

	if(inc)
	{
	    if(p->side == white)
	    {
	      cout<<"\n0-1 {black mates (claimed by Lime)}"<<endl;return TRUE;
        }
        else
        {
          cout<<"\n1-0 {white mates (claimed by Lime)}"<<endl;return TRUE;
        }
    }
    else
    {
      cout<<"\n1/2-1/2 {stalemate (claimed by Lime)}"<<endl;return TRUE;
    }
}

void readinmove(char *m)
{
    if(nopvmove(m))
    {cout<<"\nno pv move";return;}
    char *ptr;
    ptr = m;
    int prom;
    int c = 0;
    int flag;
    char move_string[6];
    while (*ptr != '\0')
         {
              move_string[0] = *ptr++;
              move_string[1] = *ptr++;
              move_string[2] = *ptr++;
              move_string[3] = *ptr++;
              c+=4;
           if(*ptr == '\0' || *ptr == ' ')
           {
              move_string[4] = '\0';
           }
           else
           {
               move_string[4] = *ptr++;
               move_string[5] = '\0';
               c++;
           }
          flag =  understandmove(move_string, &prom);
          //  printboard();
          if (flag == -1)
          {
             //cout<<"\nnot understood move "<<move_string;
           cout<<"\nnot understood"<<m;

             assert(flag!=0);
          }
          if(c>3) break;//fudge
        }
}


void xthink()
{
 double alloctime;
 if(searchparam->pon == FALSE)
 {
   alloctime = allocatetime();
  // cout<<"\nallocated "<<alloctime<<endl;
  // cout<<dec<<" moves to go["<<col[p->side]<<"] = "<<searchparam->movestogo[p->side]<<endl;
   if(alloctime < 0) alloctime = 200;
   searchparam->starttime = double(clock());
   searchparam->stoptime = searchparam->starttime+alloctime;
 }
 else
 {
   alloctime = (pondertime()*4)/3;
   searchparam->starttime = double(clock());
   cout<<" allocated "<<alloctime<<" to ponder "<<endl;
   searchparam->pontime = searchparam->starttime+alloctime;
   searchparam->stoptime = searchparam->starttime+128000000;
 }

 calc();

 // stats();
 if(searchparam->pon == FALSE)
 {
  makemove(&best);
  cout<<"\n masdemove, fifty = "<<p->fifty;
  cout<<"\nmove "<<returnmove(best);
     cout<<endl;
     if(searchparam->movestogo[p->side^1] != -1)
     {
      searchparam->movestogo[p->side^1]--;
     }
     searchparam->ponfrom = deadsquare;
     searchparam->ponto = deadsquare;
 }
 else
 {
     searchparam->ponfrom = FROM(best.m);
     searchparam->ponto = TO(best.m);
     cout<<"\n finished ponderthink, fifty = "<<p->fifty;
 }
 cout<<"\n";
}



void xboard()
{
       char line[65536], command[65536], *ptr;
       inifile();
       setboard(startfen);
       clearhash();
       int compside = noside;
       int mps,base,inc;

       cout<<endl;
       initsearchparam();
       searchparam->xbmode = TRUE;
       searchparam->usebook = TRUE;
       for (;;)
       {
          fflush(stdout);

          if(drawmaterial())
          {
              cout<<"\n1/2-1/2 (insufficient material)"<<endl;
          }

          if(checkresult()) compside = noside;

          if(compside == p->side)
          {
              if(searchparam->xtime != -1)
              {
              settime(searchparam->xtime,searchparam->xotime,compside);
              }
              xthink();
              fflush(stdout);
              if(checkresult()) compside = noside;
              if(searchparam->movestogo[p->side^1]==0) searchparam->movestogo[p->side^1] = mps;
              if(searchparam->cpon == TRUE)
              {
                  searchparam->pon = TRUE;
                  searchparam->inf = TRUE;
                  xthink();
                  if(double(clock()) > searchparam->stoptime)
                  {
                      cout<<"pondertime was exceeded!"<<endl;
                  }
                  searchparam->inf = FALSE;
                  searchparam->pon = FALSE;
              }
          }
         // cout<<"tell guest986 hello";
          if (!fgets(line, 65536, stdin))
			continue;
		  if (line[0] == '\n')
			continue;
          sscanf(line, "%s", command);
          ptr = line;

          if (!strncasecmp(command, "xboard", 6))
			{
			   // cout<<"received "<<command;
			   // cout<<endl;
                continue;
			}
	      if (!strncasecmp(command, "protover", 6))
			{
			   // cout<<"received "<<command<<endl;
                cout<<"feature usermove=1\n";
                cout<<"feature ping=1\n";
                cout<<"feature setboard=1\n";
                cout<<"feature reuse=1\n";
                cout<<"feature colors=0\n";
                cout<<"feature name=0\n";
                cout<<"feature done=1\n";
                cout<<"feature ics=1\n";

                continue;
			}
		  if (!strncasecmp(command, "accepted", 6))
			{
			   // cout<<"received "<<command<<endl;
                continue;
			}
	      if (!strncasecmp(command, "level", 6))
			{
			  initsearchparam();
			//  cout<<"received "<<command<<endl;
              sscanf(line, "level %d %d %d", &mps,&base,&inc);
			  if(mps)
			  {
			    searchparam->movestogo[white] = mps;
			    searchparam->movestogo[black] = mps;
			   // cout<<"moves to go["<<col[black]<<"] = "<<searchparam->movestogo[black]<<endl;
			   // cout<<"moves to go["<<col[white]<<"] = "<<searchparam->movestogo[white]<<endl;
			  }
			  if(inc)
			  {
			    searchparam->winc = inc*1000;
			    searchparam->binc = inc*1000;
			  }
			  searchparam->depth = -1;
			  continue;
			}

          if (!strncasecmp(command, "new", 3))
			{
                //  cout<<"received "<<command<<endl;
                  setboard(startfen);
                  clearhash();
                  compside = black;
                  continue;
            }
          if (!strncasecmp(command, "perft", 5))
			{
                  cout<<"received "<<command<<endl;
                  setboard(startfen);
                  clearhash();
                  compside = black;
                  perft(6);
                  continue;
            }
          if (!strncasecmp(command, "quit", 4))
            {
                cout<<"received "<<command<<endl;
                delete TTable;
                TTable = NULL;
                exit(0);
            }
          if (!strncasecmp(command, "force", 5))
            {
              cout<<"received "<<command<<endl;
              compside = noside;
              continue;
            }
          if (!strncasecmp(command, "go", 2))
            {
                cout<<"received "<<command<<endl;
                compside = p->side;
                continue;
            }
          if (!strncasecmp(command, "pr", 2))
            {
              //  cout<<"received "<<command<<endl;
                printboard();
                continue;
            }
          if (!strncasecmp(command, "sd", 2))
            {
              ptr += 3;
              searchparam->depth = atoi(ptr);
            //  cout<<"depth set to "<<searchparam->depth;
            //  cout<<endl;
           //   cout<<"received "<<command<<endl;
              continue;
            }
          if (!strncasecmp(command, "st", 2))
            {
              ptr += 3;
              searchparam->timepermove = (atoi(ptr)*1000);
           //   cout<<"movetime "<<searchparam->timepermove<<endl;
           //   cout<<"received "<<command<<endl;
              searchparam->depth = -1;
              cout<<endl;
              continue;
            }
          if (!strncasecmp(command, "time", 4))
            {
              ptr += 5;
              searchparam->xtime = (atoi(ptr)*10);
              searchparam->depth = -1;
             // cout<<"received "<<command<<endl;
            //  cout<<"time "<<ptr<<endl;
              continue;
            }
          if (!strncasecmp(command, "otim", 4))
            {
              ptr += 5;
              searchparam->xotime = (atoi(ptr)*10);
              searchparam->depth = -1;
             // cout<<"received "<<command<<endl;
             // cout<<"otim "<<ptr<<endl;
              continue;
            }
          if (!strncasecmp(command, "usermove", 8))
            {
              ptr += 9;
              readinmove(ptr);
             // cout<<"received "<<command;
             // cout<<" "<<ptr;
              //in the case of a mps time control, decrement the moves to
              //go for the opponent - used for ponder time allocation
              if(searchparam->movestogo[p->side^1] != -1)
              {
               searchparam->movestogo[p->side^1]--;
             //  cout<<" ( decrement "<<col[p->side^1]<<" ) "<<endl;
             //  cout<<dec<<" moves to go["<<col[p->side^1]<<"] = "<<searchparam->movestogo[p->side^1]<<endl;
               if(searchparam->movestogo[p->side^1]==0) searchparam->movestogo[p->side^1] = mps;
              }
              continue;
            }
          if (!strncasecmp(command, "ping", 4))
            {
              ptr += 5;
             // cout<<"received "<<command<<endl;
              cout<<"pong "<<atoi(ptr);
              cout<<endl;
              continue;
            }
          if (!strncasecmp(command, "draw", 4))
            {
              gameeval();
              if(isdrawnp()==0) cout<<"offer draw"<<endl;
              continue;
            }
           if (!strncasecmp(command, "setboard", 8))
            {
                ptr += 9;
                setboard(ptr);
                printboard();
                continue;
            }
            if (!strncasecmp(command, "ics", 3))
            {
               // cout<<"received "<<command<<endl;
                ptr += 4;
                if(*ptr != '-')
                {
                    searchparam->ics = TRUE;
                   // cout<<" ics set to true"<<endl;
                }
                else
                {
                    searchparam->ics = FALSE;
                   // cout<<" ics set to false"<<endl;
                }
                continue;
            }
            if (!strncasecmp(command, "hint", 4))
            {
             // cout<<"received "<<command<<endl;
              continue;
            }
            if (!strncasecmp(command, "result", 6))
            {
             // cout<<"received "<<command<<endl;
              continue;
            }
            if (!strncasecmp(command, "bk", 2))
            {
             // cout<<"received "<<command<<endl;
              continue;
            }
            if (!strncasecmp(command, "white", 5))
            {
              compside = white;
             // cout<<"received "<<command<<endl;
              continue;
            }
            if (!strncasecmp(command, "black", 5))
            {
              compside = black;
            //  cout<<"received "<<command<<endl;
              continue;
            }
            if (!strncasecmp(command, "remove", 6))
            {
             // cout<<"received "<<command<<endl;
             // printboard();
              if(histply > 0)
              {
                takemove();
              //  printboard();
                if(histply > 0)
                {
                 takemove();
              //  printboard();
                }
              }
              continue;
            }
            if (!strncasecmp(command, "undo", 4))
            {
             // cout<<"received "<<command<<endl;
            //  printboard();
              if(histply > 0)
              {
                takemove();
             //   printboard();
              }
              continue;
            }
            if (!strncasecmp(command, "hard", 4))
            {
             // cout<<"received "<<command<<endl;
              searchparam->cpon = TRUE;
              continue;
            }
            if (!strncasecmp(command, "easy", 4))
            {
             // cout<<"received "<<command<<endl;
              searchparam->cpon = FALSE;
              continue;
            }
            if (!strncasecmp(command, "post", 4))
            {
              searchparam->post = TRUE;
             // cout<<"received "<<command<<endl;
              continue;
            }
            if (!strncasecmp(command, "nopost", 4))
            {
              searchparam->post = FALSE;
             // cout<<"received "<<command<<endl;
              continue;
            }
            if (!strncasecmp(command, "name", 4))
            {
            //  cout<<"received "<<command<<endl;
              continue;
            }
            if (!strncasecmp(command, "computer", 4))
            {
             // cout<<"received "<<command<<endl;
              continue;
            }
            if (!strncasecmp(command, "random", 4))
            {
             // cout<<"received "<<command<<endl;
              continue;
            }
            cout<<"\nerror "<<ptr;
       }

}
