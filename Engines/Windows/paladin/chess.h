// get rid of silly security related warnings
#define _CRT_SECURE_NO_WARNINGS
#include "switches.h"
#include <stdio.h>
#include <string.h>
#include <thread>
#include <chrono>
#include "timer.h"

#include "bb_consts.h"

#if _DEBUG
#include <assert.h>
#else
#define assert(x)
#endif

typedef unsigned char      uint8;
typedef unsigned short     uint16;
typedef unsigned int       uint32;
typedef unsigned long long uint64;

typedef char               int8;
typedef short              int16;
typedef int                int32;
typedef long long          int64;
typedef long long          int64;

#define INF   32767

#define HI(x) ((uint32)((x)>>32))
#define LO(x) ((uint32)(x))

#define CT_ASSERT(expr) \
int __static_assert(int static_assert_failed[(expr)?1:-1])

#define BIT(i)   (1ULL << (i))

// Terminology:
//
// file - column [A - H]
// rank - row    [1 - 8]


// piece constants 
#define PAWN    1
#define KNIGHT  2
#define BISHOP  3
#define ROOK    4
#define QUEEN   5
#define KING    6

// chance (side) constants
#define WHITE   0
#define BLACK   1

#define SLIDING_PIECE_INDEX(piece) ((piece) - BISHOP)
// BISHOP 0
// ROOK   1
// QUEEN  3

// new encoding for fast table based move generation
// bits 01234 : color
// bits   567 : piece

// From http://chessprogramming.wikispaces.com/Table-driven+Move+Generation
// another encoding (a bit faster and simpler than above)
// bits  01 color	1 - white, 2 - black
// bits 234 piece
#define COLOR_PIECE(color, piece)      		((1+color) | (piece << 2))
#define COLOR(colorpiece)              		(((colorpiece & 2) >> 1))
#define PIECE(colorpiece)              		((colorpiece) >> 2)
#define EMPTY_SQUARE						0x0
#define ISEMPTY(colorpiece)					(!(colorpiece))
#define IS_OF_COLOR(colorpiece, color)		((colorpiece) & (1 << (color)))
#define IS_ENEMY_COLOR(colorpiece, color)	(IS_OF_COLOR(colorpiece, 1 - color))



#define INDEX088(rank, file)        ((rank) << 4 | (file))
#define RANK(index088)              (index088 >> 4)
#define FILE(index088)              (index088 & 0xF)

#define ISVALIDPOS(index088)        (((index088) & 0x88) == 0)

// special move flags
#define CASTLE_QUEEN_SIDE  1
#define CASTLE_KING_SIDE   2
#define EN_PASSENT         3
#define PROMOTION_QUEEN    4
#define PROMOTION_ROOK     5
#define PROMOTION_BISHOP   6
#define PROMOTION_KNIGHT   7

// castle flags in board position (1 and 2)
#define CASTLE_FLAG_KING_SIDE   1
#define CASTLE_FLAG_QUEEN_SIDE  2


enum eSquare
{
    A1, B1, C1, D1, E1, F1, G1, H1,
    A2, B2, C2, D2, E2, F2, G2, H2,
    A3, B3, C3, D3, E3, F3, G3, H3,
    A4, B4, C4, D4, E4, F4, G4, H4,
    A5, B5, C5, D5, E5, F5, G5, H5,
    A6, B6, C6, D6, E6, F6, G6, H6,
    A7, B7, C7, D7, E7, F7, G7, H7,
    A8, B8, C8, D8, E8, F8, G8, H8,
};

// size 128 bytes
// let's hope this fits in register file
// Note that this is only used as an intermediate representation for reading boards from fen
// TODO: get rid of this
struct BoardPosition088
{
    union
    {
        uint8 board[128];    // the 0x88 board

        struct
        {
            uint8 row0[8];  uint8 padding0[3];

            uint8 chance;           // whose move it is
            uint8 whiteCastle;      // whether white can castle
            uint8 blackCastle;      // whether black can castle
            uint8 enPassent;        // col + 1 (where col is the file on which enpassent is possible)
            uint8 halfMoveCounter;  // to detect draw using 50 move rule

            uint8 row1[8]; uint8 padding1[8];
            uint8 row2[8]; uint8 padding2[8];
            uint8 row3[8]; uint8 padding3[8];
            uint8 row4[8]; uint8 padding4[8];
            uint8 row5[8]; uint8 padding5[8];
            uint8 row6[8]; uint8 padding6[8];
            uint8 row7[8]; uint8 padding7[8];

            // 60 unused bytes (padding) available for storing other structures if needed
        };
    };
};

CT_ASSERT(sizeof(BoardPosition088) == 128);

