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
 * tbTest.cpp
 *
 *  Created on: Jun 2, 2014
 *      Author: petero
 */

#include "tbTest.hpp"
#include "evaluateTest.hpp"
#include "searchTest.hpp"
#include "position.hpp"
#include "moveGen.hpp"
#include "evaluate.hpp"
#include "textio.hpp"
#include "tbprobe.hpp"
#include "constants.hpp"

#include "syzygy/rtb-probe.hpp"

#include "cute.h"

void
TBTest::initTB(const std::string& gtbPath, int cacheMB,
               const std::string& rtbPath) {
    TBProbe::initialize(gtbPath, cacheMB, rtbPath);
}

/** Copy selected TB files to a temporary directory. */
static void setupTBFiles(const std::vector<std::string>& tbFiles) {
    auto system = [](const std::string& cmd) {
        ::system(cmd.c_str());
    };
    std::string tmpDir = "/tmp/tbtest";
    system("mkdir -p " + tmpDir);
    system("rm " + tmpDir + "/*");
    for (const std::string& file : tbFiles) {
        if (endsWith(file, ".gtb.cp4")) {
            system("ln -s /home/petero/chess/gtb/" + file + " " + tmpDir + "/" + file);
        } else if (endsWith(file, ".rtbw")) {
            system("ln -s /home/petero/chess/rtb/wdl/" + file + " " + tmpDir + "/" + file);
        } else if (endsWith(file, ".rtbz")) {
            system("ln -s /home/petero/chess/rtb/dtz/" + file + " " + tmpDir + "/" + file);
        } else {
            throw "Unsupported file type";
        }
    }
    TBTest::initTB("", 0, "");
    TBTest::initTB(tmpDir, gtbDefaultCacheMB, tmpDir);
}

/** Probe both DTM and WDL, check consistency and return DTM value. */
static int probeCompare(const Position& pos, int ply, int& score) {
    int dtm, wdl, wdl2, dtz;
    TranspositionTable::TTEntry ent;
    Position pos2(pos);
    int resDTM = TBProbe::gtbProbeDTM(pos2, ply, dtm);
    ASSERT(pos.equals(pos2));
    int resWDL = TBProbe::gtbProbeWDL(pos2, ply, wdl);
    ASSERT(pos.equals(pos2));
    int resWDL2 = TBProbe::rtbProbeWDL(pos2, ply, wdl2, ent);
    ASSERT(pos.equals(pos2));
    int resDTZ = TBProbe::rtbProbeDTZ(pos2, ply, dtz, ent);
    ASSERT(pos.equals(pos2));

    ASSERT_EQUAL(resDTM, resWDL);
    ASSERT_EQUAL(resWDL, resWDL2);
    ASSERT_EQUAL(resWDL2, resDTZ);

    if (!resDTM)
        return false;

    if (dtm > 0) {
        ASSERT(wdl > 0);
        ASSERT(wdl <= dtm);
        ASSERT(wdl2 > 0);
        ASSERT(wdl2 <= dtm);
        ASSERT(dtz > 0);
        ASSERT(dtz <= dtm);
        ASSERT(dtz >= wdl2);
    } else if (dtm < 0) {
        ASSERT(wdl < 0);
        ASSERT(wdl >= dtm);
        ASSERT(wdl2 < 0);
        ASSERT(wdl2 >= dtm);
        ASSERT(dtz < 0);
        ASSERT(dtz >= dtm);
        ASSERT(dtz <= wdl2);
    } else {
        ASSERT_EQUAL(0, wdl);
        ASSERT_EQUAL(0, wdl2);
        ASSERT_EQUAL(0, dtz);
    }

    score = dtm;
    return true;
}

