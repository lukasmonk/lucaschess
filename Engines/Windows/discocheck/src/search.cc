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
#include <vector>
#include "search.h"
#include "uci.h"
#include "eval.h"
#include "psq.h"
#include "movesort.h"
#include "prng.h"

using namespace std::chrono;

namespace search {

TTable TT;
Refutation R;

uint64_t node_count;

}	// namespace search

namespace {

bool can_abort, pondering;
struct AbortSearch {};
struct ForcedMove {};

uint64_t node_limit;
int time_limit[2], time_allowed;
time_point<high_resolution_clock> start;

History H;

// Formulas tuned by CLOP
int razor_margin(int depth)	  { return 73 * depth + 145; }
int eval_margin(int depth)	  { return 37 * depth + 111; }
int null_reduction(int depth) { return (13 * depth + 72) / 32; }

int DrawScore[NB_COLOR];	// Contempt draw score by color

move::move_t pv[MAX_PLY+1][MAX_PLY+1];
move::move_t best_move, ponder_move;
bool best_move_changed;

void node_poll()
{
	if ((++search::node_count & 255) == 0 && can_abort) {
		bool abort = false;

		// node limit reached ?
		if (node_limit && search::node_count >= node_limit)
			abort = true;
		// time limit reached ?
		else if (time_allowed && duration_cast<milliseconds>
				 (high_resolution_clock::now() - start).count() > time_allowed)
			abort = true;

		// limit reached: abort search, unless we're pondering
		if (abort && !pondering)
			throw AbortSearch();

		// handle input during search
		std::string token = uci::check_input();
		if (token == "stop")
			throw AbortSearch();
		else if (token == "ponderhit")
			pondering = false;
	}
}

static bool is_mate_score(int score)
{
	assert(std::abs(score) <= INF);
	return std::abs(score) >= MATE - MAX_PLY;
}

int mated_in(int ply)
{
	return ply - MATE;
}

int mate_in(int ply)
{
	return MATE - ply;
}

int score_to_tt(int score, int ply)
/* mate scores from the search, must be adjusted to be written in the TT. For example, if we find a
 * mate in 10 plies from the current position, it will be scored mate_in(15) by the search and must
 * be entered mate_in(10) in the TT */
{
	return score >= mate_in(MAX_PLY) ? score + ply :
		   score <= mated_in(MAX_PLY) ? score - ply : score;
}

int score_from_tt(int tt_score, int ply)
/* mate scores from the TT need to be adjusted. For example, if we find a mate in 10 in the TT at
 * ply 5, then we effectively have a mate in 15 plies (from the root) */
{
	return tt_score >= mate_in(MAX_PLY) ? tt_score - ply :
		   tt_score <= mated_in(MAX_PLY) ? tt_score + ply : tt_score;
}

bool can_return_tt(const TTable::Entry *tte, int depth, int beta, int ply)
// TT pruning is only done at non PV nodes, in order to display untruncated PVs
{
	if (tte->depth < depth)
		return false;

	const int tt_score = score_from_tt(tte->score, ply);
	return (tte->node_type() == Cut && tt_score >= beta)
		|| (tte->node_type() == All && tt_score < beta)
		|| tte->node_type() == PV;
}

void time_alloc(const search::Limits& sl, int result[2])
{
	if (sl.movetime > 0)
		result[0] = result[1] = sl.movetime;
	else if (sl.time > 0 || sl.inc > 0) {
		int movestogo = sl.movestogo > 0 ? sl.movestogo : 30;
		result[0] = std::max(std::min(sl.time / movestogo + sl.inc, sl.time - uci::TimeBuffer), 1);
		result[1] = std::max(std::min(sl.time / (1 + movestogo / 2) + sl.inc, sl.time - uci::TimeBuffer), 1);
	}
}

int qsearch(board::Board& B, int alpha, int beta, int depth, SearchInfo *ss)
{
	assert(depth <= 0 && alpha < beta);
	const bool pv_node = alpha < beta - 1;

	const Key key = B.get_key();
	search::TT.prefetch(key);
	node_poll();

	const bool in_check = B.is_check();
	int best_score = -INF, old_alpha = alpha;
	ss->best = move::move_t(0);

	if (pv_node)
		pv[ss->ply][0] = move::move_t(0);

	if (B.is_draw())
		return DrawScore[B.get_turn()];

	const Bitboard hanging = hanging_pieces(B);

	// TT lookup
	const TTable::Entry *tte = search::TT.probe(key);
	if (tte) {
		if (can_return_tt(tte, depth, beta, ss->ply)) {
			search::TT.refresh(tte);
			return score_from_tt(tte->score, ss->ply);
		}
		ss->eval = tte->eval;
		ss->best = tte->move;
	} else
		ss->eval = in_check ? -INF : (ss->null_child ? -(ss - 1)->eval : eval::symmetric_eval(B));

	// stand pat score
	int stand_pat = ss->eval + eval::asymmetric_eval(B, hanging);
	if (tte) {
		if (tte->score < stand_pat && tte->node_type() <= PV)
			stand_pat = tte->score;
		else if (tte->score > stand_pat && tte->node_type() >= PV)
			stand_pat = tte->score;
	}

	// consider stand pat when not in check
	if (!in_check) {
		best_score = stand_pat;
		alpha = std::max(alpha, best_score);
		if (alpha >= beta)
			return alpha;
	}

	MoveSort MS(&B, depth, ss, &H, nullptr);
	int see;
	const int fut_base = stand_pat + vEP / 2;

	while ( alpha < beta && (ss->m = MS.next(&see)) ) {
		int check = move::is_check(B, ss->m);

		// Futility pruning
		if (!check && !in_check) {
			// opt_score = current eval + some margin + max material gain of the move
			const int opt_score = fut_base
				+ psq::material(B.get_piece_on(ss->m.tsq())).eg
				+ (ss->m.flag() == move::EN_PASSANT ? vEP : 0)
				+ (ss->m.flag() == move::PROMOTION ? psq::material(ss->m.prom()).eg - vOP : 0);

			// still can't raise alpha, skip
			if (opt_score <= alpha) {
				best_score = std::max(best_score, opt_score);	// beware of fail soft side effect
				continue;
			}

			// the "SEE proxy" tells us we are unlikely to raise alpha, skip if depth < 0
			if (fut_base <= alpha && depth < 0 && see <= 0) {
				best_score = std::max(best_score, fut_base);	// beware of fail soft side effect
				continue;
			}
		}

		// SEE pruning
		if (!in_check && check != move::DISCO_CHECK && see < 0)
			continue;

		// recursion
		int score;
		if (depth <= MIN_DEPTH && !in_check)		// prevent qsearch explosion
			score = stand_pat + see;
		else {
			B.play(ss->m);
			score = -qsearch(B, -beta, -alpha, depth - 1, ss + 1);
			B.undo();
		}

		if (score > best_score) {
			best_score = score;

			if (score > alpha) {
				alpha = score;

				if (pv_node) {
					// update the PV
					pv[ss->ply][0] = ss->m;
					memcpy(&pv[ss->ply][1], &pv[ss->ply+1][0], MAX_PLY * sizeof(move::move_t));
					pv[ss->ply][MAX_PLY] = move::move_t(0);
				}
			}

			ss->best = ss->m;
		}
	}

	if (B.is_check() && !MS.get_count())
		return mated_in(ss->ply);

	// update TT
	const int node_type = best_score <= old_alpha ? All : best_score >= beta ? Cut : PV;
	search::TT.store(key, node_type, depth, score_to_tt(best_score, ss->ply), ss->eval, ss->best);

	return best_score;
}

void update_killers(const board::Board& B, SearchInfo *ss)
{
	// update killers on a LIFO basis
	if (ss->killer[0] != ss->best) {
		ss->killer[1] = ss->killer[0];
		ss->killer[0] = ss->best;
	}

	// update double move refutation hash table
	search::R.set_refutation(B.get_dm_key(), ss->best);
}

template <bool root>
int pvs(board::Board& B, int alpha, int beta, int depth, SearchInfo *ss)
{
	assert(alpha < beta);
	const bool pv_node = alpha < beta - 1;

	if (depth <= 0 || ss->ply >= MAX_DEPTH)
		return qsearch(B, alpha, beta, depth, ss);

	const Key key = B.get_key();
	search::TT.prefetch(key);

	if (pv_node)
		pv[ss->ply][0] = move::move_t(0);

	node_poll();

	const bool in_check = B.is_check();
	const int old_alpha = alpha;
	int best_score = -INF;
	ss->best = move::move_t(0);

	if (!root && (B.is_draw() || (bb::count_bit(B.st().occ) <= 4 && eval::is_tb_draw(B))))
		return DrawScore[B.get_turn()];

	// mate distance pruning
	alpha = std::max(alpha, mated_in(ss->ply));
	beta = std::min(beta, mate_in(ss->ply + 1));
	if (!root && alpha >= beta)
		return alpha;

	const Bitboard hanging = hanging_pieces(B);

	// TT lookup
	const TTable::Entry *tte = search::TT.probe(key);
	if (tte) {
		if (!pv_node && can_return_tt(tte, depth, beta, ss->ply)) {
			// Refresh TT entry to prevent ageing
			search::TT.refresh(tte);

			// update killers, refutation, and history on TT prune when alpha is raised
			if (tte->score > old_alpha && (ss->best = tte->move) && !move::is_cop(B, ss->best)) {
				update_killers(B, ss);
				H.add(B, ss->best, (depth * depth) >> (hanging != 0));
			}

			return score_from_tt(tte->score, ss->ply);
		}
		ss->eval = tte->eval;
		ss->best = tte->move;
	} else
		ss->eval = in_check ? -INF : (ss->null_child ? -(ss - 1)->eval : eval::symmetric_eval(B));

	// Stand pat score: adjust for assymetric eval, and using tte->score (when possible)
	int stand_pat = ss->eval + eval::asymmetric_eval(B, hanging);
	if (tte) {
		if (tte->score < stand_pat && tte->node_type() <= PV)
			stand_pat = tte->score;
		else if (tte->score > stand_pat && tte->node_type() >= PV)
			stand_pat = tte->score;
	}

	// post futility pruning
	if (!pv_node && depth <= 5 && !ss->skip_null
		&& !in_check && !is_mate_score(beta)
		&& stand_pat >= beta + eval_margin(depth)
		&& B.st().piece_psq[B.get_turn()])
		return stand_pat;

	// Razoring
	if (!pv_node && depth <= 3 && !in_check && !is_mate_score(alpha)) {
		const int lbound = alpha - razor_margin(depth);
		if (stand_pat <= lbound) {
			const int score = qsearch(B, lbound, lbound + 1, 0, ss + 1);
			if (score <= lbound)
				return score;
		}
	}

	// Null move pruning
	if (stand_pat >= beta
		&& !ss->skip_null && depth >= 2
		&& !in_check && !is_mate_score(beta)
		&& B.st().piece_psq[B.get_turn()]
		&& !pv_node) {
		const int reduction = null_reduction(depth) + (stand_pat - vOP >= beta);

		// if the TT entry tells us that no move can beat alpha at the null search depth or deeper,
		// we safely skip the null search.
		if (tte && tte->depth >= depth - reduction
			&& tte->node_type() != Cut	// ie. upper bound or exact score
			&& tte->score <= alpha)
			goto tt_skip_null;

		B.play(move::move_t(0));
		(ss + 1)->null_child = (ss + 1)->skip_null = true;
		const int score = -pvs<false>(B, -beta, -alpha, depth - reduction, ss + 1);
		(ss + 1)->null_child = (ss + 1)->skip_null = false;
		B.undo();

		if (score >= beta)	// null search fails high
			return score < mate_in(MAX_PLY)
				? score		// fail soft
				: beta;		// but do not return an unproven mate
		else {
			if (score <= mated_in(MAX_PLY) && (ss - 1)->reduction) {
				++depth;
				--(ss - 1)->reduction;
			}
		}
	}

tt_skip_null:

	// Internal Iterative Deepening
	if ( (!tte || !tte->move || tte->depth <= 0)
		 && depth >= (pv_node ? 4 : 7) ) {
		ss->skip_null = true;
		pvs<false>(B, alpha, beta, pv_node ? depth - 2 : depth / 2, ss);
		ss->skip_null = false;
	}

	MoveSort MS(&B, depth, ss, &H, &search::R);
	const move::move_t refutation = search::R.get_refutation(B.get_dm_key());

	int cnt = 0, LMR = 0, see;
	while ( alpha < beta && (ss->m = MS.next(&see)) ) {
		++cnt;
		const int check = move::is_check(B, ss->m);

		// check extension
		int new_depth;
		if (check && (check == move::DISCO_CHECK || see >= 0) )
			// extend relevant checks
			new_depth = depth;
		else if (MS.get_count() == 1)
			// extend forced replies
			new_depth = depth;
		else
			new_depth = depth - 1;

		// move properties
		const bool first = cnt == 1;
		const bool capture = move::is_cop(B, ss->m);
		const int hscore = capture ? 0 : H.get(B, ss->m);
		const bool bad_quiet = !capture && (hscore < 0 || (hscore == 0 && see < 0));
		const bool bad_capture = capture && see < 0;
		// dangerous movea are not reduced
		const bool dangerous = check
			|| (move::is_pawn_threat(B, ss->m) && see >= 0);

		// basic reduction (1 ply)
		ss->reduction = !first && (bad_capture || bad_quiet)
			&& ss->m != ss->killer[0] && ss->m != ss->killer[1] && ss->m != refutation;
		// further reductions (2 or 3 plies)
		if (ss->reduction && !capture && !check) {
			++LMR;
			const int idx = 2 + 8 / depth;
			ss->reduction += (LMR >= idx) + (LMR >= 3*idx);
		}

		// do not LMR into the deep QS
		if (new_depth - ss->reduction <= 0)
			ss->reduction = new_depth;

		// pruning at shallow depth
		if (!pv_node && depth <= 6 && cnt > 1
			&& !capture && !dangerous && !in_check) {

			// pre futility pruning
			const int child_depth = new_depth - ss->reduction;
			if (child_depth <= 5) {
				const int opt_score = stand_pat + vEP/2 + eval_margin(child_depth);
				if (opt_score <= alpha) {
					best_score = std::max(best_score, std::min(alpha, stand_pat + see));
					continue;
				}
			}

			// Move count pruning
			if ( LMR >= 3 + depth * (2 * depth - 1) / 2
				 && alpha > mated_in(MAX_PLY) ) {
				best_score = std::max(best_score, std::min(alpha, stand_pat + see));
				continue;
			}

			// SEE pruning near the leaves
			if (new_depth <= 1 && see < 0) {
				best_score = std::max(best_score, std::min(alpha, stand_pat + see));
				continue;
			}
		}

		B.play(ss->m);

		// PVS
		int score;
		if (first)
			// search full window full depth
			// Note that the full window is a zero window at non PV nodes
			score = -pvs<false>(B, -beta, -alpha, new_depth, ss + 1);
		else {
			// zero window search (reduced)
			score = -pvs<false>(B, -alpha - 1, -alpha, new_depth - ss->reduction, ss + 1);

			// doesn't fail low: verify at full depth, with zero window
			if (score > alpha && ss->reduction)
				score = -pvs<false>(B, -alpha - 1, -alpha, new_depth, ss + 1);

			// still doesn't fail low at PV node: full depth and full window
			if (pv_node && score > alpha)
				score = -pvs<false>(B, -beta, -alpha, new_depth , ss + 1);
		}

		B.undo();

		if (score > best_score) {
			best_score = score;
			ss->best = ss->m;

			if (score > alpha) {
				alpha = score;

				if (pv_node) {
					// update the PV
					pv[ss->ply][0] = ss->m;
					memcpy(&pv[ss->ply][1], &pv[ss->ply+1][0], MAX_PLY * sizeof(move::move_t));
					pv[ss->ply][MAX_PLY] = move::move_t(0);
				}
			}

			if (root) {
				if (best_move != ss->m) {
					best_move_changed = true;
					best_move = ss->m;
				}
				ponder_move = pv[ss->ply][1];
			}
		}
	}

	if (!MS.get_count()) {
		// mated or stalemated
		assert(!root);
		return in_check ? mated_in(ss->ply) : DrawScore[B.get_turn()];
	} else if (root && MS.get_count() == 1 && can_abort && !pondering)
		// forced move at the root node, play instantly and prevent further iterative deepening
		throw ForcedMove();

	// update TT
	const int node_type = best_score <= old_alpha ? All : best_score >= beta ? Cut : PV;
	search::TT.store(key, node_type, depth, score_to_tt(best_score, ss->ply), ss->eval, ss->best);

	// best move is quiet: update move sorting heuristics if alpha was raised
	if (best_score > old_alpha && ss->best && !move::is_cop(B, ss->best)) {
		// update killers and reuftation table
		update_killers(B, ss);

		// update history table
		// mark ss->best as good, and all other moves searched as bad
		move::move_t m;
		int bonus = std::min(depth * depth, (int)History::Max);
		if (hanging) bonus /= 2;
		while ( (m = MS.previous()) )
			if (!move::is_cop(B, m))
				H.add(B, m, m == ss->best ? bonus : -bonus);
	}

	return best_score;
}

}	// namespace

