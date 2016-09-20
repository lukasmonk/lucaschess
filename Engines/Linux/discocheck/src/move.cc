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
 *
 * Credits:
 * - iterative SEE adapted from Glaurung, by Tord Romstad
*/
#include <sstream>
#include <algorithm>
#include "move.h"
#include "board.h"
#include "psq.h"

namespace {

std::string square_to_string(int sq)
{
	std::ostringstream s;
	s << char(file(sq) + 'a') << char(rank(sq) + '1');
	return s.str();
}

}	// namespace

namespace move {

int is_check(const board::Board& B, move_t m)
/* Tests if a move is checking the enemy king. General case: direct checks, revealed checks (when
 * piece is a dchecker moving out of ray). Special cases: castling (check by the rook), en passant
 * (check through a newly revealed sliding attacker, once the ep capture square has been vacated)
 * returns 2 for a discovered check, 1 for any other check, 0 otherwise */
{
	const int us = B.get_turn(), them = opp_color(us);
	const int fsq = m.fsq(), tsq = m.tsq(), flag = m.flag();
	int kpos = B.get_king_pos(them);

	// test discovered check
	if ( (bb::test_bit(B.st().dcheckers, fsq))		// discovery checker
		 && (!bb::test_bit(bb::direction(kpos, fsq), tsq)))	// move out of its dc-ray
		return 2;
	// test direct check
	else if (flag != PROMOTION) {
		const int piece = B.get_piece_on(fsq);
		Bitboard tss = piece == PAWN ? bb::pattacks(us, tsq)
					   : bb::piece_attack(piece, tsq, B.st().occ);
		if (bb::test_bit(tss, kpos))
			return 1;
	}

	if (flag == EN_PASSANT) {
		Bitboard occ = B.st().occ;
		// play the ep capture on occ
		bb::clear_bit(&occ, fsq);
		bb::clear_bit(&occ, tsq + (us ? 8 : -8));
		bb::set_bit(&occ, tsq);
		// test for new sliding attackers to the enemy king
		if ( (B.get_RQ(us) & bb::rattacks(kpos) & bb::rattacks(kpos, occ))
			|| (B.get_BQ(us) & bb::battacks(kpos) & bb::battacks(kpos, occ)) )
			return 2;	// discovered check through the fsq or the ep captured square
	} else if (flag == CASTLING) {
		// position of the Rook after playing the castling move
		int rook_sq = (fsq + tsq) / 2;
		Bitboard occ = B.st().occ;
		bb::clear_bit(&occ, fsq);
		Bitboard RQ = B.get_RQ(us);
		bb::set_bit(&RQ, rook_sq);
		if (RQ & bb::rattacks(kpos) & bb::rattacks(kpos, occ))
			return 1;	// direct check by the castled rook
	} else if (flag == PROMOTION) {
		// test check by the promoted piece
		Bitboard occ = B.st().occ;
		bb::clear_bit(&occ, fsq);
		if (bb::test_bit(bb::piece_attack(m.prom(), tsq, occ), kpos))
			return 1;	// direct check by the promoted piece
	}

	return 0;
}

bool is_cop(const board::Board& B, move_t m)
{
	return piece_ok(B.get_piece_on(m.tsq()))
		   || m.flag() == EN_PASSANT
		   || m.flag() == PROMOTION;
}

bool is_pawn_threat(const board::Board& B, move_t m)
{
	if (B.get_piece_on(m.fsq()) == PAWN) {
		const int us = B.get_turn(), them = opp_color(us), sq = m.tsq();

		if (bb::test_bit(bb::half_board(them), sq)) {
			const Bitboard our_pawns = B.get_pieces(us, PAWN), their_pawns = B.get_pieces(them, PAWN);
			return !(bb::pawn_span(us, sq) & their_pawns)
				   && !(bb::squares_in_front(us, sq) & (our_pawns | their_pawns));
		}
	}

	return false;
}

move_t string_to_move(const board::Board& B, const std::string& s)
{
	move_t m(0);
	m.fsq(square(s[1] - '1', s[0] - 'a'));
	m.tsq(square(s[3] - '1', s[2] - 'a'));
	m.flag(NORMAL);

	if (B.get_piece_on(m.fsq()) == PAWN && m.tsq() == B.st().epsq)
		m.flag(EN_PASSANT);

	if (s[4]) {
		m.flag(PROMOTION);
		m.prom(board::PieceLabel[BLACK].find(s[4]));
	} else if (B.get_piece_on(m.fsq()) == KING && (m.fsq() + 2 == m.tsq() || m.tsq() + 2 == m.fsq()))
		m.flag(CASTLING);

	return m;
}

std::string move_to_string(move_t m)
{
	std::ostringstream s;

	s << square_to_string(m.fsq());
	s << square_to_string(m.tsq());

	if (m.flag() == PROMOTION)
		s << board::PieceLabel[BLACK][m.prom()];

	return s.str();
}

int see(const board::Board& B, move_t m)
// Iterative SEE based on Glaurung. Adapted and improved to handle promotions, promoting recaptures
// and en-passant captures.
{
	static const int see_val[NB_PIECE + 1] = {vOP, vN, vB, vR, vQ, vK, 0};

	int fsq = m.fsq(), tsq = m.tsq();
	int stm = B.get_color_on(fsq);	// side to move
	uint64_t attackers, stm_attackers;
	int swap_list[32], sl_idx = 1;
	uint64_t occ = B.st().occ;
	int piece = B.get_piece_on(fsq), capture;

	// Determine captured piece
	if (m.flag() == EN_PASSANT) {
		bb::clear_bit(&occ, bb::pawn_push(opp_color(stm), tsq));
		capture = PAWN;
	} else
		capture = B.get_piece_on(tsq);
	assert(capture != KING);

	swap_list[0] = see_val[capture];
	bb::clear_bit(&occ, fsq);

	// Handle promotion
	if (m.flag() == PROMOTION) {
		swap_list[0] += see_val[m.prom()] - see_val[PAWN];
		capture = QUEEN;
	} else
		capture = B.get_piece_on(fsq);

	// If the opponent has no attackers we are finished
	attackers = bb::test_bit(B.st().attacked, tsq) ? calc_attackers(B, tsq, occ) : 0;
	stm = opp_color(stm);
	stm_attackers = attackers & B.get_pieces(stm);
	if (!stm_attackers)
		return swap_list[0];

	/* The destination square is defended, which makes things more complicated. We proceed by
	 * building a "swap list" containing the material gain or loss at each stop in a sequence of
	 * captures to the destination square, where the sides alternately capture, and always capture
	 * with the least valuable piece. After each capture, we look for new X-ray attacks from behind
	 * the capturing piece. */
	do {
		/* Locate the least valuable attacker for the side to move. The loop below looks like it is
		 * potentially infinite, but it isn't. We know that the side to move still has at least one
		 * attacker left. */
		for (piece = PAWN; !(stm_attackers & B.get_pieces(stm, piece)); ++piece)
			assert(piece < KING);

		// remove the piece (from wherever it came)
		bb::clear_bit(&occ, bb::lsb(stm_attackers & B.get_pieces(stm, piece)));
		// scan for new X-ray attacks through the vacated square
		attackers |= (B.get_RQ() & bb::rattacks(tsq) & bb::rattacks(tsq, occ))
					 | (B.get_BQ() & bb::battacks(tsq) & bb::battacks(tsq, occ));
		// cut out pieces we've already done
		attackers &= occ;

		// add the new entry to the swap list (beware of promoting pawn captures)
		assert(sl_idx < 32);
		swap_list[sl_idx] = -swap_list[sl_idx - 1] + see_val[capture];
		if (piece == PAWN && bb::test_bit(bb::eighth_rank(stm), tsq)) {
			swap_list[sl_idx] += see_val[QUEEN] - see_val[PAWN];
			capture = QUEEN;
		} else
			capture = piece;
		sl_idx++;

		stm = opp_color(stm);
		stm_attackers = attackers & B.get_pieces(stm);

		// Stop after a king capture
		if (piece == KING && stm_attackers) {
			assert(sl_idx < 32);
			swap_list[sl_idx++] = see_val[KING];
			break;
		}
	} while (stm_attackers);

	/* Having built the swap list, we negamax through it to find the best achievable score from the
	 * point of view of the side to move */
	while (--sl_idx)
		swap_list[sl_idx - 1] = std::min(-swap_list[sl_idx], swap_list[sl_idx - 1]);

	return swap_list[0];
}

int mvv_lva(const board::Board& B, move_t m)
{
	// Queen is the best capture available (King can't be captured since move is legal)
	static const int victim[NB_PIECE + 1] = {1, 2, 2, 3, 4, 0, 0};
	// King is the best attacker (since move is legal) followed by Pawn etc.
	static const int attacker[NB_PIECE] = {4, 3, 3, 2, 1, 5};

	int victim_value = victim[m.flag() == EN_PASSANT ? PAWN : B.get_piece_on(m.tsq())]
					   + (m.flag() == PROMOTION ? victim[m.prom()] - victim[PAWN] : 0);
	int attacker_value = attacker[B.get_piece_on(m.fsq())];

	return victim_value * 8 + attacker_value;
}

/* move_t member function */

move_t::operator bool() const
{
	return b;
}

bool move_t::operator== (move_t m) const
{
	return b == m.b;
}

bool move_t::operator!= (move_t m) const
{
	return b != m.b;
}

int move_t::fsq() const
{
	return b & 0x3f;
}

int move_t::tsq() const
{
	return (b >> 6) & 0x3f;
}

int move_t::flag() const
{
	return (b >> 14) & 3;
}

int move_t::prom() const
{
	assert(flag() == PROMOTION);
	return ((b >> 12) & 3) + KNIGHT;
}

void move_t::fsq(int new_fsq)
{
	assert(square_ok(new_fsq));
	b &= 0xffc0;
	b ^= new_fsq;
}

void move_t::tsq(int new_tsq)
{
	assert(square_ok(new_tsq));
	b &= 0xf03f;
	b ^= (new_tsq << 6);
}

void move_t::flag(int new_flag)
{
	assert(new_flag < 4);
	b &= 0x3fff;
	b ^= (new_flag << 14);
}

void move_t::prom(int piece)
{
	assert(KNIGHT <= piece && piece <= QUEEN);
	b &= 0xcfff;
	b ^= (piece - KNIGHT) << 12;
}

}	// namespace move

