/*
    Cinnamon is a UCI chess engine
    Copyright (C) 2011-2014 Giuseppe Cannella

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

#include "GenMoves.h"

GenMoves::GenMoves() : perftMode ( false ), listId ( -1 ) {
    currentPly = 0;
    gen_list = ( _TmoveP* ) calloc ( MAX_PLY, sizeof ( _TmoveP ) );
    assert ( gen_list );

    for ( int i = 0; i < MAX_PLY; i++ ) {
        gen_list[i].moveList = ( _Tmove* ) calloc ( MAX_MOVE, sizeof ( _Tmove ) );
        assert ( gen_list[i].moveList );
    }

    repetitionMap = ( u64* ) malloc ( sizeof ( u64 ) * MAX_REP_COUNT );
    assert ( repetitionMap );
    repetitionMapCount = 0;
}

template <uchar type>
bool GenMoves::pushmove ( const int from, const int to, const int side, int promotionPiece, int pieceFrom ) {
    ASSERT ( chessboard[KING_BLACK] );
    ASSERT ( chessboard[KING_WHITE] );
    int piece_captured = SQUARE_FREE;
    bool res = false;

    if ( ( ( type & 0x3 ) != ENPASSANT_MOVE_MASK ) && ! ( type & 0xc ) ) {
        piece_captured = side ? getPieceAt<BLACK> ( POW2[to] ) : getPieceAt<WHITE> ( POW2[to] );

        if ( piece_captured == KING_BLACK + ( side ^ 1 ) ) {
            res = true;
        }
    }
    else if ( ! ( type & 0xc ) ) { //no castle
        piece_captured = side ^ 1;
    }

    if ( ! ( type & 0xc ) && ( forceCheck || perftMode ) ) { //no castle
        if ( side == WHITE && inCheckPerft<WHITE> ( from, to, type, pieceFrom, piece_captured, promotionPiece ) ) {
            return false;
        }

        if ( side == BLACK && inCheckPerft<BLACK> ( from, to, type, pieceFrom, piece_captured, promotionPiece ) ) {
            return false;
        }
    }

    _Tmove * mos;
    ASSERT ( listId >= 0 ) ;
    ASSERT ( listId < MAX_PLY );
    ASSERT ( getListSize() < MAX_MOVE );
    mos = &gen_list[listId].moveList[getListSize()];
    ++gen_list[listId].size;
    mos->type = rightCastle | type;
    mos->side = ( char ) side;
    mos->capturedPiece = piece_captured;

    if ( type &  0x3 ) {
        mos->from = ( uchar ) from;
        mos->to = ( uchar ) to;
        mos->pieceFrom =  pieceFrom;
        mos->promotionPiece = ( char ) promotionPiece;

        if ( !perftMode ) {
            if ( res == true ) {
                mos->score = _INFINITE;
            }
            else {
                mos->score = killerHeuristic[from][to];
                mos->score += ( PIECES_VALUE[piece_captured] >= PIECES_VALUE[pieceFrom] ) ? ( PIECES_VALUE[piece_captured] - PIECES_VALUE[pieceFrom] ) * 2 :
                              PIECES_VALUE[piece_captured];
                ASSERT ( pieceFrom >= 0 && pieceFrom < 12 && to >= 0 && to < 64 && from >= 0 && from < 64 );
            }
        }
    }
    else if ( type & 0xc ) { //castle
        ASSERT ( rightCastle );
        mos->score = 100;
    }

    mos->used = false;

    ASSERT ( getListSize() < MAX_MOVE );
    return res;
}

bool GenMoves::performRankFileCapture ( const int piece, const u64 enemies, const int side, const u64 allpieces ) {
    ASSERT_RANGE ( piece, 0, 11 );
    ASSERT_RANGE ( side, 0, 1 );
    int bound;
    u64 x, q, x2 = chessboard[piece];

    while ( x2 ) {
        int position = BITScanForward ( x2 );
        x = allpieces & FILE_[position];

        if ( x & enemies ) {
            q = x & MASK_BIT_UNSET_UP[position];

            if ( q ) {
                bound = BITScanReverse ( q );

                if ( enemies & POW2[bound] ) {
                    if ( pushmove<STANDARD_MOVE_MASK> ( position, bound, side, NO_PROMOTION, piece ) ) { return true; }
                }
            }

            q = x & MASK_BIT_UNSET_DOWN[position];

            if ( q ) {
                bound = BITScanForward ( q );

                if ( enemies & POW2[bound] ) {
                    if ( pushmove<STANDARD_MOVE_MASK> ( position, bound, side, NO_PROMOTION, piece ) ) { return true; }
                }
            }
        }

        x = allpieces & RANK[position];

        if ( x & enemies ) {
            q = x & MASK_BIT_UNSET_RIGHT[position];

            if ( q ) {
                bound = BITScanForward ( q );

                if ( enemies & POW2[bound] ) {
                    if ( pushmove<STANDARD_MOVE_MASK> ( position, bound, side, NO_PROMOTION, piece ) ) { return true; }
                }
            }

            q = x & MASK_BIT_UNSET_LEFT[position];

            if ( q ) {
                bound = BITScanReverse ( q );

                if ( enemies & POW2[bound] ) {
                    if ( pushmove<STANDARD_MOVE_MASK> ( position, bound, side, NO_PROMOTION, piece ) ) { return true; }
                }
            }
        }

        x2 &= NOTPOW2[position];
    }

    return false;
}

int GenMoves::performRankFileCaptureCount ( const int position, const u64 enemies,  const u64 allpieces ) {
    ASSERT_RANGE ( position, 0, 63 );
    int count = 0;
    u64 q;
    u64 x = allpieces & FILE_[position];

    if ( x & enemies ) {
        q = x & MASK_BIT_UNSET_UP[position];

        if ( q && enemies & POW2[BITScanReverse ( q )] ) {
            count++;
        }

        q = x & MASK_BIT_UNSET_DOWN[position];

        if ( q && enemies & POW2[BITScanForward ( q )] ) {
            count++;
        }
    }

    x = allpieces & RANK[position];

    if ( x & enemies ) {
        q = x & MASK_BIT_UNSET_RIGHT[position];

        if ( q && enemies & POW2[BITScanForward ( q )] ) {
            count++;
        }

        q = x & MASK_BIT_UNSET_LEFT[position];

        if ( q && enemies & POW2[BITScanReverse ( q )] ) {
            count++;
        }
    }

    return count;
}

u64 GenMoves::performDiagCaptureCount ( const int position, const u64 allpieces ) {
    ASSERT_RANGE ( position, 0, 63 );
    u64 capturated = 0;
    u64 c = 0;
    ///LEFT
    u64 q = allpieces & MASK_BIT_UNSET_LEFT_UP[position];

    if ( q && ( ( c = POW2[BITScanReverse ( q )] ) ) ) {
        capturated |= c;
    }

    q = allpieces & MASK_BIT_UNSET_LEFT_DOWN[position];

    if ( q && ( ( c = POW2[BITScanForward ( q )] ) ) ) {
        capturated |= c;
    }

    ///RIGHT
    q = allpieces & MASK_BIT_UNSET_RIGHT_UP[position];

    if ( q && ( ( c = POW2[BITScanReverse ( q )] ) ) ) {
        capturated |= c;
    }

    q = allpieces & MASK_BIT_UNSET_RIGHT_DOWN[position];

    if ( q && ( ( c = POW2[BITScanForward ( q )] ) ) ) {
        capturated |= c;
    }

    ///
    return capturated;
}

bool GenMoves::performDiagCapture ( const int piece, const u64 enemies, const int side, const u64 allpieces ) {
    ASSERT_RANGE ( piece, 0, 11 );
    ASSERT_RANGE ( side, 0, 1 );
    int bound;
    u64 x2 = chessboard[piece];

    while ( x2 ) {
        int position = BITScanForward ( x2 );
        ///LEFT
        u64 q = allpieces & MASK_BIT_UNSET_LEFT_UP[position];

        if ( q ) {
            bound = BITScanReverse ( q );

            if ( enemies & POW2[bound] ) {
                if ( pushmove<STANDARD_MOVE_MASK> ( position, bound, side, NO_PROMOTION, piece ) ) { return true; }
            }
        }

        q = allpieces & MASK_BIT_UNSET_LEFT_DOWN[position];

        if ( q ) {
            bound = BITScanForward ( q );

            if ( enemies & POW2[bound] ) {
                if ( pushmove<STANDARD_MOVE_MASK> ( position, bound, side, NO_PROMOTION, piece ) ) { return true; }
            }
        }

        ///RIGHT
        q = allpieces & MASK_BIT_UNSET_RIGHT_UP[position];
        if ( q ) {
            bound = BITScanReverse ( q );

            if ( enemies & POW2[bound] ) {
                if ( pushmove<STANDARD_MOVE_MASK> ( position, bound, side, NO_PROMOTION, piece ) ) { return true; }
            }
        }

        q = allpieces & MASK_BIT_UNSET_RIGHT_DOWN[position];

        if ( q ) {
            bound = BITScanForward ( q );

            if ( enemies & POW2[bound] ) {
                if ( pushmove<STANDARD_MOVE_MASK> ( position, bound, side, NO_PROMOTION, piece ) ) { return true; }
            }
        }

        ///
        x2 &= NOTPOW2[position];
    }

    return false;
}

int GenMoves::performRankFileShiftCount ( const int position, const u64 allpieces ) {
    ASSERT_RANGE ( position, 0, 63 );
    int count = 0;
    ///FILE
    ASSERT ( RANK_AT[position] == MASK_BIT_SET_COUNT[position][VERT_LOWER[position]] );
    ASSERT ( MASK_BIT_SET_COUNT_VERT_UPPER[position] == MASK_BIT_SET_COUNT[position][VERT_UPPER[position]] );
    ASSERT ( MASK_BIT_SET_COUNT_ORIZ_LEFT[position] == MASK_BIT_SET_COUNT[position][ORIZ_LEFT[position]] );
    ASSERT ( FILE_AT[position] == MASK_BIT_SET_COUNT[position][ORIZ_RIGHT[position]] );
    u64 q = allpieces &  MASK_BIT_UNSET_UP[position];
    count += q ? MASK_BIT_SET_NOBOUND_COUNT[position][BITScanReverse ( q )] : RANK_AT[position];
    q = allpieces & MASK_BIT_UNSET_DOWN[position];
    count +=  q ? MASK_BIT_SET_NOBOUND_COUNT[position][BITScanForward ( q )] : MASK_BIT_SET_COUNT_VERT_UPPER[position];
    ///RANK
    q = allpieces & MASK_BIT_UNSET_RIGHT[position];
    count +=  q ? MASK_BIT_SET_NOBOUND_COUNT[position][BITScanForward ( q )] : MASK_BIT_SET_COUNT_ORIZ_LEFT[position];
    q = allpieces & MASK_BIT_UNSET_LEFT[position];
    count +=  q ? MASK_BIT_SET_NOBOUND_COUNT[position][BITScanReverse ( q )] : FILE_AT[position];
    return count;
}

void GenMoves::performRankFileShift ( const int piece, const int side, const u64 allpieces ) {
    ASSERT_RANGE ( piece, 0, 11 );
    ASSERT_RANGE ( side, 0, 1 );
    u64 x2 = chessboard[piece];

    while ( x2 ) {
        int position = BITScanForward ( x2 );
        ///FILE
        u64 q = allpieces &  MASK_BIT_UNSET_UP[position];
        u64 k = q ? MASK_BIT_SET_NOBOUND[position][BITScanReverse ( q )] : MASK_BIT_SET_VERT_LOWER[position];
        q = allpieces & MASK_BIT_UNSET_DOWN[position];
        k |= q ? MASK_BIT_SET_NOBOUND[position][BITScanForward ( q )] : MASK_BIT_SET_VERT_UPPER[position];
        ///RANK
        q = allpieces & MASK_BIT_UNSET_RIGHT[position];
        k |= q ? MASK_BIT_SET_NOBOUND[position][BITScanForward ( q )] : MASK_BIT_SET_ORIZ_LEFT[position];
        q = allpieces & MASK_BIT_UNSET_LEFT[position];
        k |= q ? MASK_BIT_SET_NOBOUND[position][BITScanReverse ( q )] : MASK_BIT_SET_ORIZ_RIGHT[position];
        ///
        int n;

        while ( k ) {
            n = BITScanForward ( k );
            pushmove<STANDARD_MOVE_MASK> ( position, n, side, NO_PROMOTION, piece );
            k &= NOTPOW2[n];
        }

        x2 &= NOTPOW2[position];
    }
}

int GenMoves::performDiagShiftCount ( const int position, const u64 allpieces ) {
    ASSERT_RANGE ( position, 0, 63 );
    ///LEFT
    ASSERT ( MASK_BIT_SET_COUNT_LEFT_LOWER[position] == MASK_BIT_SET_COUNT[position][LEFT_LOWER[position]] );
    ASSERT ( MASK_BIT_SET_COUNT_LEFT_UPPER[position] == MASK_BIT_SET_COUNT[position][LEFT_UPPER[position]] );
    ASSERT ( MASK_BIT_SET_COUNT_RIGHT_LOWER[position] == MASK_BIT_SET_COUNT[position][RIGHT_LOWER[position]] );
    ASSERT ( MASK_BIT_SET_COUNT_RIGHT_UPPER[position] == MASK_BIT_SET_COUNT[position][RIGHT_UPPER[position]] );
    u64 q = allpieces &  MASK_BIT_UNSET_LEFT_UP[position];
    int count = q ? MASK_BIT_SET_NOBOUND_COUNT[position][BITScanReverse ( q )] : MASK_BIT_SET_COUNT_LEFT_LOWER[position];
    q = allpieces & MASK_BIT_UNSET_LEFT_DOWN[position];
    count += q ? MASK_BIT_SET_NOBOUND_COUNT[position][BITScanForward ( q )] : MASK_BIT_SET_COUNT_LEFT_UPPER[position];
    ///RIGHT
    q = allpieces & MASK_BIT_UNSET_RIGHT_UP[position];
    count += q ? MASK_BIT_SET_NOBOUND_COUNT[position][BITScanReverse ( q )] : MASK_BIT_SET_COUNT_RIGHT_LOWER[position];
    q = allpieces & MASK_BIT_UNSET_RIGHT_DOWN[position];
    count += q ? MASK_BIT_SET_NOBOUND_COUNT[position][BITScanForward ( q )] : MASK_BIT_SET_COUNT_RIGHT_UPPER[position];
    return count;
}

void GenMoves::performDiagShift ( const int piece, const int side, const u64 allpieces ) {
    ASSERT_RANGE ( piece, 0, 11 );
    ASSERT_RANGE ( side, 0, 1 );
    u64 x2 = chessboard[piece];

    while ( x2 ) {
        int position = BITScanForward ( x2 );
        ///LEFT
        u64 q = allpieces &  MASK_BIT_UNSET_LEFT_UP[position];
        u64 k = q ? MASK_BIT_SET_NOBOUND[position][BITScanReverse ( q )] : MASK_BIT_SET_LEFT_LOWER[position];
        q = allpieces & MASK_BIT_UNSET_LEFT_DOWN[position];
        k |= q ? MASK_BIT_SET_NOBOUND[position][BITScanForward ( q )] : MASK_BIT_SET_LEFT_UPPER[position];
        ///RIGHT
        q = allpieces & MASK_BIT_UNSET_RIGHT_UP[position];
        k |= q ? MASK_BIT_SET_NOBOUND[position][BITScanReverse ( q )] : MASK_BIT_SET_RIGHT_LOWER[position];
        q = allpieces & MASK_BIT_UNSET_RIGHT_DOWN[position];
        k |= q ? MASK_BIT_SET_NOBOUND[position][BITScanForward ( q )] : MASK_BIT_SET_RIGHT_UPPER[position];
        ///
        int n;

        while ( k ) {
            n = BITScanForward ( k );
            pushmove<STANDARD_MOVE_MASK> ( position, n, side, NO_PROMOTION, piece );
            k &= NOTPOW2[n];
        }

        x2 &= NOTPOW2[position];
    }
}

void GenMoves::generateMoves ( const int side, const u64 allpieces ) {
    ASSERT_RANGE ( side, 0, 1 );
    side ? generateMoves<WHITE> ( allpieces ) : generateMoves<BLACK> ( allpieces );
}

bool GenMoves::generateCaptures ( const int side, const u64 enemies, const u64 friends ) {
    ASSERT_RANGE ( side, 0, 1 );
    return side ? generateCaptures<WHITE> ( enemies, friends ) : generateCaptures<BLACK> ( enemies, friends );
}

int GenMoves::getMobilityPawns ( const int side, const int ep, const u64 ped_friends, const u64 enemies, const u64 xallpieces ) {
    ASSERT_RANGE ( side, 0, 1 );
    return
        ep == NO_ENPASSANT ? 0 : bitCount ( ENPASSANT_MASK[side ^ 1][ep] & chessboard[side] )
        +
        side == WHITE ?
        bitCount ( ( ped_friends << 8 ) & xallpieces ) +
        bitCount ( ( ( ( ( ped_friends & TABJUMPPAWN ) << 8 ) & xallpieces ) << 8 ) & xallpieces ) +
        bitCount ( ( chessboard[side] << 7 ) & TABCAPTUREPAWN_LEFT & enemies ) +
        bitCount ( ( chessboard[side] << 9 ) &TABCAPTUREPAWN_RIGHT & enemies )
        :
        bitCount ( ( ped_friends >> 8 ) &xallpieces ) +
        bitCount ( ( ( ( ( ped_friends & TABJUMPPAWN ) >> 8 ) & xallpieces ) >> 8 ) &xallpieces ) +
        bitCount ( ( chessboard[side] >> 7 ) & TABCAPTUREPAWN_RIGHT & enemies ) +
        bitCount ( ( chessboard[side] >> 9 ) &TABCAPTUREPAWN_LEFT & enemies );
}

int GenMoves::getMobilityQueen ( const int position, const u64 enemies, const u64 friends ) {
    ASSERT_RANGE ( position, 0, 63 );
    return   performRankFileCaptureCount ( position, enemies, enemies | friends ) +
             bitCount ( enemies & performDiagCaptureCount ( position, enemies | friends ) ) + performRankFileShiftCount ( position, enemies | friends ) +
             performDiagShiftCount ( position, enemies | friends );
}

int GenMoves::getMobilityRook ( const int position, const u64 enemies, const u64 friends ) {
    ASSERT_RANGE ( position, 0, 63 );
    return   performRankFileCaptureCount ( position, enemies,  enemies | friends ) + performRankFileShiftCount ( position,  enemies | friends );
}


void GenMoves::setPerft ( const bool b ) {
    perftMode = b;
}

void GenMoves::clearKillerHeuristic() {
    memset ( killerHeuristic, 0, sizeof ( killerHeuristic ) );
}

_Tmove* GenMoves::getNextMove ( _TmoveP* list ) {
    _Tmove* gen_list1 = list->moveList;
    int listcount = list->size;
    int bestId = -1;
    int j, bestScore;

    for ( j = 0; j < listcount; j++ ) {
        if ( !gen_list1[j].used ) {
            bestId = j;
            bestScore = gen_list1[bestId].score;
            break;
        }
    }

    if ( bestId == -1 ) {
        return nullptr;
    }

    for ( int i = j + 1; i < listcount; i++ ) {
        if ( !gen_list1[i].used && gen_list1[i].score > bestScore ) {
            bestId = i;
            bestScore = gen_list1[bestId].score;
        }
    }

    gen_list1[bestId].used = true;
    return &gen_list1[bestId];
}

GenMoves::~GenMoves() {
    for ( int i = 0; i < MAX_PLY; i++ ) {
        free ( gen_list[i].moveList );
    }

    free ( gen_list );
    free ( repetitionMap );
}

void GenMoves::performCastle ( const int side, const uchar type ) {
    ASSERT_RANGE ( side, 0, 1 );

    if ( side == WHITE ) {
        if ( type & KING_SIDE_CASTLE_MOVE_MASK ) {
            ASSERT ( getPieceAt ( side, POW2_3 ) == KING_WHITE );
            ASSERT ( getPieceAt ( side, POW2_1 ) == SQUARE_FREE );
            ASSERT ( getPieceAt ( side, POW2_2 ) == SQUARE_FREE );
            ASSERT ( getPieceAt ( side, POW2_0 ) == ROOK_WHITE );
            updateZobristKey ( KING_WHITE, 3 );
            updateZobristKey ( KING_WHITE, 1 );
            chessboard[KING_WHITE] = ( chessboard[KING_WHITE] | POW2_1 ) & NOTPOW2_3;
            updateZobristKey ( ROOK_WHITE, 2 );
            updateZobristKey ( ROOK_WHITE, 0 );
            chessboard[ROOK_WHITE] = ( chessboard[ROOK_WHITE] | POW2_2 ) & NOTPOW2_0;
        }
        else {
            ASSERT ( type & QUEEN_SIDE_CASTLE_MOVE_MASK );
            ASSERT ( getPieceAt ( side, POW2_3 ) == KING_WHITE );
            ASSERT ( getPieceAt ( side, POW2_4 ) == SQUARE_FREE );
            ASSERT ( getPieceAt ( side, POW2_5 ) == SQUARE_FREE );
            ASSERT ( getPieceAt ( side, POW2_6 ) == SQUARE_FREE );
            ASSERT ( getPieceAt ( side, POW2_7 ) == ROOK_WHITE );
            chessboard[KING_WHITE] = ( chessboard[KING_WHITE] | POW2_5 ) & NOTPOW2_3;
            updateZobristKey ( KING_WHITE, 5 );
            updateZobristKey ( KING_WHITE, 3 );
            chessboard[ROOK_WHITE] = ( chessboard[ROOK_WHITE] | POW2_4 ) & NOTPOW2_7;
            updateZobristKey ( ROOK_WHITE, 4 );
            updateZobristKey ( ROOK_WHITE, 7 );
        }
    }
    else {
        if ( type & KING_SIDE_CASTLE_MOVE_MASK ) {
            ASSERT ( getPieceAt ( side, POW2_59 ) == KING_BLACK );
            ASSERT ( getPieceAt ( side, POW2_58 ) == SQUARE_FREE );
            ASSERT ( getPieceAt ( side, POW2_57 ) == SQUARE_FREE );
            ASSERT ( getPieceAt ( side, POW2_56 ) == ROOK_BLACK );
            chessboard[KING_BLACK] = ( chessboard[KING_BLACK] | POW2_57 ) & NOTPOW2_59;
            updateZobristKey ( KING_BLACK, 57 );
            updateZobristKey ( KING_BLACK, 59 );
            chessboard[ROOK_BLACK] = ( chessboard[ROOK_BLACK] | POW2_58 ) & NOTPOW2_56;
            updateZobristKey ( ROOK_BLACK, 58 );
            updateZobristKey ( ROOK_BLACK, 56 );
        }
        else {
            ASSERT ( type & QUEEN_SIDE_CASTLE_MOVE_MASK );
            ASSERT ( getPieceAt ( side, POW2_59 ) == KING_BLACK );
            ASSERT ( getPieceAt ( side, POW2_60 ) == SQUARE_FREE );
            ASSERT ( getPieceAt ( side, POW2_61 ) == SQUARE_FREE );
            ASSERT ( getPieceAt ( side, POW2_62 ) == SQUARE_FREE );
            ASSERT ( getPieceAt ( side, POW2_63 ) == ROOK_BLACK );
            chessboard[KING_BLACK] = ( chessboard[KING_BLACK] | POW2_61 ) & NOTPOW2_59;
            updateZobristKey ( KING_BLACK, 61 );
            updateZobristKey ( KING_BLACK, 59 );
            chessboard[ROOK_BLACK] = ( chessboard[ROOK_BLACK] | POW2_60 ) & NOTPOW2_63;
            updateZobristKey ( ROOK_BLACK, 60 );
            updateZobristKey ( ROOK_BLACK, 63 );
        }
    }
}

int GenMoves::getMobilityCastle ( const int side, const u64 allpieces ) {
    ASSERT_RANGE ( side, 0, 1 );
    int count = 0;

    if ( side == WHITE ) {
        if ( POW2_3 & chessboard[KING_WHITE] && ! ( allpieces & 0x6ULL ) && rightCastle & RIGHT_KING_CASTLE_WHITE_MASK
                && chessboard[ROOK_WHITE] & POW2_0 && !attackSquare<WHITE> ( 1, allpieces ) && !attackSquare<WHITE> ( 2, allpieces ) && !attackSquare<WHITE> ( 3, allpieces ) ) {
            count++;
        }

        if ( POW2_3 & chessboard[KING_WHITE] && ! ( allpieces & 0x70ULL ) && rightCastle & RIGHT_QUEEN_CASTLE_WHITE_MASK
                && chessboard[ROOK_WHITE] & POW2_7 && !attackSquare<WHITE> ( 3, allpieces ) && !attackSquare<WHITE> ( 4, allpieces ) && !attackSquare<WHITE> ( 5, allpieces ) ) {
            count++;
        }
    }
    else {
        if ( POW2_59 & chessboard[KING_BLACK] && rightCastle & RIGHT_KING_CASTLE_BLACK_MASK && ! ( allpieces & 0x600000000000000ULL )
                && chessboard[ROOK_BLACK] & POW2_56 && !attackSquare<BLACK> ( 57, allpieces ) && !attackSquare<BLACK> ( 58, allpieces ) && !attackSquare<BLACK> ( 59, allpieces ) ) {
            count++;
        }

        if ( POW2_59 & chessboard[KING_BLACK] && rightCastle & RIGHT_QUEEN_CASTLE_BLACK_MASK && ! ( allpieces & 0x7000000000000000ULL )
                && chessboard[ROOK_BLACK] & POW2_63 && !attackSquare<BLACK> ( 59, allpieces ) && !attackSquare<BLACK> ( 60, allpieces ) && !attackSquare<BLACK> ( 61, allpieces ) ) {
            count++;
        }
    }

    return count;
}

void GenMoves::tryAllCastle ( const int side, const u64 allpieces ) {
    ASSERT_RANGE ( side, 0, 1 );

    if ( side == WHITE ) {
        if ( POW2_3 & chessboard[KING_WHITE] && ! ( allpieces & 0x6ULL ) && rightCastle & RIGHT_KING_CASTLE_WHITE_MASK
                && chessboard[ROOK_WHITE] & POW2_0 && !attackSquare<WHITE> ( 1, allpieces ) && !attackSquare<WHITE> ( 2, allpieces ) && !attackSquare<WHITE> ( 3, allpieces ) ) {
            pushmove<KING_SIDE_CASTLE_MOVE_MASK> ( -1, -1, WHITE, NO_PROMOTION, -1 );
        }

        if ( POW2_3 & chessboard[KING_WHITE] && ! ( allpieces & 0x70ULL ) && rightCastle & RIGHT_QUEEN_CASTLE_WHITE_MASK
                && chessboard[ROOK_WHITE] & POW2_7 && !attackSquare<WHITE> ( 3, allpieces ) && !attackSquare<WHITE> ( 4, allpieces ) && !attackSquare<WHITE> ( 5, allpieces ) ) {
            pushmove<QUEEN_SIDE_CASTLE_MOVE_MASK> ( -1, -1, WHITE, NO_PROMOTION, -1 );
        }
    }
    else {
        if ( POW2_59 & chessboard[KING_BLACK] && rightCastle & RIGHT_KING_CASTLE_BLACK_MASK && ! ( allpieces & 0x600000000000000ULL )
                && chessboard[ROOK_BLACK] & POW2_56 && !attackSquare<BLACK> ( 57, allpieces ) && !attackSquare<BLACK> ( 58, allpieces ) && !attackSquare<BLACK> ( 59, allpieces ) ) {
            pushmove<KING_SIDE_CASTLE_MOVE_MASK> ( -1, -1, BLACK, NO_PROMOTION, -1 );
        }

        if ( POW2_59 & chessboard[KING_BLACK] && rightCastle & RIGHT_QUEEN_CASTLE_BLACK_MASK && ! ( allpieces & 0x7000000000000000ULL )
                && chessboard[ROOK_BLACK] & POW2_63 && !attackSquare<BLACK> ( 59, allpieces ) && !attackSquare<BLACK> ( 60, allpieces ) && !attackSquare<BLACK> ( 61, allpieces ) ) {
            pushmove<QUEEN_SIDE_CASTLE_MOVE_MASK> ( -1, -1, BLACK, NO_PROMOTION, -1 );
        }
    }
}

bool GenMoves::performKnightShiftCapture ( const int piece, const u64 enemies, const int side ) {
    ASSERT_RANGE ( piece, 0, 11 );
    ASSERT_RANGE ( side, 0, 1 );
    u64 x = chessboard[piece];

    while ( x ) {
        int pos = BITScanForward ( x );
        u64 x1 = enemies & KNIGHT_MASK[pos];

        while ( x1 ) {
            int o = BITScanForward ( x1 );

            if ( pushmove<STANDARD_MOVE_MASK> ( pos, o, side, NO_PROMOTION, piece ) ) {
                return true;
            }

            x1 &= NOTPOW2[o];
        };

        x &= NOTPOW2[pos];
    }

    return false;
}

bool GenMoves::performKingShiftCapture ( int side , const u64 enemies ) {
    ASSERT_RANGE ( side, 0, 1 );
    int pos = BITScanForward ( chessboard[KING_BLACK + side] );
    ASSERT ( pos != -1 );
    u64 x1 = enemies & NEAR_MASK1[pos];

    while ( x1 ) {
        int o = BITScanForward ( x1 );

        if ( pushmove<STANDARD_MOVE_MASK> ( pos, o, side, NO_PROMOTION, KING_BLACK + side ) ) {
            return true;
        }

        x1 &= NOTPOW2[o];
    };

    return false;
}

template <int side>
void GenMoves::checkJumpPawn ( u64 x, const u64 xallpieces ) {
    x &= TABJUMPPAWN;

    if ( side ) {
        x = ( ( ( x << 8 ) &xallpieces ) << 8 ) &xallpieces;
    }
    else {
        x = ( ( ( x >> 8 ) &xallpieces ) >> 8 ) &xallpieces;
    };

    while ( x ) {
        int o = BITScanForward ( x );
        pushmove<STANDARD_MOVE_MASK> ( o + ( side ? -16 : 16 ), o, side, NO_PROMOTION, side );
        x &= NOTPOW2[o];
    };
}

template <int side>
void GenMoves::performPawnShift ( const u64 xallpieces ) {
    int tt;
    u64 x = chessboard[side];

    if ( x & PAWNS_JUMP[side] ) {
        checkJumpPawn<side> ( x, xallpieces );
    }

    if ( side ) {
        x <<= 8;
        tt = -8;
    }
    else {
        tt = 8;
        x >>= 8;
    };

    x &= xallpieces;

    while ( x ) {
        int o = BITScanForward ( x );
        ASSERT ( getPieceAt ( side, POW2[o + tt] ) != SQUARE_FREE );
        ASSERT ( getBitBoard ( side ) & POW2[o + tt] );

        if ( o > 55 || o < 8 ) {
            pushmove<PROMOTION_MOVE_MASK> ( o + tt, o, side, QUEEN_BLACK + side, side );

            if ( perftMode ) {
                pushmove<PROMOTION_MOVE_MASK> ( o + tt, o, side, KNIGHT_BLACK + side, side );
                pushmove<PROMOTION_MOVE_MASK> ( o + tt, o, side, BISHOP_BLACK + side, side );
                pushmove<PROMOTION_MOVE_MASK> ( o + tt, o, side, ROOK_BLACK + side, side );
            }
        }
        else {
            pushmove<STANDARD_MOVE_MASK> ( o + tt, o, side, NO_PROMOTION, side );
        }

        x &= NOTPOW2[o];
    };
}

template <int side>
bool GenMoves::performPawnCapture ( const u64 enemies ) {
    if ( !chessboard[side] ) {
        if ( enpassantPosition != NO_ENPASSANT ) {
            updateZobristKey ( 13, enpassantPosition );
        }
        enpassantPosition = NO_ENPASSANT;
        return false;
    }

    int GG;
    u64 x;

    if ( side ) {
        x = ( chessboard[side] << 7 ) & TABCAPTUREPAWN_LEFT & enemies;
        GG = -7;
    }
    else {
        x = ( chessboard[side] >> 7 ) & TABCAPTUREPAWN_RIGHT & enemies;
        GG = 7;
    };

    while ( x ) {
        int o = BITScanForward ( x );

        if ( ( side && o > 55 ) || ( !side && o < 8 ) ) { //PROMOTION
            if ( pushmove<PROMOTION_MOVE_MASK> ( o + GG, o, side, QUEEN_BLACK + side, side ) ) {
                return true;
            }

            if ( perftMode ) {
                if ( pushmove<PROMOTION_MOVE_MASK> ( o + GG, o, side, KNIGHT_BLACK + side, side ) ) {
                    return true;
                }

                if ( pushmove<PROMOTION_MOVE_MASK> ( o + GG, o, side, ROOK_BLACK + side, side ) ) {
                    return true;
                }

                if ( pushmove<PROMOTION_MOVE_MASK> ( o + GG, o, side, BISHOP_BLACK + side, side ) ) {
                    return true;
                }
            }
        }
        else if ( pushmove<STANDARD_MOVE_MASK> ( o + GG, o, side, NO_PROMOTION, side ) ) {
            return true;
        }

        x &= NOTPOW2[o];
    };

    if ( side ) {
        GG = -9;
        x = ( chessboard[side] << 9 ) &TABCAPTUREPAWN_RIGHT & enemies;
    }
    else {
        GG = 9;
        x = ( chessboard[side] >> 9 ) &TABCAPTUREPAWN_LEFT & enemies;
    };

    while ( x ) {
        int o = BITScanForward ( x );

        if ( ( side && o > 55 ) || ( !side && o < 8 ) ) { //PROMOTION
            if ( pushmove<PROMOTION_MOVE_MASK> ( o + GG, o, side, QUEEN_BLACK + side, side ) ) {
                return true;
            }

            if ( perftMode ) {
                if ( pushmove<PROMOTION_MOVE_MASK> ( o + GG, o, side, KNIGHT_BLACK + side, side ) ) {
                    return true;
                }

                if ( pushmove<PROMOTION_MOVE_MASK> ( o + GG, o, side, BISHOP_BLACK + side, side ) ) {
                    return true;
                }

                if ( pushmove<PROMOTION_MOVE_MASK> ( o + GG, o, side, ROOK_BLACK + side, side ) ) {
                    return true;
                }
            }
        }
        else if ( pushmove<STANDARD_MOVE_MASK> ( o + GG, o, side, NO_PROMOTION, side ) ) {
            return true;
        }

        x &= NOTPOW2[o];
    };

    //ENPASSANT
    if ( enpassantPosition != NO_ENPASSANT ) {
        x = ENPASSANT_MASK[side ^ 1][enpassantPosition] & chessboard[side];

        while ( x ) {
            int o = BITScanForward ( x );
            pushmove<ENPASSANT_MOVE_MASK> ( o, ( side ? enpassantPosition + 8 : enpassantPosition - 8 ), side, NO_PROMOTION, side );
            x &= NOTPOW2[o];
        }

        updateZobristKey ( 13, enpassantPosition );
        enpassantPosition = NO_ENPASSANT;
    }

    return false;
}

u64 GenMoves::getKingAttackers ( const int xside, u64 allpieces, int position ) {
    ASSERT_RANGE ( position, 0, 63 );
    ASSERT_RANGE ( xside, 0, 1 );
    int bound;
    u64 attackers = KNIGHT_MASK[position] & chessboard[KNIGHT_BLACK + xside];
    ///king
    attackers |= NEAR_MASK1[position] & chessboard[KING_BLACK + xside];
    ///pawn
    attackers |= PAWN_FORK_MASK[xside ^ 1][position] & chessboard[PAWN_BLACK + xside];
    ///bishop queen
    u64 enemies = chessboard[BISHOP_BLACK + xside] | chessboard[QUEEN_BLACK + xside];

    if ( LEFT_RIGHT_DIAG[position] & enemies ) {
        ///LEFT
        u64 q = allpieces & MASK_BIT_UNSET_LEFT_UP[position];

        if ( q ) {
            bound = BITScanReverse ( q );

            if ( enemies & POW2[bound] ) {
                attackers |= POW2[bound];
            }
        }

        q = allpieces & MASK_BIT_UNSET_LEFT_DOWN[position];

        if ( q ) {
            bound = BITScanForward ( q );

            if ( enemies & POW2[bound] ) {
                attackers |= POW2[bound];
            }
        }

        ///RIGHT
        q = allpieces & MASK_BIT_UNSET_RIGHT_UP[position];

        if ( q ) {
            bound = BITScanReverse ( q );

            if ( enemies & POW2[bound] ) {
                attackers |= POW2[bound];
            }
        }

        q = allpieces & MASK_BIT_UNSET_RIGHT_DOWN[position];

        if ( q ) {
            bound = BITScanForward ( q );

            if ( enemies & POW2[bound] ) {
                attackers |= POW2[bound];
            }
        }
    }

    enemies = chessboard[ROOK_BLACK + xside] | chessboard[QUEEN_BLACK + xside];
    u64 q;
    ///rook queen
    u64 x = allpieces & FILE_[position];

    if ( x & enemies ) {
        q = x & MASK_BIT_UNSET_UP[position];

        if ( q ) {
            bound = BITScanReverse ( q );

            if ( enemies & POW2[bound] ) {
                attackers |= POW2[bound];
            }
        }

        q = x & MASK_BIT_UNSET_DOWN[position];

        if ( q ) {
            bound = BITScanForward ( q );

            if ( enemies & POW2[bound] ) {
                attackers |= POW2[bound];
            }
        }
    }

    x = allpieces & RANK[position];

    if ( x & enemies ) {
        q = x & MASK_BIT_UNSET_RIGHT[position];

        if ( q ) {
            bound = BITScanForward ( q );

            if ( enemies & POW2[bound] ) {
                attackers |= POW2[bound];
            }
        }

        q = x & MASK_BIT_UNSET_LEFT[position];

        if ( q ) {
            bound = BITScanReverse ( q );

            if ( enemies & POW2[bound] ) {
                attackers |= POW2[bound];
            }
        }
    }

    return attackers;
}


template <int side>
bool GenMoves::attackSquare ( const uchar position, u64 allpieces ) {
    ASSERT_RANGE ( position, 0, 63 );
    ASSERT_RANGE ( side, 0, 1 );

    if ( KNIGHT_MASK[position] & chessboard[KNIGHT_BLACK + ( side ^ 1 )] ) {
        return true;
    }

    if ( NEAR_MASK1[position] & chessboard[KING_BLACK + ( side ^ 1 )] ) {
        return true;
    }

    //enpassant
    if ( PAWN_FORK_MASK[side][position] & chessboard[PAWN_BLACK + ( side ^ 1 )] ) {
        return true;
    }

    allpieces |= POW2[position];
    u64 enemies = chessboard[QUEEN_BLACK + ( side ^ 1 )] | chessboard[BISHOP_BLACK + ( side ^ 1 )];

    if ( LEFT_RIGHT_DIAG[position] & enemies ) {
        ///LEFT
        u64 q = allpieces & MASK_BIT_UNSET_LEFT_UP[position];

        if ( q && enemies & POW2[BITScanReverse ( q )] ) {
            return true;
        }

        q = allpieces & MASK_BIT_UNSET_LEFT_DOWN[position];

        if ( q && enemies & POW2[BITScanForward ( q )] ) {
            return true;
        }

        ///RIGHT
        q = allpieces & MASK_BIT_UNSET_RIGHT_UP[position];

        if ( q && enemies & POW2[BITScanReverse ( q )] ) {
            return true;
        }

        q = allpieces & MASK_BIT_UNSET_RIGHT_DOWN[position];

        if ( q && enemies & POW2[BITScanForward ( q )] ) {
            return true;
        }
    }

    ///
    u64 x = allpieces & FILE_[position];
    enemies = chessboard[QUEEN_BLACK + ( side ^ 1 )] | chessboard[ROOK_BLACK + ( side ^ 1 )];

    if ( x & enemies ) {
        u64 q = x & MASK_BIT_UNSET_UP[position];

        if ( q && enemies & POW2[BITScanReverse ( q )] ) {
            return true;
        }

        q = x & MASK_BIT_UNSET_DOWN[position];

        if ( q && enemies & POW2[BITScanForward ( q )] ) {
            return true;
        }
    }

    x = allpieces & RANK[position];

    if ( x & enemies ) {
        u64 q = x & MASK_BIT_UNSET_RIGHT[position];

        if ( q && enemies & POW2[BITScanForward ( q )] ) {
            return true;
        }

        q = x & MASK_BIT_UNSET_LEFT[position];

        if ( q && enemies & POW2[BITScanReverse ( q )] ) {
            return true;
        }
    }

    return false;
}

void GenMoves::unPerformCastle ( const int side, const uchar type ) {
    ASSERT_RANGE ( side, 0, 1 );

    if ( side == WHITE ) {
        if ( type & KING_SIDE_CASTLE_MOVE_MASK ) {
            ASSERT ( getPieceAt ( side, POW2_1 ) == KING_WHITE );
            ASSERT ( getPieceAt ( side, POW2_0 ) == 12 );
            ASSERT ( getPieceAt ( side, POW2_3 ) == 12 );
            ASSERT ( getPieceAt ( side, POW2_2 ) == ROOK_WHITE );
            chessboard[KING_WHITE] = ( chessboard[KING_WHITE] | POW2_3 ) & NOTPOW2_1;
            chessboard[ROOK_WHITE] = ( chessboard[ROOK_WHITE] | POW2_0 ) & NOTPOW2_2;
        }
        else {
            chessboard[KING_WHITE] = ( chessboard[KING_WHITE] | POW2_3 ) & NOTPOW2_5;
            chessboard[ROOK_WHITE] = ( chessboard[ROOK_WHITE] | POW2_7 ) & NOTPOW2_4;
        }
    }
    else {
        if ( type & KING_SIDE_CASTLE_MOVE_MASK ) {
            chessboard[KING_BLACK] = ( chessboard[KING_BLACK] | POW2_59 ) & NOTPOW2_57;
            chessboard[ROOK_BLACK] = ( chessboard[ROOK_BLACK] | POW2_56 ) & NOTPOW2_58;
        }
        else {
            chessboard[KING_BLACK] = ( chessboard[KING_BLACK] | POW2_59 ) & NOTPOW2_61;
            chessboard[ROOK_BLACK] = ( chessboard[ROOK_BLACK] | POW2_63 ) & NOTPOW2_60;
        }
    }
}

template <int side>
bool GenMoves::inCheckPerft ( const int from, const int to, const uchar type, const int pieceFrom, const int pieceTo, int promotionPiece ) {
    ASSERT_RANGE ( from, 0, 63 );
    ASSERT_RANGE ( to, 0, 63 );
    ASSERT_RANGE ( side, 0, 1 );
    ASSERT_RANGE ( pieceFrom, 0, 12 );
    ASSERT_RANGE ( pieceTo, 0, 12 );
    ASSERT ( perftMode );
    ASSERT ( ! ( type & 0xC ) );
    bool result = 0;

    if ( ( type & 0x3 ) == STANDARD_MOVE_MASK ) {
        u64 from1, to1 = -1;
        ASSERT ( pieceFrom != SQUARE_FREE );
        ASSERT ( pieceTo != KING_BLACK );
        ASSERT ( pieceTo != KING_WHITE );
        from1 = chessboard[pieceFrom];

        if ( pieceTo != SQUARE_FREE ) {
            to1 = chessboard[pieceTo];
            chessboard[pieceTo] &= NOTPOW2[to];
        };
        chessboard[pieceFrom] &= NOTPOW2[from];
        chessboard[pieceFrom] |= POW2[to];
        ASSERT ( chessboard[KING_BLACK] );
        ASSERT ( chessboard[KING_WHITE] );
        result = attackSquare<side> ( BITScanForward ( chessboard[KING_BLACK + side] ) );
        chessboard[pieceFrom] = from1;
        if ( pieceTo != SQUARE_FREE ) {
            chessboard[pieceTo] = to1;
        }
    }
    else if ( ( type & 0x3 ) == PROMOTION_MOVE_MASK ) {
        u64 to1 = 0;

        if ( pieceTo != SQUARE_FREE ) {
            to1 = chessboard[pieceTo];
        }

        u64 from1 = chessboard[pieceFrom];
        u64 p1 = chessboard[promotionPiece];
        chessboard[pieceFrom] &= NOTPOW2[from];

        if ( pieceTo != SQUARE_FREE ) {
            chessboard[pieceTo] &= NOTPOW2[to];
        }

        chessboard[promotionPiece] = chessboard[promotionPiece] | POW2[to];
        result = attackSquare<side> ( BITScanForward ( chessboard[KING_BLACK + side] ) );

        if ( pieceTo != SQUARE_FREE ) {
            chessboard[pieceTo] = to1;
        }

        chessboard[pieceFrom] = from1;
        chessboard[promotionPiece] = p1;
    }
    else if ( ( type & 0x3 ) == ENPASSANT_MOVE_MASK ) {
        u64 to1 = chessboard[side ^ 1];
        u64 from1 = chessboard[side];
        chessboard[side] &= NOTPOW2[from];
        chessboard[side] |= POW2[to];

        if ( side ) {
            chessboard[side ^ 1] &= NOTPOW2[to - 8];
        }
        else {
            chessboard[side ^ 1] &= NOTPOW2[to + 8];
        }

        result = attackSquare<side> ( BITScanForward ( chessboard[KING_BLACK + side] ) );
        chessboard[side ^ 1] = to1;
        chessboard[side] = from1;
    }

#ifdef DEBUG_MODE
    else {
        ASSERT (0);
    }

#endif
    return result;
}

void GenMoves::takeback ( _Tmove * move,  const u64 oldkey, bool rep ) {
    if ( rep ) { popStackMove(); }

    zobristKey = oldkey;
    enpassantPosition = NO_ENPASSANT;
    int pieceFrom, posTo, posFrom, movecapture;
    rightCastle = move->type & 0xf0;

    if ( ( move->type & 0x3 ) == STANDARD_MOVE_MASK || ( move->type & 0x3 ) == ENPASSANT_MOVE_MASK ) {
        posTo = move->to;
        posFrom = move->from;
        movecapture = move->capturedPiece;
        ASSERT ( posFrom >= 0 && posFrom < 64 );
        ASSERT ( posTo >= 0 && posTo < 64 );
        pieceFrom = move->pieceFrom;
        chessboard[pieceFrom] = ( chessboard[pieceFrom] & NOTPOW2[posTo] ) | POW2[posFrom];

        if ( movecapture != SQUARE_FREE ) {
            if ( ( ( move->type & 0x3 ) != ENPASSANT_MOVE_MASK ) ) {
                chessboard[movecapture] |= POW2[posTo];
            }
            else {
                ASSERT ( movecapture == ( move->side ^ 1 ) );

                if ( move->side ) {
                    chessboard[movecapture] |= POW2[posTo - 8];
                }
                else {
                    chessboard[movecapture] |= POW2[posTo + 8];
                }
            }
        }
    }
    else if ( ( move->type & 0x3 ) == PROMOTION_MOVE_MASK ) {
        posTo = move->to;
        posFrom = move->from;
        movecapture = move->capturedPiece;
        ASSERT ( posTo >= 0 && move->side >= 0 && move->promotionPiece >= 0 );
        chessboard[ ( uchar ) move->side] |= POW2[posFrom];
        chessboard[ ( uchar ) move->promotionPiece] &= NOTPOW2[posTo];

        if ( movecapture != SQUARE_FREE ) {
            chessboard[movecapture] |= POW2[posTo];
        }
    }
    else if ( move->type & 0xc ) { //castle
        unPerformCastle ( move->side, move->type );
    }
}


bool GenMoves::makemove ( _Tmove * move, bool rep, bool checkInCheck ) {
    ASSERT ( move );
    ASSERT ( bitCount ( chessboard[KING_WHITE] ) == 1 && bitCount ( chessboard[KING_BLACK] ) == 1 );
    int pieceFrom = SQUARE_FREE, posTo, posFrom, movecapture = SQUARE_FREE;
    uchar rightCastleOld = rightCastle;

    if ( ! ( move->type & 0xc ) ) { //no castle
        posTo = move->to;
        posFrom = move->from;
        ASSERT ( posTo >= 0 );
        movecapture = move->capturedPiece;
        ASSERT ( posFrom >= 0 && posFrom < 64 );
        ASSERT ( posTo >= 0 && posTo < 64 );
        pieceFrom = move->pieceFrom;

        if ( ( move->type & 0x3 ) == PROMOTION_MOVE_MASK ) {
            chessboard[pieceFrom] &= NOTPOW2[posFrom];
            updateZobristKey ( pieceFrom, posFrom );
            ASSERT ( move->promotionPiece >= 0 );
            chessboard[ ( uchar ) move->promotionPiece] |= POW2[posTo];
            updateZobristKey ( ( uchar ) move->promotionPiece, posTo );
        }
        else {
            chessboard[pieceFrom] = ( chessboard[pieceFrom] | POW2[posTo] ) & NOTPOW2[posFrom];
            updateZobristKey ( pieceFrom, posFrom );
            updateZobristKey ( pieceFrom, posTo );
        }

        if ( movecapture != SQUARE_FREE ) {
            if ( ( move->type & 0x3 ) != ENPASSANT_MOVE_MASK ) {
                chessboard[movecapture] &= NOTPOW2[posTo];
                updateZobristKey ( movecapture, posTo );
            }
            else {//en passant
                ASSERT ( movecapture == ( move->side ^ 1 ) );

                if ( move->side ) {
                    chessboard[movecapture] &= NOTPOW2[posTo - 8];
                    updateZobristKey ( movecapture, posTo - 8 );
                }
                else {
                    chessboard[movecapture] &= NOTPOW2[posTo + 8];
                    updateZobristKey ( movecapture, posTo + 8 );
                }
            }
        }

        //lost castle right

        switch ( pieceFrom ) {
            case KING_WHITE: {
                    rightCastle &= 0xcf;
                }
                break;

            case KING_BLACK: {
                    rightCastle &= 0x3f;
                }
                break;

            case ROOK_WHITE:
                if ( posFrom == 0 ) {
                    rightCastle &= 0xef;
                }
                else if ( posFrom == 7 ) {
                    rightCastle &= 0xdf;
                }

                break;

            case ROOK_BLACK:
                if ( posFrom == 56 ) {
                    rightCastle &= 0xbf;
                }
                else if ( posFrom == 63 ) {
                    rightCastle &= 0x7f;
                }

                break;

                //en passant
            case PAWN_WHITE:
                if ( ( RANK_1 & POW2[posFrom] ) && ( RANK_3 & POW2[posTo] ) ) {
                    enpassantPosition = posTo;
                    updateZobristKey ( 13, enpassantPosition );
                }

                break;

            case PAWN_BLACK:
                if ( ( RANK_6 & POW2[posFrom] ) && ( RANK_4 & POW2[posTo] ) ) {
                    enpassantPosition = posTo;
                    updateZobristKey ( 13, enpassantPosition );
                }

                break;

            default:
                ;
        }
    }
    else if ( move->type & 0xc ) { //castle
        performCastle ( move->side, move->type );

        if ( move->side == WHITE ) {
            rightCastle &= 0xcf;
        }
        else {
            rightCastle &= 0x3f;
        }
    }

    u64 x2 = rightCastleOld ^ rightCastle;

    while ( x2 ) {
        int position = BITScanForward ( x2 );
        updateZobristKey ( 14, position );
        x2 &= NOTPOW2[position];
    }

    if ( rep ) {
        if ( movecapture != SQUARE_FREE || pieceFrom == WHITE || pieceFrom == BLACK || move->type & 0xc ) {
            pushStackMove ( 0 );
        }

        pushStackMove ( zobristKey );
    }

    if ( ( forceCheck || ( checkInCheck && !perftMode ) ) && ( ( move->side == WHITE && inCheck<WHITE>() ) || ( move->side == BLACK && inCheck<BLACK>() ) ) ) {
        return false;
    }

    return true;
}

void GenMoves::init() {
    numMoves = numMovesq = 0;
#ifdef DEBUG_MODE
    nCutFp = nCutRazor = 0;
    betaEfficiency = betaEfficiencyCumulative = 0.0;
    nCutAB = 0;
    nNullMoveCut = nCutInsufficientMaterial = 0;
#endif
    listId = 0;
}

u64 GenMoves::getTotMoves() {
    return numMoves + numMovesq;
}
void GenMoves::setRepetitionMapCount ( int i ) {
    repetitionMapCount = i;
}

int GenMoves::loadFen ( string fen ) {
    repetitionMapCount = 0;
    int side = ChessBoard::loadFen ( fen );
    return side;
}

int GenMoves::getMoveFromSan ( const string fenStr, _Tmove* move ) {
    enpassantPosition = NO_ENPASSANT;
    memset ( move, 0, sizeof ( _Tmove ) );
    static const string MATCH_QUEENSIDE = "O-O-O e1c1 e8c8";
    static const string MATCH_QUEENSIDE_WHITE = "O-O-O e1c1";
    static const string MATCH_KINGSIDE_WHITE = "O-O e1g1";
    static const string MATCH_QUEENSIDE_BLACK = "O-O-O e8c8";
    static const string MATCH_KINGSIDE_BLACK = "O-O e8g8";

    if (
        ( ( MATCH_QUEENSIDE_WHITE.find ( fenStr ) != string::npos || MATCH_KINGSIDE_WHITE.find ( fenStr ) != string::npos ) && getPieceAt<WHITE> ( POW2[E1] ) == KING_WHITE )
        ||
        ( ( MATCH_QUEENSIDE_BLACK.find ( fenStr ) != string::npos || MATCH_KINGSIDE_BLACK.find ( fenStr ) != string::npos )
          && getPieceAt<BLACK> ( POW2[E8] ) == KING_BLACK )
    ) {
        if ( MATCH_QUEENSIDE.find ( fenStr ) != string::npos ) {
            move->type = QUEEN_SIDE_CASTLE_MOVE_MASK;
            move->from = QUEEN_SIDE_CASTLE_MOVE_MASK;
        }
        else {
            move->from = KING_SIDE_CASTLE_MOVE_MASK;
            move->type = KING_SIDE_CASTLE_MOVE_MASK;
        }

        if ( fenStr.find ( "1" ) != string::npos ) {
            move->side = WHITE;
        }
        else if ( fenStr.find ( "8" ) != string::npos ) {
            move->side = BLACK;
        }
        else { assert ( 0 ); }

        move->from = -1;
        move->capturedPiece = SQUARE_FREE;
        return move->side;
    }

    int from = -1;
    int to = -1;

    for ( int i = 0; i < 64; i++ ) {
        if ( !fenStr.compare ( 0, 2, BOARD[i] ) ) {
            from = i;
            break;
        }
    }

    if ( from == -1 ) {
        cout << fenStr << endl;
        assert ( 0 );
    }

    for ( int i = 0; i < 64; i++ ) {
        if ( !fenStr.compare ( 2, 2, BOARD[i] ) ) {
            to = i;
            break;
        }
    }

    if ( to == -1 ) {
        cout << fenStr << endl;
        assert ( 0 );
    }

    int pieceFrom;

    if ( ( pieceFrom = getPieceAt<WHITE> ( POW2[from] ) ) != 12 ) {
        move->side = WHITE;
    }
    else if ( ( pieceFrom = getPieceAt<BLACK> ( POW2[from] ) ) != 12 ) {
        move->side = BLACK;
    }
    else {
        cout << "fenStr: " << fenStr << " from: " << from << endl;
        assert ( 0 );
    }

    move->from = from;
    move->to = to;

    if ( fenStr.length() == 4 ) {
        move->type = STANDARD_MOVE_MASK;

        if ( pieceFrom == PAWN_WHITE || pieceFrom == PAWN_BLACK ) {
            if ( FILE_AT[from] != FILE_AT[to] && ( move->side ^ 1 ? getPieceAt<WHITE> ( POW2[to] ) : getPieceAt<BLACK> ( POW2[to] ) ) == SQUARE_FREE ) {
                move->type = ENPASSANT_MOVE_MASK;
            }
        }
    }
    else if ( fenStr.length() == 5 ) {
        move->type = PROMOTION_MOVE_MASK;

        if ( move->side == WHITE ) {
            move->promotionPiece = INV_FEN[toupper ( fenStr.at ( 4 ) )];
        }
        else {
            move->promotionPiece = INV_FEN[ ( uchar ) fenStr.at ( 4 )];
        }

        ASSERT ( move->promotionPiece != -1 );
    }

    if ( move->side == WHITE ) {
        move->capturedPiece = getPieceAt<BLACK> ( POW2[move->to] );
        move->pieceFrom = getPieceAt<WHITE> ( POW2[move->from] );
    }
    else {
        move->capturedPiece = getPieceAt<WHITE> ( POW2[move->to] );
        move->pieceFrom = getPieceAt<BLACK> ( POW2[move->from] );
    }

    if ( move->type == ENPASSANT_MOVE_MASK ) {
        move->capturedPiece = !move->side;
    }

    return move->side;
}

