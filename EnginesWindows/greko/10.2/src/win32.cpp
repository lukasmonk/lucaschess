//  GREKO Chess Engine
//  (c) 2002-2013 Vladimir Medvedev <vrm@bk.ru>
//  http://greko.110mb.com

//  win32.cpp: Windows-specific code
//  modified: 01-Mar-2013

#include <windows.h>
#include <conio.h>
#include <stdio.h>
#include "utils.h"

extern PROTOCOL_T g_protocol;

static int g_pipe = 0;
static HANDLE g_handle = 0;

void SleepMilliseconds(int ms) { Sleep(ms); }

void Highlight(bool on)
{
	WORD intensity = on? FOREGROUND_INTENSITY : 0;
	SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE | intensity);
}

void InitInput()
{
	DWORD dw;
	g_handle = GetStdHandle(STD_INPUT_HANDLE);
	g_pipe = !GetConsoleMode(g_handle, &dw);
	if (g_pipe) g_protocol = UCI;
	setbuf(stdout, NULL);
}

int InputAvailable()
{
	DWORD nchars;
	if (stdin->_cnt > 0) return 1;
	if (g_pipe)
	{
		if (!PeekNamedPipe(g_handle, NULL, 0, NULL, &nchars, NULL)) return 1;
		return (nchars != 0);
	}
	else
		return _kbhit() != 0;
}
