
// book_make.cpp

// includes

#include <cerrno>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "board.h"
#include "book_make.h"
#include "move.h"
#include "move_do.h"
#include "move_legal.h"
#include "pgn.h"
#include "san.h"
#include "util.h"
#include "fen.h" 
// constants

static const int COUNT_MAX = 16384;

static const int NIL = -1;

// types

struct entry_t {
   uint64 key;
   uint16 move;
   uint16 n;
   uint16 sum;
   uint16 colour;
};

struct book_t {
   int size;
   int alloc;
   uint32 mask;
   entry_t * entry;
   sint32 * hash;
};

// variables

static int MaxPly;
static int MinGame;
static double MinScore;
static bool RemoveWhite, RemoveBlack;
static bool Uniform;

static book_t Book[1];

// prototypes

static void   book_clear    ();
static void   book_insert   (const char file_name[]);
static void   book_filter   ();
static void   book_sort     ();
static void   book_save     (const char file_name[]);

static int    find_entry    (const board_t * board, int move);
static void   resize        ();
static void   halve_stats   (uint64 key);

static bool   keep_entry    (int pos);

static int    entry_score    (const entry_t * entry);

static int    key_compare   (const void * p1, const void * p2);

static void   write_integer (FILE * file, int size, uint64 n);

// functions

// book_make()

void book_make(int argc, char * argv[]) {

   int i;
   const char * pgn_file;
   const char * bin_file;

   pgn_file = NULL;
   my_string_set(&pgn_file,"book.pgn");

   bin_file = NULL;
   my_string_set(&bin_file,"book.bin");

   MaxPly = 1024;
   MinGame = 3;
   MinScore = 0.0;
   RemoveWhite = false;
   RemoveBlack = false;
   Uniform = false;

   for (i = 1; i < argc; i++) {

      if (false) {

      } else if (my_string_equal(argv[i],"make-book")) {

         // skip

      } else if (my_string_equal(argv[i],"-pgn")) {

         i++;
         if (argv[i] == NULL) my_fatal("book_make(): missing argument\n");

         my_string_set(&pgn_file,argv[i]);

      } else if (my_string_equal(argv[i],"-bin")) {

         i++;
         if (argv[i] == NULL) my_fatal("book_make(): missing argument\n");

         my_string_set(&bin_file,argv[i]);

      } else if (my_string_equal(argv[i],"-max-ply")) {

         i++;
         if (argv[i] == NULL) my_fatal("book_make(): missing argument\n");

         MaxPly = atoi(argv[i]);
         ASSERT(MaxPly>=0);

      } else if (my_string_equal(argv[i],"-min-game")) {

         i++;
         if (argv[i] == NULL) my_fatal("book_make(): missing argument\n");

         MinGame = atoi(argv[i]);
         ASSERT(MinGame>0);

      } else if (my_string_equal(argv[i],"-min-score")) {

         i++;
         if (argv[i] == NULL) my_fatal("book_make(): missing argument\n");

         MinScore = atof(argv[i]) / 100.0;
         ASSERT(MinScore>=0.0&&MinScore<=1.0);

      } else if (my_string_equal(argv[i],"-only-white")) {

         RemoveWhite = false;
         RemoveBlack = true;

      } else if (my_string_equal(argv[i],"-only-black")) {

         RemoveWhite = true;
         RemoveBlack = false;

      } else if (my_string_equal(argv[i],"-uniform")) {

         Uniform = true;

      } else {

         my_fatal("book_make(): unknown option \"%s\"\n",argv[i]);
      }
   }

   book_clear();

   printf("inserting games ...\n");
   book_insert(pgn_file);

   printf("filtering entries ...\n");
   book_filter();

   printf("sorting entries ...\n");
   book_sort();

   printf("saving entries ...\n");
   book_save(bin_file);

   printf("all done!\n");
}

// book_clear()

static void book_clear() {

   int index;

   Book->alloc = 1;
   Book->mask = (Book->alloc * 2) - 1;

   Book->entry = (entry_t *) my_malloc(Book->alloc*sizeof(entry_t));
   Book->size = 0;

   Book->hash = (sint32 *) my_malloc((Book->alloc*2)*sizeof(sint32));
   for (index = 0; index < Book->alloc*2; index++) {
      Book->hash[index] = NIL;
   }
}

