
// search_full.cpp

// includes

#include "attack.h"
#include "colour.h"
#include "eval.h"
#include "move.h"
#include "move_check.h"
#include "move_do.h"
#include "option.h"
#include "pawn.h"
#include "piece.h"
#include "probe.h"
#include "pv.h"
#include "recog.h"
#include "search.h"
#include "search_full.h"
#include "see.h"
#include "sort.h"
#include "trans.h"
#include "util.h"
#include "value.h"

// constants and variables

// things to try
static const bool TryEGBBToTrans = true; // try again with egbb before trans...

// main search
static const bool UseDistancePruning = true;

// transposition table
static const bool UseTrans = true;
static const int TransDepth = 1;
static const bool UseExact = true;

static const bool UseMateValues = true; // use mate values from shallower searches?

// null move
static const int NullDepth = 2;
static const int NullReduction = 3;
static const int VerReduction = 5;

// move ordering
static const bool UseIID = true;
static const int IIDDepth = 3;
static const int IIDReduction = 2;

// extensions
static  const  bool ExtendSingleReply = true; // true
static /*UCI*/ int  MateThreatDepth = 4;

// history pruning
static  const  bool UseHistory = true;
static  const  bool HistoryReDoSearch = true;
static  const  int  HistoryDepth = 3; // was 3
static  const  int  HistoryMoveNb = 3;
static  const  int  HistoryMoveNbNodePV = HistoryMoveNb+3;

static /*UCI*/ int  HistoryDropDepth= 6; //        0      1      2      3      4      5      6    ==   depth
static  const  int  HistoryDropValues[6 + 1] = {16384, 12288, 10240,  8192,  6144,  4096,  2048, }; // WHM matching Toga1.4beta5cWHM(31)

static /*UCI*/ int  HistoryValue[MaxThreads];
static /*UCI*/ int  TacticalDepth;
static /*UCI*/ int  JDsHangersDepth  = 8; // blitz good, deep?  

// futility pruning                              175  245  300
//                                               150  210  260  300
static /*UCI*/  int FutilityMargins[5 + 1] = {0, 135, 190, 230, 270, 300, }; // 0,1,2,3,4,5 == depth.  CFS==100,200,300,200,300.  
static /*UCI*/  int FutilityDepth = 5; // 1.2.1 was 2, 1.4.1SE was 5, 1.4beta5c was 3, 1.4beta5cWHM(31) was 5.  
static /*UCI*/  int RazorDepth = 1;

// quiescence search
static /* const */ bool UseDelta = true; // false
static /* const */ int DeltaMargin = 50;

static /* const */ int CheckNb = 1;
static /* const */ int CheckDepth[MaxThreads]; // 1 - CheckNb

// misc
static const int NodeShort = +1;
static const int NodeCut   = +3;
static const int NodePV    = 1+NodeCut;

// WSD == Win or Save Draw, by WHM.  
static /*UCI*/ bool UseWSD[MaxThreads]; // UCI
static  const  int  WSDThreshold = 225;

// free queen checks
static const bool UseFreeQueenCheck = true;
static search_param_t SearchStack[MaxThreads][HeightMax];

// "Root Alpha"
static /*UCI*/ bool UseRootAlpha = false;

// prototypes

static int  full_root(list_t * list, board_t * board, int alpha, int beta, int depth, int height, int search_type, int ThreadId);

static int  full_search    (board_t * board, int alpha, int beta, int depth, int height, int node_type, mv_t pv[],                                          int ThreadId);
static int  full_no_null   (board_t * board, int alpha, int beta, int depth, int height, int node_type, int * best_move, int trans_move, attack_t * attack, int ThreadId);
static int  full_quiescence(board_t * board, int alpha, int beta, int depth, int height,                                                                    int ThreadId);

static int  full_new_depth   (int depth,             int move, board_t * board,                    bool * move_check, int history_score, int ThreadId);
static int  full_new_depth_PV(int depth, int height, int move, board_t * board, bool single_reply, bool * move_check, int history_score, int ThreadId);

// use null move if the side-to-move has at least one piece {+king}
static bool do_null    (const board_t * board) {return board->piece_size[           board->turn ] >= 2;}
static bool do_futility(const board_t * board) {return board->piece_size[COLOUR_OPP(board->turn)] >= 2;}
// use futility  if opp side-to-move has at least one piece {+king}

static bool move_is_dangerous    (int move, const board_t * board);
static bool capture_is_dangerous (int move, const board_t * board);

static bool simple_stalemate     (const board_t * board);

static bool is_passed            (const board_t * board, int to);

static bool passed_pawn_move     (int move, const board_t * board);

static bool king_is_blocked      (const board_t * board, int sq, int king_colour, int king_sq);

#if DEBUG
#include <math.h> // for sqrt()
static bool lead_passed_pawn_move(int move, const board_t * board);
#endif

// functions

// search_full_init()

void search_full_init(list_t * list, board_t * board, int ThreadId) {

   const char * string;
   int trans_move, trans_depth, trans_flags, trans_value;

   ASSERT(list_is_ok(list));
   ASSERT(board_is_ok(board));

   // Playing Style

   string = option_get_string("Playing Style");

   Mixed = false;
   if (my_string_equal(string,"Tactical")) {
      Tactical[ThreadId] = true;
   } else if (my_string_equal(string,"Positional")) {
      Tactical[ThreadId] = false;
   } else {
      ASSERT(my_string_equal(string,"Mixed"));
      if (2 * ThreadId < NumberThreadsInternal) Tactical[ThreadId] =  true; // thread 0 is Tactical.
      else                                      Tactical[ThreadId] = false; // threads (NumberThreads/2) -> (NumberThreads-1) are Positional.
      if (NumberThreadsInternal >= 2) Mixed = true;
      else ASSERT(Tactical[ThreadId]);
   }
   
   // history-pruning, used for NodePV history pruning when not Tactical == "Positional".  

   HistoryValue[ThreadId] = (option_get_int("Positional History Value") * HistoryMax + 50) / 100;
   HistoryDropDepth       =  option_get_int("History Drop Depth");

   // "Tactical Depth"
   TacticalDepth = option_get_int("Tactical Depth");
   if (TacticalDepth < 2) {
       TacticalDepth = 0;
   } else {
       TacticalDepth += (16 - board->number[WhitePawn12] - board->number[BlackPawn12]) / 3;
   }
   
   JDsHangersDepth  = option_get_int("JD's Hangers Depth");
   
   // Win or Save Draw by WHM.  Junior killer?  

   if (option_get_bool("WSD")) {
      if (Mixed) {
         UseWSD[ThreadId] = !Tactical[ThreadId]; // WSD used only with the second half of threads, the Positional threads.  
      } else {
         UseWSD[ThreadId] = true;
      }
   } else {
      UseWSD[ThreadId] = false;
   }

   // futility-pruning options

   FutilityDepth = option_get_int("Futility Depth");

   FutilityMargins[1] = option_get_int("Futility Margin 1");
   FutilityMargins[2] = option_get_int("Futility Margin 2");
   FutilityMargins[3] = option_get_int("Futility Margin 3");
   FutilityMargins[4] = option_get_int("Futility Margin 4");
   FutilityMargins[5] = option_get_int("Futility Margin 5");
   
   RazorDepth = option_get_int("Razor Depth");

   if (board->number[WhiteQueen12] == 0  &&  
       board->number[BlackQueen12] == 0     ) {
       
       if (board->number[WhiteKnight12] >= 1  ||
           board->number[BlackKnight12] >= 1  ||
           board->number[WhiteBishop12] >= 1  ||
           board->number[BlackBishop12] >= 1     ) {

           if (FutilityMargins[2] > 240) {
               FutilityMargins[2] = 240;
           }
           if (FutilityDepth < 5) {
               FutilityDepth = 5;
           }
       }
   }
   
   // delta-pruning options
   
   UseDelta = option_get_bool("Delta Pruning");
   DeltaMargin = option_get_int("Delta Margin");

   // quiescence-search options

   CheckNb = option_get_int("Quiescence Check Plies");
   CheckDepth[ThreadId] = 1 - CheckNb;

   // "Root Alpha" and "Skip Moves"

   UseRootAlpha = option_get_bool("Root Alpha");
   SkipMoves = option_get_bool("Skip Moves");

   if (SearchInput->infinite && SearchInput->multipv >= 1) UseRootAlpha = false; // WHM MultiPV analysis mode needs this off...

   // "Mate Threat Depth"
   MateThreatDepth = option_get_int("Mate Threat Depth");

   // standard sort
   list_note(list);
   list_sort(list);
  
   // basic sort
   trans_move = MoveNone;
   if (UseTrans) trans_retrieve(Trans,board->key,&trans_move,&trans_depth,&trans_flags,&trans_value);
   note_moves(list,board,0,trans_move,ThreadId);
   list_sort(list);
}

// search_full_root()

