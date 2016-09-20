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

#include <iostream>
#include <cstdlib>
#include <cassert>
#include <string>

#include "position.h"
#include "movegen.h"

void Position::move_do(const Move move){
    switch(move_type(move)){
        case MoveQuiet:     { move_quiet_do(move); break; }
        case MoveCapture:   { move_capture_do(move); break; }
        case MoveCastle:    { move_castle_do(move); break; }
        case MoveEP:        { move_ep_do(move); break;}
        case MovePN: case MovePB: case MovePR: case MovePQ:
                            { move_promotion_do(move); break;}
        case MoveNull:      { move_null_do(); break; }
        default:{ std::cerr<<"Wrong move type ("<<move_type(move)<<")!"<<std::endl; exit(1); }
    };

    stm ^= 1;
    current->key ^= StmKey;
    const Square sq = ph[ply-1].ep;
    if(sq != NO_SQ) current->key ^= EpKey[sq];
    current->checkers = get_checkers();

    #ifndef NDEBUG
    if(!is_ok()){
        std::cout<<"DO"<<std::endl;
        print();
        exit(1);
    }
    #endif
}

void Position::move_undo(){
    assert(ply > 0);

    ply--;
    stm ^= 1;
    current= &ph[ply];
    Move move = current->move;
    Move type =  move_type(move);

    switch(type){
        case MoveQuiet:     { move_quiet_undo(move); break; }
        case MoveCapture:   { move_capture_undo(move); break; }
        case MoveCastle:    { move_castle_undo(move); break; }
        case MoveEP:        { move_ep_undo(move); break; }
        case MovePN: case MovePB: case MovePR: case MovePQ:
                            { move_promotion_undo(move); break; }
        case MoveNull:      { break; }
        default:            { std::cerr<<"Wrong move type!"<<std::endl; exit(1); }
    };

    #ifndef NDEBUG
    if(!is_ok()){
        std::cerr<<"UNDO"<<std::endl;
        print();
        std::cerr<<"Move:"<<move_to_string(move)<<std::endl;
        exit(1);
    }
    #endif
}

void Position::move_quiet_do(const Move move){
    const Square from = move_from(move);
    const Square to = move_to(move);
    const Piece p = piece[from];

    current->move = move;
    ply++;
    current= &ph[ply];
    const Key key = SqKey[p][from] ^ SqKey[p][to];
    current->key = ph[ply-1].key ^ key;
    current->pawn_key = ph[ply-1].pawn_key;
    if(p <= BP) current->pawn_key ^= key;
    current->mat_key = ph[ply-1].mat_key;
    current->pvt[Opening] = ph[ply-1].pvt[Opening] + PVT[p][to][Opening] - PVT[p][from][Opening];
    current->pvt[Endgame] = ph[ply-1].pvt[Endgame] + PVT[p][to][Endgame] - PVT[p][from][Endgame];
    current->fifty = ph[ply-1].fifty + 1;
    current->castle = ph[ply-1].castle;
    current->capture = NO_PIECE;
    current->ep = NO_SQ;
    const Bitboard bb = BB_1 << from | BB_1 << to;
    bitboard[stm] ^= bb;
    bitboard[p] ^= bb;
    occupied ^= bb;
    piece[from] = NO_PIECE;
    piece[to] = p;
    if(stm == White){
        if(p == WP){
            current->fifty = 0;
            if(to - from == 16 && (bitboard[BP] & PawnAttack[White][to-8])){
                current->ep = to - 8;
                current->key ^= EpKey[current->ep];
            }
        }
        if(current->castle & W_CASTLE){
            if(current->castle & WK_CASTLE){
                if(from == E1 || from == H1){
                    current->key ^= CastleKey[WK_CASTLE];
                    current->castle &= ~ WK_CASTLE;
                }
            }
            if(current->castle & WQ_CASTLE){
                if(from == E1 || from == A1){
                    current->key ^= CastleKey[WQ_CASTLE];
                    current->castle &= ~WQ_CASTLE;
                }
            }
        }
    }
    else{
        if(p == BP){
            current->fifty = 0;
            if(from - to == 16 && (bitboard[WP] & PawnAttack[Black][to+8])){
                current->ep = to + 8;
                current->key ^= EpKey[current->ep];
            }
        }
        if(current->castle & B_CASTLE){
            if(current->castle & BK_CASTLE){
                if(from == E8 || from == H8){
                    current->key ^= CastleKey[BK_CASTLE];
                    current->castle &= ~BK_CASTLE;
                }
            }
            if(current->castle & BQ_CASTLE){
                if(from == E8 || from == A8){
                    current->key ^= CastleKey[BQ_CASTLE];
                    current->castle &= ~BQ_CASTLE;
                }
            }
        }
    }
}

