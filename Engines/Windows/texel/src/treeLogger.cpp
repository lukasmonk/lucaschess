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
 * treeLogger.cpp
 *
 *  Created on: Feb 25, 2012
 *      Author: petero
 */

#include "treeLogger.hpp"
#include "transpositionTable.hpp"
#include "textio.hpp"
#include "position.hpp"
#include "constants.hpp"

#include <iostream>
#include <iomanip>
#include <cassert>

void
TreeLoggerWriter::open(const std::string& filename, int threadNo0) {
    auto fn = filename + std::string(".") + num2Str(threadNo0);
    os.open(fn.c_str(), std::ios_base::out |
                        std::ios_base::binary |
                        std::ios_base::trunc);
    opened = true;
    threadNo = threadNo0;
}

void
TreeLoggerWriter::close() {
    if (opened) {
        if (nInWriteCache > 0) {
            os.write((const char*)writeCache, Entry::bufSize * nInWriteCache);
            nInWriteCache = 0;
        }
        opened = false;
        os.close();
    }
}

void
TreeLoggerWriter::writePosition(const Position& pos) {
    Position::SerializeData data;
    pos.serialize(data);

    entry.type = (nextIndex == 0) ? EntryType::POSITION_INCOMPLETE : EntryType::POSITION_PART0;
    entry.p0.nextIndex = endMark;
    entry.p0.word0 = data.v[0];
    appendEntry(entry);
    nextIndex++;

    entry.type = EntryType::POSITION_PART1;
    entry.p1.word1 = data.v[1];
    entry.p1.word2 = data.v[2];
    appendEntry(entry);
    nextIndex++;

    entry.type = EntryType::POSITION_PART2;
    entry.p2.word3 = data.v[3];
    entry.p2.word4 = data.v[4];
    appendEntry(entry);
    nextIndex++;
}

void
TreeLoggerWriter::appendEntry(const Entry& entry) {
    entry.serialize(entryBuffer);
    memcpy(&writeCache[Entry::bufSize * nInWriteCache], entryBuffer, Entry::bufSize);
    nInWriteCache++;
    if (nInWriteCache == writeCacheSize) {
        os.write((const char*)writeCache, Entry::bufSize * nInWriteCache);
        nInWriteCache = 0;
    }
}


TreeLoggerReader::TreeLoggerReader(const std::string& filename)
    : fs(filename.c_str(), std::ios_base::out |
                           std::ios_base::in |
                           std::ios_base::binary),
      filePos(-1), numEntries(0) {
    fs.exceptions(std::ifstream::failbit | std::ifstream::badbit);
    fs.seekg(0, std::ios_base::end);
    S64 fileLen = fs.tellg();
    numEntries = fileLen / Entry::bufSize;
    computeForwardPointers();
}

void
TreeLoggerReader::close() {
    fs.close();
}

void
TreeLoggerReader::main(const std::string& filename) {
    try {
        TreeLoggerReader an(filename);
        an.mainLoop();
        an.close();
    } catch (const std::exception& ex) {
        std::cout << "Error: " << ex.what() << std::endl;
    }
}

void
TreeLoggerReader::computeForwardPointers() {
    readEntry(0, entry);
    if (entry.type != EntryType::POSITION_INCOMPLETE)
        return;

    std::cout << "Computing forward pointers..." << std::endl;
    const size_t batchSize = 1000000;
    std::vector<std::pair<U64,U64>> toWrite;
    toWrite.reserve(batchSize);
    U64 prevPosIdx = 0;
    for (U64 i = 0; i < numEntries; i++) {
        readEntry(i, entry);
        if (entry.type == EntryType::NODE_END) {
            U64 idx = entry.ee.startIndex;
            toWrite.push_back(std::make_pair(idx, i));
        } else if (entry.type == EntryType::POSITION_PART0) {
            if (i > prevPosIdx)
                toWrite.push_back(std::make_pair(prevPosIdx, i));
            prevPosIdx = i;
        }
        if (toWrite.size() >= batchSize) {
            flushForwardPointerData(toWrite);
            toWrite.clear();
            toWrite.reserve(batchSize);
        }
    }
    flushForwardPointerData(toWrite);

    readEntry(0, entry);
    assert(entry.type == EntryType::POSITION_INCOMPLETE);
    entry.type = EntryType::POSITION_PART0;
    writeEntry(0, entry);

    fs.flush();
    std::cout << "Computing forward pointers... done" << std::endl;
}

