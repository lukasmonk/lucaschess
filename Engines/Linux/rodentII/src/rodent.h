/*
Rodent, a UCI chess playing engine derived from Sungorus 1.4
Copyright (C) 2009-2011 Pablo Vazquez (Sungorus author)
Copyright (C) 2011-2016 Pawel Koziol

Rodent is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published
by the Free Software Foundation, either version 3 of the License,
or (at your option) any later version.

Rodent is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty
of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

// bench: 894.995
// bench 12: 8.520.838
// bench 15: 39.561.531  40.4  2.268
// REGEX to count all the lines under MSVC 13: ^(?([^\r\n])\s)*[^\s+?/]+[^\n]*$
// 6087 lines of code
// 0.9.50: 56,3% vs 0.9.33

#pragma once
#define PROG_NAME "Rodent II 0.9.64"

enum eColor{WC, BC, NO_CL};
enum ePieceType{P, N, B, R, Q, K, NO_TP};
enum ePiece{WP, BP, WN, BN, WB, BBi, WR, BR, WQ, BQ, WK, BK, NO_PC};
enum eFile {FILE_A, FILE_B, FILE_C, FILE_D, FILE_E, FILE_F, FILE_G, FILE_H};
enum eRank {RANK_1, RANK_2, RANK_3, RANK_4, RANK_5, RANK_6, RANK_7, RANK_8};
enum eCastleFlag { W_KS = 1, W_QS = 2, B_KS = 4, B_QS = 8 };
enum eMoveType {NORMAL, CASTLE, EP_CAP, EP_SET, N_PROM, B_PROM, R_PROM, Q_PROM};
enum eHashEntry{NONE, UPPER, LOWER, EXACT};
enum eMoveFlag {MV_NORMAL, MV_HASH, MV_CAPTURE, MV_KILLER, MV_BADCAPT};
enum eFactor   {F_ATT, F_MOB, F_PST, F_PAWNS, F_PASSERS, F_TROPISM, F_OUTPOST, F_LINES, F_PRESSURE, F_OTHERS, N_OF_FACTORS };
enum eDynFactor {DF_OWN_ATT, DF_OPP_ATT, DF_OWN_MOB, DF_OPP_MOB};
enum eAsymmetric {SD_ATT, SD_MOB, OPP_ATT, OPP_MOB};

enum eSquare{
  A1, B1, C1, D1, E1, F1, G1, H1,
  A2, B2, C2, D2, E2, F2, G2, H2,
  A3, B3, C3, D3, E3, F3, G3, H3,
  A4, B4, C4, D4, E4, F4, G4, H4,
  A5, B5, C5, D5, E5, F5, G5, H5,
  A6, B6, C6, D6, E6, F6, G6, H6,
  A7, B7, C7, D7, E7, F7, G7, H7,
  A8, B8, C8, D8, E8, F8, G8, H8,
  NO_SQ
};

typedef unsigned long long U64;

#define MAX_PLY         64
#define MAX_MOVES       256
#define INF             32767
#define MATE            32000
#define MAX_EVAL        29999
#define MAX_INT    2147483646
#define HIST_LIMIT (1 << 15)

#define RANK_1_BB       (U64)0x00000000000000FF
#define RANK_2_BB       (U64)0x000000000000FF00
#define RANK_3_BB       (U64)0x0000000000FF0000
#define RANK_4_BB       (U64)0x00000000FF000000
#define RANK_5_BB       (U64)0x000000FF00000000
#define RANK_6_BB       (U64)0x0000FF0000000000
#define RANK_7_BB       (U64)0x00FF000000000000
#define RANK_8_BB       (U64)0xFF00000000000000

static const U64 bbRelRank[2][8] = { { RANK_1_BB, RANK_2_BB, RANK_3_BB, RANK_4_BB, RANK_5_BB, RANK_6_BB, RANK_7_BB, RANK_8_BB },
                                     { RANK_8_BB, RANK_7_BB, RANK_6_BB, RANK_5_BB, RANK_4_BB, RANK_3_BB, RANK_2_BB, RANK_1_BB } };

static const U64 bbHomeZone[2] = { RANK_1_BB | RANK_2_BB | RANK_3_BB | RANK_4_BB,
                                   RANK_8_BB | RANK_7_BB | RANK_6_BB | RANK_5_BB };

static const U64 bbAwayZone[2] = { RANK_8_BB | RANK_7_BB | RANK_6_BB | RANK_5_BB,
                                   RANK_1_BB | RANK_2_BB | RANK_3_BB | RANK_4_BB };

#define FILE_A_BB       (U64)0x0101010101010101
#define FILE_B_BB       (U64)0x0202020202020202
#define FILE_C_BB       (U64)0x0404040404040404
#define FILE_D_BB       (U64)0x0808080808080808
#define FILE_E_BB       (U64)0x1010101010101010
#define FILE_F_BB       (U64)0x2020202020202020
#define FILE_G_BB       (U64)0x4040404040404040
#define FILE_H_BB       (U64)0x8080808080808080

#define bbWhiteSq       (U64)0x55AA55AA55AA55AA
#define bbBlackSq       (U64)0xAA55AA55AA55AA55

#define DIAG_A1H8_BB    (U64)0x8040201008040201
#define DIAG_A8H1_BB    (U64)0x0102040810204080
#define DIAG_B8H2_BB    (U64)0x0204081020408000

#define bbNotA          (U64)0xfefefefefefefefe // ~FILE_A_BB
#define bbNotH          (U64)0x7f7f7f7f7f7f7f7f // ~FILE_H_BB

#define ShiftNorth(x)   (x<<8)
#define ShiftSouth(x)   (x>>8)
#define ShiftWest(x)    ((x & bbNotA)>>1)
#define ShiftEast(x)    ((x & bbNotH)<<1)
#define ShiftNW(x)      ((x & bbNotA)<<7)
#define ShiftNE(x)      ((x & bbNotH)<<9)
#define ShiftSW(x)      ((x & bbNotA)>>9)
#define ShiftSE(x)      ((x & bbNotH)>>7)

#define JustOne(bb)     (bb && !(bb & (bb-1)))
#define MoreThanOne(bb) ( bb & (bb - 1) )

#define SCALE(x,y) ((x*y)/100)
#define SIDE_RANDOM     (~((U64)0))

#define START_POS       "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq -"

#define SqBb(x)         ((U64)1 << (x))

#define Cl(x)           ((x) & 1)
#define Tp(x)           ((x) >> 1)
#define Pc(x, y)        (((y) << 1) | (x))

#define File(x)         ((x) & 7)
#define Rank(x)         ((x) >> 3)
#define Sq(x, y)        (((y) << 3) | (x))

#define Abs(x)          ((x) > 0 ? (x) : -(x))
#define Max(x, y)       ((x) > (y) ? (x) : (y))
#define Min(x, y)       ((x) < (y) ? (x) : (y))

#define Fsq(x)          ((x) & 63)
#define Tsq(x)          (((x) >> 6) & 63)
#define MoveType(x)     ((x) >> 12)
#define IsProm(x)       ((x) & 0x4000)
#define PromType(x)     (((x) >> 12) - 3)

#define Opp(x)          ((x) ^ 1)

#define InCheck(p)      Attacked(p, KingSq(p, p->side), Opp(p->side))
#define Illegal(p)      Attacked(p, KingSq(p, Opp(p->side)), p->side)
#define MayNull(p)      (((p)->cl_bb[(p)->side] & ~((p)->tp_bb[P] | (p)->tp_bb[K])) != 0)

#define PcBb(p, x, y)   ((p)->cl_bb[x] & (p)->tp_bb[y])
#define OccBb(p)        ((p)->cl_bb[WC] | (p)->cl_bb[BC])
#define UnoccBb(p)      (~OccBb(p))
#define TpOnSq(p, x)    (Tp((p)->pc[x]))
#define KingSq(p, x)    ((p)->king_sq[x])
#define IsOnSq(p, sd, pc, sq) ( PcBb(p, sd, pc) & SqBb(sq) )

#ifdef _WIN32
#define FORCEINLINE __forceinline
#else
#define FORCEINLINE __inline
#endif

#define USE_FIRST_ONE_INTRINSICS

// Compiler and architecture dependent versions of FirstOne() function,
// triggered by defines at the top of this file.
#ifdef USE_FIRST_ONE_INTRINSICS
#ifdef _WIN32
#include <intrin.h>
#ifdef _WIN64
#pragma intrinsic(_BitScanForward64)
#endif

#ifdef _MSC_VER
#ifndef _WIN64
const int lsb_64_table[64] =
{
  63, 30, 3, 32, 59, 14, 11, 33,
  60, 24, 50, 9, 55, 19, 21, 34,
  61, 29, 2, 53, 51, 23, 41, 18,
  56, 28, 1, 43, 46, 27, 0, 35,
  62, 31, 58, 4, 5, 49, 54, 6,
  15, 52, 12, 40, 7, 42, 45, 16,
  25, 57, 48, 13, 10, 39, 8, 44,
  20, 47, 38, 22, 17, 37, 36, 26
};

/**
* bitScanForward
* @author Matt Taylor (2003)
* @param bb bitboard to scan
* @precondition bb != 0
* @return index (0..63) of least significant one bit
*/
static int FORCEINLINE  bitScanForward(U64 bb) {
  unsigned int folded;
  bb ^= bb - 1;
  folded = (int)bb ^ (bb >> 32);
  return lsb_64_table[folded * 0x78291ACF >> 26];
}
#endif
#endif
static int FORCEINLINE FirstOne(U64 x) {
#ifndef _WIN64
  return bitScanForward(x);
#else
  unsigned long index = -1;
  _BitScanForward64(&index, x);
  return index;
#endif
}

