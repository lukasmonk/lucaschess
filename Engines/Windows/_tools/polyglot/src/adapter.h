
// adapter.h

#ifndef ADAPTER_H
#define ADAPTER_H

// includes

#include "util.h"

// functions

extern void adapter_loop ();
extern void xboard_step(void);
extern void engine_move_fail(char *move_string);

#endif // !defined ADAPTER_H

// end of adapter.h

