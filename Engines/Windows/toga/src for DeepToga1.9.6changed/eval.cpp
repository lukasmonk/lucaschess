
// eval.cpp

// includes

#include <cstdlib> // for abs()
#include <cstring> // for memset()
#include <math.h> // for memset(), sqrt()

#include "attack.h"
#include "colour.h"
#include "eval.h"
#include "material.h"
#include "move.h"
#include "option.h"
#include "pawn.h"
#include "piece.h"
#include "see.h"
#include "util.h"
#include "value.h"
#include "vector.h"
//#include "probe.h"
//#include "search.h"

// macros

#define THROUGH(piece) ((piece)==Empty)

// UCI inputs/parameters

static /*UCI*/ int  PieceActivityWeight  = 256; // 100%
static /*UCI*/ int  KingSafetyWeight     = 256; // 100%
static /*UCI*/ int  PassedPawnWeight     = 256; // 100%
static /*UCI*/ int  KingAttackWeight     = 256; // 100%
static /*UCI*/ int  PieceInvasion        = 256; // 100%

static /*UCI*/ bool PawnHoles = false; // baseline false, 

// constants and variables

const int KnightOutpostMatrix[2][64] = {
     0,     0,   0,   0,   0,   0,   0,   0,
     0,     0,   0,   0,   0,   0,   0,   0,
     0,     0,   0,   0,   0,   0,   0,   0,
     0,     2,   5,  10,  10,   5,   2,   0,
     0,     2,   5,  10,  10,   5,   2,   0,
     0,     0,   4,   5,   5,   4,   0,   0,
     0,     0,   0,   0,   0,   0,   0,   0,
     0,     0,   0,   0,   0,   0,   0,   0,

     0,     0,   0,   0,   0,   0,   0,   0,
     0,     0,   0,   0,   0,   0,   0,   0,
     0,     0,   4,   5,   5,   4,   0,   0,
     0,     2,   5,  10,  10,   5,   2,   0,
     0,     2,   5,  10,  10,   5,   2,   0,
     0,     0,   0,   0,   0,   0,   0,   0,
     0,     0,   0,   0,   0,   0,   0,   0,
     0,     0,   0,   0,   0,   0,   0,   0,
};

static  const  int  lazy_piece_cutoff    = 135; // WHM  DONE: 7'th rank rook stuff to eval__attack(), then this lowered.
static  const  int  lazy_piece_cutoff_PV = 200; // WHM 
static  const  int  lazy_eval_cutoff     = 340; /* Thomas had it at 200 */
static  const  int  lazy_eval_cutoff_PV  = 400; // WHM

// Ryan's speedup of eval_piece().  Posted open source in GambitFruit.  
static const int knight_mob_opening[ 8 + 1] =                {-16, -12,  -8,  -4,  0,  4,   8, 12, 16};             // WHM; slope == KinghtMobOpening == 4
static const int knight_mob_endgame[ 8 + 1] =                {-16, -12,  -8,  -4,  0,  4,   8, 12, 16};             // WHM; slope == KinghtMobEndgame == 4
static const int bishop_mob_opening[13 + 1] =      {-30, -25, -20, -15, -10,  -5,  0,  5,  10, 15, 20, 25, 30, 35}; // WHM; slope == BishopMobOpening == 5
static const int bishop_mob_endgame[13 + 1] =      {-30, -25, -20, -15, -10,  -5,  0,  5,  10, 15, 20, 25, 30, 35}; // WHM; slope == BishopMobEndgame == 5
static const int   rook_mob_opening[14 + 1] = {-14, -12, -10,  -8,  -6,  -4,  -2,  0,  2,   4,  6,  8, 10, 12, 14}; // WHM; slope == RookMobOpening   == 2
static const int   rook_mob_endgame[14 + 1] = {-28, -24, -20, -16, -12,  -8,  -4,  0,  4,   8, 12, 16, 20, 24, 28}; // WHM; slope == RookMobEndgame   == 4

static const int MobMove = 1;
static const int MobAttack = 1;
static const int MobDefense = 0;

#if DEBUG
static const int KnightUnit = 4;
static const int BishopUnit = 6;
static const int RookUnit = 7;
static const int QueenUnit = 13;

static const int KnightMobOpening = 4;
static const int KnightMobEndgame = 4;
static const int BishopMobOpening = 5;
static const int BishopMobEndgame = 5;
static const int RookMobOpening = 2;
static const int RookMobEndgame = 4;
static const int QueenMobOpening = 1;
static const int QueenMobEndgame = 2;
static const int KingMobOpening = 0;
static const int KingMobEndgame = 0;
#endif

static const bool UseOpenFile = true;
static const int RookSemiOpenFileOpening = 10;
static const int RookSemiOpenFileEndgame = 10;
static const int RookOpenFileOpening = 20;
static const int RookOpenFileEndgame = 20;
static const int RookSemiKingFileOpening = 10;
static const int RookKingFileOpening = 20;

static const int Rook7thOpening = 20;
static const int Rook7thEndgame = 40;
static const int Queen7thOpening = 10;
static const int Queen7thEndgame = 20;

static const int TrappedBishop = 100;

static const int BlockedBishop = 50;
static const int BlockedRook = 50;

static const int KingAttackOpening = 20;
static const int WeakBackRank = 11;
static const int StormOpening = 10;

static const int PassedOpeningMin  =  10;
static const int PassedOpeningMax  =  70;
static const int PassedEndgameMin  =  20;
static const int PassedEndgameMax  = 140;

static const int UnStoppablePasser = 800;
static const int FreePasser        =  60;
static const int KingPasser        =  35; // not in Toga1.4beta5cWHM(31).  

static const int AttackerDistance  =   5;
static const int DefenderDistance  =  20;

// "constants"

static const int KingAttackPieceNBArray[16] = { // WHM was KingAttackWeight[16]
   0, 0, 128, 192, 224, 240, 248, 252, 254, 255, 256, 256, 256, 256, 256, 256,
};//  1    2    3    4    5    6    7    8   q,r,r,b,b,n,n == 7 pieces

// variables

static int MobUnit[ColourNb][PieceNb];

static int KingAttackUnit[PieceNb]; // 1,1,2,4 == mating power

static int PieceInvasionUnit[PieceNb]; // 3,3,5,9 == tactical power

// prototypes

static void eval_draw          (const board_t * board, const material_info_t * mat_info, const pawn_info_t * pawn_info, int mul[2]);

// eval
static void eval_pattern       (const board_t * board,                                                                  int * opening, int * endgame);
static void eval_passer        (const board_t * board,                                   const pawn_info_t * pawn_info, int * opening, int * endgame, bool full_q_eval);
static void eval__attack       (const board_t * board, const material_info_t * mat_info, const pawn_info_t * pawn_info, int * opening, int * endgame);
static void eval_king_safety   (const board_t * board, const material_info_t * mat_info,                                int * opening, int * endgame);
static void eval_piece         (const board_t * board, const material_info_t * mat_info, const pawn_info_t * pawn_info, int * opening, int * endgame);
static void eval_piece_holes   (const board_t * board, const material_info_t * mat_info, const pawn_info_t * pawn_info, int * opening, int * endgame);

// functions needed for eval
static bool unstoppable_passer (const board_t * board, int pawn, int colour);
static bool king_passer        (const board_t * board, int pawn, int colour);
static bool free_passer        (const board_t * board, int pawn, int colour);
static bool tough_king_passer  (const board_t * board, int pawn, int colour);

static int  pawn_att_dist      (int pawn, int king, int colour);
static int  pawn_def_dist      (int pawn, int king, int colour);

static void draw_init_list     (int list[], const board_t * board, int pawn_colour);

static bool draw_kpkq          (const int list[], int turn);
static bool draw_kpkr          (const int list[], int turn);
static bool draw_kpkb          (const int list[], int turn);
static bool draw_kpkn          (const int list[], int turn);

static bool draw_knpk          (const int list[], int turn);

static bool draw_krpkr         (const int list[], int turn);
static bool draw_kbpkb         (const int list[], int turn);

static int  shelter_square     (const board_t * board, const material_info_t * mat_info, int square, int colour);
static int  shelter_file       (const board_t * board, int file, int rank, int colour);

static int  storm_file         (const board_t * board, int file, int colour);

static bool bishop_can_attack  (const board_t * board, int to, int colour);


// functions

void eval_parameter() {

    // UCI options

   PassedPawnWeight    = (option_get_int("Passed Pawns")   * 256 + 50) / 100;
   PieceActivityWeight = (option_get_int("Piece Activity") * 256 + 50) / 100;
   KingSafetyWeight    = (option_get_int("King Safety")    * 256 + 50) / 100;
   KingAttackWeight    = (option_get_int("King Attack")    * 256 + 50) / 100;
   PieceInvasion       = (option_get_int("Piece Invasion") * 256 + 50) / 100;
   
   PawnHoles = option_get_bool("Pawn Holes"); // WHM
}

// eval_init()

