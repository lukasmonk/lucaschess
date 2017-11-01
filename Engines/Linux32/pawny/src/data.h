/*--------------------------------------------------------------------------
    Pawny 1.0, chess engine (source code).
    Copyright (C) 2009 - 2013 by Mincho Georgiev.
    
    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
    
    contact: pawnychess@gmail.com 
    web: http://www.pawny.netii.net/
----------------------------------------------------------------------------*/

#define ENGINE_NAME "Pawny"

#define POPCNT
#define GTB

#if defined(_M_IX64)
#if defined(POPCNT)
#define VERSION "1.0.x64.SSE4.2"
#else
#define VERSION "1.0.x64"
#endif
#else
#define VERSION "1.0.x86"
#endif

#ifndef _CRT_SECURE_NO_DEPRECATE
#define _CRT_SECURE_NO_DEPRECATE
#endif
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <time.h>

#ifndef true
#define true 1
#define false 0
#endif
#ifndef bool
#define bool int
#endif

#ifdef _WIN32
#define llufmt "%I64u"
#else
#define llufmt "%llu"
#endif

#define INF 30000

#ifdef _MSC_VER
#pragma warning(disable:4146)
#endif

enum
{ A1, B1, C1, D1, E1, F1, G1, H1,
  A2, B2, C2 ,D2, E2, F2, G2, H2,
  A3, B3, C3, D3, E3, F3, G3, H3,
  A4, B4, C4, D4, E4, F4, G4, H4,
  A5, B5, C5, D5, E5, F5, G5, H5,
  A6, B6, C6, D6, E6, F6, G6, H6,
  A7, B7, C7, D7, E7, F7, G7, H7,
  A8, B8, C8, D8, E8, F8, G8, H8
};
enum {W, B};
enum {FILE_A, FILE_B, FILE_C, FILE_D, FILE_E, FILE_F, FILE_G, FILE_H};
enum {RANK_1, RANK_2, RANK_3, RANK_4, RANK_5, RANK_6, RANK_7, RANK_8};

enum {PAWN=1, KNIGHT=2, BISHOP=3, ROOK=4, QUEEN=5, KING=6};
enum {WP = 1, WN = 2, WB = 3, WR = 4, WQ = 5, WK = 6, 
      BP = 9, BN = 10, BB = 11, BR = 12, BQ = 13, BK = 14};
enum {OCC_W = 7, OCC_B = 8};

#define EMPTY 0
#define WHITE W
#define BLACK B
#define WHITE_PAWN WP
#define BLACK_PAWN BP
#define WHITE_KNIGHT WN
#define BLACK_KNIGHT BN
#define WHITE_BISHOP WB
#define BLACK_BISHOP BB
#define WHITE_ROOK WR
#define BLACK_ROOK BR
#define WHITE_QUEEN WQ
#define BLACK_QUEEN BQ
#define WHITE_KING WK
#define BLACK_KING BK

///pieces values:
#define P_VALUE 100
#define N_VALUE 321
#define B_VALUE 325
#define R_VALUE 500
#define Q_VALUE 950
#define K_VALUE 1000
#define B_N_DIFF (B_VALUE-N_VALUE)
#define BISHOP_PAIR_REWARD 50

#define WHITE_OO 1
#define WHITE_OOO 2
#define BLACK_OO 4
#define BLACK_OOO 8

#define File(sq)          ((sq) & 7)
#define Rank(sq)          ((sq) >> 3)
#define CalcSq(file,rank) (((rank) << 3) + (file))
#define Piece(sq)         (pos->square[(sq)])                
#define PieceColor(sq)    ((pos->square[(sq)]) >> 3)
#define GetColor(p)       ((p) >> 3) 
#define GetType(p)        ((p) & 7)  
#define Coloured(p,stm)   ((p) | ((stm)   << 3))
#define OpColoured(p,stm) ((p) | (((stm)^1) << 3))
#define Ksq(color)        (bitscanf(pos->occ[KING | ((color)<<3)]))
#define bitset(w, index)  ((w) |=  (1ULL << (index)))

