#ifndef _CRT_SECURE_NO_DEPRECATE
#define _CRT_SECURE_NO_DEPRECATE
#endif
#include <iostream>

#include <string.h>
#include "defines.h"
#include "protos.h"
#include "globals.h"

/*
extern const unsigned char EMPTY = 0;                //  0000
extern const unsigned char WHITE_PAWN = 1;           //  0001
extern const unsigned char WHITE_KING = 2;           //  0010
extern const unsigned char WHITE_KNIGHT = 3;         //  0011
extern const unsigned char WHITE_BISHOP =  5;        //  0101
extern const unsigned char WHITE_ROOK = 6;           //  0110
extern const unsigned char WHITE_QUEEN = 7;          //  0111
extern const unsigned char BLACK_PAWN = 9;           //  1001
extern const unsigned char BLACK_KING = 10;          //  1010
extern const unsigned char BLACK_KNIGHT = 11;        //  1011
extern const unsigned char BLACK_BISHOP = 13;        //  1101
extern const unsigned char BLACK_ROOK = 14;          //  1110
extern const unsigned char BLACK_QUEEN = 15;         //  1111
*/


extern "C" __declspec(dllexport) void lc_inicio(void)
{
	dataInit();
	board.init();
}

extern "C" __declspec(dllexport) int lc_check(void)
{
	if (board.nextMove)
       {
           return isAttacked(board.blackKing, WHITE_MOVE);
       }
       else
       {
           return isAttacked(board.whiteKing, BLACK_MOVE);
       }

	return isOtherKingAttacked();
}

extern "C" __declspec(dllexport) void lc_setupFen(char *fen, char *fencolor, char *fencastling, char *fenenpassant, int fenhalfmoveclock, int fenfullmovenumber)
{
	setupFen(fen, fencolor, fencastling, fenenpassant, fenhalfmoveclock, fenfullmovenumber);
}

static char lcBuffer[4096];

const char * LCPIECES = " PKN BRQ pkn brq";
const char * MLCPIECES = " PKN BRQ PKN BRQ";
extern "C" __declspec(dllexport) char * lc_moves(void)
{
		int i,n;
		int from, to, piece, prom;
		int asciiShift, asciiDec;
		Move move;
		int pos;

		asciiShift = (int)'a'-1;
		asciiDec = (int)'0';

		pos = 0;

		board.moveBufLen[0] = 0;
		board.moveBufLen[1] = movegen(board.moveBufLen[0]);
		n = 0;
		for (i = board.moveBufLen[0]; i < board.moveBufLen[1]; i++)
		{

			if (n) lcBuffer[pos++] = ' ';
			else n = 1;

			move = board.moveBuffer[i];

			makeMove(move);
			if (isOtherKingAttacked())
			{
				unmakeMove(move);
				lcBuffer[pos++] = '-';
			}
			else
			{
				unmakeMove(move);

				piece = move.getPiec();
				from = move.getFrom();
				to = move.getTosq();
				prom = move.getProm();

				lcBuffer[pos++] = LCPIECES[piece];

				lcBuffer[pos++] = FILES[from] + asciiShift;
				lcBuffer[pos++] = RANKS[from] + asciiDec;
				lcBuffer[pos++] = FILES[to] + asciiShift;
				lcBuffer[pos++] = RANKS[to] + asciiDec;

				if (move.isPromotion()) lcBuffer[pos++] = PIECECHARS[prom][0]+97-65;
			}
		}
		lcBuffer[pos] = '\0';
		return lcBuffer;

}

