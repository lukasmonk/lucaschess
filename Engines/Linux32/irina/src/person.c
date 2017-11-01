#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef WIN32
    #include <windows.h>
#else
    #include <unistd.h>
#endif
#include <time.h>
#include "defs.h"
#include "protos.h"
#include "globals.h"

bool is_personality = false;
int person_random;
int person_advance;
int person_capture;
int person_material;
int person_sanity;
int person_queen;
int person_rook;
int person_bishop;
int person_knight;
int min_time = 0;
int max_time = 0;

bool set_personality(char* value, char* name, int random, int advance,
                     int capture, int material, int sanity,
                     int queen, int rook, int bishop, int knight, bool ownbook)
{
    if(strcmp(value, name) == 0)
    {
        is_personality = true;
        person_random = random;
        person_advance = advance;
        person_capture = capture;
        person_material = material;
        person_sanity = sanity;
        person_queen = queen;
        person_rook = rook;
        person_bishop = bishop;
        person_knight = knight;
        set_ownbook(ownbook);
        return true;
    }
    return false;
}

void set_personality_name(char * value)
{
    srand(time(NULL));
    is_personality = false;
    //                                        Rnd Adv  Cap Mat  San   Q    R   B    N    book
    if(set_personality(value, "Monkey", 100,  0,  0,  0,  0,  0,  0,  0,  0, false)) return;
    if(set_personality(value, "Donkey",  50, 30, 10, 10,  0,  1,  1,  1,  1, false)) return;
    if(set_personality(value, "Bull",    40, 40,  5, 15,  0,  2,  1,  1,  1, false)) return;
    if(set_personality(value, "Wolf",    30, 25, 30, 15,  0,  3,  2,  1,  1, false)) return;
    if(set_personality(value, "Lion",    20, 15, 30, 30,  5,  5,  3,  2,  2, false)) return;
    if(set_personality(value, "Rat",     15, 10, 25, 35, 15,  8,  4,  3,  3,  true)) return;
    if(set_personality(value, "Snake",    5, 15, 20, 40, 20, 10,  5,  3,  3,  true)) return;
    if(set_personality(value, "Random", 100,  0,  0,  0,  0,  0,  0,  0,  0, false)) return;
    if(set_personality(value, "Advance",  0,100,  0,  0,  0,  0,  0,  0,  0, false)) return;
    if(set_personality(value, "Capture",  0,  0,100,  0,  0,  0,  0,  0,  0, false)) return;

    if(!strcmp(value, "Material"))
    {
        functionEval = &eval_material;
    }
    else if(!strcmp(value, "Steven"))
    {
        functionEval = &eval_steven;
    }
    else if(!strcmp(value, "Irina"))
    {
        functionEval = &eval;
    }

    /*
       	 Random Advance	Capt Material Sanity
    Proceso: Todos los movimientos,
            Random: se suma a todos
            Advance, se mira todas las separaciones, se hace la media y todos que esten por encima de la media, se suma
            Capture, todos los que sean capturas se suma
            Material, se hace una play de material al nivel 2
            Sanity, play irina

            Se suman todos los puntos.
            Random de esos puntos
            Se elige el movimiento que salga.
    */

}

int randint( int limit )
{
    int divisor = RAND_MAX/(limit+1);
    int retval;

    do
    {
        retval = rand() / divisor;
    }
    while (retval >= limit);

    return retval;
}


int randchoice( int tam, int x[] )
{
    int sum;
    int i;
    int r;

    for(i=0, sum=0; i < tam; i++)
    {
        sum += x[i];
    }
    r = randint( sum );
    for(i=0, sum=0; i < tam; i++)
    {
        sum += x[i];
        if(sum >= r) return i;
    }
    return tam-1;
}

void set_min_time(char * value)
{
    int t;

    sscanf(value, "%d", &t);
    if( t >= 0 && t <= 99 ) min_time = t;

}

void set_max_time(char * value)
{
    int t;

    sscanf(value, "%d", &t);
    if( t >= 0 && t <= 99 ) max_time = t;

}

