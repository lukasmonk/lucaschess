
// search.cpp

// includes

#include <csetjmp>
#include <cstring>
#include <windows.h> // LC

#include "attack.h"
#include "board.h"
#include "book.h"
#include "colour.h"
#include "list.h"
#include "material.h"
#include "move.h"
#include "move_do.h"
#include "move_gen.h"
#include "option.h"
#include "pawn.h"
#include "protocol.h"
#include "pv.h"
#include "search.h"
#include "search_full.h"
#include "sort.h"
#include "trans.h"
#include "util.h"
#include "value.h"

// constants

static const bool UseCpuTime = false; // false

static const bool UseShortSearch = true;
static const int ShortSearchDepth = 1;

static const bool DispBest = true; // true
static const bool DispDepthStart = true; // true
static const bool DispDepthEnd = true; // true
static const bool DispRoot = true; // true
static const bool DispStat = true; // true

static const int EasyThreshold = 150;
static const double EasyRatio = 0.20;

static const double EarlyRatio = 0.60;

static const int BadThreshold = 50; // 50
static const bool UseExtension = true;

// variables

static search_multipv_t save_multipv[MultiPVMax];

bool trans_endgame;
search_param_t SearchStack[HeightMax];

int maxnps;

search_input_t SearchInput[1];
search_info_t SearchInfo[1];
search_root_t SearchRoot[1];
search_current_t SearchCurrent[1];
search_best_t SearchBest[MultiPVMax];

// prototypes

static void search_send_stat ();

// functions

// depth_is_ok()

bool depth_is_ok(int depth) {

    return depth > -128 && depth < DepthMax;
}

// height_is_ok()

bool height_is_ok(int height) {

    return height >= 0 && height < HeightMax;
}

// search_clear()

void search_clear() {

    maxnps = option_get_int("Max NPS");
    if( maxnps == 999999 ) maxnps = 0;

    // SearchInput

    SearchInput->infinite = false;
    SearchInput->depth_is_limited = false;
    SearchInput->depth_limit = 0;
    SearchInput->time_is_limited = false;
    SearchInput->time_limit_1 = 0.0;
    SearchInput->time_limit_2 = 0.0;



    // SearchInfo

    SearchInfo->can_stop = false;
    SearchInfo->stop = false;
    SearchInfo->check_nb = 10000; // was 100000
    SearchInfo->check_inc = 10000; // was 100000
    SearchInfo->last_time = 0.0;

    // SearchBest

    SearchBest[SearchCurrent->multipv].move = MoveNone;
    SearchBest[SearchCurrent->multipv].value = 0;
    SearchBest[SearchCurrent->multipv].flags = SearchUnknown;
    PV_CLEAR(SearchBest[SearchCurrent->multipv].pv);

    // SearchRoot

    SearchRoot->depth = 0;
    SearchRoot->move = MoveNone;
    SearchRoot->move_pos = 0;
    SearchRoot->move_nb = 0;
    SearchRoot->last_value = 0;
    SearchRoot->bad_1 = false;
    SearchRoot->bad_2 = false;
    SearchRoot->change = false;
    SearchRoot->easy = false;
    SearchRoot->flag = false;

    // SearchCurrent

    SearchCurrent->max_depth = 0;
    SearchCurrent->node_nb = 0;
    SearchCurrent->time = 0.0;
    SearchCurrent->speed = 0.0;
    SearchCurrent->cpu = 0.0;
}

// search()

