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
 * transpositionTable.cpp
 *
 *  Created on: Feb 25, 2012
 *      Author: petero
 */

#include "transpositionTable.hpp"
#include "position.hpp"
#include "moveGen.hpp"
#include "textio.hpp"
#include "largePageAlloc.hpp"

#include <iostream>
#include <iomanip>

using namespace std;


TranspositionTable::TranspositionTable(int log2Size)
    : table(nullptr), tableSize(0), ttStorage(*this) {
    reSize(log2Size);
}

void
TranspositionTable::reSize(int log2Size) {
    const size_t numEntries = ((size_t)1) << log2Size;

    tableV.clear();
    tableLP.reset();
    table = nullptr;
    tableSize = 0;

    tableLP = LargePageAlloc::allocate<TTEntryStorage>(numEntries);
    if (tableLP) {
        table = tableLP.get();
    } else {
        tableV.resize(numEntries);
        table = &tableV[0];
    }
    tableSize = numEntries;

    generation = 0;
    setHashMask(tableSize);
    tbGen.reset();
    notUsedCnt = 0;
}

void
TranspositionTable::clear() {
    setHashMask(tableSize);
    tbGen.reset();
    notUsedCnt = 0;
    TTEntry ent;
    ent.clear();
    for (size_t i = 0; i < tableSize; i++)
        ent.store(table[i]);
}

void
TranspositionTable::insert(U64 key, const Move& sm, int type, int ply, int depth, int evalScore,
                           bool busy) {
    if (depth < 0) depth = 0;
    size_t idx0 = getIndex(key);
    U64 key2 = getStoredKey(key);
    TTEntry ent, tmp;
    ent.clear();
    size_t idx = idx0;
    for (int i = 0; i < 4; i++) {
        size_t idx1 = idx0 + i;
        tmp.load(table[idx1]);
        if (tmp.getKey() == key2) {
            ent = tmp;
            idx = idx1;
            break;
        } else if (i == 0) {
            ent = tmp;
            idx = idx1;
        } else if (ent.betterThan(tmp, generation)) {
            ent = tmp;
            idx = idx1;
        }
    }
    bool doStore = true;
    if (!busy) {
        if ((ent.getKey() == key2) && (ent.getDepth() > depth) && (ent.getType() == type)) {
            if (type == TType::T_EXACT)
                doStore = false;
            else if ((type == TType::T_GE) && (sm.score() <= ent.getScore(ply)))
                doStore = false;
            else if ((type == TType::T_LE) && (sm.score() >= ent.getScore(ply)))
                doStore = false;
        }
    }
    if (doStore) {
        if ((ent.getKey() != key2) || (sm.from() != sm.to()))
            ent.setMove(sm);
        ent.setKey(key2);
        ent.setScore(sm.score(), ply);
        ent.setDepth(depth);
        ent.setBusy(busy);
        ent.setGeneration((S8)generation);
        ent.setType(type);
        ent.setEvalScore(evalScore);
        ent.store(table[idx]);
    }
}

void
TranspositionTable::setBusy(const TTEntry& ent, int ply) {
    U64 key = ent.getKey();
    int type = ent.getType();
    int depth = ent.getDepth();
    int evalScore = ent.getEvalScore();
    Move sm;
    ent.getMove(sm);
    sm.setScore(ent.getScore(ply));
    insert(key, sm, type, ply, depth, evalScore, true);
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
TranspositionTable::printStats(int rootDepth) const {
    int unused = 0;
    int thisGen = 0;
    std::vector<int> depHist;
    for (size_t i = 0; i < tableSize; i++) {
        TTEntry ent;
        ent.load(table[i]);
        if (ent.getType() == TType::T_EMPTY) {
            unused++;
        } else {
            if (ent.getGeneration() == generation)
                thisGen++;
            int d = ent.getDepth();
            while ((int)depHist.size() <= d)
                depHist.push_back(0);
            depHist[d]++;
        }
    }
    double w = 100.0 / tableSize;
    std::stringstream ss;
    ss.precision(2);
    ss << std::fixed << "hstat: d:" << rootDepth << " size:" << tableSize
       << " unused:" << unused << " (" << (unused*w) << "%)"
       << " thisGen:" << thisGen << " (" << (thisGen*w) << "%)" << std::endl;
    cout << ss.str();
    for (size_t i = 0; i < depHist.size(); i++) {
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

int
TranspositionTable::getHashFull() const {
    if (tableSize < 1000)
        return 0;
    int hashFull = 0;
    for (int i = 0; i < 1000; i++) {
        TTEntry ent;
        ent.load(table[i]);
        if ((ent.getType() != TType::T_EMPTY) &&
            (ent.getGeneration() == generation))
            hashFull++;
    }
    return hashFull;
}

// --------------------------------------------------------------------------------

bool
TranspositionTable::updateTB(const Position& pos, RelaxedShared<S64>& maxTimeMillis) {
    if (BitBoard::bitCount(pos.occupiedBB()) > 4 ||
        pos.pieceTypeBB(Piece::WPAWN, Piece::BPAWN)) { // pos not suitable for TB generation
        if (tbGen && notUsedCnt++ > 3) {
            tbGen.reset();
            setHashMask(tableSize);
            notUsedCnt = 0;
        }
        return tbGen != nullptr;
    }

    int score;
    if (tbGen && tbGen->probeDTM(pos, 0, score)) {
        notUsedCnt = 0;
        return true; // pos already in TB
    }

    static S64 requiredTime = 3000;
    if (maxTimeMillis >= 0 && maxTimeMillis < requiredTime)
        return false; // Not enough time to generate TB

    U64 ttSize = tableSize * sizeof(TTEntryStorage);
    if (ttSize < 8 * 1024 * 1024)
        return false; // Need at least 5MB for TB storage

    PieceCount pc;
    pc.nwq = BitBoard::bitCount(pos.pieceTypeBB(Piece::WQUEEN));
    pc.nwr = BitBoard::bitCount(pos.pieceTypeBB(Piece::WROOK));
    pc.nwb = BitBoard::bitCount(pos.pieceTypeBB(Piece::WBISHOP));
    pc.nwn = BitBoard::bitCount(pos.pieceTypeBB(Piece::WKNIGHT));
    pc.nbq = BitBoard::bitCount(pos.pieceTypeBB(Piece::BQUEEN));
    pc.nbr = BitBoard::bitCount(pos.pieceTypeBB(Piece::BROOK));
    pc.nbb = BitBoard::bitCount(pos.pieceTypeBB(Piece::BBISHOP));
    pc.nbn = BitBoard::bitCount(pos.pieceTypeBB(Piece::BKNIGHT));

    tbGen = make_unique<TBGenerator<TTStorage>>(ttStorage, pc);
    if (!tbGen->generate(maxTimeMillis, false)) {
        // Increase requiredTime unless computation was aborted
        S64 maxT = maxTimeMillis;
        if (maxT != 0)
            requiredTime = std::max(maxT, requiredTime) * 2;
        return false;
    }
    int shift = (ttSize < 16 * 1024 * 1024) ? 2 : 1;
    setHashMask(tableSize >> shift);
    hashMask = (tableSize - 1) >> shift;
    notUsedCnt = 0;
    return true;
}

bool
TranspositionTable::probeDTM(const Position& pos, int ply, int& score) const {
    return tbGen && tbGen->probeDTM(pos, ply, score);
}