int search_full_root(list_t * list, board_t * board, int depth, int search_type, int ThreadId) {

   int value;
   
   ASSERT(list_is_ok(list));
   ASSERT(board_is_ok(board));
   ASSERT(depth_is_ok(depth));
   ASSERT(search_type==SearchNormal||search_type==SearchShort);
   ASSERT(ThreadId>=0 && ThreadId<NumberThreadsInternal);

   ASSERT(list==SearchRoot[ThreadId]->list);
   ASSERT(!LIST_IS_EMPTY(list));
   ASSERT(board==SearchCurrent[ThreadId]->board);
   ASSERT(board_is_legal(board));
   ASSERT(depth>=1);
   
   // To original fruit 2.1:    -ValueInf,+ValueInf
   value = full_root(list,board,-ValueInf,+ValueInf,depth,0,search_type, ThreadId);
   
   ASSERT(value_is_ok(value));
   ASSERT(LIST_VALUE(list,0)==value || UseRootAlpha || SearchInfo[ThreadId]->stop); // SearchInfo[ThreadId]->stop shared.  
   
   return value;
}

// full_root()

static int full_root(list_t * list, board_t * board, int alpha, int beta, int depth, int height, int search_type, int ThreadId) {

   int old_alpha;
   int value, best_value;
   int i, move, j, k;
   int new_depth;
   int ntries = 0;
   int i1023 = 0;

   undo_t undo[1];
   mv_t new_pv[HeightMax];

   bool found;
   bool in_check; // WHM
   bool move_check;
   bool already_claimed; // WHM;

   ASSERT(list_is_ok(list));
   ASSERT(board_is_ok(board));
   ASSERT(range_is_ok(alpha,beta));
   ASSERT(depth_is_ok(depth));
   ASSERT(height_is_ok(height));
   ASSERT(search_type==SearchNormal||search_type==SearchShort);
   ASSERT(ThreadId>=0 && ThreadId<NumberThreadsInternal);

   ASSERT(list==SearchRoot[ThreadId]->list);
   ASSERT(!LIST_IS_EMPTY(list));
   ASSERT(board==SearchCurrent[ThreadId]->board);
   ASSERT(board_is_legal(board));
   ASSERT(depth>=1);

   // init
   
   SearchCurrent[ThreadId]->node_nb++;
   SearchInfo[ThreadId]->check_nb--;
   
   if (SearchInput->multipv <= 0) {
       if (depth > 2 && NumberThreadsInternal >= 2) {
           move = SearchBestAtDepth[depth-1].move; // last ply best move.  
           if (MoveNone == move) move = SearchBestAtDepth[last_complete_depth].move; // last complete depth best move.  
           for (i = 0; i < LIST_SIZE(list); i++) {
               if (move != LIST_MOVE(list,i)) list->value[i] = ValueNone;
               else                           list->value[i] = -ValueInf;
           }
           list_sort(list);
           ASSERT(LIST_MOVE(list,0) == SearchBestAtDepth[last_complete_depth].move || LIST_MOVE(list,0) == SearchBestAtDepth[depth-1].move || ThreadsPerDepth <= 1);
       } else {
           for (i = 0; i < LIST_SIZE(list); i++) list->value[i] = ValueNone;
       }
   }
   
   old_alpha = alpha;
   best_value = ValueNone;
   
   in_check = board_is_check(board);                              // WHM
   SearchStack[ThreadId][height].free_queen_check        = false; // WHM
   SearchStack[ThreadId][height].in_check                = false; // WHM
   if (in_check) {                                                // WHM
       SearchStack[ThreadId][height].in_check            = true;  // WHM
       if (is_free_queen_check(board)) {                          // WHM
          SearchStack[ThreadId][height].free_queen_check = true;  // WHM
       }                                                          // WHM
   }                                                              // WHM
   // for NodePV depth extensions, every other capture...         // WHM
   SearchStack[ThreadId][height].cap_extended            = false; // WHM
   SearchStack[ThreadId][height + 1].cap_extended        = false; // WHM

   // move loop
   
   for (i = 0; i < LIST_SIZE(list); i++) {

      move = LIST_MOVE(list,i);
      
      // skip a move search logic
      // let all the threads have a look at 
      // all the other best moves, 
      // all i == 0 cases.
      
      found = false;
      already_claimed = false;
      
      if (SkipMoves && move != SearchBestAtDepth[last_complete_depth].move) {
          
          if (NumberThreadsInternal >= 2  &&  i > SearchInput->multipv  &&  search_type == SearchNormal) {
          
              for (j = 0; j < claimed_nb_array[depth] && !already_claimed; j++) {
                  if (claimed_moves_array[depth][j] == move) {
                      already_claimed = true;
                      // in Mixed mode, both a Tactical thread 
                      // and a Positional thread need to do a move.  
                      if (Mixed) {
                          if (Tactical[ThreadId]) {
                              if (claimed_moves_mixed[depth][j] == 0  ||  
                                  claimed_moves_mixed[depth][j] == 2     ) {
                                  claimed_moves_mixed[depth][j] += 1; // add 1 for tactical.
                              } else if (claimed_moves_mixed[depth][j] == 1) {
                                  found = true; // don't need to do another tactical.  
                              }
                          } else {
                              if (claimed_moves_mixed[depth][j] == 0  ||  
                                  claimed_moves_mixed[depth][j] == 1     ) {
                                  claimed_moves_mixed[depth][j] += 2; // add 2 for positional.
                              } else if (claimed_moves_mixed[depth][j] == 2) {
                                  found = true;; // don't need to do another positional.  
                              }
                          }
                          if (claimed_moves_mixed[depth][j] >= 3) { // both a tactical and a positional were done.  
                              found = true;
                              ASSERT(claimed_moves_mixed[depth][j] == 3);
                          }
                      } else {
                         found = true; // another thread already did this move.  
                      }
                  }
              }
          
              if (found) continue; // next move

              // here this thread is claiming this move if not already claimed...
              if (!already_claimed) { // already_claimed instead of found.  
                  claimed_moves_array[depth][claimed_nb_array[depth]++] =  move;
              }
          }
      }
      
      ASSERT(!found);

      SearchRoot[ThreadId]->depth = depth;
      SearchRoot[ThreadId]->move = move;
      SearchRoot[ThreadId]->move_pos = i;
      SearchRoot[ThreadId]->move_nb = LIST_SIZE(list);

      search_update_root(ThreadId);
      
      new_depth = full_new_depth_PV(depth, height, move, board, in_check&&LIST_SIZE(list)==1, &move_check, HistoryMax, ThreadId); // &move_check
      
      move_do(board,move,undo);
      
      if (search_type == SearchShort || best_value == ValueNone) { // first move
         
         value = -full_search(board,-beta,-alpha,new_depth,height+1,NodePV,new_pv,ThreadId);
         
      } else if (i <= SearchInput->multipv) { // multipv region
         
         value = -full_search(board,-beta,-alpha,new_depth,height+1,NodePV,new_pv,ThreadId);
         
      } else { // other moves

         if (UseRootAlpha && alpha < root_alpha[depth] && !current_best_is_mate) {
                             alpha = root_alpha[depth];
         }
         
         value = -full_search(board,-alpha-1,-alpha,new_depth,height+1,NodeCut,new_pv,ThreadId);
         
         if (value > alpha) {
            
            if (UseRootAlpha && alpha < root_alpha[depth] && !current_best_is_mate) {
                                alpha = root_alpha[depth];

               if (SkipMoves || ThreadsPerDepth == 1) {
                  
                  // re-test as futility pruning only lets us win by so much,
                  // and this may be our only look at this move at this depth.  
                  
                  value = -full_search(board,-alpha-1,-alpha,new_depth,height+1,NodeCut,new_pv,ThreadId);
                  
                  if (value <= alpha) goto skip_new_best;
               }
            }
            
            SearchRoot[ThreadId]->change = true;
            SearchRoot[ThreadId]->easy = false;
            SearchRoot[ThreadId]->flag = false;
            
            search_update_root(ThreadId);
            
            value = -full_search(board,-ValueInf,-alpha,new_depth,height+1,NodePV,new_pv,ThreadId);
            
            // if the NodePV value agrees, unclaim the move so other threads can also get it sorted into their lists.  
            if (value > alpha) {
               if (i > SearchInput->multipv) {
                  found = false;
                  for (j = 0; j < claimed_nb_array[depth] && !found; j++) {
                        if (claimed_moves_array[depth][j] == move) {
                            claimed_moves_array[depth][j]  = MoveNone; // undo the move claim
                            claimed_moves_mixed[depth][j]  = 0;
                            found = true;
                        }
                  }
                  if (depth < DepthMax - 1) { // WHM; less running ahead which causes hitches...
                     found = false;
                     for (j = 0; j < claimed_nb_array[depth+1] && !found; j++) {
                        if (claimed_moves_array[depth+1][j] == move) {
                            claimed_moves_array[depth+1][j]  = MoveNone; // undo the move claim
                            claimed_moves_mixed[depth+1][j]  = 0;
                            found = true;
                        }
                     }
                  }
               }
            }
         }
      }
      
skip_new_best:

      // calculate the number of repetitions for later test...
      j = board_repetitions(board); // extern int  board_repetitions(const board_t * board);

      move_undo(board,move,undo);
      
      // if we are stopping, unclaim this move as it was NOT COMPLETED...
      if (SearchInfo[ThreadId]->stop) {
                  
                  found = false;
                  for (j = 0; j < claimed_nb_array[depth] && !found; j++) {
                           if (claimed_moves_array[depth][j] == move) {
                               claimed_moves_array[depth][j]  = MoveNone; // undo the move claim
                               claimed_moves_mixed[depth][j]  = 0;
                               found = true;
                           }
                  }
          
          return value; // SearchInfo[ThreadId]->stop shared.   // ignore this root move as it was not completed.
      }
      
      if (value_is_ok(value)) {
         if (value > +ValueEvalInf) {
             current_best_is_mate = true;
         }
      }
               
      if (value <= alpha) { // upper bound
         list->value[i] = old_alpha;
      } else if (value >= beta) { // lower bound
         list->value[i] = beta;
         ASSERT(false);
      } else { // alpha < value < beta => exact value
         list->value[i] = value;
      }
      
      if (value > best_value  ||  i <= SearchInput->multipv) {
         if (best_value == ValueNone  ||  value > alpha) {
            
            ASSERT(best_value > ValueNone || alpha == -ValueInf || ThreadsPerDepth >= 2 && UseRootAlpha);

            SearchReport[ThreadId][depth]->move = move;
            SearchReport[ThreadId][depth]->value = value;
            if (value <= alpha) { // upper bound
               SearchReport[ThreadId][depth]->flags = SearchUpper;
            } else if (value >= beta) { // lower bound
               SearchReport[ThreadId][depth]->flags = SearchLower;
            } else { // alpha < value < beta => exact value
               SearchReport[ThreadId][depth]->flags = SearchExact;
            }
            
            // WHM; best move and ponder move must both be the same else change...
            //      change turns off early exit.  When ponder changes, 
            //      then root move changes at the next depth a significant percentage of the time...
            if (depth > 1) {
               if (move      != SearchReport[ThreadId][depth-1]->pv[0]) SearchRoot[ThreadId]->change = true;
               if (new_pv[0] != SearchReport[ThreadId][depth-1]->pv[1]) SearchRoot[ThreadId]->change = true;
            }
            pv_cat(SearchReport[ThreadId][depth]->pv,new_pv,move);
            ASSERT(move      == SearchReport[ThreadId][depth]->pv[0]);
            ASSERT(new_pv[0] == SearchReport[ThreadId][depth]->pv[1]);

            // if we are repeating, turn off early exit...
            if (j >= 1) SearchRoot[ThreadId]->change = true;
            // on a game ender, set bad to go as deep as possible...
            if (j >= 2) {
                SearchRoot[ThreadId]->bad = true; // fixed.  
            }

            // WHM; here we cut the probability of 2 threads doing 
            //      search_report_and_update_best() at the same time 
            //      to almost nill...

            ntries = 0;
            i1023 = 13*ThreadId;
            while (i1023 > 0) {
               i1023--;
            }
            if (SearchBestAtDepthIsBusy[depth][ThreadId]) {
                i1023 = 1023 + 13*ThreadId;
            } else {
                i1023 = 0;
            }
            
if_at_first_you_do_not_succeed: // try, try and try again.  
            ntries++;
            while (i1023 > 0) {
               i1023--;
            }
            if (SearchBestAtDepthIsBusy[depth][ThreadId]) {
                if (ntries < 1024) {
                    i1023 = 1023 + 13*ThreadId;
                    goto if_at_first_you_do_not_succeed;
                } else {
                    SearchBestAtDepthIsBusy[depth][ThreadId] = false;
                }
            } else {
                for (k = 0; k < NumberThreadsInternal; k++) SearchBestAtDepthIsBusy[depth][k] = true;
                search_report_and_update_best(depth, ThreadId);
                for (k = 0; k < NumberThreadsInternal; k++) SearchBestAtDepthIsBusy[depth][k] = false;
            }
         }
      }

      if (value > best_value) {
         best_value = value;
         if (value > alpha) {
            if (search_type == SearchNormal && i >= SearchInput->multipv) { // multipv too
               alpha = value;
               if (UseRootAlpha && root_alpha[depth] < alpha && !current_best_is_mate) {
                                   root_alpha[depth] = alpha;
               }
            }
            if (value >= beta) {
               ASSERT(false);
               break;
            }
         }
      }
   }

   ASSERT(value_is_ok(best_value));

   list_sort(list);

   ASSERT(SearchReport[ThreadId][depth]->move==LIST_MOVE(list,0) || SearchInput->multipv >= 1 || UseRootAlpha);
   ASSERT(SearchReport[ThreadId][depth]->value==best_value || SearchInput->multipv >= 1 || UseRootAlpha);
   
   return best_value;
}

