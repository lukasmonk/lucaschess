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
 * chesstool.cpp
 *
 *  Created on: Dec 24, 2013
 *      Author: petero
 */

#include "chesstool.hpp"
#include "search.hpp"
#include "textio.hpp"
#include "gametree.hpp"
#include "computerPlayer.hpp"
#include "syzygy/rtb-probe.hpp"
#include "tbprobe.hpp"
#include "stloutput.hpp"

#include <queue>
#include <unordered_set>
#include <unistd.h>


// Static move ordering parameters
DECLARE_PARAM(moEvalWeight, 96, -128, 128, useUciParam || true);
DECLARE_PARAM(moHangPenalty1, -1, -128, 128, useUciParam || true);
DECLARE_PARAM(moHangPenalty2, -30, -128, 128, useUciParam || true);
DECLARE_PARAM(moSeeBonus, 104, -128, 128, useUciParam || true);

DEFINE_PARAM(moEvalWeight);
DEFINE_PARAM(moHangPenalty1);
DEFINE_PARAM(moHangPenalty2);
DEFINE_PARAM(moSeeBonus);

ScoreToProb::ScoreToProb(double pawnAdvantage0)
    : pawnAdvantage(pawnAdvantage0) {
    for (int i = 0; i < MAXCACHE; i++) {
        cache[i] = computeProb(i);
        logCacheP[i] = log(getProb(i));
        logCacheN[i] = log(getProb(-i));
    }
}

double
ScoreToProb::getProb(int score) const {
    bool neg = false;
    if (score < 0) {
        score = -score;
        neg = true;
    }
    double ret;
    if (score < MAXCACHE)
        ret = cache[score];
    else
        ret = computeProb(score);
    if (neg)
        ret = 1 - ret;
    return ret;
}

double
ScoreToProb::getLogProb(int score) const {
    if ((score >= 0) && (score < MAXCACHE))
        return logCacheP[score];
    if ((score < 0) && (score > -MAXCACHE))
        return logCacheN[-score];
    return log(getProb(score));
}

// --------------------------------------------------------------------------------

ChessTool::ChessTool(bool useEntropyErr, bool optMoveOrder)
    : useEntropyErrorFunction(useEntropyErr),
      optimizeMoveOrdering(optMoveOrder) {

    moEvalWeight.registerParam("MoveOrderEvalWeight", Parameters::instance());
    moHangPenalty1.registerParam("MoveOrderHangPenalty1", Parameters::instance());
    moHangPenalty2.registerParam("MoveOrderHangPenalty2", Parameters::instance());
    moSeeBonus.registerParam("MoveOrderSeeBonus", Parameters::instance());
}

void
ChessTool::setupTB() {
    UciParams::gtbPath->set("/home/petero/chess/gtb");
    UciParams::gtbCache->set("2047");
    UciParams::rtbPath->set("/home/petero/chess/rtb/wdl:"
                            "/home/petero/chess/rtb/dtz:"
                            "/home/petero/chess/rtb/6wdl:"
                            "/home/petero/chess/rtb/6dtz");
}

std::vector<std::string>
ChessTool::readFile(const std::string& fname) {
    std::ifstream is(fname);
    return readStream(is);
}

std::vector<std::string>
ChessTool::readStream(std::istream& is) {
    std::vector<std::string> ret;
    while (true) {
        std::string line;
        std::getline(is, line);
        if (!is || is.eof())
            break;
        ret.push_back(line);
    }
    return ret;
}

const int UNKNOWN_SCORE = -32767; // Represents unknown static eval score

void
ChessTool::pgnToFen(std::istream& is, int everyNth) {
    static std::vector<U64> nullHist(200);
    static TranspositionTable tt(19);
    static ParallelData pd(tt);
    static KillerTable kt;
    static History ht;
    static auto et = Evaluate::getEvalHashTables();
    static Search::SearchTables st(tt, kt, ht, *et);
    static TreeLogger treeLog;
    Random rnd;

    Position pos;
    const int mate0 = SearchConst::MATE0;
    Search sc(pos, nullHist, 0, st, pd, nullptr, treeLog);

    PgnReader reader(is);
    GameTree gt;
    int gameNo = 0;
    while (reader.readPGN(gt)) {
        gameNo++;
        GameTree::Result result = gt.getResult();
        if (result == GameTree::UNKNOWN)
            continue;
        double rScore = 0;
        switch (result) {
        case GameTree::WHITE_WIN: rScore = 1.0; break;
        case GameTree::BLACK_WIN: rScore = 0.0; break;
        case GameTree::DRAW:      rScore = 0.5; break;
        default: break;
        }
        GameNode gn = gt.getRootNode();
        while (true) {
            pos = gn.getPos();
            std::string fen = TextIO::toFEN(pos);
            if (gn.nChildren() == 0)
                break;
            gn.goForward(0);
            std::string move = TextIO::moveToUCIString(gn.getMove());
            std::string comment = gn.getComment();
            int commentScore;
            if (!getCommentScore(comment, commentScore))
                continue;

            if (everyNth > 1 && rnd.nextInt(everyNth) != 0)
                continue;

            sc.init(pos, nullHist, 0);
            sc.q0Eval = UNKNOWN_SCORE;
            int score = sc.quiesce(-mate0, mate0, 0, 0, MoveGen::inCheck(pos));
            if (!pos.isWhiteMove()) {
                score = -score;
                commentScore = -commentScore;
            }
            std::cout << fen << " : " << rScore << " : " << commentScore << " : " << score
                      << " : " << gameNo << " : " << move << '\n';
        }
    }
    std::cout << std::flush;
}

void
ChessTool::fenToPgn(std::istream& is) {
    std::vector<std::string> lines = readStream(is);
    for (const std::string& line : lines) {
        Position pos(TextIO::readFEN(line));
        writePGN(pos);
    }
}

void
ChessTool::pawnAdvTable(std::istream& is) {
    std::vector<PositionInfo> positions;
    readFENFile(is, positions);
    qEval(positions);
    for (int pawnAdvantage = 1; pawnAdvantage <= 400; pawnAdvantage += 1) {
        ScoreToProb sp(pawnAdvantage);
        double avgErr = computeAvgError(positions, sp);
        std::stringstream ss;
        ss << "pa:" << pawnAdvantage << " err:" << std::setprecision(14) << avgErr;
        std::cout << ss.str() << std::endl;
    }
}

// --------------------------------------------------------------------------------

void
ChessTool::filterScore(std::istream& is, int scLimit, double prLimit) {
    std::vector<PositionInfo> positions;
    readFENFile(is, positions);
    ScoreToProb sp;
    Position pos;
    for (const PositionInfo& pi : positions) {
        double p1 = sp.getProb(pi.searchScore);
        double p2 = sp.getProb(pi.qScore);
        if ((std::abs(p1 - p2) < prLimit) && (std::abs(pi.searchScore - pi.qScore) < scLimit)) {
            pos.deSerialize(pi.posData);
            std::string fen = TextIO::toFEN(pos);
            std::cout << fen << " : " << pi.result << " : " << pi.searchScore << " : " << pi.qScore
                      << " : " << pi.gameNo << '\n';
        }
    }
    std::cout << std::flush;
}

static int
swapSquareY(int square) {
    int x = Position::getX(square);
    int y = Position::getY(square);
    return Position::getSquare(x, 7-y);
}

static Position
swapColors(const Position& pos) {
    Position sym;
    sym.setWhiteMove(!pos.isWhiteMove());
    for (int x = 0; x < 8; x++) {
        for (int y = 0; y < 8; y++) {
            int sq = Position::getSquare(x, y);
            int p = pos.getPiece(sq);
            p = Piece::isWhite(p) ? Piece::makeBlack(p) : Piece::makeWhite(p);
            sym.setPiece(swapSquareY(sq), p);
        }
    }

    int castleMask = 0;
    if (pos.a1Castle()) castleMask |= 1 << Position::A8_CASTLE;
    if (pos.h1Castle()) castleMask |= 1 << Position::H8_CASTLE;
    if (pos.a8Castle()) castleMask |= 1 << Position::A1_CASTLE;
    if (pos.h8Castle()) castleMask |= 1 << Position::H1_CASTLE;
    sym.setCastleMask(castleMask);

    if (pos.getEpSquare() >= 0)
        sym.setEpSquare(swapSquareY(pos.getEpSquare()));

    sym.setHalfMoveClock(pos.getHalfMoveClock());
    sym.setFullMoveCounter(pos.getFullMoveCounter());

    return sym;
}

static int nPieces(const Position& pos, Piece::Type piece) {
    return BitBoard::bitCount(pos.pieceTypeBB(piece));
}

static bool isMatch(int v1, bool compare, int v2) {
    return !compare || (v1 == v2);
}

