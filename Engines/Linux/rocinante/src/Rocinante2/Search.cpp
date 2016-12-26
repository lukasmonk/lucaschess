#include <stdlib.h>
#include "CDebug.h"
#include "MoveList.h"
#include "UndoData.h"
#include "Board.h"
#include "HashJugadas.h"
#include "system.h"
#include <string.h>

const int DepthReds = 1;
const int SafeLegalRed = 5; 


void Board::InitTT(int size)
{
	CHashJugadas *aux = new CHashJugadas();
	TT = aux;
	aux->SetHashSize(size);
}

const int ProbeDepthD = 1;
int Board::Search()
{
	int Value;
	ResetHashStack();
	InBStar = 1;
	if(ProbeDepth > 3)
	{
		LimiteProfundidad = ProbeDepth;
		tiempo_limite = 0;
		Value = IterativeDeepening();
	}
	else
	{
		if(ProbeDepth == -1)
			Value = -SearchNegaM();
		else
		if(ProbeDepth == 0)
			Value = -Selective(-MATE-1, MATE+1);
		else
			Value = -AlphaBeta(ProbeDepth,-MATE-1, MATE+1);
	}
	InBStar = 0;
	return Value;
}

int Board::AlphaBeta(int depth,int alpha,int beta)
{
	int value,a;
	int BestMove;
	int legals = 0;
	UndoData undo;
	MoveList List;
	int Move; //,ScoreMove;
	int i;

	a = alpha;
	TotalNodes++;
	BestMove = -1;


	SetHashStack(hash);

	GenPseudoMoves(List);
	// Weight moves
	WeightMoves(List);
	// Sort List Moves
	List.Sort();

	a = alpha;legals = 0;
	for(i = 0; i < List.Length();i++)
	{
		Move = List.Move(i);
		if(IsKingCapture(Move))
		{
			PopHashStack();	
			return (MATE);
		}

		DoMove(Move,undo);
		// Test legality
		value = -MATE;
		if(IsLegal())
		{
			legals++;
			if(IsCheck())
				value = -AlphaBeta(depth,-beta,-a);
			else
				if(depth > 1)
					value = -AlphaBeta(depth-1,-beta,-a);
				else
					value = -Selective(-beta, -a);
		}
		UndoMove(Move,undo);
		if(value > a) 
		{
			a = value;
			BestMove = Move;
			if(a >= beta ) 
			{
				break;
			}
		}	
	} // for
	BestMoveSearch = BestMove;
	PopHashStack();
	if(legals == 0)
	{
		if(IsCheck())
			return -MATE;
		else
			return 0;
	}
	return ( a );   
}


int Board::Optimism()
{
	return -Search();
}


int Board::Selective(int alpha,int beta)
{
	int value,a;
	int Valor_i;
	int legals = 0;
	if(alpha >= beta) return alpha;

	TotalNodes++;

	int InCheck = this->IsCheck();

	Valor_i = GetEval(); 

	if(Repetition(hash))
		return 0;

	SetHashStack(hash);

	// Vamos con material de sobra
	if(!InCheck && Valor_i >= beta ) 	{	PopHashStack();	return Valor_i;	}

	value = a = alpha;
	if(Valor_i > alpha )		a = Valor_i;
	else a = alpha;

	UndoData undo;
	MoveList List;
	int Move,ScoreMove;
	int i;

	if(InCheck)
		GenPseudoMoves(List);
	else
		GenCaptures(List);
	// Weight moves
	WeightMoves(List);
	// Sort List Moves
	List.Sort();
	for(i = 0; i < List.Length();i++)
	{
		Move = List.Move(i);

		if(IsKingCapture(Move))
		{
			PopHashStack();	
			return (MATE);
		}
		ScoreMove = List.Value(i);
		// filter out irrelevant moves

		DoMove(Move,undo);
		// Test legality
		if(IsLegal())
		{
			legals++;
			value = -Selective(-beta, -a);
		}
		UndoMove(Move,undo);
		if(value > a) 
		{
			a = value;
			if(a >= beta ) 
			{
				break;
			}
		}	
	} // for
	PopHashStack();
	if(InCheck)
	{
		if(legals == 0)
			return (-MATE);
	}
	return ( a );   
}

static const int  MAXDEPTHC = 65;
static const int INFINITO = 999999;

