#include <iostream>

#include "Lime.h"
#include "data.h"
#include "prototypes.h"
#include "psqt.h"

#define beg 0
#define end 1


//#define DEBUG
/*
before reading this file, it should be noted that some of the scoring bonus
arrays are not used - this is because I am constantly updating here,
and trying new things out
*/

using namespace std;

void development();
void wrooktrapped(const int *sq);
void brooktrapped(const int *sq);
void whitepawnsstructure(const int *sq);
void blackpawnsstructure(const int *sq);
int whitekingsafety(const int *sq);
int blackkingsafety(const int *sq);
void pawnstorm();
void blockedpawn();
void bishoppair();
void bbishopmob(const int *sq);
void wbishopmob(const int *sq);
void wknightmob(const int *sq);
void bknightmob(const int *sq);
void wrookmob(const int *sq);
void brookmob(const int *sq);
void bqueenmob(const int *sq);
void wqueenmob(const int *sq);

int isdrawonep();
int wNsupport(const int *sq);
int bNsupport(const int *sq);
int brseven();
int wrseven();
void wpp(const int *sq);
void bpp(const int *sq);
void mymemset();
int isdrawnp();
int whitepawncover(const int file, const int rank);
int blackpawncover(const int file, const int rank);
int dowks();
int dobks();




//bonus for haing a piece block a passed pawn (indexed by piece type)
const int blockerbonus[16] = {0,0,0,15,15,12,12,7,7,2,2,0,0,0,0,0};

//penalty for number of undeveloped bishops and knights
const int developpenalty[16] = { 0, 5, 8, 12, 18, 25, 35, 47, 68, 80, 105, 130, 130, 130, 130, 130};

//the bonus for a bishop pair, depending on the number of pawns
//0 - 16 pawns means a 32 sized array
const int bishop_pair[32] =
{
    20,18,17,15,12,10,6,6,6,0,0,0,-5,-10,-15,-20,-20,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
};

//bonus for the rank a pawn sits on in a pawn storm
const int wstorm[8] = {0,-5,0,5,10,15,20,0};
const int bstorm[8] = {0,20,15,10,5,0,-5,0};

//penalty for pawn structure defects
const int pdefect[9] = {0, 0, 5, 10, 15, 20, 60, 60, 60};

//pawnbit assigned when a pawn sits on a given rank, eg
//a pawn on the 3rd rank will set the 4th bit (8)
const int pbit[8] = { 1,2,4,8,16,32,64,128 };

//passed pawn bonues by rank, midgame and endgame
const int mppawn[2][8] = {
    {0, 50, 30, 20, 10, 0,  0, 0},
	{0,  0, 0, 10, 20, 30, 50, 0}
};
const int eppawn[2][8] = {
    {0, 75, 60, 45, 30, 15,  5, 0},
	{0,  5, 15, 30, 45, 60, 75, 0}
};

//bonus for the king distance from a passed pawn, mid and endgame
const int kdmppawn[8] = {
	0, 150, 100, 50, 25, 10,  5, 0
};
const int kdeppawn[8] = {
	0, 200, 150, 80, 40, 25,  10, 0
};

//this is for the king safety - if the king is castled the pawn cover bonus
//is less severe for c and f files
const int kpdist[8] = { 0,0,10,20,26,31,35,38};

//trposim scores - distance the piece is from the enemy king
const int ntrop[8] =
{ 0, 10, 7, 4, 2, 0, 0, 0};
const int ptrop[8] =
{ 0, 15, 10, 6, 4, 0, 0, 0};
const int rtrop[8] =
{ 0, 8, 5, 2, 1, 0, 0, 0};
const int qtrop[8] =
{ 0, 6, 4, 2, 1, 0, 0, 0};

/*
 table for distance between squares calculation - returns the biggest difference
 between the files and ranks
*/
int distancetable[144][144];

void init_distancetable()
{
     /*
     this is called at start up, an fills the table array with the distances
     between two sqaures - the biggest distance either by file or rank
     */

     //set the table to 0
     memset(distancetable, 0, sizeof(distancetable));
     int ri, rj, fi, fj;
     for (int i = 0; i < 144; ++i)//first loop, square 1
     {
         //set the rank and file for this square 'i'
         ri = ranks[i];
         fi = files[i];
         for (int j = 0; j < 144; ++j)//second loop, sqaure 2
         {
          //set the rank and file for this square 'j'
          rj = ranks[j];
          fj = files[j];

          //now see which is greater - the difference in files or ranks,
          //and store this in the table
          distancetable[i][j] = (abs(ri-rj) > abs(fi-fj))? abs(ri-rj) : abs(fi-fj);
         }
     }

}


/*
initeval() - this is called at the start of eval to initialise the information
required from the position to evaluate things such as pawns structure,
rooks on seventh etc
*/

