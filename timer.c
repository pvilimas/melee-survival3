#include "timer.h"

bool TimeIntervalPassed(Timer *t) {
    float new_time = GetTime();
    if (new_time - t->last_recorded >= t->interval) {
        t->last_recorded = new_time;
        return true;
    }
    return false;
}

void CheckTimer(Timer t) {
    if (TimeIntervalPassed(&t))
        t.callback();
}