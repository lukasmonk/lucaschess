/***************************************************************************
 *   Copyright (C) 2009 by Borko Bošković                                  *
 *   borko.boskovic@gmail.com                                              *
 *                                                                         *
 *   This program is free software: you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation, either version 3 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>. *
 ***************************************************************************/

#include <cstdlib>
#include <iostream>

#include "position.h"
#include "thread.h"

const int Position::TrappedBishop = 100;
const int Position::BlockedBishop = 50;
const int Position::BlockedRook = 50;
const int Position::BlockedCenterPawn = 10;

const int Position::KnightUnit = 4;
const int Position::BishopUnit = 6;
const int Position::RookUnit = 7;

const int Position::KnightMob[2] = {4, 4};
const int Position::BishopMob[2] = {5, 5};
const int Position::RookMob[2] = {2, 4};
const int Position::QueenMob[2] = {1, 2};
const int Position::KingMob[2] = {0, 0};
const int Position::RookSemiOpenFile[2] = {10, 10};
const int Position::RookOpenFile[2] = {20, 20} ;
const int Position::RookSemiKingFileOpening = 10;
const int Position::RookKingFileOpening = 20;
const int Position::Rook7th[2] = {20, 40};
const int Position::Queen7th[2] = {10, 20};
const int Position::StormOpening = 10;
const int Position::KingAttackOpening = 20;
const int Position::Doubled[2] = {10, 20};
const int Position::Isolated[2] = {10, 20};
const int Position::IsolatedOpenOpening = 20;
const int Position::Backward[2] = {8, 10};
const int Position::BackwardOpenOpening = 16;
const int Position::CandidateMin[2] = {5, 10};
const int Position::CandidateMax[2] = {55, 110};
const int Position::PassedMin[2] = {10, 20};
const int Position::PassedMax[2] = {70, 140};
const int Position::ProtectedPassed[2] = {10,10};
const int Position::UnstoppablePasser = 800;
const int Position::FreePasser = 60;
const int Position::AttackerDistance = 5;
const int Position::DefenderDistance = 20;
const int Position::ShelterOpening = 256;

const int Position::PiecePhase[12] = {20, 20, 0, 0, 1, 1, 1, 1, 2, 2, 4, 4};
const int Position::TotalPhase = 24;

const int Position::KingAttackUnit[14] = {0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 4, 4, 0 ,0};
const int Position::KingAttackWeight[16] = {
    0, 0, 128, 192, 224, 240, 248, 252, 254, 255, 256, 256 ,256, 256, 256, 256};
const int Position::Bonus[8] = {0,0,0,26,77,154,256,0};

const int Position::KnightOutpost[2][64] = {{
        0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0,
        0, 2, 5, 10, 10, 5, 2, 0,
        0, 2, 5, 10, 10, 5, 2, 0,
        0, 0, 4, 5, 5, 4, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0
},{
        0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 4, 5, 5, 4, 0, 0,
        0, 2, 5, 10, 10, 5, 2, 0,
        0, 2, 5, 10, 10, 5, 2, 0,
        0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0
}};

const int Position::SSpaceWeight[16] = {
   0, 2, 5, 8, 12, 15, 20, 20, 20, 20, 20, 20 ,20, 20, 20, 20,
};

const int Position::ExchangePenalty[2] = {30, 30};
const int Position::BishopPair[2] = {50,70};

const int Position::MAT[12][2] = {
    // WHITE    BLACK
    // O, E     O, E
    {0, 0},      {0, 0},
    {70, 90},   {70, 90},   // PAWN
    {325, 315}, {325, 315}, // KNIGHT
    {325, 315}, {325, 315}, // BISHOP
    {500, 500}, {500, 500}, // ROOK
    {975, 975}, {975, 975}  // QUEEN
};

const int Position::DrawNodeFlag    = 1 << 0;
const int Position::DrawBishopFlag  = 1 << 1;

const int Position::MatRookPawnFlag = 1 << 0;
const int Position::MatBishopFlag   = 1 << 1;
const int Position::MatKnightFlag   = 1 << 2;
const int Position::MatKingFlag     = 1 << 3;

int Position::eval() const{
    int eval;
    EvalInfo info;

    MaterialHash * mh = &material_hash[current->mat_key % thread->mh_size];
    if(mh->key != current->mat_key) eval_mat(mh);
    else thread->m_hits ++;
    info.eval[Opening] = mh->eval[Opening];
    info.eval[Endgame] = mh->eval[Endgame];

    eval_draw(info, mh);

    if (info.mul[White] == 0 && info.mul[Black] == 0) return DRAW;

    info.eval[Opening] += current->pvt[Opening];
    info.eval[Endgame] += current->pvt[Endgame];

    PawnHash * ph = &pawn_hash[current->pawn_key % thread->ph_size];
    if(ph->key != current->pawn_key) eval_pawns(ph);
    else thread->p_hits ++;
    info.eval[Opening] += ph->eval[Opening];
    info.eval[Endgame] += ph->eval[Endgame];

    if(stm == White){
        info.eval[Opening] += 20;
        info.eval[Endgame] += 10;
    }
    else{
        info.eval[Opening] -= 20;
        info.eval[Endgame] -= 10;
    }

    eval_patterns(info);
    eval_pieces(info,mh);
    eval_king(info,mh);
    eval_passer(info,ph);

    eval = ((info.eval[Opening] * (256 - mh->phase)) + (info.eval[Endgame] * mh->phase)) / 256;

    if (eval > 0) eval = (eval * info.mul[White]) / 16;
    else if (eval < 0) eval = (eval * info.mul[Black]) / 16;

    if(stm == Black) eval = -eval;

    return eval;
}

