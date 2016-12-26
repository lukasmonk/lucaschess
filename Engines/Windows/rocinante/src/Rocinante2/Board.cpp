#include <stdlib.h>
#include <string.h>
#include "CDebug.h"
#include "MoveList.h"
#include "UndoData.h"
#include "Board.h"


int Pst[12][64][2]; // piece,square,stage

Board::Board(void)
{
	CastleMask_init();	
	PreEval();
	TT = NULL;
	TTSize = 0x1000;
	PonderMode = 0;
	InBStar = 0;
}


Board::~Board(void)
{
}

void Board::InitBoard()
{
	int sq,f,c;
	// naive implementation
	for(sq = 0;sq < sizeof(board);sq++)
		board[sq] = Edge;
	for(f = 0; f < 8;f++)
		for(c = 0;c < 8;c++)
			board[sq16x12(f,c)] = Empty;
	for(f=0;f < MAXPAWNS;f++)
		bPawnList[f] = 0;
	for(f=0;f < MAXKNIGHTS;f++)
		bKnightList[f] = 0;
	for(f=0;f < MAXBISHOPS;f++)
		bBishopList[f]= 0;
	for(f=0;f < MAXROOKS;f++)
		bRookList[f] = 0;
	for(f=0;f < MAXQUEENS;f++)
		bQueenList[f] = 0;
	bKingPos = 0;
	for(f=0;f < MAXPAWNS;f++)
		wPawnList[f] = 0;
	for(f=0;f < MAXKNIGHTS;f++)
		wKnightList[f] = 0;
	for(f=0;f < MAXBISHOPS;f++)
		wBishopList[f]= 0;
	for(f=0;f < MAXROOKS;f++)
		wRookList[f] = 0;
	for(f=0;f < MAXQUEENS;f++)
		wQueenList[f] = 0;
	wKingPos = 0;
	NumBPawn = 0;
	NumBBishop = 0;
	NumBRook = 0;
	NumBKnight = 0;
	NumBQueen = 0;
	NumWPawn = 0;
	NumWBishop = 0;
	NumWRook = 0;
	NumWKnight = 0;
	NumWQueen = 0;
	Opening = 0;
	EndGame = 0;
	hash = 0ull;
}

void Board::LoadFen(char *fen)
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
			PutbPawn(sq64to16x12(square));
			hash ^= GetZobKey(square,Black,Pawn);
			Opening += Pst[1][square][0];EndGame += Pst[1][square][1];
			square++;
			break;
		case 'P':
			PutwPawn(sq64to16x12(square));
			hash ^= GetZobKey(square,White,Pawn);
			Opening += Pst[0][square][0];EndGame += Pst[0][square][1];
			square++;
			break;
		case 'n':
			PutbKnight(sq64to16x12(square));
			hash ^= GetZobKey(square,Black,Knight);
			Opening += Pst[3][square][0];EndGame += Pst[3][square][1];
			square++;
			break;
		case 'N':
			PutwKnight(sq64to16x12(square));
			hash ^= GetZobKey(square,White,Knight);
			Opening += Pst[2][square][0];EndGame += Pst[2][square][1];
			square++;
			break;
		case 'b':
			PutbBishop(sq64to16x12(square));
			hash ^= GetZobKey(square,Black,Bishop);
			Opening += Pst[5][square][0];EndGame += Pst[5][square][1];
			square++;
			break;
		case 'B':
			PutwBishop(sq64to16x12(square));
			hash ^= GetZobKey(square,White,Bishop);
			Opening += Pst[4][square][0];EndGame += Pst[4][square][1];
			square++;
			break;
		case 'r':
			PutbRook(sq64to16x12(square));
			hash ^= GetZobKey(square,Black,Rook);
			Opening += Pst[7][square][0];EndGame += Pst[7][square][1];
			square++;
			break;
		case 'R':
			PutwRook(sq64to16x12(square));
			hash ^= GetZobKey(square,White,Rook);
			Opening += Pst[6][square][0];EndGame += Pst[6][square][1];
			square++;
			break;
		case 'q':
			PutbQueen(sq64to16x12(square));
			hash ^= GetZobKey(square,Black,Queen);
			Opening += Pst[9][square][0];EndGame += Pst[9][square][1];
			square++;
			break;
		case 'Q':
			PutwQueen(sq64to16x12(square));
			hash ^= GetZobKey(square,White,Queen);
			Opening += Pst[8][square][0];EndGame += Pst[8][square][1];
			square++;
			break;
		case 'k':
			PutbKing(sq64to16x12( square));
			hash ^= GetZobKey(square,Black,King);
			Opening += Pst[11][square][0];EndGame += Pst[11][square][1];
			square++;
			break;
		case 'K':
			PutwKing(sq64to16x12(square));
			hash ^= GetZobKey(square,White,King);
			Opening += Pst[10][square][0];EndGame += Pst[10][square][1];
			square++;
			break;
		}
		fen++;
	}
	fen++;
	// Field 2 
	if(*fen=='w')
		wtm = White;
	if(*fen=='b')
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
					CastleFlags |= FlagsBlackQueenCastle;
				break;
			case 'k':
				CastleFlags |= FlagsBlackKingCastle;
				break;
			case 'Q':
					CastleFlags |= FlagsWhiteQueenCastle;
				break;
			case 'K':
					CastleFlags |= FlagsWhiteKingCastle;
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
		EnPassant = sq64to16x12(EnPassant);
	}
	Initialize();
	InitAttack();
}