void
TreeLoggerReader::flushForwardPointerData(std::vector<std::pair<U64,U64>>& toWrite) {
    if (toWrite.empty())
        return;
    std::sort(toWrite.begin(), toWrite.end());
    const U64 eSize = Entry::bufSize;
    const U64 cacheSize = 512;
    U8 cacheBuf[eSize * cacheSize];
    const U64 emptyMark = std::numeric_limits<U64>::max();
    U64 cacheStart = emptyMark; // Index for first entry in cache

    for (auto p : toWrite) {
        const U64 startIdx = p.first;
        const U64 endIdx = p.second;

        if ((cacheStart != emptyMark) &&
            ((startIdx < cacheStart) || (startIdx >= cacheStart + cacheSize))) { // flush
            int nWrite = std::min(cacheSize, numEntries - cacheStart);
            fs.seekp(cacheStart * eSize, std::ios_base::beg);
            fs.write((const char*)cacheBuf, nWrite * eSize);
            cacheStart = emptyMark;
        }
        if (cacheStart == emptyMark) {
            cacheStart = startIdx;
            int nRead = std::min(cacheSize, numEntries - cacheStart);
            fs.seekg(cacheStart * eSize, std::ios_base::beg);
            fs.read((char*)cacheBuf, nRead * eSize);
        }

        U8* entAddr = &cacheBuf[(startIdx - cacheStart) * eSize];
        entry.deSerialize(entAddr);
        if (entry.type == EntryType::NODE_START) {
            entry.se.endIndex = endIdx;
        } else if ((entry.type == EntryType::POSITION_PART0) ||
                   (entry.type == EntryType::POSITION_INCOMPLETE)) {
            entry.p0.nextIndex = endIdx;
        } else
            assert(false);
        entry.serialize(entAddr);
    }
    if (cacheStart != emptyMark) { // flush
        int nWrite = std::min(cacheSize, numEntries - cacheStart);
        fs.seekp(cacheStart * eSize, std::ios_base::beg);
        fs.write((const char*)cacheBuf, nWrite * eSize);
    }
    filePos = -1;
}

void
TreeLoggerReader::getRootNode(U64 index, Position& pos) {
    readEntry(index, entry);
    if (entry.type == EntryType::POSITION_PART1) {
        index--;
        readEntry(index, entry);
    } else if (entry.type == EntryType::POSITION_PART2) {
        index -= 2;
        readEntry(index, entry);
    }
    assert((entry.type == EntryType::POSITION_INCOMPLETE) ||
           (entry.type == EntryType::POSITION_PART0));

    Position::SerializeData data;
    data.v[0] = entry.p0.word0;

    readEntry(index + 1, entry);
    assert(entry.type == EntryType::POSITION_PART1);
    data.v[1] = entry.p1.word1;
    data.v[2] = entry.p1.word2;

    readEntry(index + 2, entry);
    assert(entry.type == EntryType::POSITION_PART2);
    data.v[3] = entry.p2.word3;
    data.v[4] = entry.p2.word4;

    pos.deSerialize(data);
}

void
TreeLoggerReader::readEntry(U64 index, Entry& entry) {
    S64 offs = index * Entry::bufSize;
    if (offs != filePos)
        fs.seekg(offs, std::ios_base::beg);
    fs.read((char*)entryBuffer, sizeof(entryBuffer));
    filePos = offs + sizeof(entryBuffer);
    entry.deSerialize(entryBuffer);
}

void
TreeLoggerReader::writeEntry(U64 index, const Entry& entry) {
    entry.serialize(entryBuffer);
    S64 offs = index * Entry::bufSize;
    fs.seekp(offs, std::ios_base::beg);
    fs.write((const char*)entryBuffer, sizeof(entryBuffer));
}

static bool isNoMove(const Move& m) {
    return (m.from() == 1) && (m.to() == 1);
}

