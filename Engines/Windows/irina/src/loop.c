#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef WIN32
    #include <windows.h>
#else
    #include <unistd.h>
    #define INFINITE 9999
#endif

#include "defs.h"
#include "protos.h"
#include "globals.h"

#define VERSION "0.15"


void begin(void)
{
    init_hash();
    init_data();
    init_board();
    setbuf(stdout, NULL);
    setbuf(stdin, NULL);
}


bool scandin(char *std, char *scan)
{
    return !strncmp(std, scan, strlen(scan));
}

int scan_int(char *line, char *scan)
{
    char *find;
    int res;

    res = 0;
    find = strstr(line, scan);
    if (find)
    {
        sscanf(find + strlen(scan) + 1, "%d", &res);
    }
    return res;
}


#define SCAN(scan)    scandin(s, scan)

void loop(void)
{
    char s[2048];
    char file[256];
    int num;

    set_ownbookfile("irina.bin");
    set_ownbook(true);

    #ifdef LOG
        open_log();
    #endif

    printf("Irina is a free adaptation of Winglet, chess engine created by Stef Luijten.\nUsed to implement child personalities of Lucas Chess GUI.\n\n");

    for (;;)
    {
        if (!fgets(s, 2048, stdin))
        {
            #ifdef WIN32
                Sleep(100);
            #else
                usleep(100000);
            #endif
            continue;
        }
        #ifdef LOG
            fprintf(flog, "REC:%s\n", s);
        #endif
        if (SCAN("uci"))
        {
            printf("id name Irina %s\n", VERSION);
            printf("id author Lucas Monge\n");
            printf("option name Hash type spin min 2 max 1024 default 32\n");
            printf("option name Personality type combo default Irina var Irina var Steven var Monkey var Donkey var Bull var Wolf var Lion var Rat var Snake var Material var Random var Capture var Advance\n");
            printf("option name Min Time type spin default 0 min 0 max 99\n");
            printf("option name Max Time type spin default 0 min 0 max 99\n");
            printf("option name OwnBook type check default true\n");
            printf("option name OwnBookFile type string default irina.bin\n");
            printf("uciok\n");
            #ifdef LOG
                fprintf(flog, "id name Irina %s\n", VERSION);
                fprintf(flog, "id author Lucas Monge\n");
                fprintf(flog, "option name Hash type spin min 2 max 1024 default 32\n");
                fprintf(flog, "option name Personality type combo default Irina var Irina var Steven var Monkey var Donkey var Bull var Wolf var Lion var Rat var Snake var Material var Random var Capture var Advance\n");
                fprintf(flog, "option name Min Time type spin default 0 min 0 max 99\n");
                fprintf(flog, "option name Max Time type spin default 0 min 0 max 99\n");
                fprintf(flog, "option name OwnBook type check default true\n");
                fprintf(flog, "option name OwnBookFile type string default irina.bin\n");
                fprintf(flog, "uciok\n");
            #endif

        }
        else if (SCAN("isready"))
        {
            printf("readyok\n");
            #ifdef LOG
                fprintf(flog, "readyok\n");
            #endif
        }
        else if (SCAN("stop"))
        {
            continue;
        }
        else if (SCAN("quit"))
        {
            break;
        }
        else if (SCAN("fen"))
        {
            board_fen(s);
            printf("%s\n", s);
            #ifdef LOG
                fprintf(flog, "%s\n", s);
            #endif
        }
        else if (SCAN("test"))
        {
            test();
        }
        else if (SCAN("perft file "))
        {
            strcpy(file, s+11);
            strip(file);
            perft_file( file );
        }
        else if (SCAN("perft"))
        {
            num = scan_int(s,"perft");
            perft( num );
        }
        else if (SCAN("ucinewgame"))
        {
            open_book();
            continue;
        }
        else if (SCAN("position"))
        {
            set_position(s);
        }
        else if (SCAN("go"))
        {
            go(s);
        }
        else if (SCAN("setoption name"))
        {
            set_option(s);
        }
    }
    close_book();
    #ifdef LOG
        close_log();
    #endif
}


/*
 *
 *
 * stop
 *      stop calculating as soon as possible,
 *      don't forget the "bestmove" and possibly the "ponder" token when finishing the search
 *
 * ponderhit
 *      the user has played the expected move. This will be sent if the engine was told to ponder on the same move
 *      the user has played. The engine should continue searching but switch from pondering to normal search.
 *
 * quit
 *      quit the program as soon as possible
 *
 */

void do_move(char *ini_moves, int from, int sz)
{
    char pv[6], str_move[6];
    int i, to;

    for (i = 0; i < sz; i++)
    {
        pv[i] = ini_moves[from + i];
    }
    pv[i] = 0;

    from = board.idx_moves;
    movegen();
    to = board.idx_moves;
    for (i = from; i < to; i++)
    {
        if (!strcmp(move2str(board.moves[i], str_move), pv))
        {
            make_move(board.moves[i]);
            return;
        }
    }
}

