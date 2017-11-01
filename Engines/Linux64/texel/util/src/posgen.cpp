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
 * posgen.cpp
 *
 *  Created on: Jan 6, 2014
 *      Author: petero
 */

#include "posgen.hpp"
#include "textio.hpp"
#include "moveGen.hpp"
#include "tbprobe.hpp"
#include "search.hpp"
#include "syzygy/rtb-probe.hpp"
#include "parameters.hpp"
#include "util/timeUtil.hpp"
#include "chesstool.hpp"
#include "tbgen.hpp"

bool
PosGenerator::generate(const std::string& type) {
    if (type == "qvsn")
        genQvsN();
    else
        return false;
    return true;
}

static void
writeFEN(const Position& pos) {
    std::cout << TextIO::toFEN(pos) << '\n';
}

void
PosGenerator::genQvsN() {
    for (int bk = 0; bk < 8; bk++) {
        for (int wk = 0; wk < 8; wk++) {
            for (int q1 = 0; q1 < 8; q1++) {
                if (q1 == wk)
                    continue;
                for (int q2 = q1 + 1; q2 < 8; q2++) {
                    if (q2 == wk)
                        continue;
                    for (int q3 = q2 + 1; q3 < 8; q3++) {
                        if (q3 == wk)
                            continue;
                        Position pos;
                        for (int i = 0; i < 8; i++) {
                            pos.setPiece(Position::getSquare(i, 1), Piece::WPAWN);
                            pos.setPiece(Position::getSquare(i, 6), Piece::BPAWN);
                            pos.setPiece(Position::getSquare(i, 7), Piece::BKNIGHT);
                        }
                        pos.setPiece(Position::getSquare(bk, 7), Piece::BKING);
                        pos.setPiece(Position::getSquare(wk, 0), Piece::WKING);
                        pos.setPiece(Position::getSquare(q1, 0), Piece::WQUEEN);
                        pos.setPiece(Position::getSquare(q2, 0), Piece::WQUEEN);
                        pos.setPiece(Position::getSquare(q3, 0), Piece::WQUEEN);
                        writeFEN(pos);
                        for (int i = 0; i < 8; i++) {
                            pos.setPiece(Position::getSquare(i, 6), Piece::EMPTY);
                            writeFEN(pos);
                            pos.setPiece(Position::getSquare(i, 6), Piece::BPAWN);
                        }
                        for (int i = 0; i < 8; i++) {
                            pos.setPiece(Position::getSquare(i, 1), Piece::EMPTY);
                            writeFEN(pos);
                            pos.setPiece(Position::getSquare(i, 1), Piece::WPAWN);
                        }
                    }
                }
            }
        }
    }
}

// --------------------------------------------------------------------------------

static bool typeOk(const std::vector<int>& pieces) {
    int nQ = count(pieces.begin(), pieces.end(), 0);
    int nR = count(pieces.begin(), pieces.end(), 1);
    int nB = count(pieces.begin(), pieces.end(), 2);
    int nN = count(pieces.begin(), pieces.end(), 3);
    int nP = count(pieces.begin(), pieces.end(), 4);
    return nP <= 8 - std::max(0, (nQ-1)) -
                     std::max(0, (nR-2)) -
                     std::max(0, (nB-2)) -
                     std::max(0, (nN-2));
}

static char pTypes[] = { 'q', 'r', 'b', 'n', 'p' };
static const int nTypes = COUNT_OF(pTypes);

static void
getPieceCombos(int nPieces, std::vector<std::string>& out) {
    std::vector<int> pieces(nPieces, 0);
    while (true) {
        if (typeOk(pieces)) {
            std::string s;
            for (int i = 0; i < nPieces; i++)
                s += pTypes[pieces[i]];
            out.push_back(s);
        }

        bool done = true;
        for (int i = nPieces - 1; i >= 0; i--) {
            if (pieces[i] < nTypes-1) {
                pieces[i]++;
                for (int j = i + 1; j < nPieces; j++)
                    pieces[j] = pieces[i];
                done = false;
                break;
            }
        }
        if (done)
            break;
    }
}

static bool
wrongOrder(const std::string& w, const std::string& b) {
    std::vector<int> wOrder, bOrder;
    for (auto c : w)
        for (int i = 0; i < nTypes; i++)
            if (pTypes[i] == c) {
                wOrder.push_back(i);
                break;
            }
    for (auto c : b)
        for (int i = 0; i < nTypes; i++)
            if (pTypes[i] == c) {
                bOrder.push_back(i);
                break;
            }
    return wOrder > bOrder;
}

