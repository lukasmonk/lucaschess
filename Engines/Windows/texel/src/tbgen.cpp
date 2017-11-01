/*
    Texel - A UCI chess engine.
    Copyright (C) 2015  Peter Österlund, peterosterlund2@gmail.com

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
 * tbgen.cpp
 *
 *  Created on: Apr 3, 2015
 *      Author: petero
 */

#include "tbgen.hpp"
#include "util/timeUtil.hpp"
#include "textio.hpp"
#include "constants.hpp"
#include "transpositionTable.hpp"


static StaticInitializer<TBIndex> tbIdxInit;

int TBIndex::symType[64];
int TBIndex::kingMap[64];
int TBIndex::kingMapInverse[10];


TBIndex::TBIndex(int nWhite0, int nBlack, U32 index)
    : p(nWhite0 + nBlack), nWhite(nWhite0), idx(index) {
    colBits = 0;
    rowBits = 0;
    for (int i = 0; i < p-1; i++) {
        colBits |= 0x07 << (6*i);
        rowBits |= 0x38 << (6*i);
    }
}

void
TBIndex::setSquare(int pieceNo, int square) {
    if (pieceNo == 0) { // White king
        idx &= ~(0xf << (6*p-5));
        idx |= kingMap[square] << (6*p-5);

        int sym = symType[square];
        if (sym & 1)
            mirrorX();
        if (sym & 2)
            mirrorY();
        if (sym & 4)
            mirrorD();
    } else if (pieceNo == nWhite) { // Black king
        int oldSq = getSquare(pieceNo);
        for (int i = 1; i < p; i++) {
            if (getSquare(i) == oldSq) {
                idx &= ~(0x3f << pieceShift(i));
                idx |= square << pieceShift(i);
            }
        }
    } else {
        idx &= ~(0x3f << pieceShift(pieceNo));
        idx |= square << pieceShift(pieceNo);
    }
}

int
TBIndex::getSquare(int pieceNo) const {
    if (pieceNo == 0) {
        return kingMapInverse[(idx >> (6*p-5)) & 0xf];
    } else {
        return (idx >> pieceShift(pieceNo)) & 0x3f;
    }
}

void
TBIndex::canonize(const std::vector<int>& pieceTypes, bool duplicatedPieces) {
    if (duplicatedPieces)
        sortPieces(pieceTypes);
    if ((1ULL << getSquare(0)) & BitBoard::sqMask(A1,B2,C3,D4)) {
        U32 idx0 = idx;
        mirrorD();
        if (duplicatedPieces)
            sortPieces(pieceTypes);
        idx = std::min(idx, idx0);
    }
}

void
TBIndex::sortPieces(const std::vector<int>& pieceTypes) {
    for (int i = 1; i < p; i++) {
        for (int j = i+1; j < p; j++) {
            if (pieceTypes[i] != pieceTypes[j])
                break;
            int sqI = getSquare(i);
            int sqJ = getSquare(j);
            if (sqJ < sqI) {
                setSquare(i, sqJ);
                setSquare(j, sqI);
            }
        }
    }
}

void
TBIndex::staticInitialize() {
    for (int sq = 0; sq < 64; sq++) {
        int sym = 0;
        int mSq = sq;
        if (Position::getX(mSq) >= 4) {
            sym |= 1;
            mSq ^= 0x07;
        }
        if (Position::getY(mSq) >= 4) {
            sym |= 2;
            mSq ^= 0x38;
        }
        if (Position::getY(mSq) > Position::getX(mSq)) {
            sym |= 4;
        }
        symType[sq] = sym;
        kingMap[sq] = -1;
    }

    int kIdx = 0;
    for (int sq = 0; sq < 64; sq++) {
        if (kingMap[sq] == -1) {
            assert(kIdx < 10);
            kingMap[sq] = kIdx;
            kingMapInverse[kIdx] = sq;
            for (int s = 1; s < 8; s++) {
                int mSq = sq;
                if (s & 1) // Mirror X
                    mSq ^= 0x07;
                if (s & 2) // Mirror Y
                    mSq ^= 0x38;
                if (s & 4) // Mirror D
                    mSq = Position::getSquare(Position::getY(mSq), Position::getX(mSq));
                kingMap[mSq] = kIdx;
            }
            kIdx++;
        }
    }
}

// --------------------------------------------------------------------------------

