//  GREKO Chess Engine
//  (c) 2002-2012 Vladimir Medvedev <vrm@bk.ru>
//  http://greko.110mb.com

//  eval.h: static position evaluation
//  modified: 31-Dec-2012

#pragma once

#include "position.h"

const EVAL INFINITY_SCORE  = 50000;
const EVAL CHECKMATE_SCORE = 32768;
const EVAL DRAWm_score      = 0;

extern EVAL VAL_P;
extern EVAL VAL_N;
extern EVAL VAL_B;
extern EVAL VAL_R;
extern EVAL VAL_Q;

extern EVAL VALUE[14];

void InitEval();
EVAL Evaluate(const Position& pos, EVAL alpha, EVAL beta);

struct EvalWeights
{
  EvalWeights() :
    Material(50), BoardControl(50), Mobility(50), PawnStruct(50), PawnPassed(50), KingSafety(50), RandomEval(0) {}

  int Material;
  int BoardControl;
  int Mobility;
  int PawnStruct;
  int PawnPassed;
  int KingSafety;
  int RandomEval;
};

extern EvalWeights g_evalWeights;

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
