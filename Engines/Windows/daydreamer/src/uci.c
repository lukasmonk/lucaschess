
#include "version.h"
#include "daydreamer.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void uci_get_input(void);
static void uci_handle_command(char* command);
static void uci_position(char* uci_pos);
static void uci_go(char* command);
static void calculate_search_time(int wtime,
        int btime,
        int winc,
        int binc,
        int movestogo);
static void uci_handle_ext(char* command);
static int input_available(void);

/*
 * Print a helpful message that describes non-stadard uci commands supported
 * by Daydreamer. I secretly believe that no one but me has ever used this or
 * even read this code.
 */
static void uci_print_help(void)
{
    printf(
"Daydreamer is a chess engine that follows the UCI protocol, as described\n"
"at http://wbec-ridderkerk.nl/html/UCIProtocol.html. In addition to the  \n"
"standard UCI commands, Daydreamer understands some non-standard commands:\n"
"\n"
"    print     \tPrint the current position, along with some evaluation info.\n"
"    perft <n>  \tPrint the number of positions that could be reached from the "
"               \tcurrent position in exactly <n> moves.\n"
"    divide <n> \tThe same as perft, but break numbers down by root move.\n"
"    see <move> \tPrint the static exchange evaluation score of the given "
"move.\n"
"    bench <depth>\n"
"               \tSearch a fixed set of positions to the given depth, and\n"
"               \treport the total nodes searched and time taken.\n"
"    perftsuite <filename>\n"
"               \tRun a suite of perft tests from a file in the format\n"
"               \tdescribed at www.rocechess.ch/rocee.html\n"
"   epd <filename> <time>\n"
"              \tRead the given epd file, and search each position for <time>\n"
"               \tseconds.\n"
"   book        \tPrint book information for the current position.\n"
"               \tUses the currently loaded book.\n"
"   <move>      \tMake the given move (eg e2e4) on the internal board.\n"
"   gtb         \tLook up the current position in the Gaviota Tablebases.\n"
"   echo <text> \tEcho the given string to standard output.\n"
"   help        \tPrint this help message."
"\n\n");
}

/*
 * Read uci commands out of the given stream, until it's closed or we recieve
 * a quit command.
 */
void uci_read_stream(FILE* stream)
{
    char command[4096] = { 0 };
    while (fgets(command, 4096, stream)) uci_handle_command(command);
}

/*
 * Handle one line of uci input.
 */
static void uci_handle_command(char* command)
{
    if (!command) exit(0);

    // strip trailing newline.
    char* c = command;
    while (*c) ++c;
    while (c > command && (*--c == '\n' || *c == ' ')) *c = '\0';

    if (!strncasecmp(command, "ucinewgame", 10)) {
    } else if (!strncasecmp(command, "uci", 3)) {
        printf("id name %s %s\n", ENGINE_NAME, ENGINE_VERSION);
        printf("id author %s\n", ENGINE_AUTHOR);
        print_uci_options();
        printf("uciok\n");
    } else if (!strncasecmp(command, "isready", 7)) printf("readyok\n");
    else if (!strncasecmp(command, "quit", 4)) exit(0);
    else if (!strncasecmp(command, "position", 8)) uci_position(command+9);
    else if (!strncasecmp(command, "go", 2)) {
        uci_go(command+3);
    } else if (!strncasecmp(command, "setoption", 9)) {
        command = strcasestr(command, "name") + 5;
        set_uci_option(command);
    } else if (!strncasecmp(command, "stop", 4)) {
        root_data.engine_status = ENGINE_ABORTED;
    } else if (!strncasecmp(command, "ponderhit", 9)) {
        if (should_stop_searching(&root_data)) {
            root_data.engine_status = ENGINE_ABORTED;
        } else if (root_data.engine_status == ENGINE_PONDERING) {
            root_data.engine_status = ENGINE_THINKING;
        }
    } else if (!strncasecmp(command, "debug", 5)) {
        command += 5;
        while (isspace(*command)) ++command;
        if (!strncasecmp(command, "on", 2)) {
            set_uci_option("Verbosity value high");
        } else if (!strncasecmp(command, "off", 3)) {
            set_uci_option("Verbosity value low");
        }
    } else {
        uci_handle_ext(command);
    }
}