/*
The board representation     Free space for other structures

A	B	C	D	E	F	G	H
8	70	71	72	73	74	75	76	77	78	79	7A	7B	7C	7D	7E	7F
7	60	61	62	63	64	65	66	67	68	69	6A	6B	6C	6D	6E	6F
6	50	51	52	53	54	55	56	57	58	59	5A	5B	5C	5D	5E	5F
5	40	41	42	43	44	45	46	47	48	49	4A	4B	4C	4D	4E	4F
4	30	31	32	33	34	35	36	37	38	39	3A	3B	3C	3D	3E	3F
3	20	21	22	23	24	25	26	27	28	29	2A	2B	2C	2D	2E	2F
2	10	11	12	13	14	15	16	17	18	19	1A	1B	1C	1D	1E	1F
1	00	01	02	03	04	05	06	07	08	09	0A	0B	0C	0D	0E	0F

*/


// size 4 bytes
struct Move
{
    uint8  src;             // source position of the piece
    uint8  dst;             // destination position
    uint8  capturedPiece;   // the piece captured (if any)
    uint8  flags;           // flags to indicate special moves, e.g castling, en passent, promotion, etc
};
CT_ASSERT(sizeof(Move) == 4);


// position of the board in bit-board representation using 4 bitboards (not used)
struct QuadBitBoardPosition
{
    // 32 bytes of dense bitboard data
    uint64   black;   // 1 - black, 0 - white/empty
    uint64   PBQ;     // pawns, bishops and queens
    uint64   NB;      // knights and bishops
    uint64   RQK;     // rooks, queens and kings

    // 8 bytes of state / free space
    uint8    chance;            // whose move it is
    uint8    whiteCastle;       // whether white can castle
    uint8    blackCastle;       // whether black can castle
    uint8    enPassent;         // col + 1 (where col is the file on which enpassent is possible)
    uint8    halfMoveCounter;   // to detect 50 move draw rule
    uint8    padding[3];        // free space to store additional info if needed
};
CT_ASSERT(sizeof(QuadBitBoardPosition) == 40);

// another bit-board based board representation using 6 bitboards (this is currently used by default)
struct HexaBitBoardPosition
{
    // 48 bytes of bitboard data with interleaved game state data in pawn bitboards
    uint64   whitePieces;
    union
    {
        uint64   pawns;
        struct
        {
            uint8 whiteCastle : 2;
            uint8 blackCastle : 2;
            uint8 enPassent   : 4;       // file + 1 (file is the file containing the enpassent-target pawn)
            uint8 padding[6];
            uint8 halfMoveCounter : 7;   // to detect 50 move draw rule
            uint8 chance : 1;
        };
    };
    uint64   knights;
    uint64   bishopQueens;
    uint64   rookQueens;
    uint64   kings;

    // (stored seperately, or calculated from scratch when INCREMENTAL_ZOBRIST_UPDATE is 0)
    // uint64   zobristHash;
};
CT_ASSERT(sizeof(HexaBitBoardPosition) == 48);
//CT_ASSERT(sizeof(HexaBitBoardPosition) == 56);    // if hash is included in the board position structure above

// a more compact move structure (16 bit)
// from http://chessprogramming.wikispaces.com/Encoding+Moves
class CMove
{
public:
    CMove(uint8 from, uint8 to, uint8 flags)
    {
        m_Move = ((flags & 0xF) << 12) | ((to & 0x3F) << 6) | (from & 0x3F);
    }

    CMove()
    {
        m_Move = 0;
    }

    CMove(uint16 val)
    {
        m_Move = val;
    }
    unsigned int getTo()    const { return (m_Move >> 6) & 0x3F; }
    unsigned int getFrom()  const { return (m_Move)& 0x3F; }
    unsigned int getFlags() const { return (m_Move >> 12) & 0x0F; }
    bool isValid()          const { return m_Move != 0; }
    uint16 getVal()         const { return m_Move; }

    bool operator == (CMove a) const { return (m_Move == a.m_Move); }
    bool operator != (CMove a) const { return (m_Move != a.m_Move); }

    void operator = (CMove a)
    {
        m_Move = a.m_Move;
    }
protected:

    uint16 m_Move;

};

CT_ASSERT(sizeof(CMove) == 2);

enum eCompactMoveFlag
{
    CM_FLAG_QUIET_MOVE = 0,

    CM_FLAG_DOUBLE_PAWN_PUSH = 1,

    CM_FLAG_KING_CASTLE = 2,
    CM_FLAG_QUEEN_CASTLE = 3,

    CM_FLAG_CAPTURE = 4,
    CM_FLAG_EP_CAPTURE = 5,


    CM_FLAG_PROMOTION = 8,

    CM_FLAG_KNIGHT_PROMOTION = 8,
    CM_FLAG_BISHOP_PROMOTION = 9,
    CM_FLAG_ROOK_PROMOTION = 10,
    CM_FLAG_QUEEN_PROMOTION = 11,

    CM_FLAG_KNIGHT_PROMO_CAP = 12,
    CM_FLAG_BISHOP_PROMO_CAP = 13,
    CM_FLAG_ROOK_PROMO_CAP = 14,
    CM_FLAG_QUEEN_PROMO_CAP = 15,
};

