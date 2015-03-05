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

#include "Search.h"

Search::Search() : ponder ( false ), nullSearch ( false ) {
#ifdef DEBUG_MODE
    LazyEvalCuts = cumulativeMovesCount = totGen = 0;
#endif
    gtb = nullptr;
}

void Search::setNullMove ( bool b ) {
    nullSearch = !b;
}

void Search::startClock() {
    ftime ( &startTime );
}

void Search::setMainPly ( int m ) {
    mainDepth = m;
}

int Search::checkTime() {
    if ( running == 2 ) {
        return 2;
    }

    if ( ponder ) {
        return 1;
    }

    struct timeb t_current;

    ftime ( &t_current );

    return _time::diffTime ( t_current , startTime ) >= maxTimeMillsec ? 0 : 1;
}

Search::~Search() {
    if ( gtb ) {
        delete gtb;
    }
}

template <int side>
int Search::quiescence ( int alpha, int beta, const char promotionPiece, int N_PIECE, int depth ) {
    if ( !running ) {
        return 0;
    }

    ASSERT ( chessboard[KING_BLACK + side] );
    int score = -_INFINITE;

    if ( ! ( numMovesq++ & 1023 ) ) {
        running = checkTime();
    }

    score = getScore ( side, alpha, beta );

    if ( score >= beta ) {
        return beta;
    }

    ///************* hash ****************
    _Thash * phashe_always = nullptr;
    _Thash* phashe;
    bool hash_greater = false;
    bool hash_always = false;
    char hashf = hashfALPHA;
    u64 zobristKeyR = zobristKey ^ RANDSIDE[side];
    _Thash * phashe_greater = phashe = & ( hash_array_greater[zobristKeyR % HASH_SIZE] );

    for ( int i = 0; i < 2; i++ ) {
        if ( phashe->key == zobristKeyR ) {
            if ( phashe->from != phashe->to && phashe->flags & 0x3 ) { // hashfEXACT or hashfBETA
                !i ? hash_greater = true : hash_always = true;
            }

            if ( phashe->depth >= depth ) {
                INC ( probeHash );

                if ( !currentPly ) {
                    if ( phashe->flags == hashfBETA ) {
                        incKillerHeuristic ( phashe->from, phashe->to, 1 );
                    }
                }
                else {
                    if ( phashe->flags == hashfALPHA ) {
                        if ( phashe->score <= alpha ) {
                            INC ( n_cut_hashA );
                            return alpha;
                        }
                    }
                    else {
                        ASSERT ( phashe->flags == hashfEXACT || phashe->flags == hashfBETA );

                        if ( phashe->score >= beta ) {
                            INC ( n_cut_hashB );
                            return beta;
                        }
                    }
                }

                INC ( cutFailed );
            }
        }

        phashe_always = phashe = & ( hash_array_always[zobristKeyR % HASH_SIZE] );
    }

    ///********** end hash ***************
    /**************Delta Pruning ****************/
    char fprune = 0;
    int fscore;

    if ( ( fscore = score + ( promotionPiece == NO_PROMOTION ? VALUEQUEEN : 2 * VALUEQUEEN ) ) < alpha ) {
        fprune = 1;
    }

    /************ end Delta Pruning *************/
    if ( score > alpha ) {
        alpha = score;
    }

    incListId();
    u64 friends = getBitBoard<side>();
    u64 enemies = getBitBoard<side^1>();

    if ( generateCaptures<side> ( enemies, friends ) ) {
        decListId();
        return _INFINITE - ( mainDepth + depth );
    }

    if ( !getListSize() ) {
        --listId;
        return score;
    }

    _Tmove * move;
    _Tmove* best = nullptr;
    u64 oldKey = zobristKey;

    if ( hash_greater ) {
        sortHashMoves ( listId, phashe_greater );
    }
    else if ( hash_always ) {
        sortHashMoves ( listId, phashe_always );
    }

    while ( ( move = getNextMove ( &gen_list[listId] ) ) ) {
        if ( !makemove ( move, false, true ) ) {
            takeback ( move, oldKey, false );
            continue;
        }

        /**************Delta Pruning ****************/
        if ( fprune && ( ( move->type & 0x3 ) != PROMOTION_MOVE_MASK )
                && fscore + PIECES_VALUE[move->capturedPiece] <= alpha  ) {
            INC ( nCutFp );
            takeback ( move, oldKey, false );
            continue;
        }

        /************ end Delta Pruning *************/
        int val = -quiescence<side^1> ( -beta,  -alpha, move->promotionPiece, N_PIECE - 1, depth - 1 );
        score = max ( score, val );
        takeback ( move,  oldKey, false );

        if ( score > alpha ) {
            if ( score >= beta ) {
                decListId();
                recordHash ( running, phashe_greater, phashe_always, depth , hashfBETA, zobristKeyR, score, move );
                return beta;
            }

            best = move;
            alpha = score;
            hashf = hashfEXACT;
        }
    }

    recordHash ( running, phashe_greater, phashe_always, depth , hashf, zobristKeyR, score, best );
    decListId();
    return score;
}

