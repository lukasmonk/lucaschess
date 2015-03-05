/*
    Texel - A UCI chess engine.
    Copyright (C) 2012-2014  Peter Österlund, peterosterlund2@gmail.com

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
 * position.cpp
 *
 *  Created on: Feb 25, 2012
 *      Author: petero
 */

#include "position.hpp"
#include "evaluate.hpp"
#include "textio.hpp"

#include <iostream>
#include <cassert>

U64 Position::psHashKeys[Piece::nPieceTypes][64];
U64 Position::whiteHashKey;
U64 Position::castleHashKeys[16];
U64 Position::epHashKeys[9];
U64 Position::moveCntKeys[101];


static StaticInitializer<Position> posInit;

void
Position::staticInitialize() {
    int rndNo = 0;
    for (int p = 0; p < Piece::nPieceTypes; p++)
        for (int sq = 0; sq < 64; sq++)
            psHashKeys[p][sq] = getRandomHashVal(rndNo++);
    whiteHashKey = getRandomHashVal(rndNo++);
    for (size_t cm = 0; cm < COUNT_OF(castleHashKeys); cm++)
        castleHashKeys[cm] = getRandomHashVal(rndNo++);
    for (size_t f = 0; f < COUNT_OF(epHashKeys); f++)
        epHashKeys[f] = getRandomHashVal(rndNo++);
    for (size_t mc = 0; mc < COUNT_OF(moveCntKeys); mc++)
        moveCntKeys[mc] = getRandomHashVal(rndNo++);
}


Position::Position() {
    for (int i = 0; i < 64; i++)
        squares[i] = Piece::EMPTY;
    for (int i = 0; i < Piece::nPieceTypes; i++) {
        psScore1_[i] = 0;
        psScore2_[i] = 0;
        pieceTypeBB_[i] = 0;
    }
    whiteBB_ = blackBB_ = 0;
    whiteMove = true;
    castleMask = 0;
    epSquare = -1;
    halfMoveClock = 0;
    fullMoveCounter = 1;
    computeZobristHash();
    wKingSq_ = bKingSq_ = -1;
    wMtrl_ = bMtrl_ = -::kV;
    wMtrlPawns_ = bMtrlPawns_ = 0;
}

void
Position::setPiece(int square, int piece) {
    int removedPiece = squares[square];
    squares[square] = piece;

    // Update hash key
    hashKey ^= psHashKeys[removedPiece][square];
    hashKey ^= psHashKeys[piece][square];

    // Update material identifier
    matId.removePiece(removedPiece);
    matId.addPiece(piece);

    // Update bitboards
    const U64 sqMask = 1ULL << square;
    pieceTypeBB_[removedPiece] &= ~sqMask;
    pieceTypeBB_[piece] |= sqMask;

    if (removedPiece != Piece::EMPTY) {
        int pVal = ::pieceValue[removedPiece];
        if (Piece::isWhite(removedPiece)) {
            wMtrl_ -= pVal;
            whiteBB_ &= ~sqMask;
            if (removedPiece == Piece::WPAWN) {
                wMtrlPawns_ -= pVal;
                pHashKey ^= psHashKeys[Piece::WPAWN][square];
            }
        } else {
            bMtrl_ -= pVal;
            blackBB_ &= ~sqMask;
            if (removedPiece == Piece::BPAWN) {
                bMtrlPawns_ -= pVal;
                pHashKey ^= psHashKeys[Piece::BPAWN][square];
            }
        }
    }

    if (piece != Piece::EMPTY) {
        int pVal = ::pieceValue[piece];
        if (Piece::isWhite(piece)) {
            wMtrl_ += pVal;
            whiteBB_ |= sqMask;
            if (piece == Piece::WPAWN) {
                wMtrlPawns_ += pVal;
                pHashKey ^= psHashKeys[Piece::WPAWN][square];
            }
            if (piece == Piece::WKING)
                wKingSq_ = square;
        } else {
            bMtrl_ += pVal;
            blackBB_ |= sqMask;
            if (piece == Piece::BPAWN) {
                bMtrlPawns_ += pVal;
                pHashKey ^= psHashKeys[Piece::BPAWN][square];
            }
            if (piece == Piece::BKING)
                bKingSq_ = square;
        }
    }

    // Update piece/square table scores
    psScore1_[removedPiece] -= Evaluate::psTab1[removedPiece][square];
    psScore2_[removedPiece] -= Evaluate::psTab2[removedPiece][square];
    psScore1_[piece]        += Evaluate::psTab1[piece][square];
    psScore2_[piece]        += Evaluate::psTab2[piece][square];
}

U64
Position::hashAfterMove(const Move& move) const {
    int from = move.from();
    int to = move.to();
    int p = squares[from];
    int capP = squares[to];

    U64 ret = hashKey ^ whiteHashKey;
    ret ^= psHashKeys[capP][to];
    ret ^= psHashKeys[p][to];
    ret ^= psHashKeys[p][from];
    ret ^= psHashKeys[Piece::EMPTY][from];

    return ret;
}

void
Position::makeMove(const Move& move, UndoInfo& ui) {
    ui.capturedPiece = squares[move.to()];
    ui.castleMask = castleMask;
    ui.epSquare = epSquare;
    ui.halfMoveClock = halfMoveClock;
    bool wtm = whiteMove;

    hashKey ^= whiteHashKey;

    const int p = squares[move.from()];
    int capP = squares[move.to()];
    U64 fromMask = 1ULL << move.from();

    int prevEpSquare = epSquare;
    setEpSquare(-1);

    if ((capP != Piece::EMPTY) || ((pieceTypeBB(Piece::WPAWN, Piece::BPAWN) & fromMask) != 0)) {
        halfMoveClock = 0;

        // Handle en passant and epSquare
        if (p == Piece::WPAWN) {
            if (move.to() - move.from() == 2 * 8) {
                int x = getX(move.to());
                if (BitBoard::epMaskW[x] & pieceTypeBB(Piece::BPAWN))
                    setEpSquare(move.from() + 8);
            } else if (move.to() == prevEpSquare) {
                setPiece(move.to() - 8, Piece::EMPTY);
            }
        } else if (p == Piece::BPAWN) {
            if (move.to() - move.from() == -2 * 8) {
                int x = getX(move.to());
                if (BitBoard::epMaskB[x] & pieceTypeBB(Piece::WPAWN))
                    setEpSquare(move.from() - 8);
            } else if (move.to() == prevEpSquare) {
                setPiece(move.to() + 8, Piece::EMPTY);
            }
        }

        if ((pieceTypeBB(Piece::WKING, Piece::BKING) & fromMask) != 0) {
            if (wtm) {
                setCastleMask(castleMask & ~(1 << A1_CASTLE));
                setCastleMask(castleMask & ~(1 << H1_CASTLE));
            } else {
                setCastleMask(castleMask & ~(1 << A8_CASTLE));
                setCastleMask(castleMask & ~(1 << H8_CASTLE));
            }
        }

        // Perform move
        setPiece(move.from(), Piece::EMPTY);
        // Handle promotion
        if (move.promoteTo() != Piece::EMPTY) {
            setPiece(move.to(), move.promoteTo());
        } else {
            setPiece(move.to(), p);
        }
    } else {
        halfMoveClock++;

        // Handle castling
        if ((pieceTypeBB(Piece::WKING, Piece::BKING) & fromMask) != 0) {
            int k0 = move.from();
            if (move.to() == k0 + 2) { // O-O
                movePieceNotPawn(k0 + 3, k0 + 1);
            } else if (move.to() == k0 - 2) { // O-O-O
                movePieceNotPawn(k0 - 4, k0 - 1);
            }
            if (wtm) {
                setCastleMask(castleMask & ~(1 << A1_CASTLE));
                setCastleMask(castleMask & ~(1 << H1_CASTLE));
            } else {
                setCastleMask(castleMask & ~(1 << A8_CASTLE));
                setCastleMask(castleMask & ~(1 << H8_CASTLE));
            }
        }

        // Perform move
        movePieceNotPawn(move.from(), move.to());
    }
    if (wtm) {
        // Update castling rights when rook moves
        if ((BitBoard::maskCorners & fromMask) != 0) {
            if (p == Piece::WROOK)
                removeCastleRights(move.from());
        }
        if ((BitBoard::maskCorners & (1ULL << move.to())) != 0) {
            if (capP == Piece::BROOK)
                removeCastleRights(move.to());
        }
    } else {
        fullMoveCounter++;
        // Update castling rights when rook moves
        if ((BitBoard::maskCorners & fromMask) != 0) {
            if (p == Piece::BROOK)
                removeCastleRights(move.from());
        }
        if ((BitBoard::maskCorners & (1ULL << move.to())) != 0) {
            if (capP == Piece::WROOK)
                removeCastleRights(move.to());
        }
    }

    whiteMove = !wtm;
}

