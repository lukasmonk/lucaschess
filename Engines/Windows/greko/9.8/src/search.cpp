//  GREKO Chess Engine
//  (c) 2002-2012 Vladimir Medvedev <vrm@bk.ru>
//  http://greko.110mb.com

//  search.cpp: chess tree search
//  modified: 31-Dec-2012

#include "book.h"
#include "eval.h"
#include "moves.h"
#include "notation.h"
#include "search.h"
#include "utils.h"

extern Position   g_pos;
extern PROTOCOL_T g_protocol;
extern std::list<std::string> g_commandQueue;
extern Book g_book;

inline bool IsUCI() { return g_protocol == UCI; }

EVAL Search::AlphaBetaRoot(EVAL alpha, EVAL beta, const int depth)
{
  m_rootAlpha = alpha;
  m_rootBeta = beta;

  int ply = 0;
  bool inCheck = m_pos.InCheck();
  m_PV[ply].clear();
  m_nodes++;

  for (int j = 0; j < MAX_BRANCH; ++j)
  {
    m_multiPVs[j].mm_score = -INFINITY_SCORE;
    m_multiPVs[j].m_pv.clear();
  }

  Move hashMove = 0;
  U8 hashType = HASH_ALPHA;
  HashEntry* pentry = ProbeHash();
  if (pentry)
  {
    hashMove = pentry->m_mv;
  }

  MoveList& mvlist = m_lists[ply];
  if (inCheck)
    mvlist.GenCheckEvasions(m_pos);
  else
    mvlist.GenAllMoves(m_pos);
  UpdateScores(mvlist, hashMove, ply);

  EVAL e = 0;
  int legalMoves = 0;
  Move bestMove = 0;

  for (int i = 0; i < mvlist.Size(); ++i)
  {
    Move mv = mvlist.GetNthBest(i);
    if (m_pos.MakeMove(mv))
    {
      legalMoves++;
      m_histTry[mv.Piece()][mv.To()] += depth;

      if (IsUCI() && GetSearchTime() > 1000)
      {
        out("info currmove %s", MoveToStrLong(mv));
        out(" currmovenumber %d\n", legalMoves);
      }

      int newDepth = depth - 1;
      bool givesCheck = m_pos.InCheck();

      //
      //   EXTENSIONS
      //

      if (givesCheck)
        ++newDepth;
      else if (mv.Piece() == PW && (mv.To() / 8) == 1)
        ++newDepth;
      else if (mv.Piece() == PB && (mv.To() / 8) == 6)
        ++newDepth;
      
      if (m_multiPV > 1)
        e = -AlphaBeta(-beta - VAL_Q, -alpha + VAL_Q, newDepth, ply + 1, false);
      else
      {
        if (legalMoves == 1)
          e = -AlphaBeta(-beta, -alpha, newDepth, ply + 1, false);
        else
        {
          e = -AlphaBeta(-alpha - 1, -alpha, newDepth, ply + 1, false);
          if (e > alpha && e < beta)
            e = -AlphaBeta(-beta, -alpha, newDepth, ply + 1, false);
        }
      }
      m_pos.UnmakeMove();

      if (m_flag)
        return alpha;

      if (legalMoves == 1)
      {
        bestMove = mv;
        m_PV[ply].clear();
        m_PV[ply].push_back(mv);
        m_PV[ply].insert(m_PV[ply].end(), m_PV[ply + 1].begin(), m_PV[ply + 1].end());
      }

      //
      //   Update multipv
      //

      if (legalMoves < MAX_BRANCH)
      {
        MultiPVEntry *mpv = &(m_multiPVs[legalMoves - 1]);
        
        mpv->m_pv.clear();
        mpv->mm_score = e;
        mpv->m_pv.push_back(mv);
        mpv->m_pv.insert(mpv->m_pv.end(), m_PV[ply + 1].begin(), m_PV[ply + 1].end());
      }

      if (e > alpha)
      {
        bestMove = mv;
        if (!mv.Captured())
          m_histSuccess[mv.Piece()][mv.To()] += depth;
        hashType = HASH_EXACT;

        m_PV[ply].clear();
        m_PV[ply].push_back(mv);
        m_PV[ply].insert(m_PV[ply].end(), m_PV[ply + 1].begin(), m_PV[ply + 1].end());

        alpha = e;
        if (alpha >= beta)
        {
          hashType = HASH_BETA;
          if (!mv.Captured())
          {
            if (e > CHECKMATE_SCORE - MAX_PLY && e <= CHECKMATE_SCORE)
              m_matekillers[ply] = mv;
            else
              m_killers[ply] = mv;              
          }
          break;
        }
      }
    }
  }
  if (legalMoves == 0)
  {
    if (inCheck)
      alpha = -CHECKMATE_SCORE + ply;
    else
      alpha = DRAWm_score;
  }
  RecordHash(bestMove, depth, alpha, hashType, ply);
  return alpha;
}