void
ChessTool::filterMtrlBalance(std::istream& is, bool minorEqual,
                             const std::vector<std::pair<bool,int>>& mtrlPattern) {
    std::vector<PositionInfo> positions;
    readFENFile(is, positions);
    Position pos;
    int mtrlDiff[5];
    for (const PositionInfo& pi : positions) {
        pos.deSerialize(pi.posData);
        mtrlDiff[0] = nPieces(pos, Piece::WQUEEN)  - nPieces(pos, Piece::BQUEEN);
        mtrlDiff[1] = nPieces(pos, Piece::WROOK)   - nPieces(pos, Piece::BROOK);
        int nComp;
        if (minorEqual) {
            mtrlDiff[2] = nPieces(pos, Piece::WBISHOP) - nPieces(pos, Piece::BBISHOP) +
                          nPieces(pos, Piece::WKNIGHT) - nPieces(pos, Piece::BKNIGHT);
            mtrlDiff[3] = nPieces(pos, Piece::WPAWN)   - nPieces(pos, Piece::BPAWN);
            nComp = 4;
        } else {
            mtrlDiff[2] = nPieces(pos, Piece::WBISHOP) - nPieces(pos, Piece::BBISHOP);
            mtrlDiff[3] = nPieces(pos, Piece::WKNIGHT) - nPieces(pos, Piece::BKNIGHT);
            mtrlDiff[4] = nPieces(pos, Piece::WPAWN)   - nPieces(pos, Piece::BPAWN);
            nComp = 5;
        }
        bool inc1 = true, inc2 = true;
        for (int i = 0; i < nComp; i++) {
            if (!isMatch(mtrlDiff[i], mtrlPattern[i].first, mtrlPattern[i].second))
                inc1 = false;
            if (!isMatch(mtrlDiff[i], mtrlPattern[i].first, -mtrlPattern[i].second))
                inc2 = false;
        }
        int sign = 1;
        if (inc2 && !inc1) {
            pos = swapColors(pos);
            sign = -1;
        }
        if (inc1 || inc2) {
            std::string fen = TextIO::toFEN(pos);
            std::cout << fen << " : " << ((sign>0)?pi.result:(1-pi.result)) << " : "
                      << pi.searchScore * sign << " : " << pi.qScore * sign
                      << " : " << pi.gameNo << '\n';
        }
    }
    std::cout << std::flush;
}

void
ChessTool::filterTotalMaterial(std::istream& is, bool minorEqual,
                               const std::vector<std::pair<bool,int>>& mtrlPattern) {
    std::vector<PositionInfo> positions;
    readFENFile(is, positions);

    Position pos;
    for (const PositionInfo& pi : positions) {
        pos.deSerialize(pi.posData);
        int wQ = nPieces(pos, Piece::WQUEEN);
        int wR = nPieces(pos, Piece::WROOK);
        int wB = nPieces(pos, Piece::WBISHOP);
        int wN = nPieces(pos, Piece::WKNIGHT);
        int wP = nPieces(pos, Piece::WPAWN);
        int bQ = nPieces(pos, Piece::BQUEEN);
        int bR = nPieces(pos, Piece::BROOK);
        int bB = nPieces(pos, Piece::BBISHOP);
        int bN = nPieces(pos, Piece::BKNIGHT);
        int bP = nPieces(pos, Piece::BPAWN);

        bool inc1, inc2;
        if (minorEqual) {
            inc1 = isMatch(wQ,    mtrlPattern[0].first, mtrlPattern[0].second) &&
                   isMatch(wR,    mtrlPattern[1].first, mtrlPattern[1].second) &&
                   isMatch(wB+wN, mtrlPattern[2].first, mtrlPattern[2].second) &&
                   isMatch(wP,    mtrlPattern[3].first, mtrlPattern[3].second) &&
                   isMatch(bQ,    mtrlPattern[4].first, mtrlPattern[4].second) &&
                   isMatch(bR,    mtrlPattern[5].first, mtrlPattern[5].second) &&
                   isMatch(bB+bN, mtrlPattern[6].first, mtrlPattern[6].second) &&
                   isMatch(bP,    mtrlPattern[7].first, mtrlPattern[7].second);
            inc2 = isMatch(bQ,    mtrlPattern[0].first, mtrlPattern[0].second) &&
                   isMatch(bR,    mtrlPattern[1].first, mtrlPattern[1].second) &&
                   isMatch(bB+bN, mtrlPattern[2].first, mtrlPattern[2].second) &&
                   isMatch(bP,    mtrlPattern[3].first, mtrlPattern[3].second) &&
                   isMatch(wQ,    mtrlPattern[4].first, mtrlPattern[4].second) &&
                   isMatch(wR,    mtrlPattern[5].first, mtrlPattern[5].second) &&
                   isMatch(wB+wN, mtrlPattern[6].first, mtrlPattern[6].second) &&
                   isMatch(wP,    mtrlPattern[7].first, mtrlPattern[7].second);
        } else {
            inc1 = isMatch(wQ, mtrlPattern[0].first, mtrlPattern[0].second) &&
                   isMatch(wR, mtrlPattern[1].first, mtrlPattern[1].second) &&
                   isMatch(wB, mtrlPattern[2].first, mtrlPattern[2].second) &&
                   isMatch(wN, mtrlPattern[3].first, mtrlPattern[3].second) &&
                   isMatch(wP, mtrlPattern[4].first, mtrlPattern[4].second) &&
                   isMatch(bQ, mtrlPattern[5].first, mtrlPattern[5].second) &&
                   isMatch(bR, mtrlPattern[6].first, mtrlPattern[6].second) &&
                   isMatch(bB, mtrlPattern[7].first, mtrlPattern[7].second) &&
                   isMatch(bN, mtrlPattern[8].first, mtrlPattern[8].second) &&
                   isMatch(bP, mtrlPattern[9].first, mtrlPattern[9].second);
            inc2 = isMatch(bQ, mtrlPattern[0].first, mtrlPattern[0].second) &&
                   isMatch(bR, mtrlPattern[1].first, mtrlPattern[1].second) &&
                   isMatch(bB, mtrlPattern[2].first, mtrlPattern[2].second) &&
                   isMatch(bN, mtrlPattern[3].first, mtrlPattern[3].second) &&
                   isMatch(bP, mtrlPattern[4].first, mtrlPattern[4].second) &&
                   isMatch(wQ, mtrlPattern[5].first, mtrlPattern[5].second) &&
                   isMatch(wR, mtrlPattern[6].first, mtrlPattern[6].second) &&
                   isMatch(wB, mtrlPattern[7].first, mtrlPattern[7].second) &&
                   isMatch(wN, mtrlPattern[8].first, mtrlPattern[8].second) &&
                   isMatch(wP, mtrlPattern[9].first, mtrlPattern[9].second);
        }
        int sign = 1;
        if (inc2 && !inc1) {
            pos = swapColors(pos);
            sign = -1;
        }
        if (inc1 || inc2) {
            std::string fen = TextIO::toFEN(pos);
            std::cout << fen << " : " << ((sign>0)?pi.result:(1-pi.result)) << " : "
                      << pi.searchScore * sign << " : " << pi.qScore * sign
                      << " : " << pi.gameNo << '\n';
        }
    }
    std::cout << std::flush;
}

void
ChessTool::outliers(std::istream& is, int threshold) {
    std::vector<PositionInfo> positions;
    readFENFile(is, positions);
    qEval(positions);
    Position pos;
    for (const PositionInfo& pi : positions) {
        if (((pi.qScore >=  threshold) && (pi.result < 1.0)) ||
            ((pi.qScore <= -threshold) && (pi.result > 0.0))) {
            pos.deSerialize(pi.posData);
            std::string fen = TextIO::toFEN(pos);
            std::cout << fen << " : " << pi.result << " : " << pi.searchScore << " : " << pi.qScore
                      << " : " << pi.gameNo << '\n';
        }
    }
    std::cout << std::flush;
}

void
ChessTool::evalEffect(std::istream& is, const std::vector<ParamValue>& parValues) {
    std::vector<PositionInfo> positions;
    readFENFile(is, positions);
    qEval(positions);

    for (PositionInfo& pi : positions)
        pi.searchScore = pi.qScore;

    Parameters& uciPars = Parameters::instance();
    for (const ParamValue& pv : parValues)
        uciPars.set(pv.name, num2Str(pv.value));

    qEval(positions);
    ScoreToProb sp;
    Position pos;
    for (const PositionInfo& pi : positions) {
        if (pi.qScore == pi.searchScore)
            continue;

        double evErr0 = std::abs(sp.getProb(pi.searchScore) - pi.result);
        double evErr1 = std::abs(sp.getProb(pi.qScore)      - pi.result);
        double improvement = evErr0 - evErr1;

        std::stringstream ss;
        ss.precision(6);
        ss << std::fixed << improvement;

        pos.deSerialize(pi.posData);
        std::string fen = TextIO::toFEN(pos);
        std::cout << fen << " : " << pi.result << " : " << pi.searchScore << " : " << pi.qScore
                  << " : " << pi.gameNo << " : " << ss.str() << '\n';
    }
    std::cout << std::flush;
}

// --------------------------------------------------------------------------------

