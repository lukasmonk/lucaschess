//  GREKO Chess Engine
//  (c) 2002-2015 Vladimir Medvedev <vrm@bk.ru>
//  http://greko.su

//  main.cpp: initialize and start engine, command line interface
//  modified: 01-Aug-2015

#include "book.h"
#include "config.h"
#include "defaults.h"
#include "eval.h"
#include "moves.h"
#include "notation.h"
#include "search.h"
#include "utils.h"

const std::string VERSION = "GreKo 12.9";
const std::string RELEASE_DATE = "01-Aug-2015";

Position g_pos;
Book g_book;

std::list<std::string> g_commandQueue;
FILE* g_log = NULL;

class Engine
{
public:
	Engine() :
		m_force(false),
		m_uciLimitStrength(false),
		m_uciElo(2600),
		m_inc(0),
		m_movesPerSession(0),
		m_movesToGo(0) {}

private:
	Search      m_search;
	bool        m_force;
	bool        m_run;
	TokenString m_line;
	bool        m_uciLimitStrength;
	int         m_uciElo;
	int         m_inc;
	int         m_movesPerSession;
	int         m_movesToGo;

	std::string GetToken()
	{
		return m_line.GetToken();
	}

	void OnBk()
	{
		std::string buf;
		g_book.GetMove(g_pos, buf);
		out("%s\n\n", buf);
	}

	void OnBook()
	{
		std::string arg = GetToken();
		if (arg == "clean")
			g_book.Clean();
		else if (arg == "import")
		{
			std::string path = GetToken();
			std::string strMaxPly = GetToken();
			std::string strColor = GetToken();
			g_book.Import(path, strMaxPly, strColor);
		}
		else if (arg == "load")
		{
			std::string path = GetToken();
			g_book.Load(path);
		}
		else if (arg == "save")
		{
			std::string path = GetToken();
			g_book.Save(path);
		}
	}

	void OnConsole()
	{
		g_protocol = CONSOLE;
	}

	void OnEpdtest()
	{
		std::string strPath = GetToken();
		std::string strSec = GetToken();
		std::string strRep = GetToken();

		FILE *psrc = fopen(strPath.c_str(), "rt");
		if (!psrc)
		{
			out("Unable to open file: %s\n", strPath);
			return;
		}
		double tm = strSec.empty()? 1.0 : atof(strSec.c_str());
		int reps = strRep.empty()? 3 : atoi(strRep.c_str());
		m_search.Epdtest(psrc, tm, reps);
		fclose(psrc);
	}

	void OnEval()
	{
		EVAL e = Evaluate(g_pos, -INFINITY_SCORE, INFINITY_SCORE);
		out("eval = %d\n", e);
	}

	void OnFEN()
	{
		out("%s\n\n", g_pos.Fen());
	}

	void OnGo()
	{
		int restTimeMillisec = 0;

		for (std::string token = GetToken(); token.length() > 0; token = GetToken())
		{
			if (token == "infinite")
				m_search.SetLimits(0, 0, 0, 0);
			else if (token == "depth")
				m_search.SetLimits(atol(GetToken().c_str()), 0, 0, 0);
			else if (token == "nodes")
				m_search.SetLimits(0, atol(GetToken().c_str()), 0, 0);
			else if (token == "movetime")
			{
				token = GetToken();
				m_search.SetLimits(0, 0, atol(token.c_str()), atol(token.c_str()));
			}
			else if (token == "wtime" && g_pos.Side() == WHITE)
				restTimeMillisec = atoi(GetToken().c_str());
			else if (token == "btime" && g_pos.Side() == BLACK)
				restTimeMillisec = atoi(GetToken().c_str());
			else if(token == "winc" && g_pos.Side() == WHITE)
				m_inc = atoi(GetToken().c_str()); // in UCI time comes in milliseconds
			else if(token == "binc" && g_pos.Side() == BLACK)
				m_inc = atoi(GetToken().c_str()); // in UCI time comes in milliseconds
			else if (token == "movestogo")
				m_movesToGo = atoi(GetToken().c_str());
		}

		if (restTimeMillisec > 0)
			SetTimeLimits(restTimeMillisec);

		m_force = false;
		m_search.StartThinking(g_pos);
	}

	void OnLevel()
	{
		// mps
		m_movesPerSession = atoi(GetToken().c_str());
		// base
		GetToken();
		// inc
		m_inc = 1000 * atoi(GetToken().c_str()); // in WB increment comes in seconds
	}

