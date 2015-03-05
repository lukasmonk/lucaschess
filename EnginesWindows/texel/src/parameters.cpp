/*
    Texel - A UCI chess engine.
    Copyright (C) 2012-2014  Peter Österlund, peterosterlund2@gmail.com

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

/*
 * parameters.cpp
 *
 *  Created on: Feb 25, 2012
 *      Author: petero
 */

#include "parameters.hpp"
#include "computerPlayer.hpp"

namespace UciParams {
    std::shared_ptr<Parameters::SpinParam> hash(std::make_shared<Parameters::SpinParam>("Hash", 1, 524288, 16));
    std::shared_ptr<Parameters::CheckParam> ownBook(std::make_shared<Parameters::CheckParam>("OwnBook", false));
    std::shared_ptr<Parameters::CheckParam> ponder(std::make_shared<Parameters::CheckParam>("Ponder", true));
    std::shared_ptr<Parameters::CheckParam> analyseMode(std::make_shared<Parameters::CheckParam>("UCI_AnalyseMode", false));
    std::shared_ptr<Parameters::SpinParam> strength(std::make_shared<Parameters::SpinParam>("Strength", 0, 1000, 1000));
    std::shared_ptr<Parameters::SpinParam> threads(std::make_shared<Parameters::SpinParam>("Threads", 1, 64, 1));
    std::shared_ptr<Parameters::SpinParam> multiPV(std::make_shared<Parameters::SpinParam>("MultiPV", 1, 256, 1));

    std::shared_ptr<Parameters::StringParam> gtbPath(std::make_shared<Parameters::StringParam>("GaviotaTbPath", ""));
    std::shared_ptr<Parameters::SpinParam> gtbCache(std::make_shared<Parameters::SpinParam>("GaviotaTbCache", 1, 2047, 1));
    std::shared_ptr<Parameters::StringParam> rtbPath(std::make_shared<Parameters::StringParam>("SyzygyPath", ""));
    std::shared_ptr<Parameters::SpinParam> minProbeDepth(std::make_shared<Parameters::SpinParam>("MinProbeDepth", 0, 100, 1));

    std::shared_ptr<Parameters::ButtonParam> clearHash(std::make_shared<Parameters::ButtonParam>("Clear Hash"));
}

int pieceValue[Piece::nPieceTypes];

DEFINE_PARAM(pV);
DEFINE_PARAM(nV);
DEFINE_PARAM(bV);
DEFINE_PARAM(rV);
DEFINE_PARAM(qV);
DEFINE_PARAM(kV);

DEFINE_PARAM(pawnIslandPenalty);
DEFINE_PARAM(pawnBackwardPenalty);
DEFINE_PARAM(pawnSemiBackwardPenalty1);
DEFINE_PARAM(pawnSemiBackwardPenalty2);
DEFINE_PARAM(pawnRaceBonus);
DEFINE_PARAM(passedPawnEGFactor);
DEFINE_PARAM(RBehindPP1);
DEFINE_PARAM(RBehindPP2);

DEFINE_PARAM(QvsRMBonus1);
DEFINE_PARAM(QvsRMBonus2);
DEFINE_PARAM(knightVsQueenBonus1);
DEFINE_PARAM(knightVsQueenBonus2);
DEFINE_PARAM(knightVsQueenBonus3);
DEFINE_PARAM(krkpBonus);
DEFINE_PARAM(krpkbBonus);
DEFINE_PARAM(krpkbPenalty);
DEFINE_PARAM(krpknBonus);
DEFINE_PARAM(RvsBPBonus);

DEFINE_PARAM(pawnTradePenalty);
DEFINE_PARAM(pieceTradeBonus);
DEFINE_PARAM(pawnTradeThreshold);
DEFINE_PARAM(pieceTradeThreshold);

DEFINE_PARAM(threatBonus1);
DEFINE_PARAM(threatBonus2);

DEFINE_PARAM(rookHalfOpenBonus);
DEFINE_PARAM(rookOpenBonus);
DEFINE_PARAM(rookDouble7thRowBonus);
DEFINE_PARAM(trappedRookPenalty1);
DEFINE_PARAM(trappedRookPenalty2);

DEFINE_PARAM(bishopPairPawnPenalty);
DEFINE_PARAM(trappedBishopPenalty);
DEFINE_PARAM(oppoBishopPenalty);

DEFINE_PARAM(kingSafetyHalfOpenBCDEFG1);
DEFINE_PARAM(kingSafetyHalfOpenBCDEFG2);
DEFINE_PARAM(kingSafetyHalfOpenAH1);
DEFINE_PARAM(kingSafetyHalfOpenAH2);
DEFINE_PARAM(kingSafetyWeight1);
DEFINE_PARAM(kingSafetyWeight2);
DEFINE_PARAM(kingSafetyWeight3);
DEFINE_PARAM(kingSafetyWeight4);
DEFINE_PARAM(kingSafetyThreshold);
DEFINE_PARAM(knightKingProtectBonus);
DEFINE_PARAM(bishopKingProtectBonus);
DEFINE_PARAM(pawnStormBonus);

DEFINE_PARAM(pawnLoMtrl);
DEFINE_PARAM(pawnHiMtrl);
DEFINE_PARAM(minorLoMtrl);
DEFINE_PARAM(minorHiMtrl);
DEFINE_PARAM(castleLoMtrl);
DEFINE_PARAM(castleHiMtrl);
DEFINE_PARAM(queenLoMtrl);
DEFINE_PARAM(queenHiMtrl);
DEFINE_PARAM(passedPawnLoMtrl);
DEFINE_PARAM(passedPawnHiMtrl);
DEFINE_PARAM(kingSafetyLoMtrl);
DEFINE_PARAM(kingSafetyHiMtrl);
DEFINE_PARAM(oppoBishopLoMtrl);
DEFINE_PARAM(oppoBishopHiMtrl);
DEFINE_PARAM(knightOutpostLoMtrl);
DEFINE_PARAM(knightOutpostHiMtrl);