void
ChessTool::paramEvalRange(std::istream& is, ParamDomain& pd) {
    std::vector<PositionInfo> positions;
    readFENFile(is, positions);

    ScoreToProb sp;
    double bestVal = 1e100;
    for (int i = pd.minV; i <= pd.maxV; i += pd.step) {
        Parameters::instance().set(pd.name, num2Str(i));
        double avgErr = computeObjective(positions, sp);
        bool best = avgErr < bestVal;
        bestVal = std::min(bestVal, avgErr);
        std::stringstream ss;
        ss << "i:" << i << " err:" << std::setprecision(14) << avgErr << (best?" *":"");
        std::cout << ss.str() << std::endl;
    }
}

struct PrioParam {
    PrioParam(ParamDomain& pd0) : priority(1), pd(&pd0) {}
    double priority;
    ParamDomain* pd;
    bool operator<(const PrioParam& o) const {
        return priority < o.priority;
    }
};

// --------------------------------------------------------------------------------

void
ChessTool::accumulateATA(std::vector<PositionInfo>& positions, int beg, int end,
                         const ScoreToProb& sp,
                         std::vector<ParamDomain>& pdVec,
                         arma::mat& aTa, arma::mat& aTb,
                         arma::mat& ePos, arma::mat& eNeg) {
    Parameters& uciPars = Parameters::instance();
    const int M = end - beg;
    const int N = pdVec.size();
    const double w = 1.0 / positions.size();

    arma::mat b(M, 1);
    qEval(positions, beg, end);
    for (int i = beg; i < end; i++)
        b.at(i-beg,0) = positions[i].getErr(sp) * w;

    arma::mat A(M, N);
    for (int j = 0; j < N; j++) {
        ParamDomain& pd = pdVec[j];
        std::cout << "j:" << j << " beg:" << beg << " name:" << pd.name << std::endl;
        const int v0 = pd.value;
        const int vPos = std::min(pd.maxV, pd.value + 1);
        const int vNeg = std::max(pd.minV, pd.value - 1);
        assert(vPos > vNeg);

        uciPars.set(pd.name, num2Str(vPos));
        qEval(positions, beg, end);
        double EPos = 0;
        for (int i = beg; i < end; i++) {
            const double err = positions[i].getErr(sp);
            A.at(i-beg,j) = err;
            EPos += err * err;
        }
        ePos.at(j, 0) += sqrt(EPos * w);

        uciPars.set(pd.name, num2Str(vNeg));
        qEval(positions, beg, end);
        double ENeg = 0;
        for (int i = beg; i < end; i++) {
            const double err = positions[i].getErr(sp);
            A.at(i-beg,j) = (A.at(i-beg,j) - err) / (vPos - vNeg) * w;
            ENeg += err * err;
        }
        eNeg.at(j, 0) += sqrt(ENeg * w);

        uciPars.set(pd.name, num2Str(v0));
    }

    aTa += A.t() * A;
    aTb += A.t() * b;
}

void
ChessTool::gnOptimize(std::istream& is, std::vector<ParamDomain>& pdVec) {
    double t0 = currentTime();
    std::vector<PositionInfo> positions;
    readFENFile(is, positions);
    const int nPos = positions.size();

    const int N = pdVec.size();
    arma::mat bestP(N, 1);
    for (int i = 0; i < N; i++)
        bestP.at(i, 0) = pdVec[i].value;
    ScoreToProb sp;
    double bestAvgErr = computeAvgError(positions, sp, pdVec, bestP);
    {
        std::stringstream ss;
        ss << "Initial error: " << std::setprecision(14) << bestAvgErr;
        std::cout << ss.str() << std::endl;
    }

    const int chunkSize = 250000000 / N;

    while (true) {
        arma::mat aTa(N, N);  aTa.fill(0.0);
        arma::mat aTb(N, 1);  aTb.fill(0.0);
        arma::mat ePos(N, 1); ePos.fill(0.0);
        arma::mat eNeg(N, 1); eNeg.fill(0.0);

        for (int i = 0; i < nPos; i += chunkSize) {
            const int end = std::min(nPos, i + chunkSize);
            accumulateATA(positions, i, end, sp, pdVec, aTa, aTb, ePos, eNeg);
        }

        arma::mat delta = pinv(aTa) * aTb;
        bool improved = false;
        for (double alpha = 1.0; alpha >= 0.25; alpha /= 2) {
            arma::mat newP = bestP - delta * alpha;
            for (int i = 0; i < N; i++)
                newP.at(i, 0) = clamp((int)std::round(newP.at(i, 0)), pdVec[i].minV, pdVec[i].maxV);
            double avgErr = computeAvgError(positions, sp, pdVec, newP);
            for (int i = 0; i < N; i++) {
                ParamDomain& pd = pdVec[i];
                std::stringstream ss;
                ss << pd.name << ' ' << newP.at(i, 0) << ' ' << std::setprecision(14) << avgErr << ((avgErr < bestAvgErr) ? " *" : "");
                std::cout << ss.str() << std::endl;
            }
            if (avgErr < bestAvgErr) {
                bestP = newP;
                bestAvgErr = avgErr;
                improved = true;
                break;
            }
        }
        if (!improved)
            break;
    }
    double t1 = currentTime();
    ::usleep(100000);
    std::cerr << "Elapsed time: " << t1 - t0 << std::endl;
}

// --------------------------------------------------------------------------------

void
ChessTool::localOptimize(std::istream& is, std::vector<ParamDomain>& pdVec) {
    double t0 = currentTime();
    Parameters& uciPars = Parameters::instance();
    std::vector<PositionInfo> positions;
    readFENFile(is, positions);

    std::priority_queue<PrioParam> queue;
    for (ParamDomain& pd : pdVec)
        queue.push(PrioParam(pd));

    ScoreToProb sp;
    double bestAvgErr = computeObjective(positions, sp);
    {
        std::stringstream ss;
        ss << "Initial error: " << std::setprecision(14) << bestAvgErr;
        std::cout << ss.str() << std::endl;
    }

    std::vector<PrioParam> tried;
    while (!queue.empty()) {
        PrioParam pp = queue.top(); queue.pop();
        ParamDomain& pd = *pp.pd;
        std::cout << pd.name << " prio:" << pp.priority << " q:" << queue.size()
                  << " min:" << pd.minV << " max:" << pd.maxV << " val:" << pd.value << std::endl;
        double oldBest = bestAvgErr;
        bool improved = false;
        for (int d = 0; d < 2; d++) {
            while (true) {
                const int newValue = pd.value + (d ? -1 : 1);
                if ((newValue < pd.minV) || (newValue > pd.maxV))
                    break;

                uciPars.set(pd.name, num2Str(newValue));
                double avgErr = computeObjective(positions, sp);
                uciPars.set(pd.name, num2Str(pd.value));

                std::stringstream ss;
                ss << pd.name << ' ' << newValue << ' ' << std::setprecision(14) << avgErr << ((avgErr < bestAvgErr) ? " *" : "");
                std::cout << ss.str() << std::endl;

                if (avgErr >= bestAvgErr)
                    break;
                bestAvgErr = avgErr;
                pd.value = newValue;
                uciPars.set(pd.name, num2Str(pd.value));
                improved = true;
            }
            if (improved)
                break;
        }
        double improvement = oldBest - bestAvgErr;
        std::cout << pd.name << " improvement:" << improvement << std::endl;
        pp.priority = pp.priority * 0.1 + improvement * 0.9;
        if (improved) {
            for (PrioParam& pp2 : tried)
                queue.push(pp2);
            tried.clear();
        }
        tried.push_back(pp);
    }

    double t1 = currentTime();
    ::usleep(100000);
    std::cerr << "Elapsed time: " << t1 - t0 << std::endl;
}

static void
updateMinMax(std::map<int,double>& funcValues, int bestV, int& minV, int& maxV) {
    auto it = funcValues.find(bestV);
    assert(it != funcValues.end());
    if (it != funcValues.begin()) {
        auto it2 = it; --it2;
        int nextMinV = it2->first;
        minV = std::max(minV, nextMinV);
    }
    auto it2 = it; ++it2;
    if (it2 != funcValues.end()) {
        int nextMaxV = it2->first;
        maxV = std::min(maxV, nextMaxV);
    }
}

static int
estimateMin(std::map<int,double>& funcValues, int bestV, int minV, int maxV) {
    return (minV + maxV) / 2;
}

