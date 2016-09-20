
// pv.h

#ifndef PV_H
#define PV_H

// includes

#include "board.h"
#include "move.h"
#include "util.h"

// macros

#define PV_CLEAR(pv) (*(pv)=MoveNone)

// functions

extern bool pv_is_ok     (const mv_t pv[]);

extern void pv_copy      (mv_t dst[], const mv_t src[]);
extern void pv_cat       (mv_t dst[], const mv_t src[], int move);

extern bool pv_to_string (const mv_t pv[], char string[], int size);

#endif // !defined PV_H

// end of pv.h

