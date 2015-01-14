#ifndef _CRT_SECURE_NO_DEPRECATE
#define _CRT_SECURE_NO_DEPRECATE
#endif
#include <iostream>
#include <stdlib.h>
#include <stdio.h>
#include "extglobals.h"
#include "protos.h"
#include "timer.h"

/**
 Linux (POSIX) implementation of _kbhit().
 Morgan McGuire, morgan@cs.brown.edu
 */
#include <stdio.h>
#include <sys/select.h>
#include <termios.h>
#include <stropts.h>

int _kbhit() {
	return 0;
}


void Board::readClockAndInput()
{}








