#include <iostream>

#include "Lime.h"
#include "data.h"
#include "prototypes.h"
#include "psqt.h"

using namespace std;

int makequick(int *m, s_Pce *holdme);
void takequick(int *m, s_Pce *holdme);

void  pushmove_legal(int data)
{
    s_Pce holdme;
    //create a move pointer and point to
    //the appropriate place in the list
    s_Move *move;
    if(!makequick(&data, &holdme))
    {
     move = &p->list[p->listc[p->ply+1]++];
     move->m = data;
    }
    takequick(&data, &holdme);
}

//push pawn is called before pushmove_legal for pawns to handle promotions
//checking if a pawn has been pushed to the seventh rank
void  pushpawn_legal(int from, int to, int flag)
{
       if (to > H7 || to < A2)
       {
            pushmove_legal((from<<8) | to |  mPQ);
            pushmove_legal((from<<8) | to |  mPR);
            pushmove_legal((from<<8) | to |  mPB);
            pushmove_legal((from<<8) | to |  mPN);
       }
       else
       {
         pushmove_legal((from<<8) | to | flag);
       }
}
//knightmove is a function used to with kings an knights - non sliding
//it takes the from and to sqaures as an argument, along with the colour
//of the opposing side for capturing move detection
void knightmove_legal(int f, int t, int xcol)
{
    //if we're trying to move to an edge square, we can't so return
    if(p->board[t].typ == edge)
    return;

    //if the board is empty on the to square, then add a normal move
    if(p->board[t].typ == ety)
    {
      pushmove_legal((f<<8) | t | mNORM);
      return;
    }

    //now we can only have a board with a piece. If the colour is the same as
    //xcol, then it's a capture, if not then we return naturally because
    //we've reached the end of the function
    if( p->board[t].col == xcol )
    {
      pushmove_legal((f<<8) | t | mCAP);
      return;
    }
}

