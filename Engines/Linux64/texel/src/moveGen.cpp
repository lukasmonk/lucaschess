/*
    Texel - A UCI chess engine.
    Copyright (C) 2012-2015  Peter Österlund, peterosterlund2@gmail.com

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

/*
 * moveGen.cpp
 *
 *  Created on: Feb 25, 2012
 *      Author: petero
 */

#include "moveGen.hpp"

void
MoveList::filter(const std::vector<Move>& searchMoves)
{
    int used = 0;
    for (int i = 0;i < size; i++)
        if (std::find(searchMoves.begin(), searchMoves.end(), (*this)[i]) != searchMoves.end())
            (*this)[used++] = (*this)[i];
    size = used;
}

template void MoveGen::pseudoLegalMoves<true>(const Position& pos, MoveList& moveList);
template void MoveGen::pseudoLegalMoves<false>(const Position& pos, MoveList& moveList);

template <bool wtm>
void
MoveGen::pseudoLegalMoves(const Position& pos, MoveList& moveList) {
    using MyColor = ColorTraits<wtm>;
    const U64 occupied = pos.occupiedBB();

    // Queen moves
    U64 squares = pos.pieceTypeBB(MyColor::QUEEN);
    while (squares != 0) {
        int sq = BitBoard::extractSquare(squares);
        U64 m = (BitBoard::rookAttacks(sq, occupied) | BitBoard::bishopAttacks(sq, occupied)) & ~pos.colorBB(wtm);
        addMovesByMask(moveList, sq, m);
    }

    // Rook moves
    squares = pos.pieceTypeBB(MyColor::ROOK);
    while (squares != 0) {
        int sq = BitBoard::extractSquare(squares);
        U64 m = BitBoard::rookAttacks(sq, occupied) & ~pos.colorBB(wtm);
        addMovesByMask(moveList, sq, m);
    }

    // Bishop moves
    squares = pos.pieceTypeBB(MyColor::BISHOP);
    while (squares != 0) {
        int sq = BitBoard::extractSquare(squares);
        U64 m = BitBoard::bishopAttacks(sq, occupied) & ~pos.colorBB(wtm);
        addMovesByMask(moveList, sq, m);
    }

    // King moves
    {
        int sq = pos.getKingSq(wtm);
        U64 m = BitBoard::kingAttacks[sq] & ~pos.colorBB(wtm);
        addMovesByMask(moveList, sq, m);
        const int k0 = wtm ? E1 : E8;
        if (sq == k0) {
            const U64 OO_SQ = wtm ? BitBoard::sqMask(F1,G1) : BitBoard::sqMask(F8,G8);
            const U64 OOO_SQ = wtm ? BitBoard::sqMask(B1,C1,D1) : BitBoard::sqMask(B8,C8,D8);
            const int hCastle = wtm ? Position::H1_CASTLE : Position::H8_CASTLE;
            const int aCastle = wtm ? Position::A1_CASTLE : Position::A8_CASTLE;
            if (((pos.getCastleMask() & (1 << hCastle)) != 0) &&
                ((OO_SQ & occupied) == 0) &&
                (pos.getPiece(k0 + 3) == MyColor::ROOK) &&
                !sqAttacked(pos, k0) &&
                !sqAttacked(pos, k0 + 1)) {
                moveList.addMove(k0, k0 + 2, Piece::EMPTY);
            }
            if (((pos.getCastleMask() & (1 << aCastle)) != 0) &&
                ((OOO_SQ & occupied) == 0) &&
                (pos.getPiece(k0 - 4) == MyColor::ROOK) &&
                !sqAttacked(pos, k0) &&
                !sqAttacked(pos, k0 - 1)) {
                moveList.addMove(k0, k0 - 2, Piece::EMPTY);
            }
        }
    }

    // Knight moves
    U64 knights = pos.pieceTypeBB(MyColor::KNIGHT);
    while (knights != 0) {
        int sq = BitBoard::extractSquare(knights);
        U64 m = BitBoard::knightAttacks[sq] & ~pos.colorBB(wtm);
        addMovesByMask(moveList, sq, m);
    }

    // Pawn moves
    const U64 pawns = pos.pieceTypeBB(MyColor::PAWN);
    const int epSquare = pos.getEpSquare();
    const U64 epMask = (epSquare >= 0) ? (1ULL << epSquare) : 0ULL;
    if (wtm) {
        U64 m = (pawns << 8) & ~occupied;
        addPawnMovesByMask<wtm>(moveList, m, -8, true);
        m = ((m & BitBoard::maskRow3) << 8) & ~occupied;
        addPawnDoubleMovesByMask(moveList, m, -16);

        m = (pawns << 7) & BitBoard::maskAToGFiles & (pos.colorBB(!wtm) | epMask);
        addPawnMovesByMask<wtm>(moveList, m, -7, true);

        m = (pawns << 9) & BitBoard::maskBToHFiles & (pos.colorBB(!wtm) | epMask);
        addPawnMovesByMask<wtm>(moveList, m, -9, true);
    } else {
        U64 m = (pawns >> 8) & ~occupied;
        addPawnMovesByMask<wtm>(moveList, m, 8, true);
        m = ((m & BitBoard::maskRow6) >> 8) & ~occupied;
        addPawnDoubleMovesByMask(moveList, m, 16);

        m = (pawns >> 9) & BitBoard::maskAToGFiles & (pos.colorBB(!wtm) | epMask);
        addPawnMovesByMask<wtm>(moveList, m, 9, true);

        m = (pawns >> 7) & BitBoard::maskBToHFiles & (pos.colorBB(!wtm) | epMask);
        addPawnMovesByMask<wtm>(moveList, m, 7, true);
    }
}