// funcion para llamar desde assert
// validamos que toda la informacion de las tablas es coherente.
bool Board::board_is_ok() 
{
   int sq, pieza;
   int i,j;
   int position;
   // EDGE Verification
   for(sq = 0; sq < 35;sq++)  // inferior board edge
   {
	   if(board[sq] != -1) 
		   return false;
   }
   for(j=0; j < 8;j++) // lateral edge.
   {
	   for(i=0; i < 8;i++)
	   {
		   sq = 44+j*16+i;
		   if(board[sq] != -1)
			   return false;
	   }
   }
   for(sq = 156; sq < BoardSize; sq++)  // upper edge
   {
	   if(board[sq] != -1)
		   return false;
   }
   // board structure is ok
   for (sq = 0; sq < BoardSize; sq++) {
		pieza = board[sq];
		position = pos[sq];
		switch(pieza)
		{
		case Edge:
			if(pos[sq] != -1)
				return false;
			break;
		case Empty:
			if(pos[sq] != -1)
				return false;
			break;
		case bPawn:
			if(pos[sq] == -1)	// must be >= 0
				return false;
			if(pos[sq] >= MAXPAWNS)
				return false;
			if(pos[sq] >= NumBPawn)
				return false;
			if(bPawnList[pos[sq]] != sq)
				return false;
			break;
		case bKnight:
			if(pos[sq] == -1)	// must be >= 0
				return false;
			if(pos[sq] >= MAXKNIGHTS)
				return false;
			if(pos[sq] >= NumBKnight)
				return false;
			if(bKnightList[pos[sq]] != sq)
				return false;
			break;
		case bBishop:
			if(pos[sq] == -1)	// must be >= 0
				return false;
			if(pos[sq] >= MAXBISHOPS)
				return false;
			if(pos[sq] >= NumBBishop)
				return false;
			if(bBishopList[pos[sq]] != sq)
				return false;
			break;
		case bRook:
			if(pos[sq] == -1)	// must be >= 0
				return false;
			if(pos[sq] >= MAXROOKS)
				return false;
			if(pos[sq] >= NumBRook)
				return false;
			if(bRookList[pos[sq]] != sq)
				return false;
			break;
		case bQueen:
			if(pos[sq] == -1)	// must be >= 0
				return false;
			if(pos[sq] >= MAXQUEENS)
				return false;
			if(pos[sq] >= NumBQueen)
				return false;
			if(bQueenList[pos[sq]] != sq)
				return false;
			break;
		case bKing:
			if(pos[sq] != 0)	// must be = 0
				return false;
			if(bKingPos != sq)
				return false;
			break;

		case wPawn:
			if(pos[sq] == -1)	// must be >= 0
				return false;
			if(pos[sq] >= MAXPAWNS)
				return false;
			if(pos[sq] >= NumWPawn)
				return false;
			if(wPawnList[pos[sq]] != sq)
				return false;
			break;
		case wKnight:
			if(pos[sq] == -1)	// must be >= 0
				return false;
			if(pos[sq] >= MAXKNIGHTS)
				return false;
			if(pos[sq] >= NumWKnight)
				return false;
			if(wKnightList[pos[sq]] != sq)
				return false;
			break;
		case wBishop:
			if(pos[sq] == -1)	// must be >= 0
				return false;
			if(pos[sq] >= MAXBISHOPS)
				return false;
			if(pos[sq] >= NumWBishop)
				return false;
			if(wBishopList[pos[sq]] != sq)
				return false;
			break;
		case wRook:
			if(pos[sq] == -1)	// must be >= 0
				return false;
			if(pos[sq] >= MAXROOKS)
				return false;
			if(pos[sq] >= NumWRook)
				return false;
			if(wRookList[pos[sq]] != sq)
				return false;
			break;
		case wQueen:
			if(pos[sq] == -1)	// must be >= 0
				return false;
			if(pos[sq] >= MAXQUEENS)
				return false;
			if(pos[sq] >= NumWQueen)
				return false;
			if(wQueenList[pos[sq]] != sq)
				return false;
			break;
		case wKing:
			if(pos[sq] != 0)	// must be = 0
				return false;
			if(wKingPos != sq)
				return false;
			break;
		}
   }

   // piecelist is ok
	for(i = 0; i < NumBPawn;i++)
	{
		sq = bPawnList[i];
		if(pos[sq] != i)
			return false;
		if(board[sq] != bPawn)
			return false;
	}
	for(i = 0; i < NumBKnight;i++)
	{
		sq = bKnightList[i];
		if(pos[sq] != i)
			return false;
		if(board[sq] != bKnight)
			return false;
	}
	for(i = 0; i < NumBBishop;i++)
	{
		sq = bBishopList[i];
		if(pos[sq] != i)
			return false;
		if(board[sq] != bBishop)
			return false;
	}
	for(i = 0; i < NumBRook;i++)
	{
		sq = bRookList[i];
		if(pos[sq] != i)
			return false;
		if(board[sq] != bRook)
			return false;
	}
	for(i = 0; i < NumBQueen;i++)
	{
		sq = bQueenList[i];
		if(pos[sq] != i)
			return false;
		if(board[sq] != bQueen)
			return false;
	}
	if(pos[bKingPos] != 0)
		return false;
	if(board[bKingPos] != bKing)
		return false;

	for(i = 0; i < NumWPawn;i++)
	{
		sq = wPawnList[i];
		if(pos[sq] != i)
			return false;
		if(board[sq] != wPawn)
			return false;
	}
	for(i = 0; i < NumWKnight;i++)
	{
		sq = wKnightList[i];
		if(pos[sq] != i)
			return false;
		if(board[sq] != wKnight)
			return false;
	}
	for(i = 0; i < NumWBishop;i++)
	{
		sq = wBishopList[i];
		if(pos[sq] != i)
			return false;
		if(board[sq] != wBishop)
			return false;
	}
	for(i = 0; i < NumWRook;i++)
	{
		sq = wRookList[i];
		if(pos[sq] != i)
			return false;
		if(board[sq] != wRook)
			return false;
	}
	for(i = 0; i < NumWQueen;i++)
	{
		sq = wQueenList[i];
		if(pos[sq] != i)
			return false;
		if(board[sq] != wQueen)
			return false;
	}
	if(pos[wKingPos] != 0)
		return false;
	if(board[wKingPos] != wKing)
		return false;

	// misc
   if(wtm != White && wtm != Black)
	   return false;
   return true;
}
void Board::Clear() 
{
   int sq, sq_64;

   // edge squares

   for (sq = 0; sq < BoardSize; sq++) {
      board[sq] = Edge;
   }

   // empty squares

   for (sq_64 = 0; sq_64 < 64; sq_64++) {
      sq =  sq64to16x12(sq_64);
      board[sq] = Empty;
   }

   // misc

   wtm = 2; // HACK board is not OK
   CastleFlags = CastleNone;
   EnPassant = 0;
   PlyNumber = 0;
}
int Board::piece_to_12(int x)
{
	switch(x)
	{
	case wPawn:
		return 0;
	case bPawn:
		return 1;
	case wKnight:
		return 2;
	case bKnight:
		return 3;
	case wBishop:
		return 4;
	case bBishop:
		return 5;
	case wRook:
		return 6;
	case bRook:
		return 7;
	case wQueen:
		return 8;
	case bQueen:
		return 9;
	case wKing:
		return 10;
	case bKing:
		return 11;
	}
	return -1; // HACK forzamos un error para depurar
}
void Board::Initialize()
{
   int sq_64, sq;

   // piece lists
   for(sq_64 = 0; sq_64 < 64;sq_64++)
   {
	   sq = sq64to16x12(sq_64);
	   switch(board[sq])
	   {
	   case Edge:
		   ASSERT(false);
		   break;
	   case Empty:
		   break;
	   case bPawn:
		   pos[sq] = NumBPawn;
		   bPawnList[NumBPawn] = sq;
		   NumBPawn++;
		   break;
	   case bKnight:
		   pos[sq] = NumBKnight;
		   bKnightList[NumBKnight] = sq;
		   NumBKnight++;
		   break;
	   case bBishop:
		   pos[sq] = NumBBishop;
		   bBishopList[NumBBishop] = sq;
		   NumBBishop++;
		   break;
	   case bRook:
		   pos[sq] = NumBRook;
		   bRookList[NumBRook] = sq;
		   NumBRook++;
		   break;
	   case bQueen:
		   pos[sq] = NumBQueen;
		   bQueenList[NumBQueen] = sq;
		   NumBQueen++;
		   break;
	   case bKing:
		   pos[sq] = 0;
		   bKingPos = sq;
		   break;
	   case wPawn:
		   pos[sq] = NumWPawn;
		   wPawnList[NumWPawn] = sq;
		   NumWPawn++;
		   break;
	   case wKnight:
		   pos[sq] = NumWKnight;
		   wKnightList[NumWKnight] = sq;
		   NumWKnight++;
		   break;
	   case wBishop:
		   pos[sq] = NumWBishop;
		   wBishopList[NumWBishop] = sq;
		   NumWBishop++;
		   break;
	   case wRook:
		   pos[sq] = NumWRook;
		   wRookList[NumWRook] = sq;
		   NumWRook++;
		   break;
	   case wQueen:
		   pos[sq] = NumWQueen;
		   wQueenList[NumWQueen] = sq;
		   NumWQueen++;
		   break;
	   case wKing:
		   pos[sq] = 0;
		   wKingPos = sq;
		   break;
	   }
   }

   ////// debug

   ASSERT(board_is_ok());
}

