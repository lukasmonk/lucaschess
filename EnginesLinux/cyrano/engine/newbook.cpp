
// book.cpp
// from Polyglot
//      ========
// includes

#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <math.h>
#include <time.h>
#include "engine.hpp"
#include "board.hpp"
#include "moves.hpp"
#include "genmagic.hpp"
#include "gproto.hpp"
#include "random.hpp"
#include "newbook.hpp"

// types

#define ASSERT(a)   Assert(a)
#define MoveNone    0

typedef unsigned short int uint16;
typedef U64 uint64;

struct entry_t {
   uint64 key;
   uint16 move;
   uint16 count;
   uint16 n;
   uint16 sum;
};

// variables

static FILE * BookFile = 0;
static int BookSize = 0;

// prototypes

static int    find_pos      (uint64 key);

static void   read_entry    (entry_t * entry, int n);
//static void   write_integer (FILE * file, int size, uint64 n);
//static void   write_entry   (const entry_t * entry, int n);

static uint64 read_integer  (FILE * file, int size);

#ifdef _DEBUG
static char const s_aszModule[] = __FILE__;
#endif

static void hash_init();
static uint64 hash_key(PCON pcon, PSTE pste);

// functions

// book_clear()

void book_clear(void) {


    random_init();
    hash_init();
    srand(clock());

    BookFile = NULL;
    BookSize = 0;
}

bool book_isopen(void) {
    return (BookFile != NULL) && (BookSize > 0);
}
// book_open()

void book_open(const char file_name[]) {

   ASSERT(file_name!=NULL);

   if( BookFile )
       book_close();

   BookFile = fopen(file_name,"rb+");
   if (BookFile == NULL) {
       VPrSendComment("book_open(): can't open file \"%s\": %s\n",file_name,strerror(errno));
       return;
   }

   if (fseek(BookFile,0,SEEK_END) == -1) {
      VPrSendComment("book_open(): fseek(): %s\n",strerror(errno));
      return;
   }

    BookSize = ftell(BookFile) / 16;
    if (BookSize <= 0)
        VPrSendComment("book_open(): empty file\n");
    else
        VPrSendComment("%d book positions in %s", BookSize, file_name);

}

// book_close()

void book_close() {

    BookSize = 0;
    if (BookFile && fclose(BookFile) == EOF) {
        VPrSendComment("book_close(): fclose(): %s\n",strerror(errno));
    }
}

// my_random() from Fruit

static int my_random(int n) {

   double r = double(rand()) / (double(RAND_MAX) + 1.0);

   return int(floor(r*double(n)));
}

// is_in_book()
/*
bool is_in_book(const board_t * board) {

   int pos;
   entry_t entry[1];

   ASSERT(board!=NULL);

   for (pos = find_pos(board->key); pos < BookSize; pos++) {
      read_entry(entry,pos);
      if (entry->key == board->key) return true;
   }

   return false;
}
*/

// book_move()

static const int PromotePiece[5] = { 0, PROM_N, PROM_B, PROM_R, PROM_Q };

//	m = ((from) | ((to) << 6) | ((type) << 12) | ((capt) << 16) | (PROM_N << 20));
//	m = ((from) | ((to) << 6) | ((pc) << 12));

U32 book_convert_polymove(PCON pcon, PSTE pste, uint16 move) {
//   return (square_to_64(from) << 6) | square_to_64(to);
    int from = (move >> 6) & 0x3f;
    int to   = move & 0x3f;
    int code = (move >> 12) & 0x0f;
    int prom = 0;
    int pc   = pcon->pos[from];
    int capt = 0;
    int side = pste->side;

    // prom == 1..4
    if( code >= 1 && code <= 4 ) {
        pc = PROM|side;
        prom = PromotePiece[code];
    }
    if( pcon->pos[to] != 0 ) {
        if( (pcon->pos[to] & 1) == (side & 1) ) {
            pc = CASTLE|side;
            if (from == E1 && to == H1) {
                to = G1;
            } else if (from == E1 && to == A1) {
                to = C1;
            } else if (from == E8 && to == H8) {
                to = G8;
            } else if (from == E8 && to == A8) {
                to = C8;
            }
        } else
            capt = pcon->pos[to];
    }
    if( ((pc|1) == (BP|1)) && to == pste->ep_t ) {
        capt = EP_CAPT|side;
    }
    int m = ((from) | ((to) << 6) | ((pc) << 12) | ((capt) << 16) | (prom << 20));
    return m;
}

