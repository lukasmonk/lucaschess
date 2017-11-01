/*
    Texel - A UCI chess engine.
    Copyright (C) 2014-2016  Peter Österlund, peterosterlund2@gmail.com

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
 * bookbuild.cpp
 *
 *  Created on: Apr 27, 2014
 *      Author: petero
 */

#include "bookbuild.hpp"
#include "polyglot.hpp"
#include "gametree.hpp"
#include "moveGen.hpp"
#include "search.hpp"
#include "textio.hpp"

namespace BookBuild {

void
BookNode::updateScores(const BookData& bookData) {
    struct Compare {
        bool operator()(const BookNode* n1, const BookNode* n2) const {
            if (n1->getDepth() != n2->getDepth())
                return n1->getDepth() < n2->getDepth();
            return n1 < n2;
        }
    };
    std::set<BookNode*,Compare> toUpdate;
    toUpdate.insert(this);

    BookNode* startNode = this;

    std::function<void(BookNode*,bool,bool,bool)> updateNegaMax =
        [&updateNegaMax,&bookData,&toUpdate,startNode]
         (BookNode* node, bool updateThis, bool updateChildren, bool updateParents) {
        if (!updateThis && (node->negaMaxScore != INVALID_SCORE))
            return;
        if (updateChildren) {
            for (auto& e : node->children)
                updateNegaMax(e.second, false, true, false);
        }
        bool propagate = node->computeNegaMax(bookData);
        if (propagate)
            for (auto& e : node->children)
                toUpdate.insert(e.second);
        if (updateParents && (propagate || node == startNode)) {
            for (auto& e : node->parents) {
                BookNode* parent = e.parent;
                assert(parent);
                updateNegaMax(parent, true, false, true);
            }
        }
    };
    updateNegaMax(this, true, true, true);

    std::function<void(BookNode*)> updatePathErrors =
        [&updatePathErrors,&bookData](BookNode* node) {
        bool modified = node->computePathError(bookData);
        if (modified)
            for (auto& e : node->children)
                updatePathErrors(e.second);
    };
    for (BookNode* n : toUpdate)
        updatePathErrors(n);
}

bool
BookNode::computeNegaMax(const BookData& bookData) {
    const int oldNM = negaMaxScore;
    const int oldEW = expansionCostWhite;
    const int oldEB = expansionCostBlack;

    negaMaxScore = searchScore;
    auto it = children.find(bestNonBookMove.getCompressedMove());
    if (it != children.end()) {
        // Ignore searchScore if a child node contains information about the same move
        if (it->second->getNegaMaxScore() != INVALID_SCORE)
            negaMaxScore = IGNORE_SCORE;
    }
    if (negaMaxScore != INVALID_SCORE)
        for (const auto& e : children)
            negaMaxScore = std::max(negaMaxScore, negateScore(e.second->negaMaxScore));

    expansionCostWhite = IGNORE_SCORE;
    expansionCostBlack = IGNORE_SCORE;
    if (!bookData.isPending(hashKey)) {
        if (searchScore == INVALID_SCORE) {
            expansionCostWhite = INVALID_SCORE;
            expansionCostBlack = INVALID_SCORE;
        } else if (searchScore != IGNORE_SCORE) {
            expansionCostWhite = getExpansionCost(bookData, nullptr, true);
            expansionCostBlack = getExpansionCost(bookData, nullptr, false);
        }
    }
    for (const auto& e : children) {
        if (e.second->expansionCostWhite == INVALID_SCORE)
            expansionCostWhite = INVALID_SCORE;
        if (e.second->expansionCostBlack == INVALID_SCORE)
            expansionCostBlack = INVALID_SCORE;
    }

    for (const auto& e : children) {
        BookNode* child = e.second;
        if ((expansionCostWhite != INVALID_SCORE) &&
            (child->expansionCostWhite != IGNORE_SCORE)) {
            int cost = getExpansionCost(bookData, child, true);
            if ((expansionCostWhite == IGNORE_SCORE) || (expansionCostWhite > cost))
                expansionCostWhite = cost;
        }
        if ((expansionCostBlack != INVALID_SCORE) &&
            (child->expansionCostBlack != IGNORE_SCORE)) {
            int cost = getExpansionCost(bookData, child, false);
            if ((expansionCostBlack == IGNORE_SCORE) || (expansionCostBlack > cost))
                expansionCostBlack = cost;
        }
    }

    return ((negaMaxScore != oldNM) ||
            (expansionCostWhite != oldEW) ||
            (expansionCostBlack != oldEB));
}

int
BookNode::getExpansionCost(const BookData& bookData, const BookNode* child,
                           bool white) const {
    const int ownCost = bookData.ownPathErrorCost();
    const int otherCost = bookData.otherPathErrorCost();
    if (child) {
        int moveError = (negaMaxScore == INVALID_SCORE) ? 1000 :
                        negaMaxScore - negateScore(child->negaMaxScore);
        assert(moveError >= 0);
        bool wtm = getDepth() % 2 == 0;
        int cost = white ? child->expansionCostWhite : child->expansionCostBlack;
        if (cost != IGNORE_SCORE && cost != INVALID_SCORE)
            cost += bookData.bookDepthCost() + moveError * (wtm == white ? ownCost : otherCost);
        return cost;
    } else {
        if (children.find(bestNonBookMove.getCompressedMove()) != children.end()) {
            return -10000; // bestNonBookMove is obsoleted by a child node
        } else {
            int moveError = negaMaxScore - searchScore;
            assert(moveError >= 0);
            bool wtm = getDepth() % 2 == 0;
            int ownCost = bookData.ownPathErrorCost();
            int otherCost = bookData.otherPathErrorCost();
            return moveError * (wtm == white ? ownCost : otherCost);
        }
    }
}

bool
BookNode::computePathError(const BookData& bookData) {
    if (getDepth() == 0) {
        assert(pathErrorWhite == 0);
        assert(pathErrorBlack == 0);
        return false;
    }

    const int oldPW = pathErrorWhite;
    const int oldPB = pathErrorBlack;

    pathErrorWhite = INT_MAX;
    pathErrorBlack = INT_MAX;
    for (auto& e : parents) {
        BookNode* parent = e.parent;
        assert(parent);
        int errW = parent->getPathErrorWhite();
        int errB = parent->getPathErrorBlack();
        if (errW == INVALID_SCORE || errB == INVALID_SCORE)
            continue;
        if (getNegaMaxScore() == INVALID_SCORE || parent->getNegaMaxScore() == INVALID_SCORE)
            continue;
        int delta = parent->getNegaMaxScore() - negateScore(getNegaMaxScore());
        assert(delta >= 0);
        if ((getDepth() % 2) != 0)
            errW += delta;
        else
            errB += delta;
        pathErrorWhite = std::min(pathErrorWhite, errW);
        pathErrorBlack = std::min(pathErrorBlack, errB);
    }
    if (pathErrorWhite == INT_MAX || pathErrorBlack == INT_MAX) {
        pathErrorWhite = INVALID_SCORE;
        pathErrorBlack = INVALID_SCORE;
    }

    return (pathErrorWhite != oldPW) || (pathErrorBlack != oldPB);
}

int
BookNode::negateScore(int score) {
    if (score == IGNORE_SCORE || score == INVALID_SCORE)
        return score; // No negation
    if (SearchConst::isWinScore(score))
        return -(score-1);
    if (SearchConst::isLoseScore(score))
        return -(score+1);
    return -score;
}

void
BookNode::setSearchResult(const BookData& bookData,
                          const Move& bestMove, int score, int time) {
    bestNonBookMove = bestMove;
    searchScore = score;
    searchTime = time;
    updateScores(bookData);
}

void
BookNode::updateDepth() {
    bool updated = false;
    for (auto& e : parents) {
        BookNode* parent = e.parent;
        assert(parent);
        if (parent->depth == INT_MAX)
            parent->updateDepth();
        assert(parent->depth >= 0);
        assert(parent->depth < INT_MAX);
        if (depth != INT_MAX)
            assert((depth - parent->depth) % 2 != 0);
        if (depth > parent->depth + 1) {
            depth = parent->depth + 1;
            updated = true;
        }
    }
    if (updated)
        for (auto& e : children)
            e.second->updateDepth();
}

// ----------------------------------------------------------------------------

Book::Book(const std::string& backupFile0, int bookDepthCost,
           int ownPathErrorCost, int otherPathErrorCost)
    : startPosHash(TextIO::readFEN(TextIO::startPosFEN).bookHash()),
      backupFile(backupFile0),
      bookData(bookDepthCost, ownPathErrorCost, otherPathErrorCost) {
    addRootNode();
    if (!backupFile.empty())
        writeToFile(backupFile);
}

class DropoutSelector : public Book::PositionSelector {
public:
    DropoutSelector(Book& b, std::mutex& mutex0,
                    const std::atomic<U64>& startPosHash,
                    const std::atomic<bool>& stopFlag0) :
        book(b), mutex(mutex0), startHash(startPosHash),
        stopFlag(stopFlag0), whiteBook(true) {
    }