void Board::GenPseudoMoves(MoveList &List)
{
	List.Clear();
	GenAllMoves(List);
	GenEnPassant(List);
	GenCastleMove(List);
}
void Board::GenCaptures(MoveList &List)
{
	List.Clear();

	GenRegularCaptures(List);
	GenEnPassant(List);
}
void Board::GenAllMoves(MoveList &List)
{
   int me, opp;
   int from, to;
   int capture;
   const int * inc_ptr;
   int inc;
   int i;

   me = wtm;
   opp = ColorOpp(me);

   // piece moves
   if(wtm == White)
   {
	   // Knight
	   for(i=0; i < NumWKnight;i++)
	   {
		   from = wKnightList[i];
		   ASSERT(board[from] == wKnight);
         for (inc_ptr = &KnightDir[0]; (inc=*inc_ptr) != Empty; inc_ptr++) {
            to = from + inc;
            capture = board[to];
            if ((capture == Empty || (capture&3) == opp)) {
				List.Add(MakeMove(from,to));
            }
         }
	   }
	   // Bishop
	   for(i=0; i < NumWBishop;i++)
	   {
			from = wBishopList[i];
			ASSERT(board[from] == wBishop);
			for (inc_ptr = &BishopDir[0]; (inc=*inc_ptr) != Empty; inc_ptr++) {
				for (to = from+inc; (capture=board[to]) == Empty; to += inc) {
					List.Add(MakeMove(from,to));
				}
				if ((capture&3) == opp) List.Add(MakeMove(from,to));
			}
	   }
	   // Rook
	   for(i=0; i < NumWRook;i++)
	   {
			from = wRookList[i];
			ASSERT(board[from] == wRook);
			  for (inc_ptr = &RookDir[0]; (inc=*inc_ptr) != Empty; inc_ptr++) {
				 for (to = from+inc; (capture=board[to]) == Empty; to += inc) {
					List.Add(MakeMove(from,to));
				 }
				 if ((capture&3) == opp) List.Add(MakeMove(from,to));
			  }
	   }
	   // Queen
	   for(i=0; i < NumWQueen;i++)
	   {
			from = wQueenList[i];
			ASSERT(board[from] == wQueen);
			for (inc_ptr = &QueenDir[0]; (inc=*inc_ptr) != Empty; inc_ptr++) {
				for (to = from+inc; (capture=board[to]) == Empty; to += inc) {
				List.Add(MakeMove(from,to));
				}
				if ((capture&3) == opp) List.Add(MakeMove(from,to));
			}
	   }
		// King
		from = wKingPos;
		ASSERT(board[from] == wKing);
		for (inc_ptr = &KingDir[0]; (inc=*inc_ptr) != Empty; inc_ptr++) {
			to = from + inc;
			capture = board[to];
			if (capture == Empty || (capture&3) == opp) {
			List.Add(MakeMove(from,to));
			}
		}
		// Pawn
	   for(i=0; i < NumWPawn;i++)
	   {
			from = wPawnList[i];
			ASSERT(board[from] == wPawn);
			to = from + 15;
			if ((board[to]&3) == opp) GenOnePawnMove(List,from,to);

			to = from + 17;
			if ((board[to]&3 ) ==opp) GenOnePawnMove(List,from,to);

			to = from + 16;
			if (board[to] == Empty) {
				GenOnePawnMove(List,from,to);
				if (RankSq16x12(from) == Rank2)
				{
					to = from + 32;
					if (board[to] == Empty)
					{
						List.Add(MakeMoveSp(from,to,MoveDoublePush));
					}
				}
			}
		}

   }
   else
   {
	   // Knight
	   for(i=0; i < NumBKnight;i++)
	   {
		   from = bKnightList[i];
		   ASSERT(board[from] == bKnight);
         for (inc_ptr = &KnightDir[0]; (inc=*inc_ptr) != Empty; inc_ptr++) {
            to = from + inc;
            capture = board[to];
            if ((capture == Empty || (capture&3) == opp)) {
				List.Add(MakeMove(from,to));
            }
         }
	   }
	   // Bishop
	   for(i=0; i < NumBBishop;i++)
	   {
			from = bBishopList[i];
			ASSERT(board[from] == bBishop);
			for (inc_ptr = &BishopDir[0]; (inc=*inc_ptr) != Empty; inc_ptr++) {
				for (to = from+inc; (capture=board[to]) == Empty; to += inc) {
					List.Add(MakeMove(from,to));
				}
				if ((capture&3) == opp) List.Add(MakeMove(from,to));
			}
	   }
	   // Rook
	   for(i=0; i < NumBRook;i++)
	   {
			from = bRookList[i];
			ASSERT(board[from] == bRook);
			  for (inc_ptr = &RookDir[0]; (inc=*inc_ptr) != Empty; inc_ptr++) {
				 for (to = from+inc; (capture=board[to]) == Empty; to += inc) {
					List.Add(MakeMove(from,to));
				 }
				 if ((capture&3) == opp) List.Add(MakeMove(from,to));
			  }
	   }
	   for(i=0; i < NumBQueen;i++)
	   {
			from = bQueenList[i];
			ASSERT(board[from] == bQueen);
			for (inc_ptr = &QueenDir[0]; (inc=*inc_ptr) != Empty; inc_ptr++) {
				for (to = from+inc; (capture=board[to]) == Empty; to += inc) {
				List.Add(MakeMove(from,to));
				}
				if ((capture&3) == opp) List.Add(MakeMove(from,to));
			}
	   }
		// King
		from = bKingPos;
		ASSERT(board[from] == bKing);
		for (inc_ptr = &KingDir[0]; (inc=*inc_ptr) != Empty; inc_ptr++) {
			to = from + inc;
			capture = board[to];
			if (capture == Empty || (capture&3) == opp) {
			List.Add(MakeMove(from,to));
			}
		}
		// Pawn
	   for(i=0; i < NumBPawn;i++)
	   {
			from = bPawnList[i];
			ASSERT(board[from] == bPawn);
			to = from - 15;
			if ((board[to]&3 ) == opp) GenOnePawnMove(List,from,to);

			to = from - 17;
			if ((board[to]&3) == opp) GenOnePawnMove(List,from,to);

			to = from - 16;
			if (board[to] == Empty) {
				GenOnePawnMove(List,from,to);
				if (RankSq16x12(from) == Rank7)
				{
					to = from - 32;
					if (board[to] == Empty)
					{
						List.Add(MakeMoveSp(from,to,MoveDoublePush));
					}
				}
			}
		}
   }

}

void Board::GenOnePawnMove(MoveList &List, int from, int to) 
{
   int move;
   ASSERT(square_is_ok(from));
   ASSERT(square_is_ok(to));

   move = MakeMove(from,to);

   if (IsPromoteSquare(to)) {
	   GenPromote(List,move);
   } else {
      List.Add(move);
   }
}
void Board::GenRegularCaptures(MoveList &List)
{
   int me, opp;
   int from, to;
   int capture;
   const int * inc_ptr;
   int inc;
   int i;

   me = wtm;
   opp = ColorOpp(me);

   // piece moves
   if(wtm == White)
   {
	   // Knight
	   for(i=0; i < NumWKnight;i++)
	   {
		   from = wKnightList[i];
		   ASSERT(board[from] == wKnight);
         for (inc_ptr = &KnightDir[0]; (inc=*inc_ptr) != Empty; inc_ptr++) {
            to = from + inc;
            capture = board[to];
            if ((capture&3) == opp) {
				List.Add(MakeMove(from,to));
            }
         }
	   }
	   // Bishop
	   for(i=0; i < NumWBishop;i++)
	   {
			from = wBishopList[i];
			ASSERT(board[from] == wBishop);
			for (inc_ptr = &BishopDir[0]; (inc=*inc_ptr) != Empty; inc_ptr++) 
			{
				for (to = from+inc; (capture=board[to]) == Empty; to += inc) ;
				if ((capture&3) == opp) List.Add(MakeMove(from,to));
			}
	   }
	   // Rook
	   for(i=0; i < NumWRook;i++)
	   {
			from = wRookList[i];
			ASSERT(board[from] == wRook);
			  for (inc_ptr = &RookDir[0]; (inc=*inc_ptr) != Empty; inc_ptr++) 
			  {
				 for (to = from+inc; (capture=board[to]) == Empty; to += inc) ;
				 if ((capture&3) == opp) List.Add(MakeMove(from,to));
			  }
	   }
	   // Queen
	   for(i=0; i < NumWQueen;i++)
	   {
			from = wQueenList[i];
			ASSERT(board[from] == wQueen);
			for (inc_ptr = &QueenDir[0]; (inc=*inc_ptr) != Empty; inc_ptr++) 
			{
				for (to = from+inc; (capture=board[to]) == Empty; to += inc);
				if ((capture&3) == opp) List.Add(MakeMove(from,to));
			}
	   }
		// King
		from = wKingPos;
		ASSERT(board[from] == wKing);
		for (inc_ptr = &KingDir[0]; (inc=*inc_ptr) != Empty; inc_ptr++) {
			to = from + inc;
			capture = board[to];
			if ((capture&3) == opp) {
			List.Add(MakeMove(from,to));
			}
		}
		// Pawn
	   for(i=0; i < NumWPawn;i++)
	   {
			from = wPawnList[i];
			ASSERT(board[from] == wPawn);
			to = from + 15;
			if ((board[to]&3) == opp) GenOnePawnMove(List,from,to);

			to = from + 17;
			if ((board[to]&3 ) ==opp) GenOnePawnMove(List,from,to);
		}

   }
   else
   {
	   // Knight
	   for(i=0; i < NumBKnight;i++)
	   {
		   from = bKnightList[i];
		   ASSERT(board[from] == bKnight);
         for (inc_ptr = &KnightDir[0]; (inc=*inc_ptr) != Empty; inc_ptr++) {
            to = from + inc;
            capture = board[to];
            if ((capture&3) == opp)
			{
				List.Add(MakeMove(from,to));
            }
         }
	   }
	   // Bishop
	   for(i=0; i < NumBBishop;i++)
	   {
			from = bBishopList[i];
			ASSERT(board[from] == bBishop);
			for (inc_ptr = &BishopDir[0]; (inc=*inc_ptr) != Empty; inc_ptr++) 
			{
				for (to = from+inc; (capture=board[to]) == Empty; to += inc) ;
				if ((capture&3) == opp) List.Add(MakeMove(from,to));
			}
	   }
	   // Rook
	   for(i=0; i < NumBRook;i++)
	   {
			from = bRookList[i];
			ASSERT(board[from] == bRook);
			  for (inc_ptr = &RookDir[0]; (inc=*inc_ptr) != Empty; inc_ptr++) 
			  {
				 for (to = from+inc; (capture=board[to]) == Empty; to += inc) ;
				 if ((capture&3) == opp) List.Add(MakeMove(from,to));
			  }
	   }
	   for(i=0; i < NumBQueen;i++)
	   {
			from = bQueenList[i];
			ASSERT(board[from] == bQueen);
			for (inc_ptr = &QueenDir[0]; (inc=*inc_ptr) != Empty; inc_ptr++) 
			{
				for (to = from+inc; (capture=board[to]) == Empty; to += inc) ;
				if ((capture&3) == opp) List.Add(MakeMove(from,to));
			}
	   }
		// King
		from = bKingPos;
		ASSERT(board[from] == bKing);
		for (inc_ptr = &KingDir[0]; (inc=*inc_ptr) != Empty; inc_ptr++) 
		{
			to = from + inc;
			capture = board[to];
			if ((capture&3) == opp) {
			List.Add(MakeMove(from,to));
			}
		}
		// Pawn
	   for(i=0; i < NumBPawn;i++)
	   {
			from = bPawnList[i];
			ASSERT(board[from] == bPawn);
			to = from - 15;
			if ((board[to]&3 ) == opp) GenOnePawnMove(List,from,to);

			to = from - 17;
			if ((board[to]&3) == opp) GenOnePawnMove(List,from,to);
		}
   }
}
void Board::GenPromote(MoveList &List,int move)
{
	ASSERT(IsPromoteSquare(MoveTo(move)));
	List.Add(move|MovePromoteQueen);
	List.Add(move|MovePromoteKnight);
	List.Add(move|MovePromoteRook);
	List.Add(move|MovePromoteBishop);
}