bool book_find_move(PCON pcon, CM *cm, bool random) {

    if(!book_isopen())
        return false;

    PSTE pste = pcon->argste;
    uint64 board_key = hash_key(pcon, pste);

   int best_move = MoveNone;
   int best_score = 0;
   int min_score = 0;

   for (int pos = find_pos(board_key); pos < BookSize; pos++) {

      entry_t entry[1];
      read_entry(entry,pos);
      if (entry->key != board_key) break;

      uint16 move = entry->move;
      int score = entry->count;

      if (move != MoveNone) {
            U32 m = book_convert_polymove(pcon,pste,move);
            if(move_is_legal(pcon,pste,m)) {

                // pick this move?

                ASSERT(score>0);
                if( best_score == 0 )
                    min_score = score / 3;

                if (true && random) {
                    char move_string[256];
                    CM cm;
                    cm.m = m;
                    CmToSz( &cm, move_string );

                    best_score += score;
                    int rand_score = my_random(best_score);
                    printf("# %s score=%8d rand_score(%d)=%8d\n", move_string, score, best_score, rand_score);
                    if ( (rand_score < score) && (score >= min_score) ) {
                        best_move = m;
                    }
                } else {
                    if (score > best_score) {
                        best_move = m;
                        best_score = score;
                    }
                }
            }
      }
   }
    if( best_move == MoveNone )
        return false;
   cm->m = best_move;
   cm->cmk = 0;
   return true;
}

// book_disp()


void book_disp(PCON pcon, PSTE pste) {

   int first_pos;
   int sum;
   int pos;
   entry_t entry[1];
   int move;
   int score;
   char move_string[256];
    
    if(!book_isopen())
        return;

    uint64 board_key = hash_key(pcon, pste);

   first_pos = find_pos(board_key);

   // sum

   sum = 0;

   for (pos = first_pos; pos < BookSize; pos++) {

      read_entry(entry,pos);
      if (entry->key != board_key) break;

      sum += entry->count;
   }

   // disp
   int counter = 0;
   for (pos = first_pos; pos < BookSize; pos++) {

      read_entry(entry,pos);
      if (entry->key != board_key) break;
      if( entry->move != MoveNone ) {
        move = book_convert_polymove(pcon, pste, entry->move);
        score = entry->count;
#if 0
        if (score > 0 && move_is_legal(pcon,pste,move)) {
            CM cm;
            cm.m = move;
            CmToSz( &cm, move_string );
            float res = 0.0f;
            if( entry->n > 0 )
                res = 50.0f + 100.0f*(entry->sum - entry->n) / entry->n;
            if( res < 0.0f )
                res = 0.0f;
            float g = float(score) / float(sum);
            float potgain = res / 100.0f * entry->n;
//            float potgain2 = logf(1.0f + res / 100.0f) * entry->n;
            float potgain2 = 100.0f * logf(1.0f + g) / logf(2.0f);
            printf(" %s (%.0f%%)\t%d\tn=%d\ts=%d\tres=%3.1f\t%5.1f\t%5.1f\n",move_string,g*100.0, entry->count, entry->n, entry->sum, res, potgain, potgain2 );
            counter++;
        }
#else
        if (move_is_legal(pcon,pste,move)) {
            CM cm;
            cm.m = move;
            CmToSz( &cm, move_string );
            printf(" %s \tcount=%8d\t n=%8d\t sum=%8d\t %3.1f\n",move_string, entry->count, entry->n, entry->sum, entry->sum*50.0f/entry->n);
            counter++;
        }
#endif
      }
   }

   printf("%d moves\n", counter);
}


// book_learn_move()

/*
void book_learn_move(const board_t * board, int move, int result) {

   int pos;
   entry_t entry[1];

   ASSERT(board!=NULL);
   ASSERT(move_is_ok(move));
   ASSERT(result>=-1&&result<=+1);

   ASSERT(move_is_legal(move,board));

   for (pos = find_pos(board->key); pos < BookSize; pos++) {

      read_entry(entry,pos);
      if (entry->key != board->key) break;

      if (entry->move == move) {

         entry->n++;
         entry->sum += result+1;

         write_entry(entry,pos);

         break;
      }
   }
}
*/

// book_flush()

void book_flush() {

   if (fflush(BookFile) == EOF) {
      VPrSendComment("book_flush(): fflush(): %s\n",strerror(errno));
   }
}

// find_pos()

static int find_pos(uint64 key) {

   int left, right, mid;
   entry_t entry[1];

   // binary search (finds the leftmost entry)

   left = 0;
   right = BookSize-1;

   ASSERT(left<=right);

   while (left < right) {

      mid = (left + right) / 2;
      ASSERT(mid>=left&&mid<right);

      read_entry(entry,mid);

      if (key <= entry->key) {
         right = mid;
      } else {
         left = mid+1;
      }
   }

   ASSERT(left==right);

   read_entry(entry,left);

   return (entry->key == key) ? left : BookSize;
}

// read_entry()

static void read_entry(entry_t * entry, int n) {

   ASSERT(entry!=NULL);
   ASSERT(n>=0&&n<BookSize);

   if (fseek(BookFile,n*16,SEEK_SET) == -1) {
      VPrSendComment("read_entry(): fseek(): %s\n",strerror(errno));
      entry->move = 0;
      return;
   }

   entry->key   = read_integer(BookFile,8);
   entry->move  = uint16( read_integer(BookFile,2) );
   entry->count = uint16( read_integer(BookFile,2) );
   entry->n     = uint16( read_integer(BookFile,2) );
   entry->sum   = uint16( read_integer(BookFile,2) );
}

// write_entry()
#if 0
// write_integer()