// might want to use these flags:
/*
code	promotion	capture	special 1	special 0	kind of move
0	    0        	0    	0        	0        	quiet moves
1	    0        	0    	0        	1        	double pawn push
2	    0        	0    	1        	0        	king castle
3	    0        	0    	1        	1        	queen castle
4	    0        	1    	0        	0        	captures
5	    0        	1    	0        	1        	ep-capture
8	    1        	0    	0        	0        	knight-promotion
9	    1        	0    	0        	1        	bishop-promotion
10	    1        	0    	1        	0        	rook-promotion
11	    1        	0    	1        	1        	queen-promotion
12	    1        	1    	0        	0        	knight-promo capture
13	    1        	1    	0        	1        	bishop-promo capture
14	    1        	1    	1        	0        	rook-promo capture
15	    1        	1    	1        	1        	queen-promo capture
*/


// max no of moves possible for a given board position (this can be as large as 218 ?)
// e.g, test this FEN string "3Q4/1Q4Q1/4Q3/2Q4R/Q4Q2/3Q4/1Q4Rp/1K1BBNNk w - - 0 1"
#define MAX_MOVES 256
#define MAX_SEARCH_LENGTH 128
#define MAX_GAME_LENGTH 1024
#define MATE_SCORE_BASE 16384

// max no of moves possible by a single piece
// actually it's 27 for a queen when it's in the center of the board
#define MAX_SINGLE_PIECE_MOVES 32

// in theory - in practice should be much lesser than this
#define MAX_CAPTURE_SEQ_LENGTH 32

// random numbers for zobrist hashing
struct ZobristRandoms
{
    uint64 pieces[2][6][64];     // position of every piece on board
    uint64 castlingRights[2][2]; // king side and queen side castle for each side
    uint64 enPassentTarget[8];   // 8 possible files for en-passent target (if any)
    uint64 chance;               // chance (side to move)
    uint64 depth;                // search depth (used only by perft)
};


// indexes used to reference the zobristRandoms.pieces[] table above
#define ZOB_INDEX_PAWN     (PAWN - 1  )
#define ZOB_INDEX_KNIGHT   (KNIGHT - 1)
#define ZOB_INDEX_BISHOP   (BISHOP - 1)
#define ZOB_INDEX_ROOK     (ROOK - 1  )
#define ZOB_INDEX_QUEEN    (QUEEN - 1 )
#define ZOB_INDEX_KING     (KING - 1  )



/** Declarations for class/methods in Util.cpp **/

// utility functions for reading FEN String, EPD file, displaying board, etc

/*
Three kinds of board representations are used with varying degrees of readibility and efficiency

1. Human readable board (EPD file?)

e.g. for Starting Position:

rnbqkbnr
pppppppp
........
........
........
........
PPPPPPPP
RNBQKBNR

2. FEN string board e.g:
"rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"

3. 0x88 binary board described at the begining of this file

The first two fromats are only used for displaying/taking input and interacting with the UCI protocol
For all other purposes the 0x88 board is used

*/

class Utils {

private:

    // gets the numeric code of the piece represented by a character
    static uint8 getPieceCode(char piece);

    // Gets char representation of a piece code
    static char  getPieceChar(uint8 code);

public:

    // reads a board from text file
    static void readBoardFromFile(char filename[], char board[8][8]);
    static void readBoardFromFile(char filename[], BoardPosition088 *pos);


    // displays the board in human readable form
    static void dispBoard(char board[8][8]);
    static void dispBoard(BoardPosition088 *pos);
    static void dispBoard(HexaBitBoardPosition *pos);

    // displays a move in human readable form
    static void displayMove(Move move);
    static void displayMoveBB(Move move);
    static void displayCompactMove(CMove move);

    static void board088ToChar(char board[8][8], BoardPosition088 *pos);
    static void boardCharTo088(BoardPosition088 *pos, char board[8][8]);

    static void board088ToHexBB(HexaBitBoardPosition *posBB, BoardPosition088 *pos088);
    static void boardHexBBTo088(BoardPosition088 *pos088, HexaBitBoardPosition *posBB);

    // reads a FEN string and sets board and other Game Data accorodingly
    // returns the no. of full moves
    static int readFENString(char fen[], BoardPosition088 *pos);

    // clears the board (i.e, makes all squares blank)
    static void clearBoard(BoardPosition088 *pos);

    static int  readMove(const char *input, const HexaBitBoardPosition *pos, CMove* move);
};



// all classes contain mostly static functions and static member variables
// they act more like containers for grouping functions
class UciInterface
{
private:
    // thread used for launching search
    static std::thread *worker_thread;

    // entry point for worker thread
    static void WorkerThreadMain();

    static void Search_Go(char *params);
public:

    // process commands from standard input one by one
    static void ProcessCommands();

};