void eval_init() {

   int colour;
   int piece;
   
   // UCI options
   
   eval_parameter();
   
   // mobility table
   
   for (colour = 0; colour < ColourNb; colour++) {
      for (piece = 0; piece < PieceNb; piece++) {
         MobUnit[colour][piece] = 0;
      }
   }
   
   MobUnit[White][Empty] = MobMove;
   
   MobUnit[White][BP] = MobAttack;
   MobUnit[White][BN] = MobAttack;
   MobUnit[White][BB] = MobAttack;
   MobUnit[White][BR] = MobAttack;
   MobUnit[White][BQ] = MobAttack;
   MobUnit[White][BK] = MobAttack;
   
   MobUnit[White][WP] = MobDefense;
   MobUnit[White][WN] = MobDefense;
   MobUnit[White][WB] = MobDefense;
   MobUnit[White][WR] = MobDefense;
   MobUnit[White][WQ] = MobDefense;
   MobUnit[White][WK] = MobDefense;
   
   MobUnit[Black][Empty] = MobMove;
   
   MobUnit[Black][WP] = MobAttack;
   MobUnit[Black][WN] = MobAttack;
   MobUnit[Black][WB] = MobAttack;
   MobUnit[Black][WR] = MobAttack;
   MobUnit[Black][WQ] = MobAttack;
   MobUnit[Black][WK] = MobAttack;
   
   MobUnit[Black][BP] = MobDefense;
   MobUnit[Black][BN] = MobDefense;
   MobUnit[Black][BB] = MobDefense;
   MobUnit[Black][BR] = MobDefense;
   MobUnit[Black][BQ] = MobDefense;
   MobUnit[Black][BK] = MobDefense;
   
   // KingAttackUnit[] + PieceInvasionUnit[]
   
   for (piece = 0; piece < PieceNb; piece++) {
      KingAttackUnit[piece] = 0;
      PieceInvasionUnit[piece] = 0;
   }

   // Piece Invasion
   PieceInvasionUnit[WN] = 3;
   PieceInvasionUnit[WB] = 3;
   PieceInvasionUnit[WR] = 5;
   PieceInvasionUnit[WQ] = 9;
   
   PieceInvasionUnit[BN] = 3;
   PieceInvasionUnit[BB] = 3;
   PieceInvasionUnit[BR] = 5;
   PieceInvasionUnit[BQ] = 9;
   
   // mate power scale
   KingAttackUnit[WN] = 1;
   KingAttackUnit[WB] = 1;
   KingAttackUnit[WR] = 2;
   KingAttackUnit[WQ] = 4;
   
   KingAttackUnit[BN] = 1;
   KingAttackUnit[BB] = 1;
   KingAttackUnit[BR] = 2;
   KingAttackUnit[BQ] = 4;
}

// eval()

int eval(board_t * board, int alpha, int beta, bool full_q_eval, int ThreadId) {

   bool in_pv;

   int opening, endgame;
   int mul[ColourNb];
   int phase;
   int score; // was eval
   int wb, bb;
   int lazy_eval_value; // Thomas

   material_info_t mat_info[1];
   pawn_info_t pawn_info[1];


   ASSERT(board!=NULL);

   ASSERT(board_is_legal(board));
   ASSERT(!board_is_check(board)); // exceptions are extremely rare

   // Tempo & init

   if (COLOUR_IS_WHITE(board->turn)){
       opening = +20;
       endgame = +10;
   }
   else{ ASSERT(COLOUR_IS_BLACK(board->turn));
       opening = -20;
       endgame = -10;
   } 

   // material data gathering from mat_info

   material_get_info(mat_info,board,ThreadId);

   opening += mat_info->opening;
   endgame += mat_info->endgame;

   mul[White] = mat_info->mul[White];
   mul[Black] = mat_info->mul[Black];

   phase = mat_info->phase; // WHM; gathered the material gathering.

   // PST ... Positional Strength Tables ... Piece Square Tables

   opening += board->opening;
   endgame += board->endgame;

   // pawns

   pawn_get_info(pawn_info,board,ThreadId);

   opening += pawn_info->opening;
   endgame += pawn_info->endgame;

   // draw

   eval_draw(board,mat_info,pawn_info,mul);

   if (mat_info->mul[White] < mul[White]) mul[White] = mat_info->mul[White];
   if (mat_info->mul[Black] < mul[Black]) mul[Black] = mat_info->mul[Black];

   if (mul[White] == 0 && mul[Black] == 0) return ValueDraw;

   // eval

   // WHM: FreePasser + PassedEndgameMax >= 200, 
   //      and eval_passer() much faster than eval_king() and especially eval_piece().  

   eval_passer(board,pawn_info,&opening,&endgame,full_q_eval);
   eval_pattern(board,&opening,&endgame);
   eval_king_safety(board,mat_info,&opening,&endgame); // storm and shelter stuff

   // drawish bishop endgames

   if ((mat_info->flags & DrawBishopFlag) != 0) {

      wb = board->piece[White][1];
      ASSERT(PIECE_IS_BISHOP(board->square[wb]));

      bb = board->piece[Black][1];
      ASSERT(PIECE_IS_BISHOP(board->square[bb]));

      if (SQUARE_COLOUR(wb) != SQUARE_COLOUR(bb)) {
         mul[White] /= 2; // 1/2
         mul[Black] /= 2; // 1/2
      }
   }

   // NodePV boards, our best and final answers!  
   in_pv = (beta > 1 + alpha);

   /* lazy cutoff by Thomas */
   if (board->piece_size[White] > 3  && 
       board->piece_size[Black] > 3  &&
       16 == mul[White]              &&
       16 == mul[Black]                 ){

      lazy_eval_value = ((opening * (256 - phase)) + (endgame * phase)) / 256;
      if (COLOUR_IS_BLACK(board->turn)) lazy_eval_value = -lazy_eval_value;
            
      ASSERT(lazy_eval_value > -ValueEvalInf && lazy_eval_value < +ValueEvalInf);
      
      if (in_pv) {
         if (lazy_eval_value - lazy_eval_cutoff_PV >=  beta) return lazy_eval_value;
         if (lazy_eval_value + lazy_eval_cutoff_PV <= alpha) return lazy_eval_value;  
      } else {
         if (lazy_eval_value - lazy_eval_cutoff    >=  beta) return lazy_eval_value;
         if (lazy_eval_value + lazy_eval_cutoff    <= alpha) return lazy_eval_value;  
      }
   }
   /* ende lazy cuttoff */
   
   
   // more eval
   
   // "King Attack" PLUS piece invasion stuff PLUS rooks and queens on 7'th rank.  
   eval__attack(board,mat_info,pawn_info,&opening,&endgame);
   
   // lazy piece cutoff by William
   if (board->piece_size[White] > 3  &&  
       board->piece_size[Black] > 3  &&
       16 == mul[White]              &&
       16 == mul[Black]                 ){

      lazy_eval_value = ((opening * (256 - phase)) + (endgame * phase)) / 256;
      if (COLOUR_IS_BLACK(board->turn)) lazy_eval_value = -lazy_eval_value;

      ASSERT(lazy_eval_value > -ValueEvalInf && lazy_eval_value < +ValueEvalInf);

      if (in_pv) {
         if (lazy_eval_value - lazy_piece_cutoff_PV >=  beta) return lazy_eval_value;
         if (lazy_eval_value + lazy_piece_cutoff_PV <= alpha) return lazy_eval_value;  
      } else {
         if (lazy_eval_value - lazy_piece_cutoff    >=  beta) return lazy_eval_value;
         if (lazy_eval_value + lazy_piece_cutoff    <= alpha) return lazy_eval_value;  
      }
   }
   // end lazy piece cutoff
   
   
   // more more eval, "Pawn Holes" are for Rybka.  
   if (PawnHoles) eval_piece_holes(board,mat_info,pawn_info,&opening,&endgame); // pawn attack holes avoided
   else           eval_piece      (board,mat_info,pawn_info,&opening,&endgame); // somewhat faster
   
   
#if DEBUG // WHM testing the lazy thresholds...
   int deb_op = 0;
   int deb_eg = 0;

   eval_piece_holes(board,mat_info,pawn_info,&deb_op,&deb_eg);

   lazy_eval_value = abs(((deb_op * (256 - phase)) + (deb_eg * phase)) / 256);

   if    (lazy_eval_value >  lazy_piece_cutoff   ) printf("info - lazy_eval_piece > lazy_piece_cutoff, %d \n\r", lazy_eval_value);
   ASSERT(lazy_eval_value <= lazy_piece_cutoff_PV);

   eval__attack(board,mat_info,pawn_info,&deb_op,&deb_eg);

   lazy_eval_value = abs(((deb_op * (256 - phase)) + (deb_eg * phase)) / 256);

   if    (lazy_eval_value >  lazy_eval_cutoff) printf("info - lazy_eval_value > lazy_eval_cutoff, %d \n\r", lazy_eval_value);;
   ASSERT(lazy_eval_value <= lazy_eval_cutoff_PV);
#endif
   
   
   // phase mix
   score = ((opening * (256 - phase)) + (endgame * phase)) / 256;
   
   // draw bound
   if (mul[White] < 16  &&  score > ValueDraw) score =   ( score * mul[White]) / 16 ;
   if (mul[Black] < 16  &&  score < ValueDraw) score = -((-score * mul[Black]) / 16);
   
   // 50 move rule {== 100 ply}
   if              (100 - board->ply_nb < 32) {
             ASSERT(100 - board->ply_nb >= 0);
      if           (100 - board->ply_nb >= 0) {
          score *= (100 - board->ply_nb);
          score /= 32;
      } else {
          score = 0;
      }
   }
   
   // value range
   ASSERT(score >= -ValueEvalInf && score <= +ValueEvalInf);
   ASSERT(!value_is_mate(score));
   
   // turn & return
   if (COLOUR_IS_BLACK(board->turn)) return -score;
   else                              return  score;
}

// eval_draw()

