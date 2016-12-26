#include <iostream>

#include "Lime.h"
#include "data.h"
#include "prototypes.h"



using namespace std;


double allocatetime()
{

       /*
       first, see if we have allocated the wtime or btime variables -
       if so, they will be > 0. If not, the search is either infinite
       or depth based. So, we return a massive stop time. Another case
       is the time permove - in this case mvetime is returned if > -1 with
       a safety margin of 0.2s
       */

       if (searchparam->depth != -1)
       {
             return 128000000;
       }
       /*
       possibly, we have been given time per move. If so, return this
       with a small safety margin, to make sure we don't exceed it
       */
       if (searchparam->timepermove > -1)
       {
              return searchparam->timepermove - 200;
       }

       if (searchparam->wtime < 0 || searchparam->btime < 0 || searchparam->inf == TRUE)
       {
         return 128000000;
       }




       /*
       now there are two distinct types of time control -
       one is moves per session, the other is a finite amount
       of time plus an increment.
       */

       if (searchparam->movestogo[p->side] > 0)//given moves to next time control
       {
           /*
           choose according to side to move. An important note
           is the '-1000' - allowing 1s safety margin
           */
           if (p->side == black)
           {
               return ((searchparam->btime + (searchparam->binc*searchparam->movestogo[p->side]))/searchparam->movestogo[p->side]+1)-1000;
           }
           else
           {
               return ((searchparam->wtime + (searchparam->winc*searchparam->movestogo[p->side]))/searchparam->movestogo[p->side]+1)-1000;
           }
       }
       else
       {
           /*
           no longer a finite number of moves per time
           control, so we assume the game has 30 moves left.
           */
           if (p->side == black)
           {
               return (searchparam->btime/30 + (searchparam->binc));
           }
           else
           {
               return (searchparam->wtime/30 + (searchparam->winc));
           }
       }

       return -1;//shouldn't get here!
}


double pondertime()
{


       if (searchparam->depth != -1)
       {
             return 128000000;
       }
       if (searchparam->timepermove > -1)
       {
              return 128000000;
       }


       if (searchparam->movestogo[p->side] > 0)//given moves to next time control
       {
           /*
           choose according to side to move. An important note
           is the '-1000' - allowing 1s safety margin
           */
           if (p->side == black)
           {
               return ((searchparam->btime + (searchparam->binc*searchparam->movestogo[p->side]))/searchparam->movestogo[p->side]+1)-1000;
           }
           else
           {
               return ((searchparam->wtime + (searchparam->winc*searchparam->movestogo[p->side]))/searchparam->movestogo[p->side]+1)-1000;
           }
       }
       else
       {
           /*
           no longer a finite number of moves per time
           control, so we assume the game has 30 moves left.
           */
           if (p->side == black)
           {
               return (searchparam->btime/30 + (searchparam->binc));
           }
           else
           {
               return (searchparam->wtime/30 + (searchparam->winc));
           }
       }

       return -1;//shouldn't get here!
}

