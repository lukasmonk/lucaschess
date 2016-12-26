#include <iostream>

#include "Lime.h"
#include "data.h"
#include "prototypes.h"


using namespace std;

/*
Clear the board - set all edge squares (two sqauares around the board) to 0.
Set the Board squares to empty, for both colour and piece

.typ.........
0   0   0   0   0   0   0   0   0   0   0   0
0   0   0   0   0   0   0   0   0   0   0   0
0   0  13  13  13  13  13  13  13  13   0   0
0   0  13  13  13  13  13  13  13  13   0   0
0   0  13  13  13  13  13  13  13  13   0   0
0   0  13  13  13  13  13  13  13  13   0   0
0   0  13  13  13  13  13  13  13  13   0   0
0   0  13  13  13  13  13  13  13  13   0   0
0   0  13  13  13  13  13  13  13  13   0   0
0   0  13  13  13  13  13  13  13  13   0   0
0   0   0   0   0   0   0   0   0   0   0   0
0   0   0   0   0   0   0   0   0   0   0   0

.pce.........
0   0   0   0   0   0   0   0   0   0   0   0
0   0   0   0   0   0   0   0   0   0   0   0
0   0  13  13  13  13  13  13  13  13   0   0
0   0  13  13  13  13  13  13  13  13   0   0
0   0  13  13  13  13  13  13  13  13   0   0
0   0  13  13  13  13  13  13  13  13   0   0
0   0  13  13  13  13  13  13  13  13   0   0
0   0  13  13  13  13  13  13  13  13   0   0
0   0  13  13  13  13  13  13  13  13   0   0
0   0  13  13  13  13  13  13  13  13   0   0
0   0   0   0   0   0   0   0   0   0   0   0
0   0   0   0   0   0   0   0   0   0   0   0
*/
void clearboard()
{
    for (int sq = 0; sq < 144; ++sq)
    {
      if(ranks[sq] == -1 || files[sq] == -1)
      {
        p->board[sq].col = edge;
        p->board[sq].typ = edge;
        continue;
      }
      p->board[sq].col = npco;
      p->board[sq].typ = ety;
        continue;
    }
    p->material[0] = 0;
    p->material[1] = 0;
}

/*
Intitialise the castlebits array for updating the castle permisssions -
when a piece is moved, the castle bits for the to square are '&' masked with
the castlebits[square], so if the king or rooks move, the relevent castle
permission is updated


11  15  15  15   3  15  15   7
15  15  15  15  15  15  15  15
15  15  15  15  15  15  15  15
15  15  15  15  15  15  15  15
15  15  15  15  15  15  15  15
15  15  15  15  15  15  15  15
15  15  15  15  15  15  15  15
14  15  15  15  12  15  15  13
*/
void init_castlebits()
{
     int sq;

     for (sq = 0; sq < 144; ++sq)
     {
         switch (sq)
         {
                case A1:
                       castlebits[sq] = 11;
                       break;
                case H1:
                       castlebits[sq] = 7;
                       break;
                case A8:
                       castlebits[sq] = 14;
                       break;
                case H8:
                       castlebits[sq] = 13;
                       break;
                case E1:
                       castlebits[sq] = 3;
                       break;
                case E8:
                       castlebits[sq] = 12;
                       break;
                default:
                       castlebits[sq] = 15;
                       break;
         }
     }
}

/*
convert a given file and rank into a square on the internal board
*/
int        fileranktosquare(const int f, const int r)
{
           return (((r+2)*12 + 2 + f));
}


/*
return the integer for a given file character, eg
char file = 'a';
usage cout<<" file = "<<chartofile(file);
*/
int        chartofile (const char file)
{
       //assert (col >= 'a' && col < 'i');
       if (file == 'a') {return 0;}
       if (file == 'b') {return 1;}
       if (file == 'c') {return 2;}
       if (file == 'd') {return 3;}
       if (file == 'e') {return 4;}
       if (file == 'f') {return 5;}
       if (file == 'g') {return 6;}
       if (file == 'h') {return 7;}

       return 0;
}


/*
return the integer for a given rank character, eg
char rank = '1';
usage cout<<" rank = "<<chartofile(rank);
*/
int        chartorank(const char rank)
{
       //assert (row >= '1' && row < '8');
       if (rank == '1') {return 0;}
       if (rank == '2') {return 1;}
       if (rank == '3') {return 2;}
       if (rank == '4') {return 3;}
       if (rank == '5') {return 4;}
       if (rank == '6') {return 5;}
       if (rank == '7') {return 6;}
       if (rank == '8') {return 7;}

       return 0;
}


/*
return the character for a give rank,
usage cout<<" rank = "<<ranktochar(rank);
*/
char       ranktochar(int rank)
{
           return brdranks[rank];
}


/*
return the character for a give file,
usage cout<<" file = "<<filetochar(file);
*/
char       filetochar(int file)
{
           return brdfiles[file];
}

/*
return the character for a given piece, (eg wP)
*/
char       piece(int piece)
{
           return piecetochar[piece];
}


