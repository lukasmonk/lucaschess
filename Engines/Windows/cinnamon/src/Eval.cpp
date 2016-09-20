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

#include "Eval.h"

Eval::Eval() {
}


Eval::~Eval() {
}

void Eval::openColumn ( int side ) {
    u64 side_rooks = structure.rooks[side];
    structure.openColumn = 0;
    structure.semiOpenColumn[side] = 0;

    while ( side_rooks ) {
        int o = BITScanForward ( side_rooks );

        if ( ! ( FILE_[o] & ( structure.pawns[WHITE] | structure.pawns[BLACK] ) ) ) {
            structure.openColumn |= FILE_[o];
        }
        else if ( FILE_[o] & structure.pawns[side ^ 1] ) {
            structure.semiOpenColumn[side] |= FILE_[o];
        }

        side_rooks &= NOTPOW2[o];
    }
}

template <int side, _Tstatus status>
int Eval::evaluatePawn() {
    INC ( evaluationCount[side] );
    u64 ped_friends = structure.pawns[side];

    if ( !ped_friends ) {
        ADD ( SCORE_DEBUG.NO_PAWNS[side], -NO_PAWNS );
        return -NO_PAWNS;
    }

    structure.isolated[side] = 0;
    int result = MOB_PAWNS[getMobilityPawns ( side, enpassantPosition,  ped_friends, side == WHITE ? structure.allPiecesSide[BLACK] : structure.allPiecesSide[WHITE], ~structure.allPiecesSide[BLACK] | ~structure.allPiecesSide[WHITE] )];
    ADD ( SCORE_DEBUG.MOB_PAWNS[side], result );

    if ( bitCount ( structure.pawns[side ^ 1] ) == 8 ) {
        result -= ENEMIES_PAWNS_ALL;
        ADD ( SCORE_DEBUG.ENEMIES_PAWNS_ALL[side], -ENEMIES_PAWNS_ALL );
    }

    result += ATTACK_KING * bitCount ( ped_friends & structure.kingAttackers[side ^ 1] );
    ADD ( SCORE_DEBUG.ATTACK_KING_PAWN[side], ATTACK_KING * bitCount ( ped_friends & structure.kingAttackers[side ^ 1] ) );
    //space

    if ( status == OPEN ) {
        result += PAWN_CENTER * bitCount ( ped_friends & CENTER_MASK );
        ADD ( SCORE_DEBUG.PAWN_CENTER[side], PAWN_CENTER * bitCount ( ped_friends & CENTER_MASK ) );
    }

    u64 p = ped_friends;

    while ( p ) {
        int o = BITScanForward ( p );
        u64 pos = POW2[o];

        if ( status != OPEN ) {
            structure.kingSecurityDistance[side] += FRIEND_NEAR_KING * ( NEAR_MASK2[structure.posKing[side]] & pos ? 1 : 0 );
            structure.kingSecurityDistance[side] -= ENEMY_NEAR_KING * ( NEAR_MASK2[structure.posKing[side ^ 1]] & pos ? 1 : 0 );

            ///  pawn in race
            if ( PAWNS_7_2[side] & pos ) {
                result += PAWN_7H ;
                ADD ( SCORE_DEBUG.PAWN_7H[side], PAWN_7H );

                if ( ( ( shiftForward<side, 8> ( pos ) & ( ~structure.allPieces ) ) ||
                        ( structure.allPiecesSide[side ^ 1] &PAWN_FORK_MASK[side][o] ) )
                   ) {
                    result += PAWN_IN_RACE;
                    ADD ( SCORE_DEBUG.PAWN_IN_RACE[side], PAWN_IN_RACE );
                }
            }
        }

        /// blocked
        result -= ( ! ( PAWN_FORK_MASK[side][o] & structure.allPiecesSide[side ^ 1] ) ) && ( structure.allPieces & ( shiftForward<side, 8> ( pos ) ) ) ? PAWN_BLOCKED : 0;
        ADD ( SCORE_DEBUG.PAWN_BLOCKED[side], ( ! ( PAWN_FORK_MASK[side][o] & structure.allPiecesSide[side ^ 1] ) ) && ( structure.allPieces & ( shiftForward<side, 8> ( pos ) ) ) ? -PAWN_BLOCKED : 0 );

        /// unprotected
        if ( ! ( ped_friends & PAWN_PROTECTED_MASK[side][o] ) ) {
            result -= UNPROTECTED_PAWNS;
            ADD ( SCORE_DEBUG.UNPROTECTED_PAWNS[side], -UNPROTECTED_PAWNS );
        };

        /// isolated
        if ( ! ( ped_friends & PAWN_ISOLATED_MASK[o] ) ) {
            result -= PAWN_ISOLATED;
            ADD ( SCORE_DEBUG.PAWN_ISOLATED[side], -PAWN_ISOLATED );
            structure.isolated[side] |= pos;
        }

        /// doubled
        if ( NOTPOW2[o] & FILE_[o] & ped_friends ) {
            result -= DOUBLED_PAWNS;
            ADD ( SCORE_DEBUG.DOUBLED_PAWNS[side], -DOUBLED_PAWNS );

            if ( ! ( structure.isolated[side] & pos ) ) {
                ADD ( SCORE_DEBUG.DOUBLED_ISOLATED_PAWNS[side], -DOUBLED_ISOLATED_PAWNS );
                result -= DOUBLED_ISOLATED_PAWNS;
            }
        };

        /// backward
        if ( ! ( ped_friends & PAWN_BACKWARD_MASK[side][o] ) ) {
            ADD ( SCORE_DEBUG.BACKWARD_PAWN[side], -BACKWARD_PAWN );
            result -= BACKWARD_PAWN;
        }

        /// passed
        if ( ! ( structure.pawns[side ^ 1] & PAWN_PASSED_MASK[side][o] ) ) {
            ADD ( SCORE_DEBUG.PAWN_PASSED[side], PAWN_PASSED[side][o] );
            result += PAWN_PASSED[side][o];
        }

        p &= NOTPOW2[o];
    }

    return result;
}

