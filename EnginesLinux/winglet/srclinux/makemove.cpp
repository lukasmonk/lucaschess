#include <iostream>
#include "defines.h"
#include "protos.h"
#include "extglobals.h"
#include "move.h"

#ifdef WINGLET_DEBUG_MOVES
       void debugMoves(char *, Move &);
#endif

void makeMove(Move &move)
{
       unsigned int from = move.getFrom();
       unsigned int to = move.getTosq();
       unsigned int piece = move.getPiec();
       unsigned int captured = move.getCapt();

       board.gameLine[board.endOfSearch].move.moveInt = move.moveInt;
       board.gameLine[board.endOfSearch].castleWhite  = board.castleWhite;
       board.gameLine[board.endOfSearch].castleBlack  = board.castleBlack;
       board.gameLine[board.endOfSearch].fiftyMove    = board.fiftyMove;
       board.gameLine[board.endOfSearch].epSquare     = board.epSquare;
       board.gameLine[board.endOfSearch].key		  = board.hashkey;

       BitMap fromBitMap  = BITSET[from];
       BitMap fromToBitMap = fromBitMap  | BITSET[to];
	   board.hashkey ^= (KEY.keys[from][piece] ^ KEY.keys[to][piece]);
	   if (board.epSquare) board.hashkey ^= KEY.ep[board.epSquare];

       switch (piece)
       {
              case 1: // white pawn:
                     board.whitePawns           ^= fromToBitMap;
                     board.whitePieces          ^= fromToBitMap;
                     board.square[from]        = EMPTY;
                     board.square[to]          = WHITE_PAWN;
                     board.epSquare            = 0;
                     board.fiftyMove           = 0;
                     if ((RANKS[from] == 2) && (RANKS[to] == 4))
					 {
						board.epSquare = from + 8;
						board.hashkey ^= KEY.ep[from + 8];
					 }
                     if (captured)
                     {
                           if (move.isEnpassant())
                           {
                                  board.blackPawns           ^= BITSET[to-8];
                                  board.blackPieces          ^= BITSET[to-8];
                                  board.occupiedSquares    ^= fromToBitMap | BITSET[to-8];
                                  board.square[to-8]       = EMPTY;
								  board.totalBlackPawns		-= PAWN_VALUE;
                                  board.Material           += PAWN_VALUE;
								  board.hashkey             ^= KEY.keys[to-8][BLACK_PAWN];
                           }
                           else
                           {
                                  makeCapture(captured, to);
                                  board.occupiedSquares ^= fromBitMap;
                           }
                     }
                     else board.occupiedSquares ^= fromToBitMap;

                     if (move.isPromotion())
                     {
                           makeWhitePromotion(move.getProm(), to);
                           board.square[to]         = move.getProm();
                     }
                     break;

              case 2: // white king:
                     board.whiteKing             ^= fromToBitMap;
                     board.whitePieces          ^= fromToBitMap;
                     board.square[from]        = EMPTY;
                     board.square[to]          = WHITE_KING;
                     board.epSquare            = 0;
                     board.fiftyMove++;
					 if (board.castleWhite & CANCASTLEOO)  board.hashkey ^= KEY.wk;
					 if (board.castleWhite & CANCASTLEOOO) board.hashkey ^= KEY.wq;
                     board.castleWhite = 0;
                     if (captured)
                     {
                           makeCapture(captured, to);
                           board.occupiedSquares ^= fromBitMap;
                     }
                     else board.occupiedSquares ^= fromToBitMap;

                     if (move.isCastle())
                     {
                           if (move.isCastleOO())
                           {
                                  board.whiteRooks         ^= BITSET[H1] | BITSET[F1];
                                  board.whitePieces        ^= BITSET[H1] | BITSET[F1];
                                  board.occupiedSquares    ^= BITSET[H1] | BITSET[F1];
                                  board.square[H1]          = EMPTY;
                                  board.square[F1]          = WHITE_ROOK;
             					  board.hashkey ^= (KEY.keys[H1][WHITE_ROOK] ^ KEY.keys[F1][WHITE_ROOK]);
                           }
                           else
                           {
                                  board.whiteRooks         ^= BITSET[A1] | BITSET[D1];
                                  board.whitePieces          ^= BITSET[A1] | BITSET[D1];
                                  board.occupiedSquares    ^= BITSET[A1] | BITSET[D1];
                                  board.square[A1]          = EMPTY;
                                  board.square[D1]          = WHITE_ROOK;
             					  board.hashkey ^= (KEY.keys[A1][WHITE_ROOK] ^ KEY.keys[D1][WHITE_ROOK]);
                           }
                     }
                     break;

              case 3: // white knight:
                     board.whiteKnights         ^= fromToBitMap;
                     board.whitePieces          ^= fromToBitMap;
                     board.square[from]        = EMPTY;
                     board.square[to]          = WHITE_KNIGHT;
                     board.epSquare            = 0;
                     board.fiftyMove++;
                     if (captured)
                     {
                           makeCapture(captured, to);
                           board.occupiedSquares ^= fromBitMap;
                     }
                     else board.occupiedSquares ^= fromToBitMap;
                     break;

              case 5: // white bishop:
                     board.whiteBishops         ^= fromToBitMap;
                     board.whitePieces          ^= fromToBitMap;
                     board.square[from]        = EMPTY;
                     board.square[to]          = WHITE_BISHOP;
                     board.epSquare            = 0;
                     board.fiftyMove++;
                     if (captured)
                     {
                           makeCapture(captured, to);
                           board.occupiedSquares ^= fromBitMap;
                     }
                     else board.occupiedSquares ^= fromToBitMap;
                     break;

              case 6: // white rook:
                     board.whiteRooks         ^= fromToBitMap;
                     board.whitePieces          ^= fromToBitMap;
                     board.square[from]        = EMPTY;
                     board.square[to]          = WHITE_ROOK;
                     board.epSquare            = 0;
                     board.fiftyMove++;
                     if (from == A1)
					 {
					     if (board.castleWhite & CANCASTLEOOO)  board.hashkey ^= KEY.wq;
						 board.castleWhite &= ~CANCASTLEOOO;
					 }
                     if (from == H1)
 					 {
					    if (board.castleWhite & CANCASTLEOO)  board.hashkey ^= KEY.wk;
						board.castleWhite &= ~CANCASTLEOO;
					 }
                     if (captured)
                     {
                           makeCapture(captured, to);
                           board.occupiedSquares ^= fromBitMap;
                     }
                     else board.occupiedSquares ^= fromToBitMap;
                     break;

              case 7: // white queen:
                     board.whiteQueens          ^= fromToBitMap;
                     board.whitePieces          ^= fromToBitMap;
                     board.square[from]        = EMPTY;
                     board.square[to]          = WHITE_QUEEN;
                     board.epSquare            = 0;
                     board.fiftyMove++;
                     if (captured)
                     {
                           makeCapture(captured, to);
                           board.occupiedSquares ^= fromBitMap;
                     }
                     else board.occupiedSquares ^= fromToBitMap;
                     break;

              case 9: // black pawn:
                     board.blackPawns           ^= fromToBitMap;
                     board.blackPieces          ^= fromToBitMap;
                     board.square[from]        = EMPTY;
                     board.square[to]          = BLACK_PAWN;
                     board.epSquare            = 0;
                     board.fiftyMove = 0;
                     if ((RANKS[from] == 7) && (RANKS[to] == 5))
					 {
							board.epSquare = from - 8;
							board.hashkey ^= KEY.ep[from - 8];
					 }
                     if (captured)
                     {
                           if (move.isEnpassant())
                           {
                                  board.whitePawns           ^= BITSET[to+8];
                                  board.whitePieces          ^= BITSET[to+8];
                                  board.occupiedSquares    ^= fromToBitMap | BITSET[to+8];
                                  board.square[to+8]       = EMPTY;
                                  board.totalWhitePawns		-= PAWN_VALUE;
                                  board.Material           -= PAWN_VALUE;
								  board.hashkey             ^= KEY.keys[to+8][WHITE_PAWN];
						   }
                           else
                           {
                                  makeCapture(captured, to);
                                  board.occupiedSquares ^= fromBitMap;
                           }
                     }
                     else board.occupiedSquares ^= fromToBitMap;

                     if (move.isPromotion())
                     {
                           makeBlackPromotion(move.getProm(), to);
                           board.square[to]         = move.getProm();
                     }
                     break;

              case 10: // black king:
                     board.blackKing             ^= fromToBitMap;
                     board.blackPieces          ^= fromToBitMap;
                     board.square[from]        = EMPTY;
                     board.square[to]          = BLACK_KING;
                     board.epSquare            = 0;
                     board.fiftyMove++;
					 if (board.castleBlack & CANCASTLEOO)  board.hashkey ^= KEY.bk;
					 if (board.castleBlack & CANCASTLEOOO) board.hashkey ^= KEY.bq;
                     board.castleBlack = 0;
                     if (captured)
                     {
                           makeCapture(captured, to);
                           board.occupiedSquares ^= fromBitMap;
					 }
                     else board.occupiedSquares ^= fromToBitMap;

                     if (move.isCastle())
					 {
                           if (move.isCastleOO())
                           {
                                  board.blackRooks         ^= BITSET[H8] | BITSET[F8];
                                  board.blackPieces          ^= BITSET[H8] | BITSET[F8];
                                  board.occupiedSquares    ^= BITSET[H8] | BITSET[F8];
                                  board.square[H8]          = EMPTY;
                                  board.square[F8]          = BLACK_ROOK;
             					  board.hashkey ^= (KEY.keys[H8][BLACK_ROOK] ^ KEY.keys[F8][BLACK_ROOK]);
						   }
                           else
                           {
                                  board.blackRooks         ^= BITSET[A8] | BITSET[D8];
                                  board.blackPieces          ^= BITSET[A8] | BITSET[D8];
                                  board.occupiedSquares    ^= BITSET[A8] | BITSET[D8];
                                  board.square[A8]          = EMPTY;
                                  board.square[D8]          = BLACK_ROOK;
             					  board.hashkey ^= (KEY.keys[A8][BLACK_ROOK] ^ KEY.keys[D8][BLACK_ROOK]);
						   }
					 }
                     break;

              case 11: // black knight:
                     board.blackKnights         ^= fromToBitMap;
                     board.blackPieces          ^= fromToBitMap;
                     board.square[from]        = EMPTY;
                     board.square[to]          = BLACK_KNIGHT;
                     board.epSquare            = 0;
                     board.fiftyMove++;
                     if (captured)
                     {
                           makeCapture(captured, to);
                           board.occupiedSquares ^= fromBitMap;
                     }
                     else board.occupiedSquares ^= fromToBitMap;
                     break;

              case 13: // black bishop:
                     board.blackBishops         ^= fromToBitMap;
                     board.blackPieces          ^= fromToBitMap;
                     board.square[from]        = EMPTY;
                     board.square[to]          = BLACK_BISHOP;
                     board.epSquare            = 0;
                     board.fiftyMove++;
                     if (captured)
                     {
                           makeCapture(captured, to);
                           board.occupiedSquares ^= fromBitMap;
                     }
                     else board.occupiedSquares ^= fromToBitMap;
                     break;

              case 14: // black rook:
                     board.blackRooks         ^= fromToBitMap;
                     board.blackPieces          ^= fromToBitMap;
                     board.square[from]        = EMPTY;
                     board.square[to]          = BLACK_ROOK;
                     board.epSquare            = 0;
                     board.fiftyMove++;
                     if (from == A8)
				     {
					     if (board.castleBlack & CANCASTLEOOO)  board.hashkey ^= KEY.bq;
						 board.castleBlack &= ~CANCASTLEOOO;
					 }
                     if (from == H8)
				     {
					     if (board.castleBlack & CANCASTLEOO)  board.hashkey ^= KEY.bk;
						 board.castleBlack &= ~CANCASTLEOO;
					 }
                     if (captured)
                     {
                           makeCapture(captured, to);
                           board.occupiedSquares ^= fromBitMap;
                     }
                     else board.occupiedSquares ^= fromToBitMap;
                     break;

              case 15: // black queen:
                     board.blackQueens          ^= fromToBitMap;
                     board.blackPieces          ^= fromToBitMap;
                     board.square[from]        = EMPTY;
                     board.square[to]          = BLACK_QUEEN;
                     board.epSquare            = 0;
                     board.fiftyMove++;
                     if (captured)
                     {
                           makeCapture(captured, to);
                           board.occupiedSquares ^= fromBitMap;
                     }
                     else board.occupiedSquares ^= fromToBitMap;
                     break;
       }


       board.nextMove = !board.nextMove;
	   board.hashkey ^= KEY.side;
       board.endOfSearch++;

#ifdef WINGLET_DEBUG_MOVES
       debugMoves("makemove", move);
#endif

}

