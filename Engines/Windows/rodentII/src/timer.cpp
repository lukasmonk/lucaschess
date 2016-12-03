#include <stdio.h>
#include "timer.h"
#include <math.h>
#include "rodent.h"
#include "param.h"


#if defined(_WIN32) || defined(_WIN64)
#  include <windows.h>
#else
#  include <unistd.h>
#  include <sys/time.h>
#endif

void sTimer::SetSpeed(int elo) { // TODO: should belong to cParam
   nps_limit = 0;
   Param.eval_blur = 0;

   if (Param.fl_weakening) {
      nps_limit = EloToSpeed(elo);
	  Param.eval_blur = EloToBlur(elo);
   }
}

void sTimer::Clear(void) {

  iteration_time = MAX_INT;
  adjustement = 0;
  smart_management = 0;
  SetData(MAX_DEPTH, 64);
  allocated_time = -1;
  SetData(W_TIME,-1);
  SetData(B_TIME,-1);
  SetData(W_INC, 0);
  SetData(B_INC, 0);
  SetData(MOVE_TIME, 0);
  SetData(MAX_NODES, 0);
  SetData(MOVES_TO_GO, 40);
  SetData(FLAG_INFINITE, 0);
}

void sTimer::OnOldRootMove(void) {
  if (smart_management) {
    adjustement -= 1;
    if (adjustement < -30) adjustement = -30;
  }
}

void sTimer::OnNewRootMove(void) {
  if (smart_management) {
	adjustement += 3;
	if (adjustement > 30) adjustement = 30;
  }
}

void sTimer::OnFailLow(void) {
  if (smart_management) {
    if (adjustement < 0) adjustement = 0;
  }
}

void sTimer::SetStartTime(void) {
  start_time = GetMS();
}

int sTimer::BulletCorrection(int time) {

  if (time < 200)       return (time * 23) / 32;
  else if (time <  400) return (time * 26) / 32;
  else if (time < 1200) return (time * 29) / 32;
  else return time;
}

void sTimer::SetMoveTiming(void) {

  // User-defined time per move, no tricks available

  if ( data[MOVE_TIME] ) {
    allocated_time = data[MOVE_TIME];
    return;
  }
  
  // We are operating within some time limit. There is some scope for using
  // remaining  time  in a clever way, but current  implementation  focuses
  // on staying out of trouble: counteracting the GUI lag and being careful
  // under the incremental time control near the end of the game.

  if (data[TIME] >= 0) {
    if (data[MOVES_TO_GO] == 1) data[TIME] -= Min(1000, data[TIME] / 10);
    allocated_time = ( data[TIME] + data[INC] * ( data[MOVES_TO_GO] - 1)) / data[MOVES_TO_GO];

	// Is it safe to use more advanced time management heuristics?
	// (i.e. to modify base thinking time based on how often root
	// move changes )

    if (2 * allocated_time < data[TIME]) smart_management = 1;

	// make a percentage correction to playing speed (unless too risky)

	if (smart_management) {
      allocated_time *= time_percentage;
      allocated_time /= 100;
	}

    // assign less time per move on extremely short time controls

    allocated_time = BulletCorrection(allocated_time);

    // while in time trouble, try to save a bit on increment

    if (allocated_time < data[INC] ) allocated_time -= ( (data[INC] * 4) / 5);

    // ensure that our limit does not exceed total time available

    if (allocated_time > data[TIME]) allocated_time = data[TIME];

    // safeguard against a lag

    allocated_time -= 10;

    // ensure that we have non-zero time

    if (allocated_time < 1) allocated_time = 1;
  }
}

void sTimer::SetIterationTiming(void) {

  if (allocated_time > 0) iteration_time = ( (allocated_time * 3) / 4 );
  else                    iteration_time = MAX_INT;

  // assign less time per iteration on extremely short time controls

  iteration_time = BulletCorrection(iteration_time);
}

int sTimer::FinishIteration(void) {
  return (GetElapsedTime() >= iteration_time && !pondering && !data[FLAG_INFINITE]);
}

int sTimer::GetMS(void) {

#if defined(_WIN32) || defined(_WIN64)
  return GetTickCount(); // bugbug:drc GetTickCount() wraps once every 50 days, causeing time control to go insane.  Don't use this.
#else
  struct timeval tv;

  gettimeofday(&tv, NULL);
  return tv.tv_sec * 1000 + tv.tv_usec / 1000;
#endif
}

int sTimer::GetElapsedTime(void) {
  return (GetMS() - start_time);
}

int sTimer::IsInfiniteMode(void) {
  return( data[FLAG_INFINITE] );
}

int sTimer::TimeHasElapsed(void) {
  return (GetElapsedTime() >= (allocated_time * (100 + adjustement) / 100) );
}

int sTimer::GetData(int slot) {
  return data[slot];
}

void sTimer::SetData(int slot, int val) {
  data[slot] = val;
}

void sTimer::SetSideData(int side) {

  data[TIME] = side == WC ? GetData(W_TIME) : GetData(B_TIME);
  data[INC]  = side == WC ? GetData(W_INC)  : GetData(B_INC);
}

void sTimer::WasteTime(int miliseconds) {

#if defined(_WIN32) || defined(_WIN64)
  Sleep(miliseconds);
#else
  usleep(miliseconds * 1000);
#endif
}

void sTimer::Init(void) {

  nps_limit = 0;
  special_mode = 0;
}