int Board::IterativeDeepening(void)
{
	extern void Print(const char *fmt, ...);
	char PV[1024];
	int i;
	int IteraScores[MAXDEPTHC];
	long ini_it;
	int value,value_i;
	int hasp;
	int MateCount = 0;
	// busqueda con ventana
	int VAlpha,VBeta,Pase;

	int Valor_d1;
	char JugD1[20];

	TotalNodes = 0;
	cancelado  = 0;
	NodosRepasados = 0;
	SelDepth = 0;
	inicio = TimeElapsed();

	if(TT == NULL)
	{
		InitTT(TTSize);
	}
	ResetHashStack();


	PV[0] = '\0';
	value = -MATE-1;
	((CHashJugadas *)TT)->IncrementaEdad();

	JugadaActual[0] = '\0';
	Depth = 1;
	value_i = value = PVS(Depth,-INFINITO,+INFINITO,PV,false);
	Valor_d1 = value_i ;
	if(Valor_d1 == INFINITO || Valor_d1 == -INFINITO)
		Valor_d1 = 0;
	JugD1[0] = '\0';

	if(PV[0])
	{
		if(!InBStar)
			PrintInfo(value,PV);
		strcpy(JugadaActual,strtok(PV," "));
		strcpy(JugD1,JugadaActual);
		ParsePonder(PV);
	}
	Depth++;
	VAlpha = value-128;
	VBeta = value+128;
	Pase = 0;
	while(1)
	{
		if(cancelado)
			break;
		if(tiempo_limite && !PonderMode)
		{
			ini_it = TimeElapsed();
			if((ini_it  - inicio) > tiempo_limite) // tiempo excedido
			{
				break;
			}
		}

		if(	!InBStar && LimiteProfundidad)
			if(Depth > LimiteProfundidad)
			{
				if(LimiteProfundidad == 1)
					PrintInfo(value,PV);
				break;
			}
		if(Depth > MAXDEPTHC)
			break;

		SelDepth = 0;
		if(!InBStar && Depth > 5)
			Print("info depth %d \n",Depth);
		hasp = value;
		ResetHashStack();
		value = PVS(Depth,-INFINITO,+INFINITO,PV,false);

		if( cancelado == 1)
			break;
		value_i = value;
	

		if(!InBStar)
			PrintInfo(value,PV); // end of iteration-> get time to depth
		ParsePonder(PV);

		if(value == MATE || value == -MATE)
		{
			break;
		}

		if(PV[0])
			strcpy(JugadaActual,strtok(PV," "));
		PV[0] = '\0'; // reset de la mejor jugada
		// llevamos la cuenta de iteraciones que damos o recibimos mate
		if(value < (50-MATE) && value > -MATE)
			MateCount++;
		if(value > (MATE-50) && value < MATE)
			MateCount++;

		// a partir de 5 iteraciones de mate podemos dar el resultado por bueno
		if(tiempo_limite)
		if(MateCount >= 5)
			break;

		Depth++;
	}
	// hemos terminado de pensar
	// no salir hasta recibir el ponder hit o el stop.
	while(PonderMode) Wait();
	if(InBStar == 0)
	{
		if(ponderMove[0])
			Print("bestmove %s ponder %s\n",JugadaActual,ponderMove);
		else
			Print("bestmove %s\n",JugadaActual);
	}
	if(!cancelado && (value_i < 50-MATE || value_i > MATE-50))
	{
		char fen[75];
		SaveFEN(fen);
		fen[0] = fen[0];
	}
	return -value_i;
}

void Board::PrintInfo(int value,char * path)
{
	extern void Print(const char *fmt, ...);

	int time;
	int segs;
	int mate;
	int nps = 0;

	if(Depth < 5)
		return;

	time = TimeElapsed() - inicio;
	if(time == 0)
		time++;
	segs = time / 1000;
	if(segs == 0)
		segs++;
	if(TotalNodes < 2000000)
		nps = TotalNodes*1000/time;
	else
		nps = TotalNodes / segs;

	mate = 0;
	if(value >= MATE -50)
	{
		mate = MATE -value;
	}
	if(value <= -MATE +50)
	{
		mate = MATE +value;
		mate = -mate;
	}
	if(!mate)
	{
		Print("info depth %d seldepth %d pv %s score cp %d nodes %d nps %d hashfull %d time %ld\n",
			Depth,
			SelDepth,
			path,
			value,
			TotalNodes,
			nps,
			0,	// numero de veces que se recalcula
			time);
	}
	else
	{
		Print("info depth %d seldepth %d pv %s score mate %d nodes %d nps %d hashfull %d time %ld\n",
			Depth,
			SelDepth,
			path,
			mate/2,
			TotalNodes,
			nps,
			0,	
			time);
	}
}
void Board::ParsePonder(char *pv)
{
	char *paux = &ponderMove[0];
	ponderMove[0] = '\0';
	char *aux = pv; //strstr(pv," ");
	paux = JugadaActual;
	if(aux)
	{
		while(*aux && *aux != ' ')
		{
			*paux++ = *aux++;
		}
		*paux  = '\0';
	}

	paux = &ponderMove[0];
	if(aux)
	{
		aux++;
		while(*aux && *aux != ' ')
		{
			*paux++ = *aux++;
		}
		*paux  = '\0';
	}
}