/** Probe a position and its mirror positions and verify they have the same score. */
static int
probeDTM(const Position& pos, int ply, int& score) {
    std::string fen = TextIO::toFEN(pos);
    int ret = probeCompare(pos, ply, score);
    Position symPos = swapColors(pos);
    int score2;
    int ret2 = probeCompare(symPos, ply, score2);
    std::string fen2 = TextIO::toFEN(symPos);
    ASSERT_EQUALM((fen + " == " + fen2).c_str(), ret, ret2);
    if (ret)
        ASSERT_EQUALM((fen + " == " + fen2).c_str(), score, score2);

    if (pos.getCastleMask() == 0) {
        symPos = mirrorX(pos);
        fen2 = TextIO::toFEN(symPos);
        ret2 = probeCompare(symPos, ply, score2);
        ASSERT_EQUALM((fen + " == " + fen2).c_str(), ret, ret2);
        if (ret)
            ASSERT_EQUALM((fen + " == " + fen2).c_str(), score, score2);

        symPos = swapColors(mirrorX(pos));
        fen2 = TextIO::toFEN(symPos);
        ret2 = probeCompare(symPos, ply, score2);
        ASSERT_EQUALM((fen + " == " + fen2).c_str(), ret, ret2);
        if (ret)
            ASSERT_EQUALM((fen + " == " + fen2).c_str(), score, score2);
    }

    return ret;
}

void
TBTest::dtmTest() {
    const int mate0 = SearchConst::MATE0;
    int ply = 17;
    const int cacheMB = gtbDefaultCacheMB;

    Position pos = TextIO::readFEN("4k3/R7/4K3/8/8/8/8/8 w - - 0 1");
    int score;
    bool res = probeDTM(pos, ply, score);
    ASSERT_EQUAL(true, res);
    ASSERT_EQUAL(mate0 - ply - 2, score);

    initTB("/home/petero/chess/gtb/no_such_dir", cacheMB, "");
    res = probeDTM(pos, ply, score);
    ASSERT_EQUAL(false, res);
    initTB("/no/such/path;" + gtbDefaultPath + ";/test/;", cacheMB,
           "//dfasf/:" + rtbDefaultPath + ":a:b:");

    // Test castling
    pos = TextIO::readFEN("4k3/8/8/8/8/8/8/4K2R w K - 0 1");
    res = probeDTM(pos, ply, score);
    ASSERT_EQUAL(false, res);
    pos = TextIO::readFEN("4k3/8/8/8/8/8/8/4K2R w - - 0 1");
    res = probeDTM(pos, ply, score);
    ASSERT_EQUAL(true, res);
    ASSERT_EQUAL(mate0 - ply - 22, score);

    initTB("", cacheMB, "");
    res = probeDTM(pos, ply, score);
    ASSERT_EQUAL(false, res);
    initTB(gtbDefaultPath, cacheMB, rtbDefaultPath);

    // Test en passant
    pos = TextIO::readFEN("8/8/4k3/8/3pP3/8/3P4/4K3 b - e3 0 1");
    res = probeDTM(pos, ply, score);
    ASSERT_EQUAL(true, res);
    ASSERT_EQUAL(0, score);
    pos = TextIO::readFEN("8/8/4k3/8/3pP3/8/3P4/4K3 b - - 0 1");
    res = probeDTM(pos, ply, score);
    ASSERT_EQUAL(true, res);
    ASSERT_EQUAL(-(mate0 - ply - 48 - 1), score);

    // Test where en passant is only legal move
    pos = TextIO::readFEN("8/8/8/8/Pp6/1K6/3N4/k7 b - a3 0 1");
    res = probeDTM(pos, ply, score);
    ASSERT_EQUAL(true, res);
    ASSERT_EQUAL(-(mate0 - ply - 13), score);
    pos = TextIO::readFEN("k1K5/8/8/8/4pP2/4Q3/8/8 b - - 0 1");
    res = probeDTM(pos, ply, score);
    ASSERT_EQUAL(true, res);
    ASSERT_EQUAL(0, score);
    pos = TextIO::readFEN("k1K5/8/8/8/4pP2/4Q3/8/8 b - f3 0 1");
    res = probeDTM(pos, ply, score);
    ASSERT_EQUAL(true, res);
    ASSERT_EQUAL(-(mate0 - ply - 3), score);
}