void Position::move_quiet_undo(const Move move){
    const Square from = move_from(move);
    const Square to = move_to(move);
    const Piece p = piece[to];

    const Bitboard bb = BB_1 << from | BB_1 << to;
    bitboard[stm] ^= bb;
    bitboard[p] ^= bb;
    occupied ^= bb;
    piece[from] = p;
    piece[to] = NO_PIECE;
}


void Position::move_capture_do(const Move move){
    const Square from = move_from(move);
    const Square to = move_to(move);
    const Piece p = piece[from];
    const Piece c = piece[to];

    current->move = move;
    ply++;
    current= &ph[ply];
    current->key = ph[ply-1].key ^ SqKey[p][from] ^ SqKey[p][to] ^ SqKey[c][to];
    current->pawn_key = ph[ply-1].pawn_key;
    if(p <= BP) current->pawn_key ^= SqKey[p][from] ^ SqKey[p][to];
    if(c <= BP) current->pawn_key ^= SqKey[c][to];
    current->mat_key = ph[ply-1].mat_key ^ MatKey[c][num[c]];
    current->pvt[Opening] = ph[ply-1].pvt[Opening] + PVT[p][to][Opening] - PVT[p][from][Opening] - PVT[c][to][Opening];
    current->pvt[Endgame] = ph[ply-1].pvt[Endgame] + PVT[p][to][Endgame] - PVT[p][from][Endgame] - PVT[c][to][Endgame];
    current->fifty = 0;
    current->castle = ph[ply-1].castle;
    current->capture = c;
    current->ep = NO_SQ;
    num[stm^1] --;
    num[c] --;

    const Bitboard bbTo = BB_1 << to;
    const Bitboard bbMove = BB_1 <<from | bbTo;
    occupied ^= BB_1 << from;
    bitboard[stm] ^= bbMove;
    bitboard[p] ^= bbMove;
    bitboard[stm^1] ^= bbTo;
    bitboard[c] ^= bbTo;
    piece[from] = NO_PIECE;
    piece[to] = p;
    if(stm == White){
       if(current->castle & W_CASTLE){
           if(current->castle & WK_CASTLE){
               if(from == E1 || from == H1){
                   current->key ^= CastleKey[WK_CASTLE];
                   current->castle &= ~WK_CASTLE;
               }
           }
           if(current->castle & WQ_CASTLE){
               if(from == E1 || from == A1){
                   current->key ^= CastleKey[WQ_CASTLE];
                   current->castle &= ~WQ_CASTLE;
               }
           }
       }
       if(current->castle & B_CASTLE){
           if(current->castle & BK_CASTLE){
               if(to == H8){
                   current->key ^= CastleKey[BK_CASTLE];
                   current->castle &= ~BK_CASTLE;
               }
           }
           if(current->castle & BQ_CASTLE){
               if(to == A8){
                   current->key ^= CastleKey[BQ_CASTLE];
                   current->castle &= ~BQ_CASTLE;
               }
           }
       }
   }
   else{
       if(current->castle & B_CASTLE){
           if(current->castle & BK_CASTLE){
               if(from == E8 || from == H8){
                   current->key ^= CastleKey[BK_CASTLE];
                   current->castle &= ~BK_CASTLE;
               }
           }
           if(current->castle & BQ_CASTLE){
               if(from == E8 || from == A8){
                   current->key ^= CastleKey[BQ_CASTLE];
                   current->castle &= ~BQ_CASTLE;
               }
           }
       }
       if(current->castle & W_CASTLE){
           if(current->castle & WK_CASTLE){
               if(to == H1){
                   current->key ^= CastleKey[WK_CASTLE];
                   current->castle &= ~WK_CASTLE;
               }
           }
           if(current->castle & WQ_CASTLE){
               if(to == A1){
                   current->key ^= CastleKey[WQ_CASTLE];
                   current->castle &= ~WQ_CASTLE;
               }
           }
       }
   }
}

