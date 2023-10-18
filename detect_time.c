#include "detect_time.h"

struct timer_list mytimer;
unsigned long pm_use_time = 0;

void init_detect_timer(void)
{
	setup_timer(&mytimer, my_function, 0);
	mytimer.expires =jiffies + HZ;
	add_timer(&mytimer);
}

void my_function(unsigned long data)
{
	mod_timer(&mytimer,jiffies + HZ);
	pm_use_time += 1;
}

void exit_detect_timer(void)
{
	printk("The PM use time is %lu \n", pm_use_time);
	del_timer(&mytimer);
}


