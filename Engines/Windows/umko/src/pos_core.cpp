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
#include <string>
#include <algorithm>

using namespace std;

#include "position.h"
#include "thread.h"
#include "movegen.h"

bool Position::initialized = false;

Position::Position() {
    if(!initialized){
        init();
        initialized = true;
    }
}

inline void Position::add(const Piece p, const Square sq){
    const Bitboard bb = BB_1 << sq;
    const Color c = Color(p % 2);
    bitboard[c] ^= bb;
    bitboard[p] ^= bb;
    num[c] ++;
    num[p] ++;
    occupied ^= bb;
    const Key key = SqKey[p][sq];
    current->key ^= key;
    if(p <= BP) current->pawn_key ^= key;
    current->mat_key ^= MatKey[p][num[p]];
    current->pvt[Opening] += PVT[p][sq][Opening];
    current->pvt[Endgame] += PVT[p][sq][Endgame];
    piece[sq] = p;
}

inline void Position::remove(const Square sq){
    const Piece p = piece[sq];
    const Bitboard bb = BB_1 << sq;
    const Color c = Color(p % 2);
    bitboard[c] ^= bb;
    bitboard[p] ^= bb;
    num[c] --;
    num[p] --;
    occupied ^= bb;
    const Key key = SqKey[p][sq];
    ph[ply].key ^= key;
    if(p <= BP) ph[ply].pawn_key ^= key;
    ph[ply].mat_key ^= MatKey[p][num[p]];
    current->pvt[Opening] -= PVT[p][sq][Opening];
    current->pvt[Endgame] -= PVT[p][sq][Endgame];
    piece[sq] = NO_PIECE;
}

void Position::empty(){
    ply = 0;
    stm = White;
    for(Piece p=W; p<=BK; p++){
        bitboard[p] = 0;
        num[p] = 0;
    }
    for(Square sq=A1; sq<=H8; sq++) piece[sq] = NO_PIECE;
    occupied = 0;
    current = &ph[ply];
    current->capture = NO_PIECE;
    current->key = current->mat_key = current->pawn_key = 0;
    current->castle = N0_CASTLE;
    current->ep = NO_SQ;
    current->checkers = 0;
    current->pvt[Opening] = current->pvt[Endgame] = 0;
}

void Position::start(){
    empty();
    for(Square sq = A2; sq <= H2; sq++) add(WP, sq);
    for(Square sq = A7; sq <= H7; sq++) add(BP, sq);
    add(WR,A1);
    add(WN,B1);
    add(WB,C1);
    add(WQ,D1);
    add(WK,E1);
    add(WB,F1);
    add(WN,G1);
    add(WR,H1);
    add(BR,A8);
    add(BN,B8);
    add(BB,C8);
    add(BQ,D8);
    add(BK,E8);
    add(BB,F8);
    add(BN,G8);
    add(BR,H8);
    stm = White;
    current->castle = Castle(W_CASTLE | B_CASTLE);
    current->key ^= CastleKey[WK_CASTLE] ^ CastleKey[WQ_CASTLE]
                    ^ CastleKey[BK_CASTLE] ^  CastleKey[BQ_CASTLE];
}

void Position::print() const{
    Piece p;
    std::string board = "# # # # \n # # # #\n# # # # \n # # # #\n# # # # \n # # # #\n# # # # \n # # # #";
    for(Square sq = A1; sq<=H8; sq++){
        p = piece[sq];
        if(p != NO_PIECE)
            board[70-rank(sq)*9-(7-file(sq))] = piece_to_char(p);
    }
    std::cout<<board<<std::endl;
    if(stm == White) std::cout << "stm: White"<<std::endl;
    else std::cout<<"stm: Black"<<std::endl;
    std::cout<<"ep: "<<square_to_string(ph[ply].ep)<<std::endl;
    std::cout<<"Castle: w( ";
    if(ph[ply].castle & WK_CASTLE) std::cout<<"00 ";
    if(ph[ply].castle & WQ_CASTLE) std::cout<<"000 ";
    std::cout<<") b( ";
    if(ph[ply].castle & BK_CASTLE) std::cout<<"00 ";
    if(ph[ply].castle & BQ_CASTLE) std::cout<<"000 ";
    std::cout<<")"<<std::endl;
    std::cout<<"check: "<<is_in_check()<<std::endl;
    std::cout<<"Key: "<<current->key<<std::endl;
    std::cout<<"Moves: ";
    for(int i=0; i<ply; i++)
        std::cout<<i+1<<". "<<move_to_string(ph[i].move)<<" ";
    std::cout<<std::endl<<std::flush;
}

