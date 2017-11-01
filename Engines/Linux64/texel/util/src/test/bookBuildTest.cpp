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
 * bookBuildTest.cpp
 *
 *  Created on: May 29, 2015
 *      Author: petero
 */

#include "bookBuildTest.hpp"
#include "bookbuild.hpp"
#include "textio.hpp"

#include "cute.h"

using namespace BookBuild;

void
BookBuildTest::testBookNode() {
    BookData bd(100, 200, 50);
    {
        BookNode bn(1234);
        ASSERT_EQUAL(INT_MAX, bn.getDepth());
    }

    auto bn(std::make_shared<BookNode>(12345678, true));
    ASSERT_EQUAL(12345678, bn->getHashKey());
    ASSERT_EQUAL(0, bn->getDepth());
    ASSERT_EQUAL(BookNode::EMPTY, bn->getState());
    ASSERT_EQUAL(0, bn->getChildren().size());
    ASSERT_EQUAL(0, bn->getParents().size());

    bn->setState(BookNode::INITIALIZED);
    ASSERT_EQUAL(BookNode::INITIALIZED, bn->getState());

    Move e4(TextIO::uciStringToMove("e2e4"));
    Move d4(TextIO::uciStringToMove("d2d4"));
    bn->setSearchResult(bd, d4, 17, 4711);
    ASSERT_EQUAL(d4, bn->getBestNonBookMove());
    ASSERT_EQUAL(17, bn->getSearchScore());
    ASSERT_EQUAL(4711, bn->getSearchTime());

    ASSERT_EQUAL(17, bn->getNegaMaxScore());
    ASSERT_EQUAL(0, bn->getExpansionCostWhite());
    ASSERT_EQUAL(0, bn->getExpansionCostBlack());
    ASSERT_EQUAL(0, bn->getPathErrorWhite());
    ASSERT_EQUAL(0, bn->getPathErrorBlack());

    {
        BookNode::BookSerializeData bsd;
        bn->serialize(bsd);
        BookNode bn2(0);
        bn2.deSerialize(bsd);
        ASSERT_EQUAL(12345678, bn2.getHashKey());
        ASSERT_EQUAL(0, bn2.getChildren().size());
        ASSERT_EQUAL(0, bn2.getParents().size());
        ASSERT_EQUAL(BookNode::DESERIALIZED, bn2.getState());
        ASSERT_EQUAL(d4, bn2.getBestNonBookMove());
        ASSERT_EQUAL(17, bn2.getSearchScore());
        ASSERT_EQUAL(4711, bn2.getSearchTime());
    }

    auto child(std::make_shared<BookNode>(22222222, false));
    U16 e4c = e4.getCompressedMove();
    bn->addChild(e4c, child.get());
    child->addParent(e4c, bn.get());

    ASSERT_EQUAL(1, bn->getChildren().size());
    ASSERT_EQUAL(0, bn->getParents().size());
    ASSERT_EQUAL(0, child->getChildren().size());
    ASSERT_EQUAL(1, child->getParents().size());
    ASSERT_EQUAL(child.get(), bn->getChildren().find(e4c)->second);
    ASSERT_EQUAL(bn.get(), child->getParents().lower_bound(BookNode::ParentInfo(e4c))->parent);
    ASSERT_EQUAL(0, bn->getDepth());
    ASSERT_EQUAL(1, child->getDepth());

    Move e5(TextIO::uciStringToMove("e7e5"));
    Move c5(TextIO::uciStringToMove("c7c5"));
    child->setSearchResult(bd, c5, -20, 10000);
    ASSERT_EQUAL(20, bn->getNegaMaxScore());
    ASSERT_EQUAL(100, bn->getExpansionCostWhite());
    ASSERT_EQUAL(100, bn->getExpansionCostBlack());
    ASSERT_EQUAL(100, bn->getExpansionCost(bd, child.get(), true));
    ASSERT_EQUAL(100, bn->getExpansionCost(bd, child.get(), false));
    ASSERT_EQUAL(0, bn->getPathErrorWhite());
    ASSERT_EQUAL(0, bn->getPathErrorBlack());
    ASSERT_EQUAL(0, child->getPathErrorWhite());
    ASSERT_EQUAL(0, child->getPathErrorBlack());

    child->setSearchResult(bd, c5, -16, 10000);
    ASSERT_EQUAL(17, bn->getNegaMaxScore());
    ASSERT_EQUAL(0, bn->getExpansionCostWhite());
    ASSERT_EQUAL(0, bn->getExpansionCostBlack());
    ASSERT_EQUAL(300, bn->getExpansionCost(bd, child.get(), true));
    ASSERT_EQUAL(150, bn->getExpansionCost(bd, child.get(), false));
    ASSERT_EQUAL(0, bn->getPathErrorWhite());
    ASSERT_EQUAL(0, bn->getPathErrorBlack());
    ASSERT_EQUAL(1, child->getPathErrorWhite());
    ASSERT_EQUAL(0, child->getPathErrorBlack());

    auto child2(std::make_shared<BookNode>(33333333, false));
    U16 e5c = e5.getCompressedMove();
    child->addChild(e5c, child2.get());
    child2->addParent(e5c, child.get());

    ASSERT_EQUAL(1, bn->getChildren().size());
    ASSERT_EQUAL(0, bn->getParents().size());
    ASSERT_EQUAL(1, child->getChildren().size());
    ASSERT_EQUAL(1, child->getParents().size());
    ASSERT_EQUAL(child.get(), bn->getChildren().find(e4c)->second);
    ASSERT_EQUAL(bn.get(), child->getParents().lower_bound(BookNode::ParentInfo(e4c))->parent);
    ASSERT_EQUAL(0, child2->getChildren().size());
    ASSERT_EQUAL(1, child2->getParents().size());
    ASSERT_EQUAL(child2.get(), child->getChildren().find(e5c)->second);
    ASSERT_EQUAL(child.get(), child2->getParents().lower_bound(BookNode::ParentInfo(e5c))->parent);
    ASSERT_EQUAL(0, bn->getDepth());
    ASSERT_EQUAL(1, child->getDepth());
    ASSERT_EQUAL(2, child2->getDepth());

    Move nf3(TextIO::uciStringToMove("g1f3"));
    child2->setSearchResult(bd, nf3, 17, 10000);
    ASSERT_EQUAL(17, child2->getNegaMaxScore());
    ASSERT_EQUAL(0, child2->getExpansionCostWhite());
    ASSERT_EQUAL(0, child2->getExpansionCostBlack());
    ASSERT_EQUAL(150, child->getExpansionCost(bd, child2.get(), true));
    ASSERT_EQUAL(300, child->getExpansionCost(bd, child2.get(), false));

    ASSERT_EQUAL(-16, child->getNegaMaxScore());
    ASSERT_EQUAL(0, child->getExpansionCostWhite());
    ASSERT_EQUAL(0, child->getExpansionCostBlack());

    ASSERT_EQUAL(17, bn->getNegaMaxScore());
    ASSERT_EQUAL(0, bn->getExpansionCostWhite());
    ASSERT_EQUAL(0, bn->getExpansionCostBlack());
    ASSERT_EQUAL(300, bn->getExpansionCost(bd, child.get(), true));
    ASSERT_EQUAL(150, bn->getExpansionCost(bd, child.get(), false));

    ASSERT_EQUAL(0, bn->getPathErrorWhite());
    ASSERT_EQUAL(0, bn->getPathErrorBlack());
    ASSERT_EQUAL(1, child->getPathErrorWhite());
    ASSERT_EQUAL(0, child->getPathErrorBlack());
    ASSERT_EQUAL(1, child2->getPathErrorWhite());
    ASSERT_EQUAL(1, child2->getPathErrorBlack());

    child2->setSearchResult(bd, nf3, 10, 10000);
    ASSERT_EQUAL(10, child2->getNegaMaxScore());
    ASSERT_EQUAL(0, child2->getExpansionCostWhite());
    ASSERT_EQUAL(0, child2->getExpansionCostBlack());

    ASSERT_EQUAL(-10, child->getNegaMaxScore());
    ASSERT_EQUAL(100, child->getExpansionCostWhite());
    ASSERT_EQUAL(100, child->getExpansionCostBlack());
    ASSERT_EQUAL(100, child->getExpansionCost(bd, child2.get(), true));
    ASSERT_EQUAL(100, child->getExpansionCost(bd, child2.get(), false));

    ASSERT_EQUAL(17, bn->getNegaMaxScore());
    ASSERT_EQUAL(0, bn->getExpansionCostWhite());
    ASSERT_EQUAL(0, bn->getExpansionCostBlack());

    bn->setSearchResult(bd, d4, 5, 10000);
    child->setSearchResult(bd, c5, -25, 10000);
    child2->setSearchResult(bd, nf3, 17, 10000);
    ASSERT_EQUAL(17, bn->getNegaMaxScore());
    ASSERT_EQUAL(0, child2->getExpansionCostWhite());
    ASSERT_EQUAL(0, child2->getExpansionCostBlack());
    ASSERT_EQUAL(100, child->getExpansionCostWhite());
    ASSERT_EQUAL(100, child->getExpansionCostBlack());
    ASSERT_EQUAL(100, child->getExpansionCost(bd, child2.get(), true));
    ASSERT_EQUAL(100, child->getExpansionCost(bd, child2.get(), false));
    ASSERT_EQUAL(200, bn->getExpansionCostWhite());
    ASSERT_EQUAL(200, bn->getExpansionCostBlack());
    ASSERT_EQUAL(200, bn->getExpansionCost(bd, child.get(), true));
    ASSERT_EQUAL(200, bn->getExpansionCost(bd, child.get(), false));

    child->setSearchResult(bd, c5, -18, 10000);
    ASSERT_EQUAL(17, bn->getNegaMaxScore());
    ASSERT_EQUAL(0, child2->getExpansionCostWhite());
    ASSERT_EQUAL(0, child2->getExpansionCostBlack());
    ASSERT_EQUAL(50, child->getExpansionCostWhite());
    ASSERT_EQUAL(100, child->getExpansionCostBlack());
    ASSERT_EQUAL(150, bn->getExpansionCostWhite());
    ASSERT_EQUAL(200, bn->getExpansionCostBlack());

    bd.addPending(child2->getHashKey());
    child2->updateScores(bd);
    ASSERT_EQUAL(17, bn->getNegaMaxScore());
    ASSERT_EQUAL(IGNORE_SCORE, child2->getExpansionCostWhite());
    ASSERT_EQUAL(IGNORE_SCORE, child2->getExpansionCostBlack());
    ASSERT_EQUAL(50, child->getExpansionCostWhite());
    ASSERT_EQUAL(200, child->getExpansionCostBlack());
    ASSERT_EQUAL(150, bn->getExpansionCostWhite());
    ASSERT_EQUAL(300, bn->getExpansionCostBlack());

    bd.addPending(child->getHashKey());
    child->updateScores(bd);
    ASSERT_EQUAL(17, bn->getNegaMaxScore());
    ASSERT_EQUAL(IGNORE_SCORE, child2->getExpansionCostWhite());
    ASSERT_EQUAL(IGNORE_SCORE, child2->getExpansionCostBlack());
    ASSERT_EQUAL(IGNORE_SCORE, child->getExpansionCostWhite());
    ASSERT_EQUAL(IGNORE_SCORE, child->getExpansionCostBlack());
    ASSERT_EQUAL(12*200, bn->getExpansionCostWhite());
    ASSERT_EQUAL(12*50, bn->getExpansionCostBlack());

    bd.addPending(bn->getHashKey());
    bn->updateScores(bd);
    bd.removePending(child->getHashKey());
    child->updateScores(bd);
    ASSERT_EQUAL(17, bn->getNegaMaxScore());
    ASSERT_EQUAL(IGNORE_SCORE, child2->getExpansionCostWhite());
    ASSERT_EQUAL(IGNORE_SCORE, child2->getExpansionCostBlack());
    ASSERT_EQUAL(50, child->getExpansionCostWhite());
    ASSERT_EQUAL(200, child->getExpansionCostBlack());
    ASSERT_EQUAL(150, bn->getExpansionCostWhite());
    ASSERT_EQUAL(300, bn->getExpansionCostBlack());

    bd.removePending(bn->getHashKey());
    bn->updateScores(bd);
    bd.removePending(child2->getHashKey());
    child2->updateScores(bd);
    bn->setSearchResult(bd, d4, INVALID_SCORE, 10000);
    child->setSearchResult(bd, c5, -18, 10000);
    child2->setSearchResult(bd, nf3, 17, 10000);
    ASSERT_EQUAL(INVALID_SCORE, bn->getNegaMaxScore());
    ASSERT_EQUAL(0, child2->getExpansionCostWhite());
    ASSERT_EQUAL(0, child2->getExpansionCostBlack());
    ASSERT_EQUAL(50, child->getExpansionCostWhite());
    ASSERT_EQUAL(100, child->getExpansionCostBlack());
    ASSERT_EQUAL(INVALID_SCORE, bn->getExpansionCostWhite());
    ASSERT_EQUAL(INVALID_SCORE, bn->getExpansionCostBlack());

    bd.addPending(bn->getHashKey());
    bn->updateScores(bd);
    ASSERT_EQUAL(INVALID_SCORE, bn->getNegaMaxScore());
    ASSERT_EQUAL(0, child2->getExpansionCostWhite());
    ASSERT_EQUAL(0, child2->getExpansionCostBlack());
    ASSERT_EQUAL(50, child->getExpansionCostWhite());
    ASSERT_EQUAL(100, child->getExpansionCostBlack());
    ASSERT_EQUAL(200150, bn->getExpansionCostWhite());
    ASSERT_EQUAL(50200, bn->getExpansionCostBlack());
}

