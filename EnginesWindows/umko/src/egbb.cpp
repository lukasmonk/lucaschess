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

#include <string>
#include <cstring>
#include <iostream>

#include "egbb.h"

#if defined(__MINGW32__)

#ifdef __x86_64__
#define DLL "\\egbbdll64.dll"
#else
#define DLL "\\egbbdll.dll"
#endif

char EGBB::path[255] = ".\\egbb\\";
#define dlopen(x,RTLD_LAZY) LoadLibrary(x)
#define dlsym GetProcAddress
#define dlclose(x) FreeLibrary(x)

#else

#include <dlfcn.h>
char EGBB::path[255] = "./egbb/";

#ifdef __x86_64__
#define DLL "egbbso64.so"
#else
#define DLL "egbbso.so"
#endif

#endif

#define NOTFOUND 99999
#define WIN_SCORE 4500

enum {LOAD_NONE,LOAD_4MEN,SMART_LOAD,LOAD_5MEN};

typedef void (*LOAD) (char* path,int cache_size,int load_options);

HMODULE EGBB::lib = NULL;
PROBE EGBB::probef = NULL;
int EGBB::hits;
int EGBB::cache = 32;

void EGBB::load(){
    std::string dll = std::string(path) + DLL;
    LOAD loadf;

    int size = cache *1048576;

    if(lib){
        std::cout<<"info string Endgame bitbases closed"<<std::endl;
        dlclose(lib);
    }

    if((lib = dlopen(dll.c_str(),RTLD_LAZY))){
        loadf = (LOAD) dlsym(lib,"load_egbb_5men");
        probef = (PROBE) dlsym(lib,"probe_egbb_5men");
        loadf(path,size,SMART_LOAD);
        std::cout<<"info string Endgame bitbases is loaded"<<std::endl;
    }
    else{
        std::cout<<"info string Endgame bitbases ("<<dll<<") not loaded:"<<std::endl;
        lib = NULL;
        probef = NULL;
    }
}

bool EGBB::isLoaded(){
    if(probef) return true;
    return false;
}

bool EGBB::probe(const Position& pos, int& eval) {
    int king[2], piece[3], square[3];

	if(pos.get_ep() != NO_SQ) return NOTFOUND;

    pos.egbb_info(king,piece,square);
    eval = probef(pos.get_stm(),king[White], king[Black],
                  piece[0],square[0],
                  piece[1],square[1],
                  piece[2],square[2]);

    if(eval != NOTFOUND){
        hits ++;
        return true;
    }

    return false;
}

void EGBB::close(){
    if(lib) dlclose(lib);
}

void EGBB::test(){
    load();
    int eval = probef(0, 43, 48, 6,28, 12,31, 0,0);
    std::cout<<"Eval:"<<eval<<std::endl;
    close();
    exit(1);
}
