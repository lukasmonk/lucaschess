//  GREKO Chess Engine
//  (c) 2002-2012 Vladimir Medvedev <vrm@bk.ru>
//  http://greko.110mb.com

//  main.cpp: initialize and start engine, command line interface
//  modified: 31-Dec-2012

#include "book.h"
#include "config.h"
#include "eval.h"
#include "moves.h"
#include "notation.h"
#include "search.h"
#include "utils.h"

const std::string VERSION = "GreKo 9.8";
const std::string RELEASE_DATE = "31-Dec-2012";

Position g_pos;
Book g_book;

PROTOCOL_T g_protocol = CONSOLE;
std::list<std::string> g_commandQueue;
FILE *g_log = NULL;

class Engine
{
public:
  Engine() : _force(false), _uciLimitStrength(false), _uciElo(2600) {}

private:  
  Search      _search;
  bool        _force;
  bool        _run;
  TokenString _line;
  bool        _uciLimitStrength;
  int         _uciElo;

  std::string GetToken() { return _line.GetToken(); }

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
    _search.Epdtest(psrc, tm, reps);
    fclose(psrc);
  }

  void OnEval()
  {
    EVAL e = Evaluate(g_pos, -INFINITY_SCORE, INFINITY_SCORE);
    out("eval = %d\n", e);
  }
  
  void OnFEN() { out("%s\n\n", g_pos.Fen()); }

  void OnGo()
  {
    for (std::string token = GetToken(); token.length() > 0; token = GetToken())
    {
      if (token == "infinite")
        _search.SetLimits(0, 0, 0, 0, 0);
      else if (token == "depth")
        _search.SetLimits(0, atol(GetToken().c_str()), 0, 0, 0);
      else if (token == "nodes")
        _search.SetLimits(0, 0, atol(GetToken().c_str()), 0, 0);
      else if (token == "movetime")
      {
        token = GetToken();
        _search.SetLimits(atol(token.c_str()), 0, 0, atol(token.c_str()), atol(token.c_str()));
      }
      else if (token == "wtime" && g_pos.Side() == WHITE)
        SetTimeLimits(atoi(GetToken().c_str()));
      else if (token == "btime" && g_pos.Side() == BLACK)
        SetTimeLimits(atoi(GetToken().c_str()));
      else if(token == "winc" && g_pos.Side() == WHITE)        
        _search.SetIncrement(atoi(GetToken().c_str())); // in UCI time comes in milliseconds
      else if(token == "binc" && g_pos.Side() == BLACK)
        _search.SetIncrement(atoi(GetToken().c_str())); // in UCI time comes in milliseconds
    }

    _force = false;
    _search.StartThinking(g_pos);
  }

  void OnLevel()
  {
    GetToken(); // mps
    GetToken(); // base
    int inc = atoi(GetToken().c_str()); // inc
    _search.SetIncrement(1000 * inc); // in WB increment comes in seconds
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
      out("Can't open file: %s\n", path);
      return;
    }
    
    std::string tmp = GetToken();
    int lineNum = tmp.empty()? 1 : atoi(tmp.c_str());

    char buf[256];
    for (int i = 0; i < lineNum; ++i)
    {
      fgets(buf, sizeof(buf), psrc);
      if (buf[strlen(buf) - 1] == '\n')
        buf[strlen(buf) - 1] = 0;
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
      out("Can't open file: %s\n", path);
      return;
    }

    Position tmp = g_pos;
    char buf[256];
    while (fgets(buf, sizeof(buf), psrc))
    {
      if (buf[strlen(buf) - 1] == '\n' || buf[strlen(buf) - 1] == '\r')
        buf[strlen(buf) - 1] = 0;

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
    _force = false;
    g_pos.SetInitial();
  }

  void OnPerft()
  {
    std::string token = GetToken();
    _search.StartPerft(g_pos, atoi(token.c_str()));
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
    out("option name Hash type spin default 1 min 1 max 1024\n");
    out("option name MultiPV type spin default 1 min 1 max 64\n");
    out("option name UCI_LimitStrength type check default false\n");
    out("option name UCI_Elo type spin default 2000 min 1600 max 2400\n");
    out("option name LimitKnps type spin default 9999 min 1 max 9999\n");
    out("option name Material type spin default 50 min 0 max 100\n");
    out("option name BoardControl type spin default 50 min 0 max 100\n");
    out("option name Mobility type spin default 50 min 0 max 100\n");
    out("option name PawnStruct type spin default 50 min 0 max 100\n");
    out("option name PawnPassed type spin default 50 min 0 max 100\n");
    out("option name KingSafety type spin default 50 min 0 max 100\n");
    out("option name RandomEval type spin default 0 min 0 max 1200\n");
    out("uciok\n");
  }

  void OnSD()
  {
    int sd = atoi(GetToken().c_str());
    _search.SetLimits(0, sd, 0, 0, 0);
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

    if (key == "MultiPV")
      _search.SetMultiPV(atoi(value.c_str()));
    else if (key == "Hash")
      _search.SetHashMB(atof(value.c_str()));
    else if (key == "UCI_LimitStrength")
      _uciLimitStrength = (value != "false" && value != "0");
    else if (key == "UCI_Elo")
    {
      if (_uciLimitStrength)
      {
        _uciElo = atoi(value.c_str());
        double knps = pow(10, (_uciElo - 1600) / 400.);
        out("knps=%f\n", knps);
        _search.SetKnps(knps);
      }
    }
    else if (key == "LimitKnps")
    {
      if (!_uciLimitStrength)
        _search.SetKnps(atof(value.c_str()));
    }
    else if (key == "Material")
      g_evalWeights.Material = atoi(value.c_str());
    else if (key == "BoardControl")
      g_evalWeights.BoardControl = atoi(value.c_str());
    else if (key == "Mobility")
      g_evalWeights.Mobility = atoi(value.c_str());
    else if (key == "PawnStruct")
      g_evalWeights.PawnStruct = atoi(value.c_str());
    else if (key == "PawnPassed")
      g_evalWeights.PawnPassed = atoi(value.c_str());
    else if (key == "KingSafety")
      g_evalWeights.KingSafety = atoi(value.c_str());
    else if (key == "RandomEval")
      g_evalWeights.RandomEval = atoi(value.c_str());
  }

  void OnSN()
  {
    NODES sn = atol(GetToken().c_str());
    _search.SetLimits(0, 0, sn, 0, 0);
  }

  void OnST()
  {
    int dt = int(1000 * atof(GetToken().c_str()));
    _search.SetLimits(dt, 0, 0, dt, dt);
  }

  void OnTime()
  {
    SetTimeLimits(10 * atoi(GetToken().c_str())); // in WB time comes in centi-seconds
  }

