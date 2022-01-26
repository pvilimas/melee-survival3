#ifndef _TIMER_H_
#define _TIMER_H_

#include "raylib.h"

typedef void (*TimerCallback)(void);

typedef struct {
  float interval;
  float last_recorded;
  TimerCallback callback;
} Timer;

#define NewTimer(_interval, _callback)  \
  (Timer){                              \
    .interval = _interval,              \
    .last_recorded = 0,                 \
    .callback = _callback               \
  }                                     \

// returns true if an interval has passed, then resets the interval
bool TimeIntervalPassed(Timer *t);
void CheckTimer(Timer *t);

#endif 