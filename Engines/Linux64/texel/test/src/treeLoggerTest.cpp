/*
    Texel - A UCI chess engine.
    Copyright (C) 2013-2014  Peter Österlund, peterosterlund2@gmail.com

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
 * treeLoggerTest.cpp
 *
 *  Created on: Sep 7, 2013
 *      Author: petero
 */

#include "treeLoggerTest.hpp"
#include "treeLogger.hpp"
#include "position.hpp"
#include "textio.hpp"
#include <iostream>
#include <cstring>

#include "cute.h"

void
TreeLoggerTest::testSerialize() {
    const U32 a = 123453428;
    const S32 b = -321234355;
    const U16 c = 40000;
    const S16 d = -20000;
    const U8  e = 180;
    const S8  f = -10;

    U8 buffer[14];
    U8* ptr = Serializer::serialize<sizeof(buffer)>(&buffer[0], a, b, c, d, e, f);
    ASSERT_EQUAL(sizeof(buffer), ptr - buffer);

    U32 a1;
    S32 b1;
    U16 c1;
    S16 d1;
    U8  e1;
    S8  f1;

    const U8* ptr2 = Serializer::deSerialize<sizeof(buffer)>(&buffer[0], a1, b1, c1, d1, e1, f1);
    ASSERT_EQUAL(sizeof(buffer), ptr2 - buffer);
    ASSERT_EQUAL(a, a1);
    ASSERT_EQUAL(b, b1);
    ASSERT_EQUAL(c, c1);
    ASSERT_EQUAL(d, d1);
    ASSERT_EQUAL(e, e1);
    ASSERT_EQUAL(f, f1);
}

void
TreeLoggerTest::testLoggerData() {
    using TB = TreeLoggerBase;
    {
        TB::Entry e;
        e.type = TB::EntryType::POSITION_INCOMPLETE;
        e.p0.nextIndex = 17;
        e.p0.word0 = 0x3214876587651234ULL;
        e.p0.word1 = 0x1234454656345123ULL;
        U8 buffer[TB::Entry::bufSize];
        memset(buffer, 0xde, sizeof(buffer));
        e.serialize(buffer);
        TB::Entry e2;
        e2.deSerialize(buffer);
        ASSERT_EQUAL(e.type, e2.type);
        ASSERT_EQUAL(e.p0.nextIndex, e2.p0.nextIndex);
        ASSERT_EQUAL(e.p0.word0,     e2.p0.word0);
        ASSERT_EQUAL(e.p0.word1,     e2.p0.word1);
    }
    {
        TB::Entry e;
        e.type = TB::EntryType::POSITION_PART0;
        e.p0.nextIndex = 123987654;
        e.p0.word0 = 0x3876587651234ULL;
        e.p0.word1 = 0x1234456345123ULL;
        U8 buffer[TB::Entry::bufSize];
        memset(buffer, 0xde, sizeof(buffer));
        e.serialize(buffer);
        TB::Entry e2;
        e2.deSerialize(buffer);
        ASSERT_EQUAL(e.type, e2.type);
        ASSERT_EQUAL(e.p0.nextIndex, e2.p0.nextIndex);
        ASSERT_EQUAL(e.p0.word0,     e2.p0.word0);
        ASSERT_EQUAL(e.p0.word1,     e2.p0.word1);
    }
    {
        TB::Entry e;
        e.type = TB::EntryType::POSITION_PART1;
        e.p1.t0Index = 337289123;
        e.p1.word2 = 0xfedc1234435432ffULL;
        e.p1.word3 = 0x1324123434534ULL;
        U8 buffer[TB::Entry::bufSize];
        memset(buffer, 0xde, sizeof(buffer));
        e.serialize(buffer);
        TB::Entry e2;
        e2.deSerialize(buffer);
        ASSERT_EQUAL(e.type, e2.type);
        ASSERT_EQUAL(e.p1.t0Index, e2.p1.t0Index);
        ASSERT_EQUAL(e.p1.word2,   e2.p1.word2);
        ASSERT_EQUAL(e.p1.word3,   e2.p1.word3);
    }
    {
        TB::Entry e;
        e.type = TB::EntryType::POSITION_PART2;
        e.p2.word4 = 0x834927342342134ULL;
        e.p2.owningThread = 17;
        e.p2.moveNo = 138;
        e.p2.parentIndex = 47114711;
        U8 buffer[TB::Entry::bufSize];
        memset(buffer, 0xde, sizeof(buffer));
        e.serialize(buffer);
        TB::Entry e2;
        e2.deSerialize(buffer);
        ASSERT_EQUAL(e.type, e2.type);
        ASSERT_EQUAL(e.p2.word4,        e2.p2.word4);
        ASSERT_EQUAL(e.p2.owningThread, e2.p2.owningThread);
        ASSERT_EQUAL(e.p2.moveNo,       e2.p2.moveNo);
        ASSERT_EQUAL(e.p2.parentIndex,  e2.p2.parentIndex);
    }
    {
        TB::Entry e;
        e.type = TB::EntryType::NODE_START;
        e.se.endIndex = 134;
        e.se.parentIndex = 2342134;
        e.se.move = 0x1234;
        e.se.alpha = -20000;
        e.se.beta = 30000;
        e.se.ply = 17;
        e.se.depth = 23 * 8;
        U8 buffer[TB::Entry::bufSize];
        memset(buffer, 0xde, sizeof(buffer));
        e.serialize(buffer);
        TB::Entry e2;
        e2.deSerialize(buffer);
        ASSERT_EQUAL(e.type, e2.type);
        ASSERT_EQUAL(e.se.endIndex,    e2.se.endIndex);
        ASSERT_EQUAL(e.se.parentIndex, e2.se.parentIndex);
        ASSERT_EQUAL(e.se.move,        e2.se.move);
        ASSERT_EQUAL(e.se.alpha,       e2.se.alpha);
        ASSERT_EQUAL(e.se.beta,        e2.se.beta);
        ASSERT_EQUAL(e.se.ply,         e2.se.ply);
        ASSERT_EQUAL(e.se.depth,       e2.se.depth);
    }
    {
        TB::Entry e;
        e.type = TB::EntryType::NODE_END;
        e.ee.startIndex = 1000000000;
        e.ee.score = 17389;
        e.ee.scoreType = 2;
        e.ee.evalScore = 389;
        e.ee.hashKey = 0xf23456789abcde10ULL;
        U8 buffer[TB::Entry::bufSize];
        memset(buffer, 0xde, sizeof(buffer));
        e.serialize(buffer);
        TB::Entry e2;
        e2.deSerialize(buffer);
        ASSERT_EQUAL(e.type, e2.type);
        ASSERT_EQUAL(e.ee.startIndex, e2.ee.startIndex);
        ASSERT_EQUAL(e.ee.score,      e2.ee.score);
        ASSERT_EQUAL(e.ee.scoreType,  e2.ee.scoreType);
        ASSERT_EQUAL(e.ee.evalScore,  e2.ee.evalScore);
        ASSERT_EQUAL(e.ee.hashKey,    e2.ee.hashKey);
    }
}

cute::suite
TreeLoggerTest::getSuite() const {
    cute::suite s;
    s.push_back(CUTE(testSerialize));
    s.push_back(CUTE(testLoggerData));
    return s;
}