void Board::GenEnPassant(MoveList &List)
{
   int from, to;

   to = EnPassant;
   if (to == Empty) return;

   if (wtm == White) {

      from = to - 17;
      if (board[from] == wPawn) {
         ASSERT(!IsPromote(to));
		 List.Add(MakeMoveSp(from,to,MoveEnPassant));
      }

      from = to - 15;
      if (board[from] == wPawn) {
         ASSERT(!IsPromote(to));
		 List.Add(MakeMoveSp(from,to,MoveEnPassant));
      }

   } else { // black

      from = to + 15;
      if (board[from] == bPawn) {
         ASSERT(!IsPromote(to));
		 List.Add(MakeMoveSp(from,to,MoveEnPassant));
      }

      from = to + 17;
      if (board[from] == bPawn) {
         ASSERT(!IsPromote(to));
		 List.Add(MakeMoveSp(from,to,MoveEnPassant));
      }
   }
}
void Board::GenCastleMove(MoveList &List)
{
	if(IsInCheck(wtm)) return;

   if (wtm == White) {
      if ((CastleFlags & WhiteKingCastle) != 0
       && board[F1] == Empty
       && board[G1] == Empty
	   && board[E1] == wKing
	   && board[H1] == wRook
       && !IsAttacked(F1,Black)) {
		 List.Add(MakeMoveSp(E1,G1,MoveCastle));
      }

      if ((CastleFlags & WhiteQueenCastle) != 0
       && board[D1] == Empty
       && board[C1] == Empty
       && board[B1] == Empty
	   && board[E1] == wKing
	   && board[A1] == wRook
       && !IsAttacked(D1,Black)) {
		 List.Add(MakeMoveSp(E1,C1,MoveCastle));
      }

   } else { // black

      if ((CastleFlags & BlackKingCastle) != 0
       && board[F8] == Empty
       && board[G8] == Empty
	   && board[E8] == bKing
	   && board[H8] == bRook
       && !IsAttacked(F8,White)) {
		 List.Add(MakeMoveSp(E8,G8,MoveCastle));
      }

	  if ((CastleFlags & BlackQueenCastle) != 0
       && board[D8] == Empty
       && board[C8] == Empty
       && board[B8] == Empty
	   && board[E8] == bKing
	   && board[A8] == bRook
       && !IsAttacked(D8,White)) {
		 List.Add(MakeMoveSp(E8,C8,MoveCastle));
      }
   }
}

bool Board::IsAttacked(int to, int colour) {

   int from;
   int delta;
   int inc, sq;
   int i;
   // pawn attack

   if (colour== White) {
      if (board[to-17] == wPawn) return true;
      if (board[to-15] == wPawn) return true;
	  // Knights
	  for(i=0; i < NumWKnight;i++)
	  {
		  from = wKnightList[i];
		  delta = to - from;
		  ASSERT(DeltaIsOk(delta));

		  if (CanAttack(wKnight,delta)) {
			  return true;
		  }
	  }
	  // Bishops
	  for(i=0; i < NumWBishop;i++)
	  {
		  from = wBishopList[i];
		  delta = to - from;
		  ASSERT(DeltaIsOk(delta));

		  if (CanAttack(wBishop,delta)) {

			 inc = AttackDir(delta);
			 ASSERT(inc!=0);

			 for (sq = from+inc; sq != to; sq += inc) {
				ASSERT(square_is_ok(sq));
				if (board[sq] != Empty) goto nextWB; // blocker
			 }
			 return true;
		  }
nextWB:;
	  }
	  // Rooks
	  for(i=0; i < NumWRook;i++)
	  {
		  from = wRookList[i];
		  delta = to - from;
		  ASSERT(DeltaIsOk(delta));

		  if (CanAttack(wRook,delta)) {

			 inc = AttackDir(delta);
			 ASSERT(inc!=0);

			 for (sq = from+inc; sq != to; sq += inc) {
				ASSERT(square_is_ok(sq));
				if (board[sq] != Empty) goto nextWR; // blocker
			 }
			 return true;
		  }
nextWR:;
	  }
	  // Queen
	  for(i=0; i < NumWQueen;i++)
	  {
		  from = wQueenList[i];
		  delta = to - from;
		  ASSERT(DeltaIsOk(delta));

		  if (CanAttack(wQueen,delta)) {

			 inc = AttackDir(delta);
			 ASSERT(inc!=0);

			 for (sq = from+inc; sq != to; sq += inc) {
				ASSERT(square_is_ok(sq));
				if (board[sq] != Empty) goto nextWQ; // blocker
			 }
			 return true;
		  }
nextWQ:;
	  }
	  	  // King
		from = wKingPos;
		delta = to - from;
		ASSERT(DeltaIsOk(delta));

		if (CanAttack(wKing,delta)) {
			return true;
		}

   } else { // black
      if (board[to+15] == bPawn) return true;
      if (board[to+17] == bPawn) return true;
	  // Knights
	  for(i=0; i < NumBKnight;i++)
	  {
		  from = bKnightList[i];
		  delta = to - from;
		  ASSERT(DeltaIsOk(delta));

		  if (CanAttack(bKnight,delta)) {
			  return true;
		  }
	  }
	  // Bishops
	  for(i=0; i < NumBBishop;i++)
	  {
		  from = bBishopList[i];
		  delta = to - from;
		  ASSERT(DeltaIsOk(delta));

		  if (CanAttack(bBishop,delta)) {

			 inc = AttackDir(delta);
			 ASSERT(inc!=0);

			 for (sq = from+inc; sq != to; sq += inc) {
				ASSERT(square_is_ok(sq));
				if (board[sq] != Empty) goto nextBB; // blocker
			 }
			 return true;
		  }
nextBB:;
	  }
	  // Rooks
	  for(i=0; i < NumBRook;i++)
	  {
		  from = bRookList[i];
		  delta = to - from;
		  ASSERT(DeltaIsOk(delta));

		  if (CanAttack(bRook,delta)) {

			 inc = AttackDir(delta);
			 ASSERT(inc!=0);

			 for (sq = from+inc; sq != to; sq += inc) {
				ASSERT(square_is_ok(sq));
				if (board[sq] != Empty) goto nextBR; // blocker
			 }
			 return true;
		  }
nextBR:;
	  }
	  // Queen
	  for(i=0; i < NumBQueen;i++)
	  {
		  from = bQueenList[i];
		  delta = to - from;
		  ASSERT(DeltaIsOk(delta));

		  if (CanAttack(bQueen,delta)) {

			 inc = AttackDir(delta);
			 ASSERT(inc!=0);

			 for (sq = from+inc; sq != to; sq += inc) {
				ASSERT(square_is_ok(sq));
				if (board[sq] != Empty) goto nextBQ; // blocker
			 }
			 return true;
		  }
nextBQ:;
	  }
	  // King
		from = bKingPos;
		delta = to - from;
		ASSERT(DeltaIsOk(delta));

		if (CanAttack(bKing,delta)) {
			return true;
		}
   }

   return false;
}

