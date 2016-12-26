#include <iostream>

#include "Lime.h"
#include "data.h"
#include "prototypes.h"

using namespace std;
void uci_loop();
void uci_mode()
{


    searchparam->cpon = TRUE;
    searchparam->ucimode = TRUE;
    searchparam->usebook = FALSE;
    cout<<endl;
    cout<<"id name Lime_v66\n";
    cout<<"id author Richard Allbert\n";
    cout<<"option name Hash type spin default 16 min 4 max 512\n";
    cout<<"option name OwnBook type check default false\n";
   /* cout<<"option name PawnStructure type spin default 128 min 12 max 256\n";
    cout<<"option name KingSafety type spin default 128 min 12 max 256\n";
    cout<<"option name PassedPawn type spin default 128 min 12 max 256\n";*/
    cout<<"option name Ponder type check default true\n";
    cout<<"uciok\n";

      uci_loop();

}


void uci_loop()
{

   char line[65536], command[65536];


          for (;;)
       {
          fflush(stdout);
          if (!fgets(line, 65536, stdin))
			continue;
		  if (line[0] == '\n')
			continue;
          sscanf(line, "%s", command);
          if (!strncasecmp(command, "isready", 7))
			{
                  cout<<"readyok\n";fflush(stdout);
                  continue;
            }
          if (!strncasecmp(command, "position", 8))
			{
                  parse_position(line);
                  continue;
            }
          if(!strncasecmp(command, "setoption", 9))
          {
                  parse_option(line);
                  continue;
          }
          if (!strncasecmp(command, "ucinewgame", 10))
			{
                  setboard(startfen);
                  clearhash();
                  continue;
            }
          if (!strncasecmp(command, "go", 2))
			{
                 parse_go(line);
                 //fflush(stdin);
               //  cout<<"returned uci_loop()\n";
                  continue;
            }
            if (!strncasecmp(command, "setboard", 8))
            {

               continue;
            }

            if (!strncasecmp(command, "quit", 4))
            {
                delete TTable;
                TTable = NULL;
                exit(0);
                //return;
            }
            if (!strncasecmp(command, "uci", 3))
            {
                cout<<"id name Lime_v63\n";
                cout<<"id author Richard Allbert\n";
                cout<<"uciok\n";
                continue;
            }

            else {cout<<"\nunknown command "<<command<<"\n";continue;}
       }


}

void parse_position(char string[])
{
     char *fen;
     char *moves;
     const char *ptr;
     char move_string[256];
     int flag;
      if(logme){
       fprintf(log_file, "\n parsing position..\n");
       writestring(string);}
     //cout<<"parsing "<<string<<"\n";

     /*
      initialise pointers in the string
     */
     /*
     first, see if fen exists in the string, if so,
     point *fen to the string following 'fen'
     */
     fen = strstr(string,"fen ");

     /*
     likewise, look for moves
     */
     moves = strstr(string,"moves ");

     /*
     if we were given an fen, then fen will not == NULL
     */
     if (fen != NULL)
     {
             if (moves != NULL)
             {
                   moves[-1] = '\0';
             }
       /*
       we have been given an fen, so we'll set the position
       accordingly
       */
       setboard(fen+4);
     }
     /*
       we weren't given and fen, so initialise starting position
     */
     else
     {
         setboard(startfen);
        // printboard(game);
     }

     /*
     now, if we have moves, parse and make them
     */
     int prom = 0;
     if (moves != NULL)
     {
               ptr = moves+6;//set start position of the pointer

         while (*ptr != '\0')
         {
              move_string[0] = *ptr++;
              move_string[1] = *ptr++;
              move_string[2] = *ptr++;
              move_string[3] = *ptr++;

           if(*ptr == '\0' || *ptr == ' ')
           {
              move_string[4] = '\0';

           }
           else
           {
               move_string[4] = *ptr++;
               move_string[5] = '\0';
           }

          cout<<"\n move "<<move_string;
          cout<<" "<<p->fifty;
          flag =  understandmove(move_string, &prom);
        //  printboard();
          if (flag == -1)
          {
             //cout<<"\nnot understood move "<<move_string;
           writestring("\nnot understood");
           writestring(string);
             assert(flag!=0);
          }

          /*
          if we had a prom, prom is 1
          */
          if (prom) {ptr++;}

           while (*ptr == ' ') ptr++;
         }
       }
    //   printboard();

}




void parse_go(char string[])
{
     char *ptr;
     if(logme){
       fprintf(log_file, "\n parsing go..\n");
       writestring(string);}
//cout<<"go()\n";
   /*
    set the variables to a 'start position'
   */
   initsearchparam();

   ptr = strtok(string," "); // skip "go"

   for (ptr = strtok(NULL," "); ptr != NULL; ptr = strtok(NULL," "))
   {

     if (false)
      {
         ;
      }

      else if (!strncasecmp(ptr,"infinite", 9))
      {
             searchparam->inf = TRUE;
      }

      else if (!strncasecmp(ptr,"binc", 4))
      {
          ptr = strtok(NULL," ");
          searchparam->binc = double(atoi(ptr));
      }
      else if (!strncasecmp(ptr,"btime", 5))
      {
         ptr = strtok(NULL," ");
         searchparam->btime = double(atoi(ptr));
      }
      else if (!strncasecmp(ptr,"depth", 5))
      {
         ptr = strtok(NULL," ");
         searchparam->depth = atoi(ptr);
      }

      else if (!strncasecmp(ptr,"mate", 4))
      {

      }
      else if (!strncasecmp(ptr,"movestogo", 9))
      {
         ptr = strtok(NULL," ");
         searchparam->movestogo[p->side] = double(atoi(ptr));
      }
      else if (!strncasecmp(ptr,"movetime", 8))
      {
         ptr = strtok(NULL," ");
         searchparam->timepermove = double(atoi(ptr));
      }
      else if (!strncasecmp(ptr,"nodes", 5))
      {

      }
      else if (!strncasecmp(ptr,"ponder", 6))
      {
         searchparam->pon = TRUE;
         searchparam->inf = TRUE;
      }
      else if (!strncasecmp(ptr,"searchmoves", 11))
      {
        ;
      }
      else if (!strncasecmp(ptr,"winc", 4))
      {
           ptr = strtok(NULL," ");
           searchparam->winc = double(atoi(ptr));
      }
      else if (!strncasecmp(ptr,"wtime", 5))
      {
         ptr = strtok(NULL," ");
         searchparam->wtime = double(atoi(ptr));
      }
   }

    think();

}