void person_sleep(Bitmap mstime)
{
    Bitmap time_end;

    if( mstime == 0 )
    {
        if(max_time > min_time)
        {
            mstime = (randint(max_time-min_time+1) + min_time)*1000;
        }
    }
    if( mstime > 0 )
    {
        time_end = get_ms() + mstime;

        while( get_ms() < time_end && !bioskey())
        {
            #ifdef WIN32
                Sleep(100);
            #else
                usleep(100000);
            #endif
        }
    }
}

void time_test( Bitmap mstime )
{
    Bitmap ms;

    if(max_time > min_time)
    {
        ms = (randint(max_time-min_time+1) + min_time)*1000;
        if( ms > mstime )
        {
            person_sleep(ms-mstime);
        }
    }
}


void play_simul(int depth, int mstime, Move move )
{
    int working_depth;
    char bestmove[6];

    move2str(move, bestmove);

    movegen();


    board_reset();

    for (working_depth = 1; working_depth <= depth; working_depth++)
    {
        printf("info depth %d score 1 time 1 nodes 1 nps 1 pv %s\n", working_depth, bestmove);
    }
    person_sleep(mstime);
    printf("bestmove %s\n", bestmove);

}

Move list_moves[512];
int tam_list_moves;

void all_moves_base(bool is_capture)
{
    int k, desde, hasta;
    Move move;

    desde = board.idx_moves;
    if( is_capture )
    {

        k = movegenCaptures();
        if( k == 0 )
        {
            movegen();
        }
    }
    else
    {
        movegen();
    }
    hasta = board.idx_moves;
    tam_list_moves = 0;
    for (k = desde; k < hasta; k++)
    {
        move = board.moves[k];
        make_move(move);
        if( repetitions()<3)
        {
            list_moves[tam_list_moves] = move;
            tam_list_moves++;
        }
        unmake_move();
    }
    if( tam_list_moves == 0)
    {
        list_moves[tam_list_moves] = board.moves[desde];
        tam_list_moves = 0;
    }
}
void all_moves()
{
    all_moves_base(false);
}

void all_captures()
{
    all_moves_base(true);
}


void play_random(int depth, int time)
{
    int sel = 0;
    all_moves();
    if( tam_list_moves > 1 )
    {
        sel = randint(tam_list_moves);
    }
    play_simul(depth, time, list_moves[sel]);
}

void play_advance(int depth, int time)
{
    int sel = 0;
    int min_distance, dist;
    int i;
    int rivalking = first_one((board.color) ? board.white_king : board.black_king);

    all_moves();
    min_distance = 99;
    if( tam_list_moves > 1 )
    {
        for(i=0; i< tam_list_moves; i++)
        {
            dist = DISTANCE[list_moves[i].to][rivalking];
            if( dist < min_distance )
            {
                sel = i;
                min_distance = dist;
            }
        }
    }
    play_simul(depth, time, list_moves[sel]);
}

void play_capture(int depth, int time)
{
    int sel = 0;
    all_captures();
    if( tam_list_moves > 1 )
    {
        sel = randint(tam_list_moves);
    }
    play_simul(depth, time, list_moves[sel]);
}

void play_material(int depth, int mstime)
{
    functionEval = &eval_material;
    play_irina(depth, mstime);
    functionEval = &eval;
}

void play_person(int depth, int time)
{
    int x[] = {person_random, person_advance, person_capture, person_material, person_sanity};
    switch (randchoice(5, x))
    {
    case 0:
        printf("info string playing Random\n");
        play_random(depth, time);
        break;
    case 1:
        printf("info string playing Advance\n");
        play_advance(depth, time);
        break;
    case 2:
        printf("info string playing Capture\n");
        play_capture(depth, time);
        break;
    case 3:
        if( depth < 0) depth = 1;
        printf("info string playing Material (%d,%d)\n", depth, time);
        play_material(depth, time);
        break;
    case 4:
        if( depth < 0) depth = 1;
        printf("info string playing Sanity (%d,%d)\n", depth, time);
        play_irina(depth, time);
        break;
    }
}