//slide move adds moves for sliding pieces, taking the same arguments as
//knight move.
void slidemove_legal(int f, int t, int xcol)
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
        pushmove_legal((f<<8) | t | mNORM);
        t+=d;
       }
       //the board wasn't empty so was it an opposing colour piece? If so,
       //add a capture and break
       else if( p->board[t].col == xcol )
       {
        pushmove_legal((f<<8) | t | mCAP);
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


void movegen_legal()
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
                          pushpawn_legal(sq, tsq, mCAP);// add the capture
                     }
                     if ( p->en_pas == tsq )//is tsq an enpassant square?
                     {
                          pushmove_legal((sq<<8) | tsq | mPEP);
                     }

                     tsq = sq+11;//left capture
                     if(p->board[tsq].col == bpco)
                     {
                          pushpawn_legal(sq, tsq, mCAP);
                     }
                     if ( p->en_pas == tsq )
                     {
                          pushmove_legal((sq<<8) | tsq | mPEP);
                     }

                     tsq = sq+12;//now normal single square push
                     if( p->board[tsq].typ == ety )
                     {
                         pushpawn_legal(sq, tsq, mNORM);
                         //if we're on the second rank, we know the next
                         //square up is empty from the line above, so check
                         //if two squares forward is empty
                         if( sq < A3 && p->board[tsq+12].typ == ety )
                         {
                             pushmove_legal((sq<<8) | (tsq+12) | mPST);
                         }
                     }
                     break;

              case wN:
              //add knight moves for the 8 directions
                     knightmove_legal(sq, sq+14, bpco);
                     knightmove_legal(sq, sq+10, bpco);
                     knightmove_legal(sq, sq+25, bpco);
                     knightmove_legal(sq, sq+23, bpco);
                     knightmove_legal(sq, sq-14, bpco);
                     knightmove_legal(sq, sq-10, bpco);
                     knightmove_legal(sq, sq-25, bpco);
                     knightmove_legal(sq, sq-23, bpco);
                     break;
              case wK:
                     //add king moves for the 8 directions
                     knightmove_legal(sq, sq+1, bpco);
                     knightmove_legal(sq, sq+12, bpco);
                     knightmove_legal(sq, sq+11, bpco);
                     knightmove_legal(sq, sq+13, bpco);
                     knightmove_legal(sq, sq-1, bpco);
                     knightmove_legal(sq, sq-12, bpco);
                     knightmove_legal(sq, sq-11, bpco);
                     knightmove_legal(sq, sq-13, bpco);

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
                                       pushmove_legal( (E1<<8) | G1 | mCA);
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
                                      pushmove_legal(( E1<<8) | C1 | mCA);
                                    }
                                 }
                              }
                          }
                     }
                     break;

               case wQ:
               //add Queen moves for the 8 directions
                      slidemove_legal(sq, sq+13, bpco);
                      slidemove_legal(sq, sq+11, bpco);
                      slidemove_legal(sq, sq-13, bpco);
                      slidemove_legal(sq, sq-11, bpco);
                      slidemove_legal(sq, sq+12, bpco);
                      slidemove_legal(sq, sq-12, bpco);
                      slidemove_legal(sq, sq+1, bpco);
                      slidemove_legal(sq, sq-1, bpco);
                      break;

               case wB:
               //add Bishop moves for the 4 directions
                      slidemove_legal(sq, sq+13, bpco);
                      slidemove_legal(sq, sq+11, bpco);
                      slidemove_legal(sq, sq-13, bpco);
                      slidemove_legal(sq, sq-11, bpco);
                      break;

               case wR:
               //add rook moves for the 4 directions
                      slidemove_legal(sq, sq+12, bpco);
                      slidemove_legal(sq, sq-12, bpco);
                      slidemove_legal(sq, sq+1, bpco);
                      slidemove_legal(sq, sq-1, bpco);
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
                          pushpawn_legal(sq, tsq, mCAP);
                     }
                     if ( p->en_pas == tsq )
                     {
                          pushmove_legal( (sq<<8) | tsq | mPEP);
                     }

                     tsq = sq-11;
                     if(p->board[tsq].col == wpco)
                     {
                          pushpawn_legal(sq, tsq, mCAP);
                     }
                     if ( p->en_pas == tsq )
                     {
                          pushmove_legal( (sq<<8) | tsq | mPEP);
                     }

                     tsq = sq-12;
                     if( p->board[tsq].typ == ety )
                     {
                         pushpawn_legal(sq, tsq, mNORM);
                         if( sq > H6 && p->board[tsq-12].typ == ety )
                         {
                             pushmove_legal( (sq<<8) | (tsq-12) | mPST);
                         }
                     }
                     break;

              case bN:
                     knightmove_legal(sq, sq+14, wpco);
                     knightmove_legal(sq, sq+10, wpco);
                     knightmove_legal(sq, sq+25, wpco);
                     knightmove_legal(sq, sq+23, wpco);
                     knightmove_legal(sq, sq-14, wpco);
                     knightmove_legal(sq, sq-10, wpco);
                     knightmove_legal(sq, sq-25, wpco);
                     knightmove_legal(sq, sq-23, wpco);
                     break;
              case bK:
                     //cout<<endl<<"at king";
                     knightmove_legal(sq, sq+1, wpco);
                     knightmove_legal(sq, sq+12, wpco);
                     knightmove_legal(sq, sq+11, wpco);
                     knightmove_legal(sq, sq+13, wpco);
                     knightmove_legal(sq, sq-1, wpco);
                     knightmove_legal(sq, sq-12, wpco);
                     knightmove_legal(sq, sq-11, wpco);
                     knightmove_legal(sq, sq-13, wpco);

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
                                       pushmove_legal( (E8<<8) | G8 | mCA);
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
                                      pushmove_legal( (E8<<8) | C8 | mCA);
                                    }
                                 }
                              }
                          }
                        }
                     }
                     }
                     break;

               case bQ:
                      slidemove_legal(sq, sq+13, wpco);
                      slidemove_legal(sq, sq+11, wpco);
                      slidemove_legal(sq, sq-13, wpco);
                      slidemove_legal(sq, sq-11, wpco);
                      slidemove_legal(sq, sq+12, wpco);
                      slidemove_legal(sq, sq-12, wpco);
                      slidemove_legal(sq, sq+1, wpco);
                      slidemove_legal(sq, sq-1, wpco);
                      break;

               case bB:
                      slidemove_legal(sq, sq+13, wpco);
                      slidemove_legal(sq, sq+11, wpco);
                      slidemove_legal(sq, sq-13, wpco);
                      slidemove_legal(sq, sq-11, wpco);
                      break;

               case bR:
                      slidemove_legal(sq, sq+12, wpco);
                      slidemove_legal(sq, sq-12, wpco);
                      slidemove_legal(sq, sq+1, wpco);
                      slidemove_legal(sq, sq-1, wpco);
                      break;

                     default:
                     break;

         }//sw
      }//for
    }
}


