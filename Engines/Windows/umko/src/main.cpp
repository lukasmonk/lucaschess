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

#include <cstring>

#include "engine.h"
#include "test.h"

int main(int argc, char *argv[]){
	if(argc == 6){
        if(!strcmp("-epdtime",argv[1])){
            Test::mode = EPD_TIME;
            Test::value = atoi(argv[2]);
			Test::run(argv[3],argv[4],argv[5]);
        }
        if(!strcmp("-epddepth",argv[1])){
            Test::mode = EPD_DEPTH;
            Test::value = atoi(argv[2]);
			Test::run(argv[3],argv[4],argv[5]);
        }
        if(!strcmp("-epdnodes",argv[1])){
            Test::mode = EPD_NODES;
            Test::value = atoi(argv[2]);
			Test::run(argv[3],argv[4],argv[5]);
        }
    }
    else if(argc == 3){
        if(!strcmp("-epdeval",argv[1])){
            Test::mode = EPD_EVAL;
			Test::run(argv[2],argv[3],argv[4]);
        }
    }
    else if(argc == 1){
        Engine::run();
    }
    else{
		std::cout<<"usage: "<<argv[0]<<" [-epdtime time file.epd unsolved.epd file.log]?"<<std::endl;
    }
    return 0;
}