// TODO move to a preinitialized constant array
void Board::InitAttack()
{
   int delta;
   int dir, inc, dist;

   for (delta = -OFFSETDELTA; delta < +OFFSETDELTA; delta++) { 
      AttackDirection[OFFSETDELTA+delta] = 0;
      AttackChessman[OFFSETDELTA+delta] = 0;
   }

   AttackChessman[OFFSETDELTA-17] |= bPawn;
   AttackChessman[OFFSETDELTA-15] |= bPawn;

   AttackChessman[OFFSETDELTA+15] |= wPawn;
   AttackChessman[OFFSETDELTA+17] |= wPawn;

   for (dir = 0; dir < 8; dir++) {
      delta = KnightDir[dir];
      ASSERT(DeltaIsOk(delta));
      ASSERT(AttackDirection[OFFSETDELTA+delta]==0);
      AttackDirection[OFFSETDELTA+delta] = delta;
      AttackChessman[OFFSETDELTA+delta] |= Knight;
   }

   for (dir = 0; dir < 4; dir++) {
      inc = BishopDir[dir];
      ASSERT(inc!=0);
      for (dist = 1; dist < 8; dist++) {
         delta = inc*dist;
         ASSERT(DeltaIsOk(delta));
         ASSERT(AttackDirection[OFFSETDELTA+delta]==0);
         AttackDirection[OFFSETDELTA+delta] = inc;
         AttackChessman[OFFSETDELTA+delta] |= Bishop;
         AttackChessman[OFFSETDELTA+delta] |= Queen;
      }
   }

   for (dir = 0; dir < 4; dir++) {
      inc = RookDir[dir];
      ASSERT(inc!=0);
      for (dist = 1; dist < 8; dist++) {
         delta = inc*dist;
         ASSERT(DeltaIsOk(delta));
         ASSERT(AttackDirection[OFFSETDELTA+delta]==0);
         AttackDirection[OFFSETDELTA+delta] = inc;
         AttackChessman[OFFSETDELTA+delta] |= Rook;
         AttackChessman[OFFSETDELTA+delta] |= Queen;
      }
   }

   for (dir = 0; dir < 8; dir++) {
      delta = KingDir[dir];
      ASSERT(DeltaIsOk(delta));
      AttackChessman[OFFSETDELTA+delta] |= King;
   }
}

void Board::ClearSquare(int square)
{
   int position, pieza, colour;
   int sq;
   int i, size;

   ASSERT(square_is_ok(square));

   // init

   position = pos[square];
   ASSERT(position>=0);

   pieza = board[square];
   colour = piece_colour(pieza); 

   // square

   board[square] = Empty;

   pos[square] = -1;
   
   sq =  sq16x12to64(square);

   int *pArray;
   // piece list
   switch(pieza)
   {
   case bPawn:
	   pArray = &bPawnList[0];
	   size = --NumBPawn;
	   Opening -= Pst[1][sq][0];EndGame -= Pst[1][sq][1];
	    hash ^= GetZobKey(sq,Black,Pawn);
	   break;
   case bKnight:
	   pArray = &bKnightList[0];
	   size = --NumBKnight;
	   Opening -= Pst[3][sq][0];EndGame -= Pst[3][sq][1];
	    hash ^= GetZobKey(sq,Black,Knight);
	   break;
   case bBishop:
	   pArray = &bBishopList[0];
		size = --NumBBishop;
		Opening -= Pst[5][sq][0];EndGame -= Pst[5][sq][1];
	    hash ^= GetZobKey(sq,Black,Bishop);
	   break;
   case bRook:
	   pArray = &bRookList[0];
	   size = --NumBRook;
		Opening -= Pst[7][sq][0];EndGame -= Pst[7][sq][1];
	    hash ^= GetZobKey(sq,Black,Rook);
	   break;
   case bQueen:
	   pArray = &bQueenList[0];
	   size = --NumBQueen;
		Opening -= Pst[9][sq][0];EndGame -= Pst[9][sq][1];
	    hash ^= GetZobKey(sq,Black,Queen);
	   break;
   case bKing:
	   ASSERT(false); // not legal move
	   break;
   case wPawn:
	   pArray = &wPawnList[0];
	   size = --NumWPawn;
		Opening -= Pst[0][sq][0];EndGame -= Pst[0][sq][1];
	    hash ^= GetZobKey(sq,White,Pawn);
	   break;
   case wKnight:
	   pArray = &wKnightList[0];
	   size = --NumWKnight;
	   Opening -= Pst[2][sq][0];EndGame -= Pst[2][sq][1];
	    hash ^= GetZobKey(sq,White,Knight);
	   break;
   case wBishop:
	   pArray = &wBishopList[0];
	   size = --NumWBishop;
		Opening -= Pst[4][sq][0];EndGame -= Pst[4][sq][1];
	    hash ^= GetZobKey(sq,White,Bishop);
	   break;
   case wRook:
	   pArray = &wRookList[0];
	   size = --NumWRook;
		Opening -= Pst[6][sq][0];EndGame -= Pst[6][sq][1];
	    hash ^= GetZobKey(sq,White,Rook);
	   break;
   case wQueen:
	   pArray = &wQueenList[0];
	   size = --NumWQueen;
		Opening -= Pst[8][sq][0];EndGame -= Pst[8][sq][1];
	    hash ^= GetZobKey(sq,White,Queen);
	   break;
   case wKing:
	   ASSERT(false); // not legal move
	   break;
   default:
	   ASSERT(false);
	   break;
   }
	ASSERT(size>=0);
	ASSERT(position>=0&&position<=size);

	// shift array
    for (i = position; i < size; i++) {

        sq = pArray[i+1];

        pArray[i] = sq;

        ASSERT(pos[sq]==i+1);
        pos[sq] = i;
    }

    pArray[size] = Empty;

}
void Board::PutSquare(int square,int pieza)
{
	ASSERT(square_is_ok(square));

	// square
	board[square] = pieza;
	int sq64 = sq16x12to64(square);
	// piece List.
   switch(pieza)
   {
   case bPawn:
	   pos[square] = NumBPawn;
	   bPawnList[NumBPawn++] = square;
	   	hash ^= GetZobKey(sq64,Black,Pawn);
		Opening += Pst[1][sq64][0];EndGame += Pst[1][sq64][1];
	   break;
   case bKnight:
	   pos[square] = NumBKnight;
	   bKnightList[NumBKnight++] = square;
	   	hash ^= GetZobKey(sq64,Black,Knight);
		Opening += Pst[3][sq64][0];EndGame += Pst[3][sq64][1];
	   break;
   case bBishop:
	   pos[square] = NumBBishop;
	   bBishopList[NumBBishop++] = square;
	   	hash ^= GetZobKey(sq64,Black,Bishop);
		Opening += Pst[5][sq64][0];EndGame += Pst[5][sq64][1];
	   break;
   case bRook:
	   pos[square] = NumBRook;
	   bRookList[NumBRook++] = square;
	   	hash ^= GetZobKey(sq64,Black,Rook);
		Opening += Pst[7][sq64][0];EndGame += Pst[7][sq64][1];
	   break;
   case bQueen:
	   pos[square] = NumBQueen;
	   bQueenList[NumBQueen++] = square;
	   	hash ^= GetZobKey(sq64,Black,Queen);
		Opening += Pst[9][sq64][0];EndGame += Pst[9][sq64][1];
	   break;
   case bKing:
	   ASSERT(false); // not legal move
	   break;
   case wPawn:
	   pos[square] = NumWPawn;
	   wPawnList[NumWPawn++] = square; 
	   	hash ^= GetZobKey(sq64,White,Pawn);
		Opening += Pst[0][sq64][0];EndGame += Pst[0][sq64][1];
	   break;
   case wKnight:
	   pos[square] = NumWKnight;
	   wKnightList[NumWKnight++] = square;
	   	hash ^= GetZobKey(sq64,White,Knight);
		Opening += Pst[2][sq64][0];EndGame += Pst[2][sq64][1];
	   break;
   case wBishop:
	   pos[square] = NumWBishop;
	   wBishopList[NumWBishop++] = square;
	   	hash ^= GetZobKey(sq64,White,Bishop);
		Opening += Pst[4][sq64][0];EndGame += Pst[4][sq64][1];
	   break;
   case wRook:
	   pos[square] = NumWRook;
	   wRookList[NumWRook++] = square;
	   	hash ^= GetZobKey(sq64,White,Rook);
		Opening += Pst[6][sq64][0];EndGame += Pst[6][sq64][1];
	   break;
   case wQueen:
	   pos[square] = NumWQueen;
	   wQueenList[NumWQueen++] = square;
	   	hash ^= GetZobKey(sq64,White,Queen);
		Opening += Pst[8][sq64][0];EndGame += Pst[8][sq64][1];
	   break;
   case wKing:
	   ASSERT(false); // not legal move
	   break;
   }

}
// ClearSquare + PutSquare combinados no hay que mover los indices.
void Board::MoveSquare(int from,int to,int pieza)
{
   int colour;
   int position;

   ASSERT(square_is_ok(from));
   ASSERT(square_is_ok(to));

   // init

   colour = piece_colour(pieza);

   position = pos[from];
   ASSERT(position>=0);

   // from

   ASSERT(board[from]==pieza);
   board[from] = Empty;

   ASSERT(pos[from]==position);
   pos[from] = -1; // not needed

   // to

   ASSERT(board[to]==Empty);
   board[to] = pieza;

   ASSERT(pos[to]==-1);
   pos[to] = position;

   // piece list

   switch(pieza)
   {
   case bPawn:
	   ASSERT(bPawnList[position]==from);
	   bPawnList[position] = to;
	   break;
   case bKnight:
	   ASSERT(bKnightList[position]==from);
	   bKnightList[position] = to;
	   break;
   case bBishop:
	   ASSERT(bBishopList[position]==from);
	   bBishopList[position] = to;
	   break;
   case bRook:
	   ASSERT(bRookList[position]==from);
	   bRookList[position] = to;
	   break;
   case bQueen:
	   ASSERT(bQueenList[position]==from);
	   bQueenList[position] = to;
	   break;
   case bKing:
	   ASSERT(bKingPos==from);
	   bKingPos = to;
	   break;
   case wPawn:
	   ASSERT(wPawnList[position]==from);
	   wPawnList[position] = to;
	   break;
   case wKnight:
	   ASSERT(wKnightList[position]==from);
	   wKnightList[position] = to;
	   break;
   case wBishop:
	   ASSERT(wBishopList[position]==from);
	   wBishopList[position] = to;
	   break;
   case wRook:
	   ASSERT(wRookList[position]==from);
	   wRookList[position] = to;
	   break;
   case wQueen:
	   ASSERT(wQueenList[position]==from);
	   wQueenList[position] = to;
	   break;
   case wKing:
	   ASSERT(wKingPos==from);
	   wKingPos = to;
	   break;
   }
   int chessman = piece_to_12(pieza);
   int f = sq16x12to64(from);
   int t = sq16x12to64(to);

   	hash ^= GetZobKey(f,colour,pieza);
   	hash ^= GetZobKey(t,colour,pieza);

	Opening -= Pst[chessman][f][0];EndGame -= Pst[chessman][f][1];
	Opening += Pst[chessman][t][0];EndGame += Pst[chessman][t][1];
}

