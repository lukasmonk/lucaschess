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

#include <cstdlib>
#include <cstring>

#include "engine.h"
#include "test.h"
#include "movegen.h"
#include "search.h"
#include "thread.h"
#include "timer.h"
#include "book.h"
#include "egbb.h"

const std::string Engine::name = NAME;
const std::string Engine::version = VERSION;
const std::string Engine::author = "Borko Bošković";
const std::string Engine::year = "2009";
const std::string Engine::about = "http://umko.sourceforge.net/";

Position Engine::pos;
std::list<Option> Engine::option;

int Engine::wtime;
int Engine::btime;
int Engine::winc;
int Engine::binc;
int Engine::movestogo;
bool Engine::ponder;
int Engine::depth;
int Engine::movetime;
bool Engine::infinite;
unsigned long long Engine::nodes;
int Engine::mate;

void Engine::info(){
    std::cout<<"info string "<<name<<" "<<version<<" Copyright (C) "<<year<<" "<<author<<"\n";
    std::cout<<"info string This program comes with ABSOLUTELY NO WARRANTY."<<"\n";
    std::cout<<"info string This is free software, and you are welcome to"<<"\n";
    std::cout<<"info string redistribute it under certain conditions (GPL 3)."<<std::endl;
}

void Engine::init(){
    MoveGenerator::init();
    Trans::create();
    pos.start();
    add_options();
    Book::load();
    EGBB::load();
}

void Engine::run(){
    info();
    init();

    std::string line;
    while(getline(std::cin,line)){
        std::stringstream ss(line);
        if(!command(ss)) break;
    }
    quit();
}

bool Engine::command(std::stringstream& line)
{
    std::string command;
    line >> command;

    if(command == "stop")           { Timer::stop(); stop_search(); }
    else if(command == "ponderhit") {}
    else if(command == "go")        { go(line);}
    else if(command == "position")  { position(line); }
    else if(command == "setoption") { set_option(line); }
    else if(command == "ucinewgame"){ new_game(); }
    else if(command == "isready")   { std::cout<<"readyok"<<std::endl; }
    else if(command == "quit")      { return false; }
    else if(command == "uci")       { id(); }
    else if(command == "print")     { pos.print(); }
    else std::cerr<<"Unknown uci command: "<<command<<std::endl;

    return true;
}

void Engine::id(){
    std::cout<<"id name "<<name<<" "<<version<<"\n";
	std::cout<<"id author "<<author<<std::endl;
    option_list();
	std::cout<<"uciok"<<std::endl;
}

int Engine::get_time(){
    if(movetime != 0) return movetime;

    if((wtime + winc + btime + binc) == 0) return 0;

    if(movestogo == 0) movestogo = 40;

    if(movestogo == 1){
		if(pos.get_stm() == White){
			wtime = wtime * 0.9;
			if(wtime <= 0) wtime = 10;
			return wtime;
		}
		else{
			btime = 0.9 * btime;
			if(btime <= 0) btime = 10;
			return btime;
		}
    }

    int cwtime, cbtime;
	float relation;
    if(pos.get_stm() == White){
        if(movestogo >= 10){
            cwtime = ((wtime + winc * (movestogo-1)) / movestogo) * 1.3;
            cbtime = ((btime + binc * (movestogo-1)) / movestogo) * 1.3;
        }
        else{
            cwtime = ((wtime + winc * (movestogo-1)) / movestogo) * 0.9;
            cbtime = ((btime + binc * (movestogo-1)) / movestogo) * 0.9;
        }
		relation = cwtime/(float)cbtime;
		cwtime = cwtime * relation;
        if(cwtime > wtime) cwtime = wtime * 0.9;
		if(cwtime <= 10) cwtime = 10;
        return cwtime;
    }
    else{
        if(movestogo >= 10){
            cwtime = ((wtime + winc * (movestogo-2)) / movestogo) * 1.3;
            cbtime = ((btime + binc * (movestogo-1)) / movestogo) * 1.3;
        }
        else{
            cwtime = ((wtime + winc * (movestogo-2)) / movestogo) * 0.9;
            cbtime = ((btime + binc * (movestogo-1)) / movestogo) * 0.9;
        }
		relation = cbtime/(float)cwtime;
		cbtime = cbtime * relation;
		if(cbtime > btime) cbtime = btime * 0.9;
		if(cbtime <= 10) cbtime = 10;
		return cbtime;
    }
}

void Engine::go(std::stringstream& stream){
    std::string command;
    init_search();
    stream>>command;
    while(command != ""){
        if(command == "wtime") stream>>wtime;
        else if(command == "btime") stream>>btime;
        else if(command == "winc") stream>>winc;
        else if(command == "binc") stream>>binc;
        else if(command == "movestogo") stream>>movestogo;
        else if(command == "ponder") { ponder = true; }
        else if(command == "depth"){ stream>>depth;}
        else if(command == "movetime") stream>>movetime;
        else if(command == "infinite") infinite = true;
        else if(command == "nodes") stream>>nodes;
        else if(command == "mate") stream>>mate;
        else { std::cerr<<"Wrong go command: "<<command<<std::endl; return; }
        command = "";
        stream>>command;
    }
	Search::time = get_time();
    start_search();
}

void Engine::position(std::stringstream& stream){
    std::string command = "";
    stream>>command;
    if(command == "startpos"){
        pos.start();
        command = "";
        stream>>command;
        if(command == "moves"){
            command = "";
            stream>>command;
            while(command != ""){
                pos.move_do(command);
                command = "";
                stream>>command;
            }
        }
    }
    else if(command == "fen"){
        std::string fen;
        for(int i=0; i<6; i++){
            stream>>command;
            if(i<5) fen += command + " ";
            else fen += command;
        }
        pos.set_fen(fen);
        command = "";
        stream>>command;
        if(command == "moves"){
            command = "";
            stream>>command;
            while(command != ""){
                pos.move_do(command);
                command = "";
                stream>>command;
            }
        }
    }
}