/*
initialise the board piece lists.

The piece list structure is the same as that used in Faile - commented out
below is the original structure I had used indexed by colour as well as piece,
but I kept having makemove bugs with that version, so until I find it, I chose
to use this slightly slower alternative.

Basically, there are two stores.

1. pcenumtosq[pce]

This is indexed by piece number, and gives the square that piece is sitting on.

So, if we have 16 pieces in the list, and we want to know where piece no.4 sits,
pcenumtosq[4] gives us the square. e.g. pcenumtosq[4] = A4

2. sqtopcenum[sq]

This is the inverse of the above - for a given square, we return the piece no.

e.g sqtopcenum[A4] = 4

If a piece is captured, is is assigned the square value of '0', whcih is not a
board square, and is called dead square. Then, when cycling through the list,
if pcenumtosq[pce] returns '0' we know the piece is 'dead' and can ignore it.

The position structure, 'p', also has a counter 'pcenum' which keeps a track ot
the number of pieces
*/
void init_piecelists()//single list version
{
     int sq,pce;


     //reset the piece number to 0
     p->pcenum = 0;


     //clear the lists, set all squares to deadpiece (0)
     //and all piece nums to nopiece (0)
      for (sq = 0; sq < 144; ++sq)
         {
             p->sqtopcenum[sq] = nopiece;
             p->sqtopcenum[sq] = nopiece;
         }
         for (pce = 0; pce < 17; ++pce)
         {
             p->pcenumtosq[pce] = deadsquare;
             p->pcenumtosq[pce] = deadsquare;
         }


     //make damn sure the first one is 0 in the list - our list of pieces
     //will start from position 1
     p->pcenumtosq[0] = deadsquare;
     p->pcenumtosq[0] = deadsquare;

     //and reset the count of major pieces to 0
     p->majors = 0;

     //now loop through the board, setting the pieces
     for (sq = 0; sq < 144; ++sq)
     {
         if (p->board[sq].typ == edge)
         {
                //the square is no on the playing board, so ignore it
                continue;
         }
         if (p->board[sq].typ != ety)
         {
             //the square is on the board, and not empty, so must be a piece
             if (p->board[sq].typ != wP && p->board[sq].typ != bP)
              {
                //not a pawn, so it's a major, increment the major count
                p->majors++;
              }
              //and now store the piece square, after incrementing 'pcenum'.
              //note ++p->pcenum and not p->pcenum++;
              //likewise, index the piece number for the square
                  p->pcenumtosq[++p->pcenum] = sq;
                  p->sqtopcenum[sq] =  p->pcenum;

         }
     }
//all finished
}


/*this is the double list version, which I intend to get working soon - it
should be faster, and also allows the kings for each colour to be put at the
front of the list
*/

/*
void init_piecelists()//double list version
{
     int sq;
     int pcenum;
     int wpcnt = 0;
     int bpcnt = 0;
     int wkingsq = -1;
     int bkingsq = -1;
     int wkingnum = -1;
     int bkingnum = -1;

     //clear the lists, set all squares to deadpiece (0)
     //  and all piece nums to nopiece (0)
      for (sq = 0; sq < 144; ++sq)
         {
             p->sqtopcenum[black][sq] = nopiece;
             p->sqtopcenum[white][sq] = nopiece;
         }
         for (pcenum = 0; pcenum < 17; ++pcenum)
         {
             p->pcenumtosq[black][pcenum] = deadsquare;
             p->pcenumtosq[white][pcenum] = deadsquare;
         }


     p->pcenumtosq[white][0] = deadsquare;
     p->pcenumtosq[black][0] = deadsquare;
     p->majors = 0;

     //now loop through the board, setting the pieces

     for (sq = 0; sq < 144; ++sq)
     {
         if (p->board[sq].typ == edge)
         {
                continue;
         }
         if (p->board[sq].typ != ety)
         {
             // cout<<"\n a piece on "<<returnsquare(sq);
            //  cout<<" ("<<sq<<")";
              if (p->board[sq].typ != wP && p->board[sq].typ != bP)
              {
               p->majors++;
             //  cout<<"  a major ";
              }
              if (p->board[sq].col == wpco)
              {
                 // cout<<" white ";
                  p->pcenumtosq[white][++wpcnt] = sq;
                  p->sqtopcenum[white][sq] =  wpcnt;
                  if (p->board[sq].typ == wK)
                  {
                        wkingnum = wpcnt;
                        wkingsq = sq;
                  }
              }
              else
              {
                 // cout<<" black ";
                  p->pcenumtosq[black][++bpcnt] = sq;
                  p->sqtopcenum[black][sq] =  bpcnt;
                  if (p->board[sq].typ == bK)
                  {
                        bkingnum =  bpcnt;bkingsq = sq;
                  }
              }
         }
     }
     p->pcenum[white] = wpcnt;
     p->pcenum[black] = bpcnt;

     //---put king at front of each list

     int tsq;
     if (bkingnum != -1)//errr...should be a king,but in case
     {
        if(bkingnum != 1)//could be first piece
        {
             tsq = p->pcenumtosq[black][1];
             p->pcenumtosq[black][1] = bkingsq;
             p->sqtopcenum[black][bkingsq] = 1;
             p->sqtopcenum[black][tsq] = bkingnum;
             p->pcenumtosq[black][bkingnum] = tsq;
        }
     }
     if (wkingnum != -1)//errr...should be a king,but in case
     {
        if(wkingnum != 1)//could be first piece
        {
             tsq = p->pcenumtosq[white][1];
             p->pcenumtosq[white][1] = wkingsq;
             p->sqtopcenum[white][wkingsq] = 1;
             p->sqtopcenum[white][tsq] = wkingnum;
             p->pcenumtosq[white][wkingnum] = tsq;
        }
     }


}*/