template void MoveGen::checkEvasions<true>(const Position& pos, MoveList& moveList);
template void MoveGen::checkEvasions<false>(const Position& pos, MoveList& moveList);

template <bool wtm>
void
MoveGen::checkEvasions(const Position& pos, MoveList& moveList) {
    using MyColor = ColorTraits<wtm>;
    using OtherColor = ColorTraits<!wtm>;
    const U64 occupied = pos.occupiedBB();

    const int kingSq = pos.getKingSq(wtm);
    U64 kingThreats = pos.pieceTypeBB(OtherColor::KNIGHT) & BitBoard::knightAttacks[kingSq];
    U64 rookPieces = pos.pieceTypeBB(OtherColor::ROOK, OtherColor::QUEEN);
    if (rookPieces != 0)
        kingThreats |= rookPieces & BitBoard::rookAttacks(kingSq, occupied);
    U64 bishPieces = pos.pieceTypeBB(OtherColor::BISHOP, OtherColor::QUEEN);
    if (bishPieces != 0)
        kingThreats |= bishPieces & BitBoard::bishopAttacks(kingSq, occupied);
    const U64 myPawnAttacks = wtm ? BitBoard::wPawnAttacks[kingSq] : BitBoard::bPawnAttacks[kingSq];
    kingThreats |= pos.pieceTypeBB(OtherColor::PAWN) & myPawnAttacks;
    U64 validTargets = 0;
    if ((kingThreats != 0) && ((kingThreats & (kingThreats-1)) == 0)) { // Exactly one attacking piece
        int threatSq = BitBoard::firstSquare(kingThreats);
        validTargets = kingThreats | BitBoard::squaresBetween[kingSq][threatSq];
    }
    validTargets |= pos.pieceTypeBB(OtherColor::KING);
    // Queen moves
    U64 squares = pos.pieceTypeBB(MyColor::QUEEN);
    while (squares != 0) {
        int sq = BitBoard::extractSquare(squares);
        U64 m = (BitBoard::rookAttacks(sq, occupied) | BitBoard::bishopAttacks(sq, occupied)) &
                    ~pos.colorBB(wtm) & validTargets;
        addMovesByMask(moveList, sq, m);
    }

    // Rook moves
    squares = pos.pieceTypeBB(MyColor::ROOK);
    while (squares != 0) {
        int sq = BitBoard::extractSquare(squares);
        U64 m = BitBoard::rookAttacks(sq, occupied) & ~pos.colorBB(wtm) & validTargets;
        addMovesByMask(moveList, sq, m);
    }

    // Bishop moves
    squares = pos.pieceTypeBB(MyColor::BISHOP);
    while (squares != 0) {
        int sq = BitBoard::extractSquare(squares);
        U64 m = BitBoard::bishopAttacks(sq, occupied) & ~pos.colorBB(wtm) & validTargets;
        addMovesByMask(moveList, sq, m);
    }

    // King moves
    {
        int sq = pos.getKingSq(wtm);
        U64 m = BitBoard::kingAttacks[sq] & ~pos.colorBB(wtm);
        addMovesByMask(moveList, sq, m);
    }

    // Knight moves
    U64 knights = pos.pieceTypeBB(MyColor::KNIGHT);
    while (knights != 0) {
        int sq = BitBoard::extractSquare(knights);
        U64 m = BitBoard::knightAttacks[sq] & ~pos.colorBB(wtm) & validTargets;
        addMovesByMask(moveList, sq, m);
    }

    // Pawn moves
    const U64 pawns = pos.pieceTypeBB(MyColor::PAWN);
    const int epSquare = pos.getEpSquare();
    const U64 epMask = (epSquare >= 0) ? (1ULL << epSquare) : 0ULL;
    if (wtm) {
        U64 m = (pawns << 8) & ~occupied;
        addPawnMovesByMask<wtm>(moveList, m & validTargets, -8, true);
        m = ((m & BitBoard::maskRow3) << 8) & ~occupied;
        addPawnDoubleMovesByMask(moveList, m & validTargets, -16);

        m = (pawns << 7) & BitBoard::maskAToGFiles & ((pos.colorBB(!wtm) & validTargets) | epMask);
        addPawnMovesByMask<wtm>(moveList, m, -7, true);

        m = (pawns << 9) & BitBoard::maskBToHFiles & ((pos.colorBB(!wtm) & validTargets) | epMask);
        addPawnMovesByMask<wtm>(moveList, m, -9, true);
    } else {
        U64 m = (pawns >> 8) & ~occupied;
        addPawnMovesByMask<wtm>(moveList, m & validTargets, 8, true);
        m = ((m & BitBoard::maskRow6) >> 8) & ~occupied;
        addPawnDoubleMovesByMask(moveList, m & validTargets, 16);

        m = (pawns >> 9) & BitBoard::maskAToGFiles & ((pos.colorBB(!wtm) & validTargets) | epMask);
        addPawnMovesByMask<wtm>(moveList, m, 9, true);

        m = (pawns >> 7) & BitBoard::maskBToHFiles & ((pos.colorBB(!wtm) & validTargets) | epMask);
        addPawnMovesByMask<wtm>(moveList, m, 7, true);
    }

#ifdef MOVELIST_DEBUG
    {
        // Extra check that all valid evasions were generated
        MoveList allMoves;
        pseudoLegalMoves(pos, allMoves);
        Position tmpPos(pos);
        removeIllegal(tmpPos, allMoves);
        std::set<std::string> evMoves;
        for (int i = 0; i < moveList.size; i++)
            evMoves.insert(TextIO::moveToUCIString(moveList.m[i]));
        for (int i = 0; i < allMoves.size; i++)
            assert(evMoves.find(TextIO::moveToUCIString(allMoves.m[i])) != evMoves.end());
    }
#endif
}