// full_search()

static int full_search(board_t * board, int alpha, int beta, int depth, int height, int node_type, mv_t pv[], int ThreadId) {

   bool in_check;
   bool single_reply;
   bool reduced;
   bool prunable;
   bool cut_move_found; // WHM; WSD
   bool move_check; // = move_is_check()
   bool use_egbb; // WHM

   bool played_and_good[256]; // WSD
   
   int trans_move, trans_depth, trans_flags, trans_value;
   int old_alpha;
   int value, best_value;
   int move, best_move;
   int new_depth;
   int played_nb;
   int i;
   int opt_value, temp_value;
   int probe_score;
   int best_move_index; // WHM; WSD
   int num_FQC_target; // WHM
   int num_FQC;        // WHM

   attack_t attack[1];
   sort_t sort[1];
   undo_t undo[1];
   mv_t new_pv[HeightMax];
   mv_t played[256];
   
   ASSERT(board!=NULL);
   ASSERT(range_is_ok(alpha,beta));
   ASSERT(depth_is_ok(depth));
   ASSERT(height_is_ok(height));
   ASSERT(pv!=NULL);
   ASSERT(node_type==NodePV||node_type==NodeCut||node_type==NodeShort);
   ASSERT(ThreadId>=0 && ThreadId<NumberThreadsInternal);

   ASSERT(board_is_legal(board));

   // init pv[], WHM moved, full_q does not update pv[].
   PV_CLEAR(pv);
   
   if (node_type == NodePV) CheckDepth[ThreadId] = 1 - CheckNb - 1; // Thomas in Toga II 1.2.1
   else                     CheckDepth[ThreadId] = 1 - CheckNb;

   // horizon?  Fabien

   if (depth <= 0) return full_quiescence(board,alpha,beta,0,height,ThreadId);

   // init

   SearchCurrent[ThreadId]->node_nb++;
   SearchInfo[ThreadId]->check_nb--;

   if (height > SearchCurrent[ThreadId]->max_depth) SearchCurrent[ThreadId]->max_depth = height;

   if (SearchInfo[ThreadId]->check_nb <= 0) {
      SearchInfo[ThreadId]->check_nb += SearchInfo[ThreadId]->check_inc;
      search_check(ThreadId);
   }
   
   // draw?

   if (board_is_repetition(board)) return ValueDraw;

/*  Interior node recognizer from scorpio by Daniel Shawul
    -> dont probe at the leaves as this will slow down search
    For 3 or 4 pieces probe there also.
    -> After captures and pawn moves assume exact score and cutoff tree,
    because we are making progress. Note this is not done only for speed.
    -> if we are far from root (egbb_height), assume exact score and cutoff tree. */

    if (board->piece_nb <= 5) {

       if (    egbb_is_loaded 
           && (height <= SearchCurrent[ThreadId]->egbb_height  ||  board->piece_nb <= 4)
           && (height >= SearchCurrent[ThreadId]->egbb_height  ||  PIECE_IS_PAWN(board->moving_piece)  ||  board->cap_sq != SquareNone)
           &&  probe_bitbases(board, probe_score, ThreadId) // WHM_egbb
          ) {
          
          ASSERT(ValueDraw == probe_score || !recog_draw(board,ThreadId));
          
          use_egbb = true;

          if (ValueDraw != probe_score) { // some mates fail, best to let our own mate search find them.  
                                         //  KRPKB mate {pawn on edge == AorH} fails with egbb mate values. 
                                        //   KQKR mate fails.  KQKR mate fails with egbb version 3.2 most files dated 2/22/2009, ouch. 
              if (board->piece_size[White] == 2  &&
                  board->piece_size[Black] == 2     ) {
                  
                  if (board->piece_nb == 4) {
                      
                      if (board->number[WhiteQueen12] == 1 && board->number[BlackRook12] == 1) use_egbb = false; // KQKR
                      if (board->number[BlackQueen12] == 1 && board->number[WhiteRook12] == 1) use_egbb = false; // KRKQ
                      
                      ASSERT(0 == board->pawn_size[White]);
                      ASSERT(0 == board->pawn_size[Black]);
                  }
              }
          }
          
          if (use_egbb) {
             
             if (TryEGBBToTrans) {
                
                trans_move  = MoveNone;
                trans_depth = 63;
                trans_flags = TransUnknown;
                trans_value = probe_score;
             
                if (trans_value > alpha  ||  ValueDraw == trans_value) trans_flags |= TransLower;
                if (trans_value <  beta  ||  ValueDraw == trans_value) trans_flags |= TransUpper;
#if DEBUG
                trans_store(Trans,board->key,trans_move,trans_depth,trans_flags,trans_value,board);
#else
                trans_store(Trans,board->key,trans_move,trans_depth,trans_flags,trans_value);
#endif
                ASSERT(trans_value == value_to_trans(probe_score,height));
             }
             
             probe_score = value_from_trans(probe_score,height);
             
             return probe_score;
          }

       } else { // no egbb value available.  

           if (recog_draw(board,ThreadId)) return ValueDraw;
       }
    }

   // mate-distance pruning

   if (UseDistancePruning) {

      // lower bound

      value = VALUE_MATE(height+2); // does not work if the current position is mate, WHM fixed.  
      if (value > alpha && board_is_mate(board)) return VALUE_MATE(height);

      if (value > alpha) {
         alpha = value;
         if (value >= beta) return value;
      }

      // upper bound

      value = -VALUE_MATE(height+1);

      if (value < beta) {
         beta = value;
         if (value <= alpha) return value;
      }
   }
   
   // transposition table
   
   trans_move = MoveNone;

   if (UseTrans && depth >= TransDepth) {

      if (trans_retrieve(Trans,board->key,&trans_move,&trans_depth,&trans_flags,&trans_value)) {

         // trans_move is now updated
          
         if (node_type != NodePV) { // NodePV boards less than 1/1000 total boards early.  

            if (UseMateValues) {

               if (trans_depth < depth) {
                  if (trans_value < -ValueEvalInf && TRANS_IS_UPPER(trans_flags)) {
                      trans_depth = depth;
                      trans_flags = TransUpper;
                  } else if (trans_value > +ValueEvalInf && TRANS_IS_LOWER(trans_flags)) {
                      trans_depth = depth;
                      trans_flags = TransLower;
                  }
               }
            }

//          if (trans_depth >= depth) {
            if (   (NumberThreadsInternal == 1 || !Mixed  ||  Tactical[ThreadId]) && trans_depth >= depth
                || (NumberThreadsInternal >= 2 &&  Mixed  && !Tactical[ThreadId]) && trans_depth >  depth
               ) {

               trans_value = value_from_trans(trans_value,height);

               if (  (UseExact && TRANS_IS_EXACT(trans_flags))
                   || (TRANS_IS_LOWER(trans_flags) && trans_value >= beta)
                   || (TRANS_IS_UPPER(trans_flags) && trans_value <= alpha)) {

                  return trans_value;
               } 
            }
         }
      }
   }
   
   // height limit
   
   if (height >= HeightMax-1) return eval(board, alpha, beta, false, ThreadId);

   // more init

   old_alpha = alpha;
   best_value = ValueNone;
   best_move = MoveNone;
   played_nb = 0;
   best_move_index = 0; //WHM; WSD
   if (UseFreeQueenCheck) {
      num_FQC_target = 10;
      num_FQC = 0; // WHM
   }
   
   attack_set(attack,board);
   in_check = ATTACK_IN_CHECK(attack);
   prunable = false;                                        // WHM
   if (node_type != NodePV  &&  !in_check) prunable = true; // WHM
   opt_value = +ValueInf;                                   // WHM moved.
   cut_move_found = false;                                  // WHM; WSD, moved.  

   // null-move pruning

   if (depth >= NullDepth  &&  prunable) {

     if (trans_move == MoveNone || TRANS_IS_UPPER(trans_flags) || trans_value >= beta - 35) {

         if (do_null(board)  &&  !value_is_mate(beta)) {

            // null-move search
         
            new_depth = depth - (NullReduction + 1);
            
            move_do_null(board,undo);
            value = -full_search(board,-beta,-beta+1,new_depth,height+1,NodeShort,new_pv,ThreadId);
            move_undo_null(board,undo);

            // verification search

            if (depth > VerReduction) {

               if (value >= beta) {

                  new_depth = depth - VerReduction;
                  ASSERT(new_depth>0);

                  // attack added, pv[] removed...
                  value = full_no_null(board,alpha,beta,new_depth,height,NodeShort,&move,trans_move,attack,ThreadId); // WHM

                  if (value >= beta) {

                     ASSERT(move != MoveNone);

                     if (!move_is_tactical(move,board)) {
                        good_move(move,board,depth,height,ThreadId); // WHM
                        history_good(move,board,ThreadId);           // WHM
                     }

                     // trans tables store best_move, best_value and depth.  
                     best_move = move;
                     best_value = value;

                     goto trans_tables; // WHM
                  }
               }
            }

            // pruning

            if (value >= beta) { ASSERT(depth <= VerReduction);
                                 ASSERT(new_depth == depth - (NullReduction + 1));

               if (value > 127+ValueEvalInf)   {
                   value = 127+ValueEvalInf; // do not return unproven mates
               }
               ASSERT(value > -ValueEvalInf && value <= 127+ValueEvalInf);

               // trans stores best_move, best_value and depth.  
               best_move = MoveNone;
               best_value = value;
               goto trans_tables;
            }
         }
      }
   }

   // depths 1-3 razoring, UCI input, 0 turns razoring off.
   if (depth <= FutilityDepth  &&
       prunable                &&
       do_futility(board)      &&
      !value_is_mate(alpha)       ) {
      
      ASSERT(depth <= 5);
      temp_value = alpha - FutilityMargins[depth]; // threshold
      
      if (trans_move != MoveNone && (TRANS_IS_UPPER(trans_flags)  ||  trans_value > temp_value)) {
         opt_value = trans_value + FutilityMargins[depth];
      } else if (depth <= RazorDepth) {
         opt_value = eval(board,temp_value,temp_value,false, ThreadId) + FutilityMargins[depth];
      }
      
      if (depth <= RazorDepth  &&  opt_value <= alpha - 200) { // eval() <= alpha-300 at depth 1, eval() <= alpha-500 at depth 2...  

         value = full_quiescence(board,temp_value,1+temp_value,0,height,ThreadId);
         
         if (value <= temp_value) return value;

         ASSERT(value > alpha - FutilityMargins[depth]);
         
         if (opt_value < value) {
             opt_value = value;
         }

         ASSERT(opt_value > temp_value);
      }
   }
   
   // Internal Iterative Deepening.  
   
   if (UseIID && trans_move == MoveNone && node_type == NodePV && depth >= IIDDepth) {

      new_depth = depth - IIDReduction;
      // new_depth =     depth / 2;
      // new_depth = MAX(depth / 2, depth - 4);
      ASSERT(new_depth >= 1 && new_depth <= depth - 2);
      
      value                     = full_no_null(board,    alpha, beta,new_depth,height,NodePV,&move,trans_move,attack,ThreadId);
      if (value <= alpha) value = full_no_null(board,-ValueInf, beta,new_depth,height,NodePV,&move,trans_move,attack,ThreadId);
      
      trans_move = move; // WHM NodePV first move is always trans_move, very very very important!  nhits in entry_t re-allows deeper search.  
      ASSERT(move_is_ok(trans_move) || -ValueInf + height == value  ||  SearchInfo[ThreadId]->stop); // SearchInfo[ThreadId]->stop shared.  
   }
   
   // start WHM free_queen_check
   if (UseFreeQueenCheck) {

      SearchStack[ThreadId][height].free_queen_check = false;
      SearchStack[ThreadId][height].in_check         = false;

      if (in_check) {

         SearchStack[ThreadId][height].in_check = true;

         if (    node_type == NodePV                                         // NodePV 
             ||  height <= 4                                                 // OR root boards to get it started...
             ||  (                     height >= 2                   &&      // OR FQC tag
                 SearchStack[ThreadId][height -  2].free_queen_check    )    //
             ||  (                     height >= 8                   &&      // OR 5 checks in a row
                 SearchStack[ThreadId][height -  2].in_check         &&      //
                 SearchStack[ThreadId][height -  4].in_check         &&      //
                 SearchStack[ThreadId][height -  6].in_check         &&      //
                 SearchStack[ThreadId][height -  8].in_check            )) { //

            if (is_free_queen_check(board)) {

               SearchStack[ThreadId][height].free_queen_check = true;

               // if it might be a free queen check draw, then test for it.  
               // This gives us more depth in high queen-work positions.  
               if (                      height >= 8                    &&
                   SearchStack[ThreadId][height -  2].free_queen_check  &&
                   SearchStack[ThreadId][height -  4].free_queen_check  &&
                   SearchStack[ThreadId][height -  6].free_queen_check  &&
                   SearchStack[ThreadId][height -  8].free_queen_check     ) { // 5 in a row minimum

                   num_FQC_target = (11 + SearchCurrent[ThreadId]->act_iteration_depth) / 2;
                   num_FQC_target = MAX(num_FQC_target, 10); // min of 10
                   num_FQC_target = MIN(num_FQC_target, 13); // max of 13

                   i = height - 8;
                   num_FQC = 4;

                   do  {
                        i -= 2;
                        num_FQC++;

                   } while (i >= 0  &&  SearchStack[ThreadId][i].free_queen_check);

                   if (num_FQC >= num_FQC_target) {

                       value = full_quiescence(board, -ValueInf, +ValueInf, -3, height, ThreadId);

                       if (value > 0) {
                           return 0; // FQC draw pruning, losing guy escapes by infinite queen checks.  
                       }
                   }
               }
            }
         }
      }
   }
   // free_queen_check WHM end.


   // move generation and final move loop init
   
   sort_init(sort,board,attack,depth,height,trans_move,(NodePV==node_type),ThreadId); 
   
   single_reply = false;
   if (in_check && LIST_SIZE(sort->list) == 1) single_reply = true; // HACK
   
   // move loop

   while ((move=sort_next(sort,ThreadId)) != MoveNone) {

      // extensions
      
      if (NodePV == node_type)                    new_depth = full_new_depth_PV(depth,height, move,board,single_reply, &move_check,sort->value, ThreadId);
      else if (single_reply && ExtendSingleReply) new_depth = depth;
      else                                        new_depth = full_new_depth   (depth,        move,board,              &move_check,sort->value, ThreadId);
      
      // all move pruning INIT
      reduced = false; // moved this
      played_and_good[played_nb] = false;
      ASSERT(sort->value >= 1 && sort->value <= 16384);
      
      // all move pruning CONSTRAINTS
      if ( new_depth < depth            && 
           played_nb >= 1               &&     // don't prune all the moves...
           sort->value < HistoryKiller  &&     // HistoryMax==16384, HistoryKiller==16383, HistoryBadCap==16382, not in Toga1.4beta5cWHM(31).  
           prunable                     &&     // node_type != NodePV && !in_check
          !move_check                   &&     // !move_is_check(move,board)
          !move_is_dangerous(move,board)   ) { // passers + all Rank6 pawn pushes... 
         
         ASSERT(best_value!=ValueNone);
         ASSERT(value_is_ok(best_value));
         ASSERT(played_nb>0);
         ASSERT(sort->pos>0&&move==LIST_MOVE(sort->list,sort->pos-1));
         ASSERT(!MOVE_IS_PROMOTE(move));
         ASSERT(sort->value >= 1 && sort->value < 16384);
         ASSERT(move!=trans_move);
         ASSERT(move!=sort->killer_1 || move_is_tactical(move,board));
         ASSERT(move!=sort->killer_2 || move_is_tactical(move,board));
         ASSERT(move!=sort->killer_3 || move_is_tactical(move,board));
         ASSERT(move!=sort->killer_4 || move_is_tactical(move,board));
         ASSERT(!move_is_dangerous(move,board));
         ASSERT(!move_is_tactical(move,board) || HistoryBadCap == sort->value);
         
         if (depth <= TacticalDepth) { // allows a super fruit 2.1 mode, not in Toga1.4beta5cWHM(31).  
            
            // history drop pruning, baseline: bad-captures not dropped.  
            if (depth <= HistoryDropDepth               && 
                played_nb >= 1+depth                    &&
                sort->value < HistoryDropValues[depth]     ) { 
                
                ASSERT(!move_is_check(move,board));
                ASSERT(!move_is_tactical(move,board));
                ASSERT(!move_is_dangerous(move,board));
                
                continue;
            }
            
            // For futility pruning, {dropping moves} + JD's Hangers
            // search can't be converged to mate values.  (alpha and beta constraints)
            // Found this with KRPKB mate with pawn on edge.  
            // When we drop moves, mate search occaisionally pops out an overly optimistic answer.
            // That would snatch a LOSS from WIN.  Not acceptable for WCCC entrants.  
            // not in Toga1.4beta5cWHM(31).  
            if (alpha < +ValueEvalInf  &&     // else board->turn is mating, therefore mate values search.  
                beta > -ValueEvalInf      ) { // else COLOUR_OPP(board->turn) is mating, therefore mate values search.             
               
               // futility pruning
               if (depth <= FutilityDepth) {
                  
                  ASSERT(depth <= 5);
                  
                  // optimistic evaluation
                  if (opt_value == +ValueInf) {
                     
                     if (do_futility(board)  &&  !value_is_mate(alpha)) { // WHM; added, not in Toga1.4beta5cWHM(31).  
                        
                        temp_value = alpha - FutilityMargins[depth];
                        temp_value = eval(board,temp_value,temp_value,false, ThreadId);
                        
                        opt_value = temp_value + FutilityMargins[depth];
                        
                        ASSERT(opt_value < +ValueEvalInf);
                        ASSERT(opt_value > -ValueEvalInf);
                        
                     } else { // off
                        
                        opt_value = beta + 1;
                        
                        ASSERT(!(opt_value <= alpha));
                        ASSERT(opt_value != +ValueInf);
                     }
                  }
                  
                  if (depth <= 2                       ||  
                      sort->value < 15360 - 1024*depth ||
                      opt_value <= alpha - 50 * depth     ) { // stabilization of futility pruning and stabilization of JD's hangers reduction
                     
                     if (opt_value <= alpha && HistoryBadCap != sort->value) {
                        
                        // pruning
                        ASSERT(!move_is_check(move,board));
                        ASSERT(!move_is_tactical(move,board));
                        ASSERT(!move_is_dangerous(move,board));
                        
                        continue;
                     }
                  } 
               }
               
               // "Reduce JD's Hangers" not in Toga1.4beta5cWHM(31).  
               if (depth <= JDsHangersDepth) { // "JD's Hangers Depth", was TacticalDepth

                  if (sort->value < 15360 - 1024 * depth) {
                  
                     if (see_move(move,board) <= -ValuePawn) {
                     
                        new_depth--;
                        reduced = true;
                        ASSERT(!move_is_check(move,board));
                        ASSERT(!move_is_tactical(move,board));
                        ASSERT(!move_is_dangerous(move,board));
                     }
                  }
               }
            }
         }
         
         if (UseHistory) {
            
            if (played_nb >= HistoryMoveNb  && 
                depth >= HistoryDepth          ) {
               
               // Tactical is like beta5c & 1.4.1SE, 
               // else old style History Pruning - fruit 2.1 and Toga II 1.2.1
               if ((Tactical[ThreadId] && depth <= TacticalDepth)  ||     // Tactical "Playing Style" is like beta5c & 1.4.1SE, else old style History Pruning.  
                           sort->value <  HistoryValue[ThreadId]      ) { // Positional "Playing Style" is old style History Pruning
                   
                   new_depth--;
                   reduced = true;
                   ASSERT(!move_is_check(move,board));
                   ASSERT(!move_is_tactical(move,board) || HistoryBadCap == sort->value);
                   ASSERT(!move_is_dangerous(move,board));
               }
            }
         }
      }
      // end of all move pruning CONSTRAINTS
      
      // NodePV history pruning introduced by Thomas in Toga II 1.4beta5c
      if (UseHistory && NodePV == node_type) {
         
         if (Tactical[ThreadId] && depth <= TacticalDepth) {
            
            if ( played_nb >= HistoryMoveNbNodePV  &&  
                 depth >= HistoryDepth             &&
                 sort->value < HistoryKiller       &&
                !move_is_dangerous(move,board)        ) {
               
               ASSERT(played_nb>0);
               ASSERT(best_value!=ValueNone);
               ASSERT(sort->pos>0&&move==LIST_MOVE(sort->list,sort->pos-1));
               
               if (new_depth < depth  &&  !in_check) {
                  
                  ASSERT(best_value!=ValueNone);
                  ASSERT(value_is_ok(best_value));
                  ASSERT(played_nb>0);
                  ASSERT(sort->pos>0&&move==LIST_MOVE(sort->list,sort->pos-1));
                  ASSERT(!MOVE_IS_PROMOTE(move));
                  ASSERT(sort->value >= 1 && sort->value < 16384);
                  ASSERT(move!=trans_move);
                  ASSERT(move!=sort->killer_1 || move_is_tactical(move,board));
                  ASSERT(move!=sort->killer_2 || move_is_tactical(move,board));
                  ASSERT(move!=sort->killer_3 || move_is_tactical(move,board));
                  ASSERT(move!=sort->killer_4 || move_is_tactical(move,board));
                  ASSERT(depth >= HistoryDepth);
                  ASSERT(!passed_pawn_move(move,board));
                  ASSERT(!move_is_dangerous(move,board));
                  ASSERT(!move_is_check(move,board));
                  ASSERT(!move_is_tactical(move,board) || sort->value == HistoryBadCap);
                  ASSERT(!move_is_tactical(move,board) || !capture_is_good(move,board,NodePV==node_type));
                  
                  new_depth--;
                  reduced = true;
                  
                  ASSERT(new_depth > 0);
               }
            }
         }
      }
      // testing for all bad caps being reduced...
      ASSERT(new_depth >= depth-2 || NodePV != node_type || !move_is_tactical(move,board));
      ASSERT(new_depth >= depth-1 || !in_check);
      ASSERT(NodePV != node_type  || sort->value >= HistoryKiller || played_nb < HistoryMoveNbNodePV || depth < HistoryDepth || move_is_dangerous(move,board) || new_depth == depth || !Tactical[ThreadId] || reduced);
      if (move_is_tactical(move,board) && HistoryBadCap == sort->value && NodePV != node_type) {
         ASSERT(reduced || depth < HistoryDepth || move_is_check(move,board) || move_is_dangerous(move,board) || played_nb < HistoryMoveNb || !prunable || !Tactical[ThreadId]);
      }
      
      // recursive search

      move_do(board,move,undo);

      if (node_type != NodePV) {                                                                     // non-NodePV
         value    = -full_search(board,   -beta,-alpha,new_depth,height+1,node_type,new_pv,ThreadId);
      } else if (best_value == ValueNone) {                                                          // first move in pv <==> NodePV
         value    = -full_search(board,   -beta,-alpha,new_depth,height+1, NodePV,  new_pv,ThreadId);
      } else {                                                                                       // other moves in pv <==> NodePV
         value    = -full_search(board,-alpha-1,-alpha,new_depth,height+1, NodeCut, new_pv,ThreadId);// test
         if (value > alpha) {
            value = -full_search(board,   -beta,-alpha,new_depth,height+1, NodePV,  new_pv,ThreadId);// re-test
         }
      }

      // history-pruning redo-search

      if (HistoryReDoSearch && reduced && value >= beta) {
         
         ASSERT(node_type!=NodePV || played_nb >= HistoryMoveNbNodePV && sort->value < 16384);
         ASSERT(!in_check);
         
         new_depth++;
         ASSERT(depth-1==new_depth || depth-2 == new_depth && NodePV != node_type && depth <= JDsHangersDepth);
         
         value = -full_search(board,-beta,-alpha,new_depth,height+1,node_type,new_pv,ThreadId);
      }
      
      if (value >=  beta) played_and_good[played_nb] = true; // WSD

      
      move_undo(board,move,undo);
      
      played[played_nb++] = move;
      
      
      if (value > best_value) {
         best_value = value;
         if (NodePV == node_type) pv_cat(pv,new_pv,move); // WHM only NodePV
         if (value > alpha) {
            alpha = value;
            best_move = move;
            best_move_index = played_nb - 1; // WHM; WSD Junior Killer?  
            if (value >= beta){ 
                if (UseWSD[ThreadId] && NodePV == node_type) {
                   ASSERT(played_and_good[best_move_index]);
                   if (best_value >= WSDThreshold    ||     // Win OR
                       beta < 0  &&  best_value >= 0    ) { // Save Draw
                      goto cut;
                   } else {
                      cut_move_found = true; // WHM; WSD
                      alpha = beta - 2;     // WHM; up alpha and get more moves
                   }
                } else {
                   goto cut;
                }
            }
         }
      }
      
      if (SearchInfo[ThreadId]->stop) return value; // search is ending
   }
   
   
   // ALL node

   if (best_value == ValueNone) { // no legal move
      if (in_check) {
         ASSERT(board_is_mate(board));
         return VALUE_MATE(height);
      } else {
#if DEBUG
         if (NumberThreadsInternal == 1  &&  !board_is_stalemate(board)) {
            print_board(board);
         }
#endif
         ASSERT(board_is_stalemate(board));
         return ValueDraw;
      }
   }
   
   
cut:
   
   ASSERT(value_is_ok(best_value));
   
   // move ordering
   
   if (best_move != MoveNone) {
      
      good_move(best_move,board,depth,height,ThreadId);
      
      if (best_value >= beta  &&  !move_is_tactical(best_move,board)) {
         
         ASSERT(best_move_index < played_nb);
         ASSERT(played_and_good[best_move_index]);
         
//WSD    ASSERT(played_nb > 0 && played[played_nb-1]     == best_move);
         ASSERT(played_nb > 0 && played[best_move_index] == best_move);
         
//WSD    for (i = 0; i < played_nb-1;     i++) {
         for (i = 0; i < best_move_index; i++) {
            
            move = played[i];
            
            ASSERT(move != best_move);
            
            if (move_is_tactical(move,board)) continue; // moved this out of sort.cpp
            
            if (played_and_good[i]) history_good(     move, board, ThreadId); // WHM, not in Toga1.4beta5cWHM(31).  
            else                    history_bad (     move, board, ThreadId);
         }
                                    history_good(best_move, board, ThreadId);
      }
   }
   
   // transposition table
   
trans_tables:
   
   if (UseTrans && depth >= TransDepth) {

      trans_move = best_move;
      trans_depth = depth;
      trans_flags = TransUnknown;
      if (best_value > old_alpha) trans_flags |= TransLower;
      if (best_value <      beta) trans_flags |= TransUpper;
      trans_value = value_to_trans(best_value,height);

      // TODO: return the height at full_quiescence() in full_search(..., int * full_q_height)
      //       then use (new_full_q_height - height) as trans_depth instead, NodePV only...
      
#if DEBUG
      trans_store(Trans,board->key,trans_move,trans_depth,trans_flags,trans_value,board); // WHM board needed for ASSERT's
#else
      trans_store(Trans,board->key,trans_move,trans_depth,trans_flags,trans_value);
#endif
   }

   // free queen checks by WHM.  
   if (UseFreeQueenCheck) {
      ASSERT(num_FQC_target >= 10);
      ASSERT(num_FQC_target <= 13);
      if (num_FQC >= num_FQC_target  &&  best_value > 0  &&  best_value < +ValueEvalInf) {
         ASSERT(num_FQC >= 10);
         ASSERT(num_FQC <= 13);
         ASSERT(in_check); // 11/22/08
         ASSERT(is_free_queen_check(board)); // 11/22/08
         best_value = 0;
      }
   }

   // done

   return best_value;
}