DEFINE_PARAM(aspirationWindow);
DEFINE_PARAM(rootLMRMoveCount);

DEFINE_PARAM(razorMargin1);
DEFINE_PARAM(razorMargin2);

DEFINE_PARAM(reverseFutilityMargin1);
DEFINE_PARAM(reverseFutilityMargin2);
DEFINE_PARAM(reverseFutilityMargin3);
DEFINE_PARAM(reverseFutilityMargin4);

DEFINE_PARAM(futilityMargin1);
DEFINE_PARAM(futilityMargin2);
DEFINE_PARAM(futilityMargin3);
DEFINE_PARAM(futilityMargin4);

DEFINE_PARAM(lmpMoveCountLimit1);
DEFINE_PARAM(lmpMoveCountLimit2);
DEFINE_PARAM(lmpMoveCountLimit3);
DEFINE_PARAM(lmpMoveCountLimit4);

DEFINE_PARAM(lmrMoveCountLimit1);
DEFINE_PARAM(lmrMoveCountLimit2);

DEFINE_PARAM(quiesceMaxSortMoves);
DEFINE_PARAM(deltaPruningMargin);

DEFINE_PARAM(timeMaxRemainingMoves);
DEFINE_PARAM(bufferTime);
DEFINE_PARAM(minTimeUsage);
DEFINE_PARAM(maxTimeUsage);
DEFINE_PARAM(timePonderHitRate);

/** Piece/square table for king during middle game. */
ParamTable<64> kt1b { -200, 200, useUciParam,
    {  71, 118, 113,  98,  88, 112, 129,  44,
      117,   3,  36,  37,  64,  37,   3,  99,
      -28,  -4,  13,  12,  18,  28,  25, -41,
      -11, -11, -15, -20, -12,   4,  -9, -19,
      -36, -14, -29, -31, -33, -33, -19, -76,
      -12,   1, -25, -49, -23, -11,   7, -17,
       24,  23,   3,  -7,   2,  -1,  39,  36,
       -2,  34,  24, -27,  14, -15,  31,  10 },
    {   1,   2,   3,   4,   5,   6,   7,   8,
        9,  10,  11,  12,  13,  14,  15,  16,
       17,  18,  19,  20,  21,  22,  23,  24,
       25,  26,  27,  28,  29,  30,  31,  32,
       33,  34,  35,  36,  37,  38,  39,  40,
       41,  42,  43,  44,  45,  46,  47,  48,
       49,  50,  51,  52,  53,  54,  55,  56,
       57,  58,  59,  60,  61,  62,  63,  64 }
};
ParamTableMirrored<64> kt1w(kt1b);

/** Piece/square table for king during end game. */
ParamTable<64> kt2b { -200, 200, useUciParam,
    { -18,  48,  73,  82,  82,  73,  48, -18,
       33,  93, 108, 100, 100, 108,  93,  33,
       78, 104, 110, 109, 109, 110, 104,  78,
       68,  97, 105, 108, 108, 105,  97,  68,
       54,  79,  92,  97,  97,  92,  79,  54,
       43,  61,  75,  82,  82,  75,  61,  43,
       28,  47,  63,  62,  62,  63,  47,  28,
        0,  20,  33,  20,  20,  33,  20,   0 },
    {   1,   2,   3,   4,   4,   3,   2,   1,
        5,   6,   7,   8,   8,   7,   6,   5,
        9,  10,  11,  12,  12,  11,  10,   9,
       13,  14,  15,  16,  16,  15,  14,  13,
       17,  18,  19,  20,  20,  19,  18,  17,
       21,  22,  23,  24,  24,  23,  22,  21,
       25,  26,  27,  28,  28,  27,  26,  25,
        0,  29,  30,  31,  31,  30,  29,   0 }
};
ParamTableMirrored<64> kt2w(kt2b);

/** Piece/square table for pawns during middle game. */
ParamTable<64> pt1b { -200, 300, useUciParam,
    {   0,   0,   0,   0,   0,   0,   0,   0,
      137, 103, 117, 123,  74,  97, -12, 148,
       19,  31,  28,  26,  22,  41,  32,   8,
       -7,  -5, -11,  -1,   4, -12,  -9,  -7,
       -9,  -2,  -7,   0,  -1,   2,  -2, -17,
      -12,  -9, -27, -15,  -5, -12,  -5,  -6,
      -10, -14, -24, -20, -13,   0,   5,  -5,
        0,   0,   0,   0,   0,   0,   0,   0 },
    {   0,   0,   0,   0,   0,   0,   0,   0,
        1,   2,   3,   4,   5,   6,   7,   8,
        9,  10,  11,  12,  13,  14,  15,  16,
       17,  18,  19,  20,  21,  22,  23,  24,
       25,  26,  27,  28,  29,  30,  31,  32,
       33,  34,  35,  36,  37,  38,  39,  40,
       41,  42,  43,  44,  45,  46,  47,  48,
        0,   0,   0,   0,   0,   0,   0,   0 }
};
ParamTableMirrored<64> pt1w(pt1b);