void
BookBuildTest::testShortestDepth() {
    auto n1(std::make_shared<BookNode>(1, true));
    auto n2(std::make_shared<BookNode>(2, false));
    auto n3(std::make_shared<BookNode>(3, false));
    auto n4(std::make_shared<BookNode>(4, false));
    Move m(0, 0, Piece::EMPTY);
    U16 mc = m.getCompressedMove();

    n1->addChild(mc, n2.get());
    n2->addParent(mc, n1.get());

    n2->addChild(mc, n3.get());
    n3->addParent(mc, n2.get());

    n3->addChild(mc, n4.get());
    n4->addParent(mc, n3.get());

    ASSERT_EQUAL(0, n1->getDepth());
    ASSERT_EQUAL(1, n2->getDepth());
    ASSERT_EQUAL(2, n3->getDepth());
    ASSERT_EQUAL(3, n4->getDepth());

    Move m2(1, 1, Piece::EMPTY);
    U16 m2c = m2.getCompressedMove();
    n1->addChild(m2c, n4.get());
    n4->addParent(m2c, n1.get());

    ASSERT_EQUAL(0, n1->getDepth());
    ASSERT_EQUAL(1, n2->getDepth());
    ASSERT_EQUAL(2, n3->getDepth());
    ASSERT_EQUAL(1, n4->getDepth());
}

