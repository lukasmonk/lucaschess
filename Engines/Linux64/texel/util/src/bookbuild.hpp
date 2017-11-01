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
 * bookbuild.hpp
 *
 *  Created on: Apr 27, 2014
 *      Author: petero
 */

#ifndef BOOKBUILD_HPP_
#define BOOKBUILD_HPP_

#include "move.hpp"
#include "position.hpp"
#include "treeLogger.hpp" // Serializer namespace

#include <memory>
#include <atomic>
#include <vector>
#include <deque>
#include <unordered_set>
#include <unordered_map>
#include <map>
#include <climits>
#include <chrono>

class BookBuildTest;
class GameNode;


namespace BookBuild {

class SearchScheduler;

// Node is temporarily ignored because it is currently being searched
const int IGNORE_SCORE = SearchConst::UNKNOWN_SCORE + 1;

// Used in when no search score has been computed
const int INVALID_SCORE = SearchConst::UNKNOWN_SCORE + 2;


/** Global book data needed by per book node computations. */
class BookData {
public:
    BookData(int bookDepthCost, int ownPathErrorCost, int otherPathErrorCost)
        : bookDepthC(bookDepthCost), ownPathErrorC(ownPathErrorCost),
          otherPathErrorC(otherPathErrorCost) {}

    int bookDepthCost() const { return bookDepthC; }
    int ownPathErrorCost() const { return ownPathErrorC; }
    int otherPathErrorCost() const { return otherPathErrorC; }

    void clearPending() { pendingPositions.clear(); }
    void addPending(U64 hashKey) { pendingPositions.insert(hashKey); }
    void removePending(U64 hashKey) { pendingPositions.erase(hashKey); }

    bool isPending(U64 hashKey) const {
        return pendingPositions.find(hashKey) != pendingPositions.end();
    }

private:
    std::set<U64> pendingPositions; // Positions currently being searched
    const int bookDepthC;      // Cost per existing book depth for extending a book line one ply
    const int ownPathErrorC;   // Cost for extending a move where the book player plays inaccurate
    const int otherPathErrorC; // Cost for extending a move where the opponent plays inaccurate
};

/**
 * The BookNode class represents a book position, its connections to parent/child book
 * positions, and information about the best non-book move.
 *
 * Each node has a search score (searchScore), which has one of the following values:
 * - INVALID_SCORE : No search has been performed for this position.
 * - IGNORE_SCORE  : There is at least one legal move in this position, but all legal
 *                   moves already have a corresponding BookNode with a score that is
 *                   not INVALID_SCORE.
 * - A value returned by iterative deepening search. This value can be MATE0 or 0 if
 *   the game is over (lost or draw), or a mate or heuristic search score.
 *   The iterative deepening search must include at least all legal moves that don't
 *   correspond to a child node or corresponds to a child node with INVALID_SCORE.
 *   Mate scores are always correct in the sense that they identify the side that wins
 *   assuming optimal play, but the search is not guaranteed to have found the fastest
 *   possible mate.
 *
 * Each node has a negaMaxScore which is computed as follows:
 * - If searchScore is INVALID_SCORE, the negaMaxScore is also INVALID_SCORE.
 * - Otherwise, if the node is a leaf node, the negaMaxScore is equal to searchScore.
 * - Otherwise, the negaMaxScore is the maximum of searchScore and
 *   negateScore(child[i].negaMaxScore), taken over all child nodes.
 * Note that negaMaxScore can never be equal to IGNORE_SCORE.
 *
 * Each node has a book expansion cost for the white player. (It also has a book expansion
 * cost for the black player that works in an analogous way.) The best node to add to the
 * opening book is determined by starting at the root node and repeatedly selecting a
 * child node with the smallest expansion cost. The expansion cost is defined as follows:
 * - If the node is a leaf node, the expansion cost is:
 *   - IGNORE_SCORE if the node is currently being searched.
 *   - INVALID_SCORE if negaMaxScore is INVALID_SCORE.
 *   - 0 if negaMaxScore is not INVALID_SCORE.
 * - If the expansion cost of any child node is INVALID_SCORE, the expansion cost is
 *   INVALID_SCORE.
 * - If searchScore is INVALID_SCORE, the expansion cost is INVALID_SCORE.
 * - Otherwise, the expansion cost is the smallest of:
 *   - k * moveError, where:
 *      k = k1 if the book player is to move,
 *      k = k2 if the other player is to move
 *     moveError = negaMaxScore - searchScore (always >= 0)
 *   - ka + child[i].expansionCost + kb * moveError, where:
 *      ka = k3 if child[i].expansionCost is not ignore or invalid
 *      ka = 0  otherwise
 *      kb = k1 if the book player is to move and child[i].expansionCost is not ignore or invalid
 *      kb = k2 if the other player is to move and child[i].expansionCost is not ignore or invalid
 *      kb = 0 otherwise
 *      moveError = negaMaxScore - negateScore(child[i].negaMaxScore)
 *   Choices corresponding to nodes currently being searched are ignored. If all choices
 *   are ignored the expansion cost is IGNORE_SCORE.
 *   k1, k2 and k3 are positive constants that control how book depth, own errors and
 *   opponent errors are weighted when deciding what node to expand next.
 */
class BookNode {
public:
    /** Create an empty node. */
    BookNode(U64 hashKey, bool rootNode = false);

