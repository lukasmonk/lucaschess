/*
    Texel - A UCI chess engine.
    Copyright (C) 2012-2016  Peter Österlund, peterosterlund2@gmail.com

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
    std::shared_ptr<Parameters::StringParam> bookFile(std::make_shared<Parameters::StringParam>("BookFile", ""));
    std::shared_ptr<Parameters::CheckParam> ponder(std::make_shared<Parameters::CheckParam>("Ponder", true));
    std::shared_ptr<Parameters::CheckParam> analyseMode(std::make_shared<Parameters::CheckParam>("UCI_AnalyseMode", false));
    std::shared_ptr<Parameters::StringParam> opponent(std::make_shared<Parameters::StringParam>("UCI_Opponent", ""));
    std::shared_ptr<Parameters::SpinParam> strength(std::make_shared<Parameters::SpinParam>("Strength", 0, 1000, 1000));
#ifdef CLUSTER
    int maxThreads = 64*1024*1024;
#else
    int maxThreads = 512;
#endif
    std::shared_ptr<Parameters::SpinParam> threads(std::make_shared<Parameters::SpinParam>("Threads", 1, maxThreads, 1));
    std::shared_ptr<Parameters::SpinParam> multiPV(std::make_shared<Parameters::SpinParam>("MultiPV", 1, 256, 1));

    std::shared_ptr<Parameters::CheckParam> useNullMove(std::make_shared<Parameters::CheckParam>("UseNullMove", true));

    std::shared_ptr<Parameters::StringParam> gtbPath(std::make_shared<Parameters::StringParam>("GaviotaTbPath", ""));
    std::shared_ptr<Parameters::SpinParam> gtbCache(std::make_shared<Parameters::SpinParam>("GaviotaTbCache", 1, 2047, 1));
    std::shared_ptr<Parameters::StringParam> rtbPath(std::make_shared<Parameters::StringParam>("SyzygyPath", ""));
    std::shared_ptr<Parameters::SpinParam> minProbeDepth(std::make_shared<Parameters::SpinParam>("MinProbeDepth", 0, 100, 1));

    std::shared_ptr<Parameters::CheckParam> analysisAgeHash(std::make_shared<Parameters::CheckParam>("AnalysisAgeHash", true));
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
DEFINE_PARAM(activePawnPenalty);

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
DEFINE_PARAM(pawnTradeThreshold);

DEFINE_PARAM(threatBonus1);
DEFINE_PARAM(threatBonus2);
DEFINE_PARAM(latentAttackBonus);

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
    {  60,  63,  85,  70,  51,  80,  68,-192,
       41,   6,  18,  57,  52,  32,  24,  45,
      -28, -20,  -8,   0,  43,  66,  18, -22,
      -44, -19, -27, -27, -32,  -9, -26, -67,
      -37,  -1, -19, -37, -26, -39, -30, -85,
      -31,  20,  -9, -26, -24,  -4,   9, -21,
       36,  31,  19,  -5,   2,  -3,  41,  33,
      -11,  39,  22, -19,  14, -15,  33,  10 },
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
    {  -8,  64,  90,  93,  93,  90,  64,  -8,
       54, 105, 113, 109, 109, 113, 105,  54,
       88, 113, 115, 113, 113, 115, 113,  88,
       77, 104, 111, 113, 113, 111, 104,  77,
       55,  82,  95, 101, 101,  95,  82,  55,
       48,  63,  77,  86,  86,  77,  63,  48,
       31,  52,  65,  65,  65,  65,  52,  31,
        0,  22,  36,  21,  21,  36,  22,   0 },
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
      150, 126, 111,  92,  86,  57, -37, 125,
       12,  13,  19,  28,  28,  56,  22,  21,
       -7,  -8, -17,  -5,   2,  -4,   4,  -5,
      -11, -10, -11,  -9,  -2,  13,  -4, -19,
      -19, -17, -27, -19, -10,  -8,   1, -14,
      -13, -20, -27, -20, -19,   7,   9,  -5,
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
        4,  16,  17,   7,   7,  17,  16,   4,
       41,  40,  26,   8,   8,  26,  40,  41,
       25,  26,  11,   7,   7,  11,  26,  25,
       16,  26,  16,  17,  17,  16,  26,  16,
        8,  19,  17,  27,  27,  17,  19,   8,
        8,  22,  28,  39,  39,  28,  22,   8,
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
    {-240,-110, -79,   1,   9, -92, -75,-254,
      -43, -43,  14,  59,  14,  61, -17,   3,
      -51,  -6,  21,  39,  77, 105,  32, -24,
      -27,  -2,  15,  41,  15,  53,  14,  15,
      -23,   5,  12,   4,  16,  19,  26, -13,
      -57, -19,  -9,   6,   7, -14, -16, -52,
      -63, -48, -23,  -5, -12, -25, -48, -42,
     -103, -46, -67, -49, -42, -50, -44, -79 },
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
    { -52,  14,  20,  16,  16,  20,  14, -52,
       -1,  14,  29,  46,  46,  29,  14,  -1,
        5,  34,  49,  50,  50,  49,  34,   5,
       13,  41,  54,  58,  58,  54,  41,  13,
        8,  42,  53,  57,  57,  53,  42,   8,
      -17,  21,  31,  49,  49,  31,  21, -17,
      -15,   0,  17,  19,  19,  17,   0, -15,
      -57, -31, -12,  -1,  -1, -12, -31, -57 },
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
    { -45, -46, -46, -62, -58,-109,   8, -83,
      -37, -26, -20, -21, -31, -12, -71, -50,
      -15,  -7,  -8,  10,  23,  72,  36,   7,
      -30,  -9,  -9,  32,   3,   7, -15, -18,
      -17, -19, -11,   6,  14, -18, -15,   4,
      -25,  -2,  -7, -10, -14,  -9, -13, -12,
        1,  -8,  -3, -15,  -9,  -4,  11, -20,
      -31,  -1, -23, -31, -21, -23, -33, -33 },
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
    {  23,  23,  24,  28,  28,  24,  23,  23,
       23,  23,  36,  37,  37,  36,  23,  23,
       24,  36,  41,  48,  48,  41,  36,  24,
       28,  37,  48,  53,  53,  48,  37,  28,
       28,  37,  48,  53,  53,  48,  37,  28,
       24,  36,  41,  48,  48,  41,  36,  24,
       23,  23,  36,  37,  37,  36,  23,  23,
       23,  23,  24,  28,  28,  24,  23,  23 },
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
    { -74, -26, -17, -39, -41,  -2,  48,  20,
      -59, -82, -45, -35,-115, -40, -76,  42,
      -43, -39, -71, -53, -46, -20, -39, -55,
      -37, -28, -32, -47, -52, -55, -49, -39,
      -25, -25, -13, -29, -26, -30,  -3, -36,
      -29, -13, -13, -19, -18, -22,  -9, -33,
      -27, -17, -12,  -3, -15,  -7, -19, -47,
      -23, -18,  -9,  -5, -16, -48, -78, -61 },
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
    { -17, -18, -21, -15, -15, -21, -18, -17,
      -18, -17, -11,  -8,  -8, -11, -17, -18,
      -21, -11,  -1,   1,   1,  -1, -11, -21,
      -15,  -8,   1,  12,  12,   1,  -8, -15,
      -15,  -8,   1,  12,  12,   1,  -8, -15,
      -21, -11,  -1,   1,   1,  -1, -11, -21,
      -18, -17, -11,  -8,  -8, -11, -17, -18,
      -17, -18, -21, -15, -15, -21, -18, -17 },
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

/** Piece/square table for rooks during middle game. */
ParamTable<64> rt1b { -200, 200, useUciParam,
    {  35,  42,  45,  39,  40,  61,  66,  60,
       39,  36,  51,  54,  45,  59,  61,  49,
       28,  36,  42,  39,  53,  69,  67,  47,
       12,  18,  27,  30,  18,  45,  39,  21,
      -11,  -1,   9,   6,   6,   9,  21,  -7,
      -21, -13,  -7,  -3, -12,  -6,   3, -18,
      -24, -16,  -3,   0,  -6,  -3,  -9, -42,
        0,   0,   7,   9,   5,   9,   1,   4 },
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
        0,  26,  41,  43,  43,  41,  26,   0,
        0,  23,  39,  37,  37,  39,  23,   0,
        0,   0,  29,  34,  34,  29,   0,   0,
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
      127, 123,  95, 124, 124,  95, 123, 127,
       30,  41,  54,  68,  68,  54,  41,  30,
        5,  10,  19,  20,  20,  19,  10,   5,
        7,  10,   9,  12,  12,   9,  10,   7,
       14,  13,  22,  16,  16,  22,  13,  14,
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
      -20, -43,  22,  -2,  -2,  22, -43, -20,
        7,  -1,  14,  19,  19,  14,  -1,   7,
      -12,  -8,  -8,   7,   7,  -8,  -8, -12,
      -36,   0, -18,  10,  10, -18,   0, -36,
      -83, -29, -63, -49, -49, -63, -29, -83,
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
    {  0, 11, 10,  1 },
    {  1,  2,  3,  4 }
};

