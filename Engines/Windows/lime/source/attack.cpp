#include <iostream>


#include "Lime.h"
#include "data.h"
#include "prototypes.h"

using namespace std;

/*
This function returns TRUE if the square 'sq' is attacked by side 'side'.

Implementation is simple - just iterate in all directions from the sqaure.

If we hit a piece that can move along that direction (e.g diagonal, +13, for
queen and bishop) then we return TRUE
*/
int isattacked(int sq, int side)//side doing the attacking
{
   //cout<<"\ncalled att()";
    int tsq;
 if(side == black)
 {
     //black pawns
     if(p->board[sq+13].typ == bP) return TRUE;
     if(p->board[sq+11].typ == bP) return TRUE;

     //black knights 14,10,25,23
     if(p->board[sq+14].typ == bN) return TRUE;
     if(p->board[sq+10].typ == bN) return TRUE;
     if(p->board[sq+25].typ == bN) return TRUE;
     if(p->board[sq+23].typ == bN) return TRUE;
     if(p->board[sq-14].typ == bN) return TRUE;
     if(p->board[sq-10].typ == bN) return TRUE;
     if(p->board[sq-25].typ == bN) return TRUE;
     if(p->board[sq-23].typ == bN) return TRUE;

     /*
     the system for the sliding piece is the same - keep looping until
     the board on tsq is not empty. Then see if it is an attacking piece, or not.
     Also check for the king attacks outside the loop
     */

     //rooks and queens and king

     //set the target square, tsq to sq+1 (one square to the right on the board)
     tsq = sq+1;
     //if we have a black king on tsq, we know sq is attacked, so return TRUE.
     if(p->board[tsq].typ == bK) return TRUE;

     //otherwise, enter w while loop, which runs 'while' we are not setting tsq
     //as an edge square. Not the loop starts with the same value for tsq as
     //intially set so we check one square to the side for queens and rooks
     while (p->board[tsq].typ != edge)
     {
         //if we have a black rook or queen on tsq it's an attack, return TRUE
         if(p->board[tsq].typ == bR || p->board[tsq].typ == bQ) return TRUE;

         //if we are not empty on tsq, we can't have a black rook or queen,
         //as they were checked for above, so it moust be another piece,
         //so break - there's no attack in this direction
         if(p->board[tsq].col != npco) break;

         //otherwise, the square was empty, so increment tsq, and loop again
         tsq+=1;
     }

      tsq = sq-1;
     if(p->board[tsq].typ == bK) return TRUE;
      while (p->board[tsq].typ != edge)
     {
         if(p->board[tsq].typ == bR || p->board[tsq].typ == bQ) return TRUE;
         if(p->board[tsq].col != npco) break;
         tsq-=1;
     }

      tsq = sq+12;
     if(p->board[tsq].typ == bK) return TRUE;
     while (p->board[tsq].typ != edge)
     {
         if(p->board[tsq].typ == bR || p->board[tsq].typ == bQ) return TRUE;
         if(p->board[tsq].col != npco) break;
         tsq+=12;
     }

      tsq = sq-12;
     if(p->board[tsq].typ == bK) return TRUE;
     while (p->board[tsq].typ != edge)
     {
         if(p->board[tsq].typ == bR || p->board[tsq].typ == bQ) return TRUE;
         if(p->board[tsq].col != npco) break;
         tsq-=12;
     }

     //bishops and queens
     tsq = sq+13;
     if(p->board[tsq].typ == bK) return TRUE;
     while (p->board[tsq].typ != edge)
     {
         if(p->board[tsq].typ == bB || p->board[tsq].typ == bQ) return TRUE;
         if(p->board[tsq].col != npco) break;
         tsq+=13;
     }
      tsq = sq-13;
     if(p->board[tsq].typ == bK) return TRUE;
     while (p->board[tsq].typ != edge)
     {
         if(p->board[tsq].typ == bB || p->board[tsq].typ == bQ) return TRUE;
         if(p->board[tsq].col != npco) break;
         tsq-=13;
     }
      tsq = sq+11;
     if(p->board[tsq].typ == bK) return TRUE;
     while (p->board[tsq].typ != edge)
     {
         if(p->board[tsq].typ == bB || p->board[tsq].typ == bQ) return TRUE;
         if(p->board[tsq].col != npco) break;
         tsq+=11;
     }
      tsq = sq-11;
     if(p->board[tsq].typ == bK) return TRUE;
     while (p->board[tsq].typ != edge)
     {
         if(p->board[tsq].typ == bB || p->board[tsq].typ == bQ) return TRUE;
         if(p->board[tsq].col != npco) break;
         tsq-=11;
     }
 }
 else//black being attacked
 {
     //white pawns
     if(p->board[sq-13].typ == wP) return TRUE;
     if(p->board[sq-11].typ == wP) return TRUE;

     //white knights 14,10,25,23
     if(p->board[sq+14].typ == wN) return TRUE;
     if(p->board[sq+10].typ == wN) return TRUE;
     if(p->board[sq+25].typ == wN) return TRUE;
     if(p->board[sq+23].typ == wN) return TRUE;
     if(p->board[sq-14].typ == wN) return TRUE;
     if(p->board[sq-10].typ == wN) return TRUE;
     if(p->board[sq-25].typ == wN) return TRUE;
     if(p->board[sq-23].typ == wN) return TRUE;

     //rooks and queens and king
     tsq = sq+1;
     if(p->board[tsq].typ == wK) return TRUE;
     while (p->board[tsq].typ != edge)
     {
         if(p->board[tsq].typ == wR || p->board[tsq].typ == wQ) return TRUE;
         if(p->board[tsq].col != npco) break;
         tsq+=1;
     }

      tsq = sq-1;
     if(p->board[tsq].typ == wK) return TRUE;
      while (p->board[tsq].typ != edge)
     {
         if(p->board[tsq].typ == wR || p->board[tsq].typ == wQ) return TRUE;
         if(p->board[tsq].col != npco) break;
         tsq-=1;
     }

      tsq = sq+12;
     if(p->board[tsq].typ == wK) return TRUE;
     while (p->board[tsq].typ != edge)
     {
         if(p->board[tsq].typ == wR || p->board[tsq].typ == wQ) return TRUE;
         if(p->board[tsq].col != npco) break;
         tsq+=12;
     }

      tsq = sq-12;
     if(p->board[tsq].typ == wK) return TRUE;
     while (p->board[tsq].typ != edge)
     {
         if(p->board[tsq].typ == wR || p->board[tsq].typ == wQ) return TRUE;
         if(p->board[tsq].col != npco) break;
         tsq-=12;
     }

     //bishops and queens
     tsq = sq+13;
     if(p->board[tsq].typ == wK) return TRUE;
     while (p->board[tsq].typ != edge)
     {
         if(p->board[tsq].typ == wB || p->board[tsq].typ == wQ) return TRUE;
         if(p->board[tsq].col != npco) break;
         tsq+=13;
     }
      tsq = sq-13;
     if(p->board[tsq].typ == wK) return TRUE;
     while (p->board[tsq].typ != edge)
     {
         if(p->board[tsq].typ == wB || p->board[tsq].typ == wQ) return TRUE;
         if(p->board[tsq].col != npco) break;
         tsq-=13;
     }
      tsq = sq+11;
     if(p->board[tsq].typ == wK) return TRUE;
     while (p->board[tsq].typ != edge)
     {
         if(p->board[tsq].typ == wB || p->board[tsq].typ == wQ) return TRUE;
         if(p->board[tsq].col != npco) break;
         tsq+=11;
     }
      tsq = sq-11;
     if(p->board[tsq].typ == wK) return TRUE;
     while (p->board[tsq].typ != edge)
     {
         if(p->board[tsq].typ == wB || p->board[tsq].typ == wQ) return TRUE;
         if(p->board[tsq].col != npco) break;
         tsq-=11;
     }
 }
         return FALSE;

    return 0;
}