template void MoveGen::pseudoLegalCapturesAndChecks<true>(const Position& pos, MoveList& moveList);
template void MoveGen::pseudoLegalCapturesAndChecks<false>(const Position& pos, MoveList& moveList);

template <bool wtm>
void
MoveGen::pseudoLegalCapturesAndChecks(const Position& pos, MoveList& moveList) {
    using MyColor = ColorTraits<wtm>;
    const U64 occupied = pos.occupiedBB();

    const int oKingSq = pos.getKingSq(!wtm);
    U64 discovered = 0; // Squares that could generate discovered checks
    U64 kRookAtk = BitBoard::rookAttacks(oKingSq, occupied);
    if ((BitBoard::rookAttacks(oKingSq, occupied & ~kRookAtk) &
            pos.pieceTypeBB(MyColor::QUEEN, MyColor::ROOK)) != 0)
        discovered |= kRookAtk;
    U64 kBishAtk = BitBoard::bishopAttacks(oKingSq, occupied);
    if ((BitBoard::bishopAttacks(oKingSq, occupied & ~kBishAtk) &
            pos.pieceTypeBB(MyColor::QUEEN, MyColor::BISHOP)) != 0)
        discovered |= kBishAtk;

    // Queen moves
    U64 squares = pos.pieceTypeBB(MyColor::QUEEN);
    while (squares != 0) {
        int sq = BitBoard::extractSquare(squares);
        U64 m = (BitBoard::rookAttacks(sq, occupied) | BitBoard::bishopAttacks(sq, occupied));
        if ((discovered & (1ULL<<sq)) == 0) m &= (pos.colorBB(!wtm) | kRookAtk | kBishAtk);
        m &= ~pos.colorBB(wtm);
        addMovesByMask(moveList, sq, m);
    }

    // Rook moves
    squares = pos.pieceTypeBB(MyColor::ROOK);
    while (squares != 0) {
        int sq = BitBoard::extractSquare(squares);
        U64 m = BitBoard::rookAttacks(sq, occupied);
        if ((discovered & (1ULL<<sq)) == 0) m &= (pos.colorBB(!wtm) | kRookAtk);
        m &= ~pos.colorBB(wtm);
        addMovesByMask(moveList, sq, m);
    }

    // Bishop moves
    squares = pos.pieceTypeBB(MyColor::BISHOP);
    while (squares != 0) {
        int sq = BitBoard::extractSquare(squares);
        U64 m = BitBoard::bishopAttacks(sq, occupied);
        if ((discovered & (1ULL<<sq)) == 0) m &= (pos.colorBB(!wtm) | kBishAtk);
        m &= ~pos.colorBB(wtm);
        addMovesByMask(moveList, sq, m);
    }

    // King moves
    {
        int sq = pos.getKingSq(wtm);
        U64 m = BitBoard::kingAttacks[sq];
        m &= ((discovered & (1ULL<<sq)) == 0) ? pos.colorBB(!wtm) : ~pos.colorBB(wtm);
        addMovesByMask(moveList, sq, m);
        const int k0 = wtm ? E1 : E8;
        if (sq == k0) {
            const U64 OO_SQ = wtm ? BitBoard::sqMask(F1,G1) : BitBoard::sqMask(F8,G8);
            const U64 OOO_SQ = wtm ? BitBoard::sqMask(B1,C1,D1) : BitBoard::sqMask(B8,C8,D8);
            const int hCastle = wtm ? Position::H1_CASTLE : Position::H8_CASTLE;
            const int aCastle = wtm ? Position::A1_CASTLE : Position::A8_CASTLE;
            if (((pos.getCastleMask() & (1 << hCastle)) != 0) &&
                ((OO_SQ & occupied) == 0) &&
                (pos.getPiece(k0 + 3) == MyColor::ROOK) &&
                !sqAttacked(pos, k0) &&
                !sqAttacked(pos, k0 + 1)) {
                moveList.addMove(k0, k0 + 2, Piece::EMPTY);
            }
            if (((pos.getCastleMask() & (1 << aCastle)) != 0) &&
                ((OOO_SQ & occupied) == 0) &&
                (pos.getPiece(k0 - 4) == MyColor::ROOK) &&
                !sqAttacked(pos, k0) &&
                !sqAttacked(pos, k0 - 1)) {
                moveList.addMove(k0, k0 - 2, Piece::EMPTY);
            }
        }
    }

    // Knight moves
    U64 knights = pos.pieceTypeBB(MyColor::KNIGHT);
    U64 kKnightAtk = BitBoard::knightAttacks[oKingSq];
    while (knights != 0) {
        int sq = BitBoard::extractSquare(knights);
        U64 m = BitBoard::knightAttacks[sq] & ~pos.colorBB(wtm);
        if ((discovered & (1ULL<<sq)) == 0) m &= (pos.colorBB(!wtm) | kKnightAtk);
        addMovesByMask(moveList, sq, m);
    }

    // Pawn moves
    const U64 pawns = pos.pieceTypeBB(MyColor::PAWN);
    const int epSquare = pos.getEpSquare();
    const U64 epMask = (epSquare >= 0) ? (1ULL << epSquare) : 0ULL;
    if (wtm) {
        // Captures
        U64 m = (pawns << 7) & BitBoard::maskAToGFiles & (pos.colorBB(!wtm) | epMask);
        addPawnMovesByMask<wtm>(moveList, m, -7, false);
        m = (pawns << 9) & BitBoard::maskBToHFiles & (pos.colorBB(!wtm) | epMask);
        addPawnMovesByMask<wtm>(moveList, m, -9, false);

        // Discovered checks and promotions
        U64 pawnAll = discovered | BitBoard::maskRow7;
        m = ((pawns & pawnAll) << 8) & ~occupied;
        addPawnMovesByMask<wtm>(moveList, m, -8, false);
        m = ((m & BitBoard::maskRow3) << 8) & ~occupied;
        addPawnDoubleMovesByMask(moveList, m, -16);

        // Normal checks
        m = ((pawns & ~pawnAll) << 8) & ~occupied;
        addPawnMovesByMask<wtm>(moveList, m & BitBoard::bPawnAttacks[oKingSq], -8, false);
        m = ((m & BitBoard::maskRow3) << 8) & ~occupied;
        addPawnDoubleMovesByMask(moveList, m & BitBoard::bPawnAttacks[oKingSq], -16);
    } else {
        // Captures
        U64 m = (pawns >> 9) & BitBoard::maskAToGFiles & (pos.colorBB(!wtm) | epMask);
        addPawnMovesByMask<wtm>(moveList, m, 9, false);
        m = (pawns >> 7) & BitBoard::maskBToHFiles & (pos.colorBB(!wtm) | epMask);
        addPawnMovesByMask<wtm>(moveList, m, 7, false);

        // Discovered checks and promotions
        U64 pawnAll = discovered | BitBoard::maskRow2;
        m = ((pawns & pawnAll) >> 8) & ~occupied;
        addPawnMovesByMask<wtm>(moveList, m, 8, false);
        m = ((m & BitBoard::maskRow6) >> 8) & ~occupied;
        addPawnDoubleMovesByMask(moveList, m, 16);

        // Normal checks
        m = ((pawns & ~pawnAll) >> 8) & ~occupied;
        addPawnMovesByMask<wtm>(moveList, m & BitBoard::wPawnAttacks[oKingSq], 8, false);
        m = ((m & BitBoard::maskRow6) >> 8) & ~occupied;
        addPawnDoubleMovesByMask(moveList, m & BitBoard::wPawnAttacks[oKingSq], 16);
    }
}

