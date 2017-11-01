/*
    Texel - A UCI chess engine.
    Copyright (C) 2012-2016  Peter Österlund, peterosterlund2@gmail.com

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
 * search.hpp
 *
 *  Created on: Feb 25, 2012
 *      Author: petero
 */

#ifndef SEARCH_HPP_
#define SEARCH_HPP_

#include "constants.hpp"
#include "position.hpp"
#include "evaluate.hpp"
#include "moveGen.hpp"
#include "searchUtil.hpp"
#include "parallel.hpp"
#include "parameters.hpp"
#include "util/util.hpp"

#include <limits>
#include <memory>


class SearchTest;
class ChessTool;
class PosGenerator;
class ClusterTT;
class History;
class KillerTable;


/** Implements the NegaScout search algorithm. */
class Search {
    friend class SearchTest;
    friend class ChessTool;
    friend class PosGenerator;
public:
    /** Help tables used by the search. */
    struct SearchTables {
        SearchTables(ClusterTT& tt0, KillerTable& kt0, History& ht0,
                     Evaluate::EvalHashTables& et0);
        ClusterTT& tt;
        KillerTable& kt;
        History& ht;
        Evaluate::EvalHashTables& et;
    };

    /** Constructor. */
    Search(const Position& pos, const std::vector<U64>& posHashList,
           int posHashListSize, SearchTables& st, Communicator& comm,
           TreeLogger& logFile);

    Search(const Search& other) = delete;
    Search& operator=(const Search& other) = delete;

    /** Initializes the searchTreeInfo array. */
    void initSearchTreeInfo();

    /** Interface for reporting search information during search. */
    class Listener {
    public:
        virtual ~Listener() {}
        virtual void notifyDepth(int depth) = 0;
        virtual void notifyCurrMove(const Move& m, int moveNr) = 0;
        virtual void notifyPV(int depth, int score, S64 time, S64 nodes, S64 nps,
                              bool isMate, bool upperBound, bool lowerBound,
                              const std::vector<Move>& pv, int multiPVIndex,
                              S64 tbHits) = 0;
        virtual void notifyStats(S64 nodes, S64 nps, int hashFull, S64 tbHits, S64 time) = 0;
    };

    void setListener(Listener& listener);

    /** Exception thrown to stop the search. */
    class StopSearch : public std::exception {
    };

    class StopHandler {
    public:
        virtual ~StopHandler() {}
        virtual bool shouldStop() = 0;
    };

    void setStopHandler(std::unique_ptr<StopHandler> stopHandler);

    /** Set which thread is owning this Search object. */
    void setThreadNo(int tNo);

    void timeLimit(int minTimeLimit, int maxTimeLimit, int earlyStopPercent = -1);

    void setStrength(int strength, U64 randomSeed);

    /** Set minimum depth for TB probes. */
    void setMinProbeDepth(int depth);

    Move iterativeDeepening(const MoveList& scMovesIn,
                            int maxDepth, S64 initialMaxNodes,
                            int maxPV = 1, bool onlyExact = false,
                            int minProbeDepth = 0, bool clearHistory = false);

    /**
     * Main recursive search algorithm.
     * @return Score for the side to make a move, in position given by "pos".
     */
    template <bool tb>
    int negaScout(int alpha, int beta, int ply, int depth, int recaptureSquare,
                  const bool inCheck);
    int negaScout(bool tb,
                  int alpha, int beta, int ply, int depth, int recaptureSquare,
                  const bool inCheck);

    /** Compute extension depth for a move. */
    int getMoveExtend(const Move& m, int recaptureSquare);

    static bool canClaimDraw50(const Position& pos);

    static bool canClaimDrawRep(const Position& pos, const std::vector<U64>& posHashList,
                                int posHashListSize, int posHashFirstNew);

    /**
     * Compute scores for each move in a move list, using SEE, killer and history information.
     * @param moves  List of moves to score.
     */
    void scoreMoveList(MoveList& moves, int ply, int startIdx = 0);

    /** Set search tree information for a given ply. */
    void setSearchTreeInfo(int ply, const SearchTreeInfo& sti, U64 rootNodeIdx);

    /** Get total number of nodes searched by this thread. */
    S64 getTotalNodesThisThread() const;

    /** Get number of TB hits for this thread. */
    S64 getTbHitsThisThread() const;

    /**
     * Static exchange evaluation function.
     * @return SEE score for m. Positive value is good for the side that makes the first move.
     */
    static int SEE(Position& pos, const Move& m, int alpha, int beta);

private:
    void init(const Position& pos0, const std::vector<U64>& posHashList0,
              int posHashListSize0);

    int negaScoutRoot(bool tb, int alpha, int beta, int ply, int depth,
                      const bool inCheck);

    /** Information used for move ordering at root and for PV reporting. */
    struct MoveInfo {
        Move move;
        U64 nodes;
        bool knownLoss;
        int depth, alpha, beta;
        std::vector<Move> pv;
        MoveInfo(const Move& m, int n)
            : move(m), nodes(n), knownLoss(false), depth(0), alpha(0), beta(0) {}

        int score() const { return move.score(); }

