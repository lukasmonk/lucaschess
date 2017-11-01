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
 * treeLogger.hpp
 *
 *  Created on: Feb 25, 2012
 *      Author: petero
 */

#ifndef TREELOGGER_HPP_
#define TREELOGGER_HPP_

#include "util/util.hpp"
#include "move.hpp"

#include <vector>
#include <type_traits>
#include <utility>
#include <cstring>
#include <fstream>


class TreeLoggerWriter;
class TreeLoggerWriterDummy;

/** Change to TreeLoggerWriter to enable tree logging. */
using TreeLogger = TreeLoggerWriterDummy;


class Position;

namespace Serializer {
    template <typename Type>
    typename std::enable_if<std::is_integral<Type>::value, U8*>::type
    putBytes(U8* buffer, Type value) {
        int s = sizeof(value);
        memcpy(buffer, &value, s);
        return buffer + s;
    }

    template <int N>
    static U8* serialize(U8* buffer) {
        static_assert(N >= 0, "Buffer too small");
        return buffer;
    }

    /** Store a sequence of values in a byte buffer. */
    template <int N, typename Type0, typename... Types>
    static U8* serialize(U8* buffer, Type0 value0, Types... values) {
        const int s = sizeof(Type0);
        buffer = putBytes(buffer, value0);
        return serialize<N-s>(buffer, values...);
    }

    template <typename Type>
    typename std::enable_if<std::is_integral<Type>::value, const U8*>::type
    getBytes(const U8* buffer, Type& value) {
        int s = sizeof(value);
        memcpy(&value, buffer, s);
        return buffer + s;
    }

    template <int N>
    static const U8* deSerialize(const U8* buffer) {
        static_assert(N >= 0, "Buffer too small");
        return buffer;
    }

    /** Retrieve a sequence of values from a byte buffer. */
    template <int N, typename Type0, typename... Types>
    static const U8* deSerialize(const U8* buffer, Type0& value0, Types&... values) {
        const int s = sizeof(Type0);
        buffer = getBytes(buffer, value0);
        return deSerialize<N-s>(buffer, values...);
    }
}

/** Base class for logging search trees to file. */
class TreeLoggerBase {
    friend class TreeLoggerTest;
protected:
    /**
     * Rules for the log file format
     * - A log file contains a search tree dump for one thread.
     * - The file contains information for 0 or more searches.
     * - The log file for thread 0 contains information for a single search.
     * - Information for one search tree starts with a Position0 record and
     *   ends at the next Position0 record or at the end of the file.
     * - A start entry may not have a corresponding end entry. This happens
     *   if the search was interrupted.
     * - Start and end entries are properly nested (assuming the end entries exist)
     *   s1.index < s2.index => e1.index > e2.index
     */

    enum class EntryType : U8 {
        POSITION_INCOMPLETE, // First entry in file has this type when endIndex
                             // has not yet been computed for all StartEntries.
        POSITION_PART0,      // Position entry, first part.
        POSITION_PART1,      // Position entry, second part.
        POSITION_PART2,      // Position entry, third part.
        NODE_START,          // Start of a search node.
        NODE_END             // End of a search node.
    };

    static const U32 endMark = 0xffffffff;

    struct Position0 { // 12
        U32 nextIndex;      // Index of next position, or endMark for last position.
        U64 word0;

        template <int N> U8* serialize(U8 buffer[N]) const {
            return Serializer::serialize<N>(buffer, nextIndex, word0);
        }
        template <int N> void deSerialize(const U8 buffer[N]) {
            Serializer::deSerialize<N>(buffer, nextIndex, word0);
        }
    };

    struct Position1 { // 16
        U64 word1;
        U64 word2;

        template <int N> U8* serialize(U8 buffer[N]) const {
            return Serializer::serialize<N>(buffer, word1, word2);
        }
        template <int N> void deSerialize(const U8 buffer[N]) {
            Serializer::deSerialize<N>(buffer, word1, word2);
        }
    };

    struct Position2 { // 16
        U64 word3;
        U64 word4;

        template <int N> U8* serialize(U8 buffer[N]) const {
            return Serializer::serialize<N>(buffer, word3, word4);
        }
        template <int N> void deSerialize(const U8 buffer[N]) {
            Serializer::deSerialize<N>(buffer, word3, word4);
        }
    };

    struct StartEntry { // 17
        U32 endIndex;
        U32 parentIndex;    // Points to NODE_START or POSITION_PART0 node.
        U16 move;
        S16 alpha;
        S16 beta;
        U8 ply;
        U16 depth;

        Move getMove() const {
            Move ret;
            ret.setMove(move & 63, (move >> 6) & 63, (move >> 12) & 15, 0);
            return ret;
        }