void unmakeMove(Move &move)
{
       unsigned int piece = move.getPiec();
       unsigned int captured = move.getCapt();
       unsigned int from = move.getFrom();
       unsigned int to = move.getTosq();

       BitMap fromBitMap  = BITSET[from];
       BitMap fromToBitMap = fromBitMap  | BITSET[to];

       switch (piece)
       {
              case 1: // white pawn:
                     board.whitePawns           ^= fromToBitMap;
                     board.whitePieces          ^= fromToBitMap;
                     board.square[from]        = WHITE_PAWN;
                     board.square[to]          = EMPTY;
                     if (captured)
                     {
                           if (move.isEnpassant())
                           {
                                  board.blackPawns           ^= BITSET[to-8];
                                  board.blackPieces          ^= BITSET[to-8];
                                  board.occupiedSquares    ^= fromToBitMap | BITSET[to-8];
                                  board.square[to-8]        = BLACK_PAWN;
                                  board.totalBlackPawns		+= PAWN_VALUE;
                                  board.Material           -= PAWN_VALUE;
                           }
                           else
                           {
                                  unmakeCapture(captured, to);
                                  board.occupiedSquares ^= fromBitMap;
                           }
                     }
                     else board.occupiedSquares ^= fromToBitMap;

                     if (move.isPromotion())
                     {
                           unmakeWhitePromotion(move.getProm(), to);
                     }
                     break;

              case 2: // white king:
                     board.whiteKing             ^= fromToBitMap;
                     board.whitePieces          ^= fromToBitMap;
                     board.square[from]        = WHITE_KING;
                     board.square[to]          = EMPTY;
                     if (captured)
                     {
                           unmakeCapture(captured, to);
                           board.occupiedSquares ^= fromBitMap;
                     }
                     else board.occupiedSquares ^= fromToBitMap;

                     if (move.isCastle())
                     {
                           if (move.isCastleOO())
                           {
                                  board.whiteRooks         ^= BITSET[H1] | BITSET[F1];
                                  board.whitePieces          ^= BITSET[H1] | BITSET[F1];
                                  board.occupiedSquares    ^= BITSET[H1] | BITSET[F1];
                                  board.square[H1]          = WHITE_ROOK;
                                  board.square[F1]          = EMPTY;
                           }
                           else
                           {
                                  board.whiteRooks         ^= BITSET[A1] | BITSET[D1];
                                  board.whitePieces          ^= BITSET[A1] | BITSET[D1];
                                  board.occupiedSquares    ^= BITSET[A1] | BITSET[D1];
                                  board.square[A1]          = WHITE_ROOK;
                                  board.square[D1]          = EMPTY;
                           }
                     }
                     break;

              case 3: // white knight:
                     board.whiteKnights         ^= fromToBitMap;
                     board.whitePieces          ^= fromToBitMap;
                     board.square[from]        = WHITE_KNIGHT;
                     board.square[to]          = EMPTY;
                     if (captured)
                     {
                           unmakeCapture(captured, to);
                           board.occupiedSquares ^= fromBitMap;
                     }
                     else board.occupiedSquares ^= fromToBitMap;
                     break;

              case 5: // white bishop:
                     board.whiteBishops         ^= fromToBitMap;
                     board.whitePieces          ^= fromToBitMap;
                     board.square[from]        = WHITE_BISHOP;
                     board.square[to]          = EMPTY;
                     if (captured)
                     {
                           unmakeCapture(captured, to);
                           board.occupiedSquares ^= fromBitMap;
                     }
                     else board.occupiedSquares ^= fromToBitMap;

                     break;

              case 6: // white rook:
                     board.whiteRooks         ^= fromToBitMap;
                     board.whitePieces          ^= fromToBitMap;
                     board.square[from]        = WHITE_ROOK;
                     board.square[to]          = EMPTY;
                     if (captured)
                     {
                           unmakeCapture(captured, to);
                           board.occupiedSquares ^= fromBitMap;
                     }
                     else board.occupiedSquares ^= fromToBitMap;
                     break;

              case 7: // white queen:
                     board.whiteQueens          ^= fromToBitMap;
                     board.whitePieces          ^= fromToBitMap;
                     board.square[from]        = WHITE_QUEEN;
                     board.square[to]          = EMPTY;
                     if (captured)
                     {
                           unmakeCapture(captured, to);
                           board.occupiedSquares ^= fromBitMap;
                     }
                     else board.occupiedSquares ^= fromToBitMap;
                     break;

              case 9: // black pawn:
                     board.blackPawns           ^= fromToBitMap;
                     board.blackPieces          ^= fromToBitMap;
                     board.square[from]        = BLACK_PAWN;
                     board.square[to]          = EMPTY;
                     if (captured)
                     {
                           if (move.isEnpassant())
                           {
                                  board.whitePawns           ^= BITSET[to+8];
                                  board.whitePieces          ^= BITSET[to+8];
                                  board.occupiedSquares    ^= fromToBitMap | BITSET[to+8];
                                  board.square[to+8]        = WHITE_PAWN;
								  board.totalWhitePawns		+= PAWN_VALUE;
                                  board.Material           += PAWN_VALUE;
                           }
                           else
                           {
                                  unmakeCapture(captured, to);
                                  board.occupiedSquares ^= fromBitMap;
                           }
                     }
                     else board.occupiedSquares ^= fromToBitMap;

                     if (move.isPromotion())
                     {
                           unmakeBlackPromotion(move.getProm(), to);
                     }
                     break;

              case 10: // black king:
                     board.blackKing             ^= fromToBitMap;
                     board.blackPieces          ^= fromToBitMap;
                     board.square[from]        = BLACK_KING;
                     board.square[to]          = EMPTY;
                     if (captured)
                     {
                           unmakeCapture(captured, to);
                           board.occupiedSquares ^= fromBitMap;
                     }
                     else board.occupiedSquares ^= fromToBitMap;

                     if (move.isCastle())
                     {
                           if (move.isCastleOO())
                           {
                                  board.blackRooks         ^= BITSET[H8] | BITSET[F8];
                                  board.blackPieces          ^= BITSET[H8] | BITSET[F8];
                                  board.occupiedSquares    ^= BITSET[H8] | BITSET[F8];
                                  board.square[H8]          = BLACK_ROOK;
                                  board.square[F8]          = EMPTY;
                           }
                           else
                           {
                                  board.blackRooks         ^= BITSET[A8] | BITSET[D8];
                                  board.blackPieces          ^= BITSET[A8] | BITSET[D8];
                                  board.occupiedSquares    ^= BITSET[A8] | BITSET[D8];
                                  board.square[A8]          = BLACK_ROOK;
                                  board.square[D8]          = EMPTY;
                           }
                     }
                     break;

              case 11: // black knight:
                     board.blackKnights         ^= fromToBitMap;
                     board.blackPieces          ^= fromToBitMap;
                     board.square[from]        = BLACK_KNIGHT;
                     board.square[to]          = EMPTY;
                     if (captured)
                     {
                           unmakeCapture(captured, to);
                           board.occupiedSquares ^= fromBitMap;
                     }
                     else board.occupiedSquares ^= fromToBitMap;
                     break;

              case 13: // black bishop:
                     board.blackBishops         ^= fromToBitMap;
                     board.blackPieces          ^= fromToBitMap;
                     board.square[from]        = BLACK_BISHOP;
                     board.square[to]          = EMPTY;
                     if (captured)
                     {
                           unmakeCapture(captured, to);
                           board.occupiedSquares ^= fromBitMap;
                     }
                     else board.occupiedSquares ^= fromToBitMap;
                     break;

              case 14: // black rook:
                     board.blackRooks         ^= fromToBitMap;
                     board.blackPieces          ^= fromToBitMap;
                     board.square[from]        = BLACK_ROOK;
                     board.square[to]          = EMPTY;
                     if (captured)
                     {
                           unmakeCapture(captured, to);
                           board.occupiedSquares ^= fromBitMap;
                     }
                     else board.occupiedSquares ^= fromToBitMap;
                     break;

              case 15: // black queen:
                     board.blackQueens          ^= fromToBitMap;
                     board.blackPieces          ^= fromToBitMap;
                     board.square[from]        = BLACK_QUEEN;
                     board.square[to]          = EMPTY;
                     if (captured)
                     {
                           unmakeCapture(captured, to);
                           board.occupiedSquares ^= fromBitMap;
                     }
                     else board.occupiedSquares ^= fromToBitMap;
                     break;
       }

       board.nextMove = !board.nextMove;


       board.endOfSearch--;
       board.castleWhite         = board.gameLine[board.endOfSearch].castleWhite;
       board.castleBlack         = board.gameLine[board.endOfSearch].castleBlack;
       board.epSquare            = board.gameLine[board.endOfSearch].epSquare;
       board.fiftyMove           = board.gameLine[board.endOfSearch].fiftyMove;
	   board.hashkey			 = board.gameLine[board.endOfSearch].key;

#ifdef WINGLET_DEBUG_MOVES
       debugMoves("unmakemove", move);
#endif
	   return;
}