// full_no_null()

static int full_no_null(board_t * board, int alpha, int beta, int depth, int height, int node_type, int * best_move, int trans_move, attack_t * attack, int ThreadId) {

   bool in_check;
   bool single_reply;
   bool move_check;

   int value, best_value;
   int move;
   int new_depth;
   
// attack_t attack[1]; // WHM no need to redo attack_set()
   sort_t sort[1];
   undo_t undo[1];
   mv_t new_pv[HeightMax];

   ASSERT(board!=NULL);
   ASSERT(range_is_ok(alpha,beta));
   ASSERT(depth_is_ok(depth));
   ASSERT(height_is_ok(height));
   ASSERT(best_move!=NULL);
   ASSERT(node_type==NodePV || node_type==NodeShort);
   ASSERT(trans_move==MoveNone||move_is_ok(trans_move));
   ASSERT(ThreadId >= 0  &&  ThreadId < NumberThreadsInternal);

   ASSERT(board_is_legal(board));
   ASSERT(NodePV == node_type || !board_is_check(board));
   ASSERT(depth>=1);

   // init

   SearchCurrent[ThreadId]->node_nb++;
   SearchInfo[ThreadId]->check_nb--;
// PV_CLEAR(pv);

   if (height > SearchCurrent[ThreadId]->max_depth) SearchCurrent[ThreadId]->max_depth = height;

   if (SearchInfo[ThreadId]->check_nb <= 0) {
      SearchInfo[ThreadId]->check_nb += SearchInfo[ThreadId]->check_inc;
      search_check(ThreadId);
   }

// attack_set(attack,board); save repeating this processing...
   in_check = ATTACK_IN_CHECK(attack);
   if (UseFreeQueenCheck) {
      SearchStack[ThreadId][height].in_check         = false;
      SearchStack[ThreadId][height].free_queen_check = false;
      if (in_check) { // gotta be NodePV IID stuff
          ASSERT(NodePV == node_type);
          SearchStack[ThreadId][height].in_check     =  true;
          if (is_free_queen_check(board)) SearchStack[ThreadId][height].free_queen_check = true;
      }
   }

   *best_move = MoveNone;
   best_value = ValueNone;
   
   // move loop

   sort_init(sort,board,attack,depth,height,trans_move,(NodePV==node_type),ThreadId);

   single_reply = false;
   if (in_check && LIST_SIZE(sort->list) == 1) single_reply = true; // HACK

   while ((move=sort_next(sort,ThreadId)) != MoveNone) {
      
      if (NodePV == node_type)                    new_depth = full_new_depth_PV(depth,height, move,board,single_reply, &move_check,sort->value, ThreadId);
      else if (single_reply && ExtendSingleReply) new_depth = depth;
      else                                        new_depth = full_new_depth   (depth,        move,board,              &move_check,sort->value, ThreadId);

//    new_depth = full_new_depth(depth,move,board,single_reply,true,&move_check,sort->value,ThreadId);
      
      move_do(board,move,undo);
      value = -full_search(board,-beta,-alpha,new_depth,height+1,node_type,new_pv,ThreadId);
      move_undo(board,move,undo);
      
      if (value > best_value) {
         best_value = value;
         if (value > alpha) {
            alpha = value;
            *best_move = move;
            if (value >= beta) goto cut;
         }
      }
   }
   
   // ALL node
   
   if (best_value == ValueNone) { // no legal move => stalemate
      if (in_check) {
         ASSERT(board_is_mate(board));
         best_value = VALUE_MATE(height);
      } else {
         ASSERT(board_is_stalemate(board));
         best_value = ValueDraw;
      }
   }

cut:

   ASSERT(value_is_ok(best_value));

   return best_value;
}