void
TBTest::kpkTest() {
    auto et = Evaluate::getEvalHashTables();
    Evaluate evaluate(*et);
    const int ply = 1;
    for (int p = A2; p <= H7; p++) {
        for (int wk = 0; wk < 64; wk++) {
            if (wk == p)
                continue;
            for (int bk = 0; bk < 64; bk++) {
                if (bk == wk || bk == p)
                    continue;
                for (int c = 0; c < 2; c++) {
                    Position pos;
                    pos.setPiece(p, Piece::WPAWN);
                    pos.setPiece(wk, Piece::WKING);
                    pos.setPiece(bk, Piece::BKING);
                    pos.setWhiteMove(c == 0);
                    if (MoveGen::canTakeKing(pos))
                        continue;
                    int score;
                    int res = probeDTM(pos, ply, score);
                    ASSERT_EQUAL(true, res);
                    if (pos.isWhiteMove()) {
                        ASSERT(score >= 0);
                    } else {
                        ASSERT(score <= 0);
                    }
                    int evalWhite = evaluate.evalPos(pos);
                    if (!pos.isWhiteMove())
                        evalWhite = -evalWhite;
                    if (score == 0) {
                        ASSERT(evalWhite == 0);
                    } else {
                        ASSERT(evalWhite > 0);
                    }
                }
            }
        }
    }
}

