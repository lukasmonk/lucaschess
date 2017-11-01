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
 * gametree.cpp
 *
 *  Created on: Dec 22, 2013
 *      Author: petero
 */

#include "gametree.hpp"
#include <stdexcept>
#include <cassert>
#include <unordered_map>

// --------------------------------------------------------------------------------

PgnToken::PgnToken(int type0, const std::string& token0)
    : type(type0), token(token0) {
}

// --------------------------------------------------------------------------------

PgnScanner::PgnScanner(std::istream& is0)
    : is(is0), col0(true), eofReached(false),
      hasReturnedChar(false), returnedChar(0) {
}

void
PgnScanner::putBack(const PgnToken& tok) {
    savedTokens.push_back(tok);
}

char
PgnScanner::getNextChar() {
    int c;
    c = is.get();
    if (eofReached || (c == EOF))
        throw std::out_of_range("");
    return c;
}

char
PgnScanner::getTokenChar() {
    if (hasReturnedChar) {
        hasReturnedChar = false;
        return returnedChar;
    }
    try {
        while (true) {
            char c = getNextChar();
            if (c == '%' && col0) {
                while (true) {
                    char nextChar = getNextChar();
                    if ((nextChar == '\n') || (nextChar == '\r'))
                        break;
                }
                col0 = true;
            } else {
                col0 = ((c == '\n') || (c == '\r'));
                return c;
            }
        }
    } catch (const std::out_of_range& e) {
        if (eofReached)
            throw e;
        eofReached = true;
        return '\n'; // Terminating whitespace simplifies the tokenizer
    }
}

void
PgnScanner::returnTokenChar(char c) {
    assert(!hasReturnedChar);
    hasReturnedChar = true;
    returnedChar = c;
}

PgnToken
PgnScanner::nextToken() {
    if (savedTokens.size() > 0) {
        int len = savedTokens.size();
        PgnToken ret = savedTokens[len - 1];
        savedTokens.pop_back();
        return ret;
    }

    PgnToken ret(PgnToken::END, "");
    try {
        while (true) {
            char c = getTokenChar();
            if (isspace(c)) {
                // Skip
            } else if (c == '.') {
                ret.type = PgnToken::PERIOD;
                break;
            } else if (c == '*') {
                ret.type = PgnToken::ASTERISK;
                break;
            } else if (c == '[') {
                ret.type = PgnToken::LEFT_BRACKET;
                break;
            } else if (c == ']') {
                ret.type = PgnToken::RIGHT_BRACKET;
                break;
            } else if (c == '(') {
                ret.type = PgnToken::LEFT_PAREN;
                break;
            } else if (c == ')') {
                ret.type = PgnToken::RIGHT_PAREN;
                break;
            } else if (c == '{') {
                ret.type = PgnToken::COMMENT;
                std::string sb;
                while ((c = getTokenChar()) != '}')
                    sb += c;
                ret.token = sb;
                break;
            } else if (c == ';') {
                ret.type = PgnToken::COMMENT;
                std::string sb;
                while (true) {
                    c = getTokenChar();
                    if ((c == '\n') || (c == '\r'))
                        break;
                    sb += c;
                }
                ret.token = sb;
                break;
            } else if (c == '"') {
                ret.type = PgnToken::STRING;
                std::string sb;
                while (true) {
                    c = getTokenChar();
                    if (c == '"') {
                        break;
                    } else if (c == '\\') {
                        c = getTokenChar();
                    }
                    sb += c;
                }
                ret.token = sb;
                break;
            } else if (c == '$') {
                ret.type = PgnToken::NAG;
                std::string sb;
                while (true) {
                    c = getTokenChar();
                    if (!isdigit(c)) {
                        returnTokenChar(c);
                        break;
                    }
                    sb += c;
                }
                ret.token = sb;
                break;
            } else { // Start of symbol or integer
                ret.type = PgnToken::SYMBOL;
                std::string sb;
                sb += c;
                bool onlyDigits = isdigit(c);
                const std::string term = ".*[](){;\"$";
                while (true) {
                    c = getTokenChar();
                    if (isspace(c) || (term.find_first_of(c) != std::string::npos)) {
                        returnTokenChar(c);
                        break;
                    }
                    sb += c;
                    if (!isdigit(c))
                        onlyDigits = false;
                }
                if (onlyDigits)
                    ret.type = PgnToken::INTEGER;
                ret.token = sb;
                break;
            }
        }
    } catch (const std::out_of_range& e) {
        ret.type = PgnToken::END;
    }
    return ret;
}

PgnToken
PgnScanner::nextTokenDropComments() {
    while (true) {
        PgnToken tok = nextToken();
        if (tok.type != PgnToken::COMMENT)
            return tok;
    }
}