void
ChessTool::localOptimize2(std::istream& is, std::vector<ParamDomain>& pdVec) {
    double t0 = currentTime();
    Parameters& uciPars = Parameters::instance();
    std::vector<PositionInfo> positions;
    readFENFile(is, positions);

    std::priority_queue<PrioParam> queue;
    for (ParamDomain& pd : pdVec)
        queue.push(PrioParam(pd));

    ScoreToProb sp;
    double bestAvgErr = computeObjective(positions, sp);
    {
        std::stringstream ss;
        ss << "Initial error: " << std::setprecision(14) << bestAvgErr;
        std::cout << ss.str() << std::endl;
    }

    std::vector<PrioParam> tried;
    while (!queue.empty()) {
        PrioParam pp = queue.top(); queue.pop();
        ParamDomain& pd = *pp.pd;
        std::cout << pd.name << " prio:" << pp.priority << " q:" << queue.size()
                  << " min:" << pd.minV << " max:" << pd.maxV << " val:" << pd.value << std::endl;
        double oldBest = bestAvgErr;

        std::map<int, double> funcValues;
        funcValues[pd.value] = bestAvgErr;
        int minV = pd.minV;
        int maxV = pd.maxV;
        while (true) {
            bool improved = false;
            for (int d = 0; d < 2; d++) {
                const int newValue = pd.value + (d ? -1 : 1);
                if ((newValue < minV) || (newValue > maxV))
                    continue;
                if (funcValues.count(newValue) == 0) {
                    uciPars.set(pd.name, num2Str(newValue));
                    double avgErr = computeObjective(positions, sp);
                    funcValues[newValue] = avgErr;
                    uciPars.set(pd.name, num2Str(pd.value));
                    std::stringstream ss;
                    ss << pd.name << ' ' << newValue << ' ' << std::setprecision(14) << avgErr << ((avgErr < bestAvgErr) ? " *" : "");
                    std::cout << ss.str() << std::endl;
                }
                if (funcValues[newValue] < bestAvgErr) {
                    bestAvgErr = funcValues[newValue];
                    pd.value = newValue;
                    uciPars.set(pd.name, num2Str(pd.value));
                    updateMinMax(funcValues, pd.value, minV, maxV);
                    improved = true;

                    const int estimatedMinValue = estimateMin(funcValues, pd.value, minV, maxV);
                    if ((estimatedMinValue >= minV) && (estimatedMinValue <= maxV) &&
                        (funcValues.count(estimatedMinValue) == 0)) {
                        uciPars.set(pd.name, num2Str(estimatedMinValue));
                        double avgErr = computeObjective(positions, sp);
                        funcValues[estimatedMinValue] = avgErr;
                        uciPars.set(pd.name, num2Str(pd.value));
                        std::stringstream ss;
                        ss << pd.name << ' ' << estimatedMinValue << ' ' << std::setprecision(14) << avgErr << ((avgErr < bestAvgErr) ? " *" : "");
                        std::cout << ss.str() << std::endl;

                        if (avgErr < bestAvgErr) {
                            bestAvgErr = avgErr;
                            pd.value = estimatedMinValue;
                            uciPars.set(pd.name, num2Str(pd.value));
                            updateMinMax(funcValues, pd.value, minV, maxV);
                            break;
                        }
                    }
                }
            }
            if (!improved)
                break;
        }
        double improvement = oldBest - bestAvgErr;
        std::cout << pd.name << " improvement:" << improvement << std::endl;
        pp.priority = pp.priority * 0.1 + improvement * 0.9;
        if (improvement > 0) {
            for (PrioParam& pp2 : tried)
                queue.push(pp2);
            tried.clear();
        }
        tried.push_back(pp);
    }

    double t1 = currentTime();
    ::usleep(100000);
    std::cerr << "Elapsed time: " << t1 - t0 << std::endl;
}

// --------------------------------------------------------------------------------

void
ChessTool::simplify(std::istream& is, std::vector<ParamDomain>& zeroPars,
                    std::vector<ParamDomain>& approxPars) {
    double t0 = currentTime();
    Parameters& uciPars = Parameters::instance();
    std::vector<PositionInfo> positions;
    readFENFile(is, positions);

    qEval(positions);
    for (PositionInfo& pi : positions)
        pi.searchScore = pi.qScore; // Use searchScore to store original evaluation

    for (ParamDomain& pd : zeroPars)
        uciPars.set(pd.name, "0");

    std::priority_queue<PrioParam> queue;
    for (ParamDomain& pd : approxPars)
        queue.push(PrioParam(pd));

    ScoreToProb sp;
    auto computeAvgErr = [&positions,&sp]() -> double {
        double errSum = 0;
        for (const PositionInfo& pi : positions) {
            double p0 = sp.getProb(pi.searchScore);
            double p1 = sp.getProb(pi.qScore);
            double err = p1 - p0;
            errSum += err * err;
        }
        return sqrt(errSum / positions.size());
    };

    qEval(positions);
    double bestAvgErr = computeAvgErr();
    {
        std::stringstream ss;
        ss << "Initial error: " << std::setprecision(14) << bestAvgErr;
        std::cout << ss.str() << std::endl;
    }

    std::vector<PrioParam> tried;
    while (!queue.empty()) {
        PrioParam pp = queue.top(); queue.pop();
        ParamDomain& pd = *pp.pd;
        std::cout << pd.name << " prio:" << pp.priority << " q:" << queue.size()
                  << " min:" << pd.minV << " max:" << pd.maxV << " val:" << pd.value << std::endl;
        double oldBest = bestAvgErr;
        bool improved = false;
        for (int d = 0; d < 2; d++) {
            while (true) {
                const int newValue = pd.value + (d ? -1 : 1);
                if ((newValue < pd.minV) || (newValue > pd.maxV))
                    break;

                uciPars.set(pd.name, num2Str(newValue));
                qEval(positions);
                double avgErr = computeAvgErr();
                uciPars.set(pd.name, num2Str(pd.value));

                std::stringstream ss;
                ss << pd.name << ' ' << newValue << ' ' << std::setprecision(14) << avgErr
                   << ((avgErr < bestAvgErr) ? " *" : "");
                std::cout << ss.str() << std::endl;

                if (avgErr >= bestAvgErr)
                    break;
                bestAvgErr = avgErr;
                pd.value = newValue;
                uciPars.set(pd.name, num2Str(pd.value));
                improved = true;
            }
            if (improved)
                break;
        }
        double improvement = oldBest - bestAvgErr;
        std::cout << pd.name << " improvement:" << improvement << std::endl;
        pp.priority = pp.priority * 0.1 + improvement * 0.9;
        if (improved) {
            for (PrioParam& pp2 : tried)
                queue.push(pp2);
            tried.clear();
        }
        tried.push_back(pp);
    }

    double t1 = currentTime();
    ::usleep(100000);
    std::cerr << "Elapsed time: " << t1 - t0 << std::endl;
}

// --------------------------------------------------------------------------------

template <int N>
static void
printTableNxN(const ParamTable<N*N>& pt, const std::string& name,
              std::ostream& os) {
    os << name << ":" << std::endl;
    for (int y = 0; y < N; y++) {
        os << "    " << ((y == 0) ? "{" : " ");
        for (int x = 0; x < N; x++) {
            os << std::setw(4) << pt[y*N+x] << (((y==N-1) && (x == N-1)) ? " }," : ",");
        }
        os << std::endl;
    }
}

template <int N>
static void
printTable(const ParamTable<N>& pt, const std::string& name, std::ostream& os) {
    os << name << ":" << std::endl;
    os << "    {";
    for (int i = 0; i < N; i++)
        os << std::setw(3) << pt[i] << ((i == N-1) ? " }," : ",");
    os << std::endl;
}

