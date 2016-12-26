/*--------------------------------------------------------------------------
    Pawny 0.3.1, chess engine (source code).
    Copyright (C) 2009 - 2011 by Mincho Georgiev.
    
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

#if defined(_M_IX64)
#define VERSION "0.3.1 (x64)"
#else
#define VERSION "0.3.1"
#endif

#ifndef _CRT_SECURE_NO_DEPRECATE
#define _CRT_SECURE_NO_DEPRECATE
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <ctype.h>

///statements:
#ifndef true
 #define true 1
 #define false 0
 #define bool uint8
#endif
///constants:
#define BOARD_REPRESENTATION (16*16)
#define BREP BOARD_REPRESENTATION
#define SQ_DIFF 48 //the difference betw. real sq. and sq. offsets
#define DEFAULT_SEARCH_DEPTH 8
#define MAX_GAME_LEN 1024
#define INF 30000
#define MAX_DEPTH 50
#define MAX_PLY 256
#define MOVE_STACK 256
#define PLY 4
#define MATE_VALUE 10000
#define STALEMATE_VALUE (0)

///transposition table flags
#define TT_ALPHA 0
#define TT_BETA  1
#define TT_EXACT 2
#define TT_UNKNOWN 0xFF

///startup pos:
#define INITIAL_POSITION "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"
#define TEST "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq -"

///macros
#define Square(sq)        ((sq) - SQ_DIFF)              //converting real square index to square[] offset
#define IsOutside(sq)     ((sq) & 0x88)                 //outside the board check for sq offset
#define IsEmpty(sq)       (square[(sq)] == EMPTY_SQUARE)//empty square check
#define calc_sq(file,rank)(((rank) << 4) + (file))      //calculate square number by known rank and file:
#define calc_file(sq)     ((sq) & 7)                    //file from square number
#define calc_rank(sq)     ((sq) >> 4)                   //getting the rank from square number
#define calc_rank64(sq)   ((sq) >> 3)                   //getting the rank from bit number
#define PieceType(sq)     (square[(sq)])                //getting the coloured piece type from the board square
#define PieceColor(sq)    ((square[(sq)]) >> 3)	        //Color from square context
#define GetColor(p)       ((p) >> 3)                    //stripping the color from 'coloured' piece
#define GetType(p)        ((p) & 7)                     //stripping type from piece
#define Coloured(p)       ((p) | (board->side << 3))    //the piece gets coloured
#define OpColoured(p)     ((p) | (board->xside << 3))   //opposite coloured
#define King_Square(color)(piece[PLSIZE - (KING | color << 3)].n)
#ifndef min
#define min(a,b) ((a)<(b)?(a):(b))
#endif
#ifndef max
#define max(a,b) ((a)>(b)?(a):(b))
#endif

#ifdef _WIN32
#define llufmt "%I64u"
#else
#define llufmt "%llu"
#endif

///piece list macros:
#ifdef _MSC_VER
#pragma warning(disable:4127)
#endif
//note:fits in the max.uchar value
#define PLSIZE (BREP-1)

//startup sq (list by type): 
#define PLS(pc) (piece[PLSIZE - (pc)].n) 
#define PLP(sq) (piece[(sq)].p)
#define PLN(sq) (piece[(sq)].n)

//insert
#define PLI(pc,sq) do{\
  PLN(sq) = PLS(pc); PLP(PLN(sq)) = sq;\
  PLP(sq) = (PLSIZE - (pc));  PLS(pc)  = sq;\
}while(0);

//remove
#define PLR(sq) do{\
  PLN(PLP(sq)) = PLN(sq); PLP(PLN(sq)) = PLP(sq);\
}while(0);

//move
#define PLM(f, t) do{\
  piece[(t)] = piece[(f)];\
  PLP(PLN(t)) = (t);PLN(PLP(t))  = (t);\
}while(0);

//startup and endup squares (list by color (entire list))
#define PLStart(color)	(King_Square(color))
#define PLEnd(color)	((PLSIZE) - (color * 8))


///board related:
enum {FILE_A, FILE_B, FILE_C, FILE_D, FILE_E, FILE_F, FILE_G, FILE_H};
enum {RANK_1, RANK_2, RANK_3, RANK_4, RANK_5, RANK_6, RANK_7, RANK_8};

#define FMASK_A 0x101010101010101ULL
#define FMASK_B 0x202020202020202ULL
#define FMASK_C 0x404040404040404ULL
#define FMASK_D 0x808080808080808ULL
#define FMASK_E 0x1010101010101010ULL
#define FMASK_F 0x2020202020202020ULL
#define FMASK_G 0x4040404040404040ULL
#define FMASK_H 0x8080808080808080ULL

//square offsets:
enum 
{ A1, B1, C1, D1, E1, F1, G1, H1,   I1, J1, K1, L1, M1, N1, O1, P1,
  A2, B2, C2 ,D2, E2, F2, G2, H2,   I2, J2, K2, L2, M2, N2, O2, P2,
  A3, B3, C3, D3, E3, F3, G3, H3,   I3, J3, K3, L3, M3, N3, O3, P3,
  A4, B4, C4, D4, E4, F4, G4, H4,   I4, J4, K4, L4, M4, N4, O4, P4,
  A5, B5, C5, D5, E5, F5, G5, H5,   I5, J5, K5, L5, M5, N5, O5, P5,
  A6, B6, C6, D6, E6, F6, G6, H6,   I6, J6, K6, L6, M6, N6, O6, P6,
  A7, B7, C7, D7, E7, F7, G7, H7,   I7, J7, K7, L7, M7, N7, O7, P7,
  A8, B8, C8, D8, E8, F8, G8, H8,   I8, J8, K8, L8, M8, N8, O8, P8
};
enum {WHITE, BLACK};
enum {PAWN=1, KNIGHT=2, BISHOP=3, ROOK=4, QUEEN=5, KING=6};

#define P_NONE 0
enum//coloured piece codes:
{ WHITE_PAWN = 1,
  WHITE_KNIGHT = 2,
  WHITE_BISHOP = 3,
  WHITE_ROOK = 4,
  WHITE_QUEEN = 5,
  WHITE_KING = 6,
  BLACK_PAWN = 9,
  BLACK_KNIGHT = 10,
  BLACK_BISHOP = 11,
  BLACK_ROOK = 12,
  BLACK_QUEEN = 13,
  BLACK_KING = 14,
  EMPTY_SQUARE = 32,
  OUTSIDE_BOARD = -1
};
///short:
#define W WHITE
#define B BLACK
#define WP WHITE_PAWN
#define BP BLACK_PAWN
#define WN WHITE_KNIGHT
#define BN BLACK_KNIGHT
#define WB WHITE_BISHOP
#define BB BLACK_BISHOP
#define WR WHITE_ROOK
#define BR BLACK_ROOK
#define WQ WHITE_QUEEN
#define BQ BLACK_QUEEN
#define WK WHITE_KING
#define BK BLACK_KING

///pieces values:
#define P_VALUE 100
#define N_VALUE 321
#define B_VALUE 325
#define R_VALUE 500
#define Q_VALUE 950
#define K_VALUE 0
#define B_N_DIFF (B_VALUE-N_VALUE)

///castling availability and conduction masks
#define WHITE_OO 1
#define WHITE_OOO 2
#define BLACK_OO 4
#define BLACK_OOO 8

typedef unsigned __int64 uint64;
typedef unsigned int uint32;
typedef unsigned short uint16;
typedef short int sint16;
typedef unsigned char uint8;
typedef signed char sint8;
typedef uint64 hashkey_t;
typedef uint8 sq_t;
typedef uint64 bitboard_t;

///data types:
typedef struct _list_t
{ uint8 p;
  uint8 n;
}list_t;

///move types:
#define EP	16
#define CAP	32
#define PROM	64
#define CASTLE  128

#ifdef _MSC_VER
#pragma warning(disable:4201)
#endif
typedef struct _move_data_t
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
typedef struct _move_t
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

#define R_ATK 2
#define B_ATK 4
#define N_ATK 8

typedef struct
{ uint8 flags;
  sint8 delta;
}attack_t;

//history stack element:
typedef struct _hist_t
{ move_data_t m;  //the move maded
  sq_t old_ep;    //old ep square
  uint8 flags;    //contains the castling av. flags before this move
  uint8 moved;    //the moved piece's type
  uint8 captured; //the cap. piece code
  uint64 hash;    //the key before this move is made.
  uint64 phash;   //pawn key before the move
}hist_t;

//null move data container:
typedef struct _nullstack_t
{ sint8 side;
  sint8 xside;
  sq_t old_ep;
  int fifty;
  uint8 flags;
  uint64 hash;
}nullstack_t;

typedef struct _state_t
{ sint8 pawns;
  int material;
  int pawn_value[2];
  int material_value[2];
  sint8 piece_count[2];
  sint8 presence[2][8];
}state_t;

#define Queens(color)	(state->presence[(color)][QUEEN])
#define Rooks(color)	(state->presence[(color)][ROOK])
#define Bishops(color)	(state->presence[(color)][BISHOP])
#define Knights(color)	(state->presence[(color)][KNIGHT])
#define Pawns(color)	(state->presence[(color)][PAWN])

#define OPENING_MATERIAL (2*Q_VALUE + 4*R_VALUE + 3*B_VALUE + 3*N_VALUE)
#define MIDGAME_MATERIAL (2*Q_VALUE + 2*R_VALUE + 4*B_VALUE)
#define ENDGAME_MATERIAL (4*R_VALUE + 2*B_VALUE)
#define opening (board->state.material >= OPENING_MATERIAL) 
#define midgame ((board->state.material > ENDGAME_MATERIAL) && (board->state.material <= MIDGAME_MATERIAL))
#define endgame (board->state.material <= ENDGAME_MATERIAL)
#define pawn_endgame ((!board->state.material) && (board->state.pawns))
#define king_alone ((!board->state.material_value[W] && !board->state.pawn_value[W])\
|| (!board->state.material_value[B] && !board->state.pawn_value[B]))


//board state and information
typedef struct _position_t
{ sq_t  square[BREP];     //16*16
  list_t piece[BREP];     //16*16
  sq_t en_passant;        //the en passant square
  sint8 side, xside;      //side to move, opponent side
  uint8 castle;           //castling availability(both sides) flags
  sint8 mr50;             //the halfmove clock for the 50 move rule
  int fullmove;           //fullmove game counter
  int ply;                //ply of search
  int hply;               //halfmove counter since game beginning (history ply)
  hashkey_t hash;         //board's hash key
  hashkey_t phash;        //pawn key
  hist_t hs[MAX_GAME_LEN];//history stack
  state_t state;          //pawn and material info (for the current ply)
  bitboard_t bb_pawns[2];
}position_t, *ppos_t;

//search information
typedef struct _searchinfo_t
{ uint64 nodes;	            //node counter
  int root_moves;           //root moves count (legal)
  move_t rootmove;		      
  bool stop_search;         //"time out" indicator
  int history[128][128];    //history heuristics container (history[f][t] (Butterfly Board)
  uint32 killer[2][MAX_PLY];//killer heuristics slots 1,2		 
  uint64 num_hash_saves;    //used for displaying the hashfull% info
}searchinfo_t, *psi_t;

#define DEFAULT_TT_SIZE 32
//transposition table entry:
typedef struct _tt_t
{ uint8  depth;
  uint8  flag;
  sint16 value;
  uint32 move;
  hashkey_t key;
}tt_t, *ptt;

#define CONSOLE_MODE 0
#define UCI_MODE 1
#define DEFAULT_ASPIRATION_WINDOW 20
//options
typedef struct _option_t
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


///externs:

//piece values array:
extern const int pval[16];
extern const int slider[15];
extern attack_t attack_vector[128][128];
extern const int atk_masks[16];

extern const int dir_vect[16][9];   //direction/attack vector:
extern int distance_table[128][128];//dist. betw. 2 sq.
extern int direction[128][128];     //sliding step betw. 2 squares.

extern const int pawn_delta[2][3];
extern const int pawn_push[2][3];
extern const int pawn_prom_rank[2];
extern const int pawn_prom_rank_[2];
extern const int pawn_dpush_rank[2];
extern bitboard_t pawn_attacks[2];

//psq tables:
extern const int psq_table[14+1][128];
extern const int endgame_king_psq[2][128];
extern const int psq_move_king_end[2][128];
extern const int psq_outposts[2][128];
extern const int square_color[128];

//global pointers:
extern ppos_t board;
extern psi_t si;
extern list_t *piece;
extern hist_t *hs;
extern sq_t *squares;
extern sq_t *square;
extern option_t *opt;
extern ptt tt;

//zobrist randoms
extern const unsigned long long zobrist_psq[15][128];
extern const unsigned long long zobrist_castle[16];
extern const unsigned long long zobrist_ep[128];
extern const unsigned long long zobrist_side[2];

///function prototypes:
//attack.c
void init_attack_vector();
bool is_sq_attacked(sq_t target, int color);
bool is_in_check(int side);

//board.c
void board_init();
bool board_from_fen(char  *fen);
int  board_to_fen(char *fen);
void init_distance();
void init_direction();

//hash.c
void hash_board();

//io.c
void print_info(int r,int d);
void check_for_poll();
int pv_to_string(char *s,int depth);
int poll();
void suppress_buffering();
void update_info();

//move.c
bool move_undo();
bool move_make(move_t move);
void move_make_null(nullstack_t *ns);
void move_undo_null(nullstack_t *ns);
bool make_if_legal(move_t m);

//movegen.c
int move_gen(move_t ms[]);
int move_gen_caps(move_t ms[]);
int move_gen_legal(move_t ms[]);
int move_gen_evasions(move_t ms[]);

//perft.c
void test_divide(int depth);
void test_pawn_divide(int depth);
void test_perft(int depth);
bool test_epd(char *filename);

//eval.c
int eval();
int full_material_ballance(int color);
int material_gain(move_t m);

//endev.c
int endeval(int score[]);

//time.c
int get_time();
void time_check();
void timer_start();
void timer_stop();

//uci.c
void uci();
void uci_exit();
void uci_go(char *commands);
void uci_set_position(char *pstr);
void uci_new_game();

//tt.c
int tt_init(int MB);
void tt_free();
void tt_clear();
void tt_check();
uint32 tt_get_hashmove();
int tt_retrieve_ponder(move_t *m);
uint8 tt_probe(int depth, int *value, int beta, uint32 *h_move, uint8 *null_ok);
void tt_save(int depth, int value, uint8 flag, uint32 h_move);

//see.c
int see(move_t m);

//search.c
void think();

//main.c
bool parse_input(char *sInput);
bool parse_move(char *sInput,move_t *move);
void print_move(uint32 move);

//pawns.c
int pnt_init();
void eval_pawn_struct(int score[]);
void eval_pawn_struct_endgame(int score[]);

//book.c
void book_open();
void book_close();
bool book_move(move_t *m);

//ordering.c
void ordering(move_t ms[], int count, uint32 bestmove);
void caps_ordering(move_t ms[], int count);

//mem.c
void *aligned_malloc(size_t size, size_t alignment);
void aligned_free(void *aligned_memblock);
void aligned_wipe_out(void *aligned_memblock, size_t size, size_t alignment);
  
//bits.c
extern const int rsz[128];
extern const int rsu[64];
extern const bitboard_t file_mask[8];
extern const bitboard_t isolated_mask[8];
extern const bitboard_t passer_mask[2][64];
extern const bitboard_t pawn_front[2][64];
extern const bitboard_t king_storm_mask[2];
extern const bitboard_t queen_storm_mask[2];


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
#endif