void Board::CastleMask_init() 
{

   int sq;

   for (sq = 0; sq < BoardSize; sq++) CastleMask[sq] = 0xF;

   CastleMask[E1] &= ~WhiteKingCastle;
   CastleMask[H1] &= ~WhiteKingCastle;

   CastleMask[E1] &= ~WhiteQueenCastle;
   CastleMask[A1] &= ~WhiteQueenCastle;

   CastleMask[E8] &= ~BlackKingCastle;
   CastleMask[H8] &= ~BlackKingCastle;

   CastleMask[E8] &= ~BlackQueenCastle;
   CastleMask[A8] &= ~BlackQueenCastle;
}

int Board::IsKCapture(int move)
{
   int to,capture;
   to = MoveTo(move);
   if ((capture=board[to]) != Empty) {
      return (PieceUncolored(capture) == King);
   }
   return 0;
}
int Board::IsKingCapture(int move)
{
	int capture;
   int to = MoveTo(move);
   if ((capture=board[to]) != Empty) {
	   if(PieceUncolored(capture) == King) return 1;
   }
   return 0;
}
void Board::DoMove(int move, UndoData &undo)
{

   int me, opp;
   int from, to;
   int pieza, capture;
   int old_flags, new_flags;
   int sq;
   int pawn, rook;

   // initialise undo

   undo.capture = false;

   undo.wtm = wtm;
   undo.CastleFlags = CastleFlags;
   undo.EnPassant = EnPassant;
   undo.PlyNumber = PlyNumber;
   undo.IsPromote = false;
   // init

   me = wtm;
   opp = ColorOpp(me);

   from = MoveFrom(move);
   to = MoveTo(move);

   pieza = board[from];
   ASSERT(piece_colour(pieza) == me);

    // update turn

   wtm = opp;

   // update castling rights

   old_flags = CastleFlags;
   new_flags = old_flags & CastleMask[from] & CastleMask[to];

   CastleFlags = new_flags;
   // update en-passant square

   if ((sq=EnPassant) != Empty) {
      EnPassant = Empty;
   }

   if (MoveIsDoblePush(move)) {
      pawn = Pawn |opp;
      if (board[to-1] == pawn || board[to+1] == pawn) {
         EnPassant = sq = (from + to) / 2;
      }
   }

   // update move number (captures are handled later)

   PlyNumber++;
   if (piece_is_pawn(pieza)) PlyNumber = 0; // 50 rule

   // remove the captured piece

   sq = to;
   if (MoveIsEnpassant(move))
	   sq = SquareEnPassant(sq);

   if ((capture=board[sq]) != Empty) {

      ASSERT(piece_colour(capture) == opp);
      ASSERT(PieceUncolored(capture) != King);

	  undo.capture= true;
      undo.capture_square = sq;
      undo.capture_piece = capture;
      undo.capture_pos = pos[sq];

      ClearSquare(sq);

      PlyNumber = 0; // reset 50 moves counter
   }

   // move the piece

   if (IsPromote(move)) {

      // promote

	  undo.IsPromote = true;

      ClearSquare(from);

      pieza = PromotePiece(move) +me; 

	  PutSquare(to,pieza);

   } else {

      // normal move
	MoveSquare(from,to,pieza);
   }

   // move the rook in case of castling

   if (IsCastle(move)) {

      rook = Rook + me; 

      if (to == G1) {
         MoveSquare(H1,F1,rook);
      } else if (to == C1) {
		  MoveSquare(A1,D1,rook);
      } else if (to == G8) {
		  MoveSquare(H8,F8,rook);
      } else if (to == C8) {
		  MoveSquare(A8,D8,rook);
      } else {
         ASSERT(false);
      }
   }

   // debug

   ASSERT(board_is_ok());
}

void Board::UndoMove(int move, UndoData &undo)
{
   int me;
   int from, to;
   int pieza, position;
   int rook;

   // init
   me = wtm;

   from = MoveFrom(move);
   to = MoveTo(move);

   pieza = board[to];
   ASSERT(piece_colour(pieza) == undo.wtm);

   // castle

   if (IsCastle(move)) {

      rook = Rook +undo.wtm; // Add Color to rook

      if (to == G1) {
		  MoveSquare(F1,H1,rook);
      } else if (to == C1) {
		  MoveSquare(D1,A1,rook);
      } else if (to == G8) {
		  MoveSquare(F8,H8,rook);
      } else if (to == C8) {
		  MoveSquare(D8,A8,rook);
      } else {
         ASSERT(false);
      }
   }

   // move the piece backward

   if (undo.IsPromote) {

      // promote

      ASSERT(pieza==(PromotePiece(move)+undo.wtm)); // HACK
      ClearSquare(to);

      pieza = Pawn +undo.wtm;
	  PutSquare(from,pieza);

   } else {

      // normal move
		MoveSquare(to,from,pieza);
   }

   // put the captured piece back

   if (undo.capture) {
	   PutSquare(undo.capture_square,undo.capture_piece);
   }

   // update board info

   wtm = undo.wtm;
   CastleFlags = undo.CastleFlags;
   EnPassant = undo.EnPassant;
   PlyNumber = undo.PlyNumber;

   ASSERT(board_is_ok());
}