void Position::move_capture_undo(const Move move){
     const Square from = move_from(move);
     const Square to = move_to(move);
     const Piece p = piece[to];
     const Piece c = ph[ply+1].capture;

     const Bitboard bbTo = BB_1 << to;
     const Bitboard bbMove = BB_1 <<from | bbTo;
     occupied ^= BB_1 << from;
     bitboard[stm] ^= bbMove;
     bitboard[p] ^= bbMove;
     bitboard[stm^1] ^= bbTo;
     bitboard[c] ^= bbTo;
     num[stm^1] ++;
     num[c]++;
     piece[from] = p;
     piece[to] = ph[ply+1].capture;
}

void Position::move_ep_do(const Move move){
    const Square from = move_from(move);
    const Square to = move_to(move);
    const Piece p = PAWN + stm;
    const Piece c = PAWN+(stm^1);
    Square cSq;

    if(stm == Black) cSq = to+8;
    else cSq = to-8;

    current->move = move;
    ply++;
    current= &ph[ply];
    const Key key = SqKey[p][from] ^ SqKey[p][to] ^ SqKey[c][cSq];
    current->key = ph[ply-1].key ^ key;
    current->pawn_key = ph[ply-1].pawn_key ^ key;
    current->mat_key = ph[ply-1].mat_key ^ MatKey[c][num[c]];
    current->pvt[Opening] = ph[ply-1].pvt[Opening] + PVT[p][to][Opening] - PVT[p][from][Opening] - PVT[c][cSq][Opening];
    current->pvt[Endgame] = ph[ply-1].pvt[Endgame] + PVT[p][to][Endgame] - PVT[p][from][Endgame] - PVT[c][cSq][Endgame];
    current->fifty = 0;
    current->castle = ph[ply-1].castle;
    current->capture = c;
    current->ep = NO_SQ;

    num[stm^1] --;
    num[c] --;
    const Bitboard bbMove = BB_1 <<from | BB_1 <<to;
    const Bitboard bbCapture = BB_1 << cSq;
    bitboard[stm] ^= bbMove;
    bitboard[p] ^= bbMove;
    bitboard[stm^1] ^= bbCapture;
    bitboard[c] ^= bbCapture;
    occupied ^= bbMove | bbCapture;
    piece[from] = NO_PIECE;
    piece[cSq] = NO_PIECE;
    piece[to] = p;
}

void Position::move_ep_undo(const Move move){
    Square cSq;
    const Square from = move_from(move);
    const Square to = move_to(move);
    const Piece p= PAWN+stm;
    const Piece c = PAWN+(stm^1);

    if(stm == White) cSq = to-8;
    else cSq = to+8;

    const Bitboard bbMove = BB_1<<from | BB_1<<to;
    const Bitboard bbCapture = BB_1<<cSq;
    bitboard[stm] ^= bbMove;
    bitboard[p] ^= bbMove;
    bitboard[stm^1] ^= bbCapture;
    bitboard[c] ^= bbCapture;
    num[stm^1] ++;
    num[c]++;
    occupied ^= bbMove | bbCapture;
    piece[from] = p;
    piece[to] = NO_PIECE;
    piece[cSq] = PAWN+(stm^1);
}