void
ChessTool::printParams() {
    std::ostream& os = std::cout;
    printTableNxN<8>(kt1b, "kt1b", os);
    printTableNxN<8>(kt2b, "kt2b", os);
    printTableNxN<8>(pt1b, "pt1b", os);
    printTableNxN<8>(pt2b, "pt2b", os);
    printTableNxN<8>(nt1b, "nt1b", os);
    printTableNxN<8>(nt2b, "nt2b", os);
    printTableNxN<8>(bt1b, "bt1b", os);
    printTableNxN<8>(bt2b, "bt2b", os);
    printTableNxN<8>(qt1b, "qt1b", os);
    printTableNxN<8>(qt2b, "qt2b", os);
    printTableNxN<8>(rt1b, "rt1b", os);
    printTableNxN<8>(knightOutpostBonus, "knightOutpostBonus", os);
    printTableNxN<8>(protectedPawnBonus, "protectedPawnBonus", os);
    printTableNxN<8>(attackedPawnBonus, "attackedPawnBonus", os);
    printTable(protectBonus, "protectBonus", os);

    printTable(rookMobScore, "rookMobScore", os);
    printTable(bishMobScore, "bishMobScore", os);
    printTable(knightMobScore, "knightMobScore", os);
    printTable(queenMobScore, "queenMobScore", os);
    printTableNxN<4>(majorPieceRedundancy, "majorPieceRedundancy", os);
    printTableNxN<6>(connectedPPBonus, "connectedPPBonus", os);
    printTable(passedPawnBonusX, "passedPawnBonusX", os);
    printTable(passedPawnBonusY, "passedPawnBonusY", os);
    printTable(ppBlockerBonus, "ppBlockerBonus", os);
    printTable(candidatePassedBonus, "candidatePassedBonus", os);
    printTable(QvsRRBonus, "QvsRRBonus", os);
    printTable(RvsMBonus, "RvsMBonus", os);
    printTable(RvsMMBonus, "RvsMMBonus", os);
    printTable(bishopPairValue, "bishopPairValue", os);
    printTable(rookEGDrawFactor, "rookEGDrawFactor", os);
    printTable(RvsBPDrawFactor, "RvsBPDrawFactor", os);
    printTable(castleFactor, "castleFactor", os);
    printTable(pawnShelterTable, "pawnShelterTable", os);
    printTable(pawnStormTable, "pawnStormTable", os);
    printTable(kingAttackWeight, "kingAttackWeight", os);
    printTable(qContactCheckBonus, "qContactCheckBonus", os);
    printTable(pieceKingAttackBonus, "pieceKingAttackBonus", os);
    printTable(kingPPSupportK, "kingPPSupportK", os);
    printTable(kingPPSupportP, "kingPPSupportP", os);
    printTable(pawnDoubledPenalty, "pawnDoubledPenalty", os);
    printTable(pawnIsolatedPenalty, "pawnIsolatedPenalty", os);
    printTable(halfMoveFactor, "halfMoveFactor", os);
    printTable(stalePawnFactor, "stalePawnFactor", os);

    os << "pV : " << pV << std::endl;
    os << "nV : " << nV << std::endl;
    os << "bV : " << bV << std::endl;
    os << "rV : " << rV << std::endl;
    os << "qV : " << qV << std::endl;

    os << "pawnIslandPenalty        : " << pawnIslandPenalty << std::endl;
    os << "pawnBackwardPenalty      : " << pawnBackwardPenalty << std::endl;
    os << "pawnSemiBackwardPenalty1 : " << pawnSemiBackwardPenalty1 << std::endl;
    os << "pawnSemiBackwardPenalty2 : " << pawnSemiBackwardPenalty2 << std::endl;
    os << "pawnRaceBonus            : " << pawnRaceBonus << std::endl;
    os << "passedPawnEGFactor       : " << passedPawnEGFactor << std::endl;
    os << "RBehindPP1               : " << RBehindPP1 << std::endl;
    os << "RBehindPP2               : " << RBehindPP2 << std::endl;
    os << "activePawnPenalty        : " << activePawnPenalty << std::endl;

    os << "QvsRMBonus1         : " << QvsRMBonus1 << std::endl;
    os << "QvsRMBonus2         : " << QvsRMBonus2 << std::endl;
    os << "knightVsQueenBonus1 : " << knightVsQueenBonus1 << std::endl;
    os << "knightVsQueenBonus2 : " << knightVsQueenBonus2 << std::endl;
    os << "knightVsQueenBonus3 : " << knightVsQueenBonus3 << std::endl;
    os << "krkpBonus           : " << krkpBonus << std::endl;
    os << "krpkbBonus           : " << krpkbBonus << std::endl;
    os << "krpkbPenalty         : " << krpkbPenalty << std::endl;
    os << "krpknBonus           : " << krpknBonus << std::endl;
    os << "RvsBPBonus           : " << RvsBPBonus << std::endl;

    os << "pawnTradePenalty    : " << pawnTradePenalty << std::endl;
    os << "pieceTradeBonus     : " << pieceTradeBonus << std::endl;
    os << "pawnTradeThreshold  : " << pawnTradeThreshold << std::endl;
    os << "pieceTradeThreshold : " << pieceTradeThreshold << std::endl;

    os << "threatBonus1     : " << threatBonus1 << std::endl;
    os << "threatBonus2     : " << threatBonus2 << std::endl;
    os << "latentAttackBonus: " << latentAttackBonus << std::endl;

    os << "rookHalfOpenBonus     : " << rookHalfOpenBonus << std::endl;
    os << "rookOpenBonus         : " << rookOpenBonus << std::endl;
    os << "rookDouble7thRowBonus : " << rookDouble7thRowBonus << std::endl;
    os << "trappedRookPenalty1   : " << trappedRookPenalty1 << std::endl;
    os << "trappedRookPenalty2   : " << trappedRookPenalty2 << std::endl;

    os << "bishopPairPawnPenalty : " << bishopPairPawnPenalty << std::endl;
    os << "trappedBishopPenalty  : " << trappedBishopPenalty << std::endl;
    os << "oppoBishopPenalty     : " << oppoBishopPenalty << std::endl;

    os << "kingSafetyHalfOpenBCDEFG1 : " << kingSafetyHalfOpenBCDEFG1 << std::endl;
    os << "kingSafetyHalfOpenBCDEFG2 : " << kingSafetyHalfOpenBCDEFG2 << std::endl;
    os << "kingSafetyHalfOpenAH1     : " << kingSafetyHalfOpenAH1 << std::endl;
    os << "kingSafetyHalfOpenAH2     : " << kingSafetyHalfOpenAH2 << std::endl;
    os << "kingSafetyWeight1         : " << kingSafetyWeight1 << std::endl;
    os << "kingSafetyWeight2         : " << kingSafetyWeight2 << std::endl;
    os << "kingSafetyWeight3         : " << kingSafetyWeight3 << std::endl;
    os << "kingSafetyWeight4         : " << kingSafetyWeight4 << std::endl;
    os << "kingSafetyThreshold       : " << kingSafetyThreshold << std::endl;
    os << "knightKingProtectBonus    : " << knightKingProtectBonus << std::endl;
    os << "bishopKingProtectBonus    : " << bishopKingProtectBonus << std::endl;
    os << "pawnStormBonus            : " << pawnStormBonus << std::endl;

    os << "pawnLoMtrl          : " << pawnLoMtrl << std::endl;
    os << "pawnHiMtrl          : " << pawnHiMtrl << std::endl;
    os << "minorLoMtrl         : " << minorLoMtrl << std::endl;
    os << "minorHiMtrl         : " << minorHiMtrl << std::endl;
    os << "castleLoMtrl        : " << castleLoMtrl << std::endl;
    os << "castleHiMtrl        : " << castleHiMtrl << std::endl;
    os << "queenLoMtrl         : " << queenLoMtrl << std::endl;
    os << "queenHiMtrl         : " << queenHiMtrl << std::endl;
    os << "passedPawnLoMtrl    : " << passedPawnLoMtrl << std::endl;
    os << "passedPawnHiMtrl    : " << passedPawnHiMtrl << std::endl;
    os << "kingSafetyLoMtrl    : " << kingSafetyLoMtrl << std::endl;
    os << "kingSafetyHiMtrl    : " << kingSafetyHiMtrl << std::endl;
    os << "oppoBishopLoMtrl    : " << oppoBishopLoMtrl << std::endl;
    os << "oppoBishopHiMtrl    : " << oppoBishopHiMtrl << std::endl;
    os << "knightOutpostLoMtrl : " << knightOutpostLoMtrl << std::endl;
    os << "knightOutpostHiMtrl : " << knightOutpostHiMtrl << std::endl;

    os << "moEvalWeight   : " << moEvalWeight << std::endl;
    os << "moHangPenalty1 : " << moHangPenalty1 << std::endl;
    os << "moHangPenalty2 : " << moHangPenalty2 << std::endl;
    os << "moSeeBonus     : " << moSeeBonus << std::endl;
}

static bool strContains(const std::string& str, const std::string& sub) {
    return str.find(sub) != std::string::npos;
}

static int
findLine(const std::string start, const std::string& contain, const std::vector<std::string>& lines) {
    for (int i = 0; i < (int)lines.size(); i++) {
        const std::string& line = lines[i];
        if (startsWith(line, start) && strContains(line, contain))
            return i;
    }
    return -1;
}

std::vector<std::string> splitLines(const std::string& lines) {
    std::vector<std::string> ret;
    int start = 0;
    for (int i = 0; i < (int)lines.size(); i++) {
        if (lines[i] == '\n') {
            ret.push_back(lines.substr(start, i - start));
            start = i + 1;
        }
    }
    return ret;
}

template <int N>
static void
replaceTableNxN(const ParamTable<N*N>& pt, const std::string& name,
             std::vector<std::string>& cppFile) {
    int lineNo = findLine("ParamTable", " " + name + " ", cppFile);
    if (lineNo < 0)
        throw ChessParseError(name + " not found");
    if (lineNo + N >= (int)cppFile.size())
        throw ChessParseError("unexpected end of file");

    std::stringstream ss;
    printTableNxN<N>(pt, name, ss);
    std::vector<std::string> replaceLines = splitLines(ss.str());
    if (replaceLines.size() != N + 1)
        throw ChessParseError("Wrong number of replacement lines");
    for (int i = 1; i <= N; i++)
        cppFile[lineNo + i] = replaceLines[i];
}

template <int N>
static void
replaceTable(const ParamTable<N>& pt, const std::string& name,
           std::vector<std::string>& cppFile) {
    int lineNo = findLine("ParamTable", " " + name + " ", cppFile);
    if (lineNo < 0)
        throw ChessParseError(name + " not found");
    if (lineNo + 1 >= (int)cppFile.size())
        throw ChessParseError("unexpected end of file");

    std::stringstream ss;
    printTable<N>(pt, name, ss);
    std::vector<std::string> replaceLines = splitLines(ss.str());
    if (replaceLines.size() != 2)
        throw ChessParseError("Wrong number of replacement lines");
    cppFile[lineNo + 1] = replaceLines[1];
}

