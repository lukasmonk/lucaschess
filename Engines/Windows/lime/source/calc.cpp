#include <iostream>


#include "Lime.h"
#include "data.h"
#include "prototypes.h"

using namespace std;

void calc()
{

      //init some variables
      int score = 0;
      int loopdepth = 0;
      int l;
      //search for a book move
      int bk = -1;
      if(!searchparam->inf && searchparam->usebook)
      {
       bk = wfindhashbookmove();
       if(bk!=-1)
       {
        best = p->list[bk];
        cout<<"\ntellothers Book move "<<returnmove(p->list[bk]);
        return;
        }
      }
      //reset ply to 0
      p->ply = 0;

      //reset the pondermove
      pondermove = nomove;

      if(searchparam->depth == -1)
      {loopdepth = 32;}
      else
      {loopdepth = searchparam->depth;}
      initsearch();
      root_init();
      int lastscore = 0;

      //one legal move go to just depth 5
      if(p->listc[p->ply+1] - p->listc[p->ply] == 1)
      {
          loopdepth = 5;
      }
       //now start the iterative deepening loop, running from 1 to loopdepth
      for (itdepth = 1; itdepth <= loopdepth; ++itdepth)
      {
        followpv = TRUE;
        score = root_search(-10000, 10000, itdepth*PLY);
        best = pv[0][0];
        pondermove = pv[0][1];

        if(timeup()) { return; }

        lastscore = score;

        if(itdepth > 3)
        printpv(score);

        for(l = 0; l < 48; ++l){
         killerscore[l] = -10000;
         killerscore2[l] = -10000;
        }

        // now call time_check() to see if we have time to do the next ply.
        //if no time, stopsearch will be set to TRUE, and so the next loop will
        //return immediately
        if(time_check()==TRUE)
        {stopsearch = TRUE;}
       // if(itdepth > 6 && abs(score) > 9900)
       // {stopsearch = TRUE;}

      }//next itloop depth


}

/*
if stopsearch is true, we need to return to our UCI loop
*/
int timeup()
{
    if (stopsearch)
         {
           return 1;
         }
  return 0;
}

/*
check to see whether we an complete the next ply, assuming it will take
twice as long as the last one, which is a very optimistic view of Lime's
branch factor
*/
int time_check()
{
    //if depth < 6, don't do the check - we shoud hit 7 quite quickly.
    if (itdepth < 6) {return FALSE;}

    //get the time now in ms.
    double timenow = double(clock());

    //get the time to search to the last ply
    double timetolastply = timenow - searchparam->starttime;

    //estimate the timefor the next ply
    double timefornextply = timetolastply*2;//optimistic!

     //and see if there is no point in continuing (return TRUE)
     if ((timefornextply + timenow) > searchparam->stoptime)
     {
        return TRUE;
     }

     //otherwise, we have time to do the next ply, so continue
     return FALSE;
}

/*
looks for a move from the hash if we don't have one in the pv - this function
was originally called in calc(), but is now called in think() in uci.cpp.

I'm not sure it's correct, but it works like this...

Make the best move found, and then probe the hash table. If we get a hit,
store this move as a ponder move. If we still don't get a move, then
things are a bit desperate, and odd. This time we make certain by generating and
ordering the moves, and selecting the first one on the list.
*/
s_Move findhashmove(s_Move m)
{
    //variables to send to the hash probe - we're not in a real search
    //so they're fake
    int fakedepth = 0, fakenull = 0;
    int fakescore = 0;
    int fbeta=0;

    //the current hashmove is a null move
    s_Move hashmove = nomove;

    //make the current best move so we're in a position to probe
    makemove(&m);

    //probe the table
    fakescore = probe_hash_table(fakedepth, &hashmove, &fakenull, &fakescore, fbeta);

    //if the move probed does not make sense, we're desperate for a move,
    //so gen the moves, and pick the highest one from the ordering,
    //making sure to check it is legal
    if(nopvmove(returnmove(hashmove)))//no move from hash, either
    {
        movegen();
        order(&nomove);
        for (int i = p->listc[p->ply]; i < p->listc[p->ply+1]; ++i)
	    {
		 pick(i);
		 if (makemove(&p->list[i]))
		 {
		    takemove();
			continue;
		 }
		 else
		 {
          pick(p->listc[p->ply]);
          hashmove = p->list[i];
          break;
		 }
	    }
    }
     //takeback the move we made before the probe
    takemove();

    //return our (hopefully) accurate hashmove
    return hashmove;
}

/*
intialise the search variables - this function sets up a search by resetting all
counters to 0, or their equivalent
*/
void initsearch()
{
    //we aren't stopping yet...
    stopsearch = FALSE;

    //and our nomove is just that - nothing. This is a global that gets set to
    //the same thing every time. Really, it should be set once at program boot.
    nomove.m = 0;

    int l,j,k;

    //the following are all indexed in the search by p->ply. Note the storage
    //space is to 48 ply, but the seacrh max ply is set to 32 ply. It's
    //purely for safety.
    for(l = 0; l < 48; ++l)
    {
       killerscore[l] = -10000;
       killerscore2[l] = -10000;//reset the killer scores
       killer1[l] = nomove;
       killer2[l] = nomove;
       pvindex[l] = 0;//reset the pv index
       check[l] = 0;//reset the incheck array
       red[l] = 0;//reset the reduction array
        for(j = 0; j < 48; ++j)
        {

            pv[l][j] = nomove;//reset the pv to nomove
        }
    }
    //reset the history table to 0
    for(k = 0; k < 2; ++k)
    {
    for(j = 0; j < 144; ++j)
    {
        for(l = 0; l < 144; ++l)
        {
           history[j][l] = 0;
           hisall[k][j][l] = 8192;
           hisfh[k][j][l] = 8192;
        }
    }
    }

    for(k = 0; k < MOVEBITS; ++k)
    {
        his_table[k] = 0;
    }
    //now reset the stats collected during the search
    nodes = 0;
    qnodes = 0;
    fh = 0;
    fhf = 0;
    pvs = 0;
    pvsh = 0;
    nulltry = 0;
    nullcut = 0;
    hashprobe = 0;
    hashhit = 0;
    incheckext = 0;
    wasincheck = 0;
    matethrt = 0;
    pawnfifth = 0;
    pawnsix = 0;
    prom = 0;
    reduct = 0;
    single = 0;
    resethis = 0;

}