namespace search {

std::pair<move::move_t, move::move_t> bestmove(board::Board& B, const Limits& sl)
// returns a pair (best move, ponder move)
{
	start = high_resolution_clock::now();

	SearchInfo ss[MAX_PLY + 1];
	for (int ply = 0; ply <= MAX_PLY; ++ply)
		ss[ply].clear(ply);

	node_count = 0;
	node_limit = sl.nodes;
	pondering = sl.ponder;
	time_alloc(sl, time_limit);

	best_move = ponder_move = move::move_t(0);
	best_move_changed = false;

	H.clear();
	TT.new_search();
	B.set_root();	// remember root node, for correct 2/3-fold in is_draw()

	// Contempt Draw value
	const int us = B.get_turn(), them = opp_color(us);
	DrawScore[us] = -uci::Contempt;
	DrawScore[them] = uci::Contempt;

	uci::info ui;
	ui.pv = pv[0];

	const int max_depth = sl.depth ? std::min(MAX_DEPTH, sl.depth) : MAX_DEPTH;

	// iterative deepening loop
	for (int depth = 1, alpha = -INF, beta = +INF; depth <= max_depth; depth++) {
		ui.clear();
		ui.depth = depth;

		// We can only abort the search once iteration 1 is finished. In extreme situations (eg.
		// fixed nodes), the SearchLimits sl could trigger a search abortion before that, which is
		// disastrous, as the best move could be illegal or completely stupid.
		can_abort = depth >= 2;

		int delta = 16;

		// Time allowance
		time_allowed = time_limit[best_move_changed];
		if (best_move && move::see(B, best_move) > 0)
			time_allowed /= 2;

		best_move_changed = false;
		for (;;) {
			// Aspiration loop

			try {
				ui.score = pvs<true>(B, alpha, beta, depth, ss);
			} catch (AbortSearch e) {
				goto return_pair;
			} catch (ForcedMove e) {
				best_move = ss->best;
				goto return_pair;
			}

			ui.nodes = node_count;
			ui.time = duration_cast<milliseconds>(high_resolution_clock::now() - start).count();

			if (alpha < ui.score && ui.score < beta) {
				// score is within bounds
				ui.bound = uci::info::EXACT;

				// set aspiration window for the next depth (so aspiration starts at depth 5)
				if (depth >= 4 && !is_mate_score(ui.score)) {
					alpha = ui.score - delta;
					beta = ui.score + delta;
				}
				// stop the aspiration loop
				break;
			} else {
				// score is outside bounds: resize window and double delta
				if (ui.score <= alpha) {
					alpha -= delta;
					ui.bound = uci::info::UBOUND;
					std::cout << ui << std::endl;
				} else if (ui.score >= beta) {
					beta += delta;
					ui.bound = uci::info::LBOUND;
					std::cout << ui << std::endl;
				}
				delta *= 2;

				// increase time_allowed, to try to finish the current depth iteration
				time_allowed = time_limit[1];
			}
		}

		std::cout << ui << std::endl;
	}

return_pair:
	return std::make_pair(best_move, ponder_move);
}

void clear_state()
{
	TT.clear();
	R.clear();
}

}	// namespace search