// full_quiescence()

static int full_quiescence(board_t * board, int alpha, int beta, int depth, int height, int ThreadId) {

   bool in_check;
   
   int old_alpha;
   int value, best_value;
   int move;
   int opt_value;

   attack_t attack[1];
   sort_t sort[1];
   undo_t undo[1];

   ASSERT(board!=NULL);
   ASSERT(range_is_ok(alpha,beta));
   ASSERT(depth_is_ok(depth));
   ASSERT(height_is_ok(height));

   ASSERT(board_is_legal(board));
   ASSERT(depth<=0);

   // init

   SearchCurrent[ThreadId]->node_nb++;
   SearchInfo[ThreadId]->check_nb--;

   if (height > SearchCurrent[ThreadId]->max_depth) SearchCurrent[ThreadId]->max_depth = height;

   if (SearchInfo[ThreadId]->check_nb <= 0) {
      SearchInfo[ThreadId]->check_nb += SearchInfo[ThreadId]->check_inc;
      search_check(ThreadId);
   }

   // draw?

   if (board_is_repetition(board) || recog_draw(board,ThreadId)) return ValueDraw;

   // mate-distance pruning

   if (UseDistancePruning) {

      // lower bound

      value = VALUE_MATE(height+2); // now works if the current position is mate
      if (value > alpha && board_is_mate(board)) return VALUE_MATE(height);

      if (value > alpha) {
         alpha = value;
         if (value >= beta) return value;
      }

      // upper bound

      value = -VALUE_MATE(height+1);

      if (value < beta) {
         beta = value;
         if (value <= alpha) return value;
      }
   }

   // more init

   attack_set(attack,board);
   in_check = ATTACK_IN_CHECK(attack);

   if (in_check) {
      ASSERT(depth < 0); // important
      depth++; // in-check extension
   }

   // height limit

   if (height >= HeightMax-1) return eval(board, alpha, beta, false, ThreadId);

   // more init

   old_alpha = alpha;
   best_value = ValueNone;

   /* if (UseDelta) */ opt_value = +ValueInf;

   if (!in_check) {

      // lone-king stalemate?

      if (simple_stalemate(board)) return ValueDraw;

      // stand pat

      value = eval(board, alpha, beta, true, ThreadId);

      ASSERT(value>best_value);
      best_value = value;
      if (value > alpha) {
         alpha = value;
         if (value >= beta) goto cut;
      }

      if (UseDelta) {
         opt_value = value + DeltaMargin;
         ASSERT(opt_value<+ValueInf);
      }
   }

   // move loop

   sort_init_qs(sort,board,attack, depth>=CheckDepth[ThreadId]);

   while ((move=sort_next_qs(sort)) != MoveNone) {

      // delta pruning

      if (UseDelta && beta == old_alpha+1) {

         if (!in_check && !move_is_check(move,board) && !capture_is_dangerous(move,board)) {

            ASSERT(move_is_tactical(move,board));

            // optimistic evaluation

            value = opt_value;

            int to = MOVE_TO(move);
            int capture = board->square[to];

            if (capture != Empty) {
               value += VALUE_PIECE(capture);
            } else if (MOVE_IS_EN_PASSANT(move)) {
               value += ValuePawn;
            }

//          if (MOVE_IS_PROMOTE(move)) value += ValueQueen - ValuePawn;
            if (MOVE_IS_PROMOTE(move)) {
                value += VALUE_PIECE(move_promote(move)) - ValuePawn;
            }

            // pruning

            if (value <= alpha) {

               if (value > best_value) {
                  best_value = value;
               }

               continue;
            }
         }
      }

      move_do(board,move,undo);
      value = -full_quiescence(board,-beta,-alpha,depth-1,height+1,ThreadId);
      move_undo(board,move,undo);

      if (value > best_value) {
         best_value = value;
         if (value > alpha) {
            alpha = value;
            if (value >= beta) goto cut;
         }
      }
   }

   // ALL node

   if (best_value == ValueNone) { // no legal move
      ASSERT(board_is_mate(board));
      return VALUE_MATE(height);
   }

cut:

   ASSERT(value_is_ok(best_value));

   // done

   return best_value;
}