void
BookBuildTest::testBookNodeDAG() {
    BookData bd(100, 200, 50);
    auto n1(std::make_shared<BookNode>(1, true));
    auto n2(std::make_shared<BookNode>(2, false));
    auto n3(std::make_shared<BookNode>(3, false));
    auto n4(std::make_shared<BookNode>(4, false));
    auto n5(std::make_shared<BookNode>(5, false));
    auto n6(std::make_shared<BookNode>(6, false));

    U16 m = TextIO::uciStringToMove("e2e4").getCompressedMove();
    n1->addChild(m, n2.get());
    n2->addParent(m, n1.get());

    m = TextIO::uciStringToMove("g8f6").getCompressedMove();
    n2->addChild(m, n3.get());
    n3->addParent(m, n2.get());

    m = TextIO::uciStringToMove("d2d4").getCompressedMove();
    n3->addChild(m, n4.get());
    n4->addParent(m, n3.get());

    m = TextIO::uciStringToMove("d2d4").getCompressedMove();
    n1->addChild(m, n5.get());
    n5->addParent(m, n1.get());

    m = TextIO::uciStringToMove("g8f6").getCompressedMove();
    n5->addChild(m, n6.get());
    n6->addParent(m, n5.get());

    m = TextIO::uciStringToMove("e2e4").getCompressedMove();
    n6->addChild(m, n4.get());
    n4->addParent(m, n6.get());

    Move nm(0, 0, Piece::EMPTY);
    n1->setSearchResult(bd, nm, 10, 10000);
    n2->setSearchResult(bd, nm, -8, 10000);
    n3->setSearchResult(bd, nm, 7, 10000);
    n4->setSearchResult(bd, nm, -12, 10000);
    n5->setSearchResult(bd, nm, -12, 10000);
    n6->setSearchResult(bd, nm, 11, 10000);

    ASSERT_EQUAL(-12, n4->getNegaMaxScore());
    ASSERT_EQUAL(12, n3->getNegaMaxScore());
    ASSERT_EQUAL(-8, n2->getNegaMaxScore());
    ASSERT_EQUAL(12, n6->getNegaMaxScore());
    ASSERT_EQUAL(-12, n5->getNegaMaxScore());
    ASSERT_EQUAL(12, n1->getNegaMaxScore());

    n4->setSearchResult(bd, nm, -6, 10000);
    ASSERT_EQUAL(-6, n4->getNegaMaxScore());
    ASSERT_EQUAL(7, n3->getNegaMaxScore());
    ASSERT_EQUAL(-7, n2->getNegaMaxScore());
    ASSERT_EQUAL(11, n6->getNegaMaxScore());
    ASSERT_EQUAL(-11, n5->getNegaMaxScore());
    ASSERT_EQUAL(11, n1->getNegaMaxScore());

    n1->setSearchResult(bd, nm, 13, 10000);
    ASSERT_EQUAL(-6, n4->getNegaMaxScore());
    ASSERT_EQUAL(7, n3->getNegaMaxScore());
    ASSERT_EQUAL(-7, n2->getNegaMaxScore());
    ASSERT_EQUAL(11, n6->getNegaMaxScore());
    ASSERT_EQUAL(-11, n5->getNegaMaxScore());
    ASSERT_EQUAL(13, n1->getNegaMaxScore());
}