void Position::eval_mat(MaterialHash* mh) const{
    int recog;
    int flags, cflags[2], mul[2];

    const int wp = num[WP];
    const int wn = num[WN];
    const int wb = num[WB];
    const int wr = num[WR];
    const int wq = num[WQ];

    const int bp = num[BP];
    const int bn = num[BN];
    const int bb = num[BB];
    const int br = num[BR];
    const int bq = num[BQ];

    const int wt = num[W]-1;
    const int bt = num[B]-1;

    const int wm = wb + wn;
    const int bm = bb + bn;

    recog = EG_NONE;
    if(wt == 0 && bt == 0) { recog = EG_KK; }
    else if (wt == 1 && bt == 0) {
        if (wb == 1) recog = EG_KBK;
        if (wn == 1) recog = EG_KNK;
        if (wp == 1) recog = EG_KPK;
    } else if (wt == 0 && bt == 1) {
        if (bb == 1) recog = EG_KKB;
        if (bn == 1) recog = EG_KKN;
        if (bp == 1) recog = EG_KKP;
    } else if (wt == 1 && bt == 1) {
        if (wq == 1 && bq == 1) recog = EG_KQKQ;
        if (wq == 1 && bp == 1) recog = EG_KQKP;
        if (wp == 1 && bq == 1) recog = EG_KPKQ;

        if (wr == 1 && br == 1) recog = EG_KRKR;
        if (wr == 1 && bp == 1) recog = EG_KRKP;
        if (wp == 1 && br == 1) recog = EG_KPKR;

        if (wb == 1 && bb == 1) recog = EG_KBKB;
        if (wb == 1 && bp == 1) recog = EG_KBKP;
        if (wp == 1 && bb == 1) recog = EG_KPKB;

        if (wn == 1 && bn == 1) recog = EG_KNKN;
        if (wn == 1 && bp == 1) recog = EG_KNKP;
        if (wp == 1 && bn == 1) recog = EG_KPKN;
    } else if (wt == 2 && bt == 0) {
        if (wb == 1 && wp == 1) recog = EG_KBPK;
        if (wn == 1 && wp == 1) recog = EG_KNPK;
    } else if (wt == 0 && bt == 2) {
        if (bb == 1 && bp == 1) recog = EG_KKBP;
        if (bn == 1 && bp == 1) recog = EG_KKNP;
    } else if (wt == 2 && bt == 1) {
        if (wr == 1 && wp == 1 && br == 1) recog = EG_KRPKR;
        if (wb == 1 && wp == 1 && bb == 1) recog = EG_KBPKB;
    } else if (wt == 1 && bt == 2) {
        if (wr == 1 && br == 1 && bp == 1) recog = EG_KRKRP;
        if (wb == 1 && bb == 1 && bp == 1) recog = EG_KBKBP;
    }

    flags = cflags[White] = cflags[Black] = 0;

    if(wq+wr+wp == 0 && bq+br+bp == 0){
        if(wm + bm <= 1 || recog == EG_KBKB)
            flags |= DrawNodeFlag;
    }else if(recog == EG_KPK  || recog == EG_KKP || recog == EG_KBPK || recog == EG_KKBP) {
        flags |= DrawNodeFlag;
    }

    if (wq+wr+wn == 0 && bq+br+bn == 0) {
        if (wb == 1 && bb == 1) {
            if (wp-bp >= -2 && wp-bp <= +2) {
                flags |= DrawBishopFlag;
            }
        }
    }

    mul[White] = mul[Black] = 16;

    if (wp == 0) {
        int w_maj = wq * 2 + wr;
        int w_min = wb + wn;
        int w_tot = w_maj * 2 + w_min;

        int b_maj = bq * 2 + br;
        int b_min = bb + bn;
        int b_tot = b_maj * 2 + b_min;

        if (w_tot == 1) {
            mul[White] = 0;
        } else if (w_tot == 2 && wn == 2) {
            if (b_tot != 0 || bp == 0) {
                mul[White] = 0;
            } else {
                mul[White] = 1;
            }
        } else if (w_tot == 2 && wb == 2 && b_tot == 1 && bn == 1) {
            mul[White] = 8;
        } else if (w_tot-b_tot <= 1 && w_maj <= 2) {
            mul[White] = 2;
        }
    } else if (wp == 1) {
        int w_maj = wq * 2 + wr;
        int w_min = wb + wn;
        int w_tot = w_maj * 2 + w_min;

        int b_maj = bq * 2 + br;
        int b_min = bb + bn;
        int b_tot = b_maj * 2 + b_min;

        if (b_min != 0) {
            b_min--;
            b_tot--;

            if (w_tot == 1) {
                mul[White] = 4;
            } else if (w_tot == 2 && wn == 2) {
                mul[White] = 4;
            } else if (w_tot-b_tot <= 1 && w_maj <= 2) {
                mul[White] = 8;
            }
        } else if (br != 0) {
            b_maj--;
            b_tot -= 2;
            if (w_tot == 1) {
                mul[White] = 4;
            } else if (w_tot == 2 && wn == 2) {
                mul[White] = 4;
            } else if (w_tot-b_tot <= 1 && w_maj <= 2) {
                mul[White] = 8;
            }
        }
    }

    if (bp == 0) {
        int w_maj = wq * 2 + wr;
        int w_min = wb + wn;
        int w_tot = w_maj * 2 + w_min;

        int b_maj = bq * 2 + br;
        int b_min = bb + bn;
        int b_tot = b_maj * 2 + b_min;

        if (b_tot == 1) {
            mul[Black] = 0;
        } else if (b_tot == 2 && bn == 2) {
            if (w_tot != 0 || wp == 0) {
                mul[Black] = 0;
            } else {
                mul[Black] = 1;
            }
        } else if (b_tot == 2 && bb == 2 && w_tot == 1 && wn == 1) {
            mul[Black] = 8;
        } else if (b_tot-w_tot <= 1 && b_maj <= 2) {
            mul[Black] = 2;
        }
    } else if (bp == 1) {
        int w_maj = wq * 2 + wr;
        int w_min = wb + wn;
        int w_tot = w_maj * 2 + w_min;

        int b_maj = bq * 2 + br;
        int b_min = bb + bn;
        int b_tot = b_maj * 2 + b_min;

        if (w_min != 0) {
            w_min--;
            w_tot--;
            if (b_tot == 1) {
                mul[Black] = 4;
            } else if (b_tot == 2 && bn == 2) {
                mul[Black] = 4;
            } else if (b_tot-w_tot <= 1 && b_maj <= 2) {
                mul[Black] = 8; // 1/2
            }
        } else if (wr != 0){
            w_maj--;
            w_tot -= 2;
            if (b_tot == 1) {
                mul[Black] = 4;
            } else if (b_tot == 2 && bn == 2) {
                mul[Black] = 4;
            } else if (b_tot-w_tot <= 1 && b_maj <= 2) {
                mul[Black] = 8;
            }
        }
    }

    if (wt == wb+wp && wp >= 1) cflags[White] |= MatRookPawnFlag;
    if (wt == wb+wp && wb <= 1 && wp >= 1 && bt > bp) cflags[White] |= MatBishopFlag;
    if (wt == 2 && wn == 1 && wp == 1 && bt > bp) cflags[White] |= MatKnightFlag;

    if (bt == bb+bp && bp >= 1) cflags[Black] |= MatRookPawnFlag;
    if (bt == bb+bp && bb <= 1 && bp >= 1 && wt > wp) cflags[Black] |= MatBishopFlag;
    if (bt == 2 && bn == 1 && bp == 1 && wt > wp) cflags[Black] |= MatKnightFlag;

    if (recog == EG_KQKQ || recog == EG_KRKR) {
        mul[White] = 0;
        mul[Black] = 0;
    }

    if (bq >= 1 && bq+br+bb+bn >= 2) cflags[White] |= MatKingFlag;
    if (wq >= 1 && wq+wr+wb+wn >= 2) cflags[Black] |= MatKingFlag;

    int phase = TotalPhase;

    phase -= wp * PiecePhase[PAWN];
    phase -= wn * PiecePhase[KNIGHT];
    phase -= wb * PiecePhase[BISHOP];
    phase -= wr * PiecePhase[ROOK];
    phase -= wq * PiecePhase[QUEEN];

    phase -= bp * PiecePhase[PAWN];
    phase -= bn * PiecePhase[KNIGHT];
    phase -= bb * PiecePhase[BISHOP];
    phase -= br * PiecePhase[ROOK];
    phase -= bq * PiecePhase[QUEEN];

    if(phase < 0) phase = 0;

    phase = (phase * 256 + (TotalPhase / 2)) / TotalPhase;

    int opening = 0, endgame = 0;

    const int owf = wn*MAT[KNIGHT][Opening] + wb*MAT[BISHOP][Opening] + wr*MAT[ROOK][Opening] + wq*MAT[QUEEN][Opening];
    opening += wp * MAT[PAWN][Opening];
    opening += owf;

    const int obf = bn*MAT[KNIGHT][Opening] + bb*MAT[BISHOP][Opening] + br*MAT[ROOK][Opening] + bq*MAT[QUEEN][Opening];
    opening -= bp * MAT[PAWN][Opening];
    opening -= obf;

    const int ewf = wn*MAT[KNIGHT][Endgame] + wb*MAT[BISHOP][Endgame] + wr*MAT[ROOK][Endgame] + wq*MAT[QUEEN][Endgame];
    endgame += wp * MAT[PAWN][Endgame];
    endgame += ewf;

    const int ebf = bn*MAT[KNIGHT][Endgame] + bb*MAT[BISHOP][Endgame] + br*MAT[ROOK][Endgame] + bq*MAT[QUEEN][Endgame];
    endgame -= bp * MAT[PAWN][Endgame];
    endgame -= ebf;

    if (owf > obf && bp > wp){
        opening += ExchangePenalty[Opening];
        endgame += ExchangePenalty[Opening];
    }
    else if (obf > owf && wp > bp){
        opening -= ExchangePenalty[Opening];
        endgame -= ExchangePenalty[Opening];
    }

    if (wb >= 2) {
        opening += BishopPair[Opening];
        endgame += BishopPair[Endgame];
    }

    if (bb >= 2) {
        opening -= BishopPair[Opening];
        endgame -= BishopPair[Endgame];
    }

    mh->key = current->mat_key;
    mh->phase = phase;
    mh->eval[Opening] = opening;
    mh->eval[Endgame] = endgame;
    mh->mul[White] = mul[White];
    mh->mul[Black] = mul[Black];
    mh->cflags[White] = cflags[White];
    mh->cflags[Black] = cflags[Black];
    mh->recog = recog;
    mh->flags = flags;
}