int makequick(int *m, s_Pce *holdme)
{

    //first, set up the from, to and flag variables for the move '*m'
    int from = FROM(*m);
      int to = TO(*m);
      int flag = FLAG(*m);

    //r will be set to 1 if we're in check at the end of the move
    int r = 0;

    //holdme holds the captured piece
    holdme->typ = p->board[to].typ;
    holdme->col = p->board[to].col;


    //move the piece on the board
    p->board[to] = p->board[from];
    p->board[from].typ = ety;
    p->board[from].col = npco;

    //if the piece moves was a king, update this square in the position info.
    if(p->side == white && p->board[to].typ == wK)
    {
        p->k[white] = to;
    }
    else if(p->side == black && p->board[to].typ == bK)
    {
        p->k[black] = to;
    }

    //if the flag & mProm, it's a promotion
    if (flag & mProm)
       {
            if (flag & oPQ)
           {
              if (p->side == white)
              {
                p->board[to].typ = wQ;//change type
              }
              else
              {
                p->board[to].typ = bQ;
              }
           }
           else if (flag & oPR)
           {
              if (p->side == white)
              {
                p->board[to].typ = wR;
              }
              else
              {
                p->board[to].typ = bR;
              }
           }
           else if (flag & oPB)
           {
              if (p->side == white)
              {
                p->board[to].typ = wB;
              }
              else
              {
                p->board[to].typ = bB;
              }
           }
           else if (flag & oPN)
           {
              if (p->side == white)
              {
                p->board[to].typ = wN;
              }
              else
              {
                p->board[to].typ = bN;
              }
           }
       }
       //see if it's a caslte move
       else if (flag & mCA)//castling
       {
            if ( to == G1 )
            {
                 //move the rook
                 p->board[F1].typ = p->board[H1].typ;
                 p->board[H1].typ = ety;
                 p->board[F1].col = p->board[H1].col;
                 p->board[H1].col = npco;
            }
            if ( to  == C1 )
            {
                 p->board[D1].typ = p->board[A1].typ;
                 p->board[A1].typ = ety;
                 p->board[D1].col = p->board[A1].col;
                 p->board[A1].col = npco;
            }
            if ( to  == G8 )
            {
                 p->board[F8].typ = p->board[H8].typ;
                 p->board[H8].typ = ety;
                 p->board[F8].col = p->board[H8].col;
                 p->board[H8].col = npco;
            }
            if ( to  == C8 )
            {
                 p->board[D8].typ = p->board[A8].typ;
                 p->board[A8].typ = ety;
                 p->board[D8].col = p->board[A8].col;
                 p->board[A8].col = npco;
            }
       }
       //could be an enpassant capture
       else if (flag & oPEP)
       {
         if(p->side == white)
          {
            //remove black pawn from sq-12
            p->board[to-12].typ = ety;
            p->board[to-12].col = npco;
          }
          else
          {
            p->board[to+12].typ = ety;
            p->board[to+12].col = npco;
          }
       }

       r = isattacked(p->k[p->side], p->side^1);

       //return r (0 if the move is legal)
       return r;
}

