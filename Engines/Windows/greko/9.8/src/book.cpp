//  GREKO Chess Engine
//  (c) 2002-2012 Vladimir Medvedev <vrm@bk.ru>
//  http://greko.110mb.com

//  book.cpp: opening book
//  modified: 31-Dec-2012

#include "book.h"
#include "notation.h"
#include "utils.h"

struct MoveAndValue
{
  MoveAndValue(Move mv, int value) : m_mv(mv), _value(value) {}
  ~MoveAndValue() {}

  bool operator< (const MoveAndValue& m) const
  { 
    return _value > m._value; // reverse order!
  }

  Move m_mv;
  int  _value;
};

Move Book::GetMove(const Position& pos, std::string& comment)
{
  comment.clear();

  // check to avoid lines like 1. e4 e5 2. Nf3 a6 3. Bb5 Nc6?
  if (m_data.find(pos.Hash()) == m_data.end()) return 0;

  m_pos = pos;
  std::vector<MoveAndValue> x;

  MoveList mvlist;
  mvlist.GenAllMoves(m_pos);

  int sumVal = 0;
  for (int i = 0; i < mvlist.Size(); ++i)
  {
    Move mv = mvlist[i].m_mv;
    if (m_pos.MakeMove(mv))
    {
      std::map<U64, int>::const_iterator it = m_data.find(m_pos.Hash());
      if (it != m_data.end() && it->second > 0)
      {
        x.push_back(MoveAndValue(mv, it->second));
        sumVal += it->second;
      }
      m_pos.UnmakeMove();
    }
  }

  if (sumVal > 0)
  {
    sort(x.begin(), x.end());
    for (size_t i = 0; i < x.size(); ++i)
    {
      char buf[256];
      sprintf(buf, "%s %d%c", MoveToStrShort(x[i].m_mv, m_pos).c_str(), 100 * x[i]._value / sumVal, '%');
      comment += buf;
      if (i != x.size() - 1)
        comment += ", ";      
    }

    int N = int(Rand64() % sumVal);
    for (size_t i = 0; i < x.size(); ++i)
    {
      N -= x[i]._value;
      if (N <= 0)
        return x[i].m_mv;
    }
  }

  return 0;
}

bool Book::Import(const std::string& strPath, const std::string& strMaxPly, const std::string& strColor)
{ 
  FILE* src = fopen(strPath.c_str(), "rt");
  if (!src)
  {
    out("can't open %s\n", strPath);
    return false;
  }

  int maxPly = strMaxPly.empty() ? 20 : atoi(strMaxPly.c_str());
  out("maxPly = %d\n", maxPly);

  bool addColor[2] = {true, true};
  if (strColor.size() > 0)
  {
    if (strColor[0] == 'w')
    {
      addColor[BLACK] = false;
      out("white's moves only\n");
    }
    else if (strColor[0] == 'b')
    {
      addColor[WHITE] = false;
      out("black's moves only\n");
    }
  }
  
  Position startpos;
  startpos.SetInitial();

  int nGames = 0;
  char buf[4096];
  while (fgets(buf, sizeof(buf), src))
  {
    if (strlen(buf) < 2)
      continue;
    if (buf[0] == '[')
      continue;

    TokenString s(buf);
    for (std::string token = s.GetToken(); token.length() > 0; token = s.GetToken())
    {
      if (token == "1." || token == "1")
      {
        m_pos = startpos;
        ++nGames;
        printf("Games: %d, nodes: %d\r", nGames, m_data.size());
        continue;
      }

      if (m_pos.Ply() >= maxPly)
        continue;

      Move mv = StrToMove(token, m_pos);
      if (mv)
      {
        m_pos.MakeMove(mv);

        if (addColor[m_pos.Side() ^ 1])
          ++m_data[m_pos.Hash()];
        else
          m_data[m_pos.Hash()] += 0;
      }
    }
  }

  m_data.insert(std::pair<U64, int>(startpos.Hash(), 1));

  printf("Games: %d, nodes: %d\n", nGames, m_data.size());
  fclose(src);
  return true;
}

void Book::Init()
{
  Clean();

  if (Load("book.bin"))
    ;
  else
  { 
    FILE* src = fopen("book.txt", "rt");
    if (src)
    {
      out("reading book.txt...\n");
      char buf[256];
      while (fgets(buf, sizeof(buf), src))
      {
        m_pos.SetInitial();
        ProcessLine(buf);
      }
      fclose (src);
      out("book.txt: %d nodes\n", m_data.size());
    }
    else
    {
      out("book.txt not found\n");
    }
  } 
}

bool Book::Load(const std::string& path)
{
  FILE* srcBin = fopen(path.c_str(), "rb");
  if (srcBin)
  {
    U64 hash;
    int be;

    while (fread(&hash, sizeof(U64), 1, srcBin))
    {
      fread(&be, sizeof(int), 1, srcBin);
      m_data[hash] += be;
    }
    fclose(srcBin);
    out("%s: %d nodes\n", path.c_str(), m_data.size());
    return true;
  }
  else
  {
    out("can't open %s\n", path);
    return false;
  }
}

void Book::ProcessLine(const std::string& str)
{
  TokenString s(str);
  for (std::string token = s.GetToken(); token.length() > 0; token = s.GetToken())
  {
    Move mv = StrToMove(token, m_pos);
    if (mv)
    {
      m_pos.MakeMove(mv);
      ++m_data[m_pos.Hash()];
    }
  }
}

bool Book::Save(const std::string& path)
{
  FILE* dest = fopen(path.c_str(), "wb");
  if (dest)
  {
    if (!m_data.empty())
    {
      out("writing %s...\n", path);
    }
    for (std::map<U64, int>::const_iterator it = m_data.begin(); it != m_data.end(); ++it)
    {
      U64 hash = it->first;
      int be = it->second;
      fwrite(&hash, sizeof(U64), 1, dest);
      fwrite(&be, sizeof(int), 1, dest);
    }
    fclose(dest);
    return true;
  }

  return false;
}