extern "C" __declspec(dllexport) char * lc_exmoves(void)
{
		int i,n,j;
		int from, to, piece, prom;
		int asciiShift, asciiDec;
		char sanMove[12];
		Move move;
		int pos;

		asciiShift = (int)'a'-1;
		asciiDec = (int)'0';

		pos = 0;

		board.moveBufLen[0] = 0;
		board.moveBufLen[1] = movegen(board.moveBufLen[0]);
		n = 0;
		for (i = board.moveBufLen[0]; i < board.moveBufLen[1]; i++)
		{

			if (n) lcBuffer[pos++] = '|';
			else n++;

			move = board.moveBuffer[i];

			toSan(move, sanMove);
			makeMove(move);
			if (!isOtherKingAttacked())
			{
				piece = move.getPiec();
				from = move.getFrom();
				to = move.getTosq();
				prom = move.getProm();

				lcBuffer[pos++] = LCPIECES[piece];

				lcBuffer[pos++] = FILES[from] + asciiShift;
				lcBuffer[pos++] = RANKS[from] + asciiDec;
				lcBuffer[pos++] = FILES[to] + asciiShift;
				lcBuffer[pos++] = RANKS[to] + asciiDec;

				if (move.isPromotion()) lcBuffer[pos++] = PIECECHARS[prom][0];
				else lcBuffer[pos++] = ' ';

				if (move.isCastleOO()) lcBuffer[pos++] = 'K';
				else if (move.isCastleOOO()) lcBuffer[pos++] = 'Q';
				else lcBuffer[pos++] = ' ';

				if (move.isEnpassant()) lcBuffer[pos++] = 'E';
				else lcBuffer[pos++] = ' ';

				if (move.isCapture()) lcBuffer[pos++] = 'x';
				else lcBuffer[pos++] = ' ';

				lcBuffer[pos++] = ',';
				j = 0;
				while( sanMove[j] ) lcBuffer[pos++] = sanMove[j++];

			}
			unmakeMove(move);
		}
		lcBuffer[pos] = '\0';
		return lcBuffer;
}


extern "C" __declspec(dllexport) void lc_makemove(int pos)
{
		makeMove(board.moveBuffer[pos]);
		if( ! board.nextMove ) board.fullmovenumber++;
		board.endOfGame++;
		board.endOfSearch = board.endOfGame;
}

extern "C" __declspec(dllexport) char * lc_toSAN(int pos)
{
		toSan(board.moveBuffer[pos], lcBuffer);
		return lcBuffer;
}


extern "C" __declspec(dllexport) char * lc_getPosition(void)
{
	int i;
	int piece, pos, d;
	int asciiShift, asciiDec;

	asciiShift = (int)'a'-1;
	asciiDec = (int)'0';

	for (pos = 0; pos < 64; pos++)
	{
		piece = board.square[pos];
		if ( piece ) lcBuffer[pos] = LCPIECES[piece];
		else lcBuffer[pos] = ' ';
	}

	// Color
	if( board.nextMove ) lcBuffer[64] = 'b';
	else lcBuffer[64] = 'w';

	// Castle
	if( board.castleWhite & CANCASTLEOO) lcBuffer[65] = 'K';
	else lcBuffer[65] = ' ';
	if( board.castleWhite & CANCASTLEOOO) lcBuffer[66] = 'Q';
	else lcBuffer[66] = ' ';
	if( board.castleBlack & CANCASTLEOO) lcBuffer[67] = 'k';
	else lcBuffer[67] = ' ';
	if( board.castleBlack & CANCASTLEOOO) lcBuffer[68] = 'q';
	else lcBuffer[68] = ' ';

	// En passant
	if( board.epSquare ) {
		lcBuffer[69] = FILES[board.epSquare] + asciiShift;
		lcBuffer[70] = RANKS[board.epSquare] + asciiDec;
	}
	else {
		lcBuffer[69] = '-';
		lcBuffer[70] = ' ';
	}

	// Pawnmoves
	i = board.fiftyMove;
	d = i/10;
	if( d ) {
		lcBuffer[71] = d + asciiDec;
		i -= d*10;
	}
	else lcBuffer[71] = '0';
	lcBuffer[72] = i + asciiDec;

	// Moves
	i = board.fullmovenumber;
	d = i/100;
	if( d ) {
		lcBuffer[73] = d + asciiDec;
		i -= d*100;
	}
	else lcBuffer[73] = '0';
	d = i/10;
	if( d ) {
		lcBuffer[74] = d + asciiDec;
		i -= d*10;
	}
	else lcBuffer[74] = '0';
	lcBuffer[75] = i + asciiDec;

	lcBuffer[76] = '\0';
	return lcBuffer;
}

