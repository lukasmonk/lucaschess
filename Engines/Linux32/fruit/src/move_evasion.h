
// move_evasion.h

#ifndef MOVE_EVASION_H
#define MOVE_EVASION_H

// includes

#include "attack.h"
#include "board.h"
#include "list.h"
#include "util.h"

// functions

extern void gen_legal_evasions  (list_t * list, const board_t * board, const attack_t * attack);
extern void gen_pseudo_evasions (list_t * list, const board_t * board, const attack_t * attack);

extern bool legal_evasion_exist (const board_t * board, const attack_t * attack);

#endif // !defined MOVE_EVASION_H

// end of move_evasion.h

