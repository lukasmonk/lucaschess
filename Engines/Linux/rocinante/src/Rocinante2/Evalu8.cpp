#include <stdlib.h>
#include "CDebug.h"
#include "MoveList.h"
#include "UndoData.h"
#include "Board.h"
#include <memory.h>

#define Param(n,v)	v
short Parameters[MaxParameters] ={
#include "Param.h"
};
#undef Param
#define Param(n,v)	#n
char * ParametersN[MaxParameters] ={
#include "Param.h"
};

// list of available Evaluation function.
static EVAL_FUNCTION funs[] = {
	&Board::EvalUfo,				// 0 -> minimal level of knowledge UFO evaluation
	&Board::Eval1,
	&Board::Eval2,
};

static int KLevel; // uci configurable

static EVAL_FUNCTION EvalLevel;

void Board::SetLevel(int level)
{
	KLevel = level;
	if(level < sizeof(funs)/sizeof(EVAL_FUNCTION))
	{
		EvalLevel = funs[level];
	}
	else
	{
		EvalLevel = funs[0]; // minimal
	}
	InitPST();
}
// public
int Board::GetEval()
{
	if(wtm == White) // convert to side to move perspective.
		return (this->*EvalLevel)();
	else
		return -(this->*EvalLevel)();

}

#include "mat.h"
#include "pst.h"

int PassedWeight[8];
int PassedWeightEG[8];

int SafeWeight[256] = {
};
int nBits[256] = {
};
void GenSafeArray()
{
	int i,j,k,l;

	for(i=0; i < 256;i++)
	{
		k = l = 0;
		for(j=0; j < 8;j++)
		{
			if(i&(1<<j))
			{
				l++;
				switch(j)
				{
				case 0:
				case 1:
					k += Parameters[SafetyN];
					break;
				case 2:
				case 3:
					k += Parameters[SafetyB];
					break;
				case 4:
				case 5:
					k += Parameters[SafetyR];
					break;
				case 6:
					k += Parameters[SafetyQ];
					break;
				case 7:
					k += Parameters[SafetyK];
					break;
				}
			}
		}
		nBits[i] = l;
		SafeWeight[i] = k;
	}
}

void InitPstStarter()
{
	extern int Pst[12][64][2]; // piece,square,stage
	int sq,chessman,f,r;

	valuePiece[0][0] = Parameters[_Pawn];
	valuePiece[0][1] = Parameters[_Knight];
	valuePiece[0][2] = Parameters[_Bishop];
	valuePiece[0][3] = Parameters[_Rook];
	valuePiece[0][4] = Parameters[_Queen];
	valuePiece[1][0] = Parameters[_PawnEG];
	valuePiece[1][1] = Parameters[_KnightEG];
	valuePiece[1][2] = Parameters[_BishopEG];
	valuePiece[1][3] = Parameters[_RookEG];
	valuePiece[1][4] = Parameters[_QueenEG];

	for(r=0; r < 8;r++)
	{
		PassedWeight[r] = ((r *(r+1))/2) * Parameters[_Passed];
		PassedWeightEG[r] = ((r *(r+1))/2) * Parameters[_PassedEG];
	}

	// ini PstStarter from parameters.
//	static int px[8] = {0,1,2,3,3,2,1,0};
	static int px[8] = {1,2,3,4,4,3,2,1};
	static int ad[8] = {0,1,2,3,4,5,6,7};
	for(chessman = 0; chessman < 6;chessman++)
	{
		for(f = 0; f < 8;f++)
		{
			for(r = 0; r < 8;r++)
			{
				sq = r*8+f;
				PstStarter[chessman][sq][0] = px[r] * Parameters[_XPawn+0+chessman * 6] + ad[r] * Parameters[_XPawn+2+chessman * 6] + px[f] * Parameters[_XPawn+1+chessman*6];
				PstStarter[chessman][sq][1] = px[r] * Parameters[_XPawn+3+chessman * 6] + ad[r] * Parameters[_XPawn+5+chessman * 6] + px[f] * Parameters[_XPawn+4+chessman*6];
			}
		}
	}
	// E3 D3
	PstStarter[0][19][0] += Parameters[_PCentre3];
	PstStarter[0][20][0] += Parameters[_PCentre3];
	// E4 D4
	PstStarter[0][27][0] += Parameters[_PCentre];
	PstStarter[0][28][0] += Parameters[_PCentre];
	// E5 D5
	PstStarter[0][35][0] += Parameters[_PCentre5];
	PstStarter[0][36][0] += Parameters[_PCentre5];
	// A2 B2 C2 F2 G2 H2
	PstStarter[0][8][0] += Parameters[_PSecond];
	PstStarter[0][9][0] += Parameters[_PSecond];
	PstStarter[0][10][0] += Parameters[_PSecond];
	PstStarter[0][13][0] += Parameters[_PSecond];
	PstStarter[0][14][0] += Parameters[_PSecond];
	PstStarter[0][15][0] += Parameters[_PSecond];

	// Knight A1 H1 A8 H8
	PstStarter[0][0][0] += Parameters[_KnightA1];
	PstStarter[0][7][0] += Parameters[_KnightA1];
	PstStarter[0][56][0] += Parameters[_KnightA1];
	PstStarter[0][63][0] += Parameters[_KnightA1];
	PstStarter[0][0][1] += Parameters[_KnightA1EG];
	PstStarter[0][7][1] += Parameters[_KnightA1EG];
	PstStarter[0][56][1] += Parameters[_KnightA1EG];
	PstStarter[0][63][1] += Parameters[_KnightA1EG];

	for(chessman = 0; chessman < 6;chessman++)
	{
		for(sq = 0;sq < 64;sq++)
		{
			Pst[chessman*2][sq][0] = PstStarter[chessman][sq][0];
			Pst[chessman*2][sq][1] = PstStarter[chessman][sq][1];
		}
	}
	for(chessman = 0; chessman < 6;chessman++)
	{
		for(sq = 0;sq < 64;sq++)
		{
			Pst[1+chessman*2][sq^070][0] = -PstStarter[chessman][sq][0];
			Pst[1+chessman*2][sq^070][1] = -PstStarter[chessman][sq][1];
		}
	}
	GenSafeArray();
}