ParamTable<15> rookMobScore { -50, 50, useUciParam,
    {-23,-12, -8, -1, -1,  5,  9, 12, 16, 20, 24, 26, 26, 23, 28 },
    {   1, 2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15 }
};
ParamTable<14> bishMobScore = { -50, 50, useUciParam,
    {-18,-11, -1,  6, 13, 19, 24, 28, 31, 35, 36, 39, 41, 39 },
    {  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14 }
};
ParamTable<28> knightMobScore { -200, 200, useUciParam,
    {-31,-48, 25,-40,-14, -2,  5,-56,-24, -4,  9, 15,-26,-26,-19,-11, -2,  4,  6,-24,-24,-18,-12, -4,  1,  5,  9,  6 },
    {  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 13, 14, 15, 16, 17, 18, 19, 19, 20, 21, 22, 23, 24, 25, 26 }
};
ParamTable<28> queenMobScore { -100, 100, useUciParam,
    {  7,  3, -2, -3, -1,  1,  3,  5,  8, 10, 13, 16, 20, 23, 26, 31, 35, 38, 42, 46, 49, 48, 50, 48, 47, 40, 54, 37 },
    {  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28 }
};

ParamTable<36> connectedPPBonus { -200, 400, useUciParam,
    {  -2,  -6,   3,   7,   9,  -5,
       -6,  -4,  -1,   9,  14,   8,
        3,  -1,  16,   9,  23,  21,
        7,   9,   9,  37,   0,  46,
        9,  14,  23,   0, 113, -27,
       -5,   8,  21,  46, -27, 362 },
    {   1,   2,   4,   7,  11,  16,
        2,   3,   5,   8,  12,  17,
        4,   5,   6,   9,  13,  18,
        7,   8,   9,  10,  14,  19,
       11,  12,  13,  14,  15,  20,
       16,  17,  18,  19,  20,  21 }
};

