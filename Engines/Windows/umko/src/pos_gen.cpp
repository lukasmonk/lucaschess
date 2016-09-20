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

void Position::gen_capture(MoveList& ml) const{
    Square from, to;
    Bitboard bb, dest,possible, pawn_dest[4];

    possible = ~occupied;
    if(stm == White){
        bb = bitboard[WP];
        pawn_dest[1] = shift_up(bb) & possible & BB_RANK_8;
        pawn_dest[2] = shift_up_right(bb) & bitboard[Black];
        pawn_dest[3] = shift_up_left(bb) & bitboard[Black];
    }else{
        bb = bitboard[BP];
        pawn_dest[1] = shift_down(bb) & possible & BB_RANK_1;
        pawn_dest[2] = shift_down_right(bb) & bitboard[White];
        pawn_dest[3] = shift_down_left(bb) & bitboard[White];
    }

    for(int i=1; i<4; i++){
        while(pawn_dest[i]){
            to = first_bit_clear(pawn_dest[i]);
            from = to + PawnFrom[stm][i];
            if(to >= A8 || to <= H1){
                for(Move m = MovePQ; m >= MovePN; m-=2)
                    ml.add(move_create(m,from,to),piece_value(Piece(m))+piece_value(piece[to])-piece_value(WP));
            }
            else ml.add(move_create(MoveCapture,from,to),piece_value(piece[to])-piece_value(WP));
        }
    }

    if(current->ep != NO_SQ){
        dest = bitboard[WP+stm] & PawnAttack[stm^1][current->ep];
        while(dest){
           from = first_bit_clear(dest);
            ml.add(move_create(MoveEP,from,current->ep),0);
        }
    }

    possible = bitboard[stm^1];
    for(Piece p=KNIGHT+stm; p<=BQ; p+=2){
        bb = bitboard[p];
        while(bb){
            from = first_bit_clear(bb);
            dest = (this->*attack[p])(from) & possible;
            while(dest){
                to = first_bit_clear(dest);
                ml.add(move_create(MoveCapture,from,to),piece_value(piece[to])-piece_value(piece[from]));
            }
        }
    }

    from = first_bit(bitboard[WK+stm]);
    dest = king_attack(from) & bitboard[stm^1] & ~king_attack(first_bit(bitboard[WK+(stm^1)]));
    while(dest){
        to = first_bit_clear(dest);
        ml.add(move_create(MoveCapture,from,to),piece_value(piece[to])-piece_value(WK));
    }
}

void Position::gen_quiet(MoveList& ml) const{
    Square from, to;
    Bitboard bb, dest, empty, pawn_dest[4];

    empty = ~occupied;
    for(Piece p=KNIGHT+stm; p<=BQ; p+=2){
        bb = bitboard[p];
        while(bb){
            from = first_bit_clear(bb);
            dest = (this->*attack[p])(from) & empty;
            while(dest){
                to = first_bit_clear(dest);
                ml.add(move_create(MoveQuiet,from,to),PVT[p][to][Opening]);
            }
        }
    }

    if(stm == White){
        bb = bitboard[WP];
        pawn_dest[1] = shift_up(bb) & empty & ~BB_RANK_8;
        pawn_dest[0] = shift_up(pawn_dest[1]) & empty & BB_RANK_4;
        if(!current->checkers && current->castle & W_CASTLE){
            if(current->castle & WK_CASTLE
               && (((BB_F1|BB_G1) & empty) == (BB_F1|BB_G1))
               && !sq_attacked(F1,White)
               && ((BB_F2|BB_G2|BB_H2) & bitboard[BK]) == 0)
                ml.add(move_create(MoveCastle,E1,G1),PVT[WK][E1][Opening]);
            if(current->castle & WQ_CASTLE
               && (((BB_B1|BB_C1|BB_D1)&empty) == (BB_B1|BB_C1|BB_D1))
               && !sq_attacked(D1,White)
               && ((BB_B2|BB_C2|BB_D2) & bitboard[BK]) == 0)
                ml.add(move_create(MoveCastle,E1,C1),PVT[WK][C1][Opening]);
        }
    }else{
        bb = bitboard[BP];
        pawn_dest[1] = shift_down(bb) & empty & ~BB_RANK_1;
        pawn_dest[0] = shift_down(pawn_dest[1]) & empty & BB_RANK_5;
        if(!current->checkers && current->castle & B_CASTLE){
            if(current->castle & BK_CASTLE
               && (((BB_F8|BB_G8)&empty) == (BB_F8|BB_G8))
               && !sq_attacked(F8,Black)
               && ((BB_F7|BB_G7|BB_H7) & bitboard[WK]) == 0)
                ml.add(move_create(MoveCastle,E8,G8),PVT[BK][G8][Opening]);
            if(current->castle & BQ_CASTLE
               && (((BB_B8|BB_C8|BB_D8)&empty) == (BB_B8|BB_C8|BB_D8))
               && !sq_attacked(D8,Black)
               && ((BB_B7|BB_C7|BB_D7) & bitboard[WK]) == 0)
                ml.add(move_create(MoveCastle,E8,C8),PVT[BK][C8][Opening]);
        }
    }

    for(int i=0; i<2; i++){
        while(pawn_dest[i]){
            to = first_bit_clear(pawn_dest[i]);
            from = to + PawnFrom[stm][i];
            ml.add(move_create(MoveQuiet,from,to),PVT[PAWN+stm][to][Opening]);
        }
    }

    from = first_bit(bitboard[KING+stm]);
    dest = king_attack(from) & empty & ~king_attack(first_bit(bitboard[WK+(stm^1)]));
    while(dest){
        to = first_bit_clear(dest);
        ml.add(move_create(MoveQuiet,from,to),PVT[KING+stm][to][Opening]);
    }
}