EVAL Search::AlphaBeta(EVAL alpha, EVAL beta, const int depth, int ply, bool isNull)
{
  m_PV[ply].clear();
  ++m_nodes;

  COLOR side = m_pos.Side();
  bool onPV = (beta - alpha > 1);
  bool inCheck = m_pos.InCheck();

  CheckLimits();
  if (m_nodes % 8192 == 0)
    CheckInput();

  //
  //   DRAW DETECTION
  //

  if (ply >= MAX_PLY) return DRAWm_score;
  EVAL_ESTIMATION ee = EstimateDraw(m_pos);
  if (ee == EVAL_THEORETICAL_DRAW || ee == EVAL_PRACTICAL_DRAW) return DRAWm_score;

  if (!isNull)
  {
    int rep_total = m_pos.GetRepetitions();
    if (rep_total >= 2)
      return DRAWm_score;
  }

  //
  //   PROBING HASH
  //

  Move hashMove = 0;
  U8 hashType = HASH_ALPHA;
  HashEntry* pentry = ProbeHash();

  if (pentry)
  {
    hashMove = pentry->m_mv;
    if (pentry->m_depth >= depth)
    {
      EVAL hashm_eval = pentry->m_eval;
      if (hashm_eval > CHECKMATE_SCORE - 50 && hashm_eval <= CHECKMATE_SCORE)
        hashm_eval -= ply;
      else if (hashm_eval < -CHECKMATE_SCORE + 50 && hashm_eval >= -CHECKMATE_SCORE)
        hashm_eval += ply;

      if (pentry->Type() == HASH_EXACT)
        return hashm_eval;      
      else if (pentry->Type() == HASH_ALPHA && hashm_eval <= alpha)
        return alpha;
      else if (pentry->Type() == HASH_BETA && hashm_eval >= beta)
        return beta;
    }
  }

  //
  //   QSEARCH
  //

  if (depth <= 0 && !inCheck)
  {
    --m_nodes;
    return AlphaBetaQ(alpha, beta, ply, 0);
  }

  //
  //   PRUNING
  //

  EVAL MARGIN[4] = { INFINITY_SCORE, VAL_P - 50, VAL_B - 50, VAL_R - 50 };
  if (!inCheck && !onPV && depth > 0 && depth < 4)
  {
    EVAL staticScore = Evaluate(m_pos, alpha - MARGIN[depth], beta + MARGIN[depth]);
    if (staticScore <= alpha - MARGIN[depth])
      return AlphaBetaQ(alpha, beta, ply, 0);
    if (staticScore >= beta + MARGIN[depth])
      return beta;
  }

  //
  //   NULL MOVE
  //

  const int R = 3;
  bool mateThreat = false;
  if (!inCheck && !isNull && m_pos.MatIndex(side) > 0 && depth > 2)
  {
    m_pos.MakeNullMove();
    EVAL nullEval;
    nullEval = -AlphaBeta(-beta, -beta + 1, depth - 1 - R, ply + 1, true);
    m_pos.UnmakeNullMove();
    if (nullEval >= beta) 
        return beta;
    if (nullEval <= -CHECKMATE_SCORE + 50 && nullEval >= -CHECKMATE_SCORE)
        mateThreat = true;
  }

  MoveList& mvlist = m_lists[ply];
  if (inCheck)
    mvlist.GenCheckEvasions(m_pos);
  else
    mvlist.GenAllMoves(m_pos);
  UpdateScores(mvlist, hashMove, ply);

  int legalMoves = 0;
  EVAL e = 0;
  Move bestMove = 0;

  for (int i = 0; i < mvlist.Size(); ++i)
  {
    Move mv = mvlist.GetNthBest(i);
    if (m_pos.MakeMove(mv))
    {
      legalMoves++;
      m_histTry[mv.Piece()][mv.To()] += depth;

      int newDepth = depth - 1;
      bool givesCheck = m_pos.InCheck();

      //
      //   EXTENSIONS
      //

      if (givesCheck)
        ++newDepth;
      else if (mv.Piece() == PW && (mv.To() / 8) == 1)
        ++newDepth;
      else if (mv.Piece() == PB && (mv.To() / 8) == 6)
        ++newDepth;
      else if (mateThreat)
        ++newDepth;

      //
      //   LMR
      //

      bool reduced = false;
      if (!onPV && !inCheck && !givesCheck && !mv.Captured() && !mv.Promotion())
      {
        if (legalMoves > 3 && depth > 2)
        {
          if (m_histSuccess[mv.Piece()][mv.To()] <= m_histTry[mv.Piece()][mv.To()] / 2)
          {
            --newDepth;
            reduced = true;
          }
        }
      }
      
      if (legalMoves == 1)
        e = -AlphaBeta(-beta, -alpha, newDepth, ply + 1, false);
      else
      {
        e = -AlphaBeta(-alpha - 1, -alpha, newDepth, ply + 1, false);
        if (reduced && e > alpha)
        {
          ++newDepth;
          e = -AlphaBeta(-alpha - 1, -alpha, newDepth, ply + 1, false);
        }
        if (e > alpha && e < beta)
          e = -AlphaBeta(-beta, -alpha, newDepth, ply + 1, false);
      }
      m_pos.UnmakeMove();

      if (m_flag)
        return alpha;

      if (e > alpha)
      {
        bestMove = mv;
        if (!mv.Captured())
          m_histSuccess[mv.Piece()][mv.To()] += depth;
        hashType = HASH_EXACT;

        alpha = e;
        if (alpha >= beta)
        {
          hashType = HASH_BETA;
          if (!mv.Captured())
          {
            if (e > CHECKMATE_SCORE - MAX_PLY && e <= CHECKMATE_SCORE)
              m_matekillers[ply] = mv;
            else
              m_killers[ply] = mv;
          }

          break;
        }

        m_PV[ply].clear();
        m_PV[ply].push_back(mv);
        m_PV[ply].insert(m_PV[ply].end(), m_PV[ply + 1].begin(), m_PV[ply + 1].end());
      }
    }
  }
  if (legalMoves == 0)
  {
    if (inCheck)
      alpha = -CHECKMATE_SCORE + ply;
    else
      alpha = DRAWm_score;
  }
  else if (m_pos.Fifty() >= 100)
    alpha = DRAWm_score;

  RecordHash(bestMove, depth, alpha, hashType, ply);
  return alpha;
}

