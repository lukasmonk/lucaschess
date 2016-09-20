#include <chrono>
#include "search.h"

using namespace std::chrono;

uint64_t perft(board::Board& B, int depth, int ply)
/* Calculates perft(depth), and displays all perft(depth-1) in the initial position. This
 * decomposition is useful to debug an incorrect perft recursively, against a correct perft
 * generator */
{
	move::move_t mlist[MAX_MOVES];
	move::move_t *begin = mlist, *m;
	move::move_t *end = movegen::gen_moves(B, mlist);
	uint64_t count;

	if (depth > 1) {
		for (m = begin, count = 0ULL; m < end; m++) {
			uint64_t count_subtree;

			B.play(*m);
			count += count_subtree = perft(B, depth - 1, ply + 1);
			B.undo();

			if (!ply)
				std::cout << move_to_string(*m) << '\t' << count_subtree << std::endl;
		}
	} else {
		count = end - begin;
		if (!ply)
			for (m = begin; m < end; m++)
				std::cout << move_to_string(*m) << std::endl;
	}

	return count;
}

bool test_perft()
{
	board::Board B;

	struct TestPerft {
		const char *s;
		int depth;
		uint64_t value;
	};

	// http://chessprogramming.wikispaces.com/Perft+Results
	TestPerft Test[] = {
		{"rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1", 6, 119060324ull},
		{"r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq -", 5, 193690690ull},
		{"8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - -", 7, 178633661ull},
		{"r2q1rk1/pP1p2pp/Q4n2/bbp1p3/Np6/1B3NBn/pPPP1PPP/R3K2R b KQ - 0 1", 6, 706045033ull},
		{"rnbqkb1r/pp1p1ppp/2p5/4P3/2B5/8/PPP1NnPP/RNBQK2R w KQkq - 0 6", 5, 70202861ull},
		{nullptr, 0, 0}
	};

	uint64_t total = 0;
	auto start = high_resolution_clock::now();

	for (int i = 0; Test[i].s; i++) {
		std::cout << Test[i].s << std::endl;
		B.set_fen(Test[i].s);
		if (perft(B, Test[i].depth, 0) != Test[i].value) {
			std::cerr << "Incorrect perft" << std::endl;
			return false;
		}
		total += Test[i].value;
	}

	auto stop = high_resolution_clock::now();
	int elapsed = duration_cast<microseconds>(stop - start).count();
	std::cout << "speed: " << (int)(total / (double)elapsed * 1e6) << " leaf/sec" << std::endl;

	return true;
}

bool test_see()
{
	struct TestSEE {
		const char *fen, *move;
		int value;
	};

	TestSEE test[] = {
		{"k6K/8/4b3/8/3N4/8/8/8 w - -", "d4e6", vB},
		{"k6K/3p4/4b3/8/3N4/8/8/8 w - -", "d4e6", 0},
		{"k6K/3p4/4b3/8/3N4/8/8/4R3 w - -", "d4e6", vB - vN + vOP},
		{"k3r2K/3p4/4b3/8/3N4/8/4R3/4R3 w - -", "d4e6", vB - vN + vOP},
		{"k6K/3P4/8/8/8/8/8/8 w - -", "d7d8q", vQ - vOP},
		{"k6K/3P4/2n5/8/8/8/8/8 w - -", "d7d8q", -vOP},
		{"k6K/3P4/2n1N3/8/8/8/8/8 w - -", "d7d8q", -vOP + vN},
		{"k6K/3PP3/2n5/b7/7B/8/8/3R4 w - -", "d7d8q", vN + vB - 2*vOP},
		{"3R3K/k3P3/8/b7/8/8/8/8 b - -", "a5d8", vR - vB + vOP - vQ},
		{"8/4k3/8/8/RrR1N2r/8/5K2/8 b - - 11 1 ", "h4e4", vN - vR},
		{NULL, NULL, 0}
	};

	board::Board B;

	for (int i = 0; test[i].fen; ++i) {
		B.set_fen(test[i].fen);
		std::cout << test[i].fen << '\t' << test[i].move << std::endl;
		move::move_t m = move::string_to_move(B, test[i].move);

		const int s = move::see(B, m);

		if (s != test[i].value) {
			std::cout << B << "SEE = " << see(B, m) << std::endl;
			std::cout << "should be " << test[i].value << std::endl;
			return false;
		}
	}

	return true;
}

