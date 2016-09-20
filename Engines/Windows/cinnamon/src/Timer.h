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

#ifndef TIMER_H_
#define TIMER_H_
#include "Thread.h"

class Timer : public Thread {
public:

    Timer ( int seconds1 ) {
        seconds = seconds1;
    }

    void run() {
        while ( 1 ) {
            this_thread::sleep_for ( chrono::seconds ( seconds ) );
            notifyObservers();
        }
    }

    void registerObservers ( function<void ( void ) > f ) {
        observers.push_back ( f );
    }

    void notifyObservers ( void ) {
        for ( auto i = observers.begin(); i != observers.end(); ++i ) {
            ( *i ) ();
        }
    }

    virtual ~Timer() {}


private:
    int seconds;
    vector<function<void ( void ) >> observers;
};

#endif


