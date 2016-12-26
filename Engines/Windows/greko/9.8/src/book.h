//  GREKO Chess Engine
//  (c) 2002-2012 Vladimir Medvedev <vrm@bk.ru>
//  http://greko.110mb.com

//  book.h: opening book
//  modified: 31-Dec-2012

#pragma once

#include <map>
#include <vector>
#include "moves.h"

class Book
{
public:
  void Clean() 
  { 
    m_data.clear();
    m_pos.SetInitial();
    ++m_data[m_pos.Hash()];
  }

  Move GetMove(const Position& pos, std::string& comment);
  bool Import(const std::string& strPath, const std::string& strMaxPly, const std::string& strColor);
  void Init();
  bool Load(const std::string& path);
  bool Save(const std::string& path);

private:
  void ProcessLine(const std::string& str);

  std::map<U64, int> m_data;
  Position m_pos;
};