template void MoveGen::pseudoLegalCaptures<true>(const Position& pos, MoveList& moveList);
template void MoveGen::pseudoLegalCaptures<false>(const Position& pos, MoveList& moveList);

template <bool wtm>
void
MoveGen::pseudoLegalCaptures(const Position& pos, MoveList& moveList) {
    using MyColor = ColorTraits<wtm>;
    const U64 occupied = pos.occupiedBB();

    // Queen moves
    U64 squares = pos.pieceTypeBB(MyColor::QUEEN);
    while (squares != 0) {
        int sq = BitBoard::extractSquare(squares);
        U64 m = (BitBoard::rookAttacks(sq, occupied) | BitBoard::bishopAttacks(sq, occupied)) & pos.colorBB(!wtm);
        addMovesByMask(moveList, sq, m);
    }

    // Rook moves
    squares = pos.pieceTypeBB(MyColor::ROOK);
    while (squares != 0) {
        int sq = BitBoard::extractSquare(squares);
        U64 m = BitBoard::rookAttacks(sq, occupied) & pos.colorBB(!wtm);
        addMovesByMask(moveList, sq, m);
    }

    // Bishop moves
    squares = pos.pieceTypeBB(MyColor::BISHOP);
    while (squares != 0) {
        int sq = BitBoard::extractSquare(squares);
        U64 m = BitBoard::bishopAttacks(sq, occupied) & pos.colorBB(!wtm);
        addMovesByMask(moveList, sq, m);
    }

    // Knight moves
    U64 knights = pos.pieceTypeBB(MyColor::KNIGHT);
    while (knights != 0) {
        int sq = BitBoard::extractSquare(knights);
        U64 m = BitBoard::knightAttacks[sq] & pos.colorBB(!wtm);
        addMovesByMask(moveList, sq, m);
    }

    // King moves
    int sq = pos.getKingSq(wtm);
    U64 m = BitBoard::kingAttacks[sq] & pos.colorBB(!wtm);
    addMovesByMask(moveList, sq, m);

    // Pawn moves
    const U64 pawns = pos.pieceTypeBB(MyColor::PAWN);
    const int epSquare = pos.getEpSquare();
    const U64 epMask = (epSquare >= 0) ? (1ULL << epSquare) : 0ULL;
    if (wtm) {
        m = (pawns << 8) & ~occupied;
        m &= BitBoard::maskRow8;
        addPawnMovesByMask<wtm>(moveList, m, -8, false);

        m = (pawns << 7) & BitBoard::maskAToGFiles & (pos.colorBB(!wtm) | epMask);
        addPawnMovesByMask<wtm>(moveList, m, -7, false);
        m = (pawns << 9) & BitBoard::maskBToHFiles & (pos.colorBB(!wtm) | epMask);
        addPawnMovesByMask<wtm>(moveList, m, -9, false);
    } else {
        m = (pawns >> 8) & ~occupied;
        m &= BitBoard::maskRow1;
        addPawnMovesByMask<wtm>(moveList, m, 8, false);

        m = (pawns >> 9) & BitBoard::maskAToGFiles & (pos.colorBB(!wtm) | epMask);
        addPawnMovesByMask<wtm>(moveList, m, 9, false);
        m = (pawns >> 7) & BitBoard::maskBToHFiles & (pos.colorBB(!wtm) | epMask);
        addPawnMovesByMask<wtm>(moveList, m, 7, false);
    }
}

