#ifndef _CRT_SECURE_NO_DEPRECATE
#define _CRT_SECURE_NO_DEPRECATE
#endif
#include <windows.h>
#include <iostream>
#include <conio.h>
#include "extglobals.h" 
#include "protos.h" 
#include "timer.h" 

void Board::readClockAndInput()
{
	// check if we need to stop, because time is up, or because the user has hit the keyboard.
	// UPDATEINTERVAL defines how often this check is done in terms of nodes searched.
	// For example, if the search speed is 1000 knods per second, then a value of UPDATEINTERVAL = 100000 
	// will result in 10 checks per second (or 0.1s time intervals)

	DWORD nchar;
	char command[80];

	// reset countdown
	countdown = UPDATEINTERVAL;

	if ((!XB_NO_TIME_LIMIT && ((timer.getms() - msStart) > maxTime)) || (!XB_MODE && _kbhit()))
	{
		timedout = true;
		#ifdef WINGLET_DEBUG_WINBOARD
			if (XB_MODE)
			{
				if (_kbhit()) 
				{
					std::cout << "#-winglet : timed out, kbhit" << std::endl;
				}
				else
				{
					std::cout << "#-winglet : timed out, time" << std::endl;
				}
			}
		#endif
		return;
	}

noPonder:

	if ((XB_MODE) && (PeekNamedPipe(GetStdHandle(STD_INPUT_HANDLE), NULL, 0, NULL, &nchar, NULL)))
	{

		for (CMD_BUFF_COUNT = 0; CMD_BUFF_COUNT < (int)nchar; CMD_BUFF_COUNT++)
	    {
			CMD_BUFF[CMD_BUFF_COUNT] = getc(stdin);
			// sometimes we do not receive a newline character 
			if (((CMD_BUFF_COUNT+1)==(int)nchar) || CMD_BUFF[CMD_BUFF_COUNT] == '\n')
//			if (CMD_BUFF[CMD_BUFF_COUNT] == '\n')

			{
				if (CMD_BUFF[CMD_BUFF_COUNT] == '\n') CMD_BUFF[CMD_BUFF_COUNT] = '\0';
				else CMD_BUFF[CMD_BUFF_COUNT+1] = '\0';
//				CMD_BUFF[CMD_BUFF_COUNT] = '\0';

				if (CMD_BUFF=="" || !CMD_BUFF_COUNT) return;

				sscanf(CMD_BUFF, "%s", command);
				#ifdef WINGLET_DEBUG_WINBOARD
					std::cout << "#-winglet : in peek, CMD_BUFF=" << CMD_BUFF << " command=" << command << std::endl;
				#endif

				// do not stop thinking/pondering/analyzing for any of the following commands:
				if (!strcmp(command, ".")) return;
				if (!strcmp(command, "?")) return;
				if (!strcmp(command, "bk")) return;
				if (!strcmp(command, "easy")) 
				{
					XB_PONDER = false;
					return;
				}
				if (!strcmp(command, "hint")) return; 
				if (!strcmp(command, "nopost"))
				{
					XB_POST = false;
					return;
				}	
				if (!strcmp(command, "otim")) 
				{
					sscanf(CMD_BUFF, "otim %d", &XB_OTIM);
					XB_OTIM *= 10;  // convert centiseconds to miliseconds;
					goto noPonder;
				}	
				if (!strcmp(command, "post")) 
				{
					XB_POST = true;
					return;
				}	
				if (!strcmp(command, "time")) 
				{
					sscanf(CMD_BUFF,"time %d",&XB_CTIM);
					XB_CTIM *= 10; // convert centiseconds to milliseconds
					goto noPonder;
				}
				timedout = true;
				CMD_BUFF_COUNT = (int)strlen(CMD_BUFF);
				XB_DO_PENDING = true;
				return;
			}
		}
	}
	return;
}