bool Position::is_ok()const{
    Piece p;
    Key key = 0, pawn_key = 0, mat_key = 0;
    int count[17];
    for(int i=0; i<17; i++) count[i] = 0;
    for(Square sq=A1; sq<=H8; sq++){
        p = piece[sq];
        if(p == NO_PIECE){
            for(Piece cp=W; cp<=BK; cp++){
               if((bitboard[cp] & (BB_1<<sq)) != 0){
                    std::cerr<<"Wrong bitboard(1)["<<piece_to_char(cp)<<"] sq="<<square_to_string(sq)<<std::endl;
                    return false;
                }
            }
            if(((BB_1 << sq) & occupied) != 0){
                std::cerr<<"Wrong occupied sq="<<square_to_string(sq)<<std::endl;
                return false;
            }
            continue;
        }else{
            key ^= SqKey[p][sq];
            if(p<= BP) pawn_key ^= SqKey[p][sq];
            count[p] ++;
            mat_key ^= MatKey[p][count[p]];
        }
        if((bitboard[p%2] & (BB_1<<sq)) == 0){
            std::cerr<<"Wrong bitboard["<<piece_to_char(Piece(p%2))<<"] sq="<<square_to_string(sq)<<std::endl;
            return false;
        }
        if((bitboard[(p%2)^1] & (BB_1<<sq)) != 0){
            std::cerr<<"Wrong bitboard["<<piece_to_char(Piece((p%2)^1))<<"] sq="<<square_to_string(sq)<<std::endl;
            return false;
        }
        if((bitboard[p] & (BB_1<<sq)) == 0){
            std::cerr<<"Wrong bitboard["<<piece_to_char(p)<<"] sq="<<square_to_string(sq)<<std::endl;
            return false;
        }
        if(((BB_1 << sq) & occupied) == 0){
            std::cerr<<"Wrong occupied sq="<<square_to_string(sq)<<std::endl;
            return false;
        }
        for(Piece j= WP; j<=BK; j++){
            if(j == p) continue;
            if((bitboard[j] & (BB_1<<sq)) != 0){
                std::cerr<<"Wrong bitboard["<<piece_to_char(j)<<"] sq="<<square_to_string(sq)<<std::endl;
                return false;
            }
        }
    }
    if(stm == Black) key ^= StmKey;
    if(current->castle & WK_CASTLE) key ^= CastleKey[WK_CASTLE];
    if(current->castle & WQ_CASTLE) key ^= CastleKey[WQ_CASTLE];
    if(current->castle & BK_CASTLE) key ^= CastleKey[BK_CASTLE];
    if(current->castle & BQ_CASTLE) key ^= CastleKey[BQ_CASTLE];
    if(current->ep != NO_SQ) key ^= EpKey[current->ep];
    if(key != current->key){
        std::cerr<<"Wrong key"<<std::endl;
        return false;
    }
    if(pawn_key != current->pawn_key){
        std::cerr<<"Wrong pawn key"<<std::endl;
        return false;
    }
    if(mat_key != current->mat_key){
        std::cerr<<"Wrong material key"<<std::endl;
        return false;
    }
    if(bitboard[WK] == 0 || bit_count_3(bitboard[WK]) != num[WK] || num[WK] != 1){
        std::cerr<<"Wrong bitboard["<<piece_to_char(WK)<<"]"<<std::endl;
        return false;
    }
    if(bitboard[BK] == 0 ||  bit_count_3(bitboard[BK]) != num[BK] || num[BK] != 1){
        std::cerr<<"Wrong bitboard["<<piece_to_char(BK)<<"]"<<std::endl;
        return false;
    }
    if((bitboard[WP] != 0 && bit_count(bitboard[WP]) != num[WP]) || num[WP] > 8){
        std::cerr<<"Wrong bitboard["<<piece_to_char(WP)<<"]"<<std::endl;
        return false;
    }
    if((bitboard[BP] != 0 && bit_count(bitboard[BP]) != num[BP]) || num[BP] > 8){
        std::cerr<<"Wrong bitboard["<<piece_to_char(BP)<<"]"<<std::endl;
        return false;
    }
    if((bitboard[WP] | bitboard[BP]) & (BB_RANK_1 | BB_RANK_8)){
        std::cerr<<"Wrong position (pawn)"<<std::endl;
        return false;
    }
    Square sq = first_bit(bitboard[KING+(stm^1)]);
    if(sq_attacked(sq,(stm^1))){
        std::cerr<<"Wrong position (the king is attacked)"<<std::endl;
        return false;
    }
    if(king_attack(first_bit(bitboard[WK])) & bitboard[BK]){
        std::cerr<<"Wrong position (the white king is a neighbor of the black king)"<<std::endl;
        return false;
    }
    for(Piece p=Piece(KNIGHT); p<Piece(QUEEN); p++){
        if((bitboard[p] != 0 && bit_count(bitboard[p]) != num[p]) || num[p] > 10){
            std::cerr<<"Wrong bitboard["<<piece_to_char(p)<<"]"<<std::endl;
            return false;
        }
    }
    if((bitboard[WQ] != 0 && bit_count(bitboard[WQ]) != num[WQ]) || num[WQ] > 9){
        std::cerr<<"Wrong bitboard["<<piece_to_char(WQ)<<"]"<<std::endl;
        return false;
    }
    if((bitboard[BQ] != 0 && bit_count(bitboard[BQ]) != num[BQ]) || num[BQ] > 9){
        std::cerr<<"Wrong bitboard["<<piece_to_char(BQ)<<"]"<<std::endl;
        return false;
    }
    if(bit_count(bitboard[W]) != num[W] || num[W] > 16){
        std::cerr<<"Wrong bitboard["<<piece_to_char(W)<<"]"<<std::endl;
        return false;
    }
    if(bit_count(bitboard[B]) != num[B] || num[B] > 16){
        std::cerr<<"Wrong bitboard["<<piece_to_char(B)<<"]"<<std::endl;
        return false;
    }
    return true;
}

