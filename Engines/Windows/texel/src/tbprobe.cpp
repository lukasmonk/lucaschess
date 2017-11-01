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
 * tbprobe.cpp
 *
 *  Created on: Jun 2, 2014
 *      Author: petero
 */

#include "tbprobe.hpp"
#include "gtb/gtb-probe.h"
#include "syzygy/rtb-probe.hpp"
#include "bitBoard.hpp"
#include "position.hpp"
#include "moveGen.hpp"
#include "constants.hpp"
#include <unordered_map>
#include <cassert>

#include "util/timeUtil.hpp"

static std::string currentGtbPath;
static int currentGtbCacheMB;
static int currentGtbWdlFraction;
static std::string currentRtbPath;

static const char** gtbPaths = nullptr;
static int gtbMaxPieces = 0;

int TBProbeData::maxPieces = 0;

static std::unordered_map<int,int> maxDTM; // MatId -> Max DTM value in GTB TB
static std::unordered_map<int,int> maxDTZ; // MatId -> Max DTZ value in RTB TB
struct IIPairHash {
    size_t operator()(const std::pair<int,int>& p) const {
        return ((U64)p.first) * 0x714d3559 + (U64)p.second;
    }
};
// (MatId,maxPawnMoves) -> Max DTM in sub TBs
static std::unordered_map<std::pair<int,int>,int,IIPairHash> maxSubDTM;


void
TBProbe::initialize(const std::string& gtbPath, int cacheMB,
                    const std::string& rtbPath) {
    if (rtbPath != currentRtbPath) {
        Syzygy::init(rtbPath);
        currentRtbPath = rtbPath;
    }

    int wdlFraction = Syzygy::TBLargest >= gtbMaxPieces ? 8 : 96;
    if ((gtbPath != currentGtbPath) ||
        (cacheMB != currentGtbCacheMB) ||
        (wdlFraction != currentGtbWdlFraction)) {
        gtbInitialize(gtbPath, cacheMB, wdlFraction);
        currentGtbPath = gtbPath;
        currentGtbCacheMB = cacheMB;
        currentGtbWdlFraction = wdlFraction;
    }

    static bool initialized = false;
    if (!initialized) {
        initWDLBounds();
        initialized = true;
    }

    TBProbeData::maxPieces = std::max({4, gtbMaxPieces, Syzygy::TBLargest});
}

bool
TBProbe::tbEnabled() {
    return Syzygy::TBLargest > 0 || gtbMaxPieces > 0;
}

const int maxFrustratedDist = 1000;

static inline void updateEvScore(TranspositionTable::TTEntry& ent,
                                 int newScore) {
    int oldScore = ent.getEvalScore();
    if ((oldScore == 0) || (std::abs(newScore) < std::abs(oldScore)))
        ent.setEvalScore(newScore);
}

/**
 * Return the margin (in number of plies) for a win to turn into a draw
 * because of the 50 move rule. If the margin is negative the position is
 * a draw, and ent.evalScore is set to indicate how far away from a win
 * the position is.
 */
static inline int rule50Margin(int dtmScore, int ply, int hmc,
                               TranspositionTable::TTEntry& ent) {
    int margin = (100 - hmc) - (SearchConst::MATE0 - 1 - abs(dtmScore) - ply);
    if (margin < 0)
        updateEvScore(ent, dtmScore > 0 ? -margin : margin);
    return margin;
}

bool
TBProbe::tbProbe(Position& pos, int ply, int alpha, int beta,
                 const TranspositionTable& tt, TranspositionTable::TTEntry& ent,
                 const int nPieces) {
    // Probe on-demand TB
    const int hmc = pos.getHalfMoveClock();
    bool hasDtm = false;
    int dtmScore;
    if (nPieces <= 4 && tt.probeDTM(pos, ply, dtmScore)) {
        if ((dtmScore == 0) || (rule50Margin(dtmScore, ply, hmc, ent) >= 0)) {
            ent.setScore(dtmScore, ply);
            ent.setType(TType::T_EXACT);
            return true;
        }
        ent.setScore(0, ply);
        ent.setType(dtmScore > 0 ? TType::T_GE : TType::T_LE);
        hasDtm = true;
    }

    // Try WDL probe. If the result is not draw, it can only be trusted if hmc == 0.
    // 5-men GTB WDL probes can only be trusted if the score is draw, because they
    // don't take the 50 move draw rule into account.
    bool hasResult = false;
    bool checkABBound = false;
    int wdlScore;
    if (nPieces <= Syzygy::TBLargest && rtbProbeWDL(pos, ply, wdlScore, ent)) {
        if ((wdlScore == 0) || (hmc == 0))
            hasResult = true;
        else
            checkABBound = true;
    } else if (nPieces <= gtbMaxPieces && gtbProbeWDL(pos, ply, wdlScore)) {
        if ((wdlScore == 0) || (hmc == 0 && nPieces <= 4))
            hasResult = true;
        else
            checkABBound = true;
    }
    if (checkABBound) {
        if ((wdlScore > 0) && (beta <= 0)) { // WDL says win but could be draw due to 50move rule
            ent.setScore(0, ply);
            ent.setType(TType::T_GE);
            return true;
        }
        if ((wdlScore < 0) && (alpha >= 0)) { // WDL says loss but could be draw due to 50move rule
            ent.setScore(0, ply);
            ent.setType(TType::T_LE);
            return true;
        }
    }
    bool frustrated = false;
    if (hasResult) {
        ent.setScore(wdlScore, ply);
        if (wdlScore > 0) {
            ent.setType(TType::T_GE);
            if (wdlScore >= beta)
                return true;
        } else if (wdlScore < 0) {
            ent.setType(TType::T_LE);
            if (wdlScore <= alpha)
                return true;
        } else {
            ent.setType(TType::T_EXACT);
            int evScore = ent.getEvalScore();
            if (evScore == 0) {
                return true;
            } else if (evScore > 0) {
                if (beta <= SearchConst::minFrustrated)
                    return true;
                frustrated = true;
            } else {
                if (alpha >= -SearchConst::minFrustrated)
                    return true;
                frustrated = true;
            }
        }
    }

    const bool dtmFirst = frustrated || SearchConst::isLoseScore(alpha) || SearchConst::isWinScore(beta);
    // Try GTB DTM probe if searching for fastest mate
    if (dtmFirst && !hasDtm && nPieces <= gtbMaxPieces) {
        if (gtbProbeDTM(pos, ply, dtmScore)) {
            if ((dtmScore == 0) || (rule50Margin(dtmScore, ply, hmc, ent) >= 0)) {
                ent.setScore(dtmScore, ply);
                ent.setType(TType::T_EXACT);
                return true;
            }
            ent.setScore(0, ply);
            ent.setType(dtmScore > 0 ? TType::T_GE : TType::T_LE);
            hasDtm = true;
        }
    }

    // Try RTB DTZ probe
    int dtzScore;
    if (nPieces <= Syzygy::TBLargest && rtbProbeDTZ(pos, ply, dtzScore, ent)) {
        hasResult = true;
        ent.setScore(dtzScore, ply);
        if (dtzScore > 0) {
            ent.setType(TType::T_GE);
            if (dtzScore >= beta)
                return true;
        } else if (dtzScore < 0) {
            ent.setType(TType::T_LE);
            if (dtzScore <= alpha)
                return true;
        } else {
            ent.setType(TType::T_EXACT);
            return true;
        }
    }

    // Try GTB DTM probe if not searching for fastest mate
    if (!dtmFirst && !hasDtm && nPieces <= gtbMaxPieces) {
        if (gtbProbeDTM(pos, ply, dtmScore)) {
            if ((dtmScore == 0) || (rule50Margin(dtmScore, ply, hmc, ent) >= 0)) {
                ent.setScore(dtmScore, ply);
                ent.setType(TType::T_EXACT);
                return true;
            }
            ent.setScore(0, ply);
            ent.setType(dtmScore > 0 ? TType::T_GE : TType::T_LE);
            hasDtm = true;
        }
    }

    return hasResult || hasDtm;
}