static void write_integer(FILE * file, int size, uint64 n) {

   int i;
   int b;

   ASSERT(file!=NULL);
   ASSERT(size>0&&size<=8);
   ASSERT(size==8||n>>(size*8)==0);

   for (i = size-1; i >= 0; i--) {

      b = int( (n >> (i*8)) & 0xFF );
      ASSERT(b>=0&&b<256);

      if (fputc(b,file) == EOF) {
         VPrSendComment("write_integer(): fputc(): %s\n",strerror(errno));
      }
   }
}


static void write_entry(const entry_t * entry, int n) {

   ASSERT(entry!=NULL);
   ASSERT(n>=0&&n<BookSize);

   if (fseek(BookFile,n*16,SEEK_SET) == -1) {
      VPrSendComment("write_entry(): fseek(): %s\n",strerror(errno));
      return;
   }

   write_integer(BookFile,8,entry->key);
   write_integer(BookFile,2,entry->move);
   write_integer(BookFile,2,entry->count);
   write_integer(BookFile,2,entry->n);
   write_integer(BookFile,2,entry->sum);
}
#endif
// read_integer()

static uint64 read_integer(FILE * file, int size) {

   uint64 n;
   int i;
   int b;

   ASSERT(file!=NULL);
   ASSERT(size>0&&size<=8);

   n = 0;

   for (i = 0; i < size; i++) {

      b = fgetc(file);

      if (b == EOF) {
         if (feof(file)) {
            VPrSendComment("read_integer(): fgetc(): EOF reached\n");
         } else { // error
            VPrSendComment("read_integer(): fgetc(): %s\n",strerror(errno));
         }
         return 0;
      }

      ASSERT(b>=0&&b<256);
      n = (n << 8) | b;
   }

   return n;
}

// end of book.cpp

// variables

static uint64 Castle64[16];

// constants

const int RandomPiece     =   0; // 12 * 64
const int RandomCastle    = 768; // 4
const int RandomEnPassant = 772; // 8
const int RandomTurn      = 780; // 1

// prototypes

static uint64 hash_castle_key_debug (int flags);

// functions

// hash_init()

static int PieceTo12[12] = {
//  BK, WK, BQ, WQ, BR, WR, BB, WB, BN, WN, BP, WP
    10, 11,  8,  9,  6,  7,  4,  5,  2,  3,  0, 1
};
/*
const int BlackPawn12   =  0;
const int WhitePawn12   =  1;
const int BlackKnight12 =  2;
const int WhiteKnight12 =  3;
const int BlackBishop12 =  4;
const int WhiteBishop12 =  5;
const int BlackRook12   =  6;
const int WhiteRook12   =  7;
const int BlackQueen12  =  8;
const int WhiteQueen12  =  9;
const int BlackKing12   = 10;
const int WhiteKing12   = 11;
*/
static void hash_init() {

   int i;

   for (i = 0; i < 16; i++) Castle64[i] = hash_castle_key_debug(i);
//   for (int piece = 0; piece < 12; piece++) {
//      PieceTo12[PieceFrom12[piece]] = piece;
//   }

}

// hash_piece_key()

static uint64 hash_piece_key(int piece, int square) {

   return random_64(RandomPiece+PieceTo12[piece]*64+square);
}

// hash_ep_key()

static uint64 hash_ep_key(int square) {

   return random_64(RandomEnPassant+File(square));
}

// hash_castle_key()

static uint64 hash_castle_key(int flags) {

    uint64 result = 0ULL;

    if( flags & cfE1G1 )
        result ^= random_64(RandomCastle+0);
    if( flags & cfE1C1 )
        result ^= random_64(RandomCastle+1);
    if( flags & cfE8G8 )
        result ^= random_64(RandomCastle+2);
    if( flags & cfE8C8 )
        result ^= random_64(RandomCastle+3);
    return result;

}

// hash_castle_key_debug()

static uint64 hash_castle_key_debug(int flags) {

   uint64 key;
   int i;

   ASSERT((flags&~0xF)==0);

   key = 0;

   for (i = 0; i < 4; i++) {
      if ((flags & (1<<i)) != 0) key ^= random_64(RandomCastle+i);
   }

   return key;
}

// hash_turn_key()

static uint64 hash_turn_key(int colour) {


   return (colour == W) ? random_64(RandomTurn) : 0;
}


// hash_key()

static uint64 hash_key(PCON pcon, PSTE pste) {

   // init

   uint64 key = 0;

   // pieces
    const U8 *pos = pcon->pos;
    // for each piece of the board...
	for (bitboard pieces = pcon->pieces[OCCU]; pieces; ) {
		int sqr = PopLSB(pieces);
		int pc = pos[sqr];
        key ^= hash_piece_key(pc,sqr);
    }


   // castle flags

   key ^= hash_castle_key(pste->castle);

   // en-passant square

   int sq = pste->ep_t;
   if (sq) 
       key ^= hash_ep_key(sq);

   // turn

   key ^= hash_turn_key(pste->side);

   return key;
}

// end of hash.cpp


