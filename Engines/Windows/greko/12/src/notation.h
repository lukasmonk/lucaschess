//  GREKO Chess Engine
//  (c) 2002-2015 Vladimir Medvedev <vrm@bk.ru>
//  http://greko.su

//  notation.h: full and short algebraic notation
//  modified: 01-Mar-2013

#ifndef NOTATION_H
#define NOTATION_H

#include "position.h"

std::string FldToStr(FLD f);
std::string MoveToStrLong(Move mv);
std::string MoveToStrShort(Move mv, Position& pos);
FLD StrToFld(const std::string& str);
Move StrToMove(const std::string& str, Position& pos);

#endif