TBPosition::TBPosition(const PieceCount& pc)
    : currIdx(0, 0, 0) {
    pieceTypes.resize(pc.nPieces());
    int pIdx = 0;
    pieceTypes[pIdx++] = Piece::WKING;
    for (int i = 0; i < pc.nwn; i++)
        pieceTypes[pIdx++] = Piece::WKNIGHT;
    for (int i = 0; i < pc.nwb; i++)
        pieceTypes[pIdx++] = Piece::WBISHOP;
    for (int i = 0; i < pc.nwr; i++)
        pieceTypes[pIdx++] = Piece::WROOK;
    for (int i = 0; i < pc.nwq; i++)
        pieceTypes[pIdx++] = Piece::WQUEEN;

    nWhite = pIdx;

    pieceTypes[pIdx++] = Piece::BKING;
    for (int i = 0; i < pc.nbn; i++)
        pieceTypes[pIdx++] = Piece::BKNIGHT;
    for (int i = 0; i < pc.nbb; i++)
        pieceTypes[pIdx++] = Piece::BBISHOP;
    for (int i = 0; i < pc.nbr; i++)
        pieceTypes[pIdx++] = Piece::BROOK;
    for (int i = 0; i < pc.nbq; i++)
        pieceTypes[pIdx++] = Piece::BQUEEN;

    const int nPieces = pieceTypes.size();
    nPos = 2 * 10;
    for (int i = 1; i < nPieces; i++)
        nPos *= 64;

    duplicatedPieces = false;
    for (int i = 1; i < nPieces; i++)
        for (int j = i+1; j < nPieces; j++)
            if (pieceTypes[i] == pieceTypes[j])
                duplicatedPieces = true;

    currIdx = TBIndex(nWhite, pc.nPieces() - nWhite, 0);
}

bool
TBPosition::indexValid() {
    const int nPieces = pieceTypes.size();
    const int bKingSq = currIdx.getSquare(nWhite);

    if (currIdx.getSquare(0) == bKingSq)
        return false; // White king must be present

    U64 occupied = 0;
    for (int i = 0; i < nPieces; i++) {
        int sq = currIdx.getSquare(i);
        if (sq != bKingSq) {
            if (occupied & (1ULL << sq))
                return false; // More than one piece on sq
            occupied |= 1ULL << sq;
        }
    }

    U32 oldIdx = currIdx.getIndex();
    currIdx.canonize(pieceTypes, duplicatedPieces);
    return currIdx.getIndex() == oldIdx;
}

bool
TBPosition::setPosition(const Position& pos) {
    if (pos.getCastleMask())
        return false;

    U64 pieces[Piece::nPieceTypes];
    for (int p = Piece::WKING; p <= Piece::BPAWN; p++)
        pieces[p] = pos.pieceTypeBB((Piece::Type)p);

    const int bKingSq = pos.bKingSq();
    const int nPieces = pieceTypes.size();

    currIdx.setSquare(nWhite, bKingSq);
    for (int i = 0; i < nPieces; i++) {
        int p = pieceTypes[i];
        if (pieces[p]) {
            int sq = BitBoard::extractSquare(pieces[p]);
            if (p != Piece::WKING && p != Piece::BKING)
                currIdx.setSquare(i, sq);
        } else {
            currIdx.setSquare(i, bKingSq);
        }
    }
    currIdx.setSquare(0, pos.wKingSq());

    if (pos.isWhiteMove() != currIdx.whiteMove())
        currIdx.swapSide();

    for (int p = Piece::WKING; p <= Piece::BPAWN; p++)
        if (pieces[p])
            return false; // Could not place all pieces
    currIdx.canonize(pieceTypes, duplicatedPieces);
    return indexValid();
}

void
TBPosition::getPos(Position& pos) const {
    U64 occupied = getOccupied();
    U64 m = pos.occupiedBB() & ~occupied;
    while (m) {
        int sq = BitBoard::extractSquare(m);
        pos.setPiece(sq, Piece::EMPTY);
    }
    const int nPieces = pieceTypes.size();
    int bKingSq = currIdx.getSquare(nWhite);
    for (int i = 0; i < nPieces; i++) {
        int sq = currIdx.getSquare(i);
        if (i == nWhite || sq != bKingSq)
            pos.setPiece(sq, pieceTypes[i]);
    }
    pos.setWhiteMove(currIdx.whiteMove());
}

