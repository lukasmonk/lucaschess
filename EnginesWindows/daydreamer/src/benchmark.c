
#include "daydreamer.h"

extern search_data_t root_data;
const char* positions[] = {
    "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",
    "r4rk1/1b2qppp/p1n1p3/1p6/1b1PN3/3BRN2/PP3PPP/R2Q2K1 b - - 7 16",
    "4r1k1/ppq3pp/3b4/2pP4/2Q1p3/4B1P1/PP5P/R5K1 b - - 0 20",
    "4rrk1/pp1n3p/3q2pQ/2p1pb2/2PP4/2P3N1/P2B2PP/4RRK1 b - - 7 19",
    "rq3rk1/ppp2ppp/1bnpb3/3N2B1/3NP3/7P/PPPQ1PP1/2KR3R w - - 7 14",
    "r1bq1r1k/1pp1n1pp/1p1p4/4p2Q/4Pp2/1BNP4/PPP2PPP/3R1RK1 w - - 2 14",
    "r3r1k1/2p2ppp/p1p1bn2/8/1q2P3/2NPQN2/PPP3PP/R4RK1 b - - 2 15",
    "r1bbk1nr/pp3p1p/2n5/1N4p1/2Np1B2/8/PPP2PPP/2KR1B1R w kq - 0 13",
    "r1bq1rk1/ppp1nppp/4n3/3p3Q/3P4/1BP1B3/PP1N2PP/R4RK1 w - - 1 16",
    "4r1k1/r1q2ppp/ppp2n2/4P3/5Rb1/1N1BQ3/PPP3PP/R5K1 w - - 1 17",
    "2rqkb1r/ppp2p2/2npb1p1/1N1Nn2p/2P1PP2/8/PP2B1PP/R1BQK2R b KQ - 0 11",
    "r1bq1r1k/b1p1npp1/p2p3p/1p6/3PP3/1B2NN2/PP3PPP/R2Q1RK1 w - - 1 16",
    "3r1rk1/p5pp/bpp1pp2/8/q1PP1P2/b3P3/P2NQRPP/1R2B1K1 b - - 6 22",
    "r1q2rk1/2p1bppp/2Pp4/p6b/Q1PNp3/4B3/PP1R1PPP/2K4R w - - 2 18",
    "4k2r/1pb2ppp/1p2p3/1R1p4/3P4/2r1PN2/P4PPP/1R4K1 b - - 3 22",
    "3q2k1/pb3p1p/4pbp1/2r5/PpN2N2/1P2P2P/5PP1/Q2R2K1 b - - 4 26",
    NULL
};

/*
 * Search all of the benchmark positions either to the given depth or for the
 * given amount of time. The positions come directly from Glaurung's benchmark
 * suite.
 */
void benchmark(int depth, int time_limit)
{
    milli_timer_t bench_timer;
    uint64_t total_nodes = 0;
    int time = 0;
    init_timer(&bench_timer);
    for (int i=0; ; ++i) {
        const char* fen = positions[i];
        if (!fen) break;
        init_search_data(&root_data);
        set_position(&root_data.root_pos, fen);
        print_board(&root_data.root_pos, false);
        start_timer(&bench_timer);
        root_data.time_target = root_data.time_limit = time_limit;
        root_data.depth_limit = depth*PLY;
        deepening_search(&root_data, false);
        time = stop_timer(&bench_timer);
        printf("time: %d\ndepth: %d\nnodes: %"PRIu64"\n",
                time, (int)root_data.current_depth, root_data.nodes_searched);
        total_nodes += root_data.nodes_searched;
    }
    time = elapsed_time(&bench_timer);
    printf("aggregate nodes %"PRIu64" time %d nps %"PRIu64"\n",
            total_nodes, time, total_nodes/(time+1)*1000);
}

