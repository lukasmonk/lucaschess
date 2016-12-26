#include <iostream>

#include "Lime.h"
#include "data.h"
#include "prototypes.h"

using namespace std;




/*
The hash table is declared in init_hash_tables() - it is a finite amount of
memory, and pointed to by *TTable. It is declared using the keyword 'new',
size determined by numelem, which is defined on program startup, and can be
changed as a uci option
*/
void init_hash_tables()
{
  //get the size of a hash element
  int elemsize = sizeof(s_Hashelem);

  cout<<"\n elem size = "<<elemsize<<endl;

  //convert numelem (the number of MB required) into a number of
  //bytes required.
  numelem *= 1000000;

  //divide numelem by the elemsize to get the number of elements we will have in
  //our table
  numelem /= elemsize;

  cout<<"\nnumelem = "<<numelem;

  //now point the *TTable pointer to a newly declared table
  TTable = new s_Hashelem[numelem];

  //error check
  if (TTable == NULL){cout<<"\nerror assigning thash";}

  //print out the size in kB
  double tt = _msize(TTable);
  cout<<"\nHash size "<<(tt)/1000<<" kB"<<endl;

  //reset the hash by calling clearhash();
  clearhash();
}

//reset all of the keys in the table to 0;
void clearhash()
{
     //make a pointer to a hash element
     s_Hashelem *probe;

     //now loop through the hash elements one by one, pointing *probe to
     //each element, and setting the key to 0
     for (int i = 0; i < numelem; ++i)
     {
       probe =  TTable + i;//point at alement
       probe->hashkey = 0;//set key to 0
       probe->depth = 0;//set key to 0
       probe->score = 0;//set key to 0
       probe->flag = 0;//set key to 0
     }
   /* cout<<"\n checking hashclear values";
     for (int i = 0; i < numelem; ++i)
     {
        cout<<"\nelement "<<i;
        probe =  TTable + i;
        cout<<" key "<<probe->hashkey;
        cout<<" depth "<<probe->depth;
        cout<<" score "<<probe->score;
     }*/
}



/*
generate a full hashkey for the current position

this is done using the tables of 64 bit random numbers, defined in data.cpp

 hash_p[14][144];//piece/square
 hash_s[2];//side
 hash_ca[16];
 hash_enp[144];//one for each square

 the function cycles throught the elemnts of the board, Xoring (^ operator) with
 the relevant array of random numbers
 e.g. wP on A2

  key ^= hash_p[wP][A2];
*/
void    fullhashkey()
{

        p->hashkey = 0;
        register int i;
        //cycle through the board
        for (i = 0; i < 144; ++i)
        {
            //if empty, ignore
            if (p->board[i].typ == edge || p->board[i].typ == ety)
            {continue;}

            p->hashkey ^=  hash_p[p->board[i].typ][i];
        }

        //now Xor the side, castleflags and en_passant square
        p->hashkey ^= hash_s[p->side];
        p->hashkey ^= hash_enp[p->en_pas];
        p->hashkey ^= hash_ca[p->castleflags];

}

/*
testhashkey is a function which generates a full key from the current position
and compares it with the current key. It spits out a message if there is
a difference. I used this function in the search and makemove when making
changes to make sure the key was Ok.
*/
void    testhashkey()
{
        unsigned __int64 hashkey = 0;
        register int i;
        for (i = 0; i < 144; ++i)
        {
            if (p->board[i].typ == edge || p->board[i].typ == ety) {continue;}

            hashkey ^=  hash_p[p->board[i].typ][i];
        }

        hashkey ^= hash_s[p->side];
        hashkey ^= hash_enp[p->en_pas];
        hashkey ^= hash_ca[p->castleflags];

        if(hashkey != p->hashkey)
        {
         cout<<"\ncorrupt key";
         cout<<" board = "<<p->hashkey;
         cout<<" should be "<<hashkey<<endl;
         printboard();
        }
}


int probe_hash_table(int depth, s_Move *move, int *null, int *score, int beta)
{
    hashprobe++;//stats

    s_Hashelem *probe2;

    int flag = NOFLAG;

     probe2 = TTable + (p->hashkey % numelem);//find hash position
     /*
     if ((probe2->flag == UPPER) && (probe2->score < beta))
	*null = 0;
	*/
    if (probe2->hashkey == p->hashkey)
    {
        move->m = probe2->move;
        hashhit++;//stats
        *score = probe2->score; //set the score

        if(probe2->depth >= depth )
        {
         flag = probe2->flag; //set the flag
         *null = probe2->null; //set the nul permission
         return flag; //return the flag
        }
    }
    return flag;//will be NOFLAG if we reached here
}


void store_hash(int depth, int score, int flag, bool null, s_Move *move)
{
     //make a pointer to assign to point at a table element
     s_Hashelem *key;
     key = TTable + (p->hashkey % numelem);

     //update the key information
     if(depth >= key->depth)
     {
      key->hashkey = p->hashkey;
      key->depth = depth;
      key->score = score;
      key->flag = flag;
      key->null = null;
      key->move = move->m;
     }
}

/*
int probe_hash_table(int depth, s_Move *move, int *null, int *score)
{
    hashprobe++;//stats

    //make a pointer to be assigned to our element
    s_Hashelem *probe2;

    //set flag to NOFLAG
    int flag = NOFLAG;

    //probe the table using the modulo operator, assigning our pointer to
    //an element
    probe2 = TTable + (p->hashkey % numelem);//find hash position

    //the element key must match our key......
    if (probe2->hashkey == p->hashkey)
    {
        //keys match, so update the score and move
        move->m = probe2->move;
        hashhit++;//stats

        //if the depth is >= element depth, we can use the hash info
        if(probe2->depth >= depth )
        {
         flag = probe2->flag; //set the flag
         *null = probe2->null; //set the nul permission
         *score = probe2->score; //set the score
         if(abs(*score>(10000-200)))
         {
             if(*score>0) {*score -= p->ply;}
             else {*score += p->ply;}
         }
         assert(flag>=1&&flag<=3);
         return flag; //return the flag
        }
    }
    assert(flag==0);
    return flag;//will be NOFLAG if we reached here
}
*/


