#ifndef DETECT_TIMER_H
#define DETECT_TIMER_H
#include <linux/timer.h>
#include <linux/jiffies.h>
#include "detect.h"

extern struct timer_list mytimer;
extern unsigned long pm_use_time;

void init_detect_timer(void);
void my_function(unsigned long data);
void exit_detect_timer(void);

#endif