void Engine::init_search(){
    wtime = btime = 0;
    winc = binc = 0;
    movestogo = 0;
    infinite = ponder = false;
    mate = depth = movetime = 0;
    nodes = 0;
}

void Engine::start_search(){
    Search::start(pos,infinite,depth,nodes);
}

void Engine::stop_search(){
    Search::stop();
    RootMoves::print_best_move();
}

void Engine::quit(){
    Search::stop();
    Trans::destroy();
    Book::close();
    EGBB::close();
}

void Engine::add_spin_option(const char name[255], const int min, const int max,
                             int* value, void (*function)(void)){
    Option op;
    op.spin.type = SPIN;
    strcpy(op.spin.name,name);
    op.spin.function = function;
    op.spin.min = min;
    op.spin.max = max;
    op.spin.def = *value;
    op.spin.value = value;
    option.push_back(op);
}

void Engine::add_string_option(const char name[255], const char* value, void (*function)(void)){
    Option op;
    op.string.type = STRING;
    strcpy(op.string.name,name);
    op.string.function = function;
    strcpy(op.string.def,value);
    op.string.value = (char *)value;
    option.push_back(op);
}

void Engine::add_button_option(const char name[255],void (*function)(void)){
    Option op;
    op.button.type = BUTTON;
    op.button.function = function;
    strcpy(op.button.name,name);
    option.push_back(op);
}

void Engine::add_check_option(const char name[255], const bool def, bool* value,
                              void (*function)(void)){
    Option op;
    op.check.type = CHECK;
    op.check.function = function;
    strcpy(op.check.name,name);
    op.check.def = def;
    op.check.value = value;
    option.push_back(op);
}

void Engine::option_list(){
    std::list<Option>::iterator it;
    Option* op;
    for(it = option.begin(); it != option.end(); it++ ){
        op = &*it;
        switch(op->spin.type){
            case SPIN:{
                std::cout<<"option name "<<op->spin.name;
                std::cout<<" type spin default "<<op->spin.def;
                std::cout<<" min "<<op->spin.min;
                std::cout<<" max "<<op->spin.max<<std::endl;
                break;
            }
            case STRING:{
                std::cout<<"option name "<<op->string.name;
                std::cout<<" type string default "<<op->string.def;
                std::cout<<std::endl;
                break;
            }
            case BUTTON:{
                std::cout<<"option name "<<op->button.name;
                std::cout<<" type button "<<std::endl;
                break;
            }
            case CHECK:{
                std::cout<<"option name "<<op->check.name;
                if(op->check.def)
                    std::cout<<" type check default true"<<std::endl;
                else
                    std::cout<<" type check default false"<<std::endl;
                break;
            }
        }
    }
}

void Engine::set_option(std::stringstream& stream){
    std::string command, name = "";
    stream >> command;
    Option *op;
    if(command == "name"){
        command = "";
        stream>>command;
        name = command;
        while(true){
            command = "";
            stream>>command;
            if(command == "value" || command == "") break;
            name += " " + command;
        }
        std::list<Option>::iterator it;
        for(it = option.begin(); it != option.end(); it++ ) {
            op = &*it;
            if(name == op->spin.name){
                switch(op->spin.type){
                    case SPIN:{
                        stream>>*op->spin.value;
                        if(*op->spin.value > op->spin.max ||
                           *op->spin.value < op->spin.min){
                            std::cerr<<"Wrong uci option value(";
                            std::cerr<<name<<":"<<op->spin.value<<")";
                            std::cerr<<std::endl;
                            exit(1);
                        }
                        std::cout<<"info string "<<name;
                        std::cout<<" set to "<<*op->spin.value<<std::endl;
                        if(op->spin.function != NULL)
                            op->spin.function();
                        break;
                    }
                    case STRING:{
						stream>>op->string.value;
						stream.getline(op->string.value+strlen(op->string.value),200);
                        std::cout<<"info string "<<name;
                        std::cout<<" set to "<<op->string.value<<std::endl;
                        if(op->string.function != NULL)
                            op->string.function();
                        break;
                    }
                    case BUTTON:{
                        std::cout<<"info string "<<name;
                        std::cout<<" selected"<<std::endl;
                        if(op->button.function != NULL)
                            op->button.function();
                        break;
                    }
                    case CHECK:{
                        stream>>command;
                        if(command == "true")
                            *op->check.value = true;
                        else if(command == "false")
                            *op->check.value = false;
                        else{
                            std::cerr<<"Wron uci option value (";
                            std::cerr<<name<<":"<<command<<")";
                            std::cerr<<std::endl;
                            exit(1);
                        }
                        std::cout<<"info string "<<name;
                        std::cout<<" set to "<<command<<std::endl;
                        if(op->check.function != NULL)
                            op->check.function();
                        break;
                    }
                }
            }
        }
    }
}

void Engine::add_options(){
    add_spin_option("Hash",1,5120,&Trans::m_size,Trans::create);
    add_check_option("OwnBook",Book::use,&Book::use,NULL);
    add_string_option("Book File",Book::file_name,Book::load);
    add_string_option("Bitbases Path",EGBB::path,EGBB::load);
    add_spin_option("Bitbases Cache Size",1,256,&EGBB::cache,EGBB::load);
	add_string_option("UCI_EngineAbout", about.c_str(), NULL);
}
