/*
    Texel - A UCI chess engine.
    Copyright (C) 2013-2016  Peter Österlund, peterosterlund2@gmail.com

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
 * gametree.hpp
 *
 *  Created on: Dec 22, 2013
 *      Author: petero
 */

#ifndef GAMETREE_HPP_
#define GAMETREE_HPP_

#include "textio.hpp"
#include "util/util.hpp"

#include <map>
#include <memory>
#include <vector>
#include <set>

/** A token in a PGN data stream. Used by the PGN parser. */
class PgnToken {
public:
    PgnToken(int type, const std::string& token);

    // These are tokens according to the PGN spec
    static const int STRING = 0;
    static const int INTEGER = 1;
    static const int PERIOD = 2;
    static const int ASTERISK = 3;
    static const int LEFT_BRACKET = 4;
    static const int RIGHT_BRACKET = 5;
    static const int LEFT_PAREN = 6;
    static const int RIGHT_PAREN = 7;
    static const int NAG = 8;
    static const int SYMBOL = 9;

    // These are not tokens according to the PGN spec, but the parser
    // extracts these anyway for convenience.
    static const int COMMENT = 10;
    static const int END = 11;

    // Actual token data
    int type;
    std::string token;
};


class PgnScanner {
public:
    PgnScanner(std::istream& is);

    void putBack(const PgnToken& tok);

    PgnToken nextToken();

    PgnToken nextTokenDropComments();

    char getNextChar();
    char getTokenChar();
    void returnTokenChar(char c);

private:
    std::istream& is;
    bool col0;
    bool eofReached;
    bool hasReturnedChar;
    char returnedChar;
    std::vector<PgnToken> savedTokens;
};


/**
 *  A node object represents a position in the game tree.
 *  The position is defined by the move that leads to the position from the parent position.
 *  The root node is special in that it doesn't have a move.
 */
class Node {
public:
    Node() : nag(0) { }
    Node(const std::shared_ptr<Node>& parent, const Move& m, const UndoInfo& ui, int nag,
         const std::string& preComment, const std::string& postComment);

    std::shared_ptr<Node> getParent() const { return parent.lock(); }
    const std::vector<std::shared_ptr<Node>>& getChildren() const { return children; }
    const Move& getMove() const { return move; }
    const UndoInfo& getUndoInfo() const { return ui; }
    const std::string& getPreComment() const { return preComment; }
    const std::string& getPostComment() const { return postComment; }

    /** Add a child node given by move. */
    static void insertMove(Position& pos, std::shared_ptr<Node>& node, const Move& move);

    static void parsePgn(PgnScanner& scanner, Position pos, std::shared_ptr<Node> node);

private:
    static void addChild(Position& pos, std::shared_ptr<Node>& node,
                         std::shared_ptr<Node>& child);

    Move move;                  // Move leading to this node. Empty in root node.
    UndoInfo ui;                // UndoInfo needed to get to parent position.
    int nag;                    // Numeric annotation glyph
    std::string preComment;     // Comment before move
    std::string postComment;    // Comment after move

    std::weak_ptr<Node> parent; // Null if root node
    std::vector<std::shared_ptr<Node>> children;
};


/** A GameNode acts as a tree iterator. */
class GameNode {
public:
    GameNode(const Position& pos, const std::shared_ptr<Node>& root);
    GameNode(const Position& pos, const std::shared_ptr<Node>& root,
             const std::shared_ptr<Node>& node);

    /** Get current position. */
    const Position& getPos() const;

    /** Get the Node corresponding to the current position. */
    const std::shared_ptr<Node>& getNode() const;

    /** Get the move leading to this position. */
    const Move& getMove() const;

    /** Get preComment + postComment move leading to this position. */
    std::string getComment() const;


    /** Go to parent position, unless already at root.
     * @return True if there was a parent position. */
    bool goBack();

    /** Get number of moves in this position. */
    int nChildren() const;

    /** Go to i:th child position. */
    void goForward(int i);

    /** Add a move to the game tree. The move is inserted even if a child
     *  with the same move is already present. The inserted move is added
     *  last in the list of moves for this position. */
    void insertMove(const Move& move);

private:
    std::shared_ptr<Node> rootNode; // To prevent tree from being deleted too early
    Position currPos;
    std::shared_ptr<Node> currNode;
};


class GameTree {
public:
    /** Creates an empty GameTree starting at the standard start position. */
    GameTree();

    enum Result {
        WHITE_WIN,
        DRAW,
        BLACK_WIN,
        UNKNOWN,
    };

    /** Set start position. Drops the whole game tree. */
    void setStartPos(const Position& pos);

    struct TagPair {
        std::string tagName;
        std::string tagValue;
    };

    /** Set PGN tag pairs. */
    void setTagPairs(const std::vector<TagPair>& tPairs);

    /** Set the tree root node. */
    void setRootNode(const std::shared_ptr<Node>& gameRoot);

    /** Insert a sequence of moves in this tree. Moves already present
     *  in the three are not duplicated. The first move must correspond
     *  to the start position of this tree. */
    void insertMoves(const std::vector<Move>& moves);

    /** Insert all positions from src up to ply maxPly that are not
     *  already in this tree. The src tree and this tree must have
     *  the same starting position. If maxPly is negative, no ply
     *  limit is used. */
    void insertTree(const GameTree& src, int maxPly);


    /** Get game result. */
    Result getResult() const;

    /** Get PGN header tags and values. */
    void getHeaders(std::map<std::string, std::string>& headers);

    /** Get node corresponding to start position. */
    GameNode getRootNode() const;

    /** Get GameNode with current position set to node. */
    GameNode getNode(const std::shared_ptr<Node>& node);

    /** Mapping from character range to tree node. */
    struct RangeToNode {
        const int begin;
        const int end;
        const std::shared_ptr<Node> node;

        RangeToNode(int b, int e, const std::shared_ptr<Node>& n)
            : begin(b), end(e), node(n) {}
        bool operator<(const RangeToNode& other) const { return begin < other.begin; }
    };

    /** Convert the game tree to string representation, using PGN syntax for variations,
     *  but without line breaks, comments and move numbers. Also returns a mapping
     *  from positions in the string to corresponding tree nodes. */
    void getGameTreeString(std::string& str, std::set<RangeToNode>& posToNodes);

private:
    // Data from the seven tag roster (STR) part of the PGN standard
    std::string event, site, date, round, white, black, result;

    // Non-standard tags
    std::vector<TagPair> tagPairs;

    Position startPos;
    std::shared_ptr<Node> rootNode;
};


class PgnReader {
public:
    PgnReader(std::istream& is);

    /** Read next game. Return false if no more games to read. */
    bool readPGN(GameTree& tree);

private:
    PgnScanner scanner;
};

#endif /* GAMETREE_HPP_ */
