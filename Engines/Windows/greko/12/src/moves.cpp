//  GREKO Chess Engine
//  (c) 2002-2015 Vladimir Medvedev <vrm@bk.ru>
//  http://greko.su

//  moves.cpp: bitboard moves generator
//  modified: 21-Apr-2015

#include "eval.h"
#include "moves.h"
#include "position.h"
#include "search.h"
#include "utils.h"

Move MoveList::GetNthBest(int n, Move hashMove)
{
	if (m_data[n].m_mv == hashMove)
		return hashMove;

	int bestIndex = n;
	int bestValue = m_data[n].m_value;
	for (int i = n + 1; i < m_size; ++i)
	{
		if (m_data[i].m_value > bestValue)
		{
			bestIndex = i;
			bestValue = m_data[i].m_value;
		}
	}
	if (bestIndex != n)
		std::swap(m_data[n], m_data[bestIndex]);
	return m_data[n].m_mv;
}

void MoveList::GenAllMoves(const Position& pos)
{
	Clear();

	COLOR side = pos.Side();
	COLOR opp = side ^ 1;
	U64 freeOrOpp = ~pos.BitsAll(side);
	U64 occ = pos.BitsAll();

	U64 x, y;
	FLD from, to;
	PIECE piece, captured;

	int fwd = -8 + 16 * side;
	int second = 6 - 5 * side;
	int seventh = 1 + 5 * side;

	piece = PW | side;
	x = pos.Bits(piece);
	while (x)
	{
		from = Bitboard::PopLSB(x);
		int row = Row(from);

		to = static_cast<FLD>(from + fwd);
		if (!pos[to])
		{
			if (row == second)
			{
				Add(from, to, piece);
				to = static_cast<FLD>(to + fwd);
				if (!pos[to])
					Add(from, to, piece);
			}
			else if (row == seventh)
			{
				Add(from, to, piece, NOPIECE, QW | side);
				Add(from, to, piece, NOPIECE, RW | side);
				Add(from, to, piece, NOPIECE, BW | side);
				Add(from, to, piece, NOPIECE, NW | side);
			}
			else
				Add(from, to, piece);
		}

		y = Bitboard::PawnAttacks(from, side) & pos.BitsAll(opp);
		while (y)
		{
			to = Bitboard::PopLSB(y);
			captured = pos[to];

			if (row == seventh)
			{
				Add(from, to, piece, captured, QW | side);
				Add(from, to, piece, captured, RW | side);
				Add(from, to, piece, captured, BW | side);
				Add(from, to, piece, captured, NW | side);
			}
			else
				Add(from, to, piece, captured);
		}
	}

	to = pos.Ep();
	if (to != NF)
	{
		y = Bitboard::PawnAttacks(to, opp) & pos.Bits(PW | side);
		while (y)
		{
			from = Bitboard::PopLSB(y);
			Add(from, to, piece, PW | opp);
		}
	}

	piece = NW | side;
	x = pos.Bits(piece);
	while (x)
	{
		from = Bitboard::PopLSB(x);
		y = Bitboard::KnightAttacks(from) & freeOrOpp;
		while (y)
		{
			to = Bitboard::PopLSB(y);
			captured = pos[to];
			Add(from, to, piece, captured);
		}
	}

	piece = BW | side;
	x = pos.Bits(piece);
	while (x)
	{
		from = Bitboard::PopLSB(x);
		y = Bitboard::BishopAttacks(from, occ) & freeOrOpp;
		while (y)
		{
			to = Bitboard::PopLSB(y);
			captured = pos[to];
			Add(from, to, piece, captured);
		}
	}

	piece = RW | side;
	x = pos.Bits(piece);
	while (x)
	{
		from = Bitboard::PopLSB(x);
		y = Bitboard::RookAttacks(from, occ) & freeOrOpp;
		while (y)
		{
			to = Bitboard::PopLSB(y);
			captured = pos[to];
			Add(from, to, piece, captured);
		}
	}

	piece = QW | side;
	x = pos.Bits(piece);
	while (x)
	{
		from = Bitboard::PopLSB(x);
		y = Bitboard::QueenAttacks(from, occ) & freeOrOpp;
		while (y)
		{
			to = Bitboard::PopLSB(y);
			captured = pos[to];
			Add(from, to, piece, captured);
		}
	}

	piece = KW | side;
	x = pos.Bits(piece);
	while (x)
	{
		from = Bitboard::PopLSB(x);
		y = Bitboard::KingAttacks(from) & freeOrOpp;
		while (y)
		{
			to = Bitboard::PopLSB(y);
			captured = pos[to];
			Add(from, to, piece, captured);
		}

		if (from == E1 && side == WHITE)
		{
			if (!pos[F1] && !pos[G1] && (pos.Castlings() & WHITE_O_O))
				if (!pos.IsAttacked(E1, BLACK) && !pos.IsAttacked(F1, BLACK) && !pos.IsAttacked(G1, BLACK))
					Add(E1, G1, KW);
			if (!pos[D1] && !pos[C1] && !pos[B1] && (pos.Castlings() & WHITE_O_O_O))
				if (!pos.IsAttacked(E1, BLACK) && !pos.IsAttacked(D1, BLACK) && !pos.IsAttacked(C1, BLACK))
					Add(E1, C1, KW);
		}
		else if (from == E8 && side == BLACK)
		{
			if (!pos[F8] && !pos[G8] && (pos.Castlings() & BLACK_O_O))
				if (!pos.IsAttacked(E8, WHITE) && !pos.IsAttacked(F8, WHITE) && !pos.IsAttacked(G8, WHITE))
					Add(E8, G8, KB);
			if (!pos[D8] && !pos[C8] && !pos[B8] && (pos.Castlings() & BLACK_O_O_O))
				if (!pos.IsAttacked(E8, WHITE) && !pos.IsAttacked(D8, WHITE) && !pos.IsAttacked(C8, WHITE))
					Add(E8, C8, KB);
		}
	}
}