#elif defined(__GNUC__)

static int FORCEINLINE FirstOne(U64 x) {
  int tmp = __builtin_ffsll(x);
  if (tmp == 0) return -1;
  else return tmp - 1;
}

#endif

#else
#define FirstOne(x)     bit_table[(((x) & (~(x) + 1)) * (U64)0x0218A392CD3D5DBF) >> 58] // first "1" in a bitboard
#endif


#define REL_SQ(sq,cl)   ( sq ^ (cl * 56) )
#define RelSqBb(sq,cl)  ( SqBb(REL_SQ(sq,cl) ) )

typedef struct {
  int ttp;
  int castle_flags;
  int ep_sq;
  int rev_moves;
  U64 hash_key;
  U64 pawn_key;
} UNDO;

typedef class {
private:
  U64 p_attacks[2][64];
  U64 n_attacks[64];
  U64 k_attacks[64];

  U64 FillOcclSouth(U64 bbStart, U64 bbBlock);
  U64 FillOcclNorth(U64 bbStart, U64 bbBlock);
  U64 FillOcclEast(U64 bbStart, U64 bbBlock);
  U64 FillOcclWest(U64 bbStart, U64 bbBlock);
  U64 FillOcclNE(U64 bbStart, U64 bbBlock);
  U64 FillOcclNW(U64 bbStart, U64 bbBlock);
  U64 FillOcclSE(U64 bbStart, U64 bbBlock);
  U64 FillOcclSW(U64 bbStart, U64 bbBlock);
  U64 GetBetween(int sq1, int sq2);

public:
  U64 bbBetween[64][64];
  void Init(void);
  U64 ShiftFwd(U64 bb, int sd);
  U64 ShiftSideways(U64 bb);
  U64 GetWPControl(U64 bb);
  U64 GetBPControl(U64 bb);
  U64 GetDoubleWPControl(U64 bb);
  U64 GetDoubleBPControl(U64 bb);
  U64 GetFrontSpan(U64 bb, int sd);
  U64 FillNorth(U64 bb);
  U64 FillSouth(U64 bb);
  U64 FillNorthSq(int sq);
  U64 FillSouthSq(int sq);
  U64 FillNorthExcl(U64 bb);
  U64 FillSouthExcl(U64 bb);

  int PopCnt(U64);
  int PopFirstBit(U64 * bb);

  U64 PawnAttacks(int sd, int sq);
  U64 KingAttacks(int sq);
  U64 KnightAttacks(int sq);
  U64 RookAttacks(U64 occ, int sq);
  U64 BishAttacks(U64 occ, int sq);
  U64 QueenAttacks(U64 occ, int sq);
} cBitBoard;

