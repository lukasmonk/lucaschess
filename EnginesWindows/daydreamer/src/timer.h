
#ifndef TIMER_H
#define TIMER_H
#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    struct timeval tv_start;
    int elapsed_millis;
    bool running;
} milli_timer_t;

#ifdef __cplusplus
} // extern "C"
#endif
#endif // TIMER_H
