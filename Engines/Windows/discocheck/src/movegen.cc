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
#include <chrono>
#include "movegen.h"
#include "board.h"

namespace {

move::move_t *make_pawn_moves(const board::Board& B, int fsq, int tsq, move::move_t *mlist, bool sub_promotions)
/* Centralise the pawnm moves generation: given (fsq,tsq) the rest follows. We filter here all the
 * indirect self checks (through fsq, or through the ep captured square) */
{
	assert(square_ok(fsq) && square_ok(tsq));
	const int us = B.get_turn(), them = opp_color(us);
	int kpos = B.get_king_pos(us);

	// filter self check through fsq
	if (bb::test_bit(B.st().pinned, fsq) && !bb::test_bit(bb::direction(kpos, fsq), tsq))
		return mlist;

	move::move_t m;
	m.fsq(fsq);
	m.tsq(tsq);

	if (tsq == B.st().epsq) {
		m.flag(move::EN_PASSANT);
		Bitboard occ = B.st().occ;
		// play the ep capture on occ
		bb::clear_bit(&occ, m.fsq());
		bb::clear_bit(&occ, bb::pawn_push(them, m.tsq()));	// remove the ep captured enemy pawn
		bb::set_bit(&occ, m.tsq());
		// test for check by a sliding enemy piece
		if ((B.get_RQ(them) & bb::rattacks(kpos) & bb::rattacks(kpos, occ))
			|| (B.get_BQ(them) & bb::battacks(kpos) & bb::battacks(kpos, occ)))
			return mlist;	// illegal move by indirect self check (through the ep captured pawn)
	} else
		m.flag(move::NORMAL);

	if (bb::test_bit(bb::eighth_rank(us), tsq)) {
		// promotion(s)
		m.flag(move::PROMOTION);
		m.prom(QUEEN); *mlist++ = m;
		if (sub_promotions) {
			m.prom(KNIGHT);	*mlist++ = m;
			m.prom(ROOK); *mlist++ = m;
			m.prom(BISHOP); *mlist++ = m;
		}
	} else
		// non promotions: normal or en-passant
		*mlist++ = m;

	return mlist;
}

move::move_t *make_piece_moves(const board::Board& B, int fsq, Bitboard tss, move::move_t *mlist)
/* Centralise the generation of a piece move: given (fsq,tsq) the rest follows. We filter indirect
 * self checks here. Note that direct self-checks aren't generated, so we don't check them here. In
 * other words, we never put our King in check before calling this function */
{
	assert(square_ok(fsq));
	const int kpos = B.get_king_pos(B.get_turn());

	move::move_t m;
	m.fsq(fsq);
	m.flag(move::NORMAL);

	if (bb::test_bit(B.st().pinned, fsq))
		tss &= bb::direction(kpos, fsq);

	while (tss) {
		m.tsq(bb::pop_lsb(&tss));
		*mlist++ = m;
	}

	return mlist;
}

}	// namespace