bool
MoveGen::givesCheck(const Position& pos, const Move& m) {
    bool wtm = pos.isWhiteMove();
    int oKingSq = pos.getKingSq(!wtm);
    int oKing = wtm ? Piece::BKING : Piece::WKING;
    int p = Piece::makeWhite(m.promoteTo() == Piece::EMPTY ? pos.getPiece(m.from()) : m.promoteTo());
    int d1 = BitBoard::getDirection(m.to(), oKingSq);
    switch (d1) {
    case 8: case -8: case 1: case -1: // Rook direction
        if ((p == Piece::WQUEEN) || (p == Piece::WROOK))
            if ((d1 != 0) && (nextPiece(pos, m.to(), d1) == oKing))
                return true;
        break;
    case 9: case 7: case -9: case -7: // Bishop direction
        if ((p == Piece::WQUEEN) || (p == Piece::WBISHOP)) {
            if ((d1 != 0) && (nextPiece(pos, m.to(), d1) == oKing))
                return true;
        } else if (p == Piece::WPAWN) {
            if (((d1 > 0) == wtm) && (pos.getPiece(m.to() + d1) == oKing))
                return true;
        }
        break;
    default:
        if (d1 != 0) { // Knight direction
            if (p == Piece::WKNIGHT)
                return true;
        }
        break;
    }
    int d2 = BitBoard::getDirection(m.from(), oKingSq);
    if ((d2 != 0) && (d2 != d1) && (nextPiece(pos, m.from(), d2) == oKing)) {
        int p2 = nextPieceSafe(pos, m.from(), -d2);
        switch (d2) {
        case 8: case -8: case 1: case -1: // Rook direction
            if ((p2 == (wtm ? Piece::WQUEEN : Piece::BQUEEN)) ||
                (p2 == (wtm ? Piece::WROOK : Piece::BROOK)))
                return true;
            break;
        case 9: case 7: case -9: case -7: // Bishop direction
            if ((p2 == (wtm ? Piece::WQUEEN : Piece::BQUEEN)) ||
                (p2 == (wtm ? Piece::WBISHOP : Piece::BBISHOP)))
                return true;
            break;
        }
    }
    if ((m.promoteTo() != Piece::EMPTY) && (d1 != 0) && (d1 == d2)) {
        switch (d1) {
        case 8: case -8: case 1: case -1: // Rook direction
            if ((p == Piece::WQUEEN) || (p == Piece::WROOK))
                if ((d1 != 0) && (nextPiece(pos, m.from(), d1) == oKing))
                    return true;
            break;
        case 9: case 7: case -9: case -7: // Bishop direction
            if ((p == Piece::WQUEEN) || (p == Piece::WBISHOP)) {
                if ((d1 != 0) && (nextPiece(pos, m.from(), d1) == oKing))
                    return true;
            }
            break;
        }
    }
    if (p == Piece::WKING) {
        if (m.to() - m.from() == 2) { // O-O
            if (nextPieceSafe(pos, m.from(), -1) == oKing)
                return true;
            if (nextPieceSafe(pos, m.from() + 1, wtm ? 8 : -8) == oKing)
                return true;
        } else if (m.to() - m.from() == -2) { // O-O-O
            if (nextPieceSafe(pos, m.from(), 1) == oKing)
                return true;
            if (nextPieceSafe(pos, m.from() - 1, wtm ? 8 : -8) == oKing)
                return true;
        }
    } else if (p == Piece::WPAWN) {
        if (pos.getPiece(m.to()) == Piece::EMPTY) {
            int dx = Position::getX(m.to()) - Position::getX(m.from());
            if (dx != 0) { // en passant
                int epSq = m.from() + dx;
                int d3 = BitBoard::getDirection(epSq, oKingSq);
                switch (d3) {
                case 9: case 7: case -9: case -7:
                    if (nextPiece(pos, epSq, d3) == oKing) {
                        int p2 = nextPieceSafe(pos, epSq, -d3);
                        if ((p2 == (wtm ? Piece::WQUEEN : Piece::BQUEEN)) ||
                            (p2 == (wtm ? Piece::WBISHOP : Piece::BBISHOP)))
                            return true;
                    }
                    break;
                case 1:
                    if (nextPiece(pos, std::max(epSq, m.from()), d3) == oKing) {
                        int p2 = nextPieceSafe(pos, std::min(epSq, m.from()), -d3);
                        if ((p2 == (wtm ? Piece::WQUEEN : Piece::BQUEEN)) ||
                            (p2 == (wtm ? Piece::WROOK : Piece::BROOK)))
                            return true;
                    }
                    break;
                case -1:
                    if (nextPiece(pos, std::min(epSq, m.from()), d3) == oKing) {
                        int p2 = nextPieceSafe(pos, std::max(epSq, m.from()), -d3);
                        if ((p2 == (wtm ? Piece::WQUEEN : Piece::BQUEEN)) ||
                            (p2 == (wtm ? Piece::WROOK : Piece::BROOK)))
                            return true;
                    }
                    break;
                }
            }
        }
    }
    return false;
}

