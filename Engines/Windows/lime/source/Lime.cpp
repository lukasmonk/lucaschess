#include <iostream>

#include "Lime.h"
#include "data.h"
#include "prototypes.h"
#include "psqt.h"

using namespace std;

int main()
{
  setbuf(stdout, NULL);
  setbuf(stdin, NULL);
  setvbuf(stdout, NULL, _IONBF, 0);
  setvbuf(stdin, NULL, _IONBF, 0);
  numelem = 32;
  init_castlebits();
  init_distancetable();
  init_hash_tables();
  logme = FALSE;
  openlog();
  eo->passedpawn = 176;
  eo->kingsafety = 128;
  eo->pawnstructure = 96;
  searchparam->xbmode = FALSE;
  searchparam->ucimode = FALSE;
  searchparam->ics = FALSE;
  searchparam->cpon = FALSE;

  //cout<<"\n mvebits "<<MOVEBITS<<endl;

   /*  debugorder();
     delete TTable;
     TTable = NULL;
     closelog();
     return 0;*/

    //closelog();
  //  exit(0);
    //char a;cin>>a;
    //return 0;




  /*  cout<<"\n 64 | 32 = "<<hex<<((64 | 32)<<16);
    cout<<"\n 128 | 32 = "<<hex<<((128 | 32)<<16);
    cout<<"\n 256 | 32 = "<<hex<<((256 | 32)<<16);
    cout<<"\n 512 | 32 = "<<hex<<((512 | 32)<<16);
    cout<<"\n 64 = "<<hex<<((64 )<<16);
    cout<<"\n 128  = "<<hex<<((128)<<16);
    cout<<"\n 256  = "<<hex<<((256)<<16);
    cout<<"\n 512  = "<<hex<<((512)<<16);*/

   /* cout<<"\n mPQ = "<<mPQ;
     cout<<"\n mPR = "<<mPR;
     cout<<"\n mPB = "<<mPB;
    cout<<"\n mPN = "<<mPN;*/
/*
char m[4];
    setboard(startfen);
    for(;;)
    {
        printboard();
        cout<<"\n move > ";
        cin>>m;
        understandmove(m);
    }*/
    //setboard(f1);
    //printnums();
    //debugpiecelists();
    //debugatt();
   // setboard(startfen);
   // perft(6);
   // perftfile();
   //debugevalsee();
  // debugeval();
  /*
  debugprofile();
   delete TTable;
    TTable = NULL;

   closelog();
    //closelog();
    exit(0);*/
 // debugnps();

  // uci_mode();
 // makehash();
 book_init();
  char command[512];
   for (;;)
    {
        cin>>command;
        if (!strcmp(command, "uci"))
        {
            uci_mode();
            break;
        }
        if (!strcmp(command, "xboard"))
        {
            xboard();
            break;
        }
        else if(!strcmp(command, "quit"))
        {
            break;
        }
        else
        {
            cout<<"\nunknown command "<<command;
            cout<<" use 'uci' or 'quit'";
        }
    }
    delete TTable;
    TTable = NULL;

   closelog();


    return 0;
}

