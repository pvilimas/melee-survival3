#include "timer.h"

/* returns true if an interval has passed, then resets the interval */
bool TimeIntervalPassed(Timer *t) {
    float new_time = GetTime();
    if (new_time - t->last_recorded >= t->interval) {
        t->last_recorded = new_time;
        return true;
    }
    return false;
}

/* calls the callback if needed */
void CheckTimer(Timer *t) {
    if (TimeIntervalPassed(t)) {
        t->callback(); 
    }
}

void ResetTimer(Timer *t) {
    t->last_recorded = 0;
}