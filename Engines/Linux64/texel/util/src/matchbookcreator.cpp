/*
    Texel - A UCI chess engine.
    Copyright (C) 2016  Peter Österlund, peterosterlund2@gmail.com

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
 * matchbookcreator.cpp
 *
 *  Created on: May 15, 2016
 *      Author: petero
 */

#include "matchbookcreator.hpp"
#include "position.hpp"
#include "moveGen.hpp"
#include "search.hpp"
#include "textio.hpp"
#include "gametree.hpp"
#include <unordered_set>

MatchBookCreator::MatchBookCreator() {

}

void
MatchBookCreator::createBook(int depth, int searchTime, std::ostream& os) {
    createBookLines(depth);

    std::vector<BookLine> lines;
    for (const auto& bl : bookLines)
        lines.push_back(bl.second);
    std::random_shuffle(lines.begin(), lines.end());
    evaluateBookLines(lines, searchTime, os);
}

void
MatchBookCreator::createBookLines(int depth) {
    std::vector<Move> moveList;
    Position pos = TextIO::readFEN(TextIO::startPosFEN);
    createBookLines(pos, moveList, depth);

#if 0
    int i = 0;
    for (const auto& bl : bookLines) {
        std::cout << std::setw(5) << i;
        for (Move m : bl.second.moves)
            std::cout << ' ' << TextIO::moveToUCIString(m);
        std::cout << std::endl;
        i++;
    }
#endif
}

void
MatchBookCreator::createBookLines(Position& pos, std::vector<Move>& moveList, int depth) {
    if (depth <= 0) {
        if (bookLines.find(pos.historyHash()) == bookLines.end())
            bookLines[pos.historyHash()] = BookLine(moveList);
        return;
    }
    MoveList moves;
    MoveGen::pseudoLegalMoves(pos, moves);
    MoveGen::removeIllegal(pos, moves);
    UndoInfo ui;
    for (int mi = 0; mi < moves.size; mi++) {
        const Move& m = moves[mi];
        pos.makeMove(m, ui);
        moveList.push_back(m);
        createBookLines(pos, moveList, depth - 1);
        moveList.pop_back();
        pos.unMakeMove(m, ui);
    }
}

void
MatchBookCreator::evaluateBookLines(std::vector<BookLine>& lines, int searchTime,
                                    std::ostream& os) {
    const int nLines = lines.size();
    TranspositionTable tt(28);
    ParallelData pd(tt);
    std::shared_ptr<Evaluate::EvalHashTables> et;

#pragma omp parallel for schedule(dynamic) default(none) shared(lines,tt,pd,searchTime,os) private(et)
    for (int i = 0; i < nLines; i++) {
        BookLine& bl = lines[i];

        KillerTable kt;
        History ht;
        TreeLogger treeLog;
        if (!et)
            et = Evaluate::getEvalHashTables();

        Position pos = TextIO::readFEN(TextIO::startPosFEN);
        UndoInfo ui;
        std::vector<U64> posHashList(200 + bl.moves.size());
        int posHashListSize = 0;
        for (const Move& m : bl.moves) {
            posHashList[posHashListSize++] = pos.zobristHash();
            pos.makeMove(m, ui);
            if (pos.getHalfMoveClock() == 0)
                posHashListSize = 0;
        }

        MoveList legalMoves;
        MoveGen::pseudoLegalMoves(pos, legalMoves);
        MoveGen::removeIllegal(pos, legalMoves);

        Search::SearchTables st(tt, kt, ht, *et);
        Search sc(pos, posHashList, posHashListSize, st, pd, nullptr, treeLog);
        sc.timeLimit(searchTime, searchTime);

        int maxDepth = -1;
        S64 maxNodes = -1;
        bool verbose = false;
        int maxPV = 1;
        bool onlyExact = true;
        int minProbeDepth = 1;
        Move bestMove = sc.iterativeDeepening(legalMoves, maxDepth, maxNodes, verbose, maxPV,
                                              onlyExact, minProbeDepth);
        int score = bestMove.score();
        if (!pos.isWhiteMove())
            score = -score;
        bl.score = score;
#pragma omp critical
        {
            os << std::setw(5) << i << ' ' << std::setw(6) << score;
            for (Move m : bl.moves)
                os << ' ' << TextIO::moveToUCIString(m);
            os << std::endl;
        }
    }
}