/*
 * Handle non-standard uci extensions. These are diagnostic and debugging
 * commands that print more information about a position or more or execute
 * test suites.
 */
static void uci_handle_ext(char* command)
{
    position_t* pos = &root_data.root_pos;
    if (!strncasecmp(command, "perftsuite", 10)) {
        command+=10;
        while (isspace(*command)) command++;
        perft_testsuite(command);
    } else if (!strncasecmp(command, "perft", 5)) {
        int depth=1;
        sscanf(command+5, " %d", &depth);
        perft(pos, depth, false);
    } else if (!strncasecmp(command, "divide", 6)) {
        int depth=1;
        sscanf(command+6, " %d", &depth);
        perft(pos, depth, true);
    } else if (!strncasecmp(command, "bench", 5)) {
        int depth = 1;
        sscanf(command+5, " %d", &depth);
        benchmark(depth, 0);
    } else if (!strncasecmp(command, "see", 3)) {
        command += 3;
        while (isspace(*command)) command++;
        move_t move = coord_str_to_move(pos, command);
        printf("see: %d\n", static_exchange_eval(pos, move));
    } else if (!strncasecmp(command, "epd", 3)) {
        char filename[256];
        int time_per_move = 5;
        sscanf(command+3, " %s %d", filename, &time_per_move);
        time_per_move *= 1000;
        epd_testsuite(filename, time_per_move);
    } else if (!strncasecmp(command, "gtb", 3)) {
        if (options.use_gtb) {
            int score;
            bool success = probe_gtb_hard_dtm(pos, &score);
            if (success) {
                printf("score: %d\n", score == 0 ? 0 :
                        (MATE_VALUE-abs(score)) *
                        (score < 0 ? -1 : 1));
            } else {
                printf("Tablebase lookup failed\n");
            }
        } else {
            printf("Gaviota TBs not loaded\n");
        }
    } else if (!strncasecmp(command, "book", 4)) {
        if (!options.book_loaded) printf("opening book not loaded\n");
        else {
            move_t book_move = options.probe_book(&root_data.root_pos);
            if (book_move) {
                char move_str[7];
                move_to_coord_str(book_move, move_str);
                printf("book move %s\n", move_str);
            }
        }
    } else if (!strncasecmp(command, "print", 5)) {
        print_board(pos, false);
        move_t moves[255];
        printf("moves: ");
        generate_legal_moves(pos, moves);
        for (move_t* move = moves; *move; ++move) {
            char san[8];
            move_to_san_str(pos, *move, san);
            printf("%s ", san);
        }
        printf("\nordered moves: ");
        move_selector_t sel;
        init_move_selector(&sel, pos, PV_GEN, NULL, NO_MOVE, 0, 0);
        for (move_t move = select_move(&sel); move != NO_MOVE;
                move = select_move(&sel)) {
            char san[8];
            move_to_san_str(pos, move, san);
            printf("%s ", san);
        }
        printf("\n");
        eval_data_t ed;
        int eval = full_eval(pos, &ed);
        _check_eval_symmetry(pos, eval);
    } else if (!strncasecmp(command, "help", 4) ||
            !strncasecmp(command, "?", 1)) {
        uci_print_help();
    } else if (!strncasecmp(command, "echo", 4)) {
        printf("%s\n", command+5);
    } else {
        move_t m = coord_str_to_move(pos, command);
        if (!m) return;
        undo_info_t undo;
        do_move(pos, m, &undo);
    }
}

/*
 * Parse a uci position command and set the board appropriately.
 */
