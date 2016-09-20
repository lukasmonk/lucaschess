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

#include <fstream>
#include <inttypes.h>

#include "position.h"
#include "types.h"
#include "rootmoves.h"

#ifndef BOOK_H
#define BOOK_H

class BookEntry {
    public:
        uint64_t key;
        uint16_t move;
        uint16_t count;
        uint16_t n;
        uint16_t sum;
};

class Book{
    public:
        static bool is_loaded();
        static void load();
        static void close();
        static bool find_move(const Position& position);
        static char file_name[255];
        static bool use;
    private:
        static int find(uint64_t key, BookEntry& entry);
        static uint64_t read(int size);
        static void read(BookEntry& entry, int n);
        static std::ifstream file;
        static long length;
};

#endif
