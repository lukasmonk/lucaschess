/*
    Cinnamon is a UCI chess engine
    Copyright (C) 2011-2014 Giuseppe Cannella

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

#ifndef EVAL_H_
#define EVAL_H_
#include "GenMoves.h"
#include <fstream>
#include <string.h>
using namespace _board;
using namespace _eval;

class Eval : public GenMoves {
public:
    Eval();
    virtual ~Eval();
    int getScore (const int side, const int alpha = -_INFINITE, const int beta = _INFINITE );

    template <int side> int lazyEval() {
        return lazyEvalSide<side>() - lazyEvalSide<side^1>();
    }

protected:
    STATIC_CONST int FUTIL_MARGIN = 154;
    STATIC_CONST int EXT_FUTILY_MARGIN = 392;
    STATIC_CONST int RAZOR_MARGIN = 1071;
    STATIC_CONST int ATTACK_KING = 30;
    STATIC_CONST int BISHOP_ON_QUEEN = 2;
    STATIC_CONST int BACKWARD_PAWN = 2;
    STATIC_CONST int NO_PAWNS = 15;
    STATIC_CONST int DOUBLED_ISOLATED_PAWNS = 14;
    STATIC_CONST int DOUBLED_PAWNS = 5;
    STATIC_CONST int ENEMIES_PAWNS_ALL = 8;
    STATIC_CONST int PAWN_7H = 32;
    STATIC_CONST int PAWN_CENTER = 15;
    STATIC_CONST int PAWN_IN_RACE = 114;
    STATIC_CONST int PAWN_ISOLATED = 3;
    STATIC_CONST int PAWN_NEAR_KING = 2;
    STATIC_CONST int PAWN_BLOCKED = 5;
    STATIC_CONST int UNPROTECTED_PAWNS = 5;
    STATIC_CONST int ENEMY_NEAR_KING = 2;
    STATIC_CONST int FRIEND_NEAR_KING = 1;
    STATIC_CONST int BISHOP_NEAR_KING = 10;
    STATIC_CONST int HALF_OPEN_FILE_Q = 3;
    STATIC_CONST int KNIGHT_TRAPPED = 5;
    STATIC_CONST int END_OPENING = 6;
    STATIC_CONST int BONUS2BISHOP = 18;
    STATIC_CONST int CONNECTED_ROOKS = 7;
    STATIC_CONST int OPEN_FILE = 10;
    STATIC_CONST int OPEN_FILE_Q = 3;
    STATIC_CONST int ROOK_7TH_RANK = 10;
    STATIC_CONST int ROOK_BLOCKED = 13;
    STATIC_CONST int ROOK_TRAPPED = 6;
    STATIC_CONST int UNDEVELOPED = 4;
    STATIC_CONST int UNDEVELOPED_BISHOP = 4;
#ifdef DEBUG_MODE
    int LazyEvalCuts;
    typedef struct {
        int BAD_BISHOP[2];
        int MOB_BISHOP[2];
        int UNDEVELOPED_BISHOP[2];
        int OPEN_DIAG_BISHOP[2];
        int BONUS2BISHOP[2];

        int MOB_PAWNS[2];
        int ATTACK_KING_PAWN[2];
        int PAWN_CENTER[2];
        int PAWN_7H[2];
        int PAWN_IN_RACE[2];
        int PAWN_BLOCKED[2];
        int UNPROTECTED_PAWNS[2];
        int PAWN_ISOLATED[2];
        int DOUBLED_PAWNS[2];
        int DOUBLED_ISOLATED_PAWNS[2];
        int BACKWARD_PAWN[2];
        int FORK_SCORE[2];
        int PAWN_PASSED[2];
        int ENEMIES_PAWNS_ALL[2];
        int NO_PAWNS[2];

        int KING_SECURITY_BISHOP[2];
        int KING_SECURITY_QUEEN[2];
        int KING_SECURITY_KNIGHT[2];
        int KING_SECURITY_ROOK[2];
        int DISTANCE_KING[2];
        int END_OPENING_KING[2];
        int PAWN_NEAR_KING[2];
        int MOB_KING[2];

        int MOB_QUEEN[2];
        int OPEN_FILE_Q[2];
        int BISHOP_ON_QUEEN[2];
        int HALF_OPEN_FILE_Q[2];

        int UNDEVELOPED_KNIGHT[2];
        int KNIGHT_TRAPPED[2];
        int MOB_KNIGHT[2];


        int ROOK_7TH_RANK[2];
        int ROOK_TRAPPED[2];
        int MOB_ROOK[2];
        int ROOK_BLOCKED[2];
        int ROOK_OPEN_FILE[2];
        int CONNECTED_ROOKS[2];
    } _TSCORE_DEBUG;
    _TSCORE_DEBUG SCORE_DEBUG;
#endif

private:


#ifdef DEBUG_MODE
    int evaluationCount[2];
#endif
    void openColumn ( int side );
    template <int side, _Tstatus status> int evaluatePawn();
    template <int side, _Tstatus status> int evaluateBishop ( const u64, u64 );
    template <_Tstatus status> int evaluateQueen ( int side, u64 enemies, u64 friends );
    template <int side, _Tstatus status> int evaluateKnight ( const u64, const u64 );
    template <int side, _Tstatus status> int evaluateRook ( const u64, u64 enemies, u64 friends );
    template <_Tstatus status> int evaluateKing ( int side, u64 squares );

    template <int side> int lazyEvalSide() {
        return bitCount ( chessboard[PAWN_BLACK + side] ) * VALUEPAWN + bitCount ( chessboard[ROOK_BLACK + side] ) * VALUEROOK + bitCount ( chessboard[BISHOP_BLACK + side] ) * VALUEBISHOP
               + bitCount ( chessboard[KNIGHT_BLACK + side] ) * VALUEKNIGHT + bitCount ( chessboard[QUEEN_BLACK + side] ) * VALUEQUEEN;
    }
};
#endif