void
PosGenerator::tbList(int nPieces) {
    nPieces -= 2;
    for (int nWhite = nPieces; nWhite >= 0; nWhite--) {
        int nBlack = nPieces - nWhite;
        if (nBlack > nWhite)
            continue;
        std::vector<std::string> wCombos, bCombos;
        getPieceCombos(nWhite, wCombos);
        getPieceCombos(nBlack, bCombos);
        std::string s;
        s += 'k';
        for (auto w : wCombos)
            for (auto b : bCombos) {
                if (nWhite == nBlack && wrongOrder(w, b))
                    continue;
                std::cout << 'k' << w << 'k' << b << '\n';
            }
    }
}

// --------------------------------------------------------------------------------

static void
getPieces(const std::string& tbType, std::vector<int>& pieces,
          bool& whitePawns, bool& blackPawns) {
    if (tbType.length() < 1 || tbType[0] != 'k')
        throw ChessParseError("Invalid tbType: " + tbType);
    whitePawns = blackPawns = false;
    bool white = true;
    for (size_t i = 1; i < tbType.length(); i++) {
        switch (tbType[i]) {
            case 'k':
                if (!white)
                    throw ChessParseError("Invalid tbType: " + tbType);
                white = false;
                break;
            case 'q':
                pieces.push_back(white ? Piece::WQUEEN: Piece::BQUEEN);
                break;
            case 'r':
                pieces.push_back(white ? Piece::WROOK: Piece::BROOK);
                break;
            case 'b':
                pieces.push_back(white ? Piece::WBISHOP: Piece::BBISHOP);
                break;
            case 'n':
                pieces.push_back(white ? Piece::WKNIGHT: Piece::BKNIGHT);
                break;
            case 'p':
                pieces.push_back(white ? Piece::WPAWN: Piece::BPAWN);
                if (white)
                    whitePawns = true;
                else
                    blackPawns = true;
                break;
            default:
                throw ChessParseError("Invalid tbType: " + tbType);
        }
    }
    if (white)
        throw ChessParseError("Invalid tbType: " + tbType);
}

/** Return true if square is a valid square for piece. */
inline bool
squareValid(int square, int piece) {
    if (piece == Piece::WPAWN || piece == Piece::BPAWN) {
        return square >= 8 && square < 56;
    } else {
        return true;
    }
}

/** Return bitboard containing possible en passant squares. */
static U64
getEPSquares(const Position& pos) {
    U64 wPawns = pos.pieceTypeBB(Piece::WPAWN);
    U64 bPawns = pos.pieceTypeBB(Piece::BPAWN);
    U64 occupied = pos.occupiedBB();
    if (pos.isWhiteMove()) {
        U64 wPawnAttacks = ((wPawns & BitBoard::maskBToHFiles) << 7) |
                           ((wPawns & BitBoard::maskAToGFiles) << 9);
        return ((bPawns & BitBoard::maskRow5) << 8) & ~occupied & wPawnAttacks;
    } else {
        U64 bPawnAttacks = ((bPawns & BitBoard::maskBToHFiles) >> 9) |
                           ((bPawns & BitBoard::maskAToGFiles) >> 7);
        return ((wPawns & BitBoard::maskRow4) >> 8) & ~occupied & bPawnAttacks;
    }
}

/** Call func(pos) for all positions in a given tablebase.
 * func() must not modify pos. */
template <typename Func>
static void
iteratePositions(const std::string& tbType, Func func) {
    iteratePositions(tbType, true, func);
}