void Position::move_castle_do(const Move move){
    Square rFrom, rTo;
    const Square to = move_to(move);
    const Square from = move_from(move);
    const Piece p = KING + stm;
    const Piece r = ROOK + stm;
    switch(to){
        case G1: {
            rFrom = H1;
            rTo = F1;
            break;
        }
        case C1: {
            rFrom = A1;
            rTo = D1;
            break;
        }
        case G8: {
            rFrom = H8;
            rTo = F8;
            break;
        }
        case C8: {
            rFrom = A8;
            rTo = D8;
            break;
        }
        default: {
            std::cerr<<"Wrong castle destination!"<<std::endl;
            print();
            std::cerr<<"Move:"<<move_to_string(move)<<std::endl;
            exit(1);
        }
    }

    current->move = move;
    ply++;
    current= &ph[ply];
    current->key = ph[ply-1].key ^ SqKey[p][from] ^ SqKey[p][to] ^ SqKey[r][rFrom] ^ SqKey[r][rTo];
    current->pawn_key = ph[ply-1].pawn_key;
    current->mat_key = ph[ply-1].mat_key;
    current->pvt[Opening] = ph[ply-1].pvt[Opening] + PVT[p][to][Opening] + PVT[r][rTo][Opening] - PVT[p][from][Opening] - PVT[r][rFrom][Opening];
    current->pvt[Endgame] = ph[ply-1].pvt[Endgame] + PVT[p][to][Endgame] + PVT[r][rTo][Endgame] - PVT[p][from][Endgame] - PVT[r][rFrom][Endgame];
    current->fifty = 0;
    current->capture = NO_PIECE;
    current->ep = NO_SQ;
    current->castle = ph[ply-1].castle;
    if(stm == White){
        if(current->castle & WK_CASTLE)
            current->key^= CastleKey[WK_CASTLE];
        if(current->castle & WQ_CASTLE)
            current->key^= CastleKey[WQ_CASTLE];
        current->castle &= ~W_CASTLE;
    }else{
        if(current->castle & BK_CASTLE)
            current->key^= CastleKey[BK_CASTLE];
        if(current->castle & BQ_CASTLE)
            current->key^= CastleKey[BQ_CASTLE];
        current->castle &= ~B_CASTLE;
    }

    const Bitboard bbMove = BB_1<<from | BB_1<<to;
    const Bitboard bbRook = BB_1<<rFrom | BB_1<<rTo;
    bitboard[stm] ^= bbMove | bbRook;
    occupied ^= bbMove | bbRook;
    bitboard[p] ^= bbMove;
    bitboard[r] ^= bbRook;
    piece[from] = piece[rFrom] = NO_PIECE;
    piece[to] = p;
    piece[rTo] = r;
}

void Position::move_castle_undo(const Move move){
    Piece p, r;
    Square rFrom, rTo;
    const Square from = move_from(move);
    const Square to = move_to(move);
    switch(to){
        case G1: {
            rFrom = H1;
            rTo = F1;
            p = WK;
            r = WR;
            break;
        }
        case C1: {
            rFrom = A1;
            rTo = D1;
            p = WK;
            r = WR;
            break;
        }
        case G8: {
            rFrom = H8;
            rTo = F8;
            p = BK;
            r = BR;
            break;
        }
        case C8: {
            rFrom = A8;
            rTo = D8;
            p = BK;
            r = BR;
            break;
        }
        default: {
            std::cerr<<"Wrong castle destination!"<<std::endl;
            print();
            std::cerr<<"Move:"<<move_to_string(move)<<std::endl;
            exit(1);
        }
    }

    const Bitboard bbMove = BB_1 <<from | BB_1 <<to;
    const Bitboard bbRook = BB_1 <<rFrom | BB_1<<rTo;
    bitboard[stm] ^= bbMove | bbRook;
    occupied ^= bbMove | bbRook;
    bitboard[p] ^= bbMove;
    bitboard[r] ^= bbRook;
    piece[from] = p;
    piece[rFrom] = r;
    piece[rTo] = piece[to] = NO_PIECE;
}

