
// probe.cpp

// includes

#include <windows.h>

#include "util.h"
#include "board.h"
#include "probe.h"
//#include "protocol.h"
//#include "search.h"

// "constants"

enum {_EMPTY,_WKING,_WQUEEN,_WROOK,_WBISHOP,_WKNIGHT,_WPAWN,
             _BKING,_BQUEEN,_BROOK,_BBISHOP,_BKNIGHT,_BPAWN};

enum {LOAD_NONE,LOAD_4MEN,SMART_LOAD,LOAD_5MEN};

static int egbb_load_type = LOAD_4MEN;

// macros

/*
Load the dll and get the address of the load and probe functions.
*/

#ifdef _MSC_VER
   #define EGBB_NAME "egbbdll.dll"
#else
   #define EGBB_NAME "egbbso.so"
   #define HMODULE void*
   #define LoadLibrary(x) dlopen(x,RTLD_LAZY)
   #define GetProcAddress dlsym
#endif

// extern variables

bool egbb_is_loaded;
char * egbb_path;
uint32 egbb_cache_size; 


// functions

int LoadEgbbLibrary(char* main_path) {
    static HMODULE hmod;
    PLOAD_EGBB load_egbb[MaxThreads];
    char path[256];
    int ThreadId;// WHM_egbb
    
    strcpy(path,main_path);
    strcat(path,EGBB_NAME);
    if (hmod)
        FreeLibrary(hmod);
    if(hmod = LoadLibrary(path)) {
          for (ThreadId = 0; ThreadId < NumberThreadsInternal; ThreadId++) {             // WHM_egbb
             load_egbb[ThreadId] = (PLOAD_EGBB)  GetProcAddress(hmod,"load_egbb_5men");  // WHM_egbb
            probe_egbb[ThreadId] = (PPROBE_EGBB) GetProcAddress(hmod,"probe_egbb_5men"); // WHM_egbb
             load_egbb[ThreadId](main_path,egbb_cache_size,egbb_load_type);              // WHM_egbb
          }                                                                              // WHM_egbb
        return true;
    } else {
        //printf("EgbbProbe not Loaded!\n");
        return false;
    }
}

/*
Probe:
probe bitbase.
*/

int probe_bitbases(board_t * board, int& score, int ThreadId) { // WHM_egbb

    int piece[5];
   int square[5];
    int from;
    int count;
    sq_t * ptr;

    count = 0;
    piece[0] = _EMPTY;
    piece[1] = _EMPTY;
    piece[2] = _EMPTY;
    square[0] = _EMPTY;
    square[1] = _EMPTY;
    square[2] = _EMPTY;

       
    for (ptr = &board->piece[White][1]; (from=*ptr) != SquareNone; ptr++) { 
        square[count] = SquareTo64[from];
        piece[count++] = -((PIECE_TO_12(board->square[from]) >> 1) - 6);
       }
   for (ptr = &board->pawn[White][0]; (from=*ptr) != SquareNone; ptr++) { 
        square[count] = SquareTo64[from];
        piece[count++] = _WPAWN;
   }
   for (ptr = &board->piece[Black][1]; (from=*ptr) != SquareNone; ptr++) { 
        square[count] = SquareTo64[from];
        piece[count++] = -((PIECE_TO_12(board->square[from]) >> 1) - 12);
   }
   for (ptr = &board->pawn[Black][0]; (from=*ptr) != SquareNone; ptr++) { 
        square[count] = SquareTo64[from];
        piece[count++] = _BPAWN;
   }
    

    //EnterCriticalSection(&CriticalSection); 
    
    score = probe_egbb[ThreadId](board->turn,SquareTo64[board->piece[White][0]],SquareTo64[board->piece[Black][0]],
        piece[0],square[0],piece[1],square[1],piece[2],square[2]);

    //LeaveCriticalSection(&CriticalSection);
    
    if(score != _NOTFOUND) {
        return true;
    }

    return false;
}




// end of probe.cpp