bool
TBProbe::getSearchMoves(Position& pos, const MoveList& legalMoves,
                        std::vector<Move>& movesToSearch,
                        const TranspositionTable& tt) {
    const int mate0 = SearchConst::MATE0;
    const int ply = 0;
    TranspositionTable::TTEntry rootEnt;
    rootEnt.clear();
    if (!tbProbe(pos, ply, -mate0, mate0, tt, rootEnt) || rootEnt.getType() == TType::T_LE)
        return false;
    const int rootScore = rootEnt.getScore(ply);
    if (!SearchConst::isWinScore(rootScore))
        return false;

    // Root position is TB win
    bool hasProgress = false;
    UndoInfo ui;
    for (int mi = 0; mi < legalMoves.size; mi++) {
        const Move& m = legalMoves[mi];
        pos.makeMove(m, ui);
        TranspositionTable::TTEntry ent;
        ent.clear();
        bool progressMove = false;
        bool badMove = false;
        if (tbProbe(pos, ply+1, -mate0, mate0, tt, ent)) {
            const int type = ent.getType();
            const int score = -ent.getScore(ply+1);
            if (score >= rootScore && (type == TType::T_EXACT || type == TType::T_LE))
                progressMove = true;
            if ((score < rootScore - 1)) // -1 to handle +/-1 uncertainty in RTB tables
                badMove = true;
        }
        if (progressMove)
            hasProgress = true;
        if (!badMove)
            movesToSearch.push_back(m);
        pos.unMakeMove(m, ui);
    }

    return !hasProgress && !movesToSearch.empty();
}

bool
TBProbe::dtmProbe(Position& pos, int ply, const TranspositionTable& tt, int& score) {
    const int nPieces = BitBoard::bitCount(pos.occupiedBB());
    if (nPieces <= 4 && tt.probeDTM(pos, ply, score))
        return true;
    if (TBProbe::gtbProbeDTM(pos, ply, score))
        return true;
    return false;
}

void
TBProbe::extendPV(const Position& rootPos, std::vector<Move>& pv, const TranspositionTable& tt) {
    Position pos(rootPos);
    UndoInfo ui;
    int ply = 0;
    int score;
    for (int i = 0; i < (int)pv.size(); i++) {
        const Move& m = pv[i];
        pos.makeMove(m, ui);
        if (dtmProbe(pos, ply, tt, score) && SearchConst::isWinScore(std::abs(score)) &&
            (SearchConst::MATE0 - 1 - abs(score) - ply <= 100 - pos.getHalfMoveClock())) {
            // TB win, replace rest of PV since it may be inaccurate
            pv.erase(pv.begin()+i+1, pv.end());
            break;
        }
    }
    if (!dtmProbe(pos, ply, tt, score) || !SearchConst::isWinScore(std::abs(score)))
        return; // No TB win
    if (SearchConst::MATE0 - 1 - abs(score) - ply > 100 - pos.getHalfMoveClock())
        return; // Mate too far away, perhaps 50-move draw
    if (!pos.isWhiteMove())
        score = -score;
    while (true) {
        MoveList moveList;
        MoveGen::pseudoLegalMoves(pos, moveList);
        MoveGen::removeIllegal(pos, moveList);
        bool extended = false;
        for (int mi = 0; mi < moveList.size; mi++) {
            const Move& m = moveList[mi];
            pos.makeMove(m, ui);
            int newScore;
            if (dtmProbe(pos, ply+1, tt, newScore)) {
                if (!pos.isWhiteMove())
                    newScore = -newScore;
                if (newScore == score) {
                    pv.push_back(m);
                    ply++;
                    extended = true;
                    break;
                }
            }
            pos.unMakeMove(m, ui);
        }
        if (!extended)
            break;
    }
}

template <typename ProbeFunc>
static void handleEP(Position& pos, int ply, int& score, bool& ret, ProbeFunc probeFunc) {
    const bool inCheck = MoveGen::inCheck(pos);
    MoveList moveList;
    if (inCheck) MoveGen::checkEvasions(pos, moveList);
    else         MoveGen::pseudoLegalMoves(pos, moveList);
    const int pawn = pos.isWhiteMove() ? Piece::WPAWN : Piece::BPAWN;
    int bestEP = std::numeric_limits<int>::min();
    UndoInfo ui;
    for (int m = 0; m < moveList.size; m++) {
        const Move& move = moveList[m];
        if ((move.to() == pos.getEpSquare()) && (pos.getPiece(move.from()) == pawn)) {
            if (MoveGen::isLegal(pos, move, inCheck)) {
                pos.makeMove(move, ui);
                int score2;
                bool ret2 = probeFunc(pos, ply+1, score2);
                pos.unMakeMove(move, ui);
                if (!ret2) {
                    ret = false;
                    return;
                }
                bestEP = std::max(bestEP, -score2);
            }
        } else if (MoveGen::isLegal(pos, move, inCheck))
            return;
    }
    if (bestEP != std::numeric_limits<int>::min())
        score = bestEP;
}

bool
TBProbe::gtbProbeDTM(Position& pos, int ply, int& score) {
    if (BitBoard::bitCount(pos.occupiedBB()) > gtbMaxPieces)
        return false;

    GtbProbeData gtbData;
    getGTBProbeData(pos, gtbData);
    bool ret = gtbProbeDTM(gtbData, ply, score);
    if (ret && score == 0 && pos.getEpSquare() != -1)
        handleEP(pos, ply, score, ret, [](Position& pos, int ply, int& score) -> bool {
            return TBProbe::gtbProbeDTM(pos, ply, score);
        });
    return ret;
}

bool
TBProbe::gtbProbeWDL(Position& pos, int ply, int& score) {
    if (BitBoard::bitCount(pos.occupiedBB()) > gtbMaxPieces)
        return false;

    GtbProbeData gtbData;
    getGTBProbeData(pos, gtbData);
    bool ret = gtbProbeWDL(gtbData, ply, score);
    if (ret && score == 0 && pos.getEpSquare() != -1)
        handleEP(pos, ply, score, ret, [](Position& pos, int ply, int& score) -> bool {
            return TBProbe::gtbProbeWDL(pos, ply, score);
        });
    return ret;
}

bool
TBProbe::rtbProbeDTZ(Position& pos, int ply, int& score,
                     TranspositionTable::TTEntry& ent) {
    const int nPieces = BitBoard::bitCount(pos.occupiedBB());
    if (nPieces > Syzygy::TBLargest)
        return false;
    if (pos.getCastleMask())
        return false;

    int success;
    const int dtz = Syzygy::probe_dtz(pos, &success);
    if (!success)
        return false;
    if (dtz == 0) {
        score = 0;
        ent.setEvalScore(0);
        return true;
    }
    const int maxHalfMoveClock = std::abs(dtz) + pos.getHalfMoveClock();
    const int sgn = dtz > 0 ? 1 : -1;
    if ((maxHalfMoveClock == 100) && (pos.getHalfMoveClock() > 0) &&
        (maxDTZ.find(pos.materialId())->second != 100)) // dtz can be off by one
        return false;
    if (abs(dtz) <= 2) {
        if (maxHalfMoveClock > 101) {
            score = 0;
            updateEvScore(ent, sgn * (maxHalfMoveClock - 100));
            return true;
        } else if (maxHalfMoveClock == 101)
            return false; // DTZ can be wrong when mate-in-1
    } else {
        if (maxHalfMoveClock > 100) {
            score = 0;
            if (std::abs(dtz) <= 100)
                updateEvScore(ent, sgn * (maxHalfMoveClock - 100));
            else
                updateEvScore(ent, sgn * maxFrustratedDist);
            return true;
        }
    }
    int plyToMate = getMaxSubMate(pos) + std::abs(dtz);
    if (dtz > 0) {
        score = SearchConst::MATE0 - ply - plyToMate - 2;
    } else {
        score = -(SearchConst::MATE0 - ply - plyToMate - 2);
    }
    return true;
}

bool
TBProbe::rtbProbeWDL(Position& pos, int ply, int& score,
                     TranspositionTable::TTEntry& ent) {
    if (BitBoard::bitCount(pos.occupiedBB()) > Syzygy::TBLargest)
        return false;
    if (pos.getCastleMask())
        return false;

    int success;
    int wdl = Syzygy::probe_wdl(pos, &success);
    if (!success)
        return false;
    int plyToMate;
    switch (wdl) {
    case 0:
        score = 0;
        break;
    case 1:
        score = 0;
        if (ent.getEvalScore() == 0)
            ent.setEvalScore(maxFrustratedDist);
        break;
    case -1:
        score = 0;
        if (ent.getEvalScore() == 0)
            ent.setEvalScore(-maxFrustratedDist);
        break;
    case 2:
        plyToMate = getMaxSubMate(pos) + getMaxDTZ(pos.materialId());
        score = SearchConst::MATE0 - ply - plyToMate - 2;
        break;
    case -2:
        plyToMate = getMaxSubMate(pos) + getMaxDTZ(pos.materialId());
        score = -(SearchConst::MATE0 - ply - plyToMate - 2);
        break;
    default:
        return false;
    }

    return true;
}