void Position::eval_draw(EvalInfo & info, const MaterialHash* mh) const{
    Color c, me, opp;
    Square pSq, kSq, sf_p_sq, prom_sq;
    File f;
    Rank r;
    Square list[8];
    Bitboard pawns;

	if(num[White] >= 10 && num[Black] >= 10){
		info.mul[White] = info.mul[Black] = 16;
        return;
	}

    const int cflags[2] = { mh->cflags[White], mh->cflags[Black] };
    const int recog = mh->recog;
    int mul[2] = { mh->mul[White], mh->mul[Black] };

    for(c = White; c <= Black; c++) {
        me = c;
        opp = me^1;

        pawns = bitboard[PAWN+me];
        pSq = first_bit(pawns);

        sf_p_sq = NO_SQ;
        if(pawns != 0 && (~(BB_FILE[file(pSq)]) & pawns) == 0){
            if(me == White) sf_p_sq = first_bit(pawns);
            else sf_p_sq = last_bit(pawns);
            f = file(sf_p_sq);
            r = rank(sf_p_sq);
            if((me == White &&
               !bit_gt(bb_file(bitboard[PAWN+opp],f-1),r) &&
               !bit_gt(bb_file(bitboard[PAWN+opp],f+1),r)) ||
               (me == Black &&
               !bit_le(bb_file(bitboard[PAWN+opp],f-1),r) &&
               !bit_le(bb_file(bitboard[PAWN+opp],f+1),r)) ){
                if(me == White) sf_p_sq = last_bit(pawns);
                else sf_p_sq = first_bit(pawns);
            }
            else sf_p_sq = NO_SQ;
        }
        if(sf_p_sq != NO_SQ && (cflags[me] & MatRookPawnFlag) != 0) {
            f = file(sf_p_sq);
            r = rank(sf_p_sq);
            if(f == FILE_A || f == FILE_H){
                kSq = first_bit(bitboard[KING + opp]);
                prom_sq = pawn_promote(sf_p_sq,me);
                if(distance(kSq, prom_sq) <= 1 && !bishop_can_attack(prom_sq,me)){
                    mul[me] = 0;
                }
            }
        }
        if(sf_p_sq != NO_SQ && (cflags[me] & MatBishopFlag) != 0) {
            kSq = first_bit(bitboard[KING + opp]);
            if(file(kSq) == file(sf_p_sq) &&
               ((me == White && rank(kSq) > rank(sf_p_sq)) ||
                (me == Black && rank(kSq) < rank(sf_p_sq))
              ) && !bishop_can_attack(kSq,me)){
                mul[me] = 1;
            }
        }
        if ((cflags[me] & MatKnightFlag) != 0) {
            kSq = first_bit(bitboard[KING + opp]);
            if(file(kSq) == file(pSq) &&
               ((me == White && rank(kSq) > rank(pSq) && rank(pSq) <= RANK_6) ||
                (me == Black && rank(kSq) <  rank(pSq) && rank(pSq) >= RANK_3))) {
                if(mul[me] > 1) mul[me] = 1;
            }
        }
    }

    if (recog == EG_KPKQ) {
        draw_list(list, White);
        if(draw_KPKQ(stm,list)){
            if(mul[White] > 1) mul[White] = 1;
            if(mul[Black] > 1) mul[Black] = 1;
        }
    } else if(recog == EG_KQKP) {
        draw_list(list, Black);
        if(draw_KPKQ(stm^1,list)) {
            if(mul[White] > 1) mul[White] = 1;
            if(mul[Black] > 1) mul[Black] = 1;
        }
    } else if(recog == EG_KPKR) {
        draw_list(list, White);
        if(draw_KPKR(stm,list)) {
            if(mul[White] > 1) mul[White] = 1;
            if(mul[Black] > 1) mul[Black] = 1;
        }
    } else if (recog == EG_KRKP) {
        draw_list(list, Black);
        if(draw_KPKR(stm^1,list)){
            if(mul[White] > 1) mul[White] = 1;
            if(mul[Black] > 1) mul[Black] = 1;
        }
    } else if (recog == EG_KPKB) {
        draw_list(list, White);
        if(draw_KPKB(list)) {
            if(mul[White] > 1) mul[White] = 1;
            if(mul[Black] > 1) mul[Black] = 1;
        }
    } else if (recog == EG_KBKP) {
        draw_list(list, Black);
        if (draw_KPKB(list)) {
            if(mul[White] > 1) mul[White] = 1;
            if(mul[Black] > 1) mul[Black] = 1;
        }
    } else if (recog == EG_KPKN) {
        draw_list(list, White);
        if (draw_KPKN(list)) {
            if(mul[White] > 1) mul[White] = 1;
            if(mul[Black] > 1) mul[Black] = 1;
        }
    } else if (recog == EG_KNKP) {
        draw_list(list, Black);
        if (draw_KPKN(list)) {
            if(mul[White] > 1) mul[White] = 1;
            if(mul[Black] > 1) mul[Black] = 1;
        }
    } else if (recog == EG_KNPK) {
        draw_list(list, White);
        if (draw_KNPK(list)) {
            if(mul[White] > 1) mul[White] = 1;
            if(mul[Black] > 1) mul[Black] = 1;
        }
    } else if (recog == EG_KKNP) {
        draw_list(list, Black);
        if (draw_KNPK(list)) {
            if(mul[White] > 1) mul[White] = 1;
            if(mul[Black] > 1) mul[Black] = 1;
        }
    } else if (recog == EG_KRPKR) {
        draw_list(list, White);
        if (draw_KRPKR(list)) {
            if(mul[White] > 1) mul[White] = 1;
            if(mul[Black] > 1) mul[Black] = 1;
        }
    } else if (recog == EG_KRKRP) {
        draw_list(list, Black);
        if (draw_KRPKR(list)) {
            if(mul[White] > 1) mul[White] = 1;
            if(mul[Black] > 1) mul[Black] = 1;
        }
    } else if (recog == EG_KBPKB) {
        draw_list(list, White);
        if (draw_KBPKB(list)) {
            if(mul[White] > 1) mul[White] = 1;
            if(mul[Black] > 1) mul[Black] = 1;
        }
    } else if (recog == EG_KBKBP) {
        draw_list(list, Black);
        if (draw_KBPKB(list)) {
            if(mul[White] > 1) mul[White] = 1;
            if(mul[Black] > 1) mul[Black] = 1;
        }
    }

    if ((mh->flags & DrawBishopFlag) != 0) {
        Square wb = first_bit(bitboard[WB]);
        Square bb = first_bit(bitboard[BB]);
        if (color(wb) != color(bb)) {
            if (mul[White] == 16) mul[White] = 8;
            if (mul[Black] == 16) mul[Black] = 8;
        }
    }
    info.mul[White] = mul[White];
    info.mul[Black] = mul[Black];
}

