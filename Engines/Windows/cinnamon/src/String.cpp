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

#include "String.h"

String::String ( )  : string() {;}

String::String ( int d ) {
    stringstream ss;
    ss << d;
    assign ( ss.str() );
}

String::~String() {}

String String::trimRight ( ) {
    int pos = find_last_not_of ( " " );
    erase ( pos + 1 );
    return *this;
}

String String::replace ( char c1, char c2 ) {
    for ( unsigned i = 0; i < size(); i++ ) {
        if ( at ( i ) == c1 ) { at ( i ) = c2; }
    }

    return *this;
}

String String::replace ( string s1, string s2 ) {
    int a;

    while ( ( a = find ( s1 ) ) != ( int ) string::npos ) {
        string::replace ( a, s1.size(), s2 );
    };

    return *this;
}

String String::toUpper ( ) {
    transform ( begin(), end(), begin(), ::toupper );
    return *this;
}
String String::toLower ( ) {
    transform ( begin(), end(), begin(), ::tolower );
    return *this;
}