template <typename Func>
static void
iteratePositions(const std::string& tbType, bool skipSymmetric, Func func) {
    std::vector<int> pieces;
    bool whitePawns, blackPawns;
    getPieces(tbType, pieces, whitePawns, blackPawns);
    const int nPieces = pieces.size();
    const bool anyPawns = whitePawns || blackPawns;
    const bool epPossible = whitePawns && blackPawns;

    for (int wk = 0; wk < 64; wk++) {
        int x = Position::getX(wk);
        int y = Position::getY(wk);
        if (skipSymmetric) {
            if (x >= 4)
                continue;
            if (!anyPawns)
                if (y >= 4 || y < x)
                    continue;
        }
        for (int bk = 0; bk < 64; bk++) {
            int x2 = Position::getX(bk);
            int y2 = Position::getY(bk);
            if (std::abs(x2-x) < 2 && std::abs(y2-y) < 2)
                continue;

            Position pos;
            pos.setPiece(wk, Piece::WKING);
            pos.setPiece(bk, Piece::BKING);
            std::vector<int> squares(nPieces, 0);
            int nPlaced = 0;

            while (true) {
                // Place remaining pieces on first empty square. Multiple equal
                // pieces are placed starting with the lowest square.
                while (nPlaced < nPieces) {
                    const int p = pieces[nPlaced];
                    int first = 0;
                    if (nPlaced > 0 && pieces[nPlaced-1] == p)
                        first = squares[nPlaced-1] + 1;
                    bool ok = false;
                    for (int sq = first; sq < 64; sq++) {
                        if (!squareValid(sq, p))
                            continue;
                        if (pos.getPiece(sq) == Piece::EMPTY) {
                            pos.setPiece(sq, p);
                            squares[nPlaced] = sq;
                            nPlaced++;
                            ok = true;
                            break;
                        }
                    }
                    if (!ok)
                        break;
                }

                if (nPlaced == nPieces) {
                    pos.setWhiteMove(true);
                    bool wKingAttacked = MoveGen::sqAttacked(pos, wk);
                    pos.setWhiteMove(false);
                    bool bKingAttacked = MoveGen::sqAttacked(pos, bk);
                    for (int side = 0; side < 2; side++) {
                        bool white = side == 0;
                        if (white) {
                            if (bKingAttacked)
                                continue;
                        } else {
                            if (wKingAttacked)
                                continue;
                        }
                        pos.setWhiteMove(white);

                        U64 epSquares = epPossible ? getEPSquares(pos) : 0;
                        while (true) {
                            if (epSquares) {
                                int epSq = BitBoard::firstSquare(epSquares);
                                pos.setEpSquare(epSq);
                                TextIO::fixupEPSquare(pos);
                                if (pos.getEpSquare() == -1) {
                                    epSquares &= epSquares - 1;
                                    continue;
                                }
                            } else {
                                pos.setEpSquare(-1);
                            }
                            func(pos);
                            if (epSquares == 0)
                                break;
                            epSquares &= epSquares - 1;
                        }
                    }
                }

                // Set up next position
                bool done = false;
                while (true) {
                    nPlaced--;
                    if (nPlaced < 0) {
                        done = true;
                        break;
                    }
                    int sq0 = squares[nPlaced];
                    int p = pos.getPiece(sq0);
                    pos.setPiece(sq0, Piece::EMPTY);
                    bool foundEmpty = false;
                    for (int sq = sq0 + 1; sq < 64; sq++) {
                        if (!squareValid(sq, p))
                            continue;
                        if (pos.getPiece(sq) == Piece::EMPTY) {
                            pos.setPiece(sq, p);
                            squares[nPlaced] = sq;
                            nPlaced++;
                            foundEmpty = true;
                            break;
                        }
                    }
                    if (foundEmpty)
                        break;
                }
                if (done)
                    break;
            }
        }
    }
}

void
PosGenerator::dtmStat(const std::vector<std::string>& tbTypes) {
    ChessTool::setupTB();
    for (std::string tbType : tbTypes) {
        double t0 = currentTime();
        int negScore = std::numeric_limits<int>::min(); // Largest negative score
        int posScore = std::numeric_limits<int>::max(); // Smallest positive score
        Position negPos, posPos;
        U64 nPos = 0;
        iteratePositions(tbType, [&](Position& pos) {
            nPos++;
            int score;
            if (!TBProbe::gtbProbeDTM(pos, 0, score))
                throw ChessParseError("GTB probe failed, pos:" + TextIO::toFEN(pos));
            if (score > 0) {
                if (score < posScore) {
                    posScore = score;
                    posPos = pos;
                }
            } else if (score < 0) {
                if (score > negScore) {
                    negScore = score;
                    negPos = pos;
                }
            }
        });
        double t1 = currentTime();
        std::cout << tbType << " neg: " << negScore << " pos:" << posScore << " nPos:" << nPos
                  << " t:" << (t1-t0) << std::endl;
        std::cout << tbType << " negPos: " << TextIO::toFEN(negPos) << std::endl;
        std::cout << tbType << " posPos: " << TextIO::toFEN(posPos) << std::endl;
    }
}