U64
TBPosition::getOccupied() const {
    const int nPieces = pieceTypes.size();
    U64 occupied = 0;
    for (int i = 0; i < nPieces; i++)
        occupied |= 1ULL << currIdx.getSquare(i);
    return occupied;
}

bool
TBPosition::canTakeKing() {
    const U64 occupied = getOccupied();
    const bool whiteMove = currIdx.whiteMove();
    const int kingSq = currIdx.getSquare(whiteMove ? nWhite : 0);
    const U64 kingMask = 1ULL << kingSq;

    const int start = whiteMove ? 0 : nWhite;
    const int stop = whiteMove ? nWhite : pieceTypes.size();
    const int bKingSq = currIdx.getSquare(nWhite);

    for (int i = start; i < stop; i++) {
        int sq = currIdx.getSquare(i);
        if (i != nWhite && sq == bKingSq)
            continue; // Ignore non-present piece
        switch (pieceTypes[i]) {
        case Piece::WKING: case Piece::BKING:
            if (BitBoard::kingAttacks[sq] & kingMask)
                return true;
            break;
        case Piece::WQUEEN: case Piece::BQUEEN:
            if (BitBoard::bishopAttacks(sq, occupied) & kingMask)
                return true;
            if (BitBoard::rookAttacks(sq, occupied) & kingMask)
                return true;
            break;
        case Piece::WROOK: case Piece::BROOK:
            if (BitBoard::rookAttacks(sq, occupied) & kingMask)
                return true;
            break;
        case Piece::WBISHOP: case Piece::BBISHOP:
            if (BitBoard::bishopAttacks(sq, occupied) & kingMask)
                return true;
            break;
        case Piece::WKNIGHT: case Piece::BKNIGHT:
            if (BitBoard::knightAttacks[sq] & kingMask)
                return true;
            break;
        }
    }
    return false;
}

void
TBPosition::getMoves(TbMoveList& lst) {
    const U64 occupied = getOccupied();
    const bool whiteMove = currIdx.whiteMove();
    const int nPieces = pieceTypes.size();

    const int start = whiteMove ? 0 : nWhite;
    const int stop = whiteMove ? nWhite : nPieces;
    const int bKingSq = currIdx.getSquare(nWhite);
    const U32 origIdx = currIdx.getIndex();

    for (int i = start; i < stop; i++) {
        const int from = currIdx.getSquare(i);
        if (i != nWhite && from == bKingSq)
            continue; // Ignore non-present piece
        U64 toMask = 0;
        switch (pieceTypes[i]) {
        case Piece::WKING: case Piece::BKING:
            toMask = BitBoard::kingAttacks[from];
            break;
        case Piece::WQUEEN: case Piece::BQUEEN:
            toMask = BitBoard::bishopAttacks(from, occupied);
            toMask |= BitBoard::rookAttacks(from, occupied);
            break;
        case Piece::WROOK: case Piece::BROOK:
            toMask = BitBoard::rookAttacks(from, occupied);
            break;
        case Piece::WBISHOP: case Piece::BBISHOP:
            toMask = BitBoard::bishopAttacks(from, occupied);
            break;
        case Piece::WKNIGHT: case Piece::BKNIGHT:
            toMask = BitBoard::knightAttacks[from];
            break;
        }
        while (toMask) {
            int toSq = BitBoard::extractSquare(toMask);
            if (toSq == bKingSq)
                continue;
            int captured = -1;
            if ((1ULL << toSq) & occupied) {
                int start2 = whiteMove ? nWhite+1 : 1;
                int stop2 = whiteMove ? nPieces : nWhite;
                for (int j = start2; j < stop2; j++) {
                    if (currIdx.getSquare(j) == toSq) {
                        captured = j;
                        break;
                    }
                }
                if (captured == -1)
                    continue; // Can not capture same color
            }

            if (captured >= 0)
                currIdx.setSquare(captured, bKingSq);
            currIdx.setSquare(i, toSq);
            currIdx.swapSide();
            currIdx.canonize(pieceTypes, duplicatedPieces);
            lst.addMove(currIdx.getIndex());
            currIdx.setIndex(origIdx);
        }
    }
    lst.sort();
}