#include "ufo.h"
void InitPST()
{
	switch(KLevel)
	{
	case 1:
	case 2:
		InitPstStarter();
		break;
	case 0:
	default:
		ufoEvaluation.Initialize();
		break;
	}
}
void Board::PreEval()
{
	// de momento inicializamos ufo
//	DataEval.InitPstUfo(0);
}
int Board::Eval1()
{
	int opening,endgame;
	int Value = 0;

	EvalMaterial();
	opening = Material[0];
	endgame = Material[1];

	// PST
	opening += Opening;
	endgame += EndGame;
	
	PawnEvaluation();
	opening += PawnEval[0];
	endgame += PawnEval[1];

	Development();  // +332-329=339
	opening += DevelopmentEval[0];
	endgame += DevelopmentEval[1];

	EvalKnight();
	opening += KnightValue[0];
	endgame += KnightValue[1];

	BishopEvaluation();
	opening += EvalBishop[0];
	endgame += EvalBishop[1];

	RookEvaluation();
	opening += EvalRook[0];
	endgame += EvalRook[1];

	QueenEvaluation();
	opening += EvalQueen[0];
	endgame += EvalQueen[1];

	PawnCoverage();
	opening += PawnCover[0];
	endgame += PawnCover[1];

	 //Tappered
	int Value0 = ((opening *(phase) + endgame *(64-phase)) / 64);

	return Value0;	
}

int Board::EvalPst()
{
	extern int Pst[12][64][2]; // piece,square,stage
	int Value  = 0;
	isEndGame = (NumWRook+NumWKnight+NumWBishop+NumWQueen) < 3 || (NumBRook+NumBKnight+NumBBishop+NumBQueen) < 3;
	// reyes;
	int i,sq;
	Opening = EndGame = 0;
	for(i=0; i < NumWPawn;i++)
	{
		sq = sq16x12to64(wPawnList[i]);
		Opening += Pst[0][sq][0];EndGame += Pst[0][sq][1];
		Value += Pst[0][sq][0];
	}
	for(i=0; i < NumBPawn;i++)
	{
		sq = sq16x12to64(bPawnList[i]);
		Value += Pst[1][sq][0];
		Opening += Pst[1][sq][0];EndGame += Pst[1][sq][1];
	}

	for(i=0; i < NumWKnight;i++)
	{
		sq = sq16x12to64(wKnightList[i]);
		Value += Pst[2][sq][0];
		Opening += Pst[2][sq][0];EndGame += Pst[2][sq][1];
	}
	for(i=0; i < NumWBishop;i++)
	{
		sq = sq16x12to64(wBishopList[i]);
		Value += Pst[4][sq][0];
		Opening += Pst[4][sq][0];EndGame += Pst[4][sq][1];
	}
	for(i=0; i < NumWRook;i++)
	{
		sq = sq16x12to64(wRookList[i]);
		Value += Pst[6][sq][0];
		Opening += Pst[6][sq][0];EndGame += Pst[6][sq][1];
	}
	for(i=0; i < NumWQueen;i++)
	{
		sq = sq16x12to64(wQueenList[i]);
		Value += Pst[8][sq][0];
		Opening += Pst[8][sq][0];EndGame += Pst[8][sq][1];
	}
	sq = sq16x12to64(wKingPos);
	Opening += Pst[10][sq][0];EndGame += Pst[10][sq][1];

	if(isEndGame)
		Value += Pst[10][sq][1];
	else
		Value += Pst[10][sq][0];
	for(i=0; i < NumBKnight;i++)
	{
		sq = sq16x12to64(bKnightList[i]);
		Value += Pst[3][sq][0];
		Opening += Pst[3][sq][0];EndGame += Pst[3][sq][1];
	}
	for(i=0; i < NumBBishop;i++)
	{
		sq = sq16x12to64(bBishopList[i]);
		Value += Pst[5][sq][0];
		Opening += Pst[5][sq][0];EndGame += Pst[5][sq][1];
	}
	for(i=0; i < NumBRook;i++)
	{
		sq = sq16x12to64(bRookList[i]);
		Value += Pst[7][sq][0];
		Opening += Pst[7][sq][0];EndGame += Pst[7][sq][1];
	}
	for(i=0; i < NumBQueen;i++)
	{
		sq = sq16x12to64(bQueenList[i]);
		Value += Pst[9][sq][0];
		Opening += Pst[9][sq][0];EndGame += Pst[9][sq][1];
	}
	sq = sq16x12to64(bKingPos);
	Opening += Pst[12][sq][0];EndGame += Pst[12][sq][1];
	if(isEndGame)
		Value += Pst[11][sq][1];
	else
		Value += Pst[11][sq][0];
	return Value;
}

int Board::EvalUfo()
{
	int Value  = 0;
//	Opening = EndGame;
	isEndGame = (NumWRook+NumWKnight+NumWBishop+NumWQueen) < 3 || (NumBRook+NumBKnight+NumBBishop+NumBQueen) < 3;
	if(isEndGame)
		Value += EndGame;
	else
		Value += Opening;
//	Value = EvalPst(); // running eval of psts
//P = 100 N = 320 B = 330 R = 500 Q = 900 K = 20000
	Value += (NumWPawn - NumBPawn) * 100; 
	Value += (NumWKnight - NumBKnight) * 320;
	Value += (NumWBishop - NumBBishop) * 330;
	Value += (NumWRook - NumBRook) * 500;
	Value += (NumWQueen - NumBQueen) * 900;
	return Value;
}

int Board::ValueOfPieceFrom(int move)
{
	int p = board[MoveFrom(move)];
	switch(p)
	{
	case bPawn:
		return -100;
	case wPawn:
		return 100;
	case bKnight:
		return -320;
	case wKnight:
		return 320;
	case bBishop:
		return -330;
	case wBishop:
		return 330;
	case bRook:
		return -500;
	case wRook:
		return 500;
	case bQueen:
		return -900;
	case wQueen:
		return 900;
	}
	return 0;
}