void Position::move_promotion_do(const Move move){
    const Square from = move_from(move);
    const Square to = move_to(move);
    const Piece p = Piece(PAWN + stm);
    const Piece pp = Piece(move_type(move) + stm);
    const Piece c =piece[to];

    current->move = move;
    ply++;
    current= &ph[ply];
    current->key = ph[ply-1].key ^ SqKey[p][from] ^ SqKey[pp][to];
    current->pawn_key = ph[ply-1].pawn_key ^ SqKey[p][from];
    current->mat_key = ph[ply-1].mat_key ^ MatKey[p][num[p]] ^ MatKey[pp][num[pp]+1];
    current->pvt[Opening] = ph[ply-1].pvt[Opening] + PVT[pp][to][Opening] - PVT[p][from][Opening];
    current->pvt[Endgame] = ph[ply-1].pvt[Endgame] + PVT[pp][to][Endgame] - PVT[p][from][Endgame];
    current->fifty = 0;
    current->castle = ph[ply-1].castle;
    current->capture = NO_PIECE;
    current->ep = NO_SQ;
    current->capture = c;

    num[p]--;
    num[pp] ++;

    const Bitboard bbFrom = BB_1<<from;
    const Bitboard bbTo = BB_1<<to;
    bitboard[stm] ^= bbFrom | bbTo;
    occupied ^= bbFrom | bbTo;
    bitboard[p] ^= bbFrom;
    bitboard[pp] ^= bbTo;

    current->fifty = 0;
    piece[from] = NO_PIECE;
    piece[to] = pp;
    if(c != NO_PIECE){
        if(stm == White){
            if(current->castle & B_CASTLE){
                if(current->castle & BK_CASTLE){
                    if(to == H8){
                        current->key ^= CastleKey[BK_CASTLE];
                        current->castle &= ~BK_CASTLE;
                    }
                }
                if(current->castle & BQ_CASTLE){
                    if(to == A8){
                        current->key ^= CastleKey[BQ_CASTLE];
                        current->castle &= ~BQ_CASTLE;
                    }
                }
            }
        }
        else{
            if(current->castle & W_CASTLE){
                if(current->castle & WK_CASTLE){
                    if(to == H1){
                        current->key ^= CastleKey[WK_CASTLE];
                        current->castle &= ~WK_CASTLE;
                    }
                }
                if(current->castle & WQ_CASTLE){
                    if(to == A1){
                        current->key ^= CastleKey[WQ_CASTLE];
                        current->castle &= ~WQ_CASTLE;
                    }
                }
            }
        }
        bitboard[stm^1] ^= bbTo;
        bitboard[c] ^= bbTo;
        occupied ^= bbTo;
        current->key ^= SqKey[c][to];
        current->mat_key ^= MatKey[c][num[c]];
        current->pvt[Opening] -= PVT[current->capture][to][Opening];
        current->pvt[Endgame] -= PVT[current->capture][to][Endgame];
        num[stm^1] --;
        num[c]--;
    }
}

void Position::move_promotion_undo(const Move move){
    const Square from = move_from(move);
    const Square to = move_to(move);
    const Piece p = Piece(PAWN + stm);
    const Piece pp = Piece(move_type(move) + stm);
    const Piece c = ph[ply+1].capture;

    const Bitboard bbFrom = BB_1<<from;
    const Bitboard bbTo = BB_1<<to;
    bitboard[stm] ^= bbFrom | bbTo;
    occupied ^= bbFrom | bbTo;
    bitboard[p] ^= bbFrom;
    bitboard[pp] ^= bbTo;
    num[p] ++;
    num[pp] --;
    piece[from] = p;
    piece[to] = NO_PIECE;
    if(c != NO_PIECE){
        piece[to] = c;
        bitboard[stm^1] ^= bbTo;
        bitboard[c] ^= bbTo;
        num[stm^1] ++;
        num[c] ++;
        occupied ^= bbTo;
    }
}

void Position::move_null_do(){
    current->move = MoveNull;
    ply++;
    current= &ph[ply];
    current->key = ph[ply-1].key;
    current->pvt[Opening] = ph[ply-1].pvt[Opening];
    current->pvt[Endgame] = ph[ply-1].pvt[Endgame];
    current->pawn_key = ph[ply-1].pawn_key;
    current->mat_key = ph[ply-1].mat_key;
    current->fifty = 0;
    current->castle = ph[ply-1].castle;
    current->capture = NO_PIECE;
    current->ep = NO_SQ;
}