EVAL Search::AlphaBetaQ(EVAL alpha, EVAL beta, int ply, int qply)
{
  m_PV[ply].clear();
  ++m_nodes;

  CheckLimits();
  if (m_nodes % 8192 == 0)
    CheckInput();

  if (ply >= MAX_PLY)
    return DRAWm_score;

  bool inCheck = m_pos.InCheck();
  EVAL staticEval = Evaluate(m_pos, alpha, beta);
  if (!inCheck)
  {
    if (staticEval > alpha)
      alpha = staticEval;
  }

  if (alpha >= beta)
    return beta;

  MoveList& mvlist = m_lists[ply];
  if (inCheck)
    mvlist.GenCheckEvasions(m_pos);
  else
    mvlist.GenCaptures(m_pos, qply < 2);
  UpdateScoresQ(mvlist);

  EVAL e = 0;
  int legalMoves = 0;
  for (int i = 0; i < mvlist.Size(); ++i)
  {
    Move mv = mvlist.GetNthBest(i);    
    if (!inCheck && SEE(m_pos, mv) < 0)
      continue;

    if (m_pos.MakeMove(mv))
    {
      legalMoves++;
      e = -AlphaBetaQ(-beta, -alpha, ply + 1, qply + 1);
      m_pos.UnmakeMove();

      if (m_flag)
        return alpha;

      if (e > alpha)
      {
        alpha = e;
        if (alpha >= beta)
          break;

        m_PV[ply].clear();
        m_PV[ply].push_back(mv);
        m_PV[ply].insert(m_PV[ply].end(), m_PV[ply + 1].begin(), m_PV[ply + 1].end());
      }
    }
  }

  if (inCheck && legalMoves == 0)
    alpha = -CHECKMATE_SCORE + ply;

  return alpha;
}