void
BookBuildTest::testAddPosToBook() {
    Book book("", 100, 200, 50);
    const BookData& bd = book.bookData;
    Position pos = TextIO::readFEN(TextIO::startPosFEN);
    const U64 rootHash = pos.bookHash();

    BookNode* n1 = book.getBookNode(rootHash);
    ASSERT(n1);

    std::vector<U64> toSearch;
    Move e4(TextIO::uciStringToMove("e2e4"));
    book.addPosToBook(pos, e4, toSearch);
    n1 = book.getBookNode(rootHash);
    ASSERT(n1);
    ASSERT_EQUAL(2, book.bookNodes.size());
    ASSERT_EQUAL(2, toSearch.size());
    ASSERT_EQUAL(rootHash, pos.bookHash());
    ASSERT(contains(toSearch, rootHash));
    UndoInfo ui1;
    pos.makeMove(e4, ui1);
    const U64 n2Hash = pos.bookHash();
    BookNode* n2 = book.getBookNode(n2Hash);
    ASSERT(n2);
    ASSERT(contains(toSearch, n2Hash));

    Position pos2;
    std::vector<Move> moveList;
    ASSERT(book.getPosition(rootHash, pos2, moveList));
    ASSERT(pos2.equals(TextIO::readFEN(TextIO::startPosFEN)));
    ASSERT_EQUAL(0, moveList.size());
    moveList.clear();
    ASSERT(book.getPosition(n2Hash, pos2, moveList));
    ASSERT(pos2.equals(pos));
    ASSERT_EQUAL(1, moveList.size());
    ASSERT_EQUAL(e4, moveList[0]);

    Move nf3(TextIO::uciStringToMove("g1f3"));
    const int t = 10000;
    n1->setSearchResult(bd, nf3, 10, t);
    Move nc6(TextIO::uciStringToMove("b8c6"));
    n2->setSearchResult(bd, nc6, -8, t);
    ASSERT_EQUAL(-8, n2->getNegaMaxScore());
    ASSERT_EQUAL(10, n1->getNegaMaxScore());

    toSearch.clear();
    Move nf6(TextIO::uciStringToMove("g8f6"));
    book.addPosToBook(pos, nf6, toSearch);
    ASSERT_EQUAL(3, book.bookNodes.size());
    ASSERT_EQUAL(2, toSearch.size());
    ASSERT(contains(toSearch, n2Hash));
    UndoInfo ui2;
    pos.makeMove(nf6, ui2);
    const U64 n3Hash = pos.bookHash();
    BookNode* n3 = book.getBookNode(n3Hash);
    ASSERT(n3);
    ASSERT(contains(toSearch, n3Hash));

    n3->setSearchResult(bd, nf3, 7, t);
    ASSERT_EQUAL(7, n3->getNegaMaxScore());
    ASSERT_EQUAL(-7, n2->getNegaMaxScore());
    ASSERT_EQUAL(10, n1->getNegaMaxScore());

    moveList.clear();
    ASSERT(book.getPosition(n3Hash, pos2, moveList));
    ASSERT(pos2.equals(pos));
    ASSERT_EQUAL(2, moveList.size());
    ASSERT_EQUAL(e4, moveList[0]);
    ASSERT_EQUAL(nf6, moveList[1]);

    pos.unMakeMove(nf6, ui2);
    pos.unMakeMove(e4, ui1);
    Move d4(TextIO::uciStringToMove("d2d4"));
    toSearch.clear();
    book.addPosToBook(pos, d4, toSearch);
    ASSERT_EQUAL(4, book.bookNodes.size());
    ASSERT_EQUAL(2, toSearch.size());
    ASSERT_EQUAL(rootHash, pos.bookHash());
    pos.makeMove(d4, ui1);
    const U64 n5Hash = pos.bookHash();
    BookNode* n5 = book.getBookNode(n5Hash);
    ASSERT(n5);
    ASSERT(contains(toSearch, n5Hash));
    ASSERT(contains(toSearch, rootHash));

    n5->setSearchResult(bd, nc6, -12, t);
    ASSERT_EQUAL(-12, n5->getNegaMaxScore());
    ASSERT_EQUAL(12, n1->getNegaMaxScore());

    moveList.clear();
    ASSERT(book.getPosition(n5Hash, pos2, moveList));
    ASSERT(pos2.equals(pos));
    ASSERT_EQUAL(1, moveList.size());
    ASSERT_EQUAL(d4, moveList[0]);

    toSearch.clear();
    book.addPosToBook(pos, nf6, toSearch);
    ASSERT_EQUAL(5, book.bookNodes.size());
    ASSERT_EQUAL(2, toSearch.size());
    ASSERT_EQUAL(n5Hash, pos.bookHash());
    pos.makeMove(nf6, ui2);
    const U64 n6Hash = pos.bookHash();
    BookNode* n6 = book.getBookNode(n6Hash);
    ASSERT(n6);
    ASSERT(contains(toSearch, n6Hash));
    ASSERT(contains(toSearch, n5Hash));

    n6->setSearchResult(bd, nf3, 11, t);
    ASSERT_EQUAL(11, n6->getNegaMaxScore());
    ASSERT_EQUAL(-11, n5->getNegaMaxScore());
    ASSERT_EQUAL(11, n1->getNegaMaxScore());

    moveList.clear();
    ASSERT(book.getPosition(n6Hash, pos2, moveList));
    ASSERT(pos2.equals(pos));
    ASSERT_EQUAL(2, moveList.size());
    ASSERT_EQUAL(d4, moveList[0]);
    ASSERT_EQUAL(nf6, moveList[1]);

    toSearch.clear();
    book.addPosToBook(pos, e4, toSearch);
    ASSERT_EQUAL(6, book.bookNodes.size());
    ASSERT_EQUAL(3, toSearch.size());
    ASSERT_EQUAL(n6Hash, pos.bookHash());
    UndoInfo ui3;
    pos.makeMove(e4, ui3);
    const U64 n4Hash = pos.bookHash();
    BookNode* n4 = book.getBookNode(n4Hash);
    ASSERT(n4);
    ASSERT(contains(toSearch, n4Hash));
    ASSERT(contains(toSearch, n6Hash));
    ASSERT(contains(toSearch, n3Hash));

    n4->setSearchResult(bd, nc6, -12, t);
    ASSERT_EQUAL(-12, n4->getNegaMaxScore());
    ASSERT_EQUAL(12, n6->getNegaMaxScore());
    ASSERT_EQUAL(-12, n5->getNegaMaxScore());
    ASSERT_EQUAL(12, n3->getNegaMaxScore());
    ASSERT_EQUAL(-8, n2->getNegaMaxScore());
    ASSERT_EQUAL(12, n1->getNegaMaxScore());

    moveList.clear();
    ASSERT(book.getPosition(n4Hash, pos2, moveList));
    ASSERT(pos2.equals(pos));
    ASSERT_EQUAL(3, moveList.size());
    ASSERT_EQUAL(d4, moveList[0]);
    ASSERT_EQUAL(nf6, moveList[1]);
    ASSERT_EQUAL(e4, moveList[2]);
}

