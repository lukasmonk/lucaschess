#ifndef _CRT_SECURE_NO_DEPRECATE  // suppress MS security warnings
#define _CRT_SECURE_NO_DEPRECATE
#endif
#include <iostream>
#include "defines.h"
#include "protos.h"
#include "extglobals.h"
 
void setup()
{
	// interactively setup the chess board
	int file, rank;
	int whiteCastle; 
	int blackCastle;
	unsigned char next;
	int halfmoves;
	int epsq, i;
	char s[80], epsqc[80], castle[80], fenrank[80];
	char fen[100];
	char fencolor[2];     
	char fencastling[5];
	char fenenpassant[3];
	int fenhalfmoveclock;
	int fenfullmovenumber;
 
	whiteCastle = board.castleWhite;
	blackCastle = board.castleBlack;
	next = board.nextMove;
	halfmoves = board.fiftyMove;
	epsq = board.epSquare;
 
	if (board.nextMove == WHITE_MOVE)
	{
		std::cout << "wt> setup> type 'help' for more info" << std::endl;
	}
	else
	{
		std::cout << "bl> setup> type 'help' for more info" << std::endl;
	}
 
	// infinite loop - user input:
	for (;;)
	{
		if (board.nextMove == WHITE_MOVE)
		{
			std::cout << "wt> setup> ";
		}
		else
		{
			std::cout << "bl> setup> ";
		}
		std::cout.flush();
		std::cin >> s;
 
		if ((!strcmp(s, "help")) || (!strcmp(s, "h")) || (!strcmp(s, "?")))
		{
			std::cout  << std::endl << "setup help:" << std::endl;
			std::cout << "black               : BLACK to move" << std::endl;
			std::cout << "castle cccc         : castling rights, using FEN-style. Example: 'castle KQkq'" << std::endl;
			std::cout << "clear               : clear the board" << std::endl;
			std::cout << "d                   : display board" << std::endl;
			std::cout << "epsq cc             : set en-passant target square. Example: 'epsq e3'" << std::endl;
			std::cout << "exit                : exit setup" << std::endl;
			std::cout << "fen fenstring       : sets up the board with a FEN-string (6 elements)," << std::endl;
			std::cout << "                      for instance: n1n5/PPPk4/8/8/8/8/4Kppp/5N1N b - - 0 1" << std::endl;
			std::cout << "                      you can paste a string into the windows console" << std::endl;
			std::cout << "                      by using your mouse and right-click paste" << std::endl;
			std::cout << "fifty n             : n = half moves since last capture or pawn move" << std::endl;
			std::cout << "new                 : new game" << std::endl;
			std::cout << "r                   : rotate board" << std::endl;
			std::cout << "rank n fenrank      : piece placement for rank n (from white's perspective)" << std::endl;
			std::cout << "                      fenrank defines the contents of each square, from left to" << std::endl;
			std::cout << "                      right (file a through file h). fenrank uses FEN-style:" << std::endl;
			std::cout << "                      pieces are identified by a single letter (pawn=P," << std::endl;
			std::cout << "                      knight=N, etc), using upper-case letters for white pieces" << std::endl;
			std::cout << "                      and lowercase letters for black pieces." << std::endl;
			std::cout << "                      Blank squares are noted using digits 1 through 8 (the " << std::endl;
			std::cout << "                      number of blank squares)." << std::endl;
			std::cout << "                      Examples: 'rank 1 R1BQKBNR' or 'rank 6 3p2p1'" << std::endl;
			std::cout << "white               : WHITE to move" << std::endl << std::endl;
		}
 
		else if (!strcmp(s, "black"))
		{
			next = BLACK_MOVE;
			board.initFromSquares(board.square, next, halfmoves, whiteCastle , blackCastle , epsq);
		}
 
		else if (!strncmp(s, "castle", 5))
		{
			std::cin >> castle;
			whiteCastle = 0;
			blackCastle = 0;
			if (strstr(castle, "K")) whiteCastle += CANCASTLEOO;
			if (strstr(castle, "Q")) whiteCastle += CANCASTLEOOO;
			if (strstr(castle, "k")) blackCastle += CANCASTLEOO;
			if (strstr(castle, "q")) blackCastle += CANCASTLEOOO;
			board.initFromSquares(board.square, next, halfmoves, whiteCastle , blackCastle , epsq);
		}
 
		else if (!strcmp(s, "clear"))
		{
			for (i = 0; i < 64; i++)
			{
				board.square[i] = EMPTY;
			}
			next = WHITE_MOVE;
			halfmoves = 0;
			whiteCastle = 0;
			blackCastle = 0;
			epsq = 0;
			board.initFromSquares(board.square, next, halfmoves, whiteCastle , blackCastle , epsq);
		}
 
		else if (!strcmp(s, "d"))
		{
			board.display();
			std::cout << "  castleWhite = " << (int) board.castleWhite << " castleBlack = " << (int) board.castleBlack << " epSquare = "
			<< board.epSquare << " fiftyMove = " << board.fiftyMove << std::endl << std::endl;
		}
 
		else if (!strncmp(s, "epsq", 4))
		{
			std::cin >> epsqc;
			epsq = ((int) epsqc[0] - 96) + 8 * ((int) epsqc[1] - 48) - 9;
			board.initFromSquares(board.square, next, halfmoves, whiteCastle , blackCastle , epsq);
		}
 
		else if (!strcmp(s, "exit"))
		{
			std::cout.flush();
			std::cin.clear();
			return;
		}
 
		else if (!strncmp(s, "fen", 3))
		{
			std::cin >> fen;
			std::cin >> fencolor;
			std::cin >> fencastling;
			std::cin >> fenenpassant;
			std::cin >> fenhalfmoveclock;
			std::cin >> fenfullmovenumber;
			setupFen(fen, fencolor, fencastling, fenenpassant, fenhalfmoveclock, fenfullmovenumber);
		}
 
		else if (!strncmp(s, "fifty", 5))
		{
			std::cin >> halfmoves;
			board.initFromSquares(board.square, next, halfmoves, whiteCastle , blackCastle , epsq);
		}
 
		else if (!strcmp(s, "new"))
		{
			board.init();
		}
 
		else if (!strcmp(s, "rank"))
		{
			std::cin >> rank;
			if ((rank > 0) & (rank < 9))
			{
				std::cin >> fenrank;
				// clear the file
				for (file = 1; file < 9; file++)
				{
					board.square[BOARDINDEX[file][rank]] = EMPTY;
				}
				file = 1;    // chess board, file
				i = 0;       // location in string
				while ((fenrank[i] != '\0') && (file < 9))
				{
					// '1'  through '8':
					if (((int) fenrank[i] > 48) && ((int) fenrank[i] < 57))
					{
						file+= (int) fenrank[i] - 48;
					}
					else
					//  other characters:
					{
						switch (fenrank[i])
						{
							case 'P':
								board.square[BOARDINDEX[file][rank]] = WHITE_PAWN;
								file += 1;
								break;
 
							case 'N':
								board.square[BOARDINDEX[file][rank]] = WHITE_KNIGHT;
								file += 1;
								break;
 
							case 'B':
								board.square[BOARDINDEX[file][rank]] = WHITE_BISHOP;
								file += 1;
								break;
 
							case 'R':
								board.square[BOARDINDEX[file][rank]] = WHITE_ROOK;
								file += 1;
								break;
 
							case 'Q':
								board.square[BOARDINDEX[file][rank]] = WHITE_QUEEN;
								file += 1;
								break;
 
							case 'K':
								board.square[BOARDINDEX[file][rank]] = WHITE_KING;
								file += 1;
								break;
 
							case 'p':
								board.square[BOARDINDEX[file][rank]] = BLACK_PAWN;
								file += 1;
								break;
 
							case 'n':
								board.square[BOARDINDEX[file][rank]] = BLACK_KNIGHT;
								file += 1;
								break;
 
							case 'b':
								board.square[BOARDINDEX[file][rank]] = BLACK_BISHOP;
								file += 1;
								break;
 
							case 'r':
								board.square[BOARDINDEX[file][rank]] = BLACK_ROOK;
								file += 1;
								break;
 
							case 'q':
								board.square[BOARDINDEX[file][rank]] = BLACK_QUEEN;
								file += 1;
								break;
 
							case 'k':
								board.square[BOARDINDEX[file][rank]] = BLACK_KING;
								file += 1;
								break;
 
							default:
								break;
						}
					}
					i++;
				}
				board.initFromSquares(board.square, next, halfmoves, whiteCastle , blackCastle , epsq);     
			}
		}
 
		else if (!strcmp(s, "r"))
		{
			board.viewRotated = !board.viewRotated;
		}
 
		else if (!strcmp(s, "white"))
		{
			next = BLACK_MOVE;
			board.initFromSquares(board.square, next, halfmoves, whiteCastle , blackCastle , epsq);
		}
		else
		{
			std::cout << "    command not implemented: " << s << ", type 'help' for more info" << std::endl;
			std::cin.clear();
		}
	}
}