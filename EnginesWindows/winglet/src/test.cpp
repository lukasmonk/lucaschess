#ifndef _CRT_SECURE_NO_DEPRECATE  // suppress MS security warnings
#define _CRT_SECURE_NO_DEPRECATE
#endif
#include <io.h>
#include <iostream>
#include <string>
#include "defines.h"
#include "protos.h"
#include "extglobals.h"
 
void test(char *filename)
{
	// runs a testsuite with current depth and time search parameters

	int i, file;
	U64 inodestotal;
	U64 timetotal;
	Timer totaltime;
	std::string logfile (filename);

	totaltime.init();
	inodestotal = 0;
	std::cout << "START OF TEST:" << std::endl;
	std::cout << "MAX SEARCH DEPTH = " << board.searchDepth << std::endl;
	std::cout << "MAX SEARCH TIME = " << board.maxTime/1000 << "s" << std::endl;

	// construct logfile name
	logfile.replace(logfile.find("."),logfile.length(),".log");   

	// redirect stdout to the log file
	TO_CONSOLE = 0;
	file = _dup(_fileno(stdout));    // create a file descriptor for stdout
	freopen(logfile.c_str(),"w",stdout);  

	std::cout << std::endl << std::endl << "+++++++++++++++++++++++++++++++++++++++" << std::endl;
	std::cout << "START OF TEST:" << std::endl;
	std::cout << "MAX SEARCH DEPTH = " << board.searchDepth << std::endl;
	std::cout << "MAX SEARCH TIME = " << board.maxTime/1000 << "s" << std::endl;
	std::cout << "+++++++++++++++++++++++++++++++++++++++" << std::endl;

	i = 1;
	while (readFen(filename, i))
	{

		// write progress to console:
	    _dup2(file ,_fileno(stdout));   
		_close(file);
		printf("winglet> position #%d\n", i);
		file = _dup(_fileno(stdout));
		freopen(logfile.c_str(),"a",stdout);  

		board.display();
		board.think();
		inodestotal += board.inodes;
		i++;
	}

	timetotal = totaltime.getms();
	std::cout << std::endl << std::endl << "+++++++++++++++++++++++++++++++++++++++" << std::endl;
	std::cout << "SUMMARY:" << std::endl;
	std::cout << "TOTAL TIME SEARCHED  = " << timetotal << " ms" << std::endl;
	std::cout << "TOTAL NODES SEARCHED = " << inodestotal << std::endl;
	std::cout << "AVG SEARCH SPEED     = " << inodestotal/timetotal << " knps" << std::endl;
	std::cout << "+++++++++++++++++++++++++++++++++++++++" << std::endl;

    _dup2(file ,_fileno(stdout));   // reassign file descriptor
	_close(file);
	TO_CONSOLE = 1;

	return; 
}