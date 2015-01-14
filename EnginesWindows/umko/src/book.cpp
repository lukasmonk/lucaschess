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

#include "book.h"
#include "engine.h"

#if defined(__MINGW32__)
char Book::file_name[255] = ".\\performance.bin";
#else
char Book::file_name[255] = "./performance.bin";
#endif

bool Book::use = true;
std::ifstream Book::file;
long Book::length;

bool Book::is_loaded(){
    return Book::use && file.is_open();
}

void Book::load(){
    if(file.is_open()){
        file.close();
            std::cout<<"info string book is closed"<<std::endl;
    }
    file.open(file_name,std::ifstream::binary);

    if(file.good()){
        std::cout<<"info string Book "<<file_name<<" is loaded."<<std::endl;
        file.seekg(0, std::ios::end);
        length = file.tellg()/sizeof(BookEntry);
    }
    else std::cout<<"info string Book "<<file_name<<" not loaded."<<std::endl;
}

uint64_t Book::read(const int size){
   uint64_t n=0;
   int b;

   for(int i = 0; i < size; i++){
      b = file.get();
      n = (n << 8) | b;
   }
   return n;
}

void Book::read(BookEntry& entry, const int n) {
    file.seekg(n*sizeof(BookEntry),std::ios::beg);
    entry.key   = read(8);
    entry.move  = read(2);
    entry.count = read(2);
    entry.n     = read(2);
    entry.sum   = read(2);
}

int Book::find(const uint64_t key, BookEntry& entry){
    int left, right, mid;

    left = 0;
    right = length-1;
    entry.key = 0;
    while (left < right) {
        mid = (left + right) / 2;
        read(entry,mid);
        if(key <= entry.key) right = mid;
        else left = mid+1;
    }
    read(entry,left);
    if(entry.key == key) return left;
    return length;
}

bool Book::find_move(const Position& pos){
    int bMove, selMove, best, bScore, rec;
    float rScore;
    BookEntry entry;
    RootMove* r_move;
    uint64_t key = pos.get_book_key();
    selMove = best = rec = 0;
    for(rec = find(key,entry); rec < length; rec++){
        read(entry,rec);
        if(entry.key !=  key) break;
        bMove = entry.move;
        bScore = entry.count;

        best += bScore;
		rScore = (rand()/(RAND_MAX+1.0) * best);
        if(rScore < bScore){
            selMove  = bMove;
        }
    }

    RootMove tmp = *RootMoves::get(0);
    if(selMove != 0){
        for(int i=0; i<RootMoves::get_size(); i++){
            r_move = RootMoves::get(i);
            if(r_move->move == selMove){
                r_move->eval = 1;
                r_move->depth = 1;
                r_move->flag = Exact;
                *RootMoves::get(0) = *r_move;
                *r_move = tmp;
                return true;
            }
            else{
                r_move->eval = 0;
                r_move->depth = 1;
                r_move->flag = Beta;
            }
        }
    }
    return false;
}

void Book::close(){
    file.close();
}