	void OnList()
	{
		MoveList mvlist;
		mvlist.GenAllMoves(g_pos);
		for (int i = 0; i < mvlist.Size(); ++i)
		{
			out("%s ", MoveToStrLong(mvlist[i].m_mv));
		}
		out(" -- total %d moves\n", mvlist.Size());
	}

	void OnLoad()
	{
		std::string path = GetToken();
		FILE* psrc = fopen(path.c_str(), "rt");
		if (psrc == NULL)
		{
			out("%s not found\n", path);
			return;
		}
		std::string tmp = GetToken();
		int lineNum = tmp.empty()? 1 : atoi(tmp.c_str());
		char buf[256];
		for (int i = 0; i < lineNum; ++i)
		{
			if (fgets(buf, sizeof(buf), psrc) != NULL)
			{
				if (buf[strlen(buf) - 1] == '\n' || buf[strlen(buf) - 1] == '\r') buf[strlen(buf) - 1] = 0;
			}
		}
		if (g_pos.SetFen(buf))
			out("%s\n\n", buf);
		else
			out("Incorrect line number\n\n");
		fclose(psrc);
	}

	void OnMT()
	{
		std::string path = GetToken();
		FILE* psrc = fopen(path.c_str(), "rt");
		if (psrc == NULL)
		{
			out("%s not found\n", path);
			return;
		}

		Position tmp = g_pos;
		char buf[256];
		while (fgets(buf, sizeof(buf), psrc))
		{
			if (buf[strlen(buf) - 1] == '\n' || buf[strlen(buf) - 1] == '\r') buf[strlen(buf) - 1] = 0;
			out("%s\n", buf);
			g_pos.SetFen(buf);

			EVAL e1 = Evaluate(g_pos, -INFINITY_SCORE, INFINITY_SCORE);
			g_pos.Mirror();
			EVAL e2 = Evaluate(g_pos, -INFINITY_SCORE, INFINITY_SCORE);

			if (e1 != e2)
			{
				out("Incorrect evaluation:\n");
				out("e1 = %d\n", e1);
				out("e2 = %d\n", e2);
				break;
			}
		}
		fclose(psrc);
		g_pos = tmp;
		out("\n");
	}

	void OnNew()
	{
		m_force = false;
		g_pos.SetInitial();
	}

	void OnPerft()
	{
		std::string token = GetToken();
		m_search.StartPerft(g_pos, atoi(token.c_str()));
	}

	void OnPosition()
	{
		for (std::string token = GetToken(); token.length() > 0; token = GetToken())
		{
			if (token == "startpos")
			{
				g_pos.SetInitial();
			}
			else if (token == "moves")
			{
				for (token = GetToken(); token.length() > 0; token = GetToken())
				{
					Move mv = StrToMove(token, g_pos);
					if (mv)
						g_pos.MakeMove(mv);
					else
						break;
				}
			}
			else if (token == "fen")
			{
				std::string fen;
				for (token = GetToken(); token.length() > 0; token = GetToken())
				{
					if (token == "moves") break;
					fen += token;
					fen += " ";
				}
				g_pos.SetFen(fen);
				for (token = GetToken(); token.length() > 0; token = GetToken())
				{
					Move mv = StrToMove(token, g_pos);
					if (mv)
						g_pos.MakeMove(mv);
					else
						break;
				}
			}
		}
	}

	void OnProtover()
	{
		out("\nfeature myname=\"%s\"", VERSION);
		out(" setboard=1 analyze=1 colors=0 san=0 ping=1 name=1 done=1\n\n");
	}

	void OnRemove()
	{
		g_pos.UnmakeMove();
		g_pos.UnmakeMove();
	}