void initeval()
{
    int f;
    int index, sq;

    //first of all call mymemset() to set everything to 0 (or 7 in some cases)
    //used instead of memset(), because I don't trust memset(). :)
    mymemset();
  //  debuginiteval();

    /*
    a note on pawns - the pawn information is stored in 3 variables.

    1. pawnbits[colour] - this stores the bit for the rank the
    pawn sits on
    2. pawnset[colour][file+1] - this stores the rank the pawn sits on for file
    'file'. Note that 1 is added to the file, and the array is set for 9 files -
    this is so we can get away with looking left and right of the a and h files
    without reading out of the array bounds.

    The rank set in this array is the furthest rank back. Initially, all ranks
    for white are set to 7, and then if a pawn is found, and the rank is less
    than the rank present on that file, then the rank is changed. All pawns
    are less than 7, so if a pawn is present, the pawn_set[white][file+1] will
    yield a rank less than 7.

    The same is done for black, but using 'greater than', intially setting all
    ranks as 0.

    3. pawns[colour][file+1] is incremented counting the pawns on a file
    */

    //loop through the piece list
    for ( index = 1; index <= p->pcenum; ++index)
      {
         //if the square for the piece number 'index' is 0, we know the
         //piece is dead, so ignore it
         if ( p->pcenumtosq[index] == 0)
         {
                continue;
         }

         //we have a piece, so assign the square to 'sq'
         sq = p->pcenumtosq[index];

         //see what type of piece lives on the square 'sq'
         switch (p->board[sq].typ)
         {

              case wR:
              eval->wRc++;//inccrease the rook count
              eval->wmajors++;//inccrease tha white majors count
              break;

              case wN:
              eval->wNc++;//increase the knight count
              eval->wmajors++;
              break;

              case wB:
              eval->wBc++;
              eval->wmajors++;
              eval->wBsq = sq;
              break;

              case wQ:
              eval->wQc++;
              eval->wmajors++;
              break;

              case wK:
              break;

              case wP:
              f = files[sq]+1;
              if (eval->pawn_set[white][f] > ranks[sq])
			  {
               eval->pawn_set[white][f] = ranks[sq];
              }
              eval->wpawns++;
              eval->pawns[white][f]++;
              eval->pawnbits[white] += pbit[ranks[sq]];
              break;

              case bR:

              eval->bRc++;
              eval->bmajors++;
              break;

              case bN:
              eval->bNc++;
              eval->bmajors++;
              break;

              case bB:
              eval->bBc++;
              eval->bmajors++;
              eval->bBsq = sq;
              break;

              case bQ:
              eval->bQc++;
              eval->bmajors++;
              break;

              case bK:
              break;

              case bP:
              f = files[sq]+1;
              if (eval->pawn_set[black][f] < ranks[sq])
			  {
               eval->pawn_set[black][f] = ranks[sq];
              }
              eval->bpawns++;
              eval->pawns[black][f]++;
              eval->pawnbits[black] += pbit[ranks[sq]];
              break;

             default: break;
         }
      }
      //debuginiteval();
}

/*
gameeval() returns the evaluation score for the current position
*/
int gameeval()
{
   //call init eval, followed by midgame eval to evaluate the position
   initeval();
   midgameeval();

  int score;

  //the phase is a number between 0 and 24 which describes the game phase -
  //it is scaled and then converted into a number between 0 (opening)
  //and 256 (endgame) to describe the game phase. This is completely
  //copied as an idea from fruit - my original version kept producing
  //daft scores.
  int phase = 24-((eval->wQc + eval->bQc)*4 +
       (eval->wRc + eval->bRc)*2 +
       (eval->wBc + eval->bBc) + (eval->wNc + eval->bNc) );
  if(phase<0) phase = 0;
  //cout<<"\ninitial phase = "<<phase;

  phase = (phase * 256 + (24 / 2)) / 24;

  //cout<<"\nadjusted phase = "<<phase;

  //retrieve the beginning and andgame phase scores
   int send = eval->score[white][end]-eval->score[black][end];
   int sbeg = eval->score[white][beg]-eval->score[black][beg];

   /*cout<<"\n [white][beg] = "<<eval->score[white][beg];
   cout<<"\n [white][end] = "<<eval->score[white][end];
   cout<<"\n [black][beg] = "<<eval->score[black][beg];
   cout<<"\n [black][end] = "<<eval->score[black][end];*/




   //and now scale the two scores according to the phase. So the result
   //will lie somewhere between send and sbeg.
   score = ((sbeg * (256 - phase)) + (send * phase)) / 256;

   //cout<<"\n adjusted score = "<<score;
    //adjust the score according to the side, and return.
    if (p->side == white)
    {
        return score;
    }
    else
    {
        return -score;
    }

    return 0;
}