void makeCapture(unsigned int &captured, unsigned int &to)
{
       // deals with all captures, except en-passant
       BitMap toBitMap;
       toBitMap = BITSET[to];
	   board.hashkey ^= KEY.keys[to][captured];

       switch (captured)
       {
              case 1: // white pawn:
                     board.whitePawns           ^= toBitMap;
                     board.whitePieces          ^= toBitMap;
                     board.totalWhitePawns     -= PAWN_VALUE;
                     board.Material           -= PAWN_VALUE;
                     break;

              case 2: // white king:
                     board.whiteKing             ^= toBitMap;
                     board.whitePieces          ^= toBitMap;
                     break;

              case 3: // white knight:
                     board.whiteKnights         ^= toBitMap;
                     board.whitePieces          ^= toBitMap;
                     board.totalWhitePieces     -= KNIGHT_VALUE;
                     board.Material           -= KNIGHT_VALUE;
                     break;

              case 5: // white bishop:
                     board.whiteBishops         ^= toBitMap;
                     board.whitePieces          ^= toBitMap;
                     board.totalWhitePieces     -= BISHOP_VALUE;
                     board.Material           -= BISHOP_VALUE;
                     break;

              case 6: // white rook:
                     board.whiteRooks         ^= toBitMap;
                     board.whitePieces          ^= toBitMap;
                     board.totalWhitePieces     -= ROOK_VALUE;
                     board.Material           -= ROOK_VALUE;
                     if (to == A1)
					 {
					     if (board.castleWhite & CANCASTLEOOO)  board.hashkey ^= KEY.wq;
						 board.castleWhite &= ~CANCASTLEOOO;
					 }
                     if (to == H1)
					 {
					     if (board.castleWhite & CANCASTLEOO)  board.hashkey ^= KEY.wk;
						 board.castleWhite &= ~CANCASTLEOO;
					 }
                     break;

              case 7: // white queen:
                     board.whiteQueens          ^= toBitMap;
                     board.whitePieces          ^= toBitMap;
                     board.totalWhitePieces     -= QUEEN_VALUE;
                     board.Material           -= QUEEN_VALUE;
                     break;

              case 9: // black pawn:
                     board.blackPawns           ^= toBitMap;
                     board.blackPieces          ^= toBitMap;
                     board.totalBlackPawns     -= PAWN_VALUE;
                     board.Material           += PAWN_VALUE;
                     break;

              case 10: // black king:
                     board.blackKing             ^= toBitMap;
                     board.blackPieces          ^= toBitMap;
                     break;

              case 11: // black knight:
                     board.blackKnights         ^= toBitMap;
                     board.blackPieces          ^= toBitMap;
                     board.totalBlackPieces     -= KNIGHT_VALUE;
                     board.Material           += KNIGHT_VALUE;
                     break;

              case 13: // black bishop:
                     board.blackBishops         ^= toBitMap;
                     board.blackPieces          ^= toBitMap;
                     board.totalBlackPieces     -= BISHOP_VALUE;
                     board.Material           += BISHOP_VALUE;
                     break;

              case 14: // black rook:
                     board.blackRooks         ^= toBitMap;
                     board.blackPieces          ^= toBitMap;
                     board.totalBlackPieces     -= ROOK_VALUE;
                     board.Material           += ROOK_VALUE;
                     if (to == A8)
					 {
					     if (board.castleBlack & CANCASTLEOOO)  board.hashkey ^= KEY.bq;
					     board.castleBlack &= ~CANCASTLEOOO;
					 }
                     if (to == H8)
					 {
					     if (board.castleBlack & CANCASTLEOO)  board.hashkey ^= KEY.bk;
						 board.castleBlack &= ~CANCASTLEOO;
					 }
                     break;

              case 15: // black queen:
                     board.blackQueens          ^= toBitMap;
                     board.blackPieces          ^= toBitMap;
                     board.totalBlackPieces     -= QUEEN_VALUE;
                     board.Material           += QUEEN_VALUE;
                     break;
       }
       board.fiftyMove = 0;
}