void MoveList::GenCaptures(const Position& pos, bool genChecks)
{
	Clear();

	COLOR side = pos.Side();
	COLOR opp = side ^ 1;
	U64 targets = pos.BitsAll(opp);
	U64 occ = pos.BitsAll();
	U64 free = ~occ;
	U64 checksN = 0, checksB = 0, checksR = 0, checksQ = 0;

	FLD Kopp = pos.King(opp);
	if (genChecks)
	{
		checksN = Bitboard::KnightAttacks(Kopp) & free;
		checksB = Bitboard::BishopAttacks(Kopp, occ) & free;
		checksR = Bitboard::RookAttacks(Kopp, occ) & free;
		checksQ = checksB | checksR;
	}

	U64 x, y;
	FLD from, to;
	PIECE piece, captured;

	int fwd = -8 + 16 * side;
	int seventh = 1 + 5 * side;

	piece = PW | side;
	x = pos.Bits(piece);
	while (x)
	{
		from = Bitboard::PopLSB(x);
		int row = Row(from);

		to = static_cast<FLD>(from + fwd);
		if (!pos[to])
		{
			if (row == seventh)
			{
				Add(from, to, piece, NOPIECE, QW | side);
				Add(from, to, piece, NOPIECE, RW | side);
				Add(from, to, piece, NOPIECE, BW | side);
				Add(from, to, piece, NOPIECE, NW | side);
			}
		}

		y = Bitboard::PawnAttacks(from, side) & pos.BitsAll(opp);
		while (y)
		{
			to = Bitboard::PopLSB(y);
			captured = pos[to];

			if (row == seventh)
			{
				Add(from, to, piece, captured, QW | side);
				Add(from, to, piece, captured, RW | side);
				Add(from, to, piece, captured, BW | side);
				Add(from, to, piece, captured, NW | side);
			}
			else
				Add(from, to, piece, captured);
		}
	}

	to = pos.Ep();
	if (to != NF)
	{
		y = Bitboard::PawnAttacks(to, opp) & pos.Bits(PW | side);
		while (y)
		{
			from = Bitboard::PopLSB(y);
			Add(from, to, piece, PW | opp);
		}
	}

	piece = NW | side;
	x = pos.Bits(piece);
	while (x)
	{
		from = Bitboard::PopLSB(x);
		y = Bitboard::KnightAttacks(from) & (targets | checksN);
		while (y)
		{
			to = Bitboard::PopLSB(y);
			captured = pos[to];
			Add(from, to, piece, captured);
		}
	}

	piece = BW | side;
	x = pos.Bits(piece);
	while (x)
	{
		from = Bitboard::PopLSB(x);
		y = Bitboard::BishopAttacks(from, occ) & (targets | checksB);
		while (y)
		{
			to = Bitboard::PopLSB(y);
			captured = pos[to];
			Add(from, to, piece, captured);
		}
	}

	piece = RW | side;
	x = pos.Bits(piece);
	while (x)
	{
		from = Bitboard::PopLSB(x);
		y = Bitboard::RookAttacks(from, occ) & (targets | checksR);
		while (y)
		{
			to = Bitboard::PopLSB(y);
			captured = pos[to];
			Add(from, to, piece, captured);
		}
	}

	piece = QW | side;
	x = pos.Bits(piece);
	while (x)
	{
		from = Bitboard::PopLSB(x);
		y = Bitboard::QueenAttacks(from, occ) & (targets | checksQ);
		while (y)
		{
			to = Bitboard::PopLSB(y);
			captured = pos[to];
			Add(from, to, piece, captured);
		}
	}

	piece = KW | side;
	x = pos.Bits(piece);
	while (x)
	{
		from = Bitboard::PopLSB(x);
		y = Bitboard::KingAttacks(from) & targets;
		while (y)
		{
			to = Bitboard::PopLSB(y);
			captured = pos[to];
			Add(from, to, piece, captured);
		}
	}
}