void Board::DoNullMove(UndoData &undo) 
{

   int sq;

   // initialise undo
   undo.wtm = wtm;
   undo.EnPassant = EnPassant;
   undo.PlyNumber = PlyNumber;

   // update turn

   wtm = this->ColorOpp(wtm);

   // update en-passant square

   sq = EnPassant;
   if (sq != Empty) {
      EnPassant = Empty;
   }

   // update move number
   PlyNumber = 0; 

   // debug
   ASSERT(board_is_ok());
}
void Board::UndoNullMove(UndoData &undo)
{
   // update board info
	wtm = undo.wtm;
	EnPassant = undo.EnPassant;
	PlyNumber = undo.PlyNumber;
   // debug
   ASSERT(board_is_ok());
}



bool Board::IsLegal()
{
	if(wtm == White)
	{
		if(IsAttacked(bKingPos,White))
		{
			return false;
		}
	}
	else
	{
		if(IsAttacked(wKingPos,Black))
		{
			return false;
		}
	}
	return true;
}


bool Board::IsPromote(int move)
{
	int from,to;
	from = MoveFrom(move);
	if(PieceUncolored(board[from]) != Pawn) return false;
	to = MoveTo(move);
	if((board[from]&3) == White)
	{
		if(RankSq16x12(to) == Rank8)
			return true;
	}
	else
	{
		if(RankSq16x12(to) == Rank1)
			return true;
	}
	return false;
}

int Board::MoveToAlgebra(int move,char *dest)
{
	int from,to;
	from = MoveFrom(move);
	to = MoveTo(move);
	dest[0] = CharFile(FileSq16x12(from));
	dest[1] = CharRank(RankSq16x12(from));
	dest[2] = CharFile(FileSq16x12(to));
	dest[3] = CharRank(RankSq16x12(to));
	dest[4] = '\0';
	if(IsPromote(move))
	{
		int prom = PromotePiece(move);

		switch(prom)
		{
		case Queen:
			dest[4] = 'q';
			break;
		case Rook:
			dest[4] = 'r';
			break;
		case Bishop:
			dest[4] = 'b';
			break;
		case Knight:
			dest[4] = 'n';
			break;
		}
		dest[5] = '\0';

	}
	return piece_colour(board[from]);
}

void Board::SaveFEN(char *dest)
{
static char *pieceList[12] = {"P","p","N","n","B","b","R","r","Q","q","K","k"};
	int i= 0,j= 0;
	char s[250];
	int blancos = 0;
	// blancas
	s[0] = '\0';
	for(j=56;j>=0;j-=8)
	{
		blancos = 0;

		for(i=0;i < 8;i++)
		{
			if(board[sq64to16x12(i+j)] != Empty)
			{
				if(blancos > 0)
				{
					char tmp[2];
					sprintf(tmp,"%d",blancos);
					strcat(s,tmp);
					blancos = 0;
				}			
				strcat(s,pieceList[piece_to_12(board[sq64to16x12(i+j)])]);
			}
			else
			{
				blancos++;
			}
		}
		if(blancos > 0)
		{
			char tmp[3];
			sprintf(tmp,"%d",blancos);
			strcat(s,tmp);
			blancos = 0;
		}
		if((i+j) > 8)
			strcat(s,"/");
	}
	strcat(s," ");
	// segundo campo quien juega
	strcat(s,wtm == White ? "w ":"b ");
	// tercer campo ... enroques validos
	if(	CastleFlags == 0)
		strcat(s,"-");
	else
	{
		if(CastleFlags & FlagsWhiteKingCastle)
			strcat(s,"K");
		if(CastleFlags & FlagsWhiteQueenCastle)
			strcat(s,"Q");
		if(CastleFlags & FlagsBlackKingCastle)
			strcat(s,"k");
		if(CastleFlags & FlagsBlackQueenCastle)
			strcat(s,"q");
	}
	strcat(s," ");
	//Field 4  Enpassant
	if(EnPassant == 0)
		strcat(s,"-");
	else
	{
		char casilla[3];
		casilla[0] = 'a'+file07(EnPassant);
		casilla[1] = '1'+rank07(EnPassant);
		casilla[2] = '\0';
		strcat(s,casilla);
	}
	strcat(s," ");
	strcpy(dest,s);
}

void Board::DoMoveAlgebraic(char *move)
{
	// gen all moves
	MoveList List;
	int Move;
	int i;
	char dest[6];
	UndoData undo;

	GenPseudoMoves(List);
	for(i = 0; i < List.Length();i++)
	{
	// get algebraic
		Move = List.Move(i);
		MoveToAlgebra(Move,dest);
		if(strcmp(dest,move) == 0)
		{
			// if match domove
			DoMove(Move,undo);
			return;
		}
	}
	if(false) // debug
	{
		FILE *fdo = fopen("ucient.txt","a+");
		if(fdo)
		{
			char dest[120];
			fprintf(fdo,"move [%s] not found\n",move);
			DisplayF(fdo);
			SaveFEN(dest);
			fprintf(fdo,"%s\n",dest);
			fclose(fdo);
		}
	}

}

static int IdentWhiteC(char P)
{
	switch(P)
	{
	case 'N':
		return wKnight;
	case 'B':
		return wBishop;
	case 'R':
		return wRook;
	case 'Q':
		return wQueen;
	case 'K':
		return wKing;
	}
	return 0;
}
static int IdentBlackC(char P)
{
	switch(P)
	{
	case 'N':
		return bKnight;
	case 'B':
		return bBishop;
	case 'R':
		return bRook;
	case 'Q':
		return bQueen;
	case 'K':
		return bKing;
	}
	return 0;
}
static int IdentPromote(char P)
{
	switch(P)
	{
	case 'N':
		return Knight;
	case 'B':
		return Bishop;
	case 'R':
		return Rook;
	case 'Q':
		return Queen;
	case 'K':
		return King;
	}
	return 0;
}