#if defined(_M_IX86)
extern const unsigned __int64 notmask_array[64];
#define bitclear(w, index) ((w) &= notmask_array[(index)])
#define notmask(x) (notmask_array[(x)])
#else
#define bitclear(w, index)((w) &= ~(1ULL << (index)))
#define notmask(x) (~(1ULL << (x)))
#endif

#ifndef min
#define min(a,b) ((a)<(b)?(a):(b))
#endif
#ifndef max
#define max(a,b) ((a)>(b)?(a):(b))
#endif

typedef unsigned __int64 uint64;
typedef uint64 bitboard_t;
typedef uint64 hashkey_t;
typedef unsigned int uint32;
typedef unsigned short uint16;
typedef signed short sint16;
typedef unsigned char uint8;
typedef signed char sint8;

#define INITIAL_POSITION "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"
#define MAXDEPTH 63
#define MAXPLY 256
#define MSSIZE 256
#define GAMELEN 1024
#define PLY 4
#define MATEVALUE 10000
#define STALEMATEVALUE (0)

#define EP	16
#define CAP	32
#define PROM	64
#define CASTLE  128

#ifdef _MSC_VER
#pragma warning(disable:4201)
#endif
typedef struct
{ union{
    uint32 p;
    struct{
      uint8 from;
      uint8 to;
      uint8 type;
      uint8 promoted;  
    };
  };
  int score;
}move_t;

typedef struct
{ union{
    uint32 p;
    struct{
      uint8 from;
      uint8 to;
      uint8 type;
      uint8 promoted;  
    };
  };
}move_data_t;

typedef struct
{ move_t m;
  uint8 ep;
  uint8 castle;
  uint8 moved;
  uint8 captured;
  uint64 hash;
  uint64 phash;
  uint32 mhash;
}undo_t;

typedef struct
{ uint8 square[64];
  bitboard_t occ[16];
  uint8 pcount[16];
  sint8 mr50;
  uint8 side;
  uint8 castle;
  uint8 ep;
  int ksq[2];
  int fullmove;
  int ply;
  int sply;
  uint64 hash;
  uint64 phash;
  uint32 mhash;
  undo_t stack[GAMELEN];
}position_t;

typedef struct
{ uint64 nodes;	            //node counter
  int root_moves;           //root moves count (legal)
  move_t rootmove;		      //current best root move
  bool stop_search;         //"time out" indicator
  int history[16][64];      //history heuristics container (history[piece][to])
  uint32 killer[2][MAXPLY]; //killer heuristics slots 1,2		 
  uint64 num_hash_saves;    //used for displaying the hashfull% info
}searchinfo_t, *psi_t;

typedef struct
{ sint8 side;
  uint8 old_ep;
  sint8 fifty;
  uint8 flags;
  int __padd_0;
  uint64 hash;
}nullstack_t;


//material:
#define KNBK              (1 << 1)
#define KKNB              (1 << 2)
#define PAWNLESS_EG       (1 << 3)
#define PAWN_EG           (1 << 4)
#define KBPK              (1 << 5)
#define TRY_OPPC_BISHOPS  (1 << 6)
#define TRY_BISHOP_PAIR   (1 << 7)
#define TRY_BISHOP_PAIR_W (1 << 8)
#define TRY_BISHOP_PAIR_B (1 << 9)
#define SPECIAL_EG (PAWNLESS_EG | PAWN_EG | KBPK | KNBK | KKNB)

typedef struct 
{ int scoremg;
  int scoreeg;
  int phase;
  int flags;
  int wmul;
  int bmul;
  sint16 material[2];
  uint32 mkey;
}material_entry_t;


//pawn table
typedef struct
{ int scoremg;
  int scoreeg;
  bitboard_t attacks[2];
  bitboard_t passers;
  hashkey_t pkey;
  uint8 wg1, wb1, we1, wd1;
  uint8 bg8, bb8, be8, bd8;
}pawn_entry_t;


#define CONSOLE_MODE 0
#define UCI_MODE 1
#define DEFAULT_ASPIRATION_WINDOW 20