        struct SortByScore {
            bool operator()(const MoveInfo& mi1, const MoveInfo& mi2) const {
                return mi1.move.score() > mi2.move.score();
            }
        };
        struct SortByNodes {
            bool operator()(const MoveInfo& mi1, const MoveInfo& mi2) {
                return mi1.nodes > mi2.nodes;
            }
        };
    };

    /** Store depth, alpha, beta, score and pv in scMoves[mi]. */
    void storeSearchResult(std::vector<MoveInfo>& scMoves, int mi, int depth,
                           int alpha, int beta, int score);

    /** Report PV information to listener. */
    void notifyPV(const std::vector<MoveInfo>& moveInfo, int mi, int maxPV);
    void notifyPV(const MoveInfo& info, int multiPVIndex);

    /** Report search statistics to listener. */
    void notifyStats();

    /** Get total number of nodes searched by all threads. */
    S64 getTotalNodes() const;

    /** Get total number of TB hits for all threads. */
    S64 getTbHits() const;

    /** Determine which root moves to search, taking low strength and
     *  missing TB files into account. */
    void getRootMoves(const MoveList& rootMovesIn,
                      std::vector<MoveInfo>& rootMovesOut,
                      int maxDepth);

    /** Return true if move should be skipped in order to make engine play weaker. */
    bool weakPlaySkipMove(const Position& pos, const Move& m, int ply) const;

    static bool passedPawnPush(const Position& pos, const Move& m);

    /** Return true if m moves a piece away from a threat. */
    static bool defenseMove(const Position& pos, const Move& m);

    /** Return true if the current node at ply is an expected cut node. */
    bool isExpectedCutNode(int ply) const;

    /** Quiescence search. Only non-losing captures are searched. */
    int quiesce(int alpha, int beta, int ply, int depth, const bool inCheck);

    /**
     * Static exchange evaluation function.
     * @return SEE score for m. Positive value is good for the side that makes the first move.
     */
    int SEE(const Move& m, int alpha, int beta);

    /** Return >0, 0, <0, depending on the sign of SEE(m). */
    int signSEE(const Move& m);

    /** Return true if SEE(m) < 0. */
    bool negSEE(const Move& m);

    /** Score move list according to most valuable victim / least valuable attacker. */
    void scoreMoveListMvvLva(MoveList& moves) const;

    /** Find move with highest score and move it to the front of the list. */
    static void selectBest(MoveList& moves, int startIdx);

    /** If hashMove exists in the move list, move the hash move to the front of the list. */
    static bool selectHashMove(MoveList& moves, const Move& hashMove);

    class DefaultStopHandler : public StopHandler {
    public:
        explicit DefaultStopHandler(Search& sc0) : sc(sc0) { }
        bool shouldStop() override { return sc.shouldStop(); }
    private:
        Search& sc;
    };

    /** Return true if the search should be stopped immediately. */
    bool shouldStop();


    Position pos;
    Evaluate eval;
    KillerTable& kt;
    History& ht;
    std::vector<U64> posHashList; // List of hashes for previous positions up to the last "zeroing" move.
    int posHashListSize;          // Number of used entries in posHashList
    int posHashFirstNew;          // First entry in posHashList that has not been played OTB.
    ClusterTT& tt;
    Communicator& comm;
    int jobId = 0;
    int threadNo;
    TreeLogger& logFile;

    Listener* listener = nullptr;
    std::unique_ptr<StopHandler> stopHandler;
    Move emptyMove;

    SearchTreeInfo searchTreeInfo[SearchConst::MAX_SEARCH_DEPTH * 2];

    // Time management
    S64 tStart;                // Time when search started
    RelaxedShared<S64> minTimeMillis; // Minimum recommended thinking time
    RelaxedShared<S64> maxTimeMillis; // Maximum allowed thinking time
    int earlyStopPercentage;   // Can stop searching after this many percent of minTimeMillis
    bool searchNeedMoreTime;   // True if negaScout should use up to maxTimeMillis time.
    S64 maxNodes;              // Maximum number of nodes to search (approximately)
    int minProbeDepth;         // Minimum depth to probe endgame tablebases.
    int nodesToGo;             // Number of nodes until next time check
    int nodesBetweenTimeCheck; // How often to check remaining time

    // Reduced strength variables
    int strength;              // Strength (0-1000)
    bool weak;                 // True if strength < 1000
    U64 randomSeed;

    // Search statistics stuff
    S64 totalNodes;
    S64 tbHits;
    S64 tLastStats;        // Time when notifyStats was last called

    int q0Eval; // Static eval score at first level of quiescence search
};

inline
Search::SearchTables::SearchTables(ClusterTT& tt0, KillerTable& kt0, History& ht0,
                                   Evaluate::EvalHashTables& et0)
    : tt(tt0), kt(kt0), ht(ht0), et(et0) {
}

inline void
Search::setListener(Listener& listener) {
    this->listener = &listener;
}

inline void
Search::setStopHandler(std::unique_ptr<StopHandler> stopHandler) {
    this->stopHandler = std::move(stopHandler);
}