std::string moveToStr(const Move& m) {
    if (m.isEmpty())
        return "null";
    else if (isNoMove(m))
        return "----";
    else
        return TextIO::moveToUCIString(m);
}

void
TreeLoggerReader::mainLoop() {
    S64 currIndex = -1;
    std::string prevStr = "";

    bool doPrint = true;
    while (true) {
        if (doPrint) {
            if (currIndex >= 0) {
                std::vector<Move> moves;
                U64 rootIdx = getMoveSequence(currIndex, moves);
                if (currIndex != (S64)rootIdx) {
                    printNodeInfo(rootIdx);
                    for (size_t i = 0; i < moves.size(); i++) {
                        const Move& m = moves[i];
                        std::cout << " " << moveToStr(m);
                    }
                    std::cout << std::endl;
                }
                printNodeInfo(currIndex);
                Position pos = getPosition(currIndex);
                std::cout << TextIO::asciiBoard(pos);
                std::cout << TextIO::toFEN(pos) << std::endl;
                std::cout << num2Hex(pos.historyHash()) << std::endl;
                if (currIndex != (S64)rootIdx) {
                    std::vector<U64> children;
                    findChildren(currIndex, children);
                    for (size_t i = 0; i < children.size(); i++)
                        printNodeInfo(children[i], i);
                }
            } else {
                std::cout << std::setw(8) << currIndex << " entries:" << numEntries << std::endl;
            }
        }
        doPrint = true;
        std::cout << "Command:" << std::flush;
        std::string cmdStr;
        std::getline(std::cin, cmdStr);
        if (!std::cin)
            return;
        if (cmdStr.length() == 0)
            cmdStr = prevStr;
        if (startsWith(cmdStr, "q")) {
            return;
        } else if (startsWith(cmdStr, "?")) {
            printHelp();
            doPrint = false;
        } else if (isMove(cmdStr)) {
            if (currIndex >= 0) {
                std::vector<U64> children;
                findChildren(currIndex, children);
                std::string m = cmdStr;
                StartEntry se {};
                EndEntry ee {};
                std::vector<U64> found;
                for (size_t i = 0; i < children.size(); i++) {
                    readEntries(children[i], se, ee);
                    if (moveToStr(se.getMove()) == m)
                        found.push_back(children[i]);
                }
                if (found.empty()) {
                    std::cout << "No such move" << std::endl;
                    doPrint = false;
                } else if (found.size() > 1) {
                    std::cout << "Ambiguous move" << std::endl;
                    for (size_t i = 0; i < found.size(); i++)
                        printNodeInfo(found[i]);
                    doPrint = false;
                } else {
                    currIndex = found[0];
                }
            } else
                doPrint = false;
        } else if (startsWith(cmdStr, "u")) {
            int n = getArg(cmdStr, 1);
            for (int i = 0; i < n; i++)
                currIndex = findParent(currIndex);
        } else if (startsWith(cmdStr, "l")) {
            std::vector<U64> children;
            findChildren(currIndex, children);
            if (currIndex >= 0) {
                bool onlyBest = startsWith(cmdStr, "lb");
                std::string m = getArgStr(cmdStr, "");
                if (onlyBest) {
                    int bestDepth = -1;
                    int bestScore = std::numeric_limits<int>::min();
                    for (size_t i = 0; i < children.size(); i++) {
                        StartEntry se {};
                        EndEntry ee {};
                        bool haveEE = readEntries(children[i], se, ee);
                        if (!haveEE || (ee.scoreType == TType::T_GE) || isNoMove(se.getMove()))
                            continue;
                        int d = se.depth;
                        if ((ee.scoreType == TType::T_EXACT) && (ee.score > se.beta))
                            continue;
                        if ((d > bestDepth) || ((d == bestDepth) && (-ee.score > bestScore))) {
                            bool falseFH = false;
                            if (ee.scoreType == TType::T_LE) {
                                for (size_t ci = i+1; ci < children.size(); ci++) {
                                    StartEntry se2 {};
                                    EndEntry ee2 {};
                                    bool haveEE2 = readEntries(children[ci], se2, ee2);
                                    if (!haveEE2 || !(se2.move == se.move))
                                        break;
                                    if ((ee2.scoreType == TType::T_GE) ||
                                        ((ee2.scoreType == TType::T_EXACT) && (ee2.score > ee.score))) {
                                        falseFH = true;
                                        break;
                                    }
                                    if (ee2.scoreType == TType::T_EXACT)
                                        break;
                                }
                            }
                            if (!falseFH) {
                                printNodeInfo(children[i], i, m);
                                bestDepth = d;
                                bestScore = -ee.score;
                            }
                        }
                    }
                } else {
                    for (size_t i = 0; i < children.size(); i++)
                        printNodeInfo(children[i], i, m);
                }
            } else {
                for (size_t i = 0; i < children.size(); i++)
                    printNodeInfo(children[i], i);
            }
            doPrint = false;
        } else if (startsWith(cmdStr, "n")) {
            if (currIndex >= 0) {
                std::vector<U64> nodes;
                getNodeSequence(currIndex, nodes);
                for (size_t i = 0; i < nodes.size(); i++)
                    printNodeInfo(nodes[i]);
            }
            doPrint = false;
        } else if (startsWith(cmdStr, "d")) {
            std::vector<int> nVec;
            getArgs(cmdStr, 0, nVec);
            for (int n : nVec) {
                std::vector<U64> children;
                findChildren(currIndex, children);
                if ((n >= 0) && (n < (int)children.size())) {
                    currIndex = children[n];
                } else
                    break;
            }
        } else if (startsWith(cmdStr, "p")) {
            if (currIndex >= 0) {
                std::vector<Move> moves;
                getMoveSequence(currIndex, moves);
                for (size_t i = 0; i < moves.size(); i++)
                    std::cout << ' ' << moveToStr(moves[i]);
                std::cout << std::endl;
            }
            doPrint = false;
        } else if (startsWith(cmdStr, "h")) {
            bool onlyPrev = startsWith(cmdStr, "hp");
            U64 hashKey = currIndex >= 0 ? getPosition(currIndex).historyHash() : (U64)-1;
            hashKey = getHashKey(cmdStr, hashKey);
            std::vector<U64> nodes;
            getNodesForHashKey(hashKey, nodes, onlyPrev ? currIndex+1 : numEntries);
            for (size_t i = 0; i < nodes.size(); i++)
                printNodeInfo(nodes[i]);
            doPrint = false;
        } else {
            S64 i;
            if (str2Num(cmdStr, i))
                if ((i >= -1) && (i < (S64)numEntries))
                    currIndex = i;
        }
        prevStr = cmdStr;
    }
}