void midgameeval()
{

   int sq;
   int index;

   if(eval->wpawns + eval->bpawns == 0)
   {
   if(isdrawnp()==0)
   {
       eval->score[white][beg] = 0;
       eval->score[white][end] = 0;
       eval->score[black][beg] = 0;
       eval->score[black][end] = 0;
       return;
   }
   }

/*
the principle here is to loop through every piece on the board, assigning
a value for the material, piece tables, and other misc values and functions.
I have described each feature once. A score is assigned for the
beginning and end.
*/
   for ( index = 1; index <= p->pcenum; ++index)
   {
         if ( p->pcenumtosq[index] == 0)
         {
                continue;
         }
         sq = p->pcenumtosq[index];

         switch (p->board[sq].typ)
         {
              case wP:
                     eval->score[white][beg] += vP;//material value
                     eval->score[white][beg] += Pawn[sq];//psqt
                     eval->score[white][end] += vP;
                     //pawn structure scoring for this pawn
                     whitepawnsstructure(&sq);
                     break;

             case wN:
                     eval->score[white][beg] += vN;
                     eval->score[white][beg] += Knight[sq];
                     eval->score[white][end] += vN;
                     eval->score[white][end] += eKnight[sq];
                     eval->score[white][beg] += wNsupport(&sq);
                     wknightmob(&sq);
                     break;

             case wB:
                     eval->score[white][beg] += vB;
                     eval->score[white][beg] += Bishop[sq];
                     eval->score[white][end] += vB;
                     eval->score[white][end] += eBishop[sq];
                     wbishopmob(&sq);//mobility
                     break;
             case wR:
                     eval->score[white][beg] += vR;
                     eval->score[white][end] += vR;
                     eval->score[white][beg] += Rook[sq];
                     wrooktrapped(&sq);//trapped?
                     wrookmob(&sq);

                     //open files
                     if(eval->pawns[white][files[sq]+1] == 0)
                     {
                        eval->score[white][beg] += 8;//semiopen
                        eval->score[white][end] += 15;

                        if(eval->pawns[black][files[sq]+1] == 0)
                        {
                            eval->score[white][beg] += 5;//open
                            eval->score[white][end] += 10;
                        }
                        //on a king file, and semi open?
                        if(abs(files[sq]-files[p->k[black]] <= 1))
                        {
                         eval->score[white][beg] += 8;
                         if(files[sq] == files[p->k[black]])
                         {
                          eval->score[white][beg] += 8;
                         }
                        }
                     }

                     //seventh rank - wrseven() sees if it's worth going there
                     if(ranks[sq] == 6 && wrseven())
                     {
                         eval->score[white][beg] += 10;
                         eval->score[white][end] += 20;
                     }

                     break;

             case wQ:
                     eval->score[white][beg] += vQ;
                     eval->score[white][end] += vQ;
                     wqueenmob(&sq);
                     if(ranks[sq] == 6 && wrseven())
                     {
                         eval->score[white][beg] += 5;
                         eval->score[white][end] += 10;
                     }
                     break;

             case wK:
                     eval->score[white][beg] += KingMid[sq];
                     eval->score[white][end] += KingEnd[sq];
                     if(dowks())
                     {
                     eval->score[white][beg] += whitekingsafety(&sq);
                     }
                     break;

             case bP:
                     eval->score[black][beg] += vP;
                     eval->score[black][beg] += Pawn[opp[sq]];
                     eval->score[black][end] += vP;
                     blackpawnsstructure(&sq);
                     break;
             case bN:
                     eval->score[black][beg] += vN;
                     eval->score[black][beg] += Knight[opp[sq]];
                     eval->score[black][end] += vN;
                     eval->score[black][end] += eKnight[opp[sq]];
                     eval->score[black][beg] += bNsupport(&sq);
                     bknightmob(&sq);
                     break;

             case bB:
                     eval->score[black][beg] += vB;
                     eval->score[black][beg] += Bishop[opp[sq]];
                     eval->score[black][end] += vB;
                     eval->score[black][end] += eBishop[opp[sq]];
                     bbishopmob(&sq);
                     break;

             case bR:
                     eval->score[black][beg] += vR;
                     eval->score[black][end] += vR;
                     eval->score[black][beg] += Rook[opp[sq]];
                     brooktrapped(&sq);
                     brookmob(&sq);
                     if(eval->pawns[black][files[sq]+1] == 0)
                     {
                        eval->score[black][beg] += 8;
                        eval->score[black][end] += 15;

                        if(eval->pawns[white][files[sq]+1] == 0)
                        {
                            eval->score[black][beg] += 5;
                            eval->score[black][end] += 10;
                        }
                         if(abs(files[sq]-files[p->k[white]] <= 1))
                         {
                          eval->score[black][beg] += 8;
                          if(files[sq] == files[p->k[white]])
                          {
                          eval->score[black][beg] += 8;
                          }
                         }
                     }
                     if(ranks[sq] == 1 && brseven())
                     {
                         eval->score[black][beg] += 10;
                         eval->score[black][end] += 20;
                     }

                     break;

             case bQ:
                     eval->score[black][beg] += vQ;
                     eval->score[black][end] += vQ;
                     bqueenmob(&sq);
                     if(ranks[sq] == 1 && brseven())
                     {
                         eval->score[black][beg] += 5;
                         eval->score[black][end] += 10;
                     }
                     break;

             case bK:
                     eval->score[black][beg] += KingMid[opp[sq]];
                     eval->score[black][end] += KingEnd[opp[sq]];
                     if(dobks())
                     {
                       eval->score[black][beg] += blackkingsafety(&sq);
                     }
                     break;

                     default: break;
         }
      }

     development();//score development
     blockedpawn();//score blockinh of e & d pawns in midgame

      //bishop pair advantage
      bishoppair();

      //if the kings are more than two files apart, we can pawn storm
      if(abs(files[p->k[white]] - files[p->k[black]]) > 2)
      {
          pawnstorm();
      }
    /*  eval->score[black][beg] -= pdefect[eval->defects[black]];
      eval->score[white][beg] -= pdefect[eval->defects[white]];*/

}


//see if there are pawns on the seventh rank, or king on the eighth,
//otherwise there is no point in putting the rook on the seventh
int wrseven()
{
     if ( (64 & eval->pawnbits[black]) || (ranks[p->k[black]] == 7) )
     return 1;

     return 0;
}

int brseven()
{
     if ( (2 & eval->pawnbits[white]) || (ranks[p->k[white]] == 0) )
     return 1;

     return 0;
}