/** Piece/square table for pawns during end game. */
ParamTable<64> pt2b { -200, 200, useUciParam,
    {   0,   0,   0,   0,   0,   0,   0,   0,
      -10,   9,   4,  -9,  -9,   4,   9, -10,
       33,  35,  22,   7,   7,  22,  35,  33,
       22,  26,  15,   6,   6,  15,  26,  22,
       12,  23,  13,  13,  13,  13,  23,  12,
        4,  17,  17,  21,  21,  17,  17,   4,
        3,  17,  27,  32,  32,  27,  17,   3,
        0,   0,   0,   0,   0,   0,   0,   0 },
    {   0,   0,   0,   0,   0,   0,   0,   0,
        1,   2,   3,   4,   4,   3,   2,   1,
        5,   6,   7,   8,   8,   7,   6,   5,
        9,  10,  11,  12,  12,  11,  10,   9,
       13,  14,  15,  16,  16,  15,  14,  13,
       17,  18,  19,  20,  20,  19,  18,  17,
       21,  22,  23,  24,  24,  23,  22,  21,
        0,   0,   0,   0,   0,   0,   0,   0 }
};
ParamTableMirrored<64> pt2w(pt2b);

/** Piece/square table for knights during middle game. */
ParamTable<64> nt1b { -300, 200, useUciParam,
    {-258, -21, -42,  -4,  22, -65, -22,-239,
      -57, -52,  13,  40,  35,  98,  -2,  20,
      -41,  -5,  22,  39,  95, 102,  47, -13,
      -15,   1,   9,  38,  16,  45,  22,  23,
      -18,   6,  11,   9,  27,  25,  27, -12,
      -49, -10,  -1,  11,  19,   0,  -5, -35,
      -55, -39, -23,   2,  -2,  -7, -30, -27,
      -78, -38, -42, -34, -24, -31, -39, -94 },
    {   1,   2,   3,   4,   5,   6,   7,   8,
        9,  10,  11,  12,  13,  14,  15,  16,
       17,  18,  19,  20,  21,  22,  23,  24,
       25,  26,  27,  28,  29,  30,  31,  32,
       33,  34,  35,  36,  37,  38,  39,  40,
       41,  42,  43,  44,  45,  46,  47,  48,
       49,  50,  51,  52,  53,  54,  55,  56,
       57,  58,  59,  60,  61,  62,  63,  64 }
};
ParamTableMirrored<64> nt1w(nt1b);

/** Piece/square table for knights during end game. */
ParamTable<64> nt2b { -200, 200, useUciParam,
    { -87,   7,  11,  -2,  -2,  11,   7, -87,
      -10,   8,  20,  37,  37,  20,   8, -10,
       -6,  20,  36,  41,  41,  36,  20,  -6,
       -1,  29,  46,  47,  47,  46,  29,  -1,
        2,  30,  43,  44,  44,  43,  30,   2,
      -24,   6,  23,  34,  34,  23,   6, -24,
      -35,  -3,   4,   9,   9,   4,  -3, -35,
      -42, -57, -15, -12, -12, -15, -57, -42 },
    {   1,   2,   3,   4,   4,   3,   2,   1,
        5,   6,   7,   8,   8,   7,   6,   5,
        9,  10,  11,  12,  12,  11,  10,   9,
       13,  14,  15,  16,  16,  15,  14,  13,
       17,  18,  19,  20,  20,  19,  18,  17,
       21,  22,  23,  24,  24,  23,  22,  21,
       25,  26,  27,  28,  28,  27,  26,  25,
       29,  30,  31,  32,  32,  31,  30,  29 }
};
ParamTableMirrored<64> nt2w(nt2b);

/** Piece/square table for bishops during middle game. */
ParamTable<64> bt1b { -200, 200, useUciParam,
    { -56,  -6,  28, -46,  16, -71,  18, -41,
      -35, -29, -29,  19, -24, -11, -52, -54,
      -19,  11, -10,   2,  41,  56,  42,   3,
      -25, -10,  -5,  30,  -6,   4, -17,  -4,
       -5, -17,  -8,   9,  14, -20, -13,  -1,
      -17,   0,  -2,  -4,  -5,  -5,  -5,   0,
       10,  -5,   5,  -7,  -1,  -5,  11,   9,
      -31,   6, -16, -15, -20, -11, -26, -12 },
    {   1,   2,   3,   4,   5,   6,   7,   8,
        9,  10,  11,  12,  13,  14,  15,  16,
       17,  18,  19,  20,  21,  22,  23,  24,
       25,  26,  27,  28,  29,  30,  31,  32,
       33,  34,  35,  36,  37,  38,  39,  40,
       41,  42,  43,  44,  45,  46,  47,  48,
       49,  50,  51,  52,  53,  54,  55,  56,
       57,  58,  59,  60,  61,  62,  63,  64 }
};
ParamTableMirrored<64> bt1w(bt1b);

/** Piece/square table for bishops during end game. */
ParamTable<64> bt2b { -200, 200, useUciParam,
    {  20,  20,  17,  22,  22,  17,  20,  20,
       20,  20,  25,  30,  30,  25,  20,  20,
       17,  25,  36,  38,  38,  36,  25,  17,
       22,  30,  38,  43,  43,  38,  30,  22,
       22,  30,  38,  43,  43,  38,  30,  22,
       17,  25,  36,  38,  38,  36,  25,  17,
       20,  20,  25,  30,  30,  25,  20,  20,
       20,  20,  17,  22,  22,  17,  20,  20 },
    {  10,   1,   2,   3,   3,   2,   1,  10,
        1,   4,   5,   6,   6,   5,   4,   1,
        2,   5,   7,   8,   8,   7,   5,   2,
        3,   6,   8,   9,   9,   8,   6,   3,
        3,   6,   8,   9,   9,   8,   6,   3,
        2,   5,   7,   8,   8,   7,   5,   2,
        1,   4,   5,   6,   6,   5,   4,   1,
       10,   1,   2,   3,   3,   2,   1,  10 }
};
ParamTableMirrored<64> bt2w(bt2b);

