#include <iostream>
#include <sys/timeb.h>
#include <windows.h>
#include <io.h>

#include <signal.h>
#include <errno.h>
#include <conio.h>

#ifndef _WINDOWS_
#include <sys/types.h>
#include <sys/time.h>
#include <unistd.h>
#endif


#include "Lime.h"
#include "data.h"
#include "prototypes.h"

using namespace std;


/*
  From Beowulf, from Olithink, and I confess I have no idea how it works.
  It understands when input has become available
*/

#ifndef _WIN32

int Bioskey(void)
{
  fd_set          readfds;
  struct timeval  timeout;

  FD_ZERO(&readfds);
  FD_SET(fileno(stdin), &readfds);

  timeout.tv_sec = 0;
  timeout.tv_usec = 0;
  select(16, &readfds, 0, 0, &timeout);

  return (FD_ISSET(fileno(stdin), &readfds));
}

#else

#include <windows.h>
#define frame 0
#include <conio.h>
int bioskey(void)
{
    static int      init = 0,
                    pipe;
    static HANDLE   inh;
    DWORD           dw;

    if (!init) {
        init = 1;
        inh = GetStdHandle(STD_INPUT_HANDLE);
        pipe = !GetConsoleMode(inh, &dw);
        if (!pipe) {
            SetConsoleMode(inh, dw & ~(ENABLE_MOUSE_INPUT | ENABLE_WINDOW_INPUT));
            FlushConsoleInputBuffer(inh);
        }
    }
    if (pipe) {
        if (!PeekNamedPipe(inh, NULL, 0, NULL, &dw, NULL))
            return 1;
        return dw;
    } else {
        GetNumberOfConsoleInputEvents(inh, &dw);
        return dw <= 1 ? 0 : dw;
    }
}
#endif

/*
checkinput() is taken from Beowulf - if data is shown to be present, then
it reads this data and tries to identify whether it is a known command
or not.

Again, I'm too clear on the workings of this, but have commented some of it
*/
int checkinput()
{
  int             data, bytes;

  //the input string and a pointer to the string
  char            input[256] = "", *endc;

  data = bioskey();//see if we have had input

  if (data)
  {
     if(searchparam->xbmode)
     {
      stopsearch = TRUE;
      return 1;
     }
     do//we have input, so read it into input[]
     {
      bytes=read(fileno(stdin),input,256);
     } while (bytes<0);
     endc = strchr(input,'\n');
     if (endc) *endc=0;

   if (strlen(input)>0)//line is bigger than 0, so see if it was a command
   {
       //strncasecmp() returns 0 if true
    if (!strncasecmp(input, "quit", 4))
    {
      //quit command - delete the hash table, and exit
      delete TTable;
      TTable = NULL;
      cout<<" had an interrupt  quit";
      exit(0);
      return 1;
    }

    else if (!strncasecmp(input, "stop", 4))
    {
       //stop command - make stopsearch TRUE
      stopsearch = TRUE;
      cout<<" had an interrupt stop"<<"\n";
       //longjmp(stopped, 0);

      return 1;
    }
    else if (!strncasecmp(input, "ponderhit", 9))
    {
         //ponderhit command - we need to carry on searching,
         //but be aware that we are no on our own time, so set the
         //search variables appropriately
         searchparam->ponderhit = TRUE;

         //not pondering or infinite
         searchparam->pon = FALSE;
         searchparam->inf = FALSE;
         cout<<" had a phit"<<"\n";
         //allocate our search time
         double alloctime = allocatetime();
         if(alloctime < 0) alloctime = 200;
         searchparam->starttime = double(clock())+500;//safety
         searchparam->stoptime = searchparam->starttime + alloctime;
         return 1;
    }
    else
    {
        //unrecognised input, so stop anyway. Not sure if I should do this -
        //but sometimes it doesn't read the stop correctly
        cout<<" had a ufo "<<input<<"\n";
        //longjmp(stopped, 0);
        stopsearch = TRUE;
        //searchparam->force = TRUE;
        return 1;
    }
   }
  }
  return 0;
}

/*
checkup() checks whether we've had input or run out of time.
*/
void checkup()
{
    checkinput();//could set stopsearch to TRUE in here

    //see if we are out of time
    if ( double(clock()) >= searchparam->stoptime)
    {
      stopsearch = TRUE;
      cout<<"\ntimeup\n";
      //longjmp(stopped, 0);
    }
}