void
MatchBookCreator::countUniq(const std::string& pgnFile, std::ostream& os) {
    std::ifstream is(pgnFile);
    PgnReader reader(is);
    std::vector<std::unordered_set<U64>> uniqPositions;
    GameTree gt;
    int nGames = 0;
    try {
        while (reader.readPGN(gt)) {
            nGames++;
            GameNode gn = gt.getRootNode();
            int ply = 0;
            while (true) {
                while ((int)uniqPositions.size() <= ply)
                    uniqPositions.push_back(std::unordered_set<U64>());
                uniqPositions[ply].insert(gn.getPos().zobristHash());
                if (gn.nChildren() == 0)
                    break;
                gn.goForward(0);
                ply++;
            }
        }

        std::unordered_set<U64> uniq;
        if (uniqPositions.size() > 0)
            uniq.insert(uniqPositions[0].begin(), uniqPositions[0].end());
        for (size_t i = 1; i < uniqPositions.size(); i++) {
            int u0 = uniq.size();
            uniq.insert(uniqPositions[i].begin(), uniqPositions[i].end());
            int u1 = uniq.size();
            os << std::setw(3) << i << ' ' << u1 - u0 << std::endl;
        }
    } catch (...) {
        std::cerr << "Error parsing game " << nGames << std::endl;
        throw;
    }
}

namespace {
class PlayerInfo {
public:
    explicit PlayerInfo(const std::string& name0) : name(name0) {}

    const std::string& getName() const { return name; }

    void addWDL(double score) {
        if (score == 0)
            nLoss++;
        else if (score == 1)
            nWin++;
        else
            nDraw++;
    }

    void addScore(double score) {
        nScores++;
        scoreSum += score;
        scoreSum2 += score * score;
    }

    /** Add average search depth information for this player and the opponent player. */
    void addDepth(int myMoveSum, int myDepthSum, int oppoMoveSum, int oppoDepthSum) {
        this->myMoveSum += myMoveSum;
        this->myDepthSum += myDepthSum;
        this->oppoMoveSum += oppoMoveSum;
        this->oppoDepthSum += oppoDepthSum;
    }

    void getWDLInfo(int& w, int& d, int& l) const {
        w = nWin;
        d = nDraw;
        l = nLoss;
    }

    double getMeanScore() const {
        return scoreSum / nScores;
    }

    /** Return standard deviation of the mean score. */
    double getStdDevScore() const {
        double N = nScores;
        double sDev = ::sqrt(1/(N - 1) * (scoreSum2 - scoreSum * scoreSum / N));
        return sDev / sqrt(N);
    }

    /** Get average search depth for this player and the opponent player. */
    void getAvgDepth(double& myDepth, double& oppoDepth) const {
        myDepth = myDepthSum / (double)myMoveSum;
        oppoDepth = oppoDepthSum / (double)oppoMoveSum;
    }

private:
    const std::string name; // Player name
    int nWin = 0;           // Number of won games
    int nDraw = 0;          // Number of drawn games
    int nLoss = 0;          // Number of lost games

    double nScores = 0;     // Number of scores
    double scoreSum = 0;    // Sum of scores
    double scoreSum2 = 0;   // Sum of squared scores

    int myMoveSum = 0;      // Number of moves with depth information for this player
    int myDepthSum = 0;     // Sum of search depth for this player
    int oppoMoveSum = 0;    // Number of moves with depth information for opponent player
    int oppoDepthSum = 0;   // Sum of search depth for opponent player
};

struct GameInfo {
    int pw;
    int pb;
    double score; // Score for white player
    int wMoveSum;
    int wDepthSum;
    int bMoveSum;
    int bDepthSum;
};
}