void Position::eval_pieces(EvalInfo & info, const MaterialHash* mh) const{
    int att_num[2], att_value[2], penalty, opening[2], endgame[2], mob;
    Color opp;
    Square sq;
    File f, kF;
    Rank r, kR;
    Bitboard bb, sq_attack, sq_attack2, mob_attack, king_safety, king_safety2;

    for(Color c = White; c <= Black; c++){
        opp = c^1;
        opening[c] = endgame[c] = 0;
        att_num[c] = att_value[c] = 0;
        info.att_num[opp] = info.att_total[opp] = 0;
        sq = first_bit(bitboard[WK+opp]);
        kF = file(sq);
        kR = rank(sq);
        king_safety2 = KingAttack2[sq];
        king_safety = KingAttack[sq];
        mob_attack = ~bitboard[c];
        for(Piece p=KNIGHT+c; p<=BQ; p+=2){
            bb = bitboard[p];
            while(bb){
                sq = first_bit_clear(bb);
                sq_attack = (this->*attack[p])(sq);
                switch (p){
                    case WN: case BN:{

                        if(sq_attack & king_safety){
                            info.att_num[opp] ++;
                            info.att_total[opp] += KingAttackUnit[p];
                        }

                        mob = - KnightUnit + bit_count_15(sq_attack & mob_attack);
                        opening[c] += mob * KnightMob[Opening];
                        endgame[c] += mob * KnightMob[Endgame];

                        if(c == White){
                            if(piece[sq-7] == WP) opening[c] += KnightOutpost[c][sq];
                            if(piece[sq-9] == WP) opening[c] += KnightOutpost[c][sq];
                            if(rank(sq) >= RANK_5){
                                att_num[c]++;
                                att_value[c]++;
                            }
                        }
                        else{
                            if(piece[sq+7] == BP) opening[c] += KnightOutpost[c][sq];
                            if(piece[sq+9] == BP) opening[c] += KnightOutpost[c][sq];
                            if(rank(sq) <= RANK_4){
                                att_num[c]++;
                                att_value[c]++;
                            }
                        }
                        break;
                    }
                    case WB: case BB:{

                        sq_attack2 = sq_attack & king_safety2;
                        if(sq_attack2){
                            if(sq_attack & king_safety ||
                               bishop_attack(sq,occupied ^ (occupied & sq_attack2)) & king_safety){
                                info.att_num[opp] ++;
                                info.att_total[opp] += KingAttackUnit[p];
                            }
                        }

                        mob = - BishopUnit + bit_count_15(sq_attack & mob_attack);
                        if(c == White){
                            if(rank(sq) >= RANK_5){
                                att_num[c]++;
                                att_value[c]++;
                            }
                        }
                        else{
                            if(rank(sq) <= RANK_4){
                                att_num[c]++;
                                att_value[c] ++;
                            }
                        }
                        opening[c] += mob * BishopMob[Opening];
                        endgame[c] += mob * BishopMob[Endgame];
                        break;
                    }
                    case WR: case BR:{

                        sq_attack2 = sq_attack & king_safety2;
                        if(sq_attack2){
                            if(sq_attack & king_safety ||
                               rook_attack(sq,occupied ^ (occupied & sq_attack2)) & king_safety){
                                info.att_num[opp] ++;
                                info.att_total[opp] += KingAttackUnit[p];
                            }
                        }

                        mob = - RookUnit + bit_count_15(sq_attack & mob_attack);
                        opening[c] += mob * RookMob[Opening];
                        endgame[c] += mob * RookMob[Endgame];
                        opening[c] -= RookOpenFile[Opening] / 2;
                        endgame[c] -= RookOpenFile[Endgame] / 2;
                        f = file(sq);
                        r = rank(sq);
                        sq_attack = BB_FILE[f];
                        if((sq_attack & bitboard[PAWN+c]) == 0){
                            opening[c] += RookSemiOpenFile[Opening];
                            endgame[c] += RookSemiOpenFile[Endgame];
                            if((sq_attack & bitboard[PAWN+opp]) == 0){
                                opening[c] += RookOpenFile[Opening] - RookSemiOpenFile[Opening];
                                endgame[c] += RookOpenFile[Endgame] - RookSemiOpenFile[Endgame];
                            }
                            if(mh->cflags[opp] & MatKingFlag){
                                mob = abs(f - kF);
                                if(mob <= 1){
                                    opening[c] += RookSemiKingFileOpening;
                                    if(mob == 0) opening[c] += RookKingFileOpening - RookSemiKingFileOpening;
                                }
                            }
                        }
                        if(c == White){
                            if(r == RANK_7){
                                if((bitboard[BP] & BB_RANK[RANK_7]) || kR == RANK_8){
                                    opening[c] += Rook7th[Opening];
                                    endgame[c] += Rook7th[Endgame];
                                }
                            }
                            if(r >= RANK_5){
                                att_num[c]++;
                                att_value[c] += 2;
                            }
                        }
                        else{
                            if(r == RANK_2){
                                if((bitboard[WP] & BB_RANK[RANK_2]) || kR == RANK_1){
                                    opening[c] += Rook7th[Opening];
                                    endgame[c] += Rook7th[Endgame];
                                }
                            }
                            if(r <= RANK_4){
                                att_num[c]++;
                                att_value[c] += 2;
                            }
                        }
                        break;
                    }
                    case WQ: case BQ:{

                        sq_attack2 = sq_attack & king_safety2;
                        if(sq_attack2){
                            if(sq_attack & king_safety ||
                               queen_attack(sq,occupied ^ (occupied & sq_attack2)) & king_safety){
                                info.att_num[opp] ++;
                                info.att_total[opp] += KingAttackUnit[p];
                            }
                        }

                        f = file(sq);
                        r = rank(sq);
                        penalty = 10 -(abs(kR-r)+abs(kF-f));
                        opening[c] += penalty;
                        endgame[c] += penalty;
                        if(c == White){
                            if(r == RANK_7){
                                if((bitboard[BP] & BB_RANK[RANK_7]) || kR == RANK_8){
                                    opening[c] += Queen7th[Opening];
                                    endgame[c] += Queen7th[Endgame];
                                }
                            }
                            if(r >= RANK_5){
                                att_num[c]++;
                                att_value[c] += 4;
                            }
                        }
                        else{
                            if(r == RANK_2){
                                if((bitboard[WP] & BB_RANK[RANK_2]) || kR == RANK_1){
                                    opening[c] += Queen7th[Opening];
                                    endgame[c] += Queen7th[Endgame];
                                }
                            }
                            if(r <= RANK_4){
                                att_num[c]++;
                                att_value[c] += 4;
                            }
                        }
                        break;
                    }
                    default: { break; }
               }
           }
        }
        opening[c] += SSpaceWeight[att_num[c]] * att_value[c];
    }

    info.eval[Opening] += (opening[White] - opening[Black]);
    info.eval[Endgame] += (endgame[White] - endgame[Black]);
}

