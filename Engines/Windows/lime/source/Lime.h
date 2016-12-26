#ifndef LIME_H
#define LIME_H


/*
piece defintions - three result of three attempts - one,using just the numbers
1 - 6, and identifying by colour from the board, the other was eloborate,
using bits (up to 1024, iirc) and indexing them to a smaller array. In the end
I decided to make a piece structure on the board using the individual type
and the colour (see below)
*/
#define wP 1
#define bP 2
#define wN 3
#define bN 4
#define wB 5
#define bB 6
#define wR 7
#define bR 8
#define wQ 9
#define bQ 10
#define wK 11
#define bK 12
#define ety 13 //empty

/*
castle flags are set as '15' at the start - or 1111 in binary (KQkq)
*/
#define     WKC   8
#define     BKC   4
#define     WQC   2
#define     BQC   1

/*
the hash flags are set during the search when a positions is stored
*/
#define NOFLAG 0
#define LOWER  1
#define EXACT  3
#define UPPER  2

//squares
#define		A8		110
#define		A7		98
#define		A6		86
#define		A5		74
#define		A4		62
#define		A3		50
#define		A2		38
#define		A1		26
#define		B8		111
#define		B7		99
#define		B6		87
#define		B5		75
#define		B4		63
#define		B3		51
#define		B2		39
#define		B1		27
#define		C8		112
#define		C7		100
#define		C6		88
#define		C5		76
#define		C4		64
#define		C3		52
#define		C2		40
#define		C1		28
#define		D8		113
#define		D7		101
#define		D6		89
#define		D5		77
#define		D4		65
#define		D3		53
#define		D2		41
#define		D1		29
#define		E8		114
#define		E7		102
#define		E6		90
#define		E5		78
#define		E4		66
#define		E3		54
#define		E2		42
#define		E1		30
#define		F8		115
#define		F7		103
#define		F6		91
#define		F5		79
#define		F4		67
#define		F3		55
#define		F2		43
#define		F1		31
#define		G8		116
#define		G7		104
#define		G6		92
#define		G5		80
#define		G4		68
#define		G3		56
#define		G2		44
#define		G1		32
#define		H8		117
#define		H7		105
#define		H6		93
#define		H5		81
#define		H4		69
#define		H3		57
#define		H2		45
#define		H1		33


#define     noenpas 0
#define     nopiece 0
#define     deadsquare 0
#define     edge 0

//the side
#define white 1
#define black 0

/*
the piece colour defs (pco) - different from the side so not to be confused with
0, assigned for 'npco' (no piece colour).
*/
#define wpco 1
#define bpco 2
#define npco 13

/*
the piece values
*/
#define vP 90
#define vN 325
#define vB 325
#define vR 500
#define vQ 900
#define vK 10000

#define startfen    "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"

/*
the flags set describing a move - the flag resides in bits 17-24 of the
move integer
*/
#define mCA                  0x20000
#define mPST                 0x40000
#define mPEP                 0x180000//capture and ep flag (8 & 16)
#define mCAP                 0x100000
#define mPQ                  0x600000//64+32
#define mPR                  0xa00000//128+32
#define mPB                  0x1200000//256+32
#define mPN                  0x2200000//512+32
#define mNORM                0x10000
#define mProm                0x200000

/*
macros for retriveing squares or flagsfrom a move integer
*/
#define TO(x)  ((x)&0xff)
#define FROM(x) (((x)&0xff00)>>8)
#define FLAG(x) ((x)&0xfff0000)
#define MOVEBITS 0xffff

/*
individual promotion and en passant move flags
*/
#define oPQ 0x400000
#define oPR 0x800000
#define oPB 0x1000000
#define oPN 0x2000000
#define oPEP 0x80000

#define TRUE 1
#define FALSE 0

/*
int the search, one 'ply' or move deeper is defined as 64, to allow for
fractional extensions
*/
#define PLY 64


/*
the move structure - m is the move, the first 8 bits are the to sqaure,
th second 8 bits the from sqaure, and the remaining bits used for the flag.
also kept is the score
*/
struct s_Move {
        int m;
        int score;
};

/*
the piece structure - colour and type
*/
struct s_Pce {
    int col;
    int typ;
};


/*
the history structure, used to back up the moves played during a game
*/
struct s_Hist {
        int data;
        int en_pas;
        unsigned __int64 hashkey;
        s_Pce captured;
        int castleflags;
        int plistep;//ep piece number captured
        int plist;//piece number captured
        int fifty;//fifty move count
};

struct s_EvalOptions {
    //positional factor weights
   int pawnstructure;
   int passedpawn;
   int kingsafety;
};

/*
information for a position.
*/
struct s_Position {

/*
the piece lists - this implementation is inefficient. I have another
version indexed by colour as well, but it still has a bug in the makemove(),
so until I find it this slower version is used..
*/
int pcenumtosq[17];
int sqtopcenum[144];

/*
the number of pieces, number of major pieces, castle flags, fifty move number,
side to move, the enpassant square, and the current ply
*/
int pcenum;
int majors;
int castleflags;
int fifty;
int side;
int en_pas;
int ply;

/*
the material score for each side
*/
int material[2];

/*
current hashkey
*/
unsigned __int64 hashkey;

/*
movelist, explained in movegen
*/
s_Move list[9600];//max list of moves 9600
int listc[512];//max ply 512

/*
the board - a [144] array (64 squares surrounded by two rows on each side).

I have also written a version of this engine with 0x88 ( [128] ) but it
was much slower. The very first Lime's were [8][8], confusing and slow, and
two boards of [64] for colour and piece type.

A bitboard version of Lime also exists - passes all perfts, and uses Pradu's
magic keys for the sliding piece moves (www.prism.gatech.edu/~gtg365v/Buzz).
If you're reading this and want the code for the 0x88 or bitboard version,
please mail me.
*/
s_Pce board[144];

/*
the king sqaure for each side
*/
int k[2];

};

/*
an element to be stored in the hash table - null is the permission to null
move
*/
struct s_Hashelem {
        unsigned __int64  hashkey;
        short depth;
        short score;
        short flag;
        short null;
        int move;
};

/*
data used in eval - explained in eval.cpp
*/
struct sEvalData {
int wRc;
int bRc;
int wQf;
int bQf;
int wNc;
int bNc;
int wBc;
int bBc;
int wQc;
int bQc;
int wBsq;
int bBsq;
int bmajors;
int wmajors;
int wpawns;
int bpawns;
int pawn_set[2][10];
int pawns[2][10];
int pawnbits[2];
int score[2][2];//colour and phase
int defects[2];
};

struct s_Atab {
    int atttab[2][144];
};

/*
parameters set for the search
*/
struct s_SearchParam {
        int depth;
        double wtime;
        double btime;
        double winc;
        double binc;
        double xtime;
        double xotime;
        int inf;
        double movestogo[2];//one for each side
        double timepermove;
        double starttime;
        double stoptime;
        int pon;
        int cpon;
        int ponderhit;
        int xbmode;
        int ucimode;
        int post;
        int usebook;
        int ics;
        int ponfrom;//these store the ponder search move from and to squares
        int ponto;
        double pontime;//ponder time - if we ponder the expected move for this time,
                    //then move instantly
};


struct s_bookinfo
{
     long whitelsize;
     long whiteentries;
};

struct s_binentry
{
    unsigned __int64 k;
    char m[5];
    int freq;
};

#endif