class Game
{
private:
    // timer to check how much time is remaining and elapsed
    static Timer timer;

    // the current board position
    static HexaBitBoardPosition pos;

    // time to search for (in ms)
    static int searchTime;

    // max time limit (in ms)
    // the search must be aborted if we exceed this
    static int searchTimeLimit;

    static int maxSearchDepth;

    // the best move found so far (TODO: protect this in a Critical Section)
    static CMove bestMove;

    // the principal variation for the current search
    static CMove pv[MAX_GAME_LENGTH];

    // length of available PV
    static int pvLen;

    // to detect repetitions (and avoid/cause draw based on it)
    // plyNo is relative to the position provided in "position" uci command
    static uint64 posHashes[MAX_GAME_LENGTH];
    static int    plyNo;

    // used for history heuristic
    // TODO: also consider storing these per piece type?
#if HISTORY_PER_PIECE == 1
    static uint32 historyScore[2][6][64][64];
    static uint32 butterflyScore[2][6][64][64];
#else
    static uint32 historyScore[2][64][64];
    static uint32 butterflyScore[2][64][64];
#endif

    // a ref count of ir-reversible moves (*not* incremented during search)
    // only incremented as the game progresses (when a move is actually made)
    // used for transposition table ageing
    static uint8  irreversibleMoveRefCount;

    static uint64 nodes;

#if GATHER_STATS == 1
    static uint32 totalSearched;
    static uint32 nonTTSearched;
    static uint32 nonCaptureSearched;
    static uint32 nonKillersSearched;
#endif

    // killer moves
    static CMove killers[MAX_GAME_LENGTH][MAX_KILLERS];

    // the code assumes that there are only two killer moves
    CT_ASSERT(MAX_KILLERS == 2);

    // sort captures based on SEE
    template<uint8 chance>
    static int16 SortCapturesSEE(HexaBitBoardPosition *pos, CMove* captures, int nMoves);

    // sort moves on history heuristic
    static void SortMovesHistory(HexaBitBoardPosition *pos, CMove *moves, int nMoves, uint8 chance);

    static void UpdateHistory(HexaBitBoardPosition *pos, CMove move, int depth, uint8 chance, bool betaCutoff);


    // perform alpha-beta search on the given position
    template<uint8 chance>
    static int16 alphabeta(HexaBitBoardPosition *pos, uint64 hash, int depth, int ply, int16 alpha, int16 beta, bool tryNullMove, CMove lastMove);

    template<uint8 chance>
    static int16 alphabetaRoot(HexaBitBoardPosition *pos, int depth, int ply);


    // perform q-search
    template<uint8 chance>
    static int16 q_search(HexaBitBoardPosition *pos, uint64 hash, int depth, int16 alpha, int16 beta, int curPly);

    static uint64 perft(HexaBitBoardPosition *pos, int depth);

    // for testing
    template<uint8 chance>
    static uint64 perft_test(HexaBitBoardPosition *pos, int depth);
public:
    // set hash for a previous board position (also update ply no)
    static void SetHashForPly(int ply, uint64 hash)                  { posHashes[ply] = hash; assert(ply >= plyNo); plyNo = ply; }

    static void     SetIrReversibleRefCount(int counter)             { irreversibleMoveRefCount = (uint8)counter; printf("\nrefcount: %d\n", counter);  }
    static uint8    GetIrReversibleRefCount()                        { return irreversibleMoveRefCount; }

    // initialize/reset state for a new game
    static void Reset();

    // set curent position
    static void SetPos(HexaBitBoardPosition *position)               { pos = *position; }
    static void GetPos(HexaBitBoardPosition *position)               { *position = pos; }

    // set time controls for the search
    static void SetTimeControls(int wtime, int btime, int movestogo, int winc, int binc, int searchTimeExact);

    static void SetMaxDepth(int depth)                               { maxSearchDepth = depth; }

    // start search (called from a different thread)
    // returns when we run out of time (or if terminiated from main thread)
    static void StartSearch();

    // compute the PV from transposition table
    static void GetPVFromTT(HexaBitBoardPosition *pos);

    // get the best move resulting from the last (or ongoing) search
    static CMove GetBestMove()                                      { return bestMove; }

    // this is set to true when the worker thread is active and searching the game tree
    static volatile bool searching;

    // util routine to verify move generation
    // computes perft of the current position till the given depth
    static uint64 Perft(int depth);
};

struct FancyMagicEntry
{
    union
    {
        struct {
            unsigned long long factor;  // the magic factor
            int position;               // position in the main lookup table (of 97264 entries)

            int offset;                 // position in the byte lookup table (only used when byte lookup is enabled)
        };
#ifdef __CUDA_ARCH__
        uint4 data;
#endif
    };
};

// expanded bit board
struct ExpandedBitBoard
{
    uint64 allPieces;
    uint64 myPieces;
    uint64 enemyPieces;

