
// main.h

#ifndef MAIN_H
#define MAIN_H

// includes

#include "util.h"

// functions

extern void quit ();
#ifdef _WIN32
extern HANDLE Engine_ready_ok;
extern HANDLE Engine_sync_stop;
#endif


#endif // !defined MAIN_H

// end of main.h

