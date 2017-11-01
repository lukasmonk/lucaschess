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
 * history.hpp
 *
 *  Created on: Feb 25, 2012
 *      Author: petero
 */

#ifndef HISTORY_HPP_
#define HISTORY_HPP_

#include "piece.hpp"
#include "position.hpp"

/**
 * Implements the relative history heuristic.
 */
class History {
public:
    History();

    /** Clear all history information. */
    void init();

    /** Rescale the history counters, so that future updates have more weight. */
    void reScale();

    /** Record move as a success. */
    void addSuccess(const Position& pos, const Move& m, int depth);

    /** Record move as a failure. */
    void addFail(const Position& pos, const Move& m, int depth);

    /** Get a score between 0 and 49, depending of the success/fail ratio of the move. */
    int getHistScore(const Position& pos, const Move& m) const;

    /** Print all history tables. */
    void print() const;

private:
    static int depthWeight(int depth);

    static int depthTable[6];

    static const int log2Scale = 10;
    static const int scale = 1 << log2Scale;
    static const int maxSum = 1000;          // max value of nSuccess + nFail
    static const int maxVal = 50;            // getHistScore returns < maxVal

    // Each entry has the following encoding:
    // Bits  0-15: nSuccess + nFail
    // Bits 16-31: histScore * scale
    RelaxedShared<U32> ht[Piece::nPieceTypes][64];
};


inline
History::History() {
    init();
}

inline int
History::depthWeight(int depth) {
    return depthTable[clamp(depth, 0, (int)COUNT_OF(depthTable)-1)];
}

inline void
History::addSuccess(const Position& pos, const Move& m, int depth) {
    int p = pos.getPiece(m.from());
    int cnt = depthWeight(depth);
    if (cnt == 0)
        return;
    U32 e = ht[p][m.to()];
    int fpHistVal = e >> 16;
    int sum = e & 0xffff;

    fpHistVal = (fpHistVal * sum + (maxVal * scale - 1) * cnt) / (sum + cnt);
    sum = std::min(sum + cnt, maxSum);
    ht[p][m.to()] = (fpHistVal << 16) + sum;
}

inline void
History::addFail(const Position& pos, const Move& m, int depth) {
    int p = pos.getPiece(m.from());
    int cnt = depthWeight(depth);
    if (cnt == 0)
        return;
    U32 e = ht[p][m.to()];
    int fpHistVal = e >> 16;
    int sum = e & 0xffff;

    fpHistVal = fpHistVal * sum / (sum + cnt);
    sum = std::min(sum + cnt, maxSum);
    ht[p][m.to()] = (fpHistVal << 16) + sum;
}

inline int
History::getHistScore(const Position& pos, const Move& m) const {
    int p = pos.getPiece(m.from());
    U32 e = ht[p][m.to()];
    return e >> (16 + log2Scale);
}

#endif /* HISTORY_HPP_ */
