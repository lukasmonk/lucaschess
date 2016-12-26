//  GREKO Chess Engine
//  (c) 2002-2012 Vladimir Medvedev <vrm@bk.ru>
//  http://greko.110mb.com

//  notation.h: full and short algebraic notation
//  modified: 31-Dec-2012

#pragma once

#include "position.h"

std::string FldToStr(FLD f);
std::string MoveToStrLong(Move mv);
std::string MoveToStrShort(Move mv, Position& pos);
FLD StrToFld(const std::string& str);
Move StrToMove(const std::string& str, Position& pos);