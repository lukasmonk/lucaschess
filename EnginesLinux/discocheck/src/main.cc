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
#include "test.h"
#include "psq.h"
#include "eval.h"
#include "search.h"
#include "uci.h"

uint64_t dbg_cnt1 = 0, dbg_cnt2 = 0;

int main (int argc, char **argv)
{
	bb::init();
	psq::init();
	eval::init();

	if (argc == 2) {
		if (std::string(argv[1]) == "bench")
			bench(12);
		else if (std::string(argv[1]) == "perft")
			test_perft();
		else if (std::string(argv[1]) == "see")
			test_see();

		if (dbg_cnt1 || dbg_cnt2)
			std::cout << dbg_cnt1 << '\n' << dbg_cnt2 << std::endl;
	} else
		uci::loop();
}