static const int NMRED = 4;
static const int DepthCache = 1; //3;

int Board::PVS(int depth, int alpha, int beta,char *Global,int doNull)
{
	int ext_local; 
	u64 hash;
	char PV[1024];
	int value = 0;
	int EsJaque,legales = 0;
	int fFoundPv = false;
	int Valor_i = 0;
	int NotInPV = (alpha+1) == beta;
	IncNodes();
	if(cancelado)
		return alpha;

	// Distance Pruning
	// limite inferior
	value = ValorMate(-1);
	value += 2;
	if(value > alpha)
	{
		alpha = value;
		if(value >= beta)
			return value;
	}
	// limite superior
	value = ValorMate(1);
	value--;
	if(value < beta)
	{
		beta = value;
		if(value <= alpha)
			return value;
	}

	hash = GetHash();
	if(stHistory > 0 && Repetition(hash) ) // Queremos evitar repeticiones pero no en raiz ya que no jugariamos
	{		return 0;	}
	if((EsJaque = IsCheck()) != 0		)
	{
			depth++;
	}

	if((depth <= 0 || stHistory > MAXDEPTHC)) // extendiendo jaques agotamos la pila
	{
		TotalNodes--; // correccion para no contarlos dos veces

		value = Selective(alpha,beta);//,Global); Quiesce
#ifdef _DEBUG
		if(value <= -MATE || value >= MATE)
		{
			value = value;
		}
#endif
		return value;
	}

/*
 ************************************************************
 *   Recuperamos el valor del cache                         *
 *                                                          *
 ************************************************************
 */
	int CacheValue = 0;
	int CacheMove = 0;
	((CHashJugadas *)TT)->GetDataCache(hash,CacheValue,CacheMove);
	// si estamos buscando la variante principal 
	// queremos un valor encontrado, no algo difuso de la tt.
	if(NotInPV && CacheValue != MATE )
	{
		if( !EsJaque )
		{
			int CV = CacheValue; 
			if(CV)
			{
				value = CV & VALUEMASK; // valor
				if(value > MATE)
				{
					value |= 0xffff0000;
				}
				// verificamos la profundidad en cache
				if((CV >> 20) >= (depth) )
				{
					// el valor en cache significa algo
					if(CV & UBOUND)
					{
						if(CV & LBOUND)		
						{		
							if(value >= beta) return beta;
							if(value <= alpha) return alpha;
							return(value);		
						}
						else
						{
							// solo UBOUND
							if(value <= alpha )		{	
								return alpha;	
							}
							if(value < beta)	beta = value;
						}
					}
					else
					{
						if((value) >= beta)	{		
							return beta; 		
						}
						if(value > alpha)	{		alpha = value;		}
					}
				}
			}
		}
	} // UseCache

	// Null move 
	if( doNull &&	depth >= 2 && 	!EsJaque &&	NotInPV 
		&& 	TotalPCtm() > 0	
		) 
	{
		int d = depth -NMRED;
		if(d < 0) d = 0;
		UndoData undo;
		DoNullMove(undo);
		PV[0] = '\0';
		value = -PVS(d, -beta,-beta+1,PV,false);
		DoNullMove(undo);
		if (value >= beta)
		{
			value = beta;
			CACHE(hash,CacheMove,beta,depth,LBOUND);
#ifdef _DEBUG
			strcpy(Global," NULLMOVE ");
			strcat(Global,PV);
#endif
			return value;
		}
	}

	if(CacheMove == 0 && depth > 6) // IID
	{
		PVS(depth/2, -beta,-beta+1,PV,false);
		((CHashJugadas *)TT)->GetDataCache(hash,CacheValue,CacheMove);
	} // IID
	
	HashJugada = CacheMove; 
	
	UndoData undo;
	MoveList List;
	int Move,ScoreMove;
	int i;

	SetHashStack(hash);

	// se tiene que hacer en el mismo nivel stHistory por lo que
	// debe ir tras el set hashHistory ...
	for(int k = 0; k < MAXKILLERS;k++)
		Killer[k] = ((CHashJugadas *)TT)->GetKiller(stHistory,wtm,k);

	if(cancelado) 
	{
		PopHashStack();
		return alpha;
	}

	GenPseudoMoves(List);
	WeightMovesPVS(List);
	// Sort List Moves
	List.Sort();

	value = alpha;

	int Piezas = TotalPCtm();

	int DoLMR = false;
	if(
		!EsJaque 
		&& depth > 1 
		&& Piezas > 1 
		&& depth > DepthReds
	)
	{
		DoLMR = true;	
	}

	int MejorJ;

	bool DoFutil = false;
	static const int depthFutil = 6; // 6
	if(	depth < depthFutil && !EsJaque && NotInPV 
		)
	{
		const int Margin[8] = {0,60,80,200,300,300,300,300};
		Valor_i = GetMaterial(); 
		if((Valor_i + Margin[depth]) < alpha)
			DoFutil = true;
	}

	MejorJ = 0;	PV[0] = '\0';
	for(i = 0; i < List.Length();i++)
	{
		Move = List.Move(i);
		ScoreMove = List.Value(i);

		if(cancelado)
			break;
		ext_local = 0;

		if(!EsJaque) // ya hemos extendido
		{
			//if(	Taux.Es7a(J))
			//	ext_local = 1;
			if(DoLMR && ext_local == 0  && legales > SafeLegalRed) 
			{
				if(ScoreMove == 0)
						ext_local = -1;
			}
			if(ext_local == 0 && DoFutil && legales > 1 //SafeLegalRed //1 
				)
			{
				if(ScoreMove == 0)
					continue;
			}
		}
		DoMove(Move,undo);
		if(IsLegal())
		{
			legales++;
			PV[0] = '\0';
			if (fFoundPv) {
				value = -PVS(depth + ext_local - 1, -alpha-1, -alpha,PV,true);
				if ((value > alpha) && !cancelado) // Check for failure.
				{
					if(ext_local < 0) ext_local = 0;
					PV[0] = '\0';
					value = -PVS(depth + ext_local - 1, -beta, -alpha,PV,true);
				}
			} else
			{
				PV[0] = '\0';
				value = -PVS(depth + ext_local - 1, -beta, -alpha,PV,true);
				if (ext_local < 0 && (value > alpha) && !cancelado) // Check for failure.
				{
					PV[0] = '\0';
					value = -PVS(depth - 1, -beta, -alpha,PV,true);
				}
			}
			if(legales == 1) // For nodeall save in TT the first move.
			{
				MejorJ = Move;
			}
		}
		UndoMove(Move,undo);
		if(cancelado) 
			break;

		if(value == -INFINITO )
			continue;
        if (value >= beta)
		{
#ifdef _DEBUG
			{
				char dest[8];
				MoveToAlgebra(Move,dest);
				strcpy(Global,dest);
				strcat(Global," ");
				strcat(Global,PV);
			}
#endif
			if(ScoreMove == 0)
			{
				((CHashJugadas *)TT)->SetKiller(stHistory,wtm,Move);
			}
			CACHE(hash,Move,beta,depth,LBOUND);
			PopHashStack();
            return value; 
		}

        if (value > alpha)
		{
			if(!cancelado)
			{
				char dest[8];
				MoveToAlgebra(Move,dest);
				strcpy(Global,dest);
				strcat(Global," ");
				strcat(Global,PV);
			}
			if(!InBStar && !cancelado && Depth <=  depth && stHistory == 1)
			{
				// new PV
				PrintInfo(value,Global);
			}
			MejorJ = Move;	
            alpha = value;
			fFoundPv = true;
		}
    }
	if(legales == 0 && !cancelado)
	{
		if(EsJaque)
		{	// mate
			alpha = ValorMate(-1);
		}
		else
		{	// stalemate
			alpha = 0;
		}
	}

	PopHashStack();

	if(!cancelado ) 
	{
		if((beta-alpha) > 1)
		{
			CACHE(hash,MejorJ,alpha,depth,EXACT);
		}
		else
			CACHE(hash,MejorJ,alpha,depth,UBOUND);
	}
    return alpha;
}