void Search::CheckInput()
{
  if (m_rootPV.empty())
    return;

  if (InputAvailable())
  {
    char buf[4096];
    ReadInput(buf, sizeof(buf));
    TokenString ts(buf);
    std::string cmd = ts.GetToken();

    if (m_mode == ANALYZE)
    {
      Move mv = StrToMove(cmd, g_pos);
      if (mv)
      {
        m_flag = true;
        g_commandQueue.push_back("force");
        g_commandQueue.push_back(ts.Str());
        g_commandQueue.push_back("analyze");
      }
      else if (Is(cmd, "board", 1))
        g_pos.Print();
      else if (Is(cmd, "quit", 1))
        exit(0);
      else if (Is(cmd, "isready", 7))
        out("readyok\n");
      else if (Is(cmd, "exit", 2))
        m_flag = true;
      else if (Is(cmd, "stop", 2))
        m_flag = true;
      else if (Is(cmd, "new", 3))
        m_flag = true;
      else if (Is(cmd, "setoption", 8))
      {
        ts.GetToken(); // "name"
        std::string token = ts.GetToken(); // name
        if (token == "MultiPV")
        {
          ts.GetToken(); // "value"
          token = ts.GetToken(); // value
          m_multiPV = atoi(token.c_str());
        }
      }
      else if (Is(cmd, "undo", 4))
      {
        m_flag = true;
        g_commandQueue.push_back("undo");
        g_commandQueue.push_back("analyze");
      }
      else if (Is(cmd, "remove", 6))
      {
        m_flag = true;
        g_commandQueue.push_back("undo");
        g_commandQueue.push_back("analyze");
      }
    }

    else if (m_mode == THINKING)
    {
      if (Is(cmd, "quit", 1))
        exit(0);
      else if (Is(cmd, "isready", 7))
        out("readyok\n");
      else if (Is(cmd, "?", 1))
        m_flag = true;
      else if (Is(cmd, "stop", 4))
        m_flag = true;
      else if (Is(cmd, "new", 3))
        m_flag = true;
      else if (Is(cmd, "result", 6))
        m_flag = true;
      else if (Is(cmd, "setoption", 8))
      {
        ts.GetToken(); // "name"
        std::string token = ts.GetToken(); // name
        if (token == "MultiPV")
        {
          ts.GetToken(); // "value"
          token = ts.GetToken(); // value
          m_multiPV = atoi(token.c_str());
        }
      }
    }

    else if (m_mode == EPDTEST)
    {
      if (Is(cmd, "board", 1))
        g_pos.Print();
      else if (Is(cmd, "quit", 1))
        exit(0);
      else if (Is(cmd, "exit", 2))
        m_flag = true;
      else if (Is(cmd, "new", 3))
        m_flag = true;
    }
  }
}

void Search::CheckLimits()
{
  if (m_rootPV.empty())
    return;

  if (m_stHard)
  {
    if (GetSearchTime() >= m_stHard)
    {
      if (m_mode == THINKING || m_mode == EPDTEST)
        m_flag = true;
    }
  }

  if (m_sn && m_nodes >= m_sn)
  {
    if (m_mode == THINKING || m_mode == EPDTEST)
      m_flag = true;
  }
}

void Search::ClearHash()
{
  memset(m_hash, 0, m_hashSize * sizeof(HashEntry));
}

void Search::ClearHistory()
{  
  memset(m_histTry, 0, 64 * 14 * sizeof(m_histTry[0][0]));
  memset(m_histSuccess, 0, 64 * 14 * sizeof(m_histSuccess[0][0]));
}

void Search::ClearKillers()
{
  memset(m_killers, 0, MAX_PLY * sizeof(Move));
  memset(m_matekillers, 0, MAX_PLY * sizeof(Move));
}

