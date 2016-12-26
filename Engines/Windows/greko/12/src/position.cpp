//  GREKO Chess Engine
//  (c) 2002-2015 Vladimir Medvedev <vrm@bk.ru>
//  http://greko.su

//  position.cpp: position and game representation
//  modified: 01-Mar-2013

#include "eval.h"
#include "moves.h"
#include "notation.h"
#include "utils.h"

U64 Position::s_hashSide[2];
U64 Position::s_hash[64][14];
U32 Position::s_hashPawn[64][14];

void Position::Clear()
{
	m_castlings = 0;
	m_ep = NF;
	m_fifty = 0;
	m_hash = 0;
	m_hashPawn = 0;
	m_Kings[WHITE] = m_Kings[BLACK] = 0;
	m_material[WHITE] = m_material[BLACK] = 0;
	m_matIndex[WHITE] = m_matIndex[BLACK] = 0;
	m_ply = 0;
	m_side = WHITE;
	m_undoSize = 0;

	for (int f = 0; f < 64; f++)
	{
		m_board[f] = NOPIECE;
	}

	for (int p = 0; p < 14; p++)
	{
		m_bits[p] = 0;
		m_count[p] = 0;
	}
	m_bitsAll[WHITE] = m_bitsAll[BLACK] = 0;
}

bool Position::IsGameOver(std::string& message)
{
	bool hasMoves = false;
	MoveList mvlist;
	mvlist.GenAllMoves(*this);
	for (int i = 0; i < mvlist.Size(); ++i)
	{
		if (MakeMove(mvlist[i].m_mv))
		{
			hasMoves = true;
			UnmakeMove();
			break;
		}
	}

	if (hasMoves == false)
	{
		if (InCheck())
			message = (m_side == WHITE)? "0-1 {Black mates}" : "1-0 {White mates}";
		else
			message = "1/2-1/2 {Draw: stalemate}";
		return true;
	}

	if (EstimateDraw(*this) == EVAL_THEORETICAL_DRAW)
	{
		message = "1/2-1/2 {Draw: material}";
		return true;
	}

	if (GetRepetitions() >= 3)
	{
		message = "1/2-1/2 {Draw: 3rd repetition}";
		return true;
	}
	return false;
}

U64 Position::GetAttacks(FLD to, COLOR side, U64 occ) const
{
	U64 att = 0;

	att |= Bitboard::PawnAttacks(to, side ^ 1) & Bits(PW | side);
	att |= Bitboard::KnightAttacks(to) & Bits(NW | side);
	att |= Bitboard::KingAttacks(to) & Bits(KW | side);
	att |= Bitboard::BishopAttacks(to, occ) & (Bits(BW | side) | Bits(QW | side));
	att |= Bitboard::RookAttacks(to, occ) & (Bits(RW | side) | Bits(QW | side));
	return att;
}

std::string Position::Fen() const
{
	const std::string names = "-?PpNnBbRrQqKk";
	std::string fen;
	int empty = 0;
	for (int f = 0; f < 64; ++f)
	{
		PIECE p = m_board[f];
		if (p)
		{
			if (empty)
			{
				char t1[3];
				sprintf(t1, "%d", empty);
				fen += std::string(t1);
				empty = 0;
			}
			fen += names.substr(p, 1);
		}
		else
			++empty;

		if (Col(f) == 7)
		{
			if (empty)
			{
				char t1[3];
				sprintf(t1, "%d", empty);
				fen += std::string(t1);
				empty = 0;
			}
			if (f != 63)
				fen += "/";
		}
	}

	if (m_side == WHITE)
		fen += " w";
	else
		fen += " b";

	if (m_castlings & 0x33)
	{
		fen += " ";
		if (m_castlings & WHITE_O_O)
			fen += "K";
		if (m_castlings & WHITE_O_O_O)
			fen += "Q";
		if (m_castlings & BLACK_O_O)
			fen += "k";
		if (m_castlings & BLACK_O_O_O)
			fen += "q";
	}
	else
		fen += " -";

	if (m_ep == NF)
		fen += " -";
	else
	{
		fen += " ";
		fen += FldToStr(m_ep);
	}
	return fen;
}