ParamTable<8> passedPawnBonusX { -200, 200, useUciParam,
    {  0,  4, -2, -5, -5, -2,  4,  0 },
    {  0,  1,  2,  3,  3,  2,  1,  0 }
};

ParamTable<8> passedPawnBonusY { -200, 200, useUciParam,
    {  0,  3,  4, 14, 35, 64,103,  0 },
    {  0,  1,  2,  3,  4,  5,  6,  0 }
};

ParamTable<10> ppBlockerBonus { -50, 50, useUciParam,
    { 25, 29, 13, -9, 50,  1,  4,  3, -1,  9 },
    {  1,  2,  3,  4,  5,  6,  7,  8,  9, 10 }
};

ParamTable<8> candidatePassedBonus { -200, 200, useUciParam,
    { -1,  0,  5, 14, 42, 30, -1, -1 },
    {  0,  1,  2,  3,  4,  5,  0,  0 }
};

ParamTable<16> majorPieceRedundancy { -200, 200, useUciParam,
    {   0, -88,   0,   0,
       88,   0,   0,   0,
        0,   0,   0,  96,
        0,   0, -96,   0 },
    {   0,  -1,   0,   0,
        1,   0,   0,   0,
        0,   0,   0,   2,
        0,   0,  -2,   0 }
};

ParamTable<5> QvsRRBonus { -200, 200, useUciParam,
    {-21, -3, 23, 32, 38 },
    {  1,  2,  3,  4,  5 }
};

ParamTable<7> RvsMBonus { -200, 200, useUciParam,
    { 27, 47, 57, 52, 45,-12,-64 },
    {  1,  2,  3,  4,  5,  6,  7 }
};

ParamTable<7> RvsMMBonus { -200, 200, useUciParam,
    {-73,-73, -4, 19, 19, 33, 52 },
    {   1,   1,  2,  3,  4,  5,  6 }
};

ParamTable<4> bishopPairValue { 0, 200, useUciParam,
    { 71, 70, 60, 54 },
    {  1,  2,  3,  4 }
};