void Search::Epdtest(FILE* src, double seconds, int reps)
{
  char fen[256];
  int dt = static_cast<int>(1000 * seconds);
  SetLimits(0, 0, 0, dt, dt);
  int total = 0, solved = 0;

  while (fgets(fen, sizeof(fen), src))
  {
    if (m_pos.SetFen(fen) == false)
      continue;

    out(fen);
    out("\n");

    ++total;
    if (StartEpd(fen, reps))
      ++solved;

    out("\nScore: %d / %d\n\n", solved, total);
  }
}

NODES Search::Perft(int depth)
{
  if (depth == 0)
    return 1;

  MoveList& mvlist = m_lists[depth];
  mvlist.GenAllMoves(m_pos);
  NODES sum = 0, delta = 0;

  for (int i = 0; i < mvlist.Size(); ++i)
  {
    Move mv = mvlist[i].m_mv;
    if (m_pos.MakeMove(mv))
    {
      delta = Perft(depth - 1);
      sum += delta;
      m_pos.UnmakeMove();
    }
  }

  return sum;
}

void Search::PrintPV()
{
  Move mv = 0;

  if (IsUCI())
  {
    for (int j = 0; j < MAX_BRANCH; ++j)
      m_multiPVs[j].m_seen = false;

    for (int j = 1; j <= m_multiPV; ++j)
    {
      EVAL bestm_score = -INFINITY_SCORE;
      MultiPVEntry *best_mpv = NULL;  
      MultiPVEntry *mpv = NULL;

      for (int k = 0; k < MAX_BRANCH; ++k)
      {
        mpv = &(m_multiPVs[k]);
        if (mpv->m_seen)
          continue;
        if (mpv->m_pv.empty())
          break;
        if (mpv->mm_score > bestm_score)
        {
          bestm_score = mpv->mm_score;
          best_mpv = mpv;
        }
      }

      if (best_mpv)
      {
        best_mpv->m_seen = true;
        mpv = best_mpv;
        out("info multipv %d depth %d score", j, m_iter);

        if (mpv->mm_score > CHECKMATE_SCORE - 50)
          out(" mate %d", 1 + (CHECKMATE_SCORE - mpv->mm_score) / 2);
        else if (mpv->mm_score < -CHECKMATE_SCORE + 50)
          out(" mate -%d", (mpv->mm_score + CHECKMATE_SCORE) / 2);
        else
          out(" cp %d", mpv->mm_score);

        out(" time %ld", GetSearchTime());
        out(" nodes %ld", m_nodes);
        if (GetSearchTime() > 0)
        {
          double knps = static_cast<double>(m_nodes) / GetSearchTime();
          out (" nps %d", static_cast<int>(1000 * knps));
        }
        out(" pv ");
        for (size_t m = 0; m < mpv->m_pv.size(); ++m)
        {
          mv = mpv->m_pv[m];
          out("%s ", MoveToStrLong(mv));
        }
        out("\n");
      }
    }
    if (GetSearchTime() > 0)
    {
      out("info time %ld", GetSearchTime());
      out(" nodes %ld", m_nodes);
      double knps = static_cast<double>(m_nodes) / GetSearchTime();
      out (" nps %d", static_cast<int>(1000 * knps));
    }
    out("\n");
    return;
  }
  else
  {
    out(" %2d", m_iter);
    out(" %9d ", m_iterScore);
    out(" %7d ", GetSearchTime() / 10);
    out(" %12d ", m_nodes);
    out(" ");
  }

  Position tmp = m_pos;
  int movenum = tmp.Ply() / 2 + 1;
  for (size_t m = 0; m < m_rootPV.size(); ++m)
  {
    mv = m_rootPV[m];
    if (tmp.Side() == WHITE)
    {
      out("%d. ", movenum++);
    }
    else if (m == 0)
    {
      out("%d. ... ", movenum++);
    }

    MoveList mvlist;
    mvlist.GenAllMoves(tmp);
    out("%s", MoveToStrShort(mv, tmp));
    tmp.MakeMove(mv);

    if (tmp.InCheck())
    {
      if (int(m_iterScore + m + 1) == CHECKMATE_SCORE || int(m_iterScore - m - 1) == -CHECKMATE_SCORE)
      {
        out("#");
        if (m_iterScore > 0)
          out(" {+");
        else
          out(" {-");
        out("Mate in %d}", m / 2 + 1);
      }
      else
        out("+");
    }
    if (m == 0)
    {
      if (m_iterScore <= m_rootAlpha)
        out("?");
      else if (m_iterScore >= m_rootBeta)
        out("!");
    }
    out(" ");
  }
  out("\n");
}

