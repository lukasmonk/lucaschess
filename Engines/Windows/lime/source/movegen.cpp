#include <iostream>

#include "Lime.h"
#include "data.h"
#include "prototypes.h"
#include "psqt.h"

using namespace std;

//pointer that is set to the movegen piece square tables when scoring a move


/*
a move in Lime is a 32bit int - the first 8 bits are used for the 'to'
sqaure, the next 8 for the 'from' square and the remainder for the move flag.

So, the move looks like....

 --flag-- --from-- ---to---
 11111111 11111111 11111111.

 So, if from was 27 ( 0011011 ) and to was 88 (1011000) they would be assembled
 as follows..
 --flag--    --from-- ---to---
 <10101010>  0011011  1011000

 The from square is shifted left 8 bits before being added
 */

void  pushmove(int data)
{
    //create a move pointer and point to
    //the appropriate place in the list
    s_Move *move;
    move = &p->list[p->listc[p->ply+1]++];

    //assign the data (from,to,flag) to the move
    move->m = data;
}

//push pawn is called before pushmove for pawns to handle promotions
//checking if a pawn has been pushed to the seventh rank
void  pushpawn(int from, int to, int flag)
{
       if (to > H7 || to < A2)
       {
            pushmove((from<<8) | to |  mPQ | flag);
            pushmove((from<<8) | to |  mPR | flag);
            pushmove((from<<8) | to |  mPB | flag);
            pushmove((from<<8) | to |  mPN | flag);
       }
       else
       {
         pushmove((from<<8) | to | flag);
       }
}
//knightmove is a function used to with kings an knights - non sliding
//it takes the from and to sqaures as an argument, along with the colour
//of the opposing side for capturing move detection
void knightmove(int f, int t, int xcol)
{
    //if we're trying to move to an edge square, we can't so return
    if(p->board[t].typ == edge)
    return;

    //if the board is empty on the to square, then add a normal move
    if(p->board[t].typ == ety)
    {
      pushmove((f<<8) | t | mNORM);
      return;
    }

    //now we can only have a board with a piece. If the colour is the same as
    //xcol, then it's a capture, if not then we return naturally because
    //we've reached the end of the function
    if( p->board[t].col == xcol )
    {
      pushmove((f<<8) | t | mCAP);
      return;
    }
}

//slide move adds moves for sliding pieces, taking the same arguments as
//knight move.
void slidemove(int f, int t, int xcol)
{
   //first thing we need is the delta, 'd', between the from and to squares
   //do we have the direction to slide in
   int d = t-f;

   //check if the immediate to square iss off the board. If so, return.
   if(p->board[t].typ == edge) return;

   //now we enter a do - while loop - essentially "do" loop with delta "d"
   //while we have non edge squares. In the middle of the loop, if we hit a
   //piee, we break out
   do
   {
       //if empty on the 't' square, add a move and increment t by 'd' to move
       //to the next square. All instruction in this loop are contained
       //in and if-else-if sequence, so the order is important
       if(p->board[t].typ == ety)
       {
        pushmove((f<<8) | t | mNORM);
        t+=d;
       }
       //the board wasn't empty so was it an opposing colour piece? If so,
       //add a capture and break
       else if( p->board[t].col == xcol )
       {
        pushmove((f<<8) | t | mCAP);
        break;
       }
       //otherwise we must have hit the edge or a piece of our own colour,
       //so break
       else
       {
           break;
       }
   } while (p->board[t].typ != edge);
   return;
}

//knightmovec and slidemovec purely generate captures
void knightmovec(int f, int t, int xcol)
{
    if(p->board[t].typ == edge)
    return;

    else if( p->board[t].col == xcol )
    {
      pushmove((f<<8) | t | mCAP);
      return;
    }
    return;
}