void
PosGenerator::dtzStat(const std::vector<std::string>& tbTypes) {
    ChessTool::setupTB();
    for (std::string tbType : tbTypes) {
        double t0 = currentTime();
        int negScore = std::numeric_limits<int>::max(); // Smallest negative score
        int posScore = std::numeric_limits<int>::min(); // Largest positive score
        Position negPos, posPos;
        U64 nPos = 0;
        int negReported = -1000;
        int posReported = 1000;
        iteratePositions(tbType, [&](Position& pos) {
            nPos++;
            int success;
            int dtz = Syzygy::probe_dtz(pos, &success);
            if (!success)
                throw ChessParseError("RTB probe failed, pos:" + TextIO::toFEN(pos));
            int wdl = Syzygy::probe_wdl(pos, &success);
            if (!success)
                throw ChessParseError("RTB probe failed, pos:" + TextIO::toFEN(pos));
            if (dtz > 0) {
                if (wdl == 2) {
                    if (dtz > posScore) {
                        posScore = dtz;
                        posPos = pos;
                    }
                    if (dtz > 100 && dtz < posReported) {
                        posReported = dtz;
                        std::cout << "fen: " << TextIO::toFEN(pos) << " dtz:" << dtz << std::endl;
                    }
                }
            } else if (dtz < 0) {
                if (wdl == -2) {
                    if (dtz < negScore) {
                        negScore = dtz;
                        negPos = pos;
                    }
                    if (dtz < -100 && dtz > negReported) {
                        negReported = dtz;
                        std::cout << "fen: " << TextIO::toFEN(pos) << " dtz:" << dtz << std::endl;
                    }
                }
            }
        });
        double t1 = currentTime();
        std::cout << tbType << " neg: " << negScore << " pos:" << posScore << " nPos:" << nPos
                  << " t:" << (t1-t0) << std::endl;
        std::cout << tbType << " negPos: " << TextIO::toFEN(negPos) << std::endl;
        std::cout << tbType << " posPos: " << TextIO::toFEN(posPos) << std::endl;
    }
}

/** Convert a 2-character string to a piece type. */
static Piece::Type
getPieceType(const std::string& s) {
    if (s.length() != 2)
        return Piece::EMPTY;
    bool white;
    if (s[0] == 'w')
        white = true;
    else if (s[0] == 'b')
        white = false;
    else
        return Piece::EMPTY;
    switch (s[1]) {
    case 'k': return white ? Piece::WKING : Piece::BKING;
    case 'q': return white ? Piece::WQUEEN : Piece::BQUEEN;
    case 'r': return white ? Piece::WROOK : Piece::BROOK;
    case 'b': return white ? Piece::WBISHOP : Piece::BBISHOP;
    case 'n': return white ? Piece::WKNIGHT : Piece::BKNIGHT;
    case 'p': return white ? Piece::WPAWN : Piece::BPAWN;
    default:
        return Piece::EMPTY;
    }
}