/** Piece/square table for queens during middle game. */
ParamTable<64> qt1b { -200, 200, useUciParam,
    { -32, -23, -22, -26,  29, -11,  87,  32,
      -66,-100, -54, -79,-113, -32, -66,  49,
      -49, -41, -74, -68, -32, -22, -33, -48,
      -37, -38, -44, -66, -60, -54, -40, -45,
      -27, -32, -19, -38, -30, -32,  -3, -36,
      -27, -17, -14, -21, -11, -14,   3, -20,
      -30, -22, -13,  -4,  -5,   3,  -8, -30,
      -15, -19,  -7,  -8,  -2, -18, -48, -19 },
    {   1,   2,   3,   4,   5,   6,   7,   8,
        9,  10,  11,  12,  13,  14,  15,  16,
       17,  18,  19,  20,  21,  22,  23,  24,
       25,  26,  27,  28,  29,  30,  31,  32,
       33,  34,  35,  36,  37,  38,  39,  40,
       41,  42,  43,  44,  45,  46,  47,  48,
       49,  50,  51,  52,  53,  54,  55,  56,
       57,  58,  59,  60,  61,  62,  63,  64 }
};
ParamTableMirrored<64> qt1w(qt1b);

ParamTable<64> qt2b { -200, 200, useUciParam,
    { -17, -20, -19, -16, -16, -19, -20, -17,
      -20, -23, -14,  -8,  -8, -14, -23, -20,
      -19, -14,  -3,  -5,  -5,  -3, -14, -19,
      -16,  -8,  -5,   7,   7,  -5,  -8, -16,
      -16,  -8,  -5,   7,   7,  -5,  -8, -16,
      -19, -14,  -3,  -5,  -5,  -3, -14, -19,
      -20, -23, -14,  -8,  -8, -14, -23, -20,
      -17, -20, -19, -16, -16, -19, -20, -17 },
     { 10,   1,   2,   3,   3,   2,   1,  10,
        1,   4,   5,   6,   6,   5,   4,   1,
        2,   5,   7,   8,   8,   7,   5,   2,
        3,   6,   8,   9,   9,   8,   6,   3,
        3,   6,   8,   9,   9,   8,   6,   3,
        2,   5,   7,   8,   8,   7,   5,   2,
        1,   4,   5,   6,   6,   5,   4,   1,
       10,   1,   2,   3,   3,   2,   1,  10 }
};
ParamTableMirrored<64> qt2w(qt2b);

/** Piece/square table for rooks during end game. */
ParamTable<64> rt1b { -200, 200, useUciParam,
    {  39,  45,  36,  36,  39,  58,  54,  63,
       34,  36,  47,  53,  47,  53,  57,  47,
       27,  36,  40,  36,  53,  71,  65,  53,
       15,  18,  24,  27,  26,  33,  38,  19,
       -1,  -1,  12,   6,   9,  12,  21,   3,
      -17, -10,  -4,  -3,   0,   3,  15,  -5,
      -24, -18,  -6,   1,   2,  -3,   1, -35,
       -3,  -1,   6,   9,   9,   9,   3,   4 },
    {   1,   2,   3,   4,   5,   6,   7,   8,
        9,  10,  11,  12,  13,  14,  15,  16,
       17,  18,  19,  20,  21,  22,  23,  24,
       25,  26,  27,  28,  29,  30,  31,  32,
       33,  34,  35,  36,  37,  38,  39,  40,
       41,  42,  43,  44,  45,  46,  47,  48,
       49,  50,  51,  52,  53,  54,  55,  56,
       57,  58,  59,  60,  61,  62,  63,  64 }
};
ParamTableMirrored<64> rt1w(rt1b);

ParamTable<64> knightOutpostBonus { 0, 150, useUciParam,
    {   0,   0,   0,   0,   0,   0,   0,   0,
        0,   0,   0,   0,   0,   0,   0,   0,
        0,  29,  49,  46,  46,  49,  29,   0,
        0,  19,  38,  43,  43,  38,  19,   0,
        0,   0,  23,  43,  43,  23,   0,   0,
        0,   0,   0,   0,   0,   0,   0,   0,
        0,   0,   0,   0,   0,   0,   0,   0,
        0,   0,   0,   0,   0,   0,   0,   0 },
    {   0,   0,   0,   0,   0,   0,   0,   0,
        0,   0,   0,   0,   0,   0,   0,   0,
        0,   1,   2,   3,   3,   2,   1,   0,
        0,   4,   5,   6,   6,   5,   4,   0,
        0,   0,   7,   8,   8,   7,   0,   0,
        0,   0,   0,   0,   0,   0,   0,   0,
        0,   0,   0,   0,   0,   0,   0,   0,
        0,   0,   0,   0,   0,   0,   0,   0 }
};

ParamTable<64> protectedPawnBonus { -50, 150, useUciParam,
    {   0,   0,   0,   0,   0,   0,   0,   0,
      104, 109,  69, 136, 136,  69, 109, 104,
       28,  53,  51,  63,  63,  51,  53,  28,
        4,  11,  16,  17,  17,  16,  11,   4,
        4,   6,   5,   8,   8,   5,   6,   4,
        8,  11,  17,  11,  11,  17,  11,   8,
        0,   0,   0,   0,   0,   0,   0,   0,
        0,   0,   0,   0,   0,   0,   0,   0 },
    {   0,   0,   0,   0,   0,   0,   0,   0,
        1,   2,   3,   4,   4,   3,   2,   1,
        5,   6,   7,   8,   8,   7,   6,   5,
        9,  10,  11,  12,  12,  11,  10,   9,
       13,  14,  15,  16,  16,  15,  14,  13,
       17,  18,  19,  20,  20,  19,  18,  17,
        0,   0,   0,   0,   0,   0,   0,   0,
        0,   0,   0,   0,   0,   0,   0,   0 }
};

