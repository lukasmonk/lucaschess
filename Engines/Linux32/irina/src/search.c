#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "defs.h"
#include "protos.h"
#include "globals.h"

bool ok_time_kb;
Bitmap time_ini;
Bitmap time_end;
Bitmap time_last;

uint32_t xxx;
#define TEST_KEY_TIME    32543*2
#define MSG_INTERVAL     1800

int working_depth;

int triangularLength[MAX_PLY];
Move triangularArray[MAX_PLY][MAX_PLY];

Bitmap historia[MAX_PLY];
int len_historia;

typedef struct MoveOrder
{
    Move move;
    int score;
} MoveOrder;
MoveOrder moveOrder[MAX_PLY];
int maxMoveOrder;

FunctionEval functionEval = &eval;

void orderMoves( int ply );
void quick_sort(int low, int high);

// void show_move(Move move) {
    // printf("[%c %s%s ", NAMEPZ[move.piece], POS_AH[move.from], POS_AH[move.to]);
    // if (move.capture) {
        // printf("x%c ", NAMEPZ[move.capture]);
    // }
    // if (move.promotion) {
        // printf("prom%c ", NAMEPZ[move.promotion]);
    // }
    // if (move.is_ep) {
        // printf("ep ");
    // }
    // if (move.is_2p) {
        // printf("2p ");
    // }
    // if (move.is_castle) {
        // printf("castle %d ", move.is_castle);
    // }
    // printf("]");
// }

void play(int depth, int time)
{
    if( is_personality ) return play_person( depth, time );
    else play_irina( depth, time );
}

void play_irina(int depth, int time)
{
    int score;
    int i;
    char str_score[20];
    char str_move[20];
    char bestmove[6], ponder[6];
    Bitmap ms;
    char fen[100];

    if( using_book() )
    {
        board_fen(fen);
        if( check_book(fen, bestmove) )
        {
            printf("bestmove %s\n", bestmove);
            #ifdef LOG
                fprintf(flog, "bestmove %s\n", bestmove);
            #endif
            return;
        }
        else
        {
            close_book();
        }
    }

    ok_time_kb = true;
    time_ini = get_ms();
    if( time ) time_end = time_ini + time;
    else time_end = 0;
    time_last = time_ini;
    xxx = TEST_KEY_TIME;
    inodes = 0;
    bestmove[0] = '\0';
    ponder[0] = '\0';
    if (depth<=0) depth = 120;

    len_historia = board.ply;
    for(i=0; i < board.ply; i++)
        historia[i] = board.history[i].hashkey;
    board_reset();

    for (working_depth = 1; working_depth <= depth && ok_time_kb; working_depth++)
    {
        memset(triangularLength, 0, sizeof (triangularLength));
        memset(triangularArray, 0, sizeof (triangularArray));

        score = alphaBeta(-BIGNUMBER, +BIGNUMBER, working_depth, 0, working_depth+2);

        if (ok_time_kb)
        {
            if ((score > 9000) || (score < -9000))
            {
                sprintf(str_score, "mate %d", (score > 9000) ? 10000 - score : -(10000 + score));
            }
            else
            {
                sprintf(str_score, "cp %d", score);
            }
            ms = get_ms() - time_ini;
            if (ms == 0)
            {
                ms = 1;
            }
            printf("info depth %d score %s time %lu nodes %lu nps %lu pv",
                   working_depth, str_score, (long unsigned int) ms, (long unsigned int) inodes,
                   (long unsigned int) (inodes * 1000 / ms));
            #ifdef LOG
                fprintf(flog, "info depth %d score %s time %lu nodes %lu nps %lu pv",
                       working_depth, str_score, (long unsigned int) ms, (long unsigned int) inodes,
                       (long unsigned int) (inodes * 1000 / ms));
            #endif
            for (i = 0; i < triangularLength[0]; i++)
            {
                printf(" %s", move2str(triangularArray[0][i], str_move));
                #ifdef LOG
                    fprintf(flog, " %s", move2str(triangularArray[0][i], str_move));
                #endif
            }
            printf("\n");
            #ifdef LOG
                fprintf(flog, "\n");
            #endif
            if (triangularLength[0])
            {
                move2str(triangularArray[0][0], bestmove);
                if (triangularLength[0] > 1)
                {
                    move2str(triangularArray[0][1], ponder);
                }
            }
            if ((score > 9000) || (score < -9000))
            {
                break;
            }
        }
    }
    if (!bestmove[0])
    {
        if (triangularLength[0])
        {
            move2str(triangularArray[0][0], bestmove);
            if (triangularLength[0] > 1)
            {
                move2str(triangularArray[0][1], ponder);
            }
        }
    }
    if (bestmove[0])
    {
        if(is_personality && time==0) // From play_material
        {
            person_sleep(0);
        }
        printf("bestmove %s", bestmove);
        #ifdef LOG
            fprintf(flog, "bestmove %s", bestmove);
        #endif
        if (ponder[0])
        {
            printf(" ponder %s", ponder);
            #ifdef LOG
                fprintf(flog, " ponder %s", ponder);
            #endif
        }
        printf("\n");
        #ifdef LOG
            fprintf(flog, "\n");
        #endif
    }
    if( max_time )
    {
        time_test(get_ms()-time_ini);
    }
}

int noMovesScore(int ply)
{
    if (inCheck())
    {
        return -MATESCORE + ply / 2 + 1;
    }
    return DRAWSCORE;
}

