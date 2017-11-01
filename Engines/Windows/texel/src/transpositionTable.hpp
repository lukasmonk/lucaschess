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
#include "tbgen.hpp"

#include <memory>
#include <vector>

class Position;
class TranspositionTable;


/** TB storage type that uses part of a transposition table. */
class TTStorage {
public:
    explicit TTStorage(TranspositionTable& tt);

    void resize(U32 size);

    const PositionValue operator[](U32 idx) const;
    void store(U32 idx, PositionValue pv);

private:
    TranspositionTable& table;
    U64 idx0;
};


/**
 * Implements the main transposition table using Cuckoo hashing.
 */
class TranspositionTable {
private:
    /** In-memory representation of TT entry. Uses std::atomic for thread safety,
     * but accessed using memory_order_relaxed for maximum performance. */
    struct TTEntryStorage {
        std::atomic<U64> key;
        std::atomic<U64> data;
        TTEntryStorage();
        TTEntryStorage(const TTEntryStorage& a);
    };
    static_assert(sizeof(TTEntryStorage) == 16, "TTEntryStorage size wrong");

public:
    /** A local copy of a transposition table entry. */
    class TTEntry {
    public:
        TTEntry() {}
        TTEntry(U64 key, U64 data) : key(key), data(data) {}

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

        U64 getData() const;

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
        bool getBusy() const;
        void setBusy(bool b);
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
                        // 32  9 depth       Search depth
                        // 41  1 busy        True if some thread is searching in this position
                        // 42  4 generation  Increase when OTB position changes
                        // 46  2 type        exact score, lower bound, upper bound
                        // 48 16 evalScore   Score from static evaluation

        void setBits(int first, int size, unsigned int value);
        unsigned int getBits(int first, int size) const;
    };

    /** Constructor. Creates an empty transposition table with numEntries slots. */
    explicit TranspositionTable(int log2Size);
    TranspositionTable(const TranspositionTable& other) = delete;
    TranspositionTable operator=(const TranspositionTable& other) = delete;

    void reSize(int log2Size);

    /** Insert an entry in the hash table. */
    void insert(U64 key, const Move& sm, int type, int ply, int depth, int evalScore,
                bool busy = false);

    /** Set the busy flag for an entry. Used by "approximate ABDADA" algorithm. */
    void setBusy(const TTEntry& ent, int ply);

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
    void printStats(int rootDepth) const;

    /** Return how full the hash table is, measured in "per mill".
     *  Only an approximate value is returned. */
    int getHashFull() const;


    // Methods to handle tablebase generation and probing

    /**
     * Possibly create or remove a tablebase based on the provided root position
     * and available thinking time.
     * Return true if TBs are available.
     */
    bool updateTB(const Position& pos, RelaxedShared<S64>& maxTimeMillis);

    /** Probe tablebase.
     * @param pos  The position to probe.
     * @param ply  The ply value used to adjust mate scores.
     * @param score The tablebase score. Only modified for tablebase hits.
     * @return True if pos was found in the tablebase, false otherwise.
     */
    bool probeDTM(const Position& pos, int ply, int& score) const;

    /** Low-level methods to read/write a single byte in the table. Used by TB generator code. */
    U8 getByte(U64 idx);
    void putByte(U64 idx, U8 value);
    U64 byteSize() const;

private:
    /** Set hashMask from hash table size. */
    void setHashMask(size_t s);

    /** Get position in hash table given zobrist key. */
    size_t getIndex(U64 key) const;

    /** Get part of zobrist key to store in hash table. */
    static U64 getStoredKey(U64 key);


    TTEntryStorage* table; // Points to either tableV or tableLP
    size_t tableSize;      // Number of entries
    vector_aligned<TTEntryStorage> tableV;
    std::shared_ptr<TTEntryStorage> tableLP; // Large page allocation if used

    U64 hashMask; // Mask to convert zobrist key to table index
    U8 generation;

    // On-demand TB generation
    TTStorage ttStorage;
    std::unique_ptr<TBGenerator<TTStorage>> tbGen;
    int notUsedCnt; // Number of times updateTB() has found the tablebase
                    // unsuitable for the current root position
};