ParamTable<64> attackedPawnBonus { -150, 100, useUciParam,
    {   0,   0,   0,   0,   0,   0,   0,   0,
      -35, -55,  17, -16, -16,  17, -55, -35,
       10,   0,  20,  18,  18,  20,   0,  10,
      -10,  -5,  -6,   9,   9,  -6,  -5, -10,
      -35,  11, -12,  13,  13, -12,  11, -35,
     -101, -24, -59, -43, -43, -59, -24,-101,
        0,   0,   0,   0,   0,   0,   0,   0,
        0,   0,   0,   0,   0,   0,   0,   0 },
    {   0,   0,   0,   0,   0,   0,   0,   0,
        1,   2,   3,   4,   4,   3,   2,   1,
        5,   6,   7,   8,   8,   7,   6,   5,
        9,  10,  11,  12,  12,  11,  10,   9,
       13,  14,  15,  16,  16,  15,  14,  13,
       17,  18,  19,  20,  20,  19,  18,  17,
        0,   0,   0,   0,   0,   0,   0,   0,
        0,   0,   0,   0,   0,   0,   0,   0 }
};

ParamTable<4> protectBonus { -50, 50, useUciParam,
    {  1, 11,  7,  2 },
    {  1,  2,  3,  4 }
};

ParamTable<15> rookMobScore { -50, 50, useUciParam,
    {-17,-11, -6, -2,  0,  5,  9, 12, 17, 20, 22, 26, 28, 25, 30 },
    {   1, 2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15 }
};
ParamTable<14> bishMobScore = { -50, 50, useUciParam,
    {-14,-10, -1,  5, 12, 18, 24, 28, 31, 35, 34, 38, 39, 31 },
    {  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14 }
};
ParamTable<28> knightMobScore { -200, 200, useUciParam,
    {-96,-25, 16,-36,-12,  4, 10,-49,-18, -3, 11, 15,-20,-20,-12, -7,  1,  6,  8,-22,-22,-19,-10, -3,  2,  7,  9,  6 },
    {  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 13, 14, 15, 16, 17, 18, 19, 19, 20, 21, 22, 23, 24, 25, 26 }
};
ParamTable<28> queenMobScore { -100, 100, useUciParam,
    {  7, -1, -4, -3, -1,  1,  4,  5,  9, 10, 14, 17, 20, 24, 28, 30, 35, 36, 41, 43, 44, 46, 45, 45, 39, 39, 43, 30 },
    {  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28 }
};

ParamTable<36> connectedPPBonus { -200, 400, useUciParam,
    {  -4,  -3,   3,  11,   5, -13,
       -3,   1,   0,   7,   6,   8,
        3,   0,  16,  12,  22,  17,
       11,   7,  12,  40,  -2,  38,
        5,   6,  22,  -2, 102, -27,
      -13,   8,  17,  38, -27, 337 },
    {   1,   2,   4,   7,  11,  16,
        2,   3,   5,   8,  12,  17,
        4,   5,   6,   9,  13,  18,
        7,   8,   9,  10,  14,  19,
       11,  12,  13,  14,  15,  20,
       16,  17,  18,  19,  20,  21 }
};

ParamTable<8> passedPawnBonusX { -200, 200, useUciParam,
    {  0,  2, -2, -5, -5, -2,  2,  0 },
    {  0,  1,  2,  3,  3,  2,  1,  0 }
};

ParamTable<8> passedPawnBonusY { -200, 200, useUciParam,
    {  0,  3,  5, 14, 33, 63,103,  0 },
    {  0,  1,  2,  3,  4,  5,  6,  0 }
};

ParamTable<10> ppBlockerBonus { -50, 50, useUciParam,
    { 23, 26, 11,-11, 47, -1,  3,  1, -2,  8 },
    {  1,  2,  3,  4,  5,  6,  7,  8,  9, 10 }
};

ParamTable<8> candidatePassedBonus { -200, 200, useUciParam,
    { -1, -1,  5, 14, 36, 15, -1, -1 },
    {  0,  1,  2,  3,  4,  5,  0,  0 }
};

ParamTable<16> majorPieceRedundancy { -200, 200, useUciParam,
    {   0, -85,   0,   0,
       85,   0,   0,   0,
        0,   0,   0,  88,
        0,   0, -88,   0 },
    {   0,  -1,   0,   0,
        1,   0,   0,   0,
        0,   0,   0,   2,
        0,   0,  -2,   0 }
};

ParamTable<5> QvsRRBonus { -200, 200, useUciParam,
    {-10, 31, 55,113, 68 },
    {  1,  2,  3,  4,  5 }
};

ParamTable<7> RvsMBonus { -200, 200, useUciParam,
    { 13, 37, 45, 35, 26, -2,-60 },
    {  1,  2,  3,  4,  5,  6,  7 }
};

ParamTable<7> RvsMMBonus { -200, 200, useUciParam,
    {-92,-92,-12,  7, 12, 19, 54 },
    {   1,   1,  2,  3,  4,  5,  6 }
};

ParamTable<4> bishopPairValue { 0, 200, useUciParam,
    {100, 79, 63, 57 },
    {  1,  2,  3,  4 }
};

