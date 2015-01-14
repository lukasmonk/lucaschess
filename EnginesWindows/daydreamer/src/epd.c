#include "daydreamer.h"
#include <errno.h>
#include <stdio.h>
#include <string.h>

void epd_testsuite(char* filename, int time_per_problem)
{
    char test_storage[4096];
    char* test = test_storage;
    milli_timer_t epd_timer;
    init_timer(&epd_timer);
    FILE* test_file = fopen(filename, "r");
    if (!test_file) {
        printf("Couldn't open epd test file %s: %s\n",
                filename, strerror(errno));
        return;
    }
    int total_tests = 0, correct_tests = 0;
    while (fgets(test, 4096, test_file)) {
        char *bm=NULL, *id=NULL, *token=NULL;
        move_t best_move = NO_MOVE;
        init_search_data(&root_data);
        set_position(&root_data.root_pos, test);
        print_board(&root_data.root_pos, false);
        while ((token = strsep(&test, "; \t"))) {
            if (!*token) continue;
            if (strcasestr(token, "bm")) {
                while (!bm || !*bm) bm = strsep(&test, "; \t");
                best_move = san_str_to_move(&root_data.root_pos, bm);
            }
            if (strcasestr(token, "id")) {
                strsep(&test, "\"");
                id = strsep(&test, "\"");
            }
        }
        printf("%d: %s\t", total_tests+1, id);
        if (best_move == NO_MOVE) {
            printf("parse error: couldn't read best move\n");
            test = test_storage;
            ++total_tests;
            continue;
        }
        bool failure = false;
        start_timer(&epd_timer);
        root_data.time_target = root_data.time_limit = time_per_problem;
        deepening_search(&root_data, false);
        move_t result = root_data.pv[0];
        int time = stop_timer(&epd_timer);
        if (result != best_move) {
            failure = true;
            printf("--");
        } else printf("OK");
        printf(" / %.2fs\n", time/1000.0);
        ++total_tests;
        if (!failure) ++correct_tests;
        test = test_storage;
    }
    printf("Tests completed. %d/%d tests passed in %.2fs.\n",
            correct_tests, total_tests, elapsed_time(&epd_timer)/1000.0);
}