// --------------------------------------------------------------------------------

Node::Node(const std::shared_ptr<Node>& parent0, const Move& m, const UndoInfo& ui0, int nag0,
           const std::string& preComment0, const std::string& postComment0)
    : move(m), ui(ui0), nag(nag0), preComment(preComment0),
      postComment(postComment0), parent(parent0) {
}

void
Node::insertMove(Position& pos, std::shared_ptr<Node>& node, const Move& move) {
    std::shared_ptr<Node> child(std::make_shared<Node>());
    child->parent = node;
    node->children.push_back(child);
    child->move = move;
    pos.makeMove(move, child->ui);
    pos.unMakeMove(move, child->ui);
}

void
Node::addChild(Position& pos, std::shared_ptr<Node>& node, std::shared_ptr<Node>& child) {
    child->parent = node;
    node->children.push_back(child);
    node = child;
    child = std::make_shared<Node>();
    pos.makeMove(node->getMove(), node->ui);
}

void
Node::parsePgn(PgnScanner& scanner, Position pos, std::shared_ptr<Node> node) {
    std::shared_ptr<Node> nodeToAdd(std::make_shared<Node>());
    bool moveAdded = false;
    while (true) {
        PgnToken tok = scanner.nextToken();
        switch (tok.type) {
        case PgnToken::INTEGER:
        case PgnToken::PERIOD:
            break;
        case PgnToken::LEFT_PAREN:
            if (moveAdded) {
                addChild(pos, node, nodeToAdd);
                moveAdded = false;
            }
            if (node->getParent()) {
                Position pos2(pos);
                pos2.unMakeMove(node->getMove(), node->getUndoInfo());
                parsePgn(scanner, pos2, node->getParent());
            } else {
                int nestLevel = 1;
                while (nestLevel > 0) {
                    switch (scanner.nextToken().type) {
                    case PgnToken::LEFT_PAREN: nestLevel++; break;
                    case PgnToken::RIGHT_PAREN: nestLevel--; break;
                    case PgnToken::END: return; // Broken PGN file. Just give up.
                    }
                }
            }
            break;
        case PgnToken::NAG:
            if (moveAdded) { // NAG must be after move
                if (!str2Num(tok.token, nodeToAdd->nag))
                    nodeToAdd->nag = 0;
            }
            break;
        case PgnToken::SYMBOL: {
            if ((tok.token =="1-0") || (tok.token == "0-1") ||
                (tok.token == "1/2-1/2") || (tok.token == "*")) {
                if (moveAdded)
                    addChild(pos, node, nodeToAdd);
                return;
            }
            char lastChar = tok.token[tok.token.length() - 1];
            if (lastChar == '+')
                tok.token = tok.token.substr(0, tok.token.length() - 1);
            if ((lastChar == '!') || (lastChar == '?')) {
                int movLen = tok.token.length() - 1;
                while (movLen > 0) {
                    char c = tok.token[movLen - 1];
                    if ((c == '!') || (c == '?'))
                        movLen--;
                    else
                        break;
                }
                std::string ann = tok.token.substr(movLen);
                tok.token = tok.token.substr(0, movLen);
                int nag = 0;
                if      (ann == "!")  nag = 1;
                else if (ann == "?")  nag = 2;
                else if (ann == "!!") nag = 3;
                else if (ann == "??") nag = 4;
                else if (ann == "!?") nag = 5;
                else if (ann == "?!") nag = 6;
                if (nag > 0)
                    scanner.putBack(PgnToken(PgnToken::NAG, num2Str(nag)));
            }
            if (tok.token.length() > 0) {
                if (moveAdded) {
                    addChild(pos, node, nodeToAdd);
                    moveAdded = false;
                }
                nodeToAdd->move = TextIO::stringToMove(pos, tok.token);
                if (nodeToAdd->move.isEmpty()) {
                    std::cerr << TextIO::asciiBoard(pos) << " wtm:" << (pos.isWhiteMove()?1:0) << " move:" << tok.token << std::endl;
                    throw ChessParseError("Invalid move");
                }
                moveAdded = true;
            }
            break;
        }
        case PgnToken::COMMENT:
            if (moveAdded)
                nodeToAdd->postComment += tok.token;
            else
                nodeToAdd->preComment += tok.token;
            break;
        case PgnToken::ASTERISK:
        case PgnToken::LEFT_BRACKET:
        case PgnToken::RIGHT_BRACKET:
        case PgnToken::STRING:
        case PgnToken::RIGHT_PAREN:
        case PgnToken::END:
            if (moveAdded)
                addChild(pos, node, nodeToAdd);
            return;
        }
    }
}

// --------------------------------------------------------------------------------