// full_new_depth()

static int full_new_depth(int depth, int move, board_t * board, bool * move_check, int history_score, int ThreadId) {

   ASSERT(depth_is_ok(depth));
   ASSERT(move_is_ok(move));
   ASSERT(board!=NULL);
   ASSERT(move_check!=NULL);
   ASSERT(depth>0);
   ASSERT(history_score>=1 && history_score <= 16384);
   ASSERT(ThreadId >= 0 && ThreadId < NumberThreadsInternal);
   
   *move_check = move_is_check(move,board); // WHM
   
   if (*move_check) {
      
      if (depth == 1) return depth;
      if (depth > TacticalDepth) return depth;
      if (!Tactical[ThreadId]) return depth;
            
      if (history_score >= 12288) return depth;
      
      ASSERT(!move_is_tactical(move,board));
      if (see_move(move,board) >= -ValuePawn) return depth;
   }
   
   
   return (depth - 1);
}

// full_new_depth_PV()

static int full_new_depth_PV(int depth, int height, int move, board_t * board, bool single_reply, bool * move_check, int history_score, int ThreadId) {

   int capture;

   // WHM: mate threat stuff start
   int piece;
   const sq_t * ptr;
   int piece_nb;
   int from;
   int move_to;
   int move_from;
   int move_piece;
   int me;
   int king_sq;
   int king_colour;
   bool blocked1;
   bool blocked2;
   bool blocked3;
   bool blocked4;
   bool blocked5;
   bool blocked6;
   bool blocked7;
   bool blocked8;
   bool side1;
   bool side2;
   bool side3;
   bool side4;
   int queens;
   int rooks;
   int minors;
   int sides;
   // WHM: mate threat stuff end
   
   ASSERT(depth_is_ok(depth));
   ASSERT(height_is_ok(height));
   ASSERT(move_is_ok(move));
   ASSERT(board!=NULL);
   ASSERT(single_reply==true||single_reply==false);
   ASSERT(depth>0);
   ASSERT(move_check!=NULL);
   ASSERT(ThreadId>=0 && ThreadId <= NumberThreadsInternal);
   
   
   SearchStack[ThreadId][height + 1].cap_extended = false; // WHM
   
   *move_check = move_is_check(move,board);
   
   if (*move_check) return depth;
   
   if (single_reply && ExtendSingleReply) return depth;
   
   
   // MATE THREAT START
   // ---- ------ -----
   // attacking the narrow king box, mate threat depth extension, Rybka lessons.  
   // the accuracy of this new depth extension shows us 
   // how important this function is to our overall ELO!!!!!!!
   
   move_piece = MOVE_PIECE(move,board);
   
   if (depth <= MateThreatDepth && !PIECE_IS_PAWN(move_piece) && !PIECE_IS_KING(move_piece)) {
      
      move_to = MOVE_TO(move);
      move_from = MOVE_FROM(move);
      me = board->turn;
      king_colour = COLOUR_OPP(me);
      king_sq = KING_POS(board,king_colour);
      
      if (narrow_piece_attack_king(board,move_piece,move_to,king_sq)) { // move will attack small box around king.  
         
         if (see_move(move,board) >= 0) {
            
            piece_nb = 1;
            
            ASSERT(0==me                                || 1==me                               );
            ASSERT(0==me&&WhiteQueen12==WhiteQueen12+me || 1==me&&BlackQueen12==WhiteQueen12+me);
            
            queens = board->number[WhiteQueen12  + me];
            rooks  = board->number[WhiteRook12   + me];
            minors = board->number[WhiteKnight12 + me]
                   + board->number[WhiteBishop12 + me];
            
            if (queens == 0  &&  rooks <= 1) piece_nb -= 1;
            
            if      (PIECE_IS_KNIGHT(move_piece)) minors--;
            else if (PIECE_IS_BISHOP(move_piece)) minors--;
            else if (PIECE_IS_ROOK(move_piece))    rooks--;  // #rooks left to mate
            else if (PIECE_IS_QUEEN(move_piece))  queens--; // #queens left to mate
            
            // HACK:         no king == [1]
            for (ptr = &board->piece[me][1]; (from=*ptr) != SquareNone; ptr++) {
               
               piece = board->square[from];
               
               if (from == move_from) {
                   ASSERT(piece == move_piece); 
                   continue; // this piece already accounted for.  
               }
               
               // attacking any square on the board exactly 1 square away from king.  
               if (narrow_piece_attack_king(board,piece,from,king_sq)) {
                   piece_nb++;
                   if      (PIECE_IS_KNIGHT(piece)) minors--;
                   else if (PIECE_IS_BISHOP(piece)) minors--;
                   else if (PIECE_IS_ROOK(piece))    rooks--;  // #rooks left to mate
/*WHM              else if (PIECE_IS_QUEEN(piece))  queens--; WHM*/ // #queens left to mate
               }
            }
            
            ASSERT(minors >= 0);
            ASSERT( rooks >= 0);
            ASSERT(queens >= 0);
            
            if (piece_nb >= 1) {
                // the board as it is
                blocked1 = king_is_blocked(board, king_sq+15, king_colour, king_sq);
                blocked2 = king_is_blocked(board, king_sq+16, king_colour, king_sq);
                blocked3 = king_is_blocked(board, king_sq+17, king_colour, king_sq);
                blocked4 = king_is_blocked(board, king_sq+ 1, king_colour, king_sq);
                blocked5 = king_is_blocked(board, king_sq-15, king_colour, king_sq);
                blocked6 = king_is_blocked(board, king_sq-16, king_colour, king_sq);
                blocked7 = king_is_blocked(board, king_sq-17, king_colour, king_sq);
                blocked8 = king_is_blocked(board, king_sq- 1, king_colour, king_sq);
                // now take into account the king attack move
                if (!blocked1 && PIECE_ATTACK(board,move_piece,move_to,king_sq+15)) blocked1 = true; // move_to, move_piece, king_sq+15
                if (!blocked2 && PIECE_ATTACK(board,move_piece,move_to,king_sq+16)) blocked2 = true; // move_to, move_piece, king_sq+15
                if (!blocked3 && PIECE_ATTACK(board,move_piece,move_to,king_sq+17)) blocked3 = true; // move_to, move_piece, king_sq+15
                if (!blocked4 && PIECE_ATTACK(board,move_piece,move_to,king_sq+ 1)) blocked4 = true; // move_to, move_piece, king_sq+15
                if (!blocked5 && PIECE_ATTACK(board,move_piece,move_to,king_sq-15)) blocked5 = true; // move_to, move_piece, king_sq+15
                if (!blocked6 && PIECE_ATTACK(board,move_piece,move_to,king_sq-16)) blocked6 = true; // move_to, move_piece, king_sq+15
                if (!blocked7 && PIECE_ATTACK(board,move_piece,move_to,king_sq-17)) blocked7 = true; // move_to, move_piece, king_sq+15
                if (!blocked8 && PIECE_ATTACK(board,move_piece,move_to,king_sq- 1)) blocked8 = true; // move_to, move_piece, king_sq+15
                
                side1 = blocked1 && blocked2 && blocked3;
                side2 = blocked3 && blocked4 && blocked5;
                side3 = blocked5 && blocked6 && blocked7;
                side4 = blocked7 && blocked8 && blocked1;
                
                sides = 0;
                
                if (side1) sides++;
                if (side2) sides++;
                if (side3) sides++;
                if (side4) sides++;
                
                if      (sides >= 4 && minors+rooks+queens >= 1) return depth;
                else if (sides >= 3 &&        rooks+queens >= 1) return depth;
                else if (sides >= 2 &&              queens >= 1) return depth;
                else if (side1 && side3 &&    rooks        >= 1) return depth;
                else if (side2 && side4 &&    rooks        >= 1) return depth;
            }
         }
      }
   }
   // MATE THREAT END
   
   // Passed pawn pushes, all passers!  
   if (passed_pawn_move(move,board))  return depth;
   if (move_is_dangerous(move,board)) return depth;
       
   capture = board->square[MOVE_TO(move)];

   if (capture != Empty) {

       if (!PIECE_IS_PAWN(capture)) {

           if (   board->piece_size[White]    +    board->piece_size[Black] == 3
               || board->piece_size[White] == 3 && board->piece_size[Black] == 2 
               || board->piece_size[White] == 2 && board->piece_size[Black] == 3) {

               return depth; 
           }
       }
       // pawns also included in this.  
       if (!SearchStack[ThreadId][height    ].cap_extended && see_move(move,board) >= -100) {
            SearchStack[ThreadId][height + 1].cap_extended = true; // WHM
            return depth;
       }

   } else if (MOVE_IS_EN_PASSANT(move)) {
 
       if (!SearchStack[ThreadId][height    ].cap_extended && see_move(move,board) >= -100) {
            SearchStack[ThreadId][height + 1].cap_extended = true; // WHM
            return depth;
       }
   }
   
   
   return (depth - 1);
}