static void eval_draw(const board_t * board, const material_info_t * mat_info, const pawn_info_t * pawn_info, int mul[2]) {

   int colour;
   int me, opp;
   int pawn, king;
   int pawn_file;
   int prom;
   int list[7+1];

   ASSERT(board!=NULL);
   ASSERT(mat_info!=NULL);
   ASSERT(pawn_info!=NULL);
   ASSERT(mul!=NULL);

   // draw patterns

   for (colour = 0; colour < ColourNb; colour++) {

      me = colour;
      opp = COLOUR_OPP(me);

      // KB*P+K* draw

      if ((mat_info->cflags[me] & MatRookPawnFlag) != 0) {

         pawn = pawn_info->single_file[me];

         if (pawn != SquareNone) { // all pawns on one file

            pawn_file = SQUARE_FILE(pawn);

            if (pawn_file == FileA || pawn_file == FileH) {

               king = KING_POS(board,opp);
               prom = PAWN_PROMOTE(pawn,me);

               if (DISTANCE(king,prom) <= 1 && !bishop_can_attack(board,prom,me)) {
                  mul[me] = 0;
               }
            }
         }
      }

      // K(B)P+K+ draw

      if ((mat_info->cflags[me] & MatBishopFlag) != 0) {

         pawn = pawn_info->single_file[me];

         if (pawn != SquareNone) { // all pawns on one file

            king = KING_POS(board,opp);

            if (SQUARE_FILE(king)  == SQUARE_FILE(pawn)
             && PAWN_RANK(king,me) >  PAWN_RANK(pawn,me)
             && !bishop_can_attack(board,king,me)) {
               mul[me] = 1; // 1/16
            }
         }
      }

      // KNPK* draw

      if ((mat_info->cflags[me] & MatKnightFlag) != 0) {

         pawn = board->pawn[me][0];
         king = KING_POS(board,opp);

         if (SQUARE_FILE(king)  == SQUARE_FILE(pawn)
          && PAWN_RANK(king,me) >  PAWN_RANK(pawn,me)
          && PAWN_RANK(pawn,me) <= Rank6) {
            mul[me] = 1; // 1/16
         }
      }
   }

   // recognisers, only heuristic draws here!

   if (false) {

   } else if (mat_info->recog == MAT_KPKQ) {

      // KPKQ (white)

      draw_init_list(list,board,White);

      if (draw_kpkq(list,board->turn)) {
         mul[White] = 1; // 1/16;
         mul[Black] = 1; // 1/16;
      }

   } else if (mat_info->recog == MAT_KQKP) {

      // KPKQ (black)

      draw_init_list(list,board,Black);

      if (draw_kpkq(list,COLOUR_OPP(board->turn))) {
         mul[White] = 1; // 1/16;
         mul[Black] = 1; // 1/16;
      }

   } else if (mat_info->recog == MAT_KPKR) {

      // KPKR (white)

      draw_init_list(list,board,White);

      if (draw_kpkr(list,board->turn)) {
         mul[White] = 1; // 1/16;
         mul[Black] = 1; // 1/16;
      }

   } else if (mat_info->recog == MAT_KRKP) {

      // KPKR (black)

      draw_init_list(list,board,Black);

      if (draw_kpkr(list,COLOUR_OPP(board->turn))) {
         mul[White] = 1; // 1/16;
         mul[Black] = 1; // 1/16;
      }

   } else if (mat_info->recog == MAT_KPKB) {

      // KPKB (white)

      draw_init_list(list,board,White);

      if (draw_kpkb(list,board->turn)) {
         mul[White] = 1; // 1/16;
         mul[Black] = 1; // 1/16;
      }

   } else if (mat_info->recog == MAT_KBKP) {

      // KPKB (black)

      draw_init_list(list,board,Black);

      if (draw_kpkb(list,COLOUR_OPP(board->turn))) {
         mul[White] = 1; // 1/16;
         mul[Black] = 1; // 1/16;
      }

   } else if (mat_info->recog == MAT_KPKN) {

      // KPKN (white)

      draw_init_list(list,board,White);

      if (draw_kpkn(list,board->turn)) {
         mul[White] = 1; // 1/16;
         mul[Black] = 1; // 1/16;
      }

   } else if (mat_info->recog == MAT_KNKP) {

      // KPKN (black)

      draw_init_list(list,board,Black);

      if (draw_kpkn(list,COLOUR_OPP(board->turn))) {
         mul[White] = 1; // 1/16;
         mul[Black] = 1; // 1/16;
      }

   } else if (mat_info->recog == MAT_KNPK) {

      // KNPK (white)

      draw_init_list(list,board,White);

      if (draw_knpk(list,board->turn)) {
         mul[White] = 1; // 1/16;
         mul[Black] = 1; // 1/16;
      }

   } else if (mat_info->recog == MAT_KKNP) {

      // KNPK (black)

      draw_init_list(list,board,Black);

      if (draw_knpk(list,COLOUR_OPP(board->turn))) {
         mul[White] = 1; // 1/16;
         mul[Black] = 1; // 1/16;
      }

   } else if (mat_info->recog == MAT_KRPKR) {

      // KRPKR (white)

      draw_init_list(list,board,White);

      if (draw_krpkr(list,board->turn)) {
         mul[White] = 1; // 1/16;
         mul[Black] = 1; // 1/16;
      }

   } else if (mat_info->recog == MAT_KRKRP) {

      // KRPKR (black)

      draw_init_list(list,board,Black);

      if (draw_krpkr(list,COLOUR_OPP(board->turn))) {
         mul[White] = 1; // 1/16;
         mul[Black] = 1; // 1/16;
      }

   } else if (mat_info->recog == MAT_KBPKB) {

      // KBPKB (white)

      draw_init_list(list,board,White);

      if (draw_kbpkb(list,board->turn)) {
         mul[White] = 1; // 1/16;
         mul[Black] = 1; // 1/16;
      }

   } else if (mat_info->recog == MAT_KBKBP) {

      // KBPKB (black)

      draw_init_list(list,board,Black);

      if (draw_kbpkb(list,COLOUR_OPP(board->turn))) {
         mul[White] = 1; // 1/16;
         mul[Black] = 1; // 1/16;
      }
   }
}

// eval_piece()

static void eval_piece(const board_t * board, const material_info_t * mat_info, const pawn_info_t * pawn_info, int * opening, int * endgame) {

   const sq_t * ptr;
   const int * unit;
   
   int mob;
   int capture;
   int from, to;

   int op[ColourNb], eg[ColourNb];
   
   int colour;
   int me, opp;
   int piece;
   int rook_file, king_file;
   int king;
   int delta;
   int new_mob;
   
   ASSERT(board!=NULL);
   ASSERT(mat_info!=NULL);
   ASSERT(pawn_info!=NULL);
   ASSERT(opening!=NULL);
   ASSERT(endgame!=NULL);

   // init

   for (colour = 0; colour < ColourNb; colour++) {
      op[colour] = 0;
      eg[colour] = 0;
   }

   // eval

   for (colour = 0; colour < ColourNb; colour++) {

      me = colour;
      opp = COLOUR_OPP(me);

      unit = MobUnit[me];

      // piece loop
      // HACK:         no king == [1]
      for (ptr = &board->piece[me][1]; (from=*ptr) != SquareNone; ptr++) {

         ASSERT(*ptr >= 0);

         piece = board->square[from];

         switch (PIECE_TYPE(piece)) {

         case Knight64:

            // mobility
            
            mob = 0; ASSERT(from >= 33); ASSERT(from <= 255-33);
            
            mob += unit[board->square[from-33]]; // Toga1.4beta5cWHM(31) had this.  
            mob += unit[board->square[from-31]];
            mob += unit[board->square[from-18]];
            mob += unit[board->square[from-14]];
            mob += unit[board->square[from+14]];
            mob += unit[board->square[from+18]];
            mob += unit[board->square[from+31]];
            mob += unit[board->square[from+33]];
            
            ASSERT(knight_mob_opening[mob] == (mob-KnightUnit) * KnightMobOpening);
            ASSERT(knight_mob_endgame[mob] == (mob-KnightUnit) * KnightMobEndgame);
            
            op[me] += knight_mob_opening[mob];
            eg[me] += knight_mob_endgame[mob];
            
            // opening outpost
            mob = 0;
            
            if (me == White) {
               if (board->square[from+17] != BP  &&     // SQUARE 'from' not under pawn attack, not in Toga1.4beta5cWHM(31)...
                   board->square[from+33] != BP  &&     // not in Toga1.4beta5cWHM(31).  
//                 board->square[from+49] != BP  &&     // not in Toga1.4beta5cWHM(31).  
//                 board->square[from+47] != BP  &&     // not in Toga1.4beta5cWHM(31)...
                   board->square[from+31] != BP  &&     // not in Toga1.4beta5cWHM(31).  
                   board->square[from+15] != BP     ) { // SQUARE 'from' not under pawn attack, not in Toga1.4beta5cWHM(31). 
                  
                  if (board->square[from-17] == WP) mob += KnightOutpostMatrix[me][SquareTo64[from]]; 
                  if (board->square[from-15] == WP) mob += KnightOutpostMatrix[me][SquareTo64[from]]; 
               }
               
            } else {
               
               if (board->square[from-17] != WP  &&     // SQUARE 'from' not under pawn attack, not in Toga1.4beta5cWHM(31)...
                   board->square[from-33] != WP  &&     // not in Toga1.4beta5cWHM(31).  
//                 board->square[from-49] != WP  &&     // not in Toga1.4beta5cWHM(31).  
//                 board->square[from-47] != WP  &&     // not in Toga1.4beta5cWHM(31)...
                   board->square[from-31] != WP  &&     // not in Toga1.4beta5cWHM(31).  
                   board->square[from-15] != WP     ) { // SQUARE 'from' not under pawn attack, not in Toga1.4beta5cWHM(31). 
                  
                  if (board->square[from+17] == BP) mob += KnightOutpostMatrix[me][SquareTo64[from]]; 
                  if (board->square[from+15] == BP) mob += KnightOutpostMatrix[me][SquareTo64[from]]; 
               }
            } 
            op[me] += mob;
            // opening outpost end

            break;

         case Bishop64:

            // mobility

            mob = 0;

            for (to = from-17; capture=board->square[to], THROUGH(capture); to -= 17) {mob += MobMove; ASSERT(Empty == capture);}
            mob += unit[capture];
            ASSERT(capture != Empty);

            for (to = from-15; capture=board->square[to], THROUGH(capture); to -= 15) {mob += MobMove; ASSERT(Empty == capture);}
            mob += unit[capture];
            ASSERT(capture != Empty);

            for (to = from+15; capture=board->square[to], THROUGH(capture); to += 15) {mob += MobMove; ASSERT(Empty == capture);}
            mob += unit[capture];
            ASSERT(capture != Empty);

            for (to = from+17; capture=board->square[to], THROUGH(capture); to += 17) {mob += MobMove; ASSERT(Empty == capture);}
            mob += unit[capture];
            ASSERT(capture != Empty);

            op[me] += bishop_mob_opening[mob];
            eg[me] += bishop_mob_endgame[mob];

            ASSERT(bishop_mob_opening[mob] == (mob-BishopUnit) * BishopMobOpening);
            ASSERT(bishop_mob_endgame[mob] == (mob-BishopUnit) * BishopMobEndgame);

            break;

         case Rook64:

            // mobility

            mob = 0;

            for (to = from-16; capture=board->square[to], THROUGH(capture); to -= 16) {mob += MobMove; ASSERT(Empty == capture);}
            mob += unit[capture];
            ASSERT(capture != Empty);

            for (to = from- 1; capture=board->square[to], THROUGH(capture); to -=  1) {mob += MobMove; ASSERT(Empty == capture);}
            mob += unit[capture];
            ASSERT(capture != Empty);

            for (to = from+ 1; capture=board->square[to], THROUGH(capture); to +=  1) {mob += MobMove; ASSERT(Empty == capture);} 
            mob += unit[capture];
            ASSERT(capture != Empty);

            for (to = from+16; capture=board->square[to], THROUGH(capture); to += 16) {mob += MobMove; ASSERT(Empty == capture);}
            mob += unit[capture];
            ASSERT(capture != Empty);

            op[me] += rook_mob_opening[mob];
            eg[me] += rook_mob_endgame[mob];

            ASSERT(rook_mob_opening[mob] == (mob-RookUnit) * RookMobOpening);
            ASSERT(rook_mob_endgame[mob] == (mob-RookUnit) * RookMobEndgame);

            // open file

            if (UseOpenFile) {

               op[me] -= RookOpenFileOpening / 2;
               eg[me] -= RookOpenFileEndgame / 2;

               rook_file = SQUARE_FILE(from);

               if (board->pawn_file[me][rook_file] == 0) { // no friendly pawn

                  op[me] += RookSemiOpenFileOpening;
                  eg[me] += RookSemiOpenFileEndgame;

                  if (board->pawn_file[opp][rook_file] == 0) { // no enemy pawn
                     op[me] += RookOpenFileOpening - RookSemiOpenFileOpening;
                     eg[me] += RookOpenFileEndgame - RookSemiOpenFileEndgame;
                  }

                  if ((mat_info->cflags[opp] & MatKingFlag) != 0) {

                     king = KING_POS(board,opp);
                     king_file = SQUARE_FILE(king);

                     delta = abs(rook_file-king_file); // file distance

                     if (delta <= 1) {
                        op[me] += RookSemiKingFileOpening;
                        if (delta == 0) op[me] += RookKingFileOpening - RookSemiKingFileOpening;
                     }
                  }
               }
            }

/*          // 7th rank moved to eval__attack()

            if (PAWN_RANK(from,me) == Rank7) {
               if ((pawn_info->flags[opp] & BackRankFlag) != 0 // opponent pawn on 7th rank
                || PAWN_RANK(KING_POS(board,opp),me) == Rank8) {
                  op[me] +=     Rook7thOpening;
                  eg[me] +=     Rook7thEndgame;
               }
            } */

            break;

         case Queen64:

            // mobility
            
            new_mob = 10 - TROPISM(KING_POS(board,opp),from);
            
            op[me] +=  new_mob; // TODO: try (1 + new_mob) / 2 instead.  
            eg[me] +=  new_mob; 

/*          // 7th rank moved to eval__attack()

            if (PAWN_RANK(from,me) == Rank7) {
               if ((pawn_info->flags[opp] & BackRankFlag) != 0 // opponent pawn on 7th rank
                || PAWN_RANK(KING_POS(board,opp),me) == Rank8) {
                  op[me] +=     Queen7thOpening;
                  eg[me] +=     Queen7thEndgame; 
               }
            } */

            break;
         }
      }
   }
   
   // update

   *opening += ((op[White] - op[Black]) * PieceActivityWeight) / 256;
// *endgame += ((eg[White] - eg[Black]) * PieceActivityWeight) / 256;
   *endgame += ((eg[White] - eg[Black]) * PieceActivityWeight) / 285; // not in Toga1.4beta5cWHM(31).  
}