extern cBitBoard BB;

typedef class {
public:
  U64 cl_bb[2];
  U64 tp_bb[6];
  int pc[64];
  int king_sq[2];
  int mg_sc[2];
  int eg_sc[2];
  int cnt[2][6];
  int phase;
  int side;
  int castle_flags;
  int ep_sq;
  int rev_moves;
  int head;
  U64 hash_key;
  U64 pawn_key;
  U64 rep_list[256];

  U64 Pawns(int sd);
  U64 Knights(int sd);
  U64 Bishops(int sd);
  U64 Rooks(int sd);
  U64 Queens(int sd);
  U64 Kings(int sd);
  U64 StraightMovers(int sd);
  U64 DiagMovers(int sd);
  int PawnEndgame(void);

  void DoMove(int move, UNDO * u);
  void DoNull(UNDO * u);
  void UndoMove(int move, UNDO * u);
  void UndoNull(UNDO * u);
} POS;

typedef class {
public:
  U64 passed[2][64];
  U64 adjacent[8];
  U64 supported[2][64];
  U64 king_zone[2][64];
  void Init(void);
} cMask;

extern cMask Mask;

typedef struct {
	int mg[2][N_OF_FACTORS];
	int eg[2][N_OF_FACTORS];
	U64 bbAllAttacks[2];
	U64 bbEvAttacks[2];
	U64 bbPawnTakes[2];
	U64 bbTwoPawnsTake[2];
	U64 bbPawnCanTake[2];
} eData;

