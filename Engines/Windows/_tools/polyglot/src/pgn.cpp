
// pgn.cpp

// includes

#include <cctype>
#include <cerrno>
#include <cstdio>
#include <cstring>

#include "pgn.h"
#include "util.h"

// constants

static const bool DispMove = false;
static const bool DispToken = false;
static const bool DispChar = false;

static const int TAB_SIZE = 8;

static const int CHAR_EOF = 256;

// types

enum token_t {
   TOKEN_ERROR   = -1,
   TOKEN_EOF     = 256,
   TOKEN_SYMBOL  = 257,
   TOKEN_STRING  = 258,
   TOKEN_INTEGER = 259,
   TOKEN_NAG     = 260,
   TOKEN_RESULT  = 261
};

// prototypes

static void pgn_token_read   (pgn_t * pgn);
static void pgn_token_unread (pgn_t * pgn);

static void pgn_read_token   (pgn_t * pgn);

static bool is_symbol_start  (int c);
static bool is_symbol_next   (int c);

static void pgn_skip_blanks  (pgn_t * pgn);

static void pgn_char_read    (pgn_t * pgn);
static void pgn_char_unread  (pgn_t * pgn);

// functions

// pgn_open()

void pgn_open(pgn_t * pgn, const char file_name[]) {

   ASSERT(pgn!=NULL);
   ASSERT(file_name!=NULL);

   pgn->file = fopen(file_name,"r");
   if (pgn->file == NULL) my_fatal("pgn_open(): can't open file \"%s\": %s\n",file_name,strerror(errno));

   pgn->char_hack = CHAR_EOF; // DEBUG
   pgn->char_line = 1;
   pgn->char_column = 0;
   pgn->char_unread = false;
   pgn->char_first = true;

   pgn->token_type = TOKEN_ERROR; // DEBUG
   strcpy(pgn->token_string,"?"); // DEBUG
   pgn->token_length = -1; // DEBUG
   pgn->token_line = -1; // DEBUG
   pgn->token_column = -1; // DEBUG
   pgn->token_unread = false;
   pgn->token_first = true;

   strcpy(pgn->result,"?"); // DEBUG
   strcpy(pgn->fen,"?"); // DEBUG

   pgn->move_line = -1; // DEBUG
   pgn->move_column = -1; // DEBUG
}

// pgn_close()

void pgn_close(pgn_t * pgn) {

   ASSERT(pgn!=NULL);

   fclose(pgn->file);
}

// pgn_next_game()

bool pgn_next_game(pgn_t * pgn) {

   char name[PGN_STRING_SIZE];
   char value[PGN_STRING_SIZE];

   ASSERT(pgn!=NULL);

   // init

   strcpy(pgn->result,"*");
   strcpy(pgn->fen,"");

   // loop

   while (true) {

      pgn_token_read(pgn);

      if (pgn->token_type != '[') break;

      // tag

      pgn_token_read(pgn);
      if (pgn->token_type != TOKEN_SYMBOL) {
         my_fatal("pgn_next_game(): malformed tag at line %d, column %d, game %d\n",pgn->token_line,pgn->token_column,pgn->game_nb);
      }
      strcpy(name,pgn->token_string);

      pgn_token_read(pgn);
      if (pgn->token_type != TOKEN_STRING) {
         my_fatal("pgn_next_game(): malformed tag at line %d, column %d, game %d\n",pgn->token_line,pgn->token_column,pgn->game_nb);
      }
      strcpy(value,pgn->token_string);

      pgn_token_read(pgn);
      if (pgn->token_type != ']') {
         my_fatal("pgn_next_game(): malformed tag at line %d, column %d, game %d\n",pgn->token_line,pgn->token_column,pgn->game_nb);
      }

      // special tag?

      if (false) {
      } else if (my_string_equal(name,"Result")) {
         strcpy(pgn->result,value);

      } else if (my_string_equal(name,"FEN")) {
         strcpy(pgn->fen,value);
      }

   }

   if (pgn->token_type == TOKEN_EOF) return false;

   pgn_token_unread(pgn);

   return true;
}

// pgn_next_move()

