#ifndef __COMMON__
#define __COMMON__

//#define DEBUG_TB
#if defined DEBUG_TB
   #define ASSERT(a) {if (!(a)) {printf("line %d, assertion \"" #a "\" failed\n",__LINE__);}} 
#else
   #define ASSERT(a)
#endif

#if defined (_WIN32) || defined(_WIN64)
   #define WINDOWS
#endif

#if defined (WINDOWS)
   #include <windows.h>
   #include <io.h>
   #include <conio.h>
   #undef CDECL
//   #define CDECL __cdecl
   #define CDECL
#else
   #include <unistd.h>
   #define CDECL
#endif

#ifdef _MSC_VER
    #define BMP64 __int64
    #define FORCEINLINE __forceinline
#else
    #define BMP64 long long int
    #define FORCEINLINE __inline
#endif

//threads
#if defined (WINDOWS)
      #include <process.h>
      #define pthread_t unsigned long int
      #define t_create(f,p,t) _beginthread(f,0,(void*)p)
      #define t_sleep(x)    Sleep(x)
#else
      #include <pthread.h>
      #define t_create(f,p,t) pthread_create(&t,0,(void*(*)(void*))&f,(void*)p)
      #define t_sleep(x)    usleep((x) * 1000)
#endif

//lock
#if defined (WINDOWS)
      #define LOCK volatile int
      #define l_create(x)   ((x) = 0)
      #define l_lock(x)     while(InterlockedExchange((LPLONG)&(x),1) != 0)
      #define l_unlock(x)   ((x) = 0)
#else
      #define LOCK pthread_mutex_t
      #define l_create(x)   pthread_mutex_init(&(x),0)
      #define l_lock(x)     pthread_mutex_lock(&(x))
      #define l_unlock(x)   pthread_mutex_unlock(&(x))
#endif

#define BMP32 long int
#define BMP16 short int
#define BMP8  char

typedef unsigned  BMP64  UBMP64;
typedef unsigned  BMP32  UBMP32;
typedef unsigned  BMP16  UBMP16;
typedef unsigned  BMP8   UBMP8; 

/*types*/
enum {white,black,neutral};
enum {king = 1,queen,rook,bishop,knight,pawn};
enum {empty,wking,wqueen,wrook,wbishop,wknight,wpawn,
bking,bqueen,brook,bbishop,bknight,bpawn,elephant};
enum {RANK1,RANK2,RANK3,RANK4,RANK5,RANK6,RANK7,RANK8};
enum {FILEA,FILEB,FILEC,FILED,FILEE,FILEF,FILEG,FILEH};
enum {A1 = 0,B1,C1,D1,E1,F1,G1,H1,
A2 = 16,B2,C2,D2,E2,F2,G2,H2,
A3 = 32,B3,C3,D3,E3,F3,G3,H3,
A4 = 48,B4,C4,D4,E4,F4,G4,H4,
A5 = 64,B5,C5,D5,E5,F5,G5,H5,
A6 = 80,B6,C6,D6,E6,F6,G6,H6,
A7 = 96,B7,C7,D7,E7,F7,G7,H7,
A8 = 112,B8,C8,D8,E8,F8,G8,H8};

#define RR    0x01
#define LL   -0x01
#define RU    0x11
#define LD   -0x11
#define UU    0x10
#define DD   -0x10
#define LU    0x0f
#define RD   -0x0f

#define RRU   0x12
#define LLD  -0x12
#define LLU   0x0e
#define RRD  -0x0e
#define RUU   0x21
#define LDD  -0x21
#define LUU   0x1f
#define RDD  -0x1f

#define UUU   0x20
#define DDD  -0x20
#define RRR   0x02
#define LLL  -0x02


#define KM       1
#define QM       2
#define RM       4
#define BM       8
#define NM      16
#define WPM     32
#define BPM     64
#define QRBM    14
#define KNM     17    

#define MAX_STR            256
#define MAX_MOVES          256
#define MAX_PLY             70

#undef max
#undef min
#define max(a, b)        (((a) > (b)) ? (a) : (b))
#define min(a, b)        (((a) < (b)) ? (a) : (b))

/*square*/
#define file(x)          ((x) &  7)
#define rank(x)          ((x) >> 4)
#define file64(x)        ((x) &  7)
#define rank64(x)        ((x) >> 3)
#define SQ(x,y)          (((x) << 4) | (y))
#define SQ64(x,y)        (((x) << 3) | (y))
#define SQ8864(x)        SQ64(rank(x),file(x))
#define SQ6488(x)        SQ(rank64(x),file(x))
#define SQ6424(x)        (file64(x) + ((rank64(x) - 1) * 4))
#define SQ6448(x)        (file64(x) + ((rank64(x) - 1) * 8))
#define MIRRORF(sq)      ((sq) ^ 0x07)
#define MIRRORR(sq)      ((sq) ^ 0x70)
#define MIRRORD(sq)      SQ(file(sq),rank(sq))
#define MIRRORF64(sq)    ((sq) ^ 0x07)
#define MIRRORR64(sq)    ((sq) ^ 0x38)
#define MIRRORD64(sq)    SQ64(file64(sq),rank64(sq))

