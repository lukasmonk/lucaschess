#include <iostream>

#include "Lime.h"
#include "data.h"
#include "prototypes.h"

using namespace std;

//return a pointer to a char array representing a square
char   *returnsquare(int from)
{
       static char move[2];
       sprintf(move, "%c%c",
           filetochar(files[from]),
           ranktochar(ranks[from]));
      return move;
}

//return a move in e7e8q format
char   *returnmove(s_Move move)
{
       static char m[4];
       int f = FROM(move.m);
       int t = TO(move.m);
       int flag = FLAG(move.m);

       char one = filetochar(files[f]);
       char two = ranktochar(ranks[f]);
       char three = filetochar(files[t]);
       char four = ranktochar(ranks[t]);
       char five;

       //if it's a promotion, find which one
       if(flag & mProm)
       {
           if(flag & oPQ)
           five = 'q';
           else if (flag & oPR)
           five = 'r';
           else if (flag & oPB)
           five = 'b';
           else if (flag & oPN)
           five = 'n';
           else
           five = 'E';
       }

       sprintf(m, "%c%c%c%c",
           one,two,three,four);


      return m;
}



//try to understand the given move
int  understandmove(char move[], int *prom)
{
      //if we have no move, we return -1
      int returnflag = -1;
      *prom = 0;

      //if the characters are not in the required range, it's not a legal
      //move, so exit
      if ((move[0] < 'a' || move[0] > 'h') ||
           (move[1] < '1' || move[1] > '8') ||
            (move[2] < 'a' || move[2] > 'h') ||
           (move[3] < '1' || move[3] > '8') )
      {
           cout<<"\nILLEGAL PARSE : "<<move;
           return -1;
      }
  //it is a legal 'looking'move, so make some from and to squares out of it
  int from = fileranktosquare(chartofile(move[0]), chartorank(move[1]));
  int to =   fileranktosquare(chartofile(move[2]), chartorank(move[3]));

  //now we need to find the move in our list, so generate the moves
  movegen();

  //now find the position in the move list (position 'i')
     int i;
     for (i = p->listc[p->ply]; i < p->listc[p->ply+1]; ++i)
     {
         //see if the move matches from and to from above
         if (FROM(p->list[i].m) == from && TO(p->list[i].m) == to)
         {
           //could be a promotion - if so, check we have the exact move
           //if not, the continue the loop, if we do then break
           if(FLAG(p->list[i].m) & mProm)
           {
               if((move[4] ==  'q') && (p->list[i].m & oPQ))
                {returnflag = i;*prom = 1;
                break;}
               else if((move[4] ==  'r') && (p->list[i].m & oPR))
                {returnflag = i;*prom = 1;
                break;}
               else if((move[4] ==  'b') && (p->list[i].m & oPB))
                {returnflag = i;*prom = 1;
                break;}
               else if((move[4] ==  'n') && (p->list[i].m & oPN))
                {returnflag = i;*prom = 1;
                break;}
               else {continue;}
           }

           returnflag = i;
           break;
         }

     }
      if (returnflag == -1)//no move!!
     {
        return -1;
     }

      //now check the move is legal,by making it
            if (makemove(&p->list[i]))
           {
               takemove();
               cout<<"\nillegal move!";

               //it wasn't legal, so return
               returnflag = -1;*prom=0;
           }

       //we've made it with a legalmove, so return the position - in truth,
       //any number apart from -1 is ok here
       return returnflag;

}