void Position::gen_quiet(MoveList& ml, const int16_t history[14][64][64]) const{
    Square from, to;
    Bitboard bb, dest, empty, pawn_dest[4];

    empty = ~occupied;
    for(Piece p=KNIGHT+stm; p<=BQ; p+=2){
        bb = bitboard[p];
        while(bb){
            from = first_bit_clear(bb);
            dest = (this->*attack[p])(from) & empty;
            while(dest){
                to = first_bit_clear(dest);
				ml.add(move_create(MoveQuiet,from,to),history[p][from][to]);
            }
        }
    }

    if(stm == White){
        bb = bitboard[WP];
        pawn_dest[1] = shift_up(bb) & empty & ~BB_RANK_8;
        pawn_dest[0] = shift_up(pawn_dest[1]) & empty & BB_RANK_4;
        if(!current->checkers && current->castle & W_CASTLE){
            if(current->castle & WK_CASTLE
               && (((BB_F1|BB_G1) & empty) == (BB_F1|BB_G1))
               && !sq_attacked(F1,White)
               && ((BB_F2|BB_G2|BB_H2) & bitboard[BK]) == 0)
				ml.add(move_create(MoveCastle,E1,G1),history[WK][E1][G1]);
            if(current->castle & WQ_CASTLE
               && (((BB_B1|BB_C1|BB_D1)&empty) == (BB_B1|BB_C1|BB_D1))
               && !sq_attacked(D1,White)
               && ((BB_B2|BB_C2|BB_D2) & bitboard[BK]) == 0)
				ml.add(move_create(MoveCastle,E1,C1),history[WK][E1][C1]);
        }
    }else{
        bb = bitboard[BP];
        pawn_dest[1] = shift_down(bb) & empty & ~BB_RANK_1;
        pawn_dest[0] = shift_down(pawn_dest[1]) & empty & BB_RANK_5;
        if(!current->checkers && current->castle & B_CASTLE){
            if(current->castle & BK_CASTLE
               && (((BB_F8|BB_G8)&empty) == (BB_F8|BB_G8))
               && !sq_attacked(F8,Black)
               && ((BB_F7|BB_G7|BB_H7) & bitboard[WK]) == 0)
				ml.add(move_create(MoveCastle,E8,G8),history[BK][E8][G8]);
            if(current->castle & BQ_CASTLE
               && (((BB_B8|BB_C8|BB_D8)&empty) == (BB_B8|BB_C8|BB_D8))
               && !sq_attacked(D8,Black)
               && ((BB_B7|BB_C7|BB_D7) & bitboard[WK]) == 0)
				ml.add(move_create(MoveCastle,E8,C8),history[BK][E8][C8]);
        }
    }

    for(int i=0; i<2; i++){
        while(pawn_dest[i]){
            to = first_bit_clear(pawn_dest[i]);
            from = to + PawnFrom[stm][i];
			ml.add(move_create(MoveQuiet,from,to),history[PAWN+stm][from][to]);
        }
    }

    from = first_bit(bitboard[KING+stm]);
    dest = king_attack(from) & empty & ~king_attack(first_bit(bitboard[WK+(stm^1)]));
    while(dest){
        to = first_bit_clear(dest);
		ml.add(move_create(MoveQuiet,from,to),history[KING+stm][from][to]);
    }
}