void
MoveGen::removeIllegal(Position& pos, MoveList& moveList) {
    int length = 0;
    UndoInfo ui;

    bool isInCheck = inCheck(pos);
    const U64 occupied = pos.occupiedBB();
    int kSq = pos.getKingSq(pos.isWhiteMove());
    U64 kingAtks = BitBoard::rookAttacks(kSq, occupied) | BitBoard::bishopAttacks(kSq, occupied);
    int epSquare = pos.getEpSquare();
    if (isInCheck) {
        kingAtks |= pos.pieceTypeBB(pos.isWhiteMove() ? Piece::BKNIGHT : Piece::WKNIGHT);
        for (int mi = 0; mi < moveList.size; mi++) {
            const Move& m = moveList[mi];
            bool legal;
            if ((m.from() != kSq) && ((kingAtks & (1ULL<<m.to())) == 0) && (m.to() != epSquare)) {
                legal = false;
            } else {
                pos.makeMove(m, ui);
                pos.setWhiteMove(!pos.isWhiteMove());
                legal = !inCheck(pos);
                pos.setWhiteMove(!pos.isWhiteMove());
                pos.unMakeMove(m, ui);
            }
            if (legal)
                moveList[length++] = m;
        }
    } else {
        for (int mi = 0; mi < moveList.size; mi++) {
            const Move& m = moveList[mi];
            bool legal;
            if ((m.from() != kSq) && ((kingAtks & (1ULL<<m.from())) == 0) && (m.to() != epSquare)) {
                legal = true;
            } else {
                pos.makeMove(m, ui);
                pos.setWhiteMove(!pos.isWhiteMove());
                legal = !inCheck(pos);
                pos.setWhiteMove(!pos.isWhiteMove());
                pos.unMakeMove(m, ui);
            }
            if (legal)
                moveList[length++] = m;
        }
    }
    moveList.size = length;
}