ParamTable<7> rookEGDrawFactor { 0, 255, useUciParam,
    { 65, 70,109,137,133,156,156 },
    {  1,  2,  3,  4,  5,  6,  7 }
};

ParamTable<7> RvsBPDrawFactor { 0, 255, useUciParam,
    {128, 93,108,123,127,249,162 },
    {  0,  1,  2,  3,  4,  5,  6 }
};
ParamTable<4> castleFactor { 0, 128, useUciParam,
    { 64, 42, 30, 23 },
    {  1,  2,  3,  4 }
};

ParamTable<9> pawnShelterTable { -100, 100, useUciParam,
    { 13, 31,-20,  6, 23,-12,  1,  9,  2 },
    {  1,  2,  3,  4,  5,  6,  7,  8,  9 }
};

ParamTable<9> pawnStormTable { -400, 100, useUciParam,
    {-166,-93,-273, 37, 32,  4, 14,  4,-12 },
    {  1,   2,   3,  4,  5,  6,  7,  8,  9 }
};

ParamTable<14> kingAttackWeight { 0, 400, useUciParam,
    {  0,  3,  0,  6,  7, 14, 29, 57, 64,103,112,152,211,353 },
    {  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13 }
};

ParamTable<5> kingPPSupportK { 0, 200, useUciParam,
    { 47, 71, 65, 56, 98 },
    {  1,  2,  3,  4,  5 }
};

ParamTable<8> kingPPSupportP { 1, 64, useUciParam,
    {  0,  4,  4, 10, 16, 22, 32,  0 },
    {  0,  1,  2,  3,  4,  5,  0,  0 }
};

ParamTable<8> pawnDoubledPenalty { 0, 50, useUciParam,
    { 42, 22, 17, 11, 11, 17, 22, 42 },
    {  1,  2,  3,  4,  4,  3,  2,  1 }
};

ParamTable<8> pawnIsolatedPenalty { 0, 50, useUciParam,
    {  2, 12, 10, 14, 14, 10, 12,  2 },
    {  1,  2,  3,  4,  4,  3,  2,  1 }
};

ParamTable<10> halfMoveFactor { 0, 192, useUciParam,
    {128,128,128,128, 43, 27, 19, 13,  9,  5 },
    {  0,  0,  0,  0,  1,  2,  3,  4,  5,  6 }
};

ParamTable<9> stalePawnFactor { 0, 192, useUciParam,
    {119,127,131,133,137,129,104, 65, 28 },
    {  1,  2,  3,  4,  5,  6,  7,  8,  9 }
};