template <int side, _Tstatus status>
int Eval::evaluateBishop ( u64 enemies, u64 friends ) {
    INC ( evaluationCount[side] );
    u64 x = chessboard[BISHOP_BLACK + side];

    if ( !x ) {
        return 0;
    }

    int result = 0;

    if ( status != OPEN && bitCount ( x ) > 1 ) {
        result += BONUS2BISHOP;
        ADD ( SCORE_DEBUG.BONUS2BISHOP[side], BONUS2BISHOP );
    }

    while ( x ) {
        int o = BITScanForward ( x );
        u64 captured = performDiagCaptureCount ( o, enemies | friends ) ;
        ASSERT ( bitCount ( captured & enemies ) + performDiagShiftCount ( o, enemies | friends ) < ( int ) ( sizeof ( MOB_BISHOP ) / sizeof ( int ) ) );
        result += MOB_BISHOP[status][bitCount ( captured & enemies ) + performDiagShiftCount ( o, enemies | friends )];
        ADD ( SCORE_DEBUG.MOB_BISHOP[side], MOB_BISHOP[status][bitCount ( captured & enemies ) + performDiagShiftCount ( o, enemies | friends )] );
        structure.kingSecurityDistance[side] +=    BISHOP_NEAR_KING * ( NEAR_MASK2[structure.posKing[side]] & POW2[o] ? 1 : 0 );
        ADD ( SCORE_DEBUG.KING_SECURITY_BISHOP[side], BISHOP_NEAR_KING * ( NEAR_MASK2[structure.posKing[side]] & POW2[o] ? 1 : 0 ) );

        if ( status != OPEN ) {
            structure.kingSecurityDistance[side] -=       NEAR_MASK2[structure.posKing[side ^ 1]] & POW2[o] ? ENEMY_NEAR_KING : 0;
            ADD ( SCORE_DEBUG.KING_SECURITY_BISHOP[side ^ 1], -  NEAR_MASK2[structure.posKing[side ^ 1]] & POW2[o] ? ENEMY_NEAR_KING : 0 );
        }
        else
            //attack center
            if ( status == OPEN ) {
                if ( side ) {
                    if ( o == C1 || o == F1 ) {
                        ADD ( SCORE_DEBUG.UNDEVELOPED_BISHOP[side], -UNDEVELOPED_BISHOP );
                        result -= UNDEVELOPED_BISHOP;
                    }
                }
                else {
                    if ( o == C8 || o == F8 ) {
                        ADD ( SCORE_DEBUG.UNDEVELOPED_BISHOP[side], -UNDEVELOPED_BISHOP );
                        result -= UNDEVELOPED_BISHOP;
                    }
                }
            }
            else {
                if ( BIG_DIAG_LEFT & POW2[o] && ! ( LEFT_DIAG[o] & structure.allPieces ) ) {
                    ADD ( SCORE_DEBUG.OPEN_DIAG_BISHOP[side], OPEN_FILE );
                    result += OPEN_FILE;
                }

                if ( BIG_DIAG_RIGHT & POW2[o] && ! ( RIGHT_DIAG[o] & structure.allPieces ) ) {
                    ADD ( SCORE_DEBUG.OPEN_DIAG_BISHOP[side], OPEN_FILE );
                    result += OPEN_FILE;
                }
            }

        x &= NOTPOW2[o];
    };

    return result;
}

template <_Tstatus status>
int Eval::evaluateQueen ( int side, u64 enemies, u64 friends ) {
    INC ( evaluationCount[side] );
    int result = 0;
    u64 queen = chessboard[QUEEN_BLACK + side];

    while ( queen ) {
        int o = BITScanForward ( queen );
        ASSERT ( getMobilityQueen ( o, enemies, friends ) < ( int ) ( sizeof ( MOB_QUEEN[status] ) / sizeof ( int ) ) );
        result += MOB_QUEEN[status][getMobilityQueen ( o, enemies, friends )];
        ADD ( SCORE_DEBUG.MOB_QUEEN[side], MOB_QUEEN[status][getMobilityQueen ( o, enemies, friends )] );

        if ( status != OPEN ) {
            structure.kingSecurityDistance[side] +=         FRIEND_NEAR_KING * ( NEAR_MASK2[structure.posKing[side]] & POW2[o] ? 1 : 0 );
            ADD ( SCORE_DEBUG.KING_SECURITY_QUEEN[side],      FRIEND_NEAR_KING * ( NEAR_MASK2[structure.posKing[side]] & POW2[o] ? 1 : 0 ) );
            structure.kingSecurityDistance[side] -=       ENEMY_NEAR_KING * ( NEAR_MASK2[structure.posKing[side ^ 1]] & POW2[o] ? 1 : 0 );
            ADD ( SCORE_DEBUG.KING_SECURITY_QUEEN[side ^ 1], -   ENEMY_NEAR_KING * ( NEAR_MASK2[structure.posKing[side ^ 1]] & POW2[o] ? 1 : 0 ) );
        }
        if ( ( structure.pawns[side ^ 1] & FILE_[o] ) ) {
            ADD ( SCORE_DEBUG.HALF_OPEN_FILE_Q[side], HALF_OPEN_FILE_Q );
            result += HALF_OPEN_FILE_Q;
        }

        if ( ( FILE_[o] & structure.allPieces ) == POW2[o] ) {
            ADD ( SCORE_DEBUG.OPEN_FILE_Q[side], OPEN_FILE_Q );
            result += OPEN_FILE_Q;
        }

        if ( LEFT_RIGHT_DIAG[o] & chessboard[BISHOP_BLACK + side] ) {
            ADD ( SCORE_DEBUG.BISHOP_ON_QUEEN[side], BISHOP_ON_QUEEN );
            result += BISHOP_ON_QUEEN;
        }

        queen &= NOTPOW2[o];
    };

    return result;
}