void Board::EvalMaterial()
{
	// First isEndGame (used in IdCenter())
	isEndGame = (NumWRook+NumWKnight+NumWBishop+NumWQueen) < 3 || (NumBRook+NumBKnight+NumBBishop+NumBQueen) < 3;
	phase = (NumWKnight +NumBKnight+NumWBishop+NumBBishop) * 3;  // max 24
	phase += (NumWRook +NumBRook) * 5;		// max 20
	phase += (NumWQueen + NumBQueen) * 10;  // 20
	if(phase > 64) // more than two queen
		phase = 64;
	int pPhase = (NumWPawn + NumBPawn);
	Material[0] = Material[1] = 0;
	int opening,endgame;
	opening=0 ; endgame = 0;
	int pfPhase = 10 - NumWPawn - NumBPawn;
	CenterType ct = IdCenter();

	//if(Parameters[_UseB])
	//{
	//	//WIDEOPEN,OPEN,AVERAGE,CLOSED,BLOCKED,ENDGAME
	//	int ValuePieces[5][6] = {
	//		{100,100,100,100,100,100,}, // pawn
	//		{305,300,300,300,300,300,}, // Knight
	//		{305,300,300,300,300,300,}, // Bishop
	//		{500,500,500,500,500,500,}, // Rook
	//		{900,900,900,900,900,900,}, // Queen
	//	};
	//	opening += (NumWPawn - NumBPawn) * ValuePieces[0][ct];
	//	opening += (NumWKnight - NumBKnight) * ValuePieces[1][ct];
	//	opening += (NumWBishop - NumBBishop) * ValuePieces[2][ct];
	//	opening += (NumWRook - NumBRook) * ValuePieces[3][ct];
	//	opening += (NumWQueen - NumBQueen) * ValuePieces[4][ct];
	//	endgame += opening;
	//}
	//else
	{

	int SumP,SumN,SumB,SumR,SumQ,DifP,DifN,DifB,DifR,DifQ;
	SumP = (NumWPawn + NumBPawn);
	DifP = (NumWPawn - NumBPawn);
	SumN = (NumWKnight + NumBKnight);
	DifN = (NumWKnight - NumBKnight);
	SumB = (NumWBishop + NumBBishop);
	DifB = (NumWBishop - NumBBishop);
	SumR = (NumWRook + NumBRook);
	DifR = (NumWRook - NumBRook);
	SumQ = (NumWQueen + NumBQueen);
	DifQ = (NumWQueen - NumBQueen);

	IsImbalance = 0;
	if(DifP || DifN+DifB || DifR || DifQ)
		IsImbalance = 1;

//	MatConfig = DifP + 17 * DifN + 85 * DifB + 425 * DifR + 2125 * DifQ;
//	MatConfig = DifP + 3 * DifN + 3 * DifB + 5 * DifR + 9 * DifQ;
	if(IsImbalance )
	{
		// Q <> RR
		if(DifQ * DifR < 0)
		{
			opening += DifQ * Parameters[_Q_RR];
			endgame += DifQ * Parameters[_Q_RREG];
		}
		if(DifR * (DifN+DifB) < 0)
		{
			opening += DifR * Parameters[_R_M];
			endgame += DifR * Parameters[_R_MEG];
		}
	}

//	if(opening == 0 && endgame == 0)
	{
		opening += DifP * valuePiece[0][0]; 
		opening += DifN * valuePiece[0][1];
		opening += DifB * valuePiece[0][2];
		endgame += DifP * valuePiece[1][0]; 
		endgame += DifN * valuePiece[1][1];
		endgame += DifB * valuePiece[1][2];


		opening += DifR * valuePiece[0][3];
		opening += DifQ * valuePiece[0][4];

		endgame += DifR * valuePiece[1][3];
		endgame += DifQ * valuePiece[1][4];

		opening += ((NumWBishop/2) -(NumBBishop/2)) * Parameters[_BishopPair];
		endgame += ((NumWBishop/2) -(NumBBishop/2)) * Parameters[_BishopPairEG];

		if(ct == WIDEOPEN)
		{
			opening += ((NumWBishop) -(NumBBishop)) * Parameters[_BishopWO];
			endgame += ((NumWBishop) -(NumBBishop)) * Parameters[_BishopWOEG];
			
		}
//		opening += ((NumWRook/2) -(NumBRook/2)) * Parameters[_RookPair];
//		endgame += ((NumWRook/2) -(NumBRook/2)) * Parameters[_RookPairEG];
	}
	}
	//// Bad Bishop
	//if(ct == BLOCKED)
	//{
	//	opening += (NumWKnight - NumBKnight) * Parameters[pn_BlockedBonus];
	//	endgame += (NumWKnight - NumBKnight) * Parameters[pn_BlockedBonusEG];

	//	int ColorPawn,sq,i,sq64;
	//	// obtenemos el color de los peones centrales propios. (10%-)
	//	ColorPawn = 0;
	//	for(sq = E2; sq < E8; sq +=16)
	//	{
	//		if(board[sq] == wPawn && board[sq+16] == bPawn)
	//		{
	//			sq64 = sq16x12to64(sq);
	//			ColorPawn = ColourSq64(sq64);
	//			break;
	//		}
	//	}
	//	if(ColorPawn != 0)  // Bug ?
	//	{
	//		// buscamos el alfil de ese color
	//		for(i=0; i < NumWBishop;i++)
	//		{
	//			sq = wBishopList[i];
	//			sq64 = sq16x12to64(sq);
	//			if(ColorPawn == ColourSq64(sq64))
	//			{
	//				opening -= Parameters[pn_BadBishop];
	//				endgame -= Parameters[pn_BadBishopEG];
	//			}
	//		}
	//		for(i=0; i < NumBBishop;i++)
	//		{
	//			sq = bBishopList[i];
	//			sq64 = sq16x12to64(sq);
	//			if(ColorPawn != ColourSq64(sq64))
	//			{
	//				opening += Parameters[pn_BadBishop];
	//				endgame += Parameters[pn_BadBishopEG];
	//			}
	//		}
	//	}
	//}

	if(phase == 3) // one minor alone
	{
		if(pPhase == 0) 
		{
			// no pawn it's a draw
			opening = 0;
			endgame = 0;
		}
	}

	// asignamos el valor general
	Material[0] = opening;
	Material[1] = endgame;
}

CenterType Board::IdCenter()
{

	if(isEndGame ) return ENDGAME;
	// number of blocked pawns on d&e;
	int Blockeds = 0;
	int CentralPawns = 0;
	int sq;
	for(sq = E2; sq < E8; sq +=16)
	{
		if(board[sq] == wPawn)
		{
			CentralPawns++;
			if(board[sq+16] == bPawn)
			{
				Blockeds++;
			}
		}
		else
			if(board[sq] == bPawn)
				CentralPawns++;
	}
	for(sq = D2; sq < D8; sq +=16)
	{
		if(board[sq] == wPawn)
		{
			CentralPawns++;
			if(board[sq+16] == bPawn)
			{
				Blockeds++;
			}
		}
		else
			if(board[sq] == bPawn)
				CentralPawns++;
	}


	if(NumWPawn < 7 && NumBPawn < 7)
	{
		// There is No central pawn blocked 
		if(!Blockeds)
			return WIDEOPEN;
	}
	// open: one pair of centre pawn have been exchanged
	// and there are no blocked pawn.
	if(NumWPawn < 8 && NumBPawn < 8)
	{
		// There is No central pawn blocked 
		if(!Blockeds)
			return OPEN;
	}
//	if(!Blockeds && CentralPawns == 2)
//		return OPEN;

	if(NumWPawn == 8 && NumBPawn == 8)
	{
		// si no hay un par de peones centrales bloqueados
		if(Blockeds == 1) return CLOSED;
		if(Blockeds > 1)
			return BLOCKED;
	}
	return AVERAGE;
}