Parameters::Parameters() {
    std::string about = ComputerPlayer::engineName +
                        " by Peter Osterlund, see http://web.comhem.se/petero2home/javachess/index.html#texel";
    addPar(std::make_shared<StringParam>("UCI_EngineAbout", about));

    addPar(UciParams::hash);
    addPar(UciParams::ownBook);
    addPar(UciParams::ponder);
    addPar(UciParams::analyseMode);
    addPar(UciParams::strength);
    addPar(UciParams::threads);
    addPar(UciParams::multiPV);

    addPar(UciParams::gtbPath);
    addPar(UciParams::gtbCache);
    addPar(UciParams::rtbPath);
    addPar(UciParams::minProbeDepth);
    addPar(UciParams::clearHash);

    // Evaluation parameters
    REGISTER_PARAM(pV, "PawnValue");
    REGISTER_PARAM(nV, "KnightValue");
    REGISTER_PARAM(bV, "BishopValue");
    REGISTER_PARAM(rV, "RookValue");
    REGISTER_PARAM(qV, "QueenValue");
    REGISTER_PARAM(kV, "KingValue");

    REGISTER_PARAM(pawnIslandPenalty, "PawnIslandPenalty");
    REGISTER_PARAM(pawnBackwardPenalty, "PawnBackwardPenalty");
    REGISTER_PARAM(pawnSemiBackwardPenalty1, "PawnSemiBackwardPenalty1");
    REGISTER_PARAM(pawnSemiBackwardPenalty2, "PawnSemiBackwardPenalty2");
    REGISTER_PARAM(pawnRaceBonus, "PawnRaceBonus");
    REGISTER_PARAM(passedPawnEGFactor, "PassedPawnEGFactor");
    REGISTER_PARAM(RBehindPP1, "RookBehindPassedPawn1");
    REGISTER_PARAM(RBehindPP2, "RookBehindPassedPawn2");

    REGISTER_PARAM(QvsRMBonus1, "QueenVsRookMinorBonus1");
    REGISTER_PARAM(QvsRMBonus2, "QueenVsRookMinorBonus2");
    REGISTER_PARAM(knightVsQueenBonus1, "KnightVsQueenBonus1");
    REGISTER_PARAM(knightVsQueenBonus2, "KnightVsQueenBonus2");
    REGISTER_PARAM(knightVsQueenBonus3, "KnightVsQueenBonus3");
    REGISTER_PARAM(krkpBonus, "RookVsPawnBonus");
    REGISTER_PARAM(krpkbBonus, "RookPawnVsBishopBonus");
    REGISTER_PARAM(krpkbPenalty, "RookPawnVsBishopPenalty");
    REGISTER_PARAM(krpknBonus, "RookPawnVsKnightBonus");
    REGISTER_PARAM(RvsBPBonus, "RookVsBishopPawnBonus");

    REGISTER_PARAM(pawnTradePenalty, "PawnTradePenalty");
    REGISTER_PARAM(pieceTradeBonus, "PieceTradeBonus");
    REGISTER_PARAM(pawnTradeThreshold, "PawnTradeThreshold");
    REGISTER_PARAM(pieceTradeThreshold, "PieceTradeThreshold");

    REGISTER_PARAM(threatBonus1, "ThreatBonus1");
    REGISTER_PARAM(threatBonus2, "ThreatBonus2");

    REGISTER_PARAM(rookHalfOpenBonus, "RookHalfOpenBonus");
    REGISTER_PARAM(rookOpenBonus, "RookOpenBonus");
    REGISTER_PARAM(rookDouble7thRowBonus, "RookDouble7thRowBonus");
    REGISTER_PARAM(trappedRookPenalty1, "TrappedRookPenalty1");
    REGISTER_PARAM(trappedRookPenalty2, "TrappedRookPenalty2");

    REGISTER_PARAM(bishopPairPawnPenalty, "BishopPairPawnPenalty");
    REGISTER_PARAM(trappedBishopPenalty, "TrappedBishopPenalty");
    REGISTER_PARAM(oppoBishopPenalty, "OppositeBishopPenalty");

    REGISTER_PARAM(kingSafetyHalfOpenBCDEFG1, "KingSafetyHalfOpenBCDEFG1");
    REGISTER_PARAM(kingSafetyHalfOpenBCDEFG2, "KingSafetyHalfOpenBCDEFG2");
    REGISTER_PARAM(kingSafetyHalfOpenAH1, "KingSafetyHalfOpenAH1");
    REGISTER_PARAM(kingSafetyHalfOpenAH2, "KingSafetyHalfOpenAH2");
    REGISTER_PARAM(kingSafetyWeight1, "KingSafetyWeight1");
    REGISTER_PARAM(kingSafetyWeight2, "KingSafetyWeight2");
    REGISTER_PARAM(kingSafetyWeight3, "KingSafetyWeight3");
    REGISTER_PARAM(kingSafetyWeight4, "KingSafetyWeight4");
    REGISTER_PARAM(kingSafetyThreshold, "KingSafetyThreshold");
    REGISTER_PARAM(knightKingProtectBonus, "KnightKingProtectBonus");
    REGISTER_PARAM(bishopKingProtectBonus, "BishopKingProtectBonus");
    REGISTER_PARAM(pawnStormBonus, "PawnStormBonus");

    REGISTER_PARAM(pawnLoMtrl, "PawnLoMtrl");
    REGISTER_PARAM(pawnHiMtrl, "PawnHiMtrl");
    REGISTER_PARAM(minorLoMtrl, "MinorLoMtrl");
    REGISTER_PARAM(minorHiMtrl, "MinorHiMtrl");
    REGISTER_PARAM(castleLoMtrl, "CastleLoMtrl");
    REGISTER_PARAM(castleHiMtrl, "CastleHiMtrl");
    REGISTER_PARAM(queenLoMtrl, "QueenLoMtrl");
    REGISTER_PARAM(queenHiMtrl, "QueenHiMtrl");
    REGISTER_PARAM(passedPawnLoMtrl, "PassedPawnLoMtrl");
    REGISTER_PARAM(passedPawnHiMtrl, "PassedPawnHiMtrl");
    REGISTER_PARAM(kingSafetyLoMtrl, "KingSafetyLoMtrl");
    REGISTER_PARAM(kingSafetyHiMtrl, "KingSafetyHiMtrl");
    REGISTER_PARAM(oppoBishopLoMtrl, "OppositeBishopLoMtrl");
    REGISTER_PARAM(oppoBishopHiMtrl, "OppositeBishopHiMtrl");
    REGISTER_PARAM(knightOutpostLoMtrl, "KnightOutpostLoMtrl");
    REGISTER_PARAM(knightOutpostHiMtrl, "KnightOutpostHiMtrl");

    // Evaluation tables
    kt1b.registerParams("KingTableMG", *this);
    kt2b.registerParams("KingTableEG", *this);
    pt1b.registerParams("PawnTableMG", *this);
    pt2b.registerParams("PawnTableEG", *this);
    nt1b.registerParams("KnightTableMG", *this);
    nt2b.registerParams("KnightTableEG", *this);
    bt1b.registerParams("BishopTableMG", *this);
    bt2b.registerParams("BishopTableEG", *this);
    qt1b.registerParams("QueenTableMG", *this);
    qt2b.registerParams("QueenTableEG", *this);
    rt1b.registerParams("RookTable", *this);

    knightOutpostBonus.registerParams("KnightOutpostBonus", *this);
    protectedPawnBonus.registerParams("ProtectedPawnBonus", *this);
    attackedPawnBonus.registerParams("AttackedPawnBonus", *this);
    protectBonus.registerParams("ProtectBonus", *this);
    rookMobScore.registerParams("RookMobility", *this);
    bishMobScore.registerParams("BishopMobility", *this);
    knightMobScore.registerParams("KnightMobility", *this);
    queenMobScore.registerParams("QueenMobility", *this);
    majorPieceRedundancy.registerParams("MajorPieceRedundancy", *this);
    connectedPPBonus.registerParams("ConnectedPPBonus", *this);
    passedPawnBonusX.registerParams("PassedPawnBonusX", *this);
    passedPawnBonusY.registerParams("PassedPawnBonusY", *this);
    ppBlockerBonus.registerParams("PassedPawnBlockerBonus", *this);
    candidatePassedBonus.registerParams("CandidatePassedPawnBonus", *this);
    QvsRRBonus.registerParams("QueenVs2RookBonus", *this);
    RvsMBonus.registerParams("RookVsMinorBonus", *this);
    RvsMMBonus.registerParams("RookVs2MinorBonus", *this);
    bishopPairValue.registerParams("BishopPairValue", *this);
    rookEGDrawFactor.registerParams("RookEndGameDrawFactor", *this);
    RvsBPDrawFactor.registerParams("RookVsBishopPawnDrawFactor", *this);
    castleFactor.registerParams("CastleFactor", *this);
    pawnShelterTable.registerParams("PawnShelterTable", *this);
    pawnStormTable.registerParams("PawnStormTable", *this);
    kingAttackWeight.registerParams("KingAttackWeight", *this);
    kingPPSupportK.registerParams("KingPassedPawnSupportK", *this);
    kingPPSupportP.registerParams("KingPassedPawnSupportP", *this);
    pawnDoubledPenalty.registerParams("PawnDoubledPenalty", *this);
    pawnIsolatedPenalty.registerParams("PawnIsolatedPenalty", *this);
    halfMoveFactor.registerParams("HalfMoveFactor", *this);
    stalePawnFactor.registerParams("StalePawnFactor", *this);

    // Search parameters
    REGISTER_PARAM(aspirationWindow, "AspirationWindow");
    REGISTER_PARAM(rootLMRMoveCount, "RootLMRMoveCount");

    REGISTER_PARAM(razorMargin1, "RazorMargin1");
    REGISTER_PARAM(razorMargin2, "RazorMargin2");

    REGISTER_PARAM(reverseFutilityMargin1, "ReverseFutilityMargin1");
    REGISTER_PARAM(reverseFutilityMargin2, "ReverseFutilityMargin2");
    REGISTER_PARAM(reverseFutilityMargin3, "ReverseFutilityMargin3");
    REGISTER_PARAM(reverseFutilityMargin4, "ReverseFutilityMargin4");

    REGISTER_PARAM(futilityMargin1, "FutilityMargin1");
    REGISTER_PARAM(futilityMargin2, "FutilityMargin2");
    REGISTER_PARAM(futilityMargin3, "FutilityMargin3");
    REGISTER_PARAM(futilityMargin4, "FutilityMargin4");

    REGISTER_PARAM(lmpMoveCountLimit1, "LMPMoveCountLimit1");
    REGISTER_PARAM(lmpMoveCountLimit2, "LMPMoveCountLimit2");
    REGISTER_PARAM(lmpMoveCountLimit3, "LMPMoveCountLimit3");
    REGISTER_PARAM(lmpMoveCountLimit4, "LMPMoveCountLimit4");

    REGISTER_PARAM(lmrMoveCountLimit1, "LMRMoveCountLimit1");
    REGISTER_PARAM(lmrMoveCountLimit2, "LMRMoveCountLimit2");

    REGISTER_PARAM(quiesceMaxSortMoves, "QuiesceMaxSortMoves");
    REGISTER_PARAM(deltaPruningMargin, "DeltaPruningMargin");

    // Time management parameters
    REGISTER_PARAM(timeMaxRemainingMoves, "TimeMaxRemainingMoves");
    REGISTER_PARAM(bufferTime, "BufferTime");
    REGISTER_PARAM(minTimeUsage, "MinTimeUsage");
    REGISTER_PARAM(maxTimeUsage, "MaxTimeUsage");
    REGISTER_PARAM(timePonderHitRate, "TimePonderHitRate");
}