void
TBProbe::gtbInitialize(const std::string& path, int cacheMB, int wdlFraction) {
    static_assert((int)tb_A1 == (int)A1, "Incompatible square numbering");
    static_assert((int)tb_A8 == (int)A8, "Incompatible square numbering");
    static_assert((int)tb_H1 == (int)H1, "Incompatible square numbering");
    static_assert((int)tb_H8 == (int)H8, "Incompatible square numbering");

    tbpaths_done(gtbPaths);

    gtbMaxPieces = 0;
    gtbPaths = tbpaths_init();
    gtbPaths = tbpaths_add(gtbPaths, path.c_str());

    TB_compression_scheme scheme = tb_CP4;
    int verbose = 0;
    int cacheSize = 1024 * 1024 * cacheMB;
    static bool isInitialized = false;
    if (isInitialized) {
        tb_restart(verbose, scheme, gtbPaths);
        tbcache_restart(cacheSize, wdlFraction);
    } else {
        tb_init(verbose, scheme, gtbPaths);
        tbcache_init(cacheSize, wdlFraction);
    }
    isInitialized = true;

    unsigned int av = tb_availability();
    if (av & 3)
        gtbMaxPieces = 3;
    if (av & 12)
        gtbMaxPieces = 4;
    if (av & 48)
        gtbMaxPieces = 5;
}

void
TBProbe::getGTBProbeData(const Position& pos, GtbProbeData& gtbData) {
    gtbData.stm = pos.isWhiteMove() ? tb_WHITE_TO_MOVE : tb_BLACK_TO_MOVE;
    gtbData.epsq = pos.getEpSquare() >= 0 ? pos.getEpSquare() : tb_NOSQUARE;

    gtbData.castles = 0;
    if (pos.a1Castle()) gtbData.castles |= tb_WOOO;
    if (pos.h1Castle()) gtbData.castles |= tb_WOO;
    if (pos.a8Castle()) gtbData.castles |= tb_BOOO;
    if (pos.h8Castle()) gtbData.castles |= tb_BOO;

    int cnt = 0;
    U64 m = pos.whiteBB();
    while (m != 0) {
        int sq = BitBoard::extractSquare(m);
        gtbData.wSq[cnt] = sq;
        switch (pos.getPiece(sq)) {
        case Piece::WKING:   gtbData.wP[cnt] = tb_KING;   break;
        case Piece::WQUEEN:  gtbData.wP[cnt] = tb_QUEEN;  break;
        case Piece::WROOK:   gtbData.wP[cnt] = tb_ROOK;   break;
        case Piece::WBISHOP: gtbData.wP[cnt] = tb_BISHOP; break;
        case Piece::WKNIGHT: gtbData.wP[cnt] = tb_KNIGHT; break;
        case Piece::WPAWN:   gtbData.wP[cnt] = tb_PAWN;   break;
        default:
            assert(false);
        }
        cnt++;
    }
    gtbData.wSq[cnt] = tb_NOSQUARE;
    gtbData.wP[cnt] = tb_NOPIECE;

    cnt = 0;
    m = pos.blackBB();
    while (m != 0) {
        int sq = BitBoard::extractSquare(m);
        gtbData.bSq[cnt] = sq;
        switch (pos.getPiece(sq)) {
        case Piece::BKING:   gtbData.bP[cnt] = tb_KING;   break;
        case Piece::BQUEEN:  gtbData.bP[cnt] = tb_QUEEN;  break;
        case Piece::BROOK:   gtbData.bP[cnt] = tb_ROOK;   break;
        case Piece::BBISHOP: gtbData.bP[cnt] = tb_BISHOP; break;
        case Piece::BKNIGHT: gtbData.bP[cnt] = tb_KNIGHT; break;
        case Piece::BPAWN:   gtbData.bP[cnt] = tb_PAWN;   break;
        default:
            assert(false);
        }
        cnt++;
    }
    gtbData.bSq[cnt] = tb_NOSQUARE;
    gtbData.bP[cnt] = tb_NOPIECE;
    gtbData.materialId = pos.materialId();
}

bool
TBProbe::gtbProbeDTM(const GtbProbeData& gtbData, int ply, int& score) {
    unsigned int tbInfo;
    unsigned int plies;
    if (!tb_probe_hard(gtbData.stm, gtbData.epsq, gtbData.castles,
                       gtbData.wSq, gtbData.bSq,
                       gtbData.wP, gtbData.bP,
                       &tbInfo, &plies))
        return false;

    switch (tbInfo) {
    case tb_DRAW:
        score = 0;
        break;
    case tb_WMATE:
        score = SearchConst::MATE0 - ply - plies - 1;
        break;
    case tb_BMATE:
        score = -(SearchConst::MATE0 - ply - plies - 1);
        break;
    default:
        return false;
    };

    if (gtbData.stm == tb_BLACK_TO_MOVE)
        score = -score;

    return true;
}

bool
TBProbe::gtbProbeWDL(const GtbProbeData& gtbData, int ply, int& score) {
    unsigned int tbInfo;
    if (!tb_probe_WDL_hard(gtbData.stm, gtbData.epsq, gtbData.castles,
                           gtbData.wSq, gtbData.bSq,
                           gtbData.wP, gtbData.bP,
                           &tbInfo))
        return false;

    switch (tbInfo) {
    case tb_DRAW:
        score = 0;
        break;
    case tb_WMATE:
        score = maxDTM[gtbData.materialId] - ply;
        break;
    case tb_BMATE:
        score = -(maxDTM[gtbData.materialId] - ply);
        break;
    default:
        return false;
    };

    if (gtbData.stm == tb_BLACK_TO_MOVE)
        score = -score;
    return true;
}

void
TBProbe::initWDLBounds() {
    initMaxDTM();
    initMaxDTZ();

    // Pre-calculate all interesting maxSubDTM values
    for (int wp = 0; wp <= 4; wp++) {
        std::vector<int> pieces(Piece::nPieceTypes);
        pieces[Piece::WPAWN] = wp;
        pieces[Piece::BPAWN] = 4 - wp;
        getMaxSubMate(pieces, 4*5);
    }
}

/** Maximum DTZ value for a given material configuration. */
int
TBProbe::getMaxDTZ(int matId) {
    auto it = maxDTZ.find(matId);
    if (it == maxDTZ.end())
        return 100;
    int val = it->second;
    if (val < 0)
        return 0;
    else
        return std::min(val+2, 100); // RTB DTZ values are not exact
}

static int
getMaxPawnMoves(const Position& pos) {
    int maxPawnMoves = 0;
    U64 m = pos.pieceTypeBB(Piece::WPAWN);
    while (m != 0) {
        int sq = BitBoard::extractSquare(m);
        maxPawnMoves += 6 - Position::getY(sq);
    }
    m = pos.pieceTypeBB(Piece::BPAWN);
    while (m != 0) {
        int sq = BitBoard::extractSquare(m);
        maxPawnMoves += Position::getY(sq) - 1;
    }
    return maxPawnMoves;
}

/** Get upper bound on longest mate in all possible material configurations
 * after the next zeroing move. */
int
TBProbe::getMaxSubMate(const Position& pos) {
    int maxPawnMoves = getMaxPawnMoves(pos);
    int matId = pos.materialId();
    matId = std::min(matId, MatId::mirror(matId));
    auto it = maxSubDTM.find(std::make_pair(matId,maxPawnMoves));
    if (it != maxSubDTM.end())
        return it->second;

    std::vector<int> pieces(Piece::nPieceTypes);
    for (int p = 0; p < Piece::nPieceTypes; p++)
        pieces[p] = BitBoard::bitCount(pos.pieceTypeBB((Piece::Type)p));
    pieces[Piece::EMPTY] = pieces[Piece::WKING] = pieces[Piece::BKING] = 0;
    return getMaxSubMate(pieces, maxPawnMoves);
}

