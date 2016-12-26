#ifndef __EGBBPROBE__
#define __EGBBPROBE__

enum {_WHITE,_BLACK};
enum {_EMPTY,_WKING,_WQUEEN,_WROOK,_WBISHOP,_WKNIGHT,_WPAWN,
             _BKING,_BQUEEN,_BROOK,_BBISHOP,_BKNIGHT,_BPAWN};
enum {LOAD_NONE,LOAD_4MEN,SMART_LOAD,LOAD_5MEN};

#define _NOTFOUND 99999

#if defined (_WIN32) || defined(_WIN64)
//     #define DLLExport extern "C" __declspec(dllexport)
     #define DLLExport 
#else
     #define DLLExport extern "C"
#endif

#ifdef _MSC_VER
#   define CDECL
//#   define CDECL __cdecl
#else
#   define CDECL
#endif

//old
DLLExport int  CDECL probe_egbb(int player, int w_king, int b_king,
			   int piece1 = _EMPTY, int square1 = 0,
			   int piece2 = _EMPTY, int square2 = 0);
DLLExport void CDECL load_egbb(char* path);
//new
DLLExport int  CDECL probe_egbb_5men(int player, int w_king, int b_king,
			   int piece1 = _EMPTY, int square1 = 0,
			   int piece2 = _EMPTY, int square2 = 0,
			   int piece3 = _EMPTY, int square3 = 0);
DLLExport void CDECL load_egbb_5men(char* path,int cache_size = 4194304,int load_options = LOAD_4MEN);
DLLExport void CDECL load_egbb_into_ram(int side,int piece1,int piece2 = _EMPTY,int piece3 = _EMPTY);
DLLExport void CDECL unload_egbb_from_ram(int side,int piece1,int piece2 = _EMPTY,int piece3 = _EMPTY);

#endif