//if the bishpos or knights are on their start squares, a penalty is given
void development()
{
    int wdev = 0;
    int bdev = 0;
    if(p->board[C1].typ == wB) {wdev++;}
    if(p->board[B1].typ == wN) {wdev++;}
    if(p->board[F1].typ == wB) {wdev++;}
    if(p->board[G1].typ == wN) {wdev++;}
    if(p->board[C8].typ == bB) {bdev++;}
    if(p->board[B8].typ == bN) {bdev++;}
    if(p->board[F8].typ == bB) {bdev++;}
    if(p->board[G8].typ == bN) {bdev++;}

     eval->score[black][beg] -= developpenalty[bdev];
     eval->score[white][beg] -= developpenalty[wdev];

}

int dowks()
{
    if(eval->bQc > 0)
    {
        if( (eval->bRc+eval->bBc+eval->bNc) > 1)
        return TRUE;
    }
    return FALSE;
}


int dobks()
{
    if(eval->wQc > 0)
    {
        if( (eval->wRc+eval->wBc+eval->wNc) > 1)
        return TRUE;
    }
    return FALSE;
}

//severe penalty for blocking the e & d pawns
void blockedpawn()
{
  if(p->board[D2].typ == wP && p->board[D3].typ != ety)
  {
      eval->score[white][beg] -= 40;
  }
  if(p->board[E2].typ == wP && p->board[E3].typ != ety)
  {
      eval->score[white][beg] -= 40;
  }
  if(p->board[D7].typ == bP && p->board[D6].typ != ety)
  {
      eval->score[black][beg] -= 40;
  }
  if(p->board[E7].typ == bP && p->board[E6].typ != ety)
  {
      eval->score[black][beg] -= 40;
  }
}

//severe penalty for trapping the rook with the king
void wrooktrapped(const int *sq)
{
    if(*sq == H1 || *sq == G1)
    {
        if (p->k[white] == F1 || p->k[white] == G1)
        {
            eval->score[white][beg] -= 70;
           // cout<<"\n white rook trapped;";
        }
    }
    else if (*sq == A1 || *sq == B1)
    {
         if (p->k[white] == C1 || p->k[white] == B1)
        {
            eval->score[white][beg] -= 70;
           // cout<<"\n white rook trapped;";
        }
    }
}


void brooktrapped(const int *sq)
{
    if(*sq == H8 || *sq == G8)
    {
        if (p->k[black] == F8 || p->k[black] == G8)
        {
            eval->score[black][beg] -= 70;
           // cout<<"\n black rook trapped;";
        }
    }
    else if (*sq == A8 || *sq == B8)
    {
         if (p->k[black] == C8 || p->k[black] == B8)
        {
            eval->score[black][beg] -= 70;
           // cout<<"\n black rook trapped;";
        }
    }
}

//quick check to see if either side has a bishop pair, bonus assigned according
//to the number of pawns on the board
void bishoppair()
{

    if(eval->wBc == 2)
    {
        eval->score[white][beg] += 50;
        eval->score[white][end] += 50;
    }
    if(eval->bBc == 2)
    {
        eval->score[black][beg] += 50;
        eval->score[black][end] += 50;
    }
}


int whitekingsafety(const int *sq)
{
   int file = files[*sq]+1;
    int rank = ranks[*sq];
    int fpen = 0;

    //file penalties
    fpen += whitepawncover(file, rank);
    if(file>1)
    fpen += whitepawncover(file-1, rank);
    if(file<8)
    fpen += whitepawncover(file+1, rank);

    int ourscore = 0;
   // cout<<"\n bK, sq "<<returnsquare(*sq);

     //castling
    if (p->castleflags & 12)
    {
       if(fpen < 21) ourscore -= 25;
        if(eval->pawns[white][file] == 0)
        {
            ourscore -= 10;
           // cout<<" semio -10";
        }
        if(eval->pawns[black][file] == 0)
        {
            ourscore -= 10;
           // cout<<" open -10";
        }
    }
    else
    {
        ourscore += fpen;
    }



    //cout<<"\n wks = "<<ourscore;

    return ((ourscore*eo->kingsafety)/128);

}

int blackkingsafety(const int *sq)
{
    int file = files[*sq]+1;
    int rank = ranks[*sq];
    int fpen = 0;

    //file penalties
    fpen += blackpawncover(file, rank);
    if(file>1)
    fpen += blackpawncover(file-1, rank);
    if(file<8)
    fpen += blackpawncover(file+1, rank);

    int ourscore = 0;
   // cout<<"\n bK, sq "<<returnsquare(*sq);

     //castling
    if (p->castleflags & 3)
    {
       if(fpen < 21) ourscore -= 25;
        if(eval->pawns[white][file] == 0)
        {
            ourscore -= 10;
           // cout<<" semio -5";
        }
        if(eval->pawns[black][file] == 0)
        {
            ourscore -= 10;
           // cout<<" open -5";
        }
    }
    else
    {
        ourscore += fpen;
    }


 //cout<<"\n bks = "<<ourscore;
    return ((ourscore*eo->kingsafety)/128);

}

int whitepawncover(const int file, const int rank)
{
    int ourscore = 0;
    if(eval->pawns[white][file] && rank < eval->pawn_set[white][file])
     {
       ourscore -= kpdist[eval->pawn_set[white][file]-rank];
     }
     else
     {
       ourscore -= 35;
     }
     return ourscore;
}


int blackpawncover(const int file, const int rank)
{
    int ourscore = 0;
    if(eval->pawns[black][file] && rank > eval->pawn_set[black][file])
    {
       ourscore -= kpdist[rank-eval->pawn_set[black][file]];
    }
    else
    {
       ourscore -= 35;
    }
     return ourscore;
}

