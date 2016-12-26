#pragma once
#include "Parameters.h"
#include "zobrist.h"

const int BoardSize = 16 * 12;

const int FileA = 0;const int FileB = 1;const int FileC = 2;const int FileD = 3;
const int FileE = 4;const int FileF = 5;const int FileG = 6;const int FileH = 7;

const int Rank1 = 0;const int Rank2 = 1;const int Rank3 = 2;const int Rank4 = 3;
const int Rank5 = 4;const int Rank6 = 5;const int Rank7 = 6;const int Rank8 = 7;

const int Empty = 0;
const int Edge = -1;

const int A1=0x24, B1=0x25, C1=0x26, D1=0x27, E1=0x28, F1=0x29, G1=0x2A, H1=0x2B;
const int A2=0x34, B2=0x35, C2=0x36, D2=0x37, E2=0x38, F2=0x39, G2=0x3A, H2=0x3B;
const int A3=0x44, B3=0x45, C3=0x46, D3=0x47, E3=0x48, F3=0x49, G3=0x4A, H3=0x4B;
const int A4=0x54, B4=0x55, C4=0x56, D4=0x57, E4=0x58, F4=0x59, G4=0x5A, H4=0x5B;
const int A5=0x64, B5=0x65, C5=0x66, D5=0x67, E5=0x68, F5=0x69, G5=0x6A, H5=0x6B;
const int A6=0x74, B6=0x75, C6=0x76, D6=0x77, E6=0x78, F6=0x79, G6=0x7A, H6=0x7B;
const int A7=0x84, B7=0x85, C7=0x86, D7=0x87, E7=0x88, F7=0x89, G7=0x8A, H7=0x8B;
const int A8=0x94, B8=0x95, C8=0x96, D8=0x97, E8=0x98, F8=0x99, G8=0x9A, H8=0x9B;

const int CastleNone = 0;
const int WhiteKingCastle  = 1;
const int WhiteQueenCastle = 2;
const int BlackKingCastle  = 4;
const int BlackQueenCastle = 8;

const int MoveNormal    = 0;
const int MoveCastle    = 1 << 14;
const int MovePromote   = 2 << 14;
const int MoveEnPassant = 3 << 14;
const int MoveFlags     = 3 << 14;

const int MovePromoteKnight = MovePromote | (0 << 12);
const int MovePromoteBishop = MovePromote | (1 << 12);
const int MovePromoteRook   = MovePromote | (2 << 12);
const int MovePromoteQueen  = MovePromote | (3 << 12);

const int MoveDoublePush =   1 << 12;
const int MoveAllFlags   = 0xF << 12;

const char NullMoveString[] = "null";


// Disjoint Piece Flags
const int White = 1;
const int Black = 2;
const int Pawn = 4;
const int Knight = 8;
const int Bishop = 16;
const int Rook = 32;
const int Queen = 64;
const int King = 128;
// color piece value
const int wPawn = 5;
const int wKnight = 9;
const int wBishop = 17;
const int wRook = 33;
const int wQueen = 65;
const int wKing = 129;

const int bPawn = 6;
const int bKnight = 10;
const int bBishop = 18;
const int bRook = 34;
const int bQueen = 66;
const int bKing = 130;

//const unsigned char Empty = 0;
//const unsigned char Edge = 0xff;

const int FlagsWhiteKingCastle = 1;
const int FlagsWhiteQueenCastle = 2;
const int FlagsBlackKingCastle = 4;
const int FlagsBlackQueenCastle = 8;
// Piece Lists
const int MAXPAWNS=8;
const int MAXKNIGHTS=2+8;
const int MAXBISHOPS=2+8;
const int MAXROOKS=2+8;
const int MAXQUEENS=1+8;

const int OFFSETDELTA =120;

// See DataStructure
struct AttSquare {
   int size;
   int square[15];
   int ChessMan[16];
   void Add(int sq,int chessman);
};

typedef struct AttSquare Attacks[2];
typedef Attacks *pAttacks;

// Evaluation
extern int Pst[12][64][2]; // piece,square,stage
const int MATE = 32000;


enum CenterType {
	WIDEOPEN,OPEN,AVERAGE,CLOSED,BLOCKED,ENDGAME
};

const int KnightDir[9] = {
   -33, -31, -18, -14, +14, +18, +31, +33, 0
};

