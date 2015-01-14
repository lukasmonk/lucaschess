#ifndef WINGLET_DEFINES_H
#define WINGLET_DEFINES_H
 
#define WINGLET_PROG_VERSION "Winglet 0.0"
//#define WINGLET_DEBUG_PERFT
//#define WINGLET_DEBUG_MOVES
//#define WINGLET_VERBOSE_EVAL
//#define WINGLET_DEBUG_EVAL
//#define WINGLET_VERBOSE_SEE
#define WINGLET_DEBUG_WINBOARD

#define MAX_CMD_BUFF     256   // Console command input buffer
#define MAX_MOV_BUFF    4096   // Max number of moves that we can store (all plies)
#define MAX_PLY           64   // Max search depth
#define MAX_GAME_LINE   1024   // Max number of moves in the (game + search) line that we can store

typedef unsigned long long U64;
typedef unsigned long long BitMap;
typedef short SHORTINT;
typedef unsigned short USHORTINT;
typedef int BOOLTYPE;
 
#endif