//print the pv
void printpv(int score)
{
     int j,t;
 if(searchparam->ucimode)
 {
     cout<<"info depth "<<itdepth;
     cout<<" score cp "<<score;
     cout<<" time "<<(double(clock())-searchparam->starttime);
     cout<<" nodes "<<nodes+qnodes;
     cout<<" depth "<<itdepth<<" pv";
     if(logme){
     fprintf(log_file, "info depth %d score cp %d time %f nodes %d depth %d pv",
                itdepth, score, ((double(clock())-searchparam->starttime)), nodes+qnodes, itdepth);
     }
     for (j = 0;j< pvindex[0]; ++j)
         {
             cout<<" "<<returnmove(pv[0][j]);
             if(logme){
             fprintf(log_file, " %s", (returnmove(pv[0][j])));
             }
         }

cout<<"\n";
      if(logme){
      fprintf(log_file, "\n");
      }
 }
 else if (searchparam->post || searchparam->ics)
 {
     if(searchparam->post)
     {
         if(itdepth > 7 && searchparam->pon == FALSE)
         {
         cout<<"\ntellothers depth "<<itdepth<<" score(cp) ";
         cout<<score<<" time(s*100) ";
         cout<<(int)((double(clock())-searchparam->starttime)/10);
         cout<<" nodes ";
         cout<<nodes+qnodes;cout<<" pv = ";
         for (j = 0;j< pvindex[0]; ++j)
         {
             cout<<" "<<returnmove(pv[0][j]);
         }
        cout<<"\n";fflush(stdout);
        }
         cout<<itdepth<<" ";
         cout<<score<<" ";
         cout<<(int)((double(clock())-searchparam->starttime)/10)<<" ";
         cout<<nodes+qnodes;
         for (j = 0;j< pvindex[0]; ++j)
         {
             cout<<" "<<returnmove(pv[0][j]);
             if(logme){
             fprintf(log_file, " %s", (returnmove(pv[0][j])));
             }
         }
        cout<<"\n";fflush(stdout);
     }

 }

}

//print the search stats
void stats()
{
    cout<<"\nordering = "<<(fhf/fh)*100<<endl;
    cout<<"null success = "<<(nullcut/nulltry)*100<<endl;
    cout<<"hashhit = "<<(hashhit/hashprobe)*100<<endl;
    cout<<"pvsf = "<<((pvsh/pvs)*100)<<endl;
    cout<<"\nincheckext "<<incheckext;
    cout<<" wasincheck "<<wasincheck;
    cout<<" matethrt "<<matethrt;
    cout<<"\npawnfifth "<<pawnfifth;
    cout<<" pawnsix "<<pawnsix;
    cout<<" prom "<<prom;
    cout<<" hisred ="<<reduct;
    cout<<"\n single ="<<single;
    cout<<" resethis ="<<resethis;


}

//check if the move found is a move, return TRUE if it's bad
int nopvmove(char *move)
{
      if ((move[0] < 'a' || move[0] > 'h') ||
           (move[1] < '1' || move[1] > '8') ||
            (move[2] < 'a' || move[2] > 'h') ||
           (move[3] < '1' || move[3] > '8') )
      {
           return TRUE;
      }
      return FALSE;
}

char   *returncastle()
{
       static char m[4];

       char one = '-';
       char two = '-';
       char three = '-';
       char four = '-';
      if(p->castleflags & 8) one = 'K';
      if(p->castleflags & 4) two = 'Q';
      if(p->castleflags & 2) three = 'k';
      if(p->castleflags & 1) four = 'q';


       sprintf(m, "%c%c%c%c",
           one,two,three,four);


      return m;
}



//try to understand the given move
int  myparse(char move[])
{
      //if we have no move, we return -1
      int returnflag = -1;


      //if the characters are not in the required range, it's not a legal
      //move, so exit
      if ((move[0] < 'a' || move[0] > 'h') ||
           (move[1] < '1' || move[1] > '8') ||
            (move[2] < 'a' || move[2] > 'h') ||
           (move[3] < '1' || move[3] > '8') )
      {
           cout<<"\nILLEGAL PARSE : "<<move;
           return -1;
      }
  //it is a legal 'looking'move, so make some from and to squares out of it
  int from = fileranktosquare(chartofile(move[0]), chartorank(move[1]));
  int to =   fileranktosquare(chartofile(move[2]), chartorank(move[3]));

  //now we need to find the move in our list, so generate the moves
  movegen();

  //now find the position in the move list (position 'i')
     int i;
     for (i = p->listc[p->ply]; i < p->listc[p->ply+1]; ++i)
     {
         //see if the move matches from and to from above
         if (FROM(p->list[i].m) == from && TO(p->list[i].m) == to)
         {
           return i;
           break;
         }

     }
     if (returnflag == -1)//no move!!
     {
        return -1;
     }

       return returnflag;
}


