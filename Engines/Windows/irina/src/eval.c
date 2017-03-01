#include "defs.h"
#include "protos.h"
#include "globals.h"

int eval() {
    int score, square;
    int whitepawns, whiteknights, whitebishops, whiterooks, whitequeens, whitetotal;
    int blackpawns, blackknights, blackbishops, blackrooks, blackqueens, blacktotal;
    int totalpawns;
    int whitetotalmat, blacktotalmat, totalmat;
    int whitekingsquare, blackkingsquare;
    int valpawn, valknight, valbishop, valrook, valqueen;
    // int whitepassedpawns, blackpassedpawns;
    //bool opening, middlegame; endgame;
    bool endgame;
    Bitmap temp;
    int q;

    if( hash_probe(&score) ) return score;

    whitepawns = bit_count(board.white_pawns);
    whiteknights = bit_count(board.white_knights);
    whitebishops = bit_count(board.white_bishops);
    whiterooks = bit_count(board.white_rooks);
    whitequeens = bit_count(board.white_queens);
    whitetotalmat = 3 * whiteknights + 3 * whitebishops + 5 * whiterooks + 10 * whitequeens;
    whitetotal = whitepawns + whiteknights + whitebishops + whiterooks + whitequeens;
    blackpawns = bit_count(board.black_pawns);
    blackknights = bit_count(board.black_knights);
    blackbishops = bit_count(board.black_bishops);
    blackrooks = bit_count(board.black_rooks);
    blackqueens = bit_count(board.black_queens);
    blacktotalmat = 3 * blackknights + 3 * blackbishops + 5 * blackrooks + 10 * blackqueens;
    blacktotal = blackpawns + blackknights + blackbishops + blackrooks + blackqueens;


    // ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    // Remember where the kings are
    // ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    if (board.white_king) {
        whitekingsquare = first_one(board.white_king);
    } else {
        return (board.color) ? MATESCORE : -MATESCORE;
    }
    if (board.black_king) {
        blackkingsquare = first_one(board.black_king);
    } else {
        return (board.color) ? -MATESCORE : +MATESCORE;
    }

    // ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    // Evaluate for draws due to insufficient material:
    // ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    if (!whitepawns && !blackpawns) {
        // king versus king:
        if ((whitetotalmat == 0) && (blacktotalmat == 0)) {
            if (board.color) {
                return -DRAWSCORE;
            } else {
                return DRAWSCORE;
            }
        }

        // king and knight versus king:
        if (((whitetotalmat == 3) && (whiteknights == 1) && (blacktotalmat == 0)) ||
                ((blacktotalmat == 3) && (blackknights == 1) && (whitetotalmat == 0))) {
            if (board.color) {
                return -DRAWSCORE;
            } else {
                return DRAWSCORE;
            }
        }

        // 2 kings with one or more bishops, and all bishops on the same colour:
        if ((whitebishops + blackbishops) > 0) {
            if ((whiteknights == 0) && (whiterooks == 0) && (whitequeens == 0) &&
                    (blackknights == 0) && (blackrooks == 0) && (blackqueens == 0)) {
                if (!((board.white_bishops | board.black_bishops) & WHITE_SQUARES) ||
                        !((board.white_bishops | board.black_bishops) & BLACK_SQUARES)) {
                    return DRAWSCORE;
                }
            }
        }
    }

    // ----------------------------------------------------------------------------
    // Value of pieces
    // ----------------------------------------------------------------------------
    totalpawns = whitepawns + blackpawns;

    // closed positions - 13-16 ps left
    if( totalpawns >= 13 ){
        valqueen = QUEEN_VALUE*90/100;
        valrook = ROOK_VALUE*85/100;
        valbishop = BISHOP_VALUE;
        valknight = KNIGHT_VALUE*110/100;
    }
    // semi-closed positions - 9-12 ps left
    else if( totalpawns >= 9){
        valqueen = QUEEN_VALUE*95/100;
        valrook = ROOK_VALUE*90/100;
        valbishop = BISHOP_VALUE*105/100;
        valknight = KNIGHT_VALUE*110/100;
    }
    // semi-open positions - 5-8 ps
    else if( totalpawns >= 5){
        valqueen = QUEEN_VALUE*120/100;
        valrook = ROOK_VALUE*110/100;
        valbishop = BISHOP_VALUE*115/100;
        valknight = KNIGHT_VALUE*90/100;
    }
    // open positions - 0-4 ps
    else {
        valqueen = QUEEN_VALUE*130/100;
        valrook = ROOK_VALUE*110/100;
        valbishop = BISHOP_VALUE*120/100;
        valknight = KNIGHT_VALUE*85/100;
    }

    // Grading of pawns
    // Pawns will be graded in relation to the pieces left on the board (defining middlegame or
    // endgame; if 60 is the overall piece strength, then middlegame starts from 30 piece strength
    // upwards, and endgame downwards). It is obvious that with decreasing piece strength left ps
    // become gradually more powerful, in respect to their structure, passer status and influence on
    // the board.
    // Pawns might be graded in four categories in decreasing order:
    // Piece strength 60-45 - no change from standard value
    // Piece strength 45-30 - +5% standard value
    // Piece strength 30-15 - +10% standard value
    // Piece strength 15-0 - +15%
    totalmat = whitetotalmat+blacktotalmat;

    if( totalmat >= 48 ) valpawn = PAWN_VALUE;
    else if( totalmat >= 32 ) valpawn = PAWN_VALUE*105/100;
    else if( totalmat >= 16 ) valpawn = PAWN_VALUE*110/100;
    else valpawn = PAWN_VALUE*115/100;


    // ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    // Check if we are in the endgame
    // Anything less than a queen (=10) + rook (=5) is considered endgame
    // (pawns excluded in this count)
    // ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    endgame = (whitetotalmat < 15 || blacktotalmat < 15);


    // ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    // Evaluate MATERIAL
    // ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    score = (whitepawns-blackpawns) * valpawn +
            (whiteknights-blackknights) * valknight +
            (whitebishops-blackbishops) * valbishop +
            (whiterooks-blackrooks) * valrook +
            (whitequeens-blackqueens) * valqueen;

    // ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    // Have the winning side prefer to exchange pieces
    // Every exchange with unequal material adds 3 centipawns to the score
    // Loosing a piece (from balanced material) becomes more
    // severe in the endgame
    // ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    if (whitetotalmat + whitepawns > blacktotalmat + blackpawns) {
        score += 45 + 3 * whitetotal - 6 * blacktotal;
    } else if (whitetotalmat + whitepawns < blacktotalmat + blackpawns) {
        score += -45 - 3 * blacktotal + 6 * whitetotal;
    }

    // ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    // Evaluate WHITE PIECES
    // ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

    // ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    // Evaluate white pawns
    // ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

    // whitepassedpawns = 0;
    temp = board.white_pawns;
    while (temp) {
        square = first_one(temp);

        // - position on the board
        score += PAWNPOS_W[square];

        // - distance from opponent king
        score += PAWN_OPPONENT_DISTANCE[DISTANCE[square][blackkingsquare]];

        // - distance from own king
        if (endgame) score += PAWN_OWN_DISTANCE[DISTANCE[square][whitekingsquare]];

        // - passed, doubled, isolated or backward pawns
        if (!(PASSED_WHITE[square] & board.black_pawns)) {
            score += BONUS_PASSED_PAWN;
            // whitepassedpawns ^= BITSET[square];
        }

        if (board.white_pawns & PASSED_WHITE[square]) score -= PENALTY_DOUBLED_PAWN;

        if (!(ISOLATED_WHITE[square] & board.white_pawns)) score -= PENALTY_ISOLATED_PAWN;
        else {
             // If it is not isolated, then it might be backward. Two conditions must be true:
             //  1) if the next square is controlled by an enemy pawn - we use the PAWN_ATTACKS bitmaps to check this
             //  2) if there are no pawns left that could defend this pawn
             if ((WHITE_PAWN_ATTACKS[square + 8] & board.black_pawns) && !(BACKWARD_WHITE[square] & board.white_pawns) )
                score -= PENALTY_BACKWARD_PAWN;
        }

        temp ^= BITSET[square];
    }

    // ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    // Evaluate white knights
    // - position on the board
    // - distance from opponent king
    // ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

    temp = board.white_knights;
    while (temp) {
        square = first_one(temp);
        score += KNIGHTPOS_W[square];
        score += KNIGHT_DISTANCE[DISTANCE[square][blackkingsquare]];
        temp ^= BITSET[square];
    }

    // ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    // Evaluate white bishops
    // - having the pair
    // - position on the board
    // - distance from opponent king
    // ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

    temp = board.white_bishops;
    if (temp && (temp & WHITE_SQUARES) && (temp & BLACK_SQUARES)) score += BONUS_BISHOP_PAIR;
    while (temp) {
        square = first_one(temp);
        score += BISHOPPOS_W[square];
        score += BISHOP_DISTANCE[DISTANCE[square][blackkingsquare]];
        temp ^= BITSET[square];
    }

    // ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    // Evaluate white rooks
    // - position on the board
    // - distance from opponent king
    // - on the same file as a passed pawn
    // ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

    temp = board.white_rooks;
    q = 0;
    while (temp) {
        square = first_one(temp);
        q++;
        score += ROOKPOS_W[square];
        score += ROOK_DISTANCE[DISTANCE[square][blackkingsquare]];
        if(bit_count(COLUMNA_MASK[square]&board.all_pieces) == 1) score += BONUS_ROOK_ON_OPEN_FILE;
        // if (COLUMNA_MASK[square] & whitepassedpawns) {
             // if ((unsigned int) square < last_one(COLUMNA_MASK[square] & whitepassedpawns))
             // {
                   // score += BONUS_ROOK_BEHIND_PASSED_PAWN;
             // }
        // }
        temp ^= BITSET[square];
    }
    // if(q==2 && bit_count(COLUMNA_MASK[square]&board.white_rooks)==2 && bit_count(COLUMNA_MASK[square]&board.all_pieces)==2)
        // score += BONUS_TWO_ROOKS_ON_OPEN_FILE;

    // ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    // Evaluate white queens
    // - position on the board
    // - distance from opponent king
    // ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

    temp = board.white_queens;
    while (temp) {
        square = first_one(temp);
        score += QUEENPOS_W[square];
        score += QUEEN_DISTANCE[DISTANCE[square][blackkingsquare]];
        temp ^= BITSET[square];
    }

    // ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    // Evaluate the white king
    // - position on the board
    // - proximity to the pawns
    // - pawn shield (not in the endgame)
    // ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

    if (endgame) {
        score += KINGPOS_ENDGAME_W[whitekingsquare];
    } else {
        score += KINGPOS_W[whitekingsquare];
        score += BONUS_PAWN_SHIELD_STRONG * bit_count(KINGSHIELD_STRONG_W[whitekingsquare] & board.white_pawns);
        score += BONUS_PAWN_SHIELD_WEAK * bit_count(KINGSHIELD_WEAK_W[whitekingsquare] & board.white_pawns);
    }

    // ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    // Evaluate BLACK PIECES
    // ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

    // ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    // Evaluate black pawns
    // - position on the board
    // - distance from opponent king
    // - distance from own king
    // - passed, doubled, isolated or backward pawns
    // ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

    // blackpassedpawns = 0;
    temp = board.black_pawns;
    while (temp) {
        square = first_one(temp);
        score -= PAWNPOS_B[square];
        temp ^= BITSET[square];

        // - distance from opponent king
        score += PAWN_OPPONENT_DISTANCE[DISTANCE[square][whitekingsquare]];

        // - distance from own king
        if (endgame) score += PAWN_OWN_DISTANCE[DISTANCE[square][blackkingsquare]];

        // - passed, doubled, isolated or backward pawns
        if (!(PASSED_BLACK[square] & board.white_pawns)) {
            score -= BONUS_PASSED_PAWN;
            // blackpassedpawns ^= BITSET[square];
        }

        if (board.black_pawns & PASSED_BLACK[square]) score += PENALTY_DOUBLED_PAWN;

        if (!(ISOLATED_BLACK[square] & board.black_pawns)) score += PENALTY_ISOLATED_PAWN;
        else {
                 // If it is not isolated, then it might be backward. Two conditions must be true:
                 //  1) if the next square is controlled by an enemy pawn - we use the PAWN_ATTACKS bitmaps to check this
                 //  2) if there are no pawns left that could defend this pawn
                 if ((BLACK_PAWN_ATTACKS[square + 8] & board.white_pawns) && !(BACKWARD_BLACK[square] & board.black_pawns) )
                    score += PENALTY_BACKWARD_PAWN;
        }

        square = first_one(temp);
    }

    // ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    // Evaluate black knights
    // - position on the board
    // - distance from opponent king
    // ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

    temp = board.black_knights;
    while (temp) {
        square = first_one(temp);
        score -= KNIGHTPOS_B[square];
        score -= KNIGHT_DISTANCE[DISTANCE[square][whitekingsquare]];
        temp ^= BITSET[square];
    }

    // ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    // Evaluate black bishops
    // - having the pair
    // - position on the board
    // - distance from opponent king
    // ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

    temp = board.black_bishops;
    if (temp && (temp & WHITE_SQUARES) && (temp & BLACK_SQUARES)) score -= BONUS_BISHOP_PAIR;
    while (temp) {
        square = first_one(temp);
        score -= BISHOPPOS_B[square];
        score -= BISHOP_DISTANCE[DISTANCE[square][whitekingsquare]];
        temp ^= BITSET[square];
    }

    // ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    // Evaluate black rooks
    // - position on the board
    // - distance from opponent king
    // - on the same file as a passed pawn
    // ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

    temp = board.black_rooks;
    q = 0;
    while (temp) {
        square = first_one(temp);
        q++;
        score -= ROOKPOS_B[square];
        score -= ROOK_DISTANCE[DISTANCE[square][whitekingsquare]];
        if(bit_count(COLUMNA_MASK[square]&board.all_pieces) == 1) score -= BONUS_ROOK_ON_OPEN_FILE;
        // if (COLUMNA_MASK[square] & blackpassedpawns) {
             // if ((unsigned int) square < last_one(COLUMNA_MASK[square] & blackpassedpawns))
             // {
                   // score -= BONUS_ROOK_BEHIND_PASSED_PAWN;
             // }
        // }
        temp ^= BITSET[square];
    }
    // if(q==2 && bit_count(COLUMNA_MASK[square]&board.black_rooks)==2 && bit_count(COLUMNA_MASK[square]&board.all_pieces)==2)
        // score -= BONUS_TWO_ROOKS_ON_OPEN_FILE;

    // ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    // Evaluate black queens
    // - position on the board
    // - distance from opponent king
    // ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

    temp = board.black_queens;
    while (temp) {
        square = first_one(temp);
        score -= QUEENPOS_B[square];
        score -= QUEEN_DISTANCE[DISTANCE[square][whitekingsquare]];
        temp ^= BITSET[square];
    }

    // ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    // Evaluate the black king
    // - position on the board
    // - proximity to the pawns
    // - pawn shield (not in the endgame)
    // ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

    if (endgame) {
        score -= KINGPOS_ENDGAME_B[blackkingsquare];
    } else {
        score -= KINGPOS_B[blackkingsquare];
        score -= BONUS_PAWN_SHIELD_STRONG * bit_count(KINGSHIELD_STRONG_B[blackkingsquare] & board.black_pawns);
        score -= BONUS_PAWN_SHIELD_WEAK * bit_count(KINGSHIELD_WEAK_B[blackkingsquare] & board.black_pawns);
    }
    // ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    // Return the score
    // ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

    if (board.color) {
        score = -score;
    }

    hash_save(score);
    return score;
}

int eval_material()
{
    int score;
    int whitepawns, whiteknights, whitebishops, whiterooks, whitequeens;
    int blackpawns, blackknights, blackbishops, blackrooks, blackqueens;

    whitepawns = bit_count(board.white_pawns);
    whiteknights = bit_count(board.white_knights);
    whitebishops = bit_count(board.white_bishops);
    whiterooks = bit_count(board.white_rooks);
    whitequeens = bit_count(board.white_queens);
    blackpawns = bit_count(board.black_pawns);
    blackknights = bit_count(board.black_knights);
    blackbishops = bit_count(board.black_bishops);
    blackrooks = bit_count(board.black_rooks);
    blackqueens = bit_count(board.black_queens);

    score = (whitepawns-blackpawns)  +
            (whiteknights-blackknights) * KNIGHT_VALUE +
            (whitebishops-blackbishops) * BISHOP_VALUE +
            (whiterooks-blackrooks) * ROOK_VALUE +
            (whitequeens-blackqueens) * QUEEN_VALUE;

    if (board.color) return -score;
    else return +score;
}