    uint64 knights;
    uint64 bishopQueens;
    uint64 rookQueens;
    uint64 kings;
    uint64 pawns;

    uint64 myPawns;
    uint64 myKing;
    uint64 myKnights;
    uint64 myBishopQueens;
    uint64 myRookQueens;

    uint64 enemyPawns;
    uint64 enemyKing;
    uint64 enemyKnights;
    uint64 enemyBishopQueens;
    uint64 enemyRookQueens;

    uint64 pinned;
    uint64 threatened;

    uint8  myKingIndex;
    uint8  enPassent;
    uint8  whiteCastle;
    uint8  blackCastle;
};

// another expanded bit board structure used by evaluation function
struct EvalBitBoard
{
    uint64 allPieces;
    uint64 whitePieces;
    uint64 blackPieces;

    uint64 whitePawns;
    uint64 whiteKing;
    uint64 whiteKnights;
    uint64 whiteBishops;
    uint64 whiteRooks;
    uint64 whiteQueens;

    uint64 blackPawns;
    uint64 blackKing;
    uint64 blackKnights;
    uint64 blackBishops;
    uint64 blackRooks;
    uint64 blackQueens;
};


#define SCORE_EXACT    0
#define SCORE_GE       1
#define SCORE_LE       2

// transposition table entry
struct TTEntry
{
    union
    {
        uint64 hashKey;
        struct
        {
            // 16 LSB's are not important as the hash table size is at least > 64k entries

            // TODO at least 16 bits can be easily re-used for storing something else. Find more info that can be stored here.
            uint16 free1;
            uint8 hashPart[6];  // most significant bits of the hash key
        };
    };                      // 64 bits

    union
    {
        uint64 otherInfo;
        struct
        {
            uint16  bestMove;       // 16 bits
            int16  score;           // 16 bits
            uint8  scoreType;       // 8 bits       // this can be clubbed inside score and we can get 8 more bits of free space if needed
            uint8  depth;           // 8 bits
            uint16 age;             
        };
    };
};

// two entries per slot
// deepest and most recent
struct DualTTEntry
{
    union 
    {
        uint64 hashKey;
        struct
        {
            uint16 bestMove;     // best move found
            uint8  hashPart[6];  // most significant bits of the hash key
        };
    } deepest; // 64 bits

    union
    {
        uint64 hashKey;
        struct
        {
            uint16 bestMove;
            uint8  hashPart[6];  // most significant bits of the hash key
        };
    } mostRecent; // 64 bits

    union
    {
        uint64 otherInfo;
        struct
        {
            int16  scoreDeepest;                // 16 bits
            int16  scoreMostRecent;             // 16 bits

            uint8  scoreTypeDeepest : 2;        // 2 bits
            uint8  scoreTypeMostRecent : 2;     // 2 bits

            // 4 bits free here

            uint8  depthDeepest;                // 8 bits
            uint8  depthMostRecent;             // 8 bits

            uint8  ageDeepest;                  // 8 bits
        };
    };

};
CT_ASSERT(sizeof(DualTTEntry) == 24);

// size of q-search TT, (2 MB)
#define Q_TT_SIZE_BITS  18
#define Q_TT_ELEMENTS   (1 << Q_TT_SIZE_BITS)
#define Q_TT_INDEX_BITS (Q_TT_ELEMENTS - 1)
#define Q_TT_HASH_BITS  (0xFFFFFFFFFFFFFFFFull ^ Q_TT_INDEX_BITS)

// index bits should be large enough to hold score and score type (score type is 2 bits)
CT_ASSERT(Q_TT_SIZE_BITS >= sizeof(int16) + 2);

class TranspositionTable
{
private:
#if USE_DUAL_SLOT_TT == 1
    static DualTTEntry *TT;
#else
    static TTEntry *TT;        // the transposition table
#endif
    static uint64  size;       // size in elements
    static uint64  indexBits;  // size-1
    static uint64  hashBits;   // ALLSET ^ indexBits;

    static uint64  *qTT;       // a small TT dedicated for q-search
public:
    static void  init(int byteSize = DEAFULT_TT_SIZE);
    static void  destroy();
    static void  reset();

    static bool  lookup(uint64 hash, int searchDepth, int16 *score, uint8 *scoreType, int *foundDepth, CMove *bestMove);
    static void  update(uint64 hash, int16 score, uint8 scoreType, CMove bestMove, int depth, int age);

    static bool  lookup_q(uint64 hash, int16 *eval, uint8 *type);
    static void  update_q(uint64 hash, int16  eval, uint8  type);
};

class BitBoardUtils
{
public:
    // set of random numbers for zobrist hashing
    static ZobristRandoms zob;
private:

    // various lookup tables used for move generation
    // bit mask containing squares between two given squares
    static uint64 Between[64][64];

    // bit mask containing squares in the same 'line' as two given squares
    static uint64 Line[64][64];

