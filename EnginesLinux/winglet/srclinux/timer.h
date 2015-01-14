#ifndef WINGLET_TIMER_H
#define WINGLET_TIMER_H
 
#include <sys/timeb.h>
#include "defines.h"
 
struct Timer
{
	U64   startTime;   
	U64   stopTime;    
	U64   currentTime;
	U64   stopTimeDelta;
	timeb startBuffer;   
	timeb stopBuffer;   
	timeb currentBuffer;
	BOOLTYPE running;  
 
	void init();               // start the timer
	void stop();               // stop the timer
	void reset();              // reset the timer
	void display();                   // display time in seconds with 2 decimals
	void displayhms();         // display time in hh:mm:ss.dd
	U64 getms();               // return time in milliseconds
	U64 getsysms();         // return system time
	Timer();
};
 
 
#endif