    BookNode(const BookNode& other) = delete;
    BookNode& operator=(const BookNode& other) = delete;

    /** Return book hash key. */
    U64 getHashKey() const;

    /** Return shortest distance to the root node. */
    int getDepth() const;

    /** Get negamax scores. */
    int getNegaMaxScore() const;

    /** Return the best expansion cost for this node. */
    int getExpansionCostWhite() const;
    int getExpansionCostBlack() const;

    /** Return smallest path error from root to this node. */
    int getPathErrorWhite() const;
    int getPathErrorBlack() const;

    /** Compute expansion cost for a child node, or for this node if child is null. */
    int getExpansionCost(const BookData& bookData, const BookNode* child,
                         bool white) const;

    struct BookSerializeData {
        U8 data[16];
    };

    /** Serialize/deserialize object. */
    void serialize(BookSerializeData& bsd) const;
    void deSerialize(const BookSerializeData& bsd);
    void setRootNode();

    /** Add a parent/child relationship. */
    void addChild(U16 move, BookNode* child);
    void addParent(U16 move, BookNode* parent);

    /** Set search result data. */
    void setSearchResult(const BookData& bookData,
                         const Move& bestMove, int score, int searchTime);

    enum State {
        EMPTY,             // Newly constructed, node contains no useful data
        DESERIALIZED,      // Deserialized but non-serialized data not initialized
        INITIALIZED,       // All data initialized
    };

    State getState() const;
    void setState(State s);

    /** Recursively initialize scores (negamax, expansion costs, path errors)
     *  of this node and all children and parents. */
    void updateScores(const BookData& bookData);

    /** Get all children. */
    const std::map<U16, BookNode*>& getChildren() const { return children; }

    struct ParentInfo {
        ParentInfo(U16 cMove, BookNode* p = nullptr)
            : compressedMove(cMove), parent(p) {}

        bool operator<(const ParentInfo& other) const {
            if (compressedMove != other.compressedMove)
                return compressedMove < other.compressedMove;
            std::less<BookNode*> ol;
            return ol(parent, other.parent);
        }

        U16 compressedMove;
        BookNode* parent;
    };

    /** Get all parents. */
    const std::set<ParentInfo>& getParents() const { return parents; }

    const Move& getBestNonBookMove() const;
    const S16 getSearchScore() const;
    const U32 getSearchTime() const;

    /** Negate a score for a child node to produce the corresponding score
     *  for the parent node. Also handles special scores, i.e. mate, invalid, ignore. */
    static int negateScore(int score);

private:
    /** Update depth of this node and all descendants. */
    void updateDepth();