	void OnUCI()
	{
		g_protocol = UCI;
		out("id name %s\n", VERSION);
		out("id author Vladimir Medvedev\n");
		out("option name Hash type spin default %d min 1 max 1024\n", DEFAULT_HASH_MB);
		out("option name MultiPV type spin default %d min 1 max 64\n", DEFAULT_MULTI_PV);
		out("option name UCI_LimitStrength type check default %s\n", DEFAULT_UCI_LIMIT_STRENGTH);
		out("option name UCI_Elo type spin default %d min 1600 max 2400\n", DEFAULT_UCI_ELO);
		out("option name LimitKnps type spin default %d min 1 max 9999\n", DEFAULT_LIMIT_KNPS);
		out("option name Material type spin default %d min 0 max 100\n", DEFAULT_MATERIAL);
		out("option name BoardControl type spin default %d min 0 max 100\n", DEFAULT_BOARD_CONTROL);
		out("option name Mobility type spin default %d min 0 max 100\n", DEFAULT_MOBILITY);
		out("option name PawnStruct type spin default %d min 0 max 100\n", DEFAULT_PAWN_STRUCT);
		out("option name PawnPassed type spin default %d min 0 max 100\n", DEFAULT_PAWN_PASSED);
		out("option name KingSafety type spin default %d min 0 max 100\n", DEFAULT_KING_SAFETY);
		out("option name LazyEvalMargin type spin default %d min 50 max 500\n", DEFAULT_LAZY_EVAL_MARGIN);
		out("option name DrawScore type spin default %d min -1200 max 1200\n", DEFAULT_DRAW_SCORE);
		out("option name NullMoveReduction type spin default %d min 0 max 5\n", DEFAULT_NULL_MOVE_REDUCTION);
		out("option name NullMoveReductionPV type spin default %d min 0 max 5\n", DEFAULT_NULL_MOVE_REDUCTION_PV);
		out("option name NullMoveMinDepth type spin default %d min 1 max 9999\n", DEFAULT_NULL_MOVE_MIN_DEPTH);
		out("option name PruningMargin1 type spin default %d min 1 max 9999\n", DEFAULT_PRUNING_MARGIN_1);
		out("option name PruningMargin2 type spin default %d min 1 max 9999\n", DEFAULT_PRUNING_MARGIN_2);
		out("option name PruningMargin3 type spin default %d min 1 max 9999\n", DEFAULT_PRUNING_MARGIN_3);
		out("option name LmrMinDepth type spin default %d min 1 max 9999\n", DEFAULT_LMR_MIN_DEPTH);
		out("option name LmrMinMoveNumber type spin default %d min 1 max 9999\n", DEFAULT_LMR_MIN_MOVE_NUMBER);
		out("option name QChecks type spin default %d min 0 max 64\n", DEFAULT_QCHECKS);
		out("uciok\n");
	}

	void OnSD()
	{
		int sd = atoi(GetToken().c_str());
		m_search.SetLimits(sd, 0, 0, 0);
	}

	void OnSetboard()
	{
		std::string fen;
		std::string token;
		while ((token = GetToken()) != "")
		{
			fen += token;
			fen += " ";
		}
		g_pos.SetFen(fen);
	}

	void OnSetoption()
	{
		GetToken(); // "name"
		std::string key = GetToken();
		GetToken(); // value
		std::string value = GetToken();

		if (CaseInsensitiveEquals(key, "MultiPV"))
			m_search.SetMultiPV(atoi(value.c_str()));
		else if (CaseInsensitiveEquals(key, "Hash"))
			m_search.SetHashMB(atof(value.c_str()));
		else if (CaseInsensitiveEquals(key, "UCI_LimitStrength"))
			m_uciLimitStrength = (value != "false" && value != "0");
		else if (CaseInsensitiveEquals(key, "UCI_Elo"))
		{
			if (m_uciLimitStrength)
			{
				m_uciElo = atoi(value.c_str());
				// double knps = pow(10, (m_uciElo - 1600) / 400.);
				double knps = 0.01 + (pow(2, (m_uciElo - 1350) / 70.) / 200);  // formula suggested by Alexander Schmidt
				out("knps=%f\n", knps);
				m_search.SetKnps(knps);
			}
		}
		else if (CaseInsensitiveEquals(key, "LimitKnps"))
		{
			if (!m_uciLimitStrength)
				m_search.SetKnps(atof(value.c_str()));
		}
		else if (CaseInsensitiveEquals(key, "Material"))
			g_evalParams.Material = atoi(value.c_str());
		else if (CaseInsensitiveEquals(key, "BoardControl"))
			g_evalParams.BoardControl = atoi(value.c_str());
		else if (CaseInsensitiveEquals(key, "Mobility"))
			g_evalParams.Mobility = atoi(value.c_str());
		else if (CaseInsensitiveEquals(key, "PawnStruct"))
			g_evalParams.PawnStruct = atoi(value.c_str());
		else if (CaseInsensitiveEquals(key, "PawnPassed"))
			g_evalParams.PawnPassed = atoi(value.c_str());
		else if (CaseInsensitiveEquals(key, "KingSafety"))
			g_evalParams.KingSafety = atoi(value.c_str());
		else if (CaseInsensitiveEquals(key, "LazyEvalMargin"))
			g_evalParams.LazyEvalMargin = atoi(value.c_str());
		else if (CaseInsensitiveEquals(key, "DrawScore"))
			g_evalParams.DrawScore = atoi(value.c_str());
		else if (CaseInsensitiveEquals(key, "NullMoveReduction"))
			g_searchParams.NullMoveReduction = atoi(value.c_str());
		else if (CaseInsensitiveEquals(key, "NullMoveReductionPV"))
			g_searchParams.NullMoveReductionPV = atoi(value.c_str());
		else if (CaseInsensitiveEquals(key, "NullMoveMinDepth"))
			g_searchParams.NullMoveMinDepth = atoi(value.c_str());
		else if (CaseInsensitiveEquals(key, "PruningMargin1"))
			g_searchParams.PruningMargin1 = atoi(value.c_str());
		else if (CaseInsensitiveEquals(key, "PruningMargin2"))
			g_searchParams.PruningMargin2 = atoi(value.c_str());
		else if (CaseInsensitiveEquals(key, "PruningMargin3"))
			g_searchParams.PruningMargin3 = atoi(value.c_str());
		else if (CaseInsensitiveEquals(key, "LmrMinDepth"))
			g_searchParams.LmrMinDepth = atoi(value.c_str());
		else if (CaseInsensitiveEquals(key, "LmrMinMoveNumber"))
			g_searchParams.LmrMinMoveNumber = atoi(value.c_str());
		else if (CaseInsensitiveEquals(key, "QChecks"))
			g_searchParams.QChecks = atoi(value.c_str());
	}

	void OnSN()
	{
		NODES sn = atol(GetToken().c_str());
		m_search.SetLimits(0, sn, 0, 0);
	}

	void OnST()
	{
		int dt = int(1000 * atof(GetToken().c_str()));
		m_search.SetLimits(0, 0, dt, dt);
	}

	void OnTime()
	{
		SetTimeLimits(10 * atoi(GetToken().c_str())); // in WB time comes in centi-seconds
	}

public:
	void RunCommandLine()
	{
		out("\n");
		m_run = true;
		while (m_run)
		{
			char buf[4096];
			if (g_commandQueue.empty())
			{
				if (g_protocol == CONSOLE)
				{
					if (g_pos.Side() == WHITE)
						out("White(%d): ", g_pos.Ply() / 2 + 1);
					else
						out("Black(%d): ", g_pos.Ply() / 2 + 1);
				}
				ReadInput(buf, sizeof(buf));
				m_line = TokenString(buf);
			}
			else
			{
				m_line = TokenString(g_commandQueue.front());
				g_commandQueue.pop_front();
			}

			std::string cmd = GetToken();
			if (cmd.empty()) continue;

			Move mv = StrToMove(cmd, g_pos);
			if (mv)
			{
				g_pos.MakeMove(mv);
				if (!m_force)
					m_search.StartThinking(g_pos);
				continue;
			}

#define ON_CMD(pattern, minLen, action)    \
            if (Is(cmd, #pattern, minLen)) \
            {                              \
                action;                    \
                continue;                  \
            }

			ON_CMD (analyze,   2, m_search.StartAnalyze(g_pos))
			ON_CMD (bk,        2, OnBk())
			ON_CMD (board,     1, g_pos.Print())
			ON_CMD (book,      4, OnBook())
			ON_CMD (console,   3, OnConsole())
			ON_CMD (epdtest,   3, OnEpdtest())
			ON_CMD (eval,      1, OnEval())
			ON_CMD (fen,       3, OnFEN())
			ON_CMD (force,     5, m_force = true)
			ON_CMD (go,        2, OnGo())
			ON_CMD (isready,   7, out("readyok\n"))
			ON_CMD (level,     2, OnLevel())
			ON_CMD (list,      2, OnList())
			ON_CMD (load,      2, OnLoad())
			ON_CMD (mirror,    2, g_pos.Mirror())
			ON_CMD (mt,        2, OnMT())
			ON_CMD (new,       3, OnNew())
			ON_CMD (perft,     3, OnPerft())
			ON_CMD (ping,      4, out("pong %s\n", GetToken()))
			ON_CMD (position,  8, OnPosition())
			ON_CMD (protover,  8, OnProtover())
			ON_CMD (quit,      1, m_run = false)
			ON_CMD (remove,    6, OnRemove())
			ON_CMD (sd,        2, OnSD())
			ON_CMD (setboard,  8, OnSetboard())
			ON_CMD (setoption, 8, OnSetoption())
			ON_CMD (sn,        2, OnSN())
			ON_CMD (st,        2, OnST())
			ON_CMD (time,      2, OnTime())
			ON_CMD (xboard,    6, g_protocol = WINBOARD)
			ON_CMD (uci,       3, OnUCI())
			ON_CMD (undo,      1, g_pos.UnmakeMove())
			if (g_protocol == CONSOLE)
				out("Unknown command: %s\n", m_line.Str());
		}
	}

	void SetTimeLimits(int restMillisec)
	{
		int movesToGo = 60;

		if (m_movesToGo > 0)
			movesToGo = m_movesToGo;
		else if (m_movesPerSession > 0)
		{
			int movesPlayed = (g_pos.Ply() / 2) % m_movesPerSession;
			movesToGo = m_movesPerSession - movesPlayed;
		}

		// for safety reasons
		if (movesToGo == 1)
			movesToGo++;
		else if (movesToGo < 1)
			movesToGo = 60;

		if (m_inc > 0)
		{
			int stHard = restMillisec / 2;
			int stSoft = restMillisec / movesToGo + m_inc;
			m_search.SetLimits(0, 0, stHard, stSoft);
		}
		else
		{
			int stHard = restMillisec / movesToGo;
			int stSoft = restMillisec / movesToGo;
			m_search.SetLimits(0, 0, stHard, stSoft);
		}
	}

	void Init()
	{
		Config config("GreKo.ini");
		if (config.GetInt("WriteLog", 0)) g_log = fopen("greko.log", "at");

		Bitboard::InitBitboards();
		Position::InitHashNumbers();

		m_search.SetHashMB(config.GetDouble("HashMB", DEFAULT_HASH_MB));
		m_search.SetKnps(config.GetDouble("LimitKnps", DEFAULT_LIMIT_KNPS));

		g_evalParams.Material = config.GetInt("Material", DEFAULT_MATERIAL);
		g_evalParams.BoardControl = config.GetInt("BoardControl", DEFAULT_BOARD_CONTROL);
		g_evalParams.Mobility = config.GetInt("Mobility", DEFAULT_MOBILITY);
		g_evalParams.PawnStruct = config.GetInt("PawnStruct", DEFAULT_PAWN_STRUCT);
		g_evalParams.PawnPassed = config.GetInt("PawnPassed", DEFAULT_PAWN_PASSED);
		g_evalParams.KingSafety = config.GetInt("KingSafety", DEFAULT_KING_SAFETY);
		g_evalParams.LazyEvalMargin = config.GetInt("LazyEvalMargin", DEFAULT_LAZY_EVAL_MARGIN);
		g_evalParams.DrawScore = config.GetInt("DrawScore", DEFAULT_DRAW_SCORE);

		g_searchParams.NullMoveReduction = config.GetInt("NullMoveReduction", DEFAULT_NULL_MOVE_REDUCTION);
		g_searchParams.NullMoveReductionPV = config.GetInt("NullMoveReductionPV", DEFAULT_NULL_MOVE_REDUCTION_PV);
		g_searchParams.NullMoveMinDepth = config.GetInt("NullMoveMinDepth", DEFAULT_NULL_MOVE_MIN_DEPTH);

		g_searchParams.PruningMargin1 = config.GetInt("PruningMargin1", DEFAULT_PRUNING_MARGIN_1);
		g_searchParams.PruningMargin2 = config.GetInt("PruningMargin2", DEFAULT_PRUNING_MARGIN_2);
		g_searchParams.PruningMargin3 = config.GetInt("PruningMargin3", DEFAULT_PRUNING_MARGIN_3);

		g_searchParams.LmrMinDepth = config.GetInt("LmrMinDepth", DEFAULT_LMR_MIN_DEPTH);
		g_searchParams.LmrMinMoveNumber = config.GetInt("LmrMinMoveNumber", DEFAULT_LMR_MIN_MOVE_NUMBER);

		g_searchParams.QChecks = config.GetInt("QChecks", DEFAULT_QCHECKS);

		g_book.Init();
		RandSeed32(U32(time(0)));

		g_pos.SetInitial();
		InitEval();
	}
};

int main()
{
	InitInput();

	Highlight(true);
	out("\n%s (%s)\n\n", VERSION.c_str(), RELEASE_DATE.c_str());
	Highlight(false);

	Engine e;
	e.Init();
	e.RunCommandLine();
	return 0;
}