Parameters&
Parameters::instance() {
    static Parameters inst;
    return inst;
}

void
Parameters::getParamNames(std::vector<std::string>& parNames) {
    parNames = paramNames;
}

std::shared_ptr<Parameters::ParamBase>
Parameters::getParam(const std::string& name) const {
    auto it = params.find(toLowerCase(name));
    if (it == params.end())
        return nullptr;
    return it->second;
}

void
Parameters::addPar(const std::shared_ptr<ParamBase>& p) {
    std::string name = toLowerCase(p->name);
    assert(params.find(name) == params.end());
    params[name] = p;
    paramNames.push_back(name);
}

int
Parameters::Listener::addListener(Func f, bool callNow) {
    int id = ++nextId;
    listeners[id] = f;
    if (callNow)
        f();
    return id;
}

void
Parameters::Listener::removeListener(int id) {
    listeners.erase(id);
}

void
Parameters::Listener::notify() {
    for (auto& e : listeners)
        (e.second)();
}

void
ParamTableBase::registerParamsN(const std::string& name, Parameters& pars,
                                int* table, int* parNo, int N) {
    // Check that each parameter has a single value
    std::map<int,int> parNoToVal;
    int maxParIdx = -1;
    for (int i = 0; i < N; i++) {
        if (parNo[i] == 0)
            continue;
        const int pn = std::abs(parNo[i]);
        const int sign = parNo[i] > 0 ? 1 : -1;
        maxParIdx = std::max(maxParIdx, pn);
        auto it = parNoToVal.find(pn);
        if (it == parNoToVal.end())
            parNoToVal.insert(std::make_pair(pn, sign*table[i]));
        else
            assert(it->second == sign*table[i]);
    }
    if (!uci)
        return;
    params.resize(maxParIdx+1);
    for (const auto& p : parNoToVal) {
        std::string pName = name + num2Str(p.first);
        params[p.first] = std::make_shared<Parameters::SpinParam>(pName, minValue, maxValue, p.second);
        pars.addPar(params[p.first]);
        params[p.first]->addListener([=]() { modifiedN(table, parNo, N); }, false);
    }
    modifiedN(table, parNo, N);
}

void
ParamTableBase::modifiedN(int* table, int* parNo, int N) {
    for (int i = 0; i < N; i++)
        if (parNo[i] > 0)
            table[i] = params[parNo[i]]->getIntPar();
        else if (parNo[i] < 0)
            table[i] = -params[-parNo[i]]->getIntPar();
    notify();
}