template <typename ParType>
static void replaceValue(const ParType& par, const std::string& name,
                         std::vector<std::string>& hppFile) {
    int lineNo = findLine("DECLARE_PARAM", "(" + name + ", ", hppFile);
    if (lineNo < 0)
        throw ChessParseError(name + " not found");

    const std::string& line = hppFile[lineNo];
    const int len = line.length();
    for (int i = 0; i < len; i++) {
        if (line[i] == ',') {
            for (int j = i + 1; j < len; j++) {
                if (line[j] != ' ') {
                    int p1 = j;
                    for (int k = p1 + 1; k < len; k++) {
                        if (line[k] == ',') {
                            int p2 = k;
                            int val = par;
                            hppFile[lineNo] = line.substr(0, p1) +
                                              num2Str(val) +
                                              line.substr(p2);
                            return;
                        }
                    }
                    goto error;
                }
            }
            goto error;
        }
    }
 error:
    throw ChessParseError("Failed to patch name : " + name);
}

void
ChessTool::patchParams(const std::string& directory) {
    std::vector<std::string> cppFile = readFile(directory + "/parameters.cpp");
    std::vector<std::string> hppFile = readFile(directory + "/parameters.hpp");

    replaceTableNxN<8>(kt1b, "kt1b", cppFile);
    replaceTableNxN<8>(kt2b, "kt2b", cppFile);
    replaceTableNxN<8>(pt1b, "pt1b", cppFile);
    replaceTableNxN<8>(pt2b, "pt2b", cppFile);
    replaceTableNxN<8>(nt1b, "nt1b", cppFile);
    replaceTableNxN<8>(nt2b, "nt2b", cppFile);
    replaceTableNxN<8>(bt1b, "bt1b", cppFile);
    replaceTableNxN<8>(bt2b, "bt2b", cppFile);
    replaceTableNxN<8>(qt1b, "qt1b", cppFile);
    replaceTableNxN<8>(qt2b, "qt2b", cppFile);
    replaceTableNxN<8>(rt1b, "rt1b", cppFile);
    replaceTableNxN<8>(knightOutpostBonus, "knightOutpostBonus", cppFile);
    replaceTableNxN<8>(protectedPawnBonus, "protectedPawnBonus", cppFile);
    replaceTableNxN<8>(attackedPawnBonus, "attackedPawnBonus", cppFile);
    replaceTable(protectBonus, "protectBonus", cppFile);

    replaceTable(rookMobScore, "rookMobScore", cppFile);
    replaceTable(bishMobScore, "bishMobScore", cppFile);
    replaceTable(knightMobScore, "knightMobScore", cppFile);
    replaceTable(queenMobScore, "queenMobScore", cppFile);
    replaceTableNxN<4>(majorPieceRedundancy, "majorPieceRedundancy", cppFile);
    replaceTableNxN<6>(connectedPPBonus, "connectedPPBonus", cppFile);
    replaceTable(passedPawnBonusX, "passedPawnBonusX", cppFile);
    replaceTable(passedPawnBonusY, "passedPawnBonusY", cppFile);
    replaceTable(ppBlockerBonus, "ppBlockerBonus", cppFile);
    replaceTable(candidatePassedBonus, "candidatePassedBonus", cppFile);
    replaceTable(QvsRRBonus, "QvsRRBonus", cppFile);
    replaceTable(RvsMBonus, "RvsMBonus", cppFile);
    replaceTable(RvsMMBonus, "RvsMMBonus", cppFile);
    replaceTable(bishopPairValue, "bishopPairValue", cppFile);
    replaceTable(rookEGDrawFactor, "rookEGDrawFactor", cppFile);
    replaceTable(RvsBPDrawFactor, "RvsBPDrawFactor", cppFile);
    replaceTable(castleFactor, "castleFactor", cppFile);
    replaceTable(pawnShelterTable, "pawnShelterTable", cppFile);
    replaceTable(pawnStormTable, "pawnStormTable", cppFile);
    replaceTable(kingAttackWeight, "kingAttackWeight", cppFile);
    replaceTable(qContactCheckBonus, "qContactCheckBonus", cppFile);
    replaceTable(pieceKingAttackBonus, "pieceKingAttackBonus", cppFile);
    replaceTable(kingPPSupportK, "kingPPSupportK", cppFile);
    replaceTable(kingPPSupportP, "kingPPSupportP", cppFile);
    replaceTable(pawnDoubledPenalty, "pawnDoubledPenalty", cppFile);
    replaceTable(pawnIsolatedPenalty, "pawnIsolatedPenalty", cppFile);
    replaceTable(halfMoveFactor, "halfMoveFactor", cppFile);
    replaceTable(stalePawnFactor, "stalePawnFactor", cppFile);

    replaceValue(pV, "pV", hppFile);
    replaceValue(nV, "nV", hppFile);
    replaceValue(bV, "bV", hppFile);
    replaceValue(rV, "rV", hppFile);
    replaceValue(qV, "qV", hppFile);

    replaceValue(pawnIslandPenalty, "pawnIslandPenalty", hppFile);
    replaceValue(pawnBackwardPenalty, "pawnBackwardPenalty", hppFile);
    replaceValue(pawnSemiBackwardPenalty1, "pawnSemiBackwardPenalty1", hppFile);
    replaceValue(pawnSemiBackwardPenalty2, "pawnSemiBackwardPenalty2", hppFile);
    replaceValue(pawnRaceBonus, "pawnRaceBonus", hppFile);
    replaceValue(passedPawnEGFactor, "passedPawnEGFactor", hppFile);
    replaceValue(RBehindPP1, "RBehindPP1", hppFile);
    replaceValue(RBehindPP2, "RBehindPP2", hppFile);
    replaceValue(activePawnPenalty, "activePawnPenalty", hppFile);

    replaceValue(QvsRMBonus1, "QvsRMBonus1", hppFile);
    replaceValue(QvsRMBonus2, "QvsRMBonus2", hppFile);
    replaceValue(knightVsQueenBonus1, "knightVsQueenBonus1", hppFile);
    replaceValue(knightVsQueenBonus2, "knightVsQueenBonus2", hppFile);
    replaceValue(knightVsQueenBonus3, "knightVsQueenBonus3", hppFile);
    replaceValue(krkpBonus, "krkpBonus", hppFile);
    replaceValue(krpkbBonus,   "krpkbBonus", hppFile);
    replaceValue(krpkbPenalty, "krpkbPenalty", hppFile);
    replaceValue(krpknBonus,   "krpknBonus", hppFile);
    replaceValue(RvsBPBonus,   "RvsBPBonus", hppFile);

    replaceValue(pawnTradePenalty, "pawnTradePenalty", hppFile);
    replaceValue(pieceTradeBonus, "pieceTradeBonus", hppFile);
    replaceValue(pawnTradeThreshold, "pawnTradeThreshold", hppFile);
    replaceValue(pieceTradeThreshold, "pieceTradeThreshold", hppFile);

    replaceValue(threatBonus1, "threatBonus1", hppFile);
    replaceValue(threatBonus2, "threatBonus2", hppFile);
    replaceValue(latentAttackBonus, "latentAttackBonus", hppFile);

    replaceValue(rookHalfOpenBonus, "rookHalfOpenBonus", hppFile);
    replaceValue(rookOpenBonus, "rookOpenBonus", hppFile);
    replaceValue(rookDouble7thRowBonus, "rookDouble7thRowBonus", hppFile);
    replaceValue(trappedRookPenalty1, "trappedRookPenalty1", hppFile);
    replaceValue(trappedRookPenalty2, "trappedRookPenalty2", hppFile);

    replaceValue(bishopPairPawnPenalty, "bishopPairPawnPenalty", hppFile);
    replaceValue(trappedBishopPenalty, "trappedBishopPenalty", hppFile);
    replaceValue(oppoBishopPenalty, "oppoBishopPenalty", hppFile);

    replaceValue(kingSafetyHalfOpenBCDEFG1, "kingSafetyHalfOpenBCDEFG1", hppFile);
    replaceValue(kingSafetyHalfOpenBCDEFG2, "kingSafetyHalfOpenBCDEFG2", hppFile);
    replaceValue(kingSafetyHalfOpenAH1, "kingSafetyHalfOpenAH1", hppFile);
    replaceValue(kingSafetyHalfOpenAH2, "kingSafetyHalfOpenAH2", hppFile);
    replaceValue(kingSafetyWeight1, "kingSafetyWeight1", hppFile);
    replaceValue(kingSafetyWeight2, "kingSafetyWeight2", hppFile);
    replaceValue(kingSafetyWeight3, "kingSafetyWeight3", hppFile);
    replaceValue(kingSafetyWeight4, "kingSafetyWeight4", hppFile);
    replaceValue(kingSafetyThreshold, "kingSafetyThreshold", hppFile);
    replaceValue(knightKingProtectBonus, "knightKingProtectBonus", hppFile);
    replaceValue(bishopKingProtectBonus, "bishopKingProtectBonus", hppFile);
    replaceValue(pawnStormBonus, "pawnStormBonus", hppFile);

    replaceValue(pawnLoMtrl, "pawnLoMtrl", hppFile);
    replaceValue(pawnHiMtrl, "pawnHiMtrl", hppFile);
    replaceValue(minorLoMtrl, "minorLoMtrl", hppFile);
    replaceValue(minorHiMtrl, "minorHiMtrl", hppFile);
    replaceValue(castleLoMtrl, "castleLoMtrl", hppFile);
    replaceValue(castleHiMtrl, "castleHiMtrl", hppFile);
    replaceValue(queenLoMtrl, "queenLoMtrl", hppFile);
    replaceValue(queenHiMtrl, "queenHiMtrl", hppFile);
    replaceValue(passedPawnLoMtrl, "passedPawnLoMtrl", hppFile);
    replaceValue(passedPawnHiMtrl, "passedPawnHiMtrl", hppFile);
    replaceValue(kingSafetyLoMtrl, "kingSafetyLoMtrl", hppFile);
    replaceValue(kingSafetyHiMtrl, "kingSafetyHiMtrl", hppFile);
    replaceValue(oppoBishopLoMtrl, "oppoBishopLoMtrl", hppFile);
    replaceValue(oppoBishopHiMtrl, "oppoBishopHiMtrl", hppFile);
    replaceValue(knightOutpostLoMtrl, "knightOutpostLoMtrl", hppFile);
    replaceValue(knightOutpostHiMtrl, "knightOutpostHiMtrl", hppFile);

//    replaceValue(moEvalWeight, "moEvalWeight", hppFile);
//    replaceValue(moHangPenalty1, "moHangPenalty1", hppFile);
//    replaceValue(moHangPenalty2, "moHangPenalty2", hppFile);
//    replaceValue(moSeeBonus, "moSeeBonus", hppFile);

    std::ofstream osc(directory + "/parameters.cpp");
    for (const std::string& line : cppFile)
        osc << line << std::endl;

    std::ofstream osh(directory + "/parameters.hpp");
    for (const std::string& line : hppFile)
        osh << line << std::endl;
}

