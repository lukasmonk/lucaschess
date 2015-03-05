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

#include "namespaces.h"


#include <chrono>
namespace _time {

    int diffTime ( struct timeb t1, struct timeb t2 ) {
        return 1000 * ( t1.time - t2.time ) + t1.millitm - t2.millitm;
    }

    string getLocalTime() {
        time_t current = chrono::system_clock::to_time_t ( chrono::system_clock::now() );
        return ctime ( &current );
    }

    int getYear() {
        time_t t = time ( NULL );
        tm* timePtr = localtime ( &t );
        return 1900 + timePtr->tm_year;
    }

    int getMonth() {
        time_t t = time ( NULL );
        tm* timePtr = localtime ( &t );
        return 1 + timePtr->tm_mon;
    }

    int getDay() {
        time_t t = time ( NULL );
        tm* timePtr = localtime ( &t );
        return timePtr->tm_mday;
    }
}

namespace _file {
    bool fileExists ( string filename ) {
        ifstream inData;
        inData.open ( filename );

        if ( !inData ) {
            return false;
        }

        inData.close();
        return true;
    }

    int fileSize ( const string& FileName ) {
        struct stat file;

        if ( !stat ( FileName.c_str(), &file ) ) {
            return file.st_size;
        }

        return -1;
    }

    string extractFileName ( string path ) {
        replace ( path.begin(), path.end(), ':', '/' );
        replace ( path.begin(), path.end(), '\\', '/' );
        istringstream iss ( path );
        string token;

        while ( getline ( iss, token, '/' ) );

        return  token;
    }

}