void
PosGenerator::egStat(const std::string& tbType, const std::vector<std::string>& pieceTypes) {
    ChessTool::setupTB();
    double t0 = currentTime();

    std::vector<Piece::Type> ptVec;
    for (const std::string& s : pieceTypes) {
        Piece::Type p = getPieceType(s);
        if (p == Piece::EMPTY)
            throw ChessParseError("Invalid piece type:" + s);
        ptVec.push_back(p);
    }

    TranspositionTable tt(19);
    ParallelData pd(tt);
    std::vector<U64> nullHist(200);
    KillerTable kt;
    History ht;
    auto et = Evaluate::getEvalHashTables();
    Search::SearchTables st(tt, kt, ht, *et);
    TreeLogger treeLog;
    TranspositionTable::TTEntry ent;
    const int UNKNOWN_SCORE = -32767; // Represents unknown static eval score

    struct ScoreStat { U64 whiteWin = 0, draw = 0, blackWin = 0; };
    std::map<std::vector<int>, ScoreStat> stat; // sequence of squares -> wdl statistics
    std::vector<int> key;
    ScoreToProb s2p;
    U64 total = 0, rejected = 0, nextReport = 0;
    iteratePositions(tbType, false, [&](Position& pos) {
        total++;
        int evScore, qScore;
        {
            const int mate0 = SearchConst::MATE0;
            Search sc(pos, nullHist, 0, st, pd, nullptr, treeLog);
            sc.init(pos, nullHist, 0);
            sc.q0Eval = UNKNOWN_SCORE;
            qScore = sc.quiesce(-mate0, mate0, 0, 0, MoveGen::inCheck(pos));
            Evaluate ev(*et);
            evScore = ev.evalPos(pos);
            if (std::abs(s2p.getProb(qScore) - s2p.getProb(evScore)) > 0.25) {
                rejected++;
                return;
            }
        }

        key.clear();
        for (auto pt : ptVec) {
            U64 m = pos.pieceTypeBB(pt);
            while (m) {
                int sq = BitBoard::extractSquare(m);
                key.push_back(sq);
            }
        }
        ScoreStat& ss = stat[key];

        int score;
        if (!TBProbe::rtbProbeWDL(pos, 0, score, ent))
            throw ChessParseError("RTB probe failed, pos:" + TextIO::toFEN(pos));
        if (!pos.isWhiteMove())
            score = -score;
        if (score > 0)
            ss.whiteWin++;
        else if (score < 0)
            ss.blackWin++;
        else
            ss.draw++;

        if (total >= nextReport) {
            nextReport += 4*1024*1024;
            std::cerr << "total:" << total << " rejected:" << rejected << std::endl;
        }
    });
    double t1 = currentTime();

    int nDigits = 1;
    {
        U64 maxVal = 0;
        for (const auto& s : stat)
            maxVal = std::max({maxVal, s.second.whiteWin, s.second.draw, s.second.blackWin});
        while (maxVal >= 10) {
            nDigits++;
            maxVal /= 10;
        }
    }

    for (const auto& p : stat) {
        const std::vector<int>& key = p.first;
        const ScoreStat& ss = p.second;
        for (size_t i = 0; i < key.size(); i++) {
            if (i > 0)
                std::cout << ' ';
            std::cout << std::setw(2) << key[i];
        }
        for (size_t i = 0; i < key.size(); i++)
            std::cout << ' ' << TextIO::squareToString(key[i]);

        std::cout << " :";
        int N = ss.whiteWin + ss.draw + ss.blackWin;
        double e = (N > 0) ? (ss.whiteWin + ss.draw * 0.5) / N : 0;
        std::cout << std::setw(nDigits+1) << ss.whiteWin
                  << std::setw(nDigits+1) << ss.draw
                  << std::setw(nDigits+1) << ss.blackWin
                  << ' ' << static_cast<int>(e * 1000 + 0.5) << '\n';
    }
    std::cout << " t:" << (t1-t0) << std::endl;
}

void
PosGenerator::wdlTest(const std::vector<std::string>& tbTypes) {
    ChessTool::setupTB();
    TranspositionTable::TTEntry ent;
    for (std::string tbType : tbTypes) {
        double t0 = currentTime();
        U64 nPos = 0, nDiff = 0, nDiff50 = 0;
        iteratePositions(tbType, [&](Position& pos) {
            nPos++;
            int rtbScore, gtbScore;
            if (!TBProbe::rtbProbeWDL(pos, 0, rtbScore, ent))
                throw ChessParseError("RTB probe failed, pos:" + TextIO::toFEN(pos));
            if (!TBProbe::gtbProbeWDL(pos, 0, gtbScore))
                throw ChessParseError("GTB probe failed, pos:" + TextIO::toFEN(pos));
            bool diff;
            if (rtbScore > 0) {
                diff = gtbScore <= 0;
            } else if (rtbScore < 0) {
                diff = gtbScore >= 0;
            } else {
                diff = gtbScore != 0;
                if (diff) {
                    int scoreDTM;
                    if (!TBProbe::gtbProbeDTM(pos, 0, scoreDTM))
                        throw ChessParseError("GTB probe failed, pos:" + TextIO::toFEN(pos));
                    if (std::abs(scoreDTM) < SearchConst::MATE0 - 100) {
                        diff = false;
                        nDiff50++;
                    }
                }
            }
            if (diff) {
                nDiff++;
                std::cout << tbType << " rtb:" << rtbScore << " gtb:" << gtbScore
                          << " pos:" << TextIO::toFEN(pos) << std::endl;
            }
        });
        double t1 = currentTime();
        std::cout << tbType << " nPos:" << nPos << " nDiff:" << nDiff << " nDiff50:" << nDiff50
                  << " t:" << (t1-t0) << std::endl;
    }
}