extern "C" __declspec(dllexport) char * lc_getFen(void)
{
	int rank, file;
	int nSin, piece, pos, i, d;
	int asciiShift, asciiDec;
	char tmp[20];

	asciiShift = (int)'a'-1;
	asciiDec = (int)'0';

	nSin = 0;
	pos = 0;

	for (rank = 8; rank >= 1; rank--)
	{
	   for (file = 1; file <= 8; file++)
	   {
			piece = board.square[BOARDINDEX[file][rank]];
			if ( piece )
			{
				if( nSin ) {
					lcBuffer[pos++] = nSin + asciiDec;
					nSin = 0;
				}
				lcBuffer[pos++] = LCPIECES[piece];
			}
			else nSin++;
	   }
		if( nSin ) {
			lcBuffer[pos++] = nSin + asciiDec;
			nSin = 0;
		}
		if (rank > 1) lcBuffer[pos++] = '/';
	}

	// Color
	lcBuffer[pos++] = ' ';
	if( board.nextMove ) lcBuffer[pos++] = 'b';
	else lcBuffer[pos++] = 'w';

	// Castle
	lcBuffer[pos++] = ' ';
	nSin = 1;
	if( board.castleWhite & CANCASTLEOO) {
		nSin = 0;
		lcBuffer[pos++] = 'K';
	}
	if( board.castleWhite & CANCASTLEOOO) {
		nSin = 0;
		lcBuffer[pos++] = 'Q';
	}
	if( board.castleBlack & CANCASTLEOO) {
		nSin = 0;
		lcBuffer[pos++] = 'k';
	}
	if( board.castleBlack & CANCASTLEOOO) {
		nSin = 0;
		lcBuffer[pos++] = 'q';
	}
	if( nSin ) lcBuffer[pos++] = '-';

	// En passant
	lcBuffer[pos++] = ' ';
	if( board.epSquare ) {
		lcBuffer[pos++] = FILES[board.epSquare] + asciiShift;
		lcBuffer[pos++] = RANKS[board.epSquare] + asciiDec;
	}
	else lcBuffer[pos++] = '-';

	// Pawnmoves
	lcBuffer[pos++] = ' ';
	i = board.fiftyMove;
	d = i/10;
	if( d ) {
		lcBuffer[pos++] = d + asciiDec;
		i -= d*10;
	}
	lcBuffer[pos++] = i + asciiDec;

	// Moves
	lcBuffer[pos++] = ' ';

	sprintf( tmp, "%d", board.fullmovenumber );
	lcBuffer[pos++] = ' ';
	for( i = 0; tmp[i]; i++ ) lcBuffer[pos++] = tmp[i];

	lcBuffer[pos] = '\0';
	return lcBuffer;
}