inline bool
Search::canClaimDrawRep(const Position& pos, const std::vector<U64>& posHashList,
                       int posHashListSize, int posHashFirstNew) {
    int reps = 0;
    int stop = std::max(0, posHashListSize - pos.getHalfMoveClock());
    for (int i = posHashListSize - 4; i >= stop; i -= 2) {
        if (pos.zobristHash() == posHashList[i]) {
            reps++;
            if ((i >= posHashFirstNew) || (reps >= 2))
                return true;
        }
    }
    return false;
}

inline bool
Search::passedPawnPush(const Position& pos, const Move& m) {
    int p = pos.getPiece(m.from());
    if (pos.isWhiteMove()) {
        if (p != Piece::WPAWN)
            return false;
        if ((BitBoard::wPawnBlockerMask[m.to()] & pos.pieceTypeBB(Piece::BPAWN)) != 0)
            return false;
        return m.to() >= A6;
    } else {
        if (p != Piece::BPAWN)
            return false;
        if ((BitBoard::bPawnBlockerMask[m.to()] & pos.pieceTypeBB(Piece::WPAWN)) != 0)
            return false;
        return m.to() <= H3;
    }
}

inline bool
Search::defenseMove(const Position& pos, const Move& m) {
    int p = pos.getPiece(m.from());
    if (pos.isWhiteMove()) {
        if (p == Piece::WPAWN)
            return false;
        if ((pos.pieceTypeBB(Piece::BPAWN) & BitBoard::wPawnAttacks[m.from()]) == 0)
            return false;
        return (pos.pieceTypeBB(Piece::BPAWN) & BitBoard::wPawnAttacks[m.to()]) == 0;
    } else {
        if (p == Piece::BPAWN)
            return false;
        if ((pos.pieceTypeBB(Piece::WPAWN) & BitBoard::bPawnAttacks[m.from()]) == 0)
            return false;
        return (pos.pieceTypeBB(Piece::WPAWN) & BitBoard::bPawnAttacks[m.to()]) == 0;
    }
}

inline bool
Search::isExpectedCutNode(int ply) const {
    int nFirst = 0;
    while (ply > 0) {
        ply--;
        int moveNo = searchTreeInfo[ply].currentMoveNo;
        if (moveNo == 0)
            nFirst++;
        else
            return (nFirst % 2) == 0;
    }
    return false;
}

inline int
Search::SEE(const Move& m, int alpha, int beta) {
    return SEE(pos, m, alpha, beta);
}

inline int
Search::signSEE(const Move& m) {
    int p0 = ::pieceValue[pos.getPiece(m.from())];
    int p1 = ::pieceValue[pos.getPiece(m.to())];
    if (p0 < p1)
        return 1;
    return SEE(m, -1, 1);
}

inline bool
Search::negSEE(const Move& m) {
    int p0 = ::pieceValue[pos.getPiece(m.from())];
    int p1 = ::pieceValue[pos.getPiece(m.to())];
    if (p1 >= p0)
        return false;
    return SEE(m, -1, 0) < 0;
}

inline void
Search::scoreMoveListMvvLva(MoveList& moves) const {
    for (int i = 0; i < moves.size; i++) {
        Move& m = moves[i];
        int v = pos.getPiece(m.to());
        int a = pos.getPiece(m.from());
        m.setScore(Evaluate::pieceValueOrder[v] * 8 - Evaluate::pieceValueOrder[a]);
    }
}

inline void
Search::selectBest(MoveList& moves, int startIdx) {
    int bestIdx = startIdx;
    int bestScore = moves[bestIdx].score();
    for (int i = startIdx + 1; i < moves.size; i++) {
        int sc = moves[i].score();
        if (sc > bestScore) {
            bestIdx = i;
            bestScore = sc;
        }
    }
    std::swap(moves[bestIdx], moves[startIdx]);
}

inline void
Search::setSearchTreeInfo(int ply, const SearchTreeInfo& sti,
                          U64 rootNodeIdx) {
    searchTreeInfo[ply] = sti;
    searchTreeInfo[ply].nodeIdx = rootNodeIdx;
}

inline int
Search::negaScout(bool tb,
                  int alpha, int beta, int ply, int depth, int recaptureSquare,
                  const bool inCheck) {
    bool tb2 = tb && depth >= minProbeDepth;
    if (tb2)
        return negaScout<true>(alpha, beta, ply, depth, recaptureSquare, inCheck);
    else
        return negaScout<false>(alpha, beta, ply, depth, recaptureSquare, inCheck);
}

inline bool
Search::canClaimDraw50(const Position& pos) {
    return (pos.getHalfMoveClock() >= 100);
}

inline S64
Search::getTotalNodes() const {
    return totalNodes + comm.getNumSearchedNodes();
}

inline S64
Search::getTotalNodesThisThread() const {
    return totalNodes;
}

inline S64
Search::getTbHits() const {
    return tbHits + comm.getTbHits();
}

inline S64
Search::getTbHitsThisThread() const {
    return tbHits;
}

inline void
Search::setMinProbeDepth(int depth) {
    minProbeDepth = depth;
}

#endif /* SEARCH_HPP_ */
