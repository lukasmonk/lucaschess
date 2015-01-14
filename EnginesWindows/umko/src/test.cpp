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
#include <iostream>
#include <sstream>
#include <zlib.h>
#include <iomanip>

#include "test.h"
#include "movegen.h"
#include "search.h"
#include "trans.h"
#include "engine.h"
#include "egbb.h"

#ifdef __MINGW32__
#define sleep(x) Sleep(1000 * x)
#endif

int Test::num;
EPD Test::mode;
int Test::value;

void Test::run(char epd_file[255], char unsolved_file[255], char log_file[255]){
    std::stringstream epd (std::stringstream::in | std::stringstream::out);
    std::string fen, line, tmp;
    char buffer[1024];

    std::streambuf* cout_sbuf = std::cout.rdbuf(); 

	std::ofstream log;
	log.open(log_file);
	if(!log.good()){
		std::cerr<<"Error: can not open file "<<log_file<<"!"<<std::endl;
	}

	std::ofstream unsolved;
	unsolved.open(unsolved_file);
	if(!unsolved.good()){
		std::cerr<<"Error: can not open file "<<unsolved_file<<"!"<<std::endl;
	}

	gzFile epd_gz = gzopen(epd_file, "rb");
    if (!epd_gz){
		std::cerr<<"Error: can not open file "<<epd_file<<"!"<<std::endl;
        return;
    }
    int num_read = 0;
    while ((num_read = gzread(epd_gz, buffer, sizeof(buffer)-1)) > 0){
        buffer[num_read] = '\0';
        epd<<buffer;
    }
    gzclose(epd_gz);

	std::cout<<std::fixed<<std::setprecision(2);
    std::cout.rdbuf(log.rdbuf());
    Engine::init();
    Engine::info();
	std::cout.rdbuf(cout_sbuf);
    num = 0;
	float depth = 0, nps=0, hash=0, seldepth=0, nodes=0, num_solved = 0;
	float rtime=0, time, tbhits = 0;
    while(getline(epd,line)){
        num ++;
        std::istringstream stream(line.c_str());
        for(int i=0; i<4; i++){
            stream>>tmp;
            if(i==0) fen = tmp;
            else fen += " " + tmp;
        }

        Engine::set_fen(fen.c_str());

        if(mode == EPD_EVAL){
			std::cout<<num<<". "<<fen<<" ev "<<Engine::eval()<<std::endl;
        }
        else if(mode == EPD_TIME){
            bool solved = false;
            size_t index = std::string::npos;
            std::string san, smove;
            std::stringstream go_stream("infinite");
            std::cout.rdbuf(log.rdbuf());
			std::cout<<line<<std::endl;
            Engine::go(go_stream);
			sleep(value);
            Engine::stop_search();
            std::cout.rdbuf(cout_sbuf);
            RootMove* rm = RootMoves::get(0);
            smove = move_to_string(rm->move);
            san = Engine::san_move(rm->move);
			nodes = ((num-1)* nodes + rm->nodes)/num;
			nps = ((num-1) * nps + rm->nps)/num;
			hash = ((num -1) * hash + Trans::usage())/num;
			depth = ((num-1) * depth + rm->depth)/num;
			seldepth = ((num - 1) * seldepth + Search::max_ply) / num;
			tbhits = ((num -1) * tbhits + EGBB::hits) / num;
            Engine::new_game();

            do{ stream>>tmp; }while(tmp.find("bm") == std::string::npos);

			while(index == std::string::npos){
                stream>>tmp;
                index = tmp.find(';');
                if(index != std::string::npos) tmp.erase(index);
                if(san.compare(tmp) == 0){
                    solved = true;
					num_solved ++;
                    break;
                }
            }
            if(!solved){
				unsolved<<line<<std::endl;
				time = value;
			}
			else time = RootMoves::rtime;
			rtime += time;
			std::cout.setf(std::ios::left);
			std::cout<<num<<". r:"<<int(num_solved)<<"/"<<num<<"="<<(num_solved/num)*100<<"% ";
			std::cout<<"d:"<<depth<<" sd:"<<seldepth<<" n:"<<nodes/(float)num<<" nps:"<<nps;
			std::cout<<" h:"<<hash/10<<"% tb:"<<tbhits<<" t:"<<time<<" rt:"<<rtime<<std::endl;
        }
    }
    log.close();
    unsolved.close();
    Engine::quit();
    std::cout.rdbuf(cout_sbuf);
}