inline void test_time()
{
    Bitmap ms;
    if (--xxx == 0)
    {
        ms = get_ms();
        if ((time_end && (time_end < ms)) || bioskey())
        {
            ok_time_kb = false;
        }
        xxx = TEST_KEY_TIME;

        if (ms - time_last > MSG_INTERVAL)
        {
            time_last = ms;
            ms -= time_ini;
            if (ms)
            {
                printf("info depth %d time %lu nodes %lu nps %lu\n",
                       working_depth,
                       (long unsigned int) ms, (long unsigned int) inodes,
                       (long unsigned int) (inodes * 1000 / ms));
                #ifdef LOG
                    fprintf(flog, "info depth %d time %lu nodes %lu nps %lu\n",
                        working_depth,
                        (long unsigned int) ms, (long unsigned int) inodes,
                        (long unsigned int) (inodes * 1000 / ms));
                #endif
            }
        }
    }
}

int quiescence(int alpha, int beta, int ply, int max_ply)
{
    int k, j;
    int score;

    test_time();

    if (inCheck()) {
          return alphaBeta(alpha, beta, 1, ply, max_ply);
    }

    triangularLength[ply] = ply;

    score = functionEval();

    if( ply > max_ply) return score;

    if (score >= beta)
    {
        return beta;
    }
    if (score > alpha)
    {
        alpha = score;
    }

    movegenCaptures();
    for (k = board.ply_moves[ply]; k < board.ply_moves[ply + 1] && ok_time_kb; k++)
    {
        make_move(board.moves[k]);
        inodes++;
        score = -quiescence(-beta, -alpha, ply + 1, max_ply);
        unmake_move();
        if (score >= beta)
        {
            return beta;
        }
        if (score > alpha)
        {
            alpha = score;
            triangularArray[ply][ply] = board.moves[k];
            for (j = ply + 1; j < triangularLength[ply + 1]; j++)
            {
                triangularArray[ply][j] = triangularArray[ply + 1][j];
            }
            triangularLength[ply] = triangularLength[ply + 1];
        }
    }
    return alpha;
}

int alphaBeta(int alpha, int beta, int depth, int ply, int max_ply)
{
    int score, best_score;
    int desde, hasta;
    unsigned k, j;
    Move move;

    test_time();

    if (depth == 0)
    {
        return quiescence(alpha, beta, ply, max_ply);
    }

    if (!movegen())
    {
        return noMovesScore(ply);
    }

    if( repetitions() >= 3) {
        return DRAWSCORE;
    }

    desde = board.ply_moves[ply];
    hasta = board.ply_moves[ply + 1];
    best_score = -BIGNUMBER;
    orderMoves(ply);
    for (k = desde; k < hasta && ok_time_kb; k++)
    {
        move = board.moves[k];
        make_move(move);
        inodes++;
        score = -alphaBeta(-beta, -alpha, depth - 1, ply + 1, max_ply);
        unmake_move();

        if (score > best_score) {
            best_score = score;
            if (score >= beta)
            {
                return beta;
            }
            if (score > alpha)
            {
                alpha = score;
                triangularArray[ply][ply] = move;
                for (j = ply + 1; j < triangularLength[ply + 1]; j++)
                {
                    triangularArray[ply][j] = triangularArray[ply + 1][j];
                }
                if (triangularLength[ply + 1]) triangularLength[ply] = triangularLength[ply + 1];
                else triangularLength[ply] = 1;
            }
        }
    }
    return best_score;
}


void orderMoves( int ply )
{
    unsigned k, i, n;

    for (i = 0, n = 0, k = board.ply_moves[ply]; k < board.ply_moves[ply + 1]; k++, i++)
    {
        moveOrder[i].move = board.moves[k];
        n++;
        make_move(board.moves[k]);
        moveOrder[i].score = functionEval();
        unmake_move();
    }
    quick_sort( 0, n-1);
    for (i = 0, k = board.ply_moves[ply]; i < n; k++, i++)
    {
        board.moves[k] = moveOrder[i].move;
    }
}

int repetitions()
{
    int i, ilast, rep;
    Bitmap hashkey = board.hashkey;
    rep = 1;
    ilast = board.ply - board.fifty;
    if(ilast < 0)
    {
        for (i = len_historia - 1; i >= (len_historia+ilast)-1 && i; i--)
        {
            if( historia[i] == hashkey ) rep++;
        }
    }
    for (i = board.ply - 2; i >= ilast && i; i -= 2)
    {
        if (board.history[i].hashkey == hashkey) rep++;
    }
    return rep;
}



void quick_sort(int low, int high)
{
    int pivot,j,i;
    MoveOrder temp;

    if( low < high )
    {
        pivot = low;
        i = low;
        j = high;

        while( i < j )
        {
            while( (moveOrder[i].score <= moveOrder[pivot].score) && (i<high) ) i++;

            while(moveOrder[j].score>moveOrder[pivot].score) j--;

            if(i<j)
            {
                temp=moveOrder[i];
                moveOrder[i]=moveOrder[j];
                moveOrder[j]=temp;
            }
        }

        temp=moveOrder[pivot];
        moveOrder[pivot]=moveOrder[j];
        moveOrder[j]=temp;
        quick_sort(low,j-1);
        quick_sort(j+1,high);
    }
}