void Position::gen_evasions(MoveList & ml, const int16_t history[14][64][64]) const{
    Square from, to, checkerSq, kSq;
    Piece p;
    Bitboard bb, dest, checkers, possible = 0, pawn_dest[4];

    kSq = first_bit(bitboard[KING+stm]);
    dest = king_attack(kSq) & ~bitboard[stm] & ~king_attack(first_bit(bitboard[KING+(stm^1)]));
    while(dest){
        to = first_bit_clear(dest);
        if(ml.get_size() >= 2 || !sq_attacked(to,stm,occupied^(BB_1<<kSq))){
            if(piece[to] != NO_PIECE)
                ml.add(move_create(MoveCapture,kSq,to),HistoryMax + piece_value(piece[to])-piece_value(WK));
			else ml.add(move_create(MoveQuiet,kSq,to),history[KING+stm][kSq][to]);
        }
    }

    checkers = current->checkers;
    if((checkers & (checkers -1))  == 0){
        checkerSq = first_bit(checkers);
        p = piece[checkerSq];
        if(p >= WB && p <= BR){
            possible = (this->*attack[p])(checkerSq) & (this->*attack[p])(kSq);
        }else if(p >= WQ){
             possible = bishop_attack(checkerSq);
             if(possible & bitboard[KING+stm]) possible &= bishop_attack(kSq);
             else possible = rook_attack(checkerSq) & rook_attack(kSq);
         }
         possible |= checkers;

         Bitboard empty = ~occupied;
         if(stm == White){
            bb = bitboard[WP];
            pawn_dest[0] = shift_up(shift_up(bb) & empty) & empty & possible & BB_RANK_4;
            pawn_dest[1] = shift_up(bb) & empty & possible;
            pawn_dest[2] = shift_up_right(bb) & bitboard[Black] & possible;
            pawn_dest[3] = shift_up_left(bb) & bitboard[Black] & possible ;
         }
         else{
            bb = bitboard[BP];
            pawn_dest[0] = shift_down(shift_down(bb) & empty) & empty & possible & BB_RANK_5;
            pawn_dest[1] = shift_down(bb) & empty & possible;
            pawn_dest[2] = shift_down_right(bb) & bitboard[White] & possible;
            pawn_dest[3] = shift_down_left(bb) & bitboard[White] & possible;
        }

        for(int i=0; i<4; i++){
            while(pawn_dest[i]){
                to = first_bit_clear(pawn_dest[i]);
                from = to + PawnFrom[stm][i];
                if(ml.get_size() >= 2 || !is_pinned(from,to)){
                    if(to >= A8 || to <= H1){
                        for(Move m = MovePQ; m >= MovePN; m-=2)
                            ml.add(move_create(m,from,to),HistoryMax+piece_value(Piece(m))+piece_value(piece[to])-piece_value(WP));
                    }
                    else{
						if(i<2) ml.add(move_create(MoveQuiet,from,to),history[KING+stm][from][to]);
                        else ml.add(move_create(MoveCapture,from,to),HistoryMax+piece_value(piece[to])-piece_value(WP));
                    }
                }
            }
        }

        if(current->ep != NO_SQ){
            if(stm == White){
                if((BB_1 << (current->ep-8)) & possible){
                    dest = bitboard[WP] & PawnAttack[Black][current->ep];
                    while(dest){
                        from = first_bit_clear(dest);
                        if(ml.get_size() >= 2 || !is_pinned(from,current->ep,current->ep-8))
                            ml.add(move_create(MoveEP,from,current->ep),HistoryMax);
                    }
                }
            }
            else{
                if((BB_1 << (current->ep+8)) & possible){
                    dest = bitboard[BP] & PawnAttack[White][current->ep];
                    while(dest){
                        from = first_bit_clear(dest);
                        if(ml.get_size() >= 2 || !is_pinned(from,current->ep,current->ep+8))
                            ml.add(move_create(MoveEP,from,current->ep),HistoryMax);
                    }
                }
            }
        }

        for(p=KNIGHT+stm; p<=BQ; p+=2){
            bb = bitboard[p];
            while(bb){
                from = first_bit_clear(bb);
                dest = (this->*attack[p])(from) & possible;
                while(dest){
                    to = first_bit_clear(dest);
                    if(!ml.get_size() >= 2 || !is_pinned(from,to)){
                        if(piece[to] != NO_PIECE)
                            ml.add(move_create(MoveCapture,from,to),HistoryMax+piece_value(piece[from])-piece_value(p));
						else ml.add(move_create(MoveQuiet,from,to),history[p][from][to]);
                    }
                }
            }
        }
    }
}