void search() {

    int move;
    int depth;
    int i;
    bool search_ready;


    /*for (i = 0; i < MultiPVMax; i++){
      save_multipv[SearchCurrent->multipv].mate = 0;
      save_multipv[SearchCurrent->multipv].depth = 0;
      save_multipv[SearchCurrent->multipv].max_depth = 0;
      save_multipv[SearchCurrent->multipv].value = 0;
      save_multipv[SearchCurrent->multipv].time = 0;
      save_multipv[SearchCurrent->multipv].node_nb = 0;
      strcpy(save_multipv[SearchCurrent->multipv].pv_string,"");
    }*/

    for (i = 0; i < MultiPVMax; i++) {
        save_multipv[i].mate = 0;
        save_multipv[i].depth = 0;
        save_multipv[i].max_depth = 0;
        save_multipv[i].value = 0;
        save_multipv[i].time = 0;
        save_multipv[i].node_nb = 0;
        strcpy(save_multipv[i].pv_string,"");
    }


    SearchInput->multipv = option_get_int("MultiPV")-1;
    SearchCurrent->multipv = 0;


    ASSERT(board_is_ok(SearchInput->board));

    // opening book

    if (option_get_bool("OwnBook") && !SearchInput->infinite) {

        move = book_move(SearchInput->board);

        if (move != MoveNone) {

            // play book move

            SearchBest[SearchCurrent->multipv].move = move;
            SearchBest[SearchCurrent->multipv].value = 1;
            SearchBest[SearchCurrent->multipv].flags = SearchExact;
            SearchBest[SearchCurrent->multipv].depth = 1;
            SearchBest[SearchCurrent->multipv].pv[0] = move;
            SearchBest[SearchCurrent->multipv].pv[1] = MoveNone;

            search_update_best();

            return;
        }
    }

    // SearchInput

    gen_legal_moves(SearchInput->list,SearchInput->board);

    if (LIST_SIZE(SearchInput->list) < SearchInput->multipv+1) {
        SearchInput->multipv = LIST_SIZE(SearchInput->list)-1;
    }

    if (LIST_SIZE(SearchInput->list) <= 1) {
        SearchInput->depth_is_limited = true;
        SearchInput->depth_limit = 4; // was 1
    }

    // SearchInfo

    if (setjmp(SearchInfo->buf) != 0) {
        ASSERT(SearchInfo->can_stop);
        ASSERT(SearchBest->move!=MoveNone);
        search_update_current();
        return;
    }

    // SearchRoot

    list_copy(SearchRoot->list,SearchInput->list);

    // SearchCurrent

    board_copy(SearchCurrent->board,SearchInput->board);
    my_timer_reset(SearchCurrent->timer);
    my_timer_start(SearchCurrent->timer);

    // init

    trans_inc_date(Trans);

    sort_init();
    search_full_init(SearchRoot->list,SearchCurrent->board);

    // analyze game for evaluation

    if (SearchCurrent->board->piece_size[White] < 3 && SearchCurrent->board->piece_size[Black] < 3) {
        trans_endgame = true;
    }
    else {
        trans_endgame = false;
    }


    // iterative deepening

    search_ready = false;

    for (depth = 1; depth < DepthMax; depth++) {
        for (SearchCurrent->multipv = 0; SearchCurrent->multipv <= SearchInput->multipv; SearchCurrent->multipv++) {

            if (DispDepthStart && SearchCurrent->multipv == 0) send("info depth %d",depth);

            SearchCurrent->max_extensions = depth * 10;
            SearchRoot->bad_1 = false;
            SearchRoot->change = false;

            board_copy(SearchCurrent->board,SearchInput->board);

            if (UseShortSearch && depth <= ShortSearchDepth) {
                search_full_root(SearchRoot->list,SearchCurrent->board,depth,SearchShort);
            } else {
                search_full_root(SearchRoot->list,SearchCurrent->board,depth,SearchNormal);
            }

            search_update_current();

            if (DispDepthEnd && SearchCurrent->multipv == SearchInput->multipv) {
                send("info depth %d seldepth %d time %.0f nodes " S64_FORMAT " nps %.0f",depth,SearchCurrent->max_depth,SearchCurrent->time*1000.0,SearchCurrent->node_nb,SearchCurrent->speed);
            }

            // update search info

            if (depth >= 1) SearchInfo->can_stop = true;

            if (depth == 1
                    && LIST_SIZE(SearchRoot->list) >= 2
                    && LIST_VALUE(SearchRoot->list,0) >= LIST_VALUE(SearchRoot->list,1) + EasyThreshold) {
                SearchRoot->easy = true;
            }

            if (depth > 1) {
                SearchRoot->bad_2 = SearchRoot->bad_1;
                SearchRoot->bad_1 = false;
                ASSERT(SearchRoot->bad_2==(SearchBest->value<=SearchRoot->last_value-BadThreshold));
            }

            SearchRoot->last_value = SearchBest[SearchCurrent->multipv].value;

            // stop search?

            if (SearchInput->depth_is_limited && SearchCurrent->multipv >= SearchInput->multipv
                    && depth >= SearchInput->depth_limit) {
                SearchRoot->flag = true;
            }

            if (SearchInput->time_is_limited
                    && SearchCurrent->time * 2 >= SearchInput->time_limit_1
                    && !SearchRoot->bad_2) {
                SearchRoot->flag = true;
            }

            if (SearchInput->time_is_limited
                    && SearchCurrent->time >= SearchInput->time_limit_1 * EasyRatio
                    && SearchRoot->easy) {
                ASSERT(!SearchRoot->bad_2);
                ASSERT(!SearchRoot->change);
                SearchRoot->flag = true;
            }

            if (SearchInput->time_is_limited
                    && SearchCurrent->time >= SearchInput->time_limit_1 * EarlyRatio
                    && !SearchRoot->bad_2
                    && !SearchRoot->change) {
                SearchRoot->flag = true;
            }

            if (SearchInfo->can_stop
                    && (SearchInfo->stop || (SearchRoot->flag && !SearchInput->infinite))) {
                search_ready = true;
                break;
            }
        }
        if (search_ready)
            break;
    }
}