template <int side, _Tstatus status>
int Eval::evaluateKnight ( const u64 enemiesPawns, const u64 squares ) {
    INC ( evaluationCount[side] );
    int result = 0;
    u64 x = chessboard[KNIGHT_BLACK + side];
    if ( status == OPEN ) {
        result -= side ? bitCount ( x & 0x42ULL ) * UNDEVELOPED : bitCount ( x & 0x4200000000000000ULL ) * UNDEVELOPED;
        ADD ( SCORE_DEBUG.UNDEVELOPED_KNIGHT[side], side ? -bitCount ( x & 0x42ULL ) *UNDEVELOPED : -bitCount ( x & 0x4200000000000000ULL ) *UNDEVELOPED );
    }

    if ( side == WHITE ) {
        if ( ( A7bit & x ) && ( B7bit & enemiesPawns ) && ( C6A6bit & enemiesPawns ) ) {
            ADD ( SCORE_DEBUG.KNIGHT_TRAPPED[side], -KNIGHT_TRAPPED );
            result -= KNIGHT_TRAPPED;
        }

        if ( ( H7bit & x ) && ( G7bit & enemiesPawns ) && ( F6H6bit & enemiesPawns ) ) {
            ADD ( SCORE_DEBUG.KNIGHT_TRAPPED[side], -KNIGHT_TRAPPED );
            result -= KNIGHT_TRAPPED;
        }

        if ( ( A8bit & x ) && ( A7C7bit & enemiesPawns ) ) {
            ADD ( SCORE_DEBUG.KNIGHT_TRAPPED[side], -KNIGHT_TRAPPED );
            result -= KNIGHT_TRAPPED;
        }

        if ( ( H8bit & x ) && ( H7G7bit & enemiesPawns ) ) {
            ADD ( SCORE_DEBUG.KNIGHT_TRAPPED[side], -KNIGHT_TRAPPED );
            result -= KNIGHT_TRAPPED;
        }
    }
    else {
        if ( ( A2bit & x ) && ( B2bit & enemiesPawns ) && ( C3A3bit & enemiesPawns ) ) {
            ADD ( SCORE_DEBUG.KNIGHT_TRAPPED[side], -KNIGHT_TRAPPED );
            result -= KNIGHT_TRAPPED;
        }

        if ( ( H2bit & x ) && ( G2bit & enemiesPawns ) && ( F3H3bit & enemiesPawns ) ) {
            ADD ( SCORE_DEBUG.KNIGHT_TRAPPED[side], -KNIGHT_TRAPPED );
            result -= KNIGHT_TRAPPED;
        }

        if ( ( A1bit & x ) && ( A2C2bit & enemiesPawns ) ) {
            ADD ( SCORE_DEBUG.KNIGHT_TRAPPED[side], -KNIGHT_TRAPPED );
            result -= KNIGHT_TRAPPED;
        }

        if ( ( H1bit & x ) && ( H2G2bit & enemiesPawns ) ) {
            ADD ( SCORE_DEBUG.KNIGHT_TRAPPED[side], -KNIGHT_TRAPPED );
            result -= KNIGHT_TRAPPED;
        }
    }

    while ( x ) {
        int pos = BITScanForward ( x );
        if ( status != OPEN ) {
            structure.kingSecurityDistance[side] += FRIEND_NEAR_KING * ( NEAR_MASK2[structure.posKing[side]] & POW2[pos] ? 1 : 0 );
            ADD ( SCORE_DEBUG.KING_SECURITY_KNIGHT[side],   FRIEND_NEAR_KING * ( NEAR_MASK2[structure.posKing[side]] & POW2[pos] ? 1 : 0 ) );
            structure.kingSecurityDistance[side] -= ENEMY_NEAR_KING * ( NEAR_MASK2[structure.posKing[side ^ 1]] & POW2[pos] ? 1 : 0 );
            ADD ( SCORE_DEBUG.KING_SECURITY_KNIGHT[side ^ 1], -ENEMY_NEAR_KING * ( NEAR_MASK2[structure.posKing[side ^ 1]] & POW2[pos] ? 1 : 0 ) );
        }

        //mobility
        ASSERT ( bitCount ( squares & KNIGHT_MASK[pos] ) < ( int ) ( sizeof ( MOB_KNIGHT ) / sizeof ( int ) ) );
        result += MOB_KNIGHT[bitCount ( squares & KNIGHT_MASK[pos] )];
        ADD ( SCORE_DEBUG.MOB_KNIGHT[side], MOB_KNIGHT[bitCount ( squares & KNIGHT_MASK[pos] )] );
        x &= NOTPOW2[pos];
    };

    return result;
}