// eval_piece_holes() // not in Toga1.4beta5cWHM(31).  

static void eval_piece_holes(const board_t * board, const material_info_t * mat_info, const pawn_info_t * pawn_info, int * opening, int * endgame) {

   const sq_t * ptr;
   const int * unit;
   
   int mob;
   int capture;
   int from, to;

   int op[ColourNb], eg[ColourNb];
   
   int colour;
   int me, opp;
   int piece;
   int rook_file, king_file;
   int king;
   int delta;
   int new_mob;
   
   uint8 mob_move[256]; // memset() sets a set of bytes, consecutive in mem.  
   
   ASSERT(board!=NULL);
   ASSERT(mat_info!=NULL);
   ASSERT(pawn_info!=NULL);
   ASSERT(opening!=NULL);
   ASSERT(endgame!=NULL);

   // init

   for (colour = 0; colour < ColourNb; colour++) {
      op[colour] = 0;
      eg[colour] = 0;
   }

   // eval

   for (colour = 0; colour < ColourNb; colour++) {

      me = colour;
      opp = COLOUR_OPP(me);

      unit = MobUnit[me];

      // set holes at opp's pawn attack squares

      memset(mob_move, 1, 256); // memset() sets a set of bytes, consecutive in mem.  

      for (ptr = &board->pawn[opp][0]; (from=*ptr) != SquareNone; ptr++) {

         ASSERT(*ptr >= 0);

         to = from + PAWN_MOVE_INC(opp);

         mob_move[to - 1] = 0; // punch a hole at the pawn attack square, 16x16 board
         mob_move[to + 1] = 0; // punch a hole at the pawn attack square, 16x16 board

         // TODO: paser trace + king trace added in...
      }

      // piece loop
      // HACK:         no king == [1]
      for (ptr = &board->piece[me][1]; (from=*ptr) != SquareNone; ptr++) {

         ASSERT(*ptr >= 0);

         piece = board->square[from];

         switch (PIECE_TYPE(piece)) {

         case Knight64:

            // mobility

            mob = 0; ASSERT(from >= 33);

            to = from-33; capture = board->square[to]; if (THROUGH(capture)) mob += mob_move[to]; else mob += unit[capture]; ASSERT(board->square[to] >= 0);
            to = from-31; capture = board->square[to]; if (THROUGH(capture)) mob += mob_move[to]; else mob += unit[capture]; ASSERT(board->square[to] >= 0);
            to = from-18; capture = board->square[to]; if (THROUGH(capture)) mob += mob_move[to]; else mob += unit[capture]; ASSERT(board->square[to] >= 0);
            to = from-14; capture = board->square[to]; if (THROUGH(capture)) mob += mob_move[to]; else mob += unit[capture]; ASSERT(board->square[to] >= 0);
            to = from+14; capture = board->square[to]; if (THROUGH(capture)) mob += mob_move[to]; else mob += unit[capture]; ASSERT(board->square[to] >= 0);
            to = from+18; capture = board->square[to]; if (THROUGH(capture)) mob += mob_move[to]; else mob += unit[capture]; ASSERT(board->square[to] >= 0);
            to = from+31; capture = board->square[to]; if (THROUGH(capture)) mob += mob_move[to]; else mob += unit[capture]; ASSERT(board->square[to] >= 0);
            to = from+33; capture = board->square[to]; if (THROUGH(capture)) mob += mob_move[to]; else mob += unit[capture]; ASSERT(board->square[to] >= 0);

            op[me] += knight_mob_opening[mob];
            eg[me] += knight_mob_endgame[mob];

            ASSERT(knight_mob_opening[mob] == (int(mob)-KnightUnit) * KnightMobOpening);
            ASSERT(knight_mob_endgame[mob] == (int(mob)-KnightUnit) * KnightMobEndgame);

            // opening outpost
            if (mob_move[from] != 0) { // WHM knight not attacked by opp pawn.  
               mob = 0;
               if (me == White) {
                  if (mob_move[from+16] != 0) { // WHM
                     if (board->square[from-17] == WP) mob += KnightOutpostMatrix[me][SquareTo64[from]]; 
                     if (board->square[from-15] == WP) mob += KnightOutpostMatrix[me][SquareTo64[from]]; 
                  }
               } else {
                  if (mob_move[from-16] != 0) { // WHM
                     if (board->square[from+17] == BP) mob += KnightOutpostMatrix[me][SquareTo64[from]]; 
                     if (board->square[from+15] == BP) mob += KnightOutpostMatrix[me][SquareTo64[from]]; 
                  }
               } 
               op[me] += mob;
            }
            // opening outpost end

            break;

         case Bishop64:

            // mobility

            mob = 0;

            for (to = from-17; capture=board->square[to], THROUGH(capture); to -= 17) {mob += mob_move[to]; ASSERT(Empty == capture);}
            mob += unit[capture];
            ASSERT(capture != Empty);

            for (to = from-15; capture=board->square[to], THROUGH(capture); to -= 15) {mob += mob_move[to]; ASSERT(Empty == capture);}
            mob += unit[capture];
            ASSERT(capture != Empty);

            for (to = from+15; capture=board->square[to], THROUGH(capture); to += 15) {mob += mob_move[to]; ASSERT(Empty == capture);}
            mob += unit[capture];
            ASSERT(capture != Empty);

            for (to = from+17; capture=board->square[to], THROUGH(capture); to += 17) {mob += mob_move[to]; ASSERT(Empty == capture);}
            mob += unit[capture];
            ASSERT(capture != Empty);

            op[me] += bishop_mob_opening[mob];
            eg[me] += bishop_mob_endgame[mob];

            ASSERT(bishop_mob_opening[mob] == (mob-BishopUnit) * BishopMobOpening);
            ASSERT(bishop_mob_endgame[mob] == (mob-BishopUnit) * BishopMobEndgame);

            break;

         case Rook64:

            // mobility

            mob = 0;

            for (to = from-16; capture=board->square[to], THROUGH(capture); to -= 16) {mob += mob_move[to]; ASSERT(Empty == capture);}
            mob += unit[capture];
            ASSERT(capture != Empty);

            for (to = from- 1; capture=board->square[to], THROUGH(capture); to -=  1) {mob += mob_move[to]; ASSERT(Empty == capture);}
            mob += unit[capture];
            ASSERT(capture != Empty);

            for (to = from+ 1; capture=board->square[to], THROUGH(capture); to +=  1) {mob += mob_move[to]; ASSERT(Empty == capture);} 
            mob += unit[capture];
            ASSERT(capture != Empty);

            for (to = from+16; capture=board->square[to], THROUGH(capture); to += 16) {mob += mob_move[to]; ASSERT(Empty == capture);}
            mob += unit[capture];
            ASSERT(capture != Empty);

            op[me] += rook_mob_opening[mob];
            eg[me] += rook_mob_endgame[mob];

            ASSERT(rook_mob_opening[mob] == (mob-RookUnit) * RookMobOpening);
            ASSERT(rook_mob_endgame[mob] == (mob-RookUnit) * RookMobEndgame);

            // open file

            if (UseOpenFile) {

               op[me] -= RookOpenFileOpening / 2;
               eg[me] -= RookOpenFileEndgame / 2;

               rook_file = SQUARE_FILE(from);

               if (board->pawn_file[me][rook_file] == 0) { // no friendly pawn

                  op[me] += RookSemiOpenFileOpening;
                  eg[me] += RookSemiOpenFileEndgame;

                  if (board->pawn_file[opp][rook_file] == 0) { // no enemy pawn
                     op[me] += RookOpenFileOpening - RookSemiOpenFileOpening;
                     eg[me] += RookOpenFileEndgame - RookSemiOpenFileEndgame;
                  }

                  if ((mat_info->cflags[opp] & MatKingFlag) != 0) {

                     king = KING_POS(board,opp);
                     king_file = SQUARE_FILE(king);

                     delta = abs(rook_file-king_file); // file distance

                     if (delta <= 1) {
                        op[me] += RookSemiKingFileOpening;
                        if (delta == 0) op[me] += RookKingFileOpening - RookSemiKingFileOpening;
                     }
                  }
               }
            }

/*          // 7th rank moved to eval__attack()

            if (PAWN_RANK(from,me) == Rank7) {
               if ((pawn_info->flags[opp] & BackRankFlag) != 0 // opponent pawn on 7th rank
                || PAWN_RANK(KING_POS(board,opp),me) == Rank8) {
                  op[me] +=     Rook7thOpening;
                  eg[me] +=     Rook7thEndgame;
               }
            } */

            break;

         case Queen64:

            // mobility
            
            new_mob = 10 - TROPISM(KING_POS(board,opp),from);
            
            op[me] +=  (1 + new_mob) / 2; 
            eg[me] +=       new_mob; 

/*          // 7th rank moved to eval__attack()

            if (PAWN_RANK(from,me) == Rank7) {
               if ((pawn_info->flags[opp] & BackRankFlag) != 0 // opponent pawn on 7th rank
                || PAWN_RANK(KING_POS(board,opp),me) == Rank8) {
                  op[me] +=     Queen7thOpening;
                  eg[me] +=     Queen7thEndgame; 
               }
            } */

            break;
         }
      }
   }

   // update

   *opening += ((op[White] - op[Black]) * PieceActivityWeight) / 256;
// *endgame += ((eg[White] - eg[Black]) * PieceActivityWeight) / 256;
   *endgame += ((eg[White] - eg[Black]) * PieceActivityWeight) / 285; // not in Toga1.4beta5cWHM(31).  
}

