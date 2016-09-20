/*
 * DiscoCheck, an UCI chess engine. Copyright (C) 2011-2013 Lucas Braesch.
 *
 * DiscoCheck is free software: you can redistribute it and/or modify it under the terms of the GNU
 * General Public License as published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 *
 * DiscoCheck is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without
 * even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with this program. If not,
 * see <http://www.gnu.org/licenses/>.
*/
#include <sstream>
#include <stdio.h>
#include "uci.h"
#include "search.h"
#include "eval.h"
#include "test.h"

#if defined(_WIN32) || defined(_WIN64)
#include <windows.h>
#include <conio.h>
#else   // assume POSIX
#include <cerrno>
#include <unistd.h>
#include <sys/time.h>
#endif

namespace uci {

int Hash = 16;
int Contempt = 25;
bool Ponder = false;
int TimeBuffer = 100;

}	// namespace uci

namespace {

const char* StartFEN = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";

void intro()
{
	std::cout << "id name DiscoCheck 5.2.1\n"
		<< "id author Lucas Braesch\n"
		// Declare UCI options here
		<< "option name Hash type spin default " << uci::Hash << " min 1 max 8192\n"
		<< "option name Clear Hash type button\n"
		<< "option name Contempt type spin default " << uci::Contempt << " min 0 max 100\n"
		<< "option name Ponder type check default " << uci::Ponder << '\n'
		<< "option name Time Buffer type spin default " << uci::TimeBuffer << " min 0 max 1000\n"
		// end of UCI options
		<< "uciok" << std::endl;
}

void position(board::Board& B, std::istringstream& is)
{
	move::move_t m;
	std::string token, fen;
	is >> token;

	if (token == "startpos") {
		fen = StartFEN;
		is >> token;	// Consume "moves" token if any
	} else if (token == "fen")
		while (is >> token && token != "moves")
			fen += token + " ";
	else
		return;

	B.set_fen(fen);

	// Parse move list (if any)
	while (is >> token) {
		m = move::string_to_move(B, token);
		B.play(m);
	}
}

void go(board::Board& B, std::istringstream& is)
{
	search::Limits sl;
	std::string token;

	while (is >> token) {
		if (token == (B.get_turn() ? "btime" : "wtime"))
			is >> sl.time;
		else if (token == (B.get_turn() ? "binc" : "winc"))
			is >> sl.inc;
		else if (token == "movestogo")
			is >> sl.movestogo;
		else if (token == "movetime")
			is >> sl.movetime;
		else if (token == "depth")
			is >> sl.depth;
		else if (token == "nodes")
			is >> sl.nodes;
		else if (token == "ponder")
			sl.ponder = true;
	}

	// best and ponder move
	std::pair<move::move_t, move::move_t> best = bestmove(B, sl);
	std::cout << "bestmove " << move_to_string(best.first);
	if (best.second)
		std::cout << " ponder " << move_to_string(best.second);
	std::cout << std::endl;
}

void setoption(std::istringstream& is)
{
	std::string token, name;
	if (!(is >> token) || token != "name")
		return;

	while (is >> token && token != "value")
		name += token;

	/* UCI option 'name' has been modified. Handle here. */
	if (name == "Hash")
		is >> uci::Hash;
	else if (name == "ClearHash")
		search::clear_state();
	else if (name == "Contempt")
		is >> uci::Contempt;
	else if (name == "Ponder")
		is >> uci::Ponder;
	else if (name == "TimeBuffer")
		is >> uci::TimeBuffer;
}

bool input_available()
{
#if defined(_WIN32) || defined(_WIN64)

	static bool init = false, is_pipe;
	static HANDLE stdin_h;
	DWORD val;

	if (!init) {
		init = true;
		stdin_h = GetStdHandle(STD_INPUT_HANDLE);
		is_pipe = !GetConsoleMode(stdin_h, &val);
	}

	if (stdin->_cnt > 0)
		return true;

	if (is_pipe)
		return !PeekNamedPipe(stdin_h, nullptr, 0, nullptr, &val, nullptr) ? true : val > 0;
	else
		return _kbhit();

#else   // assume POSIX

	fd_set readfds;
	FD_ZERO(&readfds);
	FD_SET(STDIN_FILENO, &readfds);

	struct timeval timeout;
	timeout.tv_sec = timeout.tv_usec = 0;

	select(STDIN_FILENO + 1, &readfds, 0, 0, &timeout);
	return FD_ISSET(STDIN_FILENO, &readfds);

#endif
}

}	// namespace

namespace uci {

void loop()
{
	board::Board B;
	std::string cmd, token;
	std::cout << std::boolalpha;

	while (token != "quit") {
		if (!getline(std::cin, cmd) || cmd == "quit")
			break;

		std::istringstream is(cmd);
		is >> std::boolalpha;
		is >> std::skipws >> token;
		if (token == "uci")
			intro();
		else if (token == "ucinewgame")
			search::clear_state();
		else if (token == "position")
			position(B, is);
		else if (token == "go")
			go(B, is);
		else if (token == "isready") {
			search::TT.alloc(Hash << 20);
			std::cout << "readyok" << std::endl;
		} else if (token == "setoption")
			setoption(is);
		else if (token == "eval") {
			const int e = eval::symmetric_eval(B) + eval::asymmetric_eval(B, hanging_pieces(B));
			std::cout << B << "eval = " << e << std::endl;
		} else if (token == "perft") {
			int depth;
			if (is >> depth)
				std::cout << perft(B, depth, 0) << std::endl;
		}
	}
}

std::string check_input()
{
	std::string token;

	if (input_available())
		getline(std::cin, token);

	return token;
}

void info::clear()
{
	score = depth = time = 0;
	nodes = 0;
	bound = EXACT;
}

std::ostream& operator<< (std::ostream& ostrm, const info& ui)
{
	ostrm << "info score ";
	if (ui.bound == info::LBOUND)
		ostrm << "lowerbound ";
	else if (ui.bound == info::UBOUND)
		ostrm << "upperbound ";

	if (ui.score >= MATE - MAX_PLY)
		ostrm << "mate " << (MATE - ui.score + 1) / 2;
	else if (ui.score <= -MATE + MAX_PLY)
		ostrm << "mate " << -(ui.score + MATE + 1) / 2;
	else
		ostrm << "cp " << ui.score;

	ostrm << " depth " << ui.depth
		<< " nodes " << ui.nodes
		<< " time " << ui.time;

	if (ui.bound == info::EXACT) {
		ostrm << " pv";
		for (int i = 0; i <= MAX_PLY && ui.pv[i]; ++i)
			ostrm << ' ' << move_to_string(ui.pv[i]);
	}

	return ostrm;
}

}	// namespace uci