int Board::PawnEvaluation()
{
	int Value = 0;
	int ValueEG = 0;
	int i,j,pos;
	int f,r;
	int Double[2];
	int Isolany[2];
	int pCount[2][8];
	int Isolated[2][8];
	int Passed[2][8];
	int Connected[2][8];

	PawnEval[0] = PawnEval[1] = 0;
	Double[0] = Double[1] = 0;

	j = 0;
	for(i=0;i < 8;i++)
		pCount[0][i] = pCount[1][i] = 0;
	for(i=0; i < NumWPawn;i++)
	{
		pos = wPawnList[i];
		f = file07(pos);
		pCount[0][f]++;
	}
	for(i=0; i < NumBPawn;i++)
	{
		pos = bPawnList[i];
		f = file07(pos);
		pCount[1][f]++;
	}
	// evaluamos mayorias
	for(i = 0,f=0; i < 4;i++)
	{
		if(pCount[0][i])
			f++;
		if(pCount[1][i])
			f--;
	}
	if(f > 0)
	{
			ValueEG += Parameters[pn_MayoriaEG];
			Value += Parameters[pn_Mayoria] ;
	}
	if(f < 0)
	{
			ValueEG -= Parameters[pn_MayoriaEG];
			Value -= Parameters[pn_Mayoria];
	}
	for(i = 4,f=0; i < 8;i++)
	{
		if(pCount[0][i])
			f++;
		if(pCount[1][i])
			f--;
	}
	if(f > 0)
	{
		ValueEG += Parameters[pn_MayoriaEG];
		Value += Parameters[pn_Mayoria];
	}
	if(f < 0)
	{
		ValueEG -= Parameters[pn_MayoriaEG] ;
		Value -= Parameters[pn_Mayoria];
	}
	// open & Semi.
	for(i=0; i < 8;i++)
	{
		Open[i] = 0;
		Semi[0][i] = Semi[1][i] = 0;
		if(pCount[0][i] == 0)
		{
			if(pCount[1][i] == 0)
				Open[i] = 1;
			else
			{
				if(pCount[1][i] > 1)
					Double[1] += pCount[1][i]-1;
				Semi[0][i] = 1;
			}
		}
		else
		{
			if(pCount[0][i] > 1)
				Double[0] += pCount[0][i]-1;
			if(pCount[1][i] == 0)
				Semi[1][i] = 1;
			else
				if(pCount[1][i] > 1)
					Double[1] += pCount[1][i]-1;
		}
	}
	// Isolany
	Isolany[0] = Isolany[1] = 0;
	j = 0;
	for(i=0;i < 7;i++)
	{
		Isolated[0][i] = 0;
		if(pCount[0][i] && j==0 && pCount[0][i+1] == 0)
		{
			Isolated[0][i]++; Isolany[0] += pCount[0][i];
		}
		j = pCount[0][i];
	}
	Isolated[0][7] = 0;
	if(j==0 && pCount[0][7])
	{
		Isolated[0][7]++;Isolany[0] += pCount[0][7];
	}

	j = 0;
	for(i=0;i < 7;i++)
	{
		Isolated[1][i] = 0;
		if(pCount[1][i] && j==0 && pCount[1][i+1] == 0)
		{
			Isolated[1][i]++;Isolany[1] += pCount[1][i];
		}
		j = pCount[1][i];
	}
	Isolated[1][7] = 0;
	if(j==0 && pCount[1][7])
	{
		Isolated[1][7]++;Isolany[1] += pCount[1][7];
	}

	int f2,c2,ant,post;
	for(i=0; i < NumWPawn;i++)
	{
		Passed[0][i] = 1;
		pos = wPawnList[i];

		f = file07(pos);
		r = rank07(pos);

		Connected[0][i] = 0;
		if(r > 0)
		{
			if(board[pos-1] == wPawn || board[pos-17] == wPawn)
				Connected[0][i] = 1;
		}
		if(r < 7)
		{
			if(board[pos+1] == wPawn || board[pos-15] == wPawn)
				Connected[0][i] = 1;
		}

		ant = f-1;
		post = f+1;
		for(j=0; j < NumBPawn;j++)
		{
			pos = bPawnList[j];
			f2 = file07(pos);
			c2 = rank07(pos);
			if(f2 == ant || f2 == f || f2 == post)
			{
				if(c2 > r) // this is not passed
				{
					Passed[0][i] = 0;
					break;
				}
			}
		}
	}

	for(i=0; i < NumBPawn;i++)
	{
		Passed[1][i] = 1;
		pos = bPawnList[i];
		f = file07(pos);
		r = rank07(pos);
		Connected[1][i] = 0;
		if(r > 0)
		{
			if(board[pos-1] == bPawn || board[pos+15] == bPawn)
				Connected[1][i] = 1;
		}
		if(r < 7)
		{
			if(board[pos+1] == bPawn || board[pos+17] == bPawn)
				Connected[1][i] = 1;
		}

		ant = f-1;
		post = f+1;
		for(j=0; j < NumWPawn;j++)
		{
			pos = wPawnList[j];
			f2 = file07(pos);
			c2 = rank07(pos);
			if(f2 == ant || f2 == f || f2 == post)
			{
				if(c2 < r) // this is not passed
				{
					Passed[1][i] = 0;
					break;
				}
			}
		}
	}

	Backward();

	// for each pawn
	int sq64;
	for(i=0; i < NumWPawn;i++)
	{
		// PST
		pos = wPawnList[i];
		sq64 = sq16x12to64(pos);
		r = rank07(pos);
		// es pasado
		if(Passed[0][i])
		{
			ValueEG += PassedWeightEG[r];
			Value += PassedWeight[r];
		}
		else
		{
			f = file07(pos);
			if(Isolated[0][f])
			{
				ValueEG += Parameters[_IsolatedEG1+r];
				Value += Parameters[_Isolated1+r];
			}
			else
			if(Connected[0][i])
			{
				ValueEG += Parameters[_ConnectedEG1+r];
				Value += Parameters[_Connected1+r];
			}
		}
	}

	for(i=0; i < NumBPawn;i++)
	{
		// PST
		pos = bPawnList[i];
		sq64 = sq16x12to64(pos);
		sq64 = sq64^070;
		r = rank07(pos);
		r = 7-r;
		// es pasado
		if(Passed[1][i])
		{
			ValueEG -= PassedWeightEG[r];
			Value -= PassedWeight[r];
		}
		else
		{
			f = file07(pos);
			if(Isolated[1][f])
			{
				ValueEG -= Parameters[_IsolatedEG1+r];
				Value -= Parameters[_Isolated1+r];
			}
			else
			if(Connected[1][i])
			{
				ValueEG -= Parameters[_ConnectedEG1+r];
				Value -= Parameters[_Connected1+r];
			}
		}
	}
	switch(Double[0])
	{
	case 1:
		ValueEG += Parameters[_DoubledEG1];
		Value += Parameters[_Doubled1];
		break;
	case 2:
		ValueEG += Parameters[_DoubledEG2];
		Value += Parameters[_Doubled2];
		break;
	case 3:
		ValueEG += Parameters[_DoubledEG3];
		Value += Parameters[_Doubled3];
		break;
	case 4:
		ValueEG += Parameters[_DoubledEG4];
		Value += Parameters[_Doubled4];
		break;
	}
	switch(Double[1])
	{
	case 1:
		ValueEG -= Parameters[_DoubledEG1];
		Value -= Parameters[_Doubled1];
		break;
	case 2:
		ValueEG -= Parameters[_DoubledEG2];
		Value -= Parameters[_Doubled2];
		break;
	case 3:
		ValueEG -= Parameters[_DoubledEG3];
		Value -= Parameters[_Doubled3];
		break;
	case 4:
		ValueEG -= Parameters[_DoubledEG4];
		Value -= Parameters[_Doubled4];
		break;
	}

	PawnEval[0] += Value;
	PawnEval[1] += ValueEG;

	return 0;
}


