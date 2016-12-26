#define W32_BUILD
#undef W32_BUILD

#ifdef W32_BUILD
#define NTDDI_VERSION 0x05010200
#define _WIN32_WINNT 0x0501
#endif

#ifndef W32_BUILD
#define HNI
#undef HNI
#endif

#define CPU_TIMING
#undef CPU_TIMING

#define TUNER
#undef TUNER

#include <iostream>
#include "setjmp.h"
#include "windows.h"
#ifdef TUNER
#include "time.h"

//#define PGN
#define RANDOM_SPHERICAL
//#define WIN_PR
//#define TIMING
//#define RECORD_GAMES

#endif

#define EXPLAIN_EVAL
#undef EXPLAIN_EVAL

#define LARGE_PAGES
//#undef LARGE_PAGES

#define MP_NPS
//#undef MP_NPS

#define TIME_TO_DEPTH
//#undef TIME_TO_DEPTH

using namespace std;

typedef unsigned char uint8;
typedef char sint8;
typedef unsigned short uint16;
typedef short sint16;
typedef unsigned int uint32;
typedef int sint32;
typedef unsigned long long uint64;
typedef long long sint64;

#define Convert(x,type) ((type)(x))

#define Abs(x) ((x) > 0 ? (x) : (-(x)))
#define Sgn(x) ((x) == 0 ? 0 : ((x) > 0 ? 1 : (-1)))
#define Min(x,y) ((x) < (y) ? (x) : (y))
#define Max(x,y) ((x) > (y) ? (x) : (y))
#define Sqr(x) ((x) * (x))
#define T(x) ((x) != 0)
#define F(x) ((x) == 0)
#define Even(x) F((x) & 1)
#define Odd(x) T((x) & 1)
#define Combine(x,y) ((x) | ((y) << 16))
#define Compose(x,y) ((x) + ((y) << 16))
#define Compose16(x,y) Compose((x)/16,(y)/16)
#define Compose64(x,y) Compose((x)/64,(y)/64)
#define Compose256(x,y) Compose((x)/256,(y)/256)
#define Opening(x) Convert((x) & 0xFFFF,sint16)
#define Endgame(x) ((((x) >> 15) & 1) + Convert((x) >> 16,sint16))

#define File(x) ((x) & 7)
#define Rank(x) ((x) >> 3)
#define CRank(me,x) ((me) ? (7 - Rank(x)) : Rank(x))
#define NDiag(x) (7 - File(x) + Rank(x))
#define SDiag(x) (File(x) + Rank(x))
#define Dist(x,y) Max(Abs(Rank(x)-Rank(y)),Abs(File(x)-File(y)))
#define VarC(var,me) ((me) ? (var##_b) : (var##_w))
#define PVarC(prefix,var,me) ((me) ? (prefix##.##var##_b) : (prefix##.##var##_w))

#define Bit(x) (Convert(1,uint64) << (x))
#ifndef HNI
#define Cut(x) (x &= (x) - 1)
#else
#define Cut(x) (x = _blsr_u64(x))
#endif
#define Multiple(x) T((x) & ((x) - 1))
#define Single(x) F((x) & ((x) - 1))
#define Add(x,b) (x |= Bit(b))

#define From(move) (((move) >> 6) & 0x3f)
#define To(move) ((move) & 0x3f)
#define SetScore(move,score) ((move) = (((move) & 0xFFFF) | ((score) << 16)))
#define BitFrom(move) Bit(From(move))
#define BitTo(move) Bit(To(move))
#define MakeMove(from,to) ((from) << 6) | (to))
#define MakeMoveF(from,to,flags) ((from) << 6) | (to) | (flags))
#define MakeMoveFS(from,to,flags,score) ((from) << 6) | (to) | (flags) | (score))
#define PieceAtOrigin(move) Square(From(move))
#define Target(move) Square(To(move)) 

#define Empty Convert(0,uint64)
#define Filled (~Empty)
#define Interior Convert(0x007E7E7E7E7E7E00,uint64)
#define Boundary (~Interior)
#define WhiteArea Convert(0x00000000FFFFFFFF,uint64)
#define BlackArea (~WhiteArea)
#define LightArea Convert(0x55AA55AA55AA55AA,uint64)
#define DarkArea (~LightArea)
#define FileA Convert(0x0101010101010101,uint64)
#define Line0 Convert(0x00000000000000FF,uint64)

#define High32(x) ((x) >> 32)
#define Low32(x) Convert(x,uint32)

#define White 0
#define Black 1
#define WhitePawn 2
#define BlackPawn 3
#define WhiteKnight 4
#define BlackKnight 5
#define WhiteLight 6
#define BlackLight 7
#define WhiteDark 8
#define BlackDark 9
#define WhiteRook 10
#define BlackRook 11
#define WhiteQueen 12
#define BlackQueen 13
#define WhiteKing 14
#define BlackKing 15

#define IsSlider(x) T(0x3FC0 & Bit(x))

#define CanCastle_OO 1
#define CanCastle_oo 2
#define CanCastle_OOO 4
#define CanCastle_ooo 8

#define FlagCastling 0x1000
#define FlagEP 0x2000
#define FlagPKnight 0x4000
#define FlagPLight 0x6000
#define FlagPDark 0x8000
#define FlagPRook 0xA000
#define FlagPQueen 0xC000

#define IsPromotion(move) T((move) & 0xC000)
#define IsCastling(move) T((move) & 0x1000)
#define IsEP(move) (((move) & 0xF000) == 0x2000)
#define Promotion(move,side) ((side) + (((move) & 0xF000) >> 12))

const uint8 UpdateCastling[64] = {
	0xFF^CanCastle_OOO,0xFF,0xFF,0xFF,0xFF^(CanCastle_OO|CanCastle_OOO),0xFF,0xFF,0xFF^CanCastle_OO,
	0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
	0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
	0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
	0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
	0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
	0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
	0xFF^CanCastle_ooo,0xFF,0xFF,0xFF,0xFF^(CanCastle_oo|CanCastle_ooo),0xFF,0xFF,0xFF^CanCastle_oo
};

const uint64 BMagic[64] = {
    0x0048610528020080, 0x00c4100212410004, 0x0004180181002010, 0x0004040188108502, 
    0x0012021008003040, 0x0002900420228000, 0x0080808410c00100, 0x000600410c500622, 
    0x00c0056084140184, 0x0080608816830050, 0x00a010050200b0c0, 0x0000510400800181, 
    0x0000431040064009, 0x0000008820890a06, 0x0050028488184008, 0x00214a0104068200, 
    0x004090100c080081, 0x000a002014012604, 0x0020402409002200, 0x008400c240128100, 
    0x0001000820084200, 0x0024c02201101144, 0x002401008088a800, 0x0003001045009000, 
    0x0084200040981549, 0x0001188120080100, 0x0048050048044300, 0x0008080000820012, 
    0x0001001181004003, 0x0090038000445000, 0x0010820800a21000, 0x0044010108210110, 
    0x0090241008204e30, 0x000c04204004c305, 0x0080804303300400, 0x00a0020080080080, 
    0x0000408020220200, 0x0000c08200010100, 0x0010008102022104, 0x0008148118008140, 
    0x0008080414809028, 0x0005031010004318, 0x0000603048001008, 0x0008012018000100, 
    0x0000202028802901, 0x004011004b049180, 0x0022240b42081400, 0x00c4840c00400020, 
    0x0084009219204000, 0x000080c802104000, 0x0002602201100282, 0x0002040821880020, 
    0x0002014008320080, 0x0002082078208004, 0x0009094800840082, 0x0020080200b1a010, 
    0x0003440407051000, 0x000000220e100440, 0x00480220a4041204, 0x00c1800011084800, 
    0x000008021020a200, 0x0000414128092100, 0x0000042002024200, 0x0002081204004200
};

const uint64 RMagic[64] = {
    0x00800011400080a6, 0x004000100120004e, 0x0080100008600082, 0x0080080016500080, 
    0x0080040008000280, 0x0080020005040080, 0x0080108046000100, 0x0080010000204080, 
    0x0010800424400082, 0x00004002c8201000, 0x000c802000100080, 0x00810010002100b8, 
    0x00ca808014000800, 0x0002002884900200, 0x0042002148041200, 0x00010000c200a100, 
    0x00008580004002a0, 0x0020004001403008, 0x0000820020411600, 0x0002120021401a00, 
    0x0024808044010800, 0x0022008100040080, 0x00004400094a8810, 0x0000020002814c21, 
    0x0011400280082080, 0x004a050e002080c0, 0x00101103002002c0, 0x0025020900201000, 
    0x0001001100042800, 0x0002008080022400, 0x000830440021081a, 0x0080004200010084, 
    0x00008000c9002104, 0x0090400081002900, 0x0080220082004010, 0x0001100101000820, 
    0x0000080011001500, 0x0010020080800400, 0x0034010224009048, 0x0002208412000841, 
    0x000040008020800c, 0x001000c460094000, 0x0020006101330040, 0x0000a30010010028, 
    0x0004080004008080, 0x0024000201004040, 0x0000300802440041, 0x00120400c08a0011, 
    0x0080006085004100, 0x0028600040100040, 0x00a0082110018080, 0x0010184200221200, 
    0x0040080005001100, 0x0004200440104801, 0x0080800900220080, 0x000a01140081c200, 
    0x0080044180110021, 0x0008804001001225, 0x00a00c4020010011, 0x00001000a0050009, 
    0x0011001800021025, 0x00c9000400620811, 0x0032009001080224, 0x001400810044086a
};

const int BShift[64] = {
    58, 59, 59, 59, 59, 59, 59, 58, 
	59, 59, 59, 59, 59, 59, 59, 59,
    59, 59, 57, 57, 57, 57, 59, 59, 
	59, 59, 57, 55, 55, 57, 59, 59,
    59, 59, 57, 55, 55, 57, 59, 59, 
	59, 59, 57, 57, 57, 57, 59, 59,
    59, 59, 59, 59, 59, 59, 59, 59, 
	58, 59, 59, 59, 59, 59, 59, 58
};

const int BOffset[64] = {
    0, 64, 96, 128, 160, 192, 224, 256, 
    320, 352, 384, 416, 448, 480, 512, 544, 
    576, 608, 640, 768, 896, 1024, 1152, 1184, 
    1216, 1248, 1280, 1408, 1920, 2432, 2560, 2592, 
    2624, 2656, 2688, 2816, 3328, 3840, 3968, 4000, 
    4032, 4064, 4096, 4224, 4352, 4480, 4608, 4640, 
    4672, 4704, 4736, 4768, 4800, 4832, 4864, 4896, 
    4928, 4992, 5024, 5056, 5088, 5120, 5152, 5184 
};

const int RShift[64] = {
    52, 53, 53, 53, 53, 53, 53, 52, 
	53, 54, 54, 54, 54, 54, 54, 53,
    53, 54, 54, 54, 54, 54, 54, 53, 
	53, 54, 54, 54, 54, 54, 54, 53,
    53, 54, 54, 54, 54, 54, 54, 53, 
	53, 54, 54, 54, 54, 54, 54, 53,
    53, 54, 54, 54, 54, 54, 54, 53, 
	52, 53, 53, 53, 53, 53, 53, 52
};

const int ROffset[64] = {
    5248, 9344, 11392, 13440, 15488, 17536, 19584, 21632, 
    25728, 27776, 28800, 29824, 30848, 31872, 32896, 33920, 
    35968, 38016, 39040, 40064, 41088, 42112, 43136, 44160, 
    46208, 48256, 49280, 50304, 51328, 52352, 53376, 54400, 
    56448, 58496, 59520, 60544, 61568, 62592, 63616, 64640, 
    66688, 68736, 69760, 70784, 71808, 72832, 73856, 74880, 
    76928, 78976, 80000, 81024, 82048, 83072, 84096, 85120, 
    87168, 91264, 93312, 95360, 97408, 99456, 101504, 103552
};
uint64 * BOffsetPointer[64];
uint64 * ROffsetPointer[64];

#ifndef HNI
#define BishopAttacks(sq,occ) (*(BOffsetPointer[sq] + (((BMagicMask[sq] & (occ)) * BMagic[sq]) >> BShift[sq])))
#define RookAttacks(sq,occ) (*(ROffsetPointer[sq] + (((RMagicMask[sq] & (occ)) * RMagic[sq]) >> RShift[sq])))
#else
#define BishopAttacks(sq,occ) (*(BOffsetPointer[sq] + _pext_u64(occ,BMagicMask[sq])))
#define RookAttacks(sq,occ) (*(ROffsetPointer[sq] + _pext_u64(occ,RMagicMask[sq])))
#endif
#define QueenAttacks(sq,occ) (BishopAttacks(sq,occ) | RookAttacks(sq,occ))

#define MatWQ 1
#define MatBQ 3
#define MatWR (3 * 3)
#define MatBR (3 * 3 * 3)
#define MatWL (3 * 3 * 3 * 3)
#define MatBL (3 * 3 * 3 * 3 * 2)
#define MatWD (3 * 3 * 3 * 3 * 2 * 2)
#define MatBD (3 * 3 * 3 * 3 * 2 * 2 * 2)
#define MatWN (3 * 3 * 3 * 3 * 2 * 2 * 2 * 2)
#define MatBN (3 * 3 * 3 * 3 * 2 * 2 * 2 * 2 * 3)
#define MatWP (3 * 3 * 3 * 3 * 2 * 2 * 2 * 2 * 3 * 3)
#define MatBP (3 * 3 * 3 * 3 * 2 * 2 * 2 * 2 * 3 * 3 * 9)
#define TotalMat ((2*(MatWQ+MatBQ)+MatWL+MatBL+MatWD+MatBD+2*(MatWR+MatBR+MatWN+MatBN)+8*(MatWP+MatBP)) + 1)
#define FlagUnusualMaterial (1 << 30)

const int MatCode[16] = {0,0,MatWP,MatBP,MatWN,MatBN,MatWL,MatBL,MatWD,MatBD,MatWR,MatBR,MatWQ,MatBQ,0,0};
const uint64 File[8] = {FileA,FileA<<1,FileA<<2,FileA<<3,FileA<<4,FileA<<5,FileA<<6,FileA<<7};
const uint64 Line[8] = {Line0,(Line0<<8),(Line0<<16),(Line0<<24),(Line0<<32),(Line0<<40),(Line0<<48),(Line0<<56)};

#define opp (1 ^ (me))

#define IPawn(me) (WhitePawn | (me))
#define IKnight(me) (WhiteKnight | (me))
#define ILight(me) (WhiteLight | (me))
#define IDark(me) (WhiteDark | (me))
#define IRook(me) (WhiteRook | (me))
#define IQueen(me) (WhiteQueen | (me))
#define IKing(me) (WhiteKing | (me))

#define BB(i) Board->bb[i]
#define Pawn(me) (BB(WhitePawn | (me)))
#define Knight(me) (BB(WhiteKnight | (me)))
#define Bishop(me) (BB(WhiteLight | (me)) | BB(WhiteDark | (me)))
#define Rook(me) (BB(WhiteRook | (me)))
#define Queen(me) (BB(WhiteQueen | (me)))
#define King(me) (BB(WhiteKing | (me)))
#define Piece(me) (BB(me))
#define NonPawn(me) (Piece(me) ^ Pawn(me))
#define NonPawnKing(me) (NonPawn(me) ^ King(me))
#define BSlider(me) (Bishop(me) | Queen(me))
#define RSlider(me) (Rook(me) | Queen(me))
#define Major(me) RSlider(me)
#define Minor(me) (Knight(me) | Bishop(me))
#define Slider(me) (BSlider(me) | RSlider(me))
#define PieceAll (Piece(White) | Piece(Black))
#define SliderAll (Slider(White) | Slider(Black))
#define PawnAll (Pawn(White) | Pawn(Black))
#define NonPawnKingAll (NonPawnKing(White) | NonPawnKing(Black))
#define KingPos(me) (lsb(King(me)))

#define ShiftNW(target) (((target) & (~(File[0] | Line[7]))) << 7)
#define ShiftNE(target) (((target) & (~(File[7] | Line[7]))) << 9)
#define ShiftSE(target) (((target) & (~(File[7] | Line[0]))) >> 7)
#define ShiftSW(target) (((target) & (~(File[0] | Line[0]))) >> 9)
#define ShiftW(me,target) ((me) ? ShiftSW(target) : ShiftNW(target))
#define ShiftE(me,target) ((me) ? ShiftSE(target) : ShiftNE(target))
#define ShiftN(target) ((target) << 8)
#define ShiftS(target) ((target) >> 8)
#define Shift(me,target) ((me) ? ShiftS(target) : ShiftN(target))
#define PushW(me) ((me) ? (-9) : (7))
#define PushE(me) ((me) ? (-7) : (9))
#define Push(me) ((me) ? (-8) : (8))
#define Dir(me) ((me) ? (-1) : (1))
#define IsGreater(me,x,y) ((me) ? ((x) < (y)) : ((x) > (y)))

#define Line(me,n) ((me) ? Line[7 - n] : Line[n])
#define Square(sq) Board->square[sq]
#define AddMove(from,to,flags,score) { *list = ((from) << 6) | (to) | (flags) | (score); list++; }
#define AddCapture(from,to,flags) AddMove(from,to,flags,MvvLva[Square(from)][Square(to)])
#define AddCaptureP(piece,from,to,flags) AddMove(from,to,flags,MvvLva[piece][Square(to)])
#define AddHistoryP(piece,from,to,flags) AddMove(from,to,flags,HistoryP(piece,from,to))
#define AddHistory(from,to) AddMove(from,to,0,History(from,to))
#define AddDeltaP(piece,from,to,flags) AddMove(from,to,flags,Convert(DeltaScore(piece,from,to)+(sint16)0x4000,int) << 16)
#define AddDelta(from,to) AddMove(from,to,0,Convert(Delta(from,to)+(sint16)0x4000,int) << 16)
#define AddCDeltaP(piece,from,to,flags) {if (DeltaScore(piece,from,to) >= Current->margin) AddMove(from,to,flags,Convert(DeltaScore(piece,from,to)+(sint16)0x4000,int) << 16)}
#define AddCDelta(from,to) {if (Delta(from,to) >= Current->margin) AddMove(from,to,0,Convert(Delta(from,to)+(sint16)0x4000,int) << 16)}

#define Check(me) T(Current->att[(me) ^ 1] & King(me))
#define IsIllegal(me,move) ((T(Current->xray[opp] & Bit(From(move))) && F(Bit(To(move)) & FullLine[lsb(King(me))][From(move)])) \
	|| (IsEP(move) && T(Line[Rank(From(move))] & King(me)) && T(Line[Rank(From(move))] & Major(opp)) && \
	T(RookAttacks(lsb(King(me)),PieceAll ^ Bit(From(move)) ^ Bit(Current->ep_square - Push(me))) & Major(opp))))
#define IsRepetition(margin,move) ((margin) > 0 && Current->ply >= 2 && (Current-1)->move == ((To(move) << 6) | From(move)) && F(Square(To(move))) && F((move) & 0xF000))

#ifdef EXPLAIN_EVAL
FILE * fexplain;
int explain = 0, cpp_length;
char GullCppFile[16384][256];
#define IncV(var,x) (explain ? (me ? ((var -= (x)) && fprintf(fexplain,"(%d, %d): %s [Black]: %s",Opening(-(x)),Endgame(-(x)),__FUNCTION__,GullCppFile[Min(__LINE__-1,cpp_length)])) : ((var += (x)) && fprintf(fexplain,"(%d, %d): %s [White]: %s",Opening(x),Endgame(x),__FUNCTION__,GullCppFile[Min(__LINE__-1,cpp_length)]))) : (me ? (var -= (x)) : (var += (x))))
#else
#define IncV(var,x) (me ? (var -= (x)) : (var += (x)))
#endif
#define DecV(var,x) IncV(var,-(x))

#define KpkValue 300
#define EvalValue 30000
#define MateValue 32760

/* 
general move:
0 - 11: from & to
12 - 15: flags
16 - 23: history
24 - 25: spectial moves: killers, refutations...
26 - 30: MvvLva
delta move:
0 - 11: from & to
12 - 15: flags
16 - 31: sint16 delta + (sint16)0x4000
*/
const int MvvLvaVictim[16] = {0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 2, 2, 3, 3, 3, 3}; 
const int MvvLvaAttacker[16] = {0, 0, 5, 5, 4, 4, 3, 3, 3, 3, 2, 2, 1, 1, 6, 6};
const int MvvLvaAttackerKB[16] = {0, 0, 9, 9, 7, 7, 5, 5, 5, 5, 3, 3, 1, 1, 11, 11};
#define PawnCaptureMvvLva(attacker) (MvvLvaAttacker[attacker])
#define MaxPawnCaptureMvvLva (MvvLvaAttacker[15]) // 6
#define KnightCaptureMvvLva(attacker) (MaxPawnCaptureMvvLva + MvvLvaAttackerKB[attacker]) 
#define MaxKnightCaptureMvvLva (MaxPawnCaptureMvvLva + MvvLvaAttackerKB[15]) // 17
#define BishopCaptureMvvLva(attacker) (MaxPawnCaptureMvvLva + MvvLvaAttackerKB[attacker] + 1)
#define MaxBishopCaptureMvvLva (MaxPawnCaptureMvvLva + MvvLvaAttackerKB[15] + 1) // 18
#define RookCaptureMvvLva(attacker) (MaxBishopCaptureMvvLva + MvvLvaAttacker[attacker])
#define MaxRookCaptureMvvLva (MaxBishopCaptureMvvLva + MvvLvaAttacker[15]) // 24
#define QueenCaptureMvvLva(attacker) (MaxRookCaptureMvvLva + MvvLvaAttacker[attacker])

#define MvvLvaPromotion (MvvLva[WhiteQueen][BlackQueen])
#define MvvLvaPromotionKnight (MvvLva[WhiteKnight][BlackKnight])
#define MvvLvaPromotionCap(capture) (MvvLva[((capture) < WhiteRook) ? WhiteRook : ((capture) >= WhiteQueen ? WhiteKing : WhiteKnight)][BlackQueen])
#define MvvLvaPromotionKnightCap(capture) (MvvLva[WhiteKing][capture])
#define MvvLvaXray (MvvLva[WhiteQueen][WhitePawn])
#define MvvLvaXrayCap(capture) (MvvLva[WhiteKing][capture])
#define RefOneScore ((0xFF << 16) | (3 << 24))
#define RefTwoScore ((0xFF << 16) | (2 << 24))
#define KillerOneScore ((0xFF << 16) | (1 << 24))
#define KillerTwoScore (0xFF << 16)

#define halt_check if ((Current - Data) >= 126) {evaluate(); return Current->score;} \
    if (Current->ply >= 100) return 0; \
	for (i = 4; i <= Current->ply; i+= 2) if (Stack[sp-i] == Current->key) return 0
#define ExtFlag(ext) ((ext) << 16)
#define Ext(flags) (((flags) >> 16) & 0xF)
#define FlagHashCheck (1 << 20) // first 20 bits are reserved for the hash killer and extension
#define FlagHaltCheck (1 << 21)
#define FlagCallEvaluation (1 << 22)
#define FlagDisableNull (1 << 23)
#define FlagNeatSearch (FlagHashCheck | FlagHaltCheck | FlagCallEvaluation)
#define FlagNoKillerUpdate (1 << 24)
#define FlagReturnBestMove (1 << 25)

#define MSBZ(x) ((x) ? msb(x) : 63)
#define LSBZ(x) ((x) ? lsb(x) : 0)
#define NB(me, x) ((me) ? msb(x) : lsb(x))
#define NBZ(me, x) ((me) ? MSBZ(x) : LSBZ(x))

typedef struct {
	uint64 bb[16];
	uint8 square[64];
} GBoard;
__declspec(align(64)) GBoard Board[1];
uint64 Stack[2048];
int sp, save_sp;
uint64 nodes, check_node, check_node_smp;
GBoard SaveBoard[1];

typedef struct {
	uint64 key, pawn_key;
	uint16 move;
	uint8 turn, castle_flags, ply, ep_square, piece, capture;
	uint8 square[64];
	int pst, material;
} GPosData;
typedef struct {
	uint64 key, pawn_key, eval_key, att[2], patt[2], passer, xray[2], pin[2], threat, mask;
	uint8 turn, castle_flags, ply, ep_square, capture, gen_flags, piece, stage, mul, dummy;
	sint16 score;
	uint16 move, killer[3], ref[2];
	int best;
	int material, pst;
	int margin, *start, *current;
	int moves[230];
} GData;
__declspec(align(64)) GData Data[128];
GData *Current = Data;
#define FlagSort (1 << 0)
#define FlagNoBcSort (1 << 1)
GData SaveData[1];

enum {
	stage_search, s_hash_move, s_good_cap, s_special, s_quiet, s_bad_cap, s_none,
	stage_evasion, e_hash_move, e_ev, e_none, 
	stage_razoring, r_hash_move, r_cap, r_checks, r_none
};
#define StageNone ((1 << s_none) | (1 << e_none) | (1 << r_none))

typedef struct {
    uint32 key;
	uint16 date;
	uint16 move;
	sint16 low;
	sint16 high;
	uint16 flags;
	uint8 low_depth;
	uint8 high_depth;
} GEntry;
#ifndef TUNER
#define initial_hash_size (1024 * 1024)
#else
#define initial_hash_size (64 * 1024)
GEntry HashOne[initial_hash_size];
GEntry HashTwo[initial_hash_size];
#endif
sint64 hash_size = initial_hash_size;
uint64 hash_mask = (initial_hash_size - 4);
GEntry * Hash;

typedef struct {
	uint64 key;
	sint16 shelter[2];
	uint8 passer[2], draw[2];
	int score;
} GPawnEntry;
#ifndef TUNER
#define pawn_hash_size (1024 * 1024)
__declspec(align(64)) GPawnEntry PawnHash[pawn_hash_size];
#else
#define pawn_hash_size (32 * 1024)
__declspec(align(64)) GPawnEntry PawnHashOne[pawn_hash_size];
__declspec(align(64)) GPawnEntry PawnHashTwo[pawn_hash_size];
GPawnEntry * PawnHash = PawnHashOne;
#endif
#define pawn_hash_mask (pawn_hash_size - 1)

typedef struct {
	uint32 key;
	uint16 date;
	uint16 move;
	sint16 value;
	sint16 exclusion;
	uint8 depth;
	uint8 ex_depth;
	int knodes;
	int ply;
} GPVEntry;
#ifndef TUNER
#define pv_hash_size (1024 * 1024)
#else
#define pv_hash_size (16 * 1024)
GPVEntry PVHashOne[pv_hash_size];
GPVEntry PVHashTwo[pv_hash_size];
#endif
#define pv_cluster_size 4
#define pv_hash_mask (pv_hash_size - pv_cluster_size)
GPVEntry * PVHash = NULL;

int RootList[256];

#define prefetch(a,mode) _mm_prefetch(a,mode)

uint64 Forward[2][8];
uint64 West[8];
uint64 East[8];
uint64 PIsolated[8];
uint64 HLine[64];
uint64 VLine[64];
uint64 NDiag[64];
uint64 SDiag[64];
uint64 RMask[64];
uint64 BMask[64];
uint64 QMask[64];
uint64 BMagicMask[64];
uint64 RMagicMask[64];
uint64 NAtt[64];
uint64 SArea[64];
uint64 DArea[64];
uint64 NArea[64];
uint64 BishopForward[2][64];
uint64 PAtt[2][64];
uint64 PMove[2][64];
uint64 PWay[2][64];
uint64 PSupport[2][64];
uint64 Between[64][64];
uint64 FullLine[64][64];

#define magic_size 107648
uint64 * MagicAttacks;
typedef struct {
	sint16 score;
	uint8 phase, flags;
	uint8 mul[2], pieces[2];
#ifdef TUNER
	uint32 generation;
#endif
} GMaterial;
GMaterial * Material;
#define FlagSingleBishop_w (1 << 0)
#define FlagSingleBishop_b (1 << 1)
#define FlagCallEvalEndgame_w (1 << 2)
#define FlagCallEvalEndgame_b (1 << 3)

#ifndef TUNER
int Pst[16 * 64];
#else
int PstOne[16 * 64];
int PstTwo[16 * 64];
int * Pst = PstOne;
#endif
#define Pst(piece,sq) Pst[((piece) << 6) | (sq)]
int MvvLva[16][16]; // [piece][capture]
uint64 TurnKey;
uint64 PieceKey[16][64];
uint64 CastleKey[16];
uint64 EPKey[8];
uint16 date;

uint64 Kpk[2][64][64]; 

#ifndef TUNER
sint16 History[16 * 64]; 
#else
sint16 HistoryOne[16 * 64]; 
sint16 HistoryTwo[16 * 64]; 
sint16 * History = HistoryOne;
#endif
#define HistoryScore(piece,from,to) History[((piece) << 6) | (to)]
#define HistoryP(piece,from,to) ((Convert(HistoryScore(piece,from,to) & 0xFF00,int)/Convert(HistoryScore(piece,from,to) & 0x00FF,int)) << 16)
#define History(from,to) HistoryP(Square(from),from,to)
#define HistoryM(move) HistoryScore(Square(From(move)),From(move),To(move))
#define HistoryInc(depth) Min(((depth) >> 1) * ((depth) >> 1), 64)
#define HistoryGood(move) if ((HistoryM(move) & 0x00FF) >= 256 - HistoryInc(depth)) \
	HistoryM(move) = ((HistoryM(move) & 0xFEFE) >> 1) + ((HistoryInc(depth) << 8) | HistoryInc(depth)); \
	else HistoryM(move) += ((HistoryInc(depth) << 8) | HistoryInc(depth))
#define HistoryBad(move) if ((HistoryM(move) & 0x00FF) >= 256 - HistoryInc(depth)) \
	HistoryM(move) = ((HistoryM(move) & 0xFEFE) >> 1) + HistoryInc(depth); else HistoryM(move) += HistoryInc(depth)

#ifndef TUNER
sint16 Delta[16 * 4096];
#else
sint16 DeltaOne[16 * 4096];
sint16 DeltaTwo[16 * 4096];
sint16 * Delta = DeltaOne;
#endif
#define DeltaScore(piece,from,to) Delta[((piece) << 12) | ((from) << 6) | (to)]
#define Delta(from,to) DeltaScore(Square(from),from,to)
#define DeltaM(move) Delta(From(move),To(move))
#define UpdateDelta if (F(Current->capture) && T(Current->move) && F(Current->move & 0xE000) && Current > Data) { \
	if (DeltaScore(Current->piece,From(Current->move),To(Current->move)) <= -Current->score - ((Current - 1)->score)) \
	DeltaScore(Current->piece,From(Current->move),To(Current->move)) = -Current->score - ((Current - 1)->score); \
	else DeltaScore(Current->piece,From(Current->move),To(Current->move))--; }
#define DeltaMarginP(piece,from,to) (DeltaScore(piece,from,to) >= Current->margin)
#define DeltaMargin(from,to) (Delta(from,to) >= Current->margin)

typedef struct {
	uint16 ref[2];
	uint16 check_ref[2];
} GRef;
#ifndef TUNER
GRef Ref[16 * 64];
#else 
GRef RefOne[16 * 64];
GRef RefTwo[16 * 64];
GRef * Ref = RefOne;
#endif
#define RefPointer(piece,from,to) Ref[((piece) << 6) | (to)]
#define RefM(move) RefPointer(Square(To(move)),From(move),To(move))
#define UpdateRef(ref_move) if (T(Current->move) && RefM(Current->move).ref[0] != (ref_move)) { \
	RefM(Current->move).ref[1] = RefM(Current->move).ref[0]; RefM(Current->move).ref[0] = (ref_move); }
#define UpdateCheckRef(ref_move) if (T(Current->move) && RefM(Current->move).check_ref[0] != (ref_move)) { \
	RefM(Current->move).check_ref[1] = RefM(Current->move).check_ref[0]; RefM(Current->move).check_ref[0] = (ref_move); }

uint64 seed = 1;
uint8 PieceFromChar[256];
uint16 PV[128];
char info_string[1024];
char pv_string[1024];
char score_string[16];
char mstring[65536];
int MultiPV[256];
int pvp;
int pv_length;
int best_move, best_score;
int TimeLimit1, TimeLimit2, Console, HardwarePopCnt;
int DepthLimit, LastDepth, LastTime, LastValue, LastExactValue, PrevMove, InstCnt;
sint64 LastSpeed;
int PVN, Stop, Print, Input = 1, PVHashing = 1, Infinite, MoveTime, SearchMoves, SMPointer, Ponder, Searching, Previous;
typedef struct {
	int Bad, Change, Singular, Early, FailLow, FailHigh;
} GSearchInfo;
GSearchInfo CurrentSI[1], BaseSI[1];
#ifdef CPU_TIMING
int CpuTiming = 0, UciMaxDepth = 0, UciMaxKNodes = 0, UciBaseTime = 1000, UciIncTime = 5;
int GlobalTime[2] = { 0, 0 };
int GlobalInc[2] = { 0, 0 };
int GlobalTurn = 0;
#define CyclesPerMSec Convert(3400000, sint64)
#endif
int Aspiration = 1, LargePages = 1;
#define TimeSingTwoMargin 20
#define TimeSingOneMargin 30
#define TimeNoPVSCOMargin 60
#define TimeNoChangeMargin 70
#define TimeRatio 120
#define PonderRatio 120
#define MovesTg 30
#define InfoLag 5000
#define InfoDelay 1000
sint64 StartTime, InfoTime, CurrTime;
uint16 SMoves[256];

jmp_buf Jump, ResetJump;
HANDLE StreamHandle; 

#define ExclSingle(depth) 8
#define ExclDouble(depth) 16
#define ExclSinglePV(depth) 8
#define ExclDoublePV(depth) 16

// EVAL

const sint8 DistC[8] = {3, 2, 1, 0, 0, 1, 2, 3};
const sint8 RankR[8] = {-3, -2, -1, 0, 1, 2, 3, 4};

const int SeeValue[16] = {0, 0, 90, 90, 325, 325, 325, 325, 325, 325, 510, 510, 975, 975, 30000, 30000};
const int PieceType[16] = {0, 0, 0, 0, 1, 1, 2, 2, 2, 2, 3, 3, 4, 4, 5, 5};

#ifndef TUNER
#define V(x) (x)
#else
#define MaxVariables 1024
int var_number, active_vars;
typedef struct {
	char line[256];
} GString;
GString SourceFile[1000], VarName[1000];
int VarIndex[1000];
int src_str_num = 0, var_name_num = 0;
int Variables[MaxVariables];
uint8 Active[MaxVariables];
double Var[MaxVariables], Base[MaxVariables], FE[MaxVariables], SE[MaxVariables], Grad[MaxVariables];
#define V(x) Variables[x]
double EvalOne[MaxVariables], EvalTwo[MaxVariables];
int RecordGames = 0;
char RecordString[65536], PosStr[256], *Buffer;
FILE * frec;
#endif
#define GullCpp "C:/Users/Administrator/Documents/Visual Studio 2010/Projects/Gull/Gull/Gull.cpp"

#define ArrayIndex(width,row,column) (((row) * (width)) + (column))
#ifndef TUNER
#define Av(x,width,row,column) (*((x) + ArrayIndex(width,row,column)))
#else
#define Av(x,width,row,column) V((I##x) + ArrayIndex(width,row,column))
#endif
#define TrAv(x,w,r,c) Av(x,0,0,(((r)*(2*(w)-(r)+1))/2)+(c))

#define Sa(x,y) Av(x,0,0,y)
#define Ca(x,y) Compose(Av(x,0,0,((y) * 2)),Av(x,0,0,((y) * 2)+1))

// EVAL WEIGHTS

// tuner: start
enum { // tuner: enum
	IMatLinear,
	IMatQuadMe = IMatLinear + 5,
	IMatQuadOpp = IMatQuadMe + 14,
	IBishopPairQuad = IMatQuadOpp + 10,
	IMatSpecial = IBishopPairQuad + 9,
	IPstQuadWeights = IMatSpecial + 20,
	IPstLinearWeights = IPstQuadWeights + 48,
	IPstQuadMixedWeights = IPstLinearWeights + 48,
	IMobilityLinear = IPstQuadMixedWeights + 24,
	IMobilityLog = IMobilityLinear + 8,
	IShelterValue = IMobilityLog + 8,
	IStormQuad = IShelterValue + 15,
	IStormLinear = IStormQuad + 5,
	IStormHof = IStormLinear + 5,
	IPasserQuad = IStormHof + 2,
	IPasserLinear = IPasserQuad + 18,
	IPasserAttDefQuad = IPasserLinear + 18,
	IPasserAttDefLinear = IPasserAttDefQuad + 4,
	IPasserSpecial = IPasserAttDefLinear + 4,
	IIsolated = IPasserSpecial + 4,
	IUnprotected = IIsolated + 10,
	IBackward = IUnprotected + 6,
	IDoubled = IBackward + 4,
	IRookSpecial = IDoubled + 4,
	ITactical = IRookSpecial + 20,
	IKingDefence = ITactical + 12,
	IPawnSpecial = IKingDefence + 8,
	IBishopSpecial = IPawnSpecial + 8,
	IKnightSpecial = IBishopSpecial + 4,
	IPin = IKnightSpecial + 10,
	IKingRay = IPin + 10,
	IKingAttackWeight = IKingRay + 6
};

const int Phase[5] = {
	0, 325, 325, 510, 975
};
#define MaxPhase (16 * Phase[0] + 4 * Phase[1] + 4 * Phase[2] + 4 * Phase[3] + 2 * Phase[4])
#define PhaseMin (2 * Phase[3] + Phase[1] + Phase[2])
#define PhaseMax (MaxPhase - Phase[1] - Phase[2])

const int MatLinear[5] = { // tuner: type=array, var=50, active=0
	3, 0, 3, 19, 0
};
// pawn, knight, bishop, rook, queen
const int MatQuadMe[14] = { // tuner: type=array, var=1000, active=0
	-33, 17, -23, -155, -247,
	15, 296, -105, -83,
	-162, 327, 315,
	-861, -1013
};
const int MatQuadOpp[10] = { // tuner: type=array, var=1000, active=0
	-14, 47, -20, -278,
	35, 39, 49,
	9, -2,
	75
};
const int BishopPairQuad[9] = { // tuner: type=array, var=1000, active=0
	-38, 164, 99, 246, -84, -57, -184, 88, -186
};

enum { MatRB, MatRN, MatQRR, MatQRB, MatQRN, MatQ3, MatBBR, MatBNR, MatNNR, MatM };
const int MatSpecial[20] = { // tuner: type=array, var=30, active=0
	13, -13, 10, -9, 8, 12, 4, 6, 5, 9, -3, -8, -4, 7, 2, 0, 0, -6, 1, 3
};

// piece type (6) * direction (4: h center dist, v center dist, diag dist, rank) * phase (2)
const int PstQuadWeights[48] = { // tuner: type=array, var=100, active=0
	-15, -19, -70, -13, 33, -20, 0, 197, -36, -122, 0, -60, -8, -3, -17, -28,
	-27, -63, -17, -7, 14, 0, -24, -5, -64, -2, 0, -38, -8, 0, 77, 11,
	-67, 3, -4, -92, -2, 12, -13, -42, -62, -84, -175, -42, -2, -17, 40, -19
};
const int PstLinearWeights[48] = { // tuner: type=array, var=500, active=0
	-107, 67, -115, 83, -55, 67, 92, 443, -177, 5, -82, -61, -106, -104, 273, 130,
	0, -145, -105, -58, -99, -37, -133, 14, -185, -43, -67, -53, 53, -65, 174, 134,
	-129, 7, 98, -231, 107, -40, -27, 311, 256, -117, 813, -181, 2, -215, -44, 344
};
// piece type (6) * type (2: h * v, h * rank) * phase (2)
const int PstQuadMixedWeights[24] = { // tuner: type=array, var=100, active=0
	14, -6, 1, -4, -8, -2, 4, -4,
	1, -7, -12, 0, -2, -1, -5, 4,
	5, -10, 0, 4, -2, 5, 4, -2
};
// piece type (4) * phase (2)
const int MobilityLinear[8] = { // tuner: type=array, var=300, active=0
	328, 171, 311, 102, 284, 164, 155, 288
};
const int MobilityLog[8] = { // tuner: type=array, var=500, active=0
	485, -21, 388, 389, -168, 313, 438, -276
};
int Mobility[4][32];

// file type (3) * distance from 2d rank/open (5)
const int ShelterValue[15] = {  // tuner: type=array, var=10, active=0
	2, 9, 11, 0, 0, 12, 18, 11, 0, 2, 24, 7, 8, 0, 0
};
sint16 Shelter[3][8];

enum { StormBlockedMul, StormShelterAttMul, StormConnectedMul, StormOpenMul, StormFreeMul };
const int StormQuad[5] = { // tuner: type=array, var=250, active=0
	126, 328, 463, 215, 89
};
const int StormLinear[5] = { // tuner: type=array, var=500, active=0
	83, 156, 438, 321, 12
};
enum { StormHofValue, StormOfValue };
const int StormHof[2] = { // tuner: type=array, var=20, active=1
	0, 22
};
sint16 StormBlocked[4];
sint16 StormShelterAtt[4];
sint16 StormConnected[4];
sint16 StormOpen[4];
sint16 StormFree[4];

// type (7: general, blocked, free, supported, protected, connected, outside, candidate, clear) * phase (2)
const int PasserQuad[18] = { // tuner: type=array, var=50, active=0
	19, 13, 21, 3, -24, 126, 0, 65, 32, 56, 27, -5, 32, -16, 13, 4, 1, 1
};
const int PasserLinear[18] = { // tuner: type=array, var=200, active=0
	41, 2, 111, 86, 178, 113, 202, 15, -61, 21, 93, 166, 86, 92, 27, 34, -18, -7
};
// type (2: att, def) * scaling (2: linear, log) 
const int PasserAttDefQuad[4] = { // tuner: type=array, var=500, active=0
	191, 51, 83, 19
};
const int PasserAttDefLinear[4] = { // tuner: type=array, var=500, active=0
	634, 4, 233, 66
};
enum { PasserOnePiece, PasserOpKingControl, PasserOpMinorControl, PasserOpRookBlock };
const int PasserSpecial[4] = { // tuner: type=array, var=100, active=0
	0, 0, 0, 13
};

uint8 LogDist[16];
int PasserGeneral[8];
int PasserBlocked[8];
int PasserFree[8];
int PasserSupported[8];
int PasserProtected[8];
int PasserConnected[8];
int PasserOutside[8];
int PasserCandidate[8];
int PasserClear[8];
sint16 PasserAtt[8];
sint16 PasserDef[8];
sint16 PasserAttLog[8];
sint16 PasserDefLog[8];

enum { IsolatedOpen, IsolatedClosed, IsolatedBlocked, IsolatedDoubledOpen, IsolatedDoubledClosed };
const int Isolated[10] = { // tuner: type=array, var=10, active=0
	6, 6, 8, 2, -8, 0, -1, 10, 7, 9
};
enum { UpBlocked, PasserTarget, ChainRoot };
const int Unprotected[6] = { // tuner: type=array, var=10, active=0
	4, 5, -5, -1, 9, -1
};
enum { BackwardOpen, BackwardClosed };
const int Backward[4] = { // tuner: type=array, var=10, active=0
	17, 10, 4, 1
};
enum { DoubledOpen, DoubledClosed };
const int Doubled[4] = { // tuner: type=array, var=10, active=0
	3, 0, 1, 0
};

enum { RookHof, RookHofWeakPAtt, RookOf, RookOfOpen, RookOfMinorFixed, RookOfMinorHaging, RookOfKingAtt, Rook7th, Rook7thK8th, Rook7thDoubled };
const int RookSpecial[20] = { // tuner: type=array, var=10, active=0
	8, 0, 2, 0, 11, 8, -1, 2, -1, -1, 14, -1, 5, -5, -5, 0, -6, 8, -7, 31
};

enum { TacticalMajorPawn, TacticalMinorPawn, TacticalMajorMinor, TacticalMinorMinor, TacticalThreat, TacticalDoubleThreat };
const int Tactical[12] = { // tuner: type=array, var=20, active=0
	-1, 5, 0, 5, 11, 29, 23, 32, 19, 11, 41, 12
};

enum { KingDefKnight, KingDefBishop, KingDefRook, KingDefQueen };
const int KingDefence[8] = { // tuner: type=array, var=5, active=0
	2, 0, 0, 1, 0, 0, 4, 0
};

enum { PawnChainLinear, PawnChain, PawnBlocked, PawnFileSpan };
const int PawnSpecial[8] = { // tuner: type=array, var=10, active=0
	11, 9, 9, 4, 0, 9, 1, 1
};

enum { BishopNonForwardPawn, BishopPawnBlock };
const int BishopSpecial[4] = { // tuner: type=array, var=5, active=0
	0, 0, 0, 3
};

const uint64 Outpost[2] = { Convert(0x00007E7E3C000000, uint64), Convert(0x0000003C7E7E0000, uint64) };
enum { KnightOutpost, KnightOutpostProtected, KnightOutpostPawnAtt, KnightOutpostBishopAtt, KnightOutpostKingAtt };
const int KnightSpecial[10] = { // tuner: type=array, var=10, active=0
	11, 7, 23, 0, 13, 6, 1, 5, 26, 6
};

enum { WeakPin, StrongPin, ThreatPin, SelfPawnPin, SelfPiecePin };
const int Pin[10] = { // tuner: type=array, var=20, active=0
	21, 39, 6, 80, 45, 29, 8, 9, 48, 27
};

enum { QKingRay, RKingRay, BKingRay };
const int KingRay[6] = { // tuner: type=array, var=20, active=0
	4, 8, -4, 11, 11, -3
};

const int KingAttackWeight[7] = { // tuner: type=array, var=20, active=0
	17, 14, 22, 45, 48, 64, 64
};
#define KingNAttack Compose(1, Av(KingAttackWeight, 0, 0, 0))
#define KingBAttack Compose(1, Av(KingAttackWeight, 0, 0, 1))
#define KingRAttack Compose(1, Av(KingAttackWeight, 0, 0, 2))
#define KingQAttack Compose(1, Av(KingAttackWeight, 0, 0, 3))
#define KingAttack Compose(1, 0)
#define KingAttackSquare Av(KingAttackWeight, 0, 0, 4)
#define KingNoMoves Av(KingAttackWeight, 0, 0, 5)
#define KingShelterQuad Av(KingAttackWeight, 0, 0, 6)

const int KingAttackScale[16] = { 0, 1, 4, 9, 16, 25, 36, 49, 64, 64, 64, 64, 64, 64, 64, 64 };
// tuner: stop

// END EVAL WEIGHTS

// SMP

#define MaxPrN 1
#ifndef DEBUG
#ifndef TUNER
#undef MaxPrN
#ifndef W32_BUILD
#define MaxPrN 64 // mustn't exceed 64
#else
#define MaxPrN 32 // mustn't exceed 32
#endif
#endif
#endif

int PrN = 1, CPUs = 1, HT = 0, parent = 1, child = 0, WinParId, Id = 0, ResetHash = 1, NewPrN = 0;
HANDLE ChildPr[MaxPrN];
#define SplitDepth 10
#define SplitDepthPV 4
#define MaxSplitPoints 64 // mustn't exceed 64

typedef struct {
	GPosData Position[1];
	uint64 stack[100];
	uint16 killer[16][2];
	int sp, date;
} GPos;

#define FlagClaimed (1 << 1)
#define FlagFinished (1 << 2)

typedef struct {
	volatile uint16 move;
	volatile uint8 reduced_depth, research_depth, stage, ext, id, flags;
} GMove;

typedef struct {
	volatile LONG lock;
	volatile int claimed, active, finished, pv, move_number, current, depth, alpha, beta, singular, split, best_move, height; 
	GMove move[128];
	jmp_buf jump;
	GPos Pos[1];
} GSP;

typedef struct {
	volatile long long nodes, active_sp, searching;
#ifndef W32_BUILD
	volatile long long stop, fail_high;
#else
	volatile long stop, fail_high;
#endif
	volatile sint64 hash_size;
	volatile int PrN;
	GSP Sp[MaxSplitPoints];
} GSMPI;

#define SharedMaterialOffset (sizeof(GSMPI))
#define SharedMagicOffset (SharedMaterialOffset + TotalMat * sizeof(GMaterial))
#define SharedPVHashOffset (SharedMagicOffset + magic_size * sizeof(uint64))

GSMPI * Smpi;

jmp_buf CheckJump;

HANDLE SHARED = NULL, HASH = NULL;

#ifndef W32_BUILD
#define SET_BIT(var,bit) (InterlockedOr(&(var),1 << (bit)))
#define SET_BIT_64(var,bit) (InterlockedOr64(&(var),Bit(bit)));
#define ZERO_BIT_64(var,bit) (InterlockedAnd64(&(var),~Bit(bit)));
#define TEST_RESET_BIT(var,bit) (InterlockedBitTestAndReset64(&(var),bit))
#define TEST_RESET(var) (InterlockedExchange64(&(var),0))
#else
#define SET_BIT(var,bit) (_InterlockedOr(&(var),1 << (bit)))
#define SET_BIT_64(var,bit) {if ((bit) < 32) _InterlockedOr((LONG*)&(var),1 << (bit)); else _InterlockedOr(((LONG*)(&(var))) + 1,1 << ((bit) - 32));}
#define ZERO_BIT_64(var,bit) {if ((bit) < 32) _InterlockedAnd((LONG*)&(var),~(1 << (bit))); else _InterlockedAnd(((LONG*)(&(var))) + 1,~(1 << ((bit) - 32)));}
#define TEST_RESET_BIT(var,bit) (InterlockedBitTestAndReset(&(var),bit))
#define TEST_RESET(var) (InterlockedExchange(&(var),0))
#endif
#define SET(var,value) (InterlockedExchange(&(var),value))

#define LOCK(lock) {while (InterlockedCompareExchange(&(lock),1,0)) _mm_pause();}
#define UNLOCK(lock) {SET(lock,0);}

// END SMP

__forceinline int lsb(uint64 x);
__forceinline int msb(uint64 x);
__forceinline int popcnt(uint64 x);
__forceinline int MinF(int x, int y);
__forceinline int MaxF(int x, int y);
__forceinline double MinF(double x, double y);
__forceinline double MaxF(double x, double y);
template <bool HPopCnt> __forceinline int popcount(uint64 x);
uint64 BMagicAttacks(int i, uint64 occ);
uint64 RMagicAttacks(int i, uint64 occ);
uint16 rand16();
uint64 random();
void init_pst();
void init_eval();
void init();
void init_search(int clear_hash);
void setup_board();
void get_board(const char fen[]);
void init_hash();
void move_to_string(int move, char string[]);
int move_from_string(char string[]);
void pick_pv();
template <bool me> void do_move(int move);
template <bool me> void undo_move(int move);
void do_null();
void undo_null();
__forceinline void evaluate();
template <bool me> int is_legal(int move);
template <bool me> int is_check(int move);
void hash_high(int value, int depth);
void hash_low(int move, int value, int depth);
void hash_exact(int move, int value, int depth, int exclusion, int ex_depth, int knodes);
__forceinline int pick_move();
template <bool me, bool root> int get_move();
template <bool me> int see(int move, int margin);
template <bool me> void gen_root_moves();
template <bool me> int * gen_captures(int * list);
template <bool me> int * gen_evasions(int * list);
void mark_evasions(int * list);
template <bool me> int * gen_quiet_moves(int * list);
template <bool me> int * gen_checks(int * list);
template <bool me> int * gen_delta_moves(int * list);
template <bool me, bool pv> int q_search(int alpha, int beta, int depth, int flags);
template <bool me, bool pv> int q_evasion(int alpha, int beta, int depth, int flags);
template <bool me, bool exclusion> int search(int beta, int depth, int flags);
template <bool me, bool exclusion> int search_evasion(int beta, int depth, int flags);
template <bool me, bool root> int pv_search(int alpha, int beta, int depth, int flags);
template <bool me> void root();
template <bool me> int multipv(int depth);
void send_pv(int depth, int alpha, int beta, int score);
void send_multipv(int depth, int curr_number);
void send_best_move();
void get_position(char string[]);
void get_time_limit(char string[]);
sint64 get_time();
int time_to_stop(GSearchInfo * SI, int time, int searching);
void check_time(int searching);
void check_time(int time, int searching);
int input();
void uci();

#ifdef TUNER
#ifndef RECORD_GAMES
int ResignThreshold = 150;
#else
int ResignThreshold = 1500;
#endif

typedef struct {
	int wins, draws, losses;
} GMatchInfo;
GMatchInfo MatchInfo[1] = {(0, 0, 0)};

char Fen[65536][128];
int opening_positions = 0;

int Client = 0, Server = 0, Local = 1, cmd_number = 0;
int generation = 0;

#ifdef PGN
typedef struct {
	uint64 bb[6]; // white, black, pawns, minor, major, queens and knights
	uint8 ep_square, turn, ply, castle_flags;
} GPos;
GPos Pos[65536];
int pgn_positions = 0;

void position_to_pos(GPos * pos) {
	pos->bb[0] = Piece(White);
	pos->bb[1] = Piece(Black);
	pos->bb[2] = PawnAll;
	pos->bb[3] = Minor(White) | Minor(Black);
	pos->bb[4] = Major(White) | Major(Black);
	pos->bb[5] = Queen(White) | Queen(Black) | Knight(White) | Knight(Black);
	pos->ep_square = Current->ep_square;
	pos->turn = Current->turn;
	pos->castle_flags = Current->castle_flags;
}

void position_from_pos(GPos * pos) {
	Current = Data;
	memset(Board, 0, sizeof(GBoard));
	memset(Current, 0, sizeof(GData));
	BB(White) = pos->bb[0];
	BB(Black) = pos->bb[1];
	for (int me = 0; me < 2; me++) {
		BB(me) = pos->bb[me];
		BB(IPawn(me)) = pos->bb[2] & BB(me);
		BB(IKnight(me)) = pos->bb[3] & pos->bb[5] & BB(me);
		BB(ILight(me)) = pos->bb[3] & (~pos->bb[5]) & LightArea & BB(me);
		BB(IDark(me)) = pos->bb[3] & (~pos->bb[5]) & DarkArea & BB(me);
		BB(IRook(me)) = pos->bb[4] & (~pos->bb[5]) & BB(me);
		BB(IQueen(me)) = pos->bb[4] & pos->bb[5] & BB(me);
		BB(IKing(me)) = BB(me) & (~(pos->bb[2] | pos->bb[3] | pos->bb[4]));
	}
	for (int i = 2; i < 16; i++) for (uint64 u = BB(i); u; Cut(u)) Square(lsb(u)) = i;
	Current->ep_square = pos->ep_square;
	Current->ply = pos->ply;
	Current->castle_flags = pos->castle_flags;
	Current->turn = pos->turn;
	setup_board();
}

int pos_shelter_tune() {
	if (!Queen(White) || !Queen(Black)) return 0;
	if (popcnt(NonPawnKingAll) < 10) return 0;
	if (Current->castle_flags) return 0;
	if (File(lsb(King(White))) <= 2 && File(lsb(King(Black))) >= 5) return 1;
	if (File(lsb(King(White))) >= 5 && File(lsb(King(Black))) <= 2) return 1;
	return 0;
}
int pos_passer_tune() {
	if (Current->passer) return 1;
	return 0;
}
#endif

void init_openings() {
	FILE * ffen = NULL;
	ffen = fopen("8moves.epd","r");
	if (ffen != NULL) {
		for (int i = 0; i < 65536; i++) {
			fgets(Fen[i],128,ffen);
			if (feof(ffen)) {
				opening_positions = Max(opening_positions - 1, 0);
				break;
			} else opening_positions++;
		}
	} else {
		fprintf(stdout,"File '8moves.epd' not found\n");
		exit(0);
		goto no_fen;
	}
	fclose(ffen);
no_fen:
	if (opening_positions == 0) {
		sprintf(Fen[0],"rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1\n");
		opening_positions = 1;
	}
#ifdef PGN
	FILE * fpgn = fopen("uci_games.pgn", "r");
	if (fpgn == NULL) {
		fprintf(stdout, "File 'uci_games.pgn' not found\n");
		exit(0);
	}
	while (pgn_positions < 65536) {
		fgets(mstring, 65536, fpgn);
		if (feof(fpgn)) {
			pgn_positions = Max(pgn_positions - 1, 0);
			break;
		}
		if (strstr(mstring, "FEN")) get_board(mstring + 6);
		if (strchr(mstring, '[')) continue;
		if (strlen(mstring) < 100) continue;
		char * ptr = mstring;
		while (*ptr != 0) {
			evaluate();
			if (pos_passer_tune()) {
				position_to_pos(&Pos[pgn_positions++]);
				break;
			}
			pv_string[0] = *ptr++;
			pv_string[1] = *ptr++;
			pv_string[2] = *ptr++;
			pv_string[3] = *ptr++;
			if (*ptr == 0 || *ptr == ' ') pv_string[4] = 0;
			else {
				pv_string[4] = *ptr++;
				pv_string[5] = 0;
			}
			if (pv_string[0] == '1' || pv_string[0] == '0') break;
			int move = move_from_string(pv_string);
			if (Current->turn) {
				if (!is_legal<1>(move)) break;
				do_move<1>(move);
			} else {
				if (!is_legal<0>(move)) break;
				do_move<0>(move);
			}
			memcpy(Data, Current, sizeof(GData));
			Current = Data;
			while (*ptr == ' ') ptr++;
		}
	}
	fclose(fpgn);
	fprintf(stdout, "%d PGN positions\n", pgn_positions);
#endif
}
void init_variables() {
	int i, j, k, start = 0;
	FILE * f;

	if (Local) f = fopen(GullCpp, "r");
	else if (Server) f = fopen("./Server/Gull.cpp", "r");
	else f = fopen("./Client/Gull.cpp", "r");
	while (!feof(f)) {
		(void)fgets(mstring, 256, f);
		if (!start && memcmp(mstring, "// tuner: start", 15)) continue;
		start = 1;
		if (!memcmp(mstring, "// tuner: stop", 14)) break;
		memcpy(SourceFile[src_str_num].line, mstring, 256);
		src_str_num++;
	}
	fclose(f);

	var_number = 0;
	active_vars = 0;

	int curr_ind = -1, active, indexed[MaxVariables];
	double var;
	char *p, *q;
	memset(VarName, 0, 1000 * sizeof(GString)); memset(indexed, 0, MaxVariables * sizeof(int));
	for (i = 0; i < src_str_num; i++) {
		if (!strstr(SourceFile[i].line, "tuner: enum")) continue;
		for (i++; !strstr(SourceFile[i].line, "};"); i++) {
			p = strchr(SourceFile[i].line, 'I') + 1;
			strcpy(VarName[var_name_num].line, p);
			for (j = 0; VarName[var_name_num].line[j] >= '0' && VarName[var_name_num].line[j] <= 'z'; j++);
			VarName[var_name_num].line[j] = '\n';
			for (k = j + 1; k < 1000; k++) VarName[var_name_num].line[k] = 0;
			q = strchr(p, '+');
			if (q != NULL) curr_ind += atoi(q + 1);
			else curr_ind++;
			VarIndex[var_name_num] = curr_ind;
			var_name_num++;
		}
		break;
	}
	for (i = 0; i < src_str_num; i++) {
		if (!(p = strstr(SourceFile[i].line, "tuner:"))) continue;
		q = strstr(p, "type="); if (q == NULL) continue; q += 5;
		p = strstr(q, "active=");
		if (p == NULL) active = 1;
		else active = atoi(p + 7);
		uint8 active_mask[1024];
		memset(active_mask, 1, 1024);
		if (p = strstr(q, "mask=")) {
			p += 5;
			j = 0;
			while (p[0] != ' ') {
				int value = 0; if (p[0] == 's') value = 1;
				if (p[1] == '&') {
					if (value == 1) break;
					for (k = j; k < 1024; k++) active_mask[k] = 0;
					break;
				}
				for (k = 0; k < p[1] - '0'; k++) active_mask[j + k] = value;
				j += p[1] - '0';
				p += 2;
			}
		}
		p = strstr(q, "var=");
		if (p == NULL) var = 1000.0;
		else var = (double)atoi(p + 4);
		if (!memcmp(q, "array", 5)) {
			p = strstr(SourceFile[i].line, "int "); p += 4;
			q = strchr(p, '[');
			for (j = 0; j < var_name_num; j++) if (!memcmp(p, VarName[j].line, (int)(q - p))) break;
			curr_ind = VarIndex[j];
			fprintf(stdout, "Array (%d) active=%d var=%.2lf: %s", curr_ind, active, var, VarName[j].line);
		}
		i++;
		memset(mstring, 0, strlen(mstring));
		while (!strstr(SourceFile[i].line, "};")) {
			strcat(mstring, SourceFile[i].line);
			i++;
		}
		i--;
		p = mstring - 1;
		int cnt = 0;
		do {
			p++;
			Variables[curr_ind] = atoi(p); var_number++;
			if (indexed[curr_ind]) { fprintf(stdout, "index mismatch: %d (%s)\n", curr_ind, VarName[j].line); exit(0); }
			indexed[curr_ind]++;
			int activate = 0;
			if (active && active_mask[cnt]) activate = 1;
			if (activate) Var[active_vars] = var;
			Active[curr_ind++] = activate; active_vars += activate; cnt++;
		} while (p = strchr(p, ','));
	}
	for (i = 0; i < curr_ind; i++) if (!indexed[i]) { fprintf(stdout, "index skipped %d\n", i); exit(0); }
	fprintf(stdout, "%d variables, %d active\n", var_number, active_vars);
}
void eval_to_cpp(const char * filename, double * list) {
	FILE * f = fopen(filename, "w");
	for (int i = 0; i < var_name_num; i++) VarName[i].line[strlen(VarName[i].line) - 1] = 0;
	for (int i = 0; i < src_str_num; i++) {
		fprintf(f, "%s", SourceFile[i].line);
		if (!strstr(SourceFile[i].line, "type=array") || strstr(SourceFile[i].line, "active=0")) continue;
		for (int j = 0; j < var_name_num; j++) if (strstr(SourceFile[i].line, VarName[j].line)) {
			int n = 0;
start:
			i++;
			fprintf(f, "    ");
			int cnt = 0, index = 0; for (int k = 0; k < VarIndex[j]; k++) if (Active[k]) index++;
			char * p = SourceFile[i].line, *end;
			while ((p = strchr(p+1, ',')) != NULL) cnt++; if (end = strstr(SourceFile[i + 1].line, "};")) cnt++;
			for (int k = 0; k < cnt; k++) {
				fprintf(f, "%d", (int)list[index + (n++)]);
				if (k + 1 < cnt) fprintf(f, ", ");
				else if (end == NULL) fprintf(f, ",\n");
				else fprintf(f, "\n");
			}
			if (end == NULL) goto start;
		}
	}
	fclose(f);
}
void print_eval() {
	int i, j;
	FILE * f = fopen("eval.txt", "w");
	fprintf(f, "Pst\n");
	for (j = 2; j < 16; j += 2) {
		if (j == 8) continue;
		fprintf(f, "%d:\n", j);
		for (i = 0; i < 64; i++) {
			fprintf(f, "(%d,%d), ", Opening(Pst(j, i)), Endgame(Pst(j, i)));
			if ((i + 1) % 8 == 0) fprintf(f, "\n");
		}
	}
	fprintf(f, "Mobility\n");
	for (j = 0; j < 4; j++) {
		fprintf(f, "%d:\n", j);
		for (i = 0; i < 32; i++) fprintf(f, "(%d,%d), ", Opening(Mobility[j][i]), Endgame(Mobility[j][i]));
		fprintf(f, "\n");
	}
	fprintf(f, "PasserGeneral\n");
	for (i = 0; i < 8; i++) fprintf(f, "(%d,%d), ", Opening(PasserGeneral[i]), Endgame(PasserGeneral[i]));
	fprintf(f, "\n");

	fprintf(f, "PasserBlocked\n");
	for (i = 0; i < 8; i++) fprintf(f, "(%d,%d), ", Opening(PasserBlocked[i]), Endgame(PasserBlocked[i]));
	fprintf(f, "\n");

	fprintf(f, "PasserFree\n");
	for (i = 0; i < 8; i++) fprintf(f, "(%d,%d), ", Opening(PasserFree[i]), Endgame(PasserFree[i]));
	fprintf(f, "\n");

	fprintf(f, "PasserSupported\n");
	for (i = 0; i < 8; i++) fprintf(f, "(%d,%d), ", Opening(PasserSupported[i]), Endgame(PasserSupported[i]));
	fprintf(f, "\n");

	fprintf(f, "PasserProtected\n");
	for (i = 0; i < 8; i++) fprintf(f, "(%d,%d), ", Opening(PasserProtected[i]), Endgame(PasserProtected[i]));
	fprintf(f, "\n");

	fprintf(f, "PasserConnected\n");
	for (i = 0; i < 8; i++) fprintf(f, "(%d,%d), ", Opening(PasserConnected[i]), Endgame(PasserConnected[i]));
	fprintf(f, "\n");

	fprintf(f, "PasserOutside\n");
	for (i = 0; i < 8; i++) fprintf(f, "(%d,%d), ", Opening(PasserOutside[i]), Endgame(PasserOutside[i]));
	fprintf(f, "\n");

	fprintf(f, "PasserCandidate\n");
	for (i = 0; i < 8; i++) fprintf(f, "(%d,%d), ", Opening(PasserCandidate[i]), Endgame(PasserCandidate[i]));
	fprintf(f, "\n");

	fprintf(f, "PasserClear\n");
	for (i = 0; i < 8; i++) fprintf(f, "(%d,%d), ", Opening(PasserClear[i]), Endgame(PasserClear[i]));
	fprintf(f, "\n");

	fprintf(f, "PasserAtt\n");
	for (i = 0; i < 8; i++) fprintf(f, "%d, ", PasserAtt[i]);
	fprintf(f, "\n");

	fprintf(f, "PasserDef\n");
	for (i = 0; i < 8; i++) fprintf(f, "%d, ", PasserDef[i]);
	fprintf(f, "\n");

	fprintf(f, "PasserAttLog\n");
	for (i = 0; i < 8; i++) fprintf(f, "%d, ", PasserAttLog[i]);
	fprintf(f, "\n");

	fprintf(f, "PasserDefLog\n");
	for (i = 0; i < 8; i++) fprintf(f, "%d, ", PasserDefLog[i]);
	fprintf(f, "\n");

	fprintf(f, "StormBlocked\n");
	for (i = 0; i < 4; i++) fprintf(f, "%d, ", StormBlocked[i]);
	fprintf(f, "\n");

	fprintf(f, "StormShelterAtt\n");
	for (i = 0; i < 4; i++) fprintf(f, "%d, ", StormShelterAtt[i]);
	fprintf(f, "\n");

	fprintf(f, "StormConnected\n");
	for (i = 0; i < 4; i++) fprintf(f, "%d, ", StormConnected[i]);
	fprintf(f, "\n");

	fprintf(f, "StormOpen\n");
	for (i = 0; i < 4; i++) fprintf(f, "%d, ", StormOpen[i]);
	fprintf(f, "\n");

	fprintf(f, "StormFree\n");
	for (i = 0; i < 4; i++) fprintf(f, "%d, ", StormFree[i]);
	fprintf(f, "\n");

	fclose(f);
}

double ratio_from_elo(double elo) { return 1.0 / (1.0 + exp(((-elo) / 400.0)*log(10.0))); }
double elo_from_ratio(double ratio) { return -(log((1.0 / MinF(0.99999, Max(ratio, 0.00001))) - 1.0) / log(10.0)) * 400.0; }
double rand_u() { return MinF(1.0, Max(0.0, ((double)((rand() << 15) | rand())) / (32768.0 * 32768.0))); }
double gaussian(double mean, double sigma) { return sqrt(Max(0.0000001, -2.0 * log(Max(0.0000001, rand_u())))) * sin(2.0 * 3.14159265358979323846 * rand_u()) * sigma + mean; }
void int_to_double(double * dst, int * src, int n) {for (int i = 0; i < n; i++) dst[i] = (double)src[i];}
void double_to_int(int * dst, double * src, int n) {for (int i = 0; i < n; i++) dst[i] = (int)src[i];}
void double_to_double(double * dst, double * src, int n) {for (int i = 0; i < n; i++) dst[i] = src[i];}
void int_to_int(int * dst, int * src, int n) {for (int i = 0; i < n; i++) dst[i] = src[i];}
double scalar(double * one, double * two, int n) {
	double result = 0.0;
	for (int i = 0; i < n; i++) result += one[i] * two[i];
	return result;
}
void load_list(double * list) {
	int i, j = 0;
	for (i = 0; i < var_number; i++) if (Active[i]) Variables[i] = (int)list[j++];
}
void save_list(double * list) {
	int i, j = 0;
	for (i = 0; i < var_number; i++) if (Active[i]) list[j++] = (double)Variables[i];
}
void log_list(FILE * f, double * list, int n) {
	fprintf(f,"(");
	for (int i = 0; i < n; i++) {
		fprintf(f,"%.2lf",list[i]);
		if (i < n - 1) fprintf(f,",");
	}
	fprintf(f,")\n");
}
void log_list(const char * file_name, double * list, int n) {
	FILE * f = fopen(file_name,"a");
	log_list(f,list,n);
	fclose(f);
}
void log_list(char * s, double * list, int n, bool precision) {
	sprintf(s+strlen(s),"(");
	for (int i = 0; i < n; i++) {
		if (!precision) sprintf(s+strlen(s),"%.2lf",list[i]);
		else sprintf(s+strlen(s),"%lf",list[i]);
		if (i < n - 1) sprintf(s+strlen(s),",");
	}
	sprintf(s+strlen(s),") ");
}
void read_list(char * string, double * list, int n) {
	int i = 0;
	char * p = strchr(string,'(');
	do {
		p++;
		list[i++] = atof(p);
		if (i >= n) break;
	} while (p = strchr(p,','));
}
void init_eval_data(double * one, double * two) {
	if (one != EvalOne) double_to_double(EvalOne,one,var_number);
	if (two != EvalTwo) double_to_double(EvalTwo,two,var_number);
	load_list(one); Pst = PstOne; init_pst();
	load_list(two); Pst = PstTwo; init_pst();
}
void load_eval(int first) {
	int i;
	generation++;
	for (i = 1; i < 128; i++) Data[i].eval_key = 0;
	if (first) {
		load_list(EvalOne);
		Hash = HashOne;
		PawnHash = PawnHashOne;
		PVHash = PVHashOne;
		Pst = PstOne;
		History = HistoryOne;
		Delta = DeltaOne;
		Ref = RefOne;
	} else {
		load_list(EvalTwo);
		Hash = HashTwo;
		PawnHash = PawnHashTwo;
		PVHash = PVHashTwo;
		Pst = PstTwo;
		History = HistoryTwo;
		Delta = DeltaTwo;
		Ref = RefTwo;
	}
	Current->pst = 0;
	for (i = 0; i < 64; i++) if (Square(i)) Current->pst += Pst(Square(i),i);
	init_eval();
}
void compute_list(double * dst, double * base, double * dir, double * var, double a) {for (int i = 0; i < active_vars; i++) dst[i] = base[i] + dir[i] * var[i] * a;}
void scale_list(double * list, double r) {
	int i;
	double x = 0.0;
	for (i = 0; i < active_vars; i++) x += Sqr(list[i]);
	x = r/sqrt(x);
	for (i = 0; i < active_vars; i++) list[i] *= x; 
}
int play(int depth) {
	LastDepth = TimeLimit1 = TimeLimit2 = 0;
#ifdef TIMING
	Infinite = 0;
	int nmoves = MovesTg - 1;
	if (Current->ply > 40) nmoves += Min(Current->ply - 40, (100 - Current->ply) / 2);
	TimeLimit1 = Min(GlobalTime[GlobalTurn], (GlobalTime[GlobalTurn] + nmoves * GlobalInc[GlobalTurn]) / nmoves);
	TimeLimit2 = Min(GlobalTime[GlobalTurn], (GlobalTime[GlobalTurn] + nmoves * GlobalInc[GlobalTurn]) / 3);
	TimeLimit1 = Min(GlobalTime[GlobalTurn], (TimeLimit1 * TimeRatio) / 100);
	DepthLimit = 128; Searching = 1; nodes = Stop = 0;
	StartTime = get_time();
#else
	DepthLimit = 2 * depth + 2;
	Infinite = 1;
#endif
	best_score = best_move = 0;
	Print = 0;
	if (Current->turn == White) root<0>(); else root<1>();
	return best_score;
}
double play_game(double * one, double * two, int depth, char * fen) {
	int i, cnt, sdepth, value, previous = 0, im = 0;
	load_eval(0); init_search(1); load_eval(1); init_search(1);
#ifndef PGN
	get_board(fen);
	if (RecordGames) {
		RecordString[0] = 0;
		for (cnt = 0; fen[cnt] != '\n'; cnt++) PosStr[cnt] = fen[cnt];
		PosStr[cnt] = 0;
	}
#else
	position_from_pos((GPos*)fen);
#endif
	init_eval_data(one,two); 

#ifdef TIMING
	GlobalTime[0] = GlobalTime[1] = Convert((1.0 + rand_u()) * (double)(1000 << (int)(depth)),int);
	GlobalInc[0] = GlobalInc[1] = GlobalTime[0] / 200;
#endif

	for (cnt = 0; cnt < 200 + (RecordGames ? 200 : 0); cnt++) {
		GlobalTurn = Even(cnt);
		load_eval(GlobalTurn);
		memcpy(Data, Current, sizeof(GData));
		Current = Data;
		if (Even(cnt)) sdepth = depth + Odd(rand16());
		value = play(sdepth);
		if (!best_move) goto loss;
		if (value < -ResignThreshold && previous > ResignThreshold) goto loss;
		if (!RecordGames) {
			if (value == 0 && previous == 0 && cnt >= 60) goto draw;
			if (Abs(value) <= 3 && Abs(previous) <= 3 && cnt >= 120) goto draw;
		}
		if (Current->ply >= 100) goto draw;
		for (i = 4; i <= Current->ply; i += 2) if (Stack[sp - i] == Current->key) goto draw;
		int me = 0;
		if (!PawnAll) {
			int my_score = 3 * popcnt(Minor(me)) + 5 * popcnt(Rook(me)) + 9 * popcnt(Queen(me));
			int opp_score = 3 * popcnt(Minor(opp)) + 5 * popcnt(Rook(opp)) + 9 * popcnt(Queen(opp));
			if (Abs(my_score - opp_score) <= 3 && Max(popcnt(NonPawnKing(me)),popcnt(NonPawnKing(opp))) <= 2) {
				im++;
				if (im >= 10 && Abs(value) < 50 && Abs(previous) < 50) goto draw; 
			}
		}
#ifdef WIN_PR
		if (cnt >= 6 && (
			(!Queen(White) && !Queen(Black)) ||
			(popcnt(NonPawnKing(White)) <= 2 && popcnt(NonPawnKing(Black)) <= 2) ||
			(!Current->castle_flags && ((File[File(lsb(King(White)))] | PIsolated[File(lsb(King(White)))]) & King(Black)))
			)) return ratio_from_elo(3.0 * (double)value) + ratio_from_elo(-3.0 * (double)previous);
#endif
		previous = value;
		if (!Current->turn) do_move<0>(best_move);
		else do_move<1>(best_move);
		if (RecordGames) {
			move_to_string(best_move, pv_string);
			sprintf(RecordString + strlen(RecordString), "%s { %.2lf / %d } ", pv_string, (double)(Current->turn ? value : (-value)) / 100.0, LastDepth/2);
		}
	}
draw:
	if (RecordGames) sprintf(Buffer + strlen(Buffer), "[FEN \"%s\"]\n[Result \"1/2-1/2\"]\n%s\n", PosStr, RecordString);
	return 1.0;
loss:
	if (Even(cnt)) {
		if (RecordGames) {
			if (Current->turn) sprintf(Buffer + strlen(Buffer), "[FEN \"%s\"]\n[Result \"1-0\"]\n%s\n", PosStr, RecordString);
			else sprintf(Buffer + strlen(Buffer), "[FEN \"%s\"]\n[Result \"0-1\"]\n%s\n", PosStr, RecordString);
		}
		return 0.0;
	} else {
		if (RecordGames) {
			if (Current->turn) sprintf(Buffer + strlen(Buffer), "[FEN \"%s\"]\n[Result \"1-0\"]\n%s\n", PosStr, RecordString);
			else sprintf(Buffer + strlen(Buffer), "[FEN \"%s\"]\n[Result \"0-1\"]\n%s\n", PosStr, RecordString);
		}
		return 2.0;
	}
}
double play_position(double * one, double * two, int depth, char * fen, GMatchInfo * MI) {
	double result, score = 0.0;
	result = play_game(one, two, depth, fen); if (result >= 1.98) MI->wins++; else if (result <= 0.02) MI->losses++; else MI->draws++;
	score += result;
	result = play_game(two, one, depth, fen); if (result >= 1.98) MI->losses++; else if (result <= 0.02) MI->wins++; else MI->draws++;
	score += 2.0 - result;
	return score;
}
double match(double * one, double * two, int positions, int depth, GMatchInfo * MI) {
	double score = 0.0;
	memset(MI,0,sizeof(GMatchInfo));
	for (int i = 0; i < positions; i++) {
#ifndef PGN
		score += play_position(one, two, depth, Fen[random() % (uint64)opening_positions], MI);
#else
		score += play_position(one, two, depth, (char*)&Pos[random() % (uint64)pgn_positions], MI);
#endif
	}
	return (25.0 * (double)score)/(double)(positions);
}
int match_los(double * one, double * two, int positions, int chunk_size, int depth, double high, double low, double uh, double ul, GMatchInfo * MI, bool print) {
	int pos = 0;
	double score, ratio, stdev, wins, draws, losses, total, tot_score = 0.0;
	cmd_number++;

	memset(mstring,0,strlen(mstring));
	sprintf(mstring,"$ Number=%d Command=match Depth=%d Positions=%d",cmd_number,depth,chunk_size);
	sprintf(mstring+strlen(mstring)," First="); log_list(mstring,one,active_vars,false);
	sprintf(mstring+strlen(mstring)," Second="); log_list(mstring,two,active_vars,false);
	fseek(stdin,0,SEEK_END);
	fprintf(stdout,"%s\n",mstring);

	memset(MI,0,sizeof(GMatchInfo));
	while (pos < positions) {
		pos += chunk_size;
start:
		fgets(mstring,65536,stdin);
		char * p = strstr(mstring,"Number=");
		if (p == NULL) goto start;
		if (atoi(p+7) != cmd_number) goto start;
		p = strstr(mstring, "Wins="); MI->wins += atoi(p + 5); 
		p = strstr(mstring, "Draws="); MI->draws += atoi(p + 6); 
		p = strstr(mstring, "Losses="); MI->losses += atoi(p + 7); 
		p = strstr(mstring, "Result="); tot_score += atof(p + 7);

		wins = (double)MI->wins; draws = (double)MI->draws; losses = (double)MI->losses; total = Max(wins + losses,1.0);
#ifndef WIN_PR
		score = (100.0 * wins + 50.0 * draws)/(total + draws);
#else
		score = tot_score / (double)(pos / chunk_size);
#endif
		if (print) fprintf(stdout,"%.2lf (%d positions played): %d-%d-%d\n",score,pos,MI->wins,MI->draws,MI->losses);
		if (total <= 0.99) continue;
		ratio = wins/total;
		stdev = 0.5/sqrt(total);
		if (high > 0.01) {
			if (ratio >= 0.5 + stdev * high) return 1;
#ifdef WIN_PR
			if (score / 100.0 >= 0.5 + stdev * high) return 1;
#endif
		}
		if (low > 0.01) {
			if (ratio <= 0.5 - stdev * low) return -1;
#ifdef WIN_PR
			if (score / 100.0 <= 0.5 - stdev * low) return -1;
#endif
		}
		if (pos >= positions) break;
		double remaining = ((2.0 * (double)positions - total - draws) * (wins + losses)) / (total + draws);
		double target_high = 0.5 * (1.0 + (high / sqrt(total + remaining)));
		double target_low = 0.5 * (1.0 - (low / sqrt(total + remaining)));
		double ratio_high = target_high + 0.5 * (uh / sqrt(remaining));
		double ratio_low = target_low - 0.5 * (ul / sqrt(remaining));
		if (uh > 0.01) if ((wins + ratio_high * remaining) / (total + remaining) < target_high) return -1;
		if (ul > 0.01) if ((wins + ratio_low * remaining) / (total + remaining) > target_low) return 1;
	}
	return 0;
}
void gradient(double * base, double * var, int iter, int pos_per_iter, int depth, double radius, double * grad) {
	int i, j;
	double dir[MaxVariables], A[MaxVariables], B[MaxVariables], r;
	memset(grad,0,active_vars * sizeof(double));
	for (i = 0; i < iter; i++) {
#ifndef RANDOM_SPHERICAL
		for (j = 0; j < active_vars; j++) dir[j] = (Odd(rand()) ? 1.0 : (-1.0))/sqrt(active_vars);
#else
		for (j = 0, r = 0.0; j < active_vars; j++) {
			dir[j] = gaussian(0.0, 1.0);
			r += dir[j] * dir[j];
		}
		r = 1.0/sqrt(Max(r, 0.0000001));
		for (j = 0; j < active_vars; j++) dir[j] *= r;
#endif
		compute_list(A,base,dir,Var,-radius);
		compute_list(B,base,dir,Var,radius);
		r = 50.0 - match(A,B,pos_per_iter,depth,MatchInfo);
		for (j = 0; j < active_vars; j++) grad[j] += r * dir[j];
	}
	for (i = 0; i < active_vars; i++) grad[i] /= (double)iter;
}
void NormalizeVar(double * base, double * base_var, int depth, int positions, double radius, double target, double * var) {
	int i, j;
	double A[MaxVariables], r, value, curr_var;

	fprintf(stdout,"NormalizeVar(): depth=%d, positions=%d, radius=%.2lf, target=%.2lf\n",depth,positions,radius,target); 
	for (i = 0; i < active_vars; i++) {
		double_to_double(A,base,active_vars);
		curr_var = base_var[i];
		fprintf(stdout,"Variable %d (%.2lf):\n",i,curr_var);
		for (j = 0; j < 10; j++) {
			A[i] = base[i] + (radius * curr_var);
			match_los(base,A,positions,16,depth,0.0,0.0,0.0,0.0,MatchInfo,false);
			r = Convert(100 * MatchInfo->wins + 50 * (double)MatchInfo->draws,double)/(double)(MatchInfo->wins + MatchInfo->draws + MatchInfo->losses);
			value = elo_from_ratio(r * 0.01);
			if (value < target) break;
			curr_var = curr_var * MinF(sqrt(target/Max(value, 1.0)),1.5);
			fprintf(stdout,"(%.2lf,%.2lf)\n",value,curr_var);
			if (curr_var > base_var[i]) {
				curr_var = base_var[i];
				break;
			}
		}
		var[i] = curr_var;
		fprintf(stdout, "(%.2lf,%.2lf)\n", value, curr_var);
	}
	log_list("var.txt",var,active_vars);
}

void Gradient(double * base, double * var, int depth, int iter, int pos_per_iter, int max_positions, double radius, double angle_target, double * grad) {
	typedef struct {
		double grad[MaxVariables];
	} GGradient;
	GGradient A[4], N[4];
	double list[MaxVariables], av, angle;
	int i, j, cnt = 0;
	cmd_number++;

	fprintf(stdout,"Gradient(): depth=%d, iter=%d, pos_per_iter=%d, max_positions=%d, radius=%.2lf\n",depth,iter,pos_per_iter,max_positions,radius);
	memset(A,0,4 * sizeof(GGradient));
	memset(grad,0,active_vars * sizeof(double));

	memset(mstring,0,strlen(mstring));
	sprintf(mstring,"$ Number=%d Command=gradient Depth=%d Iter=%d Positions=%d Radius=%lf Var=",cmd_number,depth,iter,pos_per_iter,radius); log_list(mstring,Var,active_vars,false);
	sprintf(mstring+strlen(mstring)," Base="); log_list(mstring,Base,active_vars,false);
	fseek(stdin,0,SEEK_END);
	fprintf(stdout,"%s\n",mstring);

	while (cnt < max_positions) {
		for (j = 0; j < 4; j++) {
start:
			fgets(mstring,65536,stdin);
			char * p = strstr(mstring,"Number=");
			if (p == NULL) goto start;
			if (atoi(p+7) != cmd_number) goto start;
			p = strstr(mstring,"Grad="); read_list(p,list,active_vars);

			for (i = 0; i < active_vars; i++) {
				A[j].grad[i] += list[i]; 
				N[j].grad[i] = A[j].grad[i];
			}
			scale_list(N[j].grad,1.0);
		}
		for (i = 0; i < active_vars; i++) grad[i] = A[0].grad[i] + A[1].grad[i] + A[2].grad[i] + A[3].grad[i];
		scale_list(grad,1.0);
		av = 0.0;
		for (i = 0; i < 4; i++) for (j = i + 1; j < 4; j++) av += scalar(N[i].grad,N[j].grad,active_vars);
		av /= 6.0;
		av = Min(0.99999,Max(-0.99999,av));
		angle = (acos(av) * 180.0)/3.1415926535;
		cnt += 4 * pos_per_iter * iter;
		fprintf(stdout,"%d positions: angle = %.2lf, gradient = ",cnt,angle);
		log_list(stdout,grad,active_vars);
		if (angle < angle_target) break;
		FILE * fgrad = fopen("gradient.txt","w");
		log_list(fgrad,grad,active_vars);
		fprintf(fgrad,"%d\n",cnt);
		fclose(fgrad);
	}
}
void GD(double * base, double * var, int depth, double radius, double min_radius, double angle_target, int max_grad_positions, int max_line_positions, double high, double low, double uh, double ul) {
	double Grad[MaxVariables], a, br, A[MaxVariables], B[MaxVariables];
	FILE * fbest = fopen("gd.txt","w"); fclose(fbest);

	fprintf(stdout,"GD()\n");
	while (true) {
start:
		fbest = fopen("gd.txt","a"); fprintf(fbest,"radius = %.2lf:\n",radius); log_list(fbest,base,active_vars); fclose(fbest);
		log_list(stdout,base,active_vars);
		//radius = 2.0; read_list("(0.05,-0.04,-0.00,0.04,0.09,-0.10,-0.00,0.06,-0.14,-0.08,-0.06,0.05,-0.21,-0.10,-0.03,0.04,0.06,-0.01,-0.04,0.06,0.01,-0.05,-0.02,-0.06,-0.05,0.14,0.18,-0.01,-0.01,0.02,-0.11,0.05,-0.00,0.18,-0.15,-0.02,0.03,0.01,-0.06,-0.07,-0.03,0.11,0.13,-0.07,0.06,0.02,-0.01,0.06,-0.07,-0.09,0.01,-0.09,0.13,-0.03,0.04,0.03,-0.04,0.16,0.03,-0.21,-0.01,0.04,-0.03,-0.11,0.00,-0.03,-0.03,-0.11,-0.00,-0.06,0.04,-0.05,0.00,-0.03,-0.12,0.00,-0.07,-0.13,-0.08,0.10,0.11,0.03,0.08,0.12,-0.05,-0.07,-0.01,-0.02,0.08,-0.12,-0.05,0.02,0.03,0.13,-0.08,0.05,0.04,0.02,-0.00,0.06,-0.06,-0.07,-0.00,0.05,-0.09,-0.16,-0.02,-0.07,0.16,-0.24,0.09,0.04,-0.09,0.03,-0.06,0.01,-0.05,0.00,-0.10,-0.02,-0.12,-0.05,-0.05,0.07,0.14,0.16,-0.07,0.03,-0.06,-0.16,-0.03,0.04,-0.04,0.02,-0.12,-0.18,0.01,-0.04,-0.04,-0.18,0.08,0.09,-0.06,-0.00,0.02,-0.03,0.10,0.04,-0.02)", Grad, active_vars);
		Gradient(base,var,depth,32,1,max_grad_positions,radius,angle_target,Grad);
		min_radius = Min(radius * 0.45, min_radius);
		a = radius;
		while (a >= min_radius) {
			fprintf(stdout,"Verification %.2lf:\n",a); 
			compute_list(A,base,Grad,var,a);
			//eval_to_cpp("gd.cpp", A);
			if (match_los(A,base,max_line_positions,32,depth,high,low,uh,ul,MatchInfo,true) == 1) {
				br = a;
				a *= 0.6;
				compute_list(B,base,Grad,var,a);
				double_to_double(base,A,active_vars);
				log_list("gd.txt",base,active_vars);
				eval_to_cpp("gd.cpp", base);
				fprintf(stdout,"New best: "); log_list(stdout,base,active_vars);
				fprintf(stdout,"Try %.2lf:\n",a);
				if (match_los(B,A,max_line_positions,32,depth,2.0,2.0,2.0,0.0,MatchInfo,true) == 1) {
					br = a;
					double_to_double(base,B,active_vars);
					log_list("gd.txt",base,active_vars);
					eval_to_cpp("gd.cpp", base);
					fprintf(stdout,"New best: "); log_list(stdout,base,active_vars);
				}
				if (br < radius * 0.29) radius *= 0.7;
				goto start;
			}
			a *= 0.7;
		}
		radius *= 0.7;
	}
}
void get_command() {
	enum {mode_grad, mode_match};
	int mode, depth, positions, number;
	char * p;

	if (RecordGames) Buffer[0] = 0;
	fgets(mstring,65536,stdin);
	fseek(stdin,0,SEEK_END);
	p = strstr(mstring,"Command=");
	if (p == NULL) return;
	if (!memcmp(p+8,"gradient",8)) mode = mode_grad;
	else if (!memcmp(p+8,"match",5)) mode = mode_match;
	else return;
	p = strstr(mstring,"Number="); number = atoi(p+7);
	p = strstr(mstring,"Depth="); depth = atoi(p+6);
	p = strstr(mstring,"Positions="); positions = atoi(p+10);
	if (mode == mode_grad) {
		p = strstr(mstring,"Iter="); int iter = atoi(p+5);
		p = strstr(mstring,"Radius="); int radius = atof(p+7);
		p = strstr(mstring,"Var="); read_list(p,Var,active_vars);
		p = strstr(mstring,"Base="); read_list(p,Base,active_vars);
		gradient(Base,Var,iter,positions,depth,radius,Grad);
		memset(mstring,0,strlen(mstring));
		sprintf(mstring,"$ Number=%d Grad=",number); log_list(mstring,Grad,active_vars,true);
		fprintf(stdout,"%s\n",mstring);
	} else if (mode == mode_match) {
		p = strstr(mstring,"First="); read_list(p,FE,active_vars);
		p = strstr(mstring,"Second="); read_list(p,SE,active_vars);
		double r = match(FE,SE,positions,depth,MatchInfo);
		if (RecordGames) {
			frec = fopen("games.pgn", "a");
			fprintf(frec, "%s\n", Buffer);
			fclose(frec);
		}
		memset(mstring,0,strlen(mstring));
		sprintf(mstring,"$ Number=%d Result=%lf Wins=%d Draws=%d Losses=%d",number,r,MatchInfo->wins,MatchInfo->draws,MatchInfo->losses); 
		fprintf(stdout,"%s\n",mstring);
	} else nodes /= 0;
}
int get_mat_index(int wq, int bq, int wr, int br, int wl, int bl, int wd, int bd, int wn, int bn, int wp, int bp) {
	if (wq > 2 || bq > 2 || wr > 2 || br > 2 || wl > 1 || bl > 1 || wd > 1 || bd > 1 || wn > 2 || bn > 2 || wp > 8 || bp > 8) return -1;
	return wp*MatWP + bp*MatBP + wn*MatWN + bn*MatBN + wl*MatWL + bl*MatBL + wd*MatWD + bd*MatBD + wr*MatWR + br*MatBR + wq*MatWQ + bq*MatBQ;
}
int conj_mat_index(int index, int * conj_symm, int * conj_ld, int * conj_ld_symm) {
	int wq = index % 3; index /= 3;
	int bq = index % 3; index /= 3;
	int wr = index % 3; index /= 3;
	int br = index % 3; index /= 3;
	int wl = index % 2; index /= 2;
	int bl = index % 2; index /= 2;
	int wd = index % 2; index /= 2;
	int bd = index % 2; index /= 2;
	int wn = index % 3; index /= 3;
	int bn = index % 3; index /= 3;
	int wp = index % 9; index /= 9;
	int bp = index;
	*conj_symm = -1;
	*conj_ld = -1;
	*conj_ld_symm = -1;
	if (wq != bq || wr != br || wl != bd || wd != bl || wn != bn || wp != bp) {
		*conj_symm = get_mat_index(bq, wq, br, wr, bd, wd, bl, wl, bn, wn, bp, wp);
		if (wl != wd || bl != bd) {
			*conj_ld = get_mat_index(wq, bq, wr, br, wd, bd, wl, bl, wn, bn, wp, bp);
			*conj_ld_symm = get_mat_index(bq, wq, br, wr, bl, wl, bd, wd, bn, wn, bp, wp);
		}
	}
	return *conj_symm;
}
void pgn_stat() {
#define elo_eval_ratio 1.0
#define PosInRow 6
#define ratio_from_eval(x) ratio_from_elo(elo_eval_ratio * (x))
#define ind_from_eval(x) (ratio_from_eval((double)(x)) >= 0.5 ? Max(0, (int)((ratio_from_eval((double)(x)) - 0.5) * 100.0)) : Max(0, (int)((0.5 - ratio_from_eval((double)(x))) * 100.0)))
#define est_from_ind(x) (Eval[x].score/Max(1.0,(double)Eval[x].cnt))
#define est_from_eval(x) ((x) >= 0 ? est_from_ind(ind_from_eval(x)) : (1.0 - est_from_ind(ind_from_eval(x))))
	typedef struct {
		double score;
		double est;
		int cnt;
	} GStat;
	GStat Eval[64]; for (int i = 0; i < 64; i++) { Eval[i].cnt = 0; Eval[i].score = 1.0; }
	GStat * Mat = (GStat*)malloc(TotalMat * sizeof(GStat)); memset(Mat, 0, TotalMat * sizeof(GStat));
	FILE * fpgn;
	int iter = 0;
loop:
	fpgn = fopen("D:/Development/G3T/games.pgn", "r");
	if (fpgn == NULL) {
		fprintf(stdout, "File 'games.pgn' not found\n"); getchar();
		exit(0);
	}
	double stat = 0.0, est = 0.0;
	int cnt = 0;
	while (true) {
		fgets(mstring, 65536, fpgn);
		if (feof(fpgn)) break;
		if (strstr(mstring, "FEN")) get_board(mstring + 6);
		double result;
		if (strstr(mstring, "Result")) {
			if (strstr(mstring, "1-0")) result = 1.0;
			else if (strstr(mstring, "0-1")) result = 0.0;
			else result = 0.5;
		}
		if (strchr(mstring, '[')) continue;
		if (strlen(mstring) < 100) continue;
		char * ptr = mstring;
		int eval[20], nc = 0; memset(eval, 0, 20 * sizeof(int));
		while (*ptr != 0) {
			if (!Current->capture && !(Current->move & 0xE000)) nc++;
			else nc = 0;
			evaluate();
			if (nc == PosInRow) {
				double ratio = ratio_from_eval((double)eval[PosInRow - 2]);
				int r_index;
				if (ratio >= 0.5) r_index = Max(0, (int)((ratio - 0.5) * 100.0));
				else r_index = Max(0, (int)((0.5 - ratio) * 100.0));
				if (Even(iter)) {
					Eval[r_index].cnt++;
					Eval[r_index].score += (ratio >= 0.5 ? result : (1.0 - result));
				}

				if (!(Current->material & FlagUnusualMaterial) && Odd(iter)) {
					int index = Current->material, conj_symm, conj_ld, conj_ld_symm;
					conj_mat_index(index, &conj_symm, &conj_ld, &conj_ld_symm);
					Mat[index].cnt++; Mat[index].score += result; Mat[index].est += est_from_eval(eval[PosInRow - 1]);
					if (conj_symm >= 0) Mat[conj_symm].cnt++; Mat[conj_symm].score += 1.0 - result; Mat[conj_symm].est += 1.0 - est_from_eval(eval[PosInRow - 1]);
					if (conj_ld >= 0) Mat[conj_ld].cnt++; Mat[conj_ld].score += result; Mat[conj_ld].est += est_from_eval(eval[PosInRow - 1]);
					if (conj_ld_symm >= 0) Mat[conj_ld_symm].cnt++; Mat[conj_ld_symm].score += 1.0 - result; Mat[conj_ld_symm].est += 1.0 - est_from_eval(eval[PosInRow - 1]);
				}
			}
			pv_string[0] = *ptr++;
			pv_string[1] = *ptr++;
			pv_string[2] = *ptr++;
			pv_string[3] = *ptr++;
			if (*ptr == 0 || *ptr == ' ') pv_string[4] = 0;
			else {
				pv_string[4] = *ptr++;
				pv_string[5] = 0;
			}
			int move = move_from_string(pv_string);
			if (Current->turn) {
				if (!is_legal<1>(move)) break;
				do_move<1>(move);
			} else {
				if (!is_legal<0>(move)) break;
				do_move<0>(move);
			}
			memcpy(Data, Current, sizeof(GData));
			Current = Data;
			while (*ptr == ' ') ptr++;
			for (int i = 19; i >= 1; i--) eval[i] = eval[i - 1];
			eval[0] = (int)(atof(ptr + 2) * 100.0);
			ptr = strchr(ptr, '}') + 2;
		}
	}
	fclose(fpgn);
	if (!iter) {
		for (int i = 0; i < 64; i++) fprintf(stdout, "ratio(eval x %.2lf) in (%.2lf, %.2lf), score = %.2lf\n", elo_eval_ratio, 50.0 + (double)i, 50.0 + (double)(i + 1), (Eval[i].score * 100.0) / Max(1.0, (double)Eval[i].cnt));
		iter++;
		goto loop;
	}
	FILE * fmat = fopen("material.txt", "w");
	fprintf(fmat, "const int MaterialShift[MaterialShiftSize] = {\n");
	int mat_cnt = 0;
	for (int index = 0; index < TotalMat; index++) {
		int cnt = Mat[index].cnt;
		if (cnt < 64) continue;
		double ratio = Mat[index].score / (double)cnt;
		double est = Mat[index].est / (double)cnt;
		bool flag = (ratio < est);
		double error = (sqrt(2.0) * 0.5) / sqrt((double)cnt);
		if (Abs(ratio - est) <= error + 0.01) continue;
		ratio = ((ratio >= est) ? (ratio - error) : (ratio + error));
		if (est <= 0.5 && ratio > 0.5) ratio = 0.5000001;
		if (est >= 0.5 && ratio < 0.5) ratio = 0.4999999;
		double abs_ratio = ((ratio >= 0.5) ? ratio : (1.0 - ratio));
		double abs_est = ((est >= 0.5) ? est : (1.0 - est));
		int curr_ind = 0, new_ind = 0;
		for (int i = 0; i < 64; i++) {
			if (Eval[i].score / Max(1.0, (double)Eval[i].cnt) < abs_ratio) new_ind = i;
			if (Eval[i].score / Max(1.0, (double)Eval[i].cnt) < abs_est) curr_ind = i;
		}
		if (Abs(curr_ind - new_ind) <= 1) continue;
		if (new_ind > curr_ind) new_ind--;
		else new_ind++;
		double curr_eval = elo_from_ratio(Eval[curr_ind].score / Max(1.0, (double)Eval[curr_ind].cnt)) / elo_eval_ratio;
		double new_eval = elo_from_ratio(Eval[new_ind].score / Max(1.0, (double)Eval[new_ind].cnt)) / elo_eval_ratio;
		int score = (int)Abs(curr_eval - new_eval);
		if (flag) score = -score;
		if (Sgn(score) != Sgn(Material[index].score)) score = Sgn(score) * Min(Abs(Material[index].score), Abs(score));
		if (Abs(score) < 5) continue;
		mat_cnt++;
		fprintf(fmat, "%d, %d, ", index, score);
		if ((mat_cnt % 8) == 0) fprintf(fmat, "\n");
	}
	fprintf(fmat, "}; %d\n", mat_cnt * 2);
	fclose(fmat);
	fprintf(stdout, "Press any key...\n");
}
#endif

#ifndef W32_BUILD
__forceinline int lsb(uint64 x) {
	register unsigned long y;
	_BitScanForward64(&y, x);
	return y;
}

__forceinline int msb(uint64 x) {
	register unsigned long y;
	_BitScanReverse64(&y, x);
	return y;
}

__forceinline int popcnt(uint64 x) {
	x = x - ((x >> 1) & 0x5555555555555555);
	x = (x & 0x3333333333333333) + ((x >> 2) & 0x3333333333333333);
	x = (x + (x >> 4)) & 0x0f0f0f0f0f0f0f0f;
	return (x * 0x0101010101010101) >> 56;
}

template <bool HPopCnt> __forceinline int popcount(uint64 x) {
	return HPopCnt ? _mm_popcnt_u64(x) : popcnt(x);
}
#else
__forceinline int lsb(uint64 x) {
	_asm {
		mov eax, dword ptr x[0]
			test eax, eax
			jz l_high
			bsf eax, eax
			jmp l_ret
		l_high : bsf eax, dword ptr x[4]
				 add eax, 20h
			 l_ret :
	}
}

__forceinline int msb(uint64 x) {
	_asm {
		mov eax, dword ptr x[4]
			test eax, eax
			jz l_low
			bsr eax, eax
			add eax, 20h
			jmp l_ret
		l_low : bsr eax, dword ptr x[0]
			l_ret :
	}
}

__forceinline int popcnt(uint64 x) {
	unsigned int x1, x2;
	x1 = (unsigned int)(x & 0xFFFFFFFF);
	x1 -= (x1 >> 1) & 0x55555555;
	x1 = (x1 & 0x33333333) + ((x1 >> 2) & 0x33333333);
	x1 = (x1 + (x1 >> 4)) & 0x0F0F0F0F;
	x2 = (unsigned int)(x >> 32);
	x2 -= (x2 >> 1) & 0x55555555;
	x2 = (x2 & 0x33333333) + ((x2 >> 2) & 0x33333333);
	x2 = (x2 + (x2 >> 4)) & 0x0F0F0F0F;
	return ((x1 * 0x01010101) >> 24) + ((x2 * 0x01010101) >> 24);
}

template <bool HPopCnt> __forceinline int popcount(uint64 x) {
	return HPopCnt ? (__popcnt((int)x) + __popcnt(x >> 32)) : popcnt(x);
}
#endif

__forceinline int MinF(int x, int y) { return Min(x, y); }
__forceinline int MaxF(int x, int y) { return Max(x, y); }
__forceinline double MinF(double x, double y) { return Min(x, y); }
__forceinline double MaxF(double x, double y) { return Max(x, y); }

uint64 BMagicAttacks(int i, uint64 occ) {
    uint64 att = 0;
    for (uint64 u = BMask[i]; T(u); Cut(u)) if (F(Between[i][lsb(u)] & occ)) att |= Between[i][lsb(u)] | Bit(lsb(u));
	return att;
}

uint64 RMagicAttacks(int i, uint64 occ) {
    uint64 att = 0;
    for (uint64 u = RMask[i]; T(u); Cut(u)) if (F(Between[i][lsb(u)] & occ)) att |= Between[i][lsb(u)] | Bit(lsb(u));
	return att;
}

uint16 rand16() {
	seed = (seed * Convert(6364136223846793005,uint64)) + Convert(1442695040888963407,uint64);
	return Convert((seed >> 32) & 0xFFFF,uint16);
}

uint64 random() {
	uint64 key = Convert(rand16(),uint64); key <<= 16;
	key |= Convert(rand16(),uint64); key <<= 16;
	key |= Convert(rand16(),uint64); key <<= 16;
	return key | Convert(rand16(),uint64);
}

void init_misc() {
	int i, j, k, l, n;
	uint64 u;

	for (i = 0; i < 64; i++) {
		HLine[i] = VLine[i] = NDiag[i] = SDiag[i] = RMask[i] = BMask[i] = QMask[i] = 0;
		BMagicMask[i] = RMagicMask[i] = NAtt[i] = SArea[i] = DArea[i] = NArea[i] = 0;
		PAtt[0][i] = PAtt[1][i] = PMove[0][i] = PMove[1][i] = PWay[0][i] = PWay[1][i] = PSupport[0][i] = PSupport[1][i] = BishopForward[0][i] = BishopForward[1][i] = 0;
		for (j = 0; j < 64; j++) Between[i][j] = FullLine[i][j] = 0;
	}

	for (i = 0; i < 64; i++) for (j = 0; j < 64; j++) if (i != j) {
		u = Bit(j);
		if (File(i) == File(j)) VLine[i] |= u;
		if (Rank(i) == Rank(j)) HLine[i] |= u;
		if (NDiag(i) == NDiag(j)) NDiag[i] |= u;
		if (SDiag(i) == SDiag(j)) SDiag[i] |= u;
		if (Dist(i,j) <= 2) {
			DArea[i] |= u;
			if (Dist(i,j) <= 1) SArea[i] |= u;
			if (Abs(Rank(i)-Rank(j)) + Abs(File(i)-File(j)) == 3) NAtt[i] |= u;
		}
		if (j == i + 8) PMove[0][i] |= u;
		if (j == i - 8) PMove[1][i] |= u;
		if (Abs(File(i) - File(j)) == 1) {
			if (Rank(j) >= Rank(i)) {
				PSupport[1][i] |= u;
				if (Rank(j) - Rank(i) == 1) PAtt[0][i] |= u;
			} 
			if (Rank(j) <= Rank(i)) {
				PSupport[0][i] |= u;
				if (Rank(i) - Rank(j) == 1) PAtt[1][i] |= u;
			}
		} else if (File(i) == File(j)) {
			if (Rank(j) > Rank(i)) PWay[0][i] |= u;
			else PWay[1][i] |= u;
		}
	}
	for (i = 0; i < 64; i++) {
		RMask[i] = HLine[i] | VLine[i];
		BMask[i] = NDiag[i] | SDiag[i];
		QMask[i] = RMask[i] | BMask[i];
		BMagicMask[i] = BMask[i] & Interior;
		RMagicMask[i] = RMask[i];
		if (File(i) > 0) RMagicMask[i] &= ~File[0];
		if (Rank(i) > 0) RMagicMask[i] &= ~Line[0];
		if (File(i) < 7) RMagicMask[i] &= ~File[7];
		if (Rank(i) < 7) RMagicMask[i] &= ~Line[7];
		for (j = 0; j < 64; j++) if (NAtt[i] & NAtt[j]) Add(NArea[i],j);
	}
	for (i = 0; i < 8; i++) {
		West[i] = 0;
		East[i] = 0;
		Forward[0][i] = Forward[1][i] = 0;
		PIsolated[i] = 0;
		for (j = 0; j < 8; j++) {
			if (i < j) Forward[0][i] |= Line[j];
			else if (i > j) Forward[1][i] |= Line[j];
			if (i < j) East[i] |= File[j];
			else if (i > j) West[i] |= File[j];
		}
		if (i > 0) PIsolated[i] |= File[i - 1];
		if (i < 7) PIsolated[i] |= File[i + 1];
	}
	for (i = 0; i < 64; i++) {
		for (u = QMask[i]; T(u); Cut(u)) {
			j = lsb(u);
			k = Sgn(Rank(j)-Rank(i));
			l = Sgn(File(j)-File(i));
			for (n = i + 8 * k + l; n != j; n += (8 * k + l)) Add(Between[i][j],n);
		}
		for (u = BMask[i]; T(u); Cut(u)) {
			j = lsb(u);
			FullLine[i][j] = BMask[i] & BMask[j];
		}
		for (u = RMask[i]; T(u); Cut(u)) {
			j = lsb(u);
			FullLine[i][j] = RMask[i] & RMask[j];
		}
		BishopForward[0][i] |= PWay[0][i];
		BishopForward[1][i] |= PWay[1][i];
		for (j = 0; j < 64; j++) {
			if ((PWay[1][j] | Bit(j)) & BMask[i] & Forward[0][Rank(i)]) BishopForward[0][i] |= Bit(j);
			if ((PWay[0][j] | Bit(j)) & BMask[i] & Forward[1][Rank(i)]) BishopForward[1][i] |= Bit(j);
		}
	}

    for (i = 0; i < 16; i++) for (j = 0; j < 16; j++) {
		if (j < WhitePawn) MvvLva[i][j] = 0;
		else if (j < WhiteKnight) MvvLva[i][j] = PawnCaptureMvvLva(i) << 26;
		else if (j < WhiteLight) MvvLva[i][j] = KnightCaptureMvvLva(i) << 26;
		else if (j < WhiteRook) MvvLva[i][j] = BishopCaptureMvvLva(i) << 26;
		else if (j < WhiteQueen) MvvLva[i][j] = RookCaptureMvvLva(i) << 26;
		else MvvLva[i][j] = QueenCaptureMvvLva(i) << 26;
	}

	for (i = 0; i < 256; i++) PieceFromChar[i] = 0;
    PieceFromChar[66] = 6; PieceFromChar[75] = 14; PieceFromChar[78] = 4; PieceFromChar[80] = 2; PieceFromChar[81] = 12; PieceFromChar[82] = 10;
    PieceFromChar[98] = 7; PieceFromChar[107] = 15; PieceFromChar[110] = 5; PieceFromChar[112] = 3; PieceFromChar[113] = 13; PieceFromChar[114] = 11;

	TurnKey = random();
	for (i = 0; i < 8; i++) EPKey[i] = random();
	for (i = 0; i < 16; i++) CastleKey[i] = random();
	for (i = 0; i < 16; i++) for (j = 0; j < 64; j++) {
		if (i == 0) PieceKey[i][j] = 0;
		else PieceKey[i][j] = random();
	}
	for (i = 0; i < 16; i++) LogDist[i] = (int)(10.0 * log(1.01 + (double)i));
}

void init_magic() {
	int i, j, k, index, bits, bit_list[16];
	uint64 u;
#ifdef TUNER
	MagicAttacks = (uint64*)malloc(magic_size * sizeof(uint64));
#endif
	for (i = 0; i < 64; i++) {
		bits = 64 - BShift[i];
		for (u = BMagicMask[i], j = 0; T(u); Cut(u), j++) bit_list[j] = lsb(u);
		for (j = 0; j < Bit(bits); j++) {
			u = 0;
			for (k = 0; k < bits; k++)
				if (Odd(j >> k)) Add(u,bit_list[k]);
#ifndef HNI
			index = Convert(BOffset[i] + ((BMagic[i] * u) >> BShift[i]),int);
#else
			index = Convert(BOffset[i] + _pext_u64(u,BMagicMask[i]),int);
#endif
            MagicAttacks[index] = BMagicAttacks(i,u);
		}
		bits = 64 - RShift[i];
		for (u = RMagicMask[i], j = 0; T(u); Cut(u), j++) bit_list[j] = lsb(u);
		for (j = 0; j < Bit(bits); j++) {
			u = 0;
			for (k = 0; k < bits; k++)
				if (Odd(j >> k)) Add(u,bit_list[k]);
#ifndef HNI
			index = Convert(ROffset[i] + ((RMagic[i] * u) >> RShift[i]),int);
#else
			index = Convert(ROffset[i] + _pext_u64(u,RMagicMask[i]),int);
#endif
             MagicAttacks[index] = RMagicAttacks(i,u);
		}	
	}
}

void gen_kpk() {
	int turn, wp, wk, bk, to, cnt, old_cnt, un;
	uint64 bwp, bwk, bbk, u;
	uint8 Kpk_gen[2][64][64][64];

	memset(Kpk_gen, 0, 2 * 64 * 64 * 64);

	cnt = 0;
	old_cnt = 1;
start:
	if (cnt == old_cnt) goto end;
	old_cnt = cnt;
	cnt = 0;
	for (turn = 0; turn < 2; turn++) {
		for (wp = 0; wp < 64; wp++) {
			for (wk = 0; wk < 64; wk++) {
				for (bk = 0; bk < 64; bk++) {
					if (Kpk_gen[turn][wp][wk][bk]) continue;
					cnt++;
					if (wp < 8 || wp >= 56) goto set_draw;
					if (wp == wk || wk == bk || bk == wp) goto set_draw;
					bwp = Bit(wp);
					bwk = Bit(wk);
					bbk = Bit(bk);
					if (PAtt[White][wp] & bbk) {
						if (turn == White) goto set_draw;
						else if (F(SArea[wk] & bwp)) goto set_draw;
					}
					un = 0;
					if (turn == Black) {
						u = SArea[bk] & (~(SArea[wk] | PAtt[White][wp]));
						if (F(u)) goto set_draw;
						for (; T(u); Cut(u)) {
							to = lsb(u);
							if (Kpk_gen[turn ^ 1][wp][wk][to] == 1) goto set_draw;
							else if (Kpk_gen[turn ^ 1][wp][wk][to] == 0) un++;
						}
						if (F(un)) goto set_win;
					}
					else {
						for (u = SArea[wk] & (~(SArea[bk] | bwp)); T(u); Cut(u)) {
							to = lsb(u);
							if (Kpk_gen[turn ^ 1][wp][to][bk] == 2) goto set_win;
							else if (Kpk_gen[turn ^ 1][wp][to][bk] == 0) un++;
						}
						to = wp + 8;
						if (to != wk && to != bk) {
							if (to >= 56) {
								if (F(SArea[to] & bbk)) goto set_win;
								if (SArea[to] & bwk) goto set_win;
							}
							else {
								if (Kpk_gen[turn ^ 1][to][wk][bk] == 2) goto set_win;
								else if (Kpk_gen[turn ^ 1][to][wk][bk] == 0) un++;
								if (to < 24) {
									to += 8;
									if (to != wk && to != bk) {
										if (Kpk_gen[turn ^ 1][to][wk][bk] == 2) goto set_win;
										else if (Kpk_gen[turn ^ 1][to][wk][bk] == 0) un++;
									}
								}
							}
						}
						if (F(un)) goto set_draw;
					}
					continue;
				set_draw:
					Kpk_gen[turn][wp][wk][bk] = 1;
					continue;
				set_win:
					Kpk_gen[turn][wp][wk][bk] = 2;
					continue;
				}
			}
		}
	}
	if (cnt) goto start;
end:
	for (turn = 0; turn < 2; turn++) {
		for (wp = 0; wp < 64; wp++) {
			for (wk = 0; wk < 64; wk++) {
				Kpk[turn][wp][wk] = 0;
				for (bk = 0; bk < 64; bk++) {
					if (Kpk_gen[turn][wp][wk][bk] == 2) Kpk[turn][wp][wk] |= Bit(bk);
				}
			}
		}
	}
}

void init_pst() {
	int i, j, k, op, eg, index, r, f, d, e, distQ[4], distL[4], distM[2];
	memset(Pst,0,16 * 64 * sizeof(int));

	for (i = 0; i < 64; i++) {
		r = Rank(i);
		f = File(i);
		d = Abs(f - r);
		e = Abs(f + r - 7);
		distQ[0] = DistC[f] * DistC[f]; distL[0] = DistC[f];
		distQ[1] = DistC[r] * DistC[r]; distL[1] = DistC[r];
		distQ[2] = RankR[d] * RankR[d] + RankR[e] * RankR[e]; distL[2] = RankR[d] + RankR[e];
		distQ[3] = RankR[r] * RankR[r]; distL[3] = RankR[r];
		distM[0] = DistC[f] * DistC[r]; distM[1] = DistC[f] * RankR[r];
		for (j = 2; j < 16; j += 2) {
			index = PieceType[j];
			op = eg = 0;
			for (k = 0; k < 2; k++) {
				op += Av(PstQuadMixedWeights, 4, index, (k * 2)) * distM[k];
				eg += Av(PstQuadMixedWeights, 4, index, (k * 2) + 1) * distM[k];
			}
			for (k = 0; k < 4; k++) {
				op += Av(PstQuadWeights,8,index,(k * 2)) * distQ[k];
				eg += Av(PstQuadWeights,8,index,(k * 2) + 1) * distQ[k];
				op += Av(PstLinearWeights,8,index,(k * 2)) * distL[k];
				eg += Av(PstLinearWeights,8,index,(k * 2) + 1) * distL[k];
			}
			Pst(j,i) = Compose(op/64, eg/64);
		}
	}

	Pst(WhiteKnight,56) -= Compose(100, 0);
	Pst(WhiteKnight,63) -= Compose(100, 0);
	for (i = 0; i < 64; i++) {
		for (j = 3; j < 16; j+=2) {
			op = Opening(Pst(j-1,63-i));
			eg = Endgame(Pst(j-1,63-i));
			Pst(j,i) = Compose(-op,-eg);
		}
	}
	Current->pst = 0;
	for (i = 0; i < 64; i++)
	if (Square(i)) Current->pst += Pst(Square(i),i);
}

void init_eval() {
	int i, j, k, index;
	memset(Mobility,0,4 * 32 * sizeof(int));
	for (i = 0; i < 4; i++) for (j = 0; j < 32; j++) {
		index = i * 2;
		double op = (double)(Av(MobilityLinear,8,0,index) * j) + log(1.01 + (double)j) * (double)(Av(MobilityLog,8,0,index));
		index = i * 2 + 1;
		double eg = (double)(Av(MobilityLinear,8,0,index) * j) + log(1.01 + (double)j) * (double)(Av(MobilityLog,8,0,index));
		Mobility[i][j] = Compose((int)(op/64.0),(int)(eg/64.0));
	}
	
	for (i = 0; i < 3; i++) for (j = 7; j >= 0; j--) {
		Shelter[i][j] = 0;
		if (j > 1) for (k = 1; k < Min(j, 5); k++) Shelter[i][j] += Av(ShelterValue, 0, 0, (i * 5) + k - 1);
		if (!j) Shelter[i][j] = Shelter[i][7] + Av(ShelterValue, 0, 0, (i * 5) + 4);
	}

	for (i = 0; i < 4; i++) {
		StormBlocked[i] = ((Sa(StormQuad, StormBlockedMul) * i * i) + (Sa(StormLinear, StormBlockedMul) * (i + 1))) / 100;
		StormShelterAtt[i] = ((Sa(StormQuad, StormShelterAttMul) * i * i) + (Sa(StormLinear, StormShelterAttMul) * (i + 1))) / 100;
		StormConnected[i] = ((Sa(StormQuad, StormConnectedMul) * i * i) + (Sa(StormLinear, StormConnectedMul) * (i + 1))) / 100;
		StormOpen[i] = ((Sa(StormQuad, StormOpenMul) * i * i) + (Sa(StormLinear, StormOpenMul) * (i + 1))) / 100;
		StormFree[i] = ((Sa(StormQuad, StormFreeMul) * i * i) + (Sa(StormLinear, StormFreeMul) * (i + 1))) / 100;
	}

	for (i = 0; i < 8; i++) {
		int l = Max(i - 2, 0);
		int q = l * l;
		PasserGeneral[i] = Compose16(Av(PasserQuad, 2, 0, 0) * q + Av(PasserLinear, 2, 0, 0) * l, Av(PasserQuad, 2, 0, 1) * q + Av(PasserLinear, 2, 0, 1) * l);
		PasserBlocked[i] = Compose16(Av(PasserQuad, 2, 1, 0) * q + Av(PasserLinear, 2, 1, 0) * l, Av(PasserQuad, 2, 1, 1) * q + Av(PasserLinear, 2, 1, 1) * l);
		PasserFree[i] = Compose16(Av(PasserQuad, 2, 2, 0) * q + Av(PasserLinear, 2, 2, 0) * l, Av(PasserQuad, 2, 2, 1) * q + Av(PasserLinear, 2, 2, 1) * l);
		PasserSupported[i] = Compose16(Av(PasserQuad, 2, 3, 0) * q + Av(PasserLinear, 2, 3, 0) * l, Av(PasserQuad, 2, 3, 1) * q + Av(PasserLinear, 2, 3, 1) * l);
		PasserProtected[i] = Compose16(Av(PasserQuad, 2, 4, 0) * q + Av(PasserLinear, 2, 4, 0) * l, Av(PasserQuad, 2, 4, 1) * q + Av(PasserLinear, 2, 4, 1) * l);
		PasserConnected[i] = Compose16(Av(PasserQuad, 2, 5, 0) * q + Av(PasserLinear, 2, 5, 0) * l, Av(PasserQuad, 2, 5, 1) * q + Av(PasserLinear, 2, 5, 1) * l);
		PasserOutside[i] = Compose16(Av(PasserQuad, 2, 6, 0) * q + Av(PasserLinear, 2, 6, 0) * l, Av(PasserQuad, 2, 6, 1) * q + Av(PasserLinear, 2, 6, 1) * l);
		PasserCandidate[i] = Compose16(Av(PasserQuad, 2, 7, 0) * q + Av(PasserLinear, 2, 7, 0) * l, Av(PasserQuad, 2, 7, 1) * q + Av(PasserLinear, 2, 7, 1) * l);
		PasserClear[i] = Compose16(Av(PasserQuad, 2, 8, 0) * q + Av(PasserLinear, 2, 8, 0) * l, Av(PasserQuad, 2, 8, 1) * q + Av(PasserLinear, 2, 8, 1) * l);

		PasserAtt[i] = Av(PasserAttDefQuad, 2, 0, 0) * q + Av(PasserAttDefLinear, 2, 0, 0) * l;
		PasserDef[i] = Av(PasserAttDefQuad, 2, 1, 0) * q + Av(PasserAttDefLinear, 2, 1, 0) * l;
		PasserAttLog[i] = Av(PasserAttDefQuad, 2, 0, 1) * q + Av(PasserAttDefLinear, 2, 0, 1) * l;
		PasserDefLog[i] = Av(PasserAttDefQuad, 2, 1, 1) * q + Av(PasserAttDefLinear, 2, 1, 1) * l;
	}
}

void calc_material(int index) {
	int pawns[2], knights[2], light[2], dark[2], rooks[2], queens[2], bishops[2], major[2], minor[2], tot[2], mat[2], mul[2], quad[2], score, phase, me, i = index;
#ifdef TUNER
	Material[index].generation = generation;
#endif
	queens[White] = i % 3; i /= 3;
	queens[Black] = i % 3; i /= 3;
	rooks[White] = i % 3; i /= 3;
	rooks[Black] = i % 3; i /= 3;
	light[White] = i % 2; i /= 2;
	light[Black] = i % 2; i /= 2;
	dark[White] = i % 2; i /= 2;
	dark[Black] = i % 2; i /= 2;
	knights[White] = i % 3; i /= 3;
	knights[Black] = i % 3; i /= 3;
	pawns[White] = i % 9; i /= 9;
	pawns[Black] = i % 9;
	for (me = 0; me < 2; me++) {
		bishops[me] = light[me] + dark[me];
		major[me] = rooks[me] + queens[me];
		minor[me] = bishops[me] + knights[me];
		tot[me] = 3 * minor[me] + 5 * rooks[me] + 9 * queens[me];
		mat[me] = mul[me] = 32;
		quad[me] = 0;
	}
	score = (SeeValue[WhitePawn] + Av(MatLinear, 0, 0, 0)) * (pawns[White] - pawns[Black]) + (SeeValue[WhiteKnight] + Av(MatLinear, 0, 0, 1)) * (knights[White] - knights[Black])
		+ (SeeValue[WhiteLight] + Av(MatLinear, 0, 0, 2)) * (bishops[White] - bishops[Black]) + (SeeValue[WhiteRook] + Av(MatLinear, 0, 0, 3)) * (rooks[White] - rooks[Black])
		+ (SeeValue[WhiteQueen] + Av(MatLinear, 0, 0, 4)) * (queens[White] - queens[Black]) + 50 * ((bishops[White] / 2) - (bishops[Black] / 2));
	phase = Phase[PieceType[WhitePawn]] * (pawns[White] + pawns[Black]) + Phase[PieceType[WhiteKnight]] * (knights[White] + knights[Black])
		+ Phase[PieceType[WhiteLight]] * (bishops[White] + bishops[Black]) + Phase[PieceType[WhiteRook]] * (rooks[White] + rooks[Black])
		+ Phase[PieceType[WhiteQueen]] * (queens[White] + queens[Black]);
	Material[index].phase = Min((Max(phase - PhaseMin, 0) * 128) / (PhaseMax - PhaseMin), 128);

	int special = 0;
	for (me = 0; me < 2; me++) {
		if (queens[me] == queens[opp]) {
			if (rooks[me] - rooks[opp] == 1) {
				if (knights[me] == knights[opp] && bishops[opp] - bishops[me] == 1) IncV(special, Ca(MatSpecial, MatRB));
				else if (bishops[me] == bishops[opp] && knights[opp] - knights[me] == 1) IncV(special, Ca(MatSpecial, MatRN));
				else if (knights[me] == knights[opp] && bishops[opp] - bishops[me] == 2) DecV(special, Ca(MatSpecial, MatBBR));
				else if (bishops[me] == bishops[opp] && knights[opp] - knights[me] == 2) DecV(special, Ca(MatSpecial, MatNNR));
				else if (bishops[opp] - bishops[me] == 1 && knights[opp] - knights[me] == 1) DecV(special, Ca(MatSpecial, MatBNR));
			} else if (rooks[me] == rooks[opp] && minor[me] - minor[opp] == 1) IncV(special, Ca(MatSpecial, MatM));
		} else if (queens[me] - queens[opp] == 1) {
			if (rooks[opp] - rooks[me] == 2 && minor[opp] - minor[me] == 0) IncV(special, Ca(MatSpecial, MatQRR));
			else if (rooks[opp] - rooks[me] == 1 && knights[opp] == knights[me] && bishops[opp] - bishops[me] == 1) IncV(special, Ca(MatSpecial, MatQRB));
			else if (rooks[opp] - rooks[me] == 1 && knights[opp] - knights[me] == 1 && bishops[opp] == bishops[me]) IncV(special, Ca(MatSpecial, MatQRN));
			else if ((major[opp] + minor[opp]) - (major[me] + minor[me]) >= 2) IncV(special, Ca(MatSpecial, MatQ3));
		}
	}
	score += (Opening(special) * Material[index].phase + Endgame(special) * (128 - (int)Material[index].phase)) / 128;

	for (me = 0; me < 2; me++) {
		quad[me] += pawns[me] * (pawns[me] * TrAv(MatQuadMe, 5, 0, 0) + knights[me] * TrAv(MatQuadMe, 5, 0, 1)
			+ bishops[me] * TrAv(MatQuadMe, 5, 0, 2) + rooks[me] * TrAv(MatQuadMe, 5, 0, 3) + queens[me] * TrAv(MatQuadMe, 5, 0, 4));
		quad[me] += knights[me] * (knights[me] * TrAv(MatQuadMe, 5, 1, 0)
			+ bishops[me] * TrAv(MatQuadMe, 5, 1, 1) + rooks[me] * TrAv(MatQuadMe, 5, 1, 2) + queens[me] * TrAv(MatQuadMe, 5, 1, 3));
		quad[me] += bishops[me] * (bishops[me] * TrAv(MatQuadMe, 5, 2, 0) + rooks[me] * TrAv(MatQuadMe, 5, 2, 1) + queens[me] * TrAv(MatQuadMe, 5, 2, 2));

		quad[me] += rooks[me] * (rooks[me] * TrAv(MatQuadMe, 5, 3, 0) + queens[me] * TrAv(MatQuadMe, 5, 3, 1));
		quad[me] += pawns[me] * (knights[opp] * TrAv(MatQuadOpp, 4, 0, 0)
			+ bishops[opp] * TrAv(MatQuadOpp, 4, 0, 1) + rooks[opp] * TrAv(MatQuadOpp, 4, 0, 2) + queens[opp] * TrAv(MatQuadOpp, 4, 0, 3));
		quad[me] += knights[me] * (bishops[opp] * TrAv(MatQuadOpp, 4, 1, 0) + rooks[opp] * TrAv(MatQuadOpp, 4, 1, 1) + queens[opp] * TrAv(MatQuadOpp, 4, 1, 2));
		quad[me] += bishops[me] * (rooks[opp] * TrAv(MatQuadOpp, 4, 2, 0) + queens[opp] * TrAv(MatQuadOpp, 4, 2, 1));
		quad[me] += rooks[me] * queens[opp] * TrAv(MatQuadOpp, 4, 3, 0);

		if (bishops[me] >= 2) quad[me] += pawns[me] * Av(BishopPairQuad, 0, 0, 0) + knights[me] * Av(BishopPairQuad, 0, 0, 1) + rooks[me] * Av(BishopPairQuad, 0, 0, 2)
			+ queens[me] * Av(BishopPairQuad, 0, 0, 3) + pawns[opp] * Av(BishopPairQuad, 0, 0, 4) + knights[opp] * Av(BishopPairQuad, 0, 0, 5)
			+ bishops[opp] * Av(BishopPairQuad, 0, 0, 6) + rooks[opp] * Av(BishopPairQuad, 0, 0, 7) + queens[opp] * Av(BishopPairQuad, 0, 0, 8);
	}
	score += (quad[White] - quad[Black]) / 100;

	for (me = 0; me < 2; me++) {
		if (tot[me] - tot[opp] <= 3) {
			if (!pawns[me]) {
				if (tot[me] <= 3) mul[me] = 0;
				if (tot[me] == tot[opp] && major[me] == major[opp] && minor[me] == minor[opp]) mul[me] = major[me] + minor[me] <= 2 ? 0 : (major[me] + minor[me] <= 3 ? 16 : 32);
				else if (minor[me] + major[me] <= 2) {
					if (bishops[me] < 2) mat[me] = (bishops[me] && rooks[me]) ? 8 : 1;
					else if (bishops[opp] + rooks[opp] >= 1) mat[me] = 1;
					else mat[me] = 32;
				} else if (tot[me] - tot[opp] < 3 && minor[me] + major[me] - minor[opp] - major[opp] <= 1) mat[me] = 4;
				else if (minor[me] + major[me] <= 3) mat[me] = 8 * (1 + bishops[me]);
				else mat[me] = 8 * (2 + bishops[me]);
			}
			if (pawns[me] <= 1) {
				mul[me] = Min(28, mul[me]);
				if (rooks[me] == 1 && queens[me] + minor[me] == 0 && rooks[opp] == 1) mat[me] = Min(23, mat[me]);
			}
		}
		if (!major[me]) {
			if (!minor[me]) {
				if (!tot[me] && pawns[me] < pawns[opp]) DecV(score, (pawns[opp] - pawns[me]) * SeeValue[WhitePawn]);
			} else if (minor[me] == 1) {
				if (pawns[me] <= 1 && minor[opp] >= 1) mat[me] = 1;
				if (bishops[me] == 1) {
					if (minor[opp] == 1 && bishops[opp] == 1 && light[me] != light[opp]) {
						mul[me] = Min(mul[me], 15);
						if (pawns[me] - pawns[opp] <= 1) mul[me] = Min(mul[me], 11);
					}
				}
			} else if (!pawns[me] && knights[me] == 2 && !bishops[me]) {
				if (!tot[opp] && pawns[opp]) mat[me] = 6;
				else mul[me] = 0;
			}
		}
		if (!mul[me]) mat[me] = 0;
		if (mat[me] <= 1 && tot[me] != tot[opp]) mul[me] = Min(mul[me], 8);
	}
	if (bishops[White] == 1 && bishops[Black] == 1 && light[White] != light[Black]) {
		mul[White] = Min(mul[White], 24 + 2 * (knights[Black] + major[Black]));
		mul[Black] = Min(mul[Black], 24 + 2 * (knights[White] + major[White]));
	} else if (!minor[White] && !minor[Black] && major[White] == 1 && major[Black] == 1 && rooks[White] == rooks[Black]) {
		mul[White] = Min(mul[White], 25);
		mul[Black] = Min(mul[Black], 25);
	}
	for (me = 0; me < 2; me++) {
		Material[index].mul[me] = mul[me];
		Material[index].pieces[me] = major[me] + minor[me];
	}
	if (score > 0) score = (score * mat[White]) / 32;
	else score = (score * mat[Black]) / 32;
	Material[index].score = score;
	for (me = 0; me < 2; me++) {
		if (major[me] == 0 && minor[me] == bishops[me] && minor[me] <= 1) Material[index].flags |= VarC(FlagSingleBishop, me);
		if (((major[me] == 0 || minor[me] == 0) && major[me] + minor[me] <= 1) || major[opp] + minor[opp] == 0
			|| (!pawns[me] && major[me] == rooks[me] && major[me] == 1 && minor[me] == bishops[me] && minor[me] == 1 && rooks[opp] == 1 && !minor[opp] && !queens[opp])) Material[index].flags |= VarC(FlagCallEvalEndgame, me);
	}
}

void init_material() {
#ifdef TUNER
	Material = (GMaterial*)malloc(TotalMat * sizeof(GMaterial));
#endif
	memset(Material,0,TotalMat * sizeof(GMaterial));
	for (int index = 0; index < TotalMat; index++) calc_material(index);
}

void init_hash() {
#ifdef TUNER
	return;
#endif
	char name[256];
	sint64 size = (hash_size * sizeof(GEntry));
	sprintf(name, "GULL_HASH_%d", WinParId);
	int initialized = 0;
	if (parent && HASH != NULL) {
		initialized = 1;
		UnmapViewOfFile(Hash);
		CloseHandle(HASH);
	}
	if (parent) {
		if (!LargePages) goto no_lp;
#ifndef LARGE_PAGES
		goto no_lp;
#endif
		typedef int(*GETLARGEPAGEMINIMUM)(void);
		GETLARGEPAGEMINIMUM pGetLargePageMinimum;
		HINSTANCE hDll = LoadLibrary(TEXT("kernel32.dll"));
		if (hDll == NULL) goto no_lp;
		pGetLargePageMinimum = (GETLARGEPAGEMINIMUM)GetProcAddress(hDll, "GetLargePageMinimum");
		if (pGetLargePageMinimum == NULL) goto no_lp;
		int min_page_size = (*pGetLargePageMinimum)();
		if (size < min_page_size) size = min_page_size;
		if (!initialized) {
			TOKEN_PRIVILEGES tp;
			HANDLE hToken;
			OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken);
			LookupPrivilegeValue(NULL, "SeLockMemoryPrivilege", &tp.Privileges[0].Luid);
			tp.PrivilegeCount = 1;
			tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
			AdjustTokenPrivileges(hToken, FALSE, &tp, 0, (PTOKEN_PRIVILEGES)NULL, 0);
		}
		HASH = NULL;
		HASH = CreateFileMapping(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE | SEC_COMMIT | SEC_LARGE_PAGES, size >> 32, size & 0xFFFFFFFF, name);
		if (HASH != NULL) {
			fprintf(stdout, "Large page hash\n");
			goto hash_allocated;
		}
	no_lp:
		HASH = CreateFileMapping(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, size >> 32, size & 0xFFFFFFFF, name);
	} else HASH = OpenFileMapping(FILE_MAP_ALL_ACCESS, 0, name);
hash_allocated:
	Hash = (GEntry*)MapViewOfFile(HASH, FILE_MAP_ALL_ACCESS, 0, 0, size);
	if (parent) memset(Hash, 0, size);
	hash_mask = hash_size - 4;
}

void init_shared() {
#ifdef TUNER
	return;
#endif
	char name[256];
	sint64 size = SharedPVHashOffset + pv_hash_size * sizeof(GPVEntry);
	sprintf(name, "GULL_SHARED_%d", WinParId);
	if (parent && SHARED != NULL) {
		UnmapViewOfFile(Smpi);
		CloseHandle(SHARED);
	}
	if (parent) SHARED = CreateFileMapping(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, size, name);
	else SHARED = OpenFileMapping(FILE_MAP_ALL_ACCESS, 0, name);
	Smpi = (GSMPI*)MapViewOfFile(SHARED, FILE_MAP_ALL_ACCESS, 0, 0, size);
	if (parent) memset(Smpi, 0, size);
	Material = (GMaterial*)(((char*)Smpi) + SharedMaterialOffset);
	MagicAttacks = (uint64*)(((char*)Smpi) + SharedMagicOffset);
	PVHash = (GPVEntry*)(((char*)Smpi) + SharedPVHashOffset);
	if (parent) memset(PVHash, 0, pv_hash_size * sizeof(GPVEntry));
}

void init() {
	init_shared();
	init_misc();
	if (parent) init_magic();
	for (int i = 0; i < 64; i++) {
		BOffsetPointer[i] = MagicAttacks + BOffset[i];
		ROffsetPointer[i] = MagicAttacks + ROffset[i];
	}
	gen_kpk();
	init_pst();
	init_eval();
	if (parent) init_material();
#ifdef EXPLAIN_EVAL
	memset(GullCppFile, 0, 16384 * 256);
	FILE * fcpp; fcpp = fopen(GullCpp, "r");
	for (cpp_length = 0; cpp_length < 16384; cpp_length++) {
		if (feof(fcpp)) break;
		fgets(mstring, 65536, fcpp);
		memcpy(GullCppFile[cpp_length], mstring, 256);
		if (!strchr(GullCppFile[cpp_length], '\n')) GullCppFile[cpp_length][255] = '\n';
		char * p; for (p = GullCppFile[cpp_length]; (*p) == '\t'; p++);
		strcpy(mstring, p); strcpy(GullCppFile[cpp_length], mstring);
	}
	cpp_length--;
	fclose(fcpp);
#endif
}

void init_search(int clear_hash) {
	memset(History,1,16 * 64 * sizeof(sint16));
	memset(Delta,0,16 * 4096 * sizeof(sint16));
	memset(Ref,0,16 * 64 * sizeof(GRef));
	memset(Data + 1, 0, 127 * sizeof(GData));
	if (clear_hash) {
		date = 0;
		date = 1;
		memset(Hash,0,hash_size * sizeof(GEntry));
		memset(PVHash,0,pv_hash_size * sizeof(GPVEntry));
	}
	get_board("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
	nodes = 0;
	best_move = best_score = 0;
	LastTime = LastValue = LastExactValue = InstCnt = 0;
	LastSpeed = 0;
	PVN = 1;
	Infinite = 1;
	SearchMoves = 0;
	TimeLimit1 = TimeLimit2 = 0;
	Stop = Searching = 0;
	if (MaxPrN > 1) ZERO_BIT_64(Smpi->searching, 0);
	DepthLimit = 128;
	LastDepth = 128;
	Print = 1;
	memset(CurrentSI,0,sizeof(GSearchInfo));
	memset(BaseSI,0,sizeof(GSearchInfo));
#ifdef CPU_TIMING
	GlobalTime[GlobalTurn] = UciBaseTime;
	GlobalInc[GlobalTurn] = UciIncTime;
#endif
}

void setup_board() {
	int i;
	uint64 occ;
	GEntry * Entry;
	GPVEntry * PVEntry;

	occ = 0;
	sp = 0;
	date++;
	if (date > 0x8000) { // musn't ever happen
		date = 2;
		// now GUI must wait for readyok... we have plenty of time :)
		for (Entry = Hash, i = 0; i < hash_size; i++, Entry++) Entry->date = 1;
		for (PVEntry = PVHash, i = 0; i < pv_hash_size; i++, PVEntry++) PVEntry->date = 1;
	}
	Current->material = 0;
	Current->pst = 0;
	Current->key = PieceKey[0][0];
	if (Current->turn) Current->key ^= TurnKey;
	Current->key ^= CastleKey[Current->castle_flags];
	if (Current->ep_square) Current->key ^= EPKey[File(Current->ep_square)];
	Current->pawn_key = 0;
	Current->pawn_key ^= CastleKey[Current->castle_flags];
	for (i = 0; i < 16; i++) BB(i) = 0;
	for (i = 0; i < 64; i++) {
		if (Square(i)) {
		    Add(BB(Square(i)),i);
		    Add(BB(Square(i) & 1),i);
		    Add(occ,i);
		    Current->key ^= PieceKey[Square(i)][i];
		    if (Square(i) < WhiteKnight) Current->pawn_key ^= PieceKey[Square(i)][i];
			if (Square(i) < WhiteKing) Current->material += MatCode[Square(i)];
			else Current->pawn_key ^= PieceKey[Square(i)][i];
			Current->pst += Pst(Square(i),i);
		}
	}
	if (popcnt(BB(WhiteKnight)) > 2 || popcnt(BB(WhiteLight)) > 1 || popcnt(BB(WhiteDark)) > 1 
		|| popcnt(BB(WhiteRook)) > 2 || popcnt(BB(WhiteQueen)) > 2) Current->material |= FlagUnusualMaterial; 
	if (popcnt(BB(BlackKnight)) > 2 || popcnt(BB(BlackLight)) > 1 || popcnt(BB(BlackDark)) > 1 
		|| popcnt(BB(BlackRook)) > 2 || popcnt(BB(BlackQueen)) > 2) Current->material |= FlagUnusualMaterial; 
	Current->capture = 0;
	Current->killer[1] = Current->killer[2] = 0;
	Current->ply = 0;
	Stack[sp] = Current->key;
}

void get_board(const char fen[]) {
	int pos, i, j;
	unsigned char c;

	Current = Data;
	memset(Board,0,sizeof(GBoard));
	memset(Current,0,sizeof(GData));
	pos = 0;
	c = fen[pos];
	while (c == ' ') c = fen[++pos];
	for (i = 56; i >= 0; i -= 8) {
		for (j = 0; j <= 7; ) {
            if (c <= '8') j += c - '0';
			else {
				Square(i+j) = PieceFromChar[c];
				if (Even(SDiag(i+j)) && (Square(i+j)/2) == 3) Square(i+j) += 2;
				j++;
			}
			c = fen[++pos];
		}
		c = fen[++pos];
	}
	if (c == 'b') Current->turn = 1;
	c = fen[++pos]; c = fen[++pos];
    if (c == '-') c = fen[++pos];
	if (c == 'K') { Current->castle_flags |= CanCastle_OO; c = fen[++pos]; }
	if (c == 'Q') { Current->castle_flags |= CanCastle_OOO; c = fen[++pos]; }
	if (c == 'k') { Current->castle_flags |= CanCastle_oo; c = fen[++pos]; }
	if (c == 'q') { Current->castle_flags |= CanCastle_ooo; c = fen[++pos]; }
	c = fen[++pos];
	if (c != '-') {
        i = c + fen[++pos] * 8 - 489;
		j = i ^ 8;
		if (Square(i) != 0) i = 0;
		else if (Square(j) != (3 - Current->turn)) i = 0;
		else if (Square(j-1) != (Current->turn+2) && Square(j+1) != (Current->turn+2)) i = 0;
		Current->ep_square = i;
	}
	setup_board();
}

__forceinline GEntry * probe_hash() {
	for (GEntry * Entry = Hash + (High32(Current->key) & hash_mask); Entry < (Hash + (High32(Current->key) & hash_mask)) + 4; Entry++) if (Low32(Current->key) == Entry->key) {
		Entry->date = date;
		return Entry;
	}
	return NULL;
}

__forceinline GPVEntry * probe_pv_hash() {
	for (GPVEntry * PVEntry = PVHash + (High32(Current->key) & pv_hash_mask); PVEntry < PVHash + (High32(Current->key) & pv_hash_mask) + pv_cluster_size; PVEntry++) if (Low32(Current->key) == PVEntry->key) {
		PVEntry->date = date;
		return PVEntry;
	}
	return NULL;
}

void move_to_string(int move, char string[]) { 
	int pos = 0;
    string[pos++] = ((move >> 6) & 7) + 'a';
    string[pos++] = ((move >> 9) & 7) + '1';
    string[pos++] = (move & 7) + 'a';
    string[pos++] = ((move >> 3) & 7) + '1';
    if (IsPromotion(move)) {
        if ((move & 0xF000) == FlagPQueen)  string[pos++] = 'q';
        else if ((move & 0xF000) == FlagPRook)   string[pos++] = 'r';
        else if ((move & 0xF000) == FlagPLight || (move & 0xF000) == FlagPDark) string[pos++] = 'b';
        else if ((move & 0xF000) == FlagPKnight) string[pos++] = 'n';
    }
    string[pos] = 0;
}

int move_from_string(char string[]) { 
	int from, to, move;
    from = ((string[1] - '1') * 8) + (string[0] - 'a');
    to  = ((string[3] - '1') * 8) + (string[2] - 'a');
    move = (from << 6) | to;
    if (Board->square[from] >= WhiteKing && Abs(to - from) == 2) move |= FlagCastling;
    if (T(Current->ep_square) && to == Current->ep_square) move |= FlagEP;
    if (string[4] != 0) {
        if (string[4] == 'q') move |= FlagPQueen;
        else if (string[4] == 'r') move |= FlagPRook;
        else if (string[4] == 'b') {
			if (Odd(to ^ Rank(to))) move |= FlagPLight;
			else move |= FlagPDark;
		} else if (string[4] == 'n') move |= FlagPKnight;
    }
    return move;
}

void pick_pv() {
	GEntry * Entry;
	GPVEntry * PVEntry;
	int i, depth, move;
	if (pvp >= Min(pv_length,64)) {
		PV[pvp] = 0;
		return;
	}
	move = 0;
	depth = -256;
	if (Entry = probe_hash()) if (T(Entry->move) && Entry->low_depth > depth) {
		depth = Entry->low_depth;
		move = Entry->move;
	}
	if (PVEntry = probe_pv_hash()) if (T(PVEntry->move) && PVEntry->depth > depth) {
		depth = PVEntry->depth;
		move = PVEntry->move;
	}
	evaluate();
	if (Current->att[Current->turn] & King(Current->turn ^ 1)) PV[pvp] = 0;
	else if (move && (Current->turn ? is_legal<1>(move) : is_legal<0>(move))) {
		PV[pvp] = move;
		pvp++;
		if (Current->turn) do_move<1>(move);
		else do_move<0>(move);
		if (Current->ply >= 100) goto finish;
		for (i = 4; i <= Current->ply; i+= 2) if (Stack[sp-i] == Current->key) {
			PV[pvp] = 0;
			goto finish;
		}
		pick_pv();
finish:
		if (Current->turn ^ 1) undo_move<1>(move);
		else undo_move<0>(move);
	} else PV[pvp] = 0;
}

template <bool me> int draw_in_pv() {
	if ((Current - Data) >= 126) return 1;
	if (Current->ply >= 100) return 1;
	for (int i = 4; i <= Current->ply; i += 2) if (Stack[sp - i] == Current->key) return 1;
	if (GPVEntry * PVEntry = probe_pv_hash()) {
		if (!PVEntry->value) return 1;
		if (int move = PVEntry->move) {
			do_move<me>(move);
			int value = draw_in_pv<opp>();
			undo_move<me>(move);
			return value;
		}
	}
	return 0;
}

template <bool me> void do_move(int move) {
	GEntry * Entry;
	GPawnEntry * PawnEntry;
	int from, to, piece, capture;
	GData * Next;
	uint64 u, mask_from, mask_to;

	to = To(move);
	Next = Current + 1;
	Next->ep_square = 0;
	capture = Square(to);
    if (F(capture)) {
		Next->capture = 0;
		goto non_capture;
    }
	from = From(move);
	piece = Square(from);
	Next->turn = opp;
	Next->capture = capture;
	Square(from) = 0;
	Square(to) = piece;
	Next->piece = piece;
	mask_from = Bit(from);
	mask_to = Bit(to);
	BB(piece) ^= mask_from;
	Piece(me) ^= mask_from;
	BB(capture) ^= mask_to;
	Piece(opp) ^= mask_to;
	BB(piece) |= mask_to;
	Piece(me) |= mask_to;
	Next->castle_flags = Current->castle_flags & UpdateCastling[to] & UpdateCastling[from];
	Next->pst = Current->pst + Pst(piece,to) - Pst(piece,from) - Pst(capture,to);
	Next->key = Current->key ^ PieceKey[piece][from] ^ PieceKey[piece][to] ^ PieceKey[capture][to] ^ CastleKey[Current->castle_flags] ^ CastleKey[Next->castle_flags];
	if (capture != IPawn(opp)) Next->pawn_key = Current->pawn_key ^ CastleKey[Current->castle_flags] ^ CastleKey[Next->castle_flags]; // of course we can put a lot of operations inside this "if {}" but the speedup won't be worth the effort
	else Next->pawn_key = Current->pawn_key ^ PieceKey[IPawn(opp)][to] ^ CastleKey[Current->castle_flags] ^ CastleKey[Next->castle_flags];
	Next->material = Current->material - MatCode[capture];
	if (T(Current->material & FlagUnusualMaterial) && capture >= WhiteKnight) {
		if (popcnt(BB(WhiteQueen)) <= 2 && popcnt(BB(BlackQueen)) <= 2) {
			if (popcnt(BB(WhiteLight)) <= 1 && popcnt(BB(BlackLight)) <= 1 && popcnt(BB(WhiteKnight)) <= 2
				&& popcnt(BB(BlackKnight)) <= 2 && popcnt(BB(WhiteRook)) <= 2 && popcnt(BB(BlackRook)) <= 2)
				Next->material ^= FlagUnusualMaterial;
		}
	}
	if (piece == IPawn(me)) {
		Next->pawn_key ^= PieceKey[IPawn(me)][from] ^ PieceKey[piece][to];
		if (IsPromotion(move)) {
			piece = Promotion(move,me);
			Square(to) = piece;
		    Next->material += MatCode[piece] - MatCode[IPawn(me)];
			if (piece < WhiteRook) {
				if (piece >= WhiteLight && T(BB(piece))) Next->material |= FlagUnusualMaterial;
				if (Multiple(BB(piece))) Next->material |= FlagUnusualMaterial;
			} else if (Multiple(BB(piece))) Next->material |= FlagUnusualMaterial;
			Pawn(me) ^= mask_to;
			BB(piece) |= mask_to;
			Next->pst += Pst(piece,to) - Pst(IPawn(me),to);
			Next->key ^= PieceKey[piece][to] ^ PieceKey[IPawn(me)][to];
			Next->pawn_key ^= PieceKey[IPawn(me)][to];
		}
		PawnEntry = PawnHash + (Next->pawn_key & pawn_hash_mask);
	    prefetch((char *)PawnEntry,_MM_HINT_NTA);
	} else if (piece >= WhiteKing) {
		Next->pawn_key ^= PieceKey[piece][from] ^ PieceKey[piece][to];
		PawnEntry = PawnHash + (Next->pawn_key & pawn_hash_mask);
	    prefetch((char *)PawnEntry,_MM_HINT_NTA);
	} else if (capture < WhiteKnight) {
		PawnEntry = PawnHash + (Next->pawn_key & pawn_hash_mask);
	    prefetch((char *)PawnEntry,_MM_HINT_NTA);
	}
	if (F(Next->material & FlagUnusualMaterial)) prefetch((char *)(Material + Next->material), _MM_HINT_NTA); 
	if (Current->ep_square) Next->key ^= EPKey[File(Current->ep_square)];
	Next->turn = Current->turn ^ 1;
	Next->key ^= TurnKey;
	Entry = Hash + (High32(Next->key) & hash_mask);
	prefetch((char *)Entry,_MM_HINT_NTA);
	Next->ply = 0;
	goto finish;
non_capture:
	from = From(move);
	Next->ply = Current->ply + 1;
	piece = Square(from);
	Square(from) = 0;
	Square(to) = piece;
	Next->piece = piece;
	mask_from = Bit(from);
	mask_to = Bit(to);
	BB(piece) ^= mask_from;
	Piece(me) ^= mask_from;
	BB(piece) |= mask_to;
	Piece(me) |= mask_to;
	Next->castle_flags = Current->castle_flags & UpdateCastling[to] & UpdateCastling[from];
	Next->pst = Current->pst + Pst(piece,to) - Pst(piece,from);
	Next->key = Current->key ^ PieceKey[piece][to] ^ PieceKey[piece][from] ^ CastleKey[Current->castle_flags] ^ CastleKey[Next->castle_flags];
	Next->material = Current->material;
	if (piece == IPawn(me)) {
		Next->ply = 0;
		Next->pawn_key = Current->pawn_key ^ PieceKey[IPawn(me)][to] ^ PieceKey[IPawn(me)][from] ^ CastleKey[Current->castle_flags] ^ CastleKey[Next->castle_flags];
		if (IsEP(move)) {
			Square(to ^ 8) = 0;
			u = Bit(to ^ 8);
			Next->key ^= PieceKey[IPawn(opp)][to ^ 8];
			Next->pawn_key ^= PieceKey[IPawn(opp)][to ^ 8];
			Next->pst -= Pst(IPawn(opp),to ^ 8);
			Pawn(opp) &= ~u;
			Piece(opp) &= ~u;
			Next->material -= MatCode[IPawn(opp)];
		} else if (IsPromotion(move)) {
			piece = Promotion(move,me);
			Square(to) = piece;
		    Next->material += MatCode[piece] - MatCode[IPawn(me)];
			if (piece < WhiteRook) {
				if (piece >= WhiteLight && T(BB(piece))) Next->material |= FlagUnusualMaterial;
				if (Multiple(BB(piece))) Next->material |= FlagUnusualMaterial;
			} else if (Multiple(BB(piece))) Next->material |= FlagUnusualMaterial;
			Pawn(me) ^= mask_to;
			BB(piece) |= mask_to;
			Next->pst += Pst(piece,to) - Pst(IPawn(me),to);
			Next->key ^= PieceKey[piece][to] ^ PieceKey[IPawn(me)][to];
			Next->pawn_key ^= PieceKey[IPawn(me)][to];
		} else if ((to ^ from) == 16) {
			if (PAtt[me][(to + from) >> 1] & Pawn(opp)) {
				Next->ep_square = (to + from) >> 1;
				Next->key ^= EPKey[File(Next->ep_square)];
			}
		}
		PawnEntry = PawnHash + (Next->pawn_key & pawn_hash_mask);
	    prefetch((char *)PawnEntry,_MM_HINT_NTA);
	} else {
		if (piece < WhiteKing) Next->pawn_key = Current->pawn_key ^ CastleKey[Current->castle_flags] ^ CastleKey[Next->castle_flags];
		else {
			Next->pawn_key = Current->pawn_key ^ PieceKey[piece][to] ^ PieceKey[piece][from] ^ CastleKey[Current->castle_flags] ^ CastleKey[Next->castle_flags];
			PawnEntry = PawnHash + (Next->pawn_key & pawn_hash_mask);
	        prefetch((char *)PawnEntry,_MM_HINT_NTA);
		}
		if (IsCastling(move)) {
			int rold, rnew;
			Next->ply = 0;
			if (to == 6) {
			    rold = 7; 
			    rnew = 5;
		    } else if (to == 2) {
                rold = 0; 
			    rnew = 3;
		    } else if (to == 62) {
                rold = 63;
			    rnew = 61;
		    } else if (to == 58) {
                rold = 56; 
			    rnew = 59;
		    }
			Add(mask_to,rnew);
			Square(rold) = 0;
			Square(rnew) = IRook(me);
			BB(IRook(me)) ^= Bit(rold);
			Piece(me) ^= Bit(rold);
			BB(IRook(me)) |= Bit(rnew);
			Piece(me) |= Bit(rnew);
			Next->pst += Pst(IRook(me),rnew) - Pst(IRook(me),rold);
			Next->key ^= PieceKey[IRook(me)][rnew] ^ PieceKey[IRook(me)][rold];
		}
	}

	if (Current->ep_square) Next->key ^= EPKey[File(Current->ep_square)];
	Next->turn = opp;
	Next->key ^= TurnKey;
	Entry = Hash + (High32(Next->key) & hash_mask);
	prefetch((char *)Entry,_MM_HINT_NTA);
finish:
	sp++;
	Stack[sp] = Next->key;
	Next->move = move;
	Next->gen_flags = 0;
	Current++;
	nodes++;
}

template <bool me> void undo_move(int move) {
	int to, from, piece;
	from = From(move);
	to = To(move);
	if (IsPromotion(move)) {
		BB(Square(to)) ^= Bit(to);
		piece = IPawn(me);
	} else piece = Square(to);
	Square(from) = piece;
	BB(piece) |= Bit(from);
	Piece(me) |= Bit(from);
	BB(piece) &= ~Bit(to);
	Piece(me) ^= Bit(to);
	Square(to) = Current->capture;
	if (Current->capture) {
	    BB(Current->capture) |= Bit(to);
	    Piece(opp) |= Bit(to);
	} else {
		if (IsCastling(move)) {
			int rold, rnew;
			if (to == 6) {
			    rold = 7; 
			    rnew = 5;
		    } else if (to == 2) {
                rold = 0; 
			    rnew = 3;
		    } else if (to == 62) {
                rold = 63;
			    rnew = 61;
		    } else if (to == 58) {
                rold = 56; 
			    rnew = 59;
		    }
			Square(rnew) = 0;
			Square(rold) = IRook(me);
			Rook(me) ^= Bit(rnew);
			Piece(me) ^= Bit(rnew);
			Rook(me) |= Bit(rold);
			Piece(me) |= Bit(rold);
		} else if (IsEP(move)) {
			to = to ^ 8;
			piece = IPawn(opp);
			Square(to) = piece;
			Piece(opp) |= Bit(to);
			Pawn(opp) |= Bit(to);
		}
	}
	Current--;
	sp--;
}

void do_null() {
	GData * Next;
	GEntry * Entry;

	Next = Current + 1;
	Next->key = Current->key ^ TurnKey;
	Entry = Hash + (High32(Next->key) & hash_mask);
	prefetch((char *)Entry,_MM_HINT_NTA);
	Next->pawn_key = Current->pawn_key;
	Next->eval_key = 0;
	Next->turn = Current->turn ^ 1;
	Next->material = Current->material;
	Next->pst = Current->pst;
	Next->ply = 0;
	Next->castle_flags = Current->castle_flags;
	Next->ep_square = 0;
	Next->capture = 0;
	if (Current->ep_square) Next->key ^= EPKey[File(Current->ep_square)];
	sp++;	
	Next->att[White] = Current->att[White];
	Next->att[Black] = Current->att[Black];
	Next->patt[White] = Current->patt[White];
	Next->patt[Black] = Current->patt[Black];
	Next->xray[White] = Current->xray[White];
	Next->xray[Black] = Current->xray[Black];
	Next->pin[White] = Current->pin[White];
	Next->pin[Black] = Current->pin[Black];
	Stack[sp] = Next->key;
	Next->threat = Current->threat;
	Next->passer = Current->passer;
	Next->score = -Current->score;
	Next->move = 0;
	Next->gen_flags = 0;
	Current++;
	nodes++;
}

void undo_null() {
	Current--;
	sp--;
}

template <bool me> int krbkrx() {
	if (King(opp) & Interior) return 1;
	return 16;
}
template <bool me> int kpkx() {
	uint64 u;
	if (me == White) u = Kpk[Current->turn][lsb(Pawn(White))][lsb(King(White))] & Bit(lsb(King(Black)));
	else u = Kpk[Current->turn ^ 1][63 - lsb(Pawn(Black))][63 - lsb(King(Black))] & Bit(63 - lsb(King(White)));
	if (u) return 32;
	else if (Piece(opp) ^ King(opp)) return 1;
	else return 0;
}
template <bool me> int knpkx() {
	if (Pawn(me) & Line(me, 6) & (File[0] | File[7])) {
		int sq = lsb(Pawn(me));
		if (SArea[sq] & King(opp) & (Line(me, 6) | Line(me, 7))) return 0;
		if (Square(sq + Push(me)) == IKing(me) && (SArea[lsb(King(me))] && SArea[lsb(King(opp))] & Line(me, 7))) return 0;
	} else if (Pawn(me) & Line(me, 5) & (File[0] | File[7])) {
		int sq = lsb(Pawn(me));
		if (Square(sq + Push(me)) == IPawn(opp)) {
			if (SArea[sq + Push(me)] & King(opp) & Line(me, 7)) return 0;
			if ((SArea[sq + Push(me)] & SArea[lsb(King(opp))] & Line(me, 7)) && (!(NAtt[sq + Push(me)] & Knight(me)) || Current->turn == opp)) return 0;
		}
	}
	return 32;
}
template <bool me> int krpkrx() {
	int mul = 32;
	int sq = lsb(Pawn(me));
	int rrank = CRank(me, sq);
	int o_king = lsb(King(opp));
	int o_rook = lsb(Rook(opp));
	int m_king = lsb(King(me));
	int add_mat = T(Piece(opp) ^ King(opp) ^ Rook(opp));
	int clear = F(add_mat) || F((PWay[opp][sq] | PIsolated[File(sq)]) & Forward[opp][Rank(sq + Push(me))] & (Piece(opp) ^ King(opp) ^ Rook(opp)));

	if (!clear) return 32;
	if (!add_mat && !(Pawn(me) & (File[0] | File[7]))) {
		int m_rook = lsb(Rook(me));
		if (CRank(me, o_king) < CRank(me, m_rook) && CRank(me, m_rook) < rrank && CRank(me, m_king) >= rrank - 1 && CRank(me, m_king) > CRank(me, m_rook)
			&& ((SArea[m_king] & Pawn(me)) || (Current->turn == me && Abs(File(sq) - File(m_king)) <= 1 && Abs(rrank - CRank(me, m_king)) <= 2))) return 128;
		if (SArea[m_king] & Pawn(me)) {
			if (rrank >= 4) {
				if ((File(sq) < File(m_rook) && File(m_rook) < File(o_king)) || (File(sq) > File(m_rook) && File(m_rook) > File(o_king))) return 128;
			} else if (rrank >= 2) {
				if (!(Pawn(me) & (File[1] | File[6])) && rrank + Abs(File(sq) - File(m_rook)) > 4
					&& ((File(sq) < File(m_rook) && File(m_rook) < File(o_king)) || (File(sq) > File(m_rook) && File(m_rook) > File(o_king)))) return 128;
			}
		}
	}

	if (PWay[me][sq] & King(opp)) {
		if (Pawn(me) & (File[0] | File[7])) mul = Min(mul, add_mat << 3);
		if (rrank <= 3) mul = Min(mul, add_mat << 3);
		if (rrank == 4 && CRank(me, m_king) <= 4 && CRank(me, o_rook) == 5 && T(King(opp) & (Line(me, 6) | Line(me, 7)))
			&& (Current->turn != me || F(PAtt[me][sq] & RookAttacks(lsb(Rook(me)), PieceAll) & (~SArea[o_king])))) mul = Min(mul, add_mat << 3);
		if (rrank >= 5 && CRank(me, o_rook) <= 1 && (Current->turn != me || Check(me) || Dist(m_king, sq) >= 2)) mul = Min(mul, add_mat << 3);
		if (T(King(opp) & (File[1] | File[2] | File[6] | File[7])) && T(Rook(opp) & Line(me, 7)) && T(Between[o_king][o_rook] & (File[3] | File[4])) && F(Rook(me) & Line(me, 7))) mul = Min(mul, add_mat << 3);
		return mul;
	} else if (rrank == 6 && (Pawn(me) & (File[0] | File[7])) && ((PSupport[me][sq] | PWay[opp][sq]) & Rook(opp)) && CRank(me, o_king) >= 6) {
		int dist = Abs(File(sq) - File(o_king));
		if (dist <= 3)  mul = Min(mul, add_mat << 3);
		if (dist == 4 && ((PSupport[me][o_king] & Rook(me)) || Current->turn == opp)) mul = Min(mul, add_mat << 3);
	}

	if (SArea[o_king] & PWay[me][sq] & Line(me, 7)) {
		if (rrank <= 4 && CRank(me, m_king) <= 4 && CRank(me, o_rook) == 5) mul = Min(mul, add_mat << 3);
		if (rrank == 5 && CRank(me, o_rook) <= 1 && Current->turn != me || (F(SArea[m_king] & PAtt[me][sq] & (~SArea[o_king])) && (Check(me) || Dist(m_king, sq) >= 2)))
			mul = Min(mul, add_mat << 3);
	}

	if (T(PWay[me][sq] & Rook(me)) && T(PWay[opp][sq] & Rook(opp))) {
		if (King(opp) & (File[0] | File[1] | File[6] | File[7]) & Line(me, 6)) mul = Min(mul, add_mat << 3);
		else if ((Pawn(me) & (File[0] | File[7])) && (King(opp) & (Line(me, 5) | Line(me, 6))) && Abs(File(sq) - File(o_king)) <= 2 && File(sq) != File(o_king)) mul = Min(mul, add_mat << 3);
	}

	if (Abs(File(sq) - File(o_king)) <= 1 && Abs(File(sq) - File(o_rook)) <= 1 && CRank(me, o_rook) > rrank && CRank(me, o_king) > rrank) mul = Min(mul, (Pawn(me) & (File[3] | File[4])) ? 12 : 16);

	return mul;
}
template <bool me> int krpkbx() {
	if (!(Pawn(me) & Line(me, 5))) return 32;
	int sq = lsb(Pawn(me));
	if (!(PWay[me][sq] & King(opp))) return 32;
	int diag_sq = NB(me, BMask[sq + Push(me)]);
	if (CRank(me, diag_sq) > 1) return 32;
	uint64 mdiag = FullLine[sq + Push(me)][diag_sq] | Bit(sq + Push(me)) | Bit(diag_sq);
	int check_sq = NB(me, BMask[sq - Push(me)]);
	uint64 cdiag = FullLine[sq - Push(me)][check_sq] | Bit(sq - Push(me)) | Bit(check_sq);
	if ((mdiag | cdiag) & (Piece(opp) ^ King(opp) ^ Bishop(opp))) return 32;
	if (cdiag & Bishop(opp)) return 0;
	if ((mdiag & Bishop(opp)) && (Current->turn == opp || !(King(me) & PAtt[opp][sq + Push(me)]))) return 0;
	return 32;
}
template <bool me> int kqkp() {
	if (F(SArea[lsb(King(opp))] & Pawn(opp) & Line(me, 1) & (File[0] | File[2] | File[5] | File[7]))) return 32;
	if (PWay[opp][lsb(Pawn(opp))] & (King(me) | Queen(me))) return 32;
	if (Pawn(opp) & (File[0] | File[7])) return 1;
	else return 4;
}
template <bool me> int kqkrpx() {
	int rsq = lsb(Rook(opp));
	uint64 pawns = SArea[lsb(King(opp))] & PAtt[me][rsq] & Pawn(opp) & Interior & Line(me, 6);
	if (pawns && CRank(me, lsb(King(me))) <= 4) return 0;
	return 32;
}
template <bool me> int krkpx() {
	if (T(SArea[lsb(King(opp))] & Pawn(opp) & Line(me, 1)) & F(PWay[opp][NB(me, Pawn(opp))] & King(me))) return 0;
	return 32;
}
template <bool me> int krppkrpx() {
	if (Current->passer & Pawn(me)) {
		if (Single(Current->passer & Pawn(me))) {
			int sq = lsb(Current->passer & Pawn(me));
			if (PWay[me][sq] & King(opp) & (File[0] | File[1] | File[6] | File[7])) {
				int opp_king = lsb(King(opp));
				if (SArea[opp_king] & Pawn(opp)) {
					int king_file = File(opp_king);
					if (!((~(File[king_file] | PIsolated[king_file])) & Pawn(me))) return 1;
				}
			}
		}
		return 32;
	}
	if (F((~(PWay[opp][lsb(King(opp))] | PSupport[me][lsb(King(opp))])) & Pawn(me))) return 0;
	return 32;
}
template <bool me> int krpppkrppx() {
	if (T(Current->passer & Pawn(me)) || F((SArea[lsb(Pawn(opp))] | SArea[msb(Pawn(opp))]) & Pawn(opp))) return 32;
	if (F((~(PWay[opp][lsb(King(opp))] | PSupport[me][lsb(King(opp))])) & Pawn(me))) return 0;
	return 32;
}
template <bool me> int kbpkbx() {
	int sq = lsb(Pawn(me));
	uint64 u;
	if ((T(Board->bb[ILight(me)]) && T(Board->bb[IDark(opp)])) || (T(Board->bb[IDark(me)]) && T(Board->bb[ILight(opp)]))) {
		if (CRank(me, sq) <= 4) return 0;
		if (T(PWay[me][sq] & King(opp)) && CRank(me, sq) <= 5) return 0;
		for (u = Bishop(opp); T(u); Cut(u)) {
			if (CRank(me, lsb(u)) <= 4 && T(BishopAttacks(lsb(u), PieceAll) & PWay[me][sq])) return 0;
			if (Current->turn == opp && T(BishopAttacks(lsb(u), PieceAll) & Pawn(me))) return 0;
		}
	} else if (T(PWay[me][sq] & King(opp)) && T(King(opp) & LightArea) != T(Bishop(me) & LightArea)) return 0;
	return 32;
}
template <bool me> int kbpknx() {
	uint64 u;
	if (T(PWay[me][lsb(Pawn(me))] & King(opp)) && T(King(opp) & LightArea) != T(Bishop(me) & LightArea)) return 0;
	if (Current->turn == opp)
	for (u = Knight(opp); T(u); Cut(u))
	if (NAtt[lsb(u)] & Pawn(me)) return 0;
	return 32;
}
template <bool me> int kbppkbx() {
	int sq1 = NB(me, Pawn(me));
	int sq2 = NB(opp, Pawn(me));
	int o_king = lsb(King(opp));
	int o_bishop = lsb(Bishop(opp));

	if (File(sq1) == File(sq2)) {
		if (CRank(me, sq2) <= 3) return 0;
		if (T(PWay[me][sq2] & King(opp)) && CRank(me, sq2) <= 5) return 0;
	} else if (PIsolated[File(sq1)] & Pawn(me)) {
		if (T(King(opp) & LightArea) != T(Bishop(me) & LightArea)) {
			if (T((SArea[o_king] | King(opp)) & Bit(sq2 + Push(me))) && T(BishopAttacks(o_bishop, PieceAll) & Bit(sq2 + Push(me))))
			if (T((SArea[o_king] | King(opp)) & Bit((sq2 & 0xFFFFFFF8) | File(sq1))) && T(BishopAttacks(o_bishop, PieceAll) & Bit((sq2 & 0xFFFFFFF8) | File(sq1)))) return 0;
		}
	}
	return 32;
}
template <bool me> int krppkrx() {
	int sq1 = NB(me, Pawn(me));
	int sq2 = NB(opp, Pawn(me));

	if ((Piece(opp) ^ King(opp) ^ Rook(opp)) & Forward[me][Rank(sq1 - Push(me))]) return 32;
	if (File(sq1) == File(sq2)) {
		if (T(PWay[me][sq2] & King(opp))) return 16;
		return 32;
	}
	if (T(PIsolated[File(sq2)] & Pawn(me)) && T((File[0] | File[7]) & Pawn(me)) && T(King(opp) & Shift(me, Pawn(me)))) {
		if (CRank(me, sq2) == 5 && CRank(me, sq1) == 4 && T(Rook(opp) & (Line(me, 5) | Line(me, 6)))) return 10;
		else if (CRank(me, sq2) < 5) return 16;
	}
	return 32;
}
typedef struct {
	int king_w, king_b, score;
	uint64 patt_w, patt_b, double_att_w, double_att_b;
} GPawnEvalInfo;

template <bool me, bool HPopCnt> __forceinline void eval_pawns(GPawnEntry * PawnEntry, GPawnEvalInfo &PEI) {
	int kf = File(PVarC(PEI, king, me));
	int kr = Rank(PVarC(PEI, king, me));
	int start, inc;
	if (kf <= 3) {
		start = Max(kf - 1, 0);
		inc = 1;
	} else {
		start = Min(kf + 1, 7);
		inc = -1;
	}
	int shelter = 0;
	uint64 mpawns = Pawn(me) & Forward[me][me ? Min(kr + 1, 7) : Max(kr - 1, 0)];
	for (int file = start, i = 0; i < 3; file += inc, i++) {
		shelter += Shelter[i][CRank(me, NBZ(me, mpawns & File[file]))];
		int rank;
		if (Pawn(opp) & File[file]) {
			int sq = NB(me, Pawn(opp) & File[file]);
			if ((rank = CRank(opp, sq)) < 6) {
				if (rank >= 3) shelter += StormBlocked[rank - 3];
				if (uint64 u = (PIsolated[File(sq)] & Forward[opp][Rank(sq)] & Pawn(me))) {
					int square = NB(opp, u);
					uint64 att_sq = PAtt[me][square] & PWay[opp][sq]; // may be zero
					if ((File[File(square)] | PIsolated[File(square)]) & King(me)) if (!(PVarC(PEI, double_att, me) & att_sq) || (Current->patt[opp] & att_sq)) {
						if (PWay[opp][square] & Pawn(me)) continue;
						if (!(PawnAll & PWay[opp][sq] & Forward[me][Rank(square)])) {
							if (rank >= 3) {
								shelter += StormShelterAtt[rank - 3];
								if (PVarC(PEI, patt, opp) & Bit(sq + Push(opp))) shelter += StormConnected[rank - 3];
								if (!(PWay[opp][sq] & PawnAll)) shelter += StormOpen[rank - 3];
							}
							if (!((File[File(sq)] | PIsolated[File(sq)]) & King(opp)) && rank <= 4) shelter += StormFree[rank - 1];
						}
					}
				}
			}
		} else {
			shelter += Sa(StormHof, StormHofValue);
			if (!(Pawn(me) & File[file])) shelter += Sa(StormHof, StormOfValue);
		}
	}
	PawnEntry->shelter[me] = shelter;

	uint64 b;
	int min_file = 7, max_file = 0;
	for (uint64 u = Pawn(me); T(u); u ^= b) {
		int sq = lsb(u);
		b = Bit(sq);
		int rank = Rank(sq);
		int rrank = CRank(me, sq);
		int file = File(sq);
		uint64 way = PWay[me][sq];
		int next = Square(sq + Push(me));
		if (file < min_file) min_file = file;
		if (file > max_file) max_file = file;

		int isolated = !(Pawn(me) & PIsolated[file]);
		int doubled = T(Pawn(me) & (File[file] ^ b));
		int open = !(PawnAll & way);
		int up = !(PVarC(PEI, patt, me) & b);

		if (isolated) {
			if (open) DecV(PEI.score, Ca(Isolated, IsolatedOpen));
			else {
				DecV(PEI.score, Ca(Isolated, IsolatedClosed));
				if (next == IPawn(opp)) DecV(PEI.score, Ca(Isolated, IsolatedBlocked));
			}
			if (doubled) {
				if (open) DecV(PEI.score, Ca(Isolated, IsolatedDoubledOpen));
				else DecV(PEI.score, Ca(Isolated, IsolatedDoubledClosed));
			}
		} else {
			if (doubled) {
				if (open) DecV(PEI.score, Ca(Doubled, DoubledOpen));
				else DecV(PEI.score, Ca(Doubled, DoubledClosed));
			}
			if (rrank >= 3 && (b & (File[2] | File[3] | File[4] | File[5])) && next != IPawn(opp) && (PIsolated[file] & Line[rank] & Pawn(me)))
				IncV(PEI.score, Ca(PawnSpecial, PawnChainLinear) * (rrank - 3) + Ca(PawnSpecial, PawnChain));
		}
		int backward = 0;
		if (!(PSupport[me][sq] & Pawn(me))) {
			if (isolated) backward = 1;
			else if (uint64 v = (PawnAll | PVarC(PEI, patt, opp)) & way) if (IsGreater(me, NB(me, PVarC(PEI, patt, me) & way), NB(me, v))) backward = 1;
		}
		if (backward) {
			if (open) DecV(PEI.score, Ca(Backward, BackwardOpen));
			else DecV(PEI.score, Ca(Backward, BackwardClosed));
		} else if (open) if (!(Pawn(opp) & PIsolated[file]) || popcount<HPopCnt>(Pawn(me) & PIsolated[file]) >= popcount<HPopCnt>(Pawn(opp) & PIsolated[file])) IncV(PEI.score,PasserCandidate[rrank]); // IDEA: more precise pawn counting for the case of, say, white e5 candidate with black pawn on f5 or f4...
		if (up && next == IPawn(opp)) {
			DecV(PEI.score, Ca(Unprotected, UpBlocked));
			if (backward) {
				if (rrank <= 2) { // IDEA (based on weird passer target tuning result): may be score unprotected/backward depending on rank/file?
					DecV(PEI.score, Ca(Unprotected, PasserTarget));
					if (rrank <= 1) DecV(PEI.score, Ca(Unprotected, PasserTarget));
				}
				for (uint64 v = PAtt[me][sq] & Pawn(me); v; Cut(v)) if ((PSupport[me][lsb(v)] & Pawn(me)) == b) {
					DecV(PEI.score, Ca(Unprotected, ChainRoot));
					break;
				}
			}
		}
		if (open && !(PIsolated[file] & Forward[me][rank] & Pawn(opp))) {
			PawnEntry->passer[me] |= (uint8)(1 << file);
			if (rrank <= 2) continue;
			IncV(PEI.score, PasserGeneral[rrank]);
			int dist_att = Dist(PVarC(PEI, king, opp), sq + Push(me)); // IDEA: average the distance with the distance to the promotion square? or just use the latter?
			int dist_def = Dist(PVarC(PEI, king, me), sq + Push(me));
			IncV(PEI.score, Compose256(0, dist_att * (int)PasserAtt[rrank] + LogDist[dist_att] * (int)PasserAttLog[rrank] - dist_def * (int)PasserDef[rrank] - (int)LogDist[dist_def] * (int)PasserDefLog[rrank]));
			if (PVarC(PEI, patt, me) & b) IncV(PEI.score, PasserProtected[rrank]);
			if (!(Pawn(opp) & West[file]) || !(Pawn(opp) & East[file])) IncV(PEI.score, PasserOutside[rrank]);
		}
	}
	uint64 files = 0;
	for (int i = 1; i < 7; i++) files |= (Pawn(me) >> (i << 3)) & 0xFF;
	int file_span = (files ? (msb(files) - lsb(files)) : 0);
	IncV(PEI.score, Ca(PawnSpecial, PawnFileSpan) * file_span);
	PawnEntry->draw[me] = (7 - file_span) * Max(5 - popcount<HPopCnt>(files), 0);
}

template <bool HPopCnt> void eval_pawn_structure(GPawnEntry * PawnEntry) {
	GPawnEvalInfo PEI;
	for (int i = 0; i < sizeof(GPawnEntry) / sizeof(int); i++) *(((int*)PawnEntry) + i) = 0;
	PawnEntry->key = Current->pawn_key;

	PEI.patt_w = ShiftW(White, Pawn(White)) | ShiftE(White, Pawn(White));
	PEI.patt_b = ShiftW(Black, Pawn(Black)) | ShiftE(Black, Pawn(Black));
	PEI.double_att_w = ShiftW(White, Pawn(White)) & ShiftE(White, Pawn(White));
	PEI.double_att_b = ShiftW(Black, Pawn(Black)) & ShiftE(Black, Pawn(Black));
	PEI.king_w = lsb(King(White));
	PEI.king_b = lsb(King(Black));
	PEI.score = 0;

	eval_pawns<White, HPopCnt>(PawnEntry, PEI);
	eval_pawns<Black, HPopCnt>(PawnEntry, PEI);

	PawnEntry->score = PEI.score;
}

typedef struct {
	int score, king_w, king_b, mul;
	uint64 occ, area_w, area_b, free_w, free_b;
	uint32 king_att_w, king_att_b;
	GPawnEntry * PawnEntry;
	GMaterial * material;
} GEvalInfo;

template <bool me, bool HPopCnt> __forceinline void eval_queens(GEvalInfo &EI) {
	uint64 u, b;
	for (u = Queen(me); T(u); u ^= b) {
		int sq = lsb(u);
		b = Bit(sq);
		uint64 att = QueenAttacks(sq,EI.occ);
		Current->att[me] |= att;
		if (QMask[sq] & King(opp)) if (uint64 v = Between[PVarC(EI,king,opp)][sq] & EI.occ) if (Single(v)) {
			Current->xray[me] |= v;
			uint64 square = lsb(v); int piece = Square(square); int katt = 0;
			if (piece == IPawn(me)) {
				if (!Square(square + Push(me))) IncV(EI.score, Ca(Pin, SelfPawnPin));
			} else if ((piece & 1) == me) {
				IncV(EI.score, Ca(Pin, SelfPiecePin));
				katt = 1;
			} else if (piece != IPawn(opp) && !(((BMask[sq] & Bishop(opp)) | (RMask[sq] & Rook(opp)) | Queen(opp)) & v)) {
				IncV(EI.score, Ca(Pin, WeakPin));
				if (!(Current->patt[opp] & v)) katt = 1;
			}
			if (katt && !(att & PVarC(EI, area, opp))) PVarC(EI, king_att, me) += KingAttack;
		} else if (v == (v & Minor(opp))) IncV(EI.score, Ca(KingRay, QKingRay));
		if (att & PVarC(EI, area, opp)) {
			PVarC(EI, king_att, me) += KingQAttack;
			for (uint64 v = att & PVarC(EI, area, opp); T(v); Cut(v))
			if (FullLine[sq][lsb(v)] & att & ((Rook(me) & RMask[sq]) | (Bishop(me) & BMask[sq]))) PVarC(EI, king_att, me)++;
		}
		IncV(EI.score,Mobility[PieceType[WhiteQueen] - 1][popcount<HPopCnt>(att & PVarC(EI,free,me))]);
		if (att & PVarC(EI, free, me) & Pawn(opp)) IncV(EI.score, Ca(Tactical, TacticalMajorPawn));
		if (att & PVarC(EI, free, me) & Minor(opp)) IncV(EI.score, Ca(Tactical, TacticalMajorMinor));
		if (att & PVarC(EI, area, me)) IncV(EI.score, Ca(KingDefence, KingDefQueen));
	}
}
template <bool me, bool HPopCnt> __forceinline void eval_rooks(GEvalInfo &EI) {
	uint64 u, b;
	for (u = Rook(me); T(u); u ^= b) {
		int sq = lsb(u);
		b = Bit(sq);
		uint64 att = RookAttacks(sq,EI.occ);
		Current->att[me] |= att;
		if (RMask[sq] & King(opp)) if (uint64 v = Between[PVarC(EI, king, opp)][sq] & EI.occ) if (Single(v)) {
			Current->xray[me] |= v;
			uint64 square = lsb(v); int piece = Square(square); int katt = 0;
			if (piece == IPawn(me)) {
				if (!Square(square + Push(me))) IncV(EI.score, Ca(Pin, SelfPawnPin));
			} else if ((piece & 1) == me) {
				IncV(EI.score, Ca(Pin, SelfPiecePin));
				katt = 1;
			} else if (piece != IPawn(opp)) {
				if (piece < IRook(opp)) {
					IncV(EI.score, Ca(Pin, WeakPin));
					if (!(Current->patt[opp] & v)) katt = 1;
				} else if (piece == IQueen(opp)) IncV(EI.score, Ca(Pin, ThreatPin));
			}
			if (katt && !(att & PVarC(EI, area, opp))) PVarC(EI, king_att, me) += KingAttack;
		} else if (v == (v & (Minor(opp) | Queen(opp)))) IncV(EI.score, Ca(KingRay, RKingRay));
		if (att & PVarC(EI, area, opp)) {
			PVarC(EI, king_att, me) += KingRAttack;
			for (uint64 v = att & PVarC(EI, area, opp); T(v); Cut(v))
			if (FullLine[sq][lsb(v)] & att & Major(me)) PVarC(EI, king_att, me)++;
		}
		IncV(EI.score,Mobility[PieceType[WhiteRook] - 1][popcount<HPopCnt>(att & PVarC(EI,free,me))]);
		if (att & PVarC(EI, free, me) & Pawn(opp)) IncV(EI.score, Ca(Tactical, TacticalMajorPawn));
		if (att & PVarC(EI, free, me) & Minor(opp)) IncV(EI.score, Ca(Tactical, TacticalMajorMinor));
		if (att & PVarC(EI, area, me)) IncV(EI.score, Ca(KingDefence, KingDefRook));
		Current->threat |= att & Queen(opp);
		if (!(PWay[me][sq] & Pawn(me))) {
			IncV(EI.score, Ca(RookSpecial, RookHof));
			int hof_score = 0;
			if (!(PWay[me][sq] & Pawn(opp))) {
				IncV(EI.score, Ca(RookSpecial, RookOf));
				if (att & Line(me, 7)) hof_score += Ca(RookSpecial, RookOfOpen);
				else if (uint64 target = att & PWay[me][sq] & Minor(opp)) {
					if (!(Current->patt[opp] & target)) {
						hof_score += Ca(RookSpecial, RookOfMinorHaging);
						if (PWay[me][sq] & King(opp)) hof_score += Ca(RookSpecial, RookOfKingAtt);
					} else hof_score += Ca(RookSpecial, RookOfMinorFixed);
				}
			} else if (att & PWay[me][sq] & Pawn(opp)) {
				uint64 square = lsb(att & PWay[me][sq] & Pawn(opp));
				if (!(PSupport[opp][square] & Pawn(opp))) hof_score += Ca(RookSpecial, RookHofWeakPAtt);
			}
			IncV(EI.score, hof_score);
			if (PWay[opp][sq] & att & Major(me)) IncV(EI.score, hof_score);
		}
		if ((b & Line(me, 6)) && ((King(opp) | Pawn(opp)) & (Line(me, 6) | Line(me, 7)))) {
			IncV(EI.score, Ca(RookSpecial, Rook7th));
			if (King(opp) & Line(me, 7)) IncV(EI.score, Ca(RookSpecial, Rook7thK8th));
			if (Major(me) & att & Line(me, 6)) IncV(EI.score, Ca(RookSpecial, Rook7thDoubled));
		}
	}
}
template <bool me, bool HPopCnt> __forceinline void eval_bishops(GEvalInfo &EI) {
	uint64 u, b;
	for (u = Bishop(me); T(u); u ^= b) {
		int sq = lsb(u);
		b = Bit(sq);
		uint64 att = BishopAttacks(sq, EI.occ);
		Current->att[me] |= att;
		if (BMask[sq] & King(opp)) if (uint64 v = Between[PVarC(EI, king, opp)][sq] & EI.occ) if (Single(v)) {
			Current->xray[me] |= v;
			uint64 square = lsb(v); int piece = Square(square); int katt = 0;
			if (piece == IPawn(me)) {
				if (!Square(square + Push(me))) IncV(EI.score, Ca(Pin, SelfPawnPin));
			} else if ((piece & 1) == me) {
				IncV(EI.score, Ca(Pin, SelfPiecePin));
				katt = 1;
			} else if (piece != IPawn(opp)) {
				if (piece < ILight(opp)) {
					IncV(EI.score, Ca(Pin, StrongPin));
					if (!(Current->patt[opp] & v)) katt = 1;
				} else if (piece >= IRook(opp)) IncV(EI.score, Ca(Pin, ThreatPin));
			}
			if (katt && !(att & PVarC(EI, area, opp))) PVarC(EI, king_att, me) += KingAttack;
		} else if (v == (v & (Knight(opp) | Major(opp)))) IncV(EI.score, Ca(KingRay, BKingRay));
		if (att & PVarC(EI, area, opp)) PVarC(EI, king_att, me) += KingBAttack;
		IncV(EI.score, Mobility[PieceType[WhiteLight] - 1][popcount<HPopCnt>(att & PVarC(EI, free, me))]);
		if (att & PVarC(EI, free, me) & Pawn(opp)) IncV(EI.score, Ca(Tactical, TacticalMinorPawn));
		if (att & PVarC(EI, free, me) & Knight(opp)) IncV(EI.score, Ca(Tactical, TacticalMinorMinor));
		if (att & PVarC(EI, area, me)) IncV(EI.score, Ca(KingDefence, KingDefBishop));
		Current->threat |= att & Major(opp);
		if (b & LightArea) {
			for (uint64 v = ((~BishopForward[me][sq]) | (att & Forward[me][Rank(sq)])) & Pawn(opp) & (~Current->patt[opp]) & LightArea; v; Cut(v)) {
				uint64 square = lsb(v);
				if (!((PSupport[opp][square] | PWay[opp][square]) & Pawn(opp))) IncV(EI.score, Ca(BishopSpecial, BishopNonForwardPawn));
			}
			uint64 v = BishopForward[me][sq] & Pawn(me) & LightArea;
			v |= (v & (File[2] | File[3] | File[4] | File[5] | BMask[sq])) >> 8;
			DecV(EI.score, Ca(BishopSpecial, BishopPawnBlock) * popcount<HPopCnt>(v));
		} else {
			for (uint64 v = ((~BishopForward[me][sq]) | (att & Forward[me][Rank(sq)])) & Pawn(opp) & (~Current->patt[opp]) & DarkArea; v; Cut(v)) {
				uint64 square = lsb(v);
				if (!((PSupport[opp][square] | PWay[opp][square]) & Pawn(opp))) IncV(EI.score, Ca(BishopSpecial, BishopNonForwardPawn));
			}
			uint64 v = BishopForward[me][sq] & Pawn(me) & DarkArea;
			v |= (v & (File[2] | File[3] | File[4] | File[5] | BMask[sq])) >> 8;
			DecV(EI.score, Ca(BishopSpecial, BishopPawnBlock) * popcount<HPopCnt>(v));
		}
	}
}
template <bool me, bool HPopCnt> __forceinline void eval_knights(GEvalInfo &EI) {
	uint64 u, b;
	for (u = Knight(me); T(u); u ^= b) {
		int sq = lsb(u);
		b = Bit(sq);
		uint64 att = NAtt[sq];
		Current->att[me] |= att;
		if (att & PVarC(EI, area, opp)) PVarC(EI, king_att, me) += KingNAttack;
		IncV(EI.score, Mobility[PieceType[WhiteKnight] - 1][popcount<HPopCnt>(att & PVarC(EI, free, me))]);
		if (att & PVarC(EI, free, me) & Pawn(opp)) IncV(EI.score, Ca(Tactical, TacticalMinorPawn));
		if (att & PVarC(EI, free, me) & Bishop(opp)) IncV(EI.score, Ca(Tactical, TacticalMinorMinor));
		if (att & PVarC(EI, area, me)) IncV(EI.score, Ca(KingDefence, KingDefKnight));
		Current->threat |= att & Major(opp);
		if ((b & Outpost[me]) && !(Pawn(opp) & PIsolated[File(sq)] & Forward[me][Rank(sq)])) {
			IncV(EI.score, Ca(KnightSpecial, KnightOutpost));
			if (Current->patt[me] & b) {
				IncV(EI.score, Ca(KnightSpecial, KnightOutpostProtected));
				if (att & PVarC(EI, free, me) & Pawn(opp)) IncV(EI.score, Ca(KnightSpecial, KnightOutpostPawnAtt));
				if (att & PVarC(EI, free, me) & Bishop(opp)) IncV(EI.score, Ca(KnightSpecial, KnightOutpostBishopAtt));
			}
		}
	}
}
template <bool me, bool HPopCnt> __forceinline void eval_king(GEvalInfo &EI) {
	int cnt = Opening(PVarC(EI, king_att, me));
	int score = Endgame(PVarC(EI, king_att, me));
	if (cnt >= 2 && T(Queen(me))) {
		score += (EI.PawnEntry->shelter[opp] * KingShelterQuad)/64;
		if (uint64 u = Current->att[me] & PVarC(EI, area, opp) & (~Current->att[opp])) score += popcount<HPopCnt>(u) * KingAttackSquare;
		if (!(SArea[PVarC(EI, king, opp)] & (~(Piece(opp) | Current->att[me])))) score += KingNoMoves;
	}
	int adjusted = ((score * KingAttackScale[cnt]) >> 3) + EI.PawnEntry->shelter[opp];
	if (!Queen(me)) adjusted /= 2;
	IncV(EI.score, adjusted);
}
template <bool me, bool HPopCnt> __forceinline void eval_passer(GEvalInfo &EI) {
	for (uint64 u = EI.PawnEntry->passer[me]; T(u); Cut(u)) {
		int file = lsb(u);
		int sq = NB(opp, File[file] & Pawn(me));
		int rank = CRank(me, sq);
		Current->passer |= Bit(sq);
		if (rank <= 2) continue;
		if (!Square(sq + Push(me))) IncV(EI.score, PasserBlocked[rank]);
		uint64 way = PWay[me][sq];
		int connected = 0, supported = 0, hooked = 0, unsupported = 0, free = 0;
		if (!(way & Piece(opp))) {
			IncV(EI.score, PasserClear[rank]);
			if (PWay[opp][sq] & Major(me)) {
				int square = NB(opp, PWay[opp][sq] & Major(me));
				if (F(Between[sq][square] & EI.occ)) supported = 1;
			}
			if (PWay[opp][sq] & Major(opp)) {
				int square = NB(opp, PWay[opp][sq] & Major(opp));
				if (F(Between[sq][square] & EI.occ)) hooked = 1;
			}
			for (uint64 v = PAtt[me][sq - Push(me)] & Pawn(me); T(v); Cut(v)) {
				int square = lsb(v);
				if (F(Pawn(opp) & (File[File(square)] | PIsolated[File(square)]) & Forward[me][Rank(square)])) connected++;
			}
			if (connected) IncV(EI.score, PasserConnected[rank]);
			if (!hooked && !(Current->att[opp] & way)) {
				IncV(EI.score, PasserFree[rank]);
				free = 1;
			} else {
				uint64 attacked = Current->att[opp] | (hooked ? way : 0);
				if (supported || (!hooked && connected) || (!(Major(me) & way) && !(attacked & (~Current->att[me])))) IncV(EI.score, PasserSupported[rank]);
				else unsupported = 1;
			}
		}
		if (rank == 6) {
			if ((way & Rook(me)) && !Minor(me) && !Queen(me) && Single(Rook(me))) DecV(EI.score, Compose(0, Sa(PasserSpecial, PasserOpRookBlock)));
			if (!Major(opp) && (!NonPawnKing(opp) || Single(NonPawnKing(opp)))) {
				IncV(EI.score, Compose(0, Sa(PasserSpecial, PasserOnePiece)));
				if (!free) {
					if (!(SArea[sq + Push(me)] & King(opp))) IncV(EI.score, Compose(0, Sa(PasserSpecial, PasserOpMinorControl)));
					else IncV(EI.score, Compose(0, Sa(PasserSpecial, PasserOpKingControl)));
				}
			}
		}
	}
}
template <bool me, bool HPopCnt> __forceinline void eval_pieces(GEvalInfo &EI) {
	Current->threat |= Current->att[opp] & (~Current->att[me]) & Piece(me);
	if (uint64 u = Current->threat & Piece(me)) {
		DecV(EI.score, Ca(Tactical, TacticalThreat));
		Cut(u);
		if (u) {
			DecV(EI.score, Ca(Tactical, TacticalThreat) + Ca(Tactical, TacticalDoubleThreat));
			for (Cut(u); u; Cut(u)) DecV(EI.score, Ca(Tactical, TacticalThreat));
		}
	}
}
template <bool me, bool HPopCnt> void eval_endgame(GEvalInfo &EI) {
	if ((EI.material->flags & VarC(FlagSingleBishop, me)) && Pawn(me)) {
		int sq = (Board->bb[ILight(me)] ? (me ? 0 : 63) : (Board->bb[IDark(me)] ? (me ? 7 : 56) : (File(lsb(King(opp))) <= 3 ? (me ? 0 : 56) : (me ? 7 : 63))));
		if (!(Pawn(me) & (~PWay[opp][sq]))) {
			if ((SArea[sq] | Bit(sq)) & King(opp)) EI.mul = 0;
			else if ((SArea[sq] & SArea[lsb(King(opp))] & Line(me, 7)) && Square(sq - Push(me)) == IPawn(opp) && Square(sq - 2 * Push(me)) == IPawn(me)) EI.mul = 0;
		} else if ((King(opp) & Line(me, 6) | Line(me, 7)) && Abs(File(sq) - File(lsb(King(opp)))) <= 3 && !(Pawn(me) & (~PSupport[me][sq])) && (Pawn(me) & Line(me, 5) & Shift(opp, Pawn(opp)))) EI.mul = 0;
		if (Single(Pawn(me))) {
			if (!Bishop(me)) {
				EI.mul = MinF(EI.mul, kpkx<me>());
				if (Piece(opp) == King(opp) && EI.mul == 32) IncV(Current->score, KpkValue);
			} else {
				sq = lsb(Pawn(me));
				if ((Pawn(me) & (File[1] | File[6]) & Line(me, 5)) && Square(sq + Push(me)) == IPawn(opp) && ((PAtt[me][sq + Push(me)] | PWay[me][sq + Push(me)]) & King(opp))) EI.mul = 0;
			}
		}
		if (Bishop(opp) && Single(Bishop(opp)) && T(BB(ILight(me))) != T(BB(ILight(opp)))) {
			int pcnt = 0;
			if (T(King(opp) & LightArea) == T(Bishop(opp) & LightArea)) {
				for (uint64 u = Pawn(me); u; Cut(u)) {
					if (pcnt >= 2) goto check_for_partial_block;
					pcnt++;
					int sq = lsb(u);
					if (!(PWay[me][sq] & (PAtt[me][PVarC(EI, king, opp)] | PAtt[opp][PVarC(EI, king, opp)]))) {
						if (!(PWay[me][sq] & Pawn(opp))) goto check_for_partial_block;
						int bsq = lsb(Bishop(opp));
						uint64 att = BishopAttacks(bsq, EI.occ);
						if (!(att & PWay[me][sq] & Pawn(opp))) goto check_for_partial_block;
						if (!(BishopForward[me][bsq] & att & PWay[me][sq] & Pawn(opp)) && popcount<HPopCnt>(FullLine[lsb(att & PWay[me][sq] & Pawn(opp))][bsq] & att) <= 2)  goto check_for_partial_block;
					}
				}
				EI.mul = 0;
				return;
			}
		check_for_partial_block:
			if (pcnt <= 2 && Multiple(Pawn(me)) && !Pawn(opp) && !(Pawn(me) & Boundary) && EI.mul) {
				int sq1 = lsb(Pawn(me));
				int sq2 = msb(Pawn(me));
				int fd = Abs(File(sq2) - File(sq1));
				if (fd >= 5) EI.mul = 32;
				else if (fd >= 4) EI.mul = 26;
				else if (fd >= 3) EI.mul = 20;
			}
			if ((SArea[PVarC(EI, king, opp)] | Current->patt[opp]) & Bishop(opp)) {
				uint64 push = Shift(me, Pawn(me));
				if (!(push & (~(Piece(opp) | Current->att[opp]))) && (King(opp) & (Board->bb[ILight(opp)] ? LightArea : DarkArea))) {
					EI.mul = Min(EI.mul, 8);
					int bsq = lsb(Bishop(opp));
					uint64 att = BishopAttacks(bsq, EI.occ);
					uint64 prp = (att | SArea[PVarC(EI, king, opp)]) & Pawn(opp) & (Board->bb[ILight(opp)] ? LightArea : DarkArea);
					uint64 patt = ShiftW(opp, prp) | ShiftE(opp, prp);
					if ((SArea[PVarC(EI, king, opp)] | patt) & Bishop(opp)) {
						uint64 double_att = (SArea[PVarC(EI, king, opp)] & patt) | (patt & att) | (SArea[PVarC(EI, king, opp)] & att);
						if (!(push & (~(King(opp) | Bishop(opp) | prp | double_att)))) {
							EI.mul = 0;
							return;
						}
					}
				}
			}
		}
	}
	if (F(Major(me))) {
		if (T(Bishop(me)) && F(Knight(me)) && Single(Bishop(me)) && T(Pawn(me))) {
			int number = popcount<HPopCnt>(Pawn(me));
			if (number == 1) {
				if (Bishop(opp)) EI.mul = MinF(EI.mul, kbpkbx<me>());
				else if (Knight(opp)) EI.mul = MinF(EI.mul, kbpknx<me>());
			} else if (number == 2 && T(Bishop(opp))) EI.mul = MinF(EI.mul, kbppkbx<me>());
		} else if (!Bishop(me) && Knight(me) && Single(Knight(me)) && Pawn(me) && Single(Pawn(me))) EI.mul = MinF(EI.mul, knpkx<me>());
	} else if (F(Minor(me))) {
		if (F(Pawn(me)) && F(Rook(me)) && T(Queen(me)) && T(Pawn(opp))) {
			if (F(NonPawnKing(opp)) && Single(Pawn(opp))) EI.mul = MinF(EI.mul, kqkp<me>());
			else if (Rook(opp)) EI.mul = MinF(EI.mul, kqkrpx<me>());
		} else if (F(Queen(me)) && T(Rook(me)) && Single(Rook(me))) {
			int number = popcount<HPopCnt>(Pawn(me));
			if (number <= 3) {
				if (number == 0) {
					if (Pawn(opp)) EI.mul = MinF(EI.mul, krkpx<me>());
				} else if (Rook(opp)) {
					if (number == 1) {
						int new_mul = krpkrx<me>();
						EI.mul = (new_mul <= 32 ? Min(EI.mul, new_mul) : new_mul);
					} else {
						if (number == 2) EI.mul = MinF(EI.mul, krppkrx<me>());
						if (Pawn(opp)) {
							if (number == 2) EI.mul = MinF(EI.mul, krppkrpx<me>());
							else if (Multiple(Pawn(opp))) EI.mul = MinF(EI.mul, krpppkrppx<me>());
						}
					}
				} else if (number == 1 && Bishop(opp)) EI.mul = MinF(EI.mul, krpkbx<me>());
			}
		}
	} else if (!Pawn(me) && Single(Rook(me)) && !Queen(me) && Single(Bishop(me)) && !Knight(me) && Rook(opp)) EI.mul = MinF(EI.mul, krbkrx<me>());
	if (F(NonPawnKing(opp)) && Current->turn == opp && F(Current->att[me] & King(opp)) && !(SArea[PVarC(EI, king, opp)] & (~(Current->att[me] | Piece(opp))))
		&& F(Current->patt[opp] & Piece(me)) && F(Shift(opp, Pawn(opp)) & (~EI.occ)))
		EI.mul = 0;
}
template <bool HPopCnt> void eval_unusual_material(GEvalInfo &EI) {
	int wp, bp, wlight, blight, wr, br, wq, bq;
	wp = popcount<HPopCnt>(Pawn(White));
	bp = popcount<HPopCnt>(Pawn(Black));
	wlight = popcount<HPopCnt>(Minor(White));
	blight = popcount<HPopCnt>(Minor(Black));
	wr = popcount<HPopCnt>(Rook(White));
	br = popcount<HPopCnt>(Rook(Black));
	wq = popcount<HPopCnt>(Queen(White));
	bq = popcount<HPopCnt>(Queen(Black));
	int phase = Min(24, (wlight + blight) + 2 * (wr + br) + 4 * (wq + bq));
	int mat_score = SeeValue[WhitePawn] * (wp - bp) + SeeValue[WhiteKnight] * (wlight - blight) + SeeValue[WhiteRook] * (wr - br) + SeeValue[WhiteQueen] * (wq - bq);
	mat_score = Compose(mat_score,mat_score);
	Current->score = (((Opening(mat_score + EI.score) * phase) + (Endgame(mat_score + EI.score) * (24 - phase)))/24);
	if (Current->turn) Current->score = -Current->score;
	UpdateDelta
}

template <bool HPopCnt> void evaluation() {
	GEvalInfo EI;
	
	if (Current->eval_key == Current->key) return;
	Current->eval_key = Current->key;

	EI.king_w = lsb(King(White));
	EI.king_b = lsb(King(Black));
	EI.occ = PieceAll;
	Current->patt[White] = ShiftW(White,Pawn(White)) | ShiftE(White,Pawn(White));
	Current->patt[Black] = ShiftW(Black,Pawn(Black)) | ShiftE(Black,Pawn(Black));
	EI.area_w = (SArea[EI.king_w] | King(White)) & ((~Current->patt[White]) | Current->patt[Black]);
	EI.area_b = (SArea[EI.king_b] | King(Black)) & ((~Current->patt[Black]) | Current->patt[White]);
	Current->att[White] = Current->patt[White];
	Current->att[Black] = Current->patt[Black];
	Current->passer = 0;
	Current->threat = (Current->patt[White] & NonPawn(Black)) | (Current->patt[Black] & NonPawn(White));
	EI.score = Current->pst;

#define me White
	Current->xray[me] = 0;
	PVarC(EI, free, me) = Queen(opp) | King(opp) | (~(Current->patt[opp] | Pawn(me) | King(me)));
	DecV(EI.score, popcount<HPopCnt>(Shift(opp, EI.occ) & Pawn(me)) * Ca(PawnSpecial, PawnBlocked));
	if (Current->patt[me] & PVarC(EI, area, opp)) PVarC(EI, king_att, me) = KingAttack;
	else PVarC(EI, king_att, me) = 0;
	eval_queens<me, HPopCnt>(EI);
	PVarC(EI, free, me) |= Rook(opp);
	eval_rooks<me, HPopCnt>(EI);
	PVarC(EI, free, me) |= Minor(opp);
	eval_bishops<me, HPopCnt>(EI);
	eval_knights<me, HPopCnt>(EI);
#undef me
#define me Black
	Current->xray[me] = 0;
	PVarC(EI, free, me) = Queen(opp) | King(opp) | (~(Current->patt[opp] | Pawn(me) | King(me)));
	DecV(EI.score, popcount<HPopCnt>(Shift(opp, EI.occ) & Pawn(me)) * Ca(PawnSpecial, PawnBlocked));
	if (Current->patt[me] & PVarC(EI, area, opp)) PVarC(EI, king_att, me) = KingAttack;
	else PVarC(EI, king_att, me) = 0;
	eval_queens<me, HPopCnt>(EI);
	PVarC(EI, free, me) |= Rook(opp);
	eval_rooks<me, HPopCnt>(EI);
	PVarC(EI, free, me) |= Minor(opp);
	eval_bishops<me, HPopCnt>(EI);
	eval_knights<me, HPopCnt>(EI);
#undef me

	EI.PawnEntry = PawnHash + (Current->pawn_key & pawn_hash_mask);
	if (Current->pawn_key != EI.PawnEntry->key) eval_pawn_structure<HPopCnt>(EI.PawnEntry);
	EI.score += EI.PawnEntry->score;

	eval_king<White, HPopCnt>(EI);
	eval_king<Black, HPopCnt>(EI);
	Current->att[White] |= SArea[EI.king_w];
	Current->att[Black] |= SArea[EI.king_b];

	eval_passer<White, HPopCnt>(EI);
	eval_pieces<White, HPopCnt>(EI);
	eval_passer<Black, HPopCnt>(EI);
	eval_pieces<Black, HPopCnt>(EI);

	if (Current->material & FlagUnusualMaterial) {
		eval_unusual_material<HPopCnt>(EI);
		return;
	}
	EI.material = &Material[Current->material];
#ifdef TUNER
	if (EI.material->generation != generation) calc_material(Current->material);
#endif
	Current->score = EI.material->score + (((Opening(EI.score) * EI.material->phase) + (Endgame(EI.score) * (128 - (int)EI.material->phase)))/128);

	if (Current->ply >= 50) Current->score /= 2;
	if (Current->score > 0) {
		EI.mul = EI.material->mul[White];
		if (EI.material->flags & FlagCallEvalEndgame_w) eval_endgame<White, HPopCnt>(EI);
		Current->score -= (Min(Current->score, 100) * (int)EI.PawnEntry->draw[White]) / 64;
	} else if (Current->score < 0) {
		EI.mul = EI.material->mul[Black];
		if (EI.material->flags & FlagCallEvalEndgame_b) eval_endgame<Black, HPopCnt>(EI);
		Current->score += (Min(-Current->score, 100) * (int)EI.PawnEntry->draw[Black]) / 64;
	} else EI.mul = Min(EI.material->mul[White], EI.material->mul[Black]);
	Current->score = (Current->score * EI.mul)/32;

	if (Current->turn) Current->score = -Current->score;
	UpdateDelta
}

__forceinline void evaluate() {
	HardwarePopCnt ? evaluation<1>() : evaluation<0>();
}

template <bool me> int is_legal(int move) {
	int from, to, piece, capture;
	uint64 u, occ;

    from = From(move);
	to = To(move);
	piece = Board->square[from];
	capture = Board->square[to];
	if (piece == 0) return 0;
	if ((piece & 1) != Current->turn) return 0;
	if (capture) {
		if ((capture & 1) == (piece & 1)) return 0;
		if (capture >= WhiteKing) return 0;
	}
	occ = PieceAll;
	u = Bit(to);
	if (piece >= WhiteLight && piece < WhiteKing) {
	    if ((QMask[from] & u) == 0) return 0;
		if (Between[from][to] & occ) return 0;
	}
	if (IsEP(move)) {
		if (piece >= WhiteKnight) return 0;
		if (Current->ep_square != to) return 0;
		return 1;
	}
	if (IsCastling(move) && Board->square[from] < WhiteKing) return 0;
	if (IsPromotion(move) && Board->square[from] >= WhiteKnight) return 0;
	if (piece == IPawn(me)) {
		if (u & PMove[me][from]) {
            if (capture) return 0;
			if (T(u & Line(me,7)) && !IsPromotion(move)) return 0;
			return 1;
		} else if (to == (from + 2 * Push(me))) {
            if (capture) return 0;
			if (Square(to - Push(me))) return 0;
			if (F(u & Line(me,3))) return 0;
			return 1;
		} else if (u & PAtt[me][from]) {
            if (capture == 0) return 0;
			if (T(u & Line(me,7)) && !IsPromotion(move)) return 0;
			return 1;
		} else return 0;
	} else if (piece == IKing(me)) {
		if (me == White) {
		    if (IsCastling(move)) {
			    if (u & 0x40) {
                    if (((Current->castle_flags) & CanCastle_OO) == 0) return 0;
					if (occ & 0x60) return 0;
					if (Current->att[Black] & 0x70) return 0;
				} else {
					if (((Current->castle_flags) & CanCastle_OOO) == 0) return 0;
					if (occ & 0xE) return 0;
					if (Current->att[Black] & 0x1C) return 0;
				}
				return 1;
			}
		} else {
            if (IsCastling(move)) {
				if (u & 0x4000000000000000) {
                    if (((Current->castle_flags) & CanCastle_oo) == 0) return 0;
					if (occ & 0x6000000000000000) return 0;
					if (Current->att[White] & 0x7000000000000000) return 0;
				} else {
					if (((Current->castle_flags) & CanCastle_ooo) == 0) return 0;
					if (occ & 0x0E00000000000000) return 0;
					if (Current->att[White] & 0x1C00000000000000) return 0;
				}
				return 1;
			}
		}
        if (F(SArea[from] & u)) return 0;
	    if (Current->att[opp] & u) return 0;
		return 1;
	}
	piece = (piece >> 1) - 2;
	if (piece == 0) {
        if (u & NAtt[from]) return 1;
		else return 0;
	} else {
		if (piece <= 2) {
			if (BMask[from] & u) return 1;
		} else if (piece == 3) {
			if (RMask[from] & u) return 1;
		} else return 1;
		return 0;
	}
}

template <bool me> int is_check(int move) { // doesn't detect castling and ep checks
	uint64 king;
	int from, to, piece, king_sq;

	from = From(move);
	to = To(move);
	king = King(opp);
	king_sq = lsb(king);
	piece = Square(from);
	if (T(Bit(from) & Current->xray[me]) && F(FullLine[king_sq][from] & Bit(to))) return 1;
	if (piece < WhiteKnight) {
		if (PAtt[me][to] & king) return 1;
		if (T(Bit(to) & Line(me, 7)) && T(king & Line(me, 7)) && F(Between[to][king_sq] & PieceAll)) return 1;
	} else if (piece < WhiteLight) {
		if (NAtt[to] & king) return 1;
	} else if (piece < WhiteRook) {
		if (BMask[to] & king) if (F(Between[king_sq][to] & PieceAll)) return 1;
	} else if (piece < WhiteQueen) {
		if (RMask[to] & king) if (F(Between[king_sq][to] & PieceAll)) return 1;
	} else if (piece < WhiteKing) {
		if (QMask[to] & king) if (F(Between[king_sq][to] & PieceAll)) return 1;
	}
	return 0;
}

void hash_high(int value, int depth) {
	int i, score, min_score;
	GEntry *best, *Entry;

	min_score = 0x70000000;
	for (i = 0, best = Entry = Hash + (High32(Current->key) & hash_mask); i < 4; i++, Entry++) {
		if (Entry->key == Low32(Current->key)) {
			Entry->date = date;
			if (depth > Entry->high_depth || (depth == Entry->high_depth && value < Entry->high)) {
				if (Entry->low <= value) { 
				    Entry->high_depth = depth;
				    Entry->high = value;
				} else if (Entry->low_depth < depth) {
					Entry->high_depth = depth;
				    Entry->high = value;
					Entry->low = value;
				}
			}
			return;
		} else score = (Convert(Entry->date,int) << 3) + Convert(Max(Entry->high_depth, Entry->low_depth),int);
		if (score < min_score) {
			min_score = score;
			best = Entry;
		}
	}
	best->date = date;
	best->key = Low32(Current->key);
	best->high = value;
	best->high_depth = depth;
	best->low = 0;
	best->low_depth = 0;
	best->move = 0;
	best->flags = 0;
	return;
}

void hash_low(int move, int value, int depth) {
	int i, score, min_score;
	GEntry *best, *Entry;

	min_score = 0x70000000;
	move &= 0xFFFF;
	for (i = 0, best = Entry = Hash + (High32(Current->key) & hash_mask); i < 4; i++, Entry++) {
		if (Entry->key == Low32(Current->key)) {
			Entry->date = date;
			if (depth > Entry->low_depth || (depth == Entry->low_depth && value > Entry->low)) {
				if (move) Entry->move = move;
				if (Entry->high >= value) {
				    Entry->low_depth = depth;
				    Entry->low = value;
				} else if (Entry->high_depth < depth) {
					Entry->low_depth = depth;
				    Entry->low = value;
					Entry->high = value;
				}
			} else if (F(Entry->move)) Entry->move = move;
			return;
		} else score = (Convert(Entry->date,int) << 3) + Convert(Max(Entry->high_depth, Entry->low_depth),int);
		if (score < min_score) {
			min_score = score;
			best = Entry;
		}
	}
	best->date = date;
	best->key = Low32(Current->key);
	best->high = 0;
	best->high_depth = 0;
	best->low = value;
	best->low_depth = depth;
	best->move = move;
	best->flags = 0;
	return;
}

void hash_exact(int move, int value, int depth, int exclusion, int ex_depth, int knodes) {
	int i, score, min_score;
	GPVEntry *best;
	GPVEntry * PVEntry;

	min_score = 0x70000000;
	for (i = 0, best = PVEntry = PVHash + (High32(Current->key) & pv_hash_mask); i < pv_cluster_size; i++, PVEntry++) {
		if (PVEntry->key == Low32(Current->key)) {
			PVEntry->date = date;
			PVEntry->knodes += knodes;
			if (PVEntry->depth <= depth) {
				PVEntry->value = value;
				PVEntry->depth = depth;
				PVEntry->move = move;
				PVEntry->ply = Current->ply;
				if (ex_depth) {
					PVEntry->exclusion = exclusion;
					PVEntry->ex_depth = ex_depth;
				}
			}
			return;
		}
		score = (Convert(PVEntry->date,int) << 3) + Convert(PVEntry->depth,int);
		if (score < min_score) {
			min_score = score;
			best = PVEntry;
		}
	}
	best->key = Low32(Current->key);
	best->date = date;
	best->value = value;
	best->depth = depth;
	best->move = move;
	best->exclusion = exclusion;
	best->ex_depth = ex_depth;
	best->knodes = knodes;
	best->ply = Current->ply;
}

template <bool pv> __forceinline int extension(int move, int depth) {
	register int ext = 0;
	if (pv) {
		if (T(Current->passer & Bit(From(move))) && CRank(Current->turn, From(move)) >= 5 && depth < 16) ext = 2;
	} else {
		if (T(Current->passer & Bit(From(move))) && CRank(Current->turn, From(move)) >= 5 && depth < 16) ext = 1; 
	}
	return ext;
}

void sort(int * start, int * finish) {
	for (int * p = start; p < finish - 1; p++) {
		int * best = p;
		int value = *p;
		int previous = *p;
		for (int * q = p + 1; q < finish; q++) if ((*q) > value) {
			value = *q;
			best = q;
		}
		*best = previous;
		*p = value;
	}
}

void sort_moves(int * start, int * finish) {
	for (int * p = start + 1; p < finish; p++) for (int * q = p - 1; q >= start; q--) if (((*q) >> 16) < ((*(q+1)) >> 16)) {
		int move = *q;
		*q = *(q+1);
		*(q+1)=move;
	}
}

__forceinline int pick_move() {
	register int move, *p, *best;
	move = *(Current->current);
	if (F(move)) return 0;
	best = Current->current;
	for (p = Current->current + 1; T(*p); p++) {
		if ((*p) > move) {
			best = p;
			move = *p;
		}
	}
	*best = *(Current->current);
	*(Current->current) = move;
	Current->current++;
	return move & 0xFFFF;
}

template <bool me> void gen_next_moves() {
	int *p, *q, *r;
	Current->gen_flags &= ~FlagSort;
	switch (Current->stage) {
	case s_hash_move: case r_hash_move: case e_hash_move:
		Current->moves[0] = Current->killer[0];
		Current->moves[1] = 0;
		return;
	case s_good_cap: 
		Current->mask = Piece(opp);
		r = gen_captures<me>(Current->moves);
		for (q = r - 1, p = Current->moves; q >= p;) {
		    int move = (*q) & 0xFFFF;
		    if (!see<me>(move,0)) {
			    int next = *p;
			    *p = *q;
			    *q = next;
			    p++;
		    } else q--;
	    }
		Current->start = p;
		Current->current = p;
		sort(p, r);
		return;
	case s_special:
		Current->current = Current->start;
		p = Current->start;
		if (Current->killer[1]) {*p = Current->killer[1]; p++;}
		if (Current->killer[2]) {*p = Current->killer[2]; p++;}
		if (Current->ref[0] && Current->ref[0] != Current->killer[1] && Current->ref[0] != Current->killer[2]) {*p = Current->ref[0]; p++;}
		if (Current->ref[1] && Current->ref[1] != Current->killer[1] && Current->ref[1] != Current->killer[2]) {*p = Current->ref[1]; p++;}
		*p = 0;
		return;
	case s_quiet: 
		gen_quiet_moves<me>(Current->start);
		Current->current = Current->start;
		Current->gen_flags |= FlagSort;
		return;
	case s_bad_cap:
		*(Current->start) = 0;
		Current->current = Current->moves;
		if (!(Current->gen_flags & FlagNoBcSort)) sort(Current->moves, Current->start);
		return;
	case r_cap:
		r = gen_captures<me>(Current->moves);
		Current->current = Current->moves;
		sort(Current->moves, r);
		return;
	case r_checks:
		r = gen_checks<me>(Current->moves);
		Current->current = Current->moves; 
		sort(Current->moves, r);
		return;
	case e_ev:
		Current->mask = Filled;
		r = gen_evasions<me>(Current->moves);
		mark_evasions(Current->moves);
		sort(Current->moves, r);
		Current->current = Current->moves;
		return;
	}
}

template <bool me, bool root> int get_move() {
	int move;
	
	if (root) {
		move = (*Current->current) & 0xFFFF;
		Current->current++;
		return move;
	}
start:
	if (F(*Current->current)) {
		Current->stage++;
		if ((1 << Current->stage) & StageNone) return 0;
		gen_next_moves<me>();
		goto start;
	}
	if (Current->gen_flags & FlagSort) move = pick_move();
	else {
		move = (*Current->current) & 0xFFFF;
		Current->current++;
	}
	if (Current->stage == s_quiet) { 
		if (move == Current->killer[1] || move == Current->killer[2] || move == Current->ref[0] || move == Current->ref[1]) goto start;
	} else if (Current->stage == s_special && (Square(To(move)) || !is_legal<me>(move))) goto start;
	return move;
}

template <bool me> int see(int move, int margin) {
	int from, to, piece, capture, delta, sq, pos;
	uint64 clear, def, att, occ, b_area, r_slider_att, b_slider_att, r_slider_def, b_slider_def, r_area, u, new_att, my_bishop, opp_bishop;
	from = From(move);
	to = To(move);
	piece = SeeValue[Square(from)];
	capture = SeeValue[Square(to)];
	delta = piece - capture;
	if (delta <= -margin) return 1;
	if (piece == SeeValue[WhiteKing]) return 1;
	if (Current->xray[me] & Bit(from)) return 1;
	if (T(Current->pin[me] & Bit(from)) && piece <= SeeValue[WhiteDark]) return 1;
	if (piece > (SeeValue[WhiteKing] >> 1)) return 1;
	if (IsEP(move)) return 1;
	if (F(Current->att[opp] & Bit(to))) return 1;
	att = PAtt[me][to] & Pawn(opp);
	if (T(att) && delta + margin > SeeValue[WhitePawn]) return 0;
	clear = ~Bit(from);
	def = PAtt[opp][to] & Pawn(me) & clear;
	if (T(def) && delta + SeeValue[WhitePawn] + margin <= 0) return 1;
	att |= NAtt[to] & Knight(opp);
	if (T(att) && delta > SeeValue[WhiteDark] - margin) return 0;
	occ = PieceAll & clear;
    b_area = BishopAttacks(to,occ);
	opp_bishop = Bishop(opp);
	if (delta > SeeValue[IDark(me)] - margin) if (b_area & opp_bishop) return 0;
	my_bishop = Bishop(me);
    b_slider_att = BMask[to] & (opp_bishop | Queen(opp));
	r_slider_att = RMask[to] & Major(opp);
	b_slider_def = BMask[to] & (my_bishop | Queen(me)) & clear;
	r_slider_def = RMask[to] & Major(me) & clear;
	att |= (b_slider_att & b_area);
	def |= NAtt[to] & Knight(me) & clear;
	r_area = RookAttacks(to,occ);
	att |= (r_slider_att & r_area);
	def |= (b_slider_def & b_area);
	def |= (r_slider_def & r_area);
	att |= SArea[to] & King(opp);
	def |= SArea[to] & King(me) & clear;
	while (true) {
		if (u = (att & Pawn(opp))) {
			capture -= piece;
			piece = SeeValue[WhitePawn];
			sq = lsb(u);
			occ ^= Bit(sq);
			att ^= Bit(sq);
			for (new_att = FullLine[to][sq] & b_slider_att & occ & (~att); T(new_att); Cut(new_att)) {
                pos = lsb(new_att);
				if (F(Between[to][pos] & occ)) {
                    Add(att,pos);
					break;
				}
			}
		} else if (u = (att & Knight(opp))) {
			capture -= piece;
			piece = SeeValue[WhiteKnight];
			att ^= (~(u-1)) & u;
		} else if (u = (att & opp_bishop)) {
            capture -= piece;
			piece = SeeValue[WhiteDark];
			sq = lsb(u);
			occ ^= Bit(sq);
			att ^= Bit(sq);
			for (new_att = FullLine[to][sq] & b_slider_att & occ & (~att); T(new_att); Cut(new_att)) {
                pos = lsb(new_att);
				if (F(Between[to][pos] & occ)) {
                    Add(att,pos);
					break;
				}
			}
		} else if (u = (att & Rook(opp))) {
            capture -= piece;
			piece = SeeValue[WhiteRook];
			sq = lsb(u);
			occ ^= Bit(sq);
			att ^= Bit(sq);
			for (new_att = FullLine[to][sq] & r_slider_att & occ & (~att); T(new_att); Cut(new_att)) {
                pos = lsb(new_att);
				if (F(Between[to][pos] & occ)) {
                    Add(att,pos);
					break;
				}
			}
		} else if (u = (att & Queen(opp))) {
            capture -= piece;
			piece = SeeValue[WhiteQueen];
			sq = lsb(u);
			occ ^= Bit(sq);
			att ^= Bit(sq);
			for (new_att = FullLine[to][sq] & (r_slider_att | b_slider_att) & occ & (~att); T(new_att); Cut(new_att)) {
                pos = lsb(new_att);
				if (F(Between[to][pos] & occ)) {
                    Add(att,pos);
					break;
				}
			}
		} else if (u = (att & King(opp))) {
            capture -= piece;
			piece = SeeValue[WhiteKing];
		} else return 1;
		if (capture < -(SeeValue[WhiteKing] >> 1)) return 0;
		if (piece + capture < margin) return 0;
		if (u = (def & Pawn(me))) {
            capture += piece;
			piece = SeeValue[WhitePawn];
            sq = lsb(u);
			occ ^= Bit(sq);
			def ^= Bit(sq);
			for (new_att = FullLine[to][sq] & b_slider_def & occ & (~att); T(new_att); Cut(new_att)) {
                pos = lsb(new_att);
				if (F(Between[to][pos] & occ)) {
                    Add(def,pos);
					break;
				}
			}
		} else if (u = (def & Knight(me))) {
            capture += piece;
			piece = SeeValue[WhiteKnight];
			def ^= (~(u-1)) & u;
		} else if (u = (def & my_bishop)) {
            capture += piece;
			piece = SeeValue[WhiteDark];
            sq = lsb(u);
			occ ^= Bit(sq);
			def ^= Bit(sq);
			for (new_att = FullLine[to][sq] & b_slider_def & occ & (~att); T(new_att); Cut(new_att)) {
                pos = lsb(new_att);
				if (F(Between[to][pos] & occ)) {
                    Add(def,pos);
					break;
				}
			}
		} else if (u = (def & Rook(me))) {
            capture += piece;
			piece = SeeValue[WhiteRook];
            sq = lsb(u);
			occ ^= Bit(sq);
			def ^= Bit(sq);
			for (new_att = FullLine[to][sq] & r_slider_def & occ & (~att); T(new_att); Cut(new_att)) {
                pos = lsb(new_att);
				if (F(Between[to][pos] & occ)) {
                    Add(def,pos);
					break;
				}
			}
		} else if (u = (def & Queen(me))) {
            capture += piece;
			piece = SeeValue[WhiteQueen];
			sq = lsb(u);
			occ ^= Bit(sq);
			def ^= Bit(sq);
			for (new_att = FullLine[to][sq] & (r_slider_def | b_slider_def) & occ & (~att); T(new_att); Cut(new_att)) {
                pos = lsb(new_att);
				if (F(Between[to][pos] & occ)) {
                    Add(def,pos);
					break;
				}
			}
		} else if (u = (def & King(me))) {
            capture += piece;
			piece = SeeValue[WhiteKing];
		} else return 0;
		if (capture > (SeeValue[WhiteKing] >> 1)) return 1;
		if (capture - piece >= margin) return 1;
	}
}

template <bool me> void gen_root_moves() {
	int i, *p, killer, depth = -256, move;
	GEntry * Entry;
	GPVEntry * PVEntry;

	killer = 0;
	if (Entry = probe_hash()) {
		if (T(Entry->move) && Entry->low_depth > depth) {
			depth = Entry->low_depth;
			killer = Entry->move;
		}
	}
	if (PVEntry = probe_pv_hash()) {
		if (PVEntry->depth > depth && T(PVEntry->move)) {
			depth = PVEntry->depth;
			killer = PVEntry->move;
		}
	}

	Current->killer[0] = killer;
	if (Check(me)) Current->stage = stage_evasion;
	else {
		Current->stage = stage_search;
		Current->ref[0] = RefM(Current->move).ref[0];
	    Current->ref[1] = RefM(Current->move).ref[1];
	}
	Current->gen_flags = 0;
	p = RootList;
	Current->current = Current->moves;
	Current->moves[0] = 0;
	while (move = get_move<me,0>()) {
		if (IsIllegal(me,move)) continue;
		if (p > RootList && move == killer) continue;
		if (SearchMoves) {
			for (i = 0; i < SMPointer; i++)
				if (SMoves[i] == move) goto keep_move;
			continue;
	    }
keep_move:
		*p = move;
		p++;
	}
	*p = 0;
}

template <bool me> int * gen_captures(int * list) {
	uint64 u, v;

	if (Current->ep_square)
		for (v = PAtt[opp][Current->ep_square] & Pawn(me); T(v); Cut(v)) AddMove(lsb(v),Current->ep_square,FlagEP,MvvLva[IPawn(me)][IPawn(opp)])
	for (u = Pawn(me) & Line(me,6); T(u); Cut(u))
    	if (F(Square(lsb(u) + Push(me)))) {
			AddMove(lsb(u),lsb(u) + Push(me),FlagPQueen,MvvLvaPromotion)
			if (NAtt[lsb(King(opp))] & Bit(lsb(u) + Push(me))) AddMove(lsb(u),lsb(u) + Push(me),FlagPKnight,MvvLvaPromotionKnight)
		}
	for (v = ShiftW(opp,Current->mask) & Pawn(me) & Line(me,6); T(v); Cut(v)) {
		AddMove(lsb(v),lsb(v)+PushE(me),FlagPQueen,MvvLvaPromotionCap(Square(lsb(v)+PushE(me))))
		if (NAtt[lsb(King(opp))] & Bit(lsb(v) + PushE(me))) AddMove(lsb(v),lsb(v)+PushE(me),FlagPKnight,MvvLvaPromotionKnightCap(Square(lsb(v)+PushE(me))))
	}
	for (v = ShiftE(opp,Current->mask) & Pawn(me) & Line(me,6); T(v); Cut(v)) {
		AddMove(lsb(v),lsb(v)+PushW(me),FlagPQueen,MvvLvaPromotionCap(Square(lsb(v)+PushW(me))))
		if (NAtt[lsb(King(opp))] & Bit(lsb(v) + PushW(me))) AddMove(lsb(v),lsb(v)+PushW(me),FlagPKnight,MvvLvaPromotionKnightCap(Square(lsb(v)+PushE(me))))
	}
	if (F(Current->att[me] & Current->mask)) goto finish;
	for (v = ShiftW(opp,Current->mask) & Pawn(me) & (~Line(me,6)); T(v); Cut(v)) AddCaptureP(IPawn(me),lsb(v),lsb(v)+PushE(me),0)
	for (v = ShiftE(opp,Current->mask) & Pawn(me) & (~Line(me,6)); T(v); Cut(v)) AddCaptureP(IPawn(me),lsb(v),lsb(v)+PushW(me),0)
	for (v = SArea[lsb(King(me))] & Current->mask & (~Current->att[opp]); T(v); Cut(v)) AddCaptureP(IKing(me),lsb(King(me)),lsb(v),0)
	for (u = Knight(me); T(u); Cut(u))
		for (v = NAtt[lsb(u)] & Current->mask; T(v); Cut(v)) AddCaptureP(IKnight(me),lsb(u),lsb(v),0)
	for (u = Bishop(me); T(u); Cut(u))
		for (v = BishopAttacks(lsb(u),PieceAll) & Current->mask; T(v); Cut(v)) AddCapture(lsb(u),lsb(v),0)
	for (u = Rook(me); T(u); Cut(u))
		for (v = RookAttacks(lsb(u),PieceAll) & Current->mask; T(v); Cut(v)) AddCaptureP(IRook(me),lsb(u),lsb(v),0)
	for (u = Queen(me); T(u); Cut(u))
		for (v = QueenAttacks(lsb(u),PieceAll) & Current->mask; T(v); Cut(v)) AddCaptureP(IQueen(me),lsb(u),lsb(v),0)
finish:
	*list = 0;
	return list;
}

template <bool me> int * gen_evasions(int * list) {
	int king, att_sq, from;
	uint64 att, esc, b, u;

	king = lsb(King(me));
	att = (NAtt[king] & Knight(opp)) | (PAtt[me][king] & Pawn(opp));
	for (u = (BMask[king] & BSlider(opp)) | (RMask[king] & RSlider(opp)); T(u); u ^= b) {
		b = Bit(lsb(u));
		if (F(Between[king][lsb(u)] & PieceAll)) att |= b;
	}
	att_sq = lsb(att);
	esc = SArea[king] & (~(Piece(me) | Current->att[opp])) & Current->mask;
	if (Square(att_sq) >= WhiteLight) esc &= ~FullLine[king][att_sq];
	Cut(att);
	if (att) {
		att_sq = lsb(att);
		if (Square(att_sq) >= WhiteLight) esc &= ~FullLine[king][att_sq];
		for (; T(esc); Cut(esc)) AddCaptureP(IKing(me),king,lsb(esc),0)
		*list = 0;
		return list;
	}
	if (Bit(att_sq) & Current->mask) {
	    if (T(Current->ep_square) && Current->ep_square == att_sq + Push(me))
		    for (u = PAtt[opp][att_sq + Push(me)] & Pawn(me); T(u); Cut(u)) AddMove(lsb(u),att_sq + Push(me),FlagEP,MvvLva[IPawn(me)][IPawn(opp)])
	}
	for (u = PAtt[opp][att_sq] & Pawn(me); T(u); Cut(u)) {
        from = lsb(u);
		if (Bit(att_sq) & Line(me,7)) AddMove(from,att_sq,FlagPQueen,MvvLvaPromotionCap(Square(att_sq)))
		else if (Bit(att_sq) & Current->mask) AddCaptureP(IPawn(me),from,att_sq,0)
	}
	for ( ; T(esc); Cut(esc)) AddCaptureP(IKing(me),king,lsb(esc),0)
	att = Between[king][att_sq];
	for (u = Shift(opp,att) & Pawn(me); T(u); Cut(u)) {
        from = lsb(u);
		if (Bit(from) & Line(me,6)) AddMove(from,from + Push(me),FlagPQueen,MvvLvaPromotion)
		else if (F(~Current->mask)) AddMove(from,from + Push(me),0,0)
	}
	if (F(~Current->mask)) {
	    for (u = Shift(opp,Shift(opp,att)) & Line(me, 1) & Pawn(me); T(u); Cut(u))
            if (F(Square(lsb(u)+Push(me)))) AddMove(lsb(u),lsb(u) + 2 * Push(me),0,0)
    }
	att |= Bit(att_sq);
	for (u = Knight(me); T(u); Cut(u))
        for (esc = NAtt[lsb(u)] & att; T(esc); esc ^= b) {
			b = Bit(lsb(esc));
			if (b & Current->mask) AddCaptureP(IKnight(me),lsb(u),lsb(esc),0)
		}
	for (u = Bishop(me); T(u); Cut(u))
        for (esc = BishopAttacks(lsb(u),PieceAll) & att; T(esc); esc ^= b) {
			b = Bit(lsb(esc));
			if (b & Current->mask) AddCapture(lsb(u),lsb(esc),0)
		}
	for (u = Rook(me); T(u); Cut(u))
        for (esc = RookAttacks(lsb(u),PieceAll) & att; T(esc); esc ^= b) {
			b = Bit(lsb(esc));
			if (b & Current->mask) AddCaptureP(IRook(me),lsb(u),lsb(esc),0)
		}
	for (u = Queen(me); T(u); Cut(u))
        for (esc = QueenAttacks(lsb(u),PieceAll) & att; T(esc); esc ^= b) {
			b = Bit(lsb(esc));
			if (b & Current->mask) AddCaptureP(IQueen(me),lsb(u),lsb(esc),0)
		}
	*list = 0;
	return list;
}

void mark_evasions(int * list) {
	for (; T(*list); list++) {
		register int move = (*list) & 0xFFFF;
	    if (F(Square(To(move))) && F(move & 0xE000)) {
			if (move == Current->ref[0]) *list |= RefOneScore;
			else if (move == Current->ref[1]) *list |= RefTwoScore;
			else if (move == Current->killer[1]) *list |= KillerOneScore;
			else if (move == Current->killer[2]) *list |= KillerTwoScore;
			else *list |= HistoryP(Square(From(move)),From(move),To(move));
		}
	}
}

template <bool me> int * gen_quiet_moves(int * list) {
	int to;
	uint64 u, v, free, occ;

    occ = PieceAll;
	free = ~occ;
	if (me == White) {
		if (T(Current->castle_flags & CanCastle_OO) && F(occ & 0x60) && F(Current->att[Black] & 0x70)) AddHistoryP(IKing(White),4,6,FlagCastling)
	    if (T(Current->castle_flags & CanCastle_OOO) && F(occ & 0xE) && F(Current->att[Black] & 0x1C)) AddHistoryP(IKing(White),4,2,FlagCastling)
	} else {
		if (T(Current->castle_flags & CanCastle_oo) && F(occ & 0x6000000000000000) && F(Current->att[White] & 0x7000000000000000)) AddHistoryP(IKing(Black),60,62,FlagCastling)
	    if (T(Current->castle_flags & CanCastle_ooo) && F(occ & 0x0E00000000000000) && F(Current->att[White] & 0x1C00000000000000)) AddHistoryP(IKing(Black),60,58,FlagCastling)
	}
	for (v = Shift(me,Pawn(me)) & free & (~Line(me,7)); T(v); Cut(v)) {
        to = lsb(v);
	    if (T(Bit(to) & Line(me,2)) && F(Square(to + Push(me)))) AddHistoryP(IPawn(me),to - Push(me),to + Push(me),0)
		AddHistoryP(IPawn(me),to - Push(me),to,0)
	}
	for (u = Knight(me); T(u); Cut(u))
		for (v = free & NAtt[lsb(u)]; T(v); Cut(v)) AddHistoryP(IKnight(me),lsb(u),lsb(v),0)
	for (u = Bishop(me); T(u); Cut(u))
		for (v = free & BishopAttacks(lsb(u),occ); T(v); Cut(v)) AddHistory(lsb(u),lsb(v))
	for (u = Rook(me); T(u); Cut(u))
		for (v = free & RookAttacks(lsb(u),occ); T(v); Cut(v)) AddHistoryP(IRook(me),lsb(u),lsb(v),0)
	for (u = Queen(me); T(u); Cut(u))
		for (v = free & QueenAttacks(lsb(u),occ); T(v); Cut(v)) AddHistoryP(IQueen(me),lsb(u),lsb(v),0)
	for (v = SArea[lsb(King(me))] & free & (~Current->att[opp]); T(v); Cut(v)) AddHistoryP(IKing(me),lsb(King(me)),lsb(v),0)
	*list = 0;
	return list;
}

template <bool me> int * gen_checks(int * list) {
	int king, from;
    uint64 u, v, target, b_target, r_target, clear, xray;

	clear = ~(Piece(me) | Current->mask);
    king = lsb(King(opp));
	for (u = Current->xray[me] & Piece(me); T(u); Cut(u)) {
		from = lsb(u);
		target = clear & (~FullLine[king][from]);
		if (Square(from) == IPawn(me)) {
			if (F(Bit(from + Push(me)) & Line(me,7))) {
			    if (T(Bit(from + Push(me)) & target) && F(Square(from + Push(me)))) AddMove(from,from + Push(me),0,MvvLvaXray)
				for (v = PAtt[me][from] & target & Piece(opp); T(v); Cut(v)) AddMove(from,lsb(v),0,MvvLvaXrayCap(Square(lsb(v))))
			}
		} else {
			if (Square(from) < WhiteLight) v = NAtt[from] & target;
			else if (Square(from) < WhiteRook) v = BishopAttacks(from,PieceAll) & target;
			else if (Square(from) < WhiteQueen) v = RookAttacks(from,PieceAll) & target;
			else if (Square(from) < WhiteKing) v = QueenAttacks(from,PieceAll) & target;
			else v = SArea[from] & target & (~Current->att[opp]);
			for ( ; T(v); Cut(v)) AddMove(from,lsb(v),0,MvvLvaXrayCap(Square(lsb(v))))
		}
	}
	xray = ~(Current->xray[me] & Board->bb[me]);
	for (u = Knight(me) & NArea[king] & xray; T(u); Cut(u))
		for (v = NAtt[king] & NAtt[lsb(u)] & clear; T(v); Cut(v)) AddCaptureP(IKnight(me),lsb(u),lsb(v),0)
    for (u = DArea[king] & Pawn(me) & (~Line(me,6)) & xray; T(u); Cut(u)) {
		from = lsb(u);
		for (v = PAtt[me][from] & PAtt[opp][king] & clear & Piece(opp); T(v); Cut(v)) AddCaptureP(IPawn(me),from,lsb(v),0)
		if (F(Square(from + Push(me))) && T(Bit(from + Push(me)) & PAtt[opp][king])) AddMove(from,from + Push(me),0,0)
	}
	b_target = BishopAttacks(king,PieceAll) & clear;
	r_target = RookAttacks(king,PieceAll) & clear;
	for (u = (Odd(king ^ Rank(king)) ? Board->bb[WhiteLight | me] : Board->bb[WhiteDark | me]) & xray; T(u); Cut(u))
		for (v = BishopAttacks(lsb(u),PieceAll) & b_target; T(v); Cut(v)) AddCapture(lsb(u),lsb(v),0)
	for (u = Rook(me) & xray; T(u); Cut(u)) 
		for (v = RookAttacks(lsb(u),PieceAll) & r_target; T(v); Cut(v)) AddCaptureP(IRook(me),lsb(u),lsb(v),0)
	for (u = Queen(me) & xray; T(u); Cut(u)) 
		for (v = QueenAttacks(lsb(u),PieceAll) & (b_target | r_target); T(v); Cut(v)) AddCaptureP(IQueen(me),lsb(u),lsb(v),0)
	*list = 0;
	return list;
}

template <bool me> int * gen_delta_moves(int * list) {
	int to;
	uint64 u, v, free, occ;

    occ = PieceAll;
	free = ~occ;
	if (me == White) {
		if (T(Current->castle_flags & CanCastle_OO) && F(occ & 0x60) && F(Current->att[Black] & 0x70)) AddCDeltaP(IKing(White),4,6,FlagCastling)
	    if (T(Current->castle_flags & CanCastle_OOO) && F(occ & 0xE) && F(Current->att[Black] & 0x1C)) AddCDeltaP(IKing(White),4,2,FlagCastling)
	} else {
		if (T(Current->castle_flags & CanCastle_oo) && F(occ & 0x6000000000000000) && F(Current->att[White] & 0x7000000000000000)) AddCDeltaP(IKing(Black),60,62,FlagCastling)
	    if (T(Current->castle_flags & CanCastle_ooo) && F(occ & 0x0E00000000000000) && F(Current->att[White] & 0x1C00000000000000)) AddCDeltaP(IKing(Black),60,58,FlagCastling)
	}
	for (v = Shift(me,Pawn(me)) & free & (~Line(me,7)); T(v); Cut(v)) {
        to = lsb(v);
	    if (T(Bit(to) & Line(me,2)) && F(Square(to + Push(me)))) AddCDeltaP(IPawn(me),to - Push(me),to + Push(me),0)
		AddCDeltaP(IPawn(me),to - Push(me),to,0)
	}
	for (u = Knight(me); T(u); Cut(u))
		for (v = free & NAtt[lsb(u)]; T(v); Cut(v)) AddCDeltaP(IKnight(me),lsb(u),lsb(v),0)
	for (u = Bishop(me); T(u); Cut(u))
		for (v = free & BishopAttacks(lsb(u),occ); T(v); Cut(v)) AddCDelta(lsb(u),lsb(v))
	for (u = Rook(me); T(u); Cut(u))
		for (v = free & RookAttacks(lsb(u),occ); T(v); Cut(v)) AddCDeltaP(IRook(me),lsb(u),lsb(v),0)
	for (u = Queen(me); T(u); Cut(u))
		for (v = free & QueenAttacks(lsb(u),occ); T(v); Cut(v)) AddCDeltaP(IQueen(me),lsb(u),lsb(v),0)
	for (v = SArea[lsb(King(me))] & free & (~Current->att[opp]); T(v); Cut(v)) AddCDeltaP(IKing(me),lsb(King(me)),lsb(v),0)
	*list = 0;
	return list;
}

template <bool me> int singular_extension(int ext, int prev_ext, int margin_one, int margin_two, int depth, int killer) {
	int value = -MateValue;
	int singular = 0;
	if (ext < 1 + (prev_ext < 1)) {
		if (Check(me)) value = search_evasion<me, 1>(margin_one, depth, killer); 
		else value = search<me, 1>(margin_one, depth, killer); 
		if (value < margin_one) singular = 1;
	}
	if (value < margin_one && ext < 2 + (prev_ext < 1) - (prev_ext >= 2)) {
		if (Check(me)) value = search_evasion<me, 1>(margin_two, depth, killer); 
		else value = search<me, 1>(margin_two, depth, killer); 
		if (value < margin_two) singular = 2;
	}
	return singular;
}

template <bool me> __forceinline void capture_margin(int alpha, int &score) {
	if (Current->score + 200 < alpha) {
		if (Current->att[me] & Pawn(opp)) {
			Current->mask ^= Pawn(opp);
			score = Current->score + 200;
		}
		if (Current->score + 500 < alpha) {
			if (Current->att[me] & Minor(opp)) {
				Current->mask ^= Minor(opp);
				score = Current->score + 500;
			}
			if (Current->score + 700 < alpha) {
				if (Current->att[me] & Rook(opp)) {
					Current->mask ^= Rook(opp);
					score = Current->score + 700;
				}
				if (Current->score + 1400 < alpha && (Current->att[me] & Queen(opp))) {
					Current->mask ^= Queen(opp);
					score = Current->score + 1400;
				}
			}
		}
	}
}

template <bool me, bool pv> int q_search(int alpha, int beta, int depth, int flags) {
	int i, value, score, move, hash_move, hash_depth, cnt;
	GEntry * Entry;

	if (flags & FlagHaltCheck) halt_check;
#ifdef CPU_TIMING
#ifndef TIMING
	if (nodes > check_node + 0x4000) {
#else
	if (nodes > check_node + 0x100) {
#endif
		check_node = nodes;
#ifdef TIMING
		if (LastDepth >= 6)
#endif
		check_time(1);
#ifdef TUNER
		if (nodes > 64 * 1024 * 1024) longjmp(Jump, 1);
#endif
	}
#endif
	if (flags & FlagCallEvaluation) evaluate();
	if (Check(me)) return q_evasion<me, pv>(alpha, beta, depth, FlagHashCheck);
	score = Current->score + 3;
	if (score > alpha) {
		alpha = score;
		if (score >= beta) return score;
	}

	hash_move = hash_depth = 0;
	if (flags & FlagHashCheck) {
	    for (i = 0, Entry = Hash + (High32(Current->key) & hash_mask); i < 4; Entry++, i++) {
		    if (Low32(Current->key) == Entry->key) {
			    if (T(Entry->low_depth)) {
				    if (Entry->low >= beta && !pv) return Entry->low;
				    if (Entry->low_depth > hash_depth && T(Entry->move)) {
					    hash_move = Entry->move;
					    hash_depth = Entry->low_depth;
				    }
			    }
			    if (T(Entry->high_depth) && Entry->high <= alpha && !pv) return Entry->high;
				break;
		    }
	    }
	}

	Current->mask = Piece(opp);
	capture_margin<me>(alpha, score);

	cnt = 0;
	if (T(hash_move)) {
		if (F(Bit(To(hash_move)) & Current->mask) && F(hash_move & 0xE000) && (depth < -8 || (Current->score + DeltaM(hash_move) <= alpha && F(is_check<me>(hash_move))))) goto skip_hash_move;
		if (is_legal<me>(move = hash_move)) {
			if (IsIllegal(me,move)) goto skip_hash_move;
			if (SeeValue[Square(To(move))] > SeeValue[Square(From(move))]) cnt++;
			do_move<me>(move);
		    value = -q_search<opp, pv>(-beta, -alpha, depth - 1, FlagNeatSearch);
		    undo_move<me>(move);
			if (value > score) {
			    score = value;
			    if (value > alpha) {
				    alpha = value;
			        if (value >= beta) goto cut;
			    }
		    }
			if (F(Bit(To(hash_move)) & Current->mask) && F(hash_move & 0xE000) && (depth < -2 || depth <= -1 && Current->score + 50 < alpha) && alpha >= beta - 1 && !pv) return alpha;
		}
	}
skip_hash_move:
	gen_captures<me>(Current->moves);
	Current->current = Current->moves;
	while (move = pick_move()) {
		if (move == hash_move) continue;
		if (IsIllegal(me,move)) continue;
		if (F(see<me>(move,-50))) continue;
		if (SeeValue[Square(To(move))] > SeeValue[Square(From(move))]) cnt++;
		do_move<me>(move);
		value = -q_search<opp, pv>(-beta, -alpha, depth - 1, FlagNeatSearch);
		undo_move<me>(move);
		if (value > score) {
			score = value;
			if (value > alpha) {
				alpha = value;
			    if (value >= beta) goto cut;
			}
		}
	}

	if (depth < -2) goto finish;
	if (depth <= -1 && Current->score + 50 < alpha) goto finish;
	gen_checks<me>(Current->moves);
	Current->current = Current->moves;
	while (move = pick_move()) {
		if (move == hash_move) continue;
		if (IsIllegal(me,move)) continue;
		if (IsRepetition(alpha + 1,move)) continue;
		if (F(see<me>(move,-50))) continue;
		do_move<me>(move);
		value = -q_evasion<opp, pv>(-beta, -alpha, depth - 1, FlagNeatSearch);
		undo_move<me>(move);
		if (value > score) {
			score = value;
			if (value > alpha) {
				alpha = value;
			    if (value >= beta) goto cut;
			}
		}
	}

	if (T(cnt) || Current->score + 30 < alpha || T(Current->threat & Piece(me)) || T((Current->xray[opp] | Current->pin[opp]) & NonPawn(opp)) 
		|| T(Pawn(opp) & Line(me, 1) & Shift(me,~PieceAll))) goto finish;
	Current->margin = alpha - Current->score + 6;
	gen_delta_moves<me>(Current->moves);
	Current->current = Current->moves;
	while (move = pick_move()) {
		if (move == hash_move) continue;
		if (IsIllegal(me,move)) continue;
		if (IsRepetition(alpha + 1,move)) continue;
		if (F(see<me>(move,-50))) continue;
		cnt++;
		do_move<me>(move);
		value = -q_search<opp, pv>(-beta, -alpha, depth - 1, FlagNeatSearch);
		undo_move<me>(move);
		if (value > score) {
			score = value;
			if (value > alpha) {
				alpha = value;
			    if (value >= beta) {
					if (Current->killer[1] != move) {
						Current->killer[2] = Current->killer[1];
						Current->killer[1] = move;
					}
					goto cut;
				}
			}
		}
		if (cnt >= 3) break; 
	}

finish:
	if (depth >= -2 && (depth >= 0 || Current->score + 50 >= alpha)) hash_high(score, 1);
	return score;
cut:
	hash_low(move, score, 1);
	return score;
}

template <bool me, bool pv> int q_evasion(int alpha, int beta, int depth, int flags) {
	int i, value, pext, score, move, cnt, hash_move, hash_depth;
	int *p;
	GEntry * Entry;

	score = Convert((Current - Data),int) - MateValue;
	if (flags & FlagHaltCheck) halt_check;

	hash_move = hash_depth = 0;
	if (flags & FlagHashCheck) {
	    for (i = 0, Entry = Hash + (High32(Current->key) & hash_mask); i < 4; Entry++, i++) {
		    if (Low32(Current->key) == Entry->key) {
			    if (T(Entry->low_depth)) {
				    if (Entry->low >= beta && !pv) return Entry->low;
				    if (Entry->low_depth > hash_depth && T(Entry->move)) {
					    hash_move = Entry->move;
					    hash_depth = Entry->low_depth;
				    }
			    }
			    if (T(Entry->high_depth) && Entry->high <= alpha && !pv) return Entry->high;
				break;
		    }
	    }
	}

	if (flags & FlagCallEvaluation) evaluate();
	Current->mask = Filled;
	if (Current->score - 10 <= alpha && !pv) {
		Current->mask = Piece(opp);
		score = Current->score - 10;
		capture_margin<me>(alpha, score);
	}

	alpha = Max(score, alpha);
	pext = 0;
	gen_evasions<me>(Current->moves);
	Current->current = Current->moves;
	if (F(Current->moves[0])) return score;
	if (F(Current->moves[1])) pext = 1;
	else {
		Current->ref[0] = RefM(Current->move).check_ref[0];
		Current->ref[1] = RefM(Current->move).check_ref[1];
		mark_evasions(Current->moves);
	    if (T(hash_move) && (T(Bit(To(hash_move)) & Current->mask) || T(hash_move & 0xE000))) {
	        for (p = Current->moves; T(*p); p++) {
		        if (((*p) & 0xFFFF) == hash_move) {
				    *p |= 0x7FFF0000;
				    break;
			    }
	        }
	    }
	}
	cnt = 0;
	while (move = pick_move()) {
		if (IsIllegal(me,move)) continue;
		cnt++;
		if (IsRepetition(alpha + 1,move)) {
			score = Max(0, score);
			continue;
		}
		if (F(Square(To(move))) && F(move & 0xE000)) {
			if (cnt > 3 && F(is_check<me>(move)) && !pv) continue;
			if ((value = Current->score + DeltaM(move) + 10) <= alpha && !pv) {
				score = Max(value, score);
				continue;
			}
		}
		do_move<me>(move);
		value = -q_search<opp, pv>(-beta, -alpha, depth - 1 + pext, FlagNeatSearch);
		undo_move<me>(move);
		if (value > score) {
			score = value;
			if (value > alpha) {
				alpha = value;
			    if (value >= beta) goto cut;
			}
		}
	}
	return score;
cut:
	return score;
}

void send_position(GPos * Pos) {
	Pos->Position->key = Current->key;
	Pos->Position->pawn_key = Current->pawn_key;
	Pos->Position->move = Current->move;
	Pos->Position->capture = Current->capture;
	Pos->Position->turn = Current->turn;
	Pos->Position->castle_flags = Current->castle_flags;
	Pos->Position->ply = Current->ply;
	Pos->Position->ep_square = Current->ep_square;
	Pos->Position->piece = Current->piece;
	Pos->Position->pst = Current->pst;
	Pos->Position->material = Current->material;
	for (int i = 0; i < 64; i++) Pos->Position->square[i] = Board->square[i];
	Pos->date = date;
	Pos->sp = sp;
	for (int i = 0; i <= Current->ply; i++) Pos->stack[i] = Stack[sp - i];
	for (int i = 0; i < Min(16, 126 - (int)(Current - Data)); i++) {
		Pos->killer[i][0] = (Current + i + 1)->killer[1];
		Pos->killer[i][1] = (Current + i + 1)->killer[2];
	}
	for (int i = Min(16, 126 - (int)(Current - Data)); i < 16; i++) Pos->killer[i][0] = Pos->killer[i][1] = 0;
}

void retrieve_board(GPos * Pos) {
	for (int i = 0; i < 16; i++) Board->bb[i] = 0;
	for (int i = 0; i < 64; i++) {
		int piece = Pos->Position->square[i];
		Board->square[i] = piece;
		if (piece) {
			Board->bb[piece & 1] |= Bit(i);
			Board->bb[piece] |= Bit(i);
		}
	}
}

void retrieve_position(GPos * Pos, int copy_stack) {
	Current->key = Pos->Position->key;
	Current->pawn_key = Pos->Position->pawn_key;
	Current->move = Pos->Position->move;
	Current->capture = Pos->Position->capture;
	Current->turn = Pos->Position->turn;
	Current->castle_flags = Pos->Position->castle_flags;
	Current->ply = Pos->Position->ply;
	Current->ep_square = Pos->Position->ep_square;
	Current->piece = Pos->Position->piece;
	Current->pst = Pos->Position->pst;
	Current->material = Pos->Position->material;
	retrieve_board(Pos);
	date = Pos->date;
	if (copy_stack) {
		sp = Current->ply;
		for (int i = 0; i <= sp; i++) Stack[sp - i] = Pos->stack[i];
	} else sp = Pos->sp;
	for (int i = 0; i < 16; i++) {
		(Current + i + 1)->killer[1] = Pos->killer[i][0];
		(Current + i + 1)->killer[2] = Pos->killer[i][1];
	}
}

void halt_all(GSP * Sp, int locked) {
	GMove * M;
	if (!locked) LOCK(Sp->lock);
	if (Sp->active) {
		for (int i = 0; i < Sp->move_number; i++) {
			M = &Sp->move[i];
			if ((M->flags & FlagClaimed) && !(M->flags & FlagFinished) && M->id != Id) SET_BIT_64(Smpi->stop, M->id);
		}
		Sp->active = Sp->claimed = 0;
		ZERO_BIT_64(Smpi->active_sp, (int)(Sp - Smpi->Sp));
	}
	if (!locked) UNLOCK(Sp->lock);
}

void halt_all(int from, int to) {
	for (uint64 u = Smpi->active_sp; u; Cut(u)) {
		GSP * Sp = &Smpi->Sp[lsb(u)];
		LOCK(Sp->lock);
		if (Sp->height >= from && Sp->height <= to) halt_all(Sp, 1);
		UNLOCK(Sp->lock);
	}
}

void init_sp(GSP * Sp, int alpha, int beta, int depth, int pv, int singular, int height) {
	Sp->claimed = 1;
	Sp->active = Sp->finished = 0;
	Sp->best_move = 0;
	Sp->alpha = alpha;
	Sp->beta = beta;
	Sp->depth = depth;
	Sp->split = 0;
	Sp->singular = singular;
	Sp->height = height;
	Sp->move_number = 0;
	Sp->pv = pv;
}

template <bool me> int smp_search(GSP * Sp) {
	int i, value, move, alpha, iter = 0;
	if (!Sp->move_number) return Sp->alpha;
	send_position(Sp->Pos);
	if (setjmp(Sp->jump)) {
		LOCK(Sp->lock);
		halt_all(Sp, 1);
		UNLOCK(Sp->lock);
		halt_all(Sp->height + 1, 127);
		Current = Data + Sp->height;
		sp = Sp->Pos->sp;
		retrieve_board(Sp->Pos);
		return Sp->beta;
	}
	LOCK(Sp->lock);
	SET_BIT_64(Smpi->active_sp, (int)(Sp - Smpi->Sp));
	Sp->active = 1;
	Sp->claimed = Sp->finished = 0;
loop:
	for (i = 0; i < Sp->move_number; i++) {
		GMove * M = &Sp->move[i];
		if (!iter) Sp->current = i;
		if (M->flags & FlagFinished) continue;
		if (!iter) {
			if (M->flags & FlagClaimed) continue;
			M->flags |= FlagClaimed;
			M->id = Id;
		} else if (M->flags & FlagClaimed) {
			SET_BIT_64(Smpi->stop, M->id);
			M->id = Id;
		}
		move = M->move;
		alpha = Sp->alpha;
		UNLOCK(Sp->lock);
		do_move<me>(move);
		value = -search<opp, 0>(-alpha, M->reduced_depth, FlagNeatSearch | ExtFlag(M->ext));
		if (value > alpha && (Sp->pv || M->reduced_depth < M->research_depth)) {
			if (Sp->pv) value = -pv_search<opp, 0>(-Sp->beta, -Sp->alpha, M->research_depth, FlagNeatSearch | ExtFlag(M->ext));
			else value = -search<opp, 0>(-alpha, M->research_depth, FlagNeatSearch | FlagDisableNull | ExtFlag(M->ext));
		}
		undo_move<me>(move);
		LOCK(Sp->lock);
		if (Sp->finished) goto cut;
		M->flags |= FlagFinished;
		if (value > Sp->alpha) {
			Sp->best_move = move;
			Sp->alpha = Min(value, Sp->beta);
			if (value >= Sp->beta) goto cut;
		}
	}
	if (!iter) {
		iter++;
		goto loop;
	}
	halt_all(Sp, 1);
	UNLOCK(Sp->lock);
	return Sp->alpha;
cut:
	halt_all(Sp, 1);
	UNLOCK(Sp->lock);
	return Sp->beta;
}

template <bool me, bool exclusion> int search(int beta, int depth, int flags) {
	int i, value, cnt, flag, moves_to_play, check, score, move, ext, margin, hash_move, do_split, sp_init, singular, played,
		high_depth, high_value, hash_value, new_depth, move_back, hash_depth, *p;
	int height = (int)(Current - Data);
	GSP * Sp;

#ifndef TUNER
	if (nodes > check_node_smp + 0x10) {
#ifndef W32_BUILD
		InterlockedAdd64(&Smpi->nodes, (long long)(nodes)-(long long)(check_node_smp));
#else
		Smpi->nodes += (long long)(nodes)-(long long)(check_node_smp);
#endif
		check_node_smp = nodes;
		check_state();
		if (nodes > check_node + 0x4000 && parent) {
			check_node = nodes;
			check_time(1);
			if (Searching) SET_BIT_64(Smpi->searching, Id); // BUG, don't know why this is necessary
		}
	}
#endif

	if (depth <= 1) return q_search<me, 0>(beta - 1, beta, 1, flags);
	if (flags & FlagHaltCheck) {
	    if (height - MateValue >= beta) return beta;
	    if (MateValue - height < beta) return beta - 1;
	    halt_check;
	}

	if (exclusion) {
		cnt = high_depth = do_split = sp_init = singular = played = 0;
		flag = 1;
		score = beta - 1;
		high_value = MateValue; 
		hash_value = -MateValue;
		hash_depth = -1;
		hash_move = flags & 0xFFFF;
		goto skip_hash_move;
	}

	if (flags & FlagCallEvaluation) evaluate();
	if (Check(me)) return search_evasion<me, 0>(beta, depth, flags & (~(FlagHaltCheck | FlagCallEvaluation)));

	if ((value = Current->score - 90 - (depth << 3) - (Max(depth - 5, 0) << 5)) >= beta && F(Pawn(opp) & Line(me, 1) & Shift(me,~PieceAll)) && T(NonPawnKing(me)) && F(flags & (FlagReturnBestMove | FlagDisableNull)) && depth <= 13) return value;
	if ((value = Current->score + 50) < beta && depth <= 3) return MaxF(value, q_search<me, 0>(beta - 1, beta, 1, FlagHashCheck | (flags & 0xFFFF)));

	high_depth = 0;
	high_value = MateValue;
	hash_value = -MateValue;
	hash_depth = -1;
	Current->best = hash_move = flags & 0xFFFF;
	if (GEntry * Entry = probe_hash()) {
		if (Entry->high_depth > high_depth) {
			high_depth = Entry->high_depth;
			high_value = Entry->high;
		}
		if (Entry->high < beta && Entry->high_depth >= depth) return Entry->high;
		if (T(Entry->move) && Entry->low_depth > hash_depth) {
			Current->best = hash_move = Entry->move;
			hash_depth = Entry->low_depth;
			if (Entry->low_depth) hash_value = Entry->low;
		}
		if (Entry->low >= beta && Entry->low_depth >= depth) {
			if (Entry->move) {
				Current->best = Entry->move;
				if (F(Square(To(Entry->move))) && F(Entry->move & 0xE000)) {
					if (Current->killer[1] != Entry->move && F(flags & FlagNoKillerUpdate)) {
						Current->killer[2] = Current->killer[1];
						Current->killer[1] = Entry->move;
					}
					UpdateRef(Entry->move);
				}
				return Entry->low;
			}
			if (F(flags & FlagReturnBestMove)) return Entry->low;
		}
	}
	if (depth >= 20) if (GPVEntry * PVEntry = probe_pv_hash()) {
		hash_low(PVEntry->move,PVEntry->value,PVEntry->depth);
		hash_high(PVEntry->value,PVEntry->depth);
		if (PVEntry->depth >= depth) {
			if (PVEntry->move) Current->best = PVEntry->move;
			if (F(flags & FlagReturnBestMove) && ((Current->ply <= 50 && PVEntry->ply <= 50) || (Current->ply >= 50 && PVEntry->ply >= 50))) return PVEntry->value;
		}
		if (T(PVEntry->move) && PVEntry->depth > hash_depth) {
			Current->best = hash_move = PVEntry->move;
			hash_depth = PVEntry->depth;
			hash_value = PVEntry->value;
		}
	}
	if (depth < 10) score = height - MateValue;
	else score = beta - 1;
	if (depth >= 12 && (F(hash_move) || hash_value < beta || hash_depth < depth - 12) && (high_value >= beta || high_depth < depth - 12) && F(flags & FlagDisableNull)) {
		new_depth = depth - 8;
		value = search<me, 0>(beta, new_depth, FlagHashCheck | FlagNoKillerUpdate | FlagDisableNull | FlagReturnBestMove | hash_move);
		if (value >= beta) {
			if (Current->best) hash_move = Current->best;
			hash_depth = new_depth;
			hash_value = beta;
		}
	}
	if (depth >= 4 && Current->score + 3 >= beta && F(flags & (FlagDisableNull | FlagReturnBestMove))
		&& (high_value >= beta || high_depth < depth - 10) && (depth < 12 || (hash_value >= beta && hash_depth >= depth - 12)) && beta > -EvalValue && T(NonPawnKing(me))) {
		new_depth = depth - 8;
		do_null();
	    value = -search<opp, 0>(1 - beta, new_depth, FlagHashCheck);
		undo_null();
		if (value >= beta) {
			if (depth < 12) hash_low(0, value, depth);
			return value;
		}
	}

	cnt = flag = singular = played = 0;
	if (T(hash_move) && is_legal<me>(move = hash_move)) {
		if (IsIllegal(me,move)) goto skip_hash_move;
		cnt++;
		check = is_check<me>(move);
		if (check) ext = 1 + (depth < 16);
		else ext = extension<0>(move, depth);
		if (depth >= 16 && hash_value >= beta && hash_depth >= (new_depth = depth - Min(12, depth/2))) {
			int margin_one = beta - ExclSingle(depth);
			int margin_two = beta - ExclDouble(depth);
			int prev_ext = Ext(flags);
			singular = singular_extension<me>(ext,prev_ext,margin_one,margin_two,new_depth,hash_move);
			if (singular) ext = Max(ext, singular + (prev_ext < 1) - (singular >= 2 && prev_ext >= 2));
		}
		if (depth < 16 && To(move) == To(Current->move) && T(Square(To(move)))) ext = Max(ext, 2);
		new_depth = depth - 2 + ext;
		do_move<me>(move);
		value = -search<opp, 0>(1 - beta, new_depth, FlagNeatSearch | ((hash_value >= beta && hash_depth >= depth - 12) ? FlagDisableNull : 0) | ExtFlag(ext));
		undo_move<me>(move);
		played++;
		if (value > score) {
			score = value;
			if (value >= beta) goto cut;
		}
	}
skip_hash_move:
	Current->killer[0] = 0;
	Current->stage = stage_search;
	Current->gen_flags = 0;
	Current->ref[0] = RefM(Current->move).ref[0];
	Current->ref[1] = RefM(Current->move).ref[1];
	move_back = 0;
	if (beta > 0 && Current->ply >= 2) {
		if (F((Current - 1)->move & 0xF000)) {
			move_back = (To((Current - 1)->move) << 6) | From((Current - 1)->move);
			if (Square(To(move_back))) move_back = 0;
		}
	}
	moves_to_play = 3 + (depth * depth)/6;
	margin = Current->score + 70 + (depth << 3) + (Max(depth - 7, 0) << 5);
	if ((value = margin) < beta && depth <= 19) {
		flag = 1;
		score = Max(value, score);
		Current->stage = stage_razoring;
		Current->mask = Piece(opp);
		if ((value = Current->score + 200 + (depth << 5)) < beta) {
			score = Max(value, score);
			Current->mask ^= Pawn(opp);
		}
	}
	Current->current = Current->moves;
	Current->moves[0] = 0;
	if (depth <= 5) Current->gen_flags |= FlagNoBcSort;
	
	do_split = sp_init = 0;
	if (depth >= SplitDepth && PrN > 1 && parent && !exclusion) do_split = 1;

	while (move = get_move<me,0>()) {
		if (move == hash_move) continue;
		if (IsIllegal(me,move)) continue;
		cnt++;
		if (move == move_back) {
			score = Max(0, score);
			continue;
		}
		if (Current->stage == r_checks) check = 1;
		else check = is_check<me>(move);
		if (T(check) && T(see<me>(move, 0))) ext = 1 + (depth < 16);
		else ext = extension<0>(move, depth);
		new_depth = depth - 2 + ext;
		if (F(Square(To(move))) && F(move & 0xE000)) {
			if (move != Current->killer[1] && move != Current->killer[2]) {
				if (F(check) && cnt > moves_to_play) {
				    Current->gen_flags &= ~FlagSort;
				    continue;
		        }
				if (depth >= 6) {
					int reduction = msb(cnt);
					if (move == Current->ref[0] || move == Current->ref[1]) reduction = Max(0, reduction - 1);
					if (reduction >= 2 && !(Queen(White) | Queen(Black)) && popcnt(NonPawnKingAll) <= 4) reduction += reduction / 2;
					if (new_depth - reduction > 3)
						if (F(see<me>(move, -50))) reduction += 2;
					if (T(reduction) && reduction < 2 && new_depth - reduction > 3) {
						if (cnt > 3) reduction = 2;
						else reduction = 0;
					}
					new_depth = Max(3, new_depth - reduction);
				}
		    }
			if (F(check)) {
			    if ((value = Current->score + DeltaM(move) + 10) < beta && depth <= 3) {
				    score = Max(value, score);
				    continue;
			    }
				if (cnt > 7 && (value = margin + DeltaM(move) - 25 * msb(cnt)) < beta && depth <= 19) {
					score = Max(value, score);
					continue;
				}
			}
			if (depth <= 9 && T(NonPawnKing(me)) && F(see<me>(move,-50))) continue;
		} else {
			if (Current->stage == r_cap) {
				if (F(check) && depth <= 9 && F(see<me>(move,-50))) continue;
			} else if (Current->stage == s_bad_cap && F(check) && depth <= 5) continue;
		}
		if (do_split && played >= 1) {
			if (!sp_init) {
				sp_init = 1;
				uint64 u = ~Smpi->active_sp;
				if (!u) {
					do_split = 0;
					goto make_move;
				}
				Sp = &Smpi->Sp[lsb(u)];
				init_sp(Sp, beta - 1, beta, depth, 0, singular, height);
			}
			GMove * M = &Sp->move[Sp->move_number++];
			M->ext = ext;
			M->flags = 0;
			M->move = move;
			M->reduced_depth = new_depth;
			M->research_depth = depth - 2 + ext;
			M->stage = Current->stage;
			continue;
		}
make_move:
		do_move<me>(move);
		value = -search<opp, 0>(1 - beta, new_depth, FlagNeatSearch | ExtFlag(ext));
		if (value >= beta && new_depth < depth - 2 + ext) value = -search<opp, 0>(1 - beta, depth - 2 + ext, FlagNeatSearch | FlagDisableNull | ExtFlag(ext));
		undo_move<me>(move);
		played++;
		if (value > score) {
			score = value;
			if (value >= beta) goto cut;
		}
	}
	if (do_split && sp_init) {
		value = smp_search<me>(Sp);
		if (value >= beta && Sp->best_move) {
			score = beta;
			Current->best = move = Sp->best_move;
			for (i = 0; i < Sp->move_number; i++) {
				GMove * M = &Sp->move[i];
				if ((M->flags & FlagFinished) && M->stage == s_quiet && M->move != move) HistoryBad(M->move);
			}
		}
		if (value >= beta) goto cut;
	}
	if (F(cnt) && F(flag)) {
		hash_high(0, 127);
		hash_low(0, 0, 127);
		return 0;
	}
	if (F(exclusion)) hash_high(score, depth);
	return score;
cut:
	if (exclusion) return score;
	Current->best = move;
	if (depth >= 10) score = Min(beta, score);
	hash_low(move, score, depth);
	if (F(Square(To(move))) && F(move & 0xE000)) {
		if (Current->killer[1] != move && F(flags & FlagNoKillerUpdate)) {
			Current->killer[2] = Current->killer[1];
			Current->killer[1] = move;
		}
		HistoryGood(move);
		if (move != hash_move && Current->stage == s_quiet && !sp_init) for (p = Current->start; p < (Current->current - 1); p++) HistoryBad(*p);
		UpdateRef(move);
	}
	return score;
}

template <bool me, bool exclusion> int search_evasion(int beta, int depth, int flags) {
	int i, value, score, pext, move, cnt, hash_value = -MateValue, hash_depth, hash_move, new_depth, ext, check, moves_to_play;
	int height = (int)(Current - Data);

	if (depth <= 1) return q_evasion<me, 0>(beta - 1, beta, 1, flags);
	score = height - MateValue;
	if (flags & FlagHaltCheck) {
	    if (score >= beta) return beta;
	    if (MateValue - height < beta) return beta - 1;
	    halt_check;
	}

	hash_depth = -1;
	hash_move = flags & 0xFFFF;
	if (exclusion) {
		cnt = pext = 0;
		score = beta - 1;
		gen_evasions<me>(Current->moves);
	    if (F(Current->moves[0])) return score;
		goto skip_hash_move;
	}
	Current->best = hash_move;
	if (GEntry * Entry = probe_hash()) {
		if (Entry->high < beta && Entry->high_depth >= depth) return Entry->high;
		if (T(Entry->move) && Entry->low_depth > hash_depth) {
			Current->best = hash_move = Entry->move;
			hash_depth = Entry->low_depth;
		}
		if (Entry->low >= beta && Entry->low_depth >= depth) {
			if (Entry->move) {
				Current->best = Entry->move;
				if (F(Square(To(Entry->move))) && F(Entry->move & 0xE000)) UpdateCheckRef(Entry->move);
			}
			return Entry->low;
		}
		if (Entry->low_depth >= depth - 8 && Entry->low > hash_value) hash_value = Entry->low;
	}

	if (depth >= 20) if (GPVEntry * PVEntry  = probe_pv_hash()) {
		hash_low(PVEntry->move,PVEntry->value,PVEntry->depth);
		hash_high(PVEntry->value,PVEntry->depth);
		if (PVEntry->depth >= depth) {
			if (PVEntry->move) Current->best = PVEntry->move;
			return PVEntry->value;
		}
		if (T(PVEntry->move) && PVEntry->depth > hash_depth) {
			Current->best = hash_move = PVEntry->move;
			hash_depth = PVEntry->depth;
			hash_value = PVEntry->value;
		}
	}

	if (hash_depth >= depth && hash_value > -EvalValue) score = Min(beta - 1, Max(score, hash_value));
	if (flags & FlagCallEvaluation) evaluate();

	Current->mask = Filled;
	if (Current->score - 10 < beta && depth <= 3) {
		Current->mask = Piece(opp);
		score = Current->score - 10;
	    capture_margin<me>(beta, score);
	}
	cnt = 0;
	pext = 0;
    gen_evasions<me>(Current->moves);
	if (F(Current->moves[0])) return score;
	if (F(Current->moves[1])) pext = 2;

	if (T(hash_move) && is_legal<me>(move = hash_move)) {
		if (IsIllegal(me,move)) goto skip_hash_move;
		cnt++;
		check = is_check<me>(move);
		if (check) ext = Max(pext, 1 + (depth < 16));
		else ext = MaxF(pext, extension<0>(move, depth));
		if (depth >= 16 && hash_value >= beta && hash_depth >= (new_depth = depth - Min(12, depth/2))) {
			int margin_one = beta - ExclSingle(depth);
			int margin_two = beta - ExclDouble(depth);
			int prev_ext = Ext(flags);
			int singular = singular_extension<me>(ext,prev_ext,margin_one,margin_two,new_depth,hash_move);
			if (singular) ext = Max(ext, singular + (prev_ext < 1) - (singular >= 2 && prev_ext >= 2));
		}
		new_depth = depth - 2 + ext;
		do_move<me>(move);
		evaluate();
		if (Current->att[opp] & King(me)) {
			undo_move<me>(move);
			cnt--;
			goto skip_hash_move;
		}
		value = -search<opp, 0>(1 - beta, new_depth, FlagHaltCheck | FlagHashCheck | ((hash_value >= beta && hash_depth >= depth - 12) ? FlagDisableNull : 0) | ExtFlag(ext));
		undo_move<me>(move);
		if (value > score) {
			score = value;
			if (value >= beta) goto cut;
		}
	}
skip_hash_move:
	moves_to_play = 3 + ((depth * depth) / 6); 
	Current->ref[0] = RefM(Current->move).check_ref[0];
	Current->ref[1] = RefM(Current->move).check_ref[1];
	mark_evasions(Current->moves);
	Current->current = Current->moves;
	while (move = pick_move()) {
		if (move == hash_move) continue;
		if (IsIllegal(me,move)) continue;
		cnt++;
		if (IsRepetition(beta,move)) {
			score = Max(0, score);
			continue;
		}
		check = is_check<me>(move);
		if (check) ext = Max(pext, 1 + (depth < 16));
		else ext = MaxF(pext, extension<0>(move, depth));
		new_depth = depth - 2 + ext;
		if (F(Square(To(move))) && F(move & 0xE000)) {
			if (F(check)) {
				if (cnt > moves_to_play) continue;
				if ((value = Current->score + DeltaM(move) + 10) < beta && depth <= 3) {
					score = Max(value, score);
					continue;
				}
			}
			if (depth >= 6 && cnt > 3) {
				int reduction = msb(cnt);
				if (reduction >= 2 && !(Queen(White) | Queen(Black)) && popcnt(NonPawnKingAll) <= 4) reduction += reduction / 2;
				new_depth = Max(3, new_depth - reduction);
			}
		}
		do_move<me>(move);
		value = -search<opp, 0>(1 - beta, new_depth, FlagNeatSearch | ExtFlag(ext));
		if (value >= beta && new_depth < depth - 2 + ext) value = -search<opp, 0>(1 - beta, depth - 2 + ext, FlagNeatSearch | FlagDisableNull | ExtFlag(ext));
		undo_move<me>(move);
		if (value > score) {
			score = value;
			if (value >= beta) goto cut;
		}
	}
	if (F(exclusion)) hash_high(score, depth);
	return score;
cut:
	if (exclusion) return score;
	Current->best = move;
	hash_low(move, score, depth);
	if (F(Square(To(move))) && F(move & 0xE000)) UpdateCheckRef(move);
	return score;
}

template <bool me, bool root> int pv_search(int alpha, int beta, int depth, int flags) {
	int i, value, move, cnt, pext = 0, ext, check, hash_value = -MateValue, margin, do_split = 0, sp_init = 0, singular = 0, played = 0,
		new_depth, hash_move, hash_depth, old_alpha = alpha, old_best, ex_depth = 0, ex_value = 0, start_knodes = (nodes >> 10);
	GSP * Sp;
	int height = (int)(Current - Data);

	if (root) {
		depth = Max(depth, 2);
		flags |= ExtFlag(1);
		if (F(RootList[0])) return 0;
	    if (Print) {
			fprintf(stdout,"info depth %d\n",(depth/2)); 
			fflush(stdout);
		}
		int * p;
		for (p = RootList; *p; p++);
		sort_moves(RootList,p);
		for (p = RootList; *p; p++) *p &= 0xFFFF;
		SetScore(RootList[0],2);
		goto check_hash;
	}
	if (depth <= 1) return q_search<me, 1>(alpha, beta, 1, FlagNeatSearch);
	if (Convert((Current - Data),int) - MateValue >= beta) return beta;
	if (MateValue - Convert((Current - Data),int) <= alpha) return alpha;
	halt_check;

check_hash:
	hash_depth = -1;
	Current->best = hash_move = 0;
    if (GPVEntry * PVEntry = probe_pv_hash()) {
		hash_low(PVEntry->move,PVEntry->value,PVEntry->depth);
		hash_high(PVEntry->value,PVEntry->depth);
		if (PVEntry->depth >= depth && T(PVHashing)) {
			if (PVEntry->move) Current->best = PVEntry->move;
			if ((Current->ply <= 50 && PVEntry->ply <= 50) || (Current->ply >= 50 && PVEntry->ply >= 50)) if (!PVEntry->value || !draw_in_pv<me>()) return PVEntry->value;
		}
		if (T(PVEntry->move) && PVEntry->depth > hash_depth) {
			Current->best = hash_move = PVEntry->move;
			hash_depth = PVEntry->depth;
			hash_value = PVEntry->value;
		}
	}
	if (GEntry * Entry = probe_hash()) {
		if (T(Entry->move) && Entry->low_depth > hash_depth) {
			Current->best = hash_move = Entry->move;
			hash_depth = Entry->low_depth;
			if (Entry->low_depth) hash_value = Entry->low;
		}
	}

	if (root) {
		hash_move = RootList[0];
		hash_value = Previous;
		hash_depth = Max(0, depth - 2);
	}

	evaluate();

	if (F(root) && depth >= 6 && (F(hash_move) || hash_value <= alpha || hash_depth < depth - 8)) {
		if (F(hash_move)) new_depth = depth - 2;
		else new_depth = depth - 4;
		value = pv_search<me, 0>(alpha, beta, new_depth, hash_move);
		if (value > alpha) {
hash_move_found:
			if (Current->best) hash_move = Current->best;
		    hash_depth = new_depth;
		    hash_value = value;
			goto skip_iid;
		} else {
			i = 0;		
			new_depth = depth - 8;
iid_loop:
			margin = alpha - (8 << i);
			if (T(hash_move) && hash_depth >= Min(new_depth, depth - 8) && hash_value >= margin) goto skip_iid;
			value = search<me, 0>(margin, new_depth, FlagHashCheck | FlagNoKillerUpdate | FlagDisableNull | FlagReturnBestMove | hash_move);
			if (value >= margin) goto hash_move_found;
			i++;
			if (i < 5) goto iid_loop;
		}
	}
skip_iid:
	if (F(root) && Check(me)) {
		alpha = Max(Convert((Current - Data),int) - MateValue, alpha);
		Current->mask = Filled;
		gen_evasions<me>(Current->moves);
		if (F(Current->moves[0])) return Convert((Current - Data),int) - MateValue; 
	    if (F(Current->moves[1])) pext = 2;
	}

	cnt = 0;
	if (hash_move && is_legal<me>(move = hash_move)) {
		cnt++;
		if (root) {
#ifndef TUNER
		    memset(Data + 1, 0, 127 * sizeof(GData));
#endif
		    move_to_string(move,score_string);
		    if (Print) sprintf(info_string,"info currmove %s currmovenumber %d\n",score_string,cnt);
		}
		check = is_check<me>(move);
		if (check) ext = 2;
		else ext = MaxF(pext, extension<1>(move, depth));
		if (depth >= 12 && hash_value > alpha && hash_depth >= (new_depth = depth - Min(12,depth/2))) {
			int margin_one = hash_value - ExclSinglePV(depth);
			int margin_two = hash_value - ExclDoublePV(depth);
			int prev_ext = Ext(flags);
			singular = singular_extension<me>(root ? 0 : ext,root ? 0 : prev_ext,margin_one,margin_two,new_depth,hash_move);
			if (singular) {
				ext = Max(ext, singular + (prev_ext < 1) - (singular >= 2 && prev_ext >= 2));
				if (root) CurrentSI->Singular = singular;
				ex_depth = new_depth;
				ex_value = (singular >= 2 ? margin_two : margin_one) - 1;
			}
		}
		new_depth = depth - 2 + ext;
		do_move<me>(move);
		if (PrN > 1) {
			evaluate();
			if (Current->att[opp] & King(me)) {
				undo_move<me>(move);
				cnt--;
				goto skip_hash_move;
			}
		}
		value = -pv_search<opp, 0>(-beta, -alpha, new_depth, ExtFlag(ext));
		undo_move<me>(move);
		played++;
		if (value > alpha) {
			if (root) {
				CurrentSI->FailLow = 0;
			    best_move = move;
			    best_score = value;
				hash_low(best_move,value,depth);
				if (depth >= 14 || T(Console)) send_pv(depth/2, old_alpha, beta, value);
			}
		    alpha = value;
			Current->best = move;
			if (value >= beta) goto cut;
		} else if (root) {
			CurrentSI->FailLow = 1;
			CurrentSI->FailHigh = 0;
			CurrentSI->Singular = 0;
			if (depth >= 14 || T(Console)) send_pv(depth/2, old_alpha, beta, value);
		}
	}
skip_hash_move:
	Current->gen_flags = 0;
	if (F(Check(me))) {
		Current->stage = stage_search;
		Current->ref[0] = RefM(Current->move).ref[0];
	    Current->ref[1] = RefM(Current->move).ref[1];
	} else {
		Current->stage = stage_evasion;
		Current->ref[0] = RefM(Current->move).check_ref[0];
		Current->ref[1] = RefM(Current->move).check_ref[1];
	}
	Current->killer[0] = 0;
	Current->moves[0] = 0;
	if (root) Current->current = RootList + 1;
	else Current->current = Current->moves;

	if (PrN > 1 && !root && parent && depth >= SplitDepthPV) do_split = 1;

	while (move = get_move<me,root>()) {
		if (move == hash_move) continue;
		if (IsIllegal(me,move)) continue;
		cnt++;
		if (root) {
#ifndef TUNER
		    memset(Data + 1, 0, 127 * sizeof(GData));
#endif
		    move_to_string(move,score_string);
		    if (Print) sprintf(info_string,"info currmove %s currmovenumber %d\n",score_string,cnt);
		}
		if (IsRepetition(alpha + 1,move)) continue;
		check = is_check<me>(move);
		if (check) ext = 2;
		else ext = MaxF(pext, extension<1>(move, depth));
		new_depth = depth - 2 + ext;
		if (depth >= 6 && F(move & 0xE000) && F(Square(To(move))) && (T(root) || (move != Current->killer[1] && move != Current->killer[2]) || T(Check(me))) && cnt > 3) {
			int reduction = msb(cnt) - 1;
			if (move == Current->ref[0] || move == Current->ref[1]) reduction = Max(0, reduction - 1);
			if (reduction >= 2 && !(Queen(White) | Queen(Black)) && popcnt(NonPawnKingAll) <= 4) reduction += reduction / 2;
			new_depth = Max(3, new_depth - reduction);
		}
		if (do_split && played >= 1) {
			if (!sp_init) {
				sp_init = 1;
				uint64 u = ~Smpi->active_sp;
				if (!u) {
					do_split = 0;
					goto make_move;
				}
				Sp = &Smpi->Sp[lsb(u)];
				init_sp(Sp, alpha, beta, depth, 1, singular, height);
			}
			GMove * M = &Sp->move[Sp->move_number++];
			M->ext = ext;
			M->flags = 0;
			M->move = move;
			M->reduced_depth = new_depth;
			M->research_depth = depth - 2 + ext;
			M->stage = Current->stage;
			continue;
		}
make_move:
		do_move<me>(move);
		if (new_depth <= 1) value = -pv_search<opp, 0>(-beta, -alpha, new_depth, ExtFlag(ext));
		else value = -search<opp, 0>(-alpha, new_depth, FlagNeatSearch | ExtFlag(ext));
		if (value > alpha && new_depth > 1) {
			if (root) {
			    SetScore(RootList[cnt - 1],1);
			    CurrentSI->Early = 0;
			    old_best = best_move;
			    best_move = move;
			}
			new_depth = depth - 2 + ext;
			value = -pv_search<opp, 0>(-beta, -alpha, new_depth, ExtFlag(ext));
			if (T(root) && value <= alpha) best_move = old_best;
		}
		undo_move<me>(move);
		played++;
		if (value > alpha) {
			if (root) {
				SetScore(RootList[cnt - 1],cnt + 3);
				CurrentSI->Change = 1;
				CurrentSI->FailLow = 0;
			    best_move = move;
			    best_score = value;
				hash_low(best_move,value,depth);
				if (depth >= 14 || T(Console)) send_pv(depth/2, old_alpha, beta, value);
			}
		    alpha = value;
			Current->best = move;
			if (value >= beta) goto cut;
		}
	}
	if (do_split && sp_init) {
		value = smp_search<me>(Sp);
		if (value > alpha && Sp->best_move) {
			alpha = value;
			Current->best = move = Sp->best_move;
		}
		if (value >= beta) goto cut;
	}
	if (F(cnt) && F(Check(me))) {
		hash_high(0, 127);
		hash_low(0, 0, 127);
		hash_exact(0, 0, 127, 0, 0, 0);
	    return 0;
	}
	if (F(root) || F(SearchMoves)) hash_high(alpha, depth);
	if (alpha > old_alpha) {
		hash_low(Current->best,alpha,depth); 
		if (Current->best != hash_move) ex_depth = 0;
		if (F(root) || F(SearchMoves)) hash_exact(Current->best,alpha,depth,ex_value,ex_depth,Convert(nodes >> 10,int) - start_knodes); 
	}
	return alpha;
cut:
	hash_low(move, alpha, depth);
	return alpha;
}

template <bool me> void root() {
	int i, depth, value, alpha, beta, delta, start_depth = 2, hash_depth = 0, hash_value, store_time = 0, time_est, ex_depth = 0, ex_value, prev_time = 0, knodes = 0;
	sint64 time;
	GPVEntry * PVEntry;

	date++;
	nodes = check_node = check_node_smp = 0;
#ifndef TUNER
	if (parent) Smpi->nodes = 0;
#endif
	memcpy(Data,Current,sizeof(GData));
	Current = Data;
	evaluate();
	gen_root_moves<me>();
	if (PVN > 1) {
		memset(MultiPV,0,128 * sizeof(int));
		for (i = 0; MultiPV[i] = RootList[i]; i++);
	}
	best_move = RootList[0];
	if (F(best_move)) return;
	if (F(Infinite) && !RootList[1]) {
		Infinite = 1;
		value = pv_search<me, 1>(-MateValue, MateValue, 4, FlagNeatSearch);
		Infinite = 0;
		LastDepth = 128;
		send_pv(6, -MateValue, MateValue, value);
		send_best_move();
		Searching = 0;
		if (MaxPrN > 1) ZERO_BIT_64(Smpi->searching, 0);
		return;
	}

	memset(CurrentSI,0,sizeof(GSearchInfo));
	memset(BaseSI,0,sizeof(GSearchInfo));
	Previous = -MateValue;
	if (PVEntry = probe_pv_hash()) {
		if (is_legal<me>(PVEntry->move) && PVEntry->move == best_move && PVEntry->depth > hash_depth) {
			hash_depth = PVEntry->depth;
			hash_value = PVEntry->value;
			ex_depth = PVEntry->ex_depth;
			ex_value = PVEntry->exclusion;
			knodes = PVEntry->knodes;
		}
	}
	if (T(hash_depth) && PVN == 1) {
		Previous = best_score = hash_value;
		depth = hash_depth;
		if (PVHashing) {
	        send_pv(depth/2, -MateValue, MateValue, best_score);
		    start_depth = (depth + 2) & (~1);
		}
		if ((depth >= LastDepth - 8 || T(store_time)) && LastValue >= LastExactValue && hash_value >= LastExactValue && T(LastTime) && T(LastSpeed)) {
			time = TimeLimit1;
			if (ex_depth >= depth - Min(12, depth/2) && ex_value <= hash_value - ExclSinglePV(depth)) {
				BaseSI->Early = 1;
				BaseSI->Singular = 1;
				if (ex_value <= hash_value - ExclDoublePV(depth)) {
					time = (time * TimeSingTwoMargin)/100;
					BaseSI->Singular = 2;
				}
				else time = (time * TimeSingOneMargin)/100;
			}
			time_est = Min(LastTime,(knodes << 10)/LastSpeed);
			time_est = Max(time_est, store_time);
set_prev_time:
			LastTime = prev_time = time_est;
			if (prev_time >= time && F(Infinite)) {
				InstCnt++;
				if (time_est <= store_time) InstCnt = 0;
				if (InstCnt > 2) {
					if (T(store_time) && store_time < time_est) {
						time_est = store_time;
						goto set_prev_time;
					}
					LastSpeed = 0;
					LastTime = 0;
					prev_time = 0;
					goto set_jump;
				}
				if (hash_value > 0 && Current->ply >= 2 && F(Square(To(best_move))) && F(best_move & 0xF000) && PrevMove == ((To(best_move) << 6) | From(best_move))) goto set_jump;
				do_move<me>(best_move);
				if (Current->ply >= 100) {
					undo_move<me>(best_move);
					goto set_jump;
				}
				for (i = 4; i <= Current->ply; i+=2) if (Stack[sp-i] == Current->key) {
					undo_move<me>(best_move);
					goto set_jump;
				}
				undo_move<me>(best_move);
				LastDepth = depth;
				LastTime = prev_time;
				LastValue = LastExactValue = hash_value;
				send_best_move();
				Searching = 0;
				if (MaxPrN > 1) ZERO_BIT_64(Smpi->searching, 0);
				return;
			} else goto set_jump;
		}
	}
	LastTime = 0;
set_jump:
	memcpy(SaveBoard,Board,sizeof(GBoard));
	memcpy(SaveData,Data,sizeof(GData));
	save_sp = sp;
	if (setjmp(Jump)) {
		Current = Data;
		Searching = 0;
		if (MaxPrN > 1) {
			halt_all(0, 127);
			ZERO_BIT_64(Smpi->searching, 0);
		}
		memcpy(Board,SaveBoard,sizeof(GBoard));
		memcpy(Data,SaveData,sizeof(GData));
		sp = save_sp;
		send_best_move();
		return;
	}
	for (depth = start_depth; depth < DepthLimit; depth += 2) {
#ifndef TUNER
		memset(Data + 1, 0, 127 * sizeof(GData));
#endif
		CurrentSI->Early = 1;
		CurrentSI->Change = CurrentSI->FailHigh = CurrentSI->FailLow = CurrentSI->Singular = 0;
		if (PVN > 1) value = multipv<me>(depth);
		else if ((depth/2) < 7 || F(Aspiration)) LastValue = LastExactValue = value = pv_search<me, 1>(-MateValue, MateValue, depth, FlagNeatSearch);
		else {
			delta = 8;
			alpha = Previous - delta;
			beta = Previous + delta;
loop:
			if (delta >= 16 * 32) {
				LastValue = LastExactValue = value = pv_search<me, 1>(-MateValue, MateValue, depth, FlagNeatSearch);
				goto finish;
			}
			value = pv_search<me, 1>(alpha, beta, depth, FlagNeatSearch);
			if (value <= alpha) {
				CurrentSI->FailHigh = 0;
				CurrentSI->FailLow = 1;
				alpha -= delta;
				delta *= 2;
				LastValue = value;
				memcpy(BaseSI,CurrentSI,sizeof(GSearchInfo));
				goto loop;
			} else if (value >= beta) {
				CurrentSI->FailHigh = 1;
				CurrentSI->FailLow = 0;
				CurrentSI->Early = 1;
				CurrentSI->Change = 0;
				CurrentSI->Singular = Max(CurrentSI->Singular, 1);
				beta += delta;
				delta *= 2;
				LastDepth = depth;
				LastTime = MaxF(prev_time,get_time() - StartTime);
				LastSpeed = nodes/Max(LastTime, 1);
				if (depth + 2 < DepthLimit) depth += 2;
				InstCnt = 0;
#ifdef TIMING
				if (depth >= 6)
#endif
				check_time(LastTime,0);
#ifndef TUNER
				memset(Data + 1, 0, 127 * sizeof(GData));
#endif
				LastValue = value;
				memcpy(BaseSI,CurrentSI,sizeof(GSearchInfo));
				goto loop;
			} else LastValue = LastExactValue = value;
		}
finish:
		if (value < Previous - 50) CurrentSI->Bad = 1;
		else CurrentSI->Bad = 0;
		memcpy(BaseSI,CurrentSI,sizeof(GSearchInfo));
		LastDepth = depth;
		LastTime = MaxF(prev_time,get_time() - StartTime);
		LastSpeed = nodes/Max(LastTime, 1);
		Previous = value;
		InstCnt = 0;
#ifdef TIMING
		if (depth >= 6)
#endif
		check_time(LastTime,0);
	}
	Searching = 0;
	if (MaxPrN > 1) ZERO_BIT_64(Smpi->searching, 0);
	if (F(Infinite) || DepthLimit < 128) send_best_move();
}

template <bool me> int multipv(int depth) {
	int move, low = MateValue, value, i, cnt, ext, new_depth = depth;
	fprintf(stdout,"info depth %d\n",(depth/2)); fflush(stdout);
	for (cnt = 0; cnt < PVN && T(move = (MultiPV[cnt] & 0xFFFF)); cnt++) {
		MultiPV[cnt] = move;
		move_to_string(move,score_string);
		if (T(Print)) sprintf(info_string,"info currmove %s currmovenumber %d\n",score_string,cnt + 1);
		new_depth = depth - 2 + (ext = extension<1>(move, depth));
		do_move<me>(move);
		value = -pv_search<opp, 0>(-MateValue,MateValue,new_depth,ExtFlag(ext));
		MultiPV[cnt] |= value << 16;
		if (value < low) low = value;
		undo_move<me>(move);
		for (i = cnt - 1; i >= 0; i--) {
			if ((MultiPV[i] >> 16) < value) {
				MultiPV[i + 1] = MultiPV[i];
				MultiPV[i] = move | (value << 16);
			}
		}
		best_move = MultiPV[0] & 0xFFFF;
		Current->score = MultiPV[0] >> 16;
		send_multipv((depth/2), cnt);
	}
	for (;T(move = (MultiPV[cnt] & 0xFFFF)); cnt++) {
		MultiPV[cnt] = move;
		move_to_string(move,score_string);
		if (T(Print)) sprintf(info_string,"info currmove %s currmovenumber %d\n",score_string,cnt + 1);
		new_depth = depth - 2 + (ext = extension<1>(move, depth));
		do_move<me>(move);
		value = -search<opp, 0>(-low, new_depth, FlagNeatSearch | ExtFlag(ext));
		if (value > low) value = -pv_search<opp, 0>(-MateValue,-low,new_depth,ExtFlag(ext));
		MultiPV[cnt] |= value << 16;
		undo_move<me>(move);
		if (value > low) {
			for (i = cnt; i >= PVN; i--) MultiPV[i] = MultiPV[i - 1];
			MultiPV[PVN - 1] = move | (value << 16);
			for (i = PVN - 2; i >= 0; i--) {
				if ((MultiPV[i] >> 16) < value) {
					MultiPV[i + 1] = MultiPV[i];
					MultiPV[i] = move | (value << 16);
				}
			}
			best_move = MultiPV[0] & 0xFFFF;
		    Current->score = MultiPV[0] >> 16;
			low = MultiPV[PVN - 1] >> 16;
			send_multipv((depth/2), cnt);
		}
	}
	return Current->score;
}

void send_pv(int depth, int alpha, int beta, int score) {
	int i, pos, move, mate = 0, mate_score, sel_depth;
	sint64 nps, snodes;
	if (F(Print)) return;
	for (sel_depth = 1; sel_depth < 127 && T((Data + sel_depth)->att[0]); sel_depth++);
	sel_depth--;
	pv_length = 64;
	if (F(move = best_move)) move = RootList[0];
	if (F(move)) return;
	PV[0] = move;
	if (Current->turn) do_move<1>(move);
	else do_move<0>(move);
	pvp = 1;
	pick_pv();
	if (Current->turn ^ 1) undo_move<1>(move);
	else undo_move<0>(move);
	pos = 0;
	for (i = 0; i < 64 && T(PV[i]); i++) {
		if (pos > 0) { 
			pv_string[pos] = ' '; 
			pos++; 
		}
        move = PV[i];
        pv_string[pos++] = ((move >> 6) & 7) + 'a';
        pv_string[pos++] = ((move >> 9) & 7) + '1';
        pv_string[pos++] = (move & 7) + 'a';
        pv_string[pos++] = ((move >> 3) & 7) + '1';
        if (IsPromotion(move)) {
            if ((move & 0xF000) == FlagPQueen)  pv_string[pos++] = 'q';
            else if ((move & 0xF000) == FlagPRook)   pv_string[pos++] = 'r';
            else if ((move & 0xF000) == FlagPLight || (move & 0xF000) == FlagPDark) pv_string[pos++] = 'b';
            else if ((move & 0xF000) == FlagPKnight) pv_string[pos++] = 'n';
		}
        pv_string[pos] = 0;
	}
	score_string[0] = 'c';
	score_string[1] = 'p';
    if (score > EvalValue) {
		mate = 1;
		strcpy(score_string,"mate ");
		mate_score = (MateValue - score + 1)/2;
	    score_string[6] = 0;
	} else if (score < -EvalValue) {
		mate = 1;
        strcpy(score_string,"mate ");
		mate_score = -(score + MateValue + 1)/2;
		score_string[6] = 0;
	} else {
        score_string[0] = 'c';
	    score_string[1] = 'p';
		score_string[2] = ' ';
		score_string[3] = 0;
	}
	nps = get_time() - StartTime;
#ifdef MP_NPS
	snodes = Smpi->nodes;
#else
	snodes = nodes;
#endif
	if (nps) nps = (snodes * 1000)/nps; 
	if (score < beta) {
		if (score <= alpha) fprintf(stdout,"info depth %d seldepth %d score %s%d upperbound nodes %I64d nps %I64d pv %s\n",depth,sel_depth,score_string,(mate ? mate_score : score),snodes,nps,pv_string);
		else fprintf(stdout,"info depth %d seldepth %d score %s%d nodes %I64d nps %I64d pv %s\n",depth,sel_depth,score_string,(mate ? mate_score : score),snodes,nps,pv_string);
	} else fprintf(stdout,"info depth %d seldepth %d score %s%d lowerbound nodes %I64d nps %I64d pv %s\n",depth,sel_depth,score_string,(mate ? mate_score : score),snodes,nps,pv_string);
	fflush(stdout);
}

void send_multipv(int depth, int curr_number) {
	int i, j, pos, move, score;
	sint64 nps, snodes;
	if (F(Print)) return;
	for (j = 0; j < PVN && T(MultiPV[j]); j++) {
		pv_length = 63;
		pvp = 0;
		move = MultiPV[j] & 0xFFFF;
		score = MultiPV[j] >> 16;
		memset(PV,0,64 * sizeof(uint16));
		if (Current->turn) do_move<1>(move);
	    else do_move<0>(move);
		pick_pv();
		if (Current->turn ^ 1) undo_move<1>(move);
	    else undo_move<0>(move);
		for (i = 63; i > 0; i--) PV[i] = PV[i - 1];
		PV[0] = move;
		pos = 0;
		for (i = 0; i < 64 && T(PV[i]); i++) {
			if (pos > 0) { 
				pv_string[pos] = ' '; 
				pos++; 
			}
        	move = PV[i];
        	pv_string[pos++] = ((move >> 6) & 7) + 'a';
        	pv_string[pos++] = ((move >> 9) & 7) + '1';
        	pv_string[pos++] = (move & 7) + 'a';
        	pv_string[pos++] = ((move >> 3) & 7) + '1';
        	if (IsPromotion(move)) {
            	if ((move & 0xF000) == FlagPQueen)  pv_string[pos++] = 'q';
            	else if ((move & 0xF000) == FlagPRook)   pv_string[pos++] = 'r';
            	else if ((move & 0xF000) == FlagPLight || (move & 0xF000) == FlagPDark) pv_string[pos++] = 'b';
            	else if ((move & 0xF000) == FlagPKnight) pv_string[pos++] = 'n';
			}
        	pv_string[pos] = 0;
		}
		score_string[0] = 'c';
		score_string[1] = 'p';
		if (score > EvalValue) {
			strcpy(score_string,"mate ");
			score = (MateValue - score + 1)/2;
	    	score_string[6] = 0;
		} else if (score < -EvalValue) {
        	strcpy(score_string,"mate ");
			score = -(score + MateValue + 1)/2;
			score_string[6] = 0;
		} else {
        	score_string[0] = 'c';
	    	score_string[1] = 'p';
			score_string[2] = ' ';
			score_string[3] = 0;
		}
		nps = get_time() - StartTime;
#ifdef MP_NPS
		snodes = Smpi->nodes;
#else
		snodes = nodes;
#endif
	    if (nps) nps = (snodes * 1000)/nps; 
		fprintf(stdout,"info multipv %d depth %d score %s%d nodes %I64d nps %I64d pv %s\n",j + 1,(j <= curr_number ? depth : depth - 1),score_string,score,snodes,nps,pv_string);
		fflush(stdout);
	}
}

void send_best_move() {
	uint64 snodes;
	int ponder;
#ifdef CPU_TIMING
	GlobalTime[GlobalTurn] -= Convert(get_time() - StartTime, int) - GlobalInc[GlobalTurn];
	if (GlobalTime[GlobalTurn] < GlobalInc[GlobalTurn]) GlobalTime[GlobalTurn] = GlobalInc[GlobalTurn];
#endif
	if (F(Print)) return;
#ifdef MP_NPS
	snodes = Smpi->nodes;
#else
	snodes = nodes;
#endif
	fprintf(stdout,"info nodes %I64d score cp %d\n",snodes,best_score);
	if (!best_move) return;
	Current = Data;
	evaluate();
	if (Current->turn) do_move<1>(best_move);
	else do_move<0>(best_move);
	pv_length = 1;
	pvp = 0;
	pick_pv();
	ponder = PV[0];
	if (Current->turn ^ 1) undo_move<1>(best_move);
	else undo_move<0>(best_move);
	move_to_string(best_move,pv_string);
	if (ponder) {
		move_to_string(ponder,score_string);
		fprintf(stdout,"bestmove %s ponder %s\n",pv_string,score_string);
	} else fprintf(stdout,"bestmove %s\n",pv_string);
	fflush(stdout);
}

void get_position(char string[]) {
	const char * fen;
    char * moves;
    const char * ptr;
    int move, move1 = 0;

    fen = strstr(string,"fen ");
    moves = strstr(string,"moves ");
    if (fen != NULL) get_board(fen+4);
    else get_board("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
	PrevMove = 0;
    if (moves != NULL) {
        ptr = moves+6;
        while (*ptr != 0) {
            pv_string[0] = *ptr++;
            pv_string[1] = *ptr++;
            pv_string[2] = *ptr++;
            pv_string[3] = *ptr++;
            if (*ptr == 0 || *ptr == ' ') pv_string[4] = 0;
            else { 
				pv_string[4] = *ptr++; 
				pv_string[5] = 0; 
			}
			evaluate();
            move = move_from_string(pv_string);
			PrevMove = move1;
			move1 = move;
            if (Current->turn) do_move<1>(move);
	        else do_move<0>(move);
			memcpy(Data,Current,sizeof(GData));
			Current = Data;
            while (*ptr == ' ') ptr++;
        }
    }
	memcpy(Stack, Stack + sp - Current->ply, (Current->ply + 1) * sizeof(uint64));
	sp = Current->ply;
}

void get_time_limit(char string[]) {
	const char * ptr;
	int i, time, inc, wtime, btime, winc, binc, moves, pondering, movetime = 0, exp_moves = MovesTg - 1;
  
	Infinite = 1;
	MoveTime = 0;
	SearchMoves = 0;
	SMPointer = 0;
	pondering = 0;
	TimeLimit1 = 0;
	TimeLimit2 = 0;
	wtime = btime = 0;
	winc = binc = 0;
	moves = 0;
	Stop = 0;
	DepthLimit = 128;
    ptr = strtok(string," ");
    for (ptr = strtok(NULL," "); ptr != NULL; ptr = strtok(NULL," ")) {
		if (!strcmp(ptr,"binc")) {
			ptr = strtok(NULL," "); 
			binc = atoi(ptr);
			Infinite = 0;
		} else if (!strcmp(ptr,"btime")) { 
			ptr = strtok(NULL," "); 
			btime = atoi(ptr);
			Infinite = 0;
		} else if (!strcmp(ptr,"depth")) { 
			ptr = strtok(NULL," "); 
			DepthLimit = 2 * atoi(ptr) + 2; 
			Infinite = 1;
		} else if (!strcmp(ptr,"infinite")) { 
			Infinite = 1; 
		} else if (!strcmp(ptr,"movestogo")) { 
			ptr = strtok(NULL," "); 
			moves = atoi(ptr);
			Infinite = 0;
		} else if (!strcmp(ptr,"winc")) { 
			ptr = strtok(NULL," "); 
			winc = atoi(ptr);
			Infinite = 0;
		} else if (!strcmp(ptr,"wtime")) { 
			ptr = strtok(NULL," "); 
			wtime = atoi(ptr); 
			Infinite = 0;
		} else if (!strcmp(ptr,"movetime")) { 
			ptr = strtok(NULL," "); 
			movetime = atoi(ptr);
			MoveTime = 1;
			Infinite = 0;
		} else if (!strcmp(ptr,"searchmoves")) {
			if (F(SearchMoves)) {
				for (i = 0; i < 256; i++) SMoves[i] = 0;
			}
		    SearchMoves = 1;
		    ptr += 12;
			while (ptr != NULL && ptr[0] >= 'a' && ptr[0] <= 'h' && ptr[1] >= '1' && ptr[1] <= '8') {
				pv_string[0] = *ptr++;
                pv_string[1] = *ptr++;
                pv_string[2] = *ptr++;
                pv_string[3] = *ptr++;
                if (*ptr == 0 || *ptr == ' ') pv_string[4] = 0;
                else { 
				    pv_string[4] = *ptr++; 
				    pv_string[5] = 0; 
			    }
				SMoves[SMPointer] = move_from_string(pv_string);
				SMPointer++;
				ptr = strtok(NULL," ");
			}
		} else if (!strcmp(ptr,"ponder")) pondering = 1;
    }
	if (pondering) Infinite = 1;
	if (Current->turn == White) {
		time = wtime;
		inc = winc;
	} else {
		time = btime;
		inc = binc;
	}
#ifdef CPU_TIMING
	if (CpuTiming) {
		time = GlobalTime[GlobalTurn];
		inc = GlobalInc[GlobalTurn];
		if (UciMaxDepth) DepthLimit = 2 * UciMaxDepth + 2;
	}
#endif
	if (moves) moves = Max(moves - 1, 1);
	int time_max = Max(time - Min(1000, time/2), 0);
	int nmoves;
	if (moves) nmoves = moves;
	else {
		nmoves = MovesTg - 1;
		if (Current->ply > 40) nmoves += Min(Current->ply - 40, (100 - Current->ply)/2);
		exp_moves = nmoves;
	}
	TimeLimit1 = Min(time_max, (time_max + (Min(exp_moves, nmoves) * inc))/Min(exp_moves, nmoves));
	TimeLimit2 = Min(time_max, (time_max + (Min(exp_moves, nmoves) * inc))/Min(3,Min(exp_moves, nmoves)));
	TimeLimit1 = Min(time_max, (TimeLimit1 * TimeRatio)/100);
	if (Ponder) TimeLimit1 = (TimeLimit1 * PonderRatio)/100;
	if (MoveTime) {
		TimeLimit2 = movetime;
		TimeLimit1 = TimeLimit2 * 100;
	}
    InfoTime = StartTime = get_time();
	Searching = 1;
	if (MaxPrN > 1) SET_BIT_64(Smpi->searching, 0);
	if (F(Infinite)) PVN = 1;
	if (Current->turn == White) root<0>(); else root<1>();
}

sint64 get_time() {
#ifdef CPU_TIMING
#ifndef TIMING
	if (CpuTiming) {
#endif
		uint64 ctime;
		QueryProcessCycleTime(GetCurrentProcess(), &ctime);
#ifdef TIMING
		return ctime / (CyclesPerMSec / 1000);
#endif
		return (ctime / CyclesPerMSec);
#ifndef TIMING
	}
#endif
#endif
	return GetTickCount();
}

int time_to_stop(GSearchInfo * SI, int time, int searching) {
	if (Infinite) return 0;
	if (time > TimeLimit2) return 1;
	if (searching) return 0;
	if (2 * time > TimeLimit2 && F(MoveTime)) return 1;
	if (SI->Bad) return 0;
	if (time > TimeLimit1) return 1;
	if (T(SI->Change) || T(SI->FailLow)) return 0;
	if (time * 100 > TimeLimit1 * TimeNoChangeMargin) return 1;
	if (F(SI->Early)) return 0;
	if (time * 100 > TimeLimit1 * TimeNoPVSCOMargin) return 1;
	if (SI->Singular < 1) return 0;
	if (time * 100 > TimeLimit1 * TimeSingOneMargin) return 1;
	if (SI->Singular < 2) return 0;
	if (time * 100 > TimeLimit1 * TimeSingTwoMargin) return 1;
	return 0;
}

void check_time(int searching) {
#ifdef CPU_TIMING
	if (CpuTiming && UciMaxKNodes && nodes > UciMaxKNodes * 1024) Stop = 1;
#endif
#ifdef TUNER
#ifndef TIMING
	return;
#endif
#else
	while (!Stop && input()) uci();
#endif
	if (Stop) goto jump;
	CurrTime = get_time();
	int Time = Convert(CurrTime - StartTime,int);
	if (T(Print) && Time > InfoLag && CurrTime - InfoTime > InfoDelay) {
		InfoTime = CurrTime;
		if (info_string[0]) {
			fprintf(stdout,"%s",info_string);
			info_string[0] = 0;
			fflush(stdout);
		}
	}
	if (time_to_stop(CurrentSI, Time, searching)) goto jump;
	return;
jump:
	Stop = 1;
	longjmp(Jump,1);
}

void check_time(int time, int searching) {
#ifdef TUNER
#ifndef TIMING
	return;
#endif
#else
	while (!Stop && input()) uci();
#endif
	if (Stop) goto jump;
	CurrTime = get_time();
	int Time = Convert(CurrTime - StartTime,int);
	if (T(Print) && Time > InfoLag && CurrTime - InfoTime > InfoDelay) {
		InfoTime = CurrTime;
		if (info_string[0]) {
			fprintf(stdout,"%s",info_string);
			info_string[0] = 0;
			fflush(stdout);
		}
	}
	if (time_to_stop(CurrentSI, time, searching)) goto jump;
	return;
jump:
	Stop = 1;
	longjmp(Jump,1);
}

void check_state() {
	GSP *Sp, *Spc;
	int n, nc, score, best, pv, alpha, beta, new_depth, r_depth, ext, move, value;
	GMove * M;

	if (parent) {
		for (uint64 u = TEST_RESET(Smpi->fail_high); u; Cut(u)) {
			Sp = &Smpi->Sp[lsb(u)];
			LOCK(Sp->lock);
			if (Sp->active && Sp->finished) {
				UNLOCK(Sp->lock);
				longjmp(Sp->jump, 1);
			}
			UNLOCK(Sp->lock);
		}
		return;
	}

start:
	if (TEST_RESET_BIT(Smpi->stop, Id)) longjmp(CheckJump, 1);
	if (Smpi->searching & Bit(Id)) return;
	if (!(Smpi->searching & 1)) {
		Sleep(1);
		return;
	}
	while ((Smpi->searching & 1) && !Smpi->active_sp) _mm_pause();
	while ((Smpi->searching & 1) && !(Smpi->searching & Bit(Id - 1))) _mm_pause();

	Sp = NULL; best = -0x7FFFFFFF;
	for (uint64 u = Smpi->active_sp; u; Cut(u)) {
		Spc = &Smpi->Sp[lsb(u)];
		if (!Spc->active || Spc->finished || Spc->lock) continue;
		for (nc = Spc->current + 1; nc < Spc->move_number; nc++) if (!(Spc->move[nc].flags & FlagClaimed)) break;
		if (nc < Spc->move_number) score = 1024 * 1024 + 512 * 1024 * (Spc->depth >= 20) + 128 * 1024 * (!(Spc->split))
			+ ((Spc->depth + 2 * Spc->singular) * 1024) - (((16 * 1024) * (nc - Spc->current)) / nc);
		else continue;
		if (score > best) {
			best = score;
			Sp = Spc;
			n = nc;
		}
	}

	if (Sp == NULL) goto start;
	if (!Sp->active || Sp->finished || (Sp->move[n].flags & FlagClaimed) || n <= Sp->current || n >= Sp->move_number) goto start;
	if (Sp->lock) goto start;

	LOCK(Sp->lock);
	if (!Sp->active || Sp->finished || (Sp->move[n].flags & FlagClaimed) || n <= Sp->current || n >= Sp->move_number) {
		UNLOCK(Sp->lock);
		goto start;
	}

	M = &Sp->move[n];
	M->flags |= FlagClaimed;
	M->id = Id;
	Sp->split |= Bit(Id);
	pv = Sp->pv;
	alpha = Sp->alpha;
	beta = Sp->beta;
	new_depth = M->reduced_depth;
	r_depth = M->research_depth;
	ext = M->ext;
	move = M->move;

	Current = Data;
	retrieve_position(Sp->Pos, 1);
	evaluate();
	SET_BIT_64(Smpi->searching, Id);
	UNLOCK(Sp->lock);

	if (setjmp(CheckJump)) {
		ZERO_BIT_64(Smpi->searching, Id);
		return;
	}
	if (Current->turn == White) {
		do_move<0>(move);
		if (pv) {
			value = -search<1, 0>(-alpha, new_depth, FlagNeatSearch | ExtFlag(ext));
			if (value > alpha) value = -pv_search<1, 0>(-beta, -alpha, r_depth, ExtFlag(ext));
		} else {
			value = -search<1, 0>(1 - beta, new_depth, FlagNeatSearch | ExtFlag(ext));
			if (value >= beta && new_depth < r_depth) value = -search<1, 0>(1 - beta, r_depth, FlagNeatSearch | FlagDisableNull | ExtFlag(ext));
		}
		undo_move<0>(move);
	} else {
		do_move<1>(move);
		if (pv) {
			value = -search<0, 0>(-alpha, new_depth, FlagNeatSearch | ExtFlag(ext));
			if (value > alpha) value = -pv_search<0, 0>(-beta, -alpha, r_depth, ExtFlag(ext));
		} else {
			value = -search<0, 0>(1 - beta, new_depth, FlagNeatSearch | ExtFlag(ext));
			if (value >= beta && new_depth < r_depth) value = -search<0, 0>(1 - beta, r_depth, FlagNeatSearch | FlagDisableNull | ExtFlag(ext));
		}
		undo_move<1>(move);
	}

	LOCK(Sp->lock);
	ZERO_BIT_64(Smpi->searching, Id);
	if (TEST_RESET_BIT(Smpi->stop, Id)) {
		UNLOCK(Sp->lock);
		return;
	}
	M->flags |= FlagFinished;
	if (value > Sp->alpha) {
		Sp->alpha = Min(value, beta);
		Sp->best_move = move;
		if (value >= beta) {
			Sp->finished = 1;
			SET_BIT_64(Smpi->fail_high, (int)(Sp - Smpi->Sp));
		}
	}
	UNLOCK(Sp->lock);
}

int input() {
	if (child) return 0;
    DWORD p;
	if (F(Input)) return 0;
	if (F(Console)) {
	    if (PeekNamedPipe(StreamHandle,NULL,0,NULL,&p,NULL)) return (p > 0);
        else return 1;
	} else return 0;
}

void epd_test(char string[], int time_limit) {
	int n = 0, positions = 4000;
	uint64 Time, all_nodes = 0, new_time, total_time;
	double prod = 0.0;
	char * ptr;
	FILE * f = fopen(string, "r");
	if (f == NULL) {
		fprintf(stdout, "File not found\n");
		return;
	}
	Infinite = 1;
	Time = get_time();
	int total_depth = 0;
	Print = 0;
	Input = 0;
	total_time = 1;
	while (!feof(f) && n < positions) {
	new_position:
		(void)fgets(mstring, 65536, f);
		ptr = strchr(mstring, '\n');
		if (ptr != NULL) *ptr = 0;
		get_board(mstring);
		evaluate();
		if (Current->turn == White) {
			gen_root_moves<0>();
		} else {
			gen_root_moves<1>();
		}
		Infinite = 0;
		MoveTime = TimeLimit1 = 100000000;
#ifndef TIME_TO_DEPTH
		TimeLimit2 = time_limit;
#else
		TimeLimit2 = TimeLimit1;
#endif
		DepthLimit = 127;
		n++;
		Stop = 0;
		Smpi->nodes = nodes = check_node = check_node_smp = 0;
		StartTime = get_time();
		if (setjmp(Jump)) {
			halt_all(0, 127);
		stop_searching:
			ZERO_BIT_64(Smpi->searching, Id);
			Searching = 0;
			Current = Data;
			new_time = Max(get_time() - StartTime, 1);
			total_time += new_time;
#ifdef MP_NPS
			all_nodes += Smpi->nodes;
#else
			all_nodes += nodes;
#endif
			total_depth += LastDepth / 2;
#ifndef TIME_TO_DEPTH
			fprintf(stdout, "Position %d: %d [%lf, %d]\n", n, LastDepth / 2, ((double)total_depth) / ((double)n), (all_nodes * Convert(1000, uint64)) / total_time);
#else
			prod += log((double)new_time);
			fprintf(stdout, "Position %d: %d [%.0lf, %d]\n", n, new_time, exp(prod / (double)n), (all_nodes * Convert(1000, uint64)) / total_time);
#endif
			goto new_position;
		}
		for (int d = 4; d < 128; d += 2) {
			LastDepth = d;
			Searching = 1;
			SET_BIT_64(Smpi->searching, Id);
			if (Current->turn == White) {
				pv_search<0, 1>(-MateValue, MateValue, d, FlagNeatSearch);
			} else {
				pv_search<1, 1>(-MateValue, MateValue, d, FlagNeatSearch);
			}
#ifdef TIME_TO_DEPTH
			if (d >= (time_limit * 2)) goto stop_searching;
#endif
		}
	}
	if (n == 0) {
		fprintf(stdout, "Empty file\n");
		return;
	}
	fclose(f);
}

void uci() {
    char *ptr = NULL;
	int i;
	sint64 value;

    (void)fgets(mstring, 65536, stdin);
    if (feof(stdin)) exit(0);
    ptr = strchr(mstring, '\n');
    if (ptr != NULL) *ptr = 0;
    if (!strcmp(mstring, "uci")) {
#ifndef W32_BUILD
		fprintf(stdout,"id name Gull 3 x64\n");
#else
		fprintf(stdout,"id name Gull 3\n");
#endif
        fprintf(stdout,"id author ThinkingALot\n");
#ifndef W32_BUILD
		fprintf(stdout,"option name Hash type spin min 1 max 65536 default 16\n");
#else
		fprintf(stdout,"option name Hash type spin min 1 max 1024 default 16\n");
#endif
		fprintf(stdout,"option name Ponder type check default false\n");
		fprintf(stdout,"option name MultiPV type spin min 1 max 64 default 1\n");
		fprintf(stdout,"option name Clear Hash type button\n");
		fprintf(stdout,"option name PV Hash type check default true\n");
		fprintf(stdout,"option name Aspiration window type check default true\n");
#ifdef CPU_TIMING
		fprintf(stdout, "option name CPUTiming type check default false\n");
		fprintf(stdout, "option name MaxDepth type spin min 0 max 128 default 0\n");
		fprintf(stdout, "option name MaxKNodes type spin min 0 max 65536 default 0\n");
		fprintf(stdout, "option name BaseTime type spin min 0 max 1000000 default 1000\n");
		fprintf(stdout, "option name IncTime type spin min 0 max 1000000 default 5\n");
#endif
		fprintf(stdout, "option name Threads type spin min 1 max %d default %d\n", Min(CPUs, MaxPrN), PrN);
#ifdef LARGE_PAGES
		fprintf(stdout, "option name Large memory pages type check default true\n");
#endif
        fprintf(stdout,"uciok\n");
		if (F(Searching)) init_search(1);
    } else if (!strcmp(mstring,"ucinewgame")) {
        Stop = 0;
		init_search(1);
    } else if (!strcmp(mstring,"isready")) {
        fprintf(stdout,"readyok\n");
		fflush(stdout);
    }  else if (!memcmp(mstring,"position",8)) {
        if (F(Searching)) get_position(mstring);
    } else if (!memcmp(mstring,"go",2)) {
        if (F(Searching)) get_time_limit(mstring);
    } else if (!memcmp(mstring,"setoption",9)) {
		ptr = strtok(mstring," ");
		for (ptr = strtok(NULL," "); ptr != NULL; ptr = strtok(NULL," ")) {
			if (!memcmp(ptr,"Hash",4) && !Searching) {
				ptr += 11;
				value = atoi(ptr);
				if (value < 1) value = 1;
#ifdef W32_BUILD
				if (value > 1024) value = 1024;
#else
				if (value > 65536) value = 65536;
#endif
				value = (Bit(msb(value)) * Convert(1024 * 1024, sint64)) / Convert(sizeof(GEntry), sint64);
				if (value != hash_size) {
					ResetHash = 1;
					hash_size = value;
					longjmp(ResetJump, 1);
				}
			} else if (!memcmp(ptr, "Threads", 7) && !Searching) {
				ptr += 14;
				value = atoi(ptr);
				if (value != PrN) {
					NewPrN = Max(1, Min(MaxPrN, value));
					ResetHash = 0;
					longjmp(ResetJump, 1);
				}
			} else if (!memcmp(ptr, "MultiPV", 7)) {
				ptr += 14;
			    PVN = atoi(ptr);
				Stop = 1;
			} else if (!memcmp(ptr,"Ponder",6)) {
				ptr += 13;
				if (ptr[0] == 't') Ponder = 1;
				else Ponder = 0;
			} else if (!memcmp(ptr,"Clear",5)) {
				init_search(1);
				break;
			} else if (!memcmp(ptr,"PV",2)) {
				ptr += 14;
				if (ptr[0] == 't') PVHashing = 1;
				else PVHashing = 0;
			} else if (!memcmp(ptr, "Large", 5) && !Searching) {
				ptr += 25;
				if (ptr[0] == 't') {
					if (LargePages) return;
					LargePages = 1;
				} else {
					if (!LargePages) return;
					LargePages = 0;
				}
				ResetHash = 1;
				longjmp(ResetJump, 1);
			} else if (!memcmp(ptr, "Aspiration", 10)) {
				ptr += 24;
				if (ptr[0] == 't') Aspiration = 1;
				else Aspiration = 0;
			}
#ifdef CPU_TIMING
			else if (!memcmp(ptr, "CPUTiming", 9)) {
				ptr += 16;
				if (ptr[0] == 't') CpuTiming = 1;
				else CpuTiming = 0;
			} else if (!memcmp(ptr, "MaxDepth", 8)) UciMaxDepth = atoi(ptr + 15);
			else if (!memcmp(ptr, "MaxKNodes", 9)) UciMaxKNodes = atoi(ptr + 16);
			else if (!memcmp(ptr, "BaseTime", 8)) UciBaseTime = atoi(ptr + 15);
			else if (!memcmp(ptr, "IncTime", 7)) UciIncTime = atoi(ptr + 14);
#endif
        }
	} else if (!strcmp(mstring,"stop")) {
		Stop = 1;
		if (F(Searching)) send_best_move();
	} else if (!strcmp(mstring,"ponderhit")) {
		Infinite = 0;
		if (!RootList[1]) Stop = 1;
		if (F(CurrentSI->Bad) && F(CurrentSI->FailLow) && time_to_stop(BaseSI, LastTime, 0)) Stop = 1;
		if (F(Searching)) send_best_move();
	} else if (!strcmp(mstring, "quit")) {
		for (i = 1; i < PrN; i++) {
			TerminateProcess(ChildPr[i], 0);
			CloseHandle(ChildPr[i]);
		}
		exit(0);
	} else if (!memcmp(mstring, "epd", 3)) {
		ptr = mstring + 4;
		value = atoi(ptr);
		epd_test("op.epd", value);
	}
}

HANDLE CreateChildProcess(int child_id) {
	char name[1024];
	TCHAR szCmdline[1024];
	PROCESS_INFORMATION piProcInfo;
	STARTUPINFO siStartInfo;
	BOOL bSuccess = FALSE;

	ZeroMemory(&piProcInfo, sizeof(PROCESS_INFORMATION));
	ZeroMemory(&siStartInfo, sizeof(STARTUPINFO));
	ZeroMemory(szCmdline, 1024 * sizeof(TCHAR));
	ZeroMemory(name, 1024);

	siStartInfo.cb = sizeof(STARTUPINFO);
	siStartInfo.dwFlags |= STARTF_USESTDHANDLES;

	GetModuleFileName(NULL, name, 1024);
	sprintf(szCmdline, " child %d %d", WinParId, child_id);

	bSuccess = CreateProcess(TEXT(name), TEXT(szCmdline), NULL, NULL, FALSE, CREATE_NO_WINDOW, NULL, NULL, &siStartInfo, &piProcInfo);

	if (bSuccess) {
		CloseHandle(piProcInfo.hThread);
		return piProcInfo.hProcess;
	} else {
		fprintf(stdout, "Error %d\n", GetLastError());
		return NULL;
	}
}

void main(int argc, char *argv[]) {
	DWORD p;
	int i, HT = 0;
	SYSTEM_INFO sysinfo;

	if (argc >= 2) if (!memcmp(argv[1], "child", 5)) {
		child = 1; parent = 0;
		WinParId = atoi(argv[2]);
		Id = atoi(argv[3]);
	}

	int CPUInfo[4] = { -1 };
	__cpuid(CPUInfo, 1);
	HardwarePopCnt = (CPUInfo[2] >> 23) & 1;

	if (parent) {
		if (((CPUInfo[3] >> 28) & 1) && GetProcAddress(GetModuleHandle(TEXT("kernel32")), "GetLogicalProcessorInformation") != NULL) {
			SYSTEM_LOGICAL_PROCESSOR_INFORMATION syslogprocinfo[1];
			p = sizeof(SYSTEM_LOGICAL_PROCESSOR_INFORMATION);
#ifndef W32_BUILD
			GetLogicalProcessorInformation(syslogprocinfo, &p);
			if (syslogprocinfo->ProcessorCore.Flags == 1) HT = 1;
#endif
		}
		WinParId = GetProcessId(GetCurrentProcess());
		HANDLE JOB = CreateJobObject(NULL, NULL);
		JOBOBJECT_EXTENDED_LIMIT_INFORMATION jeli = { 0 };
		jeli.BasicLimitInformation.LimitFlags = JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE;
		SetInformationJobObject(JOB, JobObjectExtendedLimitInformation, &jeli, sizeof(jeli));
		AssignProcessToJobObject(JOB, GetCurrentProcess());
		if (MaxPrN > 1) {
			GetSystemInfo(&sysinfo);
			CPUs = sysinfo.dwNumberOfProcessors;
			PrN = Min(CPUs, MaxPrN);
			if (HT) PrN = Max(1, Min(PrN, CPUs / 2));
		}
	}

#ifdef CPU_TIMING
	SetPriorityClass(GetCurrentProcess(), IDLE_PRIORITY_CLASS);
#endif

	init();

	StreamHandle = GetStdHandle(STD_INPUT_HANDLE);
	Console = GetConsoleMode(StreamHandle, &p);
	if (Console) {
		SetConsoleMode(StreamHandle, p & (~(ENABLE_MOUSE_INPUT | ENABLE_WINDOW_INPUT)));
		FlushConsoleInputBuffer(StreamHandle);
	}

	setbuf(stdout, NULL);
	setbuf(stdin, NULL);
	setvbuf(stdout, NULL, _IONBF, 0);
	setvbuf(stdin, NULL, _IONBF, 0);
	fflush(NULL);

#ifndef W32_BUILD
	fprintf(stdout, "Gull 3 x64\n");
#else
	fprintf(stdout, "Gull 3\n");
#endif

reset_jump:
#ifndef TUNER
	if (parent) {
		if (setjmp(ResetJump)) {
			for (i = 1; i < PrN; i++) TerminateProcess(ChildPr[i], 0);
			for (i = 1; i < PrN; i++) {
				WaitForSingleObject(ChildPr[i], INFINITE);
				CloseHandle(ChildPr[i]);
			}
			Smpi->searching = Smpi->active_sp = Smpi->stop = 0;
			for (i = 0; i < MaxSplitPoints; i++) Smpi->Sp->active = Smpi->Sp->claimed = 0;
				
			Smpi->hash_size = hash_size;
			if (NewPrN) Smpi->PrN = PrN = NewPrN;
			goto reset_jump;
		}
		Smpi->hash_size = hash_size;
		Smpi->PrN = PrN;
	} else {
		hash_size = Smpi->hash_size;
		PrN = Smpi->PrN;
	}
#endif
	if (ResetHash) init_hash();
	init_search(0);

	if (child) while (true) check_state();
	if (parent) for (i = 1; i < PrN; i++) ChildPr[i] = CreateChildProcess(i);

#ifdef EXPLAIN_EVAL
	get_board("3rr1k1/1pp2qpp/pnn1pp2/8/3PPB2/PQ3N1P/1PR2PP1/2R3K1 b - - 0 28"); 
	fexplain = fopen("evaluation.txt", "w");
	explain = 1; evaluate(); 
	fclose(fexplain); 
	fprintf(stdout, "Press any key...\n"); getchar(); exit(0);
#endif

#ifdef TUNER
	if (argc >= 2) {
		if (!memcmp(argv[1], "client", 6)) Client = 1;
		else if (!memcmp(argv[1], "server", 6)) Server = 1;
		if (Client || Server) Local = 0;
	}
	fprintf(stdout, Client ? "Client\n" : (Server ? "Server\n" : "Local\n"));

	uint64 ctime; QueryProcessCycleTime(GetCurrentProcess(), &ctime); srand(time(NULL) + 123 * GetProcessId(GetCurrentProcess()) + ctime);
	QueryProcessCycleTime(GetCurrentProcess(), &ctime); seed = (uint64)(time(NULL) + 345 * GetProcessId(GetCurrentProcess()) + ctime);
	init_openings();
	init_variables();

	if (Client) {
#ifdef RECORD_GAMES
		RecordGames = 1;
		Buffer = (char*)malloc(16 * 1024 * 1024);
#endif
		while (true) get_command();
	}

	init_pst();
	init_eval();
	print_eval();

	//read_list("(-0.24,0.69,-3.56,14.38,-18.97,-9.43,31.93,-42.58,-84.76,-239.60,62.93,83.44,-124.95,25.59,-22.50,152.24,472.44,-652.13,-903.63,-16.63,11.50,-0.02,-202.44,29.65,-2.27,-62.69,-81.95,61.32,-492.11,-51.01,-23.03,-15.79,283.90,-116.64,-4.38,-92.49,-30.59,-48.53,-35.85,15.25,-83.44,-32.20,33.31,-14.71,27.13,215.48,-48.91,-107.82,5.28,-59.32,-9.16,-16.93,-21.26,-21.12,-35.52,-41.67,-35.52,-16.59,21.48,-1.20,-26.27,-23.81,-58.82,-9.36,38.87,-34.02,-10.33,0.07,101.64,11.30,-66.04,-4.39,10.43,-60.66,-6.41,0.68,-15.18,-69.89,-41.54,-84.48,-143.38,-46.16,-3.12,-13.96,31.00,-16.14,-89.96,100.44,-137.64,97.51,-85.03,62.93,78.39,444.37,-143.70,25.65,-74.57,-143.94,-106.03,-128.86,285.08,111.90,-24.94,-104.36,-142.29,-59.11,-92.95,-32.91,-153.55,15.40,-181.39,-35.76,14.98,-5.08,76.49,-80.38,177.51,132.39,-134.36,-6.67,49.81,-260.99,101.53,-41.31,-26.30,418.42,220.09,-127.18,762.99,-117.88,246.62,-203.99,18.52,266.32,290.73,112.16,292.84,127.11,277.25,189.46,214.95,304.06,399.54,-195.77,280.34,351.89,-485.96,-2.82,251.09,38.25,82.39,152.04,53.11,8.04,7.61,-21.45,10.43,-0.53,4.19,-9.26,13.89,14.56,19.18,7.64,-2.16,138.97,6.71,57.43,0.28,56.89,0.92,-9.14,35.31,1.05,8.57,10.12,34.71,0.23,71.71,76.05,153.65,114.23,85.39,1.34,-12.79,26.11,48.42,125.83,147.73,148.27,41.60,42.53,-14.37,6.87,-6.88,-2.23,130.20,22.09,45.46,15.40,13.11,8.80,2.28,2.99,-0.83,-3.11,-0.81,4.40,6.09,6.27,5.79,5.24,-2.88,-0.26,16.45,-2.67,11.20,7.72,6.17,1.23,3.61,0.08,-0.51,-0.25,9.09,2.08,0.69,0.35,13.18,6.69,0.52,1.58,1.56,-0.95,11.40,0.81,-6.78,3.32,-4.89,8.87,-5.50,31.67,0.30,2.94,0.18,5.42,14.11,33.51,28.03,32.65,21.20,11.16,48.32,14.90,4.31,2.41,2.18,2.69,0.78,0.05,4.27,1.51,17.77,7.82,5.21,1.29,0.15,4.35,-0.12,-0.06,-0.25,3.24,5.37,5.85,14.36,-1.62,9.45,0.47,4.07,5.19,26.33,2.20,20.31,37.81,1.02,82.85,56.61,23.77,19.82,-3.83,47.50,25.50)", Base, active_vars);
	//eval_to_cpp("gd.cpp", Base);

	save_list(Base);

	//pgn_stat();
#ifdef RECORD_GAMES
	match_los(Base, Base, 4 * 64 * 1024, 512, 7, 0.0, 0.0, 0.0, 0.0, MatchInfo, 1);
#endif

	double NormalizedVar[MaxVariables]; NormalizeVar(Base,Var,2,256,10.0,20.0,NormalizedVar); double_to_double(Var,NormalizedVar,active_vars);
	//read_list("(4.10,3.41,7.24,11.13,44.59,41.51,67.17,126.38,183.25,328.40,95.55,442.95,110.04,439.85,199.82,506.49,1000.00,531.27,1000.00,94.70,40.64,96.88,182.65,154.15,152.52,490.96,231.09,605.53,1000.00,36.49,55.68,30.62,35.70,21.40,18.08,38.61,48.23,17.96,33.78,25.56,31.86,16.40,22.84,18.78,35.36,26.99,27.03,27.50,41.10,24.01,21.96,28.26,24.41,19.37,21.28,49.80,30.27,10.66,12.25,43.65,28.65,35.98,75.89,26.88,47.37,8.62,37.29,22.31,60.45,28.59,18.53,100.00,54.22,9.86,10.63,83.68,25.20,124.05,121.47,93.76,81.23,48.30,56.78,56.15,67.16,78.24,169.91,68.80,114.34,43.30,55.89,95.85,122.56,102.36,77.96,112.11,88.70,53.74,76.44,47.93,46.64,70.83,57.70,137.88,108.52,125.92,79.97,33.71,49.84,44.58,192.99,129.64,271.09,79.00,145.01,69.40,193.09,156.78,186.59,391.22,150.93,346.72,80.75,230.85,128.81,46.74,49.53,19.18,39.71,27.84,39.56,60.18,55.76,40.46,31.10,34.48,41.23,25.69,22.04,14.65,27.90,31.79,85.75,49.45,100.00,48.27,25.91,60.07,62.46)", Var, active_vars);

	GD(Base,Var,7,5.0,1.0,50.0,16 * 1024,16 * 1024,3.0,2.0,2.0,0.0);

	double New[1024]; read_list("(5.07,27.02,27.37,15.16,28.60,14.62,40.93,8.61,14.02,172.58,178.09,180.83,457.03,128.24,172.66,178.21,343.44,1281.53,45.85)", New, active_vars);
	for (i = 7; i < 64; i++) {
		fprintf(stdout, "\ndepth = %d/%d: \n", i, i + 1);
		match_los(New, Base, 4 * 1024, 128, i, 3.0, 3.0, 0.0, 0.0, MatchInfo, 1);
	}
#endif

	while (true) uci();
}