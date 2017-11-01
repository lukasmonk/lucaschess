/*
 * Demolito, a UCI chess engine.
 * Copyright 2015 lucasart.
 *
 * Demolito is free software: you can redistribute it and/or modify it under the terms of the GNU
 * General Public License as published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 *
 * Demolito is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without
 * even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with this program. If
 * not, see <http://www.gnu.org/licenses/>.
*/
#include <iostream>
#include "gen.h"
#include "move.h"
#include "bitboard.h"

namespace {

template <bool Promotion>
move_t *serialize_moves(Move& m, bitboard_t tss, move_t *emList, bool subPromotions = true)
{
    while (tss) {
        m.to = bb::pop_lsb(tss);

        if (Promotion) {
            if (subPromotions) {
                for (m.prom = QUEEN; m.prom >= KNIGHT; --m.prom)
                    *emList++ = m;
            } else {
                m.prom = QUEEN;
                *emList++ = m;
            }
        } else
            *emList++ = m;
    }

    return emList;
}

}    // namespace

namespace gen {

move_t *pawn_moves(const Position& pos, move_t *emList, bitboard_t targets, bool subPromotions)
{
    const Color us = pos.turn(), them = ~us;
    const int push = push_inc(us);
    const bitboard_t capturable = pos.by_color(them) | ep_square_bb(pos);
    bitboard_t fss, tss;
    Move m;

    // Non promotions
    fss = pieces(pos, us, PAWN) & ~bb::rank(relative_rank(us, RANK_7));

    while (fss) {
        m.from = bb::pop_lsb(fss);

        // Calculate to squares: captures, single pushes and double pushes
        tss = bb::pattacks(us, m.from) & capturable & targets;

        if (bb::test(~pieces(pos), m.from + push)) {
            if (bb::test(targets, m.from + push))
                bb::set(tss, m.from + push);

            if (relative_rank(us, m.from) == RANK_2
                    && bb::test(targets & ~pieces(pos), m.from + 2 * push))
                bb::set(tss, m.from + 2 * push);
        }

        // Generate moves
        m.prom = NB_PIECE;
        emList = serialize_moves<false>(m, tss, emList);
    }

    // Promotions
    fss = pieces(pos, us, PAWN) & bb::rank(relative_rank(us, RANK_7));

    while (fss) {
        m.from = bb::pop_lsb(fss);

        // Calculate to squares: captures and single pushes
        tss = bb::pattacks(us, m.from) & capturable & targets;

        if (bb::test(targets & ~pieces(pos), m.from + push))
            bb::set(tss, m.from + push);

        // Generate moves (or promotions)
        emList = serialize_moves<true>(m, tss, emList, subPromotions);
    }

    return emList;
}

move_t *piece_moves(const Position& pos, move_t *emList, bitboard_t targets, bool kingMoves)
{
    const Color us = pos.turn();
    bitboard_t fss, tss;

    Move m;
    m.prom = NB_PIECE;

    // King moves
    if (kingMoves) {
        m.from = king_square(pos, us);
        tss = bb::kattacks(m.from) & targets;
        emList = serialize_moves<false>(m, tss, emList);
    }

    // Knight moves
    fss = pieces(pos, us, KNIGHT);

    while (fss) {
        m.from = bb::pop_lsb(fss);
        tss = bb::nattacks(m.from) & targets;
        emList = serialize_moves<false>(m, tss, emList);
    }

    // Rook moves
    fss = pieces(pos, us, ROOK, QUEEN);

    while (fss) {
        m.from = bb::pop_lsb(fss);
        tss = bb::rattacks(m.from, pieces(pos)) & targets;
        emList = serialize_moves<false>(m, tss, emList);
    }

    // Bishop moves
    fss = pieces(pos, us, BISHOP, QUEEN);

    while (fss) {
        m.from = bb::pop_lsb(fss);
        tss = bb::battacks(m.from, pieces(pos)) & targets;
        emList = serialize_moves<false>(m, tss, emList);
    }

    return emList;
}

move_t *castling_moves(const Position& pos, move_t *emList)
{
    assert(!pos.checkers());
    Move m;
    m.from = king_square(pos, pos.turn());
    m.prom = NB_PIECE;

    bitboard_t tss = pos.castlable_rooks() & pos.by_color(pos.turn());

    while (tss) {
        m.to = bb::pop_lsb(tss);
        const Square kto = square(rank_of(m.to), m.to > m.from ? FILE_G : FILE_C);
        const Square rto = square(rank_of(m.to), m.to > m.from ? FILE_F : FILE_D);
        const bitboard_t s = bb::segment(m.from, kto) | bb::segment(m.to, rto);

        if (bb::count(s & pieces(pos)) == 2)
            *emList++ = m;
    }

    return emList;
}

move_t *check_escapes(const Position& pos, move_t *emList, bool subPromotions)
{
    assert(pos.checkers());
    bitboard_t ours = pos.by_color(pos.turn());
    const Square king = king_square(pos, pos.turn());
    bitboard_t tss;
    Move m;

    // King moves
    tss = bb::kattacks(king) & ~ours;
    m.from = king;
    m.prom = NB_PIECE;
    emList = serialize_moves<false>(m, tss, emList);

    if (!bb::several(pos.checkers())) {
        // Single checker
        const Square checkerSquare = bb::lsb(pos.checkers());
        const Piece checkerPiece = pos.piece_on(checkerSquare);

        // Piece moves must cover the checking segment for a sliding check, or capture the
        // checker otherwise.
        tss = BISHOP <= checkerPiece && checkerPiece <= QUEEN
              ? bb::segment(king, checkerSquare)
              : pos.checkers();

        emList = piece_moves(pos, emList, tss & ~ours, false);

        // if checked by a Pawn and epsq is available, then the check must result from a
        // pawn double push, and we also need to consider capturing it en-passant to solve
        // the check.
        if (checkerPiece == PAWN && pos.ep_square() < NB_SQUARE)
            bb::set(tss, pos.ep_square());

        emList = pawn_moves(pos, emList, tss, subPromotions);
    }

    return emList;
}

move_t *all_moves(const Position& pos, move_t *emList)
{
    if (pos.checkers())
        return check_escapes(pos, emList);
    else {
        bitboard_t targets = ~pos.by_color(pos.turn());
        move_t *em = emList;

        em = pawn_moves(pos, em, targets);
        em = piece_moves(pos, em, targets);
        em = castling_moves(pos, em);
        return em;
    }
}

template <bool Root>
uint64_t perft(const Position& pos, int depth)
{
    // Do not use bulk-counting. It's faster, but falses profiling results.
    if (depth <= 0)
        return 1;

    uint64_t result = 0;
    Position after;
    move_t emList[MAX_MOVES];
    move_t *end = all_moves(pos, emList);

    for (move_t *em = emList; em != end; em++) {
        const Move m(*em);

        if (!m.pseudo_is_legal(pos))
            continue;

        after.set(pos, m);
        const uint64_t sub_tree = perft<false>(after, depth - 1);
        result += sub_tree;

        if (Root)
            std::cout << m.to_string(pos) << '\t' << sub_tree << std::endl;
    }

    return result;
}

template uint64_t perft<true>(const Position& pos, int depth);

}    // namespace gen