static void uci_position(char* uci_pos)
{
    while (isspace(*uci_pos)) ++uci_pos;
    if (!strncasecmp(uci_pos, "startpos", 8)) {
        set_position(&root_data.root_pos, FEN_STARTPOS);
        uci_pos += 8;
    } else if (!strncasecmp(uci_pos, "fen", 3)) {
        uci_pos += 3;
        while (*uci_pos && isspace(*uci_pos)) ++uci_pos;
        uci_pos = set_position(&root_data.root_pos, uci_pos);
    }
    while (isspace(*uci_pos)) ++uci_pos;
    if (!strncasecmp(uci_pos, "moves", 5)) {
        uci_pos += 5;
        while (isspace(*uci_pos)) ++uci_pos;
        while (*uci_pos) {
            move_t move = coord_str_to_move(&root_data.root_pos, uci_pos);
            if (move == NO_MOVE) {
                printf("Warning: could not parse %s\n", uci_pos);
                print_board(&root_data.root_pos, true);
                return;
            }
            undo_info_t dummy_undo;
            do_move(&root_data.root_pos, move, &dummy_undo);
            while (*uci_pos && !isspace(*uci_pos)) ++uci_pos;
            while (isspace(*uci_pos)) ++uci_pos;
        }
    }
}

/*
 * Parse the uci go command and start searching.
 */
static void uci_go(char* command)
{
    assert(command);
    char* info;
    int wtime=0, btime=0, winc=0, binc=0, movestogo=0, movetime=0;
    bool ponder = false;

    init_search_data(&root_data);
    if ((info = strcasestr(command, "searchmoves"))) {
        info += 11;
        int move_index=0;
        while (isspace(*info)) ++info;
        while (*info) {
            move_t move = coord_str_to_move(&root_data.root_pos, info);
            if (move == NO_MOVE) {
                break;
            }
            if (!is_move_legal(&root_data.root_pos, move)) {
                printf("%s is not a legal move\n", info);
            }
            init_root_move(&root_data.root_moves[move_index++], move);
            while (*info && !isspace(*info)) ++info;
            while (isspace(*info)) ++info;
        }
    }
    if ((strcasestr(command, "ponder"))) {
        ponder = true;
    }
    if ((info = strcasestr(command, "wtime"))) {
        sscanf(info+5, " %d", &wtime);
    }
    if ((info = strcasestr(command, "btime"))) {
        sscanf(info+5, " %d", &btime);
    }
    if ((info = strcasestr(command, "winc"))) {
        sscanf(info+4, " %d", &winc);
    }
    if ((info = strcasestr(command, "binc"))) {
        sscanf(info+4, " %d", &binc);
    }
    if ((info = strcasestr(command, "movestogo"))) {
        sscanf(info+9, " %d", &movestogo);
    }
    if ((info = strcasestr(command, "depth"))) {
        int depth;
        sscanf(info+5, " %d", &depth);
        root_data.depth_limit = (float)depth;
    }
    if ((info = strcasestr(command, "nodes"))) {
        sscanf(info+5, " %"PRIu64, &root_data.node_limit);
    }
    if ((info = strcasestr(command, "mate"))) {
        sscanf(info+4, " %d", &root_data.mate_search);
    }
    if ((info = strcasestr(command, "movetime"))) {
        sscanf(info+8, " %d", &movetime);
        root_data.time_target = root_data.time_limit = movetime;
    }
    if ((strcasestr(command, "infinite"))) {
        root_data.infinite = true;
    }

    if (!movetime && !root_data.infinite) {
        calculate_search_time(wtime, btime, winc, binc, movestogo);
    }
    if (!ponder && options.verbosity > 1) {
        print_board(&root_data.root_pos, true);
    }
    root_data.time_bonus = 0;
    deepening_search(&root_data, ponder);
}

/*
 * Given uci time management parameters, determine how long to spend on this
 * move. We compute both a target time (the amount of time we'd like to spend)
 * that can be ignored if the position needs more time (e.g. we just failed
 * high at the root) and a higher time limit that should not be exceeded.
 */