int Position::see(const Move move) const{
    Piece p, c;
    Square from, to, sq;
    Color att, def;
    Bitboard bb, attack;
    Move t;

    from = move_from(move);
    to = move_to(move);
    p = piece[from];

    att = Color(p % 2);
    def = att ^ 1;
    bb = occupied ^ (BB_1 << from);
    t = move_type(move);

    if(t >= MovePN && t <= MovePQ) p = Piece(t);

    if(t == MoveEP){
        if(p == WP){
            c = BP;
            bb ^= BB_1 << (to - 8);
        }
        if(p == BP){
            c = WP;
            bb ^= BB_1 << (to + 8);
        }
    }
    else c = piece[to];

    attack = rook_attack(to,bb) & (bitboard[WR] | bitboard[BR] | bitboard[WQ] | bitboard[BQ]);
    attack |= bishop_attack(to,bb) & (bitboard[WB] | bitboard[BB] | bitboard[WQ] | bitboard[BQ]);
    attack |= knight_attack(to) & (bitboard[WN] | bitboard[BN]);
    attack |= king_attack(to) & (bitboard[WK] | bitboard[BK]);
    attack |= (shift_up_right(BB_1<<to) | shift_up_left(BB_1<<to)) & bitboard[BP];
    attack |= (shift_down_right(BB_1<<to) | shift_down_left(BB_1<<to)) & bitboard[WP];
    attack &= bb;

    if((attack & bitboard[def]) == 0) return piece_value(c);

    int last_att_value = piece_value(p), n=1, swap_list[32];
    Color color = def;
    swap_list[0] = piece_value(c);

    do{
        for(p = PAWN+color; p<=BK; p+=2)
            if(bitboard[p] & attack) break;

        sq = first_bit(bitboard[p] & attack);
        bb ^= (BB_1<<sq);
        attack |= rook_attack(to,bb) & (bitboard[WR] | bitboard[BR] | bitboard[WQ] | bitboard[BQ]);
        attack |= bishop_attack(to,bb) & (bitboard[WB] | bitboard[BB] | bitboard[WQ] | bitboard[BQ]);
        attack &= bb;
        swap_list[n] = - swap_list[n-1] + last_att_value;

        n++;
        last_att_value = piece_value(p);
        color ^= 1;

        if(p >= WK && (attack & bitboard[color])){
            n--;
            break;
        }
    }while(attack & bitboard[color]);

    while(--n) swap_list[n-1] = std::min(-swap_list[n], swap_list[n-1]);

    return swap_list[0];
}

