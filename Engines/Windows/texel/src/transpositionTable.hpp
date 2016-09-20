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
 * transpositionTable.hpp
 *
 *  Created on: Feb 25, 2012
 *      Author: petero
 */

#ifndef TRANSPOSITIONTABLE_HPP_
#define TRANSPOSITIONTABLE_HPP_

#include "util/util.hpp"
#include "move.hpp"
#include "constants.hpp"
#include "util/alignedAlloc.hpp"

#include <vector>

class Position;

/**
 * Implements the main transposition table using Cuckoo hashing.
 */
class TranspositionTable {
public:
    /** In-memory representation of TT entry. Uses std::atomic for thread safety,
     * but accessed using memory_order_relaxed for maximum performance. */
    struct TTEntryStorage {
        std::atomic<U64> key;
        std::atomic<U64> data;
        TTEntryStorage();
        TTEntryStorage(const TTEntryStorage& a);
    };
    static_assert(sizeof(TTEntryStorage) == 16, "TTEntryStorage size wrong");

    /** A local copy of a transposition table entry. */
    class TTEntry {
    public:
        /** Set type to T_EMPTY. */
        void clear();

        /** Store in transposition table, encoded for thread safety. */
        void store(TTEntryStorage& ent);

        /** Load from transposition table, decode the thread safety encoding. */
        void load(const TTEntryStorage& ent);

        /** Return true if this object is more valuable than the other, false otherwise. */
        bool betterThan(const TTEntry& other, int currGen) const;

        U64 getKey() const;
        void setKey(U64 k);

        void getMove(Move& m) const;

        void setMove(const Move& m);

        /** Get the score from the hash entry and convert from "mate in x" to "mate at ply". */
        int getScore(int ply) const;

        /** Convert score from "mate at ply" to "mate in x" and store in hash entry. */
        void setScore(int score, int ply);

        /** Return true if entry is good enough to cut off this branch in the search tree. */
        bool isCutOff(int alpha, int beta, int ply, int depth) const;

        int getDepth() const;
        void setDepth(int d);
        int getGeneration() const;
        void setGeneration(int g);
        int getType() const;
        void setType(int t);
        int getEvalScore() const;
        void setEvalScore(int s);

    private:
        U64 key;        //  0 64 key         Zobrist hash key
        U64 data;       //  0 16 move        from + (to<<6) + (promote<<12)
                        // 16 16 score       Score from search
                        // 32 10 depth       Search depth
                        // 42  4 generation  Increase when OTB position changes
                        // 46  2 type        exact score, lower bound, upper bound
                        // 48 16 evalScore   Score from static evaluation

        void setBits(int first, int size, unsigned int value);
        unsigned int getBits(int first, int size) const;
    };

    /** Constructor. Creates an empty transposition table with numEntries slots. */
    TranspositionTable(int log2Size);

    void reSize(int log2Size);

    /** Insert an entry in the hash table. */
    void insert(U64 key, const Move& sm, int type, int ply, int depth, int evalScore);

    /** Retrieve an entry from the hash table corresponding to position with zobrist key "key". */
    void probe(U64 key, TTEntry& result);

    /** Prefetch cache line. */
    void prefetch(U64 key);

    /**
     * Increase hash table generation. This means that subsequent inserts will be considered
     * more valuable than the entries currently present in the hash table.
     */
    void nextGeneration();

    /** Clear the transposition table. */
    void clear();

    /** Extract a list of PV moves, starting from "rootPos" and first move "mFirst". */
    void extractPVMoves(const Position& rootPos, const Move& mFirst, std::vector<Move>& pv);

    /** Extract the PV starting from posIn, using hash entries, both exact scores and bounds. */
    std::string extractPV(const Position& posIn);

    /** Print hash table statistics. */
    void printStats() const;

private:
    /** Get position in hash table given zobrist key. */
    size_t getIndex(U64 key) const;

    /** Get part of zobrist key to store in hash table. */
    static U64 getStoredKey(U64 key);


    vector_aligned<TTEntryStorage> table;
    U8 generation;
};


inline
TranspositionTable::TTEntryStorage::TTEntryStorage() {
    key.store(0, std::memory_order_relaxed);
    data.store(0, std::memory_order_relaxed);
}

inline
TranspositionTable::TTEntryStorage::TTEntryStorage(const TTEntryStorage& a) {
    key.store(a.key.load(std::memory_order_relaxed), std::memory_order_relaxed);
    data.store(a.data.load(std::memory_order_relaxed), std::memory_order_relaxed);
}


inline void
TranspositionTable::TTEntry::clear() {
    key = 0;
    data = 0;
    static_assert(TType::T_EMPTY == 0, "type not set to T_EMPTY");
}

inline void
TranspositionTable::TTEntry::store(TTEntryStorage& ent) {
    ent.key.store(key ^ data, std::memory_order_relaxed);
    ent.data.store(data, std::memory_order_relaxed);
}

inline void
TranspositionTable::TTEntry::load(const TTEntryStorage& ent) {
    key = ent.key.load(std::memory_order_relaxed);
    data = ent.data.load(std::memory_order_relaxed);
    key ^= data;
}