void Board::Backward()
{

	int i,pos;
	int Opening,EndGame;
	Opening = EndGame = 0;
	if( NumWPawn > 1 && NumBPawn > 1)// && fase < 20)
	{
		// peones retrasados
		for(i=0; i < NumWPawn;i++)
		{
			// un peon no pasado o atacado por otro peon
			// no defendido por otro peon
			// casilla libre atacada por un peon contrario y no defendida por uno propio
//			if(!Passed[0][i])
			{
				pos = wPawnList[i];
				// no atacado por otro peon
				if(board[pos+15] != bPawn && board[pos+17] != bPawn)
				{
					// no defendido por otro peon
					if(board[pos-15] != wPawn && board[pos-17] != wPawn)
					{
						// casilla libre atacada por un peon contrario y no defendida por uno propio
						if(board[pos-1] != wPawn && board[pos+1] != wPawn)
						{
							if(board[pos+31] == bPawn || board[pos+33] == bPawn)
							{
								Opening -= Parameters[_BackPawn];
								EndGame -= Parameters[_BackPawnEG];
							}
						}
					}
				}
			}
		}
		// ahora peones retrasados del negro
		// peones retrasados
		for(i=0; i < NumBPawn;i++)
		{
			// un peon no pasado o atacado por otro peon
			// no defendido por otro peon
			// casilla libre atacada por un peon contrario y no defendida por uno propio
//			if(!Passed[1][i])
			{
				pos = bPawnList[i];
				// no atacado por otro peon
				if(board[pos-15] != wPawn && board[pos-17] != wPawn)
				{
					// no defendido por otro peon
					if(board[pos+15] != bPawn && board[pos+17] != bPawn)
					{
						// casilla libre atacada por un peon contrario y no defendida por uno propio
						if(board[pos-1] != bPawn && board[pos+1] != bPawn)
						{
							if(board[pos-31] == wPawn || board[pos-33] == wPawn)
							{
								Opening += Parameters[_BackPawn];
								EndGame += Parameters[_BackPawnEG]; 
							}
						}
					}
				}
			}
		}
	}
	PawnEval[0] += Opening;
	PawnEval[1] += EndGame;
}

// Devel: 35 elo
void Board::Development()
{
	int bd,nd;
	bd = nd = -10;
	DevelopmentEval[0] = DevelopmentEval[1] = 0;
	if(isEndGame) return;
	// 1 point side on move
	if(wtm == White) bd++;
	else nd++;

	//
	// a) 1 point for each piece  that is in a good developped position 
	//		(including those that have not moved yet).
	//		a rook is considered developped if the pawn of its own side in front of it 
	//		is advanced al least to the 5th rank or gone altogether.
	
	// cada pieza que no esta en el tablero esta desarrollada
	bd += 7 - (NumWRook+NumWKnight+NumWBishop+ NumWQueen);
	nd += 7 - (NumBRook+NumBKnight+NumBBishop+ NumBQueen);
	//
	// tomamos el pst de la pieza calculamos la media si en su pos está por encima de la media es ok
	// AveragePst
	//bd += PieceDevelopped(Board,caballo,blanco);
	//bd += PieceDevelopped(Board,alfil,blanco);
	//bd += PieceDevelopped(Board,torre,blanco);
	//bd += PieceDevelopped(Board,dama,blanco);

	//nd += PieceDevelopped(Board,caballo,negro);
	//nd += PieceDevelopped(Board,alfil,negro);
	//nd += PieceDevelopped(Board,torre,negro);
	//nd += PieceDevelopped(Board,dama,negro);
	// minor out of 1º rank
	bd += 4;
	if(board[B1] == wKnight)
		bd--;
	if(board[G1] == wKnight)
		bd--;
	if(board[C1] == wBishop)
		bd--;
	if(board[F1] == wBishop)
		bd--;
	nd += 4;
	if(board[B8] == bKnight)
		nd--;
	if(board[G8] == bKnight)
		nd--;
	if(board[C8] == bBishop)
		nd--;
	if(board[F8] == bBishop)
		nd--;

	// b) 1 point for having the king in a safe position.
	//		castling get 1 point.
	//
	if(board[G1] == wKing || board[H1] == wKing)
	{
		if(board[G2] == wPawn && (board[H2] == wPawn || board[H3] == wPawn))
			bd++;
	}
	else
	{
		if(board[A1] == wKing || board[B1] == wKing)
		{
			if(board[B2] == wPawn && (board[A2] == wPawn || board[A3] == wPawn))
				bd++;
		}
	}
	if(board[G8] == bKing || board[H8] == bKing)
	{
		if(board[G7] == bPawn && (board[H7] == bPawn || board[H6] == bPawn))
			nd++;
	}
	else
	{
		if(board[A8] == bKing || board[B8] == bKing)
		{
			if(board[B7] == bPawn && (board[A7] == bPawn || board[A6] == bPawn))
				nd++;
		}
	}

	//
	// c) 1 for each of the two centre pawns advanced beyond their original square
	//
	if(board[E2] != wPawn)
		bd++;
	if(board[D2] != wPawn)
		bd++;
	if(board[E7] != bPawn)
		nd++;
	if(board[D7] != bPawn)
		nd++;
	// ATG penalty for a a blocked pawn on original square
	if(board[E2] == wPawn && board[E3] != Empty)
		bd--;
	if(board[D2] == wPawn && board[D3] != Empty)
		bd--;
	if(board[E7] == bPawn && board[E6] != Empty)
		nd--;
	if(board[D7] == bPawn && board[D6] != Empty)
		nd--;

	// d) 1 extra point for a knight in the centre, as long as it cannot be driven
	//		away, and the player has already developed over half his pieces.


	// calibramos

	if(bd > 0) bd = 0;
	if(nd > 0) nd = 0;
	int factor;

	DevelopmentEval[0] = DevelopmentEval[1] = 0;

	if(bd > nd )
	{
		factor = Parameters[DevelopFactor] + Parameters[DevelopAdvance] * bd;
		if(factor > 0)
			DevelopmentEval[0] = factor * (bd-nd);
		factor = Parameters[DevelopFactorEG] + Parameters[DevelopAdvanceEG] * bd;
		if(factor > 0)
			DevelopmentEval[1] = factor * (bd-nd);
	}
	if(nd > bd )
	{
		factor = Parameters[DevelopFactor] + Parameters[DevelopAdvance] * nd;
		if(factor > 0)
			DevelopmentEval[0] = factor * (bd-nd);
		factor = Parameters[DevelopFactorEG] + Parameters[DevelopAdvanceEG] * nd;
		if(factor > 0)
			DevelopmentEval[1] = factor * (bd-nd);
	}
}