void set_position(char *line)
{
    char *ini_moves;
    char *ini_fen;
    char *ini_startpos;
    int from, sz;

    strip(line);

    ini_moves = strstr(line, "moves");
    ini_fen = strstr(line, "fen");
    ini_startpos = strstr(line, "startpos");

    if (ini_startpos)
    {
        init_board();
    }
    else if (ini_fen)
    {
        fen_board(ini_fen + 4);
    }

    if (ini_moves)
    {
        for (from = 6, sz = 0; ini_moves[from]; from++)
        {
            if (ini_moves[from] == ' ')
            {
                if (sz >= 4)
                {
                    do_move(ini_moves, from - sz, sz);
                }
                sz = 0;
            }
            else
            {
                sz++;
            }
        }
        if (sz >= 4)
        {

            do_move(ini_moves, from - sz, sz);
        }
    }


    /*
     * position [fen <fenstring> | startpos ]  moves <move1> .... <movei>
     * set up the position described in fenstring on the internal board and
     * play the moves on the internal chess board.
     * if the game was played  from the start position the string "startpos" will be sent
     * Note: no "new" command is needed. However, if this position is from a different game than
     * the last position sent to the engine, the GUI should have sent a "ucinewgame" inbetween.
     */
}


#define SCAN_GO(scan)    scan_int(line, scan)

void go(char *line)
{
    int wtime, btime, winc, binc, depth, movestogo, movetime;

    strip(line);

    wtime = SCAN_GO("wtime");
    btime = SCAN_GO("btime");
    winc = SCAN_GO("winc");
    binc = SCAN_GO("binc");
    depth = SCAN_GO("depth");
    movestogo = SCAN_GO("movestogo");
    movetime = SCAN_GO("movetime");

    if (!movetime)
    {
        if (!movestogo)
        {
            movestogo = 40;
        }
        if (board.color)
        {
            movetime = btime + movestogo * binc;
        }
        else
        {
            movetime = wtime + movestogo * winc;
        }
        movetime = movetime*9/(movestogo*10);
        #ifdef LOG
            fprintf(flog, "movetime=%d\n", movetime);
        #endif
    }
    if (!depth)
    {
        depth = INFINITE;
    }

    play(depth, movetime);

    /*
     * go
     * start calculating on the current position set up with the "position" command.
     * There are a number of commands that can follow this command, all will be sent in the same string.
     * If one command is not sent its value should be interpreted as it would not influence the search.
     * searchmoves <move1> .... <movei>
     *      restrict search to this moves only
     *      Example: After "position startpos" and "go infinite searchmoves e2e4 d2d4"
     *      the engine should only search the two moves e2e4 and d2d4 in the initial position.
     * ponder
     *      start searching in pondering mode.
     *      Do not exit the search in ponder mode, even if it's mate!
     *      This means that the last move sent in in the position string is the ponder move.
     *      The engine can do what it wants to do, but after a "ponderhit" command
     *      it should execute the suggested move to ponder on. This means that the ponder move sent by
     *      the GUI can be interpreted as a recommendation about which move to ponder. However, if the
     *      engine decides to ponder on a different move, it should not display any mainlines as they are
     *      likely to be misinterpreted by the GUI because the GUI expects the engine to ponder
     * on the suggested move.
     * wtime <x>
     *      white has x msec left on the clock
     * btime <x>
     *      black has x msec left on the clock
     * winc <x>
     *      white increment per move in mseconds if x > 0
     * binc <x>
     *      black increment per move in mseconds if x > 0
     * movestogo <x>
     * there are x moves to the next time control,
     *      this will only be sent if x > 0,
     *      if you don't get this and get the wtime and btime it's sudden death
     * depth <x>
     *      search x plies only.
     * nodes <x>
     * search x nodes only,
     * mate <x>
     *      search for a mate in x moves
     * movetime <x>
     *      search exactly x mseconds
     * infinite
     *      search until the "stop" command. Do not exit the search without being told so in this mode!
     */
}


void set_option(char *line)
{
    char name[64];
    char value[64];
    char *find_value;

    strip(line);
    if( strlen(line) > 64 ) return;

    strcpy(name, line+15);
    find_value = strstr(line, " value ");
    if(find_value)
    {
        strcpy(value, find_value+7);
        name[strlen(line+15)-strlen(value)-7] = '\0';
    }
    else
    {
        value[0] = '\0';
    }

    if(strcmp(name, "Personality") == 0)
    {
        set_personality_name( value );
    }
    else if(strcmp(name, "Min Time") == 0)
    {
        set_min_time( value );
    }
    else if(strcmp(name, "Max Time") == 0)
    {
        set_max_time( value );
    }
    else if(strcmp(name, "Hash") == 0)
    {
        set_hash( value );
    }
    else if(strcmp(name, "OwnBook") == 0)
    {
        set_ownbook( strcmp(value, "true") == 0 );
    }
    else if(strcmp(name, "OwnBookFile") == 0)
    {
        set_ownbookfile( value );
    }
    /* setoption name <id> [value <x>]
    	this is sent to the engine when the user wants to change the internal parameters
    	of the engine. For the "button" type no value is needed.
    	One string will be sent for each parameter and this will only be sent when the engine is waiting.
    	The name and value of the option in <id> should not be case sensitive and can inlude spaces.
    	The substrings "value" and "name" should be avoided in <id> and <x> to allow unambiguous parsing,
    	for example do not use <name> = "draw value".
    	Here are some strings for the example below:
    	   "setoption name Nullmove value true\n"
           "setoption name Selectivity value 3\n"
    	   "setoption name Style value Risky\n"
    	   "setoption name Clear Hash\n"
    	   "setoption name NalimovPath value c:\chess\tb\4;c:\chess\tb\5\n"
    */
}
