
// line.cpp

// includes

#include <cstring>

#include "board.h"
#include "line.h"
#include "move.h"
#include "move_do.h"
#include "move_legal.h"
#include "san.h"
#include "util.h"

// constants

static const bool Strict = false; // false
static const bool UseDebug = false; // false

static const int StringSize = 1024;

// functions

// line_is_ok()

bool line_is_ok(const move_t line[]) {

   int move;

   if (line == NULL) return false;

   while ((move = *line++) != MoveNone) {
      if (!move_is_ok(move)) return false;
   }

   return true;
}

// line_clear()

void line_clear(move_t line[]) {

   ASSERT(line!=NULL);

   *line = MoveNone;
}

// line_copy()

void line_copy(move_t dst[], const move_t src[]) {

   ASSERT(dst!=NULL);
   ASSERT(src!=NULL);

   ASSERT(dst!=src);

   while ((*dst++ = *src++) != MoveNone)
      ;
}

// line_from_can()

bool line_from_can (move_t line[], const board_t * board, const char string[], int size) {

   int pos;
   char new_string[StringSize], *p;
   int move;
   board_t new_board[1];

   ASSERT(line!=NULL);
   ASSERT(board_is_ok(board));
   ASSERT(string!=NULL);
   ASSERT(size>=LineSize);

   // init

   pos = 0;
   board_copy(new_board,board);

   // loop

   strcpy(new_string,string); // HACK

   for (p = strtok(new_string," "); p != NULL; p = strtok(NULL," ")) {
      move = move_from_can(p,new_board);
      ASSERT(move!=MoveNone);
      ASSERT(move_is_legal(move,new_board));
      if (move == MoveNone || !move_is_legal(move,new_board)) break; // HACK: ignore illegal moves
      if (pos >= size) return false;
      line[pos++] = move;

      move_do(new_board,move);
   }

   if (pos >= size) return false;
   line[pos] = MoveNone;

   return true;
}

// line_to_can()

bool line_to_can(const move_t line[], const board_t * board, char string[], int size) {

   board_t new_board[1];
   int pos;
   int move;

   ASSERT(line_is_ok(line));
   ASSERT(board_is_ok(board));
   ASSERT(string!=NULL);
   ASSERT(size>=StringSize);

   // init

   if (size < StringSize) return false;

   board_copy(new_board,board);
   pos = 0;

   // loop

   while ((move = *line++) != MoveNone) {

      if (pos != 0) {
         if (pos >= size) return false;
         string[pos++] = ' ';
      }

      if (!move_to_can(move,new_board,&string[pos],size-pos)) return false;
      pos += (int)strlen(&string[pos]);

      move_do(new_board,move);
   }

   if (pos >= size) return false;
   string[pos] = '\0';

   return true;
}

// line_to_san()

bool line_to_san(const move_t line[], const board_t * board, char string[], int size) {

	board_t new_board[1];
	int pos;
	int move;
	char move_string[256];

	ASSERT(line_is_ok(line));
	ASSERT(board_is_ok(board));
	ASSERT(string!=NULL);
	ASSERT(size>=StringSize);

	// init
	string[0]='\0';
	if (size < StringSize) return false;
//    return false;
	board_copy(new_board,board);
	pos = 0;

	// loop

	while ((move = *line++) != MoveNone) {
		if (pos != 0) {
			if (pos >= size) return false;
			string[pos++] = ' ';
		}
		if (!move_is_legal(move,new_board)
			|| !move_to_san(move,new_board,&string[pos],size-pos)) {
				if (Strict || UseDebug) {
					move_to_can(move,new_board,move_string,sizeof(move_string));
					my_log("POLYGLOT ILLEGAL MOVE IN LINE %s\n",move_string);
					board_disp(new_board);
					if (Strict) my_fatal("line_to_san(): illegal move\n");
				}
				break;
		}
		pos += (int)strlen(&string[pos]);
		move_do(new_board,move);
	}
	if (pos >= size) return false;
	string[pos] = '\0';
	return true;
}

// end of line.cpp