    /** Compute negaMax scores for this node assuming all child nodes are already up to date.
     * Return true if any score was modified. */
    bool computeNegaMax(const BookData& bookData);

    /** Compute path error for this node assuming all parent nodes are already up to date.
     * Return true if white or black path error was modified. */
    bool computePathError(const BookData& bookData);


    U64 hashKey;
    int depth;              // Length of shortest path to the root node

    Move bestNonBookMove;   // Best non-book move. Empty if all legal moves are
                            // included in the book.
    S16 searchScore;        // Score for best non-book move.
                            // IGNORE_SCORE, -MATE0 or 0 (stalemate) if no non-book move.
    U32 searchTime;         // Time in milliseconds spent on computing searchScore and bestNonBookMove

    int negaMaxScore;       // Best score in this position. max(searchScore, -child_i(pos))
    int expansionCostWhite; // Smallest expansion cost for white
    int expansionCostBlack; // Smallest expansion cost for black

    int pathErrorWhite;     // Smallest path error for white from root to this node
    int pathErrorBlack;     // Smallest path error for black from root to this node

    std::map<U16, BookNode*> children; // Compressed move -> BookNode
    std::set<ParentInfo> parents;      // Compressed move -> BookNode
    State state;
};

/** Represents an opening book and methods that can improve the book
 *  by extension and engine analysis. */
class Book {
    friend class ::BookBuildTest;
public:
    /** Constructor. Create an empty book.
     * @param backupFile The backup path name. Empty string disables backup. */
    Book(const std::string& backupFile, int bookDepthCost = 100,
         int ownPathErrorCost = 200, int otherPathErrorCost = 50);

    Book(const Book& other) = delete;
    Book& operator=(const Book& other) = delete;

    class Listener {
    public:
        virtual ~Listener() {}
        virtual void queueChanged(int nPendingBookTasks) = 0;
    };
    void setListener(std::unique_ptr<Listener> listener);

    /** Improve the opening book. If startMoves is a non-empty string, only improve the part
     * of the book rooted at the position obtained after playing those moves.
     * This function does not return until no more book moves can be added, which in
     * practice never happens unless startMoves leads to a position not in the book. */
    void improve(const std::string& bookFile, int searchTime, int numThreads,
                 const std::string& startMoves);

    /** Improve the opening book. It is possible to dynamically change which
     * subtree of the book to improve. */
    void interactiveExtendBook(int searchTime, int numThreads,
                               TranspositionTable& tt,
                               const std::atomic<U64>& startHash,
                               const std::atomic<bool>& stopFlag);

    /** Stop improving the opening book as soon as possible. */
    void abortExtendBook();

    /** Add moves from a PGN file to the book. */
    void importPGN(const std::string& bookFile, const std::string& pgnFile, int maxPly);

    /** Add all moves in a game tree up to ply maxPly to the book. */
    void addToBook(int maxPly, GameNode& gn, int& nAdded);

    /** Convert the book to polyglot format. */
    void exportPolyglot(const std::string& bookFile, const std::string& polyglotFile,
                        int maxErrSelf, double errOtherExpConst);

    /** Query the book interactively, taking query commands from standard input. */
    void interactiveQuery(const std::string& bookFile, int maxErrSelf, double errOtherExpConst);

    /** Compute and print statistics about the book. */
    void statistics(const std::string& bookFile);


    /** Read opening book from file. */
    void readFromFile(const std::string& filename);

    /** Write opening book to file. */
    void writeToFile(const std::string& filename);


    /** Given a hash key, retrieve the corresponding book position,
     * the best moves leading to the position, and the best continuation
     * from the position. Return false if hashKey is not a book position. */
    bool getBookPV(U64 hashKey, Position& pos,
                   std::vector<Move>& movesBefore,
                   std::vector<Move>& movesAfter) const;