void
MatchBookCreator::pgnStat(const std::string& pgnFile, bool pairMode, std::ostream& os) {
    std::vector<PlayerInfo> players;
    std::vector<GameInfo> games;

    std::ifstream is(pgnFile);
    PgnReader reader(is);
    GameTree gt;
    int nGames = 0;
    int nMoves = 0;
    std::map<std::string, std::string> headers;

    auto playerNo = [&players](const std::string& name) -> int {
        for (size_t i = 0; i < players.size(); i++)
            if (players[i].getName() == name)
                return i;
        players.push_back(PlayerInfo(name));
        return players.size() - 1;
    };

    try {
        while (reader.readPGN(gt)) {
            nGames++;
            GameNode gn = gt.getRootNode();
            int wMoveSum = 0, wDepthSum = 0;
            int bMoveSum = 0, bDepthSum = 0;
            int ply = 0;
            while (true) {
                if (gn.nChildren() == 0)
                    break;
                bool wtm = gn.getPos().isWhiteMove();
                gn.goForward(0);
                int depth;
                if (getCommentDepth(gn.getComment(), depth)) {
                    if (wtm) {
                        wDepthSum += depth;
                        wMoveSum++;
                    } else {
                        bDepthSum += depth;
                        bMoveSum++;
                    }
                }
                ply++;
                nMoves++;
            }

            headers.clear();
            gt.getHeaders(headers);
            int pw = playerNo(headers["White"]);
            int pb = playerNo(headers["Black"]);
            double score;
            switch (gt.getResult()) {
            case GameTree::WHITE_WIN: score = 1;   break;
            case GameTree::DRAW:      score = 0.5; break;
            case GameTree::BLACK_WIN: score = 0;   break;
            default:                 throw ChessParseError("Unknown result");
            }
            games.push_back(GameInfo{pw, pb, score, wMoveSum, wDepthSum, bMoveSum, bDepthSum});
        }

        std::stringstream ss;
        ss.precision(1);
        ss << std::fixed << (nMoves / (double)nGames);
        os << "nGames: " << nGames << " nMoves: " << nMoves << " plies/game: " << ss.str() << std::endl;

        if (pairMode && players.size() != 2) {
            std::cerr << "Pair mode requires two players" << std::endl;
            return;
        }

        for (size_t i = 0; i < games.size(); i++) {
            const GameInfo& gi = games[i];
            players[gi.pw].addWDL(gi.score);
            players[gi.pb].addWDL(1-gi.score);
            if (pairMode) {
                if (i % 2 != 0) {
                    double score = gi.score + (1 - games[i-1].score);
                    players[gi.pw].addScore(score);
                    players[gi.pb].addScore(2 - score);
                }
            } else {
                players[gi.pw].addScore(gi.score);
                players[gi.pb].addScore(1-gi.score);
            }
            players[gi.pw].addDepth(gi.wMoveSum, gi.wDepthSum, gi.bMoveSum, gi.bDepthSum);
            players[gi.pb].addDepth(gi.bMoveSum, gi.bDepthSum, gi.wMoveSum, gi.wDepthSum);
        }

        for (const PlayerInfo& pi : players) {
            int win, draw, loss;
            pi.getWDLInfo(win, draw, loss);
            double mean = pi.getMeanScore();
            double sDev = pi.getStdDevScore();
            if (pairMode) {
                mean /= 2;
                sDev /= 2;
            }
            os << pi.getName() << " : WDL: " << win << " - " << draw << " - " << loss
                      << " m: " << mean << " sDev: " << sDev;
            if (sDev > 0) {
                std::stringstream ss;
                ss.precision(2);
                ss << std::fixed << (mean - 0.5) / sDev;
                os << " c: " << ss.str();
            }
            os << std::endl;
            double elo = 400 * log10(mean/(1-mean));
            double drawRate = draw / (double)(win + draw + loss);
            std::stringstream ss;
            ss.precision(1);
            ss << "            elo: " << std::fixed << elo;
            ss.precision(4);
            ss << " draw: " << std::fixed << drawRate;
            double myDepth, oppoDepth;
            pi.getAvgDepth(myDepth, oppoDepth);
            ss.precision(2);
            ss << " depth: " << std::fixed << myDepth << " - " << std::fixed << oppoDepth;
            os << ss.str() << std::endl;
            if (pairMode)
                break;
        }
    } catch (...) {
        std::cerr << "Error parsing game " << nGames << std::endl;
        throw;
    }
}

bool
MatchBookCreator::getCommentDepth(const std::string& comment, int& depth) {
    if (startsWith(comment, "+M") || startsWith(comment, "-M"))
        return false;
    auto n = comment.find('/');
    if (n == std::string::npos)
        return false;
    return str2Num(comment.substr(n+1), depth);
}
