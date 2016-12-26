//  GREKO Chess Engine
//  (c) 2002-2015 Vladimir Medvedev <vrm@bk.ru>
//  http://greko.su

//  notation.cpp: full and short algebraic notation
//  modified: 01-Mar-2013

#include "moves.h"
#include "notation.h"

std::string FldToStr(FLD f)
{
	if (f == NF)
		return "-";

	char buf[3];
	buf[0] = static_cast<char>('a' + Col(f));
	buf[1] = static_cast<char>('8' - Row(f));
	buf[2] = 0;

	return std::string(buf);
}

std::string MoveToStrLong(Move mv)
{
	std::string s = FldToStr(mv.From()) + FldToStr(mv.To());
	switch (mv.Promotion())
	{
	case QW: case QB: s += "q"; break;
	case RW: case RB: s += "r"; break;
	case BW: case BB: s += "b"; break;
	case NW: case NB: s += "n"; break;
	default: break;
	}
	return s;
}

std::string MoveToStrShort(Move mv, Position& pos)
{
	if (mv == Move(E1, G1, KW) || mv == Move(E8, G8, KB))
		return "O-O";
	if (mv == Move(E1, C1, KW) || mv == Move(E8, C8, KB))
		return "O-O-O";

	PIECE piece = mv.Piece();
	FLD from = mv.From();
	FLD to = mv.To();
	PIECE captured = mv.Captured();
	PIECE promotion = mv.Promotion();
	std::string strFrom, strTo, strPiece, strCaptured, strPromotion;

	switch (piece)
	{
	case PW: case PB:
		if (captured)
			strFrom = FldToStr(from).substr(0, 1);
		break;
	case NW: case NB: strPiece = "N"; break;
	case BW: case BB: strPiece = "B"; break;
	case RW: case RB: strPiece = "R"; break;
	case QW: case QB: strPiece = "Q"; break;
	case KW: case KB: strPiece = "K"; break;
	default: break;
	}

	if (captured)
		strCaptured = "x";

	strTo = FldToStr(to);

	switch (promotion)
	{
	case QW: case QB: strPromotion = "=Q"; break;
	case RW: case RB: strPromotion = "=R"; break;
	case BW: case BB: strPromotion = "=B"; break;
	case NW: case NB: strPromotion = "=N"; break;
	default: break;
	}

	// resolve ambiguity

	int uniq_col = 1;
	int uniq_row = 1;
	bool ambiguity = false;

	int row0 = Row(from);
	int col0 = Col(from);

	MoveList mvlist;
	mvlist.GenAllMoves(pos);

	for (int i = 0; i < mvlist.Size(); ++i)
	{
		Move mvi = mvlist[i].m_mv;

		if (mvi.To() != to)
			continue;
		if (mvi.Piece() != piece)
			continue;
		if (mvi.From() == from)
			continue;
		if (!pos.MakeMove(mvi))
			continue;
		pos.UnmakeMove();

		ambiguity = true; // two or more pieces of the same type can move to field
		int row1 = Row(mvi.From());
		int col1 = Col(mvi.From());

		if (row0 == row1)
			uniq_row = 0;

		if (col0 == col1)
			uniq_col = 0;
	}

	if (ambiguity)
	{
		if (uniq_col)
			strFrom = FldToStr(from).substr(0, 1);
		else if (uniq_row)
			strFrom = FldToStr(from).substr(1, 1);
		else
			strFrom = FldToStr(from);
	}

	return strPiece + strFrom + strCaptured + strTo + strPromotion;
}

FLD StrToFld(const std::string& s)
{
	if (s.length() != 2)
		return NF;

	int col = s[0] - 'a';
	int row = 7 - (s[1] - '1');

	if (col < 0 || col > 7 || row < 0 || row > 7)
		return NF;

	return static_cast<FLD>(8 * row + col);
}

Move StrToMove(const std::string& str, Position& pos)
{
	for (std::string::const_iterator it = str.begin(); it != str.end(); ++it)
	{
		if (isdigit(*it) || *it == 'O')
			break;
		if (*it == ' ' || *it == 0)
			return 0;
	}

	MoveList mvlist;
	mvlist.GenAllMoves(pos);

	for (int n = 0; n < mvlist.Size(); ++n)
	{
		Move mv = mvlist[n].m_mv;
		if (MoveToStrLong(mv) == str)
			return mv;
	}

	for (int n = 0; n < mvlist.Size(); ++n)
	{
		Move mv = mvlist[n].m_mv;
		if (MoveToStrShort(mv, pos) == str)
			return mv;
	}

	return 0;
}