void
TBPosition::getUnMoves(TbMoveList& lst) {
    const U64 occupied = getOccupied();
    const bool whiteMove = currIdx.whiteMove();
    const int nPieces = pieceTypes.size();

    U32 missingPieces = 0;
    const int start0 = whiteMove ? 1 : nWhite+1;
    const int stop0 = whiteMove ? nWhite : nPieces;
    const int bKingSq = currIdx.getSquare(nWhite);
    for (int i = start0; i < stop0; i++)
        if (currIdx.getSquare(i) == bKingSq)
            missingPieces |= 1ULL << i;

    const int start = whiteMove ? nWhite : 0;
    const int stop = whiteMove ? nPieces : nWhite;
    const U32 origIdx = currIdx.getIndex();

    for (int i = start; i < stop; i++) {
        int to = currIdx.getSquare(i);
        if (i != nWhite && to == bKingSq)
            continue; // Ignore non-present piece
        U64 fromMask = 0;
        switch (pieceTypes[i]) {
        case Piece::WKING: case Piece::BKING:
            fromMask = BitBoard::kingAttacks[to];
            break;
        case Piece::WQUEEN: case Piece::BQUEEN:
            fromMask = BitBoard::bishopAttacks(to, occupied);
            fromMask |= BitBoard::rookAttacks(to, occupied);
            break;
        case Piece::WROOK: case Piece::BROOK:
            fromMask = BitBoard::rookAttacks(to, occupied);
            break;
        case Piece::WBISHOP: case Piece::BBISHOP:
            fromMask = BitBoard::bishopAttacks(to, occupied);
            break;
        case Piece::WKNIGHT: case Piece::BKNIGHT:
            fromMask = BitBoard::knightAttacks[to];
            break;
        }
        fromMask &= ~occupied;
        while (fromMask) {
            int fromSq = BitBoard::extractSquare(fromMask);
            currIdx.setSquare(i, fromSq);
            currIdx.swapSide();
            currIdx.canonize(pieceTypes, duplicatedPieces);
            lst.addMove(currIdx.getIndex());
            currIdx.setIndex(origIdx);
            U64 m = missingPieces;
            while (m) {
                int j = BitBoard::extractSquare(m);
                if (i == nWhite) { // black king, must set i first
                    currIdx.setSquare(i, fromSq);
                    currIdx.setSquare(j, to);
                } else { // if white king, must set j first
                    currIdx.setSquare(j, to);
                    currIdx.setSquare(i, fromSq);
                }
                currIdx.swapSide();
                currIdx.canonize(pieceTypes, duplicatedPieces);
                lst.addMove(currIdx.getIndex());
                currIdx.setIndex(origIdx);
            }
        }
    }
    lst.sort();
}

// --------------------------------------------------------------------------------

template <typename TBStorage>
TBGenerator<TBStorage>::TBGenerator(TBStorage& storage, const PieceCount& pc)
    : pieceCount(pc), table(storage) {
    assert(pieceCount.nwq >= 0);
    assert(pieceCount.nwr >= 0);
    assert(pieceCount.nwb >= 0);
    assert(pieceCount.nwn >= 0);
    assert(pieceCount.nbq >= 0);
    assert(pieceCount.nbr >= 0);
    assert(pieceCount.nbb >= 0);
    assert(pieceCount.nbn >= 0);
    assert(pieceCount.nPieces() <= 5);

    TBPosition tbPos(pc);
    table.resize(tbPos.nPositions());
}