const int BishopDir[5] = {
   -17, -15, +15, +17, 0
};

const int RookDir[5] = {
   -16, -1, +1, +16, 0
};

const int QueenDir[9] = {
   -17, -16, -15, -1, +1, +15, +16, +17, 0
};

const int KingDir[9] = {
   -17, -16, -15, -1, +1, +15, +16, +17, 0
};

// Attacks bit masks
const int PawnBitMask = 0;
const int KnightBitMask = 8;
const int BishopBitMask = 16;
const int RookBitMask = 24;
const int QueenBitMask = 32;
const int KingBitMask = 40;


class Board
{
public:
	Board(void);
	~Board(void);

	int wtm;
	int IsImbalance;
	int MatConfig;
	u64 hash;

	void LoadFen(char *fen);
	void SaveFEN(char *dest);
	// search
	int TotalNodes;
	int Search();
	int Optimism();
	int MoveToAlgebra(int move,char *dest);
	// Move Gen
	void GenPseudoMoves(MoveList &List);
	int IsKCapture(int move);
	void DoMove(int move, UndoData &undo);
	void UndoMove(int move, UndoData &undo);
	bool IsLegal();
	bool IsCheck() { return IsInCheck(wtm);};
	void DoNullMove(UndoData &undo);
	void UndoNullMove(UndoData &undo);
	void DoMoveAlgebraic(char *move);
	int IdentificaPgn(char *JugAlg);
	void WeightPstMoves(MoveList &List);
	// Eval.
	int GetEval();
	void EvalMaterial();
	void MEvalMaterial();
	int phase;
	int Material[2];
	int PawnEval[2];
	int DevelopmentEval[2];
	int KnightValue[2];
	int EvalBishop[2];
	int EvalRook[2];
	int EvalQueen[2];
	int PawnCover[2];
	int PiecePlacement[2];
	int isEndGame;
// debug
	void Display();
	void Display(FILE *fdout){DisplayF(fdout);};
	void DoPerft(int ply);
	void TestPerft();
	void DebugSimetric(char *Epdfilename);
	void LoadFenSimetric(char *fen);
	void DisplayScores();

private:
// ref: http://chessprogramming.wikispaces.com/16%2A12+Board
	//The 16*12 board Vector Attacks combines the property of mailbox with its surrounding ranks and files, 
	//which ensure a knight can not jump off the board, with the 0x88 property of unique differences of any 
	//two squares with respect to distance and direction by using eight boarder files. 
	//Similar to 0x88 the 16*12 board can get the single step move-increment and number of steps directly 
	//from a one-dimensional lookup table, indexed by the difference of from- and to-squares plus some offset, 
	//to perform an efficient move legality test (e.g. a move from TT or killer) similar to obstruction lookup 
	//by 0x88 difference with bitboards, but a short blocker loop. 
	//XXXXXXXXXXXXXXXX
	//XXXXXXXXXXXXXXXX
	//XXXX--------XXXX
	//XXXX--------XXXX
	//XXXX--------XXXX
	//XXXX--------XXXX
	//XXXX--------XXXX
	//XXXX--------XXXX
	//XXXX--------XXXX
	//XXXX--------XXXX
	//XXXXXXXXXXXXXXXX
	//XXXXXXXXXXXXXXXX
	int board[BoardSize];
   int pos[BoardSize];
	int CastleFlags;
	int EnPassant;
public:
   int PlyNumber;
private:
	// A peculiarity of the mailbox structure is that the difference between two squares tell us 
    // if a chessman can attack and in which direction.
	// Attacks
	int AttackDirection[256]; 
	int AttackChessman[256]; 

	// Piece Lists
	// definition of maximum possible piece constants
	int bPawnList[MAXPAWNS];
	int bKnightList[MAXKNIGHTS];
	int bBishopList[MAXBISHOPS];
	int bRookList[MAXROOKS];
	int bQueenList[MAXQUEENS];
	int bKingPos;
	int wPawnList[MAXPAWNS];
	int wKnightList[MAXKNIGHTS];
	int wBishopList[MAXBISHOPS];
	int wRookList[MAXROOKS];
	int wQueenList[MAXQUEENS];
	int wKingPos;
public:
	int NumBPawn;
	int NumBKnight;
	int NumBBishop;
	int NumBRook;
	int NumBQueen;
	int NumWPawn;
	int NumWKnight;
	int NumWBishop;
	int NumWRook;
	int NumWQueen;

private:

	int CastleMask[BoardSize];


	int VisitedNodes;
	
	// Coordinate Transformation
	inline int sq16x12(int rank07,int file07) { return 16 * rank07 + file07 + 36;};
	inline int rank07(int sq16x12) {return (sq16x12 - 36) >> 4;}; 
	inline int file07(int sq16x12) {return (sq16x12 - 36) &  7;}; 
	inline int sq64to16x12(int x) { return x+(x& ~7)+A1;};
	inline int sq16x12to64(int x) { return RankSq16x12(x)*8+FileSq16x12(x);};
	inline int FileSq16x12(int x) { return (x-A1)&7;};
	inline int RankSq16x12(int x) { return (x-A1) >>4;};
	inline int sq16x12Color(int x) { return sq64Color(sq16x12to64(x));};
	inline int sq64Color(int x) { return ((x ^ (x >> 4)) & 1)^1;};
	inline int FileFChar(char x) { return (x-'a');};
	inline int RankFChar(char x) { return (x-'1');};
	inline char CharFile(int x) { return 'a'+x;};
	inline char CharRank(int x) { return '1'+x;};
	inline int piece_colour(int p) { return (p&3);}; 
	inline int square_is_ok(int x) {return sq16x12to64(x) >= 0 && sq16x12to64(x) < 64;};
	inline bool IsPiece(int x) { return x >= 0 && x < 256;};
	inline int PieceUncolored(int x) { return x &~3;};
public:
	inline int MoveTo(short move) { return sq64to16x12(move & 077);};
	inline int MoveFrom(short move) { return sq64to16x12(((move)>>6)&077);};
	static inline int MoveTo64(short move) { return (move & 077);};
	static inline int MoveFrom64(short move) { return (((move)>>6)&077);};
	int IsKingCapture(int move);
	// move gen
private:
	void GenCaptures(MoveList &List);
	void InitBoard();
	inline void PutbPawn(int square){board[square] = bPawn;	};
	inline void PutwPawn(int square){board[square] = wPawn;};
	inline void PutbKnight(int square){board[square] = bKnight;};
	inline void PutwKnight(int square){board[square] = wKnight;};
	inline void PutbBishop(int square){board[square] = bBishop;};
	inline void PutwBishop(int square){board[square] = wBishop;};
	inline void PutbRook(int square){board[square] = bRook;};
	inline void PutwRook(int square){board[square] = wRook;};
	inline void PutbQueen(int square){board[square] = bQueen;};
	inline void PutwQueen(int square){board[square] = wQueen;};
	inline void PutbKing(int square){board[square] = bKing;};
	inline void PutwKing(int square){board[square] = wKing;};
	inline int piece_is_pawn(int x) {return x & 4;};
	inline int ColorOpp(int x) { return x^3;};
	bool board_is_ok();
	void Clear();
	void Initialize();
	// move 
	inline short MakeMove(int f,int t) { return sq16x12to64(f)<<6 | sq16x12to64(t);};
	inline short MakeMoveSp(int f,int t,int flags) { return sq16x12to64(f)<<6 | sq16x12to64(t)|flags;};
	inline bool IsPromoteSquare(int x) { return RankSq16x12(x) == Rank1 ||RankSq16x12(x) == Rank8;};
	inline int KingPos(int colour) {  return wtm == White ? wKingPos : bKingPos;};
	inline bool MoveIsPromote(int move) { return (((move)&MoveFlags)==MovePromote);}
	// move gen
	void GenAllMoves(MoveList &List);
	void GenOnePawnMove(MoveList &List, int from, int to);
	void GenPromote(MoveList &List,int move);
	void GenRegularCaptures(MoveList &List);
	void GenEnPassant(MoveList &List);
	void GenCastleMove(MoveList &List);

	// Attacks
	void InitAttack();
	inline bool DeltaIsOk(int delta) {   if (delta < -119 || delta > +119) return false;
	   return true;};

	inline bool IsInCheck(int colour) {   return IsAttacked(KingPos(colour),ColorOpp(colour));}
	inline bool IsInCheck() { return IsInCheck(wtm);};
	bool IsAttacked(int to, int colour);