static void calculate_search_time(int wtime,
        int btime,
        int winc,
        int binc,
        int movestogo)
{
    // TODO: Cool heuristics for time mangement.
    // TODO: formula for expected number of remaining moves.
    // For now, just use a simple static rule and look at our own time only.
    color_t side = root_data.root_pos.side_to_move;
    int inc = side == WHITE ? winc : binc;
    int time = side == WHITE ? wtime : btime;
    if (!movestogo) {
        // x+y time control
        root_data.time_target = time/30 + inc;
        root_data.time_limit = MAX(time/5, inc-250);
    } else {
        // x/y time control
        root_data.time_target = time/CLAMP(movestogo, 2, 20);
        root_data.time_limit = movestogo == 1 ?
            MAX(time-250, time/2) :
            MIN(time/4, time*4/movestogo);
    }
    if (options.ponder) {
        root_data.time_target =
            MIN(root_data.time_limit, root_data.time_target * 5 / 4);
    }
    // TODO: Adjust polling interval based on time remaining?
    //       This might help for really low time-limit testing.
}

/*
 * Handle any ready uci commands. Called periodically during search.
 */
void uci_check_for_command()
{
    char input[4096];
    if (input_available()) {
        if (!fgets(input, 4096, stdin)) exit(0);
        else if (!strncasecmp(input, "quit", 4)) exit(0);
        else if (!strncasecmp(input, "stop", 4)) {
            root_data.engine_status = ENGINE_ABORTED;
        } else if (strncasecmp(input, "ponderhit", 9) == 0) {
            if (should_stop_searching(&root_data)) {
                root_data.engine_status = ENGINE_ABORTED;
            } else if (root_data.engine_status == ENGINE_PONDERING) {
                root_data.engine_status = ENGINE_THINKING;
            }
        } else if (strncasecmp(input, "isready", 7) == 0) {
            printf("readyok\n");
        }
    }
}

/*
 * Wait for the next uci command. Called when we're done pondering but
 * haven't gotten a ponder hit or miss yet.
 */
void uci_wait_for_command()
{
    char command[4096];
    fgets(command, 4096, stdin);
    uci_handle_command(command);
}

/*
 * Boilerplate code to see if data is available to be read on stdin.
 * Cross-platform for unix/windows.
 *
 * Many thanks to the original author(s). I've seen minor variations on this
 * in Scorpio, Viper, Beowulf, Olithink, and others, so I don't know where
 * it's from originally. I'm just glad I didn't have to figure out how to
 * do this on windows.
 */
#ifndef _WIN32
// Posix version.
int input_available(void)
{
    fd_set readfds;
    struct timeval timeout;

    FD_ZERO(&readfds);
    FD_SET(fileno(stdin), &readfds);
    // Set to timeout immediately
    timeout.tv_sec = 0;
    timeout.tv_usec = 0;
    select(16, &readfds, 0, 0, &timeout);
    return (FD_ISSET(fileno(stdin), &readfds));
}

#else

// Windows version.
#include <conio.h>
int input_available(void)
{
    static int init=0, pipe=0;
    static HANDLE inh;
    DWORD dw;
#if defined(FILE_CNT)
    if (stdin->_cnt > 0) return stdin->_cnt;
#endif
    if (!init) {
        init = 1;
        inh = GetStdHandle(STD_INPUT_HANDLE);
        pipe = !GetConsoleMode(inh, &dw);
        if (!pipe) {
            SetConsoleMode(inh,
                    dw & ~(ENABLE_MOUSE_INPUT | ENABLE_WINDOW_INPUT));
            FlushConsoleInputBuffer(inh);
        }
    }
    if (pipe) {
        if (!PeekNamedPipe(inh, NULL, 0, NULL, &dw, NULL)) return 1;
        return dw;
    } else {
        GetNumberOfConsoleInputEvents(inh, &dw);
        return dw <= 1 ? 0 : dw;
    }
}
#endif