void Search::setPonder ( bool r ) {
    ponder = r;
}

void Search::setRunning ( int r ) {
    running = r;

    if ( !r ) { maxTimeMillsec = 0; }
}

int Search::getRunning() {
    return running;
}

void Search::setMaxTimeMillsec ( int n ) {
    maxTimeMillsec = n;
}

int Search::getMaxTimeMillsec() {
    return maxTimeMillsec;
}

void Search::sortHashMoves ( int listId1, _Thash* phashe ) {
    for ( int r = 0; r < gen_list[listId1].size; r++ ) {
        _Tmove * mos = &gen_list[listId1].moveList[r];

        if ( phashe && phashe->from == mos->from && phashe->to == mos->to ) {
            mos->score = _INFINITE / 2;
            return;
        }
    }
}
bool Search::checkInsufficientMaterial ( int N_PIECE ) {
    //regexp: KN?B*KB*
    if ( N_PIECE > 6 ) { return false; }

    // KK
    if ( N_PIECE == 2 ) { return true; }

    if ( !chessboard[PAWN_BLACK] && !chessboard[PAWN_WHITE] && !chessboard[ROOK_BLACK] && !chessboard[ROOK_WHITE] && !chessboard[QUEEN_WHITE]
            && !chessboard[QUEEN_BLACK] ) {
        u64 allBishop = chessboard[BISHOP_BLACK] | chessboard[BISHOP_WHITE];
        u64 allKnight = chessboard[KNIGHT_BLACK] | chessboard[KNIGHT_WHITE];

        if ( allBishop || allKnight ) {
            //insufficient material to mate
            if ( !allKnight ) {
                //regexp: KB+KB*
                if ( ( bitCount ( allBishop ) == 1 ) || ( ( allBishop & BLACK_SQUARES ) == allBishop || ( allBishop & WHITE_SQUARES ) == allBishop ) ) {
                    return true;
                }
            }
            else {
                //KNKN*
                if ( !allBishop && bitCount ( allKnight ) < 3 ) {
                    return true;
                }
            }
        }
    }

    return false;
}



bool Search::checkDraw ( u64 key ) {
    int o = 0;
    int count = 0;

    for ( int i = repetitionMapCount - 1; i >= 0; i-- ) {
        if ( repetitionMap[i] == 0 ) {
            return false;
        }

        if ( ++count >= 99 ) {
            return true;
        }

        if ( repetitionMap[i] == key && ++o > 2 ) {
            return true;
        }
    }

    return false;
}

bool Search::getGtbAvailable() {
    return gtb;
}

void Search::createGtb() {
    if ( !gtb ) {
        gtb = new Tablebase();
    }
}

Tablebase&  Search::getGtb() const {
    return *gtb;
}

void  Search::deleteGtb() {
    if ( gtb ) { delete gtb; }

    gtb = nullptr;
}

