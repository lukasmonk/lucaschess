/*
 * Demolito, a UCI chess engine.
 * Copyright 2015 lucasart.
 *
 * Demolito is free software: you can redistribute it and/or modify it under the terms of the GNU
 * General Public License as published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 *
 * Demolito is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without
 * even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with this program. If
 * not, see <http://www.gnu.org/licenses/>.
*/
#include <algorithm>
#include "pst.h"

namespace pst {

eval_t table[NB_COLOR][NB_PIECE][NB_SQUARE];

const int Center[NB_FILE] = {-5,-2, 0, 2, 2, 0,-2,-5};

eval_t knight(Rank r, File f)
{
    const eval_t CenterWeight = {10, 3};
    return CenterWeight * (Center[r] + Center[f]);
}

eval_t bishop(Rank r, File f)
{
    const eval_t CenterWeight = {2, 3};
    const eval_t DiagonalWeight = {8, 0};
    const eval_t BackRankWeight = {-20, 0};

    return CenterWeight * (Center[r] + Center[f])
           + DiagonalWeight * (r + f == 7 || r - f == 0)
           + BackRankWeight * (r == RANK_1);
}

eval_t rook(Rank r, File f)
{
    const eval_t FileWeight = {3, 0};
    const eval_t SeventhWeight = {16, 16};

    return FileWeight * Center[f]
           + SeventhWeight * (r == RANK_7);
}

eval_t queen(Rank r, File f)
{
    const eval_t CenterWeight = {0, 4};
    const eval_t BackRankWeight = {-10, 0};

    return CenterWeight * (Center[r] + Center[f])
           + BackRankWeight * (r == RANK_1);
}

eval_t king(Rank r, File f)
{
    static const int FileWeight[NB_FILE] = {54, 84, 40, 0, 0, 40, 84, 54};
    static const int RankWeight[NB_RANK] = {28, 0,-28,-46,-58,-70,-70,-70};
    static const int CenterWeight = 14;

    return eval_t {
        FileWeight[f] + RankWeight[r],
        CenterWeight * (Center[r] + Center[f])
    };
}

eval_t pawn(Rank r, File f)
{
    const eval_t PCenter = {36, 0};
    eval_t e = {0, 0};

    if (f == FILE_D || f == FILE_E) {
        if (r == RANK_3 || r == RANK_5)
            e += PCenter / 2;
        else if (r == RANK_4)
            e += PCenter;
    }

    return e;
}

void init()
{
    typedef eval_t (*pst_fn)(Rank, File);
    const pst_fn PstFn[NB_PIECE] = {&knight, &bishop, &rook, &queen, &king, &pawn};

    // Calculate PST, based on specialized functions for each piece
    for (Color c = WHITE; c <= BLACK; ++c)
        for (Piece p = KNIGHT; p < NB_PIECE; ++p)
            for (Square s = A1; s <= H8; ++s) {
                const Rank rr = Rank(rank_of(s) ^ (RANK_8 * c));
                const File f = file_of(s);
                table[c][p][s] = (Material[p] + (*PstFn[p])(rr, f)) * (c == WHITE ? 1 : -1);
            }
}

}    // namespace pst
