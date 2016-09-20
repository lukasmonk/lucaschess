/*
    Texel - A UCI chess engine.
    Copyright (C) 2012  Peter Österlund, peterosterlund2@gmail.com

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
 * uciprotocol.cpp
 *
 *  Created on: Mar 4, 2012
 *      Author: petero
 */

#include "uciprotocol.hpp"
#include "searchparams.hpp"
#include "computerPlayer.hpp"
#include "textio.hpp"

#include <iostream>


void
UCIProtocol::main(bool autoStart) {
    UCIProtocol uciProt;
    uciProt.mainLoop(std::cin, std::cout, autoStart);
}

UCIProtocol::UCIProtocol()
    : pos(TextIO::readFEN(TextIO::startPosFEN)),
      quit(false)
{
}

void
UCIProtocol::mainLoop(std::istream& is, std::ostream& os, bool autoStart) {
    if (autoStart)
        handleCommand("uci", os);
    std::string line;
    while (true) {
        getline(is, line);
        if (!is.good()) {
            if (engine)
                engine->stopSearch();
            break;
        }
        handleCommand(line, os);
        if (quit)
            break;
    }
}

void
UCIProtocol::handleCommand(const std::string& cmdLine, std::ostream& os) {
    std::vector<std::string> tokens;
    tokenize(cmdLine, tokens);
    const int nTok = (int)tokens.size();
    if (nTok == 0)
        return;
    try {
        std::string cmd = tokens[0];
        if (cmd == "uci") {
            os << "id name " << ComputerPlayer::engineName << std::endl;
            os << "id author Peter Osterlund" << std::endl;
            EngineControl::printOptions(os);
            os << "uciok" << std::endl;
        } else if (cmd == "isready") {
            initEngine(os);
            os << "readyok" << std::endl;
        } else if (cmd == "setoption") {
            initEngine(os);
            std::string optionName;
            std::string optionValue;
            if (nTok < 2)
                return;
            if (tokens[1] == "name") {
                int idx = 2;
                while ((idx < nTok) && (tokens[idx] != "value")) {
                    optionName += toLowerCase(tokens[idx++]);
                    optionName += ' ';
                }
                if ((idx < nTok) && (tokens[idx++] == "value")) {
                    while ((idx < nTok)) {
                        optionValue += toLowerCase(tokens[idx++]);
                        optionValue += ' ';
                    }
                }
                engine->setOption(trim(optionName), trim(optionValue), true);
            }
        } else if (cmd == "ucinewgame") {
            if (engine)
                engine->newGame();
        } else if (cmd ==  "position") {
            std::string fen;
            int idx = 1;
            if (nTok < 2)
                return;
            if (tokens[idx] == "startpos") {
                idx++;
                fen = TextIO::startPosFEN;
            } else if (tokens[idx] == "fen") {
                idx++;
                std::string sb;
                while ((idx < nTok) && (tokens[idx] != "moves")) {
                    sb += tokens[idx++];
                    sb += ' ';
                }
                fen = trim(sb);
            }
            if (fen.length() > 0) {
                pos = TextIO::readFEN(fen);
                moves.clear();
                if ((idx < nTok) && (tokens[idx++] == "moves")) {
                    for (int i = idx; i < nTok; i++) {
                        Move m = TextIO::uciStringToMove(tokens[i]);
                        if (m.isEmpty())
                            break;
                        moves.push_back(m);
                    }
                }
            }
        } else if (cmd == "go") {
            initEngine(os);
            int idx = 1;
            SearchParams sPar;
            bool ponder = false;
            while (idx < nTok) {
                std::string subCmd = tokens[idx++];
                if (subCmd == "searchmoves") {
                    while (idx < nTok) {
                        Move m = TextIO::uciStringToMove(tokens[idx]);
                        if (m.isEmpty())
                            break;
                        sPar.searchMoves.push_back(m);
                        idx++;
                    }
                } else if (subCmd == "ponder") {
                    ponder = true;
                } else if (subCmd == "wtime") {
                    if (idx < nTok)
                        str2Num(tokens[idx++], sPar.wTime);
                } else if (subCmd == "btime") {
                    if (idx < nTok)
                        str2Num(tokens[idx++], sPar.bTime);
                } else if (subCmd == "winc") {
                    if (idx < nTok)
                        str2Num(tokens[idx++], sPar.wInc);
                } else if (subCmd == "binc") {
                    if (idx < nTok)
                        str2Num(tokens[idx++], sPar.bInc);
                } else if (subCmd == "movestogo") {
                    if (idx < nTok)
                        str2Num(tokens[idx++], sPar.movesToGo);
                } else if (subCmd == "depth") {
                    if (idx < nTok)
                        str2Num(tokens[idx++], sPar.depth);
                } else if (subCmd == "nodes") {
                    if (idx < nTok)
                        str2Num(tokens[idx++], sPar.nodes);
                } else if (subCmd == "mate") {
                    if (idx < nTok)
                        str2Num(tokens[idx++], sPar.mate);
                } else if (subCmd == "movetime") {
                    if (idx < nTok)
                        str2Num(tokens[idx++], sPar.moveTime);
                } else if (subCmd == "infinite") {
                    sPar.infinite = true;
                }
            }
            if (ponder) {
                engine->startPonder(pos, moves, sPar);
            } else {
                engine->startSearch(pos, moves, sPar);
            }
        } else if (cmd == "stop") {
            if (engine)
                engine->stopSearch();
        } else if (cmd == "ponderhit") {
            engine->ponderHit();
        } else if (cmd == "quit") {
            if (engine)
                engine->stopSearch();
            quit = true;
        }
    } catch (const ChessParseError&) {
    }
}

void
UCIProtocol::initEngine(std::ostream& os) {
    if (!engine)
        engine = std::make_shared<EngineControl>(os);
}

/** Convert a string to tokens by splitting at whitespace characters. */
void
UCIProtocol::tokenize(const std::string& cmdLine, std::vector<std::string>& tokens) {
    tokens.clear();
    std::string tmp = trim(cmdLine);
    int start = 0;
    bool inWord = true;
    for (int i = 0; i < (int)tmp.size(); i++) {
        if (inWord) {
            if (isspace(tmp[i])) {
                tokens.push_back(tmp.substr(start, i - start));
                inWord = false;
            }
        } else {
            if (!isspace(tmp[i])) {
                start = i;
                inWord = true;
            }
        }
    }
    if (inWord)
        tokens.push_back(tmp.substr(start, tmp.size() - start));
}