//options
typedef struct
{ int mode;             //UCI or CONSOLE
  bool analyze;         //'analyze mode' indicator
  int comp_side;        //used in console mode only
  uint64 tt_size;       //transposition table size in bytes
  uint64 tt_numentries; //tt entries count, used so far only for the 'hashfull' calculation in io.c.
  int max_depth;        //if 'sd' is present
  int time_high;        //high t-boundary - maximum time allowed for 1 move.
  int time_low;         //low  t-boundary - iteration time constraint.
  bool time_fixed;      //if true - search until reach time_high, without breaking the iteration.
  int startup_time;     //used for io calculation
  int movestogo;        //uci 'movestogo' container
  int aspiration;       //'aspiration window width' variable
  bool rtp;             //reset timer on ponderhit
  bool book_active;     //'book found and loaded' indicator
}option_t;

#define DEFAULT_PNT_SIZE 16
#define DEFAULT_TT_SIZE 32
//transposition table entry:
typedef struct
{ uint8  depth;
  uint8  flag;
  sint16 value;
  uint32 move;
  uint64 key;
}tt_t, *ptt;

///transposition table flags
#define TT_ALPHA 0
#define TT_BETA  1
#define TT_EXACT 2
#define TT_UNKNOWN 0xFF

extern void init_movegen();
extern uint64 *b_moves[64];
extern uint64 *r_moves[64];
extern const uint64 n_moves[64];
extern const uint64 k_moves[64];
extern const uint64 p_attacks[2][64];
extern const uint64 r_magics[64];
extern const uint64 r_mask[64];
extern const uint64 b_magics[64];
extern const uint64 b_mask[64];
extern const unsigned int b_shift[64];
extern const unsigned int r_shift[64];
extern const uint64 file_mask[8];
extern const uint64 rank_mask[8];
extern int distance[64][64];
extern const uint64 zobrist_psq[16][64];
extern const uint64 zobrist_castle[16];
extern const uint64 zobrist_ep[64];
extern const uint64 zobrist_btm;
extern const uint32 zobrist_material[16][8];

#define bmoves(f, occ) (b_moves[(f)][(((occ) & b_mask[(f)]) * b_magics[(f)]) >> b_shift[(f)]])
#define rmoves(f, occ) (r_moves[(f)][(((occ) & r_mask[(f)]) * r_magics[(f)]) >> r_shift[(f)]])
#define qmoves(f, occ) ((rmoves((f), (occ))) | (bmoves((f), (occ))))

#define FMASK_A 0x101010101010101ULL
#define FMASK_B 0x202020202020202ULL
#define FMASK_C 0x404040404040404ULL
#define FMASK_D 0x808080808080808ULL
#define FMASK_E 0x1010101010101010ULL
#define FMASK_F 0x2020202020202020ULL
#define FMASK_G 0x4040404040404040ULL
#define FMASK_H 0x8080808080808080ULL

#define RMASK_1 0xffULL
#define RMASK_2 0xff00ULL
#define RMASK_3 0xff0000ULL
#define RMASK_4 0xff000000ULL
#define RMASK_5 0xff00000000ULL
#define RMASK_6 0xff0000000000ULL
#define RMASK_7 0xff000000000000ULL
#define RMASK_8 0xff00000000000000ULL

//castle empty squares masks:
#define W_OO_MASK 0x60ULL
#define W_OOO_MASK 0xeULL
#define B_OO_MASK 0x6000000000000000ULL
#define B_OOO_MASK 0xe00000000000000ULL

///externs
//psq tables:
#ifndef POPCNT
extern unsigned int bittable[0xFFFF];
#endif
//psq tables:
extern const int psq_outposts_mg[2][64];
extern const int psq_outposts_eg[2][64];
extern const int psq_pawn_mg[2][64];
extern const int psq_outposts_eg[2][64];

extern const int psq_knight_mg[2][64];
extern const int psq_knight_eg[2][64];
extern const int psq_bishop_mg[2][64];
extern const int psq_bishop_eg[2][64];
extern const int psq_rook_mg[2][64];
extern const int psq_rook_eg[2][64];
extern const int psq_queen_mg[2][64];
extern const int psq_queen_eg[2][64];
extern const int psq_king_mg[2][64];
extern const int psq_king_eg[2][64];

//masks
extern const bitboard_t passer_mask[2][64];
extern const bitboard_t isolated_mask[8];
extern const bitboard_t pawn_front[2][64];
extern const bitboard_t rook_backward[2][64];
extern const bitboard_t pawn_atk_delta[2][64];
extern const bitboard_t connected_mask[2][64];
extern const bitboard_t backward_mask[2][64];

