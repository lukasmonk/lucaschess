#ifndef _CRT_SECURE_NO_DEPRECATE
#define _CRT_SECURE_NO_DEPRECATE
#endif
#include <iostream>
#include <iomanip>
#include "defines.h"
#include "protos.h"
#include "extglobals.h"
 
void dataInit()
{
//     ===========================================================================
//	   Initialization of global data at program startup.
//	   This function should be called only once (or else the mirrored data will be 
//	   mirrored back again!!)
//     ===========================================================================
       unsigned char CHARBITSET[8];
       int i, square, rank, file, arank, afile, state, slide, diaga1h8, diaga8h1, attackbit;
       unsigned char state6Bit, state8Bit, attack8Bit;
       Move move;

	   board.searchDepth = 16; // default for startup
	   board.maxTime = 2000; // default for startup, milliseconds

//     ===========================================================================
//     BITSET has only one bit set:
//     ===========================================================================
       BITSET[0] = 0x1;
       for (i = 1; i < 64 ; i++)
       {
			BITSET[i] = BITSET[i-1] << 1;
       }
 
//     ===========================================================================
//     BOARDINDEX is used to translate [file][rank] to [square],
//     Note that file is from 1..8 and rank from 1..8 (not starting from 0)
//     ===========================================================================
       for (rank = 0 ; rank < 9; rank++)
       {
              for (file = 0 ; file < 9; file++)
              {
                     BOARDINDEX[file][rank] = (rank-1) * 8 + file - 1;
              }
       }
 
//     ===========================================================================
//     PIECEVALUES are used in quiesence move ordering only:
//     ===========================================================================
       for (i = 0; i < 16 ; i++) PIECEVALUES[i] = 0;
       PIECEVALUES[WHITE_PAWN] = PAWN_VALUE;
       PIECEVALUES[WHITE_KNIGHT] = KNIGHT_VALUE;
       PIECEVALUES[WHITE_BISHOP] = BISHOP_VALUE;
       PIECEVALUES[WHITE_ROOK] = ROOK_VALUE;
       PIECEVALUES[WHITE_QUEEN] = QUEEN_VALUE;
       PIECEVALUES[WHITE_KING] = KING_VALUE;  
       PIECEVALUES[BLACK_PAWN] = PAWN_VALUE;
       PIECEVALUES[BLACK_KNIGHT] = KNIGHT_VALUE;
       PIECEVALUES[BLACK_BISHOP] = BISHOP_VALUE;
       PIECEVALUES[BLACK_ROOK] = ROOK_VALUE;
       PIECEVALUES[BLACK_QUEEN] = QUEEN_VALUE;
       PIECEVALUES[BLACK_KING] = KING_VALUE;

//     ===========================================================================
//     Initialize MS1BTABLE, used in lastOne (see bitops.cpp)
//     ===========================================================================
    for (i = 0; i < 256; i++)
    {
        MS1BTABLE[i] = (
            (i > 127) ? 7 :
            (i >  63) ? 6 :
            (i >  31) ? 5 :
            (i >  15) ? 4 :
            (i >   7) ? 3 :
            (i >   3) ? 2 :
            (i >   1) ? 1 : 0 );
    }
 
//     ===========================================================================
//     Initialize rank, file and diagonal 6-bit masking bitmaps, to get the
//     occupancy state, used in the movegenerator (see movegen.ccp)
//     ===========================================================================
 
       for (square = 0; square < 64; square++)
       {
              RANKMASK[square] = 0x0;
              FILEMASK[square] = 0x0;
              DIAGA8H1MASK[square] = 0x0;
              DIAGA1H8MASK[square] = 0x0;
              FILEMAGIC[square] = 0x0;
              DIAGA8H1MAGIC[square] = 0x0;
              DIAGA1H8MAGIC[square] = 0x0;
       }
 
       for (file = 1; file < 9; file++)
       {
              for (rank = 1; rank < 9; rank++)
              {
//             ===========================================================================
//             initialize 6-bit rank mask, used in the movegenerator (see movegen.ccp)
//             ===========================================================================
 
                     RANKMASK[BOARDINDEX[file][rank]]  = BITSET[BOARDINDEX[2][rank]] | BITSET[BOARDINDEX[3][rank]] | BITSET[BOARDINDEX[4][rank]] ;
                     RANKMASK[BOARDINDEX[file][rank]] |= BITSET[BOARDINDEX[5][rank]] | BITSET[BOARDINDEX[6][rank]] | BITSET[BOARDINDEX[7][rank]] ;
 
//             ===========================================================================
//             initialize 6-bit file mask, used in the movegenerator (see movegen.ccp)
//             ===========================================================================
                     FILEMASK[BOARDINDEX[file][rank]]  = BITSET[BOARDINDEX[file][2]] | BITSET[BOARDINDEX[file][3]] | BITSET[BOARDINDEX[file][4]] ;
                     FILEMASK[BOARDINDEX[file][rank]] |= BITSET[BOARDINDEX[file][5]] | BITSET[BOARDINDEX[file][6]] | BITSET[BOARDINDEX[file][7]] ;
 
//             ===========================================================================
//             Initialize diagonal magic multiplication numbers, used in the movegenerator (see movegen.ccp)
//             ===========================================================================
                     diaga8h1 = file + rank; // from 2 to 16, longest diagonal = 9
                     DIAGA8H1MAGIC[BOARDINDEX[file][rank]] = _DIAGA8H1MAGICS[diaga8h1 - 2];
 
//             ===========================================================================
//             Initialize 6-bit diagonal mask, used in the movegenerator (see movegen.ccp)
//             ===========================================================================
                     DIAGA8H1MASK[BOARDINDEX[file][rank]] = 0x0;
                     if (diaga8h1 < 10)  // lower half, diagonals 2 to 9
                     {
                           for (square = 2 ; square < diaga8h1-1 ; square ++)
                           {
                                  DIAGA8H1MASK[BOARDINDEX[file][rank]] |= BITSET[BOARDINDEX[square][diaga8h1-square]];
                           }
                     }
                     else  // upper half, diagonals 10 to 16
                     {
                           for (square = 2 ; square < 17 - diaga8h1 ; square ++)
                           {
                                  DIAGA8H1MASK[BOARDINDEX[file][rank]] |= BITSET[BOARDINDEX[diaga8h1+square-9][9-square]];
                           }
                     }
      
//             ===========================================================================
//             Initialize diagonal magic multiplication numbers, used in the movegenerator (see movegen.ccp)
//             ===========================================================================
                     diaga1h8 = file - rank; // from -7 to +7, longest diagonal = 0
                     DIAGA1H8MAGIC[BOARDINDEX[file][rank]] = _DIAGA1H8MAGICS[diaga1h8+7];
 
//             ===========================================================================
//             Initialize 6-bit diagonal mask, used in the movegenerator (see movegen.ccp)
//             ===========================================================================
                     DIAGA1H8MASK[BOARDINDEX[file][rank]] = 0x0;
                     if (diaga1h8 > -1)  // lower half, diagonals 0 to 7
                     {
                           for (square = 2 ; square < 8 - diaga1h8 ; square ++)
                           {
                                  DIAGA1H8MASK[BOARDINDEX[file][rank]] |= BITSET[BOARDINDEX[diaga1h8 + square][square]];
                           }
                     }
                     else
                     {
                           for (square = 2 ; square < 8 + diaga1h8 ; square ++)
                           {
                                  DIAGA1H8MASK[BOARDINDEX[file][rank]] |= BITSET[BOARDINDEX[square][square - diaga1h8]];
                           }
                     }
 
//             ===========================================================================
//             Initialize file magic multiplication numbers, used in the movegenerator (see movegen.ccp)
//             ===========================================================================
                     FILEMAGIC[BOARDINDEX[file][rank]] = _FILEMAGICS[file-1];
 
              }
       }
 
//     ===========================================================================
//     Now initialize the GEN_SLIDING_ATTACKS array, used to generate the sliding
//     attack bitboards.
//     unsigned char GEN_SLIDING_ATTACKS[8 squares][64 states] holds the attacks
//     for any file, rank or diagonal - it is going to be usefull when generating the
//     RANK_ATTACKS[64][64], FILE_ATTACKS[64][64], DIAGA8H1_ATTACKS[64][64] and
//     DIAGA1H8_ATTACKS[64][64] arrays
//     ===========================================================================
 
       // initialize CHARBITSET, this array is equivalant to BITSET for bitboards:
       // 8 chars, each with only 1 bit set.
       CHARBITSET[0] = 1;
       for (square = 1; square <= 7; square++) 
       {
              CHARBITSET[square] = CHARBITSET[square-1] << 1;
       }
 
       // loop over rank, file or diagonal squares:
       for (square = 0; square <= 7; square++)
       {
              // loop of occupancy states
              // state6Bit represents the 64 possible occupancy states of a rank,
              // except the 2 end-bits, because they don't matter for calculating attacks
              for (state6Bit = 0; state6Bit < 64; state6Bit++)      
              {
                     state8Bit = state6Bit << 1; // create an 8-bit occupancy state
                     attack8Bit = 0;
                     if (square < 7)
                     {
                           attack8Bit |= CHARBITSET[square + 1];
                     }
                     slide = square + 2;
                     while (slide <= 7) // slide in '+' direction
                     {
                           if ((~state8Bit) & (CHARBITSET[slide - 1]))
                           {
                                  attack8Bit |= CHARBITSET[slide];
                           }
                           else break;
                           slide++;
                     }
                     if (square > 0)
                     {
                           attack8Bit |= CHARBITSET[square - 1];
                     }
                     slide = square - 2;
                     while (slide >= 0) // slide in '-' direction
                     {
                           if ((~state8Bit) & (CHARBITSET[slide + 1]))
                           {
                                  attack8Bit |= CHARBITSET[slide];
                           }
                           else break;
                           slide--;
                     }
                     GEN_SLIDING_ATTACKS[square][state6Bit] = attack8Bit;
              }
       }
 
//     ===========================================================================
//     Initialize all attack bitmaps, used in the movegenerator (see movegen.ccp)
//     ===========================================================================
 
       for (square = 0; square < 64; square++)
       {
              KNIGHT_ATTACKS[square] = 0x0;
              KING_ATTACKS[square] = 0x0;
              WHITE_PAWN_ATTACKS[square] = 0x0;
              WHITE_PAWN_MOVES[square] = 0x0;
              WHITE_PAWN_DOUBLE_MOVES[square] = 0x0;
              BLACK_PAWN_ATTACKS[square] = 0x0;
              BLACK_PAWN_MOVES[square] = 0x0;
              BLACK_PAWN_DOUBLE_MOVES[square] = 0x0;   
              for (state = 0; state < 64; state++)
              {
                     RANK_ATTACKS[square][state] = 0x0;
                     FILE_ATTACKS[square][state] = 0x0;
                     DIAGA8H1_ATTACKS[square][state] = 0x0;
                     DIAGA1H8_ATTACKS[square][state] = 0x0;
              }
       }
 
       // WHITE_PAWN_ATTACKS
       for (square = 0; square < 64; square++)
       {
              file = FILES[square]; rank = RANKS[square];
              afile = file - 1; arank = rank + 1;
              if ((afile >= 1) & (afile <= 8) & (arank >= 1) & (arank <= 8))
                     WHITE_PAWN_ATTACKS[square] |= BITSET[BOARDINDEX[afile][arank]];
              afile = file + 1; arank = rank + 1;
              if ((afile >= 1) & (afile <= 8) & (arank >= 1) & (arank <= 8))
                     WHITE_PAWN_ATTACKS[square] |= BITSET[BOARDINDEX[afile][arank]];
       }
 
       // WHITE_PAWN_MOVES
       for (square = 0; square <64; square++)
       {
              file = FILES[square]; rank = RANKS[square];
              afile = file; arank = rank + 1;
              if ((afile >= 1) & (afile <= 8) & (arank >= 1) & (arank <= 8))
                     WHITE_PAWN_MOVES[square] |= BITSET[BOARDINDEX[afile][arank]];       
              if (rank == 2)
              {
                     afile = file; arank = rank + 2;
                     if ((afile >= 1) & (afile <= 8) & (arank >= 1) & (arank <= 8))
                           WHITE_PAWN_DOUBLE_MOVES[square] |= BITSET[BOARDINDEX[afile][arank]];
              }
       }
 
       // BLACK_PAWN_ATTACKS
       for (square = 0; square < 64; square++)
       {
              file = FILES[square]; rank = RANKS[square];
              afile = file - 1; arank = rank - 1;
              if ((afile >= 1) & (afile <= 8) & (arank >= 1) & (arank <= 8))
                     BLACK_PAWN_ATTACKS[square] |= BITSET[BOARDINDEX[afile][arank]];
              afile = file + 1; arank = rank - 1;
              if ((afile >= 1) & (afile <= 8) & (arank >= 1) & (arank <= 8))
                     BLACK_PAWN_ATTACKS[square] |= BITSET[BOARDINDEX[afile][arank]];
       }
 
       // BLACK_PAWN_MOVES
       for (square = 0; square < 64; square++)
       {
              file = FILES[square]; rank = RANKS[square];
              afile = file; arank = rank - 1;
              if ((afile >= 1) & (afile <= 8) & (arank >= 1) & (arank <= 8))
                     BLACK_PAWN_MOVES[square] |= BITSET[BOARDINDEX[afile][arank]];
              if (rank == 7)
              {
                     afile = file; arank = rank - 2;
                     if ((afile >= 1) & (afile <= 8) & (arank >= 1) & (arank <= 8))
                           BLACK_PAWN_DOUBLE_MOVES[square] |= BITSET[BOARDINDEX[afile][arank]];
              }
       }
 
       // KNIGHT attacks;
       for (square = 0; square < 64; square++)
       {
              file = FILES[square];
              rank = RANKS[square];
              afile = file - 2; arank = rank + 1;
              if ((afile >= 1) & (afile <= 8) & (arank >= 1) & (arank <= 8))
                     KNIGHT_ATTACKS[square] |= BITSET[BOARDINDEX[afile][arank]];
              afile = file - 1; arank = rank + 2;
              if ((afile >= 1) & (afile <= 8) & (arank >= 1) & (arank <= 8))
                     KNIGHT_ATTACKS[square] |= BITSET[BOARDINDEX[afile][arank]];
              afile = file + 1; arank = rank + 2;
              if ((afile >= 1) & (afile <= 8) & (arank >= 1) & (arank <= 8))
                     KNIGHT_ATTACKS[square] |= BITSET[BOARDINDEX[afile][arank]];
              afile = file + 2; arank = rank + 1;
              if ((afile >= 1) & (afile <= 8) & (arank >= 1) & (arank <= 8))
                     KNIGHT_ATTACKS[square] |= BITSET[BOARDINDEX[afile][arank]];
              afile = file + 2; arank = rank - 1;
              if ((afile >= 1) & (afile <= 8) & (arank >= 1) & (arank <= 8))
                     KNIGHT_ATTACKS[square] |= BITSET[BOARDINDEX[afile][arank]];
              afile = file + 1; arank = rank - 2;
              if ((afile >= 1) & (afile <= 8) & (arank >= 1) & (arank <= 8))
                     KNIGHT_ATTACKS[square] |= BITSET[BOARDINDEX[afile][arank]];
              afile = file - 1; arank = rank - 2;
              if ((afile >= 1) & (afile <= 8) & (arank >= 1) & (arank <= 8))
                     KNIGHT_ATTACKS[square] |= BITSET[BOARDINDEX[afile][arank]];
              afile = file - 2; arank = rank - 1;
              if ((afile >= 1) & (afile <= 8) & (arank >= 1) & (arank <= 8))
                     KNIGHT_ATTACKS[square] |= BITSET[BOARDINDEX[afile][arank]];
       }
 
       // KING attacks;
       for (square = 0; square < 64; square++)
       {
              file = FILES[square]; rank = RANKS[square];
              afile = file - 1; arank = rank;
              if ((afile >= 1) & (afile <= 8) & (arank >= 1) & (arank <= 8))
                     KING_ATTACKS[square] |= BITSET[BOARDINDEX[afile][arank]];
              afile = file - 1; arank = rank + 1;
              if ((afile >= 1) & (afile <= 8) & (arank >= 1) & (arank <= 8))
                     KING_ATTACKS[square] |= BITSET[BOARDINDEX[afile][arank]];
              afile = file; arank = rank + 1;
              if ((afile >= 1) & (afile <= 8) & (arank >= 1) & (arank <= 8))
                     KING_ATTACKS[square] |= BITSET[BOARDINDEX[afile][arank]];
              afile = file + 1; arank = rank + 1;
              if ((afile >= 1) & (afile <= 8) & (arank >= 1) & (arank <= 8))
                     KING_ATTACKS[square] |= BITSET[BOARDINDEX[afile][arank]];
              afile = file + 1; arank = rank;
              if ((afile >= 1) & (afile <= 8) & (arank >= 1) & (arank <= 8))
                     KING_ATTACKS[square] |= BITSET[BOARDINDEX[afile][arank]];
              afile = file + 1; arank = rank - 1;
              if ((afile >= 1) & (afile <= 8) & (arank >= 1) & (arank <= 8))
                     KING_ATTACKS[square] |= BITSET[BOARDINDEX[afile][arank]];
              afile = file; arank = rank - 1;
              if ((afile >= 1) & (afile <= 8) & (arank >= 1) & (arank <= 8))
                     KING_ATTACKS[square] |= BITSET[BOARDINDEX[afile][arank]];
              afile = file - 1; arank = rank - 1;
              if ((afile >= 1) & (afile <= 8) & (arank >= 1) & (arank <= 8))
                     KING_ATTACKS[square] |= BITSET[BOARDINDEX[afile][arank]];
       }
 
       //  RANK attacks (ROOKS and QUEENS):
       //  use           unsigned char GEN_SLIDING_ATTACKS[8 squares] [64 states]
       //  to initialize BitMap        RANK_ATTACKS       [64 squares][64 states]
       //
       for (square = 0; square < 64; square++)
       {
              for (state6Bit = 0; state6Bit < 64; state6Bit++)
              {
                     RANK_ATTACKS[square][state6Bit] = 0;
                     RANK_ATTACKS[square][state6Bit] |=
                           BitMap(GEN_SLIDING_ATTACKS[FILES[square]-1][state6Bit]) << (RANKSHIFT[square] - 1);
              }
       }
 
       //  FILE attacks (ROOKS and QUEENS):
       //  use           unsigned char GEN_SLIDING_ATTACKS[8 squares] [64 states]
       //  to initialize BitMap        FILE_ATTACKS       [64 squares][64 states]
       //
       //  Occupancy transformation is as follows:
       //
       //   occupancy state bits of the file:               occupancy state bits in GEN_SLIDING_ATTACKS:
       //
       //        . . . . . . . . MSB                           LSB         MSB
       //        . . . . . A . .                    =>         A B C D E F . .                            
       //        . . . . . B . .
       //        . . . . . C . .
       //        . . . . . D . .
       //        . . . . . E . .
       //        . . . . . F . .
       //    LSB . . . . . . . .
       //
       //  The reverse transformation is as follows:
       //
       //   attack bits in GEN_SLIDING_ATTACKS:             attack bits in the file:
       //
       //        LSB         MSB                               . . . . . m . . MSB
       //        m n o p q r s t                    =>         . . . . . n . .
       //                                                      . . . . . o . .
       //                                                      . . . . . p . .
       //                                                      . . . . . q . .
       //                                                      . . . . . r . .
       //                                                      . . . . . s . .
       //                                                 LSB  . . . . . t . .
       //
       for (square = 0; square < 64; square++)
       {
              for (state6Bit = 0; state6Bit < 64; state6Bit++)
              {
                     FILE_ATTACKS[square][state6Bit] = 0x0;
 
                     // check to see if attackbit'-th  bit is set in GEN_SLIDING_ATTACKS, for this combination of square/occupancy state
                     for (attackbit = 0; attackbit < 8; attackbit++) // from LSB to MSB
                     {
                           //  conversion from 64 board squares to the 8 corresponding positions in the GEN_SLIDING_ATTACKS array: "8-RANKS[square]"
                           if (GEN_SLIDING_ATTACKS[8-RANKS[square]][state6Bit] & CHARBITSET[attackbit])
                           {
                                  // the bit is set, so we need to update FILE_ATTACKS accordingly:
                                  // conversion of square/attackbit to the corresponding 64 board FILE: FILES[square]
                                  // conversion of square/attackbit to the corresponding 64 board RANK: 8-attackbit
                                  file = FILES[square];
                                  rank = 8 - attackbit;
                                  FILE_ATTACKS[square][state6Bit] |=  BITSET[BOARDINDEX[file][rank]];
                           }
                     }
              }
       }
 
       //  DIAGA8H1_ATTACKS attacks (BISHOPS and QUEENS):
       for (square = 0; square < 64; square++)
       {
              for (state6Bit = 0; state6Bit < 64; state6Bit++)
              {
                     DIAGA8H1_ATTACKS[square][state6Bit] = 0x0;
                     for (attackbit = 0; attackbit < 8; attackbit++) // from LSB to MSB
                     {
                           //  conversion from 64 board squares to the 8 corresponding positions in the GEN_SLIDING_ATTACKS array: MIN((8-RANKS[square]),(FILES[square]-1))
                           if (GEN_SLIDING_ATTACKS[(8-RANKS[square]) < (FILES[square]-1) ? (8-RANKS[square]) : (FILES[square]-1)][state6Bit] & CHARBITSET[attackbit])
                           {
                                  // the bit is set, so we need to update FILE_ATTACKS accordingly:
                                  // conversion of square/attackbit to the corresponding 64 board file and rank:
                                  diaga8h1 = FILES[square] + RANKS[square]; // from 2 to 16, longest diagonal = 9
                                  if (diaga8h1 < 10)
                                  {
                                    file = attackbit + 1;
                                    rank = diaga8h1 - file;
                                  }
                                  else
                                  {
                                    rank = 8 - attackbit;
                                    file = diaga8h1 - rank;
                                  }
                                  if ((file > 0) && (file < 9) && (rank > 0) && (rank < 9))
                                  {
                                         DIAGA8H1_ATTACKS[square][state6Bit] |=  BITSET[BOARDINDEX[file][rank]];
                                  }
                           }
                     }
              }
       }
 
       //  DIAGA1H8_ATTACKS attacks (BISHOPS and QUEENS):
       for (square = 0; square < 64; square++)
       {
              for (state6Bit = 0; state6Bit < 64; state6Bit++)
              {
                     DIAGA1H8_ATTACKS[square][state6Bit] = 0x0;
                     for (attackbit = 0; attackbit < 8; attackbit++) // from LSB to MSB
                     {
                           //  conversion from 64 board squares to the 8 corresponding positions in the GEN_SLIDING_ATTACKS array: MIN((8-RANKS[square]),(FILES[square]-1))
                           if (GEN_SLIDING_ATTACKS[(RANKS[square]-1) < (FILES[square]-1) ? (RANKS[square]-1) : (FILES[square]-1)][state6Bit] & CHARBITSET[attackbit])
                           {
                                  // the bit is set, so we need to update FILE_ATTACKS accordingly:
                                  // conversion of square/attackbit to the corresponding 64 board file and rank:
                                  diaga1h8 = FILES[square] - RANKS[square]; // from -7 to 7, longest diagonal = 0
                                  if (diaga1h8 < 0)
                                  {
                                    file = attackbit + 1;
                                    rank = file - diaga1h8;
                                  }
                                  else
                                  {
                                    rank = attackbit + 1;
                                    file = diaga1h8 + rank;
                                  }
                                  if ((file > 0) && (file < 9) && (rank > 0) && (rank < 9))
                                  {
                                         DIAGA1H8_ATTACKS[square][state6Bit] |=  BITSET[BOARDINDEX[file][rank]];
                                  }
                           }
                     }
              }
       }
 
 
//     ===========================================================================
//     Masks for castling, index 0 is for white, 1 is for black
//     ===========================================================================
 
       maskEG[0] = BITSET[E1] | BITSET[F1] | BITSET[G1];
       maskEG[1] = BITSET[E8] | BITSET[F8] | BITSET[G8];
 
       maskFG[0] = BITSET[F1] | BITSET[G1];
       maskFG[1] = BITSET[F8] | BITSET[G8];
 
       maskBD[0] = BITSET[B1] | BITSET[C1] | BITSET[D1];
       maskBD[1] = BITSET[B8] | BITSET[C8] | BITSET[D8];
 
       maskCE[0] = BITSET[C1] | BITSET[D1] | BITSET[E1];
       maskCE[1] = BITSET[C8] | BITSET[D8] | BITSET[E8];
 
//     ===========================================================================
//     The 4 castling moves can be predefined:
//     ===========================================================================
 
       move.clear();
       move.setCapt(EMPTY);
       move.setPiec(WHITE_KING);
       move.setProm(WHITE_KING);
       move.setFrom(E1);
       move.setTosq(G1);
       WHITE_OO_CASTL = move.moveInt;
       move.setTosq(C1);
       WHITE_OOO_CASTL = move.moveInt;
 
       move.setPiec(BLACK_KING);
       move.setProm(BLACK_KING);
       move.setFrom(E8);
       move.setTosq(G8);
       BLACK_OO_CASTL = move.moveInt;
       move.setTosq(C8);
       BLACK_OOO_CASTL = move.moveInt;
 
//     ===========================================================================
//     Initialize evaluation data & bitmaps
//     ===========================================================================
 
       BLACK_SQUARES = 0;
       for (i = 0; i < 64; i++)
       {
              if ((i + RANKS[i]) % 2) BLACK_SQUARES ^= BITSET[i];
       }
       WHITE_SQUARES = ~BLACK_SQUARES;
 
       // Clear bitmaps:
       for (i = 0; i < 64; i++)
       {
              PASSED_WHITE[i] = 0;
              PASSED_BLACK[i] = 0;
              ISOLATED_WHITE[i] = 0;
              ISOLATED_BLACK[i] = 0;
              BACKWARD_WHITE[i] = 0;
              BACKWARD_BLACK[i] = 0;
              KINGSHIELD_STRONG_W[i] = 0;
              KINGSHIELD_STRONG_B[i] = 0;
              KINGSHIELD_WEAK_W[i] = 0;
              KINGSHIELD_WEAK_B[i] = 0;
       }
 
       for (i = 0; i < 64; i++)
       {
        //  PASSED_WHITE:
              for (rank = RANKS[i] + 1; rank < 8; rank++)
              {
                     // 3 files:
                     if (FILES[i] - 1 > 0) PASSED_WHITE[i] ^= BITSET[BOARDINDEX[FILES[i] - 1][rank]];
                     PASSED_WHITE[i] ^= BITSET[BOARDINDEX[FILES[i]][rank]];
                     if (FILES[i] + 1 < 9 ) PASSED_WHITE[i] ^= BITSET[BOARDINDEX[FILES[i] + 1][rank]];
              }
 
              // ISOLATED_WHITE:
              for (rank = 2; rank < 8; rank++)
              {
                     // 2 files:
                     if (FILES[i] - 1 > 0) ISOLATED_WHITE[i] ^= BITSET[BOARDINDEX[FILES[i] - 1][rank]];
                     if (FILES[i] + 1 < 9 ) ISOLATED_WHITE[i] ^= BITSET[BOARDINDEX[FILES[i] + 1][rank]];
              }
 
              //  BACKWARD_WHITE:
              for (rank = 2; rank <= RANKS[i]; rank++)
              {
                     // 2 files:
                     if (FILES[i] - 1 > 0) BACKWARD_WHITE[i] ^= BITSET[BOARDINDEX[FILES[i] - 1][rank]];
                     if (FILES[i] + 1 < 9 ) BACKWARD_WHITE[i] ^= BITSET[BOARDINDEX[FILES[i] + 1][rank]];
              }
       }
 
       // Pawn shield bitmaps for king safety, only if the king is on the first 3 ranks:
       for (i = 0; i < 24; i++)
       {
        //  KINGSHIELD_STRONG_W & KINGSHIELD_WEAK_W:
              KINGSHIELD_STRONG_W[i] ^= BITSET[i + 8];
              KINGSHIELD_WEAK_W[i] ^= BITSET[i + 16];
              if (FILES[i] > 1)
              {
                     KINGSHIELD_STRONG_W[i] ^= BITSET[i + 7];
                     KINGSHIELD_WEAK_W[i] ^= BITSET[i + 15];
              }
              if (FILES[i] < 8)
              {
                     KINGSHIELD_STRONG_W[i] ^= BITSET[i + 9];
                     KINGSHIELD_WEAK_W[i] ^= BITSET[i + 17];
              }
              if (FILES[i]== 1)
              {
                     KINGSHIELD_STRONG_W[i] ^= BITSET[i + 10];
                     KINGSHIELD_WEAK_W[i] ^= BITSET[i + 18];
              }
              if (FILES[i]== 8)
              {
                     KINGSHIELD_STRONG_W[i] ^= BITSET[i + 6];
                     KINGSHIELD_WEAK_W[i] ^= BITSET[i + 14];
              }
       }
 
//     ===========================================================================
//     DISTANCE array, distance is measured as max of (rank,file)-difference
//     ===========================================================================
       for (i = 0 ; i < 64; i++)
       {
              for (square = 0 ; square < 64; square++)
              {
                     if (abs(RANKS[i] - RANKS[square]) > abs(FILES[i] - FILES[square]))
                           DISTANCE[i][square] = abs(RANKS[i] - RANKS[square]);
                     else
                           DISTANCE[i][square] = abs(FILES[i] - FILES[square]);
              }
       }
 
//     ===========================================================================
//     Initialize MIRRORed data:
//     ===========================================================================
       // Data is supplied as mirrored for WHITE, so it's ready for BLACK to use:
       for (square = 0; square < 64; square++)
       {
              PAWNPOS_B[square] = PAWNPOS_W[square];
              KNIGHTPOS_B[square] = KNIGHTPOS_W[square];
              BISHOPPOS_B[square] = BISHOPPOS_W[square];
              ROOKPOS_B[square] = ROOKPOS_W[square];
              QUEENPOS_B[square] = QUEENPOS_W[square];
              KINGPOS_B[square] = KINGPOS_W[square];
              KINGPOS_ENDGAME_B[square] = KINGPOS_ENDGAME_W[square];
       }
 
       // Complete missing mirrored data:
       for (i = 0; i < 64; i++)
       {
              PAWNPOS_W[i] = PAWNPOS_B[MIRROR[i]];
              KNIGHTPOS_W[i] = KNIGHTPOS_B[MIRROR[i]];
              BISHOPPOS_W[i] = BISHOPPOS_B[MIRROR[i]];
              ROOKPOS_W[i] = ROOKPOS_B[MIRROR[i]];
              QUEENPOS_W[i] = QUEENPOS_B[MIRROR[i]];
              KINGPOS_W[i] = KINGPOS_B[MIRROR[i]];
              KINGPOS_ENDGAME_W[i] = KINGPOS_ENDGAME_B[MIRROR[i]];
 
              for (square = 0; square < 64; square ++)
              {
                     //  PASSED_BLACK bitmaps (mirror of PASSED_WHITE bitmaps):
                     if (PASSED_WHITE[i] & BITSET[square]) PASSED_BLACK[MIRROR[i]] |= BITSET[MIRROR[square]];
 
                     //  ISOLATED_BLACK bitmaps (mirror of ISOLATED_WHITE bitmaps):
                     if (ISOLATED_WHITE[i] & BITSET[square]) ISOLATED_BLACK[MIRROR[i]] |= BITSET[MIRROR[square]];
 
                     //  BACKWARD_BLACK bitmaps (mirror of BACKWARD_WHITE bitmaps):
                     if (BACKWARD_WHITE[i] & BITSET[square]) BACKWARD_BLACK[MIRROR[i]] |= BITSET[MIRROR[square]];
 
                     //  KINGSHIELD_STRONG_B bitmaps (mirror of KINGSHIELD_STRONG_W bitmaps):
                     if (KINGSHIELD_STRONG_W[i] & BITSET[square]) KINGSHIELD_STRONG_B[MIRROR[i]] |= BITSET[MIRROR[square]];
 
                     //  KINGSHIELD_WEAK_B bitmaps (mirror of KINGSHIELD_WEAK_W bitmaps):
                     if (KINGSHIELD_WEAK_W[i] & BITSET[square]) KINGSHIELD_WEAK_B[MIRROR[i]] |= BITSET[MIRROR[square]];
              }
       }

	NOMOVE.moveInt = 0;
	KEY.init();

//	===========================================================================
//	HEADINGS and RAYS, used in SEE:
//	===========================================================================
	for (i = 0 ; i < 64; i++)
	{
		RAY_W[i]  = 0;
		RAY_NW[i] = 0;
		RAY_N[i]  = 0;
		RAY_NE[i] = 0;
		RAY_E[i]  = 0;
		RAY_SE[i] = 0;
		RAY_S[i]  = 0;
		RAY_SW[i] = 0;
		for (square = 0 ; square < 64; square ++)
		{
			HEADINGS[i][square] = 0;
		}
	}
	
	for (rank = 1 ; rank < 9; rank++)
		for (file = 1 ; file < 9; file++)
		{
			i = BOARDINDEX[file][rank];			
			// WEST:
			for (afile = file - 1 ; afile > 0; afile--)
			{
				HEADINGS[i][BOARDINDEX[afile][rank]] = WEST;
				RAY_W[BOARDINDEX[file][rank]] |= BITSET[BOARDINDEX[afile][rank]];
			}
			// NORTHWEST:
			for (afile = file - 1, arank = rank + 1 ; (afile > 0) && (arank < 9); afile--,  arank++) 
			{
				HEADINGS[i][BOARDINDEX[afile][arank]] = NORTHWEST;
				RAY_NW[BOARDINDEX[file][rank]] |= BITSET[BOARDINDEX[afile][arank]];
			}
			// NORTH:
			for (arank = rank + 1 ; arank < 9; arank++) 
			{
				HEADINGS[i][BOARDINDEX[file][arank]] = NORTH;
				RAY_N[BOARDINDEX[file][rank]] |= BITSET[BOARDINDEX[file][arank]];
			}
			// NORTHEAST:
			for (afile = file + 1, arank = rank + 1 ; (afile < 9) && (arank < 9); afile++,  arank++) 
			{
				HEADINGS[i][BOARDINDEX[afile][arank]] = NORTHEAST;
				RAY_NE[BOARDINDEX[file][rank]] |= BITSET[BOARDINDEX[afile][arank]];
			}
			// EAST:
			for (afile = file + 1 ; afile < 9; afile++) 
			{
				HEADINGS[i][BOARDINDEX[afile][rank]] = EAST;
				RAY_E[BOARDINDEX[file][rank]] |= BITSET[BOARDINDEX[afile][rank]];
			}
			// SOUTHEAST:
			for (afile = file + 1, arank = rank - 1 ; (afile < 9) && (arank > 0); afile++,  arank--) 
			{
				HEADINGS[i][BOARDINDEX[afile][arank]] = SOUTHEAST;
				RAY_SE[BOARDINDEX[file][rank]] |= BITSET[BOARDINDEX[afile][arank]];
			}
			// SOUTH:
			for (arank = rank - 1 ; arank > 0; arank--) 
			{
				HEADINGS[i][BOARDINDEX[file][arank]] = SOUTH;
				RAY_S[BOARDINDEX[file][rank]] |= BITSET[BOARDINDEX[file][arank]];
			}
			// SOUTHWEST:
			for (afile = file - 1, arank = rank - 1 ; (afile > 0) && (arank > 0); afile--,  arank--) 
			{
				HEADINGS[i][BOARDINDEX[afile][arank]] = SOUTHWEST;
				RAY_SW[BOARDINDEX[file][rank]] |= BITSET[BOARDINDEX[afile][arank]];
			}
		}

	// required for running test suites, to prevent 
	// writing escape characters to a file
	TO_CONSOLE = 1;

	// Winboard parameters:
	XB_NONE = 2;  // not 0 or 1
	XB_ANALYZE = 3;   // not 0, 1, or 2
	XB_MODE = false;
	XB_POST = true;
	XB_PONDER = false;
	XB_DO_PENDING = false;
	XB_NO_TIME_LIMIT = false;
    return;
}
 