int Position::GetRepetitions() const
{
	int total = 1;
	for (int i = m_undoSize - 1; i >= 0; --i)
	{
		if (m_undos[i].m_hash == m_hash) ++total;
		if (m_undos[i].m_mv == 0) return 0;
		if (m_undos[i].m_mv.Captured()) break;
		if (m_undos[i].m_mv.Piece() == PW) break;
		if (m_undos[i].m_mv.Piece() == PB) break;
	}
	return total;
}

void Position::InitHashNumbers()
{
	RandSeed32(42);
	for (int f = 0; f < 64; f++)
	{
		for (int p = 0; p < 14; p++)
		{
			s_hash[f][p] = Rand64();
			s_hashPawn[f][p] = (p == PW || p == PB)? Rand32() : 0;
		}
	}
	s_hashSide[0] = 0;
	s_hashSide[1] = Rand64();
}

bool Position::IsAttacked(FLD to, COLOR side) const
{
	if (Bitboard::PawnAttacks(to, side ^ 1) & Bits(PW | side)) return true;
	if (Bitboard::KnightAttacks(to) & Bits(NW | side)) return true;
	if (Bitboard::KingAttacks(to) & Bits(KW | side)) return true;

	U64 occ = m_bitsAll[WHITE] | m_bitsAll[BLACK];
	U64 x = Bitboard::BishopAttacks(to) & (Bits(QW | side) | Bits(BW | side));
	while (x)
	{
		FLD from = Bitboard::PopLSB(x);
		if ((Bitboard::Between(from, to) & occ) == 0) return true;
	}
	x = Bitboard::RookAttacks(to) & (Bits(QW | side) | Bits(RW | side));
	while (x)
	{
		FLD from = Bitboard::PopLSB(x);
		if ((Bitboard::Between(from, to) & occ) == 0) return true;
	}
	return false;
}

bool Position::MakeMove(Move mv)
{
	Undo& undo = m_undos[m_undoSize++];
	undo.m_castlings = m_castlings;
	undo.m_ep = m_ep;
	undo.m_fifty = m_fifty;
	undo.m_hash = m_hash;
	undo.m_mv = mv;

	FLD from = mv.From();
	FLD to = mv.To();
	PIECE piece = mv.Piece();
	PIECE captured = mv.Captured();
	PIECE promotion = mv.Promotion();

	COLOR side = m_side;
	COLOR opp = side ^ 1;
	m_hash ^= s_hashSide[1];

	++m_fifty;
	if (captured)
	{
		m_fifty = 0;
		if (to == m_ep)
			Remove(to + 8 - 16 * side);
		else
			Remove(to);
	}
	Remove(from);
	Put(to, piece);
	m_ep = NF;

	switch (piece)
	{
	case PW:
	case PB:
		m_fifty = 0;
		if (to - from == -16 + 32 * side)
			m_ep = to + 8 - 16 * side;
		else if (promotion)
		{
			Remove(to);
			Put(to, promotion);
		}
		break;
	case KW:
		m_Kings[WHITE] = to;
		if (from == E1)
		{
			if (to == G1)
			{
				Remove(H1);
				Put(F1, RW);
				m_castlings ^= WHITE_DID_O_O;
			}
			else if (to == C1)
			{
				Remove(A1);
				Put(D1, RW);
				m_castlings ^= WHITE_DID_O_O_O;
			}
		}
		break;
	case KB:
		m_Kings[BLACK] = to;
		if (from == E8)
		{
			if (to == G8)
			{
				Remove(H8);
				Put(F8, RB);
				m_castlings ^= BLACK_DID_O_O;
			}
			else if (to == C8)
			{
				Remove(A8);
				Put(D8, RB);
				m_castlings ^= BLACK_DID_O_O_O;
			}
		}
		break;
	default:
		break;
	}

	static const U8 castlingDelta[64] =
	{
		0xf7, 0xff, 0xff, 0xff, 0xf3, 0xff, 0xff, 0xfb,
		0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
		0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
		0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
		0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
		0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
		0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
		0xfd, 0xff, 0xff, 0xff, 0xfc, 0xff, 0xff, 0xfe
	};

	m_castlings &= castlingDelta[from];
	m_castlings &= castlingDelta[to];

	++m_ply;
	m_side ^= 1;

	if (IsAttacked(m_Kings[side], opp))
	{
		UnmakeMove();
		return false;
	}
	return true;
}