void slidemovec(int f, int t, int xcol)
{
   int d = t-f;
   if(p->board[t].typ == edge) return;
   do
   {
      // cout<<"\n sliding to "<<returnsquare(t)<<" ("<<p->board[t]<<")";
       if(p->board[t].typ == ety)
       {
       t+=d;
       }
       else if( p->board[t].col == xcol )
       {
        pushmove((f<<8) | t | mCAP);
        break;
       }
       else
       {
           break;
       }
   } while (p->board[t].typ != edge);
   return;
}

/*

Before describing movegen(), a quick explanation of how the move list
is maintained, show by the line "p->listc[p->ply+1] = p->listc[p->ply];".

First set the position in the move list counter for the moves being
generated at this ply. p->listc is incremented each time a
move is added in pushmove(), above. That way, p->listc provides
an index number for each move we generate.

For example, the start position, p->ply = 0.

so, p->listc[0] is set to p->listc[1] = 0;

We then generate a2a3 in pushmove() at the following position

p->list[p->listc[p->ply+1]++];

Or..

p->list[0], and p->listc[1] is incermented to 1.

So, the move list is

p->list[0] = a2a3

and the index for the start of this list,ply 0, is p->plistc[0] = 0.
The index for the end of the list is at ply = 1, where p->listc[1] = 1;

And so, if we then add a2a4, it is added at p->list[1],
and p->listc[1] is set to 2. And so on. There are 20 legal moves from the start
position, so our final list looks like...

p->listc[0] = 0;
p->listc[1] = 20;

so the move list is index positions 0 - 19. (p->list[0] - p->list[19]).
*/