bool pgn_next_move(pgn_t * pgn, char string[], int size) {

   int depth;

   ASSERT(pgn!=NULL);
   ASSERT(string!=NULL);
   ASSERT(size>=PGN_STRING_SIZE);

   // init

   pgn->move_line = -1; // DEBUG
   pgn->move_column = -1; // DEBUG

   // loop

   depth = 0;

   while (true) {

      pgn_token_read(pgn);

      if (false) {

      } else if (pgn->token_type == '(') {

         // open RAV

         depth++;

      } else if (pgn->token_type == ')') {

         // close RAV

         if (depth == 0) {
            my_fatal("pgn_next_move(): malformed variation at line %d, column %d, game %d\n",pgn->token_line,pgn->token_column,pgn->game_nb);
         }

         depth--;
         ASSERT(depth>=0);

      } else if (pgn->token_type == TOKEN_RESULT) {

         // game finished

         if (depth > 0) {
            my_fatal("pgn_next_move(): malformed variation at line %d, column %d, game %d\n",pgn->token_line,pgn->token_column,pgn->game_nb);
         }

         return false;

      } else {

         // skip optional move number

         if (pgn->token_type == TOKEN_INTEGER) {
            do pgn_token_read(pgn); while (pgn->token_type == '.');
         }

         // move must be a symbol

         if (pgn->token_type != TOKEN_SYMBOL) {
            my_fatal("pgn_next_move(): malformed move at line %d, column %d, game %d\n",pgn->token_line,pgn->token_column,pgn->game_nb);
         }

         // store move for later use

         if (depth == 0) {

            if (pgn->token_length >= size) {
               my_fatal("pgn_next_move(): move too long at line %d, column %d, game %d\n",pgn->token_line,pgn->token_column,pgn->game_nb);
            }

            strcpy(string,pgn->token_string);
            pgn->move_line = pgn->token_line;
            pgn->move_column = pgn->token_column;
         }

         // skip optional NAGs

         do pgn_token_read(pgn); while (pgn->token_type == TOKEN_NAG);
         pgn_token_unread(pgn);

         // return move

         if (depth == 0) {
            if (DispMove) printf("move=\"%s\"\n",string);
            return true;
         }
      }
   }

   ASSERT(false);

   return false;
}

// pgn_token_read()

static void pgn_token_read(pgn_t * pgn) {

   ASSERT(pgn!=NULL);

   // token "stack"

   if (pgn->token_unread) {
      pgn->token_unread = false;
      return;
   }

   // consume the current token

   if (pgn->token_first) {
      pgn->token_first = false;
   } else {
      ASSERT(pgn->token_type!=TOKEN_ERROR);
      ASSERT(pgn->token_type!=TOKEN_EOF);
   }

   // read a new token

   pgn_read_token(pgn);
   if (pgn->token_type == TOKEN_ERROR) my_fatal("pgn_token_read(): lexical error at line %d, column %d, game %d\n",pgn->char_line,pgn->char_column,pgn->game_nb);

   if (DispToken) printf("< L%d C%d \"%s\" (%03X)\n",pgn->token_line,pgn->token_column,pgn->token_string,pgn->token_type);
}

// pgn_token_unread()

static void pgn_token_unread(pgn_t * pgn) {

   ASSERT(pgn!=NULL);

   ASSERT(!pgn->token_unread);
   ASSERT(!pgn->token_first);

   pgn->token_unread = true;
}

// pgn_read_token()