// --------------------------------------------------------------------------------

void
ChessTool::evalStat(std::istream& is, std::vector<ParamDomain>& pdVec) {
    Parameters& uciPars = Parameters::instance();
    std::vector<PositionInfo> positions;
    readFENFile(is, positions);
    const int nPos = positions.size();

    ScoreToProb sp;
    const double avgErr0 = computeObjective(positions, sp);
    std::vector<int> qScores0;
    for (const PositionInfo& pi : positions)
        qScores0.push_back(pi.qScore);

    for (ParamDomain& pd : pdVec) {
        int newVal1 = (pd.value - pd.minV) > (pd.maxV - pd.value) ? pd.minV : pd.maxV;
        uciPars.set(pd.name, num2Str(newVal1));
        double avgErr = computeObjective(positions, sp);
        uciPars.set(pd.name, num2Str(pd.value));

        double nChanged = 0;
        std::unordered_set<int> games, changedGames;
        for (int i = 0; i < nPos; i++) {
            int gameNo = positions[i].gameNo;
            games.insert(gameNo);
            if (positions[i].qScore - qScores0[i]) {
                nChanged++;
                changedGames.insert(gameNo);
            }
        }
        double errChange1 = avgErr - avgErr0;
        double nChangedGames = changedGames.size();
        double nGames = games.size();

        double errChange2;
        int newVal2 = clamp(0, pd.minV, pd.maxV);
        if (newVal2 != newVal1) {
            uciPars.set(pd.name, num2Str(newVal2));
            double avgErr2 = computeObjective(positions, sp);
            uciPars.set(pd.name, num2Str(pd.value));
            errChange2 = avgErr2 - avgErr0;
        } else {
            errChange2 = errChange1;
        }

        std::cout << pd.name << " nMod:" << (nChanged / nPos)
                  << " nModG:" << (nChangedGames / nGames)
                  << " err1:" << errChange1 << " err2:" << errChange2 << std::endl;
    }
}

void
ChessTool::printResiduals(std::istream& is, const std::string& xTypeStr, bool includePosGameNr) {
    enum XType {
        MTRL_SUM,
        MTRL_DIFF,
        PAWN_SUM,
        PAWN_DIFF,
        EVAL
    };
    XType xType;
    if (xTypeStr == "mtrlsum") {
        xType = MTRL_SUM;
    } else if (xTypeStr == "mtrldiff") {
        xType = MTRL_DIFF;
    } else if (xTypeStr == "pawnsum") {
        xType = PAWN_SUM;
    } else if (xTypeStr == "pawndiff") {
        xType = PAWN_DIFF;
    } else if (xTypeStr == "eval") {
        xType = EVAL;
    } else {
        throw ChessParseError("Invalid X axis type");
    }

    std::vector<PositionInfo> positions;
    readFENFile(is, positions);
    qEval(positions);
    const int nPos = positions.size();
    Position pos;
    ScoreToProb sp;
    for (int i = 0; i < nPos; i++) {
        const PositionInfo& pi = positions[i];
        pos.deSerialize(pi.posData);
        int x;
        switch (xType) {
        case MTRL_SUM:
            x = pos.wMtrl() + pos.bMtrl();
            break;
        case MTRL_DIFF:
            x = pos.wMtrl() - pos.bMtrl();
            break;
        case PAWN_SUM:
            x = pos.wMtrlPawns() + pos.bMtrlPawns();
            break;
        case PAWN_DIFF:
            x = pos.wMtrlPawns() - pos.bMtrlPawns();
            break;
        case EVAL:
            x = pi.qScore;
            break;
        }
        double r = pi.result - sp.getProb(pi.qScore);
        if (includePosGameNr)
            std::cout << i << ' ' << pi.gameNo << ' ';
        std::cout << x << ' ' << r << '\n';
    }
    std::cout << std::flush;
}

bool
ChessTool::getCommentScore(const std::string& comment, int& score) {
    double fScore;
    if (str2Num(comment, fScore)) {
        score = (int)std::round(fScore * 100);
        return true;
    }
    if (startsWith(comment, "+M")) {
        score = 10000;
        return true;
    }
    if (startsWith(comment, "-M")) {
        score = -10000;
        return true;
    }
    return false;
}

void
splitString(const std::string& line, const std::string& delim, std::vector<std::string>& fields) {
    size_t start = 0;
    while (true) {
        size_t n = line.find(delim, start);
        if (n == std::string::npos)
            break;
        fields.push_back(line.substr(start, n - start));
        start = n + delim.length();
    }
    if (start < line.length())
        fields.push_back(line.substr(start));
}

void
ChessTool::readFENFile(std::istream& is, std::vector<PositionInfo>& data) {
    std::vector<std::string> lines = readStream(is);
    data.resize(lines.size());
    Position pos;
    PositionInfo pi;
    const int nLines = lines.size();
    std::atomic<bool> error(false);
#pragma omp parallel for default(none) shared(data,error,lines,std::cerr) private(pos,pi)
    for (int i = 0; i < nLines; i++) {
        if (error)
            continue;
        const std::string& line = lines[i];
        std::vector<std::string> fields;
        splitString(line, " : ", fields);
        bool localError = false;
        if ((fields.size() < 4) || (fields.size() > 6))
            localError = true;
        if (!localError) {
            try {
                pos = TextIO::readFEN(fields[0]);
            } catch (ChessParseError& cpe) {
                localError = true;
            }
        }
        if (!localError) {
            pos.serialize(pi.posData);
            if (!str2Num(fields[1], pi.result) ||
                !str2Num(fields[2], pi.searchScore) ||
                !str2Num(fields[3], pi.qScore)) {
                localError = true;
            }
        }
        if (!localError) {
            pi.gameNo = -1;
            if (fields.size() >= 5)
                if (!str2Num(fields[4], pi.gameNo))
                    localError = true;
        }
        if (!localError) {
            pi.cMove = Move().getCompressedMove();
            if (fields.size() >= 6)
                pi.cMove = TextIO::uciStringToMove(fields[5]).getCompressedMove();
        }
        if (!localError)
            data[i] = pi;

        if (localError) {
#pragma omp critical
            if (!error) {
                std::cerr << "line:" << line << std::endl;
                std::cerr << "fields:" << fields << std::endl;
                error = true;
            }
        }
    }
    if (error)
        throw ChessParseError("Invalid file format");

    if (optimizeMoveOrdering) {
        std::cout << "positions before: " << data.size() << std::endl;
        // Only include positions where non-capture moves were played
        auto remove = [](const PositionInfo& pi) -> bool {
            Position pos;
            pos.deSerialize(pi.posData);
            Move m;
            m.setFromCompressed(pi.cMove);
            return m.isEmpty() || pos.getPiece(m.to()) != Piece::EMPTY;
        };
        data.erase(std::remove_if(data.begin(), data.end(), remove), data.end());
        std::cout << "positions after: " << data.size() << std::endl;
    }
}