void movegen()
{
     int index;
     int sq;
     int tsq;
    //see above :)
    p->listc[p->ply+1] = p->listc[p->ply];

    //if white to play.....
    if (p->side==white)//white
    {
     //loop through the piece list....
     for ( index = 1; index <= p->pcenum; ++index)
      {
         if ( p->pcenumtosq[index] == 0)//piece is dead, so ignore it
         {
                continue;
         }
        //piee isn't dead, so assign the square to 'sq'.
        sq = p->pcenumtosq[index];

        //now use a switch to see what type of piece is on square 'sq',
        //looking for just white pieces.
         switch (p->board[sq].typ)
         {
                case wP:
                     //white pawn. Look for a right capture....sq+13
                     tsq = sq+13;
                     if(p->board[tsq].col == bpco)// a blackpiece
                     {
                          pushpawn(sq, tsq, mCAP);// add the capture
                     }
                     if ( p->en_pas == tsq )//is tsq an enpassant square?
                     {
                          pushmove((sq<<8) | tsq | mPEP);
                     }

                     tsq = sq+11;//left capture
                     if(p->board[tsq].col == bpco)
                     {
                          pushpawn(sq, tsq, mCAP);
                     }
                     if ( p->en_pas == tsq )
                     {
                          pushmove((sq<<8) | tsq | mPEP);
                     }

                     tsq = sq+12;//now normal single square push
                     if( p->board[tsq].typ == ety )
                     {
                         pushpawn(sq, tsq, mNORM);
                         //if we're on the second rank, we know the next
                         //square up is empty from the line above, so check
                         //if two squares forward is empty
                         if( sq < A3 && p->board[tsq+12].typ == ety )
                         {
                             pushmove((sq<<8) | (tsq+12) | mPST);
                         }
                     }
                     break;

              case wN:
              //add knight moves for the 8 directions
                     knightmove(sq, sq+14, bpco);
                     knightmove(sq, sq+10, bpco);
                     knightmove(sq, sq+25, bpco);
                     knightmove(sq, sq+23, bpco);
                     knightmove(sq, sq-14, bpco);
                     knightmove(sq, sq-10, bpco);
                     knightmove(sq, sq-25, bpco);
                     knightmove(sq, sq-23, bpco);
                     break;
              case wK:
                     //add king moves for the 8 directions
                     knightmove(sq, sq+1, bpco);
                     knightmove(sq, sq+12, bpco);
                     knightmove(sq, sq+11, bpco);
                     knightmove(sq, sq+13, bpco);
                     knightmove(sq, sq-1, bpco);
                     knightmove(sq, sq-12, bpco);
                     knightmove(sq, sq-11, bpco);
                     knightmove(sq, sq-13, bpco);

                     //castling - if the castle permission allows
                     //castling,the king an rooks must be in position,
                     //so no need to check for this. we look to see if
                     //the inbetween squares areempty,and if they are attacked
                     //by the opposition
                     if (p->castleflags & 8)//King side
                     {
                        if ( p->board[F1].typ == ety && p->board[G1].typ == ety)
                          {
                              if (!(isattacked( F1,black )) )
                              {
                                 if (!(isattacked( E1,black )) )
                                 {
                                      if (!(isattacked( G1,black )) )
                                      {
                                       pushmove( (E1<<8) | G1 | mCA);
                                      }
                                 }
                              }
                          }
                     }
                     if (p->castleflags & 4)//Queen side
                     {
                        if ( p->board[D1].typ == ety && p->board[C1].typ == ety && p->board[B1].typ == ety)
                          {
                              if (!(isattacked( D1,black )) )
                              {
                                 if (!(isattacked( E1,black )) )
                                 {
                                    if (!(isattacked( C1,black )) )
                                    {
                                      pushmove(( E1<<8) | C1 | mCA);
                                    }
                                 }
                              }
                          }
                     }
                     break;

               case wQ:
               //add Queen moves for the 8 directions
                      slidemove(sq, sq+13, bpco);
                      slidemove(sq, sq+11, bpco);
                      slidemove(sq, sq-13, bpco);
                      slidemove(sq, sq-11, bpco);
                      slidemove(sq, sq+12, bpco);
                      slidemove(sq, sq-12, bpco);
                      slidemove(sq, sq+1, bpco);
                      slidemove(sq, sq-1, bpco);
                      break;

               case wB:
               //add Bishop moves for the 4 directions
                      slidemove(sq, sq+13, bpco);
                      slidemove(sq, sq+11, bpco);
                      slidemove(sq, sq-13, bpco);
                      slidemove(sq, sq-11, bpco);
                      break;

               case wR:
               //add rook moves for the 4 directions
                      slidemove(sq, sq+12, bpco);
                      slidemove(sq, sq-12, bpco);
                      slidemove(sq, sq+1, bpco);
                      slidemove(sq, sq-1, bpco);
                      break;

                     default:
                     break;

         }//sw
      }//for
    }//if
    else
    {
      for ( index = 1; index <= p->pcenum; ++index)
      {
         if ( p->pcenumtosq[index] == 0)
         {
                continue;
         }
        sq = p->pcenumtosq[index];

         switch (p->board[sq].typ)
         {
               case bP:
                     tsq = sq-13;
                     if(p->board[tsq].col == wpco)
                     {
                          pushpawn(sq, tsq, mCAP);
                     }
                     if ( p->en_pas == tsq )
                     {
                          pushmove( (sq<<8) | tsq | mPEP);
                     }

                     tsq = sq-11;
                     if(p->board[tsq].col == wpco)
                     {
                          pushpawn(sq, tsq, mCAP);
                     }
                     if ( p->en_pas == tsq )
                     {
                          pushmove( (sq<<8) | tsq | mPEP);
                     }

                     tsq = sq-12;
                     if( p->board[tsq].typ == ety )
                     {
                         pushpawn(sq, tsq, mNORM);
                         if( sq > H6 && p->board[tsq-12].typ == ety )
                         {
                             pushmove( (sq<<8) | (tsq-12) | mPST);
                         }
                     }
                     break;

              case bN:
                     knightmove(sq, sq+14, wpco);
                     knightmove(sq, sq+10, wpco);
                     knightmove(sq, sq+25, wpco);
                     knightmove(sq, sq+23, wpco);
                     knightmove(sq, sq-14, wpco);
                     knightmove(sq, sq-10, wpco);
                     knightmove(sq, sq-25, wpco);
                     knightmove(sq, sq-23, wpco);
                     break;
              case bK:
                     //cout<<endl<<"at king";
                     knightmove(sq, sq+1, wpco);
                     knightmove(sq, sq+12, wpco);
                     knightmove(sq, sq+11, wpco);
                     knightmove(sq, sq+13, wpco);
                     knightmove(sq, sq-1, wpco);
                     knightmove(sq, sq-12, wpco);
                     knightmove(sq, sq-11, wpco);
                     knightmove(sq, sq-13, wpco);

                     //castling
                     if(sq == E8)
                     {
                     if (p->castleflags & 2)
                     {
                        if (  p->board[H8].typ == bR )
                        {
                          if ( p->board[F8].typ == ety && p->board[G8].typ == ety)
                          {
                              if (!(isattacked( F8,white )) )
                              {
                                 if (!(isattacked( E8,white )) )
                                 {
                                     if (!(isattacked( G8,white )) )
                                     {
                                       pushmove( (E8<<8) | G8 | mCA);
                                     }
                                 }
                              }
                          }
                        }
                     }
                     if (p->castleflags & 1)
                     {
                        if (  p->board[A8].typ == bR )
                        {
                          if ( p->board[D8].typ == ety && p->board[C8].typ == ety && p->board[B8].typ == ety)
                          {
                              if (!(isattacked( D8, white )) )
                              {
                                 if (!(isattacked( E8, white )) )
                                 {
                                    if (!(isattacked( C8, white )) )
                                    {
                                      pushmove( (E8<<8) | C8 | mCA);
                                    }
                                 }
                              }
                          }
                        }
                     }
                     }
                     break;

               case bQ:
                      slidemove(sq, sq+13, wpco);
                      slidemove(sq, sq+11, wpco);
                      slidemove(sq, sq-13, wpco);
                      slidemove(sq, sq-11, wpco);
                      slidemove(sq, sq+12, wpco);
                      slidemove(sq, sq-12, wpco);
                      slidemove(sq, sq+1, wpco);
                      slidemove(sq, sq-1, wpco);
                      break;

               case bB:
                      slidemove(sq, sq+13, wpco);
                      slidemove(sq, sq+11, wpco);
                      slidemove(sq, sq-13, wpco);
                      slidemove(sq, sq-11, wpco);
                      break;

               case bR:
                      slidemove(sq, sq+12, wpco);
                      slidemove(sq, sq-12, wpco);
                      slidemove(sq, sq+1, wpco);
                      slidemove(sq, sq-1, wpco);
                      break;

                     default:
                     break;

         }//sw
      }//for
    }
}