/*
reverses makemove using the information stored in the history array
*/
void  takequick(int *m, s_Pce *holdme)
{

      //declare the from, to and flags from the stored move
      int from = FROM(*m);
      int to = TO(*m);
      int flag = FLAG(*m);

      //move the piece back to the from sqaure, and put whatever was 'captured'
      //back on the to sqaure
      p->board[from] = p->board[to];
      p->board[to].typ = holdme->typ;
      p->board[to].col = holdme->col;

      //if we're moving a king, update the relevant info
      if(p->side == white && p->board[from].typ == wK)
      {
        p->k[white] = from;
      }
      else if (p->side == black && p->board[from].typ == bK)
      {
        p->k[black] = from;
      }

      if (flag & mProm)
      {
          if (p->side == white)
          {
            p->board[from].typ = wP;
          }
          else
          {
            p->board[from].typ = bP;
          }
      }
      else if (  flag & mCA )
      {
          if ( to == G1 )
            {
                 p->board[H1].typ = p->board[F1].typ;
                 p->board[F1].typ = ety;
                 p->board[H1].col = p->board[F1].col;
                 p->board[F1].col = npco;
            }
            if ( to == C1 )
            {
                 p->board[A1].typ = p->board[D1].typ;
                 p->board[D1].typ = ety;
                 p->board[A1].col = p->board[D1].col;
                 p->board[D1].col = npco;
            }
            if ( to == G8 )
            {
                 p->board[H8].typ = p->board[F8].typ;
                 p->board[F8].typ = ety;
                 p->board[H8].col = p->board[F8].col;
                 p->board[F8].col = npco;
            }
            if ( to == C8 )
            {
                 p->board[A8].typ = p->board[D8].typ;
                 p->board[D8].typ = ety;
                 p->board[A8].col = p->board[D8].col;
                 p->board[D8].col = npco;
            }
      }
      else if ( flag & oPEP)
      {
           if(p->side == white)
           {
             p->board[to-12].typ = bP;
             p->board[to-12].col = bpco;
           }
           else
           {
             p->board[to+12].typ = wP;
             p->board[to+12].col = wpco;
           }
      }

//all done.
}