void pawnstorm()
{
    int wkf = files[p->k[white]] + 1;
    int bkf = files[p->k[black]] + 1;

    //white storm
    eval->score[white][beg] += wstorm[eval->pawn_set[white][bkf-1]];
    eval->score[white][beg] += wstorm[eval->pawn_set[white][bkf]];
    eval->score[white][beg] += wstorm[eval->pawn_set[white][bkf+1]];
    //black storm
    eval->score[black][beg] += bstorm[eval->pawn_set[black][wkf-1]];
    eval->score[black][beg] += bstorm[eval->pawn_set[black][wkf]];
    eval->score[black][beg] += bstorm[eval->pawn_set[black][wkf+1]];

    if(eval->pawns[black][wkf] == 0) eval->score[black][beg]+=15;
    if(eval->pawns[black][wkf-1] == 0) eval->score[black][beg]+=12;
    if(eval->pawns[black][wkf+1] == 0) eval->score[black][beg]+=12;

    if(eval->pawns[white][bkf] == 0) eval->score[white][beg]+=15;
    if(eval->pawns[white][bkf-1] == 0) eval->score[white][beg]+=12;
    if(eval->pawns[white][bkf+1] == 0) eval->score[white][beg]+=12;
}

void whitepawnsstructure(const int *sq)
{
    int file = files[*sq]+1;
    int rank = ranks[*sq];
    int b = 0,i = 0;
    int escore = 0;
    int mscore = 0;


    if(eval->pawns[white][file] > 1)
    {
        mscore -= 4;
        escore -= 8;
        eval->defects[white]++;
    }

    if(eval->pawn_set[white][file-1] > rank && eval->pawn_set[white][file+1] > rank)
    {
        if(rank>1)
        {
         mscore -= 10;
         escore -= 20;
        }
        eval->defects[white]++;
        b=1;
        if(eval->pawns[white][file-1] == 0 && eval->pawns[white][file+1] == 0)
        {
           if(rank>1)
           {
            mscore -= 10;
            escore -= 20;
           }
           i=1;
        }
    }
     if(eval->pawns[black][file] == 0)
    {
        if(b) mscore -= 10;
        if(i) mscore -= 10;
    }

    if(eval->pawn_set[black][file-1] <= rank && eval->pawn_set[black][file] < rank
       && eval->pawn_set[black][file+1] <= rank)
    {
       wpp(sq);
    }
    eval->score[white][beg] += ((mscore*eo->pawnstructure)/128);
    eval->score[white][end] += ((escore*eo->pawnstructure)/128);
}

void blackpawnsstructure(const int *sq)
{
    int file = files[*sq]+1;
    int rank = ranks[*sq];
    int b = 0,i = 0;
    int mscore = 0;
    int escore = 0;


    if(eval->pawns[black][file] > 1)
    {
        mscore -= 4;
        escore -= 8;
        eval->defects[black]++;
    }
    if(eval->pawn_set[black][file-1] < rank && eval->pawn_set[black][file+1] < rank)
    {
        if(rank<6)
        {
        mscore -= 10;
        escore -= 20;
        }
        b = 1;
        eval->defects[black]++;
        if(eval->pawns[black][file-1] == 0 && eval->pawns[black][file+1] == 0)
        {
          if(rank<6)
          {
           mscore -= 10;
           escore -= 20;
          }
           i = 1;
        }
    }
    if(eval->pawns[white][file] == 0)
    {
        if(b) mscore -= 10;
        if(i) mscore -= 10;
    }



    if(eval->pawn_set[white][file-1] >= rank && eval->pawn_set[white][file] > rank
       && eval->pawn_set[white][file+1] >= rank)
    {
      bpp(sq);
    }
    eval->score[black][beg] += ((mscore*eo->pawnstructure)/128);
    eval->score[black][end] += ((escore*eo->pawnstructure)/128);
}

void wpp(const int *sq)
{
    int rank = ranks[*sq];

    //assign intial scores, mid and endgame
    int mscore = mppawn[white][rank];
    int escore = eppawn[white][rank];

    //if the sqaure in front is blocked, halve the value
    if(p->board[*sq+12].typ != ety)
    {
        mscore>>=1;
        escore>>=1;
    }

    //friendly pawn near?
    if(p->board[*sq+1].typ == wP || p->board[*sq-11].typ == wP)
    {
        mscore += (mppawn[white][rank]>>2);
        mscore += (eppawn[white][rank]>>1);
    }
    //friendly pawn near?
    if(p->board[*sq-1].typ == wP || p->board[*sq-13].typ == wP)
    {
        mscore += (mppawn[white][rank]>>2);
        mscore += (eppawn[white][rank]>>1);
    }

    //now assign a bonus for the kings being near
    mscore += ((kdmppawn[distancetable[p->k[white]][*sq]])>>2);
    mscore -= ((kdmppawn[distancetable[p->k[black]][*sq]])>>2);
    escore += ((kdeppawn[distancetable[p->k[white]][*sq]])>>2);
    escore -= ((kdeppawn[distancetable[p->k[black]][*sq]])>>2);

  /*  cout<<"\n wk near mid +"<<((kdmppawn[distancetable[p->k[white]][*sq]])>>3);
    cout<<"\n bk near mid -"<<((kdmppawn[distancetable[p->k[black]][*sq]])>>3);
    cout<<"\n wk near end +"<<((kdeppawn[distancetable[p->k[white]][*sq]])>>3);
    cout<<"\n bk near end -"<<((kdeppawn[distancetable[p->k[black]][*sq]])>>3);*/

    if(eval->bmajors == 0)//no pieces, can we promote?
        {
            int psq = *sq+(7-ranks[*sq])*12;

            if(distancetable[p->k[black]][psq] > distancetable[*sq][psq])
            {
                escore+=800;
            }
        }
    //now add the scores
    //#ifdef DEBUG
 /*   cout<<"\n wpassed "<<returnsquare(*sq);
    cout<<" mscore = "<<mscore<<" escore = "<<escore;
    cout<<"\nwend dist bonus = "<<((kdeppawn[distancetable[p->k[white]][*sq]])>>2);*/
    //#endif
    eval->score[white][beg] += ((mscore*eo->passedpawn)/128);
    eval->score[white][end] += ((escore*eo->passedpawn)/128);

}