void Position::set_fen(const std::string fen){
    int index = 0, line = 7, intc;
    Square sq = A8;
    empty();
    while(fen[index] == ' ' || fen[index] == '\t') index ++;
    while(fen[index] != ' '){
        intc = (int)fen[index];
        if(intc >= 48 && intc <=59){ sq += intc - 48;}
        else if(fen[index] != '/'){ add(char_to_piece(fen[index]),sq++); }
        else {
            if(sq % 8 != 0){
                cerr<<"bad fen:"<<fen<<endl;
                exit(1);
            }
            sq = Square(--line * 8);
        }
        index ++;
    }
    if(sq % 8 != 0){
        cerr<<"bad fen:"<<fen<<endl;
        exit(1);
    }
    while(fen[index] == ' ' || fen[index] == '\t') index ++;
    if(fen[index] == 'b'){
        current->key ^= StmKey;
        stm^=1;
    }
    else if(fen[index] != 'w') {
        cerr<<"bad fen:"<<fen<<endl;
        exit(1);
    }
    index ++;
    while(fen[index] == ' ' || fen[index] == '\t') index ++;
    while(fen[index] != ' '){
        switch (fen[index]){
            case 'K':{
                current->castle |= WK_CASTLE;
                current->key ^= CastleKey[WK_CASTLE];
                break;
            }
            case 'Q':{
                current->castle |= WQ_CASTLE;
                current->key ^= CastleKey[WQ_CASTLE];
                break;
            }
            case 'k':{
                current->castle |= BK_CASTLE;
                current->key ^= CastleKey[BK_CASTLE];
                break;
            }
            case 'q':{
                current->castle |= BQ_CASTLE;
                current->key ^= CastleKey[BQ_CASTLE];
                break;
            }
            case '-':{
                break;
            }
            default: {
                cerr<<"bad fen:"<<fen<<endl;
                exit(1);
            }
        }
        index++;
    }
    index ++;
    while(fen[index] == ' ' || fen[index] == '\t') index ++;
    if(fen[index] != '-'){
        Square sq = string_to_sq(fen.substr(index,2));
        current->ep = sq;
        current->key ^= EpKey[sq];
    }
    current->checkers = get_checkers();
    #ifndef NDEBUG
    if(!is_ok()){
        cerr<<"bad fen:"<<fen<<endl;
        exit(1);
    }
    #endif
}