    // squares a piece can attack in an empty board
    static uint64 RookAttacks[64];
    static uint64 BishopAttacks[64];
    static uint64 QueenAttacks[64];
    static uint64 KingAttacks[64];
    static uint64 KnightAttacks[64];
    static uint64 pawnAttacks[2][64];

    // magic lookup tables
    // plain magics (Fancy magic lookup tables in FancyMagics.h)
    #define ROOK_MAGIC_BITS    12
    #define BISHOP_MAGIC_BITS  9

    static uint64 rookMagics[64];
    static uint64 bishopMagics[64];

    // same as RookAttacks and BishopAttacks, but corner bits masked off
    static uint64 RookAttacksMasked[64];
    static uint64 BishopAttacksMasked[64];

    static uint64   rookMagicAttackTables[64][1 << ROOK_MAGIC_BITS  ];  // 2 MB
    static uint64 bishopMagicAttackTables[64][1 << BISHOP_MAGIC_BITS];  // 256 KB

    // fancy magic lookup tables
    static uint64 fancy_magic_lookup_table[97264];      // 760 KB
    static FancyMagicEntry bishop_magics_fancy[64];     //   1 KB
    static FancyMagicEntry rook_magics_fancy[64];       //   1 KB

    // byte lookup version of the above table
    static uint8 fancy_byte_magic_lookup_table[97264];  // 95 KB
    static uint64 fancy_byte_RookLookup[4900];          // 39 K
    static uint64 fancy_byte_BishopLookup[1428];        // 11 K


    // lookup table helper functions
    static uint64 sqsInBetweenLUT(uint8 sq1, uint8 sq2);
    static uint64 sqsInLineLUT(uint8 sq1, uint8 sq2);
    static uint64 sqKnightAttacks(uint8 sq);
    static uint64 sqKingAttacks(uint8 sq);
    static uint64 sqRookAttacks(uint8 sq);
    static uint64 sqBishopAttacks(uint8 sq);
    static uint64 sqBishopAttacksMasked(uint8 sq);
    static uint64 sqRookAttacksMasked(uint8 sq);
    static uint64 sqRookMagics(uint8 sq);
    static uint64 sqBishopMagics(uint8 sq);
    static uint64 sqRookMagicAttackTables(uint8 sq, uint64 index);
    static uint64 sqBishopMagicAttackTables(uint8 sq, uint64 index);
    static uint64 sq_fancy_magic_lookup_table(int index);
    static FancyMagicEntry sq_bishop_magics_fancy(int sq);
    static FancyMagicEntry sq_rook_magics_fancy(int sq);
    static uint8 sq_fancy_byte_magic_lookup_table(int index);
    static uint64 sq_fancy_byte_BishopLookup(int index);
    static uint64 sq_fancy_byte_RookLookup(int index);


    // util functions
    static uint8 popCount(uint64 x);
    static uint8 bitScan (uint64 x);

    static uint64 getOne(uint64 x);

    static bool isMultiple(uint64 x);
    static bool isSingular(uint64 x);

    // internal functions for move generation

    static uint64 northOne(uint64 x);
    static uint64 southOne(uint64 x);
    static uint64 eastOne(uint64 x);
    static uint64 westOne(uint64 x);
    static uint64 northEastOne(uint64 x);
    static uint64 northWestOne(uint64 x);
    static uint64 southEastOne(uint64 x);
    static uint64 southWestOne(uint64 x);

    static uint64 northFill(uint64 gen, uint64 pro);
    static uint64 southFill(uint64 gen, uint64 pro);
    static uint64 eastFill(uint64 gen, uint64 pro);
    static uint64 westFill(uint64 gen, uint64 pro);
    static uint64 northEastFill(uint64 gen, uint64 pro);
    static uint64 northWestFill(uint64 gen, uint64 pro);
    static uint64 southEastFill(uint64 gen, uint64 pro);
    static uint64 southWestFill(uint64 gen, uint64 pro);


    // attacks in the given direction
    // need to OR with ~(pieces of side to move) to avoid killing own pieces

    static uint64 northAttacks(uint64 gen, uint64 pro);
    static uint64 southAttacks(uint64 gen, uint64 pro);
    static uint64 eastAttacks(uint64 gen, uint64 pro);
    static uint64 westAttacks(uint64 gen, uint64 pro);
    static uint64 northEastAttacks(uint64 gen, uint64 pro);
    static uint64 northWestAttacks(uint64 gen, uint64 pro);
    static uint64 southEastAttacks(uint64 gen, uint64 pro);
    static uint64 southWestAttacks(uint64 gen, uint64 pro);


    // attacks by pieces of given type
    // pro - empty squares