void Position::eval_patterns(EvalInfo& info) const{

    if((piece[A7] == WB && piece[B6] == BP) || (piece[B8] == WB && piece[C7] == BP)){
        info.eval[Opening] -= TrappedBishop;
        info.eval[Endgame] -= TrappedBishop;
    }
    if((piece[H7] == WB && piece[G6] == BP) || (piece[G8] == WB && piece[F7] == BP)){
        info.eval[Opening] -= TrappedBishop;
        info.eval[Endgame] -= TrappedBishop;
    }
    if((piece[A2] == BB && piece[B3] == WP) || (piece[B1] == BB && piece[C2] == WP)){
        info.eval[Opening] += TrappedBishop;
        info.eval[Endgame] += TrappedBishop;
    }
    if((piece[H2] == BB && piece[G3] == WP) || (piece[G1] == BB && piece[F2] == WP)){
        info.eval[Opening] += TrappedBishop;
        info.eval[Endgame] += TrappedBishop;
    }

    if(piece[A6] == WB && piece[B5] == BP) {
        info.eval[Opening] -= TrappedBishop / 2;
        info.eval[Endgame] -= TrappedBishop / 2;
    }
    if(piece[H6] == WB && piece[G5] == BP) {
        info.eval[Opening] -= TrappedBishop / 2;
        info.eval[Endgame] -= TrappedBishop / 2;
    }
    if(piece[A3] == BB && piece[B4] == WP) {
        info.eval[Opening] += TrappedBishop / 2;
        info.eval[Endgame] += TrappedBishop / 2;
    }
    if(piece[H3] == BB && piece[G4] == WP) {
        info.eval[Opening] += TrappedBishop / 2;
        info.eval[Endgame] += TrappedBishop / 2;
    }

    if(piece[D2] == WP && piece[D3] != NO_PIECE && piece[C1] == WB)
        info.eval[Opening] -= BlockedBishop;
    if(piece[E2] == WP && piece[E3] != NO_PIECE && piece[F1] == WB)
        info.eval[Opening] -= BlockedBishop;
    if(piece[D7] == BP && piece[D6] != NO_PIECE && piece[C8] == BB)
        info.eval[Opening] += BlockedBishop;
    if(piece[E7] == BP && piece[E6] != NO_PIECE && piece[F8] == BB)
        info.eval[Opening] += BlockedBishop;

    if((piece[C1]==WK || piece[B1]==WK) && (piece[A1]==WR || piece[A2]==WR || piece[B1]==WR))
        info.eval[Opening] -= BlockedRook;
    if((piece[F1]==WK || piece[G1]==WK) && (piece[H1]==WR || piece[H2]==WR || piece[G1]==WR))
        info.eval[Opening] -= BlockedRook;
    if((piece[C8]==BK || piece[B8]==BK) && (piece[A8]==BR || piece[A7]==BR || piece[B8]==BR))
        info.eval[Opening] += BlockedRook;
    if((piece[F8]==BK || piece[G8]==BK) && (piece[H8]==BR || piece[H7]==BR || piece[G8]==BR))
        info.eval[Opening] += BlockedRook;
}