void bench(int depth)
{
	static const char *test[] = {
		"r1bqkbnr/pp1ppppp/2n5/2p5/4P3/5N2/PPPP1PPP/RNBQKB1R w KQkq -",
		"r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq -",
		"8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - -",
		"4rrk1/pp1n3p/3q2pQ/2p1pb2/2PP4/2P3N1/P2B2PP/4RRK1 b - - 7 19",
		"rq3rk1/ppp2ppp/1bnpb3/3N2B1/3NP3/7P/PPPQ1PP1/2KR3R w - - 7 14",
		"r1bq1r1k/1pp1n1pp/1p1p4/4p2Q/4Pp2/1BNP4/PPP2PPP/3R1RK1 w - - 2 14",
		"1rbqk1nr/p3ppbp/2np2p1/2p5/1p2PP2/3PB1P1/PPPQ2BP/R2NK1NR b KQk -",
		"r1bqk2r/pp1p1ppp/2n1pn2/2p5/1bPP4/2NBP3/PP2NPPP/R1BQK2R b KQkq -",
		"rnb1kb1r/ppp2ppp/1q2p3/4P3/2P1Q3/5N2/PP1P1PPP/R1B1KB1R b KQkq -",
		"r1b2rk1/pp2nppp/1b2p3/3p4/3N1P2/2P2NP1/PP3PBP/R3R1K1 b - -",
		"n1q1r1k1/3b3n/p2p1bp1/P1pPp2p/2P1P3/2NBB2P/3Q1PK1/1R4N1 b - -",
		"r1bq1r1k/b1p1npp1/p2p3p/1p6/3PP3/1B2NN2/PP3PPP/R2Q1RK1 w - - 1 16",
		"3r1rk1/p5pp/bpp1pp2/8/q1PP1P2/b3P3/P2NQRPP/1R2B1K1 b - - 6 22",
		"r1q2rk1/2p1bppp/2Pp4/p6b/Q1PNp3/4B3/PP1R1PPP/2K4R w - - 2 18",
		"4k2r/1pb2ppp/1p2p3/1R1p4/3P4/2r1PN2/P4PPP/1R4K1 b - - 3 22",
		"3q2k1/pb3p1p/4pbp1/2r5/PpN2N2/1P2P2P/5PP1/Q2R2K1 b - - 4 26",
		"2r5/8/1n6/1P1p1pkp/p2P4/R1P1PKP1/8/1R6 w - - 0 1",
		"r2q1rk1/1b1nbppp/4p3/3pP3/p1pP4/PpP2N1P/1P3PP1/R1BQRNK1 b 0 1",
		"6k1/5pp1/7p/p1p2n1P/P4N2/6P1/1P3P1K/8 w - - 0 35",
		"r4rk1/1pp1q1pp/p2p4/3Pn3/1PP1Pp2/P7/3QB1PP/2R2RK1 b 0 1",
		nullptr
	};

	board::Board B;
	search::Limits sl;
	sl.depth = depth;
	uint64_t nodes = 0;

	search::TT.alloc(32ULL << 20);
	search::clear_state();

	time_point<high_resolution_clock> start, end;
	start = high_resolution_clock::now();

	for (int i = 0; test[i]; ++i) {
		B.set_fen(test[i]);

		std::cout << B.get_fen() << std::endl;
		bestmove(B, sl);
		std::cout << std::endl;

		nodes += search::node_count;
	}

	end = high_resolution_clock::now();
	int64_t elapsed_usec = duration_cast<microseconds>(end - start).count();

	std::cout << "nodes = " << nodes << std::endl;
	std::cout << "kn/s = " << nodes / (double)elapsed_usec * 1e3 << std::endl;
}

