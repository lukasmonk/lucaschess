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
#include "move.h"
#include "bitboard.h"
#include "position.h"

bool Move::ok() const
{
    return unsigned(from) < NB_SQUARE && unsigned(to) < NB_SQUARE
           && ((KNIGHT <= prom && prom <= QUEEN) || prom == NB_PIECE);
}

Move::operator move_t() const
{
    assert(ok());
    return from | (to << 6) | (prom << 12);
}

Move Move::operator =(move_t em)
{
    from = Square(em & 077);
    to = Square((em >> 6) & 077);
    prom = Piece(em >> 12);
    assert(ok());
    return *this;
}

bool Move::is_capture(const Position& pos) const
{
    const Color us = pos.turn(), them = ~us;
    return (bb::test(pos.by_color(them), to))
           || ((to == pos.ep_square() || relative_rank(us, to) == RANK_8)
               && pos.piece_on(from) == PAWN);
}

bool Move::is_castling(const Position& pos) const
{
    return bb::test(pos.by_color(pos.turn()), to);
}

std::string Move::to_string(const Position& pos) const
{
    assert(ok());

    std::string s;

    if (null())
        return "0000";

    const Square _tsq = !Chess960 && is_castling(pos)
                        ? (to > from ? from + 2 : from - 2)    // e1h1 -> e1g1, e1a1 -> e1c1
                        : to;

    s += file_of(from) + 'a';
    s += rank_of(from) + '1';
    s += file_of(_tsq) + 'a';
    s += rank_of(_tsq) + '1';

    if (prom < NB_PIECE)
        s += PieceLabel[BLACK][prom];

    return s;
}

void Move::from_string(const Position& pos, const std::string& s)
{
    from = square(Rank(s[1] - '1'), File(s[0] - 'a'));
    to = square(Rank(s[3] - '1'), File(s[2] - 'a'));
    prom = s[4] ? (Piece)PieceLabel[BLACK].find(s[4]) : NB_PIECE;

    if (!Chess960 && pos.piece_on(from) == KING) {
        if (to == from + 2)      // e1g1
            ++to;                // -> e1h1
        else if (to == from - 2) // e1c1
            to -= 2;             // -> e1a1
    }

    assert(ok());
}

bool Move::pseudo_is_legal(const Position& pos) const
{
    const Piece p = pos.piece_on(from);
    const Square king = king_square(pos, pos.turn());

    if (p == KING) {
        if (bb::test(pos.by_color(pos.turn()), to)) {
            // Castling: king must not move through attacked square, and rook must not
            // be pinned
            assert(pos.piece_on(to) == ROOK);
            const Square _tsq = square(rank_of(from), from < to ? FILE_G : FILE_C);
            return !(pos.attacked() & bb::segment(from, _tsq))
                   && !bb::test(pos.pins(), to);
        } else
            // Normal king move: do not land on an attacked square
            return !bb::test(pos.attacked(), to);
    } else {
        // Normal case: illegal if pinned, and moves out of pin-ray
        if (bb::test(pos.pins(), from) && !bb::test(bb::ray(king, from), to))
            return false;

        // En-passant special case: also illegal if self-check through the en-passant
        // captured pawn
        if (to == pos.ep_square() && p == PAWN) {
            const Color us = pos.turn(), them = ~us;
            bitboard_t occ = pieces(pos);
            bb::clear(occ, from);
            bb::set(occ, to);
            bb::clear(occ, to + push_inc(them));
            return !(bb::rattacks(king, occ) & pieces(pos, them, ROOK, QUEEN))
                   && !(bb::battacks(king, occ) & pieces(pos, them, BISHOP, QUEEN));
        } else
            return true;
    }
}

int Move::see(const Position& pos) const
{
    const int see_value[NB_PIECE+1] = {N, B, R, Q, MATE, P, 0};

    Color us = pos.turn();
    bitboard_t occ = pieces(pos);

    // General case
    int gain[32] = {see_value[pos.piece_on(to)]};
    Piece capture = pos.piece_on(from);
    bb::clear(occ, from);

    // Special cases
    if (capture == PAWN) {
        if (to == pos.ep_square()) {
            bb::clear(occ, to - push_inc(us));
            gain[0] = see_value[capture];
        } else if (relative_rank(us, to) == RANK_8)
            gain[0] += see_value[capture = prom] - see_value[PAWN];
    }

    // Easy case: to is not defended
    // TODO: explore performance tradeoff between using pos.attacked() and using attackers below
    if (!bb::test(pos.attacked(), to))
        return gain[0];

    bitboard_t attackers = attackers_to(pos, to, occ);
    bitboard_t our_attackers;

    int idx = 0;

    while (us = ~us, our_attackers = attackers & pos.by_color(us)) {
        // Find least valuable attacker (LVA)
        Piece p = PAWN;

        if (!(our_attackers & pieces(pos, us, PAWN))) {
            for (p = KNIGHT; p <= KING; ++p) {
                if (our_attackers & pieces(pos, us, p))
                    break;
            }
        }

        // Remove the LVA
        bb::clear(occ, bb::lsb(our_attackers & pieces(pos, us, p)));

        // Scan for new X-ray attacks through the LVA
        if (p != KNIGHT) {
            attackers |= (pos.by_piece(BISHOP) | pos.by_piece(QUEEN))
                         & bb::bpattacks(to) & bb::battacks(to, occ);
            attackers |= (pos.by_piece(ROOK) | pos.by_piece(QUEEN))
                         & bb::rpattacks(to) & bb::rattacks(to, occ);
        }

        // Remove attackers we've already done
        attackers &= occ;

        // Add the new entry to the gain[] array
        idx++;
        assert(idx < 32);
        gain[idx] = see_value[capture] - gain[idx-1];

        if (p == PAWN && relative_rank(us, to) == RANK_8) {
            gain[idx] += see_value[QUEEN] - see_value[PAWN];
            capture = QUEEN;
        } else
            capture = p;
    }

    do {
        if (-gain[idx] < gain[idx-1])
            gain[idx-1] = -gain[idx];
    } while (--idx);

    return gain[0];
}
