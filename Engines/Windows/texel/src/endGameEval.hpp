/*
    Texel - A UCI chess engine.
    Copyright (C) 2012-2015  Peter Österlund, peterosterlund2@gmail.com

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
 * endGameEval.hpp
 *
 *  Created on: Dec 26, 2014
 *      Author: petero
 */

#ifndef ENDGAMEEVAL_HPP_
#define ENDGAMEEVAL_HPP_

#include "util/util.hpp"

class Position;

class EndGameEval {
public:
    /** Implements special knowledge for some endgame situations.
     * If doEval is false the position is not evaluated. Instead 1 is returned if
     * this function has special knowledge about the current material balance, and 0
     * is returned otherwise. */
    template <bool doEval> static int endGameEval(const Position& pos,
                                                  U64 passedPawns,
                                                  int oldScore);

    /** King evaluation when no pawns left. */
    static int mateEval(int k1, int k2);

    static const int winKingTable[64];

private:
    /** Return true if the side with the bishop can not win because the opponent
     * has a fortress draw. */
    template <bool whiteBishop> static bool isBishopPawnDraw(const Position& pos);

    static int kqkpEval(int wKing, int wQueen, int bKing, int bPawn, bool whiteMove, int score);
    static int kqkrpEval(int wKing, int wQueen, int bKing, int bRook, int bPawn, bool whiteMove, int score);
    static bool kqkrmFortress(bool bishop, int wQueen, int bRook, int bMinor, U64 wPawns, U64 bPawns);

    static int kpkEval(int wKing, int bKing, int wPawn, bool whiteMove);
    static bool kpkpEval(int wKing, int bKing, int wPawn, int bPawn, int& score);

    static int krkpEval(int wKing, int bKing, int bPawn, bool whiteMove, int score);
    static int krpkrEval(int wKing, int bKing, int wPawn, int wRook, int bRook, bool whiteMove);
    static int krpkrpEval(int wKing, int bKing, int wPawn, int wRook, int bRook, int bPawn, bool whiteMove, int score);

    static int kbnkEval(int wKing, int bKing, bool darkBishop);

    static int kbpkbEval(int wKing, int wBish, int wPawn, int bKing, int bBish, int score);
    static int kbpknEval(int wKing, int wBish, int wPawn, int bKing, int bKnight, int score);
    static int knpkbEval(int wKing, int wKnight, int wPawn, int bKing, int bBish, int score, bool wtm);
    static int knpkEval(int wKing, int wKnight, int wPawn, int bKing, int score, bool wtm);

    static const int distToH1A8[8][8];
    static const U8 kpkTable[2*32*64*48/8];
    static const U8 krkpTable[2*32*48*8];
    static const U64 krpkrTable[2*24*64];
};

#endif
