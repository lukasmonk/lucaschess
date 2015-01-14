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

#ifndef EGBB_H
#define EGBB_H

#include "position.h"

#if defined(__MINGW32__)
#include <windows.h>
#else
#define HMODULE void*
#endif

typedef int (*PROBE) (int player, int w_king, int b_king,
                      int piece1, int square1,
                      int piece2, int square2,
                      int piece3, int square3);

class EGBB{
public:
        static void load();
        static bool isLoaded();
        static void close();
        static bool probe(const Position& pos, int& score);
        static int hits;
        static char path[255];
        static int cache;
        static void test();
private:
		static HMODULE lib;
        static PROBE probef;
};

#endif // EGBB_H
