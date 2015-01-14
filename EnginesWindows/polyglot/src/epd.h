
// epd.h

#ifndef EPD_H
#define EPD_H

// includes

#include "util.h"

// functions

extern void epd_test   (int argc, char * argv[]);

extern bool epd_get_op (const char record[], const char opcode[], char string[], int size);

#endif // !defined EPD_H

// end of epd.h