bool Position::move_legal(const Move move) const{
    Piece p = piece[move_from(move)];
    if(p == NO_PIECE || p % 2 != stm) return false;
    Bitboard bbFrom = BB_1<<move_from(move);
    Bitboard bbTo = BB_1<<move_to(move);
    Bitboard dest;
    Move t = move_type(move);
    if(p == WP){
        if(t == MoveQuiet){
            dest = (~occupied) & (~BB_RANK_8);
            if(shift_up(bbFrom) & dest & bbTo) return true;
            if(shift_up(shift_up(bbFrom) & dest) & BB_RANK_4 & dest & bbTo)
                return true;
        }
        else if(t == MoveCapture){
            dest = bitboard[stm^1] & (~BB_RANK_8);
            if(shift_up_right(bbFrom) & dest & bbTo) return true;
            if(shift_up_left(bbFrom) & dest & bbTo) return true;
        }
        else if(t == MoveEP){
            if(move_to(move) == current->ep && bbFrom & PawnAttack[Black][current->ep])
                return true;
        }
        else if(t >= MovePN && t <= MovePQ){
            dest = (~occupied) & BB_RANK_8;
            if(shift_up(bbFrom) & dest & bbTo) return true;
            dest = bitboard[stm^1] & BB_RANK_8;
            if(shift_up_right(bbFrom) & dest & bbTo) return true;
            if(shift_up_left(bbFrom) & dest & bbTo) return true;
        }
    }
    else if(p == BP){
        if(t == MoveQuiet){
            dest = (~occupied) & ~BB_RANK_1;
            if(shift_down(bbFrom) & dest & bbTo) return true;
            if(shift_down(shift_down(bbFrom) & dest) & BB_RANK_5 & dest & bbTo)
                return true;
        }
        else if(t == MoveCapture){
            dest = bitboard[stm^1] & (~BB_RANK_1);
            if(shift_down_right(bbFrom) & dest & bbTo) return true;
            if(shift_down_left(bbFrom) & dest & bbTo) return true;
        }
        else if(t == MoveEP){
            if(move_to(move) == current->ep && bbFrom & PawnAttack[White][current->ep])
                return true;
        }
        else if(t >= MovePN && t <= MovePQ){
            dest = (~occupied) & BB_RANK_1;
            if(shift_down(bbFrom) & dest & bbTo) return true;
            dest = bitboard[stm^1] & BB_RANK_1;
            if(shift_down_right(bbFrom) & dest & bbTo) return true;
            if(shift_down_left(bbFrom) & dest & bbTo) return true;
        }
    }
    else if (p < WK){
        if(t == MoveQuiet){
            if((this->*attack[p])(move_from(move)) & (~occupied) & bbTo)
                return true;
        }
        else if(t == MoveCapture){
            if((this->*attack[p])(move_from(move)) & bitboard[stm^1] & bbTo)
                return true;
        }
    }
    else{
        if(t == MoveQuiet){
            if(king_attack(move_from(move)) & (~occupied) & bbTo & ~king_attack(first_bit(bitboard[KING+(stm^1)])))
                return true;
        }
        else if(t == MoveCapture){
            if(king_attack(move_from(move)) & bitboard[stm^1] & bbTo & ~king_attack(first_bit(bitboard[KING+(stm^1)])))
                return true;
        }
        else if(t == MoveCastle){
            dest = ~occupied;
            if(stm == White){
               if((bbTo & BB_G1)
                   && !current->checkers
                   && (current->castle & WK_CASTLE)
                   && (((BB_F1|BB_G1) & dest) == (BB_F1|BB_G1))
                   && (((BB_F2|BB_G2|BB_H2) & bitboard[BK]) == 0)
                   && !sq_attacked(F1,White))
                   return true;
               if((bbTo & BB_C1)
                   && !current->checkers
                   && (current->castle & WQ_CASTLE)
                   && ((BB_B1|BB_C1|BB_D1)&dest)==(BB_B1|BB_C1|BB_D1)
                   && (((BB_B2|BB_C2|BB_D2) & bitboard[BK]) == 0)
                   && !sq_attacked(C1,White))
                   return true;
            }
            else{
                if((bbTo & BB_G8)
                    && !current->checkers
                    && (current->castle & BK_CASTLE)
                    && (((BB_F8|BB_G8) & dest) == (BB_F8|BB_G8))
                    && (((BB_F7|BB_G7|BB_H7) & bitboard[WK]) == 0)
                    && !sq_attacked(F8,Black))
                    return true;
                if((bbTo & BB_C8)
                    && !current->checkers
                    && (current->castle & BQ_CASTLE)
                    && ((BB_B8|BB_C8|BB_D8)&dest)==(BB_B8|BB_C8|BB_D8)
                    && (((BB_B7|BB_C7|BB_D7) & bitboard[WK]) == 0)
                    && !sq_attacked(C8,Black))
                    return true;
            }
        }
    }
    return false;
}

bool Position::move_gen_legal(const Move move) const{
    Square from = move_from(move);
    Square to = move_to(move);
    Piece p = piece[from];

    switch(move_type(move)){
        case MoveQuiet:
        case MoveCapture:{
            if(p >= KING){ if(is_attacked(from,to)) return false; }
            else{ if(is_pinned(from,to)) return false; }
            break;
        }
        case MoveCastle:{
            if(is_attacked(from,to)) return false;
            break;
        }
        case MoveEP:{
            Square cSq;
            if(stm == Black) cSq = to+8;
            else cSq = to-8;
            if(is_pinned(from,to,cSq)) return false;
            break;
        }
        case MovePN: case MovePB: case MovePR: case MovePQ:{
            if(is_pinned(from,to)) return false;
            break;
        }
        default: { std::cerr<<"Wrong move type!"<<std::endl; exit(1); }
    };
    return true;
}

