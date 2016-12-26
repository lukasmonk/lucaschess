//  GREKO Chess Engine
//  (c) 2002-2015 Vladimir Medvedev <vrm@bk.ru>
//  http://greko.su

//  search.h: chess tree search
//  modified: 01-Aug-2015

#ifndef SEARCH_H
#define SEARCH_H

#include <math.h>
#include "defaults.h"
#include "position.h"
#include "utils.h"

class Line
{
public:

	Line() : m_size(0) {}
	~Line() {}

	void Clear() { m_size = 0; }
	void Compose(Move mv, const Line& tail)
	{
		m_data[0] = mv;
		memcpy(m_data + 1, tail.m_data, tail.Size() * sizeof(Move));
		m_size = 1 + tail.Size();
	}
	size_t Size() const { return m_size; }
	Move operator[](size_t i) const { return m_data[i]; }

private:
	Move m_data[256];
	size_t m_size;
};

struct SearchParams
{
	SearchParams() :
		NullMoveReduction(DEFAULT_NULL_MOVE_REDUCTION),
		NullMoveReductionPV(DEFAULT_NULL_MOVE_REDUCTION_PV),
		NullMoveMinDepth(DEFAULT_NULL_MOVE_MIN_DEPTH),
		PruningMargin1(DEFAULT_PRUNING_MARGIN_1),
		PruningMargin2(DEFAULT_PRUNING_MARGIN_2),
		PruningMargin3(DEFAULT_PRUNING_MARGIN_3),
		LmrMinDepth(DEFAULT_LMR_MIN_DEPTH),
		LmrMinMoveNumber(DEFAULT_LMR_MIN_MOVE_NUMBER),
		QChecks(DEFAULT_QCHECKS)
	{}

	int NullMoveReduction;
	int NullMoveReductionPV;
	int NullMoveMinDepth;
	int PruningMargin1;
	int PruningMargin2;
	int PruningMargin3;
	int LmrMinDepth;
	int LmrMinMoveNumber;
	int QChecks;
};

extern SearchParams g_searchParams;

enum
{
	HASH_UNKNOWN = 0x00,
	HASH_ALPHA   = 0x10,
	HASH_BETA    = 0x20,
	HASH_EXACT   = 0x40
};

struct HashEntry
{
	U64 m_hash;
	U32 m_mv;
	I16 m_eval;
	I8  m_depth;
	U8  m_flags;

	U8 Age() const { return m_flags & 0x0f; }
	U8 Type() const { return m_flags & 0xf0; }
};

const int SORT_HASH       = 0x40000000;
const int SORT_CAPTURE    = 0x20000000;
const int SORT_MATEKILLER = 0x18000000;
const int SORT_KILLER     = 0x10000000;

class Search
{
public:
	Search() :
		m_mode(IDLE),
		m_hash(NULL),
		m_hashSize(0),
		m_knpsLimit(999999),
		m_multiPV(1),
		m_rootWindow(VAL_P),
		m_sd(0),
		m_sn(0),
		m_stHard(2000),
		m_stSoft(2000) {}

	EVAL  AlphaBetaRoot(EVAL alpha, EVAL beta, const int depth);
	EVAL  AlphaBeta(EVAL alpha, EVAL beta, const int depth, int ply, bool isNull);
	EVAL  AlphaBetaQ(EVAL alpha, EVAL beta, int ply, int qply);
	void  CheckInput();
	void  CheckLimits();
	void  ClearHash();
	void  ClearHistory();
	void  ClearKillers();
	void  Epdtest(FILE* psrc, double time_in_seconds, int reps);
	int   Extensions(Move mv, Move lastMove, bool check, int ply, bool onPV) const;
	int   GetSearchTime() const { return GetTime() - m_startTime; }   // milliseconds
	NODES Perft(int depth);
	void  PrintPV();
	HashEntry* ProbeHash();
	void  RecordHash(Move bestMove, int depth, EVAL eval, U8 type, int ply);
	void  SetHashMB(double mb);
	void  SetKnps(double knps) { m_knpsLimit = knps; }
	void  SetLimits(int sd, NODES sn, int stHard, int stSoft);
	void  SetMultiPV(int n) { m_multiPV = n; }
	void  SetNPS(int nps) { m_npsLimit = nps; }
	void  StartAnalyze(const Position& pos);
	bool  StartEpd(const std::string& fen, int reps);
	void  StartPerft(const Position& pos, int depth);
	void  StartThinking(Position& pos);
	void  UpdateScores(MoveList& mvlist, Move hashmv, int ply);
	void  UpdateScoresQ(MoveList& mvlist);

	static EVAL SEE(const Position& pos, Move mv);
	static EVAL SEE_Exchange(const Position& pos, FLD to, COLOR side, EVAL currScore, EVAL target, U64 occ);

	void LimitKnps()
	{
		int searchTime = GetSearchTime();
		double knps = (searchTime > 0)? static_cast<double>(m_nodes) / searchTime : 0;
		if (!m_flag)
		{
			if (knps > m_knpsLimit)
			{
				int sleepTime = static_cast<int>(m_nodes / m_knpsLimit) - searchTime;
				if (m_stHard)
				{
					if (sleepTime > m_stHard - searchTime)
					{
						sleepTime = m_stHard - searchTime;
						m_flag = true;
					}
				}
				SleepMilliseconds(sleepTime);
			}
		}
	}

private:

	enum { MAX_PLY = 64 };
	enum { MAX_BRANCH = 128 };
	enum { IDLE, ANALYZE, THINKING, EPDTEST } m_mode;

	struct MultiPVEntry
	{
		Line m_pv;
		EVAL m_score;
		bool m_seen;
	};

	bool         m_flag;
	HashEntry*   m_hash;
	U8           m_hashAge;
	long         m_hashSize;
	int          m_histTry[14][64];
	int          m_histSuccess[14][64];
	int          m_iter;
	EVAL         m_iterScore;
	Move         m_killers[MAX_PLY];
	double       m_knpsLimit;
	MoveList     m_lists[MAX_PLY];
	Move         m_matekillers[MAX_PLY];
	int          m_multiPV;
	MultiPVEntry m_multiPVs[MAX_BRANCH];
	NODES        m_nodes;
	int          m_npsLimit;
	Position     m_pos;
	Line         m_PV[MAX_PLY];
	Line         m_rootPV;
	EVAL         m_rootAlpha;
	EVAL         m_rootBeta;
	EVAL         m_rootWindow;
	U32          m_startTime;
	int          m_sd;
	NODES        m_sn;
	int          m_stHard;
	int          m_stSoft;
};

#endif