static void pgn_read_token(pgn_t * pgn) {

   ASSERT(pgn!=NULL);

   // skip white-space characters

   pgn_skip_blanks(pgn);

   // init

   pgn->token_type = TOKEN_ERROR;
   strcpy(pgn->token_string,"");
   pgn->token_length = 0;
   pgn->token_line = pgn->char_line;
   pgn->token_column = pgn->char_column;

   // determine token type

   if (false) {

   } else if (pgn->char_hack == CHAR_EOF) {

      pgn->token_type = TOKEN_EOF;

   } else if (strchr(".[]()<>",pgn->char_hack) != NULL) {

      // single-character token

      pgn->token_type = pgn->char_hack;
      sprintf(pgn->token_string,"%c",pgn->char_hack);
      pgn->token_length = 1;

   } else if (pgn->char_hack == '*') {

      pgn->token_type = TOKEN_RESULT;
      sprintf(pgn->token_string,"%c",pgn->char_hack);
      pgn->token_length = 1;

   } else if (pgn->char_hack == '!') {

      pgn_char_read(pgn);

      if (false) {

      } else if (pgn->char_hack == '!') { // "!!"

         pgn->token_type = TOKEN_NAG;
         strcpy(pgn->token_string,"3");
         pgn->token_length = 1;

      } else if (pgn->char_hack == '?') { // "!?"

         pgn->token_type = TOKEN_NAG;
         strcpy(pgn->token_string,"5");
         pgn->token_length = 1;

      } else { // "!"

         pgn_char_unread(pgn);

         pgn->token_type = TOKEN_NAG;
         strcpy(pgn->token_string,"1");
         pgn->token_length = 1;
      }

   } else if (pgn->char_hack == '?') {

      pgn_char_read(pgn);

      if (false) {

      } else if (pgn->char_hack == '?') { // "??"

         pgn->token_type = TOKEN_NAG;
         strcpy(pgn->token_string,"4");
         pgn->token_length = 1;

      } else if (pgn->char_hack == '!') { // "?!"

         pgn->token_type = TOKEN_NAG;
         strcpy(pgn->token_string,"6");
         pgn->token_length = 1;

      } else { // "?"

         pgn_char_unread(pgn);

         pgn->token_type = TOKEN_NAG;
         strcpy(pgn->token_string,"2");
         pgn->token_length = 1;
      }

   } else if (is_symbol_start(pgn->char_hack)) {

      // symbol, integer, or result

      pgn->token_type = TOKEN_INTEGER;
      pgn->token_length = 0;

      do {

         if (pgn->token_length >= PGN_STRING_SIZE-1) {
            my_fatal("pgn_read_token(): symbol too long at line %d, column %d,game %d\n",pgn->char_line,pgn->char_column,pgn->game_nb);
         }

         if (!isdigit(pgn->char_hack)) pgn->token_type = TOKEN_SYMBOL;

         pgn->token_string[pgn->token_length++] = pgn->char_hack;

         pgn_char_read(pgn);

      } while (is_symbol_next(pgn->char_hack));

      pgn_char_unread(pgn);

      ASSERT(pgn->token_length>0&&pgn->token_length<PGN_STRING_SIZE);
      pgn->token_string[pgn->token_length] = '\0';

      if (my_string_equal(pgn->token_string,"1-0")
       || my_string_equal(pgn->token_string,"0-1")
       || my_string_equal(pgn->token_string,"1/2-1/2")) {
         pgn->token_type = TOKEN_RESULT;
      }

   } else if (pgn->char_hack == '"') {

      // string

      pgn->token_type = TOKEN_STRING;
      pgn->token_length = 0;

      while (true) {

         pgn_char_read(pgn);

         if (pgn->char_hack == CHAR_EOF) {
            my_fatal("pgn_read_token(): EOF in string at line %d, column %d, game %d\n",pgn->char_line,pgn->char_column,pgn->game_nb);
         }

         if (pgn->char_hack == '"') break;

         if (pgn->char_hack == '\\') {

            pgn_char_read(pgn);

            if (pgn->char_hack == CHAR_EOF) {
               my_fatal("pgn_read_token(): EOF in string at line %d, column %d, game %d\n",pgn->char_line,pgn->char_column,pgn->game_nb);
            }

            if (pgn->char_hack != '"' && pgn->char_hack != '\\') {

               // bad escape, ignore

               if (pgn->token_length >= PGN_STRING_SIZE-1) {
                  my_fatal("pgn_read_token(): string too long at line %d, column %d,game %d\n",pgn->char_line,pgn->char_column,pgn->game_nb);
               }

               pgn->token_string[pgn->token_length++] = '\\';
            }
         }

         if (pgn->token_length >= PGN_STRING_SIZE-1) {
            my_fatal("pgn_read_token(): string too long at line %d, column %d,game %d\n",pgn->char_line,pgn->char_column,pgn->game_nb);
         }

         pgn->token_string[pgn->token_length++] = pgn->char_hack;
      }

      ASSERT(pgn->token_length>=0&&pgn->token_length<PGN_STRING_SIZE);
      pgn->token_string[pgn->token_length] = '\0';

   } else if (pgn->char_hack == '$') {

      // NAG

      pgn->token_type = TOKEN_NAG;
      pgn->token_length = 0;

      while (true) {

         pgn_char_read(pgn);

         if (!isdigit(pgn->char_hack)) break;

         if (pgn->token_length >= 3) {
            my_fatal("pgn_read_token(): NAG too long at line %d, column %d, game %d\n",pgn->char_line,pgn->char_column,pgn->game_nb);
         }

         pgn->token_string[pgn->token_length++] = pgn->char_hack;
      }

      pgn_char_unread(pgn);

      if (pgn->token_length == 0) {
         my_fatal("pgn_read_token(): malformed NAG at line %d, column %d,game %d\n",pgn->char_line,pgn->char_column,pgn->game_nb);
      }

      ASSERT(pgn->token_length>0&&pgn->token_length<=3);
      pgn->token_string[pgn->token_length] = '\0';

   } else {

      // unknown token

       my_fatal("lexical error at line %d, column %d, game %d\n",pgn->char_line,pgn->char_column,pgn->game_nb);
   }
}

