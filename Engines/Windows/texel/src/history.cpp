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
 * history.cpp
 *
 *  Created on: Feb 25, 2012
 *      Author: petero
 */

#include "history.hpp"

const int History::maxSum;
const int History::maxVal;

int History::depthTable[] = {
    0, 1, 6, 19, 42, 56
};

void
History::init() {
    for (int p = 0; p < Piece::nPieceTypes; p++) {
        for (int sq = 0; sq < 64; sq++) {
            ht[p][sq].nValues = 0;
            ht[p][sq].scaledScore = 0;
        }
    }
}

void
History::reScale() {
    for (int p = 0; p < Piece::nPieceTypes; p++)
        for (int sq = 0; sq < 64; sq++)
            ht[p][sq].nValues >>= 2;
}

void
History::print() const {
    for (int p = 1; p <= 12; p += 3) {
        for (int row = 7; row >= 0; row--) {
            std::cout << "hist:";
            for (int d = 0; d < 3; d++) {
                int piece = p + d;
                if (d > 0)
                    std::cout << "  ";
                for (int col = 0; col < 8; col++) {
                    int sq = Position::getSquare(col, row);
                    int hist = ht[piece][sq].scaledScore >> log2Scale;
                    std::cout << ' ' << std::setw(2) << hist;
                }
            }
            std::cout << '\n';
        }
        std::cout << "hist:\n";
    }

    for (int p = 1; p <= 12; p += 3) {
        for (int row = 7; row >= 0; row--) {
            std::cout << "hist:";
            for (int d = 0; d < 3; d++) {
                int piece = p + d;
                if (d > 0)
                    std::cout << "  ";
                for (int col = 0; col < 8; col++) {
                    int sq = Position::getSquare(col, row);
                    int s = ht[piece][sq].nValues * 100 / (maxSum + 1);
                    std::cout << ' ' << std::setw(2) << s;
                }
            }
            std::cout << '\n';
        }
        std::cout << "hist:\n";
    }

}