template <int side, _Tstatus status>
int Eval::evaluateRook ( const u64 king, u64 enemies, u64 friends ) {
    INC ( evaluationCount[side] );
    int o, result = 0;
    u64 x = chessboard[ROOK_BLACK + side];

    if ( !x ) {
        return 0;
    }

    if ( status == MIDDLE ) {
        if ( !side && ( o = bitCount ( x & RANK_1 ) ) ) {
            ADD ( SCORE_DEBUG.ROOK_7TH_RANK[side], ROOK_7TH_RANK * o );
            result += ROOK_7TH_RANK * o;
        }

        if ( side && ( o = bitCount ( x & RANK_6 ) ) ) {
            ADD ( SCORE_DEBUG.ROOK_7TH_RANK[side], ROOK_7TH_RANK * o );
            result += ROOK_7TH_RANK * o;
        }
    }

    if ( side == WHITE ) {
        if ( ( ( F1G1bit & king ) && ( H1H2G1bit & x ) ) || ( ( C1B1bit & king ) && ( A1A2B1bit & x ) ) ) {
            ADD ( SCORE_DEBUG.ROOK_TRAPPED[side], -ROOK_TRAPPED );
            result -= ROOK_TRAPPED;
        }
    }
    else {
        if ( ( ( F8G8bit & king ) && ( H8H7G8bit & x ) ) || ( ( C8B8bit & king ) && ( A8A7B8bit & x ) ) ) {
            ADD ( SCORE_DEBUG.ROOK_TRAPPED[side], -ROOK_TRAPPED );
            result -= ROOK_TRAPPED;
        }
    }

    int firstRook = -1;
    int secondRook = -1;

    while ( x ) {
        o = BITScanForward ( x );
        //mobility
        ASSERT ( getMobilityRook ( o, enemies, friends ) < ( int ) ( sizeof ( MOB_ROOK[status] ) / sizeof ( int ) ) );
        result += MOB_ROOK[status][getMobilityRook ( o, enemies, friends )];
        ADD ( SCORE_DEBUG.MOB_ROOK[side], MOB_ROOK[status][getMobilityRook ( o, enemies, friends )] );

        if ( firstRook == -1 ) {
            firstRook = o;
        }
        else {
            secondRook = o;
        }

        if ( status != OPEN ) {
            structure.kingSecurityDistance[side] += FRIEND_NEAR_KING * ( NEAR_MASK2[structure.posKing[side]] & POW2[o] ? 1 : 0 );
            ADD ( SCORE_DEBUG.KING_SECURITY_ROOK[side],  FRIEND_NEAR_KING * ( NEAR_MASK2[structure.posKing[side]] & POW2[o] ? 1 : 0 ) );
            structure.kingSecurityDistance[side] -= ENEMY_NEAR_KING * ( NEAR_MASK2[structure.posKing[side ^ 1]] & POW2[o] ? 1 : 0 );
            ADD ( SCORE_DEBUG.KING_SECURITY_ROOK[side ^ 1], -ENEMY_NEAR_KING * ( NEAR_MASK2[structure.posKing[side ^ 1]] & POW2[o] ? 1 : 0 ) );

            /* Penalise if Rook is Blocked Horizontally */
            if ( ( RANK_BOUND[o] & structure.allPieces ) == RANK_BOUND[o] ) {
                ADD ( SCORE_DEBUG.ROOK_BLOCKED[side], -ROOK_BLOCKED );
                result -= ROOK_BLOCKED;
            };
        }

        if ( ! ( structure.pawns[side] & FILE_[o] ) ) {
            ADD ( SCORE_DEBUG.ROOK_OPEN_FILE[side], OPEN_FILE );
            result += OPEN_FILE;
        }

        if ( ! ( structure.pawns[side ^ 1] & FILE_[o] ) ) {
            ADD ( SCORE_DEBUG.ROOK_OPEN_FILE[side], OPEN_FILE );
            result += OPEN_FILE;
        }

        x &= NOTPOW2[o];
    };

    if ( firstRook != -1 && secondRook != -1 ) {
        if ( ( ! ( LINK_ROOKS[firstRook][secondRook] & structure.allPieces ) ) ) {
            ADD ( SCORE_DEBUG.CONNECTED_ROOKS[side], CONNECTED_ROOKS );
            result += CONNECTED_ROOKS;
        }
    }

    return result;
}

template <_Tstatus status>
int Eval::evaluateKing ( int side, u64 squares ) {
    ASSERT ( evaluationCount[side] == 5 );
    int result = 0;
    uchar pos_king = structure.posKing[side];

    if ( status == END ) {
        ADD ( SCORE_DEBUG.DISTANCE_KING[side], DISTANCE_KING_ENDING[pos_king] );
        result = DISTANCE_KING_ENDING[pos_king];
    }
    else {
        ADD ( SCORE_DEBUG.DISTANCE_KING[side], DISTANCE_KING_OPENING[pos_king] );
        result = DISTANCE_KING_OPENING[pos_king];
    }

    u64 POW2_king = POW2[pos_king];
    //mobility
    ASSERT ( bitCount ( squares & NEAR_MASK1[pos_king] ) < ( int ) ( sizeof ( MOB_KING[status] ) / sizeof ( int ) ) );
    result += MOB_KING[status][bitCount ( squares & NEAR_MASK1[pos_king] )];
    ADD ( SCORE_DEBUG.MOB_KING[side], MOB_KING[status][bitCount ( squares & NEAR_MASK1[pos_king] )] );

    if ( status != OPEN ) {
        if ( ( structure.openColumn & POW2_king ) || ( structure.semiOpenColumn[side ^ 1] & POW2_king ) ) {
            ADD ( SCORE_DEBUG.END_OPENING_KING[side], -END_OPENING );
            result -= END_OPENING;

            if ( bitCount ( RANK[pos_king] ) < 4 ) {
                ADD ( SCORE_DEBUG.END_OPENING_KING[side], -END_OPENING );
                result -= END_OPENING;
            }
        }
    }

    ASSERT ( pos_king < 64 );

    if ( ! ( NEAR_MASK1[pos_king] & structure.pawns[side] ) ) {
        ADD ( SCORE_DEBUG.PAWN_NEAR_KING[side], -PAWN_NEAR_KING );
        result -= PAWN_NEAR_KING;
    }

    result += structure.kingSecurityDistance[side];
    return result;
}