void Board::EvalKnight()
{
	KnightValue[0] = KnightValue[1] = 0;
	if((NumWKnight+NumBKnight) == 0) return; // si no hay caballos no perdamos tiempo.
	if(NumWKnight != 1 || NumBKnight != 1) return; // solo evaluamos la prob con 1 caballo por bando

	int Value = 0;
	// casillas alcanzables por el caballo no ocupadas por piezas propias y no atacadas por peones contrarios
	int lOpening,lEndGame;
	lOpening = lEndGame = 0;
	// buscamos la casilla donde se ubica el caballo
	int i,j,sq,sq64;
	const int * inc_ptr;
   int inc;
   int to,capture;


	for(i=0; i < NumWKnight;i++)
	{
		sq = wKnightList[i];
		sq64 = sq16x12to64(sq);
		// mobility
		j = 0;
		for (inc_ptr = &KnightDir[0]; (inc=*inc_ptr) != Empty; inc_ptr++) {
            to = sq + inc;
		}

		lOpening += j * Parameters[_MOBKNIGHT]; 
		lEndGame += j * Parameters[_MOBKNIGHTEG]; 
	}
	for(i=0; i < NumBKnight;i++)
	{
		sq = bKnightList[i];
		sq64 = sq16x12to64(sq);	
		sq64 = sq64 ^070;
		// mobility
		j = 0;
		for (inc_ptr = &KnightDir[0]; (inc=*inc_ptr) != Empty; inc_ptr++) {
            to = sq + inc;
		}

		lOpening -= j * Parameters[_MOBKNIGHT]; 
		lEndGame -= j * Parameters[_MOBKNIGHTEG]; 
	}


	KnightValue[0] += lOpening;
	KnightValue[1] += lEndGame;
}

int Board::RookPin()
{
	const int _DiscoverKnight = 50;
	const int _DiscoverRook = 10;  // torres comunicadas
	const int _DiscoverKing = 10;
	const int _PinKnight = 15;
	const int _PinBishop = 15;
	const int _PinRook = 15;
	const int _PinQueen = 200;

	int Opening;
	int i,from,to,capture,inc,j;
	bool CanPin = false;
	int delta;
	Opening = 0;
	if((NumWRook + NumBRook) > 0)
	{
		j = 0;
	   for(i=0; i < NumWRook;i++)
	   {
			from = wRookList[i];
			// puede ser una clavada o descubierta ?
			delta = bKingPos - from;
			CanPin = CanAttack(wRook,delta);
			if(CanPin)
			{
				int PosBloquer;
				inc = AttackDir(delta);
				j = 0;
				for (to = from+inc; (capture=board[to]) != bKing && board[to] != Edge ; to += inc) {
					if(capture != Empty)
					{
						PosBloquer = to;
						j++;
					}
				}
				if(j==1)
				{
					// is pin or discover
					switch(board[PosBloquer])
					{
					case wPawn:
						// can I move this pawn
						break;
					case wKnight:
						Opening += _DiscoverKnight;
						break;
					case wBishop:
						// this make sense ?
						break;
					case wRook:
						Opening += _DiscoverRook;
						break;
					case wQueen:
						// this make sense ?
						break;
					case wKing:
						Opening += _DiscoverKing;
						break;
					case bPawn:
						// can I move this pawn
						break;
					case bKnight:
						Opening += _PinKnight;
						break;
					case bBishop:
						Opening += _PinBishop;
						break;
					case bRook:
						Opening += _PinRook;
						break;
					case bQueen:
						Opening += _PinQueen;
						break;
					case bKing:
						// this make sense ?
						break;
					}
				}
			}
	   }
	   for(i=0; i < NumBRook;i++)
	   {
			from = bRookList[i];
			// puede ser una clavada o descubierta ?
			delta = wKingPos - from;
			CanPin = CanAttack(bRook,delta);
			if(CanPin)
			{
				int PosBloquer;
				inc = AttackDir(delta);
				j = 0;
				for (to = from+inc; (capture=board[to]) != wKing && board[to] != Edge ; to += inc) {
					if(capture != Empty)
					{
						PosBloquer = to;
						j++;
					}
				}
				if(j==1)
				{
					// is pin or discover
					switch(board[PosBloquer])
					{
					case wPawn:
						// can I move this pawn
						break;
					case wKnight:
						Opening -= _PinKnight;
						break;
					case wBishop:
						Opening -= _PinBishop;
						break;
					case wRook:
						Opening -= _PinRook;
						break;
					case wQueen:
						Opening -= _PinQueen;
						break;
					case wKing:
						// this make sense ?
						break;
					case bPawn:
						// can I move this pawn
						break;
					case bKnight:
						Opening -= _DiscoverKnight;
						break;
					case bBishop:
						// this make sense ?
						break;
					case bRook:
						Opening -= _DiscoverRook;
						break;
					case bQueen:
						// this make sense ?
						break;
					case bKing:
						Opening -= _DiscoverKing;
						break;
					}
				}
			}
	   }
	}

	return Opening;
}
void Board::BishopEvaluation()
{
	int Opening,EndGame;
	int i,from,to,capture,inc,j,sq64;
	const int * inc_ptr;

	Opening = EndGame = 0; 
	EvalBishop[0] = EvalBishop[1] = 0;
	if((NumWBishop + NumBBishop) > 0)
	{
		// Mobility
		j = 0;
	   for(i=0; i < NumWBishop;i++)
	   {
			from = wBishopList[i];
			sq64 = sq16x12to64(from);


			ASSERT(board[from] == wBishop);
			for (inc_ptr = &BishopDir[0]; (inc=*inc_ptr) != Empty; inc_ptr++) {
				for (to = from+inc; (capture=board[to]) == Empty; to += inc) {
					j++;
				}
			}
	   }
	   Opening += j * Parameters[BishopMobility];
	   EndGame += j * Parameters[BishopMobilityEG];
		j = 0;
	   for(i=0; i < NumBBishop;i++)
	   {
			from = bBishopList[i];
			sq64 = sq16x12to64(from) ^070;
			ASSERT(board[from] == bBishop);
			for (inc_ptr = &BishopDir[0]; (inc=*inc_ptr) != Empty; inc_ptr++) {
				for (to = from+inc; (capture=board[to]) == Empty; to += inc) {
					j++;
				}
			}
	   }
	   Opening -= j * Parameters[BishopMobility];
	   EndGame -= j * Parameters[BishopMobilityEG];
	}
	EvalBishop[0] += Opening;
	EvalBishop[1] += EndGame;
}