void
BookBuildTest::testAddPosToBookConnectToChild() {
    Book book("");
    const BookData& bd = book.bookData;
    Position pos = TextIO::readFEN(TextIO::startPosFEN);
    const U64 n1Hash = pos.bookHash();

    std::vector<U64> toSearch;
    Move e4(TextIO::uciStringToMove("e2e4"));
    book.addPosToBook(pos, e4, toSearch);
    BookNode* n1 = book.getBookNode(n1Hash);
    Move nf3(TextIO::uciStringToMove("g1f3"));
    const int t = 10000;
    n1->setSearchResult(bd, nf3, 10, t);

    UndoInfo ui1;
    pos.makeMove(e4, ui1);
    const U64 n2Hash = pos.bookHash();
    BookNode* n2 = book.getBookNode(n2Hash);
    Move nc6(TextIO::uciStringToMove("b8c6"));
    n2->setSearchResult(bd, nc6, -8, t);

    Move nf6(TextIO::uciStringToMove("g8f6"));
    book.addPosToBook(pos, nf6, toSearch);
    UndoInfo ui2;
    pos.makeMove(nf6, ui2);
    const U64 n3Hash = pos.bookHash();
    BookNode* n3 = book.getBookNode(n3Hash);
    n3->setSearchResult(bd, nf3, 7, t);

    Move d4(TextIO::uciStringToMove("d2d4"));
    book.addPosToBook(pos, d4, toSearch);
    UndoInfo ui3;
    pos.makeMove(d4, ui3);
    const U64 n4Hash = pos.bookHash();
    BookNode* n4 = book.getBookNode(n4Hash);
    n4->setSearchResult(bd, nc6, -12, t);

    pos.unMakeMove(d4, ui3);
    pos.unMakeMove(nf6, ui2);
    pos.unMakeMove(e4, ui1);
    ASSERT_EQUAL(book.startPosHash, pos.bookHash());
    book.addPosToBook(pos, d4, toSearch);
    pos.makeMove(d4, ui1);
    const U64 n5Hash = pos.bookHash();
    BookNode* n5 = book.getBookNode(n5Hash);
    n5->setSearchResult(bd, nc6, -12, t);

    book.addPosToBook(pos, nf6, toSearch);
    pos.makeMove(nf6, ui2);
    const U64 n6Hash = pos.bookHash();
    BookNode* n6 = book.getBookNode(n6Hash);
    ASSERT_EQUAL(1, n6->getChildren().size());
    ASSERT_EQUAL(2, n4->getParents().size());
    n6->setSearchResult(bd, nf3, 11, t);

    ASSERT_EQUAL(-12, n4->getNegaMaxScore());
    ASSERT_EQUAL(12, n6->getNegaMaxScore());
    ASSERT_EQUAL(-12, n5->getNegaMaxScore());
    ASSERT_EQUAL(12, n3->getNegaMaxScore());
    ASSERT_EQUAL(-8, n2->getNegaMaxScore());
    ASSERT_EQUAL(12, n1->getNegaMaxScore());
}