// search_update_best()

void search_update_best() {

    int move, value, flags, depth, max_depth;
    const mv_t * pv;
    double time;
    sint64 node_nb;
    int mate, i, z;
    bool found;
    char move_string[256], pv_string[512];

    search_update_current();

    if (DispBest) {

        move = SearchBest[SearchCurrent->multipv].move;
        value = SearchBest[SearchCurrent->multipv].value;
        flags = SearchBest[SearchCurrent->multipv].flags;
        depth = SearchBest[SearchCurrent->multipv].depth;
        pv = SearchBest[SearchCurrent->multipv].pv;

        max_depth = SearchCurrent->max_depth;
        time = SearchCurrent->time;
        node_nb = SearchCurrent->node_nb;

        move_to_string(move,move_string,256);
        pv_to_string(pv,pv_string,512);

        mate = value_to_mate(value);

        if (SearchCurrent->multipv == 0) {
            save_multipv[SearchCurrent->multipv].mate = mate;
            save_multipv[SearchCurrent->multipv].depth = depth;
            save_multipv[SearchCurrent->multipv].max_depth = max_depth;
            save_multipv[SearchCurrent->multipv].value = value;
            save_multipv[SearchCurrent->multipv].time = time*1000.0;
            save_multipv[SearchCurrent->multipv].node_nb = node_nb;
            strcpy(save_multipv[SearchCurrent->multipv].pv_string,pv_string);
        }
        else {
            found = false;
            for (i = 0; i < SearchCurrent->multipv; i++) {
                if (save_multipv[i].value < value) {
                    found = true;
                    break;
                }
            }
            if (found) {

                for (z = SearchCurrent->multipv; z > i; z--) {
                    save_multipv[z].mate = save_multipv[z-1].mate;
                    save_multipv[z].depth = save_multipv[z-1].depth;
                    save_multipv[z].max_depth = save_multipv[z-1].max_depth;
                    save_multipv[z].value = save_multipv[z-1].value;
                    save_multipv[z].time = save_multipv[z-1].time;
                    save_multipv[z].node_nb = save_multipv[z-1].node_nb;
                    strcpy(save_multipv[z].pv_string,save_multipv[z-1].pv_string);
                }

                save_multipv[i].mate = mate;
                save_multipv[i].depth = depth;
                save_multipv[i].max_depth = max_depth;
                save_multipv[i].value = value;
                save_multipv[i].time = time*1000.0;
                save_multipv[i].node_nb = node_nb;
                strcpy(save_multipv[i].pv_string,pv_string);

            }
            else {
                save_multipv[SearchCurrent->multipv].mate = mate;
                save_multipv[SearchCurrent->multipv].depth = depth;
                save_multipv[SearchCurrent->multipv].max_depth = max_depth;
                save_multipv[SearchCurrent->multipv].value = value;
                save_multipv[SearchCurrent->multipv].time = time*1000.0;
                save_multipv[SearchCurrent->multipv].node_nb = node_nb;
                strcpy(save_multipv[SearchCurrent->multipv].pv_string,pv_string);
            }
        }

        if (depth > 1 || (depth == 1 && SearchCurrent->multipv == SearchInput->multipv)) {

            for (i = 0; i <= SearchInput->multipv; i++) {

                if (save_multipv[i].mate == 0) {

                    // normal evaluation

                    if (false) {
                    } else if (flags == SearchExact) {
                        send("info multipv %d depth %d seldepth %d score cp %d time %.0f nodes " S64_FORMAT " pv %s",i+1,save_multipv[i].depth,save_multipv[i].max_depth,save_multipv[i].value,save_multipv[i].time,save_multipv[i].node_nb,save_multipv[i].pv_string);
                    } else if (flags == SearchLower) {
                        send("info multipv %d depth %d seldepth %d score cp %d lowerbound time %.0f nodes " S64_FORMAT " pv %s",i+1,save_multipv[i].depth,save_multipv[i].max_depth,save_multipv[i].value,save_multipv[i].time,save_multipv[i].node_nb,save_multipv[i].pv_string);
                    } else if (flags == SearchUpper) {
                        send("info multipv %d depth %d seldepth %d score cp %d upperbound time %.0f nodes " S64_FORMAT " pv %s",i+1,save_multipv[i].depth,save_multipv[i].max_depth,save_multipv[i].value,save_multipv[i].time,save_multipv[i].node_nb,save_multipv[i].pv_string);
                    }

                } else {

                    // mate announcement

                    if (false) {
                    } else if (flags == SearchExact) {
                        send("info multipv %d depth %d seldepth %d score mate %d time %.0f nodes " S64_FORMAT " pv %s",i+1,save_multipv[i].depth,save_multipv[i].max_depth,save_multipv[i].mate,save_multipv[i].time,save_multipv[i].node_nb,save_multipv[i].pv_string);
                    } else if (flags == SearchLower) {
                        send("info multipv %d depth %d seldepth %d score mate %d lowerbound time %.0f nodes " S64_FORMAT " pv %s",i+1,save_multipv[i].depth,save_multipv[i].max_depth,save_multipv[i].mate,save_multipv[i].time,save_multipv[i].node_nb,save_multipv[i].pv_string);
                    } else if (flags == SearchUpper) {
                        send("info multipv %d depth %d seldepth %d score mate %d upperbound time %.0f nodes " S64_FORMAT " pv %s",i+1,save_multipv[i].depth,save_multipv[i].max_depth,save_multipv[i].mate,save_multipv[i].time,save_multipv[i].node_nb,save_multipv[i].pv_string);
                    }
                }
            }
        }
    }

    // update time-management info

    if (SearchBest[SearchCurrent->multipv].depth > 1) {
        if (SearchBest[SearchCurrent->multipv].value <= SearchRoot->last_value - BadThreshold) {
            SearchRoot->bad_1 = true;
            SearchRoot->easy = false;
            SearchRoot->flag = false;
        } else {
            SearchRoot->bad_1 = false;
        }
    }
}