void Board::IncNodes()
{
	static const int TOPENODOSREPASO  = 300;

	// incrementamos los nodos vistos
	TotalNodes++;
	if(TotalNodes-NodosRepasados > TOPENODOSREPASO)
	{		
		if(tiempo_limite && !PonderMode)
		{
			if(((TimeElapsed() - inicio) > tiempo_limite))
			{
				cancelado = 1;
			}
			else
				NodosRepasados = TotalNodes;
		}
	}
}

int Board::ValorMate(int signo)
{
	if(signo > 0)
		return( MATE - stHistory);
	else
		return ( -MATE + stHistory);
}

u64 Board::GetHash()
{
	// devolver el hash de esta posicion.
	if(!hash)
	{
		hash ^= GetZobColor(wtm); // esto puede ocurrir ?
    }
	// casilla de captura al paso
	u64 salida = hash;
	salida ^= GetZobEnPasant(EnPassant);
	// estado del enroque
	salida ^= GetZobCastle(CastleFlags);
	return salida;
}

int Board::TotalPCtm()
{
	if(wtm)
	{
		return NumWKnight+NumWBishop+NumWRook+NumWQueen;
	}
	else
	{
		return NumBKnight+NumBBishop+NumBRook+NumBQueen;
	}
}
int Board::GetMaterial()
{
	int opening,endgame;
	opening = endgame = 0;
	int Value0 = 0;
	PreEval();

	EvalMaterial();
	opening += Material[0];
	endgame += Material[1];

		// PST
	opening += Opening;
	endgame += EndGame;

	 //Tappered
	Value0 = ((opening *(phase) + endgame *(64-phase)) / 64);
	if(wtm == White)
		return Value0;
	else
		return -Value0;
}