int
TBProbe::getMaxSubMate(std::vector<int>& pieces, int pawnMoves) {
    assert(pawnMoves >= 0);
    if (pawnMoves > (pieces[Piece::WPAWN] + pieces[Piece::BPAWN]) * 5)
        return 0;

    MatId matId;
    for (int p = 0; p < Piece::nPieceTypes; p++)
        matId.addPieceCnt(p, pieces[p]);

    const int matIdMin = std::min(matId(), MatId::mirror(matId()));
    auto it = maxSubDTM.find(std::make_pair(matIdMin, pawnMoves));
    if (it != maxSubDTM.end())
        return it->second;

    int maxSubMate = 0;
    if (pawnMoves > 0) { // Pawn move
        maxSubMate = getMaxSubMate(pieces, pawnMoves-1) + getMaxDTZ(matId());
    }
    for (int p = 0; p < Piece::nPieceTypes; p++) { // Capture
        if (pieces[p] > 0) {
            pieces[p]--;
            matId.removePiece(p);
            int maxRemovedPawnMoves = 0;
            if (p == Piece::WPAWN || p == Piece::BPAWN)
                maxRemovedPawnMoves = 5;
            for (int i = 0; i <= maxRemovedPawnMoves; i++) {
                int newPawnMoves = pawnMoves - i;
                if (newPawnMoves >= 0) {
                    int tmp = getMaxSubMate(pieces, newPawnMoves) + getMaxDTZ(matId());
                    maxSubMate = std::max(maxSubMate, tmp);
                }
            }
            pieces[p]++;
            matId.addPiece(p);
        }
    }
    for (int c = 0; c < 2; c++) { // Promotion
        const int pawn = (c == 0) ? Piece::WPAWN : Piece::BPAWN;
        if (pieces[pawn] > 0) {
            const int p0 = (c == 0) ? Piece::WQUEEN : Piece::BQUEEN;
            const int p1 = (c == 0) ? Piece::WKNIGHT : Piece::BKNIGHT;
            for (int p = p0; p <= p1; p++) {
                pieces[pawn]--;
                pieces[p]++;
                matId.removePiece(pawn);
                matId.addPiece(p);
                int tmp = getMaxSubMate(pieces, pawnMoves) + getMaxDTZ(matId());
                maxSubMate = std::max(maxSubMate, tmp);
                pieces[pawn]++;
                pieces[p]--;
                matId.addPiece(pawn);
                matId.removePiece(p);
            }
        }
    }

#if 0
    std::cout << "wQ:" << pieces[Piece::WQUEEN]
              << " wR:" << pieces[Piece::WROOK]
              << " wB:" << pieces[Piece::WBISHOP]
              << " wN:" << pieces[Piece::WKNIGHT]
              << " wP:" << pieces[Piece::WPAWN]
              << " bQ:" << pieces[Piece::BQUEEN]
              << " bR:" << pieces[Piece::BROOK]
              << " bB:" << pieces[Piece::BBISHOP]
              << " bN:" << pieces[Piece::BKNIGHT]
              << " bP:" << pieces[Piece::BPAWN]
              << " pMoves:" << pawnMoves << " : " << maxSubMate << std::endl;
#endif
    maxSubDTM[std::make_pair(matIdMin, pawnMoves)] = maxSubMate;
    return maxSubMate;
}

void
TBProbe::initMaxDTM() {
    using MI = MatId;
    auto add = [](int id, int value) {
        maxDTM[id] = value;
        maxDTM[MatId::mirror(id)] = value;
    };

    add(MI::WQ, 31979);
    add(MI::WR, 31967);
    add(MI::WP, 31943);

    add(MI::WQ*2, 31979);
    add(MI::WQ+MI::WR, 31967);
    add(MI::WQ+MI::WB, 31979);
    add(MI::WQ+MI::WN, 31979);
    add(MI::WQ+MI::WP, 31943);
    add(MI::WR*2, 31967);
    add(MI::WR+MI::WB, 31967);
    add(MI::WR+MI::WN, 31967);
    add(MI::WR+MI::WP, 31943);
    add(MI::WB*2, 31961);
    add(MI::WB+MI::WN, 31933);
    add(MI::WB+MI::WP, 31937);
    add(MI::WN*2, 31998);
    add(MI::WN+MI::WP, 31943);
    add(MI::WP*2, 31935);
    add(MI::WQ+MI::BQ, 31974);
    add(MI::WR+MI::BQ, 31929);
    add(MI::WR+MI::BR, 31961);
    add(MI::WB+MI::BQ, 31965);
    add(MI::WB+MI::BR, 31941);
    add(MI::WB+MI::BB, 31998);
    add(MI::WN+MI::BQ, 31957);
    add(MI::WN+MI::BR, 31919);
    add(MI::WN+MI::BB, 31998);
    add(MI::WN+MI::BN, 31998);
    add(MI::WP+MI::BQ, 31942);
    add(MI::WP+MI::BR, 31914);
    add(MI::WP+MI::BB, 31942);
    add(MI::WP+MI::BN, 31942);
    add(MI::WP+MI::BP, 31933);

    add(MI::WQ*3, 31991);
    add(MI::WQ*2+MI::WR, 31987);
    add(MI::WQ*2+MI::WB, 31983);
    add(MI::WQ*2+MI::WN, 31981);
    add(MI::WQ*2+MI::WP, 31979);
    add(MI::WQ+MI::WR*2, 31985);
    add(MI::WQ+MI::WR+MI::WB, 31967);
    add(MI::WQ+MI::WR+MI::WN, 31967);
    add(MI::WQ+MI::WR+MI::WP, 31967);
    add(MI::WQ+MI::WB*2, 31961);
    add(MI::WQ+MI::WB+MI::WN, 31933);
    add(MI::WQ+MI::WB+MI::WP, 31937);
    add(MI::WQ+MI::WN*2, 31981);
    add(MI::WQ+MI::WN+MI::WP, 31945);
    add(MI::WQ+MI::WP*2, 31935);
    add(MI::WR*3, 31985);
    add(MI::WR*2+MI::WB, 31967);
    add(MI::WR*2+MI::WN, 31967);
    add(MI::WR*2+MI::WP, 31967);
    add(MI::WR+MI::WB*2, 31961);
    add(MI::WR+MI::WB+MI::WN, 31933);
    add(MI::WR+MI::WB+MI::WP, 31937);
    add(MI::WR+MI::WN*2, 31967);
    add(MI::WR+MI::WN+MI::WP, 31945);
    add(MI::WR+MI::WP*2, 31935);
    add(MI::WB*3, 31961);
    add(MI::WB*2+MI::WN, 31933);
    add(MI::WB*2+MI::WP, 31937);
    add(MI::WB+MI::WN*2, 31931);
    add(MI::WB+MI::WN+MI::WP, 31933);
    add(MI::WB+MI::WP*2, 31935);
    add(MI::WN*3, 31957);
    add(MI::WN*2+MI::WP, 31943);
    add(MI::WN+MI::WP*2, 31935);
    add(MI::WP*3, 31933);
    add(MI::WQ*2+MI::BQ, 31939);
    add(MI::WQ*2+MI::BR, 31929);
    add(MI::WQ*2+MI::BB, 31965);
    add(MI::WQ*2+MI::BN, 31957);
    add(MI::WQ*2+MI::BP, 31939);
    add(MI::WQ+MI::WR+MI::BQ, 31865);
    add(MI::WQ+MI::WR+MI::BR, 31929);
    add(MI::WQ+MI::WR+MI::BB, 31941);
    add(MI::WQ+MI::WR+MI::BN, 31919);
    add(MI::WQ+MI::WR+MI::BP, 31865);
    add(MI::WQ+MI::WB+MI::BQ, 31933);
    add(MI::WQ+MI::WB+MI::BR, 31919);
    add(MI::WQ+MI::WB+MI::BB, 31965);
    add(MI::WQ+MI::WB+MI::BN, 31957);
    add(MI::WQ+MI::WB+MI::BP, 31933);
    add(MI::WQ+MI::WN+MI::BQ, 31917);
    add(MI::WQ+MI::WN+MI::BR, 31918);
    add(MI::WQ+MI::WN+MI::BB, 31965);
    add(MI::WQ+MI::WN+MI::BN, 31957);
    add(MI::WQ+MI::WN+MI::BP, 31917);
    add(MI::WQ+MI::WP+MI::BQ, 31752);
    add(MI::WQ+MI::WP+MI::BR, 31913);
    add(MI::WQ+MI::WP+MI::BB, 31941);
    add(MI::WQ+MI::WP+MI::BN, 31939);
    add(MI::WQ+MI::WP+MI::BP, 31755);
    add(MI::WR*2+MI::BQ, 31901);
    add(MI::WR*2+MI::BR, 31937);
    add(MI::WR*2+MI::BB, 31941);
    add(MI::WR*2+MI::BN, 31919);
    add(MI::WR*2+MI::BP, 31900);
    add(MI::WR+MI::WB+MI::BQ, 31859);
    add(MI::WR+MI::WB+MI::BR, 31870);
    add(MI::WR+MI::WB+MI::BB, 31939);
    add(MI::WR+MI::WB+MI::BN, 31919);
    add(MI::WR+MI::WB+MI::BP, 31860);
    add(MI::WR+MI::WN+MI::BQ, 31861);
    add(MI::WR+MI::WN+MI::BR, 31918);
    add(MI::WR+MI::WN+MI::BB, 31937);
    add(MI::WR+MI::WN+MI::BN, 31919);
    add(MI::WR+MI::WN+MI::BP, 31864);
    add(MI::WR+MI::WP+MI::BQ, 31792);
    add(MI::WR+MI::WP+MI::BR, 31851);
    add(MI::WR+MI::WP+MI::BB, 31853);
    add(MI::WR+MI::WP+MI::BN, 31891);
    add(MI::WR+MI::WP+MI::BP, 31794);
    add(MI::WB*2+MI::BQ, 31837);
    add(MI::WB*2+MI::BR, 31938);
    add(MI::WB*2+MI::BB, 31955);
    add(MI::WB*2+MI::BN, 31843);
    add(MI::WB*2+MI::BP, 31834);
    add(MI::WB+MI::WN+MI::BQ, 31893);
    add(MI::WB+MI::WN+MI::BR, 31918);
    add(MI::WB+MI::WN+MI::BB, 31921);
    add(MI::WB+MI::WN+MI::BN, 31786);
    add(MI::WB+MI::WN+MI::BP, 31791);
    add(MI::WB+MI::WP+MI::BQ, 31899);
    add(MI::WB+MI::WP+MI::BR, 31910);
    add(MI::WB+MI::WP+MI::BB, 31898);
    add(MI::WB+MI::WP+MI::BN, 31800);
    add(MI::WB+MI::WP+MI::BP, 31865);
    add(MI::WN*2+MI::BQ, 31855);
    add(MI::WN*2+MI::BR, 31918);
    add(MI::WN*2+MI::BB, 31992);
    add(MI::WN*2+MI::BN, 31986);
    add(MI::WN*2+MI::BP, 31770);
    add(MI::WN+MI::WP+MI::BQ, 31875);
    add(MI::WN+MI::WP+MI::BR, 31866);
    add(MI::WN+MI::WP+MI::BB, 31914);
    add(MI::WN+MI::WP+MI::BN, 31805);
    add(MI::WN+MI::WP+MI::BP, 31884);
    add(MI::WP*2+MI::BQ, 31752);
    add(MI::WP*2+MI::BR, 31892);
    add(MI::WP*2+MI::BB, 31913);
    add(MI::WP*2+MI::BN, 31899);
    add(MI::WP*2+MI::BP, 31745);
}