HashEntry* Search::ProbeHash()
{
  long index = long(m_pos.Hash() % m_hashSize);
  if (m_hash[index].m_hash == m_pos.Hash())
    return m_hash + index;
  else
    return NULL;
}

void Search::RecordHash(Move bestMove, int depth, EVAL eval, U8 hashType, int ply)
{
  long index = long(m_pos.Hash() % m_hashSize);
  HashEntry* pentry = m_hash + index;

  if (depth > pentry->m_depth || m_hashAge != pentry->Age())
  {
    if (eval > CHECKMATE_SCORE - 50 && eval <= CHECKMATE_SCORE)
      eval += ply;
    else if (eval < -CHECKMATE_SCORE + 50 && eval >= -CHECKMATE_SCORE)
      eval -= ply;

    pentry->m_depth = static_cast<I8>(depth);
    pentry->m_eval = eval;
    pentry->m_hash = m_pos.Hash();
    pentry->m_mv = bestMove;
    pentry->m_flags = hashType | m_hashAge;
  }
}

void Search::SetHashMB(double mb)
{
  if (m_hash)
    delete[] m_hash;

  m_hashSize = long (1024 * 1024 * mb / sizeof(HashEntry));
  if (m_hashSize <= 0)
    m_hashSize = 1;

  m_hash = new HashEntry[m_hashSize];

  if (!IsUCI())
  {
    out("main hash: %8ld nodes = ", m_hashSize);
    out("%f MB\n", m_hashSize * sizeof(HashEntry) / (1024. * 1024.));
  }
}

void Search::SetLimits(int restMillisec, int sd, NODES sn, int stHard, int stSoft)
{
  m_restMillisec = restMillisec;
  m_sd = sd;
  m_sn = sn;
  m_stHard = stHard;
  m_stSoft = stSoft;
}

void Search::StartAnalyze(const Position& pos)
{
  m_startTime = clock();
  m_pos = pos;
  m_hashAge = (m_hashAge + 1) & 0x0f;
  m_mode = ANALYZE;
  m_flag = false;
  m_nodes = 0;

  SetLimits(0, 0, 0, 0, 0);

  ClearHash();
  ClearHistory();
  ClearKillers();

  if (!IsUCI())
  {
    out("\n%s\n\n", m_pos.Fen());
  }

  EVAL alpha = -INFINITY_SCORE;
  EVAL beta = INFINITY_SCORE;

  for (m_iter = 1; m_iter < MAX_PLY; m_iter++)
  {
    EVAL e = AlphaBetaRoot(alpha, beta, m_iter);
    LimitKnps();

    if (m_flag)
      break;

    m_iterScore = e;
    if (e > alpha)
      m_rootPV = m_PV[0];

    if (e > alpha && e < beta)
    {
      PrintPV();

      alpha = e - m_rootWindow / 2;
      beta = e + m_rootWindow / 2;
    }
    else
    {
      PrintPV();

      alpha = -INFINITY_SCORE;
      beta = INFINITY_SCORE;
      m_iter--;
    }
  }
}