void unmakeCapture(unsigned int &captured, unsigned int &to)
{
       // deals with all captures, except en-passant
       BitMap toBitMap;
       toBitMap = BITSET[to];

       switch (captured)
       {
              case 1: // white pawn:
                     board.whitePawns           ^= toBitMap;
                     board.whitePieces          ^= toBitMap;
                     board.square[to]             = WHITE_PAWN;
					 board.totalWhitePawns	  += PAWN_VALUE;
                     board.Material           += PAWN_VALUE;
                     break;

              case 2: // white king:
                     board.whiteKing             ^= toBitMap;
                     board.whitePieces          ^= toBitMap;
                     board.square[to]             = WHITE_KING;
                     break;

              case 3: // white knight:
                     board.whiteKnights         ^= toBitMap;
                     board.whitePieces          ^= toBitMap;
                     board.square[to]             = WHITE_KNIGHT;
					 board.totalWhitePieces	  += KNIGHT_VALUE;
                     board.Material           += KNIGHT_VALUE;
                     break;

              case 5: // white bishop:
                     board.whiteBishops         ^= toBitMap;
                     board.whitePieces          ^= toBitMap;
                     board.square[to]             = WHITE_BISHOP;
					 board.totalWhitePieces	  += BISHOP_VALUE;
					 board.Material           += BISHOP_VALUE;
                     break;

              case 6: // white rook:
                     board.whiteRooks         ^= toBitMap;
                     board.whitePieces          ^= toBitMap;
                     board.square[to]             = WHITE_ROOK;
					 board.totalWhitePieces	  += ROOK_VALUE;
                     board.Material           += ROOK_VALUE;
                     break;

              case 7: // white queen:
                     board.whiteQueens          ^= toBitMap;
                     board.whitePieces          ^= toBitMap;
                     board.square[to]             = WHITE_QUEEN;
					 board.totalWhitePieces	  += QUEEN_VALUE;
                     board.Material           += QUEEN_VALUE;
                     break;

              case 9: // black pawn:
                     board.blackPawns           ^= toBitMap;
                     board.blackPieces          ^= toBitMap;
                     board.square[to]             = BLACK_PAWN;
					 board.totalBlackPawns	   += PAWN_VALUE;
                     board.Material           -= PAWN_VALUE;
                     break;

              case 10: // black king:
                     board.blackKing             ^= toBitMap;
                     board.blackPieces          ^= toBitMap;
                     board.square[to]             = BLACK_KING;
                     break;

              case 11: // black knight:
                     board.blackKnights         ^= toBitMap;
                     board.blackPieces          ^= toBitMap;
                     board.square[to]             = BLACK_KNIGHT;
					 board.totalBlackPieces    += KNIGHT_VALUE;
                     board.Material           -= KNIGHT_VALUE;
                     break;

              case 13: // black bishop:
                     board.blackBishops         ^= toBitMap;
                     board.blackPieces          ^= toBitMap;
                     board.square[to]             = BLACK_BISHOP;
					 board.totalBlackPieces    += BISHOP_VALUE;
                     board.Material           -= BISHOP_VALUE;
                     break;

              case 14: // black rook:
                     board.blackRooks         ^= toBitMap;
                     board.blackPieces          ^= toBitMap;
                     board.square[to]             = BLACK_ROOK;
					 board.totalBlackPieces    += ROOK_VALUE;
                     board.Material           -= ROOK_VALUE;
                     break;

              case 15: // black queen:
                     board.blackQueens          ^= toBitMap;
                     board.blackPieces          ^= toBitMap;
                     board.square[to]             = BLACK_QUEEN;
					 board.totalBlackPieces    += QUEEN_VALUE;
                     board.Material           -= QUEEN_VALUE;
                     break;
       }
}