bool
TreeLoggerReader::isMove(std::string cmdStr) const {
    if (cmdStr.length() != 4)
        return false;
    cmdStr = toLowerCase(cmdStr);
    for (int i = 0; i < 4; i++) {
        int c = cmdStr[i];
        if ((i == 0) || (i == 2)) {
            if ((c < 'a') || (c > 'h'))
                return false;
        } else {
            if ((c < '1') || (c > '8'))
                return false;
        }
    }
    return true;
}

void
TreeLoggerReader::getNodesForHashKey(U64 hashKey, std::vector<U64>& nodes, U64 maxEntry) {
    Position pos;
    for (U64 index = 0; index < maxEntry; index++) {
        readEntry(index, entry);
        if (entry.type == EntryType::NODE_END) {
            if (entry.ee.hashKey == hashKey)
                nodes.push_back(entry.ee.startIndex);
        } else if (entry.type == EntryType::POSITION_PART0) {
            getRootNode(index, pos);
            if (pos.historyHash() == hashKey)
                nodes.push_back(index);
        }
    }
    std::sort(nodes.begin(), nodes.end());
}

U64
TreeLoggerReader::getHashKey(std::string& s, U64 defKey) const {
    U64 key = defKey;
    size_t idx = s.find_first_of(' ');
    if (idx != s.npos) {
        s = s.substr(idx + 1);
        if (startsWith(s, "0x"))
            s = s.substr(2);
        hexStr2Num(s, key);
    }
    return key;
}

int
TreeLoggerReader::getArg(const std::string& s, int defVal) {
    size_t idx = s.find_first_of(' ');
    if (idx != s.npos) {
        int tmp;
        if (str2Num(s.substr(idx+1), tmp))
            return tmp;
    }
    return defVal;
}