int Board::IdentificaPgn(char *JugAlg1)
{
	MoveList List;
	int Move;
	int i;
	int p,f,t;
	int mf,mt,ff,Promote,rf;
	ff = 0;
	rf = 0;
	Promote = 0;
	Move = 0;
	p = -1;
	if(*JugAlg1 == 'O')  // Castle Moves.
	{
		if(strcmp(JugAlg1,"O-O") == 0 || strcmp(JugAlg1,"O-O+") == 0 || strcmp(JugAlg1,"O-O#") == 0)
		{
			if(wtm == White)
			{
				Move = MakeMoveSp(E1,G1,MoveCastle);
				p = wKing;
			}
			else
			{
				Move = MakeMoveSp(E8,G8,MoveCastle);
				p = bKing;
			}
		}
		if(strcmp(JugAlg1,"O-O-O") == 0 || strcmp(JugAlg1,"O-O-O+") == 0 || strcmp(JugAlg1,"O-O-O#") == 0)
		{
			if(wtm == White)
			{
				Move = MakeMoveSp(E1,C1,MoveCastle);
				p = wKing;
			}
			else
			{
				Move = MakeMoveSp(E8,C8,MoveCastle);
				p = bKing;
			}
		}
		if(Move != 0)
		{
			// verify it is legal
			GenPseudoMoves(List);

			for(i = 0; i < List.Length();i++)
			{
				if(Move == List.Move(i))
					return Move;
			}
			return 0; // illegal.
		}
	}
	if(strlen(JugAlg1) == 2) // simple pawn move e4 or d5
	{
		if(wtm == White)
			p = wPawn;
		else
			p = bPawn;
		f = 0;
		t = 36 + (JugAlg1[0] - 'a')  + (JugAlg1[1] -'1')*16;
	}
	else
	if(strlen(JugAlg1) == 3) // simple Piece move Nf3 or Bc5
	{
		if(JugAlg1[2] == '+' || JugAlg1[2] == '#') // g5+ e4#
		{
			if(wtm == White)
				p = wPawn;
			else
				p = bPawn;
			f = 0;
			t = 36 + (JugAlg1[0] - 'a')  + (JugAlg1[1] -'1')*16;
		}
		else
		{
			if(wtm == White)
				p = IdentWhiteC(JugAlg1[0]);
			else
				p = IdentBlackC(JugAlg1[0]);

			f = 0;
			t = 36+(JugAlg1[1] - 'a') + (JugAlg1[2] -'1')*16;
		}
	}
	else
	if(strlen(JugAlg1) == 4) // simple Piece capture Nxe5 or Bxe7
	{
		if(JugAlg1[1] == 'x')
		{
			if(wtm == White)
				p = IdentWhiteC(JugAlg1[0]);
			else
				p = IdentBlackC(JugAlg1[0]);
			if(p==0)
			{	// axb6
				if(wtm== White)
					p = wPawn;
				else
					p = bPawn;
				ff = JugAlg1[0] - 'a'+1;
			}
			t = 36+(JugAlg1[2] - 'a') + (JugAlg1[3] -'1')*16;
			f = 0;
			if(p==0)  // Simple pawn capture axb5
			{
				if(wtm == White)
					p = wPawn;
				else
					p = bPawn;
				ff = (JugAlg1[0] - 'a')+1;
			}
		}
		else if(JugAlg1[2] == '=') // g1=R g1=Q
		{
			t = 36+(JugAlg1[0] - 'a') + (JugAlg1[1] -'1')*16;
			f = 0;
			if(wtm == White)
				p = wPawn;
			else
				p = bPawn;

			
			Promote = IdentPromote(JugAlg1[3]);
		}
		else	// Rae8
		{
			if(JugAlg1[3] == '+' || JugAlg1[3] == '#') // Rd6+
			{
				if(wtm == White)
					p = IdentWhiteC(JugAlg1[0]);
				else
					p = IdentBlackC(JugAlg1[0]);
				t = 36+(JugAlg1[1] - 'a') + (JugAlg1[2] -'1')*16;
				f = 0;
			}
			else
			{
				if(wtm == White)
					p = IdentWhiteC(JugAlg1[0]);
				else
					p = IdentBlackC(JugAlg1[0]);
				if(JugAlg1[1] < 'a')
					rf = (JugAlg1[1] - '1')+1;
				else
					ff = (JugAlg1[1] - 'a')+1;
				t = 36+(JugAlg1[2] - 'a') + (JugAlg1[3] -'1')*16;
				f = 0;
			}
		}
	}
	if(strlen(JugAlg1) == 5) // Rfe1+
	{
		if(JugAlg1[1] == 'x')  // Rxd7+
		{
			if(wtm == White)
				p = IdentWhiteC(JugAlg1[0]);
			else
				p = IdentBlackC(JugAlg1[0]);
			t = 36+(JugAlg1[2] - 'a') + (JugAlg1[3] -'1')*16;
			f = 0;
			if(p==0)  // Simple pawn capture axb5+
			{
				if(wtm == White)
					p = wPawn;
				else
					p = bPawn;
				ff = (JugAlg1[0] - 'a')+1;
			}
		}
		else
		{
			if(JugAlg1[4] == '+' || JugAlg1[4] == '#')
			{
				if(JugAlg1[2] == '=')   // g1=Q#
				{
					t = 36+(JugAlg1[0] - 'a') + (JugAlg1[1] -'1')*16;
					f = 0;
					if(wtm == White)
						p = wPawn;
					else
						p = bPawn;
					Promote = IdentPromote(JugAlg1[3]);

				}
				else
				{
					if(wtm == White)
						p = IdentWhiteC(JugAlg1[0]);
					else
						p = IdentBlackC(JugAlg1[0]);
					ff = (JugAlg1[1] - 'a')+1;
					if(ff < 0)
					{
						ff = 0;
						rf = (JugAlg1[1] - '1')+1;
					}
					t = 36+(JugAlg1[2] - 'a') + (JugAlg1[3] -'1')*16;
					f = 0;
					if(p==0)  // Simple pawn capture axb5
					{
						if(wtm == White)
							p = wPawn;
						else
							p = bPawn;
					}
				}
			}
			else
			{
				if(JugAlg1[2] == 'x')   // Rdxd3
				{
					if(wtm == White)
						p = IdentWhiteC(JugAlg1[0]);
					else
						p = IdentBlackC(JugAlg1[0]);
					if(JugAlg1[1] < 'a')
						rf = (JugAlg1[1] - '1')+1;
					else
						ff = (JugAlg1[1] - 'a')+1;
					t = 36+(JugAlg1[3] - 'a') + (JugAlg1[4] -'1')*16;
					f = 0;
				}
			}
		}
		if(p == -1)
		{
			if(JugAlg1[2] == 'x')   // h4xg3
			{
				if(wtm == White)
					p = wPawn;
				else
					p = bPawn;
				f = 36+(JugAlg1[0] - 'a') + (JugAlg1[1] -'1')*16;
				t =  36+(JugAlg1[3] - 'a') + (JugAlg1[4] -'1')*16;
			}
			else
			{
			// Kd3c4
			if(wtm == White)
				p = IdentWhiteC(JugAlg1[0]);
			else
				p = IdentBlackC(JugAlg1[0]);
			f = 36+(JugAlg1[1] - 'a') + (JugAlg1[2] -'1')*16;
			t =  36+(JugAlg1[3] - 'a') + (JugAlg1[4] -'1')*16;
			}

		}
	}
	if(strlen(JugAlg1) == 6) // Rgxg7+
	{
		if(JugAlg1[5] == '+' || JugAlg1[5] == '#')
		{
			if(JugAlg1[2] == 'x')   // Rdxd3
			{
				if(wtm == White)
					p = IdentWhiteC(JugAlg1[0]);
				else
					p = IdentBlackC(JugAlg1[0]);
				if(JugAlg1[1] < 'a')
					rf = (JugAlg1[1] - '1')+1;
				else
					ff = (JugAlg1[1] - 'a')+1;
				t = 36+(JugAlg1[3] - 'a') + (JugAlg1[4] -'1')*16;
				f = 0;
			}
		}
		else
		{
			if(JugAlg1[1] == 'x' && JugAlg1[4] == '=')  // cxb8=R
			{
				t = 36+(JugAlg1[2] - 'a') + (JugAlg1[3] -'1')*16;
				f = 0;
				if(wtm == White)
					p = wPawn;
				else
					p = bPawn;

				ff = (JugAlg1[0] - 'a')+1;

				Promote = IdentPromote(JugAlg1[5]);

			}
		}
	}
	if(strlen(JugAlg1) == 7) // dxe8=Q+
	{
		if(JugAlg1[6] == '+' || JugAlg1[6] == '#')
		{
			if(JugAlg1[1] == 'x' && JugAlg1[4] == '=')  // cxb8=R#
			{
				t = 36+(JugAlg1[2] - 'a') + (JugAlg1[3] -'1')*16;
				f = 0;
				if(wtm == White)
					p = wPawn;
				else
					p = bPawn;

				ff = (JugAlg1[0] - 'a')+1;
				Promote = IdentPromote(JugAlg1[5]);

			}
		}
	}
	// buscar en la lista de jugadas
	Move = 0;
	GenPseudoMoves(List);
	UndoData undo;
	for(i = 0; i < List.Length();i++)
	{
		Move = List.Move(i);
		mf = MoveFrom(Move);
		mt = MoveTo(Move);
		if(board[mf] == p)
		{
			if(mt == t && f == 0)
			{
				if(Promote != 0)
				{
					int pp = PromotePiece(Move);
					if(pp == Promote)
					{
						// Test legality
						DoMove(Move,undo);
						if(IsLegal())
						{
							UndoMove(Move,undo);
							return Move;
						}
						else
						{
							UndoMove(Move,undo);
						}
					}
					if(pp != Empty)
						continue;
				}
				if(ff == 0)
				{
					if(rf == 0)
					{
						// Test legality
						DoMove(Move,undo);
						if(IsLegal())
						{
							UndoMove(Move,undo);
							return Move;
						}
						else
						{
							UndoMove(Move,undo);
						}
					}
					else
						if((rf-1) == rank07(mf))
						{
							// Test legality
							DoMove(Move,undo);
							if(IsLegal())
							{
								UndoMove(Move,undo);
								return Move;
							}
							else
							{
								UndoMove(Move,undo);
							}
						}
				}
				else
					if((ff-1) == file07(mf))
					{
						// Test legality
						DoMove(Move,undo);
						if(IsLegal())
						{
							UndoMove(Move,undo);
							return Move;
						}
						else
						{
							UndoMove(Move,undo);
						}
					}
			}
		}
	}
	// si no devolver Error
	return 0;
}

int Board::Repetition(u64 hash)
// verifica si esta posicion ya la hemos visto
{
	int i;
	int repe = 0;
	for(i=0;i < stHistory;i++)
	{
		if(HashStack[i] == hash)
			repe++;
	}
	if(repe)
		return repe;
	return 0;
}
void Board::SetHashStack(u64 hash)
// asigna una firma de la posicion
{
	if(stHistory < MAXDEPTH )
		HashStack[stHistory++] = hash;
	else
		stHistory--;
}
void Board::ResetHashStack()
{
	memset(HashStack,0,sizeof(HashStack));
	stHistory = 0;
}
void Board::PopHashStack()
{
	stHistory--;
	if(stHistory < 0)
		stHistory = 0;
}