    static uint64 bishopAttacksKoggeStone(uint64 bishops, uint64 pro);
    static uint64 rookAttacksKoggeStone(uint64 rooks, uint64 pro);

#if USE_SLIDING_LUT == 1
    static uint64 bishopAttacks(uint64 bishop, uint64 pro);
    static uint64 rookAttacks(uint64 rook, uint64 pro);
    static uint64 multiBishopAttacks(uint64 bishops, uint64 pro);
    static uint64 multiRookAttacks(uint64 rooks, uint64 pro);
#else
    // kogge stone handles multiple attackers automatically
    #define bishopAttacks bishopAttacksKoggeStone
    #define rookAttacks   rookAttacksKoggeStone
    #define multiBishopAttacks bishopAttacksKoggeStone
    #define multiRookAttacks   rookAttacksKoggeStone
#endif

    static uint64 multiKnightAttacks(uint64 knights);

    // not used
#if 0
    static uint64 queenAttacks(uint64 queens, uint64 pro);
#endif

    static uint64 kingAttacks(uint64 kingSet);
    static uint64 knightAttacks(uint64 knights);

    // compute without using lookup table
    static uint64 squaresInBetween(uint8 sq1, uint8 sq2);
    static uint64 squaresInLine(uint8 sq1, uint8 sq2);

    // this is the generic version that either uses lookup table, or calls the above functions if LUT is disabled
    static uint64 sqsInBetween(uint8 sq1, uint8 sq2);
    static uint64 sqsInLine(uint8 sq1, uint8 sq2);

    static void updateCastleFlag(HexaBitBoardPosition *pos, uint64 dst, uint8 chance);
    static uint64 findPinnedPieces(uint64 myKing, uint64 enemyBishops, uint64 enemyRooks, uint64 allPieces, uint8 kingIndex);
    static uint64 findAttackedSquares(uint64 emptySquares, uint64 enemyBishops, uint64 enemyRooks, uint64 enemyPawns, uint64 enemyKnights,
                                                    uint64 enemyKing, uint64 myKing, uint8 enemyColor);
private:

    static void addMove(int *nMoves, HexaBitBoardPosition **newPos, HexaBitBoardPosition *newBoard);
    static void addSlidingMove(int *nMoves, HexaBitBoardPosition **newPos, HexaBitBoardPosition *pos, uint64 src, uint64 dst, uint8 chance);
    static void addKnightMove(int *nMoves, HexaBitBoardPosition **newPos, HexaBitBoardPosition *pos, uint64 src, uint64 dst, uint8 chance);
    static void addKingMove(int *nMoves, HexaBitBoardPosition **newPos, HexaBitBoardPosition *pos, uint64 src, uint64 dst, uint8 chance);
    static void addCastleMove(int *nMoves, HexaBitBoardPosition **newPos, HexaBitBoardPosition *pos,
                              uint64 kingFrom, uint64 kingTo, uint64 rookFrom, uint64 rookTo, uint8 chance);
    static void addSinglePawnMove(int *nMoves, HexaBitBoardPosition **newPos, HexaBitBoardPosition *pos,
                                  uint64 src, uint64 dst, uint8 chance, bool doublePush, uint8 pawnIndex);
    static void addEnPassentMove(int *nMoves, HexaBitBoardPosition **newPos, HexaBitBoardPosition *pos, uint64 src, uint64 dst, uint8 chance);
    static void addPawnMoves(int *nMoves, HexaBitBoardPosition **newPos, HexaBitBoardPosition *pos, uint64 src, uint64 dst, uint8 chance);

    static void addCompactMove(int *nMoves, CMove **genMoves, uint8 from, uint8 to, uint8 flags);
    static void addCompactPawnMoves(int *nMoves, CMove **genMoves, uint8 from, uint64 dst, uint8 flags);

    // used for magic lookup table initialization
    static uint64 getOccCombo(uint64 mask, uint64 i);
    static uint64 findMagicCommon(uint64 occCombos[], uint64 attacks[], uint64 attackTable[], int numCombos, int bits, uint64 preCalculatedMagic = 0,
                                  uint64 *uniqueAttackTable = NULL, uint8 *byteIndices = NULL, int *numUniqueAttacks = NULL);
    static uint64 findRookMagicForSquare(int square, uint64 magicAttackTable[], uint64 magic = 0, uint64 *uniqueAttackTable = NULL, uint8 *byteIndices = NULL, int *numUniqueAttacks = 0);
    static uint64 findBishopMagicForSquare(int square, uint64 magicAttackTable[], uint64 magic = 0, uint64 *uniqueAttackTable = NULL, uint8 *byteIndices = NULL, int *numUniqueAttacks = 0);

    template<uint8 chance>
    static int generateBoardsOutOfCheck(HexaBitBoardPosition *pos, HexaBitBoardPosition *newPositions, uint64 allPawns,
                                        uint64 allPieces, uint64 myPieces, uint64 enemyPieces, uint64 pinned, uint64 threatened, uint8 kingIndex);

    template<uint8 chance>
    static int countMovesOutOfCheck(HexaBitBoardPosition *pos, uint64 allPawns, uint64 allPieces, uint64 myPieces, uint64 enemyPieces, uint64 pinned, uint64 threatened, uint8 kingIndex);