// book_insert()

static void book_insert(const char file_name[]) {

   pgn_t pgn[1];
   board_t board[1];
   int ply;
   int result;
   char string[256];
   int move;
   int pos;

   ASSERT(file_name!=NULL);

   // init

   pgn->game_nb=1;
   // scan loop

   pgn_open(pgn,file_name);
   while (pgn_next_game(pgn)) {

      board_start(board);
      ply = 0;
      result = 0;

	  if(strlen(pgn->fen)>0) //we've got FEN !
	  {
		  board_from_fen(board,pgn->fen);
		  //convert move number to ply number
		  ply=(board->move_nb-1)/2;
		  if(board->turn==Black) ply++;
	  }
      if (false) {
      } else if (my_string_equal(pgn->result,"1-0")) {
         result = +1;
      } else if (my_string_equal(pgn->result,"0-1")) {
         result = -1;
      }

      while (pgn_next_move(pgn,string,sizeof(string))) {

         if (ply < MaxPly) {

            move = move_from_san(string,board);

            if (move == MoveNone || !move_is_legal(move,board)) {
               my_fatal("book_insert(): illegal move \"%s\" at line %d, column %d,game %d\n",string,pgn->move_line,pgn->move_column,pgn->game_nb);
            }

            pos = find_entry(board,move);

            Book->entry[pos].n++;
            Book->entry[pos].sum += (uint16)(result+1);

            if (Book->entry[pos].n >= COUNT_MAX) {
               halve_stats(board->key);
            }

            move_do(board,move);
            ply++;
            result = -result;
         }
      }
	  pgn->game_nb++;
      if (pgn->game_nb % 10000 == 0) printf("%d games ...\n",pgn->game_nb);
   }

   pgn_close(pgn);

   printf("%d game%s.\n",pgn->game_nb,(pgn->game_nb>2)?"s":"");
   printf("%d entries.\n",Book->size);

   return;
}

// book_filter()

static void book_filter() {

   int src, dst;

   // entry loop

   dst = 0;

   for (src = 0; src < Book->size; src++) {
      if (keep_entry(src)) Book->entry[dst++] = Book->entry[src];
   }

   ASSERT(dst>=0&&dst<=Book->size);
   Book->size = dst;

   printf("%d entries.\n",Book->size);
}

// book_sort()

static void book_sort() {

   // sort keys for binary search

   qsort(Book->entry,Book->size,sizeof(entry_t),&key_compare);
}

// book_save()

static void book_save(const char file_name[]) {

   FILE * file;
   int pos;

   ASSERT(file_name!=NULL);

   file = fopen(file_name,"wb");
   if (file == NULL) my_fatal("book_save(): can't open file \"%s\" for writing: %s\n",file_name,strerror(errno));

   // entry loop

   for (pos = 0; pos < Book->size; pos++) {

      ASSERT(keep_entry(pos));

      write_integer(file,8,Book->entry[pos].key);
      write_integer(file,2,Book->entry[pos].move);
      write_integer(file,2,entry_score(&Book->entry[pos]));
      write_integer(file,2,0);
      write_integer(file,2,0);
   }

   fclose(file);
}

// find_entry()

static int find_entry(const board_t * board, int move) {

   uint64 key;
   int index;
   int pos;

   ASSERT(board!=NULL);
   ASSERT(move_is_ok(move));

   ASSERT(move_is_legal(move,board));

   // init

   key = board->key;

   // search

   for (index = (int)(key & Book->mask); (pos=Book->hash[index]) != NIL; index = (index+1) & Book->mask) {

      ASSERT(pos>=0&&pos<Book->size);

      if (Book->entry[pos].key == key && Book->entry[pos].move == move) {
         return pos; // found
      }
   }

   // not found

   ASSERT(Book->size<=Book->alloc);

   if (Book->size == Book->alloc) {

      // allocate more memory

      resize();

      for (index = (int)(key & Book->mask); Book->hash[index] != NIL; index = (index+1) & Book->mask)
         ;
   }

   // create a new entry

   ASSERT(Book->size<Book->alloc);
   pos = Book->size++;

   Book->entry[pos].key = key;
   Book->entry[pos].move = (uint16)move;
   Book->entry[pos].n = 0;
   Book->entry[pos].sum = 0;
   Book->entry[pos].colour = board->turn;

   // insert into the hash table

   ASSERT(index>=0&&index<Book->alloc*2);
   ASSERT(Book->hash[index]==NIL);
   Book->hash[index] = pos;

   ASSERT(pos>=0&&pos<Book->size);

   return pos;
}

