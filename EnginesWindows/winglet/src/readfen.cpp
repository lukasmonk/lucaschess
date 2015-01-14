#ifndef _CRT_SECURE_NO_DEPRECATE  // suppress MS security warnings
#define _CRT_SECURE_NO_DEPRECATE
#endif
#include <iostream>
#include "defines.h"
#include "protos.h"
#include "extglobals.h"
 
BOOLTYPE readFen(char *filename, int number)
{
       int numberf;
       char s[180];
       char fenwhite[80];
       char fenblack[80];
       char fen[100];
       char fencolor[2];     
       char fencastling[5];
       char fenenpassant[3];
       char temp[80];
       int fenhalfmoveclock;
       int fenfullmovenumber;
       BOOLTYPE returnValue;
       FILE * fp;
 
       returnValue = false;
       if (number <= 0) return returnValue;
 
    // open the file for read and scan through until we find the number-th position:
       fp=fopen(filename, "rt");
       if (fp != NULL)
       {
              numberf = 0;
              while (fscanf(fp, "%s", s) != EOF) 
              {
                     if (!strcmp(s, "[White"))
                     {
                           fscanf(fp, "%s", fenwhite);
                           // remove first (") and last two characters ("]) from fenwhite:
                           strcpy(temp, "");
                           strncat(temp, fenwhite, strlen(fenwhite)-2);
                           strcpy(temp, temp+1);
                           strcpy(fenwhite, temp);
                     }
                     if (!strcmp(s, "[Black"))
                     {
                           fscanf(fp, "%s", fenblack);
                           // remove first (") and last two characters ("]) from fenblack:
                           strcpy(temp, "");
                           strncat(temp, fenblack, strlen(fenblack)-2);
                           strcpy(temp, temp+1);
                           strcpy(fenblack, temp);
                     }
                     if (!strcmp(s, "[FEN"))
                     {
                           // position found, so increment numberf.
                           // we already have fenwhite and fenblack.
                           numberf++;
                           if (numberf == number)
                           {
                                  fscanf(fp, "%s", fen);
                                  fscanf(fp, "%s", fencolor);           // b or w
                                  fscanf(fp, "%s", fencastling);        // -, or KQkq
                                  fscanf(fp, "%s", fenenpassant);       // -, or e3, or b6, etc
                                  fscanf(fp, "%d", &fenhalfmoveclock);  // int, used for the fifty move draw rule
                                  fscanf(fp, "%d", &fenfullmovenumber); // int. start with 1, It is incremented after move by Black
 
                                  std::cout << std::endl << "winglet> fen #" << numberf << " in " << filename << ":" << std::endl << std::endl;
                                  std::cout << " White: " << fenwhite << std::endl;
                                  std::cout << " Black: " << fenblack << std::endl;
                                  std::cout << " " << &fen[1] << std::endl;
                                  if (fencolor[0] == 'w')
                                  {
                                         std::cout << " wt to move next" << std::endl;
                                  }
                                  else
                                  {
                                         std::cout << " bl to move next" << std::endl;
                                  }
                                  std::cout << " Castling: " << fencastling << std::endl;
                                  std::cout << " EP square: " << fenenpassant << std::endl;
                                  std::cout << " Fifty move count: " << fenhalfmoveclock << std::endl;
                                  std::cout << " Move number: " << fenfullmovenumber << std::endl << std::endl;
                           }
                     }
              }
 
              if (numberf < number)
              {
                     printf("winglet> only %d fens present in %s, fen #%d not found\n",
                     numberf, filename, number);
                     returnValue = false;
              }
              else
              {
                     setupFen(fen, fencolor, fencastling, fenenpassant, fenhalfmoveclock, fenfullmovenumber);
                     returnValue = true;
              }
              fclose(fp);
       }
       else
       {
              printf("winglet> error opening file: %s\n", filename);
              returnValue = false;
       }
       return returnValue;
}
 