void bpp(const int *sq)
{
    int rank = ranks[*sq];

    //assign intial scores, mid and endgame
    int mscore = mppawn[black][rank];
    int escore = eppawn[black][rank];

    //if the sqaure in front is blocked, halve the value
    if(p->board[*sq-12].typ != ety)
    {
        mscore>>=1;
        escore>>=1;
    }

    //friendly pawn near?
    if(p->board[*sq+1].typ == bP || p->board[*sq+13].typ == bP)
    {
        mscore += (mppawn[black][rank]>>2);
        mscore += (eppawn[black][rank]>>1);
    }
    //friendly pawn near?
    if(p->board[*sq-1].typ == bP || p->board[*sq+11].typ == bP)
    {
        mscore += (mppawn[black][rank]>>2);
        mscore += (eppawn[black][rank]>>1);
    }

    //now assign a bonus for the kings being near
    mscore += ((kdmppawn[distancetable[p->k[black]][*sq]])>>2);
    mscore -= ((kdmppawn[distancetable[p->k[white]][*sq]])>>2);
    escore += ((kdeppawn[distancetable[p->k[black]][*sq]])>>2);
    escore -= ((kdeppawn[distancetable[p->k[white]][*sq]])>>2);

  /*  cout<<"\n bk near mid +"<<((kdmppawn[distancetable[p->k[black]][*sq]])>>3);
    cout<<"\n wk near mid -"<<((kdmppawn[distancetable[p->k[white]][*sq]])>>3);
    cout<<"\n bk near end +"<<((kdeppawn[distancetable[p->k[black]][*sq]])>>3);
    cout<<"\n wk near end -"<<((kdeppawn[distancetable[p->k[white]][*sq]])>>3);*/

    if(eval->wmajors == 0)
        {
             int psq = *sq-(ranks[*sq]*12);
            if(distancetable[p->k[white]][psq] > distancetable[*sq][psq])
            {
                escore+=800;
            }
        }

    //now add the scores
   // #ifdef DEBUG
 /*   cout<<"\n bpassed "<<returnsquare(*sq);
    cout<<" mscore = "<<mscore<<" escore = "<<escore;*/
   // #endif

   eval->score[black][beg] += ((mscore*eo->passedpawn)/128);
    eval->score[black][end] += ((escore*eo->passedpawn)/128);


}
int wNsupport(const int *sq)
{
  int score = SupN[*sq];
    if(score == 0) return score;

  //we have a possible score
  if(eval->pawn_set[black][files[*sq+2]] <= ranks[*sq] && eval->pawn_set[black][files[*sq]] <= ranks[*sq])
  {
    if(eval->bNc == 0)
    {
       if( (eval->bBc == 0) || (eval->bBc == 1 && sqcol[eval->bBsq] != sqcol[*sq]) )
       {
          return score;
       }
       else
       {
           return (score>>2);
       }
    }
  }


  return score;

}

int bNsupport(const int *sq)
{
  int score = SupN[opp[*sq]];
  if(score == 0) return score;

  //we have a possible score
  if(eval->pawn_set[white][files[*sq+2]] >= ranks[*sq] && eval->pawn_set[white][files[*sq]] >= ranks[*sq])
  {
    if(eval->wNc == 0)
    {
       if( (eval->wBc == 0) || (eval->wBc == 1 && sqcol[eval->wBsq] != sqcol[*sq]) )
       {
          return score;
       }
       else
       {
           return (score>>2);
       }
    }
  }


  return score;

}