void Position::MakeNullMove()
{
	Undo& undo = m_undos[m_undoSize++];
	undo.m_castlings = m_castlings;
	undo.m_ep = m_ep;
	undo.m_fifty = m_fifty;
	undo.m_mv = 0;

	m_ep = NF;
	m_side ^= 1;
	++m_ply;
	m_hash ^= s_hashSide[1];
}

void Position::Mirror()
{
	Position old = *this;
	Clear();
	for (int f = 0; f < 64; f++)
	{
		PIECE p = old[f];
		if (p == NOPIECE) continue;
		Put(FLIP[f], p ^ 1);
		if (p == KW)
			m_Kings[BLACK] = FLIP[f];
		else if (p == KB)
			m_Kings[WHITE] = FLIP[f];
	}
	m_side = old.Side() ^ 1;
	m_fifty = old.Fifty();
	if (old.Ep() != NF) m_ep = FLIP[old.Ep()];
	m_ply = old.Ply();
	int bit0 = old.m_castlings & 0x01;
	int bit1 = (old.m_castlings & 0x02) >> 1;
	int bit2 = (old.m_castlings & 0x04) >> 2;
	int bit3 = (old.m_castlings & 0x08) >> 3;
	int bit4 = (old.m_castlings & 0x10) >> 4;
	int bit5 = (old.m_castlings & 0x20) >> 5;
	int bit6 = (old.m_castlings & 0x40) >> 6;
	int bit7 = (old.m_castlings & 0x80) >> 7;
	m_castlings = static_cast<U8>((bit5 << 7) | (bit4 << 6) | (bit7 << 5) | (bit6 << 4) | (bit1 << 3) | (bit0 << 2) | (bit3 << 1) | (bit2 << 0));
}

void Position::Print() const
{
	const char names[] = "-?PpNnBbRrQqKk";
	out("\n");
	for (int f = 0; f < 64; f++)
	{
		PIECE p = m_board[f];
		Highlight (p && GetColor(p) == WHITE);
		out(" %c", names[p]);
		Highlight(false);
		if (Col(f) == 7) out("\n");
	}
	if (m_undoSize)
	{
		out("\n ");
		for (int m = 0; m < m_undoSize; ++m)
			out("%s ", MoveToStrLong(m_undos[m].m_mv));
		out("\n ");
	}
	out("\n");
}