void
BookBuildTest::testSelector() {
    TranspositionTable tt(27);
    auto system = [](const std::string& cmd) { ::system(cmd.c_str()); };
    std::string tmpDir = "/tmp/booktest";
    system("mkdir -p " + tmpDir);
    system("rm " + tmpDir + "/* 2>/dev/null");

    {
        std::string backupFile = tmpDir + "/backup";
        Book book(backupFile);
        class TestSelector : public Book::PositionSelector {
        public:
            TestSelector(Book& b) : book(b) { }
            bool getNextPosition(Position& pos, Move& move) override {
                nCalls++;
                return false;
            }
            int nCalls = 0;
        private:
            Book& book;
        };
        TestSelector selector(book);
        int searchTime = 10;
        int nThreads = 1;
        book.extendBook(selector, searchTime, nThreads, tt);
        ASSERT_EQUAL(1, selector.nCalls);
        ASSERT_EQUAL(1, book.bookNodes.size());

        Book book2("");
        book2.readFromFile(backupFile);
        ASSERT_EQUAL(1, book2.bookNodes.size());
    }
    {
        std::string backupFile = tmpDir + "/backup";
        Book book(backupFile);
        class TestSelector : public Book::PositionSelector {
        public:
            TestSelector(Book& b) : book(b) { }
            bool getNextPosition(Position& pos, Move& move) override {
                nCalls++;
                if (idx < (int)bookLine.size()) {
                    Move m = TextIO::stringToMove(currPos, bookLine[idx]);
                    pos = currPos;
                    move = m;
                    UndoInfo ui;
                    currPos.makeMove(m, ui);
                    idx++;
                    return true;
                }
                return false;
            }
            int nCalls = 0;
        private:
            Book& book;
            Position currPos = TextIO::readFEN(TextIO::startPosFEN);
            std::vector<std::string> bookLine { "e4", "e5", "Nf3", "Nc6", "Bb5", "a6", "Ba4", "b5" };
            int idx = 0;
        };
        TestSelector selector(book);
        int searchTime = 10;
        int nThreads = 1;
        book.extendBook(selector, searchTime, nThreads, tt);
        ASSERT(selector.nCalls >= 9);
        ASSERT_EQUAL(9, book.bookNodes.size());
    }
}

cute::suite
BookBuildTest::getSuite() const {
    cute::suite s;
    s.push_back(CUTE(testBookNode));
    s.push_back(CUTE(testShortestDepth));
    s.push_back(CUTE(testBookNodeDAG));
    s.push_back(CUTE(testAddPosToBook));
    s.push_back(CUTE(testAddPosToBookConnectToChild));
    s.push_back(CUTE(testSelector));
    return s;
}