void
TBTest::rtbTest() {
    int ply = 17;
    int wdl, dtm, dtz;
    TranspositionTable::TTEntry ent;

    Position pos = TextIO::readFEN("8/8/4k3/8/8/8/4K3/3NB3 w - - 0 1");
    bool resWDL = TBProbe::rtbProbeWDL(pos, ply, wdl, ent);
    ASSERT_EQUAL(true, resWDL);
    ASSERT(SearchConst::isWinScore(wdl));

    pos = TextIO::readFEN("8/8/4k3/8/8/8/4K3/3NB3 b - - 0 1");
    resWDL = TBProbe::rtbProbeWDL(pos, ply, wdl, ent);
    ASSERT_EQUAL(true, resWDL);
    ASSERT(SearchConst::isLoseScore(wdl));

    pos = TextIO::readFEN("8/8/4k3/8/8/8/4K3/3BB3 b - - 0 1");
    resWDL = TBProbe::rtbProbeWDL(pos, ply, wdl, ent);
    ASSERT_EQUAL(true, resWDL);
    ASSERT(SearchConst::isLoseScore(wdl));

    pos = TextIO::readFEN("8/8/4k3/8/8/8/4K3/3NN3 b - - 0 1");
    ent.clear();
    resWDL = TBProbe::rtbProbeWDL(pos, ply, wdl, ent);
    ASSERT_EQUAL(true, resWDL);
    ASSERT_EQUAL(0, wdl);
    ASSERT_EQUAL(0, ent.getEvalScore());

    initTB(gtbDefaultPath, 16, "");
    initTB(gtbDefaultPath, 16, "");
    initTB(gtbDefaultPath, 16, rtbDefaultPath);

    pos = TextIO::readFEN("8/8/4k3/8/8/8/4K3/3NN3 b - - 0 1");
    ent.clear();
    resWDL = TBProbe::rtbProbeWDL(pos, ply, wdl, ent);
    ASSERT_EQUAL(true, resWDL);
    ASSERT_EQUAL(0, wdl);
    ASSERT_EQUAL(0, ent.getEvalScore());

    // Check that DTZ probes do not give too good (incorrect) bounds
    pos = TextIO::readFEN("8/8/8/8/7B/8/3k4/K2B4 w - - 0 1");
    bool resDTM = TBProbe::gtbProbeDTM(pos, ply, dtm);
    ASSERT(resDTM);
    bool resDTZ = TBProbe::rtbProbeDTZ(pos, ply, dtz, ent);
    ASSERT(resDTZ);
    ASSERT(SearchConst::isWinScore(dtz));
    ASSERT(dtz <= dtm);

    pos = TextIO::readFEN("1R5Q/8/6k1/8/4q3/8/8/K7 b - - 0 1");
    probeDTM(pos, ply, dtm);
    resWDL = TBProbe::rtbProbeWDL(pos, ply, wdl, ent);
    ASSERT(resWDL);
    resDTZ = TBProbe::rtbProbeDTZ(pos, ply, dtz, ent);
    ASSERT(resDTZ);
    ASSERT(SearchConst::isLoseScore(wdl));
    ASSERT(SearchConst::isLoseScore(dtz));
    ASSERT(dtz <= wdl);

    // Tests where DTZ is close to 100
    pos = TextIO::readFEN("1R5Q/8/6k1/8/4q3/8/8/K7 b - - 0 1"); // DTZ = 100
    resWDL = TBProbe::rtbProbeWDL(pos, ply, wdl, ent);
    ASSERT(resWDL);
    resDTZ = TBProbe::rtbProbeDTZ(pos, ply, dtz, ent);
    ASSERT(resDTZ);
    ASSERT(SearchConst::isLoseScore(wdl));
    ASSERT(SearchConst::isLoseScore(dtz));
    ASSERT(dtz <= wdl);

    pos = TextIO::readFEN("1R5Q/8/6k1/8/4q3/8/8/K7 b - - 1 1"); // DTZ = 100
    resWDL = TBProbe::rtbProbeWDL(pos, ply, wdl, ent);
    ASSERT(resWDL);
    ASSERT(SearchConst::isLoseScore(wdl)); // WDL probes assume half-move clock is 0
    ent.clear();
    resDTZ = TBProbe::rtbProbeDTZ(pos, ply, dtz, ent);
    ASSERT(resDTZ);
    ASSERT_EQUAL(0, dtz);
    ASSERT_EQUAL(-1, ent.getEvalScore());


    pos = TextIO::readFEN("7q/3N2k1/8/8/8/7Q/8/1K6 w - - 0 1"); // DTZ = 30
    resWDL = TBProbe::rtbProbeWDL(pos, ply, wdl, ent);
    ASSERT(resWDL);
    ASSERT(SearchConst::isWinScore(wdl));
    resDTZ = TBProbe::rtbProbeDTZ(pos, ply, dtz, ent);
    ASSERT(resDTZ);
    ASSERT(SearchConst::isWinScore(dtz));
    ASSERT(dtz >= wdl);

    pos = TextIO::readFEN("7q/3N2k1/8/8/8/7Q/8/1K6 w - - 69 1");
    resWDL = TBProbe::rtbProbeWDL(pos, ply, wdl, ent);
    ASSERT(resWDL);
    ASSERT(SearchConst::isWinScore(wdl));
    resDTZ = TBProbe::rtbProbeDTZ(pos, ply, dtz, ent);
    ASSERT(resDTZ);
    ASSERT(SearchConst::isWinScore(dtz));

    // DTZ = 30, DTZ + hmc = 100. RTB does not know the answer
    // because the TB class has maxDTZ < 100
    pos = TextIO::readFEN("7q/3N2k1/8/8/8/7Q/8/1K6 w - - 70 1");
    resWDL = TBProbe::rtbProbeWDL(pos, ply, wdl, ent);
    ASSERT(resWDL);
    ASSERT(SearchConst::isWinScore(wdl)); // WDL probes assume hmc is 0
    resDTZ = TBProbe::rtbProbeDTZ(pos, ply, dtz, ent);
    ASSERT(!resDTZ);

    // DTZ + hmc > 100, draw
    pos = TextIO::readFEN("7q/3N2k1/8/8/8/7Q/8/1K6 w - - 71 1");
    resWDL = TBProbe::rtbProbeWDL(pos, ply, wdl, ent);
    ASSERT(resWDL);
    ASSERT(SearchConst::isWinScore(wdl)); // WDL probes assume hmc is 0
    ent.clear();
    resDTZ = TBProbe::rtbProbeDTZ(pos, ply, dtz, ent);
    ASSERT(resDTZ);
    ASSERT_EQUAL(0, dtz);
    ASSERT_EQUAL(1, ent.getEvalScore());


    pos = TextIO::readFEN("8/1R6/4q3/6k1/8/8/6K1/1Q6 b - - 0 1"); // DTZ = 46
    resWDL = TBProbe::rtbProbeWDL(pos, ply, wdl, ent);
    ASSERT(resWDL);
    resDTZ = TBProbe::rtbProbeDTZ(pos, ply, dtz, ent);
    ASSERT(resDTZ);
    ASSERT(SearchConst::isLoseScore(wdl));
    ASSERT(SearchConst::isLoseScore(dtz));
    ASSERT(dtz <= wdl);

    // DTZ + hmc = 100, but RTB still knows the answer because maxDTZ = 100
    pos = TextIO::readFEN("8/1R6/4q3/6k1/8/8/6K1/1Q6 b - - 54 1"); // DTZ = 46
    resWDL = TBProbe::rtbProbeWDL(pos, ply, wdl, ent);
    ASSERT(resWDL);
    ASSERT(SearchConst::isLoseScore(wdl));
    resDTZ = TBProbe::rtbProbeDTZ(pos, ply, dtz, ent);
    ASSERT(resDTZ);
    ASSERT(SearchConst::isLoseScore(dtz));

    // DTZ + hmc = 101, draw
    pos = TextIO::readFEN("8/1R6/4q3/6k1/8/8/6K1/1Q6 b - - 55 1"); // DTZ = 46
    resWDL = TBProbe::rtbProbeWDL(pos, ply, wdl, ent);
    ASSERT(resWDL);
    ASSERT(SearchConst::isLoseScore(wdl));
    ent.clear();
    resDTZ = TBProbe::rtbProbeDTZ(pos, ply, dtz, ent);
    ASSERT(resDTZ);
    ASSERT_EQUAL(0, dtz);
    ASSERT_EQUAL(-1, ent.getEvalScore());


    pos = TextIO::readFEN("1R5Q/8/6k1/8/8/8/8/K1q5 w - - 0 1"); // DTZ == 101
    ent.clear();
    resWDL = TBProbe::rtbProbeWDL(pos, ply, wdl, ent);
    ASSERT(resWDL);
    ASSERT_EQUAL(0, wdl);
    ASSERT_EQUAL(1000, ent.getEvalScore());
    ent.clear();
    resDTZ = TBProbe::rtbProbeDTZ(pos, ply, dtz, ent);
    ASSERT(resDTZ);
    ASSERT_EQUAL(0, dtz);
    ASSERT_EQUAL(1000, ent.getEvalScore());

    pos = TextIO::readFEN("1R5Q/8/6k1/8/8/8/2q5/K7 b - - 0 1"); // DTZ == -102
    ent.clear();
    resWDL = TBProbe::rtbProbeWDL(pos, ply, wdl, ent);
    ASSERT(resWDL);
    ASSERT_EQUAL(0, wdl);
    ASSERT_EQUAL(-1000, ent.getEvalScore());
    ent.clear();
    resDTZ = TBProbe::rtbProbeDTZ(pos, ply, dtz, ent);
    ASSERT(resDTZ);
    ASSERT_EQUAL(0, dtz);
    ASSERT_EQUAL(-1000, ent.getEvalScore());

    pos = TextIO::readFEN("8/8/8/pk1K4/8/3N1N2/8/8 w - - 0 1"); // DTZ == 22
    resWDL = TBProbe::rtbProbeWDL(pos, ply, wdl, ent);
    ASSERT(resWDL);
    ASSERT(SearchConst::isWinScore(wdl));
    resDTZ = TBProbe::rtbProbeDTZ(pos, ply, dtz, ent);
    ASSERT(resDTZ);
    ASSERT(SearchConst::isWinScore(dtz));

    pos = TextIO::readFEN("8/8/8/pk1K4/8/3N1N2/8/8 w - - 85 1"); // DTZ == 22
    ent.clear();
    resWDL = TBProbe::rtbProbeWDL(pos, ply, wdl, ent);
    ASSERT(resWDL);
    ASSERT(SearchConst::isWinScore(wdl)); // WDL probes ignore halfMoveClock
    ent.clear();
    resDTZ = TBProbe::rtbProbeDTZ(pos, ply, dtz, ent);
    ASSERT(resDTZ);
    ASSERT_EQUAL(0, dtz);
    ASSERT_EQUAL(7, ent.getEvalScore());

    pos = TextIO::readFEN("6k1/8/5Q2/6K1/6Pp/8/8/7Q b - g3 0 1");
    int success;
    dtz = Syzygy::probe_dtz(pos, &success);
    ASSERT_EQUAL(1, success);
    ASSERT_EQUAL(-2, dtz);

    pos = TextIO::readFEN("3K4/8/3k4/8/4p3/4B3/5P2/8 w - - 0 5");
    dtz = Syzygy::probe_dtz(pos, &success);
    ASSERT_EQUAL(1, success);
    ASSERT_EQUAL(15, dtz);
}