void makeWhitePromotion(unsigned int prom, unsigned int &to)
{
       BitMap toBitMap;
       toBitMap = BITSET[to];

       board.whitePawns ^= toBitMap;
	   board.totalWhitePawns -= PAWN_VALUE;
       board.Material -= PAWN_VALUE;
	   board.hashkey ^= (KEY.keys[to][WHITE_PAWN] ^ KEY.keys[to][prom]);

       if (prom == 7)
       {
              board.whiteQueens          ^= toBitMap;
			  board.totalWhitePieces    += QUEEN_VALUE;
              board.Material           += QUEEN_VALUE;
       }
       else if (prom == 6)
       {
              board.whiteRooks         ^= toBitMap;
			  board.totalWhitePieces    += ROOK_VALUE;
              board.Material           += ROOK_VALUE;
       }
       else if (prom == 5)
       {
              board.whiteBishops       ^= toBitMap;
			  board.totalWhitePieces    += BISHOP_VALUE;
              board.Material           += BISHOP_VALUE;
       }
       else if (prom == 3)
       {
              board.whiteKnights       ^= toBitMap;
			  board.totalWhitePieces    += KNIGHT_VALUE;
              board.Material           += KNIGHT_VALUE;
       }
}

void unmakeWhitePromotion(unsigned int prom, unsigned int &to)
{
       BitMap toBitMap;
       toBitMap = BITSET[to];

       board.whitePawns ^= toBitMap;
	   board.totalWhitePawns += PAWN_VALUE;
       board.Material += PAWN_VALUE;

       if (prom == 7)
       {
              board.whiteQueens          ^= toBitMap;
			  board.totalWhitePieces    -= QUEEN_VALUE;
              board.Material           -= QUEEN_VALUE;
       }
       else if (prom == 6)
       {
              board.whiteRooks         ^= toBitMap;
			  board.totalWhitePieces    -= ROOK_VALUE;
              board.Material           -= ROOK_VALUE;
       }
       else if (prom == 5)
       {
              board.whiteBishops       ^= toBitMap;
			  board.totalWhitePieces    -= BISHOP_VALUE;
              board.Material           -= BISHOP_VALUE;
       }
       else if (prom == 3)
       {
              board.whiteKnights       ^= toBitMap;
			  board.totalWhitePieces    -= KNIGHT_VALUE;
              board.Material           -= KNIGHT_VALUE;
       }
}


