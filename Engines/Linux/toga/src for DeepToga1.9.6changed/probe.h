
// probe.h

// includes
#include "search.h" // WHM_egbb

// constants

// macros

// types

typedef int (*PPROBE_EGBB) (int player, int w_king, int b_king,
                            int piece1, int square1,
                            int piece2, int square2,
                            int piece3, int square3);

typedef void (*PLOAD_EGBB) (char* path,int cache_size,int load_options);
//static PPROBE_EGBB probe_egbb;
  static PPROBE_EGBB probe_egbb[MaxThreads]; // WHM_egbb


// "constants"

#define _NOTFOUND 99999
#define WIN_SCORE 3000
#define WIN_PLY   40 

// variables

extern bool egbb_is_loaded;
extern char * egbb_path;
extern uint32 egbb_cache_size; 

// functions

extern int LoadEgbbLibrary(char* main_path);
extern int probe_bitbases(board_t * board, int& score, int ThreadId); // WHM_egbb
// end of probe.h

