#include <iostream>

#include "Lime.h"
#include "data.h"
#include "prototypes.h"
#include "psqt.h"

#define	HASH	65536
#define	M_KILLER	65526
#define	WIN_CAPT1	65516
#define	WIN_CAPT2	65506
#define	WIN_CAPT3	65496
#define	Q_PROM_CAPT	65486
#define	Q_PROM	65476
#define	GCAP_QQ	65466
#define	GCAP_RR	65456
#define	GCAP_NN	65446
#define	GCAP_BB	65436
#define	GCAP_PP	65426
#define	SEECAP	65416
#define	KILLER1	65406
#define	KILLER1_PLY	65396
#define	KILLER2	65376
#define	KILLER2_PLY	65366
#define	OO	65356
#define	OOO	65346
#define	MINORPROM	65336



using namespace std;

const int equalcap[16] = {0,GCAP_PP,GCAP_PP,GCAP_NN,GCAP_NN,GCAP_BB,GCAP_BB,GCAP_RR,GCAP_RR,GCAP_QQ,GCAP_QQ,10000,10000,0,0,0};

const int *tab;
int pause;

void update_history(s_Move move, int depth)
{

int h;
  his_table[move.m&MOVEBITS] += (depth/PLY);

    if(his_table[move.m&MOVEBITS] > MOVEBITS)
    {
        for(h = 0; h < MOVEBITS; ++h)
        {
           his_table[h] = his_table[h]>>1;
        }
    }

}

void update_killers (s_Move move, int score)
{

  if(score > 10000-p->ply)
  {
   matekiller[p->ply] = move;
   return;
  }
  if( !(move.m&mCAP) )
  {
     if(move.m != killer1[p->ply].m)
     {
     killer2[p->ply] = killer1[p->ply];
     killer1[p->ply] = move;
     }
     else if(move.m != killer2[p->ply].m)
      {
      killer2[p->ply] = move;
      }
  }
}

//if no killer found, return 0
int score_killer(s_Move *m)
{
    int from = FROM(m->m);
	int to = TO(m->m);

    if(from == FROM(killer1[p->ply].m) && to == TO(killer1[p->ply].m))
	{
	   m->score = KILLER1;
	   return 1;
	}
    else if(from == FROM(killer1[p->ply-1].m) && to == TO(killer1[p->ply-1].m) && p->ply)
	{
	   m->score = KILLER1_PLY;
	   return 1;
	}
	else if(from == FROM(killer2[p->ply].m) && to == TO(killer2[p->ply].m))
	{
	   m->score = KILLER2;
	   return 1;
	}
	else if(from == FROM(killer2[p->ply-1].m) && to == TO(killer2[p->ply-1].m) && p->ply)
	{
	   m->score = KILLER2_PLY;
	   return 1;
	}
	return 0;
}

void score_ca(s_Move *m)
{
    m->score = OO;
}


void score_prom(s_Move *m)
{
  if(m->m&oPQ)//queen
      {
          m->score = Q_PROM;
      }
      else
      {
          m->score = MINORPROM;
      }
}

void score_capture(s_Move *m)
{
  int from,to,val;
  if(m->m&mProm)//promotion
  {
      if(m->m&oPQ)//queen
      {
          m->score = Q_PROM_CAPT;
      }
      else
      {
          m->score = MINORPROM;
      }
  }
  else//not a prom
  {
    from = FROM(m->m);
	to = TO(m->m);
	val = vals[p->board[TO(m->m)].typ] - vals[p->board[FROM(m->m)].typ];
	if(val>=600)
	{
          m->score = WIN_CAPT1;
    }
    else if(val>=400)
	{
          m->score = WIN_CAPT2;
    }
    else if(val>=200)
    {
          m->score = WIN_CAPT3;
    }
    else if(val == 0)
    {
        m->score = equalcap[p->board[FROM(m->m)].typ];
    }
    else
    {
        if((isattacked(to,p->side^1)))
        {
           m->score = 0;
        }
        else
        {
           m->score = SEECAP;
        }
    }
  }

}


