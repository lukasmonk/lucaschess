#ifndef IRINA_PROTOS_H
#define IRINA_PROTOS_H

#include "defs.h"


// util.c
unsigned int bit_count(Bitmap bitmap);
unsigned int first_one(Bitmap bitmap);
unsigned int last_one(Bitmap bitmap);
int ah_pos(char *ah);
Bitmap get_ms(void);
int bioskey(void);
char *move2str(Move move, char *str_dest);
char *strip(char *txt);
void show_bitmap(Bitmap bm);



// perft.c
Bitmap calc_perft(char *fen, int depth);
void perft(int depth);
void perft_file(char * file);
void test(void);


// eval.c
int eval(void);
int eval_material(void);

// loop.c
void begin(void);
void loop(void);
void set_position(char *line);
void go(char *line);
void set_option(char *line);
void set_Personality(char * value);


// data.c
void init_data(void);

// board.c
void init_board(void);
void board_reset(void);
void fen_board(char *fen);
void bitmap_pz(unsigned pz[], Bitmap bm, int piece);
char *board_fen(char *fen);
Bitmap board_hashkey(void);

// movegen.c
int movegen(void);
void addMove(Move move);
bool isAttacked(Bitmap targetBitmap, int fromSide);
bool inCheck(void);
bool inCheckOther(void);
unsigned int movegenCaptures(void);

// makemove.c
void make_move(Move move);
void unmake_move(void);

// search.c
void play(int depth, int time);
void play_irina(int depth, int time);
int alphaBeta(int alpha, int beta, int depthleft, int ply, int max_ply);
int quiescence(int alpha, int beta, int ply, int max_ply);
int alphaBetaFast(int alpha, int beta, int depth, int ply, int max_ply);
int repetitions(void);

// hash.c
Bitmap rand64();
void init_hash();
void set_hash(char * value);
void hash_save(int val);
bool hash_probe(int * val);

// person.c
void set_personality_name(char * value);
void play_person(int depth, int time);
void set_min_time(char * value);
void set_max_time(char * value);
void person_sleep(Bitmap mstime);
void time_test( Bitmap mstime );

// evalst.c
void init_data_steven(void);
int eval_steven(void);

// book.c
bool using_book(void);
void set_ownbook( bool ok );
void set_ownbookfile( char * name );
void close_book( void );
void open_book();
bool check_book( char * fen, char * move );

// log.c
void open_log();
void close_log();

#endif