bool Search::StartEpd(const std::string& fen, int reps)
{
  m_hashAge = (m_hashAge + 1) & 0x0f;
  m_mode = EPDTEST;
  m_flag = false;
  m_nodes = 0;
  int counter = 0;  

  ClearHash();
  ClearHistory();
  ClearKillers();

   m_startTime = clock();

  EVAL alpha = -INFINITY_SCORE;
  EVAL beta = INFINITY_SCORE;

  for (m_iter = 1; m_iter < MAX_PLY; m_iter++)
  {
    bool moveFound = false;
    EVAL e = AlphaBetaRoot(alpha, beta, m_iter);
    LimitKnps();

    if (m_flag)
      break;

    m_iterScore = e;
    if (e > alpha)
      m_rootPV = m_PV[0];

    if (!m_rootPV.empty())
    {
      Move mv = m_rootPV[0];
      std::string mvstr = MoveToStrShort(mv, m_pos);

      if (fen.find(mvstr) != std::string::npos)
        moveFound = true;
      else
      {
        mvstr = MoveToStrLong(mv);
        if (fen.find(mvstr) != std::string::npos)
          moveFound = true;
      }

      if (fen.find("am") != std::string::npos && fen.find("bm") == std::string::npos)
        moveFound = !moveFound;
    }

    if (e > alpha && e < beta)
    {
      alpha = e - m_rootWindow / 2;
      beta = e + m_rootWindow / 2;
    }
    else
    {
      alpha = -INFINITY_SCORE;
      beta = INFINITY_SCORE;
      m_iter--;
    }

    if (moveFound)
    {
      ++counter;
      Highlight(true);
      out(" yes ");
      Highlight(false);
    }
    else
    {
      counter = 0;
      out("  no ");
    }

    Highlight(counter >= reps);
    PrintPV();
    Highlight(false);

    if (counter >= reps)
      break;
  }

  return (counter > 0);
}

void Search::StartThinking(Position& pos)
{
  m_startTime = clock();

  std::string message;
  if (pos.IsGameOver(message))
  {
    out("%s\n", message);
    return;
  }

  m_pos = pos;
  m_hashAge = (m_hashAge + 1) & 0x0f;
  m_rootPV.clear();
  EVAL e = 0;

  m_mode = THINKING;
  m_flag = false;
  m_nodes = 0;

  // ClearHash();
  ClearHistory();
  ClearKillers();

  if (!IsUCI())
  {
    out("\n%s\n\n", m_pos.Fen());
  }

  EVAL alpha = -INFINITY_SCORE;
  EVAL beta = INFINITY_SCORE;
  Move best_move = 0;

  // Book

  std::string buf;
  Move book_move = g_book.GetMove(pos, buf);
  if (book_move)
  {
    best_move = book_move;
    if (!IsUCI())
      out(" 0 0 0 0      (%s)", buf);
    m_flag = true;
    goto MAKE_MOVE;
  }

  for (m_iter = 1; m_iter < MAX_PLY; ++m_iter)
  {
    e = AlphaBetaRoot(alpha, beta, m_iter);
    LimitKnps();

    if (m_flag)
      break;

    m_iterScore = e;
    if (e > alpha)
      m_rootPV = m_PV[0];

    if (e > alpha && e < beta)
    {
      alpha = e - m_rootWindow / 2;
      beta = e + m_rootWindow / 2;

      if (m_stSoft)
      {
        if (GetSearchTime() >= m_stSoft)
          m_flag = true;
      }
      if (m_sd)
      {
        if (m_iter >= m_sd)
          m_flag = true;
      }
    }
    else
    {
      // Additional time
      if (m_restMillisec)
      {
        if (m_restMillisec > 10 * m_stHard)
        {
          int delta = m_stHard / 2;
          m_stHard += delta;
          m_stSoft += delta;
        }
      }
 
      // Opening window
      if (e <= alpha)
        alpha = -INFINITY_SCORE;
      else
        beta = INFINITY_SCORE;
      --m_iter;
    }

    PrintPV();
    best_move = m_rootPV[0];

    if (best_move && (m_iter + e >= CHECKMATE_SCORE))
      m_flag = true;
  }

  if (m_iter == MAX_PLY && best_move)
    m_flag = true;

MAKE_MOVE:

  if (m_flag)
  {    
    if (IsUCI())
      out("\nbestmove %s\n", MoveToStrLong(best_move));
    else
    {
      Highlight(true);
      out("\nmove %s\n", MoveToStrLong(best_move));
      Highlight(false);
      pos.MakeMove(best_move);
      if (pos.IsGameOver(message))
        out("%s\n", message);
      out("\n");
    }
  }
}

