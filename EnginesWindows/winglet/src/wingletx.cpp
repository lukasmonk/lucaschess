#ifndef _CRT_SECURE_NO_DEPRECATE
#define _CRT_SECURE_NO_DEPRECATE
#endif
// windows only:
#include <Windows.h>
#include <signal.h>
#include <iostream>
#include "defines.h"
#include "protos.h"
#include "globals.h"

int main(int argc, char *argv[])
{
	int i;

	signal(SIGINT, SIG_IGN); 
	setbuf(stdin, NULL);

	// windows only:
	SYSTEM_INFO sysinfo;

	std::cout << WINGLET_PROG_VERSION << std::endl;
	#ifdef WINGLET_DEBUG_PERFT
		std::cout << "WINGLET_DEBUG_PERFT defined" << std::endl;
	#endif
	#ifdef WINGLET_DEBUG_MOVES
		std::cout << "WINGLET_DEBUG_MOVES defined" << std::endl;
	#endif
	#ifdef WINGLET_VERBOSE_EVAL
		std::cout << "WINGLET_VERBOSE_EVAL defined" << std::endl;
	#endif
	#ifdef WINGLET_DEBUG_EVAL
		std::cout << "WINGLET_DEBUG_EVAL defined" << std::endl;
	#endif
	#ifdef WINGLET_VERBOSE_SEE
		std::cout << "WINGLET_VERBOSE_SEE defined" << std::endl;
	#endif
	#ifdef WINGLET_DEBUG_WINBOARD
		std::cout << "WINGLET_DEBUG_WINBOARD defined" << std::endl;
	#endif

	dataInit();
	board.init();

	// windows only:
	GetSystemInfo(&sysinfo);
    if (sysinfo.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_INTEL)
		std::cout << "Version: X86, ";
	else if (sysinfo.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_IA64)
		std::cout << "Version: IA64, ";
	else if (sysinfo.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_AMD64)
		std::cout << "Version: X64, ";
	std::cout << sysinfo.dwNumberOfProcessors << " CPU's (1 CPU used)" << std::endl;
	std::cout << "Search structure = " << sizeof(board)/1024 << "kB" << std::endl;

	// read the initialization file:
	strcpy(PATHNAME, argv[0]);
	strcpy(INIFILE, "wingletx.ini");  // default name
	// check command-line to see if we need to use another ini-file:
	// usage: "wingletx.exe i=somefile.ini"
	for (i = 1; i < argc; i++) 
	{
		if (!strncmp(argv[i], "i=", 2)) sscanf_s(argv[i]+2,"%s", INIFILE);
	}
	readIniFile();

	std::cout <<  "'help' displays a list of commands" << std::endl;
	commands();

	return 0;

}