void
Position::makeMoveB(const Move& move, UndoInfo& ui) {
    ui.capturedPiece = squares[move.to()];
    ui.castleMask = castleMask;

    const int p = squares[move.from()];
    int capP = squares[move.to()];
    U64 fromMask = 1ULL << move.from();

    int prevEpSquare = epSquare;

    if ((capP != Piece::EMPTY) || ((pieceTypeBB(Piece::WPAWN, Piece::BPAWN) & fromMask) != 0)) {
        // Handle en passant
        if (p == Piece::WPAWN) {
            if (move.to() == prevEpSquare)
                setPieceB(move.to() - 8, Piece::EMPTY);
        } else if (p == Piece::BPAWN) {
            if (move.to() == prevEpSquare)
                setPieceB(move.to() + 8, Piece::EMPTY);
        }

        // Perform move
        setPieceB(move.from(), Piece::EMPTY);
        // Handle promotion
        if (move.promoteTo() != Piece::EMPTY) {
            setPieceB(move.to(), move.promoteTo());
        } else {
            setPieceB(move.to(), p);
        }
    } else {
        // Handle castling
        if ((pieceTypeBB(Piece::WKING, Piece::BKING) & fromMask) != 0) {
            int k0 = move.from();
            if (move.to() == k0 + 2) { // O-O
                movePieceNotPawnB(k0 + 3, k0 + 1);
            } else if (move.to() == k0 - 2) { // O-O-O
                movePieceNotPawnB(k0 - 4, k0 - 1);
            }
        }

        // Perform move
        movePieceNotPawnB(move.from(), move.to());
    }
}

void
Position::movePieceNotPawn(int from, int to) {
    const int piece = squares[from];
    hashKey ^= psHashKeys[piece][from];
    hashKey ^= psHashKeys[piece][to];
    hashKey ^= psHashKeys[Piece::EMPTY][from];
    hashKey ^= psHashKeys[Piece::EMPTY][to];

    squares[from] = Piece::EMPTY;
    squares[to] = piece;

    const U64 sqMaskF = 1ULL << from;
    const U64 sqMaskT = 1ULL << to;
    pieceTypeBB_[piece] &= ~sqMaskF;
    pieceTypeBB_[piece] |= sqMaskT;
    if (Piece::isWhite(piece)) {
        whiteBB_ &= ~sqMaskF;
        whiteBB_ |= sqMaskT;
        if (piece == Piece::WKING)
            wKingSq_ = to;
    } else {
        blackBB_ &= ~sqMaskF;
        blackBB_ |= sqMaskT;
        if (piece == Piece::BKING)
            bKingSq_ = to;
    }

    psScore1_[piece] += Evaluate::psTab1[piece][to] - Evaluate::psTab1[piece][from];
    psScore2_[piece] += Evaluate::psTab2[piece][to] - Evaluate::psTab2[piece][from];
}

// ----------------------------------------------------------------------------

void
Position::serialize(SerializeData& data) const {
    for (int i = 0; i < 4; i++) {
        int sq0 = i * 16;
        U64 v = 0;
        for (int sq = 0; sq < 16; sq++)
            v = (v << 4) | squares[sq0 + sq];
        data.v[i] = v;
    }
    U64 flags = whiteMove;
    flags = (flags << 4) | castleMask;
    flags = (flags << 8) | (epSquare & 0xff);
    flags = (flags << 8) | (halfMoveClock & 0xff);
    flags = (flags << 16) | (fullMoveCounter & 0xffff);
    data.v[4] = flags;
}

void
Position::deSerialize(const SerializeData& data) {
    for (int i = 0; i < Piece::nPieceTypes; i++) {
        psScore1_[i] = 0;
        psScore2_[i] = 0;
        pieceTypeBB_[i] = 0;
    }
    whiteBB_ = blackBB_ = 0;
    wMtrl_ = bMtrl_ = -::kV;
    wMtrlPawns_ = bMtrlPawns_ = 0;
    for (int i = 0; i < 4; i++) {
        int sq0 = i * 16;
        U64 v = data.v[i];
        for (int sq = 15; sq >= 0; sq--) {
            int piece = v & 0xf;
            v >>= 4;
            int square = sq0 + sq;
            squares[square] = piece;

            const U64 sqMask = 1ULL << square;
            pieceTypeBB_[piece] |= sqMask;
            if (piece != Piece::EMPTY) {
                int pVal = ::pieceValue[piece];
                if (Piece::isWhite(piece)) {
                    wMtrl_ += pVal;
                    whiteBB_ |= sqMask;
                    if (piece == Piece::WPAWN)
                        wMtrlPawns_ += pVal;
                    if (piece == Piece::WKING)
                        wKingSq_ = square;
                } else {
                    bMtrl_ += pVal;
                    blackBB_ |= sqMask;
                    if (piece == Piece::BPAWN)
                        bMtrlPawns_ += pVal;
                    if (piece == Piece::BKING)
                        bKingSq_ = square;
                }
            }
            psScore1_[piece] += Evaluate::psTab1[piece][square];
            psScore2_[piece] += Evaluate::psTab2[piece][square];
        }
    }

    U64 flags = data.v[4];
    fullMoveCounter = flags & 0xffff;
    flags >>= 16;
    halfMoveClock = flags & 0xff;
    flags >>= 8;
    int ep = flags & 0xff; if (ep == 0xff) ep = -1;
    epSquare = ep;
    flags >>= 8;
    castleMask = flags & 0xf;
    flags >>= 4;
    whiteMove = (flags & 1) != 0;

    computeZobristHash();
}