typedef class {
private:
  void Add(eData *e, int sd, int factor, int mg_bonus, int eg_bonus);
  void Add(eData *e, int sd, int factor, int bonus);
  void ScoreMaterial(POS * p, eData *e, int sd);
  void ScorePassers(POS * p, eData *e, int sd);
  void ScorePieces(POS * p, eData *e, int sd);
  void ScoreHanging(POS *p, eData *e, int sd);
  void ScorePatterns(POS *p, eData *e);
  void ScoreKing(POS *p, eData *e, int sd);
  void ScoreUnstoppable(eData *e, POS *p);
  void ScoreKingFile(POS * p, int sd, U64 bbFile, int *shelter, int *storm);
  int ScoreFileShelter(U64 bbOwnPawns, int sd);
  int ScoreFileStorm(U64 bbOppPawns, int sd);
  int ScoreChains(POS *p, int sd);
  void ScoreOutpost(POS * p, eData *e, int sd, int pc, int sq);
  void ScorePawns(POS * p, eData *e, int sd);
  void FullPawnEval(POS * p, eData *e, int use_hash);

public:
  int prog_side;
  void Init(void);
  int Return(POS * p, eData * e, int use_hash);
  void Print(POS *p);
} cEval;

extern cEval Eval;

typedef struct {
  POS *p;
  int phase;
  int trans_move;
  int ref_move;
  int killer1;
  int killer2;
  int *next;
  int *last;
  int move[MAX_MOVES];
  int value[MAX_MOVES];
  int *badp;
  int bad[MAX_MOVES];
} MOVES;

typedef struct {
  U64 key;
  short date;
  short move;
  short score;
  unsigned char flags;
  unsigned char depth;
} ENTRY;

