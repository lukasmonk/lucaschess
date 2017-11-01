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
 * position.cpp
 *
 *  Created on: Feb 25, 2012
 *      Author: petero
 */

#include "position.hpp"
#include "evaluate.hpp"
#include "textio.hpp"
#include "parameters.hpp"

#include <iostream>
#include <cassert>

const U64 hashEmpty = 0x5fd230cc43568439ULL;

U8 Position::castleSqMask[64];

static StaticInitializer<Position> posInit;

void
Position::staticInitialize() {
    for (int i = 0; i < 64; i++)
        castleSqMask[i] = ((1 << A1_CASTLE) | (1 << H1_CASTLE) |
                           (1 << A8_CASTLE) | (1 << H8_CASTLE));
    castleSqMask[A1] &= ~(1 << A1_CASTLE);
    castleSqMask[E1] &= ~((1 << A1_CASTLE) | (1 << H1_CASTLE));
    castleSqMask[H1] &= ~(1 << H1_CASTLE);
    castleSqMask[A8] &= ~(1 << A8_CASTLE);
    castleSqMask[E8] &= ~((1 << A8_CASTLE) | (1 << H8_CASTLE));
    castleSqMask[H8] &= ~(1 << H8_CASTLE);
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
        } else {
            bMtrl_ += pVal;
            blackBB_ |= sqMask;
            if (piece == Piece::BPAWN) {
                bMtrlPawns_ += pVal;
                pHashKey ^= psHashKeys[Piece::BPAWN][square];
            }
        }
    }

    // Update piece/square table scores
    psScore1_[removedPiece] -= Evaluate::psTab1[removedPiece][square];
    psScore2_[removedPiece] -= Evaluate::psTab2[removedPiece][square];
    psScore1_[piece]        += Evaluate::psTab1[piece][square];
    psScore2_[piece]        += Evaluate::psTab2[piece][square];
}

