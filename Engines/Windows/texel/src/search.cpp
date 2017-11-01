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
 * search.cpp
 *
 *  Created on: Feb 25, 2012
 *      Author: petero
 */

#include "search.hpp"
#include "history.hpp"
#include "killerTable.hpp"
#include "numa.hpp"
#include "cluster.hpp"
#include "clustertt.hpp"
#include "tbprobe.hpp"
#include "treeLogger.hpp"
#include "textio.hpp"
#include "util/logger.hpp"

#include <iostream>
#include <iomanip>
#include <cmath>
#include <limits>

using namespace SearchConst;


Search::Search(const Position& pos0, const std::vector<U64>& posHashList0,
               int posHashListSize0, SearchTables& st, Communicator& comm,
               TreeLogger& logFile)
    : eval(st.et), kt(st.kt), ht(st.ht), tt(st.tt), comm(comm), threadNo(0),
      logFile(logFile) {
    stopHandler = make_unique<DefaultStopHandler>(*this);
    init(pos0, posHashList0, posHashListSize0);
}

void
Search::init(const Position& pos0, const std::vector<U64>& posHashList0,
             int posHashListSize0) {
    pos = pos0;
    posHashList = posHashList0;
    posHashListSize = posHashListSize0;
    posHashFirstNew = posHashListSize;
    minTimeMillis = -1;
    maxTimeMillis = -1;
    earlyStopPercentage = minTimeUsage;
    searchNeedMoreTime = false;
    maxNodes = -1;
    minProbeDepth = 0;
    nodesBetweenTimeCheck = 1000;
    strength = 1000;
    weak = false;
    randomSeed = 0;
    tLastStats = currentTimeMillis();
    totalNodes = 0;
    tbHits = 0;
    nodesToGo = 0;
}

void
Search::timeLimit(int minTimeLimit, int maxTimeLimit, int earlyStopPercent) {
    minTimeMillis = minTimeLimit;
    maxTimeMillis = maxTimeLimit;
    earlyStopPercentage = (earlyStopPercent > 0 ? earlyStopPercent : minTimeUsage);
}

void
Search::setStrength(int strength, U64 randomSeed) {
    if (strength < 0) strength = 0;
    if (strength > 1000) strength = 1000;
    this->strength = strength;
    weak = strength < 1000;
    this->randomSeed = randomSeed;
}

Move
Search::iterativeDeepening(const MoveList& scMovesIn,
                           int maxDepth, S64 initialMaxNodes,
                           int maxPV, bool onlyExact,
                           int minProbeDepth, bool clearHistory) {
    tStart = currentTimeMillis();
    totalNodes = 0;
    tbHits = 0;
    nodesToGo = 0;
    if (scMovesIn.size <= 0)
        return Move(); // No moves to search

    logFile.open("/home/petero/treelog.dmp", threadNo);
    const U64 rootNodeIdx = logFile.logPosition(pos);

    kt.clear();
    maxNodes = initialMaxNodes;
    this->minProbeDepth = TBProbe::tbEnabled() ? minProbeDepth : MAX_SEARCH_DEPTH;
    if ((maxDepth < 0) && (maxNodes < 0) && !TBProbe::tbEnabled())
        if (tt.updateTB(pos, maxTimeMillis))
            this->minProbeDepth = 1; // In-memory on-demand tables can be probed aggressively
    std::vector<MoveInfo> rootMoves;
    getRootMoves(scMovesIn, rootMoves, maxDepth);

    Position origPos(pos);
    bool firstIteration = true;
    Move bestMove = rootMoves[0].move; // bestMove is != rootMoves[0].move when there is an unresolved fail high
    Move bestExactMove = rootMoves[0].move; // Only updated when new best move has exact score
    if ((maxDepth < 0) || (maxDepth > MAX_SEARCH_DEPTH))
        maxDepth = MAX_SEARCH_DEPTH;
    maxPV = std::min(maxPV, (int)rootMoves.size());
    const int evalScore = eval.evalPos(pos);
    initSearchTreeInfo();
    ht.reScale();
    comm.sendInitSearch(pos, posHashList, posHashListSize, clearHistory);

    int posHashFirstNew0 = posHashFirstNew;
    bool knownLoss = false; // True if at least one of the first maxPV moves is a known loss
    try {
    for (int depth = 1; ; depth++, firstIteration = false) {
        if (listener) listener->notifyDepth(depth);
        int aspirationDelta = 0;
        UndoInfo ui;
        bool needMoreTime = false;
        for (int mi = 0; mi < (int)rootMoves.size(); mi++) {
            posHashFirstNew = posHashFirstNew0 + ((maxPV > 1) ? 1 : 0);
            if (mi < maxPV)
                aspirationDelta = isWinScore(std::abs(rootMoves[mi].score())) ? 3000 : aspirationWindow;
            int alpha, beta;
            if (mi < maxPV) {
                if (firstIteration) {
                    alpha = -MATE0;
                    beta = MATE0;
                } else {
                    alpha = std::max(rootMoves[mi].score() - aspirationDelta, -MATE0);
                    beta  = std::min(rootMoves[mi].score() + aspirationDelta, MATE0);
                }
            } else {
                alpha = rootMoves[maxPV-1].score();
                beta = alpha + 1;
            }

            searchNeedMoreTime = (mi > 0);
            Move& m = rootMoves[mi].move;
            if (currentTimeMillis() - tStart >= 1000)
                if (listener) listener->notifyCurrMove(m, mi + 1);
            S64 nodesThisMove = -totalNodes;
            posHashList[posHashListSize++] = pos.zobristHash();
            bool givesCheck = MoveGen::givesCheck(pos, m);

            int lmrS = 0;
            bool isCapture = (pos.getPiece(m.to()) != Piece::EMPTY);
            bool isPromotion = (m.promoteTo() != Piece::EMPTY);
            if ((depth >= 3) && !isCapture && !isPromotion &&
                !givesCheck && !passedPawnPush(pos, m) && (mi >= rootLMRMoveCount + maxPV)) {
                lmrS = 1;
            }
            pos.makeMove(m, ui);
            totalNodes++;
            SearchTreeInfo& sti = searchTreeInfo[0];
            sti.currentMove = m;
            sti.currentMoveNo = mi;
            sti.evalScore = evalScore;
            sti.nodeIdx = rootNodeIdx;
            int score = -negaScoutRoot(true, -beta, -alpha, 1, depth - lmrS - 1, givesCheck);
            if ((lmrS > 0) && (score > alpha))
                score = -negaScoutRoot(true, -beta, -alpha, 1, depth - 1, givesCheck);
            nodesThisMove += totalNodes;
            posHashListSize--;
            pos.unMakeMove(m, ui);
            storeSearchResult(rootMoves, mi, depth, alpha, beta, score);
            if ((mi < maxPV) || (score > rootMoves[maxPV-1].score()))
                notifyPV(rootMoves, mi, maxPV);
            int betaRetryDelta = aspirationDelta;
            int alphaRetryDelta = aspirationDelta;
            while ((score >= beta) || ((mi < maxPV) && (score <= alpha))) {
                if (!knownLoss && !rootMoves[mi].knownLoss && isLoseScore(score) && (score <= alpha))
                    break;
                nodesThisMove -= totalNodes;
                posHashList[posHashListSize++] = pos.zobristHash();
                bool fh = score >= beta;
                if (fh) {
                    if (isWinScore(score))
                        betaRetryDelta = MATE0; // Don't use aspiration window when searching for faster mate
                    beta = std::min(score + betaRetryDelta, MATE0);
                    betaRetryDelta = betaRetryDelta * 3 / 2;
                    if (mi != 0)
                        needMoreTime = true;
                    bestMove = m;
                } else { // score <= alpha
                    if (isLoseScore(score))
                        alphaRetryDelta = MATE0; // Don't use aspiration window when searching for faster mate
                    alpha = std::max(score - alphaRetryDelta, -MATE0);
                    alphaRetryDelta = alphaRetryDelta * 3 / 2;
                    needMoreTime = searchNeedMoreTime = true;
                }
                pos.makeMove(m, ui);
                totalNodes++;
                score = -negaScoutRoot(true, -beta, -alpha, 1, depth - 1, givesCheck);
                nodesThisMove += totalNodes;
                posHashListSize--;
                pos.unMakeMove(m, ui);
                storeSearchResult(rootMoves, mi, depth, alpha, beta, score);
                notifyPV(rootMoves, mi, maxPV);
            }
            rootMoves[mi].nodes += nodesThisMove;
            rootMoves[mi].knownLoss = isLoseScore(score);
            if ((mi < maxPV) || (score > rootMoves[maxPV-1].move.score())) {
                MoveInfo tmp = rootMoves[mi];
                int i = mi;
                while ((i > 0) && (rootMoves[i-1].score() < tmp.score())) {
                    rootMoves[i] = rootMoves[i-1];
                    i--;
                }
                rootMoves[i] = tmp;
            }
            bestMove = rootMoves[0].move;
            bestExactMove = bestMove;
            if (!firstIteration) {
                S64 timeLimit = needMoreTime ? maxTimeMillis : minTimeMillis;
                if (timeLimit >= 0) {
                    U64 tNow = currentTimeMillis();
                    if (tNow - tStart >= (U64)timeLimit)
                        break;
                }
            }
        }
        S64 tNow = currentTimeMillis();
        if (maxTimeMillis >= 0)
            if (tNow - tStart > minTimeMillis * 0.01 * earlyStopPercentage)
                break;
        if (depth >= maxDepth)
            break;
        if (maxNodes >= 0)
            if (getTotalNodes() >= maxNodes)
                break;
        bool enoughDepth = true;
        for (int i = 0; i < maxPV; i++) {
            int plyToMate = MATE0 - std::abs(rootMoves[i].score());
            if (depth < plyToMate)
                enoughDepth = false;
        }
        if (enoughDepth)
            break;
        if (firstIteration) {
            std::stable_sort(rootMoves.begin()+maxPV, rootMoves.end(), MoveInfo::SortByScore());
        } else {
            // Moves that were hard to search should be searched early in the next iteration
            std::stable_sort(rootMoves.begin()+maxPV, rootMoves.end(), MoveInfo::SortByNodes());
        }
        if (!knownLoss && rootMoves[maxPV - 1].knownLoss) {
            depth = std::max(0, depth - 1);
            knownLoss = true;
        }
    }
    } catch (const StopSearch&) {
        pos = origPos;
    }
    notifyStats();

    logFile.close();
    return onlyExact ? bestExactMove : bestMove;
}

class HelperThreadResult : public std::exception {
public:
    explicit HelperThreadResult(int score) : score(score) {}
    int getScore() const { return score; }
private:
    int score;
};

int
Search::negaScoutRoot(bool tb, int alpha, int beta, int ply, int depth,
                      const bool inCheck) {
    SearchTreeInfo sti = searchTreeInfo[ply-1];
    jobId++;
    comm.sendStartSearch(jobId, sti, alpha, beta, depth);
    U64 nodeIdx = logFile.peekNextNodeIdx();
    Position pos0(pos);
    int posHashListSize0 = posHashListSize;
    try {
        return negaScout(tb, alpha, beta, ply, depth, -1, inCheck);
    } catch (const HelperThreadResult& res) {
        initSearchTreeInfo();
        searchTreeInfo[ply-1] = sti;
        pos = pos0;
        posHashListSize = posHashListSize0;

        const U64 hKey = pos.historyHash();
        int score = res.getScore();
        int evalScore = UNKNOWN_SCORE;
        int type = score <= alpha ? TType::T_LE : score >= beta ? TType::T_GE : TType::T_EXACT;
        logFile.logNodeEnd(nodeIdx, score, type, evalScore, hKey);
        return score;
    }
}

void
Search::storeSearchResult(std::vector<MoveInfo>& scMoves, int mi, int depth,
                          int alpha, int beta, int score) {
//    std::cout << "d:" << depth << " mi:" << mi << " a:" << alpha
//              << " b:" << beta << " s:" << score << std::endl;
    scMoves[mi].depth = depth;
    scMoves[mi].alpha = alpha;
    scMoves[mi].beta = beta;
    scMoves[mi].move.setScore(score);
    scMoves[mi].pv.clear();
    tt.extractPVMoves(pos, scMoves[mi].move, scMoves[mi].pv);
    if ((maxTimeMillis < 0) && isWinScore(std::abs(score)))
        TBProbe::extendPV(pos, scMoves[mi].pv, tt.getTT());
}

void
Search::notifyPV(const std::vector<MoveInfo>& moveInfo, int mi, int maxPV) {
    bool miNotified = false;
    int lastReportedDepth = -1;
    for (int i = 0, n = 0; n < maxPV; i++) {
        if (!miNotified && (moveInfo[mi].score() > moveInfo[i].score()) &&
            (moveInfo[mi].score() > moveInfo[mi].alpha)) {
            notifyPV(moveInfo[mi], maxPV > 1 ? n : -1);
            lastReportedDepth = moveInfo[mi].depth;
            miNotified = true;
            n++;
            if (n >= maxPV)
                break;
        }
        if (i == mi) {
            if (!miNotified) {
                notifyPV(moveInfo[mi], maxPV > 1 ? n : -1);
                lastReportedDepth = moveInfo[mi].depth;
                miNotified = true;
                n++;
            }
        } else {
            notifyPV(moveInfo[i], maxPV > 1 ? n : -1);
            lastReportedDepth = moveInfo[i].depth;
            n++;
        }
    }
    if (listener && (moveInfo[mi].depth != lastReportedDepth))
        listener->notifyDepth(moveInfo[mi].depth);
}

void
Search::notifyPV(const MoveInfo& info, int multiPVIndex) {
    if (info.depth <= 0)
        return;
    bool uBound = info.score() <= info.alpha;
    bool lBound = info.score() >= info.beta;
    int score = info.move.score();
    if (!listener)
        return;
    bool isMate = false;
    if (isWinScore(score)) {
        isMate = true;
        score = (MATE0 - score) / 2;
    } else if (isLoseScore(score)) {
        isMate = true;
        score = -((MATE0 + score - 1) / 2);
    }
    S64 tNow = currentTimeMillis();
    S64 time = tNow - tStart;
    S64 totNodes = getTotalNodes();
    S64 tbHits = getTbHits();
    S64 nps = (time > 0) ? (S64)(totNodes / (time / 1000.0)) : 0;
    listener->notifyPV(info.depth, score, time, totNodes, nps, isMate,
                       uBound, lBound, info.pv, multiPVIndex, tbHits);
    tLastStats = tNow;
}

void
Search::notifyStats() {
    S64 tNow = currentTimeMillis();
    if (listener) {
        S64 time = tNow - tStart;
        S64 totNodes = getTotalNodes();
        S64 nps = (time > 0) ? (S64)(totNodes / (time / 1000.0)) : 0;
        S64 tbHits = getTbHits();
        int hashFull = tt.getHashFull();
        listener->notifyStats(totNodes, nps, hashFull, tbHits, time);
    }
    tLastStats = tNow;
}

bool
Search::shouldStop() {
    class Handler : public Communicator::CommandHandler {
    public:
        explicit Handler(int jobId) : jobId(jobId) {}
        void reportResult(int jobId, int score) override {
            if (jobId == this->jobId)
                throw HelperThreadResult(score);
        }
    private:
        int jobId;
    };
    Handler handler(jobId);
    comm.poll(handler);

    S64 tNow = currentTimeMillis();
    S64 timeLimit = searchNeedMoreTime ? maxTimeMillis : minTimeMillis;
    if (    ((timeLimit >= 0) && (tNow - tStart >= timeLimit)) ||
            ((maxNodes >= 0) && (getTotalNodes() >= maxNodes)))
        return true;
    if (tNow - tLastStats >= 1000)
        notifyStats();
    return false;
}

template <bool tb>
int
Search::negaScout(int alpha, int beta, int ply, int depth, int recaptureSquare,
                  const bool inCheck) {
    // Mate distance pruning
    beta = std::min(beta, MATE0-ply-1);
    if (alpha >= beta)
        return alpha;

    if (logFile.isOpened()) {
        const SearchTreeInfo& sti = searchTreeInfo[ply-1];
        U64 idx = logFile.logNodeStart(sti.nodeIdx, sti.currentMove, alpha, beta, ply, depth);
        searchTreeInfo[ply].nodeIdx = idx;
    }
    if (nodesToGo <= 0) {
        nodesToGo = nodesBetweenTimeCheck;
        if (stopHandler->shouldStop())
            throw StopSearch();
    }

    const U64 hKey = pos.historyHash();
    SearchTreeInfo& sti = searchTreeInfo[ply];
    sti.currentMove = emptyMove;
    sti.currentMoveNo = -1;

    // Draw tests
    if (canClaimDraw50(pos)) {
        if (inCheck) {
            MoveList moves;
            MoveGen::pseudoLegalMoves(pos, moves);
            MoveGen::removeIllegal(pos, moves);
            if (moves.size == 0) {            // Can't claim draw if already check mated.
                int score = -(MATE0-(ply+1));
                logFile.logNodeEnd(searchTreeInfo[ply].nodeIdx, score, TType::T_EXACT, UNKNOWN_SCORE, hKey);
                return score;
            }
        }
        logFile.logNodeEnd(searchTreeInfo[ply].nodeIdx, 0, TType::T_EXACT, UNKNOWN_SCORE, hKey);
        return 0;
    }
    if (canClaimDrawRep(pos, posHashList, posHashListSize, posHashFirstNew)) {
        logFile.logNodeEnd(searchTreeInfo[ply].nodeIdx, 0, TType::T_EXACT, UNKNOWN_SCORE, hKey);
        return 0;            // No need to test for mate here, since it would have been
                             // discovered the first time the position came up.
    }

    // Check transposition table
    int evalScore = UNKNOWN_SCORE;
    TranspositionTable::TTEntry ent;
    ent.clear();
    const bool singularSearch = !sti.singularMove.isEmpty();
    const bool useTT = !singularSearch;
    if (useTT) tt.probe(hKey, ent);
    Move hashMove;
    if (ent.getType() != TType::T_EMPTY) {
        int score = ent.getScore(ply);
        evalScore = ent.getEvalScore();
        ent.getMove(hashMove);
        if (((beta == alpha + 1) || (depth <= ply)) && ent.isCutOff(alpha, beta, ply, depth)) {
            if (score >= beta) {
                if (!hashMove.isEmpty())
                    if (pos.getPiece(hashMove.to()) == Piece::EMPTY)
                        kt.addKiller(ply, hashMove);
            }
            sti.bestMove = hashMove;
            logFile.logNodeEnd(sti.nodeIdx, score, ent.getType(), evalScore, hKey);
            return score;
        }
    }
    if (depth >= 7) {
        bool excl = sti.abdadaExclusive;
        sti.abdadaExclusive = false;
        if (excl && ent.getBusy()) {
            logFile.logNodeEnd(sti.nodeIdx, BUSY, TType::T_EMPTY, UNKNOWN_SCORE, hKey);
            return BUSY;
        }
        if (ent.getType() != TType::T_EMPTY) {
            tt.setBusy(ent, ply);
        }
    }
    if (singularSearch)
        hashMove = sti.singularMove;

    // Probe endgame tablebases
    const int illegalScore = -(MATE0-(ply+1));
    int tbScore = illegalScore;
    if (tb && depth >= minProbeDepth && !singularSearch) {
        TranspositionTable::TTEntry tbEnt;
        tbEnt.clear();
        if (TBProbe::tbProbe(pos, ply, alpha, beta, tt.getTT(), tbEnt)) {
            tbHits++;
            nodesToGo -= 100;
            int type = tbEnt.getType();
            int score = tbEnt.getScore(ply);
            bool cutOff = false;
            const int drawSwindleReduction = 16;
            if (score == 0 && type == TType::T_EXACT) {
                const int maxSwindle = maxFrustrated;
                if (depth < drawSwindleReduction) {
                    if (evalScore == UNKNOWN_SCORE)
                        evalScore = eval.evalPos(pos);
                    score = Evaluate::swindleScore(evalScore, tbEnt.getEvalScore());
                    cutOff = true;
                } else if (alpha >= maxSwindle) {
                    tbEnt.setType(TType::T_LE);
                    score = maxSwindle;
                    cutOff = true;
                } else if (beta <= -maxSwindle) {
                    tbEnt.setType(TType::T_GE);
                    score = -maxSwindle;
                    cutOff = true;
                }
            } else {
                bool checkCutOff = true;
                if (score == 0) { // Draw or frustrated win/loss
                    if (depth < drawSwindleReduction)
                        score = Evaluate::swindleScore(0, tbEnt.getEvalScore());
                    else
                        checkCutOff = false;
                }
                if (checkCutOff)
                    if ( (type == TType::T_EXACT) ||
                         ((type == TType::T_GE) && (score >= beta)) ||
                         ((type == TType::T_LE) && (score <= alpha)))
                        cutOff = true;
            }
            if (cutOff) {
                emptyMove.setScore(score);
                if (useTT) tt.insert(hKey, emptyMove, tbEnt.getType(), ply, depth, evalScore);
                logFile.logNodeEnd(sti.nodeIdx, score, tbEnt.getType(), evalScore, hKey);
                return score;
            }
            if ((type == TType::T_GE) && (score > alpha)) {
                tbScore = score;
                alpha = score - 1;
            }
        }
    }

    // If out of depth, perform quiescence search
    if (depth <= 0) {
        q0Eval = evalScore;
        sti.bestMove.setMove(0,0,0,0);
        int score = quiesce(alpha, beta, ply, 0, inCheck);
        int type = TType::T_EXACT;
        if (score <= alpha) {
            type = TType::T_LE;
        } else if (score >= beta) {
            type = TType::T_GE;
        }
        sti.bestMove.setScore(score);
        if (useTT) tt.insert(hKey, sti.bestMove, type, ply, depth, q0Eval);
        logFile.logNodeEnd(sti.nodeIdx, score, type, q0Eval, hKey);
        return score;
    }

    // Razoring
    const bool normalBound = !isLoseScore(alpha) && !isWinScore(beta);
    if (normalBound && !inCheck && (depth < 4) && (beta == alpha + 1) && !singularSearch) {
        if (evalScore == UNKNOWN_SCORE) {
            evalScore = eval.evalPos(pos);
        }
        const int razorMargin = (depth <= 1) ? razorMargin1 : razorMargin2;
        if (evalScore < beta - razorMargin) {
            q0Eval = evalScore;
            int score = quiesce(alpha-razorMargin, beta-razorMargin, ply, 0, inCheck);
            if (score <= alpha-razorMargin) {
                emptyMove.setScore(score);
                if (useTT) tt.insert(hKey, emptyMove, TType::T_LE, ply, depth, q0Eval);
                logFile.logNodeEnd(sti.nodeIdx, score, TType::T_LE, q0Eval, hKey);
                return score;
            }
        }
    }

    // Reverse futility pruning
    if (!inCheck && (depth < 5) && normalBound && !singularSearch) {
        bool mtrlOk;
        if (pos.isWhiteMove()) {
            mtrlOk = (pos.wMtrl() > pos.wMtrlPawns()) && (pos.wMtrlPawns() > 0);
        } else {
            mtrlOk = (pos.bMtrl() > pos.bMtrlPawns()) && (pos.bMtrlPawns() > 0);
        }
        if (mtrlOk) {
            int margin;
            if (depth <= 1)      margin = reverseFutilityMargin1;
            else if (depth <= 2) margin = reverseFutilityMargin2;
            else if (depth <= 3) margin = reverseFutilityMargin3;
            else                 margin = reverseFutilityMargin4;
            if (evalScore == UNKNOWN_SCORE)
                evalScore = eval.evalPos(pos);
            if (evalScore - margin >= beta) {
                emptyMove.setScore(evalScore - margin);
                if (useTT) tt.insert(hKey, emptyMove, TType::T_GE, ply, depth, evalScore);
                logFile.logNodeEnd(sti.nodeIdx, evalScore - margin, TType::T_GE, evalScore, hKey);
                return evalScore - margin;
            }
        }
    }

    // Null-move pruning
    if ((depth >= 3) && !inCheck && sti.allowNullMove && !isWinScore(beta) &&
            !singularSearch && (beta == alpha + 1)) {
        bool nullOk;
        if (pos.isWhiteMove()) {
            nullOk = (pos.wMtrl() > pos.wMtrlPawns()) && (pos.wMtrlPawns() > 0);
        } else {
            nullOk = (pos.bMtrl() > pos.bMtrlPawns()) && (pos.bMtrlPawns() > 0);
        }
        const int R = std::min(depth, 4);
        if (nullOk) {
            if (((ent.getType() == TType::T_EXACT) || (ent.getType() == TType::T_LE)) &&
                (ent.getDepth() >= depth - R) && (ent.getScore(ply) < beta))
                nullOk = false;
        }
        if (nullOk) {
            if (evalScore == UNKNOWN_SCORE)
                evalScore = eval.evalPos(pos);
            if (evalScore < beta)
                nullOk = false;
        }
        if (nullOk) {
            int score;
            {
                pos.setWhiteMove(!pos.isWhiteMove());
                const int epSquare = pos.getEpSquare();
                pos.setEpSquare(-1);
                searchTreeInfo[ply+1].allowNullMove = false;
                searchTreeInfo[ply+1].bestMove.setMove(0,0,0,0);
                const int hmc = pos.getHalfMoveClock();
                pos.setHalfMoveClock(0);
                score = -negaScout(tb, -beta, -(beta - 1), ply + 1, depth - R, -1, false);
                pos.setEpSquare(epSquare);
                pos.setWhiteMove(!pos.isWhiteMove());
                pos.setHalfMoveClock(hmc);
                searchTreeInfo[ply+1].allowNullMove = true;
            }
            if ((score >= beta) && (depth >= 10)) {
                // Null-move verification search
                SearchTreeInfo& sti2 = searchTreeInfo[ply-1];
                SearchTreeInfo& sti3 = searchTreeInfo[ply+1];
                const Move savedMove = sti2.currentMove;
                const int savedMoveNo = sti2.currentMoveNo;
                const S64 savedNodeIdx2 = sti2.nodeIdx;
                sti2.currentMove = Move(1,1,0); // Represents "no move"
                sti2.currentMoveNo = -1;
                sti2.nodeIdx = sti.nodeIdx;
                const S64 savedNodeIdx = sti.nodeIdx;
                sti.allowNullMove = false;
                score = negaScout(tb, beta - 1, beta, ply, depth - R, recaptureSquare, inCheck);
                sti.allowNullMove = true;
                sti.nodeIdx = savedNodeIdx;
                sti2.currentMove = savedMove;
                sti2.currentMoveNo = savedMoveNo;
                sti2.nodeIdx = savedNodeIdx2;
                sti3.bestMove.setMove(0,0,0,0);
            }
            if (score >= beta) {
                if (isWinScore(score))
                    score = beta;
                emptyMove.setScore(score);
                if (useTT) tt.insert(hKey, emptyMove, TType::T_GE, ply, depth, evalScore);
                logFile.logNodeEnd(sti.nodeIdx, score, TType::T_GE, evalScore, hKey);
                return score;
            }
        }
    }

    bool futilityPrune = false;
    int futilityScore = alpha;
    if (!inCheck && (depth < 5) && normalBound && !singularSearch) {
        int margin;
        if (depth <= 1)      margin = futilityMargin1;
        else if (depth <= 2) margin = futilityMargin2;
        else if (depth <= 3) margin = futilityMargin3;
        else                 margin = futilityMargin4;
        if (evalScore == UNKNOWN_SCORE)
            evalScore = eval.evalPos(pos);
        futilityScore = evalScore + margin;
        if (futilityScore <= alpha)
            futilityPrune = true;
    }

    // Internal iterative deepening
    if ((depth > 4) && hashMove.isEmpty() && !inCheck) {
        bool isPv = beta > alpha + 1;
        if (isPv || (depth > 8)) {
            SearchTreeInfo& sti2 = searchTreeInfo[ply-1];
            const Move savedMove = sti2.currentMove;
            const int savedMoveNo = sti2.currentMoveNo;
            const S64 savedNodeIdx2 = sti2.nodeIdx;
            sti2.currentMove = Move(1,1,0); // Represents "no move"
            sti2.currentMoveNo = -1;
            sti2.nodeIdx = sti.nodeIdx;
            const S64 savedNodeIdx = sti.nodeIdx;
            int newDepth = isPv ? depth  - 2 : depth * 3 / 8;
            negaScout(tb, alpha, beta, ply, newDepth, -1, inCheck);
            sti.nodeIdx = savedNodeIdx;
            sti2.currentMove = savedMove;
            sti2.currentMoveNo = savedMoveNo;
            sti2.nodeIdx = savedNodeIdx2;
            tt.probe(hKey, ent);
            if (ent.getType() != TType::T_EMPTY)
                ent.getMove(hashMove);
        }
    }

    // Generate move list
    MoveList moves;
    if (inCheck)
        MoveGen::checkEvasions(pos, moves);
    else
        MoveGen::pseudoLegalMoves(pos, moves);
    bool seeDone = false;
    bool hashMoveSelected = true;
    if (hashMove.isEmpty() || !selectHashMove(moves, hashMove)) {
        scoreMoveList(moves, ply);
        seeDone = true;
        hashMoveSelected = false;
    }

    // Handle singular extension
    bool singularExtend = false;
    if ((depth > 6) &&
            hashMoveSelected && !singularSearch &&
            (ent.getType() != TType::T_LE) &&
            (ent.getDepth() >= depth - 3) &&
            (!isWinScore(std::abs(ent.getScore(ply))) || !normalBound) &&
            (getMoveExtend(hashMove, recaptureSquare) <= 0) &&
            (ply + depth < MAX_SEARCH_DEPTH) &&
            MoveGen::isLegal(pos, hashMove, inCheck)) {
        SearchTreeInfo& sti2 = searchTreeInfo[ply-1];
        const Move savedMove = sti2.currentMove;
        const int savedMoveNo = sti2.currentMoveNo;
        const S64 savedNodeIdx2 = sti2.nodeIdx;
        sti2.currentMove = Move(1,1,0); // Represents "no move"
        sti2.currentMoveNo = -1;
        sti2.nodeIdx = sti.nodeIdx;
        const S64 savedNodeIdx = sti.nodeIdx;
        sti.singularMove = hashMove;
        int newDepth = depth / 2;
        int newBeta = ent.getScore(ply) - 25;
        int singScore = negaScout(tb, newBeta-1, newBeta, ply, newDepth,
                                  recaptureSquare, inCheck);
        sti.singularMove.setMove(0,0,0,0);
        sti.nodeIdx = savedNodeIdx;
        sti2.currentMove = savedMove;
        sti2.currentMoveNo = savedMoveNo;
        sti2.nodeIdx = savedNodeIdx2;
        if (singScore <= newBeta-1)
            singularExtend = true;
    }

    sti.evalScore = evalScore;
    bool badPrevMove = evalScore != UNKNOWN_SCORE && ply >= 2 &&
                       evalScore < searchTreeInfo[ply-2].evalScore;

    // Determine late move pruning (LMP) limit
    int lmpMoveCountLimit = 256;
    if (beta == alpha + 1) {
        bool lmpOk;
        if (pos.isWhiteMove())
            lmpOk = (pos.wMtrl() > pos.wMtrlPawns()) && (pos.wMtrlPawns() > 0);
        else
            lmpOk = (pos.bMtrl() > pos.bMtrlPawns()) && (pos.bMtrlPawns() > 0);
        if (lmpOk) {
            if (depth <= 1) {
                lmpMoveCountLimit = badPrevMove ?  2 : lmpMoveCountLimit1;
            } else if (depth <= 2) {
                lmpMoveCountLimit = badPrevMove ?  4 : lmpMoveCountLimit2;
            } else if (depth <= 3) {
                lmpMoveCountLimit = badPrevMove ?  7 : lmpMoveCountLimit3;
            } else if (depth <= 4) {
                lmpMoveCountLimit = badPrevMove ? 15 : lmpMoveCountLimit4;
            }
        }
    }

    // Start searching move alternatives
    bool expectedCutNodeComputed = false;
    bool expectedCutNode = false;
    UndoInfo ui;
    bool haveLegalMoves = false;
    int b = beta;
    int bestScore = illegalScore;
    int bestMove = -1;
    int lmrCount = 0;
    if (tb && tbScore != illegalScore) {
        bestScore = tbScore - 1;
        bestMove = -2;
        sti.bestMove.setMove(0, 0, 0, 0);
    }
    bool allDone = false;
    for (int pass = 0; pass < 2 && !allDone; pass++) {
        allDone = true;
        for (int mi = 0; mi < moves.size; mi++) {
            if (pass > 0 && moves[mi].score() > BUSY)
                continue;
            if (pass == 0) {
                if ((mi == 1) && !seeDone) {
                    scoreMoveList(moves, ply, 1);
                    seeDone = true;
                }
                if ((mi < lmpMoveCountLimit) || (depth >= 2 && lmrCount <= lmrMoveCountLimit1))
                    if ((mi > 0) || !hashMoveSelected)
                        selectBest(moves, mi);
            }
            Move& m = moves[mi];
            bool isCapture = (pos.getPiece(m.to()) != Piece::EMPTY);
            bool isPromotion = (m.promoteTo() != Piece::EMPTY);
            int sVal = std::numeric_limits<int>::min();
            bool mayReduce = (m.score() < 30) && (!isCapture || m.score() < 0) && !isPromotion;
            bool givesCheck = MoveGen::givesCheck(pos, m);
            bool doFutility = false;
            if ((pass == 0) && mayReduce && haveLegalMoves && !givesCheck && !passedPawnPush(pos, m)) {
                if (normalBound && !isLoseScore(bestScore) && (mi >= lmpMoveCountLimit))
                    continue; // Late move pruning
                if (futilityPrune)
                    doFutility = true;
            }
            int score = illegalScore;
            if (doFutility) {
                score = futilityScore;
            } else {
#ifdef HAS_PREFETCH
                U64 nextHash = pos.hashAfterMove(m);
                tt.prefetch(nextHash);
                eval.prefetch(nextHash);
#endif
                if (pass == 0) {
                    if ((mi == 0) && m.equals(sti.singularMove))
                        continue;
                    if (!MoveGen::isLegal(pos, m, inCheck))
                        continue;
                }
                int extend = givesCheck && ((depth <= 2) || !negSEE(m)) ? 1 : getMoveExtend(m, recaptureSquare);
                if (singularExtend && (mi == 0))
                    extend = 1;
                int lmr = 0;
                if (pass == 0) {
                    if ((depth >= 2) && mayReduce && (extend == 0) && !passedPawnPush(pos, m)) {
                        lmrCount++;
                        if ((lmrCount > lmrMoveCountLimit2) && (depth >= 5) && !isCapture) {
                            lmr = 3;
                        } else if ((lmrCount > lmrMoveCountLimit1) && (depth >= 3) && !isCapture) {
                            lmr = 2;
                        } else if (mi >= 2) {
                            lmr = 1;
                        }
                        if ((lmr > 0) && !isCapture && defenseMove(pos, m))
                            lmr = 0;
                        if ((lmr > 0) && (lmr + 3 <= depth) && (beta == alpha + 1)) {
                            if (!expectedCutNodeComputed) {
                                expectedCutNode = isExpectedCutNode(ply);
                                expectedCutNodeComputed = true;
                            }
                            if (expectedCutNode)
                                lmr += 1; // Reduce expected cut nodes more
                            else if (badPrevMove)
                                lmr += 1;
                            else if (m.score() < 20)
                                lmr += 1; // Bad history score
                        }
                    }
                } else {
                    lmr = BUSY - m.score();
                }
                int newDepth = depth - 1 + extend - lmr;
                int newCaptureSquare = -1;
                if (isCapture && (givesCheck || (depth + extend) > 1)) {
                    // Compute recapture target square, but only if we are not going
                    // into q-search at the next ply.
                    int fVal = ::pieceValue[pos.getPiece(m.from())];
                    int tVal = ::pieceValue[pos.getPiece(m.to())];
                    const int pV = ::pV;
                    if (std::abs(tVal - fVal) < pV / 2) {    // "Equal" capture
                        int hp = pV / 2;
                        sVal = SEE(m, -hp, hp);
                        if (std::abs(sVal) < hp)
                            newCaptureSquare = m.to();
                    }
                }
                posHashList[posHashListSize++] = pos.zobristHash();
                pos.makeMove(m, ui);
                totalNodes++;
                nodesToGo--;
                sti.currentMove = m;
                sti.currentMoveNo = mi;

                searchTreeInfo[ply+1].abdadaExclusive = pass == 0 && haveLegalMoves;
                score = -negaScout(tb, -b, -alpha, ply + 1, newDepth, newCaptureSquare, givesCheck);
                searchTreeInfo[ply+1].abdadaExclusive = false;

                if (score == -BUSY) {
                    m.setScore(BUSY - lmr);
                    allDone = false;
                    posHashListSize--;
                    pos.unMakeMove(m, ui);
                    continue;
                }
                if (((lmr > 0) && (score > alpha)) ||
                        ((score > alpha) && (score < beta) && (b != beta))) {
                    newDepth += lmr;
                    score = -negaScout(tb, -beta, -alpha, ply + 1, newDepth, newCaptureSquare, givesCheck);
                }

                posHashListSize--;
                pos.unMakeMove(m, ui);
            }

            if (weak && haveLegalMoves)
                if (weakPlaySkipMove(pos, m, ply))
                    score = illegalScore;
            m.setScore(score);

            if (score != illegalScore)
                haveLegalMoves = true;
            bestScore = std::max(bestScore, score);
            if (score > alpha) {
                alpha = score;
                bestMove = mi;
                sti.bestMove.setMove(m.from(), m.to(), m.promoteTo(), sti.bestMove.score());
            }
            if (alpha >= beta) {
                if (pos.getPiece(m.to()) == Piece::EMPTY) {
                    kt.addKiller(ply, m);
                    ht.addSuccess(pos, m, depth);
                    for (int mi2 = mi - 1; mi2 >= 0; mi2--) {
                        Move m2 = moves[mi2];
                        if (pos.getPiece(m2.to()) == Piece::EMPTY)
                            if (m2.score() > BUSY)
                                ht.addFail(pos, m2, depth);
                    }
                }
                if (((ent.getType() == TType::T_EXACT || ent.getType() == TType::T_LE)) &&
                        (ent.getScore(ply) < beta) && isLoseScore(ent.getScore(ply))) {
                    score = ent.getScore(ply);
                    emptyMove.setScore(score);
                    if (useTT) tt.insert(hKey, emptyMove, TType::T_LE, ply, depth, evalScore);
                    logFile.logNodeEnd(sti.nodeIdx, score, TType::T_LE, evalScore, hKey);
                } else {
                    if (useTT) tt.insert(hKey, m, TType::T_GE, ply, depth, evalScore);
                    logFile.logNodeEnd(sti.nodeIdx, alpha, TType::T_GE, evalScore, hKey);
                }
                return alpha;
            }
            b = alpha + 1;
        }
    }

    if (!haveLegalMoves && !inCheck) {
        if (singularSearch) {
            logFile.logNodeEnd(sti.nodeIdx, alpha, TType::T_LE, evalScore, hKey);
            return alpha; // Only one legal move, fail low to trigger singular extension
        }
        emptyMove.setScore(0);
        if (useTT) tt.insert(hKey, emptyMove, TType::T_EXACT, ply, depth, evalScore);
        logFile.logNodeEnd(sti.nodeIdx, 0, TType::T_EXACT, evalScore, hKey);
        return 0;       // Stale-mate
    }
    if (tb && bestMove == -2) { // TB win, unknown move
        bestScore = tbScore;
        emptyMove.setScore(bestScore);
        if (useTT) tt.insert(hKey, emptyMove, TType::T_EXACT, ply, depth, evalScore);
        logFile.logNodeEnd(sti.nodeIdx, bestScore, TType::T_EXACT, evalScore, hKey);
    } else if (bestMove >= 0) {
        if (useTT) tt.insert(hKey, moves[bestMove], TType::T_EXACT, ply, depth, evalScore);
        logFile.logNodeEnd(sti.nodeIdx, bestScore, TType::T_EXACT, evalScore, hKey);
    } else {
        if (((ent.getType() == TType::T_EXACT || ent.getType() == TType::T_GE)) &&
                (ent.getScore(ply) > alpha) && isWinScore(ent.getScore(ply))) {
            bestScore = ent.getScore(ply);
            ent.getMove(hashMove);
            hashMove.setScore(bestScore);
            if (useTT) tt.insert(hKey, hashMove, TType::T_GE, ply, depth, evalScore);
            logFile.logNodeEnd(sti.nodeIdx, bestScore, TType::T_GE, evalScore, hKey);
        } else {
            emptyMove.setScore(bestScore);
            if (useTT) tt.insert(hKey, emptyMove, TType::T_LE, ply, depth, evalScore);
            logFile.logNodeEnd(sti.nodeIdx, bestScore, TType::T_LE, evalScore, hKey);
        }
    }
    return bestScore;
}

int
Search::getMoveExtend(const Move& m, int recaptureSquare) {
    if ((m.to() == recaptureSquare)) {
        int tVal = ::pieceValue[pos.getPiece(m.to())];
        int a = tVal - pV / 2;
        int sVal = SEE(m, a, a + 1);
        if (sVal > a)
            return 1;
    }
    bool isCapture = (pos.getPiece(m.to()) != Piece::EMPTY);
    if (isCapture && (pos.wMtrlPawns() + pos.bMtrlPawns() > pV)) {
        // Extend if going into pawn endgame
        int capVal = ::pieceValue[pos.getPiece(m.to())];
        if (pos.isWhiteMove()) {
            if ((pos.wMtrl() == pos.wMtrlPawns()) && (pos.bMtrl() - pos.bMtrlPawns() == capVal))
                return 1;
        } else {
            if ((pos.bMtrl() == pos.bMtrlPawns()) && (pos.wMtrl() - pos.wMtrlPawns() == capVal))
                return 1;
        }
    }
    return 0;
}

void
Search::getRootMoves(const MoveList& rootMovesIn,
                     std::vector<MoveInfo>& rootMovesOut,
                     int maxDepth) {
    MoveList rootMoves(rootMovesIn);
    if ((maxTimeMillis >= 0) || (maxNodes >= 0) || (maxDepth >= 0)) {
        MoveList legalMoves;
        MoveGen::pseudoLegalMoves(pos, legalMoves);
        MoveGen::removeIllegal(pos, legalMoves);
        if (rootMoves.size == legalMoves.size) {
            // Game mode, handle missing TBs
            std::vector<Move> movesToSearch;
            if (TBProbe::getSearchMoves(pos, legalMoves, movesToSearch, tt.getTT()))
                rootMoves.filter(movesToSearch);
        }
    }
    scoreMoveList(rootMoves, 0, 0);
    for (int i = 0; i < rootMoves.size; i++)
        selectBest(rootMoves, i);

    // If strength is < 10%, only include a subset of the root moves.
    // At least one move is always included though.
    std::vector<bool> includedMoves(rootMoves.size);
    U64 rndL = pos.zobristHash() ^ randomSeed;
    includedMoves[(int)(rndL % rootMoves.size)] = true;
    double pIncl = (strength < 100) ? strength * strength * 1e-4 : 1.0;
    for (int mi = 0; mi < rootMoves.size; mi++) {
        rndL = 6364136223846793005ULL * rndL + 1442695040888963407ULL;
        double rnd = ((rndL & 0x7fffffffffffffffULL) % 1000000000) / 1e9;
        if (!includedMoves[mi] && (rnd < pIncl))
            includedMoves[mi] = true;
    }
    for (int mi = 0; mi < rootMoves.size; mi++) {
        if (includedMoves[mi]) {
            const Move& m = rootMoves[mi];
            rootMovesOut.push_back(MoveInfo(m, 0));
        }
    }
}

bool
Search::weakPlaySkipMove(const Position& pos, const Move& m, int ply) const {
    U64 rndL = pos.zobristHash() ^ Position::getHashKey(Piece::EMPTY, m.from()) ^
               Position::getHashKey(Piece::EMPTY, m.to()) ^ randomSeed;
    double rnd = ((rndL & 0x7fffffffffffffffULL) % 1000000000) / 1e9;

    double s = strength * 1e-3;
    double offs = (17 - 50 * s) / 3;
    double effPly = ply * Evaluate::interpolate(pos.wMtrl() + pos.bMtrl(), 0, 30, ::qV * 4, 100) * 1e-2;
    double t = effPly + offs;
    double p = 1/(1+exp(t)); // Probability to "see" move
    bool easyMove = ((pos.getPiece(m.to()) != Piece::EMPTY) ||
                     (ply < 2) || (searchTreeInfo[ply-2].currentMove.to() == m.from()));
    if (easyMove)
        p = 1-(1-p)*(1-p);
    if (rnd > p)
        return true;
    return false;
}

int
Search::quiesce(int alpha, int beta, int ply, int depth, const bool inCheck) {
    int score;
    if (inCheck) {
        score = -(MATE0 - (ply+1));
    } else {
        if ((depth == 0) && (q0Eval != UNKNOWN_SCORE)) {
            score = q0Eval;
        } else {
            score = eval.evalPos(pos);
            if (depth == 0)
                q0Eval = score;
        }
    }
    if (score >= beta)
        return score;
    const int evalScore = score;
    if (score > alpha)
        alpha = score;
    int bestScore = score;
    const bool tryChecks = (depth > -1);
    MoveList moves;
    if (inCheck) {
        MoveGen::checkEvasions(pos, moves);
    } else if (tryChecks) {
        MoveGen::pseudoLegalCapturesAndChecks(pos, moves);
    } else {
        MoveGen::pseudoLegalCaptures(pos, moves);
    }

    bool realInCheckComputed = false;
    bool realInCheck = false;
    if (depth > -2) {
        realInCheckComputed = true;
        realInCheck = inCheck;
    }
    scoreMoveListMvvLva(moves);
    UndoInfo ui;
    for (int mi = 0; mi < moves.size; mi++) {
        if (mi < quiesceMaxSortMoves) {
            // If the first N moves didn't fail high this is probably an ALL-node,
            // so spending more effort on move ordering is probably wasted time.
            selectBest(moves, mi);
        }
        const Move& m = moves[mi];
        bool givesCheck = false;
        bool givesCheckComputed = false;
        if (inCheck) {
            // Allow all moves
        } else {
            if ((pos.getPiece(m.to()) == Piece::EMPTY) && (m.promoteTo() == Piece::EMPTY)) {
                // Non-capture
                if (!tryChecks)
                    continue;
                givesCheck = MoveGen::givesCheck(pos, m);
                givesCheckComputed = true;
                if (!givesCheck)
                    continue;
                if (negSEE(m)) // Needed because m.score() is not computed for non-captures
                    continue;
            } else {
                if (negSEE(m))
                    continue;
                int capt = ::pieceValue[pos.getPiece(m.to())];
                int prom = ::pieceValue[m.promoteTo()];
                int optimisticScore = evalScore + capt + prom + deltaPruningMargin;
                if (optimisticScore < alpha) { // Delta pruning
                    if ((pos.wMtrlPawns() > 0) && (pos.wMtrl() > capt + pos.wMtrlPawns()) &&
                        (pos.bMtrlPawns() > 0) && (pos.bMtrl() > capt + pos.bMtrlPawns())) {
                        if (depth -1 > -2) {
                            givesCheck = MoveGen::givesCheck(pos, m);
                            givesCheckComputed = true;
                        }
                        if (!givesCheck) {
                            if (optimisticScore > bestScore)
                                bestScore = optimisticScore;
                            continue;
                        }
                    }
                }
            }
        }
        if (depth < -6 && mi >= 2)
            continue;
        if (!realInCheckComputed) {
            realInCheck = MoveGen::inCheck(pos);
            realInCheckComputed = true;
        }
        if (!MoveGen::isLegal(pos, m, realInCheck))
            continue;

        if (!givesCheckComputed && (depth - 1 > -2))
            givesCheck = MoveGen::givesCheck(pos, m);
        const bool nextInCheck = (depth - 1) > -2 ? givesCheck : false;

        pos.makeMove(m, ui);
        totalNodes++;
        nodesToGo--;
        score = -quiesce(-beta, -alpha, ply + 1, depth - 1, nextInCheck);
        pos.unMakeMove(m, ui);
        if (score > bestScore) {
            bestScore = score;
            if (score > alpha) {
                if (depth == 0) {
                    SearchTreeInfo& sti = searchTreeInfo[ply];
                    sti.bestMove.setMove(m.from(), m.to(), m.promoteTo(), score);
                }
                alpha = score;
                if (alpha >= beta)
                    return alpha;
            }
        }
    }
    return bestScore;
}

int
Search::SEE(Position& pos, const Move& m, int alpha, int beta) {
    int captures[64];   // Value of captured pieces
    const int kV = ::kV;

    const int square = m.to();
    if (square == pos.getEpSquare()) {
        captures[0] = ::pV;
    } else {
        captures[0] = ::pieceValue[pos.getPiece(square)];
    }
    int nCapt = 1;                  // Number of entries in captures[]

    UndoInfo ui;
    pos.makeSEEMove(m, ui);
    bool white = pos.isWhiteMove();
    int valOnSquare = ::pieceValue[pos.getPiece(square)];
    U64 occupied = pos.occupiedBB();
    int currScore = -captures[0];
    int tmp = alpha; alpha = -beta; beta = -tmp;
    while (true) {
        if ((currScore + valOnSquare <= alpha) || (currScore >= beta))
            break;
        alpha = std::max(alpha, currScore);
        int bestValue = std::numeric_limits<int>::max();
        U64 atk;
        if (white) {
            atk = BitBoard::bPawnAttacks[square] & pos.pieceTypeBB(Piece::WPAWN) & occupied;
            if (atk != 0) {
                bestValue = ::pV;
            } else {
                atk = BitBoard::knightAttacks[square] & pos.pieceTypeBB(Piece::WKNIGHT) & occupied;
                if (atk != 0) {
                    bestValue = ::nV;
                } else {
                    U64 bAtk = BitBoard::bishopAttacks(square, occupied) & occupied;
                    atk = bAtk & pos.pieceTypeBB(Piece::WBISHOP);
                    if (atk != 0) {
                        bestValue = ::bV;
                    } else {
                        U64 rAtk = BitBoard::rookAttacks(square, occupied) & occupied;
                        atk = rAtk & pos.pieceTypeBB(Piece::WROOK);
                        if (atk != 0) {
                            bestValue = ::rV;
                        } else {
                            atk = (bAtk | rAtk) & pos.pieceTypeBB(Piece::WQUEEN);
                            if (atk != 0) {
                                bestValue = ::qV;
                            } else {
                                atk = BitBoard::kingAttacks[square] & pos.pieceTypeBB(Piece::WKING) & occupied;
                                if (atk != 0) {
                                    bestValue = kV;
                                } else {
                                    break;
                                }
                            }
                        }
                    }
                }
            }
        } else {
            atk = BitBoard::wPawnAttacks[square] & pos.pieceTypeBB(Piece::BPAWN) & occupied;
            if (atk != 0) {
                bestValue = ::pV;
            } else {
                atk = BitBoard::knightAttacks[square] & pos.pieceTypeBB(Piece::BKNIGHT) & occupied;
                if (atk != 0) {
                    bestValue = ::nV;
                } else {
                    U64 bAtk = BitBoard::bishopAttacks(square, occupied) & occupied;
                    atk = bAtk & pos.pieceTypeBB(Piece::BBISHOP);
                    if (atk != 0) {
                        bestValue = ::bV;
                    } else {
                        U64 rAtk = BitBoard::rookAttacks(square, occupied) & occupied;
                        atk = rAtk & pos.pieceTypeBB(Piece::BROOK);
                        if (atk != 0) {
                            bestValue = ::rV;
                        } else {
                            atk = (bAtk | rAtk) & pos.pieceTypeBB(Piece::BQUEEN);
                            if (atk != 0) {
                                bestValue = ::qV;
                            } else {
                                atk = BitBoard::kingAttacks[square] & pos.pieceTypeBB(Piece::BKING) & occupied;
                                if (atk != 0) {
                                    bestValue = kV;
                                } else {
                                    break;
                                }
                            }
                        }
                    }
                }
            }
        }
        captures[nCapt++] = valOnSquare;
        if (valOnSquare == kV)
            break;
        currScore = -(currScore + valOnSquare);
        int tmp = alpha; alpha = -beta; beta = -tmp;
        valOnSquare = bestValue;
        occupied &= ~(atk & -atk);
        white = !white;
    }
    pos.unMakeSEEMove(m, ui);

    int score = 0;
    for (int i = nCapt - 1; i > 0; i--)
        score = std::max(0, captures[i] - score);
    return captures[0] - score;
}

void
Search::scoreMoveList(MoveList& moves, int ply, int startIdx) {
    for (int i = startIdx; i < moves.size; i++) {
        Move& m = moves[i];
        bool isCapture = (pos.getPiece(m.to()) != Piece::EMPTY) || (m.promoteTo() != Piece::EMPTY);
        int score = 0;
        if (isCapture) {
            int seeScore = signSEE(m);
            int v = pos.getPiece(m.to());
            int a = pos.getPiece(m.from());
            score = Evaluate::pieceValueOrder[v] * 8 - Evaluate::pieceValueOrder[a];
            if (seeScore > 0)
                score += 100;
            else if (seeScore == 0)
                score += 50;
            else
                score -= 50;
            score *= 100;
        } else {
            int ks = kt.getKillerScore(ply, m);
            if (ks > 0) {
                score += ks + 50;
            } else {
                int hs = ht.getHistScore(pos, m);
                score += hs;
            }
        }
        m.setScore(score);
    }
}

bool
Search::selectHashMove(MoveList& moves, const Move& hashMove) {
    for (int i = 0; i < moves.size; i++) {
        Move& m = moves[i];
        if (m.equals(hashMove)) {
            m.setScore(10000);
            std::swap(moves[i], moves[0]);
            return true;
        }
    }
    return false;
}

void
Search::setThreadNo(int tNo) {
    threadNo = tNo;
}

void
Search::initSearchTreeInfo() {
    bool useNullMove = UciParams::useNullMove->getBoolPar();
    for (size_t i = 0; i < COUNT_OF(searchTreeInfo); i++) {
        searchTreeInfo[i].allowNullMove = useNullMove;
        searchTreeInfo[i].singularMove.setMove(0,0,0,0);
        searchTreeInfo[i].abdadaExclusive = false;
    }
}