void Board::RookEvaluation()
{
	EvalRook[0] = EvalRook[1] = 0;
	if((NumWRook + NumBRook) == 0)
		return;

	int Opening,EndGame;
	Opening = EndGame = 0; 

	int from, to;
	int capture;
	const int * inc_ptr;
	int inc;
	int i,j;
	j = 0;
	for(i=0; i < NumWRook;i++)
	{
		from = wRookList[i];
		j = file07(from);
		// Open
		if(Open[j])
		{
			Opening += Parameters[EvOpenFile];
			EndGame += Parameters[EvOpenFileEG];
		}
		else
		// semiOpen
		if(Semi[0][j])
		{
			Opening += Parameters[SemiOpenFile];
			EndGame += Parameters[SemiOpenFileEG];
		}
		else
		{
			if(from == H1 && (board[G1] == wKing || board[F1] == wKing))
			{
				Opening -= Parameters[RookTrap];
				EndGame -= Parameters[RookTrapEG];
			}
			if(from == A1 && (board[B1] == wKing || board[C1] == wKing))
			{
				Opening -= Parameters[RookTrap];
				EndGame -= Parameters[RookTrapEG];
			}

		}
		// 7º
		j = rank07(from);
		if(j==6)
		{
			Opening += Parameters[SevenRank];
			EndGame += Parameters[SevenRankEG];
		}

		j = 0;
		ASSERT(board[from] == wRook);
			for (inc_ptr = &RookDir[0]; (inc=*inc_ptr) != Empty; inc_ptr++) {
				for (to = from+inc; (capture=board[to]) == Empty; to += inc) {
				j++;
				}
				if ((capture&3) == Black) j++;
			}
			Opening += j * Parameters[RookMob];
			EndGame += j * Parameters[RookMobEG];
	}

	j = 0;
	for(i=0; i < NumBRook;i++)
	{
		from = bRookList[i];
		j = file07(from);
		// Open
		if(Open[j])
		{
			Opening -= Parameters[EvOpenFile];
			EndGame -= Parameters[EvOpenFileEG];
		}
		else
		// semiOpen
		if(Semi[1][j])
		{
			Opening -= Parameters[SemiOpenFile];
			EndGame -= Parameters[SemiOpenFileEG];
		}
		else
		{
			if(from == H8 && (board[G8] == bKing || board[F8] == bKing))
			{
				Opening += Parameters[RookTrap];
				EndGame += Parameters[RookTrapEG];
			}
			if(from == A8 && (board[B8] == bKing || board[C8] == bKing))
			{
				Opening += Parameters[RookTrap];
				EndGame += Parameters[RookTrapEG];
			}

		}
		// 7º
		j = rank07(from);
		if(j==1)
		{
			Opening -= Parameters[SevenRank];
			EndGame -= Parameters[SevenRankEG];
		}
		

		j = 0;
		ASSERT(board[from] == bRook);
		for (inc_ptr = &RookDir[0]; (inc=*inc_ptr) != Empty; inc_ptr++) {
			for (to = from+inc; (capture=board[to]) == Empty; to += inc) {
			j++;
			}
			if ((capture&3) == White) j++;
		}
		Opening -= j * Parameters[RookMob];
		EndGame -= j * Parameters[RookMobEG];
	}


	EvalRook[0] = Opening;
	EvalRook[1] = EndGame;
}

void Board::QueenEvaluation()
{
   int Opening, EndGame;
   int from, to,sq64;
   int capture;
   const int * inc_ptr;
   int inc;
   int i,j;
	Opening = EndGame = 0;
	EvalQueen[0] = EvalQueen[1] = 0;
//	if(NumWQueen != 1 || NumBQueen != 1) return;

	j = 0;
	for(i=0; i < NumWQueen;i++)
	{
		from = wQueenList[i];
		sq64 = sq16x12to64(from);

		ASSERT(board[from] == wQueen);
		for (inc_ptr = &QueenDir[0]; (inc=*inc_ptr) != Empty; inc_ptr++) {
			for (to = from+inc; (capture=board[to]) == Empty; to += inc) {
				j++;
			}
			if ((capture&3) == Black) j++;
		}
	}
	Opening += j * Parameters[QueenMob];
	EndGame += j * Parameters[QueenMobEG];
	j = 0;
	for(i=0; i < NumBQueen;i++)
	{
		from = bQueenList[i];
		sq64 = sq16x12to64(from) ^070;
		ASSERT(board[from] == bQueen);
		for (inc_ptr = &QueenDir[0]; (inc=*inc_ptr) != Empty; inc_ptr++) {
			for (to = from+inc; (capture=board[to]) == Empty; to += inc) {
				j++;
			}
			if ((capture&3) == White) j++;
		}
	}
	Opening -= j * Parameters[QueenMob];
	EndGame -= j * Parameters[QueenMobEG];
	EvalQueen[0] = Opening;
	EvalQueen[1] = EndGame;
}


void Board::PawnCoverage()
{
	int Value = 0;
	PawnCover[0] = PawnCover[1] = 0;
	if(isEndGame) return;

	// value from 0 to 8
	// 1 2 1   |2 2
	// 2 4 2   |4 4
	//   K     |K

const int MinCov = 10;
static const int Cov4 = 4;
static const int Cov2 = 2;
static const int Cov1 = 1;

	int sq,file;

	sq = wKingPos;

	// king on open file
	file = file07(sq);
	if(Open[file] || Semi[0][file])
	{
		if((NumBRook + NumBQueen) > 0)
		{
			PawnCover[0] -= Parameters[KingOpen];
			PawnCover[1] -= Parameters[KingOpenEG];
		}
	}
	if(rank07(sq) < 3)
	{
		if(file == 0)
		{
			if(board[sq+16] == wPawn)				Value += Cov4;
			if(board[sq+17] == wPawn)				Value += Cov4;
			if(board[sq+32] == wPawn)				Value += Cov2;
			if(board[sq+33] == wPawn)				Value += Cov2;
		}
		else
		{
			if(file == 7)
			{
				if(board[sq+16] == wPawn)					Value += Cov4;
				if(board[sq+15] == wPawn)					Value += Cov4;
				if(board[sq+32] == wPawn)					Value += Cov2;
				if(board[sq+31] == wPawn)					Value += Cov2;
			}
			else
			{
				if(board[sq+16] == wPawn)					Value += Cov4;
				if(board[sq+15] == wPawn)					Value += Cov2;
				if(board[sq+17] == wPawn)					Value += Cov2;
				if(board[sq+32] == wPawn)					Value += Cov2;
				if(board[sq+31] == wPawn)					Value += Cov1;
				if(board[sq+33] == wPawn)					Value += Cov1;
			}
		}
	}
	Value -= MinCov; // 0->8 => -8 -> 0
	if(Value > 0) Value = 0;
	PawnCover[0] += Value * Parameters[PawnCov];
	PawnCover[1] += Value * Parameters[PawnCovEG];

	Value = 0;
	sq = bKingPos;

	// king on open file
	file = file07(sq);
	if(Open[file] || Semi[1][file])
	{
		if((NumWRook + NumWQueen) > 0)
		{
			PawnCover[0] += Parameters[KingOpen];
			PawnCover[1] += Parameters[KingOpenEG];
		}
	}
	if(rank07(sq) > 4)
	{
		if(file == 0)
		{
			if(board[sq-16] == bPawn)				Value += Cov4;
			if(board[sq-15] == bPawn)				Value += Cov4;
			if(board[sq-32] == bPawn)				Value += Cov2;
			if(board[sq-31] == bPawn)				Value += Cov2;
		}
		else
		{
			if(file == 7)
			{
				if(board[sq-16] == bPawn)					Value += Cov4;
				if(board[sq-17] == bPawn)					Value += Cov4;
				if(board[sq-32] == bPawn)					Value += Cov2;
				if(board[sq-33] == bPawn)					Value += Cov2;
			}
			else
			{
				if(board[sq-16] == bPawn)					Value += Cov4;
				if(board[sq-15] == bPawn)					Value += Cov2;
				if(board[sq-17] == bPawn)					Value += Cov2;
				if(board[sq-32] == bPawn)					Value += Cov2;
				if(board[sq-31] == bPawn)					Value += Cov1;
				if(board[sq-33] == bPawn)					Value += Cov1;
			}
		}
	}
	Value -= MinCov; // 0->8 => -8 -> 0
	if(Value > 0) Value = 0;
	PawnCover[0] -= Value * Parameters[PawnCov];
	PawnCover[1] -= Value * Parameters[PawnCovEG];
}


void Board::DisplayScores()
{
	printf("Material %d %d\n",Material[0],Material[1]);
	printf("Pst %d %d\n",Opening,EndGame);
	printf("Development %d %d\n",DevelopmentEval[0],DevelopmentEval[1]);
	printf("Knights %d %d\n",KnightValue[0],KnightValue[1]);
	printf("Bishops %d %d\n",EvalBishop[0],EvalBishop[1]);
	printf("Rooks %d %d\n",EvalRook[0],EvalRook[1]);
	printf("Queens %d %d\n",EvalQueen[0],EvalQueen[1]);
	printf("PawnCover %d %d\n",PawnCover[0],PawnCover[1]);

}


//////////////////////////////////////////////////////////////////////////////////////////////////
// Evaluation with static exchange
//////////////////////////////////////////////////////////////////////////////////////////////////
int Board::Eval2()
{
	// Por ahora
	GenAttacks();
	int opening,endgame;
	int Value = 0;

	EvalMaterial();
	opening = Material[0];
	endgame = Material[1];

	// PST
	opening += Opening;
	endgame += EndGame;
	
	PawnEvaluation();
	opening += PawnEval[0];
	endgame += PawnEval[1];

	Development();  // +332-329=339
	opening += DevelopmentEval[0];
	endgame += DevelopmentEval[1];

	EvalKnight();
	opening += KnightValue[0];
	endgame += KnightValue[1];

	BishopEvaluation();
	opening += EvalBishop[0];
	endgame += EvalBishop[1];

	RookEvaluation();
	opening += EvalRook[0];
	endgame += EvalRook[1];

	QueenEvaluation();
	opening += EvalQueen[0];
	endgame += EvalQueen[1];

	PawnCoverage();
	opening += PawnCover[0];
	endgame += PawnCover[1];

	 //Tappered
	int Value0 = ((opening *(phase) + endgame *(64-phase)) / 64);

	return Value0;	
}

const int ValueCM[6] = {100,320,330,500,900,30000};
const int MAXVALUE = 32000;
int Peso(int cual)
{
	switch(cual)
	{
	case 0:
	case 1:
	case 2:
	case 3:
	case 4:
	case 5:
	case 6:
	case 7:
		return 100;
	break;
	case 8:
	case 9:
		return 320;
	break;
	case 10:
	case 11:
		return 330;
	break;
	case 12:
	case 13:
		return 500;
	break;
	case 14:
		return 900;
	break;
	case 15:
		return 30000;
	break;
	}
	return 0;
}

int See(int a,int b,int value)
{
	int Value = 0;
	// suponemos a empieza.
	int i,move,WeightMove;
	int ret;
	for(i=0; i < 16;i++)
	{

		if(a & (1<<i))
		{

			// probamos esta.
			WeightMove = Peso(i);

			// make move
			a -= (1<<i);

			ret = value-See(b,a,WeightMove);

			// Takeback
			a += (1<<i);

			if(ret > Value)
			{
				Value = ret;
				break;
			}
		}
	} 
	if(Value < 0) return 0;
	return Value;
}