namespace movegen {

move::move_t *gen_piece_moves(const board::Board& B, Bitboard targets, move::move_t *mlist, bool king_moves)
/* Generates piece moves, when the board is not in check. Uses targets to filter the tss, eg.
 * targets = ~friends (all moves), empty (quiet moves only), enemies (captures only). */
{
	assert(!king_moves || !B.is_check());	// do not use when in check (use gen_evasion)
	const int us = B.get_turn();
	assert(!(targets & B.get_pieces(us)));
	Bitboard fss;

	// Knight Moves
	fss = B.get_pieces(us, KNIGHT);
	while (fss) {
		int fsq = bb::pop_lsb(&fss);
		Bitboard tss = bb::nattacks(fsq) & targets;
		mlist = make_piece_moves(B, fsq, tss, mlist);
	}

	// Rook Queen moves
	fss = B.get_RQ(us);
	while (fss) {
		int fsq = bb::pop_lsb(&fss);
		Bitboard tss = targets & bb::rattacks(fsq, B.st().occ);
		mlist = make_piece_moves(B, fsq, tss, mlist);
	}

	// Bishop Queen moves
	fss = B.get_BQ(us);
	while (fss) {
		int fsq = bb::pop_lsb(&fss);
		Bitboard tss = targets & bb::battacks(fsq, B.st().occ);
		mlist = make_piece_moves(B, fsq, tss, mlist);
	}

	// King moves (king_moves == false is only used for check escapes)
	if (king_moves) {
		int fsq = B.get_king_pos(us);
		// here we also filter direct self checks, which shouldn't be sent to serialize_moves
		Bitboard tss = bb::kattacks(fsq) & targets & ~B.st().attacked;
		mlist = make_piece_moves(B, fsq, tss, mlist);
	}

	return mlist;
}

move::move_t *gen_castling(const board::Board& B, move::move_t *mlist)
/* Generates castling moves, when the board is not in check. The only function that doesn't go
 * through serialize_moves, as castling moves are very peculiar, and we don't want to pollute
 * serialize_moves with this over-specific code */
{
	assert(!B.is_check());
	const int us = B.get_turn();

	move::move_t m;
	m.fsq(B.get_king_pos(us));
	m.flag(move::CASTLING);

	if (B.st().crights & (board::OO << (2 * us))) {
		Bitboard safe = 3ULL << (m.fsq() + 1);	// must not be attacked
		Bitboard empty = safe;					// must be empty

		if (!(B.st().attacked & safe) && !(B.st().occ & empty)) {
			m.tsq(m.fsq() + 2);
			*mlist++ = m;
		}
	}
	if (B.st().crights & (board::OOO << (2 * us))) {
		Bitboard safe = 3ULL << (m.fsq() - 2);	// must not be attacked
		Bitboard empty = safe | (1ULL << (m.fsq() - 3));	// must be empty

		if (!(B.st().attacked & safe) && !(B.st().occ & empty)) {
			m.tsq(m.fsq() - 2);
			*mlist++ = m;
		}
	}

	return mlist;
}

move::move_t *gen_pawn_moves(const board::Board& B, Bitboard targets, move::move_t *mlist,
							 bool sub_promotions)
/* Generates pawn moves, when the board is not in check. These are of course: double and single
 * pushes, normal captures, en passant captures. Promotions are considered in serialize_moves (so
 * for under-promotion pruning, modify only serialize_moves) */
{
	const int us = B.get_turn(), them = opp_color(us);
	const int lc_inc = us ? -NB_FILE - 1 : NB_FILE - 1;	// left capture increment
	const int rc_inc = us ? -NB_FILE + 1 : NB_FILE + 1;	// right capture increment
	const int sp_inc = us ? -NB_FILE : NB_FILE;		// single push increment
	const int dp_inc = 2 * sp_inc;					// double push increment
	const Bitboard fss = B.get_pieces(us, PAWN);
	const Bitboard enemies = B.get_pieces(them) | B.st().epsq_bb();	// capture targets, incl. epsq

	/* First we calculate the to squares (tss) */

	Bitboard tss, tss_sp, tss_dp, tss_lc, tss_rc, fssd;

	// single pushes
	tss_sp = bb::shift_bit(fss, sp_inc) & ~B.st().occ;

	// double pushes
	fssd = fss & bb::second_rank(us)				// pawns on their initial rank
		   & ~bb::shift_bit(B.st().occ, -sp_inc)	// can push once
		   & ~bb::shift_bit(B.st().occ, -dp_inc);	// can push twice
	tss_dp = bb::shift_bit(fssd, dp_inc);			// double push fssd

	// captures (including en passant if epsq != NO_SQUARE)
	tss_lc = bb::shift_bit(fss & ~bb::FileA_bb, lc_inc) & enemies;	// right captures
	tss_rc = bb::shift_bit(fss & ~bb::FileH_bb, rc_inc) & enemies;	// right captures

	// total
	tss = (tss_sp | tss_dp | tss_lc | tss_rc) & targets;

	/* Then we loop on the tss and find the possible from square(s) */

	while (tss) {
		const int tsq = bb::pop_lsb(&tss);

		if (bb::test_bit(tss_sp, tsq))		// can we single push to tsq ?
			mlist = make_pawn_moves(B, tsq - sp_inc, tsq, mlist, sub_promotions);
		else if (bb::test_bit(tss_dp, tsq))	// can we double push to tsq ?
			mlist = make_pawn_moves(B, tsq - dp_inc, tsq, mlist, sub_promotions);
		else {	// can we capture tsq ?
			if (bb::test_bit(tss_lc, tsq))	// can we left capture tsq ?
				mlist = make_pawn_moves(B, tsq - lc_inc, tsq, mlist, sub_promotions);
			if (bb::test_bit(tss_rc, tsq))	// can we right capture tsq ?
				mlist = make_pawn_moves(B, tsq - rc_inc, tsq, mlist, sub_promotions);
		}
	}

	return mlist;
}

move::move_t *gen_evasion(const board::Board& B, move::move_t *mlist)
/* Generates evasions, when the board is in check. These are of 2 kinds: the king moves, or a piece
 * covers the check. Note that cover means covering the ]ksq,checker_sq], so it includes capturing
 * the checking piece. */
{
	assert(B.is_check());
	const int us = B.get_turn();
	const int kpos = B.get_king_pos(us);
	const Bitboard checkers = B.st().checkers;
	const int csq = bb::lsb(checkers);			// checker square
	const int cpiece = B.get_piece_on(csq);	// checker piece
	Bitboard tss;

	// normal king escapes
	tss = bb::kattacks(kpos) & ~B.get_pieces(us) & ~B.st().attacked;

	// The king must also get out of all sliding checkers' firing lines
	Bitboard _checkers = checkers;
	while (_checkers) {
		const int _csq = bb::pop_lsb(&_checkers);
		const int _cpiece = B.get_piece_on(_csq);
		if (is_slider(_cpiece))
			tss &= ~bb::direction(_csq, kpos);
	}

	// generate King escapes
	mlist = make_piece_moves(B, kpos, tss, mlist);

	if (!bb::several_bits(B.st().checkers)) {
		// piece moves (only if we're not in double check)
		tss = is_slider(cpiece)
			  ? bb::between(kpos, csq)	// cover the check (inc capture the sliding checker)
			  : checkers;				// capture the checker

		/* if checked by a Pawn and epsq is available, then the check must result from a pawn double
		 * push, and we also need to consider capturing it en passant to solve the check */
		Bitboard ep_tss = cpiece == PAWN ? B.st().epsq_bb() : 0ULL;

		mlist = gen_piece_moves(B, tss, mlist, 0);
		mlist = gen_pawn_moves(B, tss | ep_tss, mlist, true);
	}

	return mlist;
}

move::move_t *gen_quiet_checks(const board::Board& B, move::move_t *mlist)
/* Generates non capturing checks by pieces (not pawns nor the king) */
{
	assert(!B.is_check());
	const int us = B.get_turn(), them = opp_color(us);
	const int ksq = B.get_king_pos(them);
	const Bitboard occ = B.st().occ;
	Bitboard fss, tss;

	// Pawn push checks (single push only)
	if (B.get_pieces(us, PAWN) & bb::nattacks(ksq) & bb::pawn_span(them, ksq)) {
		tss = bb::pattacks(them, ksq) & ~occ;
		if (tss)
			mlist = gen_pawn_moves(B, tss, mlist, false);
	}

	// Piece quiet checks (direct + discovered)
	for (int piece = KNIGHT; piece <= QUEEN; piece++) {
		const Bitboard check_squares = bb::piece_attack(piece, ksq, occ);
		fss = B.get_pieces(us, piece);

		while (fss) {
			const int fsq = bb::pop_lsb(&fss);
			// possible destinations of piece on fsq
			Bitboard attacks = bb::piece_attack(piece, fsq, occ);
			// direct checks
			tss = attacks & check_squares;
			// revealed checks
			if (bb::test_bit(B.st().dcheckers, fsq))
				tss |= attacks & ~bb::direction(ksq, fsq);
			// exclude captures
			tss &= ~occ;

			mlist = make_piece_moves(B, fsq, tss, mlist);
		}
	}

	return mlist;
}

move::move_t *gen_moves(const board::Board& B, move::move_t *mlist)
/* Generates all moves in the position, using all the other specific move generators. This function
 * is quite fast but not flexible, and only used for debugging (eg. computing perft values) */
{
	if (B.is_check())
		return gen_evasion(B, mlist);
	else {
		// legal castling moves
		mlist = gen_castling(B, mlist);

		const Bitboard targets = ~B.get_pieces(B.get_turn());

		// generate moves
		mlist = gen_piece_moves(B, targets, mlist, true);
		mlist = gen_pawn_moves(B, targets, mlist, true);

		return mlist;
	}
}

}	// namespace movegen

