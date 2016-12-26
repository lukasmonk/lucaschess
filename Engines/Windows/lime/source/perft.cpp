#include <iostream>

#include "Lime.h"
#include "data.h"
#include "prototypes.h"

//#define DEBUGKEY

using namespace std;

long pnodes,actnodes, pdepth;
bool target;
void go(int depth);
void goroot(int depth);
void   goshow(int depth);
long rootnodes[100];


void perft(int depth)
{
     pnodes = 0; actnodes = 0;
     p->ply = 0;
     float s = double(clock());
     pdepth = depth;
     memset(rootnodes, 0, sizeof(rootnodes));
     goroot(depth);
     float f = double(clock());

     cout<<"\n pnodes = "<<pnodes;
     cout<<"\nactnodes = "<<actnodes;
     cout<<"\n time = "<<(f-s)/1000<<"s";

}

void   go(int depth)
{
       if (depth == 0) return;
       movegen();

       //capgen(game);
       for (int i = p->listc[p->ply]; i < p->listc[p->ply+1]; ++i)
       {


         if (makemove(&p->list[i]))
           {
              takemove();
               continue;
           }
           #ifdef DEBUGKEY
           testhashkey();
           #endif
           ++pnodes;

           if (pdepth == p->ply) {++actnodes;}
           go(depth-1);
           takemove();
       }
       return;
}


void   goshow(int depth)
{
       if (depth == 0) return;
       movegen();
       //capgen(game);
       for (int i = p->listc[p->ply]; i < p->listc[p->ply+1]; ++i)
       {

         if (makemove(&p->list[i]))
           {
              takemove();
               continue;
           }
           cout<<"\nmaking "<<returnmove(p->list[i]);
           #ifdef DEBUGKEY
           testhashkey();
           #endif
           ++pnodes;

           if (pdepth == p->ply) {++actnodes;}
           go(depth-1);
           takemove();
       }
       return;
}

void   goroot(int depth)
{
       if (depth == 0) return;
       movegen();
       int oldnodes = 0;

       //capgen(game);
       for (int i = p->listc[p->ply]; i < p->listc[p->ply+1]; ++i)
       {


         if (makemove(&p->list[i]))
           {
              takemove();
              continue;
           }

           #ifdef DEBUGKEY
           testhashkey();
           #endif
           ++pnodes;
           if (pdepth == p->ply) {++actnodes;}

           go(depth-1);

           takemove();

           rootnodes[i] = actnodes-oldnodes;
           cout<<returnmove(p->list[i]);
           cout<<" "<<rootnodes[i]<<endl;
           oldnodes = actnodes;
       }
       return;
}

void perftfile()
{
    FILE *testfile;
    char *finddepth;
    char filename[64];
    char fileline[512];
    long x;
    vector <long> myscore;
    vector <long> targetscore;
    int tests = 0;
    cout<<"\n1. This function will always test to depth 6!!\n";
    cout<<"\n the perft file must have the target number of nodes at the end of\n";
    cout<<" the fen in the following format:  'D6 1888900'. The program looks for 'D6'";
    cout<<endl<<endl;
    cout<<"\nEnter filename..";
    cin>>filename;

    testfile = fopen(filename, "r");

    while (fgets(fileline, 512, testfile))
    {
        setboard(fileline);
        //printboard();
        finddepth = strstr(fileline, "D5");
        x = atol(finddepth+3);
        targetscore.push_back(x);
        perft(5);
        myscore.push_back(actnodes);
        cout<<" position "<<tests+1<<" target = "<<x;
        cout<<" actual = "<<actnodes<<endl;
        tests++;
    }

    cout<<endl<<endl;
    cout<<" Test complete.";



    for (int i = 0; i < tests; ++i)
    {
        cout<<"\nPosition "<<i+1;
        cout<<setw(20)<<targetscore[i];
        cout<<setw(20)<<myscore[i];

        if(targetscore[i] == myscore[i])
        {
            cout<<setw(5)<<" PASS ";
        }
        else
        {
            cout<<setw(5)<<" FAIL ";
        }
        cout<<"\n";
    }




}