	inline int AttackCM(int delta) { return AttackChessman[OFFSETDELTA+(delta)];};
	inline int CanAttack(int piece,int delta) { return (piece & 252) & AttackCM(delta);};
	inline int AttackDir(int delta) { return AttackDirection[OFFSETDELTA+(delta)];};

		// Moves
	void ClearSquare(int square);
	void PutSquare(int square,int pieza);
	void MoveSquare(int from,int to,int pieza);
	void CastleMask_init();
	inline bool MoveIsDoblePush(int move){ return (move & MoveAllFlags)==MoveDoublePush;};
	inline bool MoveIsEnpassant(int move) { return (move & MoveFlags)==MoveEnPassant;};
	inline int SquareEnPassant(int square) { return square ^ 16;};
	inline int PromotePiece(int move) { return 4 << (((move >> 12)&3)+1);};
	inline bool IsCastle(int move) { return (move & MoveFlags)==MoveCastle;};
	inline unsigned int Rank64(int s)	{return ((s) >> 3);};
	inline unsigned int File64(int s)	{ return ((s) & 7);};
	inline int ColourSq64(int s)	{return (((Rank64(s) + File64(s)) % 2)^1)+1;};
	int piece_to_12(int x);

	// Search
	int AlphaBeta(int depth, int alpha,int beta);
	int Selective(int alpha,int beta);

	void Perft(int ply);
	bool IsPromote(int move);

	// Evalu8
	CenterType IdCenter();
	int Semi[2][8];
	int Open[8];
	int PawnEvaluation();
	int MPawnEvaluation();
	void Backward();
	void Development();
	void EvalKnight();
	void BishopEvaluation();
	void RookEvaluation();
	void QueenEvaluation();
	void PawnCoverage();
	void PPlacement();
	int RookPin();
	void WeightMoves(MoveList &List);
	// See
	int SeeMove(int move);
public:
	void DisplayF(FILE *display_file);
	int EvalUfo();
	int Eval1();
	int Eval2();
	int BestMoveSearch;
	int ProbeDepth;
	int EvalPst();

	// Knowledge level
public:
	static void SetLevel(int level);
	void AcumTuneError(int error);
	int GetIdStruct();
	inline int GetFromTo(int move,int &from,int &to){ from = sq16x12to64(MoveFrom(move)); to = sq16x12to64(MoveTo(move));return piece_to_12(board[MoveFrom(move)]);}
	void PreEval();
	int Opening,EndGame;
	int ValueOfPieceFrom(int move);
private:
	int stHistory;
	static const int MAXDEPTH = 20;
	// moves 
	u64 HashStack[MAXDEPTH];
	void SetHashStack(u64 hash);
	int Repetition(u64 hash);
	void ResetHashStack();
	void PopHashStack();
	// search Simplex
	int NodosRepasados;
	int Depth;
	int SelDepth;
	int inicio; // para controlar el tiempo de pensar.

	void PrintInfo(int value,char * path);
	void ParsePonder(char *pv);
	int PVS(int depth, int alpha, int beta,char *Global,int doNull);
	void IncNodes();
	int ValorMate(int signo);
	u64 GetHash();
	int TotalPCtm();

	int HashJugada;
	int Killer[2];

	int GetMaterial();
	void WeightMovesPVS(MoveList &List);

public:
	// Transposition Table
	void *TT;
	void InitTT(int size);
	int IterativeDeepening(void);
	int tiempo_limite; // limite de tiempo.
	int LimiteProfundidad;
	int PonderMode;
	char JugadaActual[10]; // Change to ActualMove
	char ponderMove[8];
	int TTSize;
	int InBStar;
	int cancelado;
private:
	// This array must be the base for some evaluation concepts (mobility,King safety) and to complement
	// the evaluation with a static exchange evaluation.
	u64 wAttacks[BoardSize];
	u64 bAttacks[BoardSize];
	u64 wManBits; // bits for actual situation on board.
	u64 bManBits;
	void GenAttacks();
	int NegaMax(int depth,int alpha,int beta);
	int HandleCheck(int depth,int alpha,int beta);
	int SeeSquare(u64 bits1,u64 bits2,int bet);
	int SeeBoard();
	int Undefend();
public:
	int SearchNegaM();
	int SeeEval();
};

typedef int (Board::*EVAL_FUNCTION) (void);
extern void InitPST();