// eval__attack()   {King Attack + Piece Invasion + 7th rank rooks and queens}

static void eval__attack(const board_t * board, const material_info_t * mat_info, const pawn_info_t * pawn_info, int * opening, int * endgame) {

   const sq_t * ptr;

   int op[ColourNb], eg[ColourNb];

   int colour;
   int me, opp;
   int from;
   int penalty;
   int king;
   int piece;
   int attack_tot;
   int piece_nb;
   int pi_attack_tot;
   int pi_piece_nb;
// int rooks_7;
   
   int pawn_phase;         // WHM
   int penalty_inc;        // WHM
   
   ASSERT(board!=NULL);
   ASSERT(mat_info!=NULL);
   ASSERT(opening!=NULL);
   ASSERT(endgame!=NULL);

   // init

   for (colour = 0; colour < ColourNb; colour++) {
      op[colour] = 0;
      eg[colour] = 0;
   }

   // "King Attack" plus "Piece Invasion" plus 7th rank stuff.  

   for (colour = 0; colour < ColourNb; colour++) {

      me = colour;
      opp = COLOUR_OPP(me);
         
      king = KING_POS(board,me);
      ASSERT(SQUARE_IS_OK(king));
      
      // piece attacks against my king, "King Attack"
      attack_tot = 0;
      piece_nb = 0;
//    rooks_7 = 0;
         
      // Thomas' piece invasion
      pi_attack_tot = 0;
      pi_piece_nb = 0;
         
      // HACK:          no king == [1]
      for (ptr = &board->piece[opp][1]; (from=*ptr) != SquareNone; ptr++) {
            
         piece = board->square[from];
         
         // "King Attack"
         if ((mat_info->cflags[colour] & MatKingFlag) != 0) { // "King Attack".
             if (piece_attack_king(board,piece,from,king)) {
                 piece_nb++;
                 attack_tot += KingAttackUnit[piece];
             }
         }
         
         // Thomas' "Piece Invasion"
         if (PAWN_RANK(from,opp) >= Rank5) {
             pi_piece_nb++;
             pi_attack_tot += PieceInvasionUnit[piece];
         }

         // 7th rank stuff
         if (PIECE_IS_ROOK(piece)) {
            if (PAWN_RANK(from,opp) == Rank7) {                     // opp rook on my second rank == opp 7th rank
               if (   (pawn_info->flags[me] & BackRankFlag) != 0    //  my pawn on my second rank == opp 7th rank
                   || PAWN_RANK(KING_POS(board,me),opp) == Rank8) { //  my king on my first rank  == opp 8th rank

//                rooks_7++; // not in Toga1.4beta5cWHM(31).  

//                if (rooks_7 <= 1) {
//                    print_board(board);
                      op[opp] += Rook7thOpening;
                      eg[opp] += Rook7thEndgame;
//                } else {
//                    print_board(board);
//                    op[opp] += 2 * Rook7thOpening; // extra value not in Toga1.4beta5cWHM(31).  
//                    eg[opp] += 2 * Rook7thEndgame; // extra value not in Toga1.4beta5cWHM(31).  
//                }
               }
            } 

         } else if (PIECE_IS_QUEEN(piece)) {

            // 7th rank stuff
            if (PAWN_RANK(from,opp) == Rank7) {
               if (   (pawn_info->flags[me] & BackRankFlag) != 0 // pawn on 7th rank
                   ||  PAWN_RANK(KING_POS(board,me),opp) == Rank8) {

//                print_board(board);
                  op[opp] += Queen7thOpening;
                  eg[opp] += Queen7thEndgame; 
               }
            } 
         }
      }
         
      // Thomas' piece invasion.
      penalty = pi_attack_tot * pi_piece_nb;
      penalty *= (board->pawn_size[White] + board->pawn_size[Black]);
      penalty /= 16;
      if (256 != PieceInvasion) {
        penalty *= PieceInvasion;
        penalty /= 256;
      }
      op[colour] -= penalty;
      
      // "King Attack" scoring
      ASSERT(piece_nb >= 0 && piece_nb < 16);
      
      if (piece_nb >= 2) { // gangup always required in all fruit's and toga's
         
         penalty = (attack_tot * KingAttackOpening * KingAttackPieceNBArray[piece_nb]) / 256;
         ASSERT(penalty <= 255);
         ASSERT(PIECE_IS_QUEEN(board->square[board->piece[opp][1]]));
         ASSERT(board->number[WhiteQueen12 + opp] >= 1 && (1==opp || 0==opp));
         ASSERT(board->piece_size[opp] >= 3);
         
         // WHM "King Attack" adjustments...
         penalty_inc = 0;
         
//TODO:  // king in corner {or side}  TODO: tune and test.  
//TODO:  if (SQUARE_FILE(king) == FileA  ||  SQUARE_FILE(king) == FileH) {  // WHM
//TODO:     penalty_inc += penalty / 10;                                    // WHM 10% increase
//TODO:  }                                                                  // WHM
         
         // pawn phase  TODO: tune and test.  
         pawn_phase= 16 * (16 - board->pawn_size[White] - board->pawn_size[Black]);
         if (pawn_phase > mat_info->phase) {                                // WHM lotsa pawns were whacked.  
            penalty_inc += penalty * (pawn_phase - mat_info->phase) / 1280; // WHM full game -> 20% increase, Q+R 60->72
         }   
         
         penalty += penalty_inc;                                            // WHM
         ASSERT(penalty <= 275);                                            // WHM
         
         if (KingAttackWeight != 256) {
            penalty *= KingAttackWeight;
            penalty /= 256;
         } 
         
         op[colour] -= penalty;
      }
   }
   
   
   // update
   
   *opening += (op[White] - op[Black]);
   *endgame += (eg[White] - eg[Black]);
}

// eval_king_safety()

static void eval_king_safety(const board_t * board, const material_info_t * mat_info, int * opening, int * endgame) {

   int op[ColourNb], eg[ColourNb];

   int colour;
   int me;
   int penalty;
   int penalty_1, penalty_2, tmp;
   int king;
   int file;
   
   ASSERT(board!=NULL);
   ASSERT(mat_info!=NULL);
   ASSERT(opening!=NULL);
   ASSERT(endgame!=NULL);
   
   // init
   
   for (colour = 0; colour < ColourNb; colour++) {
      op[colour] = 0;
      eg[colour] = 0;
   }
   
   // White pawn shelter - "King Safety"

   if ((mat_info->cflags[White] & MatKingFlag) != 0) { 

      ASSERT(board->number[BlackQueen12] > 0 && board->piece_size[Black] >= 3);  

      // White init

      penalty = 0;
      me = White;
      king = KING_POS(board,me);
      file = SQUARE_FILE(king);
      
      // king

      penalty_1 = shelter_square(board,mat_info,king,me);

      // castling

      penalty_2 = penalty_1;

      if ((board->flags & FlagsWhiteKingCastle) != 0) {
         tmp = shelter_square(board,mat_info,G1,me);
         if (tmp < penalty_2) penalty_2 = tmp;
      }

      if ((board->flags & FlagsWhiteQueenCastle) != 0) {
         tmp = shelter_square(board,mat_info,B1,me);
         if (tmp < penalty_2) penalty_2 = tmp;
      }

      ASSERT(penalty_2>=0&&penalty_2<=penalty_1);

      // penalty

      penalty = (penalty_1 + penalty_2) / 2;
      ASSERT(penalty>=0);
      
      penalty                    += storm_file(board,file,  me);
      if (file != FileA) penalty += storm_file(board,file-1,me);
      if (file != FileH) penalty += storm_file(board,file+1,me);
      
      if (KingSafetyWeight != 256) {
         penalty *= KingSafetyWeight;
         penalty /= 256;
      }
          
      op[me] -= penalty;
   }
   
   
   // Black pawn shelter - "King Safety"

   if ((mat_info->cflags[Black] & MatKingFlag) != 0) { 

      ASSERT(board->number[WhiteQueen12] > 0 && board->piece_size[White] >= 3);

      // Black init

      penalty = 0;
      me = Black;
      king = KING_POS(board,me);
      file = SQUARE_FILE(king);
      
      // king

      penalty_1 = shelter_square(board,mat_info,king,me);

      // castling

      penalty_2 = penalty_1;

      if ((board->flags & FlagsBlackKingCastle) != 0) {
         tmp = shelter_square(board,mat_info,G8,me);
         if (tmp < penalty_2) penalty_2 = tmp;
      }

      if ((board->flags & FlagsBlackQueenCastle) != 0) {
         tmp = shelter_square(board,mat_info,B8,me);
         if (tmp < penalty_2) penalty_2 = tmp;
      }

      ASSERT(penalty_2>=0&&penalty_2<=penalty_1);

      // penalty

      penalty = (penalty_1 + penalty_2) / 2;
      ASSERT(penalty>=0);
      
      penalty                    += storm_file(board,file,  me);
      if (file != FileA) penalty += storm_file(board,file-1,me);
      if (file != FileH) penalty += storm_file(board,file+1,me);
                
      if (KingSafetyWeight != 256) {
         penalty *= KingSafetyWeight;
         penalty /= 256;
      }

      op[me] -= penalty;
   }
   
   
   // update
   
   *opening += (op[White] - op[Black]);
   *endgame += (eg[White] - eg[Black]);
}