int isdrawnp()
{
    if (!eval->wRc && !eval->bRc && !eval->wQc && !eval->bQc)
	{
	  if (!eval->bBc && !eval->wBc)
	  {
	      /* only knights */
	      /* it pretty safe to say this is a draw */
	      if (eval->wNc < 3 && eval->bNc < 3)
		  {
		   return 0;
		  }
      }
	  else if (!eval->wNc && !eval->bNc)
	  {
	      /* only bishops */
	      /* not a draw if one side two other side zero
		 else its always a draw                     */
	    if (abs(eval->wBc - eval->bBc) < 2)
		{
		  return 0;
		}
      }
	  else if ((eval->wNc < 3 && !eval->wBc) || (eval->wBc == 1 && !eval->wNc))
	  {
	      /* we cant win, but can black? */
	    if ((eval->bNc < 3 && !eval->bBc) || (eval->bBc == 1 && !eval->bNc))
		{
		  /* guess not */
		  return 0;
		}
	  }
	}
    else if (!eval->wQc && !eval->bQc)
	{
	     if (eval->wRc == 1 && eval->bRc == 1)
	     {
	      /* rooks equal */
	      if ((eval->wNc + eval->wBc) < 2 && (eval->bNc + eval->bBc) < 2)
		  {
		  /* one minor difference max */
		  /* a draw too usually */
		  return 0;
		  }
	     }
	     else if (eval->wRc == 1 && !eval->bRc)
	     {
	      /* one rook */
	      /* draw if no minors to support AND
		 minors to defend  */
	      if ((eval->wNc + eval->wBc == 0) && (((eval->bNc + eval->bBc) == 1) || ((eval->bNc + eval->bBc) == 2)))
		  {
		   return 0;
		  }
	     }
	     else if (eval->bRc == 1 && !eval->wRc)
	     {
	      /* one rook */
	      /* draw if no minors to support AND
		 minors to defend  */
	      if ((eval->bNc + eval->bBc == 0) && (((eval->wNc + eval->wBc) == 1) || ((eval->wNc + eval->wBc) == 2)))
		  {
		    return 0;
		  }
	     }
	}
	return 1;
}

void wbishopmob(const int *sq)
{
    int t,m=0;

    for(t = *sq+11; ; t+=11)
    {
        if(p->board[t].typ != ety)
        {
            break;
        }
            m++;
    }
    for(t = *sq+13; ; t+=13)
    {
        if(p->board[t].typ != ety)
        {
            break;
        }
            m++;
    }
    for(t = *sq-11; ; t-=11)
    {
        if(p->board[t].typ != ety)
        {
            break;
        }
            m++;
    }
    for(t = *sq-13; ; t-=13)
    {
        if(p->board[t].typ != ety)
        {
            break;
        }
            m++;
    }
    eval->score[white][beg] += m;
    eval->score[white][end] += (m*2);
}


void bbishopmob(const int *sq)
{
    int t,m=0;

    for(t = *sq+11; ; t+=11)
    {
        if(p->board[t].typ != ety)
        {
            break;
        }
            m++;
    }
    for(t = *sq+13; ; t+=13)
    {
        if(p->board[t].typ != ety)
        {
            break;
        }
            m++;
    }
    for(t = *sq-11; ; t-=11)
    {
        if(p->board[t].typ != ety)
        {
            break;
        }
            m++;
    }
    for(t = *sq-13; ; t-=13)
    {
        if(p->board[t].typ != ety)
        {
            break;
        }
           m++;
    }
    eval->score[black][beg] += m;
    eval->score[black][end] += (m*2);

}

void wrookmob(const int *sq)
{
    int t,m=0;
    for(t = *sq+1; ; t+=1)
    {
        if(p->board[t].typ != ety)
        {
            break;
        }
            m++;
    }
    for(t = *sq-1; ; t-=1)
    {
         if(p->board[t].typ != ety)
        {
            break;
        }
            m++;
    }
    for(t = *sq+12; ; t+=12)
    {
         if(p->board[t].typ != ety)
        {
            break;
        }
            m++;
    }
    for(t = *sq-12; ; t-=12)
    {
        if(p->board[t].typ != ety)
        {
            break;
        }
            m++;
    }
    eval->score[white][end] += m*2;

}

void brookmob(const int *sq)
{
    int t,m=0;
    for(t = *sq+1; ; t+=1)
    {
        if(p->board[t].typ != ety)
        {
           break;
        }
           m++;
    }
    for(t = *sq-1; ; t-=1)
    {
       if(p->board[t].typ != ety)
        {
            break;
        }
           m++;
    }
    for(t = *sq+12; ; t+=12)
    {
        if(p->board[t].typ != ety)
        {
            break;
        }
            m++;
    }
    for(t = *sq-12; ; t-=12)
    {
        if(p->board[t].typ != ety)
        {
            break;
        }
            m++;
    }
   eval->score[black][end] += m*2;

}


void bqueenmob(const int *sq)
{
    int t,m=0;
    for(t = *sq+1; ; t+=1)
    {
        if(p->board[t].typ != ety)
        {
            break;
        }
            m++;
    }
    for(t = *sq-1; ; t-=1)
    {
        if(p->board[t].typ != ety)
        {
            break;
        }
            m++;
    }
    for(t = *sq+12; ; t+=12)
    {
       if(p->board[t].typ != ety)
        {
            break;
        }
            m++;
    }
    for(t = *sq-12; ; t-=12)
    {
       if(p->board[t].typ != ety)
        {
            break;
        }

            m++;
    }
    for(t = *sq+11; ; t+=11)
    {
       if(p->board[t].typ != ety)
        {
            break;
        }

            m++;
    }
    for(t = *sq+13; ; t+=13)
    {
       if(p->board[t].typ != ety)
        {

            break;
        }
       m++;
    }
    for(t = *sq-11; ; t-=11)
    {
        if(p->board[t].typ != ety)
        {
            break;
        }
        m++;
    }
    for(t = *sq-13; ; t-=13)
    {
        if(p->board[t].typ != ety)
        {
            break;
        }
        m++;
    }

    eval->score[black][end] += m*2;

}