void
PosGenerator::dtzTest(const std::vector<std::string>& tbTypes) {
    ChessTool::setupTB();
    TranspositionTable::TTEntry ent;
    for (std::string tbType : tbTypes) {
        double t0 = currentTime();
        U64 nPos = 0, nDiff = 0, nDiff50 = 0;
        int minSlack = std::numeric_limits<int>::max();
        int maxSlack = std::numeric_limits<int>::min();
        int minSlack2 = std::numeric_limits<int>::max();
        int maxSlack2 = std::numeric_limits<int>::min();
        iteratePositions(tbType, [&](Position& pos) {
            nPos++;
            int dtz, dtm, wdl;
            if (!TBProbe::rtbProbeDTZ(pos, 0, dtz, ent))
                throw ChessParseError("RTB probe failed, pos:" + TextIO::toFEN(pos));
            if (!TBProbe::gtbProbeDTM(pos, 0, dtm))
                throw ChessParseError("GTB probe failed, pos:" + TextIO::toFEN(pos));
            if (!TBProbe::rtbProbeWDL(pos, 0, wdl, ent))
                throw ChessParseError("RTB probe failed, pos:" + TextIO::toFEN(pos));
            bool diff;
            int slack = 0;
            int slack2 = 0;
            if (dtz > 0) {
                slack = dtm - dtz;
                slack2 = dtz - wdl;
                diff = dtm <= 0 || slack < 0 || slack2 < 0;
            } else if (dtz < 0) {
                slack = -(dtm - dtz);
                slack2 = -(dtz - wdl);
                diff = dtm >= 0 || slack < 0 || slack2 < 0;
            } else {
                diff = dtm != 0;
                if (diff) {
                    if (std::abs(dtm) < SearchConst::MATE0 - 100) {
                        diff = false;
                        nDiff50++;
                    }
                }
            }
            minSlack = std::min(minSlack, slack);
            maxSlack = std::max(maxSlack, slack);
            minSlack2 = std::min(minSlack2, slack2);
            maxSlack2 = std::max(maxSlack2, slack2);
            if (diff) {
                nDiff++;
                std::cout << tbType << " dtz:" << dtz << " dtm:" << dtm
                          << " pos:" << TextIO::toFEN(pos) << std::endl;
            }
        });
        double t1 = currentTime();
        std::cout << tbType << " nPos:" << nPos << " nDiff:" << nDiff << " nDiff50:" << nDiff50
                  << " t:" << (t1-t0) << std::endl;
        std::cout << tbType << " minSlack:" << minSlack << " maxSlack:" << maxSlack
                  << " minSlack2:" << minSlack2 << " maxSlack2:" << maxSlack2 << std::endl;
    }
}

void
PosGenerator::tbgenTest(const std::vector<std::string>& tbTypes) {
    ChessTool::setupTB();
    for (std::string tbType : tbTypes) {
        std::vector<int> pieces;
        bool whitePawns, blackPawns;
        getPieces(tbType, pieces, whitePawns, blackPawns);
        if (whitePawns || blackPawns) {
            std::cout << "tbType: " << tbType << " pawns not supported" << std::endl;
            continue;
        }
        PieceCount pc;
        pc.nwq = pc.nwr = pc.nwb = pc.nwn = 0;
        pc.nbq = pc.nbr = pc.nbb = pc.nbn = 0;
        for (int p : pieces) {
            switch (p) {
            case Piece::WQUEEN:  pc.nwq++; break;
            case Piece::WROOK:   pc.nwr++; break;
            case Piece::WBISHOP: pc.nwb++; break;
            case Piece::WKNIGHT: pc.nwn++; break;
            case Piece::BQUEEN:  pc.nbq++; break;
            case Piece::BROOK:   pc.nbr++; break;
            case Piece::BBISHOP: pc.nbb++; break;
            case Piece::BKNIGHT: pc.nbn++; break;
            }
        }
        VectorStorage vs;
        TBGenerator<VectorStorage> tbGen(vs, pc);
        RelaxedShared<S64> maxTimeMillis(-1);
        tbGen.generate(maxTimeMillis, false);
        double t0 = currentTime();

        U64 nPos = 0;
        iteratePositions(tbType, [&](Position& pos) {
            nPos++;
            int score, gtbScore;
            if (!tbGen.probeDTM(pos, 0, score))
                throw ChessParseError("tbGen probe failed, pos:" + TextIO::toFEN(pos));
            if (!TBProbe::gtbProbeDTM(pos, 0, gtbScore))
                throw ChessParseError("GTB probe failed, pos:" + TextIO::toFEN(pos));
            if (score != gtbScore) {
                std::cout << tbType << " i:" << nPos << " score:" << score << " gtbScore:" << gtbScore
                          << " pos:" << TextIO::toFEN(pos) << std::endl;
                throw ChessParseError("stop");
            }
        });
        double t1 = currentTime();
        std::cout << tbType << " nPos:" << nPos << " compare time:" << (t1-t0) << std::endl;
    }
}