// eval_passer()

static void eval_passer(const board_t * board, const pawn_info_t * pawn_info, int * opening, int * endgame, bool full_q_eval) {

   int colour;
   int op[ColourNb], eg[ColourNb];
   int att, def;
   int bits;
   int file, rank;
   int sq;
// int min, max;
   int delta;
   
   ASSERT(board!=NULL);
   ASSERT(pawn_info!=NULL);
   ASSERT(opening!=NULL);
   ASSERT(endgame!=NULL);

   // init

   for (colour = 0; colour < ColourNb; colour++) {
      op[colour] = 0;
      eg[colour] = 0;
   }

   // passed pawns

   for (colour = 0; colour < ColourNb; colour++) {

      att = colour;
      def = COLOUR_OPP(att);

      for (bits = pawn_info->passed_bits[att]; bits != 0; bits &= bits-1) {

         file = BIT_FIRST(bits);
         ASSERT(file>=FileA&&file<=FileH);

         rank = BIT_LAST(board->pawn_file[att][file]);
         ASSERT(rank>=Rank2&&rank<=Rank7);

         sq = SQUARE_MAKE(file,rank);
         if (COLOUR_IS_BLACK(att)) sq = SQUARE_RANK_MIRROR(sq);

         ASSERT(PIECE_IS_PAWN(board->square[sq]));
         ASSERT(COLOUR_IS(board->square[sq],att));
         ASSERT(PAWN_RANK(sq,att)==rank); // WHM
         
         // opening scoring

         op[att] += quad(PassedOpeningMin,PassedOpeningMax,rank);

         // endgame scoring init

//       min = PassedEndgameMin;
//       max = PassedEndgameMax;

//       delta = max - min;
         delta = PassedEndgameMax - PassedEndgameMin;

         // "dangerous" bonus

         if (board->piece_size[def] <= 1 // defender has no piece
          && (unstoppable_passer(board,sq,att) || king_passer(board,sq,att))) {
            delta += UnStoppablePasser;
         } else if (free_passer(board,sq,att)) {
            delta += FreePasser;
         }
         
         // king-distance bonus
         
         if (delta < UnStoppablePasser && tough_king_passer(board,sq,att)) { // not in Toga1.4beta5cWHM(31).  
             delta += KingPasser;
             delta += 7 * DefenderDistance; // king chase distance battle has been won.  
         } else {
            delta -= pawn_att_dist(sq,KING_POS(board,att),att) * AttackerDistance;
            delta += pawn_def_dist(sq,KING_POS(board,def),att) * DefenderDistance;
         }
         
         
         ASSERT(delta > 84);
         ASSERT(delta < 365  ||  delta > UnStoppablePasser); // sometimes delta is much greater than VALUE_PIECE(minor) - VALUE_PIECE(pawn)
                                                            // and we sure want to win at least a minor...
         
         // when we can promote, we need to test that hypothesis.  
         if (full_q_eval                && 
             Rank7 == rank              && 
             att == board->turn         && 
             delta < UnStoppablePasser      ) {
             
             // if () then we can promote.  
             if ( Empty == board->square[PAWN_PROMOTE(sq,att)    ]       ||  
                 COLOUR_IS(board->square[PAWN_PROMOTE(sq,att) + 1],def)  ||
                 COLOUR_IS(board->square[PAWN_PROMOTE(sq,att) - 1],def)     ) {
                 
                 delta /= 2;
                 ASSERT(delta < 185); // minor sac is at least 325 - 90 or [235 to 255].  R-B-P is between [85 to 105].  
             }
         }
         
         // endgame scoring
         
         eg[att] += PassedEndgameMin; // min;
         eg[att] += quad(0,delta,rank);
      }
   }

   // update

   *opening += ((op[White] - op[Black]) * PassedPawnWeight) / 256;
   *endgame += ((eg[White] - eg[Black]) * PassedPawnWeight) / 256;
}

// eval_pattern()

static void eval_pattern(const board_t * board, int * opening, int * endgame) {

   ASSERT(board!=NULL);
   ASSERT(opening!=NULL);
   ASSERT(endgame!=NULL);

   // trapped bishop (7th rank)

   if ((board->square[A7] == WB && board->square[B6] == BP)
    || (board->square[B8] == WB && board->square[C7] == BP)) {
      *opening -= TrappedBishop;
      *endgame -= TrappedBishop;
   }

   if ((board->square[H7] == WB && board->square[G6] == BP)
    || (board->square[G8] == WB && board->square[F7] == BP)) {
      *opening -= TrappedBishop;
      *endgame -= TrappedBishop;
   }

   if ((board->square[A2] == BB && board->square[B3] == WP)
    || (board->square[B1] == BB && board->square[C2] == WP)) {
      *opening += TrappedBishop;
      *endgame += TrappedBishop;
   }

   if ((board->square[H2] == BB && board->square[G3] == WP)
    || (board->square[G1] == BB && board->square[F2] == WP)) {
      *opening += TrappedBishop;
      *endgame += TrappedBishop;
   }

   // trapped bishop (6th rank)

   if (board->square[A6] == WB && board->square[B5] == BP) {
      *opening -= TrappedBishop / 2;
      *endgame -= TrappedBishop / 2;
   }

   if (board->square[H6] == WB && board->square[G5] == BP) {
      *opening -= TrappedBishop / 2;
      *endgame -= TrappedBishop / 2;
   }

   if (board->square[A3] == BB && board->square[B4] == WP) {
      *opening += TrappedBishop / 2;
      *endgame += TrappedBishop / 2;
   }

   if (board->square[H3] == BB && board->square[G4] == WP) {
      *opening += TrappedBishop / 2;
      *endgame += TrappedBishop / 2;
   }

   // blocked bishop

   if (board->square[D2] == WP && board->square[D3] != Empty && board->square[C1] == WB) {
      *opening -= BlockedBishop;
   }

   if (board->square[E2] == WP && board->square[E3] != Empty && board->square[F1] == WB) {
      *opening -= BlockedBishop;
   }

   if (board->square[D7] == BP && board->square[D6] != Empty && board->square[C8] == BB) {
      *opening += BlockedBishop;
   }

   if (board->square[E7] == BP && board->square[E6] != Empty && board->square[F8] == BB) {
      *opening += BlockedBishop;
   }

   // blocked rook

   if ((board->square[C1] == WK || board->square[B1] == WK)
    && (board->square[A1] == WR || board->square[A2] == WR || board->square[B1] == WR)) {
      *opening -= BlockedRook;
   }

   if ((board->square[F1] == WK || board->square[G1] == WK)
    && (board->square[H1] == WR || board->square[H2] == WR || board->square[G1] == WR)) {
      *opening -= BlockedRook;
   }

   if ((board->square[C8] == BK || board->square[B8] == BK)
    && (board->square[A8] == BR || board->square[A7] == BR || board->square[B8] == BR)) {
      *opening += BlockedRook;
   }

   if ((board->square[F8] == BK || board->square[G8] == BK)
    && (board->square[H8] == BR || board->square[H7] == BR || board->square[G8] == BR)) {
      *opening += BlockedRook;
   }
}

// unstoppable_passer()

static bool unstoppable_passer(const board_t * board, int pawn, int colour) {

   int me, opp;
   int file, rank;
   int king;
   int prom;
   const sq_t * ptr;
   int sq;
   int dist;

   ASSERT(board!=NULL);
   ASSERT(SQUARE_IS_OK(pawn));
   ASSERT(COLOUR_IS_OK(colour));

   me = colour;
   opp = COLOUR_OPP(me);

   file = SQUARE_FILE(pawn);
   rank = PAWN_RANK(pawn,me);

   king = KING_POS(board,opp);
   ASSERT(1 == board->piece_size[opp]);

   // clear promotion path?

   for (ptr = &board->piece[me][0]; (sq=*ptr) != SquareNone; ptr++) {
      if (SQUARE_FILE(sq) == file && PAWN_RANK(sq,me) > rank) {
         return false; // "friendly" blocker
      }
   }

   // init

   if (rank == Rank2) {
      pawn += PAWN_MOVE_INC(me);
      rank++;
      ASSERT(rank==PAWN_RANK(pawn,me));
   }

   ASSERT(rank>=Rank3&&rank<=Rank7);

   prom = PAWN_PROMOTE(pawn,me);

   dist = DISTANCE(pawn,prom);
   ASSERT(dist==Rank8-rank);
   if (board->turn == opp) dist++;

   if (DISTANCE(king,prom) > dist) return true; // not in the square

   return false;
}

// king_passer()

