
#include "daydreamer.h"

/*
 * Initialize a timer.
 */
void init_timer(milli_timer_t* timer)
{
    timer->elapsed_millis = 0;
    timer->running = false;
}

/*
 * Start up a timer with a new clock.
 */
void start_timer(milli_timer_t* timer)
{
    timer->running = true;
    gettimeofday(&timer->tv_start, NULL);
}

/*
 * Stop a running timer, returning the elapsed milliseconds since it was
 * last started. The timer can be resumed using resume_timer.
 */
int stop_timer(milli_timer_t* timer)
{
    struct timeval tv_end;
    gettimeofday(&tv_end, NULL);
    int elapsed_millis = (tv_end.tv_sec*1000 + tv_end.tv_usec/1000) - 
        (timer->tv_start.tv_sec*1000 + timer->tv_start.tv_usec/1000);
    timer->elapsed_millis += elapsed_millis;
    timer->running = false;
    return elapsed_millis;
}

/*
 * Get the number of milliseconds elapsed over all the intervals during which
 * this timer has been running since it was last reset.
 */
int elapsed_time(milli_timer_t* timer)
{
    if (timer->running) {
        struct timeval tv_end;
        gettimeofday(&tv_end, NULL);
        int elapsed_millis = (tv_end.tv_sec*1000 + tv_end.tv_usec/1000) - 
            (timer->tv_start.tv_sec*1000 + timer->tv_start.tv_usec/1000);
        timer->elapsed_millis += elapsed_millis;
        timer->tv_start.tv_sec = tv_end.tv_sec;
        timer->tv_start.tv_usec = tv_end.tv_usec;
    }
    return timer->elapsed_millis;
}