    bool getNextPosition(Position& pos, Move& move) override {
        std::lock_guard<std::mutex> L(mutex);
        if (stopFlag.load())
            return false;
        BookNode* ptr = book.getBookNode(startHash.load());
        if (!ptr)
            return false;
        while (true) {
            int cost = whiteBook ? ptr->getExpansionCostWhite() : ptr->getExpansionCostBlack();
            if (cost == IGNORE_SCORE)
                return false;
            std::vector<BookNode*> goodChildren;
            for (const auto& e : ptr->getChildren()) {
                BookNode* child = e.second;
                int childCost = ptr->getExpansionCost(book.bookData, child, whiteBook);
                if (cost == childCost)
                    goodChildren.push_back(child);
            }
            if (goodChildren.empty())
                break;
            std::random_shuffle(goodChildren.begin(), goodChildren.end());
            ptr = goodChildren[0];
        }
        move = ptr->getBestNonBookMove();
        if (ptr->getChildren().find(move.getCompressedMove()) != ptr->getChildren().end())
            move = Move();
        std::vector<Move> moveList;
        book.getPosition(ptr->getHashKey(), pos, moveList);
        whiteBook = !whiteBook;
        return true;
    }

private:
    Book& book;
    std::mutex& mutex;
    const std::atomic<U64>& startHash;
    const std::atomic<bool>& stopFlag;
    bool whiteBook;
};

void
Book::improve(const std::string& bookFile, int searchTime, int numThreads,
              const std::string& startMoves) {
    readFromFile(bookFile);

    Position startPos = TextIO::readFEN(TextIO::startPosFEN);
    UndoInfo ui;
    std::vector<std::string> startMoveVec;
    splitString(startMoves, startMoveVec);
    for (const auto& ms : startMoveVec) {
        Move m = TextIO::stringToMove(startPos, ms);
        startPos.makeMove(m, ui);
    }

    std::atomic<U64> startHash(startPos.bookHash());
    std::atomic<bool> stopFlag(false);
    DropoutSelector selector(*this, mutex, startHash, stopFlag);
    TranspositionTable tt(27);
    extendBook(selector, searchTime, numThreads, tt);
}

void
Book::interactiveExtendBook(int searchTime, int numThreads,
                            TranspositionTable& tt,
                            const std::atomic<U64>& startHash,
                            const std::atomic<bool>& stopFlag) {
    DropoutSelector selector(*this, mutex, startHash, stopFlag);
    extendBook(selector, searchTime, numThreads, tt);
}

void
Book::abortExtendBook() {
    std::lock_guard<std::mutex> L(mutex);
    std::shared_ptr<SearchScheduler> sched = searchScheduler.lock();
    if (sched)
        sched->abort();
}

void
Book::importPGN(const std::string& bookFile, const std::string& pgnFile,
                int maxPly) {
    readFromFile(bookFile);

    // Create book nodes for all positions in the PGN file
    std::ifstream is(pgnFile);
    PgnReader reader(is);
    GameTree gt;
    int nGames = 0;
    int nAdded = 0;
    try {
        while (reader.readPGN(gt)) {
            nGames++;
            GameNode gn = gt.getRootNode();
            addToBook(maxPly, gn, nAdded);
        }
    } catch (...) {
        std::cerr << "Error parsing game " << nGames << std::endl;
        throw;
    }
    std::cout << "Added " << nAdded << " positions from " << nGames << " games" << std::endl;
}

void
Book::addToBook(int maxPly, GameNode& gn, int& nAdded) {
    std::lock_guard<std::mutex> L(mutex);
    std::function<void(int)> addToBookInternal =
            [&addToBookInternal, this, maxPly, &gn, &nAdded](int ply) {
        if (ply >= maxPly)
            return;
        Position base = gn.getPos();
        for (int i = 0; i < gn.nChildren(); i++) {
            gn.goForward(i);
            if (!getBookNode(gn.getPos().bookHash())) {
                assert(!gn.getMove().isEmpty());
                std::vector<U64> toSearch;
                addPosToBook(base, gn.getMove(), toSearch);
                nAdded++;
            }
            addToBookInternal(ply+1);
            gn.goBack();
        }
    };
    addToBookInternal(0);
}

void
Book::exportPolyglot(const std::string& bookFile, const std::string& polyglotFile,
                     int maxErrSelf, double errOtherExpConst) {
    readFromFile(bookFile);

    WeightInfo weights;
    computeWeights(maxErrSelf, errOtherExpConst, weights);

    class BookMoves {
    public:
        void addMove(U16 m, double w) {
            assert(w >= 0.0);
            if (w == 0.0)
                w = 1.0; // Happens when best move has path error > maxErrSelf
            for (size_t i = 0; i < vec.size(); i++)
                if (vec[i].move == m) {
                    vec[i].weight += w;
                    return;
                }
            vec.push_back(MoveWeight(m, w));
        }

        void scaleWeights() {
            double maxW = 0;
            for (size_t i = 0; i < vec.size(); i++)
                maxW = std::max(maxW, vec[i].weight);
            const int maxAllowedW = 0xffff;
            for (size_t i = 0; i < vec.size(); i++) {
                double w = vec[i].weight * maxAllowedW / maxW;
                vec[i].weight = clamp((int)w, 1, maxAllowedW);
            }
        }

        int nMoves() const { return vec.size(); }
        U16 getMove(int i) const { return vec[i].move; }
        U16 getWeight(int i) const { return (U16)vec[i].weight; }

    private:
        struct MoveWeight {
            MoveWeight(U16 m, double w) : move(m), weight(w) {}
            U16 move;
            double weight;
        };
        std::vector<MoveWeight> vec;
    };
    std::map<U64, BookMoves> pgBook;

    Position pos;
    std::vector<Move> moveList;
    for (auto& e : bookNodes) {
        const BookNode* node = e.second.get();
        moveList.clear();
        if (!getPosition(node->getHashKey(), pos, moveList))
            assert(false);
        const bool wtm = pos.isWhiteMove();
        BookMoves& bm = pgBook[PolyglotBook::getHashKey(pos)];
        for (auto& c : node->getChildren()) {
            U16 cMove = c.first;
            BookNode* child = c.second;
            if (bookMoveOk(*node, cMove, maxErrSelf)) {
                Move move;
                move.setFromCompressed(cMove);
                U16 pgMove = PolyglotBook::getPGMove(pos, move);
                auto wi = weights.find(child->getHashKey());
                assert(wi != weights.end());
                double w = wtm ? wi->second.weightWhite : wi->second.weightBlack;
                bm.addMove(pgMove, w);
            }
        }

        if (bookMoveOk(*node, Move().getCompressedMove(), maxErrSelf)) {
            Move move = node->getBestNonBookMove();
            U16 pgMove = PolyglotBook::getPGMove(pos, move);
            int errW, errB;
            getDropoutPathErrors(*node, errW, errB);
            double wW = 0.0, wB = 0.0;
            if (errW != INVALID_SCORE && errB != INVALID_SCORE) {
                wW = errW <= maxErrSelf ? exp(-errB / errOtherExpConst) : 0.0;
                wB = errB <= maxErrSelf ? exp(-errW / errOtherExpConst) : 0.0;
            }
            double w = wtm ? wW : wB;
            bm.addMove(pgMove, w);
        }
    }

    std::ofstream os;
    os.open(polyglotFile.c_str(), std::ios_base::out |
                                  std::ios_base::binary |
                                  std::ios_base::trunc);
    os.exceptions(std::ifstream::failbit | std::ifstream::badbit);
    PolyglotBook::PGEntry ent;
    for (auto& e : pgBook) {
        U64 pgHash = e.first;
        BookMoves& bm = e.second;
        bm.scaleWeights();
        for (int i = 0; i < bm.nMoves(); i++) {
            assert(bm.getWeight(i) > 0);
            PolyglotBook::serialize(pgHash, bm.getMove(i), bm.getWeight(i), ent);
            os.write((const char*)&ent.data[0], sizeof(ent.data));
        }
    }
}

void
Book::interactiveQuery(const std::string& bookFile, int maxErrSelf, double errOtherExpConst) {
    readFromFile(bookFile);

    WeightInfo weights;
    computeWeights(maxErrSelf, errOtherExpConst, weights);

    Position pos = TextIO::readFEN(TextIO::startPosFEN);
    std::vector<Move> movePath;

    for (std::string prevStr, cmdStr; true; prevStr = cmdStr) {
        printBookInfo(pos, movePath, weights, maxErrSelf, errOtherExpConst);
        std::cout << "Command:" << std::flush;
        std::getline(std::cin, cmdStr);
        if (!std::cin)
            break;
        cmdStr = trim(cmdStr);
        if (cmdStr.length() == 0)
            cmdStr = prevStr;

        if (cmdStr ==  "q")
            break;

        if (startsWith(cmdStr, "?")) {
            std::cout << "  d n           - Go to child \"n\"" << std::endl;
            std::cout << "  move          - Go to child \"move\"" << std::endl;
            std::cout << "  u [levels]    - Move up" << std::endl;
            std::cout << "  h key         - Go to node with given hash key" << std::endl;
            std::cout << "  f fen         - Go to node given by FEN string" << std::endl;
            std::cout << "  r [errS errO] - Re-read book from file" << std::endl;
            std::cout << "  q             - Quit" << std::endl;
            std::cout << "  ?             - Print this help" << std::endl;
            continue;
        }
        std::vector<Move> childMoves;
        BookNode* node = getBookNode(pos.bookHash());
        assert(node);
        getOrderedChildMoves(*node, childMoves);
        Move childMove;
        for (const Move& m : childMoves) {
            std::string moveS = TextIO::moveToString(pos, m, false);
            if (toLowerCase(moveS) == toLowerCase(cmdStr)) {
                childMove = m;
                break;
            }
        }
        if (childMove.isEmpty() && startsWith(cmdStr, "d")) {
            std::vector<std::string> split;
            splitString(cmdStr, split);
            if (split.size() > 1) {
                int moveNo = -1;
                if (str2Num(split[1], moveNo) && (moveNo >= 0) &&
                        (moveNo < (int)childMoves.size()))
                    childMove = childMoves[moveNo];
            }
        }
        if (!childMove.isEmpty()) {
            UndoInfo ui;
            pos.makeMove(childMove, ui);
            movePath.push_back(childMove);
        } else if (startsWith(cmdStr, "u")) {
            int levels = 1;
            std::vector<std::string> split;
            splitString(cmdStr, split);
            if (split.size() > 1)
                str2Num(split[1], levels);
            while (levels > 0 && !movePath.empty()) {
                movePath.pop_back();
                levels--;
            }
            pos = TextIO::readFEN(TextIO::startPosFEN);
            for (const Move& m : movePath) {
                UndoInfo ui;
                pos.makeMove(m, ui);
            }
        } else if (startsWith(cmdStr, "h")) {
            std::vector<std::string> split;
            splitString(cmdStr, split);
            if (split.size() > 1) {
                std::string tmp = split[1];
                if (startsWith(tmp, "0x"))
                    tmp = tmp.substr(2);
                U64 hashKey;
                if (hexStr2Num(tmp, hashKey)) {
                    Position tmpPos;
                    std::vector<Move> moveList;
                    if (getPosition(hashKey, tmpPos, moveList)) {
                        pos = tmpPos;
                        movePath = moveList;
                    }
                }
            }
        } else if (startsWith(cmdStr, "f")) {
            size_t idx = cmdStr.find(' ');
            if (idx != std::string::npos) {
                std::string fen = cmdStr.substr(idx + 1);
                fen = trim(fen);
                try {
                    Position tmpPos = TextIO::readFEN(fen);
                    std::vector<Move> moveList;
                    if (getPosition(tmpPos.bookHash(), tmpPos, moveList)) {
                        pos = tmpPos;
                        movePath = moveList;
                    }
                } catch (const ChessParseError& ex) {
                    std::cerr << "Error: " << ex.what() << std::endl;
                }
            }
        } else if (startsWith(cmdStr, "r")) {
            std::vector<std::string> split;
            splitString(cmdStr, split);
            if (split.size() == 3) {
                int errSelf;
                double errOther;
                if (str2Num(split[1], errSelf) && (errSelf >= 0) &&
                    str2Num(split[2], errOther) && (errOther > 0)) {
                    maxErrSelf = errSelf;
                    errOtherExpConst = errOther;
                }
            }
            readFromFile(bookFile);
            computeWeights(maxErrSelf, errOtherExpConst, weights);
            if (!getBookNode(pos.bookHash())) {
                pos = TextIO::readFEN(TextIO::startPosFEN);
                movePath.clear();
            }
        }
    }
}

void
Book::statistics(const std::string& bookFile) {
    readFromFile(bookFile);
    const int maxPly = 1000;
    Histogram<0, maxPly> hist;
    for (auto& bn : bookNodes)
        hist.add(bn.second->getDepth());

    int maxNonZero = 0;
    for (int i = 1; i < maxPly; i++)
        if (hist.get(i) > 0)
            maxNonZero = i;

    for (int i = 0; i <= maxNonZero; i++)
        std::cout << std::setw(2) << i << ' ' << hist.get(i) << std::endl;
}

// ----------------------------------------------------------------------------

void
Book::addRootNode() {
    if (!getBookNode(startPosHash)) {
        auto rootNode(std::make_shared<BookNode>(startPosHash, true));
        rootNode->setState(BookNode::INITIALIZED);
        bookNodes[startPosHash] = rootNode;
        Position pos = TextIO::readFEN(TextIO::startPosFEN);
        setChildRefs(pos);
        writeBackup(*rootNode);
    }
}

void
Book::readFromFile(const std::string& filename) {
    bookNodes.clear();
    hashToParent.clear();
    bookData.clearPending();

    // Read all book entries
    std::ifstream is;
    is.open(filename.c_str(), std::ios_base::in |
                              std::ios_base::binary);

    std::set<U64> zeroTime;
    while (true) {
        BookNode::BookSerializeData bsd;
        is.read((char*)&bsd.data[0], sizeof(bsd.data));
        if (!is)
            break;
        auto bn(std::make_shared<BookNode>(0));
        bn->deSerialize(bsd);
        if (bn->getSearchTime() == 0) {
            zeroTime.insert(bn->getHashKey());
        } else {
            zeroTime.erase(bn->getHashKey());
        }
        if (bn->getHashKey() == startPosHash)
            bn->setRootNode();
        bookNodes[bn->getHashKey()] = bn;
    }
    is.close();

    // Find positions for all book entries by exploring moves from the starting position
    Position pos = TextIO::readFEN(TextIO::startPosFEN);
    initPositions(pos);
    addRootNode();

    // Initialize all negamax scores
    getBookNode(startPosHash)->updateScores(bookData);

    if (!backupFile.empty())
        writeToFile(backupFile);

    std::cout << "nZeroTime:" << zeroTime.size() << std::endl;
}

void
Book::writeToFile(const std::string& filename) {
    std::lock_guard<std::mutex> L(mutex);
    std::ofstream os;
    os.open(filename.c_str(), std::ios_base::out |
                              std::ios_base::binary |
                              std::ios_base::trunc);
    os.exceptions(std::ifstream::failbit | std::ifstream::badbit);

    for (const auto& e : bookNodes) {
        auto& node = e.second;
        BookNode::BookSerializeData bsd;
        node->serialize(bsd);
        os.write((const char*)&bsd.data[0], sizeof(bsd.data));
    }
}

void
Book::extendBook(PositionSelector& selector, int searchTime, int numThreads,
                 TranspositionTable& tt) {
    std::shared_ptr<SearchScheduler> scheduler;
    {
        std::lock_guard<std::mutex> L(mutex);
        scheduler = std::make_shared<SearchScheduler>();
        searchScheduler = scheduler;
        for (int i = 0; i < numThreads; i++) {
            auto sr = make_unique<SearchRunner>(i, tt);
            scheduler->addWorker(std::move(sr));
        }
    }
    scheduler->startWorkers();

    int numPending = 0;
    const int desiredQueueLen = numThreads + 1;
    int workId = 0;   // Work unit ID number
    int commitId = 0; // Next work unit to be stored in opening book
    std::set<SearchScheduler::WorkUnit> completed; // Completed but not yet committed to book
    while (true) {
        bool workAdded = false;
        if (numPending < desiredQueueLen) {
            Position pos;
            Move move;
            if (selector.getNextPosition(pos, move)) {
                std::lock_guard<std::mutex> L(mutex);
                assert(getBookNode(pos.bookHash()));
                std::vector<U64> toSearch;
                if (move.isEmpty()) {
                    toSearch.push_back(pos.bookHash());
                } else
                    addPosToBook(pos, move, toSearch);
                for (U64 hKey : toSearch) {
                    Position pos2;
                    std::vector<Move> moveList;
                    if (!getPosition(hKey, pos2, moveList))
                        assert(false);
                    SearchScheduler::WorkUnit wu;
                    wu.id = workId++;
                    wu.hashKey = hKey;
                    wu.gameMoves = moveList;
                    wu.movesToSearch = getMovesToSearch(pos2);
                    wu.searchTime = searchTime;
                    scheduler->addWorkUnit(wu);
                    numPending++;
                    addPending(hKey);
                    workAdded = true;
                }
            }
        }
        if (!workAdded && (numPending == 0))
            break;
        if (workAdded && listener)
            listener->queueChanged(numPending);
        if (!workAdded || (numPending >= desiredQueueLen)) {
            SearchScheduler::WorkUnit wu;
            scheduler->getResult(wu);
            completed.insert(wu);
            bool workRemoved = false;
            while (!completed.empty() && completed.begin()->id == commitId) {
                std::lock_guard<std::mutex> L(mutex);
                wu = *completed.begin();
                completed.erase(completed.begin());
                numPending--;
                commitId++;
                workRemoved = true;
                removePending(wu.hashKey);
                if (!scheduler->isAborting()) {
                    auto bn = getBookNode(wu.hashKey);
                    assert(bn);
                    bn->setSearchResult(bookData,
                                        wu.bestMove, wu.bestMove.score(), wu.searchTime);
                    writeBackup(*bn);
                    scheduler->reportResult(wu);
                }
            }
            if (workRemoved && listener && numPending > 0)
                listener->queueChanged(numPending);
        }
    }
    if (listener)
        listener->queueChanged(0);
}

std::vector<Move>
Book::getMovesToSearch(Position& pos) {
    std::vector<Move> ret;
    MoveList moves;
    MoveGen::pseudoLegalMoves(pos, moves);
    MoveGen::removeIllegal(pos, moves);
    UndoInfo ui;
    for (int i = 0; i < moves.size; i++) {
        const Move& m = moves[i];
        pos.makeMove(m, ui);
        auto bn = getBookNode(pos.bookHash());
        bool include = !bn;
        if (!include) {
            if (!bookData.isPending(pos.bookHash())) {
                if (bn->getNegaMaxScore() == INVALID_SCORE)
                    include = true;
            }
        }
        if (include)
            ret.push_back(m);
        pos.unMakeMove(m, ui);
    }
    return ret;
}

void
Book::addPending(U64 hashKey) {
    bookData.addPending(hashKey);
    auto bn = getBookNode(hashKey);
    assert(bn);
    bn->updateScores(bookData);
}

void
Book::removePending(U64 hashKey) {
    bookData.removePending(hashKey);
    auto bn = getBookNode(hashKey);
    assert(bn);
    bn->updateScores(bookData);
}

void
Book::addPosToBook(Position& pos, const Move& move, std::vector<U64>& toSearch) {
    assert(getBookNode(pos.bookHash()));

    UndoInfo ui;
    pos.makeMove(move, ui);
    U64 childHash = pos.bookHash();
    assert(!getBookNode(childHash));
    auto childNode = std::make_shared<BookNode>(childHash);

    bookNodes[childHash] = childNode;

    toSearch.push_back(pos.bookHash());

    std::size_t bucket = hashToParent.bucket(H2P(childHash, 0));
    auto b = hashToParent.begin(bucket);
    auto e = hashToParent.end(bucket);
    int nParents = 0;
    for (auto p = b; p != e; ++p) {
        if (p->childHash != childHash)
            continue;
        U64 parentHash = p->parentHash;
        BookNode* parent = getBookNode(parentHash);
        assert(parent);
        nParents++;

        Position pos2;
        std::vector<Move> dummyMoves;
        bool ok = getPosition(parentHash, pos2, dummyMoves);
        assert(ok);

        MoveList moves;
        MoveGen::pseudoLegalMoves(pos2, moves);
        MoveGen::removeIllegal(pos2, moves);
        Move move2;
        bool found = false;
        for (int i = 0; i < moves.size; i++) {
            UndoInfo ui2;
            pos2.makeMove(moves[i], ui2);
            if (pos2.bookHash() == childHash) {
                move2 = moves[i];
                found = true;
                break;
            }
            pos2.unMakeMove(moves[i], ui2);
        }
        assert(found);

        parent->addChild(move2.getCompressedMove(), childNode.get());
        childNode->addParent(move2.getCompressedMove(), parent);
        toSearch.push_back(parent->getHashKey());
    }
    assert(nParents > 0);

    setChildRefs(pos);
    childNode->updateScores(bookData);

    pos.unMakeMove(move, ui);
    childNode->setState(BookNode::State::INITIALIZED);
    writeBackup(*childNode);
}

bool
Book::getPosition(U64 hashKey, Position& pos, std::vector<Move>& moveList) const {
    if (hashKey == startPosHash) {
        pos = TextIO::readFEN(TextIO::startPosFEN);
        return true;
    }

    auto node = getBookNode(hashKey);
    if (!node)
        return false;

    int bestErr = INT_MAX;
    BookNode* bestParent = nullptr;
    Move bestMove;
    for (const auto& p : node->getParents()) {
        BookNode* parent = p.parent;
        assert(parent);
        int err = parent->getPathErrorWhite() + parent->getPathErrorBlack();
        if (err < bestErr) {
            bestErr = err;
            bestParent = parent;
            bestMove.setFromCompressed(p.compressedMove);
            assert(!bestMove.isEmpty());
        }
    }
    assert(bestParent);

    bool ok = getPosition(bestParent->getHashKey(), pos, moveList);
    assert(ok);

    UndoInfo ui;
    pos.makeMove(bestMove, ui);
    moveList.push_back(bestMove);
    return true;
}

bool
Book::getBookPV(U64 hashKey, Position& pos,
                std::vector<Move>& movesBefore,
                std::vector<Move>& movesAfter) const {
    std::lock_guard<std::mutex> L(mutex);
    if (!getPosition(hashKey, pos, movesBefore))
        return false;
    BookNode* node = getBookNode(hashKey);
    assert(node);
    Position tmpPos(pos);
    UndoInfo ui;
    while (node && (node->getNegaMaxScore() != INVALID_SCORE)) {
        std::vector<Move> childMoves;
        getOrderedChildMoves(*node, childMoves);
        if (childMoves.empty())
            break;
        int s1 = node->getNegaMaxScore();
        const Move& m = childMoves[0];
        tmpPos.makeMove(m, ui);
        node = getBookNode(tmpPos.bookHash());
        int s2 = node->negateScore(node->getNegaMaxScore());
        if ((s2 == INVALID_SCORE) || (s2 < s1))
            break;
        movesAfter.push_back(m);
    }
    return true;
}


BookNode*
Book::getBookNode(U64 hashKey) const {
    auto it = bookNodes.find(hashKey);
    if (it == bookNodes.end())
        return nullptr;
    return it->second.get();
}

void
Book::initPositions(Position& pos) {
    const U64 hash = pos.bookHash();
    BookNode* node = getBookNode(hash);
    if (!node)
        return;

    setChildRefs(pos);
    for (auto& e : node->getChildren()) {
        if (e.second->getState() == BookNode::DESERIALIZED) {
            UndoInfo ui;
            Move m;
            m.setFromCompressed(e.first);
            pos.makeMove(m, ui);
            initPositions(pos);
            pos.unMakeMove(m, ui);
        }
    }
    node->setState(BookNode::INITIALIZED);
}

void
Book::setChildRefs(Position& pos) {
    const U64 hash = pos.bookHash();
    BookNode* node = getBookNode(hash);
    assert(node);

    MoveList moves;
    MoveGen::pseudoLegalMoves(pos, moves);
    MoveGen::removeIllegal(pos, moves);
    UndoInfo ui;
    for (int i = 0; i < moves.size; i++) {
        pos.makeMove(moves[i], ui);
        U64 childHash = pos.bookHash();
        hashToParent.insert(H2P(childHash, hash));
        BookNode* child = getBookNode(childHash);
        if (child) {
            node->addChild(moves[i].getCompressedMove(), child);
            child->addParent(moves[i].getCompressedMove(), node);
        }
        pos.unMakeMove(moves[i], ui);
    }
}

void
Book::writeBackup(const BookNode& bookNode) {
    if (backupFile.empty())
        return;
    std::ofstream os;
    os.open(backupFile.c_str(), std::ios_base::out |
                                std::ios_base::binary |
                                std::ios_base::app);
    os.exceptions(std::ifstream::failbit | std::ifstream::badbit);
    BookNode::BookSerializeData bsd;
    bookNode.serialize(bsd);
    os.write((const char*)&bsd.data[0], sizeof(bsd.data));
}

void
Book::computeWeights(int maxErrSelf, double errOtherExpConst, WeightInfo& weights) {
    weights.clear();

    std::function<void(const BookNode*,WeightInfo&,int,int)> propagateWeights =
        [&propagateWeights,maxErrSelf,errOtherExpConst]
        (const BookNode* node, WeightInfo& w, int errW, int errB) {
        const double oldWW = w[node->getHashKey()].weightWhite;
        const double oldWB = w[node->getHashKey()].weightBlack;
        double wW = std::max(oldWW, errW <= maxErrSelf ? exp(-errB / errOtherExpConst) : 0.0);
        double wB = std::max(oldWB, errB <= maxErrSelf ? exp(-errW / errOtherExpConst) : 0.0);
        if (wW == oldWW && wB == oldWB)
            return;
        w[node->getHashKey()] = BookWeight(wW, wB);

        for (const auto& p : node->getParents()) {
            BookNode* parent = p.parent;
            assert(parent);
            int eW = std::max(errW, parent->getPathErrorWhite());
            int eB = std::max(errB, parent->getPathErrorBlack());
            propagateWeights(parent, w, eW, eB);
        }
    };

    WeightInfo w;
    for (const auto& e : bookNodes) {
        const BookNode* node = e.second.get();
        auto it = node->getChildren().find(node->getBestNonBookMove().getCompressedMove());
        if ((it != node->getChildren().end()) &&
            (it->second->getNegaMaxScore() != INVALID_SCORE))
            continue;

        int errW, errB;
        getDropoutPathErrors(*node, errW, errB);
        if (errW == INVALID_SCORE || errB == INVALID_SCORE)
            continue;

        w.clear();
        propagateWeights(node, w, errW, errB);

        for (const auto& e2 : w)
            weights[e2.first] += e2.second;
    }

    for (const auto& e : bookNodes) {
        const U64 hKey = e.first;
        if (weights.find(hKey) == weights.end())
            weights.insert(std::make_pair(hKey, BookWeight(0, 0)));
    }
}

void
Book::getDropoutPathErrors(const BookNode& node, int& errW, int& errB) {
    errW = errB = INVALID_SCORE;
    if ((node.getNegaMaxScore() == INVALID_SCORE) ||
        (node.getSearchScore() == INVALID_SCORE) ||
        (node.getSearchScore() == IGNORE_SCORE))
        return;

    U16 cMove = node.getBestNonBookMove().getCompressedMove();
    if (node.getChildren().find(cMove) != node.getChildren().end())
        return;

    errW = node.getPathErrorWhite();
    errB = node.getPathErrorBlack();
    if (errW != INVALID_SCORE && errB != INVALID_SCORE) {
        int delta = node.getNegaMaxScore() - node.getSearchScore();
        assert(delta >= 0);
        if ((node.getDepth() % 2) == 0)
            errW += delta;
        else
            errB += delta;
    }
}

bool
Book::bookMoveOk(const BookNode& node, U16 cMove, int maxErrSelf) const {
    if (node.getNegaMaxScore() == INVALID_SCORE)
        return false;
    int errW = node.getPathErrorWhite();
    int errB = node.getPathErrorBlack();
    if (errW == INVALID_SCORE || errB == INVALID_SCORE)
        return false;
    Move move;
    move.setFromCompressed(cMove);
    int delta;
    if (move.isEmpty()) {
        if (node.getSearchScore() == INVALID_SCORE ||
            node.getSearchScore() == IGNORE_SCORE)
            return false;
        auto it = node.getChildren().find(node.getBestNonBookMove().getCompressedMove());
        if ((it != node.getChildren().end()) &&
            (it->second->getNegaMaxScore() != INVALID_SCORE))
            return false;
        delta = node.getNegaMaxScore() - node.getSearchScore();
    } else {
        auto it = node.getChildren().find(cMove);
        assert(it != node.getChildren().end());
        auto child = it->second;
        if (child->getNegaMaxScore() == INVALID_SCORE)
            return false;
        delta = node.getNegaMaxScore() - BookNode::negateScore(child->getNegaMaxScore());
    }
    assert(delta >= 0);
    if (delta == 0)
        return true; // Best move is always good enough
    const bool wtm = (node.getDepth() % 2) == 0;
    if (wtm) {
        errW += delta;
    } else {
        errB += delta;
    }
    return (wtm ? errW : errB) <= maxErrSelf;
}

void
Book::printBookInfo(Position& pos, const std::vector<Move>& movePath,
                    const WeightInfo& weights,
                    int maxErrSelf, double errOtherExpConst) const {
    std::cout << TextIO::asciiBoard(pos);
    std::cout << TextIO::toFEN(pos) << std::endl;
    std::cout << num2Hex(pos.bookHash()) << std::endl;
    const BookNode* node = getBookNode(pos.bookHash());
    if (!node) {
        std::cout << "not in book" << std::endl;
        return;
    }
    {
        UndoInfo ui;
        std::string moves;
        Position tmpPos = TextIO::readFEN(TextIO::startPosFEN);
        for (const Move& m : movePath) {
            if (!moves.empty())
                moves += ' ';
            moves += TextIO::moveToString(tmpPos, m, false);
            tmpPos.makeMove(m, ui);
        }
        std::cout << (moves.empty() ? "root position" : moves) << std::endl;
    }
    {
        Position tmpPos;
        std::vector<Move> moveList;
        if (!getPosition(pos.bookHash(), tmpPos, moveList)) {
            std::cout << "error, getPosition failed" << std::endl;
            return;
        }
        UndoInfo ui;
        std::string moves;
        tmpPos = TextIO::readFEN(TextIO::startPosFEN);
        for (const Move& m : moveList) {
            if (!moves.empty())
                moves += ' ';
            moves += TextIO::moveToString(tmpPos, m, false);
            tmpPos.makeMove(m, ui);
        }
        std::cout << (moves.empty() ? "root position" : moves) << std::endl;
    }

    auto d2Str = [](double d) {
        std::stringstream ss;
        ss.precision(3);
        ss << d;
        return ss.str();
    };

    std::vector<Move> childMoves;
    getOrderedChildMoves(*node, childMoves);
    for (size_t mi = 0; mi < childMoves.size(); mi++) {
        const Move& childMove = childMoves[mi];
        auto it = node->getChildren().find(childMove.getCompressedMove());
        assert(it != node->getChildren().end());
        const BookNode* child = it->second;
        int negaMaxScore = child->getNegaMaxScore();
        if (pos.isWhiteMove())
            negaMaxScore = BookNode::negateScore(negaMaxScore);
        int expandCostW = node->getExpansionCost(bookData, child, true);
        int expandCostB = node->getExpansionCost(bookData, child, false);
        auto wi = weights.find(child->getHashKey());
        assert(wi != weights.end());
        std::cout << std::setw(2) << mi << ' '
                  << std::setw(6) << TextIO::moveToString(pos, childMove, false) << ' '
                  << std::setw(6) << negaMaxScore << ' '
                  << std::setw(6) << child->getPathErrorWhite() << ' '
                  << std::setw(6) << child->getPathErrorBlack() << ' '
                  << std::setw(6) << expandCostW << ' '
                  << std::setw(6) << expandCostB << ' '
                  << std::setw(10) << d2Str(wi->second.weightWhite) << ' '
                  << std::setw(10) << d2Str(wi->second.weightBlack) << ' '
                  << std::endl;
    }

    std::string moveS = node->getBestNonBookMove().isEmpty() ? "--" :
                        TextIO::moveToString(pos, node->getBestNonBookMove(), false);

    int errW, errB;
    getDropoutPathErrors(*node, errW, errB);

    int expCostW = node->getExpansionCost(bookData, nullptr, true);
    int expCostB = node->getExpansionCost(bookData, nullptr, false);
    double wW = 0.0, wB = 0.0;
    if (errW != INVALID_SCORE && errB != INVALID_SCORE) {
        wW = errW <= maxErrSelf ? exp(-errB / errOtherExpConst) : 0.0;
        wB = errB <= maxErrSelf ? exp(-errW / errOtherExpConst) : 0.0;
    }
    int searchScore = node->getSearchScore();
    if (!pos.isWhiteMove())
        if (searchScore != IGNORE_SCORE && searchScore != INVALID_SCORE)
            searchScore = -searchScore;
    std::cout << "-- "
              << std::setw(6) << moveS << ' '
              << std::setw(6) << searchScore << ' '
              << std::setw(6) << errW << ' '
              << std::setw(6) << errB << ' ';
    if (node->getSearchScore() == IGNORE_SCORE) {
        std::cout << std::setw(6) << "--" << ' '
                  << std::setw(6) << "--" << ' ';
    } else {
        std::cout << std::setw(6) << expCostW << ' '
                  << std::setw(6) << expCostB << ' ';
    }
    std::cout << std::setw(10) << d2Str(wW) << ' '
              << std::setw(10) << d2Str(wB) << ' '
              << std::setw(8) << node->getSearchTime() << std::endl;
}

bool
Book::getTreeData(const Position& pos, TreeData& treeData) const {
    std::lock_guard<std::mutex> L(mutex);

    treeData.parents.clear();
    treeData.children.clear();

    const BookNode* node = getBookNode(pos.bookHash());
    if (!node)
        return false;

    for (const auto& p : node->getParents()) {
        BookNode* parent = p.parent;
        assert(parent);
        Position parentPos;
        std::vector<Move> moveList;
        if (getPosition(parent->getHashKey(), parentPos, moveList)) {
            TreeData::Parent parentData;
            parentData.fen = TextIO::toFEN(parentPos);
            Move move; move.setFromCompressed(p.compressedMove);
            parentData.move = TextIO::moveToString(parentPos, move, true);
            treeData.parents.push_back(parentData);
        }
    }

    std::vector<Move> childMoves;
    getOrderedChildMoves(*node, childMoves);
    for (size_t mi = 0; mi < childMoves.size(); mi++) {
        const Move& childMove = childMoves[mi];
        auto it = node->getChildren().find(childMove.getCompressedMove());
        assert(it != node->getChildren().end());
        const BookNode* child = it->second;
        int negaMaxScore = child->getNegaMaxScore();
        if (pos.isWhiteMove())
            negaMaxScore = BookNode::negateScore(negaMaxScore);

        TreeData::Child childData;
        childData.move = TextIO::moveToString(pos, childMove, false);
        childData.score = negaMaxScore;
        childData.pathErrW = child->getPathErrorWhite();
        childData.pathErrB = child->getPathErrorBlack();
        childData.expandCostW = node->getExpansionCost(bookData, child, true);
        childData.expandCostB = node->getExpansionCost(bookData, child, false);

        treeData.children.push_back(childData);
    }

    int errW, errB;
    getDropoutPathErrors(*node, errW, errB);

    int expCostW = node->getExpansionCost(bookData, nullptr, true);
    int expCostB = node->getExpansionCost(bookData, nullptr, false);
    if (node->getSearchScore() == IGNORE_SCORE)
        expCostW = expCostB = INT_MAX;

    int searchScore = node->getSearchScore();
    if (!pos.isWhiteMove())
        if (searchScore != IGNORE_SCORE && searchScore != INVALID_SCORE)
            searchScore = -searchScore;

    TreeData::Child dropoutData;
    dropoutData.move = node->getBestNonBookMove().isEmpty() ? "--" :
                       TextIO::moveToString(pos, node->getBestNonBookMove(), false);
    dropoutData.score = searchScore;
    dropoutData.pathErrW = errW;
    dropoutData.pathErrB = errB;
    dropoutData.expandCostW = expCostW;
    dropoutData.expandCostB = expCostB;
    treeData.children.push_back(dropoutData);

    treeData.searchTime = node->getSearchTime();
    return true;
}

void
Book::getOrderedChildMoves(const BookNode& node, std::vector<Move>& moves) const {
    std::vector<std::pair<int,U16>> childMoves;
    for (const auto& e : node.getChildren()) {
        Move childMove;
        childMove.setFromCompressed(e.first);
        const BookNode* child = e.second;
        int score = -BookNode::negateScore(child->getNegaMaxScore());
        childMoves.push_back(std::make_pair(score,
                                            childMove.getCompressedMove()));
    }
    std::sort(childMoves.begin(), childMoves.end());
    moves.clear();
    for (const auto& e : childMoves) {
        Move m;
        m.setFromCompressed(e.second);
        moves.push_back(m);
    }
}

void
Book::getQueueData(QueueData& queueData) const {
    queueData.items.clear();
    std::shared_ptr<SearchScheduler> sched = searchScheduler.lock();
    if (sched)
        sched->getQueueData(queueData);
}

// ----------------------------------------------------------------------------

SearchRunner::SearchRunner(int instanceNo0, TranspositionTable& tt0)
    : instanceNo(instanceNo0), tt(tt0), pd(tt), aborted(false) {
}

Move
SearchRunner::analyze(const std::vector<Move>& gameMoves,
                      const std::vector<Move>& movesToSearch,
                      int searchTime) {
    Position pos = TextIO::readFEN(TextIO::startPosFEN);
    UndoInfo ui;
    std::vector<U64> posHashList(200 + gameMoves.size());
    int posHashListSize = 0;
    for (const Move& m : gameMoves) {
        posHashList[posHashListSize++] = pos.zobristHash();
        pos.makeMove(m, ui);
        if (pos.getHalfMoveClock() == 0)
            posHashListSize = 0;
    }

    if (movesToSearch.empty()) {
        MoveList legalMoves;
        MoveGen::pseudoLegalMoves(pos, legalMoves);
        MoveGen::removeIllegal(pos, legalMoves);
        Move bestMove;
        int bestScore = IGNORE_SCORE;
        if (legalMoves.size == 0) {
            if (MoveGen::inCheck(pos))
                bestScore = -SearchConst::MATE0 + 1;
            else
                bestScore = 0; // stalemate
        }
        bestMove.setScore(bestScore);
        return bestMove;
    }

    kt.clear();
    ht.init();
    Search::SearchTables st(tt, kt, ht, et);
    std::shared_ptr<Search> sc;
    {
        std::lock_guard<std::mutex> L(mutex);
        sc = std::make_shared<Search>(pos, posHashList, posHashListSize, st, pd, nullptr, treeLog);
        search = sc;
        int minTimeLimit = aborted ? 0 : searchTime;
        int maxTimeLimit = aborted ? 0 : searchTime;
        sc->timeLimit(minTimeLimit, maxTimeLimit);
    }

    MoveList moveList;
    for (const Move& m : movesToSearch)
        moveList.addMove(m.from(), m.to(), m.promoteTo());

    int maxDepth = -1;
    S64 maxNodes = -1;
    bool verbose = false;
    int maxPV = 1;
    bool onlyExact = true;
    int minProbeDepth = 1;
    Move bestMove = sc->iterativeDeepening(moveList, maxDepth, maxNodes, verbose, maxPV,
                                           onlyExact, minProbeDepth);
    return bestMove;
}

void
SearchRunner::abort() {
    std::lock_guard<std::mutex> L(mutex);
    aborted = true;
    std::shared_ptr<Search> sc = search.lock();
    if (sc)
        sc->timeLimit(0, 0);
}

SearchScheduler::SearchScheduler()
    : stopped(false) {
}

SearchScheduler::~SearchScheduler() {
    abort();
    waitWorkers();
}

void
SearchScheduler::addWorker(std::unique_ptr<SearchRunner> sr) {
    workers.push_back(std::move(sr));
}

void
SearchScheduler::startWorkers() {
    for (auto& w : workers) {
        SearchRunner& sr = *w;
        auto thread = make_unique<std::thread>([this,&sr]() {
            workerLoop(sr);
        });
        threads.push_back(std::move(thread));
    }
}

void
SearchScheduler::abort() {
    std::lock_guard<std::mutex> L(mutex);
    stopped = true;
    for (auto& w : workers)
        w->abort();
    pendingCv.notify_all();
}

bool
SearchScheduler::isAborting() const {
    std::lock_guard<std::mutex> L(mutex);
    return stopped;
}

void
SearchScheduler::waitWorkers() {
    for (auto& t : threads) {
        t->join();
        t.reset();
    }
}

void
SearchScheduler::addWorkUnit(const WorkUnit& wu) {
    std::lock_guard<std::mutex> L(mutex);
    bool empty = pending.empty();
    pending.push_back(wu);
    if (empty)
        pendingCv.notify_all();
}

void
SearchScheduler::getResult(WorkUnit& wu) {
    std::unique_lock<std::mutex> L(mutex);
    while (complete.empty())
        completeCv.wait(L);
    wu = complete.front();
    complete.pop_front();
}

void
SearchScheduler::reportResult(const WorkUnit& wu) const {
    Position pos = TextIO::readFEN(TextIO::startPosFEN);
    UndoInfo ui;
    std::string moves;
    for (const Move& m : wu.gameMoves) {
        if (!moves.empty())
            moves += ' ';
        moves += TextIO::moveToString(pos, m, false);
        pos.makeMove(m, ui);
    }
    std::set<std::string> excluded;
    MoveList legalMoves;
    MoveGen::pseudoLegalMoves(pos, legalMoves);
    MoveGen::removeIllegal(pos, legalMoves);
    for (int i = 0; i < legalMoves.size; i++) {
        const Move& m = legalMoves[i];
        if (!contains(wu.movesToSearch, m))
            excluded.insert(TextIO::moveToString(pos, m, false));
    }
    std::string excludedS;
    for (const std::string& m : excluded) {
        if (!excludedS.empty())
            excludedS += ' ';
        excludedS += m;
    }

    int score = wu.bestMove.score();
    if (!pos.isWhiteMove())
        score = -score;

    std::string bestMove = wu.bestMove.isEmpty() ? "--" :
                           TextIO::moveToString(pos, wu.bestMove, false);

    std::cout << std::setw(5) << std::right << wu.id << ' '
              << std::setw(6) << std::right << score << ' '
              << std::setw(6) << std::left  << bestMove << ' '
              << std::setw(6) << std::right << wu.searchTime << " : "
              << moves << " : "
              << excludedS << " : "
              << TextIO::toFEN(pos)
              << std::endl;
}

void
SearchScheduler::workerLoop(SearchRunner& sr) {
    while (true) {
        WorkUnit wu;
        QueueItem item;
        {
            std::unique_lock<std::mutex> L(mutex);
            while (!stopped && pending.empty())
                pendingCv.wait(L);
            if (stopped && pending.empty())
                return;
            wu = pending.front();
            pending.pop_front();

            item.hashKey = wu.hashKey;
            item.startTime = std::chrono::system_clock::now();
            item.completed = false;
            runningItems[sr.instNo()] = item;
        }
        wu.bestMove = sr.analyze(wu.gameMoves, wu.movesToSearch, wu.searchTime);
        wu.instNo = sr.instNo();
        {
            std::unique_lock<std::mutex> L(mutex);
            bool empty = complete.empty();
            complete.push_back(wu);
            if (empty)
                completeCv.notify_all();

            runningItems.erase(sr.instNo());
            item.completed = true;
            finishedItems.push_back(item);
            while (finishedItems.size() > 10)
                finishedItems.pop_front();
        }
    }
}

void
SearchScheduler::getQueueData(Book::QueueData& queueData) const {
    std::lock_guard<std::mutex> L(mutex);
    for (auto& e : runningItems)
        queueData.items.push_back(e.second);
    std::sort(queueData.items.begin(), queueData.items.end(),
              [](const QueueItem& a, const QueueItem& b) { return a.startTime < b.startTime; });
    queueData.items.insert(queueData.items.begin(), finishedItems.begin(), finishedItems.end());
}

} // Namespace BookBuild
