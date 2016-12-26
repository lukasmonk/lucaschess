#include <iostream>

#include "Lime.h"
#include "data.h"
#include "prototypes.h"



using namespace std;


/*
the search algorithm is a standard alpha beta with null move, pvs search,
hash table cut offs. All of this was learnt from Bruce Moreland's brilliant
site... http://www.seanet.com/~brucemo/topics/topics.htm
*/

//the last argument is the permission to do a null move on this ply
int    search(int alpha, int beta, int depth, int null)
{
    int i, j, score = 0, hashscore = 0, hflag = NOFLAG;
    int inc = 0;
    s_Move hashmove = nomove;
    int old_alpha = alpha;


	 if (((nodes+qnodes) & 2047) == 0)
		checkup();

	 if(stopsearch) return 0;

    if (p->ply && isrep())
		return 0;

	if (p->ply > 31)
    return gameeval();

    ++nodes;
    pvindex[p->ply] = p->ply;

    hflag = probe_hash_table(depth, &hashmove, &null, &hashscore, beta);
         //see if the hash flag was set, if so, look for a cut off
       switch(hflag)
         {
                   case EXACT:
                        return (hashscore);
                   case UPPER:
                        if (hashscore <= alpha)
	                    return hashscore;
      	                break;
                   case LOWER:
                        if (hashscore >= beta)
	                    return hashscore;
                        break;

                   default:
                            break;
         }

   //initialise our extension variable
	int extend = 0;

	//see if we are in check
    inc = isattacked(p->k[p->side], p->side^1);

    //back up the followpv variable in ase it gets fouled up in null move
    int opv = followpv;

    //if in check,extend by one PLY, and set the historical check[] array
    //to 1 for this ply.
	if (inc)
	{
		extend=PLY;
		check[p->ply]=1;
		incheckext++;
	}
	else
	{
	    check[p->ply]=0;//we weren't in check

	    if(p->pcenum > 4 && null && depth > PLY && !followpv)
	    {
	        int tep = p->en_pas;

            //make hash changes, changes the side
	        p->hashkey ^= hash_s[p->side];
            p->hashkey ^= hash_enp[p->en_pas];
            p->en_pas = noenpas;
            p->side ^= 1;
            p->hashkey ^= hash_s[p->side];
            p->hashkey ^= hash_enp[p->en_pas];
            int ns;

            //use a reduction of depth-4 or depth-3 depending on the remaining
            //depth
            if(depth > 7)
            {
            ns = -search(-beta, -beta+1, depth - 4*PLY, FALSE);
            }
            else
            {
             ns = -search(-beta, -beta+1, depth - 3*PLY, FALSE);
            }

           followpv = opv;

            p->hashkey ^= hash_s[p->side];
            p->hashkey ^= hash_enp[p->en_pas];
            p->en_pas = tep;
            p->side ^= 1;
            p->hashkey ^= hash_s[p->side];
            p->hashkey ^= hash_enp[p->en_pas];
            if(stopsearch) {return 0;}

           // testhashkey();

            if(ns >= beta)
            {
               //  store_hash(depth, score, LOWER, null, &nomove);
			      return beta;
            }
            if(ns<-9900)
            {
                //mate threat extension - if ns was a mate score, then we are
                //potentially just a few moves from mate if we make an error,
                //so extend the search. This doesn't happen very often
                extend=PLY;
            }
	    }
	}

    if (depth < PLY && !extend)
	{
	  return quies(alpha,beta);
    }

    if(!hashmove.m) hashmove = pv[p->ply][0];

    //generate the legal moves for this position
	movegen();

    //order the moves (see sort.cpp)
	order(&hashmove);

	//initialise some of the variables used during the execution of the moves -
	int played = 0;// no. moves we have played
	int nd = 0; // new depth, assigned by extradepth() (see sort.cpp)
	int iend = p->listc[p->ply+1]-1;//the location of the end of the list of
	                                //moves, used for single reply extension
    s_Move bestmove = nomove;
    int bestscore = -10001;
    int h;


	// loop through the moves
	for (i = p->listc[p->ply]; i < p->listc[p->ply+1]; ++i)
	{
		pick(i);

		if (makemove(&p->list[i]))
		{
		    takemove();
			continue;
		}
		played++;

        if( (i == iend) && played == 1)
        {
            extend = PLY;
            single++;
        }

        if(!extend)
        {
          nd = extradepth(&p->list[i]);
        }

        if(played == 1)
		 {
		    score = -search(-beta, -alpha, depth - PLY + extend + nd, TRUE);
		 }
		 else
		 {
		    score = -search(-alpha-1, -alpha, depth - PLY + extend + nd, TRUE);
		    pvs++;
		    if ((score > alpha) && (score < beta))
		    {
		        pvsh++;
		        score = -search(-beta, -alpha, depth - PLY + extend + nd, TRUE);
		    }
		 }

		takemove();

     	if(stopsearch) {return 0;}

        if(score > bestscore)
        {
          bestscore = score;
          bestmove = p->list[i];

         if (score > alpha)
		 {
			alpha = score;
            //incerement the history table with the good move
		    update_history(p->list[i],depth);
            //now check for a fail high - >= beta.
			if (score >= beta)
			{
			    if(played==1) {fhf++;}
			    fh++;
			    //update the killers for move ordering
			    update_killers(p->list[i], score);
			    //store the hash with a lower flag, as we have a fail high
			    store_hash(depth, score, LOWER, null, &bestmove);
			    return beta;
			}


			// update the PV
			pv[p->ply][p->ply] = p->list[i];
			for (j = p->ply + 1; j < pvindex[p->ply + 1]; ++j)
				pv[p->ply][j] = pv[p->ply + 1][j];
			pvindex[p->ply] = pvindex[p->ply + 1];
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

    if(alpha>old_alpha)
    {
    store_hash(depth, bestscore, EXACT, null, &bestmove);
    }
    else
    store_hash(depth, alpha, UPPER, null, &nomove);

	return alpha;


}

int    firstquies(int alpha, int beta)
{
	return alpha;
}


int   quies(int alpha, int beta)
{
   int i, j, score;

   if (((nodes+qnodes) & 2047) == 0)
		checkup();

	if(stopsearch) return 0;

     ++qnodes;

	if (p->ply > 31)
		return gameeval();

    pvindex[p->ply] = p->ply;
	score = gameeval();
	if (score >= beta)
		return beta;
	if (score > alpha)
		alpha = score;

/*	int inc = isattacked(p->k[p->side], p->side^1);
    if(inc)
    {
       movegen_legal();

       if(p->listc[p->ply+1]-p->listc[p->ply]==0)
       {return -10000 + p->ply;}
       order(&nomove);
    }
    else
    {
      capgen();
      qorder(&nomove);
    }*/

    capgen();
    qorder(&nomove);

	/* loop through the moves */
	for (i = p->listc[p->ply]; i < p->listc[p->ply+1]; ++i)
	{
		pick(i);
		//if(!inc&&p->list[i].score==0) continue;

		if (makemove(&p->list[i]))
		{
			takemove();
			continue;
		}

		score = -quies(-beta, -alpha);
		takemove();
		if(stopsearch) return 0;
		if (score > alpha)
		{
            alpha = score;
			if (score >= beta)
			{
				return beta;
			}
			/* update the PV */
			pv[p->ply][p->ply] = p->list[i];
			for (j = p->ply + 1; j < pvindex[p->ply + 1]; ++j)
				pv[p->ply][j] = pv[p->ply + 1][j];
			pvindex[p->ply] = pvindex[p->ply + 1];
		}
	}

	return alpha;
}

void pick(int from)
{
	int i;
	int bs;
	int bi;
	s_Move g;

	bs = -1000000;
	bi = from;
	for (i = from; i < p->listc[p->ply+1]; ++i)
		if ( p->list[i].score > bs)
        {
			bs =  p->list[i].score;
			bi = i;
		}
	 g =  p->list[from];
	 p->list[from] =  p->list[bi];
	 p->list[bi] = g;
}


int isrep()
{
    register int i;
    if(p->fifty > 101)
    {
        return 1;
    }


   for (i = 0; i < histply; ++i)
	{
	   	if (hist[i].hashkey == p->hashkey)
		{
		  return 1;
		}
    }
	return 0;
}