/** Test TBProbe::tbProbe() function. */
void
TBTest::tbTest() {
    int ply = 29;
    const int mate0 = SearchConst::MATE0;
    TranspositionTable& tt = SearchTest::tt;

    // DTM > 100 when ignoring 50-move rule, RTB probes must be used when available
    Position pos = TextIO::readFEN("1R5Q/8/6k1/8/4q3/8/8/K7 b - - 0 1");
    TranspositionTable::TTEntry ent;
    bool res = TBProbe::tbProbe(pos, ply, -10, 10, tt, ent);
    ASSERT(res);
    ASSERT_EQUAL(TType::T_LE, ent.getType());
    ASSERT(ent.getScore(ply) < 0);

    res = TBProbe::tbProbe(pos, ply, -mate0, mate0, tt, ent);
    ASSERT(res);
    ASSERT_EQUAL(TType::T_LE, ent.getType());
    ASSERT(ent.getScore(ply) < 0);

    initTB(gtbDefaultPath, gtbDefaultCacheMB, ""); // Disable syzygy tables
    res = TBProbe::tbProbe(pos, ply, -mate0, mate0, tt, ent);
    ASSERT(res);
    ASSERT_EQUAL(TType::T_LE, ent.getType());
    ASSERT_EQUAL(0, ent.getScore(ply));
    ASSERT_EQUAL(-14, ent.getEvalScore());

    initTB(gtbDefaultPath, gtbDefaultCacheMB, rtbDefaultPath);

    // Half-move clock small, DTM mate wins
    pos = TextIO::readFEN("R5Q1/8/6k1/8/4q3/8/8/K7 b - - 0 1");
    res = TBProbe::tbProbe(pos, ply, -mate0, mate0, tt, ent);
    ASSERT(res);
    ASSERT_EQUAL(TType::T_EXACT, ent.getType());
    ASSERT_EQUAL(-(mate0 - ply - 23), ent.getScore(ply));
    res = TBProbe::tbProbe(pos, ply, -10, 10, tt, ent);
    ASSERT(res);
    ASSERT_EQUAL(TType::T_LE, ent.getType());
    ASSERT(SearchConst::isLoseScore(ent.getScore(ply)));

    // Half-move clock large, must follow DTZ path to win
    pos = TextIO::readFEN("R5Q1/8/6k1/8/4q3/8/8/K7 b - - 90 1");
    res = TBProbe::tbProbe(pos, ply, -mate0, mate0, tt, ent);
    ASSERT(res);
    ASSERT_EQUAL(TType::T_LE, ent.getType());
    ASSERT(SearchConst::isLoseScore(ent.getScore(ply)));
    ASSERT(ent.getScore(ply) > -(mate0 - ply - 23));
    res = TBProbe::tbProbe(pos, ply, -10, 10, tt, ent);
    ASSERT(res);
    ASSERT_EQUAL(TType::T_LE, ent.getType());
    ASSERT(SearchConst::isLoseScore(ent.getScore(ply)));

    // Mate in one, half-move clock small
    pos = TextIO::readFEN("8/8/4B3/8/kBK5/8/8/8 w - - 0 1");
    res = TBProbe::tbProbe(pos, ply, -mate0, mate0, tt, ent);
    ASSERT(res);
    ASSERT_EQUAL(TType::T_EXACT, ent.getType());
    ASSERT_EQUAL(mate0 - 2 - ply, ent.getScore(ply));

    // Mate in one, half-move clock large
    pos = TextIO::readFEN("8/8/4B3/8/kBK5/8/8/8 w - - 99 1");
    res = TBProbe::tbProbe(pos, ply, -mate0, mate0, tt, ent);
    ASSERT(res);
    ASSERT_EQUAL(TType::T_EXACT, ent.getType());
    ASSERT_EQUAL(mate0 - 2 - ply, ent.getScore(ply));
    // Same position, no GTB tables available
    initTB("/no/such/dir", gtbDefaultCacheMB, rtbDefaultPath);
    res = TBProbe::tbProbe(pos, ply, -mate0, mate0, tt, ent);
    ASSERT(!res || ent.getScore(ply) != 0);
    initTB(gtbDefaultPath, gtbDefaultCacheMB, rtbDefaultPath);

    pos = TextIO::readFEN("8/8/3pk3/8/8/3NK3/3N4/8 w - - 70 1"); // DTZ = 38
    res = TBProbe::tbProbe(pos, ply, -mate0, mate0, tt, ent);
    ASSERT(res);
    ASSERT_EQUAL(TType::T_EXACT, ent.getType());
    ASSERT_EQUAL(0, ent.getScore(ply));
    ASSERT_EQUAL(8, ent.getEvalScore());
    ent.clear();
    res = TBProbe::tbProbe(pos, ply, -15, 15, tt, ent);
    ASSERT(res);
    ASSERT_EQUAL(TType::T_EXACT, ent.getType());
    ASSERT_EQUAL(0, ent.getScore(ply));
    ASSERT_EQUAL(8, ent.getEvalScore());

    pos = TextIO::readFEN("8/8/4k1N1/p7/8/8/3N2K1/8 w - - 0 1"); // DTZ = 116
    ent.clear();
    res = TBProbe::tbProbe(pos, ply, -mate0, mate0, tt, ent);
    ASSERT(res);
    ASSERT_EQUAL(TType::T_EXACT, ent.getType());
    ASSERT_EQUAL(0, ent.getScore(ply));
    ASSERT_EQUAL(93, ent.getEvalScore());
    ent.clear();
    res = TBProbe::tbProbe(pos, ply, -15, 15, tt, ent);
    ASSERT(res);
    ASSERT_EQUAL(TType::T_EXACT, ent.getType());
    ASSERT_EQUAL(0, ent.getScore(ply));
    ASSERT_EQUAL(1000, ent.getEvalScore());

    {
        pos = TextIO::readFEN("2R5/4k3/Q7/8/8/8/8/K7 w - - 98 1");
        Search sc(pos, SearchTest::nullHist, 0, SearchTest::st, SearchTest::pd,
                  nullptr, SearchTest::treeLog);
        Move m = SearchTest::idSearch(sc, 4, 0);
        ASSERT_EQUAL("a6e6", TextIO::moveToUCIString(m));
    }
    {
        pos = TextIO::readFEN("2R5/4k3/Q7/8/8/8/8/K7 w - - 97 1");
        Search sc(pos, SearchTest::nullHist, 0, SearchTest::st, SearchTest::pd,
                  nullptr, SearchTest::treeLog);
        Move m = SearchTest::idSearch(sc, 4, 1);
        ASSERT_EQUAL("c8c7", TextIO::moveToUCIString(m));
    }
}