int Board::SearchNegaM()
{
	const int PDepth = 1;
	int Value = SeeEval();
	return Value;
//	 do negamax.
//	return NegaMax(PDepth,-MATE,MATE);
}

int Board::NegaMax(int depth,int alpha,int beta)
{
	int BestValue = alpha;
	int Value;
	int i,inc,sq,legals,EsJaque,tope;
	int move;
	int from;
	int to;
	int piece;
	int nTactics;
	MoveList List;
	UndoData undo;
	legals = 0;
	if((EsJaque = IsCheck()) == 0	)
	{
		if(depth <= 0)
		{
			Value = GetEval();
			GenAttacks();
			Value += SeeBoard();
			return Value;
		}
	}
	
	GenPseudoMoves(List);
	WeightMoves(List);

	// Sort List Moves by evaluation
	List.Sort();


	//// rest by value order
	int LegalOne;
	for(i=0; i < List.Length();i++)
	{
		{
			move = List.Move(i);
			LegalOne = 0;
			DoMove(move,undo);
			if(IsLegal())
			{
				legals++;
				LegalOne = 1;
				Value = -NegaMax(depth-1,-beta,-BestValue);
			}
			// undo move
			UndoMove(move,undo); 
			// is better
			if(LegalOne && Value > BestValue)
			{
				BestValue = Value;
				if(BestValue >= beta)
					return BestValue;
			}
		}
	}
	if(legals == 0)
	{
		if(EsJaque)
			return -MATE;
		return 0;
	}
	return BestValue;
}
int Board::HandleCheck(int depth,int alpha,int beta)
{
	// we are in check.
	// generate all move do all legals
	UndoData undo;
	MoveList List;
	int Move,ScoreMove;
	int i;
	int legals = 0;

	if(IsLegal())
		return -MATE;

	GenPseudoMoves(List);
	WeightMovesPVS(List);
	// Sort List Moves
	List.Sort();
	int BestValue = -MATE;
	int Value,move,legalone;
	for(i=0; i < List.Length();i++)
	{
		move = List.Move(i);
		DoMove(move,undo);
		legalone = 0;
		if(IsLegal())
		{
			legalone = 1;
			legals++;
			// evaluate
			Value = -NegaMax(depth-1,-beta,-BestValue);
		}
		// undo move
		UndoMove(move,undo); 
		// is better
		if(legalone)
		{
			if(Value >BestValue)
				BestValue = Value;
			if(BestValue >= beta)
				return BestValue;
		}
	}
	if(legals ==0)
	{
		if(IsCheck())
			return -MATE;
		return 0;
	}
	return BestValue;
}