void think()
{
 double alloctime = allocatetime();
 cout<<"\nallocated "<<alloctime<<endl;
 if(alloctime < 0) alloctime = 200;
  searchparam->starttime = double(clock());
 searchparam->stoptime = searchparam->starttime+alloctime;
 if(logme){
 fprintf(log_file, "\nCalling calc(), alloctime = %d", alloctime);
 writeboard();}

 calc();
 if(logme){
 fprintf(log_file, "\nReturned from calc()");
 writeboard();}
 //stats();
 searchparam->inf = FALSE;
 /*
   if(nopvmove(returnmove(pondermove)))
   {
    pondermove = findhashmove(best);
   }*/

//---------------------------------------//
/*
DUMP THE HASH

s_Hashelem *probe;
int f,t;
ofstream hashlog("hashlog.txt");
hashlog<<"\n checking hashclear values";
     for (int hh = 0; hh < numelem; ++hh)
     {
        hashlog<<"\nelement "<<hh;
        probe =  TTable + hh;
        hashlog<<" key "<<probe->hashkey;
        hashlog<<" depth "<<probe->depth;
        hashlog<<" score "<<probe->score;
        f = FROM(probe->move);t=TO(probe->move);
        hashlog<<" "<<returnsquare(f);
        hashlog<<returnsquare(t);
     }
hashlog.close();*//*
 for (int  j = 0;j< pvindex[0]; ++j)
         {
             cout<<" ("<<FROM(pv[0][j].m)<<" ";
             cout<<TO(pv[0][j].m)<<")";
         }*/



//--------------------------------------//

     cout<<"\nbestmove "<<returnmove(best);
     cout<<" ponder "<<returnmove(pondermove);

     cout<<"\n";

}

void initsearchparam()
{
     searchparam->depth = -1;
     searchparam->winc = -1;
     searchparam->binc = -1;
     searchparam->wtime = -1;
     searchparam->btime = -1;
     searchparam->xtime = -1;
     searchparam->xotime = -1;
     searchparam->movestogo[white] = -1;
     searchparam->movestogo[black] = -1;
     searchparam->timepermove = -1;
     searchparam->starttime = -1;
     searchparam->stoptime = -1;
     searchparam->inf = FALSE;
     searchparam->pon = FALSE;
     searchparam->ponderhit = FALSE;
     searchparam->post = TRUE;
}

void parse_option(char string[])
{
     char *Option = NULL;
     int i;


     Option = strstr(string,"Hash ");
     if(Option != NULL)//we've had a hash option
     {
         delete TTable;
         TTable = NULL;

         Option += 11;
         i = atoi(Option);
         if (i < 4) {i = 4;}
         numelem = i;
         init_hash_tables();
         return;
     }
     Option = strstr(string,"OwnBook ");
     if(Option != NULL)//we've had a hash option
     {
           Option += 14;
          if (!strncasecmp(Option, "true", 4))
			{
			    searchparam->usebook = TRUE;
			    cout<<" using book"<<endl;
			}
	       if (!strncasecmp(Option, "false", 5))
			{
			    searchparam->usebook = FALSE;
			    cout<<" not using book"<<endl;
			}


         return;
     }
     Option = strstr(string,"KingSafety ");
     if(Option != NULL)//we've had a hash option
     {
         delete TTable;
         TTable = NULL;

         Option += 17;
         i = atoi(Option);
        eo->kingsafety = i;
        fprintf(log_file, "\nKing safety adjusted to %d\n", i);
        fflush(log_file);
        cout<<"\nKing safety adjusted to "<<i<<endl;
        return;
     }
     Option = strstr(string,"PassedPawn ");
     if(Option != NULL)//we've had a hash option
     {
         delete TTable;
         TTable = NULL;

         Option += 17;
         i = atoi(Option);
        eo->passedpawn = i;
        fprintf(log_file, "\nPassedPawn adjusted to %d\n", i);
        fflush(log_file);
        cout<<"\nPassedPawn adjusted to "<<i<<endl;
        return;
     }
     Option = strstr(string,"PawnStructure ");
     if(Option != NULL)//we've had a hash option
     {
         delete TTable;
         TTable = NULL;

         Option += 20;
         i = atoi(Option);
        eo->pawnstructure = i;
        fprintf(log_file, "\nPawnStructure adjusted to %d\n", i);
        fflush(log_file);
        cout<<"\nPawnStructure adjusted to "<<i<<endl;
        return;
     }

}


