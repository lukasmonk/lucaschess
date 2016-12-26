
// main.cpp

// includes

#include <cstdio>
#include <cstdlib>
#include <string.h>

#if defined _WIN32
#include <windows.h>
#endif

#include "attack.h"
#include "book.h"
#include "hash.h"
#include "move_do.h"
#include "option.h"
#include "pawn.h"
#include "piece.h"
#include "protocol.h"
#include "random.h"
#include "square.h"
#include "trans.h"
#include "util.h"
#include "value.h"
#include "vector.h"

// functions
#if defined _WIN32
void LoadEgbbLibrary(char* main_path);
#endif

// main()

int main(int argc, char * argv[]) {

    // init

    util_init();
    my_random_init(); // for opening book

    printf("Gambit Fruit based on Fruit 2.1 and Toga by Ryan Benitez, Thomas Gaksch and Fabien Letouzey\n");
    printf( "LC change to add NPS control. Michele Tumbarello specifications\n" );

    // early initialisation (the rest is done after UCI options are parsed in protocol.cpp)

#if defined _WIN32

    {
        char* egbb_path = "egbb\\";
        LoadEgbbLibrary(egbb_path);
    }
#endif

    option_init();

    square_init();
    piece_init();
    pawn_init_bit();
    value_init();
    vector_init();
    attack_init();
    move_do_init();

    random_init();
    hash_init();

    trans_init(Trans);
    book_init();

    // loop

    loop();

    return EXIT_SUCCESS;
}

/*
Bitbases
*/

#if defined _WIN32

PPROBE_EGBB probe_egbb;
int egbb_is_loaded;
typedef void (*PLOAD_EGBB) (char* path);

void LoadEgbbLibrary(char* main_path) {
    HMODULE hmod;
    PLOAD_EGBB load_egbb;
    char path[256];

    strcpy(path,main_path);
    strcat(path,"egbbdll.dll");
    if(hmod = LoadLibrary(path)) {
        load_egbb = (PLOAD_EGBB) GetProcAddress(hmod,"load_egbb");
        probe_egbb = (PPROBE_EGBB) GetProcAddress(hmod,"probe_egbb");
        load_egbb(main_path);
        egbb_is_loaded = 1;
        printf("Bitbase loaded\n");
    } else {
        egbb_is_loaded = 0;
        printf("Bitbase not loaded\n");
    }
}

#endif
/*
EndBitbases
*/


// end of main.cpp

