
#include "daydreamer.h"
#include <errno.h>
#include <stdio.h>
#include <string.h>

static uint64_t full_search(position_t* pos, int depth);
static uint64_t divide(position_t* pos, int depth);

/*
 * Execute a series of perft tests from a given file. The test file consists of
 * any number of test lines, and each line has the following format:
 * <fen description>( ;D<d> <nodes>)+
 * where <d> is the target depth and <nodes is the number of nodes at that
 * depth. So, for example, to test the initial position at depths 1 and 2:
 * rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1 ;D1 20 ;D2 400
 * The test results and elapsed time are printed to stdout.
 *
 * This file format and the associated test files are taken from ROCE. For
 * more information, see http://www.rocechess.ch/rocee.html.
 */
void perft_testsuite(char* filename)
{
    char test_storage[4096];
    char* test = test_storage;
    position_t pos;
    milli_timer_t perft_timer;
    init_timer(&perft_timer);
    FILE* test_file = fopen(filename, "r");
    if (!test_file) {
        printf("Couldn't open perft test file %s: %s\n",
                filename, strerror(errno));
        return;
    }
    int total_tests = 0, correct_tests = 0;
    while (fgets(test, 4096, test_file)) {
        char* fen = strsep(&test, ";");
        set_position(&pos, fen);
        printf("Test %d: %s\n", total_tests+1, fen);
        bool failure = false;
        do {
            int depth;
            uint64_t correct_answer;
            sscanf(test, "D%d %"PRIu64, &depth, &correct_answer);
            start_timer(&perft_timer);
            uint64_t result = full_search(&pos, depth);
            int time = stop_timer(&perft_timer);
            printf("\tDepth %d: %15"PRIu64, depth, result);
            if (result != correct_answer) {
                failure = true;
                printf(" expected %15"PRIu64" -- FAIL",
                        correct_answer);
            } else printf(" -- SUCCESS");
            printf(" / %.2fs\n", time/1000.0);
        } while ((test = strchr(test, ';') + 1) != (char*)1);
        ++total_tests;
        if (!failure) ++correct_tests;
        test = test_storage;
    }
    printf("Tests completed. %d/%d tests passed in %.2fs.\n",
            correct_tests, total_tests, elapsed_time(&perft_timer)/1000.0);
}

/*
 * Determine the total number of nodes at depth |depth| in the game tree
 * rooted at |position|. If |div| is true, report numbers for each current
 * legal move. The number of nodes found and elapsed time are reported.
 */
uint64_t perft(position_t* position, int depth, bool div)
{
    milli_timer_t perft_timer;
    init_timer(&perft_timer);
    start_timer(&perft_timer);
    uint64_t nodes;
    if (div) {
        nodes = divide(position, depth);
    } else {
        nodes = full_search(position, depth);
        printf("%"PRIu64" nodes", nodes);
    }
    stop_timer(&perft_timer);
    printf(", elapsed time %d ms\n", elapsed_time(&perft_timer));
    return nodes;
}

/*
 * Print the number of nodes descended from each legal move in the given
 * position at depth |depth|, returning the total number of nodes.
 */
static uint64_t divide(position_t* pos, int depth)
{
    move_t move_list[256];
    move_t* current_move = move_list;
    int num_moves = 0;
    move_selector_t selector;
    init_move_selector(&selector, pos, PV_GEN, NULL, NO_MOVE, 0, 0);
    for (move_t move = select_move(&selector); move != NO_MOVE;
            move = select_move(&selector), ++num_moves) {
        move_list[num_moves] = move;
    }
    move_list[num_moves] = NO_MOVE;

    uint64_t child_nodes, total_nodes=0;
    char coord_move[7];
    while (*current_move) {
        undo_info_t undo;
        do_move(pos, *current_move, &undo);
        child_nodes = full_search(pos, depth-1);
        total_nodes += child_nodes;
        undo_move(pos, *current_move, &undo);
        move_to_coord_str(*current_move, coord_move);
        printf("%s: %8"PRIu64"\n", coord_move, child_nodes);
        ++current_move;
    }
    printf("%d moves, %"PRIu64" nodes", num_moves, total_nodes);
    return total_nodes;
}

/*
 * Do a full search of the position tree rooted at |pos|, to depth |depth|.
 * This does no evaluation whatsoever, it just counts nodes.
 */
static uint64_t full_search(position_t* pos, int depth)
{
    if (depth <= 0) return 1;
    move_t move_list[256];
    move_t* current_move = move_list;
    int num_moves = 0;
    move_selector_t selector;
    init_move_selector(&selector, pos, PV_GEN, NULL, NO_MOVE, 0, 0);
    for (move_t move = select_move(&selector); move != NO_MOVE;
            move = select_move(&selector), ++num_moves) {
        move_list[num_moves] = move;
    }
    move_list[num_moves] = NO_MOVE;

    if (depth == 1) return num_moves;

    uint64_t nodes = 0;
    while (*current_move) {
        undo_info_t undo;
        do_move(pos, *current_move, &undo);
        nodes += full_search(pos, depth-1);
        undo_move(pos, *current_move, &undo);
        ++current_move;
    }
    return nodes;
}