ParamTable<7> rookEGDrawFactor { 0, 255, useUciParam,
    { 71, 75,110,139,136,154,158 },
    {  1,  2,  3,  4,  5,  6,  7 }
};

ParamTable<7> RvsBPDrawFactor { 0, 255, useUciParam,
    {128, 90, 93,133,118,229,189 },
    {  0,  1,  2,  3,  4,  5,  6 }
};
ParamTable<4> castleFactor { 0, 128, useUciParam,
    { 64, 43, 27, 11 },
    {  1,  2,  3,  4 }
};

ParamTable<9> pawnShelterTable { -100, 100, useUciParam,
    { 17, 31,-16,  3, 19,  4, -1, 14, 10 },
    {  1,  2,  3,  4,  5,  6,  7,  8,  9 }
};

ParamTable<9> pawnStormTable { -400, 100, useUciParam,
    {-105,-47,-262, 43, 55,  9, 13,-15,-13 },
    {  1,   2,   3,  4,  5,  6,  7,  8,  9 }
};

ParamTable<14> kingAttackWeight { 0, 400, useUciParam,
    {  0,  3,  0,  6,  6, 13, 25, 47, 62, 97,104,147,207,321 },
    {  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13 }
};

ParamTable<5> qContactCheckBonus { -1000, 1000, useUciParam,
    {-512,-188,  0,188,512 },
    {  -2,  -1,  0,   1,   2 }
};

ParamTable<7> pieceKingAttackBonus { -1000, 1000, useUciParam,
    {-51,-26,-10,  0, 10, 26, 51 },
    { -3, -2, -1,  0,  1,  2,  3 }
};

ParamTable<5> kingPPSupportK { 0, 200, useUciParam,
    { 41, 71, 66, 60, 97 },
    {  1,  2,  3,  4,  5 }
};

ParamTable<8> kingPPSupportP { 1, 64, useUciParam,
    {  0,  3,  4,  9, 15, 21, 32,  0 },
    {  0,  1,  2,  3,  4,  5,  0,  0 }
};

ParamTable<8> pawnDoubledPenalty { 0, 50, useUciParam,
    { 41, 22, 20, 15, 15, 20, 22, 41 },
    {  1,  2,  3,  4,  4,  3,  2,  1 }
};

ParamTable<8> pawnIsolatedPenalty { 0, 50, useUciParam,
    {  1, 11,  6, 11, 11,  6, 11,  1 },
    {  1,  2,  3,  4,  4,  3,  2,  1 }
};

ParamTable<10> halfMoveFactor { 0, 192, useUciParam,
    {128,128,128,128, 31, 19, 12,  9,  5,  3 },
    {  0,  0,  0,  0,  1,  2,  3,  4,  5,  6 }
};

ParamTable<9> stalePawnFactor { 0, 192, useUciParam,
    {114,124,129,129,132,126,106, 73, 41 },
    {  1,  2,  3,  4,  5,  6,  7,  8,  9 }
};

Parameters::Parameters() {
    std::string about = ComputerPlayer::engineName +
                        " by Peter Osterlund, see http://hem.bredband.net/petero2b/javachess/index.html#texel";
    addPar(std::make_shared<StringParam>("UCI_EngineAbout", about));

    addPar(UciParams::hash);
    addPar(UciParams::ownBook);
    addPar(UciParams::bookFile);
    addPar(UciParams::ponder);
    addPar(UciParams::analyseMode);
    addPar(UciParams::opponent);
    addPar(UciParams::strength);
    addPar(UciParams::threads);
    addPar(UciParams::multiPV);

    addPar(UciParams::useNullMove);

    addPar(UciParams::gtbPath);
    addPar(UciParams::gtbCache);
    addPar(UciParams::rtbPath);
    addPar(UciParams::minProbeDepth);
    addPar(UciParams::analysisAgeHash);
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
    REGISTER_PARAM(activePawnPenalty, "ActivePawnPenalty");

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
    REGISTER_PARAM(pawnTradeThreshold, "PawnTradeThreshold");

    REGISTER_PARAM(threatBonus1, "ThreatBonus1");
    REGISTER_PARAM(threatBonus2, "ThreatBonus2");
    REGISTER_PARAM(latentAttackBonus, "LatentAttackBonus");

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
    qContactCheckBonus.registerParams("QueenContactCheckBonus", *this);
    pieceKingAttackBonus.registerParams("PieceKingAttackBonus", *this);
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
