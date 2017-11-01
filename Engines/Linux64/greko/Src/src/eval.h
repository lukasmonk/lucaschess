//  GREKO Chess Engine
//  (c) 2002-2013 Vladimir Medvedev <vrm@bk.ru>
//  http://greko.110mb.com

//  eval.h: static position evaluation
//  modified: 30-June-2013

#ifndef EVAL_H
#define EVAL_H

#include "defaults.h"
#include "position.h"

const EVAL INFINITY_SCORE  = 50000;
const EVAL CHECKMATE_SCORE = 32768;
const EVAL DRAW_SCORE      = 0;

extern EVAL VAL_P;
extern EVAL VAL_N;
extern EVAL VAL_B;
extern EVAL VAL_R;
extern EVAL VAL_Q;
extern EVAL VALUE[14];

void InitEval();
EVAL Evaluate(const Position& pos, EVAL alpha, EVAL beta);

struct EvalParams
{
	EvalParams() :
		Material(DEFAULT_MATERIAL),
		BoardControl(DEFAULT_BOARD_CONTROL),
		Mobility(DEFAULT_MOBILITY),
		PawnStruct(DEFAULT_PAWN_STRUCT),
		PawnPassed(DEFAULT_PAWN_PASSED),
		KingSafety(DEFAULT_KING_SAFETY),
		LazyEvalMargin(DEFAULT_LAZY_EVAL_MARGIN),
		DrawScore(DEFAULT_DRAW_SCORE)
	{}

	int Material;
	int BoardControl;
	int Mobility;
	int PawnStruct;
	int PawnPassed;
	int KingSafety;
	int LazyEvalMargin;
	int DrawScore;
};

extern EvalParams g_evalParams;

enum EVAL_ESTIMATION
{
	EVAL_THEORETICAL_DRAW = 0,
	EVAL_PRACTICAL_DRAW   = 1,
	EVAL_PROBABLE_DRAW    = 2,
	EVAL_WHITE_CANNOT_WIN = 3,
	EVAL_BLACK_CANNOT_WIN = 4,
	EVAL_UNKNOWN          = 5
};

EVAL_ESTIMATION EstimateDraw(const Position& pos);

#endif
