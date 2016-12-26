// Debugging utilities.
// Display board
// perft

// simetry test.

#include <stdlib.h>
#include "system.h"
#include "CDebug.h"
#include "MoveList.h"
#include "UndoData.h"
#include "Board.h"

void Board::Display()
{
	DisplayF(stdout);
}
void Board::DisplayF(FILE *display_file)
{
	static const char Algebra[] = {' ','P','C','A','T','D','R'};
  char display_board[64][4];

  int       i, j;

  for (i = 0; i < 64; i++) {
	  display_board[i][0] = '\0';
	  display_board[i][1] = '\0';
	  display_board[i][2] = '\0';
	  display_board[i][3] = '\0';
	  switch(board[sq64to16x12(i)])
	  {
	  case wPawn:
			display_board[i][0] = '<';
			display_board[i][1] = 'P';
			display_board[i][2] = '>';
			break;
	  case bPawn:
			display_board[i][0] = '-';
			display_board[i][1] = 'P';
			display_board[i][2] = '-';
			break;
	  case wKnight:
			display_board[i][0] = '<';
			display_board[i][1] = 'N';
			display_board[i][2] = '>';
			break;
	  case bKnight:
			display_board[i][0] = '-';
			display_board[i][1] = 'N';
			display_board[i][2] = '-';
			break;
	  case wBishop:
			display_board[i][0] = '<';
			display_board[i][1] = 'B';
			display_board[i][2] = '>';
			break;
	  case bBishop:
			display_board[i][0] = '-';
			display_board[i][1] = 'B';
			display_board[i][2] = '-';
			break;
	  case wRook:
			display_board[i][0] = '<';
			display_board[i][1] = 'R';
			display_board[i][2] = '>';
			break;
	  case bRook:
			display_board[i][0] = '-';
			display_board[i][1] = 'R';
			display_board[i][2] = '-';
			break;
	  case wQueen:
			display_board[i][0] = '<';
			display_board[i][1] = 'Q';
			display_board[i][2] = '>';
			break;
	  case bQueen:
			display_board[i][0] = '-';
			display_board[i][1] = 'Q';
			display_board[i][2] = '-';
			break;
	  case wKing:
			display_board[i][0] = '<';
			display_board[i][1] = 'K';
			display_board[i][2] = '>';
			break;
	  case bKing:
			display_board[i][0] = '-';
			display_board[i][1] = 'K';
			display_board[i][2] = '-';
			break;
	  default:
			display_board[i][0] = ' ';
			display_board[i][1] = ColourSq64(i)?'.':' ';
			display_board[i][2] = ' ';
			break;
	  }
  }
  fprintf(display_file, "\n       +---+---+---+---+---+---+---+---+\n");
  for (i = 7; i >= 0; i--) {
    fprintf(display_file, "   %2d  ", i + 1);
    for (j = 0; j < 8; j++)
      fprintf(display_file, "|%s", display_board[i * 8 + j]);
    fprintf(display_file, "|\n");
    fprintf(display_file, "       +---+---+---+---+---+---+---+---+\n");
  }
  fprintf(display_file, "         a   b   c   d   e   f   g   h\n\n");
  fprintf(display_file,"%s to move.\n",wtm == White ? "White":"Black");
}

void Board::Perft(int ply)
{
	const bool ShowPartials = false;
	if(ply == 0)
		return;
	ASSERT(IsLegal());

	UndoData undo;
	MoveList List;
	int Move;
	int i;
#ifdef _DEBUG
	char dest[5];
	int BoardCopy[BoardSize];
	int j;
	for(j = 0; j < BoardSize;j++)
		BoardCopy[j] = board[j];
#endif

	GenPseudoMoves(List);
	for(i = 0; i < List.Length();i++)
	{
		Move = List.Move(i);
#ifdef _DEBUG
		MoveToAlgebra(Move,dest);
#endif
		DoMove(Move,undo);
		// Test legality
		if(IsLegal())
		{
			if(ply -1)
			{
#ifdef _DEBUG
				if(ShowPartials && ply == 2)
				{
					int ini,fin;
					ini = VisitedNodes;
					Perft(ply-1);
					fin = VisitedNodes;
					printf("parcial %s -> %d\n",dest,fin-ini);
				}
				else
#endif
					Perft(ply-1);
			}
			else
				VisitedNodes++;
		}

		UndoMove(Move,undo);
#ifdef _DEBUG
		// after undo all is back
	for(j = 0; j < BoardSize;j++)
		ASSERT(BoardCopy[j] == board[j]);
#endif
	}
}

void Board::DoPerft(int ply)
{
	extern long TimeElapsed();
	static long init;

	init = TimeElapsed();
	// init
	VisitedNodes = 0;
	InitAttack();
	CastleMask_init();
	// ****************
	Perft(ply);
	// ****************
	int elapsed = TimeElapsed()-init;
	if(elapsed == 0)elapsed = 1000;
	printf("Total Jugadas %ld en %ld nps=%ld\n",VisitedNodes,elapsed,VisitedNodes/(elapsed));
}