void makeBlackPromotion(unsigned int prom, unsigned int &to)
{
       BitMap toBitMap;
       toBitMap = BITSET[to];

       board.blackPawns ^= toBitMap;
	   board.totalBlackPawns -= PAWN_VALUE;
       board.Material += PAWN_VALUE;
	   board.hashkey ^= (KEY.keys[to][BLACK_PAWN] ^ KEY.keys[to][prom]);

       if (prom == 15)
       {
              board.blackQueens          ^= toBitMap;
			  board.totalBlackPieces    += QUEEN_VALUE;
              board.Material           -= QUEEN_VALUE;
       }
       else if (prom == 14)
       {
              board.blackRooks         ^= toBitMap;
			  board.totalBlackPieces    += ROOK_VALUE;
              board.Material           -= ROOK_VALUE;
       }
       else if (prom == 13)
       {
              board.blackBishops       ^= toBitMap;
			  board.totalBlackPieces    += BISHOP_VALUE;
              board.Material           -= BISHOP_VALUE;
       }
       else if (prom == 11)
       {
              board.blackKnights       ^= toBitMap;
			  board.totalBlackPieces    += KNIGHT_VALUE;
              board.Material           -= KNIGHT_VALUE;
       }
}

void unmakeBlackPromotion(unsigned int prom, unsigned int &to)
{
       BitMap toBitMap;
       toBitMap = BITSET[to];

       board.blackPawns ^= toBitMap;
	   board.totalBlackPawns += PAWN_VALUE;
       board.Material -= PAWN_VALUE;

       if (prom == 15)
       {
              board.blackQueens          ^= toBitMap;
			  board.totalBlackPieces    -= QUEEN_VALUE;
              board.Material           += QUEEN_VALUE;
       }
       else if (prom == 14)
       {
              board.blackRooks         ^= toBitMap;
			  board.totalBlackPieces    -= ROOK_VALUE;
              board.Material           += ROOK_VALUE;
       }
       else if (prom == 13)
       {
              board.blackBishops       ^= toBitMap;
			  board.totalBlackPieces    -= BISHOP_VALUE;
              board.Material           += BISHOP_VALUE;
       }
       else if (prom == 11)
       {
              board.blackKnights       ^= toBitMap;
			  board.totalBlackPieces    -= KNIGHT_VALUE;
              board.Material           += KNIGHT_VALUE;
       }
}

BOOLTYPE isOwnKingAttacked()
{
       // check to see if we are leaving our king in check
       if (board.nextMove)
       {
           return isAttacked(board.blackKing, !board.nextMove);
       }
       else
       {
           return isAttacked(board.whiteKing, !board.nextMove);
       }
}

BOOLTYPE isOtherKingAttacked()
{
       // check to see if we are leaving our king in check
       if (board.nextMove)
       {
           return isAttacked(board.whiteKing, board.nextMove);
       }
       else
       {
           return isAttacked(board.blackKing, board.nextMove);
       }
}

BOOLTYPE isValidTextMove(char *userMove, Move &move)
{
       // Checks if userMove is valid by comparing it with moves from the move generator
       // If found valid, the move is returned

       unsigned char userFrom, userTo, userPromote;
       BOOLTYPE moveFound;
       int i;

       if (strlen(userMove) > 3)
       {
              userFrom = userMove[0] - 97;
              userFrom += 8 * (userMove[1] - 49);
              userTo = userMove[2] - 97;
              userTo += 8 * (userMove[3] - 49);
       }

       userPromote = 0;
       if (strlen(userMove) > 4)
       {
              if (board.nextMove == WHITE_MOVE)
              {
                     switch (userMove[4])
                     {
                           case 'q': userPromote = WHITE_QUEEN; break;
                           case 'r': userPromote = WHITE_ROOK; break;
                           case 'b': userPromote = WHITE_BISHOP; break;
                           case 'n': userPromote = WHITE_KNIGHT; break;
                     }
              }
              else
                     switch (userMove[4])
                     {
                           case 'q': userPromote = BLACK_QUEEN; break;
                           case 'r': userPromote = BLACK_ROOK; break;
                           case 'b': userPromote = BLACK_BISHOP; break;
                           case 'n': userPromote = BLACK_KNIGHT; break;
                     }
       }

       moveFound = false;
       for (i = board.moveBufLen[0]; i < board.moveBufLen[1]; i++)
       {
              if ((board.moveBuffer[i].getFrom() == userFrom) && (board.moveBuffer[i].getTosq() == userTo))
              {
                     if (((board.square[userFrom] == WHITE_PAWN) && (RANKS[userFrom] == 7)) ||
                         ((board.square[userFrom] == BLACK_PAWN) && (RANKS[userFrom] == 2)))
                     {
                           if (board.moveBuffer[i].getProm() == userPromote)
                           {
                                  move.moveInt = board.moveBuffer[i].moveInt;
                                  return true;
                           }
                     }
                     else
                     {
                           move.moveInt = board.moveBuffer[i].moveInt;
                           return true;
                     }
              }
       }
       move.moveInt = 0;
       return false;
}