static bool king_passer(const board_t * board, int pawn, int colour) {

   int me;
   int king;
   int file;
   int prom;

   ASSERT(board!=NULL);
   ASSERT(SQUARE_IS_OK(pawn));
   ASSERT(COLOUR_IS_OK(colour));

   me = colour;

   king = KING_POS(board,me);
   file = SQUARE_FILE(pawn);
   prom = PAWN_PROMOTE(pawn,me);

   if (DISTANCE(king,prom) <= 1
    && DISTANCE(king,pawn) <= 1
    && (SQUARE_FILE(king) != file
     || (file != FileA && file != FileH))) {
      return true;
   }

   return false;
}

// tough_king_passer()  by WHM; king not allowed on advance square.

static bool tough_king_passer(const board_t * board, int pawn, int colour) {

   int me;
   int king;
   int file;
   int prom;

   ASSERT(board!=NULL);
   ASSERT(SQUARE_IS_OK(pawn));
   ASSERT(COLOUR_IS_OK(colour));

   me = colour;

   king = KING_POS(board,me);
   file = SQUARE_FILE(pawn);
   prom = PAWN_PROMOTE(pawn,me);

   if (   DISTANCE(king,prom) <= 1
       && DISTANCE(king,pawn) <= 1
       && SQUARE_FILE(king)   != file) { // WHM king not allowed on advance square.
      
      return true;
   }

   return false;
}

// free_passer()

static bool free_passer(const board_t * board, int pawn, int colour) {

   int me;
// int opp;
   int inc;
   int sq;
   int move;

   ASSERT(board!=NULL);
   ASSERT(SQUARE_IS_OK(pawn));
   ASSERT(COLOUR_IS_OK(colour));

   me = colour;
// opp = COLOUR_OPP(me);

   inc = PAWN_MOVE_INC(me);
   sq = pawn + inc;
   ASSERT(SQUARE_IS_OK(sq));

   if (board->square[sq] != Empty) return false;

   move = MOVE_MAKE(pawn,sq);
   if (see_move(move,board) < 0) return false;

   return true;
}

// pawn_att_dist()

static int pawn_att_dist(int pawn, int king, int colour) {

   int me;
   int inc;
   int target;

   ASSERT(SQUARE_IS_OK(pawn));
   ASSERT(SQUARE_IS_OK(king));
   ASSERT(COLOUR_IS_OK(colour));

   me = colour;
   inc = PAWN_MOVE_INC(me);

   target = pawn + inc;

   ASSERT(DISTANCE(king,target) >= 0); // WHM
   ASSERT(DISTANCE(king,target) <= 7); // WHM

   return DISTANCE(king,target);
}

// pawn_def_dist()

static int pawn_def_dist(int pawn, int king, int colour) {

   int me;
   int inc;
   int target;

   ASSERT(SQUARE_IS_OK(pawn));
   ASSERT(SQUARE_IS_OK(king));
   ASSERT(COLOUR_IS_OK(colour));

   me = colour;
   inc = PAWN_MOVE_INC(me);

   target = pawn + inc;

   ASSERT(DISTANCE(king,target) >= 0); // WHM
   ASSERT(DISTANCE(king,target) <= 7); // WHM

   return DISTANCE(king,target);
}

// draw_init_list()

static void draw_init_list(int list[], const board_t * board, int pawn_colour) {

   int pos;
   int att, def;
   const sq_t * ptr;
   int sq;
   int pawn;
   int i;

   ASSERT(list!=NULL);
   ASSERT(board!=NULL);
   ASSERT(COLOUR_IS_OK(pawn_colour));

   // init

   pos = 0;

   att = pawn_colour;
   def = COLOUR_OPP(att);

   ASSERT(board->pawn_size[att]==1);
   ASSERT(board->pawn_size[def]==0);

   // att

   for (ptr = &board->piece[att][0]; (sq=*ptr) != SquareNone; ptr++) {
      list[pos++] = sq;
   }

   for (ptr = &board->pawn[att][0]; (sq=*ptr) != SquareNone; ptr++) {
      list[pos++] = sq;
   }

   // def

   for (ptr = &board->piece[def][0]; (sq=*ptr) != SquareNone; ptr++) {
      list[pos++] = sq;
   }

   for (ptr = &board->pawn[def][0]; (sq=*ptr) != SquareNone; ptr++) {
      list[pos++] = sq;
   }

   // end marker

   ASSERT(pos==board->piece_nb);

   list[pos] = SquareNone;

   // file flip?

   pawn = board->pawn[att][0];

   if (SQUARE_FILE(pawn) >= FileE) {
      for (i = 0; i < pos; i++) {
         list[i] = SQUARE_FILE_MIRROR(list[i]);
      }
   }

   // rank flip?

   if (COLOUR_IS_BLACK(pawn_colour)) {
      for (i = 0; i < pos; i++) {
         list[i] = SQUARE_RANK_MIRROR(list[i]);
      }
   }
}

// draw_kpkq()

static bool draw_kpkq(const int list[], int turn) {

   int wk, wp, bk, bq;
   int prom;
   int dist;
   // int wp_file, wp_rank;

   ASSERT(list!=NULL);
   ASSERT(COLOUR_IS_OK(turn));

   // load

   wk = *list++;
   ASSERT(SQUARE_IS_OK(wk));

   wp = *list++;
   ASSERT(SQUARE_IS_OK(wp));
   ASSERT(SQUARE_FILE(wp)<=FileD);

   bk = *list++;
   ASSERT(SQUARE_IS_OK(bk));

   bq = *list++;
   ASSERT(SQUARE_IS_OK(bq));

   ASSERT(*list==SquareNone);

   // test

   if (false) {

   } else if (wp == A7) {

      prom = A8;
      dist = 4;

      if (wk == B7 || wk == B8) { // best case
         if (COLOUR_IS_WHITE(turn)) dist--;
      } else if (wk == A8 || ((wk == C7 || wk == C8) && bq != A8)) { // white loses a tempo
         if (COLOUR_IS_BLACK(turn) && SQUARE_FILE(bq) != FileB) return false;
      } else {
         return false;
      }

      ASSERT(bq!=prom);
      if (DISTANCE(bk,prom) > dist) return true;

   } else if (wp == C7) {

      prom = C8;
      dist = 4;

      if (false) {

      } else if (wk == C8) { // dist = 0

         dist++; // self-blocking penalty
         if (COLOUR_IS_WHITE(turn)) dist--; // right-to-move bonus

      } else if (wk == B7 || wk == B8) { // dist = 1, right side

         dist--; // right-side bonus
         if (DELTA_INC_LINE(wp-bq) == wk-wp) dist++; // pinned-pawn penalty
         if (COLOUR_IS_WHITE(turn)) dist--; // right-to-move bonus

      } else if (wk == D7 || wk == D8) { // dist = 1, wrong side

         if (DELTA_INC_LINE(wp-bq) == wk-wp) dist++; // pinned-pawn penalty
         if (COLOUR_IS_WHITE(turn)) dist--; // right-to-move bonus

      } else if ((wk == A7 || wk == A8) && bq != C8) { // dist = 2, right side

         if (COLOUR_IS_BLACK(turn) && SQUARE_FILE(bq) != FileB) return false;

         dist--; // right-side bonus

      } else if ((wk == E7 || wk == E8) && bq != C8) { // dist = 2, wrong side

         if (COLOUR_IS_BLACK(turn) && SQUARE_FILE(bq) != FileD) return false;

      } else {

         return false;
      }

      ASSERT(bq!=prom);
      if (DISTANCE(bk,prom) > dist) return true;
   }

   return false;
}

// draw_kpkr()

static bool draw_kpkr(const int list[], int turn) {

   int wk, wp, bk, br;
   int wk_file, wk_rank;
   int wp_file, wp_rank;
   int br_file, br_rank;
   int inc;
   int prom;
   int dist;

   ASSERT(list!=NULL);
   ASSERT(COLOUR_IS_OK(turn));

   // load

   wk = *list++;
   ASSERT(SQUARE_IS_OK(wk));

   wp = *list++;
   ASSERT(SQUARE_IS_OK(wp));
   ASSERT(SQUARE_FILE(wp)<=FileD);

   bk = *list++;
   ASSERT(SQUARE_IS_OK(bk));

   br = *list++;
   ASSERT(SQUARE_IS_OK(br));

   ASSERT(*list==SquareNone);

   // init

   wk_file = SQUARE_FILE(wk);
   wk_rank = SQUARE_RANK(wk);

   wp_file = SQUARE_FILE(wp);
   wp_rank = SQUARE_RANK(wp);

   br_file = SQUARE_FILE(br);
   br_rank = SQUARE_RANK(br);

   inc = PAWN_MOVE_INC(White);
   prom = PAWN_PROMOTE(wp,White);

   // conditions

   if (false) {

   } else if (DISTANCE(wk,wp) == 1) {

      ASSERT(abs(wk_file-wp_file)<=1);
      ASSERT(abs(wk_rank-wp_rank)<=1);

      // no-op

   } else if (DISTANCE(wk,wp) == 2 && abs(wk_rank-wp_rank) <= 1) {

      ASSERT(abs(wk_file-wp_file)==2);
      ASSERT(abs(wk_rank-wp_rank)<=1);

      if (COLOUR_IS_BLACK(turn) && br_file != (wk_file + wp_file) / 2) return false;

   } else {

      return false;
   }

   // white features

   dist = DISTANCE(wk,prom) + DISTANCE(wp,prom);
   if (wk == prom) dist++;

   if (wk == wp+inc) { // king on pawn's "front square"
      if (wp_file == FileA) return false;
      dist++; // self-blocking penalty
   }

   // black features

   if (br_file != wp_file && br_rank != Rank8) {
      dist--; // misplaced-rook bonus
   }

   // test

   if (COLOUR_IS_WHITE(turn)) dist--; // right-to-move bonus

   if (DISTANCE(bk,prom) > dist) return true;

   return false;
}

// draw_kpkb()

