//  GREKO Chess Engine
//  (c) 2002-2012 Vladimir Medvedev <vrm@bk.ru>
//  http://greko.110mb.com

//  moves.h: bitboard moves generator
//  modified: 12-June-2012

#pragma once

#include "position.h"

struct MoveEntry
{
  MoveEntry() {}
  MoveEntry(Move mv) : m_mv(mv) {}
  Move m_mv;
  EVAL _value;
};

class MoveList
{
public:

  MoveList() : m_size(0) {}

  MoveEntry& operator[] (int n) { return m_data[n]; }
  const MoveEntry& operator[] (int n) const { return m_data[n]; }

  void Clear() { m_size = 0; }
  void GenAllMoves(const Position& pos);
  void GenCaptures(const Position& pos, bool genChecks);
  void GenCheckEvasions(const Position& pos);
  Move GetNthBest(int n);
  int  Size() const { return m_size; }
  
private:

  void Add(FLD from, FLD to, PIECE piece)
  {
    m_data[m_size++].m_mv = Move(from, to, piece);
  }

  void Add(FLD from, FLD to, PIECE piece, PIECE captured)
  {
    m_data[m_size++].m_mv = Move(from, to, piece, captured);
  }

  void Add(FLD from, FLD to, PIECE piece, PIECE captured, PIECE promotion)
  {
    m_data[m_size++].m_mv = Move(from, to, piece, captured, promotion);
  }

  enum { MAX_SIZE = 256 };
  MoveEntry m_data[MAX_SIZE];
  int m_size;
};
