#pragma once
#include "board.h"

extern uint64_t perft(board::Board& B, int depth, int ply);
extern bool test_perft();
extern bool test_see();

extern void bench(int depth);

