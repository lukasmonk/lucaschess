#ifndef _CRT_SECURE_NO_DEPRECATE
#define _CRT_SECURE_NO_DEPRECATE
#endif
#include <conio.h>
#include <iostream>
#include "defines.h"
#include "protos.h"
#include "extglobals.h"
#include "board.h"
#include "timer.h"

void commands()
{

// ================================================================
// commands is used to read console input and execute the commands
// It also serves as winboard driver. 
// The code is based on H.G. Muller's model WinBoard protocol driver:
// http://www.open-aurec.com/wbforum/viewtopic.php?f=24&t=51739
// =================================================================

	int i, j, number;
	int fenhalfmoveclock;
	int fenfullmovenumber;
	char fen[100];
	char fencolor[1];      
	char fencastling[4];
	char fenenpassant[2];
	char sanMove[12];
	char command[80];
	char userinput[80];
	U64 msStart,msStop, perftcount;
	Timer timer;
	Move move, dummy;

	// =================================================================
	// infinite loop:
	// =================================================================

	while (1) 
	{ 

		fflush(stdout);                 

		// =================================================================
		// think & move
		// =================================================================

		if (XB_MODE)
		{
			if (XB_COMPUTER_SIDE == board.nextMove) 
			{
				#ifdef WINGLET_DEBUG_WINBOARD
					std::cout << "#-winglet : start think" << std::endl;
				#endif
				move = board.think();
				#ifdef WINGLET_DEBUG_WINBOARD
					std::cout << "#-winglet : exit think" << std::endl;
					std::cout << "#<winglet : move " << SQUARENAME[move.getFrom()] << SQUARENAME[move.getTosq()]  << std::endl;
				#endif
				if (move.moveInt) 
				{
					printf("move "); printf("%s",SQUARENAME[move.getFrom()]); printf("%s",SQUARENAME[move.getTosq()]); 
					if (move.isPromotion()) printf("%s",PIECECHARS[move.getProm()]);
					printf("\n");
					makeMove(move);
					board.endOfGame++;
					board.endOfSearch = board.endOfGame;
				}
			}
			fflush(stdout); 

			// =================================================================
			// ponder
			// =================================================================

			if (XB_COMPUTER_SIDE != XB_NONE && XB_COMPUTER_SIDE != XB_ANALYZE && XB_PONDER && board.endOfGame)
			{
				XB_NO_TIME_LIMIT = true;
				#ifdef WINGLET_DEBUG_WINBOARD
					std::cout << "#-winglet : start ponder" << std::endl;
				#endif
				move = board.think();
				#ifdef WINGLET_DEBUG_WINBOARD
					std::cout << "#-winglet : exit ponder" << std::endl;
				#endif
				XB_NO_TIME_LIMIT = false;
			} 

			// =================================================================
			// analyze
			// =================================================================

			if (XB_COMPUTER_SIDE == XB_ANALYZE)
			{
				XB_NO_TIME_LIMIT = true;
				#ifdef WINGLET_DEBUG_WINBOARD
					std::cout << "#-winglet : start analyze" << std::endl;
				#endif
				move = board.think();
				#ifdef WINGLET_DEBUG_WINBOARD
					std::cout << "#-winglet : exit analyze" << std::endl;
				#endif
				XB_NO_TIME_LIMIT = false;
			} 
		}

noPonder:

		// =================================================================
		// display the command prompt
		// =================================================================

		if (!XB_MODE)
		{
			if (board.nextMove == WHITE_MOVE) std::cout << "wt> ";
			else std::cout << "bl> ";
			fflush(stdout);
		}

		// =================================================================
		// read input, but only after attending a pending command received during 
		// search/ponder/analyze:
		// =================================================================

		if (!XB_DO_PENDING)
		{
			#ifdef WINGLET_DEBUG_WINBOARD
				if (XB_MODE)
					std::cout << "#-winglet : COMPUTER_SIDE=" << (int)XB_COMPUTER_SIDE << " PONDER=" << XB_PONDER << " nextMove=" << (int)board.nextMove << std::endl;
			#endif

			for (CMD_BUFF_COUNT = 0; (CMD_BUFF[CMD_BUFF_COUNT] = getchar()) != '\n'; CMD_BUFF_COUNT++);
			CMD_BUFF[CMD_BUFF_COUNT+1] = '\0';

			#ifdef WINGLET_DEBUG_WINBOARD
				if (XB_MODE) std::cout << "#>winglet : " << CMD_BUFF << std::endl;
			#endif
		}
		#ifdef WINGLET_DEBUG_WINBOARD
			else
			{
				if (XB_MODE) std::cout << "#>winglet : " << CMD_BUFF << " (from peek)" << std::endl;
			}
		#endif
		XB_DO_PENDING = false;
	
		// =================================================================
		// ignore empty lines
		// =================================================================

		if (!CMD_BUFF_COUNT) continue; 

		// =================================================================
		// extract the first word
		// =================================================================

		sscanf(CMD_BUFF, "%s", command);

		// =================================================================
		// help, h or ?: show this help - list of CONSOLE-ONLY COMMANDS
		// =================================================================
		if ((!XB_MODE) && ((!strcmp(command, "help")) || (!strcmp(command, "h")) || (!strcmp(command, "?"))))
		{ 
			std::cout << std::endl << "help:" << std::endl;
			std::cout << "black               : BLACK to move" << std::endl;
			std::cout << "cc                  : play computer-to-computer " << std::endl;
			std::cout << "d                   : display board " << std::endl;
			std::cout << "eval                : show static evaluation of this position" << std::endl;
			std::cout << "exit                : exit program " << std::endl;
			std::cout << "game                : show game moves " << std::endl;
			std::cout << "go                  : computer next move " << std::endl;
			std::cout << "help, h, or ?       : show this help " << std::endl;
			std::cout << "info                : display variables (for testing purposes)" << std::endl;
			std::cout << "ini                 : read the initialization file" << std::endl;
			std::cout << "memory n            : max memory to use (in MB)" << std::endl;
			std::cout << "move e2e4, or h7h8q : enter a move (use this format)" << std::endl;
			std::cout << "moves               : show all legal moves" << std::endl;
			std::cout << "new                 : start new game" << std::endl;
			std::cout << "perft n             : calculate raw number of nodes from here, depth n " << std::endl;
			#ifdef WINGLET_VERBOSE_SEE
				std::cout << "qsearch             : shows sorted capture movelist" << std::endl;
			#endif
			std::cout << "quit                : exit program " << std::endl;
			std::cout << "r                   : rotate board " << std::endl;
			std::cout << "readfen filename n  : reads #-th FEN position from filename" << std::endl;
			std::cout << "sd n                : set the search depth to n" << std::endl;
			std::cout << "setup               : setup board... " << std::endl;
			std::cout << "test filename       : starts search on all FEN position in 'filename'" << std::endl;
			std::cout << "                      using current time & search depth parameters" << std::endl;
			std::cout << "                      output is written in test.log" << std::endl;
			std::cout << "time s              : time per move in seconds" << std::endl;
			std::cout << "undo                : take back last move" << std::endl;
			std::cout << "white               : WHITE to move" << std::endl;
			std::cout << std::endl;
			continue; 
		}

		// =================================================================
		// accepted: in reply to the "feature" command
		// =================================================================

		if (XB_MODE && !strcmp(command, "accepted")) continue; 

		// =================================================================
		// analyze: enter analyze mode
		// =================================================================

		if (XB_MODE && !strcmp(command, "analyze")) 
		{ 
			XB_COMPUTER_SIDE = XB_ANALYZE;			
			continue; 
		}

		// =================================================================
		// black: BLACK to move
		// =================================================================

		if (!XB_MODE && !strcmp(command, "black") && board.nextMove == WHITE_MOVE)
		{ 
			board.hashkey ^= KEY.side;
			board.endOfSearch = 0; 
			board.endOfGame = 0;
			board.nextMove = BLACK_MOVE;
			continue; 
		}

		// =================================================================
		// bk: show book moves from this position, if any
		// =================================================================

		if (XB_MODE && !strcmp(command, "bk")) continue; 

		// =================================================================
		// cc: play computer-to-computer
		// =================================================================

		if (!XB_MODE && !strcmp(command, "cc"))    
		{ 
			while (!_kbhit() && !board.isEndOfgame(i, dummy))
			{
				move = board.think();
				if (move.moveInt) 
				{
					makeMove(move);
					board.endOfGame++;
					board.endOfSearch = board.endOfGame;
					board.display();
				}
			}
			continue; 
		}

		// =================================================================
		// computer: the opponent is also a computer chess engine
		// =================================================================

		if (XB_MODE && !strcmp(command, "computer")) continue; 

		// =================================================================
		// cores n: informs the engine on how many CPU cores it is allowed to use maximally
		// =================================================================

		if (XB_MODE && !strcmp(command, "cores")) continue; 

		// =================================================================
		// d: display board
		// =================================================================

		if (!XB_MODE && !strcmp(command, "d"))
		{
			board.display();
			continue; 
		}

		// =================================================================
		// easy: turn off pondering
		// =================================================================

		if (XB_MODE && !strcmp(command, "easy"))    
		{ 
			XB_PONDER = false;
			continue; 
		}

		// =================================================================
		// egtpath type path: informs the engine in which directory it can find end-game tables
		// =================================================================

		if (XB_MODE && !strcmp(command, "egtpath")) continue; 

		// =================================================================
		// eval: show static evaluation of this position
		// =================================================================

		if (!XB_MODE && !strcmp(command, "eval"))    
		{ 
			number = board.eval();
			std::cout << "eval score = " << number << std::endl;
			#ifdef WINGLET_DEBUG_EVAL
				board.mirror();
				board.display();
				i = board.eval();
				std::cout << "eval score = " << i << std::endl;
				board.mirror();
				if (number != i) std::cout << "evaluation is not symmetrical! " << number << std::endl;
				else std::cout << "evaluation is symmetrical" << std::endl;
			#endif
			continue; 
		}

		// =================================================================
		// exit: leave analyze mode / exit program (if not in WB)
		// =================================================================

		if (!strcmp(command, "exit"))    
		{ 
			if (XB_MODE)
			{
				XB_COMPUTER_SIDE = XB_NONE;			
				continue; 
			}
			else break;
		}

		// =================================================================
		// force: Set the engine to play neither color
		// =================================================================

		if (XB_MODE && !strcmp(command, "force"))   
		{ 
			XB_COMPUTER_SIDE = XB_NONE;
			continue; 
		}

		// =================================================================
		// game: show game moves
		// =================================================================

		if (!XB_MODE && !strcmp(command, "game"))   
		{ 
			if (board.endOfGame)
			{
				// make a temporary copy of board.gameLine[];
				number = board.endOfGame;
				GameLineRecord *tmp = new GameLineRecord[number];
				memcpy(tmp, board.gameLine, number * sizeof(GameLineRecord));

				// unmake all moves:
				for (i = number-1 ; i >= 0 ; i--) 
				{ 
					unmakeMove(tmp[i].move);
					board.endOfSearch = --board.endOfGame;
				}

				// redo all moves:
				j = board.nextMove;
				for (i = 0 ; i < number; i++)
				{
					// move numbering:
					if (!((i+j+2)%2)) std::cout << (i+2*j+2)/2 << ". ";
					else if (!i) std::cout << "1. ... ";

					// construct the move string
					toSan(tmp[i].move, sanMove);
					std::cout << sanMove; 

					// output CRLF, or space:
					if (!((i+j+1)%2)) std::cout << std::endl;
					else std::cout << " ";

					// make the move:
					makeMove(tmp[i].move);
					board.endOfSearch = ++board.endOfGame;
				}
				std::cout << std::endl;

				// delete the temporary copy:
				delete[] tmp;
			} 
			else
			{
				std::cout << "there are no game moves" << std::endl;        
			}
			continue; 
		}

		// =================================================================
		// go: leave force mode and set the engine to play the color that is on move
		// =================================================================

		if (!strcmp(command, "go"))      
		{
			if (XB_MODE)
			{
				XB_COMPUTER_SIDE = board.nextMove;  
				continue; 
			}
			else
			{
				if (!board.isEndOfgame(i, dummy))
				{
					move = board.think();
					if (move.moveInt) 
					{
						makeMove(move);
						board.endOfGame++;
						board.endOfSearch = board.endOfGame;
					}
					board.display();
					board.isEndOfgame(i, dummy);
					CMD_BUFF_COUNT = '\0';
				}
				else
				{
					board.display();
					CMD_BUFF_COUNT = '\0';
				}
			}
			continue;
		}

		// =================================================================
		// hard: turn on pondering
		// =================================================================

		if (XB_MODE && !strcmp(command, "hard"))    
		{ 
			XB_PONDER = true;  
			continue; 
		}

		// =================================================================
		// hint: respond with "Hint: xxx", where xxx is a suggested move
		// =================================================================

		if (XB_MODE && !strcmp(command, "hint")) 
		{
				continue; 
		}

		// =================================================================
		// ics hostname: the engine is playing on an Internet Chess Server (ICS) with the given hostname
		// =================================================================

		if (XB_MODE && !strcmp(command, "ics"))     { continue; }

		// =================================================================
		// info: display variables (for testing purposes)
		// =================================================================

		if (!XB_MODE && !strcmp(command, "info"))    
		{ 
			info();
			continue; 
		}

		// =================================================================
		// ini: read the initialization file
		// =================================================================

		if (!XB_MODE && !strcmp(command, "ini"))    
		{ 
			readIniFile();
			continue; 
		}

		// =================================================================
		// level mps base inc: set time controls
		// =================================================================

		if (XB_MODE && !strcmp(command, "level"))   
		{
			sscanf(CMD_BUFF, "level %d %d %d", &XB_MPS, &XB_MIN, &XB_INC) == 3 ||  
			sscanf(CMD_BUFF, "level %d %d:%d %d", &XB_MPS, &XB_MIN, &XB_SEC, &XB_INC);
			XB_INC *= 1000;
			continue;
		}

		// =================================================================
		// memory n: informs the engine on how much memory it is allowed to use maximally, in MB
		// =================================================================

		if (XB_MODE && !strcmp(command, "memory")) continue; 

		// =================================================================
		// moves: show all legal moves
		// =================================================================

		if (!XB_MODE && !strcmp(command, "moves"))    
		{ 
			board.moveBufLen[0] = 0;
			board.moveBufLen[1] = movegen(board.moveBufLen[0]);
			std::cout << std::endl << "moves from this position:" << std::endl;
			number = 0;
			for (i = board.moveBufLen[0]; i < board.moveBufLen[1]; i++)
			{
				makeMove(board.moveBuffer[i]);
				if (isOtherKingAttacked())
				{
					unmakeMove(board.moveBuffer[i]);
				}
				else
				{
					unmakeMove(board.moveBuffer[i]);
					toSan(board.moveBuffer[i], sanMove);
					std::cout << ++number << ". " << sanMove << std::endl;
				}
			}
			continue; 
		}

		// =================================================================
		// move: enter a move (use this format: move e2e4, or h7h8q)
		// =================================================================

		if (!XB_MODE && !strcmp(command, "move"))    
		{
			sscanf(CMD_BUFF,"move %s",userinput);
			// generate the pseudo-legal move list
			board.moveBufLen[0] = 0;
			board.moveBufLen[1] = movegen(board.moveBufLen[0]);
 
			if (isValidTextMove(userinput, move))        // check to see if the user move is also found in the pseudo-legal move list
			{
				makeMove(move);
 
				if (isOtherKingAttacked())              // post-move check to see if we are leaving our king in check
				{
					unmakeMove(move);
					std::cout << "    invalid move, leaving king in check: " << userinput << std::endl;
				}
				else
				{
					board.endOfGame++;
					board.endOfSearch = board.endOfGame;
					board.display();
				}
			}
			else
			{
				std::cout << "    move is invalid or not recognized: " << userinput << std::endl;
			}
			continue; 
		}

		// =================================================================
		// name <something>: informs the engine of its opponent's name
		// =================================================================

		if (XB_MODE && !strcmp(command, "name")) continue; 

		// =================================================================
		// new: reset the board to the standard chess starting position
		// =================================================================

		if (!strcmp(command, "new"))     
		{
			board.init(); 
			if (XB_MODE) 
			{
				XB_COMPUTER_SIDE = BLACK_MOVE;
				board.searchDepth = MAX_PLY;
			}
			continue; 
		}

		// =================================================================
		// nopost: turn off thinking/pondering output
		// =================================================================

		if (XB_MODE && !strcmp(command, "nopost"))  
		{ 
			XB_POST = false;
			continue; 
		}

		// =================================================================
		// otim n: set a clock that belongs to the opponent, in centiseconds
		// =================================================================
		if (XB_MODE && !strcmp(command, "otim"))    
		{ 
			// do not start pondering after receiving time commands, as a move will follow immediately
			sscanf(CMD_BUFF, "otim %d", &XB_OTIM);
			XB_OTIM *= 10;  // convert to miliseconds;
			goto noPonder; 
		} 

		// =================================================================
		// option name[=value]: setting of an engine-define option
		// =================================================================
		if (XB_MODE && !strcmp(command, "option"))  continue;

		// =================================================================
		// perft: calculate raw number of nodes from here, depth n 
		// =================================================================
		if (!XB_MODE && !strcmp(command, "perft"))  
		{ 
			sscanf(CMD_BUFF,"perft %d", &number);
			std::cout << "    starting perft " << number << "..." << std::endl;
			timer.init();
			board.moveBufLen[0] = 0;
 
			#ifdef WINGLET_DEBUG_PERFT
				ICAPT = 0;
				IEP = 0;
				IPROM = 0;
				ICASTLOO = 0;
				ICASTLOOO = 0;
				ICHECK = 0;
			#endif
 
			msStart = timer.getms();
			perftcount = perft(0, number);
			msStop = timer.getms();
 
			std::cout << "nodes        = " << perftcount << ", " << msStop - msStart << " ms, ";
			if ((msStop - msStart) > 0)
			std::cout << (perftcount/(msStop - msStart)) << " knods/s";
			std::cout << std::endl;
			CMD_BUFF_COUNT = '\0';
 
			#ifdef WINGLET_DEBUG_PERFT
				std::cout << "captures     = " << ICAPT << std::endl;
				std::cout << "en-passant   = " << IEP << std::endl;
				std::cout << "castlings    = " << ICASTLOO + ICASTLOOO << std::endl;
				std::cout << "promotions   = " << IPROM << std::endl;
				std::cout << "checks       = " << ICHECK << std::endl;
			#endif
			continue; 
		}

		// =================================================================
		// ping n: reply by sending the string pong n
		// =================================================================

		if (XB_MODE && !strcmp(command, "ping"))    
		{ 
			sscanf(CMD_BUFF,"ping %d", &number);
			std::cout << "pong " << number << std::endl; 
			continue; 
		}

		// =================================================================
		// post: turn on thinking/pondering output
		// =================================================================

		if (XB_MODE && !strcmp(command, "post"))    
		{ 
			XB_POST = true; 
			continue; 
		}

		// =================================================================
		// protover n: protocol version
		// =================================================================

		if (XB_MODE && !strcmp(command, "protover")) 
		{
			std::cout << "feature ping=1" << std::endl;
			std::cout << "feature setboard=1" << std::endl;
			std::cout << "feature colors=0" << std::endl;
			std::cout << "feature usermove=1" << std::endl;
			std::cout << "feature memory=1" << std::endl;
			std::cout << "feature debug=1" << std::endl;
			std::cout << "feature done=1" << std::endl;

			continue;
		}

		#ifdef WINGLET_VERBOSE_SEE
		// =================================================================
		// qsearch: shows sorted capture movelist
		// =================================================================
				if (!XB_MODE && !strcmp(command, "qsearch"))  
				{ 
					board.moveBufLen[0] = 0;
					board.moveBufLen[1] = captgen(board.moveBufLen[0]);
					std::cout << std::endl << "sorted capturing moves from this position:" << std::endl;
					std::cout << std::endl << "        score:" << std::endl;
					number = 0;
					for (i = board.moveBufLen[0]; i < board.moveBufLen[1]; i++)
					{
						makeMove(board.moveBuffer[i]);
						if (isOtherKingAttacked())
						{
							unmakeMove(board.moveBuffer[i]);
						}
						else
						{
							unmakeMove(board.moveBuffer[i]);
							std::cout << ++number << ". "; 
							displayMove(board.moveBuffer[i]);
							std::cout << "   " << board.moveBuffer[i + OFFSET].moveInt << std::endl;
						}
					}
					continue; 
				}
		#endif

		// =================================================================
		// quit: exit program
		// =================================================================

		if (!strcmp(command, "quit")) break; 

		// =================================================================
		// r: rotate board
		// =================================================================

		if (!XB_MODE && !strcmp(command, "r"))  
		{ 
			board.viewRotated = !board.viewRotated;
			continue; 
		}

		// =================================================================
		// random: ignored
		// =================================================================

		if (XB_MODE && !strcmp(command, "random")) continue;

		// =================================================================
		// rating: ICS opponent's rating
		// =================================================================

		if (XB_MODE && !strcmp(command, "rating")) continue;

		// =================================================================
		// readfen filename n: reads #-th FEN position from filename
		// =================================================================

		if (!XB_MODE && !strcmp(command, "readfen"))  
		{ 
			sscanf(CMD_BUFF,"readfen %s %d", userinput, &number);
			board.init();
			readFen(userinput, number);
			board.display();
			continue; 
		}

		// =================================================================
		// rejected: feature is rejected
		// =================================================================

		if (XB_MODE && !strcmp(command, "rejected")) continue;

		// =================================================================
		// remove: undo the last two moves (one for each player) and continue playing the same color.
		// =================================================================

		if (XB_MODE && !strcmp(command, "remove"))  
		{ 
			if (board.endOfGame)
			{
				unmakeMove(board.gameLine[--board.endOfGame].move);
				board.endOfSearch = board.endOfGame;
			}
			if (board.endOfGame)
			{
				unmakeMove(board.gameLine[--board.endOfGame].move);
				board.endOfSearch = board.endOfGame;
			}
			continue; 
		}

		// =================================================================
		// result string {comment}: end the each game, e.g.: result 1-0 {White mates}
		// =================================================================

		if (XB_MODE && !strcmp(command, "result"))  
		{ 
			XB_COMPUTER_SIDE = XB_NONE;
			continue; 
		}

		// =================================================================
		// sd n: set the search depth to n
		// =================================================================

		if (!strcmp(command, "sd"))      
		{ 
			sscanf(CMD_BUFF,"sd %d", &board.searchDepth);
			if (board.searchDepth < 1) board.searchDepth = 1;
			if (board.searchDepth > MAX_PLY) board.searchDepth = MAX_PLY;
			std::cout << "winglet> search depth " << board.searchDepth << std::endl;
			continue; 
		}

		// =================================================================
		// setboard fen: set up the board/position 
		// =================================================================

		if (XB_MODE && !strcmp(command, "setboard"))
		{ 
			XB_COMPUTER_SIDE = XB_NONE;
			sscanf(CMD_BUFF, "setboard %s %s %s %s %d %d", fen, fencolor, fencastling, fenenpassant, &fenhalfmoveclock, &fenfullmovenumber);
			setupFen(fen, fencolor, fencastling, fenenpassant, fenhalfmoveclock, fenfullmovenumber);
			continue; 
		}

		// =================================================================
		// setup: setup board... 
		// =================================================================

		if (!XB_MODE && !strcmp(command, "setup"))
		{ 
			setup();
			continue; 
		}

		// =================================================================
		// stopfrac (0-100%): undocumented command to interactively change this 
		// parameter (e.g. for running testsuites), default value is 60
		// Don't start a new iteration if STOPFRAC fraction of the max search time 
		// has passed
		// =================================================================

		if (!XB_MODE && !strcmp(command, "stopfrac"))      
		{ 
			number = (int)(STOPFRAC * 100);
			sscanf(CMD_BUFF, "stopfrac %d", &number);
			if (number < 1) number = 1;
			if (number > 100) number = 100;
			STOPFRAC = (float)(number/100.0);
			std::cout << "winglet> stopfrac " << 100*STOPFRAC << std::endl;
			continue; 
		}

		// =================================================================
		// st time: set time controls
		// =================================================================

		if (XB_MODE && !strcmp(command, "st"))      
		{ 
			sscanf(CMD_BUFF, "st %d", &board.maxTime);
			board.maxTime *= board.maxTime;  // convert to ms
			continue; 
		}

		// =================================================================
		// test filename: starts search on all FEN position in 'filename
		// =================================================================

		if (!XB_MODE && !strcmp(command, "test"))      
		{ 
			sscanf(CMD_BUFF,"test %s", userinput);
			board.init();
			test(userinput);
			continue; 
		}

		// =================================================================
		// time: set a clock that belongs to the engine
		// =================================================================

		if (!strcmp(command, "time"))    
		{ 
			number = (int)board.maxTime / 1000;
			sscanf(CMD_BUFF,"time %d", &number);
			if (number < 1) number = 1;
			if (!XB_MODE) std::cout << "winglet> search time " << number << " seconds" << std::endl;
			if (XB_MODE)
			{
				XB_CTIM = number * 10;
				board.maxTime = number * 10; // conversion to ms
			}
			else
			{
				board.maxTime = number * 1000; // conversion to ms
			}
			goto noPonder; 
		}

		// =================================================================
		// undo: take back last move
		// =================================================================

		if (!strcmp(command, "undo"))    
		{ 
			if (board.endOfGame)
			{
				unmakeMove(board.gameLine[--board.endOfGame].move);
				board.endOfSearch = board.endOfGame;
				if (!XB_MODE) board.display();
			}
			else if (!XB_MODE) std::cout << "already at start of game" << std::endl;
			continue; 
		}

		// =================================================================
		// usermove move: do a move
		// =================================================================

		if (XB_MODE && !strcmp(command, "usermove"))
		{
			sscanf(CMD_BUFF,"usermove %s",userinput);

			// generate the pseudo-legal move list
			board.moveBufLen[0] = 0;
			board.moveBufLen[1] = movegen(board.moveBufLen[0]);
 
			if (isValidTextMove(userinput, move))        // check to see if the user move is also found in the pseudo-legal move list
			{
				makeMove(move);
				if (isOtherKingAttacked())              // post-move check to see if we are leaving our king in check
				{
					#ifdef WINGLET_DEBUG_WINBOARD
						std::cout << "#-winglet : usermove illegal" << std::endl;
					#endif
					unmakeMove(move);
				}
				else
				{
					#ifdef WINGLET_DEBUG_WINBOARD
						std::cout << "#-winglet : usermove " << userinput << " made" << std::endl;
					#endif
					board.endOfGame++;
					board.endOfSearch = board.endOfGame;
				}
			}
			else
			{
				#ifdef WINGLET_DEBUG_WINBOARD
						std::cout << "#-winglet : usermove illegal" << std::endl;
				#endif
			} 
			continue;
		}

		// =================================================================
		// variant: the game is not standard chess
		// =================================================================

		if(XB_MODE && !strcmp(command, "variant")) continue; 

		// =================================================================
		// white: WHITE to move
		// =================================================================

		if (!XB_MODE && !strcmp(command, "white") && board.nextMove == BLACK_MOVE)    
		{ 
			board.hashkey ^= KEY.side;
			board.endOfSearch = 0; 
			board.endOfGame = 0;
			board.nextMove = WHITE_MOVE;
			continue; 
		}

		// =================================================================
		// xboard: put the engine into "xboard mode", stop all unsolicited output
		// =================================================================

		if (!XB_MODE && !strcmp(command, "xboard"))  
		{ 
			#ifdef WINGLET_DEBUG_WINBOARD
				if (XB_MODE) std::cout << "#>winglet : xboard" << std::endl;
			#endif

			std::cout << std::endl;
			XB_COMPUTER_SIDE = XB_NONE;
			XB_MODE = true;
			XB_POST = false;
			board.init();
			continue; 
		}

		// =================================================================
		// unknown command: 
		// =================================================================

		printf("Error: unknown command: %s\n", command);
		#ifdef WINGLET_DEBUG_WINBOARD
			if (XB_MODE) std::cout << "#<winglet : Error: unknown command: " << command << std::endl;
		#endif
	}
}