// pgn_skip_blanks()

static void pgn_skip_blanks(pgn_t * pgn) {

   ASSERT(pgn!=NULL);

   while (true) {

      pgn_char_read(pgn);

	  if (false) {
	  }else if(pgn->char_hack==CHAR_EOF){ break;
      } else if (isspace(pgn->char_hack)) {

         // skip white space

      } else if (pgn->char_hack == ';') {

         // skip comment to EOL

         do {

            pgn_char_read(pgn);

            if (pgn->char_hack == CHAR_EOF) {
               my_fatal("pgn_skip_blanks(): EOF in comment at line %d, column %d,game %d\n",pgn->char_line,pgn->char_column,pgn->game_nb);
            }

         } while (pgn->char_hack != '\n');

      } else if (pgn->char_hack == '%' && pgn->char_column == 0) {

         // skip comment to EOL

         do {

            pgn_char_read(pgn);

            if (pgn->char_hack == CHAR_EOF) {
               my_fatal("pgn_skip_blanks(): EOF in comment at line %d, column %d, game %d\n",pgn->char_line,pgn->char_column,pgn->game_nb);
            }

         } while (pgn->char_hack != '\n');

      } else if (pgn->char_hack == '{') {

         // skip comment to next '}'

         do {

            pgn_char_read(pgn);

            if (pgn->char_hack == CHAR_EOF) {
               my_fatal("pgn_skip_blanks(): EOF in comment at line %d, column %d, game %d\n",pgn->char_line,pgn->char_column,pgn->game_nb);
            }

         } while (pgn->char_hack != '}');

      } else { // not a white space

         break;
      }
   }
}

// is_symbol_start()

static bool is_symbol_start(int c) {

   return strchr("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789",c) != NULL;
}

// is_symbol_next()

static bool is_symbol_next(int c) {

   return strchr("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789_+#=:-/",c) != NULL;
}

// pgn_char_read()

static void pgn_char_read(pgn_t * pgn) {

   ASSERT(pgn!=NULL);

   // char "stack"

   if (pgn->char_unread) {
      pgn->char_unread = false;
      return;
   }

   // consume the current character

   if (pgn->char_first) {

      pgn->char_first = false;

   } else {

      // update counters

      ASSERT(pgn->char_hack!=CHAR_EOF);

      if (false) {
      } else if (pgn->char_hack == '\n') {
         pgn->char_line++;
         pgn->char_column = 0;
      } else if (pgn->char_hack == '\t') {
         pgn->char_column += TAB_SIZE - (pgn->char_column % TAB_SIZE);
      } else {
         pgn->char_column++;
      }
   }

   // read a new character

   pgn->char_hack = fgetc(pgn->file);

   if (pgn->char_hack == EOF) {
      if (ferror(pgn->file)) my_fatal("pgn_char_read(): fgetc(): %s\n",strerror(errno));
      pgn->char_hack = CHAR_EOF;
   }

   if (DispChar) printf("< L%d C%d '%c' (%02X)\n",pgn->char_line,pgn->char_column,pgn->char_hack,pgn->char_hack);
}

// pgn_char_unread()

static void pgn_char_unread(pgn_t * pgn) {

   ASSERT(pgn!=NULL);

   ASSERT(!pgn->char_unread);
   ASSERT(!pgn->char_first);

   pgn->char_unread = true;
}

// end of pgn.cpp

