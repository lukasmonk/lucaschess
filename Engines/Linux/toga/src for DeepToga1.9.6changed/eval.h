
// eval.h

#ifndef EVAL_H
#define EVAL_H

// includes

#include "board.h"

// functions

extern void eval_init ();
extern void eval_parameter ();

extern int  eval      (board_t * board, int alpha, int beta, bool full_q_eval, int ThreadId);

#endif // !defined EVAL_H

// end of eval.h