void Position::strip(){
    int i;
    for(i=0; i<=current->fifty; i++){
        ph[i] = ph[ply-(current->fifty-i)];
    }
    ply = --i;
    current = &ph[ply];
}


bool Position::draw() const{
    for(int i=4; i<=current->fifty; i+=2) {
        if(current->key == ph[ply-i].key)
            return true;
    }
    if(current->fifty == 100) return true;
    return false;
}

bool Position::is_passed(const Square sq) const{
	const File f = file(sq);
	const Rank r = rank(sq);
	Bitboard bb = BB_FILE[f] & (bitboard[WP] | bitboard[BP]);
	if(stm == White){
		if(!bit_gt(bb,r)){
			if(f == FILE_A) bb = BB_FILE[f+1] & bitboard[BP];
			else if(f == FILE_H) bb = BB_FILE[f-1] & bitboard[BP];
			else bb = (BB_FILE[f-1] | BB_FILE[f+1]) & bitboard[BP];
			if(!bit_gt(bb,r)) return true;
		}
	}
	else{
		if(!bit_le(bb,r)){
			if(f == FILE_A) bb = BB_FILE[f+1] & bitboard[WP];
			else if(f == FILE_H) bb = BB_FILE[f-1] & bitboard[WP];
			else bb = (BB_FILE[f-1] | BB_FILE[f+1]) & bitboard[WP];
			if(!bit_le(bb,r)) return true;
		}
	}
	return false;
}

bool Position::endgame() const{
	int wp = num[White] - num[WP];
	int bp = num[Black] - num[BP];
	if((wp + bp) == 3) return true;
	if((wp == 3 && bp == 2) || (wp == 2 && bp == 3))
		return true;
	return false;
}

void Position::set_thread(Thread* thread){
    this->thread = thread;
    material_hash = thread->material_hash;
    pawn_hash = thread->pawn_hash;
}

void Position::update_current(){
    current = &ph[ply];
}

uint64_t Position::get_book_key() const{
    uint64_t b_key = 0, bb;
    Square sq;
    Color c;
    Piece p;

    for(c = White; c<=Black; c++){
        bb = bitboard[c];
        while(bb != 0){
            sq = first_bit_clear(bb);
            p = piece[sq];
            switch(p){
                case WP:{p = Piece(1); break;}
                case WN:{p = Piece(3); break;}
                case WB:{p = Piece(5); break;}
                case WR:{p = Piece(7); break;}
                case WQ:{p = Piece(9); break;}
                case WK:{p = Piece(11); break;}
                case BP:{p = Piece(0); break;}
                case BN:{p = Piece(2); break;}
                case BB:{p = Piece(4); break;}
                case BR:{p = Piece(6); break;}
                case BQ:{p = Piece(8); break;}
                case BK:{p = Piece(10); break;}
                default:{
                        std::cerr<<"Wrong piece (book)!"<<std::endl;
                        exit(1);
                }
           }
           b_key ^= book_key[p*64+sq];
       }
    }

    for(int i = 0; i < 4; i++) {
        if ((current->castle & (1<<i)) != 0) b_key ^= book_key[768+i];
    }

    sq = current->ep;
    if (sq != NO_SQ) b_key ^= book_key[772+file(sq)];

    if(stm == White) b_key ^= book_key[780];
    return b_key;
}