void
ChessTool::writePGN(const Position& pos) {
    std::cout << "[Event \"?\"]" << std::endl;
    std::cout << "[Site \"?\"]" << std::endl;
    std::cout << "[Date \"????.??.??\"]" << std::endl;
    std::cout << "[Round \"?\"]" << std::endl;
    std::cout << "[White \"?\"]" << std::endl;
    std::cout << "[Black \"?\"]" << std::endl;
    std::cout << "[Result \"*\"]" << std::endl;
    std::cout << "[FEN \"" << TextIO::toFEN(pos) << "\"]" << std::endl;
    std::cout << "[SetUp \"1\"]" << std::endl;
    std::cout << "*" << std::endl;
}

// --------------------------------------------------------------------------------

double
ChessTool::computeObjective(std::vector<PositionInfo>& positions, const ScoreToProb& sp) {
    if (optimizeMoveOrdering) {
        return computeMoveOrderObjective(positions, sp);
    } else {
        qEval(positions);
        return computeAvgError(positions, sp);
    }
}

void
ChessTool::qEval(std::vector<PositionInfo>& positions) {
    qEval(positions, 0, positions.size());
}

void
ChessTool::qEval(std::vector<PositionInfo>& positions, const int beg, const int end) {
    TranspositionTable tt(19);
    ParallelData pd(tt);

    std::vector<U64> nullHist(200);
    KillerTable kt;
    History ht;
    std::shared_ptr<Evaluate::EvalHashTables> et;
    TreeLogger treeLog;
    Position pos;

    const int chunkSize = 5000;

#pragma omp parallel for default(none) shared(positions,tt,pd) private(kt,ht,et,treeLog,pos) firstprivate(nullHist)
    for (int c = beg; c < end; c += chunkSize) {
        if (!et)
            et = Evaluate::getEvalHashTables();
        Search::SearchTables st(tt, kt, ht, *et);

        const int mate0 = SearchConst::MATE0;
        Search sc(pos, nullHist, 0, st, pd, nullptr, treeLog);

        for (int i = 0; i < chunkSize; i++) {
            if (c + i >= end)
                break;
            PositionInfo& pi = positions[c + i];
            pos.deSerialize(pi.posData);
            sc.init(pos, nullHist, 0);
            sc.q0Eval = UNKNOWN_SCORE;
            int score = sc.quiesce(-mate0, mate0, 0, 0, MoveGen::inCheck(pos));
            if (!pos.isWhiteMove())
                score = -score;
            pi.qScore = score;
        }
    }
}

double
ChessTool::computeAvgError(std::vector<PositionInfo>& positions, const ScoreToProb& sp,
                           const std::vector<ParamDomain>& pdVec, arma::mat& pdVal) {
    assert(pdVal.n_rows == pdVec.size());
    assert(pdVal.n_cols == 1);

    Parameters& uciPars = Parameters::instance();
    for (int i = 0; i < (int)pdVal.n_rows; i++)
        uciPars.set(pdVec[i].name, num2Str(pdVal.at(i, 0)));
    qEval(positions);
    return computeAvgError(positions, sp);
}

double
ChessTool::computeAvgError(const std::vector<PositionInfo>& positions, const ScoreToProb& sp) {
    double errSum = 0;
    if (useEntropyErrorFunction) {
        for (const PositionInfo& pi : positions) {
            double err = -(pi.result * sp.getLogProb(pi.qScore) + (1 - pi.result) * sp.getLogProb(-pi.qScore));
            errSum += err;
        }
        return errSum / positions.size();
    } else {
        for (const PositionInfo& pi : positions) {
            double p = sp.getProb(pi.qScore);
            double err = p - pi.result;
            errSum += err * err;
        }
        return sqrt(errSum / positions.size());
    }
}

double
ChessTool::computeMoveOrderObjective(std::vector<PositionInfo>& positions, const ScoreToProb& sp) {
    const int beg = 0;
    const int end = positions.size();

    std::shared_ptr<Evaluate::EvalHashTables> et;
    Position pos;

    const int chunkSize = 5000;

#pragma omp parallel for default(none) shared(positions,sp) private(et,pos)
    for (int c = beg; c < end; c += chunkSize) {
        if (!et)
            et = Evaluate::getEvalHashTables();
        Evaluate eval(*et);

        for (int i = 0; i < chunkSize; i++) {
            if (c + i >= end)
                break;
            PositionInfo& pi = positions[c + i];
            pos.deSerialize(pi.posData);

            MoveList moves;
            MoveGen::pseudoLegalMoves(pos, moves);
            MoveGen::removeIllegal(pos, moves);

            staticScoreMoveListQuiet(pos, eval, moves);

            double probSum = 0;
            for (int mi = 0; mi < moves.size; mi++) {
                Move& m = moves[mi];
                if (pos.getPiece(m.to()) != Piece::EMPTY)
                    continue;
                probSum += sp.getProb(m.score());
            }
            double probFactor = probSum <= 0 ? 1 : 1 / probSum;
            double errSum = 0;
            int errCnt = 0;
            for (int mi = 0; mi < moves.size; mi++) {
                Move& m = moves[mi];
                if (pos.getPiece(m.to()) != Piece::EMPTY)
                    continue;
                double p = sp.getProb(m.score()) * probFactor;
                double expectedP = m.getCompressedMove() == pi.cMove ? 1 : 0;
                double err = p - expectedP;
                errSum += err * err;
                errCnt++;
            }
            pi.result = errCnt > 0 ? errSum / errCnt : -1;
        }
    }

    double errSum = 0;
    int errCnt = 0;
    for (int i = beg; i < end; i++) {
        PositionInfo& pi = positions[i];
        if (pi.result >= 0) {
            errSum += pi.result;
            errCnt++;
        }
    }

    return errCnt > 0 ? sqrt(errSum / errCnt) : 0;
}

void
ChessTool::staticScoreMoveListQuiet(Position& pos, Evaluate& eval, MoveList& moves) {
    int score0 = eval.evalPos(pos);
    bool wtm = pos.isWhiteMove();
    UndoInfo ui;
    for (int i = 0; i < moves.size; i++) {
        Move& m = moves[i];
        int score = 0;

        int pVal = ::pieceValue[pos.getPiece(m.from())];
        int prevHang = 0;
        if (pVal > ::pV) {
            if (wtm) {
                if (BitBoard::wPawnAttacks[m.from()] & pos.pieceTypeBB(Piece::BPAWN))
                    prevHang = pVal;
            } else {
                if (BitBoard::bPawnAttacks[m.from()] & pos.pieceTypeBB(Piece::WPAWN))
                    prevHang = pVal;
            }
        }
        score += prevHang * moHangPenalty1 / 32;

        int seeScore = Search::SEE(pos, m);
        score += seeScore * moSeeBonus / 32;

        pos.makeMove(m, ui);
        int score1 = -eval.evalPos(pos);
        score += (score1 - score0) * moEvalWeight / 32;

        int currHang = 0;
        if (pVal > ::pV) {
            if (wtm) {
                if (BitBoard::wPawnAttacks[m.to()] & pos.pieceTypeBB(Piece::BPAWN))
                    currHang = pVal;
            } else {
                if (BitBoard::bPawnAttacks[m.to()] & pos.pieceTypeBB(Piece::WPAWN))
                    currHang = pVal;
            }
        }

        score -= currHang * moHangPenalty2 / 32;

        m.setScore(score);
        pos.unMakeMove(m, ui);
    }
}

// --------------------------------------------------------------------------------

void
ChessTool::probeDTZ(const std::string& fen) {
    setupTB();
    Position pos = TextIO::readFEN(fen);
    int success;
    int dtz = Syzygy::probe_dtz(pos, &success);
    std::cout << fen << " raw:";
    if (success)
        std::cout << dtz;
    else
        std::cout << "---";

    int score = 0;
    TranspositionTable::TTEntry ent;
    bool ok = TBProbe::rtbProbeDTZ(pos, 0, score, ent);
    std::cout << " dtz:";
    if (ok) {
        std::cout << score;
        if (score == 0) {
            std::cout << " (" << ent.getEvalScore() << ")";
        }
    } else {
        std::cout << "---";
    }

    ok = TBProbe::rtbProbeWDL(pos, 0, score, ent);
    std::cout << " wdl:";
    if (ok) {
        std::cout << score;
        if (score == 0) {
            std::cout << " (" << ent.getEvalScore() << ")";
        }
    } else {
        std::cout << "---";
    }

    ok = TBProbe::gtbProbeDTM(pos, 0, score);
    std::cout << " dtm:";
    if (ok)
        std::cout << score;
    else
        std::cout << "---";
    std::cout << std::endl;
}