void
Position::clearPiece(int square) {
    int removedPiece = squares[square];
    squares[square] = Piece::EMPTY;

    // Update hash key
    hashKey ^= psHashKeys[removedPiece][square];

    // Update material identifier
    matId.removePiece(removedPiece);

    // Update bitboards
    const U64 sqMask = 1ULL << square;
    pieceTypeBB_[removedPiece] &= ~sqMask;
    pieceTypeBB_[Piece::EMPTY] |= sqMask;

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

    // Update piece/square table scores
    psScore1_[removedPiece] -= Evaluate::psTab1[removedPiece][square];
    psScore2_[removedPiece] -= Evaluate::psTab2[removedPiece][square];
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
                clearPiece(move.to() - 8);
            }
        } else if (p == Piece::BPAWN) {
            if (move.to() - move.from() == -2 * 8) {
                int x = getX(move.to());
                if (BitBoard::epMaskB[x] & pieceTypeBB(Piece::WPAWN))
                    setEpSquare(move.from() - 8);
            } else if (move.to() == prevEpSquare) {
                clearPiece(move.to() + 8);
            }
        }

        // Perform move
        clearPiece(move.from());
        setPiece(move.to(), move.promoteTo() != Piece::EMPTY ? move.promoteTo() : p);
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
        }

        // Perform move
        movePieceNotPawn(move.from(), move.to());
    }

    setCastleMask(getCastleMask() & castleSqMask[move.from()] & castleSqMask[move.to()]);

    if (!wtm)
        fullMoveCounter++;
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

    squares[from] = Piece::EMPTY;
    squares[to] = piece;

    const U64 sqMaskF = 1ULL << from;
    const U64 sqMaskT = 1ULL << to;
    pieceTypeBB_[piece] &= ~sqMaskF;
    pieceTypeBB_[piece] |= sqMaskT;
    if (Piece::isWhite(piece)) {
        whiteBB_ &= ~sqMaskF;
        whiteBB_ |= sqMaskT;
    } else {
        blackBB_ &= ~sqMaskF;
        blackBB_ |= sqMaskT;
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
    U64 hash = hashEmpty;
    pHashKey = hashEmpty;
    matId = {};
    for (int i = 0; i < 4; i++) {
        int sq0 = i * 16;
        U64 v = data.v[i];
        for (int sq = 15; sq >= 0; sq--) {
            int piece = v & 0xf;
            v >>= 4;
            int square = sq0 + sq;
            squares[square] = piece;

            U64 key = psHashKeys[piece][square];
            hash ^= key;
            const U64 sqMask = 1ULL << square;
            pieceTypeBB_[piece] |= sqMask;
            if (piece != Piece::EMPTY) {
                matId.addPiece(piece);
                int pVal = ::pieceValue[piece];
                if (Piece::isWhite(piece)) {
                    wMtrl_ += pVal;
                    whiteBB_ |= sqMask;
                    if (piece == Piece::WPAWN) {
                        wMtrlPawns_ += pVal;
                        pHashKey ^= key;
                    }
                } else {
                    bMtrl_ += pVal;
                    blackBB_ |= sqMask;
                    if (piece == Piece::BPAWN) {
                        bMtrlPawns_ += pVal;
                        pHashKey ^= key;
                    }
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

    if (whiteMove)
        hash ^= whiteHashKey;
    hash ^= castleHashKeys[castleMask];
    hash ^= epHashKeys[(epSquare >= 0) ? getX(epSquare) + 1 : 0];
    hashKey = hash;
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
    U64 hash = hashEmpty;
    pHashKey = hashEmpty;
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

const U64 Position::psHashKeys[Piece::nPieceTypes][64] = {
    {
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
    },
    {
        0x00a0f8864fbfa47bULL, 0x6149a3743bbf7441ULL, 0x2b70bc7b9cf8bcdfULL, 0x80f40e53c2e94e43ULL,
        0x16ea0c0fd76bafd1ULL, 0x575ebf56d96c6100ULL, 0x86e35a73fda47fc3ULL, 0xdf693c0f0871c872ULL,
        0x64fe055b8e17690dULL, 0xacbd6d400b9d699aULL, 0x0c95a9534227f214ULL, 0x6d13838fba237093ULL,
        0xa41ea5e45e09c461ULL, 0x9127addfbe15178aULL, 0x9b9108fd5e330f66ULL, 0x568bfd8805eb75e9ULL,
        0x8aa786296b4b4ad4ULL, 0x8388b96b99de173dULL, 0x6678eb8a2881814fULL, 0x37e4e35f6f671c83ULL,
        0xa41fa8ea20de1a6cULL, 0x96bb2348795c81a3ULL, 0x638780ce87df2113ULL, 0xb8cd919bca6c3777ULL,
        0xf5e87156ea7af407ULL, 0x097f243637c33904ULL, 0x47578ef0905d887bULL, 0xa05c4ad2f9541b1cULL,
        0xc03fd407b61e4474ULL, 0x26827ca6395534f1ULL, 0xd1b3efcfb6647bf5ULL, 0x3a583fbb75f49ee2ULL,
        0xf9b2e9a53bc68a25ULL, 0x13607ccdfdad2c0fULL, 0xb734aa67a4387110ULL, 0x6a0fd6cebec99a5fULL,
        0x3fe8756e8c900933ULL, 0x5d614ab864cc77bcULL, 0x16b6131d111e0353ULL, 0x34a60e0afdf3258bULL,
        0x36f83abba48936b2ULL, 0x51ae50defc19e0aaULL, 0xe4ee8260ec8daad7ULL, 0x15d1eb884c8d9cf3ULL,
        0x0ab141420e35e1b8ULL, 0x21753ae57ef6f7abULL, 0x8dbefaad263d849bULL, 0x40fffc7e287f45f3ULL,
        0xe2c5120d568bebefULL, 0x416e150f5d6f4c82ULL, 0x49e22657b3cbd040ULL, 0xd7f6716b9762a1c2ULL,
        0xef9285ce045f0fb7ULL, 0x0cfad2c7720bfda2ULL, 0x3e4bd80d941ab0d2ULL, 0xc4a28df3468aca67ULL,
        0x9ba75d781035660eULL, 0x6b2fa89859607740ULL, 0x88e09e3cd55507ffULL, 0xdda1bcbb5ef6190aULL,
        0x9fb765c48a10319aULL, 0x99ad6df590e70f9cULL, 0xe3c1d1fc8f59a7e3ULL, 0x4ba9732f13e02827ULL
    },
    {
        0x7c18f4361f491071ULL, 0x56141c53455ec5dfULL, 0xc8972ceeb2ce7331ULL, 0xf908376505cc0a41ULL,
        0x63d6afea92023e8aULL, 0xc089c7c3e0a692d4ULL, 0xd18e596193fc8a1dULL, 0x375009ffe7497f2fULL,
        0x838b2941d202a159ULL, 0xbc1ba37f02b3e266ULL, 0x8696e44fcac03135ULL, 0xfc5b427dd9885619ULL,
        0xa61ef17d96335045ULL, 0x6393dc95960a1343ULL, 0x2b8e55183fb8cce1ULL, 0x2ed114d7ca1a44d4ULL,
        0x023d00fe23d77b4aULL, 0x6464de4c956d685cULL, 0x29bb76d1543ad8a0ULL, 0xe1a969018d1ead69ULL,
        0x4551d3af1baeebe3ULL, 0xfe0098cb913eff16ULL, 0xef2f03a076f63546ULL, 0xc3529ca2fdf70defULL,
        0x108fb26a14752313ULL, 0xaba3bdd9281fbeabULL, 0x1ed591ff5ff68adaULL, 0xa2671949619a89adULL,
        0xa6b4db59c015a180ULL, 0xacae8f5271ace9dfULL, 0x3665c5c1789b4ea4ULL, 0xf9f077e96da79891ULL,
        0xcd7c15a006da72eaULL, 0x5b8a1f76b6bcb511ULL, 0x4307bea125ffe527ULL, 0x9cd385915d73a6b2ULL,
        0xdab4eea9f7f225b7ULL, 0x1c2ef6db75e56bf1ULL, 0x948d48db6b35a6a0ULL, 0x4b269ae693db9729ULL,
        0x2c38c25012b8b816ULL, 0x87e711f8fc6f4b8cULL, 0x4ce965c817299362ULL, 0xd2ad355e63740666ULL,
        0x263afe9bafd4cbabULL, 0x857d473b2c40f9a4ULL, 0x6e0ef33b2e2b0724ULL, 0xeac1fe1fa5159c27ULL,
        0xec79f566ab023abdULL, 0x1e47ace84e09a5d4ULL, 0x31c349292d06282bULL, 0x48ec0d2f122ca837ULL,
        0xc7c6a9399c7cd707ULL, 0x096af4542d610aebULL, 0x19a6dd4f3ab1ec68ULL, 0x1626d384ed01d117ULL,
        0x90767e9ebe9b8b63ULL, 0xde3da4b933085c83ULL, 0x8d85140f03f850b2ULL, 0x5a874d33ae28d71cULL,
        0xf2fb743bada5b998ULL, 0x99886ccbf2246666ULL, 0x667aa89f94bce309ULL, 0xdad81b7a0a068583ULL
    },
    {
        0xc48edff7d00bca38ULL, 0x5e86c4cf280db4cbULL, 0xfb091cdfb2ce5b9aULL, 0x682e7cb2fdcd628eULL,
        0x8e4cf8940b3a63b0ULL, 0x3a726de341d2cc16ULL, 0x1c5b54689cc27db1ULL, 0x1c23c6766b7bc7f9ULL,
        0x7f4692b1e6ebbffdULL, 0xee6b2a4fbf114180ULL, 0x699545a1e7d599ebULL, 0xe91db31857129fa7ULL,
        0xf6e253f52d9363f3ULL, 0x18a840395f031d4bULL, 0xd5f1f7494f3367beULL, 0xcd987e8c3342b92dULL,
        0x0975a5ad604644ebULL, 0xf95fb5d84cd5136fULL, 0xcc219607b4fe778aULL, 0x8f0ee232b96f2fc1ULL,
        0xee0b6175e3025fb6ULL, 0x5ff8a16f75a4b29dULL, 0x4e6bde5f1e1fa9a8ULL, 0x15c5288ea5d5ed4fULL,
        0x067cd3332c49d0ecULL, 0xcb8ac2ac5e948ea5ULL, 0x17f3d8f96e6ba398ULL, 0xe92ab12c0046279bULL,
        0x2ecaa94997470b75ULL, 0xdcb000eed287a00fULL, 0x2a7573d4ef0e4989ULL, 0xcc54718c8f1274a3ULL,
        0x758f0f5fec759e32ULL, 0x7cc9e60be5629ad8ULL, 0x3dc06c1080223a5dULL, 0x9454f49e75ec326dULL,
        0xe9d66420694baa42ULL, 0xeafbc9c4081d61a8ULL, 0xb7816d9ad579f4b9ULL, 0xc2259878228b151eULL,
        0x69e133ec3c163f3dULL, 0x323182afe64d0384ULL, 0x14a25e894c6cb3baULL, 0x8d13ebad77a6b8b4ULL,
        0x274de1fb1891df57ULL, 0xcdaecff47e60ad7bULL, 0x72dbc18992af9378ULL, 0xe176d29ae19501c6ULL,
        0x0db6fe5f13abec78ULL, 0xfd97dc1aa91b59e9ULL, 0x99ccd5169ac0fc4dULL, 0xebfc6550f05f2d2dULL,
        0x3d599c85a6d6ac29ULL, 0x507bb8690fcd6c62ULL, 0x226467f6dad0ebe0ULL, 0x25091a89a3b8f597ULL,
        0x0d8931134c7cc6aeULL, 0x1f1326c4e73eeabdULL, 0x7b86bf4d331af58dULL, 0xe02ee3e772bb04c5ULL,
        0xb93161efb75907abULL, 0x98627858577820c0ULL, 0x1a0b80d939dc3124ULL, 0x77f73ed48e157550ULL
    },
    {
        0x28422e4539509fafULL, 0x09aab21219307788ULL, 0xdc0f60d784721765ULL, 0x86cd4d7bfc8f6e49ULL,
        0x6351fe8c3ff0d5b2ULL, 0x6b1aae9695b47c74ULL, 0x8c9b1acf54845e0fULL, 0x2cd1d21230b327b4ULL,
        0xbc63e9d83a0da03cULL, 0x912cd7d41abbf7b0ULL, 0xc5cbbbff0df5e015ULL, 0x4f5b50b12f4c35b3ULL,
        0xcd6b00009579e375ULL, 0x9849253c6f9d573cULL, 0xb0814db92ded2bafULL, 0x72fc3991c51826a8ULL,
        0xb3c92c7652453d9bULL, 0xbf687be59dd05429ULL, 0x83426426c2205302ULL, 0xf5246808fcff3cd7ULL,
        0x393cef4afb46cf08ULL, 0x1c45e8f31beac230ULL, 0xbaa245c73bb40938ULL, 0xc10bf13dfb296b7cULL,
        0x6fc4e1f8f278d345ULL, 0x82f85144cf050537ULL, 0x9bdeadf48607a55aULL, 0x89e64782b9dec532ULL,
        0x57a4f70576a6e3efULL, 0xce5992e14bc81c82ULL, 0x142a9857999a6a07ULL, 0x1a03d3fc9aea9635ULL,
        0xa0b5e401bbbc77eaULL, 0xe1bd11ffe90427a8ULL, 0xf7977987861c40d3ULL, 0xa3aca66c6ed25623ULL,
        0x12e11ba4e58e587aULL, 0xf49b844dda41cd17ULL, 0x63867e96aff27d9eULL, 0x469b0d8b31055e2eULL,
        0xcb2524b609c6a3c0ULL, 0x6f0fd97dfb2df61bULL, 0xfe8065308fb11a25ULL, 0x116c93d509744665ULL,
        0x1be70eb559030f81ULL, 0xc7dd1fd9ac050b02ULL, 0xba66467da129cc34ULL, 0xffe20b75345bc824ULL,
        0x4ce38caf921a1637ULL, 0x3c9f42314878e776ULL, 0xb75701dc34283fc4ULL, 0x1a0494dc4dea7459ULL,
        0x3ad593e5cac65af8ULL, 0x9f47501b115290b3ULL, 0xbc2dff873d623886ULL, 0xd64ca0fb2192f9c0ULL,
        0xf8aa65c56adefd34ULL, 0xa187352cad6d4770ULL, 0x4ec6edf368740ae4ULL, 0x3e18c5624f3f10eeULL,
        0xb0eaa4716c167efdULL, 0x6a48e5af4e2b5ac8ULL, 0xf7eb7499605add80ULL, 0x3e9d4cf5c864576fULL
    },
    {
        0x19e7e23d963f9fd6ULL, 0x7d06a6919730cf56ULL, 0xe4b330c9a39d27a9ULL, 0xc716b9385ecbc6d8ULL,
        0x838fc579e3908229ULL, 0xc99c8a21b25528afULL, 0x6163b767406cda68ULL, 0xa52b9d5b0953e2eeULL,
        0x568a88bb96b2a5bfULL, 0xa025b60864bad1b7ULL, 0x55bb7e97a6d318eaULL, 0xcd39b6965cf75787ULL,
        0x442d5341c2924596ULL, 0x13f4dc19c47af0b9ULL, 0xbe19b03e67ff8e48ULL, 0x6dbab7d18db095f9ULL,
        0x10e740982eb7c771ULL, 0x43db983bff7389feULL, 0x6e5940578cb06e55ULL, 0x87d67bdfa8bd8340ULL,
        0xdcf1e7cb650a17b2ULL, 0x34fc5a82fff67302ULL, 0xdd23380e1d85e507ULL, 0xf9f29286161fe419ULL,
        0xb5e1970d66d13fdfULL, 0xf9aee5332e66d6deULL, 0x255c65f3dc8ce6f2ULL, 0xc201f60b7c3d03dfULL,
        0x2a5689ba446c1ebeULL, 0x49ee7afdd049ce49ULL, 0xd7eb2426007f9f63ULL, 0x65f43984cf1c23f1ULL,
        0xc2d814ba16c50323ULL, 0x64ee64651b755e9dULL, 0xaaeba2f89795ba07ULL, 0x9d66211d640ba5d4ULL,
        0x0d9a67be3ab0b7edULL, 0xd390641eac37da15ULL, 0x8ce9a64fa456334dULL, 0x5ea67e4272155db5ULL,
        0x49bde87ee9f4185bULL, 0x7437d88ff7e66709ULL, 0x9a5ced94e6289f56ULL, 0x58c012eee86ae97fULL,
        0xd2521c721774ab16ULL, 0xe20162bd642745f5ULL, 0x29398bd2d8683bc6ULL, 0x8db1fa4b2057a5d5ULL,
        0x3cf335271142093eULL, 0xae450424babdec83ULL, 0x4ac24b29ef867626ULL, 0x196a4e9f07932bb8ULL,
        0x4d1e3f272dff7cb0ULL, 0xfe4a5d447d2a7f46ULL, 0x455c229f7f291508ULL, 0xf41849c532ff0a04ULL,
        0x2cc58a59bbc8da5fULL, 0x06b7b89f89711cccULL, 0x5167217c294ff872ULL, 0x1c22f072e56763dbULL,
        0x84a63f2ccc189928ULL, 0xbef4172532aff1aaULL, 0x492d096086e479fcULL, 0x327c3c20d0465b66ULL
    },
    {
        0xc0f0aadb3ab78c7dULL, 0x9c7b12a4c97743c9ULL, 0x3bfd2ca85692c75cULL, 0x4415e203352c2396ULL,
        0x1f2306d85c2de7b2ULL, 0xaa6987767be5580dULL, 0xbe26697484956320ULL, 0x958fee620b568f8eULL,
        0xa59b01d839e352beULL, 0xddaf48cc6ff12c73ULL, 0x801f89ad59049661ULL, 0x2c8a63c4f136c300ULL,
        0xc9c5d4461052f613ULL, 0x251c403b2a707b35ULL, 0x5e20f4c9962b8cf3ULL, 0xdd8c7a8a22136682ULL,
        0x1dd8e97acaf094bfULL, 0x32d86f486233786fULL, 0x841d8dfc11fe3360ULL, 0xae285d7d5d15e1c5ULL,
        0x488a2fb4aeade10bULL, 0x58e46b728a3b8a8fULL, 0x2aa6818643c59160ULL, 0x7f5ec293d632d50fULL,
        0x2575cb81dd895f60ULL, 0xd7d8a74ad4c739ddULL, 0xc778d64c725274dcULL, 0xc48d61a710111a3dULL,
        0x0c4f67bfdfb71c60ULL, 0xf9b91da1129923c1ULL, 0x3f359a60f941a357ULL, 0x88da3d5511cb3193ULL,
        0x512d4737d4e95acbULL, 0xcdba63ff5f1d0a2bULL, 0x6e6ed849f023c133ULL, 0xc2c8d8898d9f1d36ULL,
        0x278b966de4f33e53ULL, 0xf3117f630254acf2ULL, 0xe9cadc30fd65fd45ULL, 0x896b700c81973ce6ULL,
        0x970ceb33954e4d06ULL, 0x3693879c01c08dfbULL, 0xcd193b952aab5340ULL, 0x70cd97c3baa437f8ULL,
        0xebd39087dfb949a3ULL, 0x8d1de0b5fe9df76bULL, 0xc49dc3d6ce93e2b4ULL, 0x3e3c7db1ed8fe4ffULL,
        0xc8e65ce3b66d7251ULL, 0x7c318dfe05df44c1ULL, 0x13b0cc0a5a3878cdULL, 0xe005308239144e6eULL,
        0x93c8c03225e3f863ULL, 0xb61cbee74cf603f7ULL, 0x2609ebec2d505973ULL, 0x8617e7da4605e48dULL,
        0x2f8f1c46df2ea0d0ULL, 0xc84ad807472aa31bULL, 0xf8edf9a905e939e9ULL, 0x79363734dd45e7feULL,
        0x462819e99be7b18eULL, 0xb3a71b62926be510ULL, 0x43a06388cffcf308ULL, 0xe657f0dfa3898d6cULL
    },
    {
        0x261c9498c8b1ff90ULL, 0x4c84573372e7b0c2ULL, 0xa44db72bdef07704ULL, 0xb738deefd52795fdULL,
        0xf842ccb939b926fdULL, 0xe4ec5396ea3b08c3ULL, 0xed9ab3c8d8c0c754ULL, 0x8a22bc918826f24eULL,
        0x137f5f24324970f6ULL, 0x96f0257d0642eb6cULL, 0x035a0a3ae1eea866ULL, 0xb73f40ce1925df81ULL,
        0x77c7381a7db5f37eULL, 0xd06bf3434561edc8ULL, 0x1973469ed890edb6ULL, 0x3ceec9259a8638a4ULL,
        0x7eff91e2202f5e68ULL, 0x7577383703bba556ULL, 0xb745dc71ac8e2fc1ULL, 0x0761d696bb0a49dfULL,
        0x2b0908c95336f032ULL, 0xdf5660ccfb914068ULL, 0x8c6a332e6016d88cULL, 0x1d199e1fa17c55d1ULL,
        0x8c6e718c9678c7f5ULL, 0xe272c135bf2a6bdfULL, 0xd1633848151d6317ULL, 0x7f2568493f3949ddULL,
        0x5fc65f23e3e27462ULL, 0x11e301fbe8d801e0ULL, 0x0f611ff5f67ef58fULL, 0xabcfd9c8871d08a8ULL,
        0xaa4fe88ce302d9a9ULL, 0x8853ad40817e9131ULL, 0x0e0d9e04cc79b53eULL, 0x7caef5185f759ae6ULL,
        0x5ad3862dd1e2ddb7ULL, 0x092663b4b7131dedULL, 0x6d50928956cb98eeULL, 0x4b831401ce005fd7ULL,
        0xb452cf44505b5678ULL, 0x7cc099155771b901ULL, 0x740d1a6534a9f0e1ULL, 0x81d549475cb0b0b0ULL,
        0x6514331f6b42c608ULL, 0xa82d604239ffd4a4ULL, 0xbcbad30c5f758f96ULL, 0x2b0263f466907201ULL,
        0x94f0a10198ee0fbdULL, 0xd16e84434af80bbfULL, 0xc6361675858f2bafULL, 0x3cab55e952330d11ULL,
        0xbe6ad0eb0836e75fULL, 0x617047f38e5ef2bbULL, 0x35160cb82fd24e79ULL, 0x906ee7d9b9bc13f4ULL,
        0xc0a2e2d207b8a9a3ULL, 0x53f7d4bd6c3c6c45ULL, 0xa6215126992f30a2ULL, 0x48bd42731b562291ULL,
        0x521edf44db857150ULL, 0xddff8341a762e2d2ULL, 0xc64fe4b92e09cc70ULL, 0x3ed5dd0bb38eed05ULL
    },
    {
        0x56c07535144122d0ULL, 0x3caf53b352c0a930ULL, 0x4b87fedc6d32a8e2ULL, 0x8b6927fa7f27dcaeULL,
        0xdb6497e5976b2986ULL, 0x1a89aba5db0e95f2ULL, 0xf717a1184ff461f5ULL, 0x167f2efcd18279aaULL,
        0xb96d6afbad4e6a31ULL, 0xd2b4e8322961e2ecULL, 0x028bb4e53172a7a5ULL, 0x205ed0e2d4609c22ULL,
        0xee187500a5713dbaULL, 0x6913202f230d3461ULL, 0xd6901354fd764487ULL, 0xd9519c8150410f18ULL,
        0x854db86c19cefd75ULL, 0xaeb86bf4f60fa304ULL, 0xe06886d4541baca2ULL, 0xd6fec32043120674ULL,
        0x2363fbbf1f67e722ULL, 0x2ad104550bad8cf4ULL, 0x7c14644e8bfa97eaULL, 0x2164ea229e7bdc23ULL,
        0xa0adbba3783076a5ULL, 0xe7eeed7d1f11ef03ULL, 0x507c3ea8f8f149c9ULL, 0x8c320326235517faULL,
        0xc2f8d685b70ab8d3ULL, 0x64fee0ed3521fd2bULL, 0xfe4a343960894733ULL, 0x966fd16cc8b855e4ULL,
        0xe897d1f9c224ce92ULL, 0x27f95b0f6fdb3671ULL, 0x91ed0e5d52ea3ecdULL, 0x775c9b71b4874b29ULL,
        0xcb4328264f46d789ULL, 0x2b9690ba38bf12e0ULL, 0xae5ec1ef99ba1279ULL, 0x868f88471de8815cULL,
        0xeadddcb1645ae9c9ULL, 0xdf2e2319e407de07ULL, 0x784732d246a5de79ULL, 0xd23473bf4539f5dbULL,
        0x12c8a27f5e0d8152ULL, 0x71cac0280ca09008ULL, 0x14568f19b801cf48ULL, 0xcc7fb4b467f54e14ULL,
        0xd5b0e828b0e3949bULL, 0x8aea9c620b02c4f2ULL, 0xdfaa659d27ee4797ULL, 0x09cfd17ed28f5395ULL,
        0x968cc401ea4a301cULL, 0x27234a0d7077af98ULL, 0x3ec8c91f40da1932ULL, 0x3fde3699d8561fc0ULL,
        0x61b0e4639de8e40fULL, 0xd0b303bd63137bf1ULL, 0xef7117fc7d683acbULL, 0xf5327deb45e11cd2ULL,
        0xe9c9349838700e94ULL, 0x593c91ca016ec02fULL, 0x61f30c266028c178ULL, 0x979a2f232700c267ULL
    },
    {
        0xdbe912f42580f883ULL, 0xa604e8b0f8845713ULL, 0xde8dca3f2438684eULL, 0x29162df09f9caef7ULL,
        0xa8e427ad493cdf19ULL, 0x5d2e7e43f7adf80cULL, 0x5d615fc044daf7bfULL, 0xdaa694e9fb6eaa57ULL,
        0xdaa2edb376a1a312ULL, 0xa3b39e30ea9d3741ULL, 0x13614b0958710878ULL, 0x136a6ad99f0bf289ULL,
        0x5f9ee7dee507a281ULL, 0x1551e03cda488cc9ULL, 0x618133b5a79506dfULL, 0x45fb9a968406f58bULL,
        0xfc81aac78775ee6dULL, 0xb16f8f91fc7bf3e6ULL, 0x9a7daeff269bb076ULL, 0x193fca1fd9cc4659ULL,
        0x0c55b9ad769f69baULL, 0x3ce6e04dee324700ULL, 0xf6f07a4ae3a57ebdULL, 0x036a4734d5819929ULL,
        0x84c65f081845bfe1ULL, 0xe11696fedf011d77ULL, 0xa7ecdc3a53db7349ULL, 0x5ad27ba47729988dULL,
        0x21b8a68326183ad1ULL, 0x2ba5b1fff74a71e6ULL, 0xe53b475d19f161caULL, 0xed21205b954431e3ULL,
        0xfa84b99849e09721ULL, 0x1629ff41383a8c0fULL, 0xa75c09b5d64a44caULL, 0xaf877f6c30c912edULL,
        0xa221cf3c6d1038beULL, 0x800a62d632476804ULL, 0xf9f306a96835a212ULL, 0x4e67cc660d1d195bULL,
        0xc615d4c347f07ee5ULL, 0xb0bca408e86f7ff6ULL, 0x282e81c216a2fa95ULL, 0xdc796defc29b9381ULL,
        0x75c97fb69ce47192ULL, 0xfcc972e2310f85dcULL, 0x9cefddb7090b5329ULL, 0xdf831c1fafaf0458ULL,
        0x9258abf19b39d9b6ULL, 0x9689849476d04a9dULL, 0xddac2b5821f8d87bULL, 0xb595ee788063483dULL,
        0xa3af26331aff21e4ULL, 0x1fd7bfcb5e378636ULL, 0xc955b6d5820e3683ULL, 0x1dcd75a9752e19a6ULL,
        0x4ad68ad0928acfa1ULL, 0x1055e86bff5adad0ULL, 0x7a4cad7a6c69b58aULL, 0xffdee3e2b84a6dd8ULL,
        0xd415281a1a436034ULL, 0x6e18a1089a61a06aULL, 0x616ed7b1c9ef7f39ULL, 0x2774c97798a0ff96ULL
    },
    {
        0xa85135d391c0fa83ULL, 0x73b4b78ec98a9f2dULL, 0x3564d80840262aa1ULL, 0x167561510a89bfb4ULL,
        0xf0cccb06a986e07dULL, 0xad5a2f2e888bbf83ULL, 0x4cfb6f0eea21f4edULL, 0xe6e89fbcbfcadcbcULL,
        0x894cc1029ad6a3c7ULL, 0x145ee9654684b143ULL, 0x1c24ffa7e058fd11ULL, 0x9139d8196f5e0febULL,
        0x21cbd29ab423a46eULL, 0xc848c8ed1a552781ULL, 0x62cddd9bc77bf31aULL, 0x96427e2333a98f09ULL,
        0x3a0e3c8f6f0f64adULL, 0xb97e14ffab7cbc29ULL, 0x2d0f89442ae6419aULL, 0x70845de1e9629714ULL,
        0xfcae01afc1505374ULL, 0x647aed36e372bd31ULL, 0x50c26790d0cb0d0dULL, 0x52c3344ce64f2538ULL,
        0x452ef97a3be09141ULL, 0x4b352131764afd5dULL, 0xb973ff4213591d7dULL, 0x6fdf07cd48aaa8d7ULL,
        0x0ee1a030096559c3ULL, 0xee7fed0bbd712ba5ULL, 0x4fedb17af097feceULL, 0xf7e665ff6808c145ULL,
        0x3dbc872702c60c43ULL, 0x3cc8bc82c3db869aULL, 0x2cf8eb289313db23ULL, 0x4939575712d93b55ULL,
        0x4a847ee2b2a5271aULL, 0xa29399b77a853bceULL, 0xc94727f19076c235ULL, 0xa4b7633472005bbeULL,
        0xcb11e2be924dbdeaULL, 0x7757edb70d25d4aaULL, 0xe24fab9288a6dc87ULL, 0xf80d19c2779573f0ULL,
        0x3c7914a0b04a1dd5ULL, 0x0a19f6fcd049f62aULL, 0x160e333ecf688f95ULL, 0x5559b744f5f225deULL,
        0x6d1a909c06b56455ULL, 0x6ebadb05afc3bc90ULL, 0x16b72cd7b647e030ULL, 0xa4804cc11fb7fe4eULL,
        0x23e3d68563953520ULL, 0x905891bf3cf27bd2ULL, 0x762a9a525eca7113ULL, 0xd76fc6d343a0a8a1ULL,
        0x84567e7edfe95a24ULL, 0x55b06687fa5cb98dULL, 0xc4d1cf56e09ddc72ULL, 0x37799f50d8586a3dULL,
        0x3519b428bbf9855bULL, 0xcf7578157d381f4bULL, 0x629657fe7eaf1524ULL, 0xd6c5337a7e63cae0ULL
    },
    {
        0x25e6d41e83f1c952ULL, 0xb1f7ed31a99abf1aULL, 0x791ffb7bd70dee86ULL, 0x1cf6a7ddfee0ef67ULL,
        0xe834118bc007ff70ULL, 0x4830f02438e47bdaULL, 0x73be86c0360ecfe9ULL, 0xb7234d55c09b81ccULL,
        0x5ec975fcf1a707f8ULL, 0xe4d995b7c0538711ULL, 0x8b8a86969142055eULL, 0x4d20b14595c9d083ULL,
        0xbd56eead9c226d63ULL, 0x82906306f54e2cefULL, 0x213222ac311df06dULL, 0xa5760fc129dbc4ebULL,
        0xacb933d22bfaf2ebULL, 0xec9188399f4d02a0ULL, 0x56b15b13eaf77f58ULL, 0x84bc5f169442ab0aULL,
        0x98baf950de3436c2ULL, 0x6c4ea1ff185b54d6ULL, 0x4e5c4a981de51231ULL, 0xc16ff9eca6ac0813ULL,
        0x7eca8e15be98f449ULL, 0x4b0a529b4fa165beULL, 0x2d460ae5ab313683ULL, 0x3859390acb186af7ULL,
        0x22e40235b6db51e4ULL, 0x5df0f8e33dbf2de8ULL, 0xbed6cb921b7b758fULL, 0xb9a4d5e5b6a8ebb6ULL,
        0x9640745003bfb64aULL, 0xdf6569750eaa47bdULL, 0x0074f54dee31237dULL, 0x45069707e65682ebULL,
        0xec38580ffa4d9c27ULL, 0xdd4de97eedc93eefULL, 0xbd6c645396a6d824ULL, 0x522115426fadb7aaULL,
        0x669e316810217958ULL, 0xdbf8da2587d67244ULL, 0x4773f4e4d7bb4e4eULL, 0x5bd1416e255c3e9eULL,
        0xbbf75fd8cffeb194ULL, 0xb3026984df5d0cf8ULL, 0x372d3370302dffedULL, 0x6141b53104eb6243ULL,
        0xa04cf6e7682cec30ULL, 0xdb1b851bb6af6effULL, 0xfa62b243b296667eULL, 0x94fd3aca608078cdULL,
        0x2898afe1c3fbf419ULL, 0x38849664902cb2dfULL, 0x4ee2d503736895f2ULL, 0x055f3c9e707672d2ULL,
        0x0c3a725010e0da28ULL, 0x9f8384efcf346b39ULL, 0xef776d61a4887d2aULL, 0x783dee8ed604f954ULL,
        0x093e430013f69bb8ULL, 0x269ecc56c2308745ULL, 0x26e96d66e4751047ULL, 0xe876558219655cc3ULL
    },
    {
        0xbae39e8443e5e6e2ULL, 0x122a971851a1051bULL, 0x95858d7a0d331043ULL, 0x331efed262165f4cULL,
        0x88928b2bd68c2d3aULL, 0x444af4dc2950b135ULL, 0xea1f962fc6db770bULL, 0x43caa6ca58450d31ULL,
        0x8278c1cc412d74b2ULL, 0x9d9cca9f8d35c6a2ULL, 0x967ae50b5b41f99fULL, 0xc0eef490dc553864ULL,
        0x042735782bd2b4fdULL, 0x2eba5dc83e0a4a2bULL, 0x775670180e36e636ULL, 0x2070719ee07d208fULL,
        0x6cede6a505d1adb9ULL, 0x756114032e1da93bULL, 0xbe5da52cdfcb6b4aULL, 0xa35505d767d94714ULL,
        0xc23d1199aa3e62abULL, 0xd597ae99712e3e2dULL, 0xdc16a4cb754fe085ULL, 0x36826811c24f91ffULL,
        0x95e7d33649d6da32ULL, 0xd311c5e13ae6e6bbULL, 0xe2bb317c43fc7641ULL, 0x285bb6911079e7beULL,
        0x1023d6acb07866e4ULL, 0x03f59228e14eeacdULL, 0x7f1206ee125b93e7ULL, 0x61b50c7c569bdda2ULL,
        0x1d157e4c8d5de1a9ULL, 0xbceb94e869ed545aULL, 0xea5678656b934b39ULL, 0x3454065d5c66f3beULL,
        0x5aef3f9902bf2b28ULL, 0xa93131376f91db99ULL, 0xc886c0a29d56c945ULL, 0x6721cf3cabf9d5d3ULL,
        0xbfab6b1dda52086dULL, 0x72b0d968c3090846ULL, 0xea2c0633d854c01bULL, 0x44d71f47e0e20835ULL,
        0x1223e5412fec547dULL, 0x6e1f8aee6ca9d12dULL, 0xfff013507fb9e168ULL, 0x9f2f2eb4a1645b6fULL,
        0xec299c901f7a4906ULL, 0x6955c101334b69f9ULL, 0xd440bcd5eb4e3731ULL, 0x13de3b2174360102ULL,
        0x3a0513d2de442f6cULL, 0xbd7dff317fa31d91ULL, 0x461eb31c36500917ULL, 0xd3149aecba708aabULL,
        0xeaa67ec2c549c187ULL, 0x4c5fc5e34232ed63ULL, 0xbe87449eff1784e9ULL, 0x7d2eb98c6762019cULL,
        0x294a8c5351a3fe38ULL, 0x50bb6e54a12a9171ULL, 0xb74ab01166d73a14ULL, 0x91b9a77548888521ULL
    },
};
const U64 Position::whiteHashKey;
const U64 Position::castleHashKeys[16] = {
    0x659e2fb4bcff90a7ULL, 0x11ae0639334eee27ULL, 0x83e7533307ff2e6fULL, 0x60f5f716547d8cc3ULL,
    0xc4bfead179135b68ULL, 0x08f766a9a3a8de62ULL, 0xb684258554ee0537ULL, 0x4891d873f8e3e0feULL,
    0xf54572e5ac0b3a3cULL, 0x977a735beb8878cfULL, 0x4be958fbc1d86455ULL, 0x0d7de03cb6629efdULL,
    0x7b2c68ae5c970120ULL, 0x5ed392cce09f00e4ULL, 0x7da97f4ab6824d5bULL, 0xa23b535e2b4a30c3ULL
};
const U64 Position::epHashKeys[9] = {
    0xcbe9d4ab2a70eeccULL, 0x158896735f4e2fccULL, 0x83df96151a52fc84ULL, 0xa6a2844c5b1131c4ULL,
    0xe4e63b91dbcf8115ULL, 0xc83c4b75bc1a5b80ULL, 0x09f3cd287f693d64ULL, 0xa366a3a824cec2f0ULL,
    0x7e17190295ea7e8bULL
};
const U64 Position::moveCntKeys[101] = {
    0x65d4cb9fc6225bc2ULL, 0x29c5a0905e3c2736ULL, 0x68e90502619dbf85ULL, 0xc1c427d5896e405cULL,
    0xfa4afc8f8de8ecefULL, 0x8e5301dd532114e7ULL, 0x933156b65ee4a918ULL, 0x76fd40da0354fd37ULL,
    0x295d788895520e8aULL, 0x82d8f759909f819eULL, 0x6f8be0b29b366883ULL, 0x57192fcaf5eb578aULL,
    0x86a3f9fa0fd932f0ULL, 0x3a9712777a36e539ULL, 0x4e9a81b491b610d4ULL, 0x112ab256f111f7b3ULL,
    0xeb18a646723db641ULL, 0xf5df4391e176cfdfULL, 0x0988c3ae0a878c1eULL, 0x83eee968ca9994a3ULL,
    0x2f87a0054d15a108ULL, 0x8e8f6ad28a52fb62ULL, 0x4506387e508b536eULL, 0xf4be16fd24b9def7ULL,
    0xccdee07b69cfc3dcULL, 0x1d61f742a60333e8ULL, 0x2bf5dd9169aa7f01ULL, 0xc226cf16c071fa2eULL,
    0x817f04841559b8a3ULL, 0xf6af48567c6e78c2ULL, 0xcd804be16811e77bULL, 0x6b397f055a9882efULL,
    0x088fda249f941a3fULL, 0x6dbf67f07092c293ULL, 0x1645327035a2bd54ULL, 0xc39c205b04ec4e8dULL,
    0xc01c17f1dbd0fc6aULL, 0xa28db4ab9710fa3aULL, 0x7d90dcc1fdbb11c3ULL, 0x103546bdb065c73eULL,
    0x8141482edf322617ULL, 0x4a346c9c5f8d353bULL, 0xb383563ab40c28f6ULL, 0x02d49f9b654646e2ULL,
    0x40cf6a1d632f905cULL, 0xc79b313acfb4b5f7ULL, 0xef662c62dca07ef1ULL, 0xcef5d422f7659834ULL,
    0x8c73e6fb1b697f4eULL, 0x7231cadcef269eadULL, 0x9951b99f01fb70f1ULL, 0x536778b8e9f3b25bULL,
    0xa75da0d5cb39c1beULL, 0xabcaba5e733f76a3ULL, 0x5a3b1f3f07d061f6ULL, 0xb96c2bd8a2f04332ULL,
    0xb9205d72024398b9ULL, 0x7b3dee2a6799b3bdULL, 0xaa9c6adb7db1a38aULL, 0x08b635cdf28e5f5dULL,
    0xc915c83f2ee319ebULL, 0xcd207081bbf6d18fULL, 0xd4fef1db4cd1fb0dULL, 0x481eda802b30c94aULL,
    0x7b404bcd3fb72ca8ULL, 0xbedcddb701683d01ULL, 0x0d0e7d869fc2a5c9ULL, 0x9f32dc09ea332545ULL,
    0x932528e46eff6c95ULL, 0x3e4c75e674bd93fbULL, 0x8c8391e4a1861342ULL, 0xe7ae7559f3c3c3adULL,
    0x62c883f00d9e4d9fULL, 0x0bc550d471eac53eULL, 0xb6b944c871a8565dULL, 0x2a01cfb60b741f8eULL,
    0x49db94bdff8b73b5ULL, 0x80b474d2d8fd36f6ULL, 0xa6c75296fb9f028aULL, 0xfc0f8bbf3ad79375ULL,
    0xeaeb54285d08c14fULL, 0x0eef9a5fb0c3e2edULL, 0x54d588939fe544f6ULL, 0x17084509804201ffULL,
    0x31b8372478d93e4dULL, 0x5885e4f1e37edfceULL, 0x4e1bdff27c17069aULL, 0x08a253bd9b9cb273ULL,
    0x5435a8db7d2a894eULL, 0x975dafbadbaa58a0ULL, 0xfdc47ff3f5f9a594ULL, 0x4323e44de8a86bb3ULL,
    0x2be02fa2a15f8860ULL, 0xa0f6d6b4378d111eULL, 0x628b1647a8f34039ULL, 0xb1ae42641adc6944ULL,
    0xce22f3b15bbca65dULL, 0xff839edc88ee6833ULL, 0x9b944c8fbaffbe94ULL, 0x4dcf12e95dbf59d8ULL,
    0xd4e8006c8265fa6dULL
};
