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

#ifndef UCI_H_
#define UCI_H_

#include "IterativeDeeping.h"
#include "Perft.h"
#include <string.h>
#include "String.h"
class Uci {
public:
    Uci();
    virtual ~Uci();

private:
    IterativeDeeping* iterativeDeeping;
    bool uciMode;
    void listner ( IterativeDeeping* it );
    void getToken ( istringstream& uip, String& token );

};
#endif