void AllocTrans(int mbsize);
int Attacked(POS *p, int sq, int sd);
U64 AttacksFrom(POS *p, int sq);
U64 AttacksTo(POS *p, int sq);
int BadCapture(POS *p, int move);
void Bench(int depth);
void BuildPv(int *dst, int *src, int move);
void CheckTimeout(void);
int CheckmateHelper(POS *p);
void ClearEvalHash(void);
void ClearPawnHash(void);
void ClearHist(void);
void ClearTrans(void);
void DecreaseHistory(POS *p, int move, int depth);
void DisplayCurrmove(int move, int tried);
void DisplayPv(int score, int *pv);
void DisplaySpeed(void);
int DrawScore(POS * p);
int EloToSpeed(int elo);
int EloToBlur(int elo);
int *GenerateCaptures(POS *p, int *list);
int *GenerateQuiet(POS *p, int *list);
int *GenerateQuietChecks(POS *p, int *list);
U64 GetNps(int elapsed);
int GetDrawFactor(POS *p, int sd);
void UpdateHistory(POS *p, int last_move, int move, int depth, int ply);
void Init(void);
void InitSearch(void);
void InitCaptures(POS *p, MOVES *m);
void InitMoves(POS *p, MOVES *m, int trans_move, int ref_move, int ply);
void InitWeights(void);
int InputAvailable(void);
U64 InitHashKey(POS * p);
U64 InitPawnKey(POS * p);
void Iterate(POS *p, int *pv);
int Legal(POS *p, int move);
void MoveToStr(int move, char *move_str);
void PrintMove(int move);
int MvvLva(POS *p, int move);
int NextCapture(MOVES *m);
int NextCaptureOrCheck(MOVES * m);
int NextMove(MOVES *m, int *flag);
void ParseGo(POS *, char *);
void ParseMoves(POS *p, char *ptr);
void ParsePosition(POS *, char *);
void ParseSetoption(char *);
int Perft(POS *p, int ply, int depth);
void PrintBoard(POS *p);
char *ParseToken(char *, char *);
void PvToStr(int *, char *);
int Quiesce(POS *p, int ply, int alpha, int beta, int *pv);
int QuiesceChecks(POS *p, int ply, int alpha, int beta, int *pv);
int QuiesceFlee(POS *p, int ply, int alpha, int beta, int *pv);
U64 Random64(void);
void ReadLine(char *str, int n);
void ResetEngine(void);
int IsDraw(POS * p);
int KPKdraw(POS *p, int sd);
void ScoreCaptures(MOVES *);
void ScoreQuiet(MOVES *m);
void SetWeight(int weight_name, int value);
int Widen(POS *p, int depth, int * pv, int lastScore);
int Refutation(int move);
void ReadPersonality(char *fileName);
int SearchRoot(POS *p, int ply, int alpha, int beta, int depth, int *pv);
int Search(POS *p, int ply, int alpha, int beta, int depth, int was_null, int last_move, int last_capt_sq, int node_type, int *pv);
int SelectBest(MOVES *m);
void SetPosition(POS *p, char *epd);
void SetAsymmetricEval(int sd);
int StrToMove(POS *p, char *move_str);
int Swap(POS *p, int from, int to);
void Think(POS *p, int *pv);
void TrimHistory(void);
int Timeout(void);
int TransRetrieve(U64 key, int *move, int *score, int alpha, int beta, int depth, int ply);
void TransStore(U64 key, int move, int score, int flags, int depth, int ply);
void UciLoop(void);

extern int castle_mask[64];
extern const int bit_table[64];
extern const int tp_value[7];
extern const int phase_value[7];
extern int refutation[64][64];
extern int root_side;
extern int history[12][64];
extern int killer[MAX_PLY][2];
extern U64 zob_piece[12][64];
extern U64 zob_castle[16];
extern U64 zob_ep[8];
extern int pondering;
extern int root_depth;
extern U64 nodes;
extern int abort_search;
extern ENTRY *tt;
extern int tt_size;
extern int tt_mask;
extern int tt_date;

extern int weights[N_OF_FACTORS];
extern int dyn_weights[5];
extern int curr_weights[2][2];
extern int panel_style;
extern int verbose;
extern int time_percentage;
extern int use_book;
extern int hist_limit;
extern int hist_perc;
extern int fl_reading_personality;
extern int fl_separate_books;
extern int fl_elo_slider;

int DifferentBishops(POS * p);
int NotOnBishColor(POS * p, int bishSide, int sq);
int PcMatNone(POS *p, int sd);
int PcMat1Minor(POS *p, int sd);
int PcMat2Minors(POS *p, int sd);
int PcMatNN(POS *p, int sd);
int PcMatBN(POS *p, int sd);
int PcMatB(POS *p, int sd);
int PcMatQ(POS *p, int sd);
int PcMatR(POS *p, int sd);
int PcMatRm(POS *p, int sd);
int PcMatRR(POS *p, int sd);
int PcMatRRm(POS *p, int sd);