extern const bitboard_t light_squares;
extern const bitboard_t dark_squares;
extern const bitboard_t woutposts;
extern const bitboard_t boutposts;

//globals
extern const int pval[16];
extern bitboard_t pawn_attacks[2];

//global structures:
extern position_t *pos;
extern searchinfo_t si;
extern option_t opt;


//mem.c
void *aligned_malloc(size_t size, size_t bound);
void aligned_free(void *aligned_memblock);
void aligned_wipe_out(void *aligned_memblock, size_t size, size_t bound);
//init.c
void generate_magics();
//pos.c
bool position_set(char *fen);
int position_get(char *fen);

//atk.c
bool is_sq_attacked_w(int sq);
bool is_sq_attacked_b(int sq);
bool is_in_check_w();
bool is_in_check_b();
bool is_pinned_w(int from, int to);
bool is_pinned_b(int from, int to);
bool is_pinned_ep_w(int from, int to);
bool is_pinned_ep_b(int from, int to);
int checkers_w(bitboard_t *attackers, bitboard_t *front, bitboard_t *across);
int checkers_b(bitboard_t *attackers, bitboard_t *front, bitboard_t *across);
bool is_in_check();

//gen.c
int move_gen(move_t *ms);
int move_gen_caps(move_t *ms);

//move.c:
void move_make(move_t m);
void move_unmake();
bool make_if_legal(move_t m);
void move_make_null(nullstack_t *ns);
void move_unmake_null(nullstack_t *ns);
bool is_legal(move_t m);

//perft.c
void test_divide(int depth);
bool test_epd(char *filename);
void perftest(int depth);

//io.c
void print_move(uint32 pm);
bool parse_move(char *sInput,move_t *move);
void print_info(int r,int d);
void update_info();
void check_for_poll();
void suppress_buffering();

//time.c:
int get_time();
void timer_start();
void timer_stop();
void time_check();

//main.c:
bool parse_input(char *sInput);

//hash.c
uint64 hash_position();

//eval.c
int eval();

//search.c
void think();

//ordering.c
void ordering(move_t *ms, int count, uint32 bestmove);
void caps_ordering(move_t *ms, int count);
void rootsort(move_t *ms, int size, int *rmscore);

//uci.c
void uci();
void uci_exit();

//tt.c
int tt_init(int MB);
uint8 tt_probe(int depth, int *value, int beta, uint32 *h_move, uint8 *null_ok);
void tt_save(int depth, int value, uint8 flag, uint32 h_move);
uint32 tt_get_hashmove();
int tt_retrieve_ponder(move_t *m);
void tt_age();
void tt_free();

//see.c
int see(move_t m);
int see_squares(int from, int to);

//pawns.c
int pnt_init();
pawn_entry_t *eval_pawn_struct();
void eval_pawn_struct_endgame(int *score);
void eval_passers(int *score, bitboard_t passers, bitboard_t watk_map, bitboard_t batk_map, bitboard_t occ);

//material.c
int init_material();
material_entry_t *eval_material();
uint32 hash_material();
bool is_pawn_endgame();
bool is_king_alone();
bool is_opening();
bool is_endgame();
int material_gain(move_t m);
int get_material_w();
int get_material_b();
int get_piece_count(int color);
int get_phase();

//safety:
void eval_shield(pawn_entry_t *pt, int *scoremg);

//endev.c:
bool eval_endgame(material_entry_t *mt, int *scoremg, int *scoreeg);

//gtb.c
#ifdef GTB
#define GTB_CACHE_DEFAULT 32
#define GTB_SCHEME_DEFAULT 4
#define MenCount (state->piece_count[W] + state->piece_count[B] + state->pawns + 2)
extern bool gtb_uci;
extern bool gtb_ok;
extern int gtb_max_men;
extern int tbhits;
extern char gtb_path[256];
extern int gtb_dec_scheme;
extern int gtb_cachesize;
void gtb_init();
void gtb_clear();
bool gtb_probe(int *value);
bool gtb_root_probe(int *value);
int gtb_men();
#endif