bool
MoveGen::isLegal(Position& pos, const Move& m, bool isInCheck) {
    UndoInfo ui;
    int kSq = pos.getKingSq(pos.isWhiteMove());
    const int epSquare = pos.getEpSquare();
    if (isInCheck) {
        if ((m.from() != kSq) && (m.to() != epSquare)) {
            U64 occupied = pos.occupiedBB();
            U64 toMask = 1ULL << m.to();
            Piece::Type knight = pos.isWhiteMove() ? Piece::BKNIGHT : Piece::WKNIGHT;
            if (((BitBoard::rookAttacks(kSq, occupied) & toMask) == 0) &&
                ((BitBoard::bishopAttacks(kSq, occupied) & toMask) == 0) &&
                ((BitBoard::knightAttacks[kSq] & pos.pieceTypeBB(knight) & toMask) == 0))
                return false;
        }
        pos.makeMoveB(m, ui);
        bool legal = !inCheck(pos);
        pos.unMakeMoveB(m, ui);
        return legal;
    } else {
        if (m.from() == kSq) {
            U64 occupied = pos.occupiedBB() & ~(1ULL<<m.from());
            return !MoveGen::sqAttacked(pos, m.to(), occupied);
        } else {
            if (m.to() != epSquare) {
                U64 occupied = pos.occupiedBB();
                U64 fromMask = 1ULL << m.from();
                if (((BitBoard::rookAttacks(kSq, occupied) & fromMask) == 0) &&
                    ((BitBoard::bishopAttacks(kSq, occupied) & fromMask) == 0))
                    return true;
                else if (BitBoard::getDirection(kSq, m.from()) == BitBoard::getDirection(kSq, m.to()))
                    return true;
            }
            pos.makeMoveB(m, ui);
            bool legal = !inCheck(pos);
            pos.unMakeMoveB(m, ui);
            return legal;
        }
    }
}