void Position::eval_pawns(PawnHash* ph) const{
    Bitboard pawns, neighbour1, neighbour2, bb_passed[2];
    Square sq;
    Piece p;
    File f;
    Rank r;
    Color opp;
    int opening[2], endgame[2];
    bool backward, doubled, passed, isolated, open, candidate;

    for(Color c = White; c <= Black; c++){
        opp = c ^ 1;
        p = PAWN+c;
        pawns = bitboard[p];
        opening[c] = endgame[c] = 0;
        bb_passed[c] = 0;
        while(pawns){
            sq = first_bit_clear(pawns);
            f = file(sq);
            r = rank(sq);
            backward = doubled = isolated = open = passed = candidate = false;

            if(bit_le((BB_FILE[f] & bitboard[p]),r)) doubled = true;

            neighbour1 = bb_file(bitboard[p],f-1) | bb_file(bitboard[p],f+1);
            neighbour2 = (bitboard[WP] | bitboard[BP]) & BB_FILE[f];

            if(c == White){
                if(!neighbour1) isolated = true;
                else{
                    if(!bit_le_eq(neighbour1,r)){
                        backward = true;
                        if((neighbour1 & BB_RANK[r+1]) != 0){
                            if(!((neighbour2 & BB_RANK[r+1]) | // ? |
                            ((bb_file(bitboard[BP],f-1) | bb_file(bitboard[BP],f+1)) &
                              (BB_RANK[r+1] | BB_RANK[r+2]))))
                                  backward = false;
                        }
                        else if(r == RANK_2 && ((neighbour1 & BB_RANK[r+2]) != 0)){
                            if(!((neighbour2 & (BB_RANK[r+1] | BB_RANK[r+2])) | // ? | združiti
                                ((bb_file(bitboard[BP],f-1) | bb_file(bitboard[BP],f+1)) &
                                    (BB_RANK[r+1]|BB_RANK[r+2]|BB_RANK[r+3]))))
                                        backward = false;
                        }
                    }
                }
                if(!bit_gt(neighbour2, r)){
                       open = true;
                    if(!bit_gt(bb_file(bitboard[BP], f-1),r) && !bit_gt(bb_file(bitboard[BP],f+1),r)){
                        passed = true;
                        bb_passed[c] |= BB_1<<sq;
                    }else{
                        int n = 0;
                        n += bit_num_le_eq(bb_file(bitboard[WP],f-1),r);
                        n += bit_num_le_eq(bb_file(bitboard[WP],f+1),r);
                        n -= bit_num_gt(bb_file(bitboard[BP],f-1),r);
                        n -= bit_num_gt(bb_file(bitboard[BP],f+1),r);
                        if(n>=0){
                            n = bit_num_eq(bb_file(bitboard[WP],f-1),r-1);
                            n += bit_num_eq(bb_file(bitboard[WP],f+1),r-1);
                            n -= bit_num_eq(bb_file(bitboard[BP],f-1),r+1);
                            n -= bit_num_eq(bb_file(bitboard[BP],f+1),r+1);
                            if(n >= 0) candidate = true;
                        }
                    }
                }
            }
            else{
                if(!neighbour1) isolated = true;
                else{
                    if(!bit_gt_eq(neighbour1,r)){
                        backward = true;
                        if((neighbour1 & BB_RANK[r-1]) != 0){
                            if(!((neighbour2 & BB_RANK[r-1]) |
                                ((bb_file(bitboard[WP],f-1) | bb_file(bitboard[WP],f+1)) &
                                 (BB_RANK[r-1] | BB_RANK[r-2]))))
                                    backward = false;
                        }
                        else if(r == RANK_7 && ((neighbour1 & BB_RANK[r-2]) != 0)){
                            if(!((neighbour2 & (BB_RANK[r-1] | BB_RANK[r-2])) |
                                ((bb_file(bitboard[WP],f-1) | bb_file(bitboard[WP],f+1)) &
                                 (BB_RANK[r-1]|BB_RANK[r-2]|BB_RANK[r-3]))))
                                    backward = false;
                        }
                    }
                }
                if(!bit_le(neighbour2, r)){
                    open = true;
                    if(!bit_le(bb_file(bitboard[WP],f-1),r) &&
                        !bit_le(bb_file(bitboard[WP],f+1),r)){
                            passed = true;
                            bb_passed[c] |= BB_1<<sq;
                    }else{
                        int n = bit_num_gt_eq(bb_file(bitboard[BP],f-1),r);
                        n += bit_num_gt_eq(bb_file(bitboard[BP],f+1),r);
                        n -= bit_num_le(bb_file(bitboard[WP],f-1),r);
                        n -= bit_num_le(bb_file(bitboard[WP],f+1),r);
                       if(n >= 0){
                            n = bit_num_eq(bb_file(bitboard[BP],f-1),r+1);
                            n += bit_num_eq(bb_file(bitboard[BP],f+1),r+1);
                            n -= bit_num_eq(bb_file(bitboard[WP],f-1),r-1);
                            n -= bit_num_eq(bb_file(bitboard[WP],f+1),r-1);
                            if(n >= 0) candidate = true;
                        }
                   }
                }
            }
            if(doubled){
                opening[c] -= Doubled[Opening];
                endgame[c] -= Doubled[Endgame];
            }
            if (isolated){
                if (open){
                    opening[c] -= IsolatedOpenOpening;
                    endgame[c] -= Isolated[Endgame];
                }else{
                    opening[c] -= Isolated[Opening];
                    endgame[c] -= Isolated[Endgame];
                }
            }
            if (backward){
                if (open){
                    opening[c] -= BackwardOpenOpening;
                    endgame[c] -= Backward[Endgame];
                }else{
                    opening[c] -= Backward[Opening];
                    endgame[c] -= Backward[Endgame];
                }
            }
            if (candidate) {
                if(c == White){
                    opening[c] += quad(CandidateMin[Opening],CandidateMax[Opening],r);
                    endgame[c] += quad(CandidateMin[Endgame],CandidateMax[Endgame],r);
                }else{
                    opening[c] += quad(CandidateMin[Opening],CandidateMax[Opening],RANK_8-r);
                    endgame[c] += quad(CandidateMin[Endgame],CandidateMax[Endgame],RANK_8-r);
                }
            }
        }
    }

    ph->key = current->pawn_key;
    ph->passed[White] = bb_passed[White];
    ph->passed[Black] = bb_passed[Black];
    ph->eval[Opening] = opening[White] - opening[Black];
    ph->eval[Endgame] = endgame[White] - endgame[Black];
}