void setupFen(char *fen, char *fencolor, char *fencastling, char *fenenpassant, int fenhalfmoveclock, int fenfullmovenumber)
{
       int i, file, rank, counter, piece;
       int whiteCastle, blackCastle, next, epsq;

	   board.fullmovenumber = fenfullmovenumber; // LC change
 
       piece = 0;
       for (i = 0; i < 64; i++)
       {
              board.square[i] = EMPTY;
       }
 
       // loop over the FEN string characters, and populate board.square[]
       // i is used as index for the FEN string
       // counter is the index for board.square[], 0..63
       // file and rank relate to the position on the chess board, 1..8
       // There is no error/legality checking on the FEN string!!
       file = 1;
       rank = 8;
       i = 0;
       counter = 0;
       while ((counter < 64) && (fen[i] != '\0'))
       {
              // '1'  through '8':
              if (((int) fen[i] > 48) && ((int) fen[i] < 57))
              {
                     file+= (int) fen[i] - 48;
                     counter+= (int) fen[i] - 48;
              }
              else
              //  other characters:
              {
                     switch (fen[i])
                     {
                           case '/':
                                  rank--;
                                  file = 1;
                                  break;
 
                           case 'P':
                                  board.square[BOARDINDEX[file][rank]] = WHITE_PAWN;
                                  file += 1;
                                  counter += 1;
                                  break;
 
                           case 'N':
                                  board.square[BOARDINDEX[file][rank]] = WHITE_KNIGHT;
                                  file += 1;
                                  counter += 1;
                                  break;
 
                           case 'B':
                                  board.square[BOARDINDEX[file][rank]] = WHITE_BISHOP;
                                  file += 1;
                                  counter += 1;
                                  break;
 
                           case 'R':
                                  board.square[BOARDINDEX[file][rank]] = WHITE_ROOK;
                                  file += 1;
                                  counter += 1;
                                  break;
 
                           case 'Q':
                                  board.square[BOARDINDEX[file][rank]] = WHITE_QUEEN;
                                  file += 1;
                                  counter += 1;
                                  break;
 
                           case 'K':
                                  board.square[BOARDINDEX[file][rank]] = WHITE_KING;
                                  file += 1;
                                  counter += 1;
                                  break;
 
                           case 'p':
                                  board.square[BOARDINDEX[file][rank]] = BLACK_PAWN;
                                  file += 1;
                                  counter += 1;
                                  break;
 
                           case 'n':
                                  board.square[BOARDINDEX[file][rank]] = BLACK_KNIGHT;
                                  file += 1;
                                  counter += 1;
                                  break;
 
                           case 'b':
                                  board.square[BOARDINDEX[file][rank]] = BLACK_BISHOP;
                                  file += 1;
                                  counter += 1;
                                  break;
 
                           case 'r':
                                  board.square[BOARDINDEX[file][rank]] = BLACK_ROOK;
                                  file += 1;
                                  counter += 1;
                                  break;
 
                           case 'q':
                                  board.square[BOARDINDEX[file][rank]] = BLACK_QUEEN;
                                  file += 1;
                                  counter += 1;
                                  break;
 
                           case 'k':
                                  board.square[BOARDINDEX[file][rank]] = BLACK_KING;
                                  file += 1;
                                  counter += 1;
                                  break;
 
                           default:
                                  break;
                     }
              }
              i++;
       }
       next = WHITE_MOVE;
       if (fencolor[0] == 'b') next = BLACK_MOVE;
 
       whiteCastle = 0;
       blackCastle = 0;
       if (strstr(fencastling, "K")) whiteCastle += CANCASTLEOO;
       if (strstr(fencastling, "Q")) whiteCastle += CANCASTLEOOO;
       if (strstr(fencastling, "k")) blackCastle += CANCASTLEOO;
       if (strstr(fencastling, "q")) blackCastle += CANCASTLEOOO;
       if (strstr(fenenpassant, "-"))
       {
              epsq = 0;
       }
       else
       {
              // translate a square coordinate (as string) to int (eg 'e3' to 20):
              epsq = ((int) fenenpassant[0] - 96) + 8 * ((int) fenenpassant[1] - 48) - 9;
       }
       board.initFromSquares(board.square, next, fenhalfmoveclock, whiteCastle , blackCastle , epsq);
 
}