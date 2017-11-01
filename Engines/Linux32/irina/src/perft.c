#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include "defs.h"
#include "protos.h"
#include "globals.h"


Bitmap xperft(int depth) {
    Bitmap x, k, desde, hasta, r;

    desde = (Bitmap) board.ply_moves[board.ply - 1];
    hasta = (Bitmap) board.ply_moves[board.ply];

    if (depth > 1) {
        x = 0;
        // xfen();xl();
        for (k = desde; k < hasta; k++) {
            // b0 = board; //DBG

            make_move(board.moves[k]);

            movegen();

            r = xperft(depth - 1);
            x += r;
            unmake_move();

        }
        return x;
    } else {
        return hasta - desde;
    }
}

Bitmap calc_perft(char *fen, int depth) {
    fen_board(fen);
    movegen();
    return xperft(depth);
}

void perft_file(char *file) {
    FILE *f;
    char s[256];
    char fen[256];
    int nmoves, ln;
    int depth;
    Bitmap ms, ds;
    char id[100];
    Bitmap x;
    Bitmap inx;

    ms = get_ms();
    inx = 0;
    ln = 0;

    f = fopen(file, "rb");
    if( ! f ) {
        printf( "%s can't be open\n", file );
        return;
    }
    strcpy( id, "-" );
    while (fgets(s, 256, f)) {
        ++ln;
        if (!strncmp(s, "id ", 3)) {
            strcpy(id, s + 4);
            strip(id);
        } else if (!strncmp(s, "epd  ", 4)) {
            strcpy(fen, s + 4);
            strip(fen);
            strcat(fen, " 0 1");
        } else if (!strncmp(s, "perft ", 6)) {
            sscanf(s + 6, "%d %d", &depth, &nmoves);
            printf( "%5d: [%s] depth:%2d must be:%10d -> ", ln, fen, depth, nmoves );
            x = calc_perft(fen, depth);
            if (nmoves == x) printf( "ok\n" );
            else {
                printf( "ERROR calculated:%d\n", (int)x );
                break;
            }
            inx += x;
        }
    }
    fclose(f);

    ds = get_ms() - ms;
    printf("\nTotal:%lu ms", (long unsigned int) ds);
    if( ds ) {
        printf( " (%lu)", (long unsigned int) (inx * 1000 / ds));
    }
    printf( "\n");
}

void perft(int depth) {
    Bitmap ms, ds;
    Bitmap rs;

    ms = get_ms();
    board_reset();
    movegen();
    rs = xperft(depth);
    ds = get_ms() - ms;

    printf("Total:%lu ", (long unsigned int) rs);
    if( ds ) {
        printf( " (%lu positions/second)", (long unsigned int) (rs * 1000 / ds));
    }
    printf( "\n");

}

void test(void)
{
    char * fen = "8/8/2p5/8/8/4k3/2p1q3/7K w - - 0 2";
    fen_board(fen);
    play( 4, 0 );
}