void Position::eval_king(EvalInfo& info, const MaterialHash* mh) const{
    int opening[2] = {0, 0};
    Square kSq[2];
    File kF[2];
    int penalty = 0, penalty1, penalty2, tmp;

    if(mh->cflags[White] & MatKingFlag){
        kSq[White] = first_bit(bitboard[WK]);
        if(bitboard[BQ] != 0){
            penalty1 = shelter_sq(kSq[White],White);
            penalty2 = penalty1;
            if((current->castle & WK_CASTLE) != 0){
                tmp = shelter_sq(G1,White);
                if(tmp < penalty2) penalty2 = tmp;
            }
            if((current->castle & WQ_CASTLE) != 0){
                tmp = shelter_sq(B1,White);
                if(tmp < penalty2) penalty2 = tmp;
            }
            penalty = (penalty1+penalty2)/2;
        }
        kF[White] = file(kSq[White]);
        penalty += storm_file(kF[White],White);
        if(kF[White] != FILE_A) penalty += storm_file(kF[White]-1,White);
        if(kF[White] != FILE_H) penalty += storm_file(kF[White]+1,White);
        opening[White] -= (penalty * ShelterOpening) / 256;
    }

    if(mh->cflags[Black] & MatKingFlag){
        kSq[Black] = first_bit(bitboard[BK]);
        if((bitboard[WQ] != 0)){
            penalty1 = shelter_sq(kSq[Black],Black);
            penalty2 = penalty1;
            if((current->castle & BK_CASTLE) != 0){
                tmp = shelter_sq(G8,Black);
                if(tmp < penalty2) penalty2 = tmp;
            }
            if((current->castle & BQ_CASTLE) != 0){
                tmp = shelter_sq(B8,Black);
                if(tmp < penalty2) penalty2 = tmp;
            }
            penalty = (penalty1+penalty2)/2;
        }
        kF[Black] = file(kSq[Black]);
        penalty += storm_file(kF[Black],Black);
        if(kF[Black] != FILE_A) penalty += storm_file(kF[Black]-1,Black);
        if(kF[Black] != FILE_H) penalty += storm_file(kF[Black]+1,Black);
        opening[Black] -= (penalty * ShelterOpening) / 256;
    }

    if(true){
    if((mh->cflags[White] & MatKingFlag) != 0)
        opening[White] -= (info.att_total[White] * KingAttackOpening * KingAttackWeight[info.att_num[White]]) / 256;
    if((mh->cflags[Black] & MatKingFlag) != 0)
        opening[Black] -= (info.att_total[Black] * KingAttackOpening * KingAttackWeight[info.att_num[Black]]) / 256;
    }

    info.eval[Opening] += opening[White] - opening[Black];
}

void Position::eval_passer(EvalInfo& info, const PawnHash* ph) const {
    Bitboard bb,b;
    Square sq;
    Rank r;
    int opening[2] = {0,0}, endgame[2] = {0,0}, delta, min, max;
    Color opp;

    Square kSq = first_bit(bitboard[WK]);
    Square opp_kSq = first_bit(bitboard[BK]);

    for(Color c = White; c <= Black; c++){
        bb = ph->passed[c];
        opp = c^1;

        if(c == Black){
            delta = kSq;
            kSq = opp_kSq;
            opp_kSq = Square(delta);
        }

        while(bb){
            sq = first_bit_clear(bb);

            if(c == White) r = rank(sq);
            else r = Rank(RANK_8 - rank(sq));

            b = BB_1<<sq;

            opening[c] += quad(PassedMin[Opening],PassedMax[Opening], r);

            min = PassedMin[Endgame];
            max = PassedMax[Endgame];
            delta = max - min;

            if(((bitboard[KNIGHT + opp] | bitboard[BISHOP + opp] | bitboard[ROOK + opp] | bitboard[QUEEN + opp]) == 0) &&
               (unstoppable_passer(sq,c) || king_passer(sq,c))){
                delta += UnstoppablePasser;
            }
            else if(free_passer(sq,c)){
                delta += FreePasser;
            }

            delta -= pawn_att_dist(sq,kSq,c) * AttackerDistance;
            delta += pawn_def_dist(sq,opp_kSq,c) * DefenderDistance;

            endgame[c] += min;
            if (delta > 0) endgame[c] += quad(0,delta,r);
        }
    }

    info.eval[Opening] += opening[White] - opening[Black];
    info.eval[Endgame] += endgame[White] - endgame[Black];
}

inline int Position::quad(const int min, const int max, const int x) {
   return  min + ((max - min) * Bonus[x] + 128) / 256;
}

inline int Position::shelter_sq(const Square sq, const Color c) const{
    const int f = file(sq);
    const int r = rank(sq);
    int penalty = shelter_file(f,r,c) * 2;
    if (f != FILE_A) penalty += shelter_file(f-1,r,c);
    if (f != FILE_H) penalty += shelter_file(f+1,r,c);

    if (penalty == 0) penalty = 11;

    penalty += storm_file(f,c);
    if (f != FILE_A) penalty += storm_file(f-1,c);
    if (f != FILE_H) penalty += storm_file(f+1,c);

    return penalty;
}

inline int Position::shelter_file(const int f, const int r, const int c) const{
    int distance, penalty;
    Bitboard bb = bitboard[PAWN+c] & BB_FILE[f];

    if(c == White){
        distance = r*8;
        bb = (bb >> distance)<<distance;
        if(bb) distance = RANK_8 - rank(first_bit(bb));
        else distance = 0;
    }
    else{
        distance = (RANK_8-r)*8;
        bb = (bb << distance)>>distance;
        if(bb) distance = rank(last_bit(bb));
        else distance = 0;
    }
    penalty = 36 - distance * distance;
    return penalty;
}

inline int Position::storm_file(const int f, const int c) const{
    int distance, penalty = 0;
    Bitboard bb = bitboard[PAWN+(c^1)] & BB_FILE[f];

    if(c == White){
        if(bb) distance = RANK_8 - rank(first_bit(bb));
        else distance = 0;
    }
    else{
        if(bb) distance = rank(last_bit(bb));
        else distance = 0;
    }
    switch (distance) {
        case RANK_4:{ penalty = StormOpening * 1; break;}
        case RANK_5:{ penalty = StormOpening * 2; break;}
        case RANK_6:{ penalty = StormOpening * 3; break;}
    }
    return penalty;
}

inline int Position::pawn_att_dist(const Square sq, const Square kSq, const Color c)const {
    Square pSq;
    if(c == White) pSq = sq + 8;
    else pSq = sq - 8;
    return distance(pSq,kSq);
}

inline int Position::pawn_def_dist(const Square sq, const Square kSq, const Color c)const {
    Square pSq;
    if(c == White) pSq = sq + 8;
    else pSq = sq - 8;
    return distance(pSq,kSq);
}

inline bool Position::unstoppable_passer(Square sq, const Color c) const{
    Square opp_kSq;
    int dist;
    Rank r = rank(sq);

    if(c == White){
        opp_kSq = first_bit(bitboard[BK]);
        if(bit_gt(BB_FILE[file(sq)] & bitboard[White],r))
            return false;
        if(r == RANK_2){
            r ++;
            sq += 8;
        }
        dist = distance(opp_kSq,sq+(8*(RANK_8-r)));
        if(stm == Black) dist --;
        if(dist > RANK_8 - r) return true;
    }
    else{
        opp_kSq = first_bit(bitboard[WK]);
        if(bit_le(BB_FILE[file(sq)] & bitboard[Black],r))
            return false;
        if(r == RANK_7){
            r --;
            sq -= 8;
        }
        dist = distance(opp_kSq,sq-(8*r));
        if(stm == White) dist --;
        if(dist > r) return true;
    }
    return false;
}

inline bool Position::king_passer(const Square sq, const Color c) const{
    Square pSq;
    const Square kSq =  first_bit(bitboard[KING+c]);
    const File f = file(sq);

    if(c == White) pSq = kSq + (8*(RANK_8-rank(kSq)));
    else pSq = kSq - (8*rank(kSq));

    if(distance(kSq,pSq) <= 1 && distance(kSq,sq) == 1 &&
       (file(kSq) != f || (f != FILE_A && f != FILE_H))){
        return true;
    }
    return false;
}