#ifdef WINGLET_DEBUG_MOVES

void debugMoves(char *caller, Move &move)
{

       // check if board.square, piece bitmaps and whitepieces and blackpiese and occupied square are all consistent after (un)makmove

       char input[80];
       int mat, i, j;
	   int totwpawn, totbpawn, totwpiec, totbpiec;

	 	U64 key;

       // check if both kings are present
       if ((bitCnt(board.whiteKing) != 1) || (bitCnt(board.blackKing) != 1))
       {
              std::cout << "king captured after  " << caller << std::endl;
              displayMove(move); std::cout << std::endl;
              for (j = 0 ; j < board.endOfSearch ; j++)
       {
                     std::cout << j+1 << ". ";
                     displayMove(board.gameLine[j].move);
                     std::cout <<std::endl;
              }
              board.display();
              std::cin >> input;
       }

       // check if board.square and piece bitmaps and whitepieces and blackpiese and occupied square are all telling the same
       for (i = 0 ; i < 64 ; i++)
       {
              if ((board.blackQueens & BITSET[i]) && (board.square[i] != BLACK_QUEEN))
              {
                     std::cout << "inconsistency in black queens after " << caller << std::endl;
                     displayMove(move); std::cout << std::endl;
                     for (j = 0 ; j < board.endOfSearch ; j++)
              {
                           std::cout << j+1 << ". ";
                           displayMove(board.gameLine[j].move);
                           std::cout <<std::endl;
                     }
                     board.display();
                     std::cin >> input;
              }
              if ((board.blackRooks & BITSET[i]) && (board.square[i] != BLACK_ROOK))
              {
                     std::cout << "inconsistency in black rooks after " << caller << std::endl;
                     displayMove(move); std::cout << std::endl;
                     for (j = 0 ; j < board.endOfSearch ; j++)
              {
                           std::cout << j+1 << ". ";
                           displayMove(board.gameLine[j].move);
                           std::cout <<std::endl;
                     }
                     board.display();
                     displayBitmap(board.blackRooks);
                     std::cin >> input;
              }
              if ((board.blackBishops & BITSET[i]) && (board.square[i] != BLACK_BISHOP))
              {
                     std::cout << "inconsistency in black bishops after " << caller << std::endl;
                     displayMove(move); std::cout << std::endl;
                     for (j = 0 ; j < board.endOfSearch ; j++)
              {
                           std::cout << j+1 << ". ";
                           displayMove(board.gameLine[j].move);
                           std::cout <<std::endl;
                     }
                     board.display();
                     std::cin >> input;
              }
              if ((board.blackKnights & BITSET[i]) && (board.square[i] != BLACK_KNIGHT))
              {
                     std::cout << "inconsistency in black knights after " << caller << std::endl;
                     displayMove(move); std::cout << std::endl;
                     displayMove(move);
                     for (j = 0 ; j < board.endOfSearch ; j++)
              {
                           std::cout << j+1 << ". ";
                           displayMove(board.gameLine[j].move);
                           std::cout <<std::endl;
                     }
                     board.display();
                     std::cin >> input;
              }
              if ((board.blackKing & BITSET[i]) && (board.square[i] != BLACK_KING))
              {
                     std::cout << "inconsistency in black king after " << caller << std::endl;
                     displayMove(move); std::cout << std::endl;
                     for (j = 0 ; j < board.endOfSearch ; j++)
              {
                           std::cout << j+1 << ". ";
                           displayMove(board.gameLine[j].move);
                           std::cout <<std::endl;
                     }
                     board.display();
                     std::cin >> input;
              }
              if ((board.blackPawns & BITSET[i]) && (board.square[i] != BLACK_PAWN))
              {
                     std::cout << "inconsistency in black pawns after " << caller << std::endl;
                     displayMove(move); std::cout << std::endl;
                     for (j = 0 ; j < board.endOfSearch ; j++)
              {
                           std::cout << j+1 << ". ";
                           displayMove(board.gameLine[j].move);
                           std::cout <<std::endl;
                     }
                     board.display();
                     displayBitmap(board.blackPawns);
                     std::cout.flush();
                     std::cin >> input;
              }
              if ((board.whiteQueens & BITSET[i]) && (board.square[i] != WHITE_QUEEN))
              {
                     std::cout << "inconsistency in white queens after " << caller << std::endl;
                     displayMove(move); std::cout << std::endl;
                     for (j = 0 ; j < board.endOfSearch ; j++)
              {
                           std::cout << j+1 << ". ";
                           displayMove(board.gameLine[j].move);
                           std::cout <<std::endl;
                     }
                     board.display();
                     std::cin >> input;
              }
              if ((board.whiteRooks & BITSET[i]) && (board.square[i] != WHITE_ROOK))
              {
                     std::cout << "inconsistency in white rooks after " << caller << std::endl;
                     displayMove(move); std::cout << std::endl;
                     for (j = 0 ; j < board.endOfSearch ; j++)
              {
                           std::cout << j+1 << ". ";
                           displayMove(board.gameLine[j].move);
                            std::cout <<std::endl;
                     }
                     board.display();
                     displayBitmap(board.whiteRooks);
                     std::cin >> input;
              }
              if ((board.whiteBishops & BITSET[i]) && (board.square[i] != WHITE_BISHOP))
              {
                     std::cout << "inconsistency in white bishops after " << caller << std::endl;
                     displayMove(move); std::cout << std::endl;
                     for (j = 0 ; j < board.endOfSearch ; j++)
              {
                           std::cout << j+1 << ". ";
                           displayMove(board.gameLine[j].move);
                           std::cout <<std::endl;
                     }
                     board.display();
                     std::cin >> input;
              }
              if ((board.whiteKnights & BITSET[i]) && (board.square[i] != WHITE_KNIGHT))
              {
                     std::cout << "inconsistency in white knights  after " << caller << std::endl;
                     displayMove(move); std::cout << std::endl;
                     for (j = 0 ; j < board.endOfSearch ; j++)
              {
                           std::cout << j+1 << ". ";
                           displayMove(board.gameLine[j].move);
                           std::cout <<std::endl;
                     }
                     board.display();
                     std::cin >> input;
              }
              if ((board.whiteKing & BITSET[i]) && (board.square[i] != WHITE_KING))
              {
                     std::cout << "inconsistency in white king after " << caller << std::endl;
                     displayMove(move); std::cout << std::endl;
                     for (j = 0 ; j < board.endOfSearch ; j++)
              {
                           std::cout << j+1 << ". ";
                           displayMove(board.gameLine[j].move);
                            std::cout <<std::endl;
                     }
                     board.display();
                     std::cin >> input;
              }
              if ((board.whitePawns & BITSET[i]) && (board.square[i] != WHITE_PAWN))
              {
                     std::cout << "inconsistency in white pawns after " << caller << std::endl;
                     displayMove(move); std::cout << std::endl;
                     for (j = 0 ; j < board.endOfSearch ; j++)
              {
                           std::cout << j+1 << ". ";
                           displayMove(board.gameLine[j].move);
                           std::cout <<std::endl;
                     }
                     board.display();
                     std::cin >> input;
              }
       }

       if (board.whitePieces != (board.whiteKing | board.whiteQueens | board.whiteRooks | board.whiteBishops | board.whiteKnights | board.whitePawns ))
       {
              std::cout << "inconsistency in whitePieces after " << caller << std::endl;
              displayMove(move); std::cout << std::endl;
              for (j = 0 ; j < board.endOfSearch ; j++)
       {
                     std::cout << j+1 << ". ";
                     displayMove(board.gameLine[j].move);
                     std::cout <<std::endl;
              }
              board.display();
              displayBitmap(board.whitePieces);
              std::cin >> input;
       }

       if (board.blackPieces != (board.blackKing | board.blackQueens | board.blackRooks | board.blackBishops | board.blackKnights | board.blackPawns ))
       {
              std::cout << "inconsistency in blackPieces after " << caller << std::endl;
              displayMove(move); std::cout << std::endl;
              for (j = 0 ; j < board.endOfSearch ; j++)
       {
                     std::cout << j+1 << ". ";
                     displayMove(board.gameLine[j].move);
                     std::cout <<std::endl;
              }
              board.display();
              displayBitmap(board.blackPieces);
              std::cin >> input;
       }

       if (board.occupiedSquares != (board.whitePieces | board.blackPieces))
       {
              std::cout << "inconsistency in occupiedSquares after " << caller << std::endl;
              displayMove(move); std::cout << std::endl;
              for (j = 0 ; j < board.endOfSearch ; j++)
       {
                     std::cout << j+1 << ". ";
                     displayMove(board.gameLine[j].move);
                     std::cout <<std::endl;
              }
              board.display();
              displayBitmap(board.occupiedSquares);
              std::cin >> input;
       }

		totwpawn = bitCnt(board.whitePawns) * PAWN_VALUE;
		totbpawn = bitCnt(board.blackPawns) * PAWN_VALUE;
		totwpiec = 	bitCnt(board.whiteKnights) * KNIGHT_VALUE + bitCnt(board.whiteBishops) * BISHOP_VALUE +
							bitCnt(board.whiteRooks) * ROOK_VALUE + bitCnt(board.whiteQueens) * QUEEN_VALUE;
		totbpiec = 	bitCnt(board.blackKnights) * KNIGHT_VALUE + bitCnt(board.blackBishops) * BISHOP_VALUE +
							bitCnt(board.blackRooks) * ROOK_VALUE + bitCnt(board.blackQueens) * QUEEN_VALUE;
		mat = totwpawn + totwpiec - totbpawn - totbpiec;

       if (board.totalWhitePawns != totwpawn)
       {
              std::cout << "inconsistency in white pawn material after " << caller << std::endl;
              displayMove(move);
              board.display();
              std::cin >> input;
       }
       if (board.totalBlackPawns != totbpawn)
       {
              std::cout << "inconsistency in black pawn material after " << caller << std::endl;
              displayMove(move);
              board.display();
              std::cin >> input;
       }
       if (board.totalWhitePieces!= totwpiec)
       {
              std::cout << "inconsistency in white pieces material after " << caller << std::endl;
              displayMove(move);
              board.display();
              std::cin >> input;
       }
       if (board.totalBlackPieces != totbpiec)
       {
              std::cout << "inconsistency in black pieces material after " << caller << std::endl;
              displayMove(move);
              board.display();
              std::cin >> input;
       }

       if (board.Material != mat)
       {
              std::cout << "inconsistency in material after " << caller << std::endl;
              displayMove(move);
              board.display();
              std::cin >> input;
       }

	key = 0;
	for (i = 0; i < 64; i++)
	{
		if (board.square[i] != EMPTY) key ^= KEY.keys[i][board.square[i]];
	}
	if (board.castleWhite & CANCASTLEOO)  key ^= KEY.wk;
	if (board.castleWhite & CANCASTLEOOO) key ^= KEY.wq;
	if (board.castleBlack & CANCASTLEOO)  key ^= KEY.bk;
	if (board.castleBlack & CANCASTLEOOO) key ^= KEY.bq;
	if (board.nextMove) key ^= KEY.side;
	if (board.epSquare) key ^= KEY.ep[board.epSquare];
    if (board.hashkey != key)
    {
		std::cout << "inconsistency in hashkey after " << caller << std::endl;
        displayMove(move);
        board.display();
        std::cin >> input;
	}
	return;
}
#endif
