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

#ifndef ITERATIVEDEEPING_H_
#define ITERATIVEDEEPING_H_
#include <cstring>
#include <string.h>
#include "String.h"
#include "Search.h"
#include "Thread.h"
#include "OpenBook.h"
#include <mutex>
#include <stdio.h>
#include <stdlib.h>

class IterativeDeeping: public Thread , public Search {
public:
    IterativeDeeping();
    virtual ~IterativeDeeping();
    mutex mutex1;
    virtual void run();
    bool getPonderEnabled();
    bool getGtbAvailable();
    bool getUseBook();
    void setUseBook ( bool );
    void enablePonder ( bool );
    void setMaxDepth ( int );
    void loadBook ( string );
    int printDtm ( ) ;
    bool setParameter ( String param, int value );
    Tablebase& getGtb();
private:

    int maxDepth;
    STATIC_CONST int VAL_WINDOW = 50;
    bool useBook;

    OpenBook* openBook;
    bool ponderEnabled;
};
#endif

