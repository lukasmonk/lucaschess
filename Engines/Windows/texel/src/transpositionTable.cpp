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
 * transpositionTable.cpp
 *
 *  Created on: Feb 25, 2012
 *      Author: petero
 */

#include "transpositionTable.hpp"
#include "position.hpp"
#include "moveGen.hpp"
#include "textio.hpp"

#include <iostream>
#include <iomanip>

using namespace std;


void
TranspositionTable::reSize(int log2Size) {
    const size_t numEntries = ((size_t)1) << log2Size;
    table.resize(numEntries);
    generation = 0;
}

void
TranspositionTable::insert(U64 key, const Move& sm, int type, int ply, int depth, int evalScore) {
    if (depth < 0) depth = 0;
    size_t idx0 = getIndex(key);
    U64 key2 = getStoredKey(key);
    TTEntry ent0, ent1;
    ent0.load(table[idx0]);
    size_t idx = idx0;
    TTEntry* ent = &ent0;
    if (ent0.getKey() != key2) {
        size_t idx1 = idx0 ^ 1;
        ent1.load(table[idx1]);
        idx = idx1;
        ent = &ent1;
        if (ent1.getKey() != key2)
            if (ent1.betterThan(ent0, generation)) {
                idx = idx0;
                ent = &ent0;
            }
    }
    bool doStore = true;
    if ((ent->getKey() == key2) && (ent->getDepth() > depth) && (ent->getType() == type)) {
        if (type == TType::T_EXACT)
            doStore = false;
        else if ((type == TType::T_GE) && (sm.score() <= ent->getScore(ply)))
            doStore = false;
        else if ((type == TType::T_LE) && (sm.score() >= ent->getScore(ply)))
            doStore = false;
    }
    if (doStore) {
        if ((ent->getKey() != key2) || (sm.from() != sm.to()))
            ent->setMove(sm);
        ent->setKey(key2);
        ent->setScore(sm.score(), ply);
        ent->setDepth(depth);
        ent->setGeneration((S8)generation);
        ent->setType(type);
        ent->setEvalScore(evalScore);
        ent->store(table[idx]);
    }
}

void
TranspositionTable::extractPVMoves(const Position& rootPos, const Move& mFirst, std::vector<Move>& pv) {
    Position pos(rootPos);
    Move m(mFirst);
    UndoInfo ui;
    std::vector<U64> hashHistory;
    while (true) {
        pv.push_back(m);
        pos.makeMove(m, ui);
        if (contains(hashHistory, pos.zobristHash()))
            break;
        hashHistory.push_back(pos.zobristHash());
        TTEntry ent;
        ent.clear();
        probe(pos.historyHash(), ent);
        if (ent.getType() == TType::T_EMPTY)
            break;
        ent.getMove(m);
        MoveList moves;
        MoveGen::pseudoLegalMoves(pos, moves);
        MoveGen::removeIllegal(pos, moves);
        bool contains = false;
        for (int mi = 0; mi < moves.size; mi++)
            if (moves[mi].equals(m)) {
                contains = true;
                break;
            }
        if  (!contains)
            break;
    }
}

/** Extract the PV starting from posIn, using hash entries, both exact scores and bounds. */
std::string
TranspositionTable::extractPV(const Position& posIn) {
    std::string ret;
    Position pos(posIn);
    bool first = true;
    TTEntry ent;
    ent.clear();
    probe(pos.historyHash(), ent);
    UndoInfo ui;
    std::vector<U64> hashHistory;
    bool repetition = false;
    while (ent.getType() != TType::T_EMPTY) {
        Move m;
        ent.getMove(m);
        MoveList moves;
        MoveGen::pseudoLegalMoves(pos, moves);
        MoveGen::removeIllegal(pos, moves);
        bool valid = false;
        for (int mi = 0; mi < moves.size; mi++)
            if (moves[mi].equals(m)) {
                valid = true;
                break;
            }
        if  (!valid)
            break;
        if (repetition)
            break;
        if (!first)
            ret += ' ';
        if (ent.getType() == TType::T_LE)
            ret += '<';
        else if (ent.getType() == TType::T_GE)
            ret += '>';
        std::string moveStr = TextIO::moveToString(pos, m, false);
        ret += moveStr;
        pos.makeMove(m, ui);
        if (contains(hashHistory, pos.zobristHash()))
            repetition = true;
        hashHistory.push_back(pos.zobristHash());
        probe(pos.historyHash(), ent);
        first = false;
    }
    return ret;
}

void
TranspositionTable::printStats() const {
    int unused = 0;
    int thisGen = 0;
    std::vector<int> depHist;
    const int maxDepth = 20*8;
    depHist.resize(maxDepth);
    for (size_t i = 0; i < table.size(); i++) {
        TTEntry ent;
        ent.load(table[i]);
        if (ent.getType() == TType::T_EMPTY) {
            unused++;
        } else {
            if (ent.getGeneration() == generation)
                thisGen++;
            if (ent.getDepth() < maxDepth)
                depHist[ent.getDepth()]++;
        }
    }
    double w = 100.0 / table.size();
    std::stringstream ss;
    ss.precision(2);
    ss << std::fixed << "hstat: size:" << table.size()
       << " unused:" << unused << " (" << (unused*w) << "%)"
       << " thisGen:" << thisGen << " (" << (thisGen*w) << "%)" << std::endl;
    cout << ss.str();
    for (int i = 0; i < maxDepth; i++) {
        int c = depHist[i];
        if (c > 0) {
            std::stringstream ss;
            ss.precision(2);
            ss << std::setw(4) << i
               << ' ' << std::setw(8) << c
               << " " << std::setw(6) << std::fixed << (c*w);
            std::cout << "hstat:" << ss.str() << std::endl;
        }
    }
}