void Board::TestPerft()
{
	extern long TimeElapsed();
	static long init;
	int num = 0;
	init = TimeElapsed();

	FILE *fd = fopen("salida.epd","r");
	if(fd)
	{
		InitAttack();
		CastleMask_init();

		char buffer[150];
		int cuenta;
		char *aux;
		while(fgets(buffer,sizeof(buffer),fd))
		{
			aux = &buffer[0];
			while(*aux != ';')aux++;
			aux++;
			cuenta = atoi(aux);
			// leemos 
			this->LoadFen(buffer);
			// hacemos un  perft 
			VisitedNodes = 0;
			Perft(3);
			// verificamos
			ASSERT(cuenta == VisitedNodes);
			num++;
			if(num > 100000) break;
		}
		fclose(fd);
	}
	printf("tiempo %d perft = %ld\n",num,TimeElapsed()-init);
}


void Board::DebugSimetric(char *Epdfilename)
{
	FILE *fd = fopen(Epdfilename,"r");
	if(fd)
	{
		char buffer[150];
		int Eval1,Eval2;
		while(fgets(buffer,sizeof(buffer),fd))
		{
			LoadFen(buffer);
			Eval1 = SeeEval(); // GetEval();
			LoadFenSimetric(buffer);
			Eval2 = SeeEval(); //GetEval();
			if(Eval1 != Eval2)
			{
				printf("No Simetrical evaluation for position\n%s\n",buffer);
				LoadFen(buffer);
				Display();
				LoadFenSimetric(buffer);
				Display();
			}
		}
		fclose(fd);
		printf("end of test\n");
	}
}
void Board::LoadFenSimetric(char *fen)
{

	InitBoard();
	// hash

	int square;
	EnPassant = 0;
	CastleFlags = 0;
	square = 56;

	while(*fen != ' ')
	{
		switch(*fen)
		{
		case '1':
			square++;
			break;
		case '2':
			square+=2;
			break;
		case '3':
			square+=3;
			break;
		case '4':
			square+=4;
			break;
		case '5':
			square+=5;
			break;
		case '6':
			square+=6;
			break;
		case '7':
			square+=7;
			break;
		case '8':
			square+=8;
			break;
		case '/':
			square-= 16;
			break;
		case 'p':
			PutwPawn(sq64to16x12(square^070));
			Opening += Pst[0][square^070][0];EndGame += Pst[0][square^070][1];
			square++;
			break;
		case 'P':
			PutbPawn(sq64to16x12(square^070));
			Opening += Pst[1][square^070][0];EndGame += Pst[1][square^070][1];
			square++;
			break;
		case 'n':
			PutwKnight(sq64to16x12(square^070));
			Opening += Pst[2][square^070][0];EndGame += Pst[2][square^070][1];
			square++;
			break;
		case 'N':
			PutbKnight(sq64to16x12(square^070));
			Opening += Pst[3][square^070][0];EndGame += Pst[3][square^070][1];
			square++;
			break;
		case 'b':
			PutwBishop(sq64to16x12(square^070));
			Opening += Pst[4][square^070][0];EndGame += Pst[4][square^070][1];
			square++;
			break;
		case 'B':
			PutbBishop(sq64to16x12(square^070));
			Opening += Pst[5][square^070][0];EndGame += Pst[5][square^070][1];
			square++;
			break;
		case 'r':
			PutwRook(sq64to16x12(square^070));
			Opening += Pst[6][square^070][0];EndGame += Pst[6][square^070][1];
			square++;
			break;
		case 'R':
			PutbRook(sq64to16x12(square^070));
			Opening += Pst[7][square^070][0];EndGame += Pst[7][square^070][1];
			square++;
			break;
		case 'q':
			PutwQueen(sq64to16x12(square^070));
			Opening += Pst[8][square^070][0];EndGame += Pst[8][square^070][1];
			square++;
			break;
		case 'Q':
			PutbQueen(sq64to16x12(square^070));
			Opening += Pst[9][square^070][0];EndGame += Pst[9][square^070][1];
			square++;
			break;
		case 'k':
			PutwKing(sq64to16x12( square^070));
			Opening += Pst[10][square^070][0];EndGame += Pst[10][square^070][1];
			square++;
			break;
		case 'K':
			PutbKing(sq64to16x12(square^070));
			Opening += Pst[11][square^070][0];EndGame += Pst[11][square^070][1];
			square++;
			break;
		}
		fen++;
	}
	fen++;
	// Field 2 
	if(*fen=='b')
		wtm = White;
	if(*fen=='w')
		wtm = Black;
	fen++; 
	fen++; 
	// Field 3 Castling right
	if(*fen == '-')
	{
		CastleFlags = 0;
	}
	else
	{
		CastleFlags = 0;
		while(*fen != ' ')
		{
			switch(*fen)
			{
			case 'q':
					CastleFlags |= FlagsWhiteQueenCastle;
				break;
			case 'k':
				CastleFlags |= FlagsWhiteKingCastle;
				break;
			case 'Q':
					CastleFlags |= FlagsBlackQueenCastle;
				break;
			case 'K':
					CastleFlags |= FlagsBlackKingCastle;
				break;
			}
			fen++;
		}
	}
	fen++;
	while(*fen == ' ')fen++;
	//Field 4  Enpassant
	if(*fen=='-')
	{
		EnPassant = 0;
	}
	else
	{
		EnPassant = 0;
		EnPassant = (*fen)-'a';
		fen++;
		EnPassant += ((*fen)-'1')*8;
		EnPassant = sq64to16x12(EnPassant^070);
	}
	Initialize();
	InitAttack();
}