    class PositionSelector {
    public:
        virtual ~PositionSelector() {}
        /** Retrieve position and move which can be used to extend the opening book.
         * If move is empty, don't extend book, only search non-book moves in pos.
         * Return true if a position was retrieved, false otherwise. */
        virtual bool getNextPosition(Position& pos, Move& move) = 0;
    };
    friend class DropoutSelector;

    struct TreeData {
        struct Parent {
            std::string fen;  // Parent position
            std::string move; // Move from parent position leading to current position
        };
        std::vector<Parent> parents;

        struct Child {
            std::string move;  // Book move
            int score;         // Negamax score from white's point of view
            int pathErrW;      // Accumulated white path error
            int pathErrB;      // Accumulated black path error
            int expandCostW;   // Expansion cost when building book for white
            int expandCostB;   // Expansion cost when building book for black
        };
        std::vector<Child> children; // Child moves, including the dropout move
        int searchTime;        // Search time in ms for the dropout move
    };

    /** Get information about the book node given by "pos". */
    bool getTreeData(const Position& pos, TreeData& treeData) const;

    struct QueueData {
        struct Item {
            using TimePoint = std::chrono::time_point<std::chrono::system_clock>;
            U64 hashKey;           // Hash key for root position of search
            TimePoint startTime;   // Time (in ms since epoch) when search started
            bool completed;        // True if search is completed
        };
        std::vector<Item> items;
    };

    /** Get information about the currently running search jobs,
     *  and the last N completed jobs. */
    void getQueueData(QueueData& queueData) const;

private:
    /** Add root node if not already present. */
    void addRootNode();

    /** Extend book using positions provided by the selector. */
    void extendBook(PositionSelector& selector, int searchTime, int numThreads,
                    TranspositionTable& tt);

    /** Get the list of legal moves to include in the search. */
    std::vector<Move> getMovesToSearch(Position& pos);

    /** Add/remove a position to the set of positions currently being searched. */
    void addPending(U64 hashKey);
    void removePending(U64 hashKey);

    /** Add the position resulting from playing "move" in position "pos" to the
     * book. The new position and its parent position(s) that need to be
     * re-searched are returned in the "toSearch" vector. "pos" must already be
     * in the book and "pos + move" must not already be in the book. */
    void addPosToBook(Position& pos, const Move& move, std::vector<U64>& toSearch);

    /**
     * Retrieve the position corresponding to a hash key. Only works for positions
     * that are included in the opening book. Returns true if the position could
     * be retrieved, false otherwise. "pos" may be modified even if the function
     * returns false.
     */
    bool getPosition(U64 hashKey, Position& pos, std::vector<Move>& moveList) const;

    /** Get the book node corresponding to a hash key.
     * Return null if there is no matching node in the book. */
    BookNode* getBookNode(U64 hashKey) const;

    /** Initialize parent/child relations in all book nodes
     *  by following legal moves from pos. */
    void initPositions(Position& pos);

    /** Find all children of pos in book and update parent/child pointers. */
    void setChildRefs(Position& pos);

    /** Write a book node to the backup file. */
    void writeBackup(const BookNode& bookNode);

    struct BookWeight {
        BookWeight(double wW = 0.0, double wB = 0.0) : weightWhite(wW), weightBlack(wB) {}
        BookWeight& operator+=(const BookWeight& bw) {
            weightWhite += bw.weightWhite;
            weightBlack += bw.weightBlack;
            return *this;
        }
        double weightWhite; // Weight when book player is white
        double weightBlack; // Weight when book player is black
    };
    using WeightInfo = std::unordered_map<U64,BookWeight>;

    /** Compute book weights for all nodes in the tree. weightWhite for a node is computed as:
     *    weightWhite = sum(exp(-errB / errOtherExpConst))
     *  where the sum is taken over all descendant leaf nodes with errW <= maxErrSelf
     *  and errW,errB is the maximum path error for all nodes from this node to the leaf node.
     *  weightBlack is computed in an analogous way. */
    void computeWeights(int maxErrSelf, double errOtherExpConst,
                        WeightInfo& weights);