void
TreeLoggerReader::getArgs(const std::string& s, int defVal, std::vector<int>& args) {
    std::vector<std::string> split;
    splitString(s, split);
    for (size_t i = 1; i < split.size(); i++) {
        int tmp;
        if (!str2Num(split[i], tmp)) {
            args.clear();
            break;
        }
        args.push_back(tmp);
    }
    if (args.empty())
        args.push_back(defVal);
}

std::string
TreeLoggerReader::getArgStr(const std::string& s, const std::string& defVal) {
    size_t idx = s.find_first_of(' ');
    if (idx != s.npos)
        return s.substr(idx+1);
    return defVal;
}

void
TreeLoggerReader::printHelp() {
    std::cout << "  p              - Print move sequence" << std::endl;
    std::cout << "  n              - Print node info corresponding to move sequence" << std::endl;
    std::cout << "  l [move]       - List child nodes, optionally only for one move" << std::endl;
    std::cout << "  lb [move]      - List best child nodes, optionally only for one move" << std::endl;
    std::cout << "  d [n1 [n2...]] - Go to child \"n\"" << std::endl;
    std::cout << "  move           - Go to child \"move\", if unique" << std::endl;
    std::cout << "  u [levels]     - Move up" << std::endl;
    std::cout << "  h [key]        - Find nodes with current or given hash key" << std::endl;
    std::cout << "  hp [key]       - Find nodes with current or given hash key before current node" << std::endl;
    std::cout << "  num            - Go to node \"num\"" << std::endl;
    std::cout << "  q              - Quit" << std::endl;
    std::cout << "  ?              - Print this help" << std::endl;
}

bool
TreeLoggerReader::readEntries(int index, StartEntry& se, EndEntry& ee) {
    readEntry(index, entry);
    if (entry.type == EntryType::NODE_START) {
        se = entry.se;
        U32 eIdx = se.endIndex;
        if (eIdx == endMark)
            return false;
        readEntry(eIdx, entry);
        assert(entry.type == EntryType::NODE_END);
        ee = entry.ee;
    } else {
        assert(entry.type == EntryType::NODE_END);
        ee = entry.ee;
        readEntry(ee.startIndex, entry);
        assert(entry.type == EntryType::NODE_START);
        se = entry.se;
    }
    return true;
}

S64
TreeLoggerReader::findParent(S64 index) {
    if (index < 0)
        return -1;
    readEntry(index, entry);
    if (entry.type == EntryType::NODE_END) {
        index = entry.ee.startIndex;
        readEntry(index, entry);
    }
    if (entry.type == EntryType::NODE_START) {
        return entry.se.parentIndex;
    } else if ((entry.type == EntryType::POSITION_PART0) ||
               (entry.type == EntryType::POSITION_PART1) ||
               (entry.type == EntryType::POSITION_PART2)) {
        return -1;
    } else {
        assert(false);
        return -1;
    }
}

void
TreeLoggerReader::findChildren(S64 index, std::vector<U64>& childs) {
    if (index < 0) {
        if (numEntries == 0)
            return;
        U64 child = 0;
        while (child < numEntries - 1) {
            readEntry(child, entry);
            assert(entry.type == EntryType::POSITION_PART0);
            childs.push_back(child);
            child = entry.p0.nextIndex;
        }
    } else {
        readEntry(index, entry);
        U64 child;
        switch (entry.type) {
        case EntryType::NODE_END:
            index = entry.ee.startIndex;
            // Fall through
        case EntryType::NODE_START:
            child = index + 1;
            break;
        case EntryType::POSITION_PART2:
            index--;
            // Fall through
        case EntryType::POSITION_PART1:
            index--;
            // Fall through
        case EntryType::POSITION_PART0:
            child = index + 3;
            break;
        default:
            assert(false);
        }
        StartEntry se {};
        EndEntry ee {};
        while (child < numEntries) {
            readEntry(child, entry);
            if (entry.type != EntryType::NODE_START)
                break;
            bool haveEE = readEntries(child, se, ee);
            if (se.parentIndex == index)
                childs.push_back(child);
            if (!haveEE)
                break;
            if (se.endIndex == endMark)
                break;
            child = se.endIndex + 1;
        }
    }
}

