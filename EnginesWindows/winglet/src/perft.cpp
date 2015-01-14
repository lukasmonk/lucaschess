#include <iostream>
#include "defines.h"
#include "extglobals.h"
#include "protos.h"
 
U64 perft(int ply, int depth)
{
 
       // Raw node count, up to depth, doing a full tree search.
       // perft is very similar to the search algorithm - instead of evaluating the leaves, we count them.
       //
       // Be carefull when calling this function with depths > 7, because it can take a very long
       // time before the result is returned: the average branching factor in chess is 35, so every
       // increment in depth will require 35x more computer time.
       //
       // perft is a good way of verifying correctness of the movegenerator and (un)makeMove,
       // because you can compare the results with published results for certain test positions.
       //
       // perft is also used to measure the performance of the move generator and (un)makeMove in terms
       // of speed, and to compare different implementations of generating, storing and (un)making moves.
 
       U64 retVal = 0;     
       int i;
 
       // count this node
       if (depth == 0)
       {
              #ifdef WINGLET_DEBUG_EVAL
                     int before = board.eval();
                     board.mirror();
                     int after = board.eval();
                     board.mirror();
                     if (before != after)
                     {
                           std::cout << "evaluation is not symmetrical! " << before << std::endl;
                           for (int j = 0 ; j < board.endOfSearch ; j++)
                     {
                                  std::cout << j+1 << ". ";
                                  displayMove(board.gameLine[j].move);
                                  std::cout <<std::endl;
                           }
                           board.display();
                     }
              #endif
              return 1;
       }
 
       // generate moves from this position
       board.moveBufLen[ply+1] = movegen(board.moveBufLen[ply]);
 
       // loop over moves:
       for (i = board.moveBufLen[ply]; i < board.moveBufLen[ply+1]; i++)
       {
              makeMove(board.moveBuffer[i]);
              {
                     if (!isOtherKingAttacked())
                     {
                           // recursively call perft
                           retVal += perft(ply + 1, depth-1);
 
                           #ifdef WINGLET_DEBUG_PERFT
                           if (depth == 1)
                           {
                                  if (board.moveBuffer[i].isCapture()) ICAPT++;
                                  if (board.moveBuffer[i].isEnpassant()) IEP++;
                                  if (board.moveBuffer[i].isPromotion()) IPROM++;
                                  if (board.moveBuffer[i].isCastleOO()) ICASTLOO++;
                                  if (board.moveBuffer[i].isCastleOOO()) ICASTLOOO++;
                                  if (isOwnKingAttacked()) ICHECK++;
                           }
                          #endif
                     }
              }
              unmakeMove(board.moveBuffer[i]);
       }
       return retVal;
}