static void getLegalMoves(Position& pos, MoveList& legalMoves) {
    legalMoves.clear();
    MoveGen::pseudoLegalMoves(pos, legalMoves);
    MoveGen::removeIllegal(pos, legalMoves);
}

static void compareMoves(const std::vector<std::string>& strMoves,
                         const std::vector<Move>& moves) {
    ASSERT_EQUAL(strMoves.size(), moves.size());
    for (const Move& m : moves)
        ASSERT(contains(strMoves, TextIO::moveToUCIString(m)));
}

void
TBTest::testMissingTables() {
    TranspositionTable& tt = SearchTest::tt;
    for (int loop = 0; loop < 2; loop++) {
        bool gtb = loop == 1;
        // No progress move in TBs, must search all zeroing moves
        if (gtb)
            setupTBFiles(std::vector<std::string>{"kpk.gtb.cp4"});
        else
            setupTBFiles(std::vector<std::string>{"KPvK.rtbw", "KPvK.rtbz"});
        Position pos = TextIO::readFEN("8/4P3/8/8/2k1K3/8/8/8 w - - 0 1");
        MoveList legalMoves;
        getLegalMoves(pos, legalMoves);
        std::vector<Move> movesToSearch;
        bool res = TBProbe::getSearchMoves(pos, legalMoves, movesToSearch, tt);
        ASSERT_EQUAL(true, res);
        if (gtb)
            compareMoves(std::vector<std::string>{"e7e8q", "e7e8r", "e7e8b", "e7e8n"}, movesToSearch);
        {
            Search sc(pos, SearchTest::nullHist, 0, SearchTest::st, SearchTest::pd,
                      nullptr, SearchTest::treeLog);
            Move m = SearchTest::idSearch(sc, 4, 3);
            ASSERT_EQUAL("e7e8q", TextIO::moveToUCIString(m));
        }

        // Progress (queen promotion) in TB, no need to limit moves to search
        if (gtb)
            setupTBFiles(std::vector<std::string>{"kpk.gtb.cp4", "kqk.gtb.cp4"});
        else
            setupTBFiles(std::vector<std::string>{"KPvK.rtbw", "KPvK.rtbz", "KQvK.rtbw", "KQvK.rtbz"});
        pos = TextIO::readFEN("8/4P3/8/8/2k1K3/8/8/8 w - - 0 1");
        getLegalMoves(pos, legalMoves);
        movesToSearch.clear();
        res = TBProbe::getSearchMoves(pos, legalMoves, movesToSearch, tt);
        ASSERT_EQUAL(false, res);

        // No progress move in TBs, must search all unknown zeroing moves
        if (gtb)
            setupTBFiles(std::vector<std::string>{"kpk.gtb.cp4", "krk.gtb.cp4"});
        else
            setupTBFiles(std::vector<std::string>{"KPvK.rtbw", "KPvK.rtbz", "KRvK.rtbw", "KRvK.rtbz"});
        pos = TextIO::readFEN("8/4P3/8/8/2k1K3/8/8/8 w - - 0 1");
        getLegalMoves(pos, legalMoves);
        movesToSearch.clear();
        res = TBProbe::getSearchMoves(pos, legalMoves, movesToSearch, tt);
        if (gtb) {
            ASSERT_EQUAL(true, res);
            compareMoves(std::vector<std::string>{"e7e8q", "e7e8b", "e7e8n"}, movesToSearch);
        } else {
            ASSERT_EQUAL(false, res); // Rook promotion is an improvement when using only DTZ TBs
        }

        // Non-zeroing move makes progress, search all legal moves
        if (gtb) {
            setupTBFiles(std::vector<std::string>{"kpk.gtb.cp4"});
            pos = TextIO::readFEN("8/4P3/8/8/1k2K3/8/8/8 w - - 0 1");
            getLegalMoves(pos, legalMoves);
            movesToSearch.clear();
            res = TBProbe::getSearchMoves(pos, legalMoves, movesToSearch, tt);
            ASSERT_EQUAL(false, res);
        }
    }

    initTB(gtbDefaultPath, gtbDefaultCacheMB, rtbDefaultPath);
}

void
TBTest::testMaxSubMate() {
    using MI = MatId;
    initTB(gtbDefaultPath, gtbDefaultCacheMB, rtbDefaultPath);
    Position pos = TextIO::readFEN("3qk3/8/8/8/8/8/8/3QK3 w - - 0 1");
    int maxSub = TBProbe::getMaxSubMate(pos);
    ASSERT_EQUAL(TBProbe::getMaxDTZ(MI::WQ), maxSub);
}

cute::suite
TBTest::getSuite() const {
    cute::suite s;
    s.push_back(CUTE(dtmTest));
    s.push_back(CUTE(kpkTest));
    s.push_back(CUTE(rtbTest));
    s.push_back(CUTE(tbTest));
    s.push_back(CUTE(testMissingTables));
    s.push_back(CUTE(testMaxSubMate));
    return s;
}