void Search::StartPerft(const Position& pos, int depth)
{
  m_pos = pos;
  int t0 = clock();
  NODES sum = 0, delta = 0;

  MoveList& mvlist = m_lists[depth];
  mvlist.GenAllMoves(pos);

  out("\n");
  int i = 0;
  for (i = 0; i < mvlist.Size(); ++i)
  {
    Move mv = mvlist[i].m_mv;

    if (m_pos.MakeMove(mv))
    {
      delta = Perft(depth - 1);
      sum += delta;
      m_pos.UnmakeMove();

      out(" %s - ", MoveToStrLong(mv));
      out("%d\n", delta);
      if (InputAvailable())
      {
        char s[256];
        ReadInput(s, sizeof(s));
        if (Is(std::string(s), "exit", 2))
          break;
      }
    }
  }

  if (i == mvlist.Size())
  {
    int t1 = clock();
    double dt = (t1 - t0) / 1000.;

    out("\n Nodes: %d\n", sum);
    out(" Time:  %f\n", dt);
    out(" Knps:  %f\n\n", sum / dt / 1000.);
  }
}

static const EVAL SEE_VALUE[14] =
{ 0, 0, 100, 100, 400, 400, 400, 400, 600, 600, 1200, 1200, 20000, 20000 };

EVAL Search::SEE_Swap(const Position& pos, FLD to, EVAL currScore, PIECE currPiece, U64 mask, COLOR side)
{
  FLD from = pos.FindLeastAttacker(to, side, mask);
  if (from != NF)
  {
      EVAL newScore = -SEE_Swap(pos, 
                                to, 
                                -currScore - SEE_VALUE[currPiece], 
                                pos[from], 
                                mask ^ BB_SINGLE[from], 
                                side ^ 1);
       if (newScore > currScore) currScore = newScore;
  }
  return currScore;
}

EVAL Search::SEE(const Position& pos, Move mv)
{
  FLD from = mv.From();
  FLD to = mv.To();
  PIECE piece = mv.Piece();
  PIECE captured = mv.Captured();
  PIECE promotion = mv.Promotion();
  COLOR side = GetColor(piece);
  U64 mask = pos.BitsAll() ^ BB_SINGLE[from];

  PIECE initPiece = piece;
  EVAL initScore = SEE_VALUE[captured];
  if (promotion)
  {
      initPiece = promotion;
      initScore += SEE_VALUE[promotion] - SEE_VALUE[piece];
  }

  return -SEE_Swap(pos, to, -initScore, initPiece, mask, side ^ 1);
}

int Search::MvvLva(Move mv)
{
	int p = mv.Piece() / 2 - 1;    // 0 - 5
	int c = mv.Captured() / 2 - 1; // 0 - 4

	static const int score[5][6] =
	{
		{  6,  5,  4,  3,  2,  1 },
		{ 12, 11, 10,  9,  8,  7 },
		{ 18, 17, 16, 15, 14, 13 },
		{ 24, 23, 22, 21, 20, 19 },
		{ 30, 29, 28, 27, 26, 25 },
	};

	return score[c][p];
}

void Search::UpdateScores(MoveList& mvlist, Move hashmv, int ply)
{
  for (int i = 0; i < mvlist.Size(); ++i)
  {
    Move mv = mvlist[i].m_mv;
    mvlist[i]._value = 0;

    if (mv == hashmv)
      mvlist[i]._value = SORT_HASH;
    else if (mv.Captured())
      mvlist[i]._value = SORT_CAPTURE + MvvLva(mv);
    else if (mv == m_matekillers[ply])
      mvlist[i]._value = SORT_MATEKILLER;
    else if (mv == m_killers[ply])
      mvlist[i]._value = SORT_KILLER;
    else
    {
      PIECE piece = mv.Piece();
      FLD to = mv.To();
      if (m_histTry[piece][to])
        mvlist[i]._value = 100 * m_histSuccess[piece][to] / m_histTry[piece][to];
    }
  }
}

void Search::UpdateScoresQ(MoveList& mvlist)
{
  for (int i = 0; i < mvlist.Size(); ++i)
  {
    Move mv = mvlist[i].m_mv;
    mvlist[i]._value = 0;

    if (mv.Captured())
      mvlist[i]._value = SORT_CAPTURE + MvvLva(mv);
    else
    {
      PIECE piece = mv.Piece();
      FLD to = mv.To();
      if (m_histTry[piece][to])
        mvlist[i]._value = 100 * m_histSuccess[piece][to] / m_histTry[piece][to];
    }
  }
}