void info()
{
 
       //  your playground... display variables - meant for testing/verification purposes only
       std::cout << std::endl << "============ info start ==============" << std::endl;
       std::cout << "size of board, in bytes   = " << sizeof(board) << std::endl;
       std::cout << "Material value            = " << board.Material << std::endl;
       std::cout << "White castling rights     = " << int(board.castleWhite) << std::endl;
       std::cout << "Black castling rights     = " << int(board.castleBlack) << std::endl;
       std::cout << "En-passant square         = " << board.epSquare << std::endl;
       std::cout << "Fifty move count          = " << board.fiftyMove << std::endl;
 
       std::cout << "bitCnt of white pawns     = " << bitCnt(board.whitePawns) << std::endl;
       std::cout << std::endl << "bitmap of blackKnights | board.whitePawns:" << std::endl;
       displayBitmap(board.blackKnights | board.whitePawns);
       std::cout << "============ info end ================" << std::endl << std::endl;
 
       return;
}

void readIniFile()
{
	int nextc;
	FILE* inifile;

//	===========================================================================
//	Open the ini file:
//	===========================================================================
	inifile = fopen(INIFILE, "r");
	if (!inifile) 
	{
		std::cout << "warning: no ini file " << INIFILE << std::endl;
		std::cout << "path: " << PATHNAME << std::endl;
		return;
	}
	else
	{
		std::cout << "Initialization file: " << INIFILE << std::endl;
	}

	while ((nextc = getc(inifile)) != EOF)
	{
		if (nextc == '\n')
		{
			CMD_BUFF[CMD_BUFF_COUNT] = '\0';
			CMD_BUFF_COUNT = '\0';
			if (!doIniCommand(CMD_BUFF)) break;
		}
		else
		{
			if (CMD_BUFF_COUNT >= MAX_CMD_BUFF-1)
			{
				std::cout << "Warning: command buffer full !! " << std::endl;
				CMD_BUFF_COUNT = 0;
			}
			CMD_BUFF[CMD_BUFF_COUNT++] = nextc;
		}
	}
	fclose(inifile);
	return;
}
 