int Search::search ( int depth, int alpha, int beta, _TpvLine * pline, int *mateIn ) {
    return getSide() ?
           search<WHITE> ( depth, alpha, beta, pline, bitCount ( getBitBoard<WHITE>() | getBitBoard<BLACK>() ), mateIn )
           :
           search<BLACK> ( depth, alpha, beta, pline, bitCount ( getBitBoard<WHITE>() | getBitBoard<BLACK>() ) , mateIn );
}

template <int side>
int Search::search ( int depth, int alpha, int beta, _TpvLine * pline, int N_PIECE, int *mateIn ) {
    ASSERT_RANGE ( side, 0, 1 );

    if ( !running ) {
        return 0;
    }

    int score = -_INFINITE;

    /* gtb */
    if ( gtb && pline->cmove && maxTimeMillsec > 1000 && gtb->isInstalledPieces ( N_PIECE ) && depth >= gtb->getProbeDepth ( ) ) {
        int v = gtb->getDtm <side, false> ( chessboard , rightCastle, depth );

        if ( abs ( v ) != INT_MAX ) {
            *mateIn = v;
            int res = 0;

            if ( v == 0 ) {res = 0; }
            else {
                res = _INFINITE - ( abs ( v ) );
                if ( v < 0 ) {res = -res; }
            }

            ASSERT ( res >= -_INFINITE && res <= _INFINITE );
            ASSERT ( mainDepth >= depth );
            cout << side << " " << ( *mateIn ) << " " << res << endl;
            return res;
        }
    }

    u64 oldKey = zobristKey;
    INC ( cumulativeMovesCount );
#ifdef DEBUG_MODE
    double betaEfficiencyCount = 0.0;
#endif
    ASSERT ( chessboard[KING_BLACK] );
    ASSERT ( chessboard[KING_WHITE] );
    ASSERT ( chessboard[KING_BLACK + side] );
    int extension = 0;
    int is_incheck_side = inCheck<side>();

    if ( !is_incheck_side && depth != mainDepth ) {
        if ( checkInsufficientMaterial ( N_PIECE ) ) {
            if ( inCheck<side^1>() ) { return _INFINITE - ( mainDepth - depth + 1 ); }

            return -lazyEval<side>() * 2;
        }

        if ( checkDraw ( zobristKey ) ) {
            return -lazyEval<side>() * 2;
        }
    }

    if ( is_incheck_side ) {
        extension++;
    }

    depth += extension;

    if ( depth == 0 ) {
        return quiescence<side> ( alpha,  beta, -1, N_PIECE, 0 );
    }

    //************* hash ****************
    bool hash_greater = false;
    u64 zobristKeyR = zobristKey ^ RANDSIDE[side];
    _Thash * phashe_greater = & ( hash_array_greater[zobristKeyR % HASH_SIZE] );

    if ( phashe_greater->key == zobristKeyR ) {
        if ( phashe_greater->from != phashe_greater->to && phashe_greater->flags & 0x3 ) { // hashfEXACT or hashfBETA
            hash_greater = true;
        }

        if ( phashe_greater->depth >= depth ) {
            INC ( probeHash );

            if ( !currentPly ) {
                if ( phashe_greater->flags == hashfBETA ) { incKillerHeuristic ( phashe_greater->from, phashe_greater->to, 1 ); }
            }
            else {
                switch ( phashe_greater->flags ) {
                    case hashfEXACT:
                        INC ( n_cut_hashE );

                        if ( phashe_greater->score >= beta ) {
                            return beta;
                        }

                        break;

                    case hashfBETA:
                        incKillerHeuristic ( phashe_greater->from, phashe_greater->to, 1 );

                        if ( phashe_greater->score >= beta ) {
                            INC ( n_cut_hashB );
                            return beta;
                        }

                        break;

                    case hashfALPHA:
                        if ( phashe_greater->score <= alpha ) {
                            INC ( n_cut_hashA );
                            return alpha;
                        }

                        break;

                    default:
                        break;
                }

                INC ( cutFailed );
            }
        }
    }

    bool hash_always = false;
    _Thash * phashe_always = & ( hash_array_always[zobristKeyR % HASH_SIZE] );

    if ( phashe_always->key == zobristKeyR ) {
        if ( phashe_always->from != phashe_always->to && phashe_always->flags & 0x3 ) { // hashfEXACT or hashfBETA
            hash_always = true;
        }

        if ( phashe_always->depth >= depth ) {
            INC ( probeHash );

            if ( !currentPly ) {
                if ( phashe_always->flags == hashfBETA ) { incKillerHeuristic ( phashe_always->from, phashe_always->to, 1 ); }
            }
            else {
                switch ( phashe_always->flags ) {
                    case hashfEXACT:
                        INC ( n_cut_hashE );

                        if ( phashe_always->score >= beta ) {
                            return beta;
                        }

                        break;

                    case hashfBETA:
                        incKillerHeuristic ( phashe_always->from, phashe_always->to, 1 );

                        if ( phashe_always->score >= beta ) {
                            INC ( n_cut_hashB );
                            return beta;
                        }

                        break;

                    case hashfALPHA:
                        if ( phashe_always->score <= alpha ) {
                            INC ( n_cut_hashA );
                            return alpha;
                        }

                        break;

                    default:
                        break;
                }

                INC ( cutFailed );
            }
        }
    }

    ///********** end hash ***************
    if ( ! ( numMoves & 1023 ) ) {
        running = checkTime();
    }

    ++numMoves;
    ///********* null move ***********
    int n_pieces_side;
    _TpvLine line;
    line.cmove = 0;

    if ( !is_incheck_side && !nullSearch && depth >= NULLMOVE_DEPTH && ( n_pieces_side = getNpiecesNoPawnNoKing<side>() ) >= NULLMOVES_MIN_PIECE ) {
        nullSearch = true;
        int nullScore = -search<side^1> ( depth - ( NULLMOVES_R1 + ( depth > ( NULLMOVES_R2 + ( n_pieces_side < NULLMOVES_R3 ? NULLMOVES_R4 : 0 ) ) ) ) - 1, -beta, -beta + 1, &line, N_PIECE, mateIn );
        nullSearch = false;

        if ( nullScore >= beta ) {
            INC ( nNullMoveCut );
            return nullScore;
        }
    }

    ///******* null move end ********
    /**************Futility Pruning****************/
    /**************Futility Pruning razor at pre-pre-frontier*****/
    bool futilPrune = false;
    int futilScore = 0;

    if ( depth <= 3 && !is_incheck_side ) {
        int matBalance =  lazyEval<side>();

        if ( ( futilScore = matBalance + FUTIL_MARGIN ) <= alpha ) {
            if ( depth == 3 && ( matBalance + RAZOR_MARGIN ) <= alpha && getNpiecesNoPawnNoKing<side^1>() > 3 ) {
                INC ( nCutRazor );
                depth--;
            }
            else

                ///**************Futility Pruning at pre-frontier*****
                if ( depth == 2 && ( futilScore = matBalance + EXT_FUTILY_MARGIN ) <= alpha ) {
                    futilPrune = true;
                    score = futilScore;
                }
                else

                    ///**************Futility Pruning at frontier*****
                    if ( depth == 1 /*&& (futilScore = matBalance + FUTIL_MARGIN) <= alpha*/ ) {
                        futilPrune = true;
                        score = futilScore;
                    }
        }
    }

    /************ end Futility Pruning*************/
    incListId();
    ASSERT ( KING_BLACK + side >= 0 && KING_BLACK + side < 12 );
    ASSERT ( KING_BLACK + ( side ^ 1 ) >= 0 && KING_BLACK + ( side ^ 1 ) < 12 );
    friendKing[side] = BITScanForward ( chessboard[KING_BLACK + side] );
    friendKing[side ^ 1] = BITScanForward ( chessboard[KING_BLACK + ( side ^ 1 )] );
    u64 friends = getBitBoard<side>();
    u64 enemies = getBitBoard<side^1>();

    if ( generateCaptures<side> ( enemies, friends ) ) {
        decListId();
        score = _INFINITE - ( mainDepth - depth + 1 );
        return score;
    }

    generateMoves<side> ( friends | enemies );
    int listcount = getListSize();

    if ( !listcount ) {
        --listId;

        if ( is_incheck_side ) {
            return -_INFINITE + ( mainDepth - depth + 1 );
        }
        else {
            return -lazyEval<side>() * 2;
        }
    }


    _Tmove* best = nullptr;

    if ( hash_greater ) {
        sortHashMoves ( listId, phashe_greater );
    }
    else if ( hash_always ) {
        sortHashMoves ( listId, phashe_always );
    }

    INC ( totGen );
    _Tmove * move;
    bool checkInCheck = false;
    int countMove = 0;
    char hashf = hashfALPHA;

    while ( ( move = getNextMove ( &gen_list[listId] ) ) ) {
        countMove++;
        INC ( betaEfficiencyCount );

        if ( !makemove ( move, true, checkInCheck ) ) {
            takeback ( move, oldKey, true );
            continue;
        }

        checkInCheck = true;

        if ( futilPrune  && ( ( move->type & 0x3 ) != PROMOTION_MOVE_MASK )
                && futilScore + PIECES_VALUE[move->capturedPiece] <= alpha && !inCheck<side>() ) {
            INC ( nCutFp );
            takeback ( move,  oldKey, true );
            continue;
        }

        //Late Move Reduction
        int val = INT_MAX;

        if ( countMove > 4 && !is_incheck_side && depth >= 3 && move->capturedPiece == SQUARE_FREE && move->promotionPiece == NO_PROMOTION ) {
            currentPly++;
            val = -search<side^1> ( depth - 2, - ( alpha + 1 ), -alpha, &line, N_PIECE, mateIn );
            ASSERT ( val != INT_MAX );
            currentPly--;
        }

        if ( val > alpha ) {
            int doMws = ( score > -_INFINITE + 100 );
            int lwb = max ( alpha, score );
            int upb = ( doMws ? ( lwb + 1 ) : beta );
            currentPly++;
            val = -search<side^1> ( depth - 1, -upb, -lwb, &line, move->capturedPiece == SQUARE_FREE ? N_PIECE : N_PIECE - 1, mateIn );
            ASSERT ( val != INT_MAX );
            currentPly--;

            if ( doMws && ( lwb < val ) && ( val < beta ) ) {
                currentPly++;
                val = -search<side^1> ( depth - 1, -beta, -val + 1, &line, move->capturedPiece == SQUARE_FREE ? N_PIECE : N_PIECE - 1, mateIn );
                currentPly--;
            }
        }

        score = max ( score, val );
        takeback ( move, oldKey, true );
        move->score = score;

        if ( score > alpha ) {
            if ( score >= beta ) {
                decListId();
                ASSERT ( move->score == score );
                INC ( nCutAB );
                ADD ( betaEfficiency, betaEfficiencyCount / ( double ) listcount * 100.0 );
                recordHash ( running, phashe_greater, phashe_always, depth - extension, hashfBETA, zobristKeyR, score, move );
                setKillerHeuristic ( move->from, move->to, 0x400 );
                return score;
            }

            alpha = score;
            hashf = hashfEXACT;
            best = move;
            move->score = score;
            updatePv ( pline, &line, move );
        }
    }

    recordHash ( running, phashe_greater, phashe_always, depth - extension, hashf, zobristKeyR, score, best );
    decListId();
    return score;
}

void Search::updatePv ( _TpvLine * pline, const _TpvLine * line, const _Tmove * move ) {
    ASSERT ( line->cmove < MAX_PLY - 1 );
    memcpy ( & ( pline->argmove[0] ), move, sizeof ( _Tmove ) );
    memcpy ( pline->argmove + 1, line->argmove, line->cmove * sizeof ( _Tmove ) );
    ASSERT ( line->cmove >= 0 );
    pline->cmove = line->cmove + 1;
}