bool Position::is_move_check(const Move move) const{
    const Square from = move_from(move);
    const Square to = move_to(move);
    Piece p = piece[from];

    if(p <= BP){
        if(PawnAttack[stm][to] & bitboard[KING + (stm^1)])
            return true;
        const Square kSq = first_bit(bitboard[KING + (stm^1)]);
        const Bitboard dc = discover_check(kSq);
        if(dc & (BB_1 << from) && Direction[kSq][from] != Direction[from][to])
            return true;
        Move type = move_type(move);
        if(type == MoveEP){
            Square cSq;
            if(stm == White) cSq = to - 8;
            else cSq = to + 8;
            const Bitboard bb = occupied ^ (BB_1 << from) ^ (BB_1 << to) ^ (BB_1 << cSq);
            if((bishop_attack(kSq,bb) & (bitboard[QUEEN+stm] | bitboard[BISHOP+stm])) ||
                (rook_attack(kSq,bb) & (bitboard[QUEEN+stm] | bitboard[ROOK+stm])))
                return true;
        }
        else if(type >= MovePN && type <= MovePQ){
            p = Piece(type + stm);
            if((*attack_bb[p])(to,occupied ^ (BB_1<<from)) & bitboard[KING + (stm^1)])
                return true;
        }
    }
    else if(p <= BQ){
         if((*attack_bb[p])(to,occupied ^ (BB_1<<from)) & bitboard[KING + (stm^1)])
            return true;
         const Square kSq = first_bit(bitboard[KING + (stm^1)]);
         const Bitboard dc = discover_check(kSq);
         if(dc & (BB_1 << from)) return true;
    }
    else{
         const Square kSq = first_bit(bitboard[KING + (stm^1)]);
         const Bitboard dc = discover_check(kSq);
         if(dc & (BB_1 << from) && Direction[from][kSq] != Direction[from][to])
             return true;
         Move type = move_type(move);
         if(type == MoveCastle){
             const Bitboard bb = occupied ^ (BB_1<<from);
             Square rTo;
             switch(to){
                 case G1:{ rTo = F1; break; }
                 case C1:{ rTo = D1; break; }
                 case G8:{ rTo = F8; break; }
                 case C8:{ rTo = D8; break; }
                 default:{ std::cerr<<"Wrong square (move_is_check)"<<std::endl; exit(1); }
             };
             if(rook_attack(kSq,bb) & (BB_1 << rTo)) return true;
         }
    }
    return false;
}

bool Position::is_dangerous(const Move move) const{
    if(piece[move_from(move)]<=BP && is_passed(move_to(move))) return true;
    return false;
}

bool Position::is_capture_dangerous(const Move move) const{
    Piece p = piece[move_from(move)];
    const Square to = move_to(move);
    if(p == WP && rank(to) >= RANK_7) return true;
    if(p == BP && rank(to) <= RANK_2) return true;

    p = piece[to];
    if(p >= QUEEN) return true;

    if(p == WP && rank(to) <= RANK_2) return true;
    if(p == BP && rank(to) >= RANK_7) return true;

    return false;
}

int Position::move_value(const Move move) const{
    Move type = move_type(move);
    switch (type){
        case MoveQuiet:     { return 0; }
        case MoveCapture :  {return piece_value(piece[move_to(move)]); }
        case MoveEP:        { return piece_value(Piece(PAWN)); }
        case MoveCastle:    { return 0; }
        case MovePN:        { return piece_value(Piece(KNIGHT)) - piece_value(Piece(PAWN)) ; }
        case MovePB:        { return piece_value(Piece(BISHOP)) - piece_value(Piece(PAWN)) ; }
        case MovePR:        { return piece_value(Piece(ROOK)) - piece_value(Piece(PAWN)) ; }
        case MovePQ:        { return piece_value(Piece(QUEEN)) - piece_value(Piece(PAWN)) ; }
        default:{ std::cerr<<"Wrong move type (move_value)"<<std::endl; exit(1); }
    };

}

void Position::move_do(const std::string move){
    MoveGenerator mg(*this);
    Move m;
	int eval;
	while((m = mg.next(eval)) != MoveNull){
        if(move_to_string(m) == move){
            move_do(m);
            if(ply >= 50) strip();
            return;
        }
    }
    std::cerr<<"Wrong move:"<<move<<"!"<<std::endl;
    exit(1);
}
