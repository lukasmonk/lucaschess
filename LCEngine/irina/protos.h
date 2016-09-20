#ifndef IRINA_PROTOS_H
#define IRINA_PROTOS_H

#include "defs.h"
#include "hash.h"

// util.c
unsigned int bit_count(Bitmap bitmap);
unsigned int first_one(Bitmap bitmap);
int ah_pos(char *ah);
Bitmap get_ms(void);
bool bioskey(void);
char *move2str(Move move, char *str_dest);

// test.c
void test(void);
char *strip(char *txt);

void xmove(Move move);
void xbitmap(Bitmap bm);
void xfen(void);

void xm(const char *fmt, ...);
void xl(void);
void xt(int num);

void show_move(Move move);
int move_num(Move move);
Move num_move(int num);
void show_bitmap(Bitmap bm);
void show_4bitmap(Bitmap bm1, Bitmap bm2, Bitmap bm3, Bitmap bm4);
void show_move(Move move);
bool equal_boards(Board b0, Board b1, Board b2, Move mv);
Bitmap calc_perft(char *fen, int depth);
void perft(int depth);
void perft_file(char * file);

// eval.c
int eval(void);
void set_level(int lv);

// loop.c
void begin(void);
void loop(void);
void set_position(char *line);
void go(char *line);

// data.c
void init_data(void);

// board.c
void init_board(void);
void board_reset(void);
void fen_board(char *fen);
void bitmap_pz(unsigned pz[], Bitmap bm, int piece);
char *board_fen(char *fen);
char *board_fenM2(char *fen);
Bitmap board_hashkey(void);

// movegen.c
int movegen(void);
void addMove(Move move);
bool isAttacked(Bitmap targetBitmap, int fromSide);
bool inCheck(void);
bool inCheckOther(void);
unsigned int movegenCaptures(void);

int movegen_piece(unsigned piece);
int movegen_piece_to(int piece, unsigned xto);

// makemove.c
void make_move(Move move);
void unmake_move(void);

// search.c
char * play(int depth, int time);
int alphaBeta(int alpha, int beta, int depthleft, int ply);
int quiescence(int alpha, int beta, int ply);


// hash.c
Bitmap rand64();
void init_hash();

// lc.c
int pgn2pv(char *pgn, char * pv);
int make_nummove(int num);
char * playFen( char * fen, int depth, int time );
int numMoves( void );
void getMove( int num, char * pv );
int numBaseMove( void );
int searchMove( char *desde, char *hasta, char * promotion );
void getMoveEx( int num, char * info );
char * toSan(int num, char *sanMove);

#endif