int order(s_Move *hm)
{
 int i,from,to;

 if(followpv)
 {
	followpv = FALSE;
	for(i = p->listc[p->ply]; i < p->listc[p->ply+1]; ++i)
	{
		from = FROM(p->list[i].m);
		to = TO(p->list[i].m);

		if (from == FROM(pv[0][p->ply].m) && to == TO(pv[0][p->ply].m))
		{
			followpv = TRUE;
			p->list[i].score = HASH;
		}
		else if(from == FROM(hm->m) && to == TO(hm->m))
		{
		    p->list[i].score = HASH;
		}
		else if(from == FROM(matekiller[p->ply].m) && to == TO(matekiller[p->ply].m))
		{
		    p->list[i].score = M_KILLER;
		}
		else if(p->list[i].m&mCAP)
		{
		    score_capture(&p->list[i]);
		}
		else if(p->list[i].m&mProm)
		{
		    score_prom(&p->list[i]);
		}
		else if(p->list[i].m&mCA)
		{
		    score_ca(&p->list[i]);
		}
		else if (!score_killer(&p->list[i]))
		{
		   p->list[i].score = his_table[p->list[i].m&MOVEBITS];
           if(p->majors > 4)
           {tab = midtab[p->board[FROM(p->list[i].m)].typ];}
           else
           {tab = endtab[p->board[FROM(p->list[i].m)].typ];}
            p->list[i].score+=tab[TO(p->list[i].m)]-tab[FROM(p->list[i].m)];
		}
	}
 }//if followpv
 else
 {
    for(i = p->listc[p->ply]; i < p->listc[p->ply+1]; ++i)
	{
		from = FROM(p->list[i].m);
		to = TO(p->list[i].m);

		if(from == FROM(hm->m) && to == TO(hm->m))
		{
		    p->list[i].score = HASH;
		}
		else if(from == FROM(matekiller[p->ply].m) && to == TO(matekiller[p->ply].m))
		{
		    p->list[i].score = M_KILLER;
		}
		else if(p->list[i].m&mCAP)
		{
		    score_capture(&p->list[i]);
		}
		else if(p->list[i].m&mProm)
		{
		    score_prom(&p->list[i]);
		}
		else if(p->list[i].m&mCA)
		{
		    score_ca(&p->list[i]);
		}
		else if (!score_killer(&p->list[i]))
		{
		   p->list[i].score = his_table[p->list[i].m&MOVEBITS];
           if(p->majors > 4)
           {tab = midtab[p->board[FROM(p->list[i].m)].typ];}
           else
           {tab = endtab[p->board[FROM(p->list[i].m)].typ];}
            p->list[i].score+=tab[TO(p->list[i].m)]-tab[FROM(p->list[i].m)];
		}
	}
 }

 return 1;
}

int qorder(s_Move *hm)
{
 int i,from,to,val;
 for(i = p->listc[p->ply]; i < p->listc[p->ply+1]; ++i)
	{
		from = FROM(p->list[i].m);
		to = TO(p->list[i].m);
		p->list[i].score = 0;

		if(p->list[i].m == pv[p->ply][0].m)
		{
		p->list[i].score = HASH;
		}
		else
		{
		    val = vals[p->board[to].typ] - vals[p->board[from].typ];
		    p->list[i].score = 10000 + val;

		    if (val<0)
		    {
		        if(isattacked(to,p->side^1))
		        p->list[i].score = 0;
		    }
		}
	}
/*
	printboard();
	for(i = p->listc[p->ply]; i < p->listc[p->ply+1]; ++i)
	{
       cout<<endl<<returnmove(p->list[i])<<" : "<<p->list[i].score;
	}
	cin>>from;
*/
}

int extradepth(s_Move *m)
{
    int nd = 0;
    int to = TO(m->m);

    //note, the incheck extension and mate threat has already been done
    if( (m->m)&oPQ)
    {
        assert((m->m)&mProm);
        nd+=48;
        prom++;
    }
    if(p->board[to].typ == wP)
    {
     if(ranks[to] == 6)
     {
        nd+=48;
        pawnsix++;
     }
     if(ranks[to] == 5)
     {
        nd+=24;
        pawnsix++;
     }
    }
    if(p->board[to].typ == bP)
    {
     if(ranks[to] == 1)
     {
        nd+=48;
        pawnsix++;
     }
     if(ranks[to] == 2)
     {
        nd+=24;
        pawnsix++;
     }
    }
    if(nd>PLY) nd = PLY;
    return nd;

}