void Position::egbb_info(int king[2], int piece[3], int square[3]) const{
        enum {_EMPTY,_WKING,_WQUEEN,_WROOK,_WBISHOP,_WKNIGHT,_WPAWN,
                _BKING,_BQUEEN,_BROOK,_BBISHOP,_BKNIGHT,_BPAWN};

        int from, type, count;

        count = 0;
        piece[0] = piece[1] = piece[2] = _EMPTY;
        square[0] = square[1] = square[2] = _EMPTY;

        Bitboard bb = occupied;
        while (bb) {
                from = first_bit_clear(bb);
                type = this->piece[from];
                if(type != WK && (type != BK)){
                        square[count] = from;
                        switch(type){
                                case WP:{piece[count] = _WPAWN; break;}
                                case WN:{piece[count] = _WKNIGHT; break;}
                                case WB:{piece[count] = _WBISHOP; break;}
                                case WR:{piece[count] = _WROOK; break;}
                                case WQ:{piece[count] = _WQUEEN; break;}
                                case BP:{piece[count] = _BPAWN; break;}
                                case BN:{piece[count] = _BKNIGHT; break;}
                                case BB:{piece[count] = _BBISHOP; break;}
                                case BR:{piece[count] = _BROOK; break;}
                                case BQ:{piece[count] = _BQUEEN; break;}
                        }
                        count++;
                }
        }
        king[White] = first_bit(bitboard[WK]);
        king[Black] = first_bit(bitboard[BK]);
}

std::string Position::move_to_san(const Move move) const{
    std::string san;
    Square from = move_from(move);
    Square to = move_to(move);
    Move type = move_type(move);
    bool is_pawn = (piece[from] == WP || piece[from] == BP);

    if (type == MoveCastle) {
        if (from < to) san = "O-O";
        else san = "O-O-O";
    } else {
        if (!is_pawn){
            san = piece_to_san_char(piece[from]);

            Bitboard attack = 0;
            switch (piece[from]) {
                case WN: case BN: { attack = bitboard[WN+stm] & knight_attack(to); break; }
                case WB: case BB: { attack = bitboard[WB+stm] & bishop_attack(to); break; }
                case WR: case BR: { attack = bitboard[WR+stm] & rook_attack(to); break; }
                case WQ: case BQ: { attack = bitboard[WQ+stm] & queen_attack(to); break; }
                case WK: case BK: { attack = bitboard[WK+stm] & king_attack(to); break; }
                default: { std::cerr<<"Something really wrong (move to san)!"<<std::endl; break;}
            }

            attack ^= BB_1<<from;
            attack &= bitboard[stm];
            if (attack) {
                bool row = false, column = false;
                if (attack & BB_RANK[rank(from)]) column = true;
                if (attack & BB_FILE[file(from)]) row = true;
                else column = true;

                if (column) san += 'a' + file(from);
                if (row) san += '1' + rank(from);
            }
        }

        if (piece[to] != NO_PIECE || type == MoveEP) {
            if (is_pawn) san += 'a' + file(from);
            san += 'x';
        }

        san += 'a' + file(to);
        san += '1' + rank(to);
    }

    if (type >= MovePN && type <= MovePQ) {
        san += '=';
        san += piece_to_san_char(Piece(type));
    }

    Thread thread(1,1,1);
    thread.set_position(*this);
    thread.pos.move_do(move);
	int eval;
    if (thread.pos.is_in_check()){
        MoveGenerator mg(thread,MoveNull,0,0);
		if(mg.next(eval) == MoveNull) san += '#';
        else san += '+';
    }

    return san;
}