public:
  
  void RunCommandLine()
  {
    out("\n");
    _run = true;

    while (_run)
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
        _line = TokenString(buf);
      }
      else
      {
        _line = TokenString(g_commandQueue.front());
        g_commandQueue.pop_front();
      }

      std::string cmd = GetToken();
      if (cmd.empty())
        continue;

      Move mv = StrToMove(cmd, g_pos);
      if (mv)
      {
        g_pos.MakeMove(mv);
        if (!_force)
          _search.StartThinking(g_pos);
        continue;
      }

#define ON_CMD(pattern, minLen, action)  \
  if (Is(cmd, #pattern, minLen))         \
      {                                  \
      action;                            \
      continue;                          \
      }

      ON_CMD (analyze,   2, _search.StartAnalyze(g_pos))
      ON_CMD (bk,        2, OnBk())
      ON_CMD (board,     1, g_pos.Print())
      ON_CMD (book,      4, OnBook())
      ON_CMD (epdtest,   3, OnEpdtest())
      ON_CMD (eval,      1, OnEval())
      ON_CMD (fen,       3, OnFEN())
      ON_CMD (force,     5, _force = true)
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
      ON_CMD (quit,      1, _run = false)
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
        out("Unknown command: %s\n", _line.Str());
    }
  }

  void SetTimeLimits(int restMillisec)
  {
    // all values in milliseconds
    int dt = restMillisec / 20 + _search.GetIncrement();
    if (dt > restMillisec - 1000)
      dt = restMillisec / 20;
    int stHard = dt;
    int stSoft = dt / 2;
    _search.SetLimits(restMillisec, 0, 0, stHard, stSoft);
  }

  void Init()
  {
    Config config("GreKo.ini");
    if (config.GetInt("WriteLog", 0))
      g_log = fopen("greko.log", "at");

    InitInput();
    InitBitboards();
    Position::InitHashNumbers();

    _search.SetHashMB(config.GetDouble("HashMB", 32.0));
    _search.SetKnps(config.GetDouble("LimitKnps", 999999));

    g_evalWeights.Material = config.GetInt("Material", 50);
    g_evalWeights.BoardControl = config.GetInt("BoardControl", 50);
    g_evalWeights.Mobility = config.GetInt("Mobility", 50);
    g_evalWeights.PawnStruct = config.GetInt("PawnStruct", 50);
    g_evalWeights.PawnPassed = config.GetInt("PawnPassed", 50);
    g_evalWeights.KingSafety = config.GetInt("KingSafety", 50);
	g_evalWeights.RandomEval = config.GetInt("RandomEval", 0);

    g_book.Init();
    RandSeed32(U32(time(0)));

    g_pos.SetInitial();
    InitEval();
  }
};

int main()
{
  Highlight(true);
  out("\n%s (%s)\n\n", VERSION.c_str(), RELEASE_DATE.c_str());
  Highlight(false);

  Engine e;
  e.Init();
  e.RunCommandLine();
  return 0;
}
