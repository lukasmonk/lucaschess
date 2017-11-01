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
 * gameTreeTest.cpp
 *
 *  Created on: Apr 9, 2016
 *      Author: petero
 */

#include "gameTreeTest.hpp"
#include "textio.hpp"
#include "gametree.hpp"

#include "cute.h"

struct RExpected {
    int b;
    int e;
    std::string m;
};

static void
checkPosToNodes(GameTree& gt, const std::set<GameTree::RangeToNode>& posToNodes,
                const std::vector<RExpected>& expected) {
    ASSERT_EQUAL(posToNodes.size(), expected.size());
    int i = 0;
    for (const GameTree::RangeToNode& rangeInfo : posToNodes) {
        ASSERT_EQUAL(expected[i].b, rangeInfo.begin);
        ASSERT_EQUAL(expected[i].e, rangeInfo.end);
        GameNode gn = gt.getNode(rangeInfo.node);
        Move m = gn.getMove();
        bool result = gn.goBack();
        ASSERT(result);
        std::string s = TextIO::moveToString(gn.getPos(), m, false);
        ASSERT_EQUAL(expected[i].m, s);
        i++;
    }
}

void
GameTreeTest::testReadInsert() {
    std::string pgn = R"raw(
[Event "event01"]
[Site "site02"]
[Date "2016.04.09"]
[Round "1"]
[White "white player"]
[Black "black player"]
[Result "1-0"]
[ECO "E10"]

1. d4 {[%eval 0,1] [%emt 0:00:00]} Nf6 {[%eval 0,1] [%emt 0:00:00]}
2. c4 {[%eval 0,1] [%emt 0:00:00]} e6 {[%eval 0,1] [%emt 0:00:00]} 3. Nf3
d5  4. Nc3 dxc4 5. e4  Bb4  6. Bg5  c5  7. dxc5
1-0
)raw";
    std::stringstream is(pgn);
    GameTree gt, gt3;
    PgnReader reader(is);
    bool result = reader.readPGN(gt);
    ASSERT(result);
    result = reader.readPGN(gt3);
    ASSERT(!result);
    std::string str;
    std::set<GameTree::RangeToNode> posToNodes;
    gt.getGameTreeString(str, posToNodes);
    ASSERT_EQUAL("d4 Nf6 c4 e6 Nf3 d5 Nc3 dxc4 e4 Bb4 Bg5 c5 dxc5", str);

    gt.insertMoves({
        TextIO::uciStringToMove("d2d4"), TextIO::uciStringToMove("g8f6"),
        TextIO::uciStringToMove("g1f3"), TextIO::uciStringToMove("d7d5")
    });
    gt.getGameTreeString(str, posToNodes);
    ASSERT_EQUAL("d4 Nf6 c4 (Nf3 d5) e6 Nf3 d5 Nc3 dxc4 e4 Bb4 Bg5 c5 dxc5", str);

    gt.insertMoves({
        TextIO::uciStringToMove("d2d4"), TextIO::uciStringToMove("g8f6"),
        TextIO::uciStringToMove("g1f3"), TextIO::uciStringToMove("c7c6")
    });
    gt.getGameTreeString(str, posToNodes);
    ASSERT_EQUAL("d4 Nf6 c4 (Nf3 d5 (c6)) e6 Nf3 d5 Nc3 dxc4 e4 Bb4 Bg5 c5 dxc5", str);
    std::vector<RExpected> expected {
        { 0,  2, "d4"},
        { 3,  6, "Nf6"},
        { 7,  9, "c4"},
        {11, 14, "Nf3"},
        {15, 17, "d5"},
        {19, 21, "c6"},
        {24, 26, "e6"},
        {27, 30, "Nf3"},
        {31, 33, "d5"},
        {34, 37, "Nc3"},
        {38, 42, "dxc4"},
        {43, 45, "e4"},
        {46, 49, "Bb4"},
        {50, 53, "Bg5"},
        {54, 56, "c5"},
        {57, 61, "dxc5"}
    };
    checkPosToNodes(gt, posToNodes, expected);

    std::string pgn2 = R"raw(
e4 e5 Nf3 Nc6 Bb5 (Bc4 Bc5 c3) (Nc3 Nf6) a6 Ba4
)raw";
    std::stringstream is2(pgn2);
    GameTree gt2;
    PgnReader reader2(is2);
    result = reader2.readPGN(gt2);
    ASSERT(result);
    result = reader.readPGN(gt3);
    ASSERT(!result);
    gt2.getGameTreeString(str, posToNodes);
    ASSERT_EQUAL("e4 e5 Nf3 Nc6 Bb5 (Bc4 Bc5 c3) (Nc3 Nf6) a6 Ba4", str);

    gt.insertTree(gt2, -1);
    gt2.getGameTreeString(str, posToNodes);
    ASSERT_EQUAL("e4 e5 Nf3 Nc6 Bb5 (Bc4 Bc5 c3) (Nc3 Nf6) a6 Ba4", str);
    gt.getGameTreeString(str, posToNodes);
    ASSERT_EQUAL("d4 (e4 e5 Nf3 Nc6 Bb5 (Bc4 Bc5 c3) (Nc3 Nf6) a6 Ba4) "
                 "Nf6 c4 (Nf3 d5 (c6)) e6 Nf3 d5 Nc3 dxc4 e4 Bb4 Bg5 c5 dxc5", str);

    {
        pgn = R"raw(
[Event "event01"]
[Site "site02"]
[Date "2016.04.09"]
[Round "1"]
[White "white player"]
[Black "black player"]
[Result "1-0"]
[ECO "E10"]

e4 e5 Nf3 Nc6 *

[Event "event01"]
[Site "site02"]
[Date "2016.04.09"]
[Round "1"]
[White "white player"]
[Black "black player"]
[Result "1-0"]
[ECO "E10"]

d4 Nc6 c4 d6 *
)raw";
        gt = GameTree();
        std::stringstream is(pgn);
        PgnReader reader(is);
        result = reader.readPGN(gt);
        ASSERT(result);
        gt.getGameTreeString(str, posToNodes);
        ASSERT_EQUAL("e4 e5 Nf3 Nc6", str);
        gt2 = GameTree();
        result = reader.readPGN(gt2);
        ASSERT(result);
        gt2.getGameTreeString(str, posToNodes);
        ASSERT_EQUAL("d4 Nc6 c4 d6", str);
        result = reader.readPGN(gt3);
        ASSERT(!result);
    }
}

cute::suite
GameTreeTest::getSuite() const {
    cute::suite s;
    s.push_back(CUTE(testReadInsert));
    return s;
}