int Eval::getScore (const int side , const int alpha, const int beta ) {
    int lazyscore_white = lazyEvalSide<WHITE>();
    int lazyscore_black = lazyEvalSide<BLACK>();
    int lazyscore = lazyscore_black - lazyscore_white;

    if ( side ) {
        lazyscore = -lazyscore;
    }

    if ( lazyscore > ( beta + FUTIL_MARGIN ) || lazyscore < ( alpha - FUTIL_MARGIN ) ) {
        INC ( LazyEvalCuts );
        return lazyscore;
    }

#ifdef DEBUG_MODE
    evaluationCount[WHITE] = evaluationCount[BLACK] = 0;
    memset ( &SCORE_DEBUG, 0, sizeof ( _TSCORE_DEBUG ) );
#endif
    memset ( structure.kingSecurityDistance, 0,  sizeof ( structure.kingSecurityDistance ) );
    int npieces = getNpiecesNoPawnNoKing<WHITE>() + getNpiecesNoPawnNoKing<BLACK>();
    _Tstatus status;

    if ( npieces < 4 ) {
        status = END;
    }
    else if ( npieces < 11 ) {
        status = MIDDLE;
    }
    else {
        status = OPEN;
    }

    structure.allPiecesNoPawns[BLACK] = getBitBoardNoPawns<BLACK>();
    structure.allPiecesNoPawns[WHITE] = getBitBoardNoPawns<WHITE>();
    structure.allPiecesSide[BLACK] = structure.allPiecesNoPawns[BLACK] | chessboard[PAWN_BLACK];
    structure.allPiecesSide[WHITE] = structure.allPiecesNoPawns[WHITE] | chessboard[PAWN_WHITE];
    structure.allPieces = structure.allPiecesSide[BLACK] | structure.allPiecesSide[WHITE];
    structure.posKing[BLACK] = ( uchar ) BITScanForward ( chessboard[KING_BLACK] );
    structure.posKing[WHITE] = ( uchar ) BITScanForward ( chessboard[KING_WHITE] );
    structure.kingAttackers[WHITE] = getKingAttackers ( BLACK, structure.allPieces, structure.posKing[WHITE] );
    structure.kingAttackers[BLACK] = getKingAttackers ( WHITE, structure.allPieces, structure.posKing[BLACK] );
    structure.pawns[BLACK] = chessboard[BLACK];
    structure.pawns[WHITE] = chessboard[WHITE];
    structure.rooks[BLACK] = chessboard[ROOK_BLACK];
    structure.rooks[WHITE] = chessboard[ROOK_WHITE];
    openColumn ( WHITE );
    openColumn ( BLACK );
    int pawns_score_black ;
    int pawns_score_white ;
    int bishop_score_black;
    int bishop_score_white;
    int queens_score_black ;
    int queens_score_white ;
    int rooks_score_black ;
    int rooks_score_white ;
    int knights_score_black;
    int knights_score_white ;
    int kings_score_black;
    int kings_score_white;
    int bonus_attack_black_king = 0;
    int bonus_attack_white_king = 0;

    if ( status == OPEN ) {
        pawns_score_black = evaluatePawn<BLACK, OPEN>();
        pawns_score_white = evaluatePawn<WHITE, OPEN>();
        bishop_score_black = evaluateBishop<BLACK, OPEN> ( structure.allPiecesSide[WHITE], structure.allPiecesSide[BLACK] );
        bishop_score_white = evaluateBishop<WHITE, OPEN> ( structure.allPiecesSide[BLACK], structure.allPiecesSide[WHITE] );
        queens_score_black =  evaluateQueen<OPEN> ( BLACK, structure.allPiecesSide[WHITE], structure.allPiecesSide[BLACK] );
        queens_score_white =  evaluateQueen<OPEN> ( WHITE, structure.allPiecesSide[BLACK], structure.allPiecesSide[WHITE] );
        rooks_score_black = evaluateRook<BLACK, OPEN> ( chessboard[KING_BLACK], structure.allPiecesSide[WHITE], structure.allPiecesSide[BLACK] );
        rooks_score_white = evaluateRook<WHITE, OPEN> ( chessboard[KING_WHITE], structure.allPiecesSide[BLACK], structure.allPiecesSide[WHITE] );
        knights_score_black = evaluateKnight<BLACK, OPEN> ( structure.pawns[WHITE], ~structure.allPiecesSide[BLACK] );
        knights_score_white = evaluateKnight<WHITE, OPEN> ( structure.pawns[BLACK], ~structure.allPiecesSide[WHITE] );
        kings_score_black = evaluateKing<OPEN> ( BLACK, ~structure.allPiecesSide[BLACK] );
        kings_score_white = evaluateKing<OPEN> ( WHITE, ~structure.allPiecesSide[WHITE] );
    }
    else {
        bonus_attack_black_king = BONUS_ATTACK_KING[bitCount ( structure.kingAttackers[WHITE] )];
        bonus_attack_white_king = BONUS_ATTACK_KING[bitCount ( structure.kingAttackers[BLACK] )];

        if ( status == END ) {
            pawns_score_black = evaluatePawn<BLACK, END>();
            pawns_score_white = evaluatePawn<WHITE, END>();
            bishop_score_black = evaluateBishop<BLACK, END> ( structure.allPiecesSide[WHITE], structure.allPiecesSide[BLACK] );
            bishop_score_white = evaluateBishop<WHITE, END> ( structure.allPiecesSide[BLACK], structure.allPiecesSide[WHITE] );
            queens_score_black =  evaluateQueen<END> ( BLACK, structure.allPiecesSide[WHITE], structure.allPiecesSide[BLACK] );
            queens_score_white =  evaluateQueen<END> ( WHITE, structure.allPiecesSide[BLACK], structure.allPiecesSide[WHITE] );
            rooks_score_black = evaluateRook<BLACK, END> ( chessboard[KING_BLACK], structure.allPiecesSide[WHITE], structure.allPiecesSide[BLACK] );
            rooks_score_white = evaluateRook<WHITE, END> ( chessboard[KING_WHITE], structure.allPiecesSide[BLACK], structure.allPiecesSide[WHITE] );
            knights_score_black = evaluateKnight<BLACK, END> ( structure.pawns[WHITE], ~structure.allPiecesSide[BLACK] );
            knights_score_white = evaluateKnight<WHITE, END> ( structure.pawns[BLACK], ~structure.allPiecesSide[WHITE] );
            kings_score_black = evaluateKing<END> ( BLACK, ~structure.allPiecesSide[BLACK] );
            kings_score_white = evaluateKing<END> ( WHITE, ~structure.allPiecesSide[WHITE] );
        }
        else {
            pawns_score_black = evaluatePawn<BLACK, MIDDLE>();
            pawns_score_white = evaluatePawn<WHITE, MIDDLE>();
            bishop_score_black = evaluateBishop<BLACK, MIDDLE> ( structure.allPiecesSide[WHITE], structure.allPiecesSide[BLACK] );
            bishop_score_white = evaluateBishop<WHITE, MIDDLE> ( structure.allPiecesSide[BLACK], structure.allPiecesSide[WHITE] );
            queens_score_black =  evaluateQueen<MIDDLE> ( BLACK, structure.allPiecesSide[WHITE], structure.allPiecesSide[BLACK] );
            queens_score_white =  evaluateQueen<MIDDLE> ( WHITE, structure.allPiecesSide[BLACK], structure.allPiecesSide[WHITE] );
            rooks_score_black = evaluateRook<BLACK, MIDDLE> ( chessboard[KING_BLACK], structure.allPiecesSide[WHITE], structure.allPiecesSide[BLACK] );
            rooks_score_white = evaluateRook<WHITE, MIDDLE> ( chessboard[KING_WHITE], structure.allPiecesSide[BLACK], structure.allPiecesSide[WHITE] );
            knights_score_black = evaluateKnight<BLACK, MIDDLE> ( structure.pawns[WHITE], ~structure.allPiecesSide[BLACK] );
            knights_score_white = evaluateKnight<WHITE, MIDDLE> ( structure.pawns[BLACK], ~structure.allPiecesSide[WHITE] );
            kings_score_black = evaluateKing<MIDDLE> ( BLACK, ~structure.allPiecesSide[BLACK] );
            kings_score_white = evaluateKing<MIDDLE> ( WHITE, ~structure.allPiecesSide[WHITE] );
        }
    }

    ASSERT ( getMobilityCastle ( WHITE, structure.allPieces ) < ( int ) ( sizeof ( MOB_CASTLE[status] ) / sizeof ( int ) ) );
    ASSERT ( getMobilityCastle ( BLACK, structure.allPieces ) < ( int ) ( sizeof ( MOB_CASTLE[status] ) / sizeof ( int ) ) );
    int mobWhite = MOB_CASTLE[status][getMobilityCastle ( WHITE, structure.allPieces )];
    int mobBlack = MOB_CASTLE[status][getMobilityCastle ( BLACK, structure.allPieces )];
    int attack_king_white = ATTACK_KING * bitCount ( structure.kingAttackers[BLACK] );
    int attack_king_black = ATTACK_KING * bitCount ( structure.kingAttackers[WHITE] );
    side == WHITE ? lazyscore_black -= 5 : lazyscore_white += 5;
    int result = ( mobBlack + attack_king_black + bonus_attack_black_king + lazyscore_black  + pawns_score_black + knights_score_black + bishop_score_black
                   + rooks_score_black + queens_score_black + kings_score_black )
                 - ( mobWhite + attack_king_white + bonus_attack_white_king + lazyscore_white  + pawns_score_white + knights_score_white + bishop_score_white
                     + rooks_score_white + queens_score_white + kings_score_white );
#ifdef DEBUG_MODEx
    cout << endl << "total..........   " << ( double ) result / 100 << endl;
    cout << "PHASE: ";

    if ( status == OPEN ) { cout << " OPEN" << endl; }
    else if ( status == MIDDLE ) { cout << " MIDDLE" << endl; }
    else { cout << " END" << endl; }

    cout << "VALUES:";
    cout << "\tPAWN: " << ( double ) VALUEPAWN / 100;
    cout << " ROOK: " << ( double ) VALUEROOK / 100;
    cout << " BISHOP: " << ( double ) VALUEBISHOP / 100;
    cout << " KNIGHT: " << ( double ) VALUEKNIGHT / 100;
    cout << " QUEEN: " << ( double ) VALUEQUEEN / 100 << endl;
    cout << "\t\t\t\tTOT (white)\tWHITE\t\tBLACK" << endl;
    cout << "material:\t\t\t\t" << ( double ) ( lazyscore_white - lazyscore_black ) / 100 << "\t\t" << ( double ) ( lazyscore_white ) / 100 << "\t\t" << ( double ) ( lazyscore_black ) / 100 << endl;
    cout << "mobility:\t\t\t\t" << ( double ) ( mobWhite - mobBlack ) / 100 << "\t\t" << ( double ) ( mobWhite ) / 100 << "\t\t" << ( double ) ( mobBlack ) / 100 << endl;
    cout << "attack king:\t\t\t" << ( double ) ( attack_king_white - attack_king_black ) / 100 << "\t\t" << ( double ) ( attack_king_white ) / 100 << "\t\t" << ( double ) ( attack_king_black ) / 100 << endl;
    cout << "bonus attack king:\t\t" << ( double ) ( bonus_attack_white_king - bonus_attack_black_king ) / 100 << "\t\t" << ( double ) ( bonus_attack_white_king ) / 100 << "\t\t" << ( double ) ( bonus_attack_black_king ) / 100 << endl;
    cout << "\npawn:\t\t\t\t\t" << ( double ) ( pawns_score_white - pawns_score_black ) / 100 << "\t\t" << ( double ) ( pawns_score_white ) / 100 << "\t\t" << ( double ) ( pawns_score_black ) / 100 << endl;
    cout << "\tmobility:\t\t\t\t\t" << ( double ) ( SCORE_DEBUG.MOB_PAWNS[WHITE] ) / 100 << "\t\t" << ( double ) ( SCORE_DEBUG.MOB_PAWNS[BLACK] ) / 100 << endl;
    cout << "\tattack king:\t\t\t\t" << ( double ) ( SCORE_DEBUG.ATTACK_KING_PAWN[WHITE] ) / 100 << "\t\t" << ( double ) ( SCORE_DEBUG.ATTACK_KING_PAWN[BLACK] ) / 100 << endl;
    cout << "\tcenter:\t\t\t\t\t" << ( double ) ( SCORE_DEBUG.PAWN_CENTER[WHITE] ) / 100 << "\t\t" << ( double ) ( SCORE_DEBUG.PAWN_CENTER[BLACK] ) / 100 << endl;
    cout << "\t7h:\t\t\t\t\t\t" << ( double ) ( SCORE_DEBUG.PAWN_7H[WHITE] ) / 100 << "\t\t" << ( double ) ( SCORE_DEBUG.PAWN_7H[BLACK] ) / 100 << endl;
    cout << "\tin race:\t\t\t\t\t" << ( double ) ( SCORE_DEBUG.PAWN_IN_RACE[WHITE] ) / 100 << "\t\t" << ( double ) ( SCORE_DEBUG.PAWN_IN_RACE[BLACK] ) / 100 << endl;
    cout << "\tblocked:\t\t\t\t\t" << ( double ) ( SCORE_DEBUG.PAWN_BLOCKED[WHITE] ) / 100 << "\t\t" << ( double ) ( SCORE_DEBUG.PAWN_BLOCKED[BLACK] ) / 100 << endl;
    cout << "\tunprotected:\t\t\t\t" << ( double ) ( SCORE_DEBUG.UNPROTECTED_PAWNS[WHITE] ) / 100 << "\t\t" << ( double ) ( SCORE_DEBUG.UNPROTECTED_PAWNS[BLACK] ) / 100 << endl;
    cout << "\tisolated\t\t\t\t\t" << ( double ) ( SCORE_DEBUG.PAWN_ISOLATED[WHITE] ) / 100 << "\t\t" << ( double ) ( SCORE_DEBUG.PAWN_ISOLATED[BLACK] ) / 100 << endl;
    cout << "\tdouble\t\t\t\t\t" << ( double ) ( SCORE_DEBUG.DOUBLED_PAWNS[WHITE] ) / 100 << "\t\t" << ( double ) ( SCORE_DEBUG.DOUBLED_PAWNS[BLACK] ) / 100 << endl;
    cout << "\tdouble isolated\t\t\t\t" << ( double ) ( SCORE_DEBUG.DOUBLED_ISOLATED_PAWNS[WHITE] ) / 100 << "\t\t" << ( double ) ( SCORE_DEBUG.DOUBLED_ISOLATED_PAWNS[BLACK] ) / 100 << endl;
    cout << "\tbackward\t\t\t\t\t" << ( double ) ( SCORE_DEBUG.BACKWARD_PAWN[WHITE] ) / 100 << "\t\t" << ( double ) ( SCORE_DEBUG.BACKWARD_PAWN[BLACK] ) / 100 << endl;
    cout << "\tfork:\t\t\t\t\t\t" << ( double ) ( SCORE_DEBUG.FORK_SCORE[WHITE] ) / 100 << "\t\t" << ( double ) ( SCORE_DEBUG.FORK_SCORE[BLACK] ) / 100 << endl;
    cout << "\tpassed:\t\t\t\t\t" << ( double ) ( SCORE_DEBUG.PAWN_PASSED[WHITE] ) / 100 << "\t\t" << ( double ) ( SCORE_DEBUG.PAWN_PASSED[BLACK] ) / 100 << endl;
    cout << "\tall enemies:\t\t\t\t" << ( double ) ( SCORE_DEBUG.ENEMIES_PAWNS_ALL[WHITE] ) / 100 << "\t\t" << ( double ) ( SCORE_DEBUG.ENEMIES_PAWNS_ALL[BLACK] ) / 100 << endl;
    cout << "\tnone:\t\t\t\t\t\t" << ( double ) ( SCORE_DEBUG.NO_PAWNS[WHITE] ) / 100 << "\t\t" << ( double ) ( SCORE_DEBUG.NO_PAWNS[BLACK] ) / 100 << endl;
    cout << "\nknight:\t\t\t\t" << ( double ) ( knights_score_white - knights_score_black ) / 100 << "\t\t" << ( double ) ( knights_score_white ) / 100 << "\t\t" << ( double ) ( knights_score_black ) / 100 << endl;
    cout << "\tundevelop:\t\t\t\t\t" << ( double ) ( SCORE_DEBUG.UNDEVELOPED_KNIGHT[WHITE] ) / 100 << "\t\t" << ( double ) ( SCORE_DEBUG.UNDEVELOPED_KNIGHT[BLACK] ) / 100 << endl;
    cout << "\ttrapped:\t\t\t\t\t" << ( double ) ( SCORE_DEBUG.KNIGHT_TRAPPED[WHITE] ) / 100 << "\t\t" << ( double ) ( SCORE_DEBUG.KNIGHT_TRAPPED[BLACK] ) / 100 << endl;
    cout << "\tmobility:\t\t\t\t\t" << ( double ) ( SCORE_DEBUG.MOB_KNIGHT[WHITE] ) / 100 << "\t\t" << ( double ) ( SCORE_DEBUG.MOB_KNIGHT[BLACK] ) / 100 << endl;
    cout << "\nbishop:\t\t\t\t" << ( double ) ( bishop_score_white - bishop_score_black ) / 100 << "\t\t" << ( double ) ( bishop_score_white ) / 100 << "\t\t" << ( double ) ( bishop_score_black ) / 100 << endl;
    cout << "\tbad:\t\t\t\t\t\t" << ( double ) ( SCORE_DEBUG.BAD_BISHOP[WHITE] ) / 100 << "\t\t" << ( double ) ( SCORE_DEBUG.BAD_BISHOP[BLACK] ) / 100 << endl;
    cout << "\tmobility:\t\t\t\t\t" << ( double ) ( SCORE_DEBUG.MOB_BISHOP[WHITE] ) / 100 << "\t\t" << ( double ) ( SCORE_DEBUG.MOB_BISHOP[BLACK] ) / 100 << endl;
    cout << "\tundevelop:\t\t\t\t\t" << ( double ) ( SCORE_DEBUG.UNDEVELOPED_BISHOP[WHITE] ) / 100 << "\t\t" << ( double ) ( SCORE_DEBUG.UNDEVELOPED_BISHOP[BLACK] ) / 100 << endl;
    cout << "\topen diag:\t\t\t\t\t" << ( double ) ( SCORE_DEBUG.OPEN_DIAG_BISHOP[WHITE] ) / 100 << "\t\t" << ( double ) ( SCORE_DEBUG.OPEN_DIAG_BISHOP[BLACK] ) / 100 << endl;
    cout << "\tbonus 2 bishops:\t\t\t\t" << ( double ) ( SCORE_DEBUG.BONUS2BISHOP[WHITE] ) / 100 << "\t\t" << ( double ) ( SCORE_DEBUG.BONUS2BISHOP[BLACK] ) / 100 << endl;
    cout << "\nrook:\t\t\t\t\t" << ( double ) ( rooks_score_white - rooks_score_black ) / 100 << "\t\t" << ( double ) ( rooks_score_white ) / 100 << "\t\t" << ( double ) ( rooks_score_black ) / 100 << endl;
    cout << "\t7th:\t\t\t\t\t\t" << ( double ) ( SCORE_DEBUG.ROOK_7TH_RANK[WHITE] ) / 100 << "\t\t" << ( double ) ( SCORE_DEBUG.ROOK_7TH_RANK[BLACK] ) / 100 << endl;
    cout << "\ttrapped:\t\t\t\t\t" << ( double ) ( SCORE_DEBUG.ROOK_TRAPPED[WHITE] ) / 100 << "\t\t" << ( double ) ( SCORE_DEBUG.ROOK_TRAPPED[BLACK] ) / 100 << endl;
    cout << "\tmobility:\t\t\t\t\t" << ( double ) ( SCORE_DEBUG.MOB_ROOK[WHITE] ) / 100 << "\t\t" << ( double ) ( SCORE_DEBUG.MOB_ROOK[BLACK] ) / 100 << endl;
    cout << "\tblocked:\t\t\t\t\t" << ( double ) ( SCORE_DEBUG.ROOK_BLOCKED[WHITE] ) / 100 << "\t\t" << ( double ) ( SCORE_DEBUG.ROOK_BLOCKED[BLACK] ) / 100 << endl;
    cout << "\topen file:\t\t\t\t\t" << ( double ) ( SCORE_DEBUG.ROOK_OPEN_FILE[WHITE] ) / 100 << "\t\t" << ( double ) ( SCORE_DEBUG.ROOK_OPEN_FILE[BLACK] ) / 100 << endl;
    cout << "\tconnected:\t\t\t\t\t" << ( double ) ( SCORE_DEBUG.CONNECTED_ROOKS[WHITE] ) / 100 << "\t\t" << ( double ) ( SCORE_DEBUG.CONNECTED_ROOKS[BLACK] ) / 100 << endl;
    cout << "\nqueen:\t\t\t\t" << ( double ) ( queens_score_white - queens_score_black ) / 100 << "\t\t" << ( double ) ( queens_score_white ) / 100 << "\t\t" << ( double ) ( queens_score_black ) / 100 << endl;
    cout << "\tmobility:\t\t\t\t\t" << ( double ) ( SCORE_DEBUG.MOB_QUEEN[WHITE] ) / 100 << "\t\t" << ( double ) ( SCORE_DEBUG.MOB_QUEEN[BLACK] ) / 100 << endl;
    cout << "\topen file:\t\t\t\t\t" << ( double ) ( SCORE_DEBUG.OPEN_FILE_Q[WHITE] ) / 100 << "\t\t" << ( double ) ( SCORE_DEBUG.OPEN_FILE_Q[BLACK] ) / 100 << endl;
    cout << "\tbishop on queen:\t\t\t\t" << ( double ) ( SCORE_DEBUG.BISHOP_ON_QUEEN[WHITE] ) / 100 << "\t\t" << ( double ) ( SCORE_DEBUG.BISHOP_ON_QUEEN[BLACK] ) / 100 << endl;
    cout << "\tsemi open file:\t\t\t\t" << ( double ) ( SCORE_DEBUG.HALF_OPEN_FILE_Q[WHITE] ) / 100 << "\t\t" << ( double ) ( SCORE_DEBUG.HALF_OPEN_FILE_Q[BLACK] ) / 100 << endl;
    cout << "\nking:\t\t\t\t\t" << ( double ) ( kings_score_white - kings_score_black ) / 100 << "\t\t" << ( double ) ( kings_score_white ) / 100 << "\t\t" << ( double ) ( kings_score_black ) / 100 << endl;
    cout << "\tdistance:\t\t\t\t\t" << ( double ) ( SCORE_DEBUG.DISTANCE_KING[WHITE] ) / 100 << "\t\t" << ( double ) ( SCORE_DEBUG.DISTANCE_KING[BLACK] ) / 100 << endl;
    cout << "\topen file:\t\t\t\t\t" << ( double ) ( SCORE_DEBUG.END_OPENING_KING[WHITE] ) / 100 << "\t\t" << ( double ) ( SCORE_DEBUG.END_OPENING_KING[BLACK] ) / 100 << endl;
    cout << "\tpawn near:\t\t\t\t\t" << ( double ) ( SCORE_DEBUG.PAWN_NEAR_KING[WHITE] ) / 100 << "\t\t" << ( double ) ( SCORE_DEBUG.PAWN_NEAR_KING[BLACK] ) / 100 << endl;
    cout << "\tmobility:\t\t\t\t\t" << ( double ) ( SCORE_DEBUG.MOB_KING[WHITE] ) / 100 << "\t\t" << ( double ) ( SCORE_DEBUG.MOB_KING[BLACK] ) / 100 << endl;
    cout << "\tsecurity bishop:\t\t\t\t" << ( double ) ( SCORE_DEBUG.KING_SECURITY_BISHOP[WHITE] ) / 100 << "\t\t" << ( double ) ( SCORE_DEBUG.KING_SECURITY_BISHOP[BLACK] ) / 100 << endl;
    cout << "\tsecurity queen:\t\t\t\t" << ( double ) ( SCORE_DEBUG.KING_SECURITY_QUEEN[WHITE] ) / 100 << "\t\t" << ( double ) ( SCORE_DEBUG.KING_SECURITY_QUEEN[BLACK] ) / 100 << endl;
    cout << "\tsecurity knight:\t\t\t\t" << ( double ) ( SCORE_DEBUG.KING_SECURITY_KNIGHT[WHITE] ) / 100 << "\t\t" << ( double ) ( SCORE_DEBUG.KING_SECURITY_KNIGHT[BLACK] ) / 100 << endl;
    cout << "\tsecurity rook:\t\t\t\t" << ( double ) ( SCORE_DEBUG.KING_SECURITY_ROOK[WHITE] ) / 100 << "\t\t" << ( double ) ( SCORE_DEBUG.KING_SECURITY_ROOK[BLACK] ) / 100 << endl;
    cout << endl;
#endif
    return side ? -result : result;
}

