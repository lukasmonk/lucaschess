#include <iostream>

#include "Lime.h"
#include "data.h"
#include "prototypes.h"


using namespace std;

void  openlog()
{
      log_file = fopen("log_v66_norm.txt", "a");
}

void closelog()
{
	if (log_file)
		fclose(log_file);
	log_file = NULL;
}


void  writeply(int ply)
{
      fprintf(log_file, " ply %d;", ply);
}

void writemove(s_Move our)
{
     char *move = returnmove(our);

     fprintf(log_file, " move %d %s ", move);
}


void writesq(int our)
{
     char *sq = returnsquare(our);

     fprintf(log_file, "%s", sq);
}
void writegamehist()
{
     char *move;
     fprintf(log_file, " \ngame history ");
     for (int i = 0; i < histply-1; ++i)
     {
         fprintf(log_file, " move %d %s ", i, move);
     }
     //fprintf(log_file, " \n ");
}

void writescore(int score)
{
     fprintf(log_file, " score %d;", score);
}

void writefpv(bool fpv)
{
     if (fpv)
     {
        fprintf(log_file, " following pv ");
     }
     else
     {
        fprintf(log_file, " not pv ");
     }
}

void writestring(const char *s)
{
     fprintf(log_file, "%s", s);
}
void writeint(int i)
{
     fprintf(log_file, "%d", i);
}

void writedouble(double i)
{
     fprintf(log_file, "%f", i);
}
void writespace()
{
     fprintf(log_file, " ");
}
void writenewline()
{
     fprintf(log_file, "\n");
}

void writeboard()
{
     fprintf(log_file, "\nprinting board..\n\n");
     for (int r = 7; r >= 0; r--)
     {
         fprintf(log_file, "\n");
         for (int f = 0; f < 8; ++f)
         {
             if (p->board[fileranktosquare(f, r)].typ != ety)
             {
              fprintf(log_file, " %c ", piecetochar[p->board[fileranktosquare(f, r)].typ]);
             }
             else
             {
                fprintf(log_file," . ");
             }

         }
     }

      fprintf(log_file, "\n side = %d", p->side);
      writestring("\n Castle Flags ");
      writestring(returncastle());
      writesq(p->en_pas);
      fprintf(log_file, "\n hashkey %X",p->hashkey);
      fprintf(log_file, "\n side = %d",colours[p->side]);
      fprintf(log_file, "\n majors = %d",p->majors);
      fprintf(log_file, "\n ply = %d",p->ply);
      fprintf(log_file, "\n wk ");
      writesq(p->k[white]);
      fprintf(log_file, "\n bk = ");
      writesq(p->k[black]);
      fprintf(log_file, "\n white material = %d",p->material[white]);
      fprintf(log_file, "\n black material = %d",p->material[black]);
      fprintf(log_file, "\n fifty = %d",p->fifty);
      fprintf(log_file, "\n\n");

}