// search_update_root()

void search_update_root() {

    int move, move_pos, move_nb;
    double time;
    sint64 node_nb;
    char move_string[256];

    if (DispRoot) {

        search_update_current();

        if (SearchCurrent->time >= 1.0) {

            move = SearchRoot->move;
            move_pos = SearchRoot->move_pos;
            move_nb = SearchRoot->move_nb;

            time = SearchCurrent->time;
            node_nb = SearchCurrent->node_nb;

            move_to_string(move,move_string,256);

            send("info currmove %s currmovenumber %d",move_string,move_pos+1);
        }
    }
}

// search_update_current()

void search_update_current() {

    my_timer_t *timer;
    sint64 node_nb;
    double time, speed, cpu;

    timer = SearchCurrent->timer;
    node_nb = SearchCurrent->node_nb;
    time = (UseCpuTime) ? my_timer_elapsed_cpu(timer) : my_timer_elapsed_real(timer);
    speed = (time > 0.0) ? double(node_nb) / time : 0.0;
    cpu = my_timer_cpu_usage(timer);

    if ( maxnps ) {
        while( speed > 1.0*maxnps ) {
            Sleep(100);
            time = (UseCpuTime) ? my_timer_elapsed_cpu(timer) : my_timer_elapsed_real(timer);
            speed = (time > 0.0) ? double(node_nb) / time : 0.0;
            cpu = my_timer_cpu_usage(timer);
        }
    }
    SearchCurrent->time = time;
    SearchCurrent->speed = speed;
    SearchCurrent->cpu = cpu;
}

// search_check()

void search_check() {

    search_send_stat();

    event();

    if (SearchInput->depth_is_limited
            && SearchRoot->depth > SearchInput->depth_limit) {
        SearchRoot->flag = true;
    }

    if (SearchInput->time_is_limited
            && SearchCurrent->time >= SearchInput->time_limit_2) {
        SearchRoot->flag = true;
    }

    if (SearchInput->time_is_limited
            && SearchCurrent->time >= SearchInput->time_limit_1
            && !SearchRoot->bad_1
            && !SearchRoot->bad_2
            && (!UseExtension || SearchRoot->move_pos == 0)) {
        SearchRoot->flag = true;
    }

    if (SearchInfo->can_stop
            && (SearchInfo->stop || (SearchRoot->flag && !SearchInput->infinite))) {
        longjmp(SearchInfo->buf,1);
    }
}

// search_send_stat()

static void search_send_stat() {

    double time, speed, cpu;
    sint64 node_nb;


    search_update_current();

    if (DispStat && SearchCurrent->time >= SearchInfo->last_time + 1.0) { // at least one-second gap

        SearchInfo->last_time = SearchCurrent->time;

        time = SearchCurrent->time;
        speed = SearchCurrent->speed;
        cpu = SearchCurrent->cpu;
        node_nb = SearchCurrent->node_nb;

        send("info time %.0f nodes " S64_FORMAT " nps %.0f cpuload %.0f",time*1000.0,node_nb,speed,cpu*1000.0);

        trans_stats(Trans);
    }
}

// end of search.cpp