inline bool
TranspositionTable::TTEntry::betterThan(const TTEntry& other, int currGen) const {
    if ((getGeneration() == currGen) != (other.getGeneration() == currGen))
        return getGeneration() == currGen;   // Old entries are less valuable
    if ((getType() == TType::T_EXACT) != (other.getType() == TType::T_EXACT))
        return getType() == TType::T_EXACT;         // Exact score more valuable than lower/upper bound
    if (getDepth() != other.getDepth())
        return getDepth() > other.getDepth();     // Larger depth is more valuable
    return false;   // Otherwise, pretty much equally valuable
}

inline U64
TranspositionTable::TTEntry::getKey() const {
    return key;
}

inline void
TranspositionTable::TTEntry::setKey(U64 k) {
    key = k;
}

inline void
TranspositionTable::TTEntry::getMove(Move& m) const {
    int move = getBits(0, 16);
    m.setMove(move & 63, (move >> 6) & 63, (move >> 12) & 15, m.score());
}

inline void
TranspositionTable::TTEntry::setMove(const Move& m) {
    int move = (short)(m.from() + (m.to() << 6) + (m.promoteTo() << 12));
    setBits(0, 16, move);
}

inline int
TranspositionTable::TTEntry::getScore(int ply) const {
    int sc = (S16)getBits(16, 16);
    if (SearchConst::isWinScore(sc))
        sc -= ply;
    else if (SearchConst::isLoseScore(sc))
        sc += ply;
    return sc;
}

inline void
TranspositionTable::TTEntry::setScore(int score, int ply) {
    if (SearchConst::isWinScore(score))
        score += ply;
    else if (SearchConst::isLoseScore(score))
        score -= ply;
    setBits(16, 16, score);
}

inline bool
TranspositionTable::TTEntry::isCutOff(int alpha, int beta, int ply, int depth) const {
    using namespace SearchConst;
    const int score = getScore(ply);
    const int plyToMate = MATE0 - std::abs(getScore(0));
    const int eDepth = getDepth();
    const int eType = getType();
    if ((eDepth >= depth) || (eDepth >= plyToMate*plyScale)) {
        if ( (eType == TType::T_EXACT) ||
            ((eType == TType::T_GE) && (score >= beta)) ||
            ((eType == TType::T_LE) && (score <= alpha)))
            return true;
    }
    return false;
}

inline int
TranspositionTable::TTEntry::getDepth() const {
    return getBits(32, 10);
}

inline void
TranspositionTable::TTEntry::setDepth(int d) {
    setBits(32, 10, d);
}

inline int
TranspositionTable::TTEntry::getGeneration() const {
    return getBits(42, 4);
}

inline void
TranspositionTable::TTEntry::setGeneration(int g) {
    setBits(42, 4, g);
}

inline int
TranspositionTable::TTEntry::getType() const {
    return getBits(46, 2);
}

inline void
TranspositionTable::TTEntry::setType(int t) {
    setBits(46, 2, t);
}

inline int
TranspositionTable::TTEntry::getEvalScore() const {
    return (S16)getBits(48, 16);
}

inline void
TranspositionTable::TTEntry::setEvalScore(int s) {
    setBits(48, 16, s);
}

inline void
TranspositionTable::TTEntry::setBits(int first, int size, unsigned int value) {
    U64 mask = ((1ULL << size) - 1) << first;
    data = (data & ~mask) | (((U64)value << first) & mask);
}

inline unsigned int
TranspositionTable::TTEntry::getBits(int first, int size) const {
    U64 sizeMask = ((1ULL << size) - 1);
    return (unsigned int)((data >> first) & sizeMask);
}


inline size_t
TranspositionTable::getIndex(U64 key) const {
    return (size_t)(key & (table.size() - 1));
}

inline U64
TranspositionTable::getStoredKey(U64 key) {
    return key;
}

inline TranspositionTable::TranspositionTable(int log2Size) {
    reSize(log2Size);
}

inline void
TranspositionTable::probe(U64 key, TTEntry& result) {
    size_t idx0 = getIndex(key);
    U64 key2 = getStoredKey(key);
    TTEntry ent;
    ent.load(table[idx0]);
    if (ent.getKey() == key2) {
        if (ent.getGeneration() != generation) {
            ent.setGeneration(generation);
            ent.store(table[idx0]);
        }
        result = ent;
        return;
    }
    size_t idx1 = idx0 ^ 1;
    ent.load(table[idx1]);
    if (ent.getKey() == key2) {
        if (ent.getGeneration() != generation) {
            ent.setGeneration(generation);
            ent.store(table[idx1]);
        }
        result = ent;
        return;
    }
    result.setType(TType::T_EMPTY);
}

inline void
TranspositionTable::prefetch(U64 key) {
#ifdef HAS_PREFETCH
    size_t idx0 = getIndex(key);
    __builtin_prefetch(&table[idx0]);
#endif
}

inline void
TranspositionTable::nextGeneration() {
    generation = (generation + 1) & 15;
}

inline void
TranspositionTable::clear() {
    TTEntry ent;
    ent.clear();
    for (size_t i = 0; i < table.size(); i++)
        ent.store(table[i]);
}

#endif /* TRANSPOSITIONTABLE_HPP_ */