bool Position::SetFen(const std::string& fen)
{
	if (fen.length() < 5) return 0;
	Position tmp = *this;
	Clear();
	TokenString s(fen);
	std::string token = s.GetToken();
	FLD f = A8;
	for (size_t i = 0; i < token.length(); ++i)
	{
		PIECE piece = NOPIECE;
		switch (token[i])
		{
		case 'P': piece = PW; break;
		case 'p': piece = PB; break;
		case 'N': piece = NW; break;
		case 'n': piece = NB; break;
		case 'B': piece = BW; break;
		case 'b': piece = BB; break;
		case 'R': piece = RW; break;
		case 'r': piece = RB; break;
		case 'Q': piece = QW; break;
		case 'q': piece = QB; break;
		case 'K': piece = KW; break;
		case 'k': piece = KB; break;
		case '1': case '2': case '3': case '4':
		case '5': case '6': case '7': case '8':
			f += static_cast<U8>(token[i] - '0');
			break;
		case '/':
			if (Col(f) != 0) f = static_cast<U8>(8 * (Row(f) + 1));
			break;
		default:
			goto ILLEGAL_FEN;
		}
		if (piece)
		{
			if (f >= 64) goto ILLEGAL_FEN;
			Put(f, piece);
			if (piece == KW)
				m_Kings[WHITE] = f;
			else if (piece == KB)
				m_Kings[BLACK] = f;
			++f;
		}
	}
	token = s.GetToken();
	if (token.empty()) goto FINAL_CHECK;
	if (token[0] == 'w')
		m_side = WHITE;
	else if (token[0] == 'b')
		m_side = BLACK;
	else
		goto ILLEGAL_FEN;
	m_hash ^= s_hashSide[m_side];
	token = s.GetToken();
	if (token.empty()) goto FINAL_CHECK;
	for (size_t i = 0; i < token.length(); ++i)
	{
		switch (token[i])
		{
		case 'K' : m_castlings |= WHITE_O_O;   break;
		case 'Q' : m_castlings |= WHITE_O_O_O; break;
		case 'k' : m_castlings |= BLACK_O_O;   break;
		case 'q' : m_castlings |= BLACK_O_O_O; break;
		case '-': break;
		default: goto ILLEGAL_FEN;
		}
	}
	token = s.GetToken();
	if (token.empty()) goto FINAL_CHECK;
	if (token != "-") m_ep = StrToFld(token);

	token = s.GetToken();
	if (token.empty()) goto FINAL_CHECK;

	m_fifty = atoi(token.c_str());
	if (m_fifty < 0) m_fifty = 0;

	token = s.GetToken();
	if (token.empty()) goto FINAL_CHECK;

	m_ply = (atoi(token.c_str()) - 1) * 2;
	if (m_side == BLACK) ++m_ply;
	if (m_ply < 0) m_ply = 0;
	goto FINAL_CHECK;

ILLEGAL_FEN:
	*this = tmp;
	return false;

FINAL_CHECK:
	return true;
}

void Position::UnmakeMove()
{
	if (m_undoSize == 0) return;

	Undo& undo = m_undos[--m_undoSize];
	Move mv = undo.m_mv;
	m_ep = undo.m_ep;
	m_castlings = undo.m_castlings;
	m_fifty = undo.m_fifty;

	FLD from = mv.From();
	FLD to = mv.To();
	PIECE piece = mv.Piece();
	PIECE captured = mv.Captured();
	COLOR side = m_side ^ 1;

	m_hash ^= s_hashSide[1];

	Remove(to);
	Put(from, piece);
	if (captured)
	{
		if (to == m_ep)
			Put(to + 8 - 16 * side, captured);
		else
			Put(to, captured);
	}

	--m_ply;
	m_side ^= 1;

	if (piece == KW)
	{
		m_Kings[WHITE] = from;
		if (from == E1 && to == G1)
		{
			Remove(F1);
			Put(H1, RW);
		}
		else if (from == E1 && to == C1)
		{
			Remove(D1);
			Put(A1, RW);
		}
	}
	else if (piece == KB)
	{
		m_Kings[BLACK] = from;
		if (from == E8 && to == G8)
		{
			Remove(F8);
			Put(H8, RB);
		}
		else if (from == E8 && to == C8)
		{
			Remove(D8);
			Put(A8, RB);
		}
	}
}

void Position::UnmakeNullMove()
{
	if (m_undoSize == 0) return;

	Undo& undo = m_undos[--m_undoSize];
	m_ep = undo.m_ep;
	m_castlings = undo.m_castlings;
	m_fifty = undo.m_fifty;

	m_side ^= 1;
	--m_ply;
	m_hash ^= s_hashSide[1];
}