// ----------------------------------------------------------------------------

std::ostream&
operator<<(std::ostream& os, const Position& pos) {
    std::stringstream ss;
    ss << std::hex << pos.zobristHash();
    os << TextIO::asciiBoard(pos) << (pos.isWhiteMove() ? "white\n" : "black\n") << ss.str();
    return os;
}

U64
Position::computeZobristHash() {
    U64 hash = 0;
    pHashKey = 0;
    matId = {};
    for (int sq = 0; sq < 64; sq++) {
        int p = squares[sq];
        matId.addPiece(p);
        hash ^= psHashKeys[p][sq];
        if ((p == Piece::WPAWN) || (p == Piece::BPAWN))
            pHashKey ^= psHashKeys[p][sq];
    }
    if (whiteMove)
        hash ^= whiteHashKey;
    hash ^= castleHashKeys[castleMask];
    hash ^= epHashKeys[(epSquare >= 0) ? getX(epSquare) + 1 : 0];
    hashKey = hash;
    return hash;
}

const U64
Position::zobristRndKeys[] = {
    0xd7f5bae778359690ULL, 0x597a80e80456583cULL, 0xf41fc6250b89af0aULL, 0x60bbccb1c393eb8eULL,
    0x609a7b63b265bad6ULL, 0x7c2203b2d50e50ddULL, 0x5cdb65b020e76780ULL, 0x3ada5f98c76f082fULL,
    0xb8664a3b4835aba4ULL, 0xfd52e04af3634f9dULL, 0x19845bb0dc72bfebULL, 0x323f11923ad0ce94ULL,
    0x840b643592625af3ULL, 0x08d0487ddf83e6b1ULL, 0xe102c4c56f3cbcf1ULL, 0x8796d332562f05aeULL,
    0x8da8c0e4914f9754ULL, 0x8d33ff34f6348b49ULL, 0x9d19aea59af28996ULL, 0x9eb6168a4359a5feULL,
    0x31663b93847b0fc7ULL, 0x90eba80ddb28d49aULL, 0xccee52c57542c041ULL, 0x0860882a3e8279cfULL,
    0xde316bf1225afa4aULL, 0xc7b1c13ea1ba90c6ULL, 0x86dee889c37e993aULL, 0x84a546382686f7f9ULL,
    0xf5279c7d250960a6ULL, 0x74c682f9a715318dULL, 0xe7df0efd43f4030aULL, 0xdc6746e82902823cULL,
    0x166d42c650423d16ULL, 0xf0ec64304812b32dULL, 0x1332e60d6eec5b48ULL, 0xa6edf8bd55663265ULL,
    0x9be3edd5fba6cfd3ULL, 0xea3c98aad8c558f1ULL, 0x577d05d0fcff765dULL, 0xf4dffef665484f9aULL,
    0x80522262e9120108ULL, 0x47e9fa92bc64f2eeULL, 0x689ed873d813ba14ULL, 0xdbe14e4bad57221aULL,
    0x27dde4dac822503bULL, 0xeb54f328a3ec06bbULL, 0x385875cea7e74a41ULL, 0x9939e2b74efb03ecULL,
    0xe9dab3b1f89ac50eULL, 0x542e57cd136b4844ULL, 0xfd0d99e83d6b34b8ULL, 0x2ab51be0b039bdd7ULL,
    0xa848485bf044dac6ULL, 0xfb8b9ccc055dda28ULL, 0x6bd5fcf11932c643ULL, 0x5562cc1783eafc90ULL,
    0x84c2258075a4fc77ULL, 0xa3e2654a1b9ac399ULL, 0xa0d0b4498debbc94ULL, 0x7b5a612c9e7f4013ULL,
    0x541ad1f5345860e9ULL, 0xe994fc3ad27d8421ULL, 0x7d004d11c8d81c45ULL, 0xdf060a0b1ba0a5d0ULL,
    0xd7554261378a32ebULL, 0x3833239c3fe92c7dULL, 0xdf6f7a5e977113d5ULL, 0xe04fc2e2017aa5cdULL,
    0x7670776c650e1507ULL, 0x2b7cbce40c6231ddULL, 0xda383fc3dd431843ULL, 0xe5b36397cf1ec05dULL,
    0xdc984f60c622c2a9ULL, 0x51ef8d0af8fe2607ULL, 0x1511f2e39e554dffULL, 0x5f2c921d80f3be07ULL,
    0x2015c1d1cc6b9e92ULL, 0x99f7e5a26196f13bULL, 0x7a93cc38310fb397ULL, 0xd11d2eba53c47047ULL,
    0x070f46cdfa04dd80ULL, 0x0ebb465f6fea9c74ULL, 0xfb61452fb27308d9ULL, 0xa952f5d52c3eb97dULL,
    0x95799379a4a515abULL, 0x06508b45a2745539ULL, 0xaf69d20bf29de152ULL, 0xb0ad19b1f4ee4eb8ULL,
    0x2bd91aa7c8200e4dULL, 0xcecee5089679a9c2ULL, 0xc189667953231141ULL, 0x24f90ceadfd2ece5ULL,
    0x3518487a931724d2ULL, 0x5244fe5f9e40057cULL, 0x366ce132f59078ffULL, 0xe63f79535cf61cdeULL,
    0xefdfab636b84b733ULL, 0xe38c18fdb5bf9f22ULL, 0xa4064c6acad42a58ULL, 0xcce22e73ebafa83aULL,
    0xa40b98bb7736c6e0ULL, 0xb75dd212bc092f4dULL, 0x41cb16cdede1750eULL, 0xc079f0fc98bb6a11ULL,
    0xb6aa18d94d9b37baULL, 0x1647aa4c407d1244ULL, 0x8c705a13349e10c3ULL, 0xce30a5c3e1dabee9ULL,
    0x2d6ca598c617b183ULL, 0xca21c9cddd1af110ULL, 0xb5e68f6381dacedaULL, 0xd9c61ec96684461fULL,
    0x0b1fa1bcae112ee1ULL, 0x154042c24e0404c6ULL, 0xb4efbfbf8ea0e4f8ULL, 0xfd436a8b275b1c15ULL,
    0x47dacd95f41bd571ULL, 0xf7714e0b7756278aULL, 0x559e24fc8d287691ULL, 0x91c041e4c56036f7ULL,
    0x1f6578f865919a79ULL, 0xc8cdcdd242fab4d9ULL, 0x28302a7558bebb6bULL, 0xa6fbdd97c0895919ULL,
    0xcbadb431be485173ULL, 0x703991cf429a8bbdULL, 0x9ec19ced4781bba6ULL, 0x94af792408408df7ULL,
    0xabed4ed1677c86e1ULL, 0x0f6e9cbb41089de3ULL, 0x3c88eacbb947dc3bULL, 0x99b3fbd4c65fe1cfULL,
    0x034cd4892067845cULL, 0xbcabc47135a8c209ULL, 0x8d553cd1b31bed9dULL, 0x0d8a566720267700ULL,
    0x3bed637a9a370afdULL, 0x41494335f1d0adfbULL, 0x9f12bfff16b28edeULL, 0xce6453efe358988dULL,
    0x2215954804510ab6ULL, 0x6b4394e84989f5f2ULL, 0xca8c91dd50847010ULL, 0xa947c7e59c35417aULL,
    0x8f95c01ab298ec1eULL, 0xe95721786359e315ULL, 0xb4a2d874cec85136ULL, 0x7f1f7f8bce470897ULL,
    0x7437e83c9fd5e424ULL, 0x6eeb30c64a162b8cULL, 0x23c1516503b4f507ULL, 0xcb321488c3757420ULL,
    0xcebed99b362fd959ULL, 0x6c127ce789a52e6dULL, 0x980b79769c8813e0ULL, 0x26c25f71471c7e54ULL,
    0x53934724e51cc126ULL, 0xd8680dabd6b9d852ULL, 0xd1bacb3c3b6f4daeULL, 0x2597310144a51aadULL,
    0xdb11576656984ffcULL, 0xab667b46feae063cULL, 0x503558ac4b13be6fULL, 0x3a3e7d2c081594d7ULL,
    0x4157037c0c54ea64ULL, 0xf6126e71ad203300ULL, 0xc3f04d0b97cad0fdULL, 0xbff96410f693d8b3ULL,
    0xac6ae032fbaab91eULL, 0xc00eeb6a400bb962ULL, 0x2477bdbbcf3a2976ULL, 0x094c7b15ce23247cULL,
    0x01e71a4167f69b90ULL, 0x6e29b4138facff1fULL, 0x565686f589cc4d65ULL, 0x73f81ca8ebee9fcbULL,
    0x05a346d75398ffb3ULL, 0x4a69fb255d62ed90ULL, 0xccced0c1106d1c93ULL, 0x625916cfa21515e0ULL,
    0x6f8ee1626c380dc1ULL, 0xf2e16898283cd0c3ULL, 0x727321be23832a2bULL, 0x43441f936eeb2d87ULL,
    0x14b45b1ecb3f7714ULL, 0x7ddfc1f328929f1aULL, 0x2d55a0468e13ec26ULL, 0x21dd2c1f3057970fULL,
    0xa6e1a5ce99fdd971ULL, 0x701c90f12059e247ULL, 0x1b7ae58e5c64ff4cULL, 0x05de117111a62053ULL,
    0x137b6510a83e5ca8ULL, 0x07fc44272c5becf7ULL, 0x0f16dafab947f490ULL, 0x0895b0033e5e8900ULL,
    0xeed683f7b95fd966ULL, 0x46506e5194dc9ccbULL, 0x408031d8bc251a31ULL, 0x26f999eeac14cfd6ULL,
    0xc720d88aaede1459ULL, 0x1339ca054c720e1dULL, 0x70111e113ba72600ULL, 0xdb22a28a6dc25133ULL,
    0x72e937c0bff13900ULL, 0x107808448080fbfaULL, 0x34f3338c200fdb4fULL, 0x4a0eadbe656dbc83ULL,
    0x84dd6549f109d3bfULL, 0x746c4aecbae19826ULL, 0x513838a22e0cfe1cULL, 0x11b8f4b8fa368a3fULL,
    0xdf6d5ae667795071ULL, 0xcf130962ae8c6607ULL, 0x82858c9a6b5d69e9ULL, 0x1da5a0a49b579480ULL,
    0xd84db8c20e132aa6ULL, 0x0c3b0392ff2e1e63ULL, 0x912d3070ad153aa2ULL, 0x6d8ff71426c0d062ULL,
    0xdbed3534b24e6bd3ULL, 0xa876821775929182ULL, 0xcdaa7d29acfa4a83ULL, 0x10333764a610f69fULL,
    0x63e24d99bc37a324ULL, 0x8c25823bad7029f5ULL, 0x2ef28a1deece6115ULL, 0x32b90c23208a0008ULL,
    0x723589f592ed6591ULL, 0x00c7516ed0d83959ULL, 0xe0fc684a298682e4ULL, 0x36fa668e47c35a84ULL,
    0xe9b3118ed5043e35ULL, 0x75d8783d5a29f16aULL, 0x7c3c86fa947f09aeULL, 0x56f2a5e6daf19aaeULL,
    0x00900521d0b38f6cULL, 0x26fa3cdcdd8cabc0ULL, 0x4a83b4473548d939ULL, 0x784f302daf6e022aULL,
    0xe46c4deeeb312976ULL, 0xa9b98bd7ba7011adULL, 0x64c14cfea7abc8f5ULL, 0xc1497eb0406690faULL,
    0x9511d4de569276efULL, 0xabf024a50a90b64aULL, 0x49b19b07c3e22da3ULL, 0x706bd69e20520907ULL,
    0x894b149339d83ad9ULL, 0xbcf1438efca42924ULL, 0xdb560b04bef14919ULL, 0x9b7482cbecc444d6ULL,
    0xed2bb01a83016742ULL, 0x71f684628505a4e1ULL, 0x670bcdc8f1042d61ULL, 0xa8f134df95b5d080ULL,
    0xffb794a24165093fULL, 0x50d032fa1d662fb4ULL, 0x2810a6f28ffbb86fULL, 0xe67681ca3f1c85c7ULL,
    0x03cb85ef8d956f64ULL, 0x1738ad2440ba2ca9ULL, 0xd0407f7f7463398fULL, 0x160b8d8af7dc2f9bULL,
    0x0405a3e372380b98ULL, 0x6c7e379ee9d8b82dULL, 0xdc4fe04fd1875ffeULL, 0x7d644123159cfb27ULL,
    0x49606435071bb986ULL, 0x90996d41b01eb18dULL, 0x5183897c42d1975eULL, 0xf56aeaa393372306ULL,
    0x3e61ec92c30aaacfULL, 0x325b84d16be4df60ULL, 0x1e5bca8358d2da94ULL, 0x6b927e82bfa69929ULL,
    0x085ad4d97f3dc0cfULL, 0x8cae40fec0c216aaULL, 0x764c17024ef6c979ULL, 0xc96b7917c5ab12b3ULL,
    0xb1f58a09d022290fULL, 0x4549907a6ebf95f1ULL, 0x1d00457d45793c60ULL, 0x0d4301ba9f5832cbULL,
    0xa2836b7853af8349ULL, 0xba9f1018ecdd2d0fULL, 0xf3f596aada6e690dULL, 0xc6649514b3e81409ULL,
    0xb6d8a6c7ebfe4afcULL, 0x115175cfa1169485ULL, 0xe4a59f8ae8f01b9bULL, 0x05415ed13bb46446ULL,
    0x8902f6711e2897a9ULL, 0x1ea71ce7028495e6ULL, 0x34fb7b46530d0bc3ULL, 0xb244f37d544d11b4ULL,
    0x4b7706d4e0d4a2c8ULL, 0x28e623ef474904f5ULL, 0x961ebd4357a2a031ULL, 0xca8ddd9ea423647fULL,
    0x3c3aea6f91215fbaULL, 0x2c89ecf10fe90db9ULL, 0x823e33b306ce8675ULL, 0x66dbe9c27aa0cbc8ULL,
    0xa5393f1e6a80d339ULL, 0x68b115fc5b13af32ULL, 0x4a5a983409430b7cULL, 0x30b18f3cfdd3c98eULL,
    0x929ddbbe3a82803eULL, 0x64ccccd7140f4a9bULL, 0xd7f803762450fec5ULL, 0x832e6ceca2780550ULL,
    0x7c6840451f7a0143ULL, 0x02655066b6f784e9ULL, 0xee1659bae59fb670ULL, 0x4542a44ed14050fdULL,
    0xe4f07584584e1e14ULL, 0x83dc19959c56dee9ULL, 0x8aeb3988a882c1c5ULL, 0xe19b46fed3c4f2bfULL,
    0xce1258daee0a0946ULL, 0x247c26799366976aULL, 0x10acf6eca81488a3ULL, 0xa7ad75899d582d56ULL,
    0xe315be1a51f538ffULL, 0xb5be8993675b7872ULL, 0x3db8d2d7608bbde8ULL, 0x9ff1c2c3ce3ceac1ULL,
    0xeeecc280de870e1bULL, 0x5d77564297d99e2aULL, 0x4c3f25277aa1a701ULL, 0xff06a70466279913ULL,
    0xc026377450f01f65ULL, 0x1b2494641bf91608ULL, 0x5f1b74fb08c332b9ULL, 0xea2c64e3db9f9057ULL,
    0x9d4f807cbff85025ULL, 0xcee8670f094702b7ULL, 0xf340eef21642e7c3ULL, 0x19606d55ebe426beULL,
    0xed97dc58e1711875ULL, 0xa417f28f24dea798ULL, 0x11cd6acb68c72546ULL, 0xf1921aac289d9dd6ULL,
    0x6bd0fcfc448bc595ULL, 0x3e1f240d8fdc4618ULL, 0xa3828d7a1ff27fc8ULL, 0x46a4b0335abbf426ULL,
    0xdf7115c761657e18ULL, 0x3d28f804775cffc4ULL, 0x30342adb438b9c69ULL, 0xb9937f6ce61ea1cdULL,
    0xd4b5567c46873e35ULL, 0x940200555367edb0ULL, 0xb9d944f5f979e14fULL, 0x3b8bd9a0316d97b1ULL,
    0x96798a6bc116783eULL, 0x39acfcb474f282e4ULL, 0xdb94a39f58a94510ULL, 0xaa7980b4175d122fULL,
    0xc9efca1c00e61953ULL, 0x33de221d4b8295e7ULL, 0xf2c235e73e3b2542ULL, 0x83215ca5453dcb65ULL,
    0xf58ff8a8df56fb2dULL, 0x09559195c7cb434eULL, 0x1161fe1c7f8f7187ULL, 0x148818fc6eaca639ULL,
    0xd5298696e9d8cc30ULL, 0xfa6b53e9a9d6a4c7ULL, 0xb7cfd2c1d2ed429eULL, 0x33df557fb7aa966fULL,
    0xe556777cddbba676ULL, 0x05c1c1887877a56eULL, 0x2e89de6e661bd34bULL, 0xa17a85d2b115f694ULL,
    0xa807afd9ce6c2628ULL, 0xa555ddd592ebdf55ULL, 0xf1b79535a4a444e6ULL, 0x6778915e7b1823c8ULL,
    0xd0bceed9f840f9c1ULL, 0x5760eb1fe0d2758bULL, 0x342d44714e3c65b9ULL, 0xed7a362bcbe6feb6ULL,
    0x1705103c42821aedULL, 0xc501924ccd211bf5ULL, 0xcfe2ea8d5d1b6856ULL, 0x24ae2eb2f6bfc818ULL,
    0x7fb97dbbee485d64ULL, 0xd64b84c4aeeb08d0ULL, 0xe2fd0cc4a47204a0ULL, 0xaf55b1facc3987a1ULL,
    0x1dfd4be371d6f91aULL, 0x20fda8869c9263eeULL, 0x999bd21d8576298aULL, 0x1eb57256cbe60d94ULL,
    0x4dceb0738230ace0ULL, 0x2dcc0846f5f39d84ULL, 0xbf22300cf9173002ULL, 0x5a1aa9b8743c632cULL,
    0x9070299e5bbf03ebULL, 0xbfeb907c9407f326ULL, 0x190423598b0cbaf6ULL, 0x309e4bf71e4c443bULL,
    0x79ec14272ad6eeccULL, 0xc80fc37f51135e15ULL, 0xe648d34336875121ULL, 0x773e4ab9e8b0acc0ULL,
    0xfb44a070ffd3a52aULL, 0x10696674757da91bULL, 0x41a63ec5b12cede6ULL, 0x4028279f3697edc4ULL,
    0xf968fbc2fabe7cc6ULL, 0x8d7f9f58b58c124cULL, 0xd8ea949dbab5a05dULL, 0x54bd7bbd38c9b3afULL,
    0x474005f184ab67ddULL, 0x3d5607cf170fb906ULL, 0x7d5c3e449ecf9a7bULL, 0x64252034d8f92f53ULL,
    0xbc687bb81f55f180ULL, 0x192de7c9da91f403ULL, 0xbeb7d9e0019a8b18ULL, 0x7db48efae4df737cULL,
    0x175ec9517c5c4c0eULL, 0x717a7d0ebda47f15ULL, 0xa587e3e6f2b8e954ULL, 0xab2cd98817f315e2ULL,
    0xcc0e745d179b1998ULL, 0x6649139d5d71f1d0ULL, 0xfcc5b6186974a8f5ULL, 0xa7059f06a374e713ULL,
    0x213cef524ef7b75fULL, 0x281fda3316b40c85ULL, 0xeebd55e267534c75ULL, 0xcab02b62892df3b9ULL,
    0x3b808869d5a722a5ULL, 0x4d97222b49abd9dfULL, 0x4ddc171d34629f30ULL, 0xd3752bcdc5ef181dULL,
    0xab4d39c6aa8a5ca7ULL, 0x6ba8bd4d5cb06082ULL, 0x583d4de08802857dULL, 0x026c5618433aa7edULL,
    0x1232c81cafbfd167ULL, 0x5a33e75840166131ULL, 0x3ea02e990724ef4dULL, 0x3951fad4b82928bcULL,
    0xf1e92e7fb0846900ULL, 0x15fed7db76b1e8feULL, 0x5052710ed579d80eULL, 0xd783125e16b47e73ULL,
    0x98d8b7da8bdc9c2bULL, 0x98ce50243f35581eULL, 0xb141d678f827a0d4ULL, 0xb0f8e3094f49fa61ULL,
    0xab19151f7a7cdb52ULL, 0x6ba2c537f521a4f1ULL, 0x1ade518a3d9c178dULL, 0x8500515c23f51115ULL,
    0xf3cc5c2fefd7a98dULL, 0xd8bbbb3e9ae20b79ULL, 0xf871825bb7ac5147ULL, 0xbb781a17cca93d0aULL,
    0xf3575106b160c93cULL, 0xf844c703f58f2e1fULL, 0x2a5c72d4367ca657ULL, 0x99d7c01cf853ec21ULL,
    0x1a6f335ad74dfff5ULL, 0x4fbdc8c120b994f2ULL, 0x408461eb155418cdULL, 0x157916359ffe2c1eULL,
    0x525f1a7db4223dbfULL, 0x25c3000b1e90fb19ULL, 0x57bdd0c1d663fa2dULL, 0xfb802e7119bfbe24ULL,
    0xaae1c35ec6eb14c4ULL, 0x652583024fcd306dULL, 0xe8be1108b58af685ULL, 0x77a89f20ae1f8a94ULL,
    0xbc22aa4ab340e4bfULL, 0x78bfc970c96c221cULL, 0x1d3f7809a295ee76ULL, 0xda430da50a13a883ULL,
    0xc1306bf82a441264ULL, 0xe31afb1e6fd6451cULL, 0x3a2d9759aa34eeb3ULL, 0xbf5ceaf7ab48104dULL,
    0x3400ed26b9495770ULL, 0x3b296387eb154befULL, 0x1c93c216ecba4af5ULL, 0x5a34070cf1e792aaULL,
    0x42c9d7c5a3609633ULL, 0x4379936a9a13d21fULL, 0x84e2a6c2f892c5d7ULL, 0xb23b8143286b71edULL,
    0x7d2a12b06074cab3ULL, 0x8540d38e599343fbULL, 0x3b3b8f9db8e41f17ULL, 0x161e4e09e20ab0c6ULL,
    0x162298b0f8723d99ULL, 0x9afbdb3f8b032893ULL, 0x5ec3f04936e0883aULL, 0xc50c2bce3a56ef64ULL,
    0x4460c752721c55d4ULL, 0xf015b1f777a6afdcULL, 0x06f1e56f14c48c36ULL, 0x33e7235f85296282ULL,
    0x06040eb1efdd11b9ULL, 0x346b7f7b751f66f3ULL, 0xbb4fa9a8e6d1d035ULL, 0xe1d3d700a82e48d5ULL,
    0x8135cfd26c74b440ULL, 0x65d5d35b5696f10cULL, 0xbf9838f966bb07e8ULL, 0xebd2eb4bbcb43720ULL,
    0xbbfeec86250e9350ULL, 0x66aba8170e00c52fULL, 0xabccc4a86f130675ULL, 0x2ca5716416ed7185ULL,
    0x010b20c0e57bc195ULL, 0x2fe60878da02ad71ULL, 0x1b0fef55ed00184eULL, 0x1261c170eeb052b6ULL,
    0x6a13113537136749ULL, 0x61c36852fc8ed2d0ULL, 0x3792d791924af876ULL, 0x5ec74fb3066e0ab6ULL,
    0x08e5788888816a21ULL, 0x238b94c0003b284dULL, 0x7d712871cee92534ULL, 0x4848d5aa004ba38aULL,
    0x1205c02c9b1ce8e5ULL, 0xba3aac58d085586eULL, 0xb0fa368bfeb857abULL, 0x29046208a0f9a5ecULL,
    0x7e9cd0525a6a8cefULL, 0x205f2c43beab7fc5ULL, 0xd6a2d6213b8fd0f3ULL, 0x0897451e05d3e003ULL,
    0x37df4af89203d875ULL, 0x103862149234cca6ULL, 0x19953ac4237d4439ULL, 0x4a089784e1bad7d8ULL,
    0xfefa933f9266f384ULL, 0xd7153f3f27c9855cULL, 0x82dfe8503c066585ULL, 0xd1b163cce1e1794cULL,
    0x50a0c5f3b4e0185aULL, 0xc1aa0810e07a4a11ULL, 0xf923c43f65456424ULL, 0x725076b178a0cec6ULL,
    0x6a8ffed38d48e8c1ULL, 0x98c7d98b58632ce9ULL, 0x10d9eaa19eb6646dULL, 0x09d53df4e86ed7c1ULL,
    0x351546a5962fd169ULL, 0x9a9e3300af4c96b3ULL, 0x2c0efad71fe68509ULL, 0x55465603290e4df8ULL,
    0x3c6a5b9948795195ULL, 0xdec4cbaf18698cb6ULL, 0x22a7fc751a85732fULL, 0x237aca9e62b6ee42ULL,
    0x3ec48c5a1a0eeadaULL, 0xdca8d6c1752a75b0ULL, 0x551d35ee59e8df71ULL, 0x6abcfa8e5bbce350ULL,
    0xe572c1e3e84c1878ULL, 0x735166f77889b868ULL, 0x4fa1a3b5f083865fULL, 0x8e681cc7db9e5cc1ULL,
    0xbdd3e56d0c286e7dULL, 0xb0a86df0d313440eULL, 0x1cf34137a8f0dd3dULL, 0x489c25283ca067b7ULL,
    0x0c1ca8135db56e13ULL, 0xff7e6858fcd20f2fULL, 0x2a920c1a2fb1c744ULL, 0x49ade1415c0f4579ULL,
    0xc87e5ccefb5965cfULL, 0x210c7df122a3a8d1ULL, 0x01ba3a70643d903fULL, 0xe07ccb713c01a278ULL,
    0x62c4a7883e9408b6ULL, 0x5ee17e7a19fe78dcULL, 0x0ae510b98403b793ULL, 0x21557b4ba5db3c1dULL,
    0xdb9583eb7765f872ULL, 0x1d81a84105cb6a78ULL, 0x8083f770c8a9ba2eULL, 0xc26d49a4d229f025ULL,
    0x71296a23163a7939ULL, 0x3c5c70a50a4f78afULL, 0x0764005abc6939e0ULL, 0x8789dc959a95e3a7ULL,
    0x3d33823ef2e4667dULL, 0xac0d4840351a939aULL, 0x3a1e288f96e7befcULL, 0x0b0acf1eeb03e0e6ULL,
    0x5af734f93a1f45abULL, 0x26a757c07ebb8db1ULL, 0x213234b390a5ea73ULL, 0xde773d9c51af6f74ULL,
    0xd49f3afe03115a77ULL, 0x5f633306505f406bULL, 0x02e449a05a0562c0ULL, 0x314666b3bc46b3dfULL,
    0xece9fb5e19a2aa37ULL, 0xe6c59b7170283f22ULL, 0xb46eefb8b8a61f82ULL, 0x096a87d165af2088ULL,
    0x39c222e996b6f76dULL, 0x6a36fa7cea8230f5ULL, 0xae8e037994cad44fULL, 0xbab83290685556c1ULL,
    0x4647f6a1aee27fedULL, 0xf7555e9a540b8d18ULL, 0x40b059b1ceb14081ULL, 0x079823a46fccb19bULL,
    0x52149b6c54c621a9ULL, 0x179d81ca92e38367ULL, 0xa4b7a879aeec1968ULL, 0x46bafea8e15407b4ULL,
    0x7b82184063a31cb8ULL, 0xc2a7d35965bb02d9ULL, 0x20a1b2b01c93ecc3ULL, 0x9f20f598305af5eaULL,
    0x0be76e68eabbfb22ULL, 0xe45c23075b6a5c1eULL, 0xa2804a249b3cf0c0ULL, 0x48afb9bef6c4e536ULL,
    0xce14af50e72e33d6ULL, 0xb3b78d21e4c01949ULL, 0xda9c1933e182091eULL, 0x848482ce26352dcbULL,
    0x800ff9ef2e1b00ddULL, 0x878c5d32481c244bULL, 0x1c6e9aa00137637cULL, 0xf872c37c83005a46ULL,
    0x7fa48f34e9f56c13ULL, 0x2ace3766cddcc711ULL, 0xc17b1e2d4baf85abULL, 0x76ceade0c91a543aULL,
    0x9056b0651be35aabULL, 0xd1782c9c5d85ef5eULL, 0x10200abecac6936dULL, 0xdc32c02478a5d493ULL,
    0x312a8b39d2e30863ULL, 0xe90c092fb5e7fedeULL, 0x05a0a4173c2a42faULL, 0xa306c98b558ec17fULL,
    0xa5c0b6af2641fe9dULL, 0xc0988090c5d6c130ULL, 0x83cf195ea8474febULL, 0x11d4ad1165868aa7ULL,
    0xb7a6fc6bfe40f3f9ULL, 0x344debcb5d483760ULL, 0xb01627e1b014c80cULL, 0xee324b6baa3b32eaULL,
    0xcdc83a3c452b5cb3ULL, 0xf491453b385a69abULL, 0x9c2c3555a589cd4cULL, 0x5aa3bc66d8cd5cf7ULL,
    0x9b1f928b19ba6b0bULL, 0x8c84e00fd7f06d9bULL, 0x3fad17cbd0278447ULL, 0xeb7a41f56e2c5f2eULL,
    0xfbc63c4d2c6c3965ULL, 0x9ab96ff21a641a28ULL, 0xa832bf87b363fdc4ULL, 0x2b812317410a4379ULL,
    0x2bd1c5e152843155ULL, 0xcc24d8b28bc935b7ULL, 0x3fca0d25fdff806bULL, 0xefd4afea47bf0930ULL,
    0xd16793374903e8c9ULL, 0x48af011da240633fULL, 0x9e3a22216c89b468ULL, 0x50689dc217481424ULL,
    0x4b43c0dc7b5fbce2ULL, 0x30be1725b1412644ULL, 0x8ad173e150b56693ULL, 0x23ec5789dac251eaULL,
    0x1ba4f07a78684deeULL, 0xe14d05d473a5f091ULL, 0x2e5646f0688fc5d4ULL, 0xcc6055f3bb092632ULL,
    0x84c0232dfe2fa15bULL, 0x3a948cc8bca8f4d4ULL, 0xebbab53f8b2cd488ULL, 0x8e355721af8e4399ULL,
    0x8bab9ede93d1efe6ULL, 0x6bd30d7339afa1faULL, 0x1dff66a347f8b750ULL, 0x820d0ac4c04a5431ULL,
    0x00945bfeaa4da653ULL, 0xf65203cde1c67a14ULL, 0x64017b1f6d7660e6ULL, 0x4c23fe7c46272a2eULL,
    0x610365dd8fa1e5b2ULL, 0x26e1842faf459b6aULL, 0x1f961aefb6770961ULL, 0x09c3397165c36f30ULL,
    0xf2136ef9fbc45fc2ULL, 0xe88d6dd9adcce726ULL, 0x8d003d5edc84418cULL, 0x7c4d6b6c3d7304e9ULL,
    0x88ae6ae8726245a6ULL, 0x3412f396edea2b07ULL, 0x2f65e37016e9a869ULL, 0x8df912cd07f489e3ULL,
    0xe6af3fc7b992ac5cULL, 0x198b75fd3330c88cULL, 0x920edd264d30bab5ULL, 0x7f1fa0d7af191e17ULL,
    0x395d8a980e403790ULL, 0x8a402b7b2acdca5eULL, 0xc030e6695e214c9cULL, 0x22e0dcf37ff4c145ULL,
    0x2111f336bab565bfULL, 0x61a2770d697989e9ULL, 0xcba8f5b67005f6ceULL, 0x1a0a499cd71b0ef4ULL,
    0xa9dcc2c35a4f3905ULL, 0xfca509f2c373804cULL, 0x82b2185d68a7d270ULL, 0xc90f71c6982e71dcULL,
    0xa0fbe5e49cc20e03ULL, 0x8cbb93a5ee1bf578ULL, 0xab98e26c684fafb9ULL, 0xbcfc7f32ed9e9d0eULL,
    0xd7c39e4893d23142ULL, 0x29367a1a9aaa1c65ULL, 0x5909c56f588f7685ULL, 0x65c3930d9faa698aULL,
    0x802d369653fd8b5cULL, 0x2f890d4546b8f490ULL, 0x1346134080dd7835ULL, 0xe3eb6fbab330b08eULL,
    0x77dbb5da01eb53f4ULL, 0x377171d4350c661eULL, 0xea1161836a59ae79ULL, 0xa6feebb40ae5f830ULL,
    0xe6cc130af9337850ULL, 0x9c1120b73bb280aaULL, 0x2fed2c970fa8f45aULL, 0x80300f25880b1c84ULL,
    0x9c2abb0207dce1afULL, 0x58569aac7cb10a43ULL, 0x0f7546be97cab5acULL, 0xf87857864a1061afULL,
    0x4996455690b6293eULL, 0x8f35d2d6a5c426bbULL, 0x076f2bab8ffd52c6ULL, 0xbe48212ad0b9c51aULL,
    0x80d0e7ba33bf2edfULL, 0xc30f0aa8957168f7ULL, 0x253729f26a5a53b1ULL, 0x503df089f39c8e42ULL,
    0x88f857d06544265fULL, 0x3c61e1a5d4aea8a0ULL, 0x4fa7d9282963c1beULL, 0x03678fa2487bb947ULL,
    0x5d2492f527aefb51ULL, 0xcf0a306c104d0364ULL, 0x5be920772cad0c02ULL, 0x37705f8902c5f913ULL,
    0x6d1624633bd07072ULL, 0x4b5017f055f75d27ULL, 0x619a4b5f06babf49ULL, 0x53a53263a185b4c2ULL,
    0xe808f04864e997ecULL, 0x3868f76efc5ee1e8ULL, 0xb6c4f39fe63c108bULL, 0x7910f9529f2a051eULL,
    0x3a1e8bf70918df16ULL, 0x60ce2ad57e56893fULL, 0x8ffebebb87334674ULL, 0xf2d1e502e685f6f0ULL,
    0x802c514db9b0ee0eULL, 0x266a15b5e189ac9aULL, 0x9654b4dd610a5ac7ULL, 0xa7e6a2acb6522521ULL,
    0xe1452641949e3aedULL, 0xf852eb37d8292272ULL, 0x23440b894539e2dcULL, 0x3de3135d2480e2eaULL,
    0xf35b2a0a2e456d6cULL, 0x457c0694aa06eab7ULL, 0x10f8f60e000d20c4ULL, 0x3ee2e03bfccde830ULL,
    0x4bd6b8c76b8c2078ULL, 0x14a004df9b5c767dULL, 0x6465d9f58082ef7bULL, 0xacfef0a936ff1047ULL,
    0xe5044ad195710642ULL, 0x773310d1465bdb40ULL, 0x98cd081351af90edULL, 0xbdd24a947f995f9eULL,
    0x0b783c8add1fdcbfULL, 0x4c07f0d821ffe777ULL, 0xf9649e68057f1071ULL, 0x92b9fee00900c1dbULL,
    0xc10cd24cf919e4fbULL, 0x430da99db7548368ULL, 0x9ffbc57261a9bf18ULL, 0x93fe31caceb19a49ULL,
    0x3ff9497f33400965ULL, 0x355923fa7f6dfaa8ULL, 0x82b2de4000477a0fULL, 0x9f36510c4db52a2fULL,
    0x35fe019be7ce0446ULL, 0x854b79c6cf45d796ULL, 0xc7a8669ed85eab29ULL, 0x0616cc03ef9f5883ULL,
    0x05f32f21e7e08c08ULL, 0x3d7b96cc202021bdULL, 0x294d253dd6250389ULL, 0x396b20c1c40fbcd5ULL,
    0x924d5b892e00f5aaULL, 0x46f663fd7afec7b9ULL, 0x2dcb4fed2f62cf54ULL, 0x867656fb399a763bULL,
    0x6e645b42b0ed3df0ULL, 0xefbda0a959a82efaULL, 0x1e57f0d772fc387dULL, 0x0674d8a0f91d418fULL,
    0x7d505da665fb9ed1ULL, 0xb92f926e73571550ULL, 0xca4afd00ae0f2651ULL, 0x4ebfad7e532820f1ULL,
    0xc98143a7869aa213ULL, 0x659e2fb4bcff90a7ULL, 0x11ae0639334eee27ULL, 0x83e7533307ff2e6fULL,
    0x60f5f716547d8cc3ULL, 0xc4bfead179135b68ULL, 0x08f766a9a3a8de62ULL, 0xb684258554ee0537ULL,
    0x4891d873f8e3e0feULL, 0xf54572e5ac0b3a3cULL, 0x977a735beb8878cfULL, 0x4be958fbc1d86455ULL,
    0x0d7de03cb6629efdULL, 0x7b2c68ae5c970120ULL, 0x5ed392cce09f00e4ULL, 0x7da97f4ab6824d5bULL,
    0xa23b535e2b4a30c3ULL, 0xcbe9d4ab2a70eeccULL, 0x158896735f4e2fccULL, 0x83df96151a52fc84ULL,
    0xa6a2844c5b1131c4ULL, 0xe4e63b91dbcf8115ULL, 0xc83c4b75bc1a5b80ULL, 0x09f3cd287f693d64ULL,
    0xa366a3a824cec2f0ULL, 0x7e17190295ea7e8bULL, 0x65d4cb9fc6225bc2ULL, 0x29c5a0905e3c2736ULL,
    0x68e90502619dbf85ULL, 0xc1c427d5896e405cULL, 0xfa4afc8f8de8ecefULL, 0x8e5301dd532114e7ULL,
    0x933156b65ee4a918ULL, 0x76fd40da0354fd37ULL, 0x295d788895520e8aULL, 0x82d8f759909f819eULL,
    0x6f8be0b29b366883ULL, 0x57192fcaf5eb578aULL, 0x86a3f9fa0fd932f0ULL, 0x3a9712777a36e539ULL,
    0x4e9a81b491b610d4ULL, 0x112ab256f111f7b3ULL, 0xeb18a646723db641ULL, 0xf5df4391e176cfdfULL,
    0x0988c3ae0a878c1eULL, 0x83eee968ca9994a3ULL, 0x2f87a0054d15a108ULL, 0x8e8f6ad28a52fb62ULL,
    0x4506387e508b536eULL, 0xf4be16fd24b9def7ULL, 0xccdee07b69cfc3dcULL, 0x1d61f742a60333e8ULL,
    0x2bf5dd9169aa7f01ULL, 0xc226cf16c071fa2eULL, 0x817f04841559b8a3ULL, 0xf6af48567c6e78c2ULL,
    0xcd804be16811e77bULL, 0x6b397f055a9882efULL, 0x088fda249f941a3fULL, 0x6dbf67f07092c293ULL,
    0x1645327035a2bd54ULL, 0xc39c205b04ec4e8dULL, 0xc01c17f1dbd0fc6aULL, 0xa28db4ab9710fa3aULL,
    0x7d90dcc1fdbb11c3ULL, 0x103546bdb065c73eULL, 0x8141482edf322617ULL, 0x4a346c9c5f8d353bULL,
    0xb383563ab40c28f6ULL, 0x02d49f9b654646e2ULL, 0x40cf6a1d632f905cULL, 0xc79b313acfb4b5f7ULL,
    0xef662c62dca07ef1ULL, 0xcef5d422f7659834ULL, 0x8c73e6fb1b697f4eULL, 0x7231cadcef269eadULL,
    0x9951b99f01fb70f1ULL, 0x536778b8e9f3b25bULL, 0xa75da0d5cb39c1beULL, 0xabcaba5e733f76a3ULL,
    0x5a3b1f3f07d061f6ULL, 0xb96c2bd8a2f04332ULL, 0xb9205d72024398b9ULL, 0x7b3dee2a6799b3bdULL,
    0xaa9c6adb7db1a38aULL, 0x08b635cdf28e5f5dULL, 0xc915c83f2ee319ebULL, 0xcd207081bbf6d18fULL,
    0xd4fef1db4cd1fb0dULL, 0x481eda802b30c94aULL, 0x7b404bcd3fb72ca8ULL, 0xbedcddb701683d01ULL,
    0x0d0e7d869fc2a5c9ULL, 0x9f32dc09ea332545ULL, 0x932528e46eff6c95ULL, 0x3e4c75e674bd93fbULL,
    0x8c8391e4a1861342ULL, 0xe7ae7559f3c3c3adULL, 0x62c883f00d9e4d9fULL, 0x0bc550d471eac53eULL,
    0xb6b944c871a8565dULL, 0x2a01cfb60b741f8eULL, 0x49db94bdff8b73b5ULL, 0x80b474d2d8fd36f6ULL,
    0xa6c75296fb9f028aULL, 0xfc0f8bbf3ad79375ULL, 0xeaeb54285d08c14fULL, 0x0eef9a5fb0c3e2edULL,
    0x54d588939fe544f6ULL, 0x17084509804201ffULL, 0x31b8372478d93e4dULL, 0x5885e4f1e37edfceULL,
    0x4e1bdff27c17069aULL, 0x08a253bd9b9cb273ULL, 0x5435a8db7d2a894eULL, 0x975dafbadbaa58a0ULL,
    0xfdc47ff3f5f9a594ULL, 0x4323e44de8a86bb3ULL, 0x2be02fa2a15f8860ULL, 0xa0f6d6b4378d111eULL,
    0x628b1647a8f34039ULL, 0xb1ae42641adc6944ULL, 0xce22f3b15bbca65dULL, 0xff839edc88ee6833ULL,
    0x9b944c8fbaffbe94ULL, 0x4dcf12e95dbf59d8ULL, 0xd4e8006c8265fa6dULL
};

U64
Position::getRandomHashVal(int rndNo) {
    assert(rndNo >= 0);
    assert(rndNo < (int)COUNT_OF(zobristRndKeys));
    return zobristRndKeys[rndNo];
}