void MoveList::GenCheckEvasions(const Position& pos)
{
	Clear();

	COLOR side = pos.Side();
	COLOR opp = side ^ 1;
	U64 freeOrOpp = ~pos.BitsAll(side);
	U64 occ = pos.BitsAll();

	U64 x, y;
	FLD from, to;
	PIECE piece, captured;
	FLD K = pos.King(side);

	U64 attackers = pos.GetAttacks(K, opp, occ);
	U64 mask = attackers;
	while (attackers)
	{
		from = Bitboard::PopLSB(attackers);
		mask |= Bitboard::Between(from, K);
	}

	int fwd = -8 + 16 * side;
	int second = 6 - 5 * side;
	int seventh = 1 + 5 * side;

	piece = PW | side;
	x = pos.Bits(piece);
	while (x)
	{
		from = Bitboard::PopLSB(x);
		int row = Row(from);

		to = static_cast<FLD>(from + fwd);
		if (!pos[to])
		{
			if (row == second)
			{
				if (Bitboard::Single(to) & mask)
					Add(from, to, piece);

				to = static_cast<FLD>(to + fwd);
				if (!pos[to])
				{
					if (Bitboard::Single(to) & mask)
						Add(from, to, piece);
				}
			}
			else if (row == seventh)
			{
				if (Bitboard::Single(to) & mask)
				{
					Add(from, to, piece, NOPIECE, QW | side);
					Add(from, to, piece, NOPIECE, RW | side);
					Add(from, to, piece, NOPIECE, BW | side);
					Add(from, to, piece, NOPIECE, NW | side);
				}
			}
			else
			{
				if (Bitboard::Single(to) & mask)
					Add(from, to, piece);
			}
		}

		y = Bitboard::PawnAttacks(from, side) & pos.BitsAll(opp) & mask;
		while (y)
		{
			to = Bitboard::PopLSB(y);
			captured = pos[to];

			if (row == seventh)
			{
				Add(from, to, piece, captured, QW | side);
				Add(from, to, piece, captured, RW | side);
				Add(from, to, piece, captured, BW | side);
				Add(from, to, piece, captured, NW | side);
			}
			else
				Add(from, to, piece, captured);
		}
	}

	to = pos.Ep();
	if (to != NF)
	{
		y = Bitboard::PawnAttacks(to, opp) & pos.Bits(PW | side);
		while (y)
		{
			from = Bitboard::PopLSB(y);
			Add(from, to, piece, PW | opp);
		}
	}

	piece = NW | side;
	x = pos.Bits(piece);
	while (x)
	{
		from = Bitboard::PopLSB(x);
		y = Bitboard::KnightAttacks(from) & freeOrOpp & mask;
		while (y)
		{
			to = Bitboard::PopLSB(y);
			captured = pos[to];
			Add(from, to, piece, captured);
		}
	}

	piece = BW | side;
	x = pos.Bits(piece);
	while (x)
	{
		from = Bitboard::PopLSB(x);
		y = Bitboard::BishopAttacks(from, occ) & freeOrOpp & mask;
		while (y)
		{
			to = Bitboard::PopLSB(y);
			captured = pos[to];
			Add(from, to, piece, captured);
		}
	}

	piece = RW | side;
	x = pos.Bits(piece);
	while (x)
	{
		from = Bitboard::PopLSB(x);
		y = Bitboard::RookAttacks(from, occ) & freeOrOpp & mask;
		while (y)
		{
			to = Bitboard::PopLSB(y);
			captured = pos[to];
			Add(from, to, piece, captured);
		}
	}

	piece = QW | side;
	x = pos.Bits(piece);
	while (x)
	{
		from = Bitboard::PopLSB(x);
		y = Bitboard::QueenAttacks(from, occ) & freeOrOpp & mask;
		while (y)
		{
			to = Bitboard::PopLSB(y);
			captured = pos[to];
			Add(from, to, piece, captured);
		}
	}

	piece = KW | side;
	x = pos.Bits(piece);
	while (x)
	{
		from = Bitboard::PopLSB(x);
		y = Bitboard::KingAttacks(from) & freeOrOpp;
		while (y)
		{
			to = Bitboard::PopLSB(y);
			if (!pos.IsAttacked(to, opp))
			{
				captured = pos[to];
				Add(from, to, piece, captured);
			}
		}
	}
}
