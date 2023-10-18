#ifndef AVERAGE__H
#define AVERAGE__H

#include <linux/types.h>
#include <linux/init.h>
#include <linux/mm.h>
#include <linux/vmalloc.h>
#include <linux/sched.h>

extern atomic64_t count_hand;
extern atomic64_t count_average;

#endif