    static void generateSlidingCapturesForSquare(uint64 square, uint8 sqIndex, uint64 slidingSources, uint64 pinned, uint8 kingIndex, int *nMoves, CMove **genMoves);

    static bool generateFirstSlidingCapturesForSquare(uint64 square, uint8 sqIndex, uint64 slidingSources, uint64 pinned, uint8 kingIndex, CMove *genMove);

    template<uint8 chance>
    static int generateMoves(HexaBitBoardPosition *pos, CMove *genMoves);

    template<uint8 chance>
    static int generateMovesOutOfCheck(HexaBitBoardPosition *pos, CMove *genMoves, uint64 allPawns, uint64 allPieces, uint64 myPieces, uint64 enemyPieces, uint64 pinned, uint64 threatened, uint8 kingIndex);

    template<uint8 chance>
    static void generateLVACapturesForSquare(uint64 square, uint64 pinned, uint64 threatened, uint64 myKing, uint8 kingIndex, uint64 allPieces,
                                            uint64 myPawns, uint64 myNonPinnedKnights, uint64 myBishops, uint64 myRooks, uint64 myQueens, int *nMoves, CMove **genMoves);


    template<uint8 chance>
    static bool generateFirstLVACaptureForSquare(uint64 square, uint64 pinned, uint64 threatened, uint64 myKing, uint8 kingIndex, uint64 allPieces,
                                                 uint64 myPawns, uint64 myNonPinnedKnights, uint64 myBishops, uint64 myRooks, uint64 myQueens, CMove *genMove);


    // core functions
    static int16 getPieceSquareScore(uint64 pieceSet, uint64 whiteSet, const int16 table[]);

    // evaluate mobility
    static int16 evaluateMobility(const EvalBitBoard &ebb, bool endGame);

    // evaluate pawn structure
    static int16 evaluatePawnStructure(const EvalBitBoard &ebb, bool endGame);

public:
    template<uint8 chance>
    static void makeMove(HexaBitBoardPosition *pos, uint64 &hash, CMove move);

    template<uint8 chance>
    static int generateBoards(HexaBitBoardPosition *pos, HexaBitBoardPosition *newPositions);

    template<uint8 chance>
    static int countMoves(HexaBitBoardPosition *pos);

    // generate captures AND promotions
    template<uint8 chance>
    static int generateCaptures(const ExpandedBitBoard *bb, CMove *genMoves);

    // generate moves that are not captures OR promotions
    template<uint8 chance>
    static int generateNonCaptures(const ExpandedBitBoard *bb, CMove *genMoves);

    // generate moves for evading checks (called only when the side to move is in check)
    template<uint8 chance>
    static int generateMovesOutOfCheck(const ExpandedBitBoard *bb, CMove *genMoves);

    // generate moves giving check to the opponent side
    // used only by q-search
    template<uint8 chance>
    static int generateMovesCausingCheck(const ExpandedBitBoard *bb, CMove *genMoves);

    static int getPieceAtSquare(HexaBitBoardPosition *pos, uint64 square);

    template<uint8 chance>
    static int16 seeSquare(HexaBitBoardPosition *pos, uint64 square);

public:
    // unpack the bitboard structure
    template<uint8 chance>
    static ExpandedBitBoard ExpandBitBoard(HexaBitBoardPosition *pos);

    static bool IsInCheck(HexaBitBoardPosition *pos);

    // count the no of child moves possible at given board position
    static int CountMoves(HexaBitBoardPosition *pos);

    // generate moves for the given board position
    static int GenerateMoves(HexaBitBoardPosition *pos, CMove *genMoves);

    // generate child boards for the given board position
    static int GenerateBoards(HexaBitBoardPosition *pos, HexaBitBoardPosition *newPositions);

    // generate only captures - in MVV-LVA order
    static int GenerateCaptures(HexaBitBoardPosition *pos, CMove *genMoves);
    static int GenerateNonCaptures(HexaBitBoardPosition *pos, CMove *genMoves);

    // make the given move in the given board position
    static void MakeMove(HexaBitBoardPosition *pos, uint64 &hash, CMove move);

    // evaluate a board position
    static int16 Evaluate(HexaBitBoardPosition *pos);

    // evaluate a capture using SEE
    template<uint8 chance>
    static int16 EvaluateSEE(HexaBitBoardPosition *pos, CMove capture);

    // evaluate if the position is a draw
    static bool isDrawn(ExpandedBitBoard const &bb);

    // compute zobrist hash key for the given board position
    static uint64 ComputeZobristKey(HexaBitBoardPosition *pos);

    // returns if the the move made is ir-reversible or not (TODO: maybe we can just check CMove flags?)
    static bool IsIrReversibleMove(HexaBitBoardPosition *pos, CMove move);

    static void init();
};