inline bool Position::free_passer(const Square from , const Color c) const{
    Move move;
    Square to;

    if(c == White) to = from + 8;
    else to = from - 8;

    if (piece[to] != NO_PIECE) return false;

    move = move_create(MoveQuiet,from,to);

    if(see(move) < 0) return false;
    return true;
}

inline Square Position::pawn_promote(const Square sq, const Color c) const{
    if(c == White) return Square(0x38 | (sq&7));
    else return Square(sq&7);
}

inline bool Position::bishop_can_attack(const Square to, const Color c) const{
    Bitboard bb = bitboard[BISHOP+c];
    Square sq;
    while(bb){
        sq = first_bit_clear(bb);
        if(color(sq) == color(to)){
            return true;
        }
    }
    return false;
}

inline void Position::draw_list(Square list[8], const Color c) const{
    int pos = 0;
    Bitboard bb;

    for(Piece p = PAWN+c; p<= KING+c; p+=2){
        bb = bitboard[p];
        while(bb) list[pos++] = first_bit_clear(bb);
    }

    for(Piece p = PAWN+(c^1); p<= KING+(c^1); p+=2){
        bb = bitboard[p];
        while(bb) list[pos++] = first_bit_clear(bb);
    }

    list[pos] = NO_SQ;

    Square pawn_sq = first_bit(bitboard[PAWN+c]);
    if(file(pawn_sq) >= FILE_E){
        for(int i=0; i<pos; i++){
            list[i] = file_mirror_sq(list[i]);
        }
    }

    if(c == Black){
        for(int i=0; i<pos; i++){
            list[i] = rank_mirror_sq(list[i]);
        }
    }
}

inline bool Position::draw_KPKQ(const Color c, const Square list[8]) const{
    Square wk, wp, bk, bq, prom;
    int dist;

    wp = list[0];
    wk = list[1];
    bq = list[2];
    bk = list[3];

    if (wp == A7) {
        prom = A8;
        dist = 4;
        if(wk == B7 || wk == B8) {
            if(c == White) dist--;
        }
        else if(wk == A8 || ((wk == C7 || wk == C8) && bq != A8)) {
            if(c == Black && file(bq) != FILE_B) return false;
        } else { return false; }
        if(distance(bk,prom) > dist) return true;
    }else if(wp == C7){
        prom = C8;
        dist = 4;
        if (wk == C8) {
            dist++;
            if(c == White) dist--;
        } else if (wk == B7 || wk == B8) {
            dist--;
            if(QueenAttack[bq] & BB_1<<wk) dist++;
            if(c == White) dist--;
        }else if (wk == D7 || wk == D8){
            if(QueenAttack[bq] & BB_1<<wk) dist++;
            if(c == White) dist--;
        } else if ((wk == A7 || wk == A8) && bq != C8) {
            if(c == Black && file(bq) != FILE_B)
                return false;
            dist--;
        } else if((wk == E7 || wk == E8) && bq != C8) {
            if(c == Black && file(bq) != FILE_D)
                return false;
        } else { return false; }

        if(distance(bk,prom) > dist) return true;
    }
    return false;
}

inline bool Position::draw_KPKR(const Color c, const Square list[8]) const{
    Square prom;
    int dist;

    const Square wp = list[0];
    const Square wk = list[1];
    const Square br = list[2];
    const Square bk = list[3];

    const File wk_f = file(wk);
    const File wp_f = file(wp);
    const File br_f = file(br);

    const Rank wk_r = rank(wk);
    const Rank wp_r = rank(wp);
    const Rank br_r = rank(br);

    prom = pawn_promote(wp,White);

    if(distance(wk,wp) == 1){ }
    else if(distance(wk,wp) == 2 && abs(wk_r-wp_r) <= 1) {
        if(c == Black && br_f != (wk_f + wp_f) / 2) return false;
    } else {
        return false;
    }

    dist = distance(wk,prom) + distance(wp,prom);
    if (wk == prom) dist++;

    if (wk == wp+8) {
        if (wp_f == FILE_A) return false;
        dist++;
    }

    if (br_f != wp_f && br_r != RANK_8) {
        dist--;
    }

    if (c == White) dist--;

    if (distance(bk,prom) > dist) return true;

    return false;
}

inline bool Position::draw_KPKB(const Square list[8]) const{
    Square wk, wp, bk, bb;

    wp = list[0];
    wk = list[1];
    bb = list[2];
    bk = list[3];

    Bitboard b = BB_1 << bk;
    if(bit_gt(BB_FILE[file(wp)] & ((BB_1 << bb) | bishop_attack(bb,b)),rank(wp)))
        return true;
    return false;
}

inline bool Position::draw_KPKN(const Square list[8]) const{
    Square wk, wp, bk, bn;

    wp = list[0];
    wk = list[1];
    bn = list[2];
    bk = list[3];

    if(bit_gt(BB_FILE[file(wp)] & ~BB_A8 & ((BB_1 << bn) | KnightAttack[bn]),rank(wp)))
        return true;
    return false;
}

inline bool Position::draw_KNPK(const Square list[8]) const{
    Square wk, wn, wp, bk;

    wp = list[0];
    wn = list[1];
    wk = list[2];
    bk = list[3];

    if (wp == A7 && distance(bk,A8) <= 1) return true;
    return false;
}

inline bool Position::draw_KRPKR(const Square list[8]) const{
    Square wk, wr, wp, bk, br, prom;
    File wp_f, bk_f, br_f ;
    Rank wp_r, bk_r, br_r;

    wp = list[0];
    wr = list[1];
    wk = list[2];
    br = list[3];
    bk = list[4];

    wp_f = file(wp);
    wp_r = rank(wp);
    bk_f = file(bk);
    bk_r = rank(bk);
    br_f = file(br);
    br_r = rank(br);

    prom = pawn_promote(wp,White);

    if (bk == prom) {
        if (br_f > wp_f) return true;
    } else if (bk_f == wp_f && bk_r > wp_r) {
        return true;
    } else if (wr == prom && wp_r == RANK_7 && (bk == G7 || bk == H7) && br_f == wp_f) {
        if (br_r <= RANK_3) {
            if (distance(wk,wp) > 1) return true;
        } else {
            if (distance(wk,wp) > 2) return true;
        }
    }
    return false;
}

inline bool Position::draw_KBPKB(const Square list[8]) const{
    const Square wp = list[0];
    const Square wb = list[1];
    const Square bb = list[3];
    const Square bk = list[4];

    if(color(wb) == color(bb)) return false;

    const Bitboard b = BB_1 << bk;
    if(bit_gt(BB_FILE[file(wp)] & ((BB_1 << bb) | bishop_attack(bb,b)),rank(wp)))
        return true;
    return false;
}
