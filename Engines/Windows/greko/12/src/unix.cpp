//  GREKO Chess Engine
//  (c) 2002-2015 Vladimir Medvedev <vrm@bk.ru>
//  http://greko.su

//  unix.cpp: Unix-specific code
//  modified: 01-Oct-2014

#ifndef _MSC_VER

#include <stdio.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/types.h>
#include <signal.h>
#include "utils.h"

PROTOCOL_T g_protocol = UCI;
static int g_isPipe = 0;

U32 GetTime()
{
  // relative time in milliseconds
  return 1000 * clock() / CLOCKS_PER_SEC;
}

void SleepMilliseconds(int ms)
{
	usleep(1000 * ms);
}

void Highlight(bool on)
{
	//
	//   Turn highlight of the console text on or off
	//

	if (g_isPipe) return;
	if (on)
		printf("\033[1m"); // ESC-[-1-m  --- ANSI ESC sequence for bold attribute
	else
		printf("\033[m");  // ESC-[-m  --- ANSI ESC sequence resetting all attributes
}

void InitInput()
{
	out("init input...\n");
	g_isPipe = !isatty(0);
	if (g_isPipe) signal(SIGINT, SIG_IGN);
	setbuf(stdout, NULL);
	setbuf(stdin, NULL);
}

int InputAvailable()
{
	static fd_set input_fd_set;
	static fd_set except_fd_set;

	FD_ZERO (&input_fd_set);
	FD_ZERO (&except_fd_set);
	FD_SET  (0, &input_fd_set);
	FD_SET  (1, &except_fd_set);

	static struct timeval timeout;
	timeout.tv_sec = timeout.tv_usec = 0;
	static int max_fd = 2;
	// XXX -- track exceptions (in the select(2) sense) here?

	int retval = select(max_fd, &input_fd_set, NULL, &except_fd_set, &timeout);

	if (retval < 0)  // Error occured.
		return 0;

	if (retval == 0)  // timeout.
		return 0;

	if (FD_ISSET (0, &input_fd_set)) // There is input
		return 1;

	if (FD_ISSET (1, &except_fd_set)) // Exception on write,
		exit (1);                       // probably, connection closed.

	return 0;
}
#endif