    /** Compute accumulated white/black path errors for the dropout move of a node.
     *  If the dropout move is not valid, errW and errB are set to INVALID_SCORE. */
    static void getDropoutPathErrors(const BookNode& node, int& errW, int& errB);

    /** Decide if a move in a position is good enough to be a book move. */
    bool bookMoveOk(const BookNode& node, U16 cMove, int maxErrSelf) const;

    /** Print book information for a position to cout. */
    void printBookInfo(Position& pos, const std::vector<Move>& movePath,
                       const WeightInfo& weights,
                       int maxErrSelf, double errOtherExpConst) const;

    /** Get vector of moves corresponding to child nodes, ordered from best to worst move. */
    void getOrderedChildMoves(const BookNode& node, std::vector<Move>& moves) const;


    /** Hash key corresponding to initial position. */
    const U64 startPosHash;

    /** Filename where all incremental improvements are stored.
     * The backup file is a valid book file at all times. */
    std::string backupFile;

    /** All positions in the opening book. */
    std::unordered_map<U64, std::shared_ptr<BookNode>> bookNodes;

    /** Map from position hash code to all parent book position hash codes. */
    struct H2P {
        H2P(U64 c, U64 p) : childHash(c), parentHash(p) {}
        bool operator==(const H2P& o) const {
            return childHash == o.childHash && parentHash == o.parentHash;
        }
        struct HashFun { // Make sure all elements with same childHash end up in the same bucket
            std::size_t operator()(const H2P& h) const { return h.childHash; }
        };
        U64 childHash;
        U64 parentHash;
    };
    std::unordered_set<H2P, H2P::HashFun> hashToParent;

    BookData bookData;

    /** Protect concurrent read/write access to the book. */
    mutable std::mutex mutex;

    std::weak_ptr<SearchScheduler> searchScheduler;

    /** Handle notifications when book is changed. */
    std::unique_ptr<Listener> listener;
};

/** Calls Search::iterativeDeepening() to analyze a position. */
class SearchRunner {
public:
    /** Constructor. */
    SearchRunner(int instanceNo, TranspositionTable& tt);

    SearchRunner(const SearchRunner& other) = delete;
    SearchRunner& operator=(const SearchRunner& other) = delete;

    /** Analyze position and return the best move and score. */
    Move analyze(const std::vector<Move>& gameMoves,
                 const std::vector<Move>& movesToSearch,
                 int searchTime);

    /** Stop search as soon as possible. */
    void abort();

    int instNo() const { return instanceNo; }

private:
    int instanceNo;
    Evaluate::EvalHashTables et;
    KillerTable kt;
    History ht;
    TranspositionTable& tt;
    ParallelData pd;
    TreeLogger treeLog;
    std::weak_ptr<Search> search;

    std::mutex mutex;
    bool aborted;
};

/** Handles work distribution to the search threads. */
class SearchScheduler {
public:
    /** Constructor. */
    SearchScheduler();

    /** Destructor. Waits for all threads to terminate. */
    ~SearchScheduler();

    /** Add a SearchRunner. */
    void addWorker(std::unique_ptr<SearchRunner> sr);

    /** Start the worker threads. Creates one thread for each SearchRunner object. */
    void startWorkers();

    /** Stop worker threads as soon as possible. */
    void abort();

    /** Return true if worker threads are being aborted. */
    bool isAborting() const;

    struct WorkUnit {
        // Input
        int id;
        U64 hashKey;
        std::vector<Move> gameMoves;     // Moves leading to the position to search
        std::vector<Move> movesToSearch; // Set of moves to consider in the search
        int searchTime;

        // Output
        Move bestMove;         // Best move and corresponding score
        int instNo;            // Instance number that ran this WorkUnit

        bool operator<(const WorkUnit& other) const { return id < other.id; }
    };