void wqueenmob(const int *sq)
{
    int t,m=0;

    for(t = *sq+1; ; t+=1)
    {
        if(p->board[t].typ != ety)
        {
            break;
        }
        m++;
    }
    for(t = *sq-1; ; t-=1)
    {
        if(p->board[t].typ != ety)
        {
            break;
        }
        m++;
    }
    for(t = *sq+12; ; t+=12)
    {
       if(p->board[t].typ != ety)
        {
            break;
        }
        m++;
    }
    for(t = *sq-12; ; t-=12)
    {
       if(p->board[t].typ != ety)
        {
            break;
        }
        m++;
    }
    for(t = *sq+11; ; t+=11)
    {
       if(p->board[t].typ != ety)
        {
            break;
        }

            m++;
    }
    for(t = *sq+13; ; t+=13)
    {
       if(p->board[t].typ != ety)
        {
            break;
        }

            m++;
    }
    for(t = *sq-11; ; t-=11)
    {
        if(p->board[t].typ != ety)
        {
           break;
        }
            m++;
    }
    for(t = *sq-13; ; t-=13)
    {
        if(p->board[t].typ != ety)
        {
            break;
        }
        m++;
    }
   eval->score[white][end] += m*2;

}
void wknightmob(const int *sq)
{
    int m=0;

   if(p->board[*sq+25].typ == ety)
    m++;

    if(p->board[*sq+14].typ == ety)
    m++;

    if(p->board[*sq+10].typ == ety)
    m++;

    if(p->board[*sq+23].typ == ety)
    m++;

    if(p->board[*sq-25].typ == ety)
    m++;

    if(p->board[*sq-14].typ == ety)
    m++;

    if(p->board[*sq-10].typ == ety)
    m++;

    if(p->board[*sq-23].typ == ety)
    m++;


    eval->score[white][beg] += m;
}


void bknightmob(const int *sq)
{
    int m=0;

    if(p->board[*sq+25].typ == ety)
    m++;

    if(p->board[*sq+14].typ == ety)
    m++;

    if(p->board[*sq+10].typ == ety)
    m++;

    if(p->board[*sq+23].typ == ety)
    m++;

    if(p->board[*sq-25].typ == ety)
    m++;

    if(p->board[*sq-14].typ == ety)
    m++;

    if(p->board[*sq-10].typ == ety)
    m++;

    if(p->board[*sq-23].typ == ety)
    m++;


    eval->score[black][beg] += m;
}


//arrghhhhh
void mymemset()
{

eval->score[0][0] = 0;
eval->score[1][0] = 0;
eval->score[0][1] = 0;
eval->score[1][1] = 0;

eval->pawn_set[	1	]	[	0	]	=	7;
eval->pawn_set[	1	]	[	1	]	=	7;
eval->pawn_set[	1	]	[	2	]	=	7;
eval->pawn_set[	1	]	[	3	]	=	7;
eval->pawn_set[	1	]	[	4	]	=	7;
eval->pawn_set[	1	]	[	5	]	=	7;
eval->pawn_set[	1	]	[	6	]	=	7;
eval->pawn_set[	1	]	[	7	]	=	7;
eval->pawn_set[	1	]	[	8	]	=	7;
eval->pawn_set[	1	]	[	9	]	=	7;
eval->pawn_set[	0	]	[	0	]	=	0;
eval->pawn_set[	0	]	[	1	]	=	0;
eval->pawn_set[	0	]	[	2	]	=	0;
eval->pawn_set[	0	]	[	3	]	=	0;
eval->pawn_set[	0	]	[	4	]	=	0;
eval->pawn_set[	0	]	[	5	]	=	0;
eval->pawn_set[	0	]	[	6	]	=	0;
eval->pawn_set[	0	]	[	7	]	=	0;
eval->pawn_set[	0	]	[	8	]	=	0;
eval->pawn_set[	0	]	[	9	]	=	0;

eval->pawns[	0	]	[	0	]	=	0;
eval->pawns[	0	]	[	1	]	=	0;
eval->pawns[	0	]	[	2	]	=	0;
eval->pawns[	0	]	[	3	]	=	0;
eval->pawns[	0	]	[	4	]	=	0;
eval->pawns[	0	]	[	5	]	=	0;
eval->pawns[	0	]	[	6	]	=	0;
eval->pawns[	0	]	[	7	]	=	0;
eval->pawns[	0	]	[	8	]	=	0;
eval->pawns[	0	]	[	9	]	=	0;
eval->pawns[	1	]	[	0	]	=	0;
eval->pawns[	1	]	[	1	]	=	0;
eval->pawns[	1	]	[	2	]	=	0;
eval->pawns[	1	]	[	3	]	=	0;
eval->pawns[	1	]	[	4	]	=	0;
eval->pawns[	1	]	[	5	]	=	0;
eval->pawns[	1	]	[	6	]	=	0;
eval->pawns[	1	]	[	7	]	=	0;
eval->pawns[	1	]	[	8	]	=	0;
eval->pawns[	1	]	[	9	]	=	0;

eval->pawnbits[0] = 0;
eval->pawnbits[1] = 0;

eval->defects[0] = 0;
eval->defects[1] = 0;
eval->wRc = 0;
eval->bRc = 0;
eval->wQf = 0;
eval->bQf = 0;
eval->wNc = 0;
eval->bNc = 0;
eval->wBc = 0;
eval->bBc = 0;
eval->wQc = 0;
eval->bQc = 0;
eval->wBsq = 0;
eval->bBsq = 0;
eval->bmajors = 0;
eval->wmajors = 0;
eval->wpawns = 0;
eval->bpawns = 0;

}