        template <int N> U8* serialize(U8 buffer[N]) const {
            return Serializer::serialize<N>(buffer, endIndex, parentIndex, move,
                                            alpha, beta, ply, depth);
        }
        template <int N> void deSerialize(const U8 buffer[N]) {
            Serializer::deSerialize<N>(buffer, endIndex, parentIndex, move,
                                       alpha, beta, ply, depth);
        }
    };

    struct EndEntry { // 17
        U32 startIndex;
        S16 score;
        U8 scoreType;
        S16 evalScore;
        U64 hashKey;

        template <int N> U8* serialize(U8 buffer[N]) const {
            return Serializer::serialize<N>(buffer, startIndex, score, scoreType,
                                            evalScore, hashKey);
        }
        template <int N> void deSerialize(const U8 buffer[N]) {
            Serializer::deSerialize<N>(buffer, startIndex, score, scoreType,
                                       evalScore, hashKey);
        }
    };

    struct Entry {
        EntryType type;
        union {
            Position0 p0;
            Position1 p1;
            Position2 p2;
            StartEntry se;
            EndEntry ee;
        };

        static const int bufSize = 18;
        using Buffer = U8[bufSize];

        void serialize(U8 buffer[bufSize]) const {
            U8* ptr = buffer;
            using UType = std::underlying_type<EntryType>::type;
            const int su = sizeof(UType);
            UType uType = static_cast<UType>(type);
            ptr = Serializer::serialize<bufSize>(ptr, uType);
            switch (type) {
            case EntryType::POSITION_INCOMPLETE: p0.serialize<bufSize-su>(ptr); break;
            case EntryType::POSITION_PART0:      p0.serialize<bufSize-su>(ptr); break;
            case EntryType::POSITION_PART1:      p1.serialize<bufSize-su>(ptr); break;
            case EntryType::POSITION_PART2:      p2.serialize<bufSize-su>(ptr); break;
            case EntryType::NODE_START:          se.serialize<bufSize-su>(ptr); break;
            case EntryType::NODE_END:            ee.serialize<bufSize-su>(ptr); break;
            }
        }

        void deSerialize(U8 buffer[bufSize]) {
            const U8* ptr = buffer;
            using UType = std::underlying_type<EntryType>::type;
            const int su = sizeof(UType);
            UType uType;
            ptr = Serializer::deSerialize<bufSize>(ptr, uType);
            type = static_cast<EntryType>(uType);
            switch (type) {
            case EntryType::POSITION_INCOMPLETE: p0.deSerialize<bufSize-su>(ptr); break;
            case EntryType::POSITION_PART0:      p0.deSerialize<bufSize-su>(ptr); break;
            case EntryType::POSITION_PART1:      p1.deSerialize<bufSize-su>(ptr); break;
            case EntryType::POSITION_PART2:      p2.deSerialize<bufSize-su>(ptr); break;
            case EntryType::NODE_START:          se.deSerialize<bufSize-su>(ptr); break;
            case EntryType::NODE_END:            ee.deSerialize<bufSize-su>(ptr); break;
            }
        }
    };

    Entry entry;
    U8 entryBuffer[Entry::bufSize];
};

/** Writer class for logging search trees to file. */
class TreeLoggerWriter : public TreeLoggerBase {
public:
    /** Constructor. */
    TreeLoggerWriter();

    /** Destructor. */
    ~TreeLoggerWriter();

    /** Open log file for writing. */
    void open(const std::string& filename, int threadNo);

    /** Flush write cache and close log file. */
    void close();

    /** Return true if log file is opened. */
    bool isOpened() const;

    /** Log information for new position to search.
     * Return index of position entry. */
    U64 logPosition(const Position& pos);

    /** Return node index that will be returned if logNodeStart() is called. */
    U64 peekNextNodeIdx() const;

    /**
     * Log information when entering a search node.
     * @param parentId     Index of parent node.
     * @param m            Move made to go from parent node to this node
     * @param alpha        Search parameter
     * @param beta         Search parameter
     * @param ply          Search parameter
     * @param depth        Search parameter
     * @return node index
     */
    U64 logNodeStart(U64 parentIndex, const Move& m, int alpha, int beta, int ply, int depth);

    /**
     * Log information when leaving a search node.
     * @param startIndex Pointer to corresponding start node entry.
     * @param score      Computed score for this node.
     * @param scoreType  See TranspositionTable, T_EXACT, T_GE, T_LE.
     * @param evalScore  Score returned by evaluation function at this node, if known.
     * @return node index
     */
    U64 logNodeEnd(U64 startIndex, int score, int scoreType, int evalScore, U64 hashKey);

private:
    /** Write position entries to end of file. */
    void writePosition(const Position& pos);

    /** Write entry to end of file. Uses internal buffering, flushed in close(). */
    void appendEntry(const Entry& entry);


    bool opened;
    std::ofstream os;
    U64 nextIndex;

    int threadNo;

    static const int writeCacheSize = 1024;
    U8 writeCache[Entry::bufSize * writeCacheSize];
    int nInWriteCache;
};