/*distance*/
#define f_distance(x,y)  abs(file(x)-file(y))
#define r_distance(x,y)  abs(rank(x)-rank(y))
#define distance(x,y)    max(f_distance(x,y),r_distance(x,y))
#define is_light(x)      ((file(x)+rank(x)) & 1)
#define is_light64(x)    ((file64(x)+rank64(x)) & 1)

#define COLOR(x)         (col_tab[x])
#define PIECE(x)         (pic_tab[x]) 
#define DECOMB(c,x)      ((x) - ((c) ? 6 : 0)) 
#define COMBINE(c,x)     ((x) + ((c) ? 6 : 0)) 
#define invert(x)        (!(x))

/*move*/
#define FROM_FLAG        0x000000ff
#define TO_FLAG          0x0000ff00
#define PIECE_FLAG       0x000f0000
#define CAPTURE_FLAG     0x00f00000
#define PROMOTION_FLAG   0x0f000000
#define CAP_PROM         0x0ff00000
#define FROM_TO_PROM     0x0f00ffff
#define EP_FLAG          0x10000000
#define CASTLE_FLAG      0x20000000
#define m_from(x)        ((x) & FROM_FLAG)
#define m_to(x)          (((x) & TO_FLAG) >> 8)
#define m_piece(x)       (((x) & PIECE_FLAG) >> 16)
#define m_capture(x)     (((x) & CAPTURE_FLAG) >> 20)
#define m_promote(x)     (((x) & PROMOTION_FLAG) >> 24)
#define is_cap_prom(x)   ((x) & CAP_PROM)
#define is_ep(x)         ((x) & EP_FLAG)
#define is_castle(x)     ((x) & CASTLE_FLAG)

#define WSC_FLAG       1
#define WLC_FLAG       2
#define BSC_FLAG       4
#define BLC_FLAG       8
#define WSLC_FLAG      3
#define BSLC_FLAG     12 
#define WBC_FLAG      15

#define INVALID       -1
#define BLOCK_SIZE    8092
/*
Definitions
*/
typedef struct SQATTACK {
	int   step;
	int   pieces;
}*PSQATTACK;

typedef struct LIST{
	int   sq;
	LIST* prev;
	LIST* next;
}*PLIST;

typedef struct STACK{
	int move_st[MAX_MOVES];
	int count;
}*PSTACK;

struct INFO {
	UBMP8  block[BLOCK_SIZE];
	UBMP32 start_index;
};

typedef struct SEARCHER{
	int player;
	int opponent;
	int castle;
	int epsquare;
	int fifty;
	int temp_board[224];
	int* const board;
	PLIST list[128];
	PLIST plist[15];
	int ply;
	PSTACK pstack;
	STACK stack[MAX_PLY];

	INFO info;
	UBMP8 block[2 * BLOCK_SIZE];
	int used;

	SEARCHER();
	int   blocked(int,int);
	int   attacks(int,int);
	void  pcAdd(int,int);
	void  pcRemove(int,int);
	void  pcSwap(int,int);
	void  PUSH_MOVE(int);
	void  POP_MOVE(int);
	void  do_move(const int&);
    void  undo_move(const int&);
	void  gen_caps();
	void  gen_noncaps();
	void init_data();
	void  set_pos(int side,int w_ksq,int b_ksq,
		int piece1 = empty, int square1 = INVALID, 
		int piece2 = empty, int square2 = INVALID,
		int piece3 = empty, int square3 = INVALID);
	int get_score(int alpha,int beta,
		int side,int w_king,int b_king,
		int piece1 = empty,int square1 = INVALID,
		int piece2 = empty,int square2 = INVALID,
		int piece3 = empty,int square3 = INVALID);
} *PSEARCHER;

/*
inline piece list functions
*/
FORCEINLINE void SEARCHER::pcAdd(int pic,int sq) {
	PLIST* pHead;
	PLIST pPc;
	pHead = &plist[pic];
	pPc = list[sq];
	if(!(*pHead)) {
		(*pHead) = pPc;
		(*pHead)->next = 0;
		(*pHead)->prev = 0;
	} else {
		pPc->next = (*pHead)->next;
		if((*pHead)->next) (*pHead)->next->prev = pPc;
		(*pHead)->next = pPc;
		pPc->prev = (*pHead);
    }
};
FORCEINLINE void SEARCHER::pcRemove(int pic,int sq) {
	PLIST* pHead;
	PLIST pPc;
	pHead = &plist[pic];
	pPc = list[sq];
	if(pPc->next) pPc->next->prev = pPc->prev;
	if(pPc->prev) pPc->prev->next = pPc->next;
    if((*pHead) == pPc) (*pHead) = (*pHead)->next;
};
FORCEINLINE void SEARCHER::pcSwap(int from,int to) {
	PLIST pPc;
	PLIST& pTo = list[to];
    PLIST& pFrom = list[from];
    pPc = pTo;
    pTo = pFrom;
    pFrom = pPc;
    pTo->sq = to;
    pFrom->sq = from;
}
FORCEINLINE void SEARCHER::PUSH_MOVE(int move) {
	do_move(move);
    ply++;
	pstack++;
}
FORCEINLINE void SEARCHER::POP_MOVE(int move) {
	ply--;
	pstack--;
    undo_move(move);
}

extern const int col_tab[15];
extern const int pic_tab[15];
extern const int pawn_dir[2];
extern SQATTACK  temp_sqatt[0x101];
extern PSQATTACK const sqatt;

extern void init_sqatt();

#endif
