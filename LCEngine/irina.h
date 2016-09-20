#ifndef IRINA_DEFS_H
#define IRINA_DEFS_H

typedef struct
{
   unsigned from      : 6;
   unsigned to        : 6;
   unsigned piece     : 4;
   unsigned capture   : 4;
   unsigned promotion : 4;
   unsigned is_ep     : 1;
   unsigned is_2p     : 1;
   unsigned is_castle : 2;
} Move;

void init_board();
void fen_board(char *fen);
int movegen(void);
int pgn2pv(char *pgn, char * pv);
int make_nummove(int resp);
char * playFen(char * fen, int depth, int time);
int numMoves( );
void getMove( int num, char * pv );

char *board_fen(char *fen);

int numBaseMove( void );
int searchMove( char *desde, char *hasta, char * promotion );
void getMoveEx( int num, char * info );
char * toSan(int num, char *sanMove);
char inCheck(void);
void set_level(int lv);

void pgn_start(char * fich, int depth);
void pgn_stop( void );
int pgn_read( void );
char * pgn_game(void);
char * pgn_pv(void);
int pgn_numlabels(void);
char * pgn_label(int num);
char * pgn_value(int num);
int pgn_raw(void);
int pgn_numfens(void);
char * pgn_fen(int num);


#endif
