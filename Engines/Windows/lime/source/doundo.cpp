#include <iostream>

#include "Lime.h"
#include "data.h"
#include "prototypes.h"

using namespace std;

/* single list version*/

/*
makemove makes a move on the internal board, storing the information from the
previous position in the hist[] array, which holds the game history
*/

int makemove(s_Move *m)
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

       /*
       now, all possible moves are complete, so see if we're in check or not.
       If we are, r is set to 1. So if a 1 is returned from this function,
       we know it's illegal
       */
       r = isattacked(p->k[p->side], p->side^1);

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

/*
reverses makemove using the information stored in the history array
*/
void  takemove()
{

      //reduce the ply, change the side back and reset histply to index the
      //information stored in the hist[] aray
      p->ply--;
      p->side^=1;
      histply--;

      //reset the basic permission informatio from the hist[] array
      p->castleflags = hist[histply].castleflags;
      p->en_pas = hist[histply].en_pas;
      p->hashkey = hist[histply].hashkey;
      p->fifty = hist[histply].fifty;

      //declare the from, to and flags from the stored move
      int from = FROM(hist[histply].data);
      int to = TO(hist[histply].data);
      int flag = FLAG(hist[histply].data);

      //move the piece back to the from sqaure, and put whatever was 'captured'
      //back on the to sqaure
      p->board[from] = p->board[to];
      p->board[to] = hist[histply].captured;

      //reset the piece lists (explanation of how this works see makemove()
      //and board.cpp
      p->sqtopcenum[from] = p->sqtopcenum[to];
      p->sqtopcenum[to] = hist[histply].plist;
      p->pcenumtosq[p->sqtopcenum[to]] = to;
      p->pcenumtosq[p->sqtopcenum[from]] = from;

      //if we're moving a king, update the relevant info
      if(p->side == white && p->board[from].typ == wK)
      {
        p->k[white] = from;
      }
      else if (p->side == black && p->board[from].typ == bK)
      {
        p->k[black] = from;
      }


     //if it was a capture, update the majors count and the material
     if (hist[histply].captured.typ != ety)
       {
         p->material[p->side] += vals[hist[histply].captured.typ];
         if (hist[histply].captured.typ > 2)
         {
           p->majors++;
         }
       }

      //same as in makemove, deal with special moves using the & mask
      // with the move flag - process is the same as in makemove
      if (flag & mProm)
      {
          p->majors--;
          if (p->side == white)
          {
            p->board[from].typ = wP;
          }
          else
          {
            p->board[from].typ = bP;
          }

          if(flag&oPQ)
          {p->material[p->side] -= vQ-vP;}
          else if (flag&oPR)
          {p->material[p->side] -= vR-vP;}
          else if (flag&oPB)
          {p->material[p->side] -= vB-vP;}
          else if (flag&oPN)
          {p->material[p->side] -= vN-vP;}

      }
      else if (  flag & mCA )
      {
          if ( to == G1 )
            {
                 p->board[H1].typ = p->board[F1].typ;
                 p->board[F1].typ = ety;
                 p->board[H1].col = p->board[F1].col;
                 p->board[F1].col = npco;

                 p->sqtopcenum[H1] = p->sqtopcenum[F1];
                 p->sqtopcenum[F1] = 0;
                 p->pcenumtosq[p->sqtopcenum[H1]] = H1;

            }
            if ( to == C1 )
            {
                 p->board[A1].typ = p->board[D1].typ;
                 p->board[D1].typ = ety;
                 p->board[A1].col = p->board[D1].col;
                 p->board[D1].col = npco;

                 p->sqtopcenum[A1] = p->sqtopcenum[D1];
                 p->sqtopcenum[D1] = 0;
                 p->pcenumtosq[p->sqtopcenum[A1]] = A1;
            }
            if ( to == G8 )
            {
                 p->board[H8].typ = p->board[F8].typ;
                 p->board[F8].typ = ety;
                 p->board[H8].col = p->board[F8].col;
                 p->board[F8].col = npco;

                  p->sqtopcenum[H8] = p->sqtopcenum[F8];
                 p->sqtopcenum[F8] = 0;
                 p->pcenumtosq[p->sqtopcenum[H8]] = H8;

            }
            if ( to == C8 )
            {
                 p->board[A8].typ = p->board[D8].typ;
                 p->board[D8].typ = ety;
                 p->board[A8].col = p->board[D8].col;
                 p->board[D8].col = npco;

                 p->sqtopcenum[A8] = p->sqtopcenum[D8];
                 p->sqtopcenum[D8] = 0;
                 p->pcenumtosq[p->sqtopcenum[A8]] = A8;

            }
      }
      else if ( flag & oPEP)
      {
           if(p->side == white)
           {
             p->board[to-12].typ = bP;
             p->board[to-12].col = bpco;
             p->material[black] += vP;

             p->sqtopcenum[to-12] = hist[histply].plist;
             p->pcenumtosq[hist[histply].plist] = to-12;
             p->sqtopcenum[to] = 0;
           }
           else
           {
             p->board[to+12].typ = wP;
             p->board[to+12].col = wpco;
             p->material[white] += vP;

             p->sqtopcenum[to+12] = hist[histply].plist;
             p->pcenumtosq[hist[histply].plist] = to+12;
             p->sqtopcenum[to] = 0;
           }
      }

//all done.
}


