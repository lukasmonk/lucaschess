#include <iostream>

#include "Lime.h"
#include "data.h"
#include "prototypes.h"

#define ROOTLEGAL 0
#define ROOTILLEGAL 1
#define ILLEGALSCORE -32000

using namespace std;

/*
this sorts the root moves on the movelist by making each move, doing a quies()
search, and ordering for the corresponding score.
*/
void root_init()
{
    root_move_list();
    score_root_moves();
    root_sort();
}
/*
fill up the root move list array with the root moves, reset the array scores
and move to 0
*/
void root_move_list()
{
    //generate move at current ply
    movegen_legal();
   /* cout<<"\n Number Root moves: ";
    cout<<p->listc[p->ply+1];*/
}

/*
score the root list moves
*/
void score_root_moves()
{
    int i;
    int now = p->ply;

    for (i = p->listc[now]; i < p->listc[now+1]; ++i)
    {
        makelegalmove(&p->list[i]);
		p->list[i].score =  -quies(-10000,10000);
		takemove();
    }
}

void root_sort()
{
      int i, j;    // set flag to 1 to begin initial pass
      s_Move temp;             // holding variable
      int now = p->ply;
      for(i = p->listc[now]; i < p->listc[now+1]; ++i)
      {
          for (j = p->listc[now]; j < (p->listc[now+1]-1); ++j)
          {
              if (p->list[j+1].score > p->list[j].score)      // ascending order simply changes to <
              {
                    temp = p->list[j];             // swap score
                    p->list[j] = p->list[j+1];
                    p->list[j+1] = temp;
              }
          }
      }

    /* cout<<"\n\n Sorted list :\n";
     for(i = p->listc[now]; i < p->listc[now+1]; ++i)
     {
      cout<<"\n p->list : "<<returnmove(p->list[i]);
      cout<<" score : "<<p->list[i].score;
     }
     cout<<endl;*/
     return;
}

/*
searching from the root
*/
int    root_search(int alpha, int beta, int depth)
{
    int i, j, score = 0;
    int inc = 0;
    int oalpha = alpha;
    pvindex[p->ply] = p->ply;

    order(&nomove);
    //initialise some of the variables used during the execution of the moves -
	int played = 0;// no. moves we have played
    int bestscore = -10001;
    s_Move bestmove;

    // loop through the moves
	for (i = p->listc[p->ply]; i < p->listc[p->ply+1]; ++i)
	{
		pick(i);

		makelegalmove(&p->list[i]);

		if(isattacked(p->k[p->side], p->side^1))
		{
		inc = PLY;
		}

		played++;

		if(played == 1)
		{
		    score = -search(-beta, -alpha, depth - PLY + inc, TRUE);
		}
		else
		{
		    score = -search(-alpha-1, -alpha, depth - PLY + inc, TRUE);
		    pvs++;
		    if ((score > alpha) && (score < beta))
		    {
		        pvsh++;
		        score = -search(-beta, -alpha, depth - PLY + inc, TRUE);
		    }
		}

		takemove();
		inc = 0;

		if(stopsearch) {return 0;}

		if(score > bestscore)
        {
          bestscore = score;
          bestmove = p->list[i];

         if (score > alpha)
		 {
			alpha = score;

		    // update the PV
			pv[p->ply][p->ply] = p->list[i];
			for (j = p->ply + 1; j < pvindex[p->ply + 1]; ++j)
				pv[p->ply][j] = pv[p->ply + 1][j];
			pvindex[p->ply] = pvindex[p->ply + 1];

		    //now check for a fail high - >= beta.
			if (score >= beta)
			{
			    if(played==1) {fhf++;}
			    fh++;
			    return score;
			}

		 }
        }
	}

	if (played == 0)
	{
		if (inc)
		{
		    return -10000 + p->ply;
		}
		else
		{
			return 0;
		}
	}

    return bestscore;
}




