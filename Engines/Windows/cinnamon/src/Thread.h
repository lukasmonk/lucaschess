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

#ifndef THREAD_H_
#define THREAD_H_
#include <thread>
#include <mutex>
#include <condition_variable>
using namespace std;

class Runnable {
public:
    virtual void run() = 0;
};

class Thread: virtual public Runnable {
private:
    bool running = true;

    condition_variable cv;
    thread* theThread;
    Runnable * _runnable;
    Runnable * execRunnable;

    static void * __run ( void * cthis ) {
        static_cast<Runnable*> ( cthis )->run();
        return nullptr;
    }

public:
    Thread() :
        _runnable ( nullptr ) {
        theThread = nullptr;
        execRunnable = this;
    }

    virtual ~Thread() {
        if ( theThread ) {
            theThread->detach();
            delete theThread;
            theThread = nullptr;
        }
    }

    void checkWait() {
        while ( !running ) {
            mutex mtx;
            unique_lock<mutex> lck ( mtx );
            cv.wait ( lck );
        }
    }

    void notify() {
        cv.notify_all();
    }

    void start() {
        if ( this->_runnable != nullptr ) {
            execRunnable = this->_runnable;
        }

        if ( theThread ) {
            delete theThread;
        }

        theThread = new thread ( __run, execRunnable );
    }

    void join() {
        if ( theThread ) {
            theThread->join();
            delete theThread;
            theThread = nullptr;
        }
    }

    bool isJoinable() {
        return theThread->joinable();
    }

    void sleep ( bool b ) {
        running = !b;
    }

    void stop() {
        if ( theThread ) {
            theThread->detach();
            delete theThread;
            theThread = nullptr;
        }
    }

};
#endif


