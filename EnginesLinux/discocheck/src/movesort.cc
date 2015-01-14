/*
 * DiscoCheck, an UCI chess engine. Copyright (C) 2011-2013 Lucas Braesch.
 *
 * DiscoCheck is free software: you can redistribute it and/or modify it under the terms of the GNU
 * General Public License as published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 *
 * DiscoCheck is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without
 * even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with this program. If not,
 * see <http://www.gnu.org/licenses/>.
*/
#include <algorithm>
#include "movesort.h"
#include "search.h"

void SearchInfo:: clear(int _ply)
{
	ply = _ply;
	m = best = killer[0] = killer[1] = move::move_t(0);
	eval = reduction = 0;
	skip_null = null_child = false;
}

void History::clear()
{
	std::memset(h, 0, sizeof(h));
}

int History::get(const board::Board& B, move::move_t m) const
{
	const int us = B.get_turn(), piece = B.get_piece_on(m.fsq()), tsq = m.tsq();
	assert(!move::is_cop(B, m) && piece_ok(piece));

	return h[us][piece][tsq];
}

void History::add(const board::Board& B, move::move_t m, int bonus)
{
	const int us = B.get_turn(), piece = B.get_piece_on(m.fsq()), tsq = m.tsq();
	assert(!move::is_cop(B, m) && piece_ok(piece));

	h[us][piece][tsq] += bonus;

	if (std::abs(h[us][piece][tsq]) >= History::Max)
		for (int c = WHITE; c <= BLACK; ++c)
			for (int p = PAWN; p <= KING; ++p)
				for (int s = A1; s <= H8; h[c][p][s++] /= 2);
}

move::move_t Refutation::get_refutation(Key dm_key) const
{
	const size_t idx = dm_key & (count - 1);
	Entry tmp = {dm_key, move::move_t(0)};
	return r[idx].dm_key == tmp.dm_key ? r[idx].move : move::move_t(0);
}

void Refutation::set_refutation(Key dm_key, move::move_t m)
{
	const size_t idx = dm_key & (count - 1);
	r[idx] = {dm_key, m};
}

void Refutation::clear()
{
	std::memset(this, 0, count * sizeof(Entry));
}

MoveSort::MoveSort(const board::Board* _B, int _depth, const SearchInfo *_ss,
				   const History *_H, const Refutation *_R)
	: B(_B), ss(_ss), H(_H), R(_R), idx(0), depth(_depth)
{
	type = depth > 0 ? GEN_ALL : (depth == 0 ? GEN_CAPTURES_CHECKS : GEN_CAPTURES);
	/* If we're in check set type = ALL. This affects the sort() and uses SEE instead of MVV/LVA for
	 * example. It improves the quality of sorting for check evasions in the qsearch. */
	if (B->is_check())
		type = GEN_ALL;

	refutation = R ? R->get_refutation(B->get_dm_key()) : move::move_t(0);

	move::move_t mlist[MAX_MOVES];
	count = generate(mlist) - mlist;
	annotate(mlist);
}

move::move_t *MoveSort::generate(move::move_t *mlist)
{
	if (type == GEN_ALL)
		return movegen::gen_moves(*B, mlist);
	else {
		// If we are in check, then type must be ALL (see constructor)
		assert(!B->is_check());

		const int us = B->get_turn(), them = opp_color(us);
		move::move_t *end = mlist;
		Bitboard targets = B->get_pieces(them);

		// Piece captures
		if (targets & (B->get_attacks(us, KNIGHT) | B->get_attacks(us, BISHOP)
					   | B->get_attacks(us, ROOK) | B->get_attacks(us, KING)))
			end = movegen::gen_piece_moves(*B, targets, end, true);

		// Pawn captures
		targets |= B->st().epsq_bb() | bb::eighth_rank(us);
		if (targets & B->get_attacks(us, PAWN))
			end = movegen::gen_pawn_moves(*B, targets, end, false);

		// Quiet checks
		if (type == GEN_CAPTURES_CHECKS)
			end = movegen::gen_quiet_checks(*B, end);

		return end;
	}
}

void MoveSort::annotate(const move::move_t *mlist)
{
	for (int i = idx; i < count; ++i) {
		list[i].m = mlist[i];
		score(&list[i]);
	}
}

void MoveSort::score(MoveSort::Token *t)
{
	t->see = -INF;	// not computed

	if (t->m == ss->best)
		t->score = INF;
	else if (move::is_cop(*B, t->m))
		if (type == GEN_ALL) {
			// equal and winning captures, by SEE, in front of quiet moves
			// losing captures, after all quiet moves
			t->see = move::see(*B, t->m);
			t->score = t->see >= 0 ? t->see + History::Max : t->see - History::Max;
		} else
			t->score = mvv_lva(*B, t->m);
	else {
		// killers first, then the rest by history
		if (depth > 0 && t->m == ss->killer[0])
			t->score = History::Max - 1;
		else if (depth > 0 && t->m == ss->killer[1])
			t->score = History::Max - 2;
		else if (t->m == refutation)
			t->score = History::Max - 3;
		else
			t->score = H->get(*B, t->m);
	}
}

move::move_t MoveSort::next(int *see)
{
	if (idx < count) {
		std::swap(list[idx], *std::max_element(&list[idx], &list[count]));
		const Token& t = list[idx++];
		*see = t.see == -INF
			   ? move::see(*B, t.m)	// compute SEE
			   : t.see;				// use SEE cache
		return t.m;
	} else
		return move::move_t(0);
}

move::move_t MoveSort::previous()
{
	return idx > 0 ? list[--idx].m : move::move_t(0);
}
