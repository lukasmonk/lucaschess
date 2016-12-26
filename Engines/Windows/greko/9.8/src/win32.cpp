//  GREKO Chess Engine
//  (c) 2002-2012 Vladimir Medvedev <vrm@bk.ru>
//  http://greko.110mb.com

//  win32.cpp: Windows-specific code
//  modified: 31-Dec-2012

#include <windows.h>
#include <conio.h>
#include <stdio.h>
#include "utils.h"

extern PROTOCOL_T g_protocol;

static int is_pipe = 0;
static HANDLE input_handle = 0;

void SleepMilliseconds(int ms)
{
  Sleep(ms);
}

void Highlight(bool on)
{
  WORD intensity = on? FOREGROUND_INTENSITY : 0;
  SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 
    FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE | intensity);
}

void InitInput()
{
  DWORD dw;
  input_handle = GetStdHandle(STD_INPUT_HANDLE);
  is_pipe = !GetConsoleMode(input_handle, &dw);

  if (is_pipe)
    g_protocol = UCI;

  setbuf(stdout, NULL);
}

int InputAvailable()
{
  DWORD nchars;

  if (stdin->_cnt > 0)
    return 1;

  if (is_pipe)
  {
    if (!PeekNamedPipe(input_handle, NULL, 0, NULL, &nchars, NULL))
      return 1;
    return (nchars != 0);
  }
  else
    return _kbhit() != 0;
}