GameNode::GameNode(const Position& pos, const std::shared_ptr<Node>& root)
    : rootNode(root), currPos(pos), currNode(root) {
}

GameNode::GameNode(const Position& pos, const std::shared_ptr<Node>& root,
                   const std::shared_ptr<Node>& node)
    : rootNode(root), currPos(pos), currNode(node) {
}

const Position&
GameNode::getPos() const {
    return currPos;
}

const std::shared_ptr<Node>&
GameNode::getNode() const {
    return currNode;
}

const Move&
GameNode::getMove() const {
    return currNode->getMove();
}

std::string
GameNode::getComment() const {
    std::string pre = currNode->getPreComment();
    std::string post = currNode->getPostComment();
    if ((pre.length() > 0) && (post.length() > 0))
        pre += " ";
    return pre + post;
}

bool
GameNode::goBack() {
    std::shared_ptr<Node> parent = currNode->getParent();
    if (!parent)
        return false;
    currPos.unMakeMove(currNode->getMove(), currNode->getUndoInfo());
    currNode = parent;
    return true;
}

int
GameNode::nChildren() const {
    return currNode->getChildren().size();
}

void
GameNode::goForward(int i) {
    std::shared_ptr<Node> next = currNode->getChildren()[i];
    UndoInfo ui;
    currPos.makeMove(next->getMove(), ui);
    currNode = next;
}

void
GameNode::insertMove(const Move& move) {
    Node::insertMove(currPos, currNode, move);
}

// --------------------------------------------------------------------------------

GameTree::GameTree() {
    setStartPos(TextIO::readFEN(TextIO::startPosFEN));
}

void
GameTree::setStartPos(const Position& pos) {
    event = "?";
    site = "?";
    date = "?";
    round = "?";
    white = "?";
    black = "?";
    result = "?";
    startPos = pos;
    tagPairs.clear();
    rootNode = std::make_shared<Node>();
}

void
GameTree::setTagPairs(const std::vector<GameTree::TagPair>& tPairs) {
    // Store parsed data in GameTree
    const int nTags = tPairs.size();
    for (int i = 0; i < nTags; i++) {
        const std::string& name = tPairs[i].tagName;
        const std::string& val = tPairs[i].tagValue;
        if ((name == "FEN") || (name == "Setup")) {
            // Already handled
        } else if (name == "Event") {
            event = val;
        } else if (name == "Site") {
            site = val;
        } else if (name == "Date") {
            date = val;
        } else if (name == "Round") {
            round = val;
        } else if (name == "White") {
            white = val;
        } else if (name == "Black") {
            black = val;
        } else if (name == "Result") {
            result = val;
        } else {
            tagPairs.push_back(tPairs[i]);
        }
    }
}

void
GameTree::setRootNode(const std::shared_ptr<Node>& gameRoot) {
    rootNode = gameRoot;
}

void
GameTree::insertMoves(const std::vector<Move>& moves) {
    GameNode dstNode = getRootNode();
    for (const Move& m : moves) {
        std::unordered_map<U16, int> dstMoves; // Compressed move -> move index
        for (int i = 0; i < dstNode.nChildren(); i++) {
            dstNode.goForward(i);
            dstMoves[dstNode.getMove().getCompressedMove()] = i;
            dstNode.goBack();
        }
        U16 cMove = m.getCompressedMove();
        if (dstMoves.find(cMove) == dstMoves.end()) {
            dstMoves[cMove] = dstNode.nChildren();
            dstNode.insertMove(m);
        }
        dstNode.goForward(dstMoves[cMove]);
    }
}

void
GameTree::insertTree(const GameTree& src, int maxPly) {
    GameNode srcNode = src.getRootNode();
    GameNode dstNode = getRootNode();
    std::function<void(int)> iterateTree = [&](int ply) {
        if (maxPly >= 0 && ply >= maxPly)
            return;
        std::unordered_map<U16, int> dstMoves; // Compressed move -> move index
        for (int i = 0; i < dstNode.nChildren(); i++) {
            dstNode.goForward(i);
            dstMoves[dstNode.getMove().getCompressedMove()] = i;
            dstNode.goBack();
        }
        for (int i = 0; i < srcNode.nChildren(); i++) {
            srcNode.goForward(i);
            U16 cMove = srcNode.getMove().getCompressedMove();
            if (dstMoves.find(cMove) == dstMoves.end()) {
                dstMoves[cMove] = dstNode.nChildren();
                dstNode.insertMove(srcNode.getMove());
            }
            dstNode.goForward(dstMoves[cMove]);
            iterateTree(ply + 1);
            srcNode.goBack();
            dstNode.goBack();
        }
    };
    iterateTree(0);
}