// resize()

static void resize() {

   int size;
   int pos;
   int index;

   ASSERT(Book->size==Book->alloc);

   Book->alloc *= 2;
   Book->mask = (Book->alloc * 2) - 1;

   size = 0;
   size += Book->alloc * sizeof(entry_t);
   size += (Book->alloc*2) * sizeof(sint32);
   if (size >= 1048576) printf("allocating %gMB ...\n",double(size)/1048576.0);
   // resize arrays

   Book->entry = (entry_t *) my_realloc(Book->entry,Book->alloc*sizeof(entry_t));
   Book->hash = (sint32 *) my_realloc(Book->hash,(Book->alloc*2)*sizeof(sint32));

   // rebuild hash table

   for (index = 0; index < Book->alloc*2; index++) {
      Book->hash[index] = NIL;
   }

   for (pos = 0; pos < Book->size; pos++) {

      for (index = (int) (Book->entry[pos].key & Book->mask)
		   ; Book->hash[index] != NIL; index = (index+1) & Book->mask)
         ;

      ASSERT(index>=0&&index<Book->alloc*2);
      Book->hash[index] = pos;
   }
}

// halve_stats()

static void halve_stats(uint64 key) {

   int index;
   int pos;

   // search

   for (index = (int)(key & Book->mask); (pos=Book->hash[index]) != NIL; index = (index+1) & Book->mask) {

      ASSERT(pos>=0&&pos<Book->size);

      if (Book->entry[pos].key == key) {
         Book->entry[pos].n = (Book->entry[pos].n + 1) / 2;
         Book->entry[pos].sum = (Book->entry[pos].sum + 1) / 2;
      }
   }
}

// keep_entry()

static bool keep_entry(int pos) {

   const entry_t * entry;
   int colour;
   double score;

   ASSERT(pos>=0&&pos<Book->size);

   entry = &Book->entry[pos];

   // if (entry->n == 0) return false;
   if (entry->n < MinGame) return false;

   if (entry->sum == 0) return false;

   score = (double(entry->sum) / double(entry->n)) / 2.0;
   ASSERT(score>=0.0&&score<=1.0);

   if (score < MinScore) return false;

   colour = entry->colour;

   if ((RemoveWhite && colour_is_white(colour))
    || (RemoveBlack && colour_is_black(colour))) {
      return false;
   }

   if (entry_score(entry) == 0) return false; // REMOVE ME?

   return true;
}

// entry_score()

static int entry_score(const entry_t * entry) {

   int score;

   ASSERT(entry!=NULL);

   // score = entry->n; // popularity
   score = entry->sum; // "expectancy"

   if (Uniform) score = 1;

   ASSERT(score>=0);

   return score;
}

// key_compare()

static int key_compare(const void * p1, const void * p2) {

   const entry_t * entry_1, * entry_2;

   ASSERT(p1!=NULL);
   ASSERT(p2!=NULL);

   entry_1 = (const entry_t *) p1;
   entry_2 = (const entry_t *) p2;

   if (entry_1->key > entry_2->key) {
      return +1;
   } else if (entry_1->key < entry_2->key) {
      return -1;
   } else {
      return entry_score(entry_2) - entry_score(entry_1); // highest score first
   }
}

// write_integer()

static void write_integer(FILE * file, int size, uint64 n) {

   int i;
   int b;

   ASSERT(file!=NULL);
   ASSERT(size>0&&size<=8);
   ASSERT(size==8||n>>(size*8)==0);

   for (i = size-1; i >= 0; i--) {
      b = (int)((n >> (i*8)) & 0xFF);
      ASSERT(b>=0&&b<256);
      fputc(b,file);
   }
}

// end of book_make.cpp