/*
same as movegen, but generates captures only, and no castling
*/
void capgen()
{
     int index;
     int sq;
     int tsq;
     p->listc[p->ply+1] = p->listc[p->ply];


    if (p->side==white)//white
    {
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
                     tsq = sq+13;
                     if(p->board[tsq].col == bpco)
                     {
                          pushpawn(sq, tsq, mCAP);
                     }
                     if ( p->en_pas == tsq )
                     {
                          pushmove( (sq<<8) | tsq | mPEP);
                     }

                     tsq = sq+11;
                     if(p->board[tsq].col == bpco)
                     {
                          pushpawn(sq, tsq, mCAP);
                     }
                     if ( p->en_pas == tsq )
                     {
                          pushmove((sq<<8) | tsq | mPEP);
                     }
                     break;

              case wN:
                     knightmovec(sq, sq+14, bpco);
                     knightmovec(sq, sq+10, bpco);
                     knightmovec(sq, sq+25, bpco);
                     knightmovec(sq, sq+23, bpco);
                     knightmovec(sq, sq-14, bpco);
                     knightmovec(sq, sq-10, bpco);
                     knightmovec(sq, sq-25, bpco);
                     knightmovec(sq, sq-23, bpco);
                     break;
              case wK:
                     //cout<<endl<<"at king";
                     knightmovec(sq, sq+1, bpco);
                     knightmovec(sq, sq+12, bpco);
                     knightmovec(sq, sq+11, bpco);
                     knightmovec(sq, sq+13, bpco);
                     knightmovec(sq, sq-1, bpco);
                     knightmovec(sq, sq-12, bpco);
                     knightmovec(sq, sq-11, bpco);
                     knightmovec(sq, sq-13, bpco);


                     break;

               case wQ:
                      slidemovec(sq, sq+13, bpco);
                      slidemovec(sq, sq+11, bpco);
                      slidemovec(sq, sq-13, bpco);
                      slidemovec(sq, sq-11, bpco);
                      slidemovec(sq, sq+12, bpco);
                      slidemovec(sq, sq-12, bpco);
                      slidemovec(sq, sq+1, bpco);
                      slidemovec(sq, sq-1, bpco);
                      break;

               case wB:
                      slidemovec(sq, sq+13, bpco);
                      slidemovec(sq, sq+11, bpco);
                      slidemovec(sq, sq-13, bpco);
                      slidemovec(sq, sq-11, bpco);
                      break;

               case wR:
                      slidemovec(sq, sq+12, bpco);
                      slidemovec(sq, sq-12, bpco);
                      slidemovec(sq, sq+1, bpco);
                      slidemovec(sq, sq-1, bpco);
                      break;

                     default:
                     break;

         }//sw
      }//for
    }//if
    else
    {
      for ( index = 1; index <= p->pcenum; ++index)
      {
         if ( p->pcenumtosq[index] == 0)
         {
                continue;
         }
        sq = p->pcenumtosq[index];
      switch (p->board[sq].typ)
         {
               case bP:
                     tsq = sq-13;
                     if(p->board[tsq].col == wpco)
                     {
                          pushpawn(sq, tsq, mCAP);
                     }
                     if ( p->en_pas == tsq )
                     {
                          pushmove((sq<<8) | tsq | mPEP);
                     }

                     tsq = sq-11;
                     if(p->board[tsq].col == wpco)
                     {
                          pushpawn(sq, tsq, mCAP);
                     }
                     if ( p->en_pas == tsq )
                     {
                          pushmove((sq<<8) | tsq | mPEP);
                     }

                     break;

              case bN:
                     knightmovec(sq, sq+14, wpco);
                     knightmovec(sq, sq+10, wpco);
                     knightmovec(sq, sq+25, wpco);
                     knightmovec(sq, sq+23, wpco);
                     knightmovec(sq, sq-14, wpco);
                     knightmovec(sq, sq-10, wpco);
                     knightmovec(sq, sq-25, wpco);
                     knightmovec(sq, sq-23, wpco);
                     break;
              case bK:
                     //cout<<endl<<"at king";
                     knightmovec(sq, sq+1, wpco);
                     knightmovec(sq, sq+12, wpco);
                     knightmovec(sq, sq+11, wpco);
                     knightmovec(sq, sq+13, wpco);
                     knightmovec(sq, sq-1, wpco);
                     knightmovec(sq, sq-12, wpco);
                     knightmovec(sq, sq-11, wpco);
                     knightmovec(sq, sq-13, wpco);

                     break;

               case bQ:
                      slidemovec(sq, sq+13, wpco);
                      slidemovec(sq, sq+11, wpco);
                      slidemovec(sq, sq-13, wpco);
                      slidemovec(sq, sq-11, wpco);
                      slidemovec(sq, sq+12, wpco);
                      slidemovec(sq, sq-12, wpco);
                      slidemovec(sq, sq+1, wpco);
                      slidemovec(sq, sq-1, wpco);
                      break;

               case bB:
                      slidemovec(sq, sq+13, wpco);
                      slidemovec(sq, sq+11, wpco);
                      slidemovec(sq, sq-13, wpco);
                      slidemovec(sq, sq-11, wpco);
                      break;

               case bR:
                      slidemovec(sq, sq+12, wpco);
                      slidemovec(sq, sq-12, wpco);
                      slidemovec(sq, sq+1, wpco);
                      slidemovec(sq, sq-1, wpco);
                      break;

                     default:
                     break;

         }//sw
      }//for
    }
}