BOOLTYPE doIniCommand(const char *buf)
{
	int number;

	if (!strncmp(buf, "depth", 5))
	{
		sscanf(buf+5, "%d", &board.searchDepth);
		if (board.searchDepth < 1) board.searchDepth = 1;
		if (board.searchDepth > MAX_PLY) board.searchDepth = MAX_PLY;
		CMD_BUFF_COUNT = '\0';
		return true;
	}

	if (!strncmp(buf, "time", 4))
	{
		sscanf(buf+4,"%d",&board.maxTime);
		board.maxTime *= 1000;  // convert to milliseconds
		if (board.maxTime < 0) board.maxTime = 1;
		CMD_BUFF_COUNT = '\0';
		return true;
	}

	if (!strncmp(buf, "NULLMOVE_REDUCTION", 18))
	{
		sscanf(buf+18, "%d", &NULLMOVE_REDUCTION);
		CMD_BUFF_COUNT = '\0';
		return true;
	}

	if (!strncmp(buf, "NULLMOVE_LIMIT", 14))
	{
		sscanf(buf+14, "%d", &NULLMOVE_LIMIT);
		CMD_BUFF_COUNT = '\0';
		return true;
	}

	if (!strncmp(buf, "STOP_FRAC", 9))
	{
		sscanf(buf+9, "%d", &number);
		STOPFRAC = (float)(number/100.0);
		CMD_BUFF_COUNT = '\0';
		return true;
	}

	return true;
}