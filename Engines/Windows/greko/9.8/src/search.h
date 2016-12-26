//  GREKO Chess Engine
//  (c) 2002-2012 Vladimir Medvedev <vrm@bk.ru>
//  http://greko.110mb.com

//  search.h: chess tree search
//  modified: 31-Dec-2012

#pragma once

#include <math.h>
#include "position.h"
#include "utils.h"

enum
{
  HASH_UNKNOWN = 0x00, 
  HASH_ALPHA   = 0x10, 
  HASH_BETA    = 0x20, 
  HASH_EXACT   = 0x40
};

struct HashEntry
{
  I8   m_depth;
  EVAL m_eval;
  U64  m_hash;
  Move m_mv;
  U8   m_flags;

  U8 Age() const  { return m_flags & 0x0f; }
  U8 Type() const { return m_flags & 0xf0; }
};

const EVAL SORT_HASH       = 0x40000000;
const EVAL SORT_CAPTURE    = 0x20000000;
const EVAL SORT_MATEKILLER = 0x18000000;
const EVAL SORT_KILLER     = 0x10000000;

class Search
{
public:
  Search() : 
      m_mode(IDLE),
      m_hash(NULL),
      m_hashSize(0),
      m_inc(0),
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
  int   GetIncrement() const { return m_inc; }
  int   GetSearchTime() { return 1000 * (clock() - m_startTime) / CLOCKS_PER_SEC; } // milliseconds
  NODES Perft(int depth);
  void  PrintPV();
  HashEntry* ProbeHash();
  void  RecordHash(Move bestMove, int depth, EVAL eval, U8 type, int ply);
  void  SetHashMB(double mb);
  void  SetIncrement(int inc) { m_inc = inc; }
  void  SetKnps(double knps) { m_knpsLimit = knps; }
  void  SetLimits(int restMillisec, int sd, NODES sn, int stHard, int stSoft);
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
  static int MvvLva(Move mv);
  static EVAL SEE_Swap(const Position& pos, FLD to, EVAL currScore, PIECE currPiece, U64 mask, COLOR side);

  void LimitKnps()
  {
    int searchTime = GetSearchTime();
    double knps = (searchTime > 0)? static_cast<double>(m_nodes) / searchTime : 0;
    if (!m_flag)
    {
      // out("knps=%d\n", knps);
      if  (knps > m_knpsLimit)
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
        // out("sleepTime=%d\n", sleepTime);
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
    std::vector<Move> m_pv;
    EVAL mm_score;
    bool m_seen;
  };

  bool         m_flag;
  HashEntry*   m_hash;
  U8           m_hashAge;
  long         m_hashSize;
  int          m_histTry[14][64];
  int          m_histSuccess[14][64];
  int          m_inc;
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
  std::vector<Move> m_PV[MAX_PLY];
  int          m_restMillisec;
  std::vector<Move> m_rootPV;
  EVAL         m_rootAlpha;
  EVAL         m_rootBeta;
  EVAL         m_rootWindow;
  clock_t      m_startTime;
  int          m_sd;
  NODES        m_sn;
  int          m_stHard;
  int          m_stSoft;
};