// move_is_dangerous()

static bool move_is_dangerous(int move, const board_t * board) {

   int piece;
   int move_to;

   ASSERT(move_is_ok(move));
   ASSERT(board!=NULL);

//WHM ASSERT(!move_is_tactical(move,board));

   piece = MOVE_PIECE(move,board);

   if (PIECE_IS_PAWN(piece)) {

      move_to = MOVE_TO(move);
      if (PAWN_RANK(move_to,board->turn) >= Rank6) return true;
      if (is_passed(board,MOVE_TO(move))) {
         ASSERT(lead_passed_pawn_move(move,board));
         return true;
      }
      ASSERT(!lead_passed_pawn_move(move,board));
   }

   return false;
}

// capture_is_dangerous()

static bool capture_is_dangerous(int move, const board_t * board) {

   int piece, capture;

   ASSERT(move_is_ok(move));
   ASSERT(board!=NULL);

   ASSERT(move_is_tactical(move,board));

   piece = MOVE_PIECE(move,board);

   if (PIECE_IS_PAWN(piece)
    && PAWN_RANK(MOVE_TO(move),board->turn) >= Rank7) {
      return true;
   }

   capture = move_capture(move,board);

   if (PIECE_IS_QUEEN(capture)) return true;

   if (PIECE_IS_PAWN(capture)
    && PAWN_RANK(MOVE_TO(move),board->turn) <= Rank2) {
      return true;
   }

   return false;
}

// simple_stalemate()

static bool simple_stalemate(const board_t * board) {

   int me, opp;
   int king;
   int opp_flag;
   int from, to;
   int capture;
   const inc_t * inc_ptr;
   int inc;

   ASSERT(board!=NULL);

   ASSERT(board_is_legal(board));
   ASSERT(!board_is_check(board));

   // lone king?

   me = board->turn;
   if (board->piece_size[me] != 1 || board->pawn_size[me] != 0) return false; // no

   // king in a corner?

   king = KING_POS(board,me);
   if (king != A1 && king != H1 && king != A8 && king != H8) return false; // no

   // init

   opp = COLOUR_OPP(me);
   opp_flag = COLOUR_FLAG(opp);

   // king can move?

   from = king;

   for (inc_ptr = KingInc; (inc=*inc_ptr) != IncNone; inc_ptr++) {
      to = from + inc;
      capture = board->square[to];
      if (capture == Empty || FLAG_IS(capture,opp_flag)) {
         if (!is_attacked(board,to,opp)) return false; // legal king move
      }
   }

   // no legal move

   ASSERT(board_is_stalemate((board_t*)board));

   return true;
}

//          is_passed()  fast for (node_type != NodePV) which are over 99.9% of boards early.

static bool is_passed(const board_t * board, int to) { 

   int t2; 
   int me, opp;
   int file, rank;
   
   me = board->turn; 
   opp = COLOUR_OPP(me);
   file = SQUARE_FILE(to);
   rank = PAWN_RANK(to,me);
   
   t2 = board->pawn_file[me][file] | BitRev[board->pawn_file[opp][file]]; 
   
   // passed pawns 
   if ((t2 & BitGT[rank]) == 0) { 
 
      if (((BitRev[board->pawn_file[opp][file-1]] | BitRev[board->pawn_file[opp][file+1]]) & BitGT[rank]) == 0) { 

         return true;
      } 
   } 
   
   return false;
}

//          passed_pawn_move()     slow for NodePV use only, all passers even doubled...

static bool passed_pawn_move(int move, const board_t * board) {
   
   int opp_pawn, delta, to;
   
   ASSERT(board!=NULL);

   if (!PIECE_IS_PAWN(MOVE_PIECE(move,board))) {
      return false;
   }

   opp_pawn = PAWN_MAKE(COLOUR_OPP(board->turn));
   delta    =        PAWN_MOVE_INC(board->turn);
   
   for (to = MOVE_TO(move) + delta; PAWN_RANK(to,board->turn) <= Rank7; to = to + delta) {

      if (board->square[to  ] == opp_pawn  || 
          board->square[to-1] == opp_pawn  || 
          board->square[to+1] == opp_pawn     ) {

         return false;
      }
   }
      
   
   return true;
}

static bool king_is_blocked(const board_t * board, int sq, int king_colour, int king_sq) {

    ASSERT(PIECE_IS_KING(board->square[king_sq]));
    ASSERT(COLOUR_IS(board->square[king_sq],king_colour));
    ASSERT(1==DISTANCE(sq,king_sq));

    if (Edge == board->square[sq]) return true;
    if (COLOUR_IS(board->square[sq],king_colour)) return true;
    if (is_attacked(board, sq, COLOUR_OPP(king_colour))) return true;

    return false;
}

#if DEBUG

//          lead_passed_pawn_move()     slow for DEBUG use only

static bool lead_passed_pawn_move(int move, const board_t * board) {
   
   int opp_pawn, delta, to, my_pawn;
   
   ASSERT(board!=NULL);
   ASSERT(PIECE_IS_PAWN(MOVE_PIECE(move,board)));

   my_pawn  = PAWN_MAKE(           board->turn );
   opp_pawn = PAWN_MAKE(COLOUR_OPP(board->turn));
   delta    =        PAWN_MOVE_INC(board->turn);
   
   for (to = MOVE_TO(move) + delta; PAWN_RANK(to,board->turn) <= Rank7; to = to + delta) {

      if (board->square[to  ] == opp_pawn  || 
          board->square[to  ] ==  my_pawn  || // doubled passers, only count the lead passer.
          board->square[to-1] == opp_pawn  || 
          board->square[to+1] == opp_pawn     ) {

         return false;
      }
   }
      
   
   return true;
}

#endif

// end of search_full.cpp