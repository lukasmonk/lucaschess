/*
    Texel - A UCI chess engine.
    Copyright (C) 2015-2016  Peter Österlund, peterosterlund2@gmail.com

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
 * proofgame.cpp
 *
 *  Created on: Aug 15, 2015
 *      Author: petero
 */

#include "proofgame.hpp"
#include "moveGen.hpp"
#include "textio.hpp"
#include "util/timeUtil.hpp"

#include <iostream>
#include <climits>


bool ProofGame::staticInitDone = false;
U64 ProofGame::wPawnReachable[64];
U64 ProofGame::bPawnReachable[64];

const int ProofGame::bigCost;


void
ProofGame::staticInit() {
    if (staticInitDone)
        return;

    for (int y = 7; y >= 0; y--) {
        for (int x = 0; x < 8; x++) {
            int sq = Position::getSquare(x, y);
            U64 mask = 1ULL << sq;
            if (y < 7) {
                mask |= wPawnReachable[sq + 8];
                if (x > 0)
                    mask |= wPawnReachable[sq + 7];
                if (x < 7)
                    mask |= wPawnReachable[sq + 9];
            }
            wPawnReachable[sq] = mask;
        }
    }

    for (int y = 0; y < 8; y++) {
        for (int x = 0; x < 8; x++) {
            int sq = Position::getSquare(x, y);
            U64 mask = 1ULL << sq;
            if (y > 0) {
                mask |= bPawnReachable[sq - 8];
                if (x > 0)
                    mask |= bPawnReachable[sq - 9];
                if (x < 7)
                    mask |= bPawnReachable[sq - 7];
            }
            bPawnReachable[sq] = mask;
        }
    }

    staticInitDone = true;
}

ProofGame::ProofGame(const std::string& goal, int a, int b)
    : queue(TreeNodeCompare(nodes, a, b)) {
    goalPos = TextIO::readFEN(goal);
    validatePieceCounts(goalPos);
    for (int p = Piece::WKING; p <= Piece::BPAWN; p++)
        goalPieceCnt[p] = BitBoard::bitCount(goalPos.pieceTypeBB((Piece::Type)p));

    // Handle en passant square by solving for the position before the
    // the forced pawn move that set up the en passant square.
    int epSq = goalPos.getEpSquare();
    if (epSq != -1) {
        int f, t, p;
        if (goalPos.isWhiteMove()) {
            f = epSq + 8;
            t = epSq - 8;
            p = Piece::BPAWN;
        } else {
            f = epSq - 8;
            t = epSq + 8;
            p = Piece::WPAWN;
        }
        epMove = Move(f, t, Piece::EMPTY);
        goalPos.setPiece(f, p);
        goalPos.setPiece(t, Piece::EMPTY);
        goalPos.setEpSquare(-1);
        goalPos.setWhiteMove(!goalPos.isWhiteMove());
    }

    // Set up caches
    pathDataCache.resize(PathCacheSize);

    Matrix<int> m(8, 8);
    captureAP[0] = Assignment<int>(m);
    captureAP[1] = Assignment<int>(m);
    for (int c = 0; c < 2; c++) {
        for (int n = 0; n <= maxMoveAPSize; n++) {
            Matrix<int> m(n, n);
            moveAP[c][n] = Assignment<int>(m);
        }
    }

    staticInit();
}

void
ProofGame::validatePieceCounts(const Position& pos) {
    int pieceCnt[Piece::nPieceTypes];
    for (int p = Piece::WKING; p <= Piece::BPAWN; p++)
        pieceCnt[p] = BitBoard::bitCount(pos.pieceTypeBB((Piece::Type)p));

    // White must not have too many pieces
    int maxWPawns = 8;
    maxWPawns -= std::max(0, pieceCnt[Piece::WKNIGHT] - 2);
    maxWPawns -= std::max(0, pieceCnt[Piece::WBISHOP] - 2);
    maxWPawns -= std::max(0, pieceCnt[Piece::WROOK  ] - 2);
    maxWPawns -= std::max(0, pieceCnt[Piece::WQUEEN ] - 1);
    if (pieceCnt[Piece::WPAWN] > maxWPawns)
        throw ChessParseError("Too many white pieces");

    // Black must not have too many pieces
    int maxBPawns = 8;
    maxBPawns -= std::max(0, pieceCnt[Piece::BKNIGHT] - 2);
    maxBPawns -= std::max(0, pieceCnt[Piece::BBISHOP] - 2);
    maxBPawns -= std::max(0, pieceCnt[Piece::BROOK  ] - 2);
    maxBPawns -= std::max(0, pieceCnt[Piece::BQUEEN ] - 1);
    if (pieceCnt[Piece::BPAWN] > maxBPawns)
        throw ChessParseError("Too many black pieces");
}

int
ProofGame::search(const std::string& initialFen, std::vector<Move>& movePath) {
    Position startPos = TextIO::readFEN(initialFen);
    validatePieceCounts(startPos);
    addPosition(startPos, 0, true);

    double t0 = currentTime();
    Position pos;
    U64 numNodes = 0;
    int minCost = -1;
    int best = INT_MAX;
    UndoInfo ui;
    while (!queue.empty()) {
        const U32 idx = queue.top();
        queue.pop();
        const TreeNode& tn = nodes[idx];
        if (tn.ply + tn.bound >= best)
            continue;
        if (tn.ply + tn.bound > minCost) {
            minCost = tn.ply + tn.bound;
            std::cout << "min cost: " << minCost << " queue: " << queue.size()
                      << " nodes:" << numNodes
                      << " time:" << (currentTime() - t0) << std::endl;
        }

        numNodes++;

        pos.deSerialize(tn.psd);
        if (tn.ply < best && isSolution(pos)) {
            getSolution(startPos, idx, movePath);
            best = tn.ply;
        }

        U64 blocked;
        if (!computeBlocked(pos, blocked))
            continue;

#if 0
        static int cnt = 0;
        if (((++cnt) % 10000) == 0)
            std::cout << "ply:" << tn.ply << " bound:" << tn.bound << " "
                      << TextIO::toFEN(pos) << std::endl;
#endif

        MoveList moves;
        MoveGen::pseudoLegalMoves(pos, moves);
        MoveGen::removeIllegal(pos, moves);
        for (int i = 0; i < moves.size; i++) {
            if (((1ULL << moves[i].from()) | (1ULL << moves[i].to())) & blocked)
                continue;
            pos.makeMove(moves[i], ui);
            addPosition(pos, idx, false);
            pos.unMakeMove(moves[i], ui);
        }
    }
    double t1 = currentTime();
    std::cout << "nodes: " << numNodes
              << " time: " << t1 - t0 <<  std::endl;

    int epCost = epMove.isEmpty() ? 0 : 1;
    return best + epCost;
}

void
ProofGame::addPosition(const Position& pos, U32 parent, bool isRoot) {
    const int ply = isRoot ? 0 : nodes[parent].ply + 1;
    auto it = nodeHash.find(pos.zobristHash());
    if ((it != nodeHash.end()) && (it->second <= ply))
        return;

    TreeNode tn;
    pos.serialize(tn.psd);
    tn.parent = parent;
    tn.ply = ply;
    int bound = distLowerBound(pos);
    if (bound < INT_MAX) {
        tn.bound = bound;
        U32 idx = nodes.size();
        nodes.push_back(tn);
        nodeHash[pos.zobristHash()] = ply;
        queue.push(idx);
    }
}

void
ProofGame::getSolution(const Position& startPos, int idx, std::vector<Move>& movePath) const {
    std::function<void(int)> getMoves = [this,&movePath,&getMoves](U32 idx) {
        const TreeNode& tn = nodes[idx];
        if (tn.ply == 0)
            return;
        getMoves(tn.parent);
        Position target;
        target.deSerialize(tn.psd);

        Position pos;
        pos.deSerialize(nodes[tn.parent].psd);
        MoveList moves;
        MoveGen::pseudoLegalMoves(pos, moves);
        MoveGen::removeIllegal(pos, moves);
        UndoInfo ui;
        for (int i = 0; i < moves.size; i++) {
            pos.makeMove(moves[i], ui);
            if (pos.equals(target)) {
                pos.unMakeMove(moves[i], ui);
                movePath.push_back(moves[i]);
                break;
            }
            pos.unMakeMove(moves[i], ui);
        }
    };
    movePath.clear();
    getMoves(idx);
    if (!epMove.isEmpty())
        movePath.push_back(epMove);
    std::cout << nodes[idx].ply << ": ";
    Position pos = startPos;
    UndoInfo ui;
    for (size_t i = 0; i < movePath.size(); i++) {
        if (i > 0)
            std::cout << ' ';
        std::cout << TextIO::moveToString(pos, movePath[i], false);
        pos.makeMove(movePath[i], ui);
    }
    std::cout << std::endl;
}


// --------------------------------------------------------------------------------

int
ProofGame::distLowerBound(const Position& pos) {
    int pieceCnt[Piece::nPieceTypes];
    for (int p = Piece::WKING; p <= Piece::BPAWN; p++)
        pieceCnt[p] = BitBoard::bitCount(pos.pieceTypeBB((Piece::Type)p));

    if (!enoughRemainingPieces(pieceCnt))
        return INT_MAX;

    U64 blocked;
    if (!computeBlocked(pos, blocked))
        return INT_MAX;

    const int numWhiteExtraPieces = (BitBoard::bitCount(pos.whiteBB()) -
                                     BitBoard::bitCount(goalPos.whiteBB()));
    const int numBlackExtraPieces = (BitBoard::bitCount(pos.blackBB()) -
                                     BitBoard::bitCount(goalPos.blackBB()));
    const int excessWPawns = pieceCnt[Piece::WPAWN] - goalPieceCnt[Piece::WPAWN];
    const int excessBPawns = pieceCnt[Piece::BPAWN] - goalPieceCnt[Piece::BPAWN];

    if (!capturesFeasible(pos, pieceCnt, numWhiteExtraPieces, numBlackExtraPieces,
                          excessWPawns, excessBPawns))
        return INT_MAX;

    int neededMoves[2] = {0, 0};
    if (!computeNeededMoves(pos, blocked, numWhiteExtraPieces, numBlackExtraPieces,
                            excessWPawns, excessBPawns, neededMoves))
        return INT_MAX;

    // Compute number of needed moves to perform all captures
    int nBlackToCapture = BitBoard::bitCount(pos.blackBB()) - BitBoard::bitCount(goalPos.blackBB());
    int nWhiteToCapture = BitBoard::bitCount(pos.whiteBB()) - BitBoard::bitCount(goalPos.whiteBB());
    neededMoves[0] = std::max(neededMoves[0], nBlackToCapture);
    neededMoves[1] = std::max(neededMoves[1], nWhiteToCapture);

    // Compute number of needed plies from number of needed white/black moves
    int wNeededPlies = neededMoves[0] * 2;
    int bNeededPlies = neededMoves[1] * 2;
    if (pos.isWhiteMove())
        bNeededPlies++;
    else
        wNeededPlies++;
    if (goalPos.isWhiteMove())
        bNeededPlies--;
    else
        wNeededPlies--;
    int ret = std::max(wNeededPlies, bNeededPlies);
    assert(ret >= 0);
    return ret;
}

bool
ProofGame::enoughRemainingPieces(int pieceCnt[]) const {
    int wProm = pieceCnt[Piece::WPAWN] - goalPieceCnt[Piece::WPAWN];
    if (wProm < 0)
        return false;
    wProm -= std::max(0, goalPieceCnt[Piece::WQUEEN] - pieceCnt[Piece::WQUEEN]);
    if (wProm < 0)
        return false;
    wProm -= std::max(0, goalPieceCnt[Piece::WROOK] - pieceCnt[Piece::WROOK]);
    if (wProm < 0)
        return false;
    wProm -= std::max(0, goalPieceCnt[Piece::WBISHOP] - pieceCnt[Piece::WBISHOP]);
    if (wProm < 0)
        return false;
    wProm -= std::max(0, goalPieceCnt[Piece::WKNIGHT] - pieceCnt[Piece::WKNIGHT]);
    if (wProm < 0)
        return false;

    int bProm = pieceCnt[Piece::BPAWN] - goalPieceCnt[Piece::BPAWN];
    if (bProm < 0)
        return false;
    bProm -= std::max(0, goalPieceCnt[Piece::BQUEEN] - pieceCnt[Piece::BQUEEN]);
    if (bProm < 0)
        return false;
    bProm -= std::max(0, goalPieceCnt[Piece::BROOK] - pieceCnt[Piece::BROOK]);
    if (bProm < 0)
        return false;
    bProm -= std::max(0, goalPieceCnt[Piece::BBISHOP] - pieceCnt[Piece::BBISHOP]);
    if (bProm < 0)
        return false;
    bProm -= std::max(0, goalPieceCnt[Piece::BKNIGHT] - pieceCnt[Piece::BKNIGHT]);
    if (bProm < 0)
        return false;
    return true;
}

bool
ProofGame::capturesFeasible(const Position& pos, int pieceCnt[],
                             int numWhiteExtraPieces, int numBlackExtraPieces,
                             int excessWPawns, int excessBPawns) {
    for (int c = 0; c < 2; c++) {
        const Piece::Type  p = (c == 0) ? Piece::WPAWN : Piece::BPAWN;
        Assignment<int>& as = captureAP[c];
        U64 from = pos.pieceTypeBB(p);
        int fi = 0;
        while (from) {
            int fromSq = BitBoard::extractSquare(from);
            U64 to = goalPos.pieceTypeBB(p);
            int ti = 0;
            while (to) {
                int toSq = BitBoard::extractSquare(to);
                int d = std::abs(Position::getX(fromSq) - Position::getX(toSq));
                as.setCost(fi, ti, d);
                ti++;
            }
            for ( ; ti < 8; ti++)
                as.setCost(fi, ti, 0);    // sq -> captured, no cost
            fi++;
        }
        for (; fi < 8; fi++) {
            int ti;
            for (ti = 0; ti < goalPieceCnt[p]; ti++)
                as.setCost(fi, ti, bigCost); // captured -> sq, not possible
            for ( ; ti < 8; ti++)
                as.setCost(fi, ti, 0);       // captured -> captured, no cost
        }

        const std::vector<int>& s = as.optWeightMatch();
        int cost = 0;
        for (int i = 0; i < 8; i++)
            cost += as.getCost(i, s[i]);
        if (c == 0) {
            int neededBCaptured = cost;
            if (neededBCaptured > numBlackExtraPieces)
                return false;

            int neededBProm = (std::max(0, goalPieceCnt[Piece::BQUEEN]  - pieceCnt[Piece::BQUEEN]) +
                               std::max(0, goalPieceCnt[Piece::BROOK]   - pieceCnt[Piece::BROOK]) +
                               std::max(0, goalPieceCnt[Piece::BBISHOP] - pieceCnt[Piece::BBISHOP]) +
                               std::max(0, goalPieceCnt[Piece::BKNIGHT] - pieceCnt[Piece::BKNIGHT]));
            int excessBPieces = (std::max(0, pieceCnt[Piece::BQUEEN]  - goalPieceCnt[Piece::BQUEEN]) +
                                 std::max(0, pieceCnt[Piece::BROOK]   - goalPieceCnt[Piece::BROOK]) +
                                 std::max(0, pieceCnt[Piece::BBISHOP] - goalPieceCnt[Piece::BBISHOP]) +
                                 std::max(0, pieceCnt[Piece::BKNIGHT] - goalPieceCnt[Piece::BKNIGHT]));
            if (neededBCaptured + neededBProm > excessBPawns + excessBPieces)
                return false;
        } else {
            int neededWCaptured = cost;
            if (neededWCaptured > numWhiteExtraPieces)
                return false;

            int neededWProm = (std::max(0, goalPieceCnt[Piece::WQUEEN]  - pieceCnt[Piece::WQUEEN]) +
                               std::max(0, goalPieceCnt[Piece::WROOK]   - pieceCnt[Piece::WROOK]) +
                               std::max(0, goalPieceCnt[Piece::WBISHOP] - pieceCnt[Piece::WBISHOP]) +
                               std::max(0, goalPieceCnt[Piece::WKNIGHT] - pieceCnt[Piece::WKNIGHT]));
            int excessWPieces = (std::max(0, pieceCnt[Piece::WQUEEN]  - goalPieceCnt[Piece::WQUEEN]) +
                                 std::max(0, pieceCnt[Piece::WROOK]   - goalPieceCnt[Piece::WROOK]) +
                                 std::max(0, pieceCnt[Piece::WBISHOP] - goalPieceCnt[Piece::WBISHOP]) +
                                 std::max(0, pieceCnt[Piece::WKNIGHT] - goalPieceCnt[Piece::WKNIGHT]));

            if (neededWCaptured + neededWProm > excessWPawns + excessWPieces)
                return false;
        }
    }
    return true;
}

bool
ProofGame::computeNeededMoves(const Position& pos, U64 blocked,
                               int numWhiteExtraPieces, int numBlackExtraPieces,
                               int excessWPawns, int excessBPawns,
                               int neededMoves[]) {
    std::vector<SqPathData> sqPathData;
    SqPathData promPath[2][8]; // Pawn cost to each possible promotion square
    if (!computeShortestPathData(pos, numWhiteExtraPieces, numBlackExtraPieces,
                                 promPath, sqPathData, blocked))
        return false;

    // Sets of squares where pieces have to be captured
    U64 captureSquares[2][16]; // [c][idx], 0 terminated.
    // Sum of zero bits in captureSquares
    int nCaptureConstraints[2];

    // Compute initial estimate of captureSquares[0]. Further computations are based
    // on analyzing the assignment problem cost matrix for the other side.
    for (int c = 0; c < 1; c++) {
        // If a pawn has to go to a square in front of a blocked square, a capture has
        // to be made on the pawn goal square.
        const bool whiteToBeCaptured = (c == 0);
        const Piece::Type pawn = whiteToBeCaptured ? Piece::BPAWN : Piece::WPAWN;
        U64 capt = goalPos.pieceTypeBB(pawn) & ~pos.pieceTypeBB(pawn);
        capt &= whiteToBeCaptured ? (blocked >> 8) : (blocked << 8);
        int idx = 0;
        while (capt) {
            captureSquares[c][idx++] = 1ULL << BitBoard::extractSquare(capt);
        }
        nCaptureConstraints[c] = 63 * idx;
        captureSquares[c][idx++] = 0;
    }
    captureSquares[1][0] = 0;
    nCaptureConstraints[1] = 0;

    int c = 0;
    bool solved[2] = { false, false };
    while (!solved[c]) {
        const bool wtm = (c == 0);
        const int maxCapt = wtm ? numBlackExtraPieces : numWhiteExtraPieces;
        int cost = 0;
        U64 fromPieces = (wtm ? pos.whiteBB() : pos.blackBB()) & ~blocked;
        int N = BitBoard::bitCount(fromPieces);
        if (N > 0) {
            assert(N <= maxMoveAPSize);
            Assignment<int>& as = moveAP[c][N];
            int rowToSq[16], colToSq[16];
            for (int f = 0; f < N; f++) {
                assert(fromPieces != 0);
                const int fromSq = BitBoard::extractSquare(fromPieces);
                rowToSq[f] = fromSq;
                bool canPromote = wtm ? (excessWPawns > 0) && (pos.getPiece(fromSq) == Piece::WPAWN)
                                      : (excessBPawns > 0) && (pos.getPiece(fromSq) == Piece::BPAWN);
                int t = 0;
                for (size_t ci = 0; ci < sqPathData.size(); ci++) {
                    int toSq = sqPathData[ci].square;
                    int p = goalPos.getPiece(toSq);
                    if (Piece::isWhite(p) == wtm) {
                        assert(t < N);
                        colToSq[t] = toSq;
                        int pLen;
                        if (p == pos.getPiece(fromSq)) {
                            pLen = sqPathData[ci].spd->pathLen[fromSq];
                            if (pLen < 0)
                                pLen = bigCost;
                        } else
                            pLen = bigCost;
                        if (canPromote) {
                            int cost2 = promPathLen(wtm, fromSq, p, blocked, maxCapt,
                                                    *sqPathData[ci].spd, promPath[c]);
                            pLen = std::min(pLen, cost2);
                        }
                        as.setCost(f, t, pLen < 0 ? bigCost : pLen);
                        t++;
                    }
                }

                // Handle pieces to be captured
                int idx = 0;
                for (; t < N; t++) {
                    int cost = 0;
                    U64 captSquares = captureSquares[c][idx];
                    if (captSquares) {
                        cost = minDistToSquares(pos.getPiece(fromSq), fromSq, blocked, maxCapt,
                                                promPath[c], captSquares, canPromote);
                        idx++;
                    }
                    colToSq[t] = -1;
                    as.setCost(f, t, cost);
                }
                if (captureSquares[c][idx])
                    return false;
            }
            cost = solveAssignment(as);
            if (cost >= bigCost)
                return false;

            int nConstr;
            if (!computeAllCutSets(as, rowToSq, colToSq, wtm, blocked, maxCapt,
                                   captureSquares[1-c], nConstr))
                return false;
            assert(nCaptureConstraints[1-c] <= nConstr);
            if (nCaptureConstraints[1-c] < nConstr) {
                nCaptureConstraints[1-c] = nConstr;
                solved[1-c] = false;
            }
        }
        neededMoves[c] = cost;
        solved[c] = true;
        c = 1 - c;
    }
    return true;
}

bool
ProofGame::computeShortestPathData(const Position& pos,
                                    int numWhiteExtraPieces, int numBlackExtraPieces,
                                    SqPathData promPath[2][8],
                                    std::vector<SqPathData>& sqPathData, U64& blocked) {
    std::vector<SqPathData> pending;
    U64 pieces = goalPos.occupiedBB() & ~blocked;
    while (pieces) {
        int sq = BitBoard::extractSquare(pieces);
        pending.emplace_back(sq, nullptr);
    }
    while (!pending.empty()) {
        const auto e = pending.back();
        pending.pop_back();
        const int sq = e.square;
        const Piece::Type p = (Piece::Type)goalPos.getPiece(sq);
        const bool wtm = Piece::isWhite(p);
        const int maxCapt = wtm ? numBlackExtraPieces : numWhiteExtraPieces;
        auto spd = shortestPaths(p, sq, blocked, maxCapt);
        bool testPromote = false;
        switch (p) {
        case Piece::WQUEEN: case Piece::WROOK: case Piece::WBISHOP: case Piece::WKNIGHT:
            if (wtm && Position::getY(sq) == 7)
                testPromote = true;
            break;
        case Piece::BQUEEN: case Piece::BROOK: case Piece::BBISHOP: case Piece::BKNIGHT:
            if (!wtm && Position::getY(sq) == 0)
                testPromote = true;
            break;
        default:
            break;
        }
        bool promotionPossible = false;
        if (testPromote) {
            int c = wtm ? 0 : 1;
            int x = Position::getX(sq);
            Piece::Type pawn = wtm ? Piece::WPAWN : Piece::BPAWN;
            if (!promPath[c][x].spd)
                promPath[c][x].spd = shortestPaths(pawn, sq, blocked, maxCapt);
            if (promPath[c][x].spd->fromSquares & pos.pieceTypeBB(pawn))
                promotionPossible = true;
        }
        if ((spd->fromSquares == (1ULL << sq)) && !promotionPossible) {
            // Piece on goal square can not move, must be same piece on sq in pos
            if (pos.getPiece(sq) != p)
                return false;
            blocked |= 1ULL << sq;
            pending.insert(pending.end(), sqPathData.begin(), sqPathData.end());
            sqPathData.clear();
            for (int c = 0; c < 2; c++)
                for (int x = 0; x < 8; x++)
                    promPath[c][x].spd = nullptr;
        } else {
            sqPathData.emplace_back(sq, spd);
        }
    }
    return true;
}

int
ProofGame::promPathLen(bool wtm, int fromSq, int targetPiece, U64 blocked, int maxCapt,
                        const ShortestPathData& toSqPath, SqPathData promPath[8]) {
    int pLen = INT_MAX;
    switch (targetPiece) {
    case Piece::WQUEEN:  case Piece::BQUEEN:
    case Piece::WROOK:   case Piece::BROOK:
    case Piece::WBISHOP: case Piece::BBISHOP:
    case Piece::WKNIGHT: case Piece::BKNIGHT: {
        for (int x = 0; x < 8; x++) {
            int promSq = wtm ? 7*8 + x : x;
            if (!promPath[x].spd)
                promPath[x].spd = shortestPaths(wtm ? Piece::WPAWN : Piece::BPAWN,
                                                promSq, blocked, maxCapt);
            int promCost = promPath[x].spd->pathLen[fromSq];
            if (promCost >= 0) {
                int tmp = toSqPath.pathLen[promSq];
                if (tmp >= 0)
                    pLen = std::min(pLen, promCost + tmp);
            }
        }
        break;
    default:
        break;
    }
    }
    return pLen;
}

bool
ProofGame::computeAllCutSets(const Assignment<int>& as, int rowToSq[16], int colToSq[16],
                              bool wtm, U64 blocked, int maxCapt,
                              U64 cutSets[16], int& nConstraints) {
    const int N = as.getSize();
    int nCutSets = 0;
    for (int t = 0; t < N; t++) {
        int toSq = colToSq[t];
        if (toSq == -1)
            break;
        int p = goalPos.getPiece(toSq);
        if (p == (wtm ? Piece::WPAWN : Piece::BPAWN)) {
            U64 fromSqMask = 0;
            for (int f = 0; f < N; f++)
                if (as.getCost(f, t) < bigCost)
                    fromSqMask |= 1ULL << rowToSq[f];
            if (!computeCutSets(wtm, fromSqMask, toSq, blocked, maxCapt, cutSets, nCutSets))
                return false;
        }
    }
    cutSets[nCutSets++] = 0;

    int nConstr = 0;
    for (int i = 0; cutSets[i] != 0; i++) {
        assert(i < 16);
        nConstr += BitBoard::bitCount(~cutSets[i]);
    }
    nConstraints = nConstr;

    return true;
}

bool
ProofGame::computeCutSets(bool wtm, U64 fromSqMask, int toSq, U64 blocked, int maxCapt,
                           U64 cutSets[16], int& nCutSets) {
    U64 allPaths = 0;
    U64 m = fromSqMask;
    while (m) {
        int fromSq = BitBoard::extractSquare(m);
        allPaths |= allPawnPaths(wtm, fromSq, toSq, blocked, maxCapt);
    }
    if (!allPaths)
        return true;

    int n = nCutSets;
    U64 oldSquares = 0;
    U64 newSquares = 1ULL << toSq;
    while (true) {
        // Add squares reachable by non-capture moves
        while (true) {
            U64 tmp = (wtm ? (newSquares >> 8) : (newSquares << 8)) & allPaths;
            if ((newSquares | tmp) == newSquares)
                break;
            newSquares |= tmp;
        }

        if (newSquares & fromSqMask)
            break;

        if (n >= 15)
            return false;
        cutSets[n++] = newSquares & ~oldSquares;
        oldSquares = newSquares;

        // Add squares reachable by capture moves
        newSquares |= (wtm ? BitBoard::bPawnAttacksMask(newSquares)
                           : BitBoard::wPawnAttacksMask(newSquares)) & allPaths;
    }
    cutSets[n] = 0;
    nCutSets = n;
    return true;
}

U64
ProofGame::allPawnPaths(bool wtm, int fromSq, int toSq, U64 blocked, int maxCapt) {
    int yDelta = Position::getY(fromSq) - Position::getY(toSq);
    maxCapt = std::min(maxCapt, std::abs(yDelta)); // Can't make use of more than yDelta captures
    Piece::Type pawn = wtm ? Piece::WPAWN : Piece::BPAWN;
    Piece::Type oPawn = wtm ? Piece::BPAWN : Piece::WPAWN;
    U64 mask = 0;
    for (int c = 0; c <= maxCapt; c++) {
        auto tData = shortestPaths(pawn, toSq, blocked, c);
        auto fData = shortestPaths(oPawn, fromSq, blocked, maxCapt-c);
        mask |= tData->fromSquares & fData->fromSquares;
    }
    return mask;
}

int
ProofGame::minDistToSquares(int piece, int fromSq, U64 blocked, int maxCapt,
                             SqPathData promPath[8], U64 targetSquares, bool canPromote) {
    const bool wtm = Piece::isWhite(piece);
    int best = bigCost;
    while (targetSquares) {
        int captSq = BitBoard::extractSquare(targetSquares);
        auto spd = shortestPaths((Piece::Type)piece, captSq, blocked, maxCapt);
        int pLen = spd->pathLen[fromSq];
        if (pLen < 0)
            pLen = bigCost;
        if (canPromote)
            pLen = promPathLen(wtm, fromSq, blocked, maxCapt, captSq, promPath, pLen);
        best = std::min(best, pLen);
    }
    return best;
}

int
ProofGame::promPathLen(bool wtm, int fromSq, U64 blocked, int maxCapt,
                        int toSq, SqPathData promPath[8], int pLen) {
    int firstP = wtm ? Piece::WQUEEN : Piece::BQUEEN;
    int lastP = wtm ? Piece::WKNIGHT : Piece::BKNIGHT;
    for (int x = 0; x < 8; x++) {
        int promSq = wtm ? 7*8 + x : x;
        if (!promPath[x].spd)
            promPath[x].spd =
                shortestPaths(wtm ? Piece::WPAWN : Piece::BPAWN,
                              promSq, blocked, maxCapt);
        int promCost = promPath[x].spd->pathLen[fromSq];
        if (promCost >= 0 && promCost < pLen) {
            int cost2 = INT_MAX;
            for (int p = firstP; p <= lastP; p++) {
                auto spd2 = shortestPaths((Piece::Type)p, toSq, blocked, maxCapt);
                int tmp = spd2->pathLen[promSq];
                if (tmp >= 0)
                    cost2 = std::min(cost2, promCost + tmp);
            }
            pLen = std::min(pLen, cost2);
        }
    }
    return pLen;
}

int
ProofGame::solveAssignment(Assignment<int>& as) {
    const int N = as.getSize();

    // Count number of choices for each row/column
    int nValidR[16];
    int nValidC[16];
    for (int i = 0; i < N; i++)
        nValidR[i] = nValidC[i] = 0;
    for (int r = 0; r < N; r++) {
        for (int c = 0; c < N; c++) {
            if (as(r,c) < bigCost) {
                nValidR[r]++;
                nValidC[c]++;
            }
        }
    }

    // Find rows/columns with exactly one choice
    U64 rowsToCheck = 0;
    U64 colsToCheck = 0;
    for (int i = 0; i < N; i++) {
        if (nValidR[i] == 1)
            rowsToCheck |= (1 << i);
        if (nValidC[i] == 1)
            colsToCheck |= (1 << i);
    }

    // If a row/column only has one choice, remove all other choices from the corresponding column/row
    U64 rowsHandled = 0;
    U64 colsHandled = 0;
    while (true) {
        bool finished = true;
        if (rowsToCheck) {
            int r = BitBoard::extractSquare(rowsToCheck);
            if ((nValidR[r] == 1) && !(rowsHandled & (1 << r))) {
                int c;
                for (c = 0; c < N; c++)
                    if (as(r, c) < bigCost)
                        break;
                for (int r2 = 0; r2 < N; r2++) {
                    if ((r2 != r) && (as(r2, c) < bigCost)) {
                        as.setCost(r2, c, bigCost);
                        if (--nValidR[r2] == 1)
                            rowsToCheck |= 1 << r2;
                    }
                }
                finished = false;
                rowsHandled |= 1 << r;
                colsHandled |= 1 << c;
            }
        }
        if (colsToCheck) {
            int c = BitBoard::extractSquare(colsToCheck);
            if ((nValidC[c] == 1) && !(colsHandled & (1 << c))) {
                int r;
                for (r = 0; r < N; r++)
                    if (as(r, c) < bigCost)
                        break;
                for (int c2 = 0; c2 < N; c2++) {
                    if ((c2 != c) && (as(r, c2) < bigCost)) {
                        as.setCost(r, c2, bigCost);
                        if (--nValidC[c2] == 1)
                            colsToCheck |= 1 << c2;
                    }
                }
                finished = false;
                rowsHandled |= 1 << r;
                colsHandled |= 1 << c;
            }
        }
        if (finished)
            break;
    }

    // Solve the assignment problem and return the optimal cost
    int cost = 0;
    const std::vector<int>& s = as.optWeightMatch();
    for (int i = 0; i < N; i++)
        cost += as.getCost(i, s[i]);
    return cost;
}

bool
ProofGame::computeBlocked(const Position& pos, U64& blocked) const {
    const U64 wGoalPawns = goalPos.pieceTypeBB(Piece::WPAWN);
    const U64 bGoalPawns = goalPos.pieceTypeBB(Piece::BPAWN);
    const U64 wCurrPawns = pos.pieceTypeBB(Piece::WPAWN);
    const U64 bCurrPawns = pos.pieceTypeBB(Piece::BPAWN);

    U64 goalUnMovedPawns = (wGoalPawns & BitBoard::maskRow2) | (bGoalPawns & BitBoard::maskRow7);
    U64 currUnMovedPawns = (wCurrPawns & BitBoard::maskRow2) | (bCurrPawns & BitBoard::maskRow7);
    if (goalUnMovedPawns & ~currUnMovedPawns)
        return false;
    blocked = goalUnMovedPawns;

    U64 wUsefulPawnSquares = 0;
    // Compute pawns that are blocked because advancing them would leave too few
    // remaining pawns in the cone of squares that can reach the pawn square.
    U64 m = wGoalPawns & ~blocked;
    while (m) {
        int sq = BitBoard::extractSquare(m);
        U64 mask = bPawnReachable[sq];
        wUsefulPawnSquares |= mask;
        int nGoal = BitBoard::bitCount(wGoalPawns & mask);
        int nCurr = BitBoard::bitCount(wCurrPawns & mask);
        if (nCurr < nGoal)
            return false;
        if ((nCurr == nGoal) && (wCurrPawns & (1ULL << sq)))
            blocked |= (1ULL << sq);
    }

    if (BitBoard::bitCount(wGoalPawns) == BitBoard::bitCount(wCurrPawns)) {
        // Compute pawns that are blocked because advancing them would put
        // them on a square from which no goal pawn square can be reached.
        m = wGoalPawns & wCurrPawns & ~blocked;
        while (m) {
            int sq = BitBoard::extractSquare(m);
            U64 tgt = BitBoard::wPawnAttacks[sq] | (1ULL << (sq + 8));
            if ((tgt & wUsefulPawnSquares) == 0)
                blocked |= (1ULL << sq);
        }
    }

    U64 bUsefulPawnSquares = 0;
    // Compute pawns that are blocked because advancing them would leave too few
    // remaining pawns in the cone of squares that can reach the pawn square.
    m = bGoalPawns & ~blocked;
    while (m) {
        int sq = BitBoard::extractSquare(m);
        U64 mask = wPawnReachable[sq];
        bUsefulPawnSquares |= mask;
        int nGoal = BitBoard::bitCount(bGoalPawns & mask);
        int nCurr = BitBoard::bitCount(bCurrPawns & mask);
        if (nCurr < nGoal)
            return false;
        if ((nCurr == nGoal) && (bCurrPawns & (1ULL << sq)))
            blocked |= (1ULL << sq);
    }

    if (BitBoard::bitCount(bGoalPawns) == BitBoard::bitCount(bCurrPawns)) {
        // Compute pawns that are blocked because advancing them would put
        // them on a square from which no goal pawn square can be reached.
        m = bGoalPawns & bCurrPawns & ~blocked;
        while (m) {
            int sq = BitBoard::extractSquare(m);
            U64 tgt = BitBoard::bPawnAttacks[sq] | (1ULL >> (sq - 8));
            if ((tgt & bUsefulPawnSquares) == 0)
                blocked |= (1ULL << sq);
        }
    }

    // Handle castling rights
    int cMask = goalPos.getCastleMask();
    if (cMask & ~pos.getCastleMask())
        return false;
    if (goalPos.h1Castle())
        blocked |= BitBoard::sqMask(E1,H1);
    if (goalPos.a1Castle())
        blocked |= BitBoard::sqMask(E1,A1);
    if (goalPos.h8Castle())
        blocked |= BitBoard::sqMask(E8,H8);
    if (goalPos.a8Castle())
        blocked |= BitBoard::sqMask(E8,A8);

    return true;
}

// --------------------------------------------------------------------------------

#if 0
template <typename T>
static void printTable(const T* tbl) {
    for (int y = 7; y >= 0; y--) {
        for (int x = 0; x < 8; x++)
            std::cout << ' ' << std::setw(2) << (int)tbl[y*8+x];
        std::cout << '\n';
    }
}
#endif

std::shared_ptr<ProofGame::ShortestPathData>
ProofGame::shortestPaths(Piece::Type p, int toSq, U64 blocked, int maxCapt) {
    if (p != Piece::WPAWN && p != Piece::BPAWN)
        maxCapt = 6;
    U64 h = blocked * 0x9e3779b97f4a7c55ULL + (int)p * 0x9e3779b97f51ULL +
            toSq * 0x9e3779cdULL + maxCapt * 0x964e55ULL;
    h &= PathCacheSize - 1;
    PathCacheEntry& entry = pathDataCache[h];
    if (entry.blocked == blocked && entry.toSq == toSq &&
        entry.piece == (int)p && entry.maxCapt == maxCapt)
        return entry.spd;

    auto spd = std::make_shared<ShortestPathData>();
    for (int i = 0; i < 64; i++)
        spd->pathLen[i] = -1;
    spd->pathLen[toSq] = 0;
    U64 reached = 1ULL << toSq;

    if (maxCapt < 6) { // pawn
        if (maxCapt == 0) {
            int sq = toSq;
            int d = (p == Piece::WPAWN) ? -8 : 8;
            int dist = 1;
            while (true) {
                sq += d;
                if ((sq < 0) || (sq > 63) || (blocked & (1ULL << sq)))
                    break;
                spd->pathLen[sq] = dist;
                reached |= 1ULL << sq;
                if (Position::getY(sq) != ((d > 0) ? 5 : 2))
                    dist++;
            }
        } else {
            auto sub = shortestPaths(p, toSq, blocked, maxCapt-1);
            auto minLen = [](int a, int b) {
                if (b != -1)
                    b++;
                if (a == -1) return b;
                if (b == -1) return a;
                return std::min(a, b);
            };
            if (p == Piece::WPAWN) {
                for (int y = Position::getY(toSq) - 1; y >= 0; y--) {
                    bool newReached = false;
                    for (int x = 0; x < 8; x++) {
                        int sq = Position::getSquare(x, y);
                        if (blocked & (1ULL << sq))
                            continue;
                        int best = sub->pathLen[sq];
                        best = minLen(best, spd->pathLen[sq+8]);
                        if ((y == 1) && !(blocked & (1ULL << (sq+8))))
                            best = minLen(best, spd->pathLen[sq+16]);
                        if (x > 0)
                            best = minLen(best, sub->pathLen[sq+7]);
                        if (x < 7)
                            best = minLen(best, sub->pathLen[sq+9]);
                        spd->pathLen[sq] = best;
                        if (best != -1) {
                            reached |= 1ULL << sq;
                            newReached = true;
                        }
                    }
                    if (!newReached)
                        break;
                }
            } else {
                for (int y = Position::getY(toSq) + 1; y < 8; y++) {
                    bool newReached = false;
                    for (int x = 0; x < 8; x++) {
                        int sq = Position::getSquare(x, y);
                        if (blocked & (1ULL << sq))
                            continue;
                        int best = sub->pathLen[sq];
                        best = minLen(best, spd->pathLen[sq-8]);
                        if ((y == 6) && !(blocked & (1ULL << (sq-8))))
                            best = minLen(best, spd->pathLen[sq-16]);
                        if (x > 0)
                            best = minLen(best, sub->pathLen[sq-9]);
                        if (x < 7)
                            best = minLen(best, sub->pathLen[sq-7]);
                        spd->pathLen[sq] = best;
                        if (best != -1) {
                            reached |= 1ULL << sq;
                            newReached = true;
                        }
                    }
                    if (!newReached)
                        break;
                }
            }
        }
    } else {
        int dist = 1;
        U64 newSquares = reached;
        while (true) {
            U64 neighbors = computeNeighbors(p, newSquares, blocked);
            newSquares = neighbors & ~reached;
            if (newSquares == 0)
                break;
            U64 m = newSquares;
            while (m) {
                int sq = BitBoard::extractSquare(m);
                spd->pathLen[sq] = dist;
            }
            reached |= newSquares;
            dist++;
        }
    }
    spd->fromSquares = reached;

    entry.piece = p;
    entry.toSq = toSq;
    entry.maxCapt = maxCapt;
    entry.blocked = blocked;
    entry.spd = spd;

    return spd;
}

U64
ProofGame::computeNeighbors(Piece::Type p, U64 toSquares, U64 blocked) {
    U64 ret = 0;
    switch (p) {
    case Piece::WKING: case Piece::BKING:
        while (toSquares) {
            int sq = BitBoard::extractSquare(toSquares);
            ret |= BitBoard::kingAttacks[sq];
        }
        break;
    case Piece::WQUEEN: case Piece::BQUEEN:
        while (toSquares) {
            int sq = BitBoard::extractSquare(toSquares);
            ret |= BitBoard::rookAttacks(sq, blocked);
            ret |= BitBoard::bishopAttacks(sq, blocked);
        }
        break;
    case Piece::WROOK: case Piece::BROOK:
        while (toSquares) {
            int sq = BitBoard::extractSquare(toSquares);
            ret |= BitBoard::rookAttacks(sq, blocked);
        }
        break;
    case Piece::WBISHOP: case Piece::BBISHOP:
        while (toSquares) {
            int sq = BitBoard::extractSquare(toSquares);
            ret |= BitBoard::bishopAttacks(sq, blocked);
        }
        break;
    case Piece::WKNIGHT: case Piece::BKNIGHT:
        while (toSquares) {
            int sq = BitBoard::extractSquare(toSquares);
            ret |= BitBoard::knightAttacks[sq];
        }
        break;
    case Piece::WPAWN: {
        U64 tmp = (toSquares >> 8) & ~blocked;
        ret |= tmp;
        ret |= (tmp & BitBoard::maskRow3) >> 8;
        ret |= BitBoard::bPawnAttacksMask(toSquares);
        break;
    }
    case Piece::BPAWN: {
        U64 tmp = (toSquares << 8) & ~blocked;
        ret |= tmp;
        ret |= (tmp & BitBoard::maskRow6) << 8;
        ret |= BitBoard::wPawnAttacksMask(toSquares);
        break;
    }
    default:
        assert(false);
    }
    return ret & ~blocked;
}