/** Dummy version of TreeLoggerWriter. */
class TreeLoggerWriterDummy {
public:
    TreeLoggerWriterDummy() { }
    void open(const std::string& filename, int threadNo) { }
    void close() { }
    bool isOpened() const { return false; }
    U64 logPosition(const Position& pos) { return 0; }
    U64 peekNextNodeIdx() const { return 0; }
    U64 logNodeStart(U64 parentIndex, const Move& m, int alpha, int beta, int ply, int depth) { return 0; }
    U64 logNodeEnd(U64 startIndex, int score, int scoreType, int evalScore, U64 hashKey) { return 0; }
};

/**
 * Reader/analysis class for a search tree dumped to a file.
 */
class TreeLoggerReader : public TreeLoggerBase {
public:
    /** Constructor. */
    explicit TreeLoggerReader(const std::string& filename);

    void close();

    /** Main loop of the interactive tree browser. */
    static void main(const std::string& filename);

private:
    /** Compute endIndex for all StartNode entries. */
    void computeForwardPointers();

    /** Write forward pointer data to disk. */
    void flushForwardPointerData(std::vector<std::pair<U64,U64>>& toWrite);

    /** Get root node information. */
    void getRootNode(U64 index, Position& pos);

    /** Read an entry. */
    void readEntry(U64 index, Entry& entry);

    /** Write an entry. */
    void writeEntry(U64 index, const Entry& entry);

    /** Run the interactive analysis main loop. */
    void mainLoop();

    bool isMove(std::string cmdStr) const;

    /** Return all nodes with a given hash key. */
    void getNodesForHashKey(U64 hashKey, std::vector<U64>& nodes, U64 maxEntry);

    /** Get hash key from an input string. */
    U64 getHashKey(std::string& s, U64 defKey) const;

    /** Get integer parameter from an input string. */
    static int getArg(const std::string& s, int defVal);

    /** Get a list of integer parameters from an input string. */
    void getArgs(const std::string& s, int defVal, std::vector<int>& args);

    /** Get a string parameter from an input string. */
    static std::string getArgStr(const std::string& s, const std::string& defVal);

    void printHelp();

    /** Read start/end entries for a tree node. Return true if the end entry exists. */
    bool readEntries(int index, StartEntry& se, EndEntry& ee);

    /** Find the parent node to a node. */
    S64 findParent(S64 index);

    /** Find all children of a node. */
    void findChildren(S64 index, std::vector<U64>& childs);

    /** Get node position in parents children list. */
    int getChildNo(U64 index);

    /** Get list of nodes from root position to a node. */
    void getNodeSequence(U64 index, std::vector<U64>& nodes);

    /** Find list of moves from root node to a node.
     * Return root node index. */
    U64 getMoveSequence(U64 index, std::vector<Move>& moves);

    /** Find the position corresponding to a node. */
    Position getPosition(U64 index);

    void printNodeInfo(U64 index, int childNo = -1, const std::string& filterMove = "");


    std::fstream fs;
    S64 filePos;        // Current file read position (seekg)
    U64 numEntries;
};


inline
TreeLoggerWriter::TreeLoggerWriter()
    : opened(false), nextIndex(0), threadNo(-1), nInWriteCache(0) {
}

inline
TreeLoggerWriter::~TreeLoggerWriter() {
    close();
}

inline bool
TreeLoggerWriter::isOpened() const {
    return opened;
}

inline U64
TreeLoggerWriter::logPosition(const Position& pos) {
    U64 ret = nextIndex;
    writePosition(pos);
    return ret;
}

inline U64
TreeLoggerWriter::peekNextNodeIdx() const {
    return nextIndex;
}

inline U64
TreeLoggerWriter::logNodeStart(U64 parentIndex, const Move& m, int alpha, int beta, int ply, int depth) {
    if (!opened)
        return 0;
    entry.type = EntryType::NODE_START;
    entry.se.endIndex = -1;
    entry.se.parentIndex = (U32)parentIndex;
    entry.se.move = m.from() + (m.to() << 6) + (m.promoteTo() << 12);
    entry.se.alpha = alpha;
    entry.se.beta = beta;
    entry.se.ply = ply;
    entry.se.depth = depth;
    appendEntry(entry);
    return nextIndex++;
}

inline U64
TreeLoggerWriter::logNodeEnd(U64 startIndex, int score, int scoreType, int evalScore, U64 hashKey) {
    if (!opened)
        return 0;
    entry.type = EntryType::NODE_END;
    entry.ee.startIndex = (U32)startIndex;
    entry.ee.score = score;
    entry.ee.scoreType = scoreType;
    entry.ee.evalScore = evalScore;
    entry.ee.hashKey = hashKey;
    appendEntry(entry);
    return nextIndex++;
}

#endif /* TREELOGGER_HPP_ */