//this makes amove with no detection for being in check, only legal moves exist
// in the list
int makelegalmove(s_Move *m)
{

    //first, set up the from, to and flag variables for the move '*m'
    int from = FROM(m->m);
    int to = TO(m->m);
    int flag = FLAG(m->m);

    //r will be set to 1 if we're in check at the end of the move
    int r = 0;


    /*
    now, store some of the history variables

    hist[histply].data stores the 'm' part of the move structure (from, to, and flag)
    hist[histply].en_pas stores the enpassant square
    hist[histply].castleflags stores the castleflags
    hist[histply].captured stores whatever lies on the to square, empty or not
    */

    hist[histply].data = m->m;
    hist[histply].en_pas = p->en_pas;
    hist[histply].fifty = p->fifty;
    hist[histply].hashkey = p->hashkey;
    hist[histply].castleflags = p->castleflags;
    hist[histply].captured = p->board[to];


    //hash out the side, ep and castle flags
    p->hashkey ^= hash_s[p->side];
    p->hashkey ^= hash_ca[p->castleflags];
    p->hashkey ^= hash_enp[p->en_pas];

    //reset ep, and update castle flags by using the '&' mask with
    //the castlebits array (see board.cpp)
    p->en_pas = noenpas;
    p->castleflags &= castlebits[to];
    p->castleflags &= castlebits[from];

    //move the piece on the board
    p->board[to] = p->board[from];
    p->board[from].typ = ety;
    p->board[from].col = npco;

    /*
    initial piece list update - storing the piece number on the
    to square - of course, this will be 0 if the square is empty, but we need
    the number to reset the list in takeback(), if the square is not empty
    */
    hist[histply].plist = p->sqtopcenum[to];
    //set the sqaure for the piece number on the to sqaure to 0
    p->pcenumtosq[p->sqtopcenum[to]] = 0;
    //now set the square for the piece number on the from square to the to sqaure
    p->pcenumtosq[p->sqtopcenum[from]] = to;
    //now move the piece number from the from square to the to square
    p->sqtopcenum[to] = p->sqtopcenum[from];
    //finally set the piece number at the from swuare to 0
    p->sqtopcenum[from] = 0;

    //if the piece moves was a king, update this square in the position info.
    if(p->side == white && p->board[to].typ == wK)
    {
        p->k[white] = to;
    }
    else if(p->side == black && p->board[to].typ == bK)
    {
        p->k[black] = to;
    }

    //hash the piece we have moves out of the old square, into the new square
    p->hashkey ^= hash_p[p->board[to].typ][from];
    p->hashkey ^= hash_p[p->board[to].typ][to];

    //fifty count
    p->fifty++;

    //if what we 'captured' was not empty, then it's a true capture,
    //and the position needs to be updated.
    if(hist[histply].captured.typ != ety)
    {
      //if the piece is not a pawn, it's .typ number is > 2, and so
      //we have lost a major
      if(hist[histply].captured.typ > 2)
      {p->majors--;}

      //update the material count for the side
      p->material[p->side] -= vals[hist[histply].captured.typ];

      //hash out the captured piece
      p->hashkey ^= hash_p[hist[histply].captured.typ][to];

      p->fifty = 0;
    }

    if(p->board[to].typ < 3) p->fifty = 0;

    /*
    now the normal moves have been handled, we need to check for special case
    moves, and make the appropriate changes on the board
    */

    //if the flag & mProm, it's a promotion
    if (flag & mProm)
       {
           //we gain a major piece here
           p->majors++;

           //now check exactly which flag it is, again by an '&' mask.
           //once found, change the type from pawn to the promoted piece,
           //and hash out the pawn and hash in the new piece
           if (flag & oPQ)
           {
              if (p->side == white)
              {
                p->board[to].typ = wQ;//change type
                p->material[white] += vQ-vP;//update marterial value
                p->hashkey ^= hash_p[wP][to];//hash out pawn
                p->hashkey ^= hash_p[wQ][to];//hash in piece
              }
              else
              {
                p->board[to].typ = bQ;
                p->material[black] += vQ-vP;
                p->hashkey ^= hash_p[bP][to];
                p->hashkey ^= hash_p[bQ][to];
              }
           }
           else if (flag & oPR)
           {
              if (p->side == white)
              {
                p->board[to].typ = wR;
                p->material[white] += vR-vP;
                p->hashkey ^= hash_p[wP][to];
                p->hashkey ^= hash_p[wR][to];
              }
              else
              {
                p->board[to].typ = bR;
                p->material[black] += vR-vP;
                p->hashkey ^= hash_p[bP][to];
                p->hashkey ^= hash_p[bR][to];
              }
           }
           else if (flag & oPB)
           {
              if (p->side == white)
              {
                p->board[to].typ = wB;
                p->material[white] += vB-vP;
                p->hashkey ^= hash_p[wP][to];
                p->hashkey ^= hash_p[wB][to];
              }
              else
              {
                p->board[to].typ = bB;
                p->material[black] += vB-vP;
                p->hashkey ^= hash_p[bP][to];
                p->hashkey ^= hash_p[bB][to];
              }
           }
           else if (flag & oPN)
           {
              if (p->side == white)
              {
                p->board[to].typ = wN;
                p->material[white] += vN-vP;
                p->hashkey ^= hash_p[wP][to];
                p->hashkey ^= hash_p[wN][to];
              }
              else
              {
                p->board[to].typ = bN;
                p->material[black] += vN-vP;
                p->hashkey ^= hash_p[bP][to];
                p->hashkey ^= hash_p[bN][to];
              }
           }
       }
       //see if it's a pawn start - if so, set the en passant square
       else if (flag & mPST)
       {
            if(p->side == white)
            p->en_pas = to-12;

            else
            p->en_pas = to+12;
       }
       //see if it's a caslte move
       else if (flag & mCA)//castling
       {
            /*
            the type of castle move is found by looking at the destination
            square. Then, the rook is moved on the board, and moved in the piece
            lists
            */
            if ( to == G1 )
            {
                 //move the rook
                 p->board[F1].typ = p->board[H1].typ;
                 p->board[H1].typ = ety;
                 p->board[F1].col = p->board[H1].col;
                 p->board[H1].col = npco;

                 //hash the rook out of H1 and into F1
                 p->hashkey ^= hash_p[wR][H1];
                 p->hashkey ^= hash_p[wR][F1];

                 //move the rook's piece number from H1 to F1,
                 //set the piece number at H1 to 0
                 p->pcenumtosq[p->sqtopcenum[H1]] = F1;
                 p->sqtopcenum[F1] = p->sqtopcenum[H1];
                 p->sqtopcenum[H1] = 0;

            }
            if ( to  == C1 )
            {
                 p->board[D1].typ = p->board[A1].typ;
                 p->board[A1].typ = ety;
                 p->board[D1].col = p->board[A1].col;
                 p->board[A1].col = npco;

                 p->hashkey ^= hash_p[wR][A1];
                 p->hashkey ^= hash_p[wR][D1];

                 p->pcenumtosq[p->sqtopcenum[A1]] = D1;
                 p->sqtopcenum[D1] = p->sqtopcenum[A1];
                 p->sqtopcenum[A1] = 0;
            }
            if ( to  == G8 )
            {
                 p->board[F8].typ = p->board[H8].typ;
                 p->board[H8].typ = ety;
                 p->board[F8].col = p->board[H8].col;
                 p->board[H8].col = npco;

                 p->hashkey ^= hash_p[bR][H8];
                 p->hashkey ^= hash_p[bR][F8];

                 p->pcenumtosq[p->sqtopcenum[H8]] = F8;
                 p->sqtopcenum[F8] = p->sqtopcenum[H8];
                 p->sqtopcenum[H8] = 0;
            }
            if ( to  == C8 )
            {
                 p->board[D8].typ = p->board[A8].typ;
                 p->board[A8].typ = ety;
                 p->board[D8].col = p->board[A8].col;
                 p->board[A8].col = npco;

                 p->hashkey ^= hash_p[bR][A8];
                 p->hashkey ^= hash_p[bR][D8];

                 p->pcenumtosq[p->sqtopcenum[A8]] = D8;
                 p->sqtopcenum[D8] = p->sqtopcenum[A8];
                 p->sqtopcenum[A8] = 0;

            }
       }
       //could be an enpassant capture
       else if (flag & oPEP)
       {
          /*
          for an ep capture, we remove the captured pawn, hash it out, and
          then store this piece number in plist, overwriting the previous
          one, as that must have been an empty piece
          */
          if(p->side == white)
          {
            //remove black pawn from sq-12
            p->board[to-12].typ = ety;
            p->board[to-12].col = npco;

            //remove it from the hash
            p->hashkey ^= hash_p[bP][to-12];
            p->material[black] -= vP;

            //store the pawns piece number for when we take back the move
            hist[histply].plist = p->sqtopcenum[to-12];
            p->pcenumtosq[p->sqtopcenum[to-12]] = 0;
            p->sqtopcenum[to-12] = 0;

          }
          else
          {
            p->board[to+12].typ = ety;
            p->board[to+12].col = npco;

            p->hashkey ^= hash_p[wP][to+12];
            p->material[white] -= vP;

            hist[histply].plist = p->sqtopcenum[to+12];
            p->pcenumtosq[p->sqtopcenum[to+12]] = 0;
            p->sqtopcenum[to+12] = 0;
          }
       }

       //finally, increase the ply, histply, and change the side
       p->ply++;
       p->side^=1;
       histply++;

       //update the hash with the new side, castleflags and en_pas squares
       p->hashkey ^= hash_s[p->side];
       p->hashkey ^= hash_ca[p->castleflags];
       p->hashkey ^= hash_enp[p->en_pas];

       //return r (0 if the move is legal)
       return r;
}