GameTree::Result
GameTree::getResult() const {
    if (result =="1-0")
        return WHITE_WIN;
    else if (result == "0-1")
        return BLACK_WIN;
    else if (result == "1/2-1/2")
        return DRAW;
    else
        return UNKNOWN;
}

void
GameTree::getHeaders(std::map<std::string, std::string>& headers) {
    headers.insert(std::make_pair("Event", event));
    headers.insert(std::make_pair("Site",  site));
    headers.insert(std::make_pair("Date",  date));
    headers.insert(std::make_pair("Round", round));
    headers.insert(std::make_pair("White", white));
    headers.insert(std::make_pair("Black", black));
    for (size_t i = 0; i < tagPairs.size(); i++) {
        TagPair tp = tagPairs[i];
        headers.insert(std::make_pair(tp.tagName, tp.tagValue));
    }
}

GameNode
GameTree::getRootNode() const {
    return GameNode(startPos, rootNode);
}

GameNode
GameTree::getNode(const std::shared_ptr<Node>& node) {
    std::vector<Move> moves;
    std::shared_ptr<Node> n = node;
    while (true) {
        Move m = n->getMove();
        if (m.isEmpty())
            break;
        moves.push_back(m);
        n = n->getParent();
    }
    std::reverse(moves.begin(), moves.end());

    Position pos = startPos;
    UndoInfo ui;
    for (const Move& m : moves)
        pos.makeMove(m, ui);

    return GameNode(pos, rootNode, node);
}

void
GameTree::getGameTreeString(std::string& str, std::set<RangeToNode>& posToNodes) {
    str.clear();
    posToNodes.clear();
    GameNode gn = getRootNode();
    std::function<void()> iterateTree = [&] {
        if (gn.nChildren() == 0)
            return;
        if (!str.empty())
            str += ' ';
        for (int i = 0; i < gn.nChildren(); i++) {
            gn.goForward(i);
            if (i > 0)
                str += " (";
            Move m = gn.getMove();
            gn.goBack();
            int b = str.length();
            str += TextIO::moveToString(gn.getPos(), m, false);
            int e = str.length();
            gn.goForward(i);
            posToNodes.insert(RangeToNode(b, e, gn.getNode()));
            if (i > 0) {
                iterateTree();
                str += ")";
            }
            gn.goBack();
        }
        gn.goForward(0);
        iterateTree();
        gn.goBack();
    };
    iterateTree();
}

// --------------------------------------------------------------------------------

PgnReader::PgnReader(std::istream& is)
    : scanner(is) {
}

bool
PgnReader::readPGN(GameTree& tree) {
    PgnToken tok = scanner.nextToken();

    // Parse tag section
    using TagPair = GameTree::TagPair;
    std::vector<TagPair> tPairs;
    while (tok.type == PgnToken::LEFT_BRACKET) {
        TagPair tp;
        tok = scanner.nextTokenDropComments();
        if (tok.type != PgnToken::SYMBOL)
            break;
        tp.tagName = tok.token;
        tok = scanner.nextTokenDropComments();
        if (tok.type != PgnToken::STRING)
            break;
        tp.tagValue = tok.token;
        tok = scanner.nextTokenDropComments();
        if (tok.type != PgnToken::RIGHT_BRACKET) {
            // In a well-formed PGN, there is nothing between the string
            // and the right bracket, but broken headers with non-escaped
            // " characters sometimes occur. Try to do something useful
            // for such headers here.
            PgnToken prevTok(PgnToken::STRING, "");
            while ((tok.type == PgnToken::STRING) || (tok.type == PgnToken::SYMBOL)) {
                if (tok.type != prevTok.type)
                    tp.tagValue += '"';
                if ((tok.type == PgnToken::SYMBOL) && (prevTok.type == PgnToken::SYMBOL))
                    tp.tagValue += ' ';
                tp.tagValue += tok.token;
                prevTok = tok;
                tok = scanner.nextTokenDropComments();
            }
        }
        tPairs.push_back(tp);
        tok = scanner.nextToken();
    }
    scanner.putBack(tok);

    std::string fen = TextIO::startPosFEN;
    int nTags = tPairs.size();
    for (int i = 0; i < nTags; i++)
        if (tPairs[i].tagName == "FEN")
            fen = tPairs[i].tagValue;
    Position startPos(TextIO::readFEN(fen));
    tree.setStartPos(startPos);

    // Parse move section
    std::shared_ptr<Node> gameRoot(std::make_shared<Node>());
    Node::parsePgn(scanner, startPos, gameRoot);

    if ((tPairs.size() == 0) && (gameRoot->getChildren().size() == 0))
        return false;

    tree.setTagPairs(tPairs);
    tree.setRootNode(gameRoot);

    return true;
}