Bitboard Position::discover_check(const Square kSq) const{
        Bitboard bb, attack, disc = 0;
        Square sq;

        bb = bitboard[BISHOP+stm] | bitboard[QUEEN+stm];
        attack = bishop_attack(kSq) & bitboard[stm];
        while(attack){
                sq = first_bit_clear(attack);
                if(bishop_attack(kSq,occupied^(BB_1<<sq)) & bb)
                        disc |= BB_1<<sq;
        }

        bb = bitboard[ROOK+stm] | bitboard[QUEEN+stm];
        attack = rook_attack(kSq) & bitboard[stm];
        while(attack){
                sq = first_bit_clear(attack);
                if(rook_attack(kSq,occupied^(BB_1<<sq)) & bb)
                        disc |= BB_1<<sq;
        }
        return disc;
}

void Position::gen_quiet_checks(MoveList & ml) const{
        Bitboard bb, attack1, attack2, disc, empty, checkers;
        Square to, from, kSq;
        File f;

        kSq = first_bit(bitboard[KING+(stm^1)]);
        f = file(kSq);
        disc =  discover_check(kSq);
        empty = ~occupied;

        if(stm == White){
                bb = bitboard[WP] & ~ BB_FILE[f];
                attack2 = attack1 = ((bb & disc) << 8) & empty & ~BB_RANK_8;
                while(attack1){
                        to = first_bit_clear(attack1);
                        ml.add(move_create(MoveQuiet,to-8,to));
                }
                attack1 = ((attack2<< 8)& BB_RANK_4) & empty;
                while(attack1){
                        to = first_bit_clear(attack1);
                        ml.add(move_create(MoveQuiet,to-16,to));
                }
                if(f > FILE_A && f <FILE_H) attack1 = bb & ~disc & (BB_FILE[f-1] | BB_FILE[f+1]);
                else if (f == FILE_A) attack1 = bb & ~disc & BB_FILE[f+1];
                else attack1 = bb & ~disc & BB_FILE[f-1];
                attack2 = attack1 = (attack1<<8) & empty;
                attack1 &= PawnAttack[Black][kSq];
                while(attack1){
                        to = first_bit_clear(attack1);
                        ml.add(move_create(MoveQuiet,to-8,to));
                }
                attack1 = ((attack2<<8)& BB_RANK_4) & empty & PawnAttack[Black][kSq];
                while(attack1){
                        to = first_bit_clear(attack1);
                        ml.add(move_create(MoveQuiet,to-16,to));
                }
        }
        else{
                bb = bitboard[BP] & ~ BB_FILE[f];
                attack2 = attack1 = ((bb & disc) >> 8) & empty & ~BB_RANK_1;
                while(attack1){
                        to = first_bit_clear(attack1);
                        ml.add(move_create(MoveQuiet,to+8,to));
                }
                attack1 = ((attack2>> 8)& BB_RANK_5) & empty;
                while(attack1){
                        to = first_bit_clear(attack1);
                        ml.add(move_create(MoveQuiet,to+16,to));
                }
                if(f > FILE_A && f < FILE_H) attack1 = bb & ~disc & (BB_FILE[f-1] | BB_FILE[f+1]);
                else if (f == FILE_A) attack1 = bb & ~disc & BB_FILE[f+1];
                else attack1 = bb & ~disc & BB_FILE[f-1];
                attack2 = attack1 = (attack1>>8) & empty;
                attack1 &= PawnAttack[White][kSq];
                while(attack1){
                        to = first_bit_clear(attack1);
                        ml.add(move_create(MoveQuiet,to+8,to));
                }
                attack1 = ((attack2>>8)& BB_RANK_5) & empty & PawnAttack[White][kSq];
                while(attack1){
                        to = first_bit_clear(attack1);
                        ml.add(move_create(MoveQuiet,to+16,to));
                }
        }

        for(Piece p = KNIGHT+stm; p<=QUEEN+stm; p+=2){
                bb = bitboard[p];
                if(bb){
                        attack1 = bb & disc;
                        while(attack1){
                                from = first_bit_clear(attack1);
                                attack2 = (this->*attack[p])(from) & empty;
                                while(attack2){
                                        to = first_bit_clear(attack2);
                                        ml.add(move_create(MoveQuiet,from,to));
                                }
                        }
                        attack1 = bb & ~disc;
                        checkers = (this->*attack[p])(kSq) & empty;
                        while(attack1){
                                from = first_bit_clear(attack1);
                                attack2 = (this->*attack[p])(from) & checkers;
                                while(attack2){
                                        to = first_bit_clear(attack2);
                                        ml.add(move_create(MoveQuiet,from,to));
                                }
                        }
                }
        }

        if(bitboard[KING+stm] & disc){
                from = first_bit(bitboard[KING+stm]);
                attack1 = king_attack(from) & empty & (~king_attack(kSq));
                while(attack1){
                        to = first_bit_clear(attack1);
                        ml.add(move_create(MoveQuiet,from,to));
                }
        }
}