static bool draw_kpkb(const int list[], int turn) {

   int wk, wp, bk, bb;
   int inc;
   int end, to;
   int delta, inc_2;
   int sq;

   ASSERT(list!=NULL);
   ASSERT(COLOUR_IS_OK(turn));

   // load

   wk = *list++;
   ASSERT(SQUARE_IS_OK(wk));

   wp = *list++;
   ASSERT(SQUARE_IS_OK(wp));
   ASSERT(SQUARE_FILE(wp)<=FileD);

   bk = *list++;
   ASSERT(SQUARE_IS_OK(bk));

   bb = *list++;
   ASSERT(SQUARE_IS_OK(bb));

   ASSERT(*list==SquareNone);

   // blocked pawn?

   inc = PAWN_MOVE_INC(White);
   end = PAWN_PROMOTE(wp,White) + inc;

   for (to = wp+inc; to != end; to += inc) {

      ASSERT(SQUARE_IS_OK(to));

      if (to == bb) return true; // direct blockade

      delta = to - bb;
      ASSERT(delta_is_ok(delta));

      if (PSEUDO_ATTACK(BB,delta)) {

         inc_2 = DELTA_INC_ALL(delta);
         ASSERT(inc_2!=IncNone);

         sq = bb;
         do {
            sq += inc_2;
            ASSERT(SQUARE_IS_OK(sq));
            ASSERT(sq!=wk);
            ASSERT(sq!=wp);
            ASSERT(sq!=bb);
            if (sq == to) return true; // indirect blockade
         } while (sq != bk);
      }
   }

   return false;
}

// draw_kpkn()

static bool draw_kpkn(const int list[], int turn) {

   int wk, wp, bk, bn;
   int inc;
   int end;
   int file;
   int sq;

   ASSERT(list!=NULL);
   ASSERT(COLOUR_IS_OK(turn));

   // load

   wk = *list++;
   ASSERT(SQUARE_IS_OK(wk));

   wp = *list++;
   ASSERT(SQUARE_IS_OK(wp));
   ASSERT(SQUARE_FILE(wp)<=FileD);

   bk = *list++;
   ASSERT(SQUARE_IS_OK(bk));

   bn = *list++;
   ASSERT(SQUARE_IS_OK(bn));

   ASSERT(*list==SquareNone);

   // blocked pawn?

   inc = PAWN_MOVE_INC(White);
   end = PAWN_PROMOTE(wp,White) + inc;

   file = SQUARE_FILE(wp);
   if (file == FileA || file == FileH) end -= inc;

   for (sq = wp+inc; sq != end; sq += inc) {

      ASSERT(SQUARE_IS_OK(sq));

      if (sq == bn || PSEUDO_ATTACK(BN,sq-bn)) return true; // blockade
   }

   return false;
}

// draw_knpk()

static bool draw_knpk(const int list[], int turn) {

   int wk, wn, wp, bk;

   ASSERT(list!=NULL);
   ASSERT(COLOUR_IS_OK(turn));

   // load

   wk = *list++;
   ASSERT(SQUARE_IS_OK(wk));

   wn = *list++;
   ASSERT(SQUARE_IS_OK(wn));

   wp = *list++;
   ASSERT(SQUARE_IS_OK(wp));
   ASSERT(SQUARE_FILE(wp)<=FileD);

   bk = *list++;
   ASSERT(SQUARE_IS_OK(bk));

   ASSERT(*list==SquareNone);

   // test

   if (wp == A7 && DISTANCE(bk,A8) <= 1) return true;

   return false;
}

// draw_krpkr()

static bool draw_krpkr(const int list[], int turn) {

   int wk, wr, wp, bk, br;
   int wp_file, wp_rank;
   int bk_file, bk_rank;
   int br_file, br_rank;
   int prom;

   ASSERT(list!=NULL);
   ASSERT(COLOUR_IS_OK(turn));

   // load

   wk = *list++;
   ASSERT(SQUARE_IS_OK(wk));

   wr = *list++;
   ASSERT(SQUARE_IS_OK(wr));

   wp = *list++;
   ASSERT(SQUARE_IS_OK(wp));
   ASSERT(SQUARE_FILE(wp)<=FileD);

   bk = *list++;
   ASSERT(SQUARE_IS_OK(bk));

   br = *list++;
   ASSERT(SQUARE_IS_OK(br));

   ASSERT(*list==SquareNone);

   // test

   wp_file = SQUARE_FILE(wp);
   wp_rank = SQUARE_RANK(wp);

   bk_file = SQUARE_FILE(bk);
   bk_rank = SQUARE_RANK(bk);

   br_file = SQUARE_FILE(br);
   br_rank = SQUARE_RANK(br);

   prom = PAWN_PROMOTE(wp,White);

   if (false) {

   } else if (bk == prom) {

      if (br_file > wp_file) return true;

   } else if (bk_file == wp_file && bk_rank > wp_rank) {

      return true;

   } else if (wr == prom && wp_rank == Rank7 && (bk == G7 || bk == H7) && br_file == wp_file) {

      if (br_rank <= Rank3) {
         if (DISTANCE(wk,wp) > 1) return true;
      } else { // br_rank >= Rank4
         if (DISTANCE(wk,wp) > 2) return true;
      }
   }

   return false;
}

// draw_kbpkb()

static bool draw_kbpkb(const int list[], int turn) {

   int wk, wb, wp, bk, bb;
   int inc;
   int end, to;
   int delta, inc_2;
   int sq;

   ASSERT(list!=NULL);
   ASSERT(COLOUR_IS_OK(turn));

   // load

   wk = *list++;
   ASSERT(SQUARE_IS_OK(wk));

   wb = *list++;
   ASSERT(SQUARE_IS_OK(wb));

   wp = *list++;
   ASSERT(SQUARE_IS_OK(wp));
   ASSERT(SQUARE_FILE(wp)<=FileD);

   bk = *list++;
   ASSERT(SQUARE_IS_OK(bk));

   bb = *list++;
   ASSERT(SQUARE_IS_OK(bb));

   ASSERT(*list==SquareNone);

   // opposit colour?

   if (SQUARE_COLOUR(wb) == SQUARE_COLOUR(bb)) return false;

   // blocked pawn?

   inc = PAWN_MOVE_INC(White);
   end = PAWN_PROMOTE(wp,White) + inc;

   for (to = wp+inc; to != end; to += inc) {

      ASSERT(SQUARE_IS_OK(to));

      if (to == bb) return true; // direct blockade

      delta = to - bb;
      ASSERT(delta_is_ok(delta));

      if (PSEUDO_ATTACK(BB,delta)) {

         inc_2 = DELTA_INC_ALL(delta);
         ASSERT(inc_2!=IncNone);

         sq = bb;
         do {
            sq += inc_2;
            ASSERT(SQUARE_IS_OK(sq));
            ASSERT(sq!=wk);
            ASSERT(sq!=wb);
            ASSERT(sq!=wp);
            ASSERT(sq!=bb);
            if (sq == to) return true; // indirect blockade
         } while (sq != bk);
      }
   }

   return false;
}

// shelter_square()

static int shelter_square(const board_t * board, const material_info_t * mat_info, int square, int colour) {

   int penalty;
   int file, rank;

   ASSERT(board!=NULL);
   ASSERT(SQUARE_IS_OK(square));
   ASSERT(COLOUR_IS_OK(colour));
   ASSERT(PIECE_IS_KING(board->square[square]) || B1 == square || G1 == square || B8 == square || G8 == square);
   
   penalty = 0;
   
   file = SQUARE_FILE(square);
   rank = PAWN_RANK(square,colour);
   
   penalty                    += shelter_file(board,file,  rank,colour) * 2;
   if (file != FileA) penalty += shelter_file(board,file-1,rank,colour);
   if (file != FileH) penalty += shelter_file(board,file+1,rank,colour);
   
   if (penalty == 0) penalty = WeakBackRank; // weak back rank
   
   penalty                    += storm_file(board,file,  colour);
   if (file != FileA) penalty += storm_file(board,file-1,colour);
   if (file != FileH) penalty += storm_file(board,file+1,colour);

   return penalty;
}

// shelter_file()

static int shelter_file(const board_t * board, int file, int rank, int colour) {

   int dist;
   int penalty;
   
   ASSERT(board!=NULL);
   ASSERT(file>=FileA&&file<=FileH);
   ASSERT(rank>=Rank1&&rank<=Rank8);
   ASSERT(COLOUR_IS_OK(colour));
   
   dist = BIT_FIRST(board->pawn_file[colour][file]&BitGE[rank]);
   ASSERT(dist>=Rank2&&dist<=Rank8);
   
   dist = Rank8 - dist;
   ASSERT(dist>=0&&dist<=6);
                                // 8   7   6   5   4   3  2 == rank of own pawn if king on Rank1, else translated by king rank.  
   penalty = 36 - dist * dist; // 36, 35, 32, 27, 20, 11, 0
   ASSERT(penalty>=0&&penalty<=36); 

   if (penalty == 36) { // WHM open files for opp rooks and queens are deadly...
       penalty  = 40;
   }
   
   return penalty;
}

// storm_file()

static int storm_file(const board_t * board, int file, int colour) {

   int dist;
   int penalty;

   ASSERT(board!=NULL);
   ASSERT(file>=FileA&&file<=FileH);
   ASSERT(COLOUR_IS_OK(colour));

   dist = BIT_LAST(board->pawn_file[COLOUR_OPP(colour)][file]);
   ASSERT(dist>=Rank1&&dist<=Rank7);

   penalty = 0;

   switch (dist) {
   case Rank4:
      penalty = StormOpening * 1;
      break;
   case Rank5:
      penalty = StormOpening * 2;
      break;
   case Rank6:
      penalty = StormOpening * 3;
      break;
   }

   return penalty;
}

// bishop_can_attack()  TODO: pawn blockades...

static bool bishop_can_attack(const board_t * board, int to, int colour) {

   const sq_t * ptr;
   int from;
   int piece;

   ASSERT(board!=NULL);
   ASSERT(SQUARE_IS_OK(to));
   ASSERT(COLOUR_IS_OK(colour));

   for (ptr = &board->piece[colour][1]; (from=*ptr) != SquareNone; ptr++) { // HACK: no king

      piece = board->square[from];

      if (PIECE_IS_BISHOP(piece) && SQUARE_COLOUR(from) == SQUARE_COLOUR(to)) {
         return true;
      }
   }

   return false;
}

// end of eval.cpp