int
TreeLoggerReader::getChildNo(U64 index) {
    readEntry(index, entry);
    if (entry.type == EntryType::NODE_END) {
        index = entry.ee.startIndex;
        readEntry(index, entry);
    } else if (entry.type == EntryType::POSITION_PART1) {
        index--;
        readEntry(index, entry);
    } else if (entry.type == EntryType::POSITION_PART2) {
        index -= 2;
        readEntry(index, entry);
    }
    std::vector<U64> childs;
    if (entry.type == EntryType::NODE_START) {
        findChildren(entry.se.parentIndex, childs);
    } else if (entry.type == EntryType::POSITION_PART0) {
        findChildren(-1, childs);
    } else
        assert(false);

    for (int i = 0; i < (int)childs.size(); i++)
        if (childs[i] == index)
            return i;
    return -1;
}

void
TreeLoggerReader::getNodeSequence(U64 index, std::vector<U64>& nodes) {
    nodes.push_back(index);
    while (true) {
        readEntry(index, entry);
        if (entry.type == EntryType::NODE_END) {
            index = entry.ee.startIndex;
            readEntry(index, entry);
        }
        if (entry.type == EntryType::NODE_START) {
            index = entry.se.parentIndex;
            nodes.push_back(index);
        } else
            break;
    }
    std::reverse(nodes.begin(), nodes.end());
}

U64
TreeLoggerReader::getMoveSequence(U64 index, std::vector<Move>& moves) {
    StartEntry se {};
    EndEntry ee {};
    while (true) {
        readEntry(index, entry);
        if ((entry.type == EntryType::NODE_START) ||
            (entry.type == EntryType::NODE_END)) {
            readEntries(index, se, ee);
            moves.push_back(se.getMove());
            index = findParent(index);
        } else
            break;
    }
    std::reverse(moves.begin(), moves.end());
    return index;
}

Position
TreeLoggerReader::getPosition(U64 index) {
    std::vector<Move> moves;
    U64 rootPosIdx = getMoveSequence(index, moves);
    Position pos;
    getRootNode(rootPosIdx, pos);
    UndoInfo ui;
    for (size_t i = 0; i < moves.size(); i++)
        if (!isNoMove(moves[i]))
            pos.makeMove(moves[i], ui);
    return pos;
}

void
TreeLoggerReader::printNodeInfo(U64 index, int childNo, const std::string& filterMove) {
    if (childNo == -1)
        childNo = getChildNo(index);
    readEntry(index, entry);
    if ((entry.type == EntryType::NODE_START) ||
        (entry.type == EntryType::NODE_END)) {
        StartEntry se {};
        EndEntry ee {};
        bool haveEE = readEntries(index, se, ee);
        std::string m = moveToStr(se.getMove());
        if ((filterMove.length() > 0) && (m != filterMove))
            return;
        std::cout << std::setw(3) << childNo
                  << ' '   << std::setw(8) << index
                  << ' '   << m
                  << " a:" << std::setw(6) << se.alpha
                  << " b:" << std::setw(6) << se.beta
                  << " p:" << std::setw(2) << (int)se.ply
                  << " d:" << std::setw(2) << (int)se.depth;
        if (haveEE) {
            int subTreeNodes = (se.endIndex - ee.startIndex - 1) / 2;
            std::string type;
            switch (ee.scoreType) {
            case TType::T_EXACT: type = "= "; break;
            case TType::T_GE   : type = ">="; break;
            case TType::T_LE   : type = "<="; break;
            default            : type = "  "; break;
            }
            std::cout << " s:" << type << std::setw(6) << ee.score
                    << " e:" << std::setw(6) << ee.evalScore
                    << " sub:" << subTreeNodes;
        }
        std::cout << std::endl;
    } else if ((entry.type == EntryType::POSITION_PART0) ||
               (entry.type == EntryType::POSITION_PART1) ||
               (entry.type == EntryType::POSITION_PART2)) {
        Position pos;
        getRootNode(index, pos);
        std::cout << std::setw(3) << childNo
                  << ' ' << std::setw(8) << index
                  << ' ' << TextIO::toFEN(pos) << std::endl;
    } else
        assert(false);
}