void
TBProbe::initMaxDTZ() {
    using MI = MatId;
    auto add = [](int id, int value) {
        maxDTZ[id] = value;
        maxDTZ[MatId::mirror(id)] = value;
    };

    // 2-men
    add(0, -1);

    // 3-men
    add(MI::WQ, 20);
    add(MI::WR, 32);
    add(MI::WB, -1);
    add(MI::WN, -1);
    add(MI::WP, 20);

    // 4-men
    add(MI::WQ+MI::BQ, 19);
    add(MI::WN*2, 1);
    add(MI::WQ*2, 6);
    add(MI::WP*2, 14);
    add(MI::WR*2, 10);
    add(MI::WR+MI::BR, 7);
    add(MI::WQ+MI::WB, 12);
    add(MI::WQ+MI::WR, 8);
    add(MI::WQ+MI::WN, 14);
    add(MI::WR+MI::BB, 35);
    add(MI::WB+MI::BB, 1);
    add(MI::WQ+MI::WP, 6);
    add(MI::WB*2, 37);
    add(MI::WB+MI::BN, 2);
    add(MI::WR+MI::WP, 6);
    add(MI::WN+MI::BN, 1);
    add(MI::WR+MI::BN, 53);
    add(MI::WP+MI::BP, 21);
    add(MI::WB+MI::BP, 7);
    add(MI::WR+MI::WB, 24);
    add(MI::WQ+MI::BN, 38);
    add(MI::WR+MI::WN, 24);
    add(MI::WB+MI::WP, 26);
    add(MI::WN+MI::BP, 16);
    add(MI::WN+MI::WP, 26);
    add(MI::WQ+MI::BR, 62);
    add(MI::WQ+MI::BB, 24);
    add(MI::WR+MI::BP, 25);
    add(MI::WQ+MI::BP, 52);
    add(MI::WB+MI::WN, 65);

    // 5-men
    add(MI::WQ*3, 6);
    add(MI::WQ*2+MI::WR, 6);
    add(MI::WR*3, 8);
    add(MI::WQ*2+MI::WB, 6);
    add(MI::WQ*2+MI::WN, 8);
    add(MI::WQ*2+MI::WP, 6);
    add(MI::WQ+MI::WR+MI::WN, 8);
    add(MI::WQ+MI::WR*2, 8);
    add(MI::WQ+MI::WR+MI::WB, 8);
    add(MI::WQ+MI::WP*2, 6);
    add(MI::WQ+MI::WB+MI::WN, 8);
    add(MI::WR*2+MI::WP, 6);
    add(MI::WQ+MI::WB*2, 12);
    add(MI::WB*3, 20);
    add(MI::WR*2+MI::WN, 10);
    add(MI::WR*2+MI::WB, 10);
    add(MI::WQ+MI::WR+MI::WP, 6);
    add(MI::WQ+MI::WN*2, 14);
    add(MI::WQ+MI::WB+MI::WP, 6);
    add(MI::WQ+MI::WN+MI::WP, 6);
    add(MI::WR+MI::WP*2, 6);
    add(MI::WR+MI::WB*2, 20);
    add(MI::WP*3, 14);
    add(MI::WR+MI::WN*2, 20);
    add(MI::WQ*2+MI::BQ, 50);
    add(MI::WQ*2+MI::BN, 8);
    add(MI::WQ*2+MI::BB, 8);
    add(MI::WR+MI::WB+MI::WN, 14);
    add(MI::WB+MI::WP*2, 18);
    add(MI::WB*2+MI::WP, 24);
    add(MI::WQ*2+MI::BR, 28);
    add(MI::WB*2+MI::WN, 26);
    add(MI::WN+MI::WP*2, 12);
    add(MI::WQ+MI::WB+MI::BQ, 59);
    add(MI::WB+MI::WN*2, 26);
    add(MI::WN*2+MI::WP, 16);
    add(MI::WQ*2+MI::BP, 6);
    add(MI::WN*3, 41);
    add(MI::WQ+MI::WN+MI::BQ, 69);
    add(MI::WQ+MI::WR+MI::BQ, 100);
    add(MI::WQ+MI::WR+MI::BN, 10);
    add(MI::WQ+MI::WR+MI::BB, 10);
    add(MI::WQ+MI::WR+MI::BR, 30);
    add(MI::WR+MI::WB+MI::WP, 8);
    add(MI::WQ+MI::WB+MI::BN, 14);
    add(MI::WQ+MI::WB+MI::BR, 38);
    add(MI::WQ+MI::WB+MI::BB, 16);
    add(MI::WB+MI::WN+MI::WP, 10);
    add(MI::WR+MI::WN+MI::WP, 8);
    add(MI::WR*2+MI::BQ, 40);
    add(MI::WQ+MI::WN+MI::BN, 18);
    add(MI::WR+MI::WB+MI::BR, 100);
    add(MI::WQ+MI::WN+MI::BB, 18);
    add(MI::WQ+MI::WR+MI::BP, 6);
    add(MI::WR+MI::WB+MI::BQ, 82);
    add(MI::WQ+MI::WP+MI::BQ, 100);
    add(MI::WQ+MI::WP+MI::BP, 10);
    add(MI::WQ+MI::WB+MI::BP, 22);
    add(MI::WR+MI::WN+MI::BR, 64);
    add(MI::WR*2+MI::BN, 14);
    add(MI::WR*2+MI::BP, 18);
    add(MI::WQ+MI::WN+MI::BR, 44);
    add(MI::WR+MI::WN+MI::BQ, 92);
    add(MI::WR*2+MI::BB, 20);
    add(MI::WQ+MI::WN+MI::BP, 34);
    add(MI::WR*2+MI::BR, 50);
    add(MI::WB*2+MI::BR, 16);
    add(MI::WB*2+MI::BB, 11);
    add(MI::WQ+MI::WP+MI::BN, 12);
    add(MI::WR+MI::WB+MI::BN, 42);
    add(MI::WQ+MI::WP+MI::BB, 10);
    add(MI::WB+MI::WN+MI::BR, 24);
    add(MI::WB+MI::WN+MI::BB, 24);
    add(MI::WB*2+MI::BN, 100);
    add(MI::WB+MI::WN+MI::BN, 100);
    add(MI::WQ+MI::WP+MI::BR, 34);
    add(MI::WR+MI::WP+MI::BP, 19);
    add(MI::WR+MI::WP+MI::BR, 70);
    add(MI::WR+MI::WB+MI::BB, 50);
    add(MI::WB*2+MI::BP, 42);
    add(MI::WB*2+MI::BQ, 100);
    add(MI::WR+MI::WB+MI::BP, 22);
    add(MI::WN*2+MI::BR, 20);
    add(MI::WN*2+MI::BB, 6);
    add(MI::WB+MI::WP+MI::BR, 36);
    add(MI::WN*2+MI::BN, 12);
    add(MI::WB+MI::WP+MI::BB, 50);
    add(MI::WR+MI::WN+MI::BN, 48);
    add(MI::WN+MI::WP+MI::BR, 78);
    add(MI::WN*2+MI::BQ, 100);
    add(MI::WR+MI::WN+MI::BB, 50);
    add(MI::WR+MI::WN+MI::BP, 29);
    add(MI::WB+MI::WP+MI::BN, 60);
    add(MI::WB+MI::WN+MI::BQ, 84);
    add(MI::WB+MI::WP+MI::BP, 74);
    add(MI::WN*2+MI::BP, 100);
    add(MI::WN+MI::WP+MI::BB, 48);
    add(MI::WP*2+MI::BB, 24);
    add(MI::WP*2+MI::BQ, 58);
    add(MI::WP*2+MI::BP, 42);
    add(MI::WP*2+MI::BN, 27);
    add(MI::WP*2+MI::BR, 30);
    add(MI::WN+MI::WP+MI::BN, 59);
    add(MI::WN+MI::WP+MI::BP, 46);
    add(MI::WR+MI::WP+MI::BN, 62);
    add(MI::WR+MI::WP+MI::BB, 100);
    add(MI::WN+MI::WP+MI::BQ, 86);
    add(MI::WB+MI::WN+MI::BP, 40);
    add(MI::WR+MI::WP+MI::BQ, 100);
    add(MI::WB+MI::WP+MI::BQ, 84);

    // 6-men
    add(MI::WB*4, 20);
    add(MI::WB*3+MI::BB, 40);
    add(MI::WB*3+MI::BN, 28);
    add(MI::WB*3+MI::BP, 24);
    add(MI::WB*3+MI::BQ, 100);
    add(MI::WB*3+MI::BR, 100);
    add(MI::WB*3+MI::WN, 26);
    add(MI::WB*3+MI::WP, 24);
    add(MI::WB*2+MI::BB*2, 11);
    add(MI::WB*2+MI::BB+MI::BN, 40);
    add(MI::WB*2+MI::BB+MI::BP, 69);
    add(MI::WB*2+MI::BN*2, 56);
    add(MI::WB*2+MI::BN+MI::BP, 100);
    add(MI::WB*2+MI::BP*2, 39);
    add(MI::WB*2+MI::WN+MI::BB, 72);
    add(MI::WB*2+MI::WN+MI::BN, 62);
    add(MI::WB*2+MI::WN+MI::BP, 32);
    add(MI::WB*2+MI::WN+MI::BQ, 100);
    add(MI::WB*2+MI::WN+MI::BR, 100);
    add(MI::WB*2+MI::WN*2, 20);
    add(MI::WB*2+MI::WN+MI::WP, 10);
    add(MI::WB*2+MI::WP+MI::BB, 56);
    add(MI::WB*2+MI::WP+MI::BN, 100);
    add(MI::WB*2+MI::WP+MI::BP, 29);
    add(MI::WB*2+MI::WP+MI::BQ, 100);
    add(MI::WB*2+MI::WP+MI::BR, 100);
    add(MI::WB*2+MI::WP*2, 12);
    add(MI::WB+MI::WN+MI::BB+MI::BN, 17);
    add(MI::WB+MI::WN+MI::BB+MI::BP, 56);
    add(MI::WB+MI::WN+MI::BN*2, 24);
    add(MI::WB+MI::WN+MI::BN+MI::BP, 98);
    add(MI::WB+MI::WN+MI::BP*2, 48);
    add(MI::WB+MI::WN*2+MI::BB, 76);
    add(MI::WB+MI::WN*2+MI::BN, 58);
    add(MI::WB+MI::WN*2+MI::BP, 33);
    add(MI::WB+MI::WN*2+MI::BQ, 98);
    add(MI::WB+MI::WN*2+MI::BR, 96);
    add(MI::WB+MI::WN*3, 20);
    add(MI::WB+MI::WN*2+MI::WP, 10);
    add(MI::WB+MI::WN+MI::WP+MI::BB, 86);
    add(MI::WB+MI::WN+MI::WP+MI::BN, 77);
    add(MI::WB+MI::WN+MI::WP+MI::BP, 21);
    add(MI::WB+MI::WN+MI::WP+MI::BQ, 100);
    add(MI::WB+MI::WN+MI::WP+MI::BR, 100);
    add(MI::WB+MI::WN+MI::WP*2, 10);
    add(MI::WB+MI::WP+MI::BB+MI::BP, 65);
    add(MI::WB+MI::WP+MI::BN*2, 48);
    add(MI::WB+MI::WP+MI::BN+MI::BP, 62);
    add(MI::WB+MI::WP+MI::BP*2, 75);
    add(MI::WB+MI::WP*2+MI::BB, 86);
    add(MI::WB+MI::WP*2+MI::BN, 100);
    add(MI::WB+MI::WP*2+MI::BP, 61);
    add(MI::WB+MI::WP*2+MI::BQ, 78);
    add(MI::WB+MI::WP*2+MI::BR, 66);
    add(MI::WB+MI::WP*3, 18);
    add(MI::WN*2+MI::BN*2, 13);
    add(MI::WN*2+MI::BN+MI::BP, 56);
    add(MI::WN*2+MI::BP*2, 100);
    add(MI::WN*3+MI::BB, 100);
    add(MI::WN*3+MI::BN, 100);
    add(MI::WN*3+MI::BP, 41);
    add(MI::WN*3+MI::BQ, 70);
    add(MI::WN*3+MI::BR, 22);
    add(MI::WN*4, 22);
    add(MI::WN*3+MI::WP, 12);
    add(MI::WN*2+MI::WP+MI::BB, 100);
    add(MI::WN*2+MI::WP+MI::BN, 100);
    add(MI::WN*2+MI::WP+MI::BP, 33);
    add(MI::WN*2+MI::WP+MI::BQ, 100);
    add(MI::WN*2+MI::WP+MI::BR, 91);
    add(MI::WN*2+MI::WP*2, 12);
    add(MI::WN+MI::WP+MI::BN+MI::BP, 57);
    add(MI::WN+MI::WP+MI::BP*2, 66);
    add(MI::WN+MI::WP*2+MI::BB, 97);
    add(MI::WN+MI::WP*2+MI::BN, 96);
    add(MI::WN+MI::WP*2+MI::BP, 40);
    add(MI::WN+MI::WP*2+MI::BQ, 78);
    add(MI::WN+MI::WP*2+MI::BR, 81);
    add(MI::WN+MI::WP*3, 10);
    add(MI::WP*2+MI::BP*2, 31);
    add(MI::WP*3+MI::BB, 36);
    add(MI::WP*3+MI::BN, 42);
    add(MI::WP*3+MI::BP, 40);
    add(MI::WP*3+MI::BQ, 65);
    add(MI::WP*3+MI::BR, 44);
    add(MI::WP*4, 14);
    add(MI::WQ+MI::WB*3, 12);
    add(MI::WQ+MI::WB*2+MI::BB, 16);
    add(MI::WQ+MI::WB*2+MI::BN, 14);
    add(MI::WQ+MI::WB*2+MI::BP, 10);
    add(MI::WQ+MI::WB*2+MI::BQ, 100);
    add(MI::WQ+MI::WB*2+MI::BR, 40);
    add(MI::WQ+MI::WB*2+MI::WN, 10);
    add(MI::WQ+MI::WB*2+MI::WP, 6);
    add(MI::WQ+MI::WB+MI::BB*2, 26);
    add(MI::WQ+MI::WB+MI::BB+MI::BN, 32);
    add(MI::WQ+MI::WB+MI::BB+MI::BP, 44);
    add(MI::WQ+MI::WB+MI::BN*2, 26);
    add(MI::WQ+MI::WB+MI::BN+MI::BP, 53);
    add(MI::WQ+MI::WB+MI::BP*2, 34);
    add(MI::WQ+MI::WB+MI::BQ+MI::BB, 91);
    add(MI::WQ+MI::WB+MI::BQ+MI::BN, 72);
    add(MI::WQ+MI::WB+MI::BQ+MI::BP, 100);
    add(MI::WQ+MI::WB+MI::BR+MI::BB, 83);
    add(MI::WQ+MI::WB+MI::BR+MI::BN, 54);
    add(MI::WQ+MI::WB+MI::BR+MI::BP, 77);
    add(MI::WQ+MI::WB+MI::BR*2, 100);
    add(MI::WQ+MI::WB+MI::WN+MI::BB, 14);
    add(MI::WQ+MI::WB+MI::WN+MI::BN, 12);
    add(MI::WQ+MI::WB+MI::WN+MI::BP, 8);
    add(MI::WQ+MI::WB+MI::WN+MI::BQ, 100);
    add(MI::WQ+MI::WB+MI::WN+MI::BR, 44);
    add(MI::WQ+MI::WB+MI::WN*2, 10);
    add(MI::WQ+MI::WB+MI::WN+MI::WP, 6);
    add(MI::WQ+MI::WB+MI::WP+MI::BB, 12);
    add(MI::WQ+MI::WB+MI::WP+MI::BN, 12);
    add(MI::WQ+MI::WB+MI::WP+MI::BP, 8);
    add(MI::WQ+MI::WB+MI::WP+MI::BQ, 100);
    add(MI::WQ+MI::WB+MI::WP+MI::BR, 62);
    add(MI::WQ+MI::WB+MI::WP*2, 8);
    add(MI::WQ+MI::WN+MI::BB*2, 30);
    add(MI::WQ+MI::WN+MI::BB+MI::BN, 34);
    add(MI::WQ+MI::WN+MI::BB+MI::BP, 67);
    add(MI::WQ+MI::WN+MI::BN*2, 32);
    add(MI::WQ+MI::WN+MI::BN+MI::BP, 62);
    add(MI::WQ+MI::WN+MI::BP*2, 44);
    add(MI::WQ+MI::WN+MI::BQ+MI::BN, 57);
    add(MI::WQ+MI::WN+MI::BQ+MI::BP, 100);
    add(MI::WQ+MI::WN+MI::BR+MI::BB, 52);
    add(MI::WQ+MI::WN+MI::BR+MI::BN, 80);
    add(MI::WQ+MI::WN+MI::BR+MI::BP, 83);
    add(MI::WQ+MI::WN+MI::BR*2, 100);
    add(MI::WQ+MI::WN*2+MI::BB, 22);
    add(MI::WQ+MI::WN*2+MI::BN, 18);
    add(MI::WQ+MI::WN*2+MI::BP, 20);
    add(MI::WQ+MI::WN*2+MI::BQ, 100);
    add(MI::WQ+MI::WN*2+MI::BR, 44);
    add(MI::WQ+MI::WN*3, 10);
    add(MI::WQ+MI::WN*2+MI::WP, 6);
    add(MI::WQ+MI::WN+MI::WP+MI::BB, 12);
    add(MI::WQ+MI::WN+MI::WP+MI::BN, 12);
    add(MI::WQ+MI::WN+MI::WP+MI::BP, 12);
    add(MI::WQ+MI::WN+MI::WP+MI::BQ, 100);
    add(MI::WQ+MI::WN+MI::WP+MI::BR, 42);
    add(MI::WQ+MI::WN+MI::WP*2, 10);
    add(MI::WQ+MI::WP+MI::BB*2, 44);
    add(MI::WQ+MI::WP+MI::BB+MI::BN, 36);
    add(MI::WQ+MI::WP+MI::BB+MI::BP, 99);
    add(MI::WQ+MI::WP+MI::BN*2, 92);
    add(MI::WQ+MI::WP+MI::BN+MI::BP, 54);
    add(MI::WQ+MI::WP+MI::BP*2, 35);
    add(MI::WQ+MI::WP+MI::BQ+MI::BP, 100);
    add(MI::WQ+MI::WP+MI::BR+MI::BB, 100);
    add(MI::WQ+MI::WP+MI::BR+MI::BN, 100);
    add(MI::WQ+MI::WP+MI::BR+MI::BP, 100);
    add(MI::WQ+MI::WP+MI::BR*2, 100);
    add(MI::WQ+MI::WP*2+MI::BB, 12);
    add(MI::WQ+MI::WP*2+MI::BN, 12);
    add(MI::WQ+MI::WP*2+MI::BP, 10);
    add(MI::WQ+MI::WP*2+MI::BQ, 100);
    add(MI::WQ+MI::WP*2+MI::BR, 42);
    add(MI::WQ+MI::WP*3, 6);
    add(MI::WQ*2+MI::WB*2, 6);
    add(MI::WQ*2+MI::WB+MI::BB, 10);
    add(MI::WQ*2+MI::WB+MI::BN, 10);
    add(MI::WQ*2+MI::WB+MI::BP, 6);
    add(MI::WQ*2+MI::WB+MI::BQ, 58);
    add(MI::WQ*2+MI::WB+MI::BR, 52);
    add(MI::WQ*2+MI::WB+MI::WN, 8);
    add(MI::WQ*2+MI::WB+MI::WP, 6);
    add(MI::WQ*2+MI::BB*2, 16);
    add(MI::WQ*2+MI::BB+MI::BN, 16);
    add(MI::WQ*2+MI::BB+MI::BP, 12);
    add(MI::WQ*2+MI::BN*2, 14);
    add(MI::WQ*2+MI::BN+MI::BP, 11);
    add(MI::WQ*2+MI::BP*2, 6);
    add(MI::WQ*2+MI::BQ+MI::BB, 100);
    add(MI::WQ*2+MI::BQ+MI::BN, 100);
    add(MI::WQ*2+MI::BQ+MI::BP, 79);
    add(MI::WQ*2+MI::BQ*2, 87);
    add(MI::WQ*2+MI::BQ+MI::BR, 100);
    add(MI::WQ*2+MI::BR+MI::BB, 27);
    add(MI::WQ*2+MI::BR+MI::BN, 28);
    add(MI::WQ*2+MI::BR+MI::BP, 38);
    add(MI::WQ*2+MI::BR*2, 36);
    add(MI::WQ*2+MI::WN+MI::BB, 8);
    add(MI::WQ*2+MI::WN+MI::BN, 10);
    add(MI::WQ*2+MI::WN+MI::BP, 6);
    add(MI::WQ*2+MI::WN+MI::BQ, 56);
    add(MI::WQ*2+MI::WN+MI::BR, 48);
    add(MI::WQ*2+MI::WN*2, 8);
    add(MI::WQ*2+MI::WN+MI::WP, 6);
    add(MI::WQ*2+MI::WP+MI::BB, 8);
    add(MI::WQ*2+MI::WP+MI::BN, 10);
    add(MI::WQ*2+MI::WP+MI::BP, 6);
    add(MI::WQ*2+MI::WP+MI::BQ, 70);
    add(MI::WQ*2+MI::WP+MI::BR, 48);
    add(MI::WQ*2+MI::WP*2, 6);
    add(MI::WQ*3+MI::WB, 6);
    add(MI::WQ*3+MI::BB, 6);
    add(MI::WQ*3+MI::BN, 8);
    add(MI::WQ*3+MI::BP, 6);
    add(MI::WQ*3+MI::BQ, 38);
    add(MI::WQ*3+MI::BR, 40);
    add(MI::WQ*3+MI::WN, 6);
    add(MI::WQ*3+MI::WP, 6);
    add(MI::WQ*4, 6);
    add(MI::WQ*3+MI::WR, 6);
    add(MI::WQ*2+MI::WR+MI::WB, 6);
    add(MI::WQ*2+MI::WR+MI::BB, 8);
    add(MI::WQ*2+MI::WR+MI::BN, 10);
    add(MI::WQ*2+MI::WR+MI::BP, 6);
    add(MI::WQ*2+MI::WR+MI::BQ, 56);
    add(MI::WQ*2+MI::WR+MI::BR, 48);
    add(MI::WQ*2+MI::WR+MI::WN, 8);
    add(MI::WQ*2+MI::WR+MI::WP, 6);
    add(MI::WQ*2+MI::WR*2, 6);
    add(MI::WQ+MI::WR+MI::WB*2, 8);
    add(MI::WQ+MI::WR+MI::WB+MI::BB, 10);
    add(MI::WQ+MI::WR+MI::WB+MI::BN, 10);
    add(MI::WQ+MI::WR+MI::WB+MI::BP, 6);
    add(MI::WQ+MI::WR+MI::WB+MI::BQ, 98);
    add(MI::WQ+MI::WR+MI::WB+MI::BR, 50);
    add(MI::WQ+MI::WR+MI::WB+MI::WN, 8);
    add(MI::WQ+MI::WR+MI::WB+MI::WP, 8);
    add(MI::WQ+MI::WR+MI::BB*2, 24);
    add(MI::WQ+MI::WR+MI::BB+MI::BN, 22);
    add(MI::WQ+MI::WR+MI::BB+MI::BP, 28);
    add(MI::WQ+MI::WR+MI::BN*2, 21);
    add(MI::WQ+MI::WR+MI::BN+MI::BP, 26);
    add(MI::WQ+MI::WR+MI::BP*2, 12);
    add(MI::WQ+MI::WR+MI::BQ+MI::BB, 100);
    add(MI::WQ+MI::WR+MI::BQ+MI::BN, 100);
    add(MI::WQ+MI::WR+MI::BQ+MI::BP, 100);
    add(MI::WQ+MI::WR+MI::BQ+MI::BR, 100);
    add(MI::WQ+MI::WR+MI::BR+MI::BB, 42);
    add(MI::WQ+MI::WR+MI::BR+MI::BN, 42);
    add(MI::WQ+MI::WR+MI::BR+MI::BP, 44);
    add(MI::WQ+MI::WR+MI::BR*2, 68);
    add(MI::WQ+MI::WR+MI::WN+MI::BB, 8);
    add(MI::WQ+MI::WR+MI::WN+MI::BN, 12);
    add(MI::WQ+MI::WR+MI::WN+MI::BP, 7);
    add(MI::WQ+MI::WR+MI::WN+MI::BQ, 100);
    add(MI::WQ+MI::WR+MI::WN+MI::BR, 48);
    add(MI::WQ+MI::WR+MI::WN*2, 8);
    add(MI::WQ+MI::WR+MI::WN+MI::WP, 8);
    add(MI::WQ+MI::WR+MI::WP+MI::BB, 8);
    add(MI::WQ+MI::WR+MI::WP+MI::BN, 10);
    add(MI::WQ+MI::WR+MI::WP+MI::BP, 7);
    add(MI::WQ+MI::WR+MI::WP+MI::BQ, 100);
    add(MI::WQ+MI::WR+MI::WP+MI::BR, 60);
    add(MI::WQ+MI::WR+MI::WP*2, 6);
    add(MI::WQ+MI::WR*2+MI::WB, 8);
    add(MI::WQ+MI::WR*2+MI::BB, 8);
    add(MI::WQ+MI::WR*2+MI::BN, 10);
    add(MI::WQ+MI::WR*2+MI::BP, 6);
    add(MI::WQ+MI::WR*2+MI::BQ, 82);
    add(MI::WQ+MI::WR*2+MI::BR, 46);
    add(MI::WQ+MI::WR*2+MI::WN, 8);
    add(MI::WQ+MI::WR*2+MI::WP, 6);
    add(MI::WQ+MI::WR*3, 8);
    add(MI::WR+MI::WB*3, 20);
    add(MI::WR+MI::WB*2+MI::BB, 36);
    add(MI::WR+MI::WB*2+MI::BN, 23);
    add(MI::WR+MI::WB*2+MI::BP, 24);
    add(MI::WR+MI::WB*2+MI::BQ, 88);
    add(MI::WR+MI::WB*2+MI::BR, 71);
    add(MI::WR+MI::WB*2+MI::WN, 14);
    add(MI::WR+MI::WB*2+MI::WP, 10);
    add(MI::WR+MI::WB+MI::BB*2, 100);
    add(MI::WR+MI::WB+MI::BB+MI::BN, 100);
    add(MI::WR+MI::WB+MI::BB+MI::BP, 76);
    add(MI::WR+MI::WB+MI::BN*2, 100);
    add(MI::WR+MI::WB+MI::BN+MI::BP, 90);
    add(MI::WR+MI::WB+MI::BP*2, 47);
    add(MI::WR+MI::WB+MI::BR+MI::BB, 33);
    add(MI::WR+MI::WB+MI::BR+MI::BN, 40);
    add(MI::WR+MI::WB+MI::BR+MI::BP, 94);
    add(MI::WR+MI::WB+MI::WN+MI::BB, 26);
    add(MI::WR+MI::WB+MI::WN+MI::BN, 24);
    add(MI::WR+MI::WB+MI::WN+MI::BP, 31);
    add(MI::WR+MI::WB+MI::WN+MI::BQ, 100);
    add(MI::WR+MI::WB+MI::WN+MI::BR, 72);
    add(MI::WR+MI::WB+MI::WN*2, 14);
    add(MI::WR+MI::WB+MI::WN+MI::WP, 10);
    add(MI::WR+MI::WB+MI::WP+MI::BB, 20);
    add(MI::WR+MI::WB+MI::WP+MI::BN, 20);
    add(MI::WR+MI::WB+MI::WP+MI::BP, 21);
    add(MI::WR+MI::WB+MI::WP+MI::BQ, 100);
    add(MI::WR+MI::WB+MI::WP+MI::BR, 100);
    add(MI::WR+MI::WB+MI::WP*2, 8);
    add(MI::WR+MI::WN+MI::BB*2, 100);
    add(MI::WR+MI::WN+MI::BB+MI::BN, 100);
    add(MI::WR+MI::WN+MI::BB+MI::BP, 100);
    add(MI::WR+MI::WN+MI::BN*2, 100);
    add(MI::WR+MI::WN+MI::BN+MI::BP, 100);
    add(MI::WR+MI::WN+MI::BP*2, 48);
    add(MI::WR+MI::WN+MI::BR+MI::BN, 41);
    add(MI::WR+MI::WN+MI::BR+MI::BP, 72);
    add(MI::WR+MI::WN*2+MI::BB, 24);
    add(MI::WR+MI::WN*2+MI::BN, 25);
    add(MI::WR+MI::WN*2+MI::BP, 30);
    add(MI::WR+MI::WN*2+MI::BQ, 81);
    add(MI::WR+MI::WN*2+MI::BR, 78);
    add(MI::WR+MI::WN*3, 14);
    add(MI::WR+MI::WN*2+MI::WP, 8);
    add(MI::WR+MI::WN+MI::WP+MI::BB, 26);
    add(MI::WR+MI::WN+MI::WP+MI::BN, 20);
    add(MI::WR+MI::WN+MI::WP+MI::BP, 27);
    add(MI::WR+MI::WN+MI::WP+MI::BQ, 100);
    add(MI::WR+MI::WN+MI::WP+MI::BR, 100);
    add(MI::WR+MI::WN+MI::WP*2, 10);
    add(MI::WR+MI::WP+MI::BB*2, 79);
    add(MI::WR+MI::WP+MI::BB+MI::BN, 100);
    add(MI::WR+MI::WP+MI::BB+MI::BP, 100);
    add(MI::WR+MI::WP+MI::BN*2, 84);
    add(MI::WR+MI::WP+MI::BN+MI::BP, 100);
    add(MI::WR+MI::WP+MI::BP*2, 31);
    add(MI::WR+MI::WP+MI::BR+MI::BP, 73);
    add(MI::WR+MI::WP*2+MI::BB, 36);
    add(MI::WR+MI::WP*2+MI::BN, 36);
    add(MI::WR+MI::WP*2+MI::BP, 26);
    add(MI::WR+MI::WP*2+MI::BQ, 100);
    add(MI::WR+MI::WP*2+MI::BR, 90);
    add(MI::WR+MI::WP*3, 6);
    add(MI::WR*2+MI::WB*2, 12);
    add(MI::WR*2+MI::WB+MI::BB, 14);
    add(MI::WR*2+MI::WB+MI::BN, 12);
    add(MI::WR*2+MI::WB+MI::BP, 8);
    add(MI::WR*2+MI::WB+MI::BQ, 100);
    add(MI::WR*2+MI::WB+MI::BR, 62);
    add(MI::WR*2+MI::WB+MI::WN, 12);
    add(MI::WR*2+MI::WB+MI::WP, 8);
    add(MI::WR*2+MI::BB*2, 74);
    add(MI::WR*2+MI::BB+MI::BN, 51);
    add(MI::WR*2+MI::BB+MI::BP, 52);
    add(MI::WR*2+MI::BN*2, 66);
    add(MI::WR*2+MI::BN+MI::BP, 50);
    add(MI::WR*2+MI::BP*2, 50);
    add(MI::WR*2+MI::BR+MI::BB, 100);
    add(MI::WR*2+MI::BR+MI::BN, 100);
    add(MI::WR*2+MI::BR+MI::BP, 100);
    add(MI::WR*2+MI::BR*2, 35);
    add(MI::WR*2+MI::WN+MI::BB, 14);
    add(MI::WR*2+MI::WN+MI::BN, 14);
    add(MI::WR*2+MI::WN+MI::BP, 18);
    add(MI::WR*2+MI::WN+MI::BQ, 100);
    add(MI::WR*2+MI::WN+MI::BR, 66);
    add(MI::WR*2+MI::WN*2, 12);
    add(MI::WR*2+MI::WN+MI::WP, 8);
    add(MI::WR*2+MI::WP+MI::BB, 14);
    add(MI::WR*2+MI::WP+MI::BN, 12);
    add(MI::WR*2+MI::WP+MI::BP, 22);
    add(MI::WR*2+MI::WP+MI::BQ, 100);
    add(MI::WR*2+MI::WP+MI::BR, 56);
    add(MI::WR*2+MI::WP*2, 6);
    add(MI::WR*3+MI::WB, 10);
    add(MI::WR*3+MI::BB, 10);
    add(MI::WR*3+MI::BN, 12);
    add(MI::WR*3+MI::BP, 6);
    add(MI::WR*3+MI::BQ, 100);
    add(MI::WR*3+MI::BR, 42);
    add(MI::WR*3+MI::WN, 10);
    add(MI::WR*3+MI::WP, 8);
    add(MI::WR*4, 8);
}