extern "C" __declspec(dllexport) char * lc_pgn2pv(char * pgn)
{
	int p_desde, p_hasta, count, i;
	char pz, cprom;
	char hasta[2], desde[2], mdesde[2], mhasta[2];
	int castle, castOO, castOOO;
	int pos;
	int n;
	int from, to, piece, prom;
	int asciiShift, asciiDec;
	int ok;
	Move move;

	lcBuffer[0] = '\0';

	//board.display();

	p_desde = 0;
	p_hasta = strlen(pgn)-1;

	desde[0] = 0;
	desde[1] = 0;
	castle = 0;
	cprom = '\0';

	switch(pgn[0])	{
		case 'K':
		case 'Q':
		case 'R':
		case 'B':
		case 'N':
			pz = pgn[0];
			p_desde = 1;
			break;
		case '0':
		case 'O':
		case 'o':
			pz = 'K';
			castle = 1;
			castOO = 0;
			castOOO = 0;
			for( count=0,i = p_desde; i <= p_hasta; i++ ) if( pgn[i] == '-' ) count++;
			if( count == 1 ) castOO = 1;
			else if( count == 2 ) castOOO = 1;
			else return lcBuffer;
			break;
		default:
			pz = 'P';
			break;
	}

	if( ! castle )	{
		switch(pgn[p_hasta]) {
			case 'Q':
			case 'R':
			case 'B':
			case 'N':
				cprom = pgn[p_hasta--];
				if(pgn[p_hasta]=='=') p_hasta--;
				break;
			case 'p':
				while( p_hasta && (pgn[p_hasta] < '1' || pgn[p_hasta] > '8') ) p_hasta--;
		}

		if( (pgn[p_hasta] < '1') || (pgn[p_hasta] > '8') ) return lcBuffer;
		hasta[1] = pgn[p_hasta--];
		if( (pgn[p_hasta] < 'a') || (pgn[p_hasta] > 'h') ) return lcBuffer;
		hasta[0] = pgn[p_hasta--];

		if( p_hasta >= p_desde ) {
			if( pgn[p_hasta] == 'x' ) p_hasta--;
		}
		if( p_hasta >= p_desde ) {
			if(pgn[p_hasta] >= '1' && pgn[p_hasta] <= '8') desde[1] = pgn[p_hasta--];
		}
		if( p_hasta >= p_desde ) {
			if(pgn[p_hasta] >= 'a' && pgn[p_hasta] <= 'h') desde[0] = pgn[p_hasta--];
		}
	}


	// Buscamos el movimiento que mas se acomode a las condiciones de pz, cprom, desde, hasta, castle
	asciiShift = (int)'a'-1;
	asciiDec = (int)'0';

	pos = 0;

	board.moveBufLen[0] = 0;
	board.moveBufLen[1] = movegen(board.moveBufLen[0]);
	n = 0;
	ok = 0;
	for (i = board.moveBufLen[0]; i < board.moveBufLen[1]; i++)
	{

		move = board.moveBuffer[i];
		makeMove(move);
		while( 1 ) {

			if (isOtherKingAttacked()) break;

			piece = move.getPiec();
			if ( MLCPIECES[piece] != pz ) break;

			to = move.getTosq();
			from = move.getFrom();
			mdesde[0] = FILES[from] + asciiShift;
			mdesde[1] = RANKS[from] + asciiDec;
			mhasta[0] = FILES[to] + asciiShift;
			mhasta[1] = RANKS[to] + asciiDec;

			if( castle ) {
				if( castOO ) {
					if( move.isCastleOO() ) ok = 1;
				}
				else if( castOOO ) {
					if( move.isCastleOOO() ) ok = 1;
				}
				break;
			}

			// hasta
			if ( ( mhasta[0] != hasta[0] ) || ( mhasta[1] != hasta[1] ) ) break;

			// desde
			if( ( desde[0] && (mdesde[0] != desde[0]) ) ||
				( desde[1] && (mdesde[1] != desde[1]) ) ) break;

			// Promotion
			if( cprom ) {
				prom = move.getProm();
				if( cprom != PIECECHARS[prom][0] ) break;
			}
			ok = 1;
			break;
		}
		if( ok ) {
			lcBuffer[pos++] = mdesde[0];
			lcBuffer[pos++] = mdesde[1];
			lcBuffer[pos++] = mhasta[0];
			lcBuffer[pos++] = mhasta[1];
			if( cprom ) lcBuffer[pos++] = cprom-65+97;
			if( ! board.nextMove ) board.fullmovenumber++;
			//board.display();
			break;
		}
		unmakeMove(move);
	}

	lcBuffer[pos] = '\0';
	return lcBuffer;

}

extern "C" __declspec(dllexport) char * lc_think(int sd)
{
	int from, to, piece, prom, n;
	int asciiShift, asciiDec;
	Move move;
	int pos;
	char tmp[20];

	asciiShift = (int)'a'-1;
	asciiDec = (int)'0';

	pos = 0;

	board.searchDepth = sd;
	move = board.think();
	if (move.moveInt)
	{
		piece = move.getPiec();
		from = move.getFrom();
		to = move.getTosq();
		prom = move.getProm();

		lcBuffer[pos++] = FILES[from] + asciiShift;
		lcBuffer[pos++] = RANKS[from] + asciiDec;
		lcBuffer[pos++] = FILES[to] + asciiShift;
		lcBuffer[pos++] = RANKS[to] + asciiDec;

		if (move.isPromotion()) lcBuffer[pos++] = PIECECHARS[prom][0]+97-65;
		lcBuffer[pos++] = '|';
		sprintf( tmp, "%d", board.lastScore );
		for( n = 0; tmp[n]; n++ ) lcBuffer[pos++] = tmp[n];
	}
	lcBuffer[pos] = '\0';
	return lcBuffer;
}