template <typename TBStorage>
bool
TBGenerator<TBStorage>::generate(RelaxedShared<S64>& maxTimeMillis, bool verbose) {
    double t0 = currentTime();

    TBPosition tbPos(pieceCount);
    const U32 nPos = tbPos.nPositions();
    std::vector<U8> newMated, oldMated;
    size_t bitSize = nPos / 64;
    newMated.resize(bitSize);
    oldMated.resize(bitSize);

    // Classify positions into INVALID, MATE_IN_0 and UNKNOWN
    PositionValue pv;
    for (U32 idx = 0; idx < nPos; idx++) {
        if (((idx & 0xffff) == 0) && (maxTimeMillis >= 0)) {
            double t = currentTime();
            if (t - t0 > 0.3e-3 * maxTimeMillis)
                return false;
        }
        tbPos.setIndex(idx);
        if (!tbPos.indexValid()) {
            pv.setInvalid();
        } else if (tbPos.canTakeKing()) {
            pv.setMateInN(0);
        } else {
            pv.setUnknown();
        }
        table.store(idx, pv);
    }

    // Classify positions into MATED_IN_0, DRAW (stalemate), and REMAINING_N
    for (U32 idx = 0; idx < nPos; idx++) {
        if (((idx & 0xffff) == 0) && (maxTimeMillis >= 0)) {
            double t = currentTime();
            if (t - t0 > 0.3e-3 * maxTimeMillis)
                return false;
        }
        if (!table[idx].isUnknown())
            continue;
        tbPos.setIndex(idx);
        TbMoveList moves;
        tbPos.getMoves(moves);
        int nLegal = 0;
        for (int m = 0; m < moves.getSize(); m++) {
            if (m > 0 && moves[m] == moves[m-1])
                continue; // Skip duplicated moves
            int idx2 = moves[m];
            if (!table[idx2].isMateInN(0))
                nLegal++;
        }
        if (nLegal > 0) {
            pv.setRemaining(nLegal);
        } else {
            tbPos.swapSide();
            int idx2 = tbPos.getIndex();
            if (table[idx2].isMateInN(0)) {
                pv.setMatedInN(0);
                newMated[idx>>6] = 1;
            } else {
                pv.setDraw();
            }
        }
        table.store(idx, pv);
    }

    double t1 = currentTime();

    // Find all MATE_IN_N and MATED_IN_N positions
    for (int n = 1; ; n++) {
        if (maxTimeMillis == 0)
            return false; // Cancelled by UCI stop command
        double t2 = currentTime();
        int modified = 0;
        int handled = 0;
        oldMated.swap(newMated);
        for (size_t i = 0; i < bitSize; i++) newMated[i] = 0;
        for (U32 idx = 0; idx < nPos; idx++) {
            if (((idx & 63) == 0) && !oldMated[idx>>6]) {
                idx += 63;
                continue;
            }
            if (!table[idx].isMatedInN(n-1))
                continue;
            tbPos.setIndex(idx);
            handled++;
            TbMoveList lst;
            tbPos.getUnMoves(lst);
            for (int m1 = 0; m1 < lst.getSize(); m1++) {
                if (m1 > 0 && lst[m1] == lst[m1-1])
                    continue; // Skip duplicated moves
                int idx2 = lst[m1];
                tbPos.setIndex(idx2);
                if (!table[idx2].isComputed()) {
                    modified++;
                    pv.setMateInN(n); table.store(idx2, pv);
                    TbMoveList lst2;
                    tbPos.getUnMoves(lst2);
                    for (int m2 = 0; m2 < lst2.getSize(); m2++) {
                        if (m2 > 0 && lst2[m2] == lst2[m2-1])
                            continue; // Skip duplicated moves
                        int idx3 = lst2[m2];
                        pv = table[idx3];
                        if (pv.isRemainingN()) {
                            if (pv.decRemaining()) {
                                pv.setMatedInN(n);
                                newMated[idx3>>6] = 1;
                            }
                            table.store(idx3, pv);
                        }
                    }
                }
            }
        }
        double t3 = currentTime();
        if (verbose)
            std::cout << "n: " << std::setw(2) << n << " handled: " << std::setw(8) << handled
                      << " modified: " << std::setw(8) << modified << " t: " << (t3 - t2) << std::endl;
        if (modified == 0)
            break;
    }
    if (verbose) {
        double t2 = currentTime();
        std::cout << "t:" << (t1 - t0) << " t2:" << (t2 - t1) << std::endl;
    }

    // Remaining positions are DRAW
    for (U32 idx = 0; idx < nPos; idx++) {
        if (table[idx].isRemainingN()) {
            pv.setDraw(); table.store(idx, pv);
        }
    }

    return true;
}

template <typename TBStorage>
bool
TBGenerator<TBStorage>::probeDTM(const Position& pos, int ply, int& score) const {
    TBPosition tbPos(pieceCount);
    if (!tbPos.setPosition(pos))
        return false;

    PositionValue posVal = table[tbPos.getIndex()];
    int n;
    if (posVal.getMateInN(n)) {
        score = SearchConst::MATE0 - ply - n * 2;
        return true;
    } else if (posVal.getMatedInN(n)) {
        score = -(SearchConst::MATE0 - ply - n * 2 - 1);
        return true;
    } else if (posVal.isDraw()) {
        score = 0;
        return true;
    }
    return false;
}

template class TBGenerator<VectorStorage>;
template class TBGenerator<TTStorage>;