inline
TTStorage::TTStorage(TranspositionTable& tt)
    : table(tt), idx0(0) {}

inline void
TTStorage::resize(U32 size) {
    assert(table.byteSize() > size);
    idx0 = table.byteSize() - size;
}

inline const PositionValue
TTStorage::operator[](U32 idx) const {
    return PositionValue(table.getByte(idx0 + idx));
}

inline void
TTStorage::store(U32 idx, PositionValue pv) {
    table.putByte(idx0 + idx, (U8)pv.getState());
}


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
    int e1 = (getType() == TType::T_EXACT) ? 3 : 0;
    int e2 = (other.getType() == TType::T_EXACT) ? 3 : 0;
    int d1 = getDepth() + e1;
    int d2 = other.getDepth() + e2;
    if (d1 != d2)
        return d1 > d2; // Larger depth is more valuable
    return false;       // Otherwise, pretty much equally valuable
}

inline U64
TranspositionTable::TTEntry::getKey() const {
    return key;
}

inline void
TranspositionTable::TTEntry::setKey(U64 k) {
    key = k;
}

inline U64
TranspositionTable::TTEntry::getData() const {
    return data;
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
    if ((eDepth >= depth) || (eDepth >= plyToMate)) {
        if ( (eType == TType::T_EXACT) ||
            ((eType == TType::T_GE) && (score >= beta)) ||
            ((eType == TType::T_LE) && (score <= alpha)))
            return true;
    }
    if (isWinScore(score) && score >= beta &&
            (eType == TType::T_EXACT || eType == TType::T_GE))
        return true;
    if (isLoseScore(score) && score <= alpha &&
            (eType == TType::T_EXACT || eType == TType::T_LE))
        return true;
    return false;
}

inline int
TranspositionTable::TTEntry::getDepth() const {
    return getBits(32, 9);
}

inline void
TranspositionTable::TTEntry::setDepth(int d) {
    setBits(32, 9, d);
}

inline bool
TranspositionTable::TTEntry::getBusy() const {
    return getBits(41, 1);
}

inline void
TranspositionTable::TTEntry::setBusy(bool b) {
    setBits(41, 1, b);
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


inline void
TranspositionTable::setHashMask(size_t s) {
    hashMask = tableSize - 1;
    hashMask &= ~((size_t)3);
}

inline size_t
TranspositionTable::getIndex(U64 key) const {
    return (size_t)(key & hashMask);
}

inline U64
TranspositionTable::getStoredKey(U64 key) {
    return key;
}

inline void
TranspositionTable::probe(U64 key, TTEntry& result) {
    size_t idx0 = getIndex(key);
    U64 key2 = getStoredKey(key);
    TTEntry ent;
    for (int i = 0; i < 4; i++) {
        ent.load(table[idx0 + i]);
        if (ent.getKey() == key2) {
            if (ent.getGeneration() != generation) {
                ent.setGeneration(generation);
                ent.store(table[idx0 + i]);
            }
            result = ent;
            return;
        }
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

inline U8
TranspositionTable::getByte(U64 idx) {
    U64 ent = idx / 16;
    int offs = idx & 0xf;
    U64 data = (offs < 8) ? table[ent].key.load(std::memory_order_relaxed)
                          : table[ent].data.load(std::memory_order_relaxed);
    offs &= 0x7;
    return (data >> (offs * 8)) & 0xff;
}

inline void
TranspositionTable::putByte(U64 idx, U8 value) {
    U64 ent = idx / 16;
    int offs = idx & 0xf;
    if (offs < 8) {
        U64 data = table[ent].key.load(std::memory_order_relaxed);
        data &= ~(0xffULL << (offs * 8));
        data |= ((U64)value) << (offs * 8);
        table[ent].key.store(data, std::memory_order_relaxed);
    } else {
        offs &= 0x07;
        U64 data = table[ent].data.load(std::memory_order_relaxed);
        data &= ~(0xffULL << (offs * 8));
        data |= ((U64)value) << (offs * 8);
        table[ent].data.store(data, std::memory_order_relaxed);
    }
}

inline U64
TranspositionTable::byteSize() const {
    return tableSize * sizeof(TTEntryStorage);
}


#endif /* TRANSPOSITIONTABLE_HPP_ */