    /** Add a WorkUnit to the queue. */
    void addWorkUnit(const WorkUnit& wu);

    /** Wait until a result is ready and retrieve the corresponding WorkUnit. */
    void getResult(WorkUnit& wu);

    /** Report finished WorkUnit information to cout. */
    void reportResult(const WorkUnit& wu) const;

    /** Get information about the currently running search jobs,
     *  and the last N completed jobs. */
    void getQueueData(Book::QueueData& queueData) const;

private:
    /** Worker thread main loop. */
    void workerLoop(SearchRunner& sr);

    /** Wait for all WorkUnits to finish and then stops all threads. */
    void waitWorkers();

    bool stopped;
    mutable std::mutex mutex;

    std::vector<std::unique_ptr<SearchRunner>> workers;
    std::vector<std::unique_ptr<std::thread>> threads;

    std::deque<WorkUnit> pending;
    std::condition_variable pendingCv;

    std::deque<WorkUnit> complete;
    std::condition_variable completeCv;

    // Information about current search jobs
    using QueueItem = Book::QueueData::Item;
    std::map<int, QueueItem> runningItems;
    std::deque<QueueItem> finishedItems;
};

// ----------------------------------------------------------------------------

inline
BookNode::BookNode(U64 hashKey0, bool rootNode)
    : hashKey(hashKey0), depth(INT_MAX),
      searchScore(INVALID_SCORE), searchTime(0),
      negaMaxScore(INVALID_SCORE),
      expansionCostWhite(INVALID_SCORE),
      expansionCostBlack(INVALID_SCORE),
      pathErrorWhite(INVALID_SCORE),
      pathErrorBlack(INVALID_SCORE),
      state(BookNode::EMPTY) {
    if (rootNode)
        setRootNode();
}

inline U64
BookNode::getHashKey() const {
    return hashKey;
}

inline int
BookNode::getDepth() const {
    return depth;
}

inline int
BookNode::getNegaMaxScore() const {
    return negaMaxScore;
}

inline int
BookNode::getExpansionCostWhite() const {
    return expansionCostWhite;
}

inline int
BookNode::getExpansionCostBlack() const {
    return expansionCostBlack;
}

inline int
BookNode::getPathErrorWhite() const {
    return pathErrorWhite;
}

inline int
BookNode::getPathErrorBlack() const {
    return pathErrorBlack;
}

inline void
BookNode::serialize(BookSerializeData& bsd) const {
    U16 move = bestNonBookMove.getCompressedMove();
    Serializer::serialize<sizeof(bsd.data)>(bsd.data, hashKey, move, searchScore, searchTime);
}

inline void
BookNode::deSerialize(const BookSerializeData& bsd) {
    U16 move;
    Serializer::deSerialize<sizeof(bsd.data)>(bsd.data, hashKey, move, searchScore, searchTime);
    bestNonBookMove.setFromCompressed(move);
    state = DESERIALIZED;
}

inline void
BookNode::setRootNode() {
    depth = 0;
    pathErrorWhite = 0;
    pathErrorBlack = 0;
}

inline void
BookNode::addChild(U16 move, BookNode* child) {
    children.insert(std::make_pair(move, child));
}

inline void
BookNode::addParent(U16 move, BookNode* parent) {
    parents.insert(ParentInfo(move, parent));
    updateDepth();
}

inline BookNode::State
BookNode::getState() const {
    return state;
}

inline void
BookNode::setState(State s) {
    state = s;
}

inline const Move&
BookNode::getBestNonBookMove() const {
    return bestNonBookMove;
}

inline const S16
BookNode::getSearchScore() const {
    return searchScore;
}

inline const U32
BookNode::getSearchTime() const {
    return searchTime;
}


inline void
Book::setListener(std::unique_ptr<Listener> listener0) {
    listener = std::move(listener0);
}

} // Namespace BookBuild

#endif /* BOOKBUILD_